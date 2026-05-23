#include "inicializar.h"

void inicializar_log_y_config(char* path) {
    loggerMemory = log_create("memory.log", "memory.c", true, LOG_LEVEL_INFO);
    configMemory = config_create(path);
}

int inicializar_proceso(uint32_t pid, char* path) {
    char* full_path = malloc(strlen(scriptsBasePath) + strlen(path) + 2);
    sprintf(full_path, "%s/%s", scriptsBasePath, path);

    FILE* f = fopen(full_path, "r");
    if (!f) {
        log_error(loggerMemory, "No se pudo abrir el archivo: %s", full_path);
        free(full_path);
        return 0;
    }
    fclose(f);

    t_proceso_memory* proceso = malloc(sizeof(t_proceso_memory));
    proceso->pid = pid;
    proceso->path_pseudocodigo = full_path;
    proceso->contexto = malloc(sizeof(t_contexto_ejecucion));
    memset(proceso->contexto, 0, sizeof(t_contexto_ejecucion));
    proceso->contexto->tabla_segmentos = list_create();

    pthread_mutex_lock(&procesos_mutex);
    list_add(lista_procesos, proceso);
    pthread_mutex_unlock(&procesos_mutex);

    log_info(loggerMemory, "## PID: %d - Proceso Creado", pid);
    return 1;
}

t_proceso_memory* buscar_proceso(uint32_t pid) {
    pthread_mutex_lock(&procesos_mutex);
    t_proceso_memory* resultado = NULL;
    for (int i = 0; i < list_size(lista_procesos); i++) {
        t_proceso_memory* p = list_get(lista_procesos, i);
        if (p->pid == pid) { resultado = p; break; }
    }
    pthread_mutex_unlock(&procesos_mutex);
    return resultado;
}

char* leer_instruccion(t_proceso_memory* proceso, uint32_t pc) {
    FILE* f = fopen(proceso->path_pseudocodigo, "r");
    if (!f) {
        log_error(loggerMemory, "PID %d: no se pudo abrir %s", proceso->pid, proceso->path_pseudocodigo);
        return NULL;
    }

    char* linea = NULL;
    size_t   capacidad   = 0;
    uint32_t linea_actual = 0;

    while (getline(&linea, &capacidad, f) != -1) {
        if (linea_actual == pc) {
            fclose(f);
            linea[strcspn(linea, "\n")] = '\0';
            return linea;
        }
        linea_actual++;
    }

    fclose(f);
    free(linea);
    log_error(loggerMemory, "PID %d: PC %d fuera de rango", proceso->pid, pc);
    return NULL;
}

// Memory Sticks 

void agregar_memory_stick(int socket, uint32_t size) {
    t_memory_stick_info* stick = malloc(sizeof(t_memory_stick_info));
    stick->socket = socket;
    stick->size   = size;

    pthread_mutex_lock(&memoria_mutex);

    stick->base_acumulada = memoria_total_size; // se pega al final del espacio ya existente

    list_add(lista_memory_sticks, stick);

    memoria_total_size += size;

    pthread_mutex_unlock(&memoria_mutex);

    // TODO: notificar al Kernel Scheduler que hay más memoria disponible.
    // Requiere un nuevo op_code NUEVA_MEMORIA en utils y que socketScheduler esté listo.
}

t_memory_stick_info* encontrar_stick_por_dir_fisica(uint32_t dir_fisica) {
    for (int i = 0; i < list_size(lista_memory_sticks); i++) {
        t_memory_stick_info* stick = list_get(lista_memory_sticks, i);
        if (dir_fisica >= stick->base_acumulada && dir_fisica < stick->base_acumulada + stick->size)
            return stick;
    }
    return NULL;
}

int leer_de_memory_stick(uint32_t dir_fisica, uint32_t tamanio, void* buffer_out) {
    pthread_mutex_lock(&memoria_mutex);
    t_memory_stick_info* stick = encontrar_stick_por_dir_fisica(dir_fisica);
    pthread_mutex_unlock(&memoria_mutex);

    if (!stick) {
        log_error(loggerMemory, "No se encontró Memory Stick para dir_fisica %d", dir_fisica);
        return -1;
    }

    // La dirección física dentro del stick empieza en 0, así que hay que restarle su base global
    uint32_t dir_en_stick = dir_fisica - stick->base_acumulada;

/*

    Estructura para poderle mandar un mensaje al stick para que lea la memoria, en la direccion fisica que le pasaria
    Mas que estructura, una funcion y ya ta

*/

    memset(buffer_out, 0, tamanio);
    return 1;
}

