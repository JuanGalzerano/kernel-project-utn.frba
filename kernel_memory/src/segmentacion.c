#include "segmentacion.h"
#include "inicializar.h"

//HUECOS

t_hueco* encontrar_hueco_best_fit(uint32_t tamanio) {
    t_hueco* mejor = NULL;
    for (int i = 0; i < list_size(lista_huecos); i++) {
        t_hueco* h = list_get(lista_huecos, i);
        if (h->limite >= tamanio && (!mejor || h->limite < mejor->limite))
            mejor = h;
    }
    return mejor;
}

t_hueco* encontrar_hueco_worst_fit(uint32_t tamanio) {
    t_hueco* peor = NULL;
    for (int i = 0; i < list_size(lista_huecos); i++) {
        t_hueco* h = list_get(lista_huecos, i);
        if (h->limite >= tamanio && (!peor || h->limite > peor->limite))
            peor = h;
    }
    return peor;
}

t_hueco* encontrar_hueco(uint32_t tamanio){
    if (strcmp(allocation_strategy, "BEST") == 0)
        return encontrar_hueco_best_fit(tamanio);
    return encontrar_hueco_worst_fit(tamanio);
}

// Inserta un hueco en lista_huecos (ordenada por base) y fusiona con adyacentes.
// Llamar con memoria_mutex tomado.
void insertar_hueco_y_fusionar(t_hueco* nuevo){
    int pos = list_size(lista_huecos);
    for(int i = 0; i < list_size(lista_huecos); i++){
        t_hueco* h = list_get(lista_huecos, i);
        if(h->base > nuevo->base){ pos = i; break;}
    }
    list_add_in_index(lista_huecos, pos, nuevo);

    if(pos + 1 < list_size(lista_huecos)){
        t_hueco* sig = list_get(lista_huecos, pos + 1);
        if(nuevo->base + nuevo->limite == sig->base){
            nuevo->limite += sig->limite;
            list_remove_and_destroy_element(lista_huecos, pos + 1, free);
        }
    }

    if(pos > 0){
        t_hueco* ant = list_get(lista_huecos, pos - 1);
        if(ant->base + ant->limite == nuevo->base){
            ant->limite += nuevo->limite;
            list_remove_and_destroy_element(lista_huecos, pos, free);
        }
    }
}

t_segmento* buscar_segmento_proceso(t_proceso_memory* proceso, uint32_t id_segmento){
    for(int i = 0; i < list_size(proceso->contexto->tabla_segmentos); i++) {
        t_segmento* seg = list_get(proceso->contexto->tabla_segmentos, i);
        if (seg->id_segmento == id_segmento)return seg;
    }
    return NULL;
}

//ELIMINAR Y CREAR SEGMENTO

op_code crear_segmento(uint32_t pid, uint32_t id_segmento, uint32_t tamanio) {
    t_proceso_memory* proceso = buscar_proceso(pid);
    if (!proceso) {
        log_error(loggerMemory, "PID %d no encontrado al crear segmento %d", pid, id_segmento);
        return MEMORIA_NO_DISPONIBLE;
    }
    if (tamanio > segment_max_size) {
        log_error(loggerMemory, "PID %d: segmento %d excede tamanio maximo (%d > %d)", pid, id_segmento, tamanio, segment_max_size);
        return MEMORIA_NO_DISPONIBLE;
    }

    pthread_mutex_lock(&memoria_mutex);

    t_hueco* hueco = encontrar_hueco(tamanio);

    if (hueco != NULL) {
        t_segmento* seg  = malloc(sizeof(t_segmento));
        seg->id_segmento = id_segmento;
        seg->base = hueco->base;
        seg->limite = tamanio;
        list_add(proceso->contexto->tabla_segmentos, seg);

        hueco->base   += tamanio;
        hueco->limite -= tamanio;
        if (hueco->limite == 0) {
            list_remove_element(lista_huecos, hueco);
            free(hueco);
        }
        memoria_libre_size -= tamanio;

        pthread_mutex_unlock(&memoria_mutex);
        log_info(loggerMemory, "## PID: %d - Segmento Creado %d - Tamanio: %d", pid, id_segmento, tamanio);
        return MEMORIA_DISPONIBLE;
    }

    if (memoria_libre_size >= tamanio) {
        pthread_mutex_unlock(&memoria_mutex);
        log_info(loggerMemory, "PID: %d - Segmento %d - Memoria suficiente pero no contigua, requiere compactacion", pid, id_segmento);
        return COMPACTACION;
    }

    pthread_mutex_unlock(&memoria_mutex);
    log_info(loggerMemory, "PID: %d - Segmento %d - Memoria insuficiente (libre: %d, pedido: %d)", pid, id_segmento, memoria_libre_size, tamanio);
    return MEMORIA_NO_DISPONIBLE;
}