int escribir_en_memory_stick(uint32_t dir_fisica, uint32_t tamanio, void* datos) {
    pthread_mutex_lock(&memoria_mutex);
    t_memory_stick_info* stick = encontrar_stick_por_dir_fisica(dir_fisica);
    pthread_mutex_unlock(&memoria_mutex);

    if (!stick) {
        log_error(loggerMemory, "No se encontró Memory Stick para dir_fisica %d", dir_fisica);
        return -1;
    }

    uint32_t dir_en_stick = dir_fisica - stick->base_acumulada;

/*

    Estructura para poderle mandar un mensaje al stick para que escriba la memoria, en la direccion fisica que le pasaria
    Mas que estructura, una funcion y ya ta

*/

    return 1;
}

// Gestión de huecos (lo pongo en static porque solo se usa dentro de este .c) 

static t_hueco* encontrar_hueco_best_fit(uint32_t tamaño) {
    t_hueco* mejor = NULL;
    for (int i = 0; i < list_size(lista_huecos); i++) {
        t_hueco* h = list_get(lista_huecos, i);
        if (h->limite >= tamaño && (!mejor || h->limite < mejor->limite))
            mejor = h;
    }
    return mejor;
}

static t_hueco* encontrar_hueco_worst_fit(uint32_t tamaño) {
    t_hueco* peor = NULL;
    for (int i = 0; i < list_size(lista_huecos); i++) {
        t_hueco* h = list_get(lista_huecos, i);
        if (h->limite >= tamaño && (!peor || h->limite > peor->limite))
            peor = h;
    }
    return peor;
}

//det estrategia para encontrar un hueco
static t_hueco* encontrar_hueco(uint32_t tamaño) {
    if (strcmp(allocation_strategy, "BEST") == 0)
        return encontrar_hueco_best_fit(tamaño);
    return encontrar_hueco_worst_fit(tamaño);
}

// Inserta un hueco en lista_huecos (ya ordenada por base) y fusiona con adyacentes.
// usar memoria_mutex para usar funcion.
static void insertar_hueco_y_fusionar(t_hueco* nuevo) {
    //Mete hueco en el medio patenado para delante la posicion del que tiene por primera vez una vez mayor al nuevo
    int pos = list_size(lista_huecos);
    for (int i = 0; i < list_size(lista_huecos); i++) {
        t_hueco* h = list_get(lista_huecos, i);
        if (h->base > nuevo->base) { pos = i; break; }
    }
    list_add_in_index(lista_huecos, pos, nuevo);

    // Intenta fusionar con el siguiente si es adyacente
    if (pos + 1 < list_size(lista_huecos)) {
        t_hueco* sig = list_get(lista_huecos, pos + 1);
        if (nuevo->base + nuevo->limite == sig->base) {
            nuevo->limite += sig->limite;
            list_remove_and_destroy_element(lista_huecos, pos + 1, free);
        }
    }

    // Intenta fusionar con el anterior si es adyacente
    if (pos > 0) {
        t_hueco* ant = list_get(lista_huecos, pos - 1);
        if (ant->base + ant->limite == nuevo->base) {
            ant->limite += nuevo->limite;
            list_remove_and_destroy_element(lista_huecos, pos, free);
        }
    }
}

//Finalización de procesos

void finalizar_proceso(uint32_t pid) {
    pthread_mutex_lock(&procesos_mutex);
    t_proceso_memory* proceso = NULL;
    for (int i = 0; i < list_size(lista_procesos); i++) {
        t_proceso_memory* p = list_get(lista_procesos, i);
        if (p->pid == pid) { proceso = p; break; }
    }
    if (!proceso) {
        pthread_mutex_unlock(&procesos_mutex);
        log_warning(loggerMemory, "FIN_PROCESO: PID %d no encontrado", pid);
        return;
    }
    list_remove_element(lista_procesos, proceso);
    pthread_mutex_unlock(&procesos_mutex);

    // Devolver todos los segmentos como huecos libres
    pthread_mutex_lock(&memoria_mutex);
    while (list_size(proceso->contexto->tabla_segmentos) > 0) {
        t_segmento* seg = list_remove(proceso->contexto->tabla_segmentos, 0);
        t_hueco*    hueco = malloc(sizeof(t_hueco));
        hueco->base = seg->base;
        hueco->limite = seg->limite;
        free(seg);
        insertar_hueco_y_fusionar(hueco);
    }
    pthread_mutex_unlock(&memoria_mutex);

    list_destroy(proceso->contexto->tabla_segmentos);
    free(proceso->contexto);
    free(proceso->path_pseudocodigo);
    free(proceso);

    log_info(loggerMemory, "## PID: %d - Proceso Finalizado", pid);
}