int eliminar_segmento(uint32_t pid, uint32_t id_segmento) {
    t_proceso_memory* proceso = buscar_proceso(pid);
    if (!proceso) return -1;

    pthread_mutex_lock(&memoria_mutex);

    t_segmento* seg = buscar_segmento_proceso(proceso, id_segmento);
    if (!seg) {
        pthread_mutex_unlock(&memoria_mutex);
        return -1;
    }

    t_hueco* hueco_liberado = malloc(sizeof(t_hueco));
    hueco_liberado->base = seg->base;
    hueco_liberado->limite = seg->limite;

    memoria_libre_size += seg->limite;

    list_remove_element(proceso->contexto->tabla_segmentos, seg);
    free(seg);

    insertar_hueco_y_fusionar(hueco_liberado);

    pthread_mutex_unlock(&memoria_mutex);
    return 1;
}

//PARA SUSPENSION Y DESUSPENSION


// Libera la memoria fisica del segmento sin eliminarlo de la tabla.
// Usado al suspender un proceso: el segmento sigue existiendo logicamente.
void liberar_fisica_segmento(uint32_t pid, uint32_t seg_id) {
    t_proceso_memory* proceso = buscar_proceso(pid);
    if (!proceso) return;

    pthread_mutex_lock(&memoria_mutex);
    t_segmento* seg = buscar_segmento_proceso(proceso, seg_id);
    if (!seg) { pthread_mutex_unlock(&memoria_mutex); return; }

    t_hueco* hueco = malloc(sizeof(t_hueco));
    hueco->base = seg->base;
    hueco->limite = seg->limite;
    memoria_libre_size += seg->limite;
    insertar_hueco_y_fusionar(hueco);

    pthread_mutex_unlock(&memoria_mutex);
}

// Asigna memoria fisica a un segmento que ya existe en la tabla (base desactualizada).
// Usado al desuspender: encuentra un hueco y actualiza seg->base sin agregar nada a la tabla.
op_code asignar_fisica_segmento(uint32_t pid, uint32_t seg_id, uint32_t tamanio) {
    t_proceso_memory* proceso = buscar_proceso(pid);
    if(!proceso) return MEMORIA_NO_DISPONIBLE;

    pthread_mutex_lock(&memoria_mutex);
    t_segmento* seg = buscar_segmento_proceso(proceso, seg_id);
    if(!seg){ pthread_mutex_unlock(&memoria_mutex); return MEMORIA_NO_DISPONIBLE; }

    t_hueco* hueco = encontrar_hueco(tamanio);
    if(!hueco){
        op_code ret = (memoria_libre_size >= tamanio) ? COMPACTACION : MEMORIA_NO_DISPONIBLE;
        pthread_mutex_unlock(&memoria_mutex);
        return ret;
    }

    seg->base = hueco->base;
    hueco->base += tamanio;
    hueco->limite -= tamanio;
    if(hueco->limite == 0){
        list_remove_element(lista_huecos, hueco);
        free(hueco);
    }
    memoria_libre_size -= tamanio;

    pthread_mutex_unlock(&memoria_mutex);
    return MEMORIA_DISPONIBLE;
}