//Gestión de segmentos

t_segmento* buscar_segmento(t_proceso_memory* proceso, uint32_t id_segmento) {
    for (int i = 0; i < list_size(proceso->contexto->tabla_segmentos); i++) {
        t_segmento* seg = list_get(proceso->contexto->tabla_segmentos, i);
        if (seg->id_segmento == id_segmento) return seg;
    }
    return NULL;
}

// Retorna: 1=ok, 0=no hay hueco contiguo (ncesitaria compactar para buscar de nuevo un hueco), -1=error
int crear_segmento(uint32_t pid, uint32_t id_segmento, uint32_t tamaño) {
    t_proceso_memory* proceso = buscar_proceso(pid);
    if (!proceso) {
        log_error(loggerMemory, "PID %d no encontrado al crear segmento %d", pid, id_segmento);
        return -1;
    }
    if (tamaño > segment_max_size) {
        log_error(loggerMemory, "PID %d: segmento %d excede tamaño máximo (%d > %d)", pid, id_segmento, tamaño, segment_max_size);
        return -1;
    }

    pthread_mutex_lock(&memoria_mutex);

    t_hueco* hueco = encontrar_hueco(tamaño);
    if (!hueco) {
        pthread_mutex_unlock(&memoria_mutex);
        return 0; //HAY que compactar supuestamente entonces si tengo que buscar un nuevo hueco post-compactacion
    }

    t_segmento* seg  = malloc(sizeof(t_segmento));
    seg->id_segmento = id_segmento;
    seg->base  = hueco->base;
    seg->limite = tamaño;
    list_add(proceso->contexto->tabla_segmentos, seg);

    // Recortar el hueco desde el principio
    hueco->base   += tamaño;
    hueco->limite -= tamaño;
    if (hueco->limite == 0) {
        list_remove_element(lista_huecos, hueco);
        free(hueco);
    }

    pthread_mutex_unlock(&memoria_mutex);

    log_info(loggerMemory, "## PID: %d - Segmento Creado %d - Tamaño: %d", pid, id_segmento, tamaño);
    return 1;
}

// Retorna: 1=ok, -1=error (proceso o segmento no encontrado)
int eliminar_segmento(uint32_t pid, uint32_t id_segmento) {
    t_proceso_memory* proceso = buscar_proceso(pid);
    if (!proceso) return -1;

    pthread_mutex_lock(&memoria_mutex);

    t_segmento* seg = buscar_segmento(proceso, id_segmento);
    if (!seg) {
        pthread_mutex_unlock(&memoria_mutex);
        return -1;
    }

    t_hueco* hueco_liberado  = malloc(sizeof(t_hueco));
    hueco_liberado->base  = seg->base;
    hueco_liberado->limite = seg->limite;

    list_remove_element(proceso->contexto->tabla_segmentos, seg);
    free(seg);

    // Reintegrar el espacio al mapa de huecos (con fusión de adyacentes)
    insertar_hueco_y_fusionar(hueco_liberado);

    pthread_mutex_unlock(&memoria_mutex);
    return 1;
}


int traducir_y_verificar(uint32_t pid, uint32_t dir_logica, uint32_t tamanio, uint32_t* dir_fisica_out) {
    uint32_t id_segmento = dir_logica / segment_max_size;
    uint32_t offset      = dir_logica % segment_max_size;

    t_proceso_memory* proceso = buscar_proceso(pid);
    if (!proceso) return -1;

    t_segmento* seg = buscar_segmento(proceso, id_segmento);
    if (!seg) return -1;

    if (offset + tamanio > seg->limite) return -1; 

    *dir_fisica_out = seg->base + offset;//aca modifica el valor del parametro
    return 1;//indica exito en la traduccion
}
