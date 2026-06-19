#include "inicializar.h"
#include "segmentacion.h"

void inicializar_log_y_config(char* path) {
    loggerMemory = log_create("memory.log", "memory.c", true, LOG_LEVEL_INFO);
    configMemory = config_create(path);
}

int inicializar_proceso(uint32_t pid, char* path) {
    char* full_path = malloc(strlen(scriptsBasePath) + strlen(path) + 2);
    sprintf(full_path, "%s/%s", scriptsBasePath, path);

    FILE* f = fopen(full_path, "r");
    if(!f){
        log_error(loggerMemory, "No se pudo abrir el archivo: %s", full_path);
        free(full_path);
        return 0;
    }
    fclose(f);

    t_proceso_memory* proceso = malloc(sizeof(t_proceso_memory));
    proceso->pid = pid;
    proceso->path_pseudocodigo = full_path;
    proceso->en_swap =false;
    proceso->contexto = malloc(sizeof(t_contexto_ejecucion));
    memset(proceso->contexto, 0, sizeof(t_contexto_ejecucion));
    proceso->contexto->tabla_segmentos = list_create();

    pthread_mutex_lock(&procesos_mutex);
    list_add(lista_procesos, proceso);
    pthread_mutex_unlock(&procesos_mutex);

    log_info(loggerMemory, "## PID: %d - Proceso Creado", pid);
    return 1;
}

t_proceso_memory* buscar_proceso(uint32_t pid){
    pthread_mutex_lock(&procesos_mutex);
    t_proceso_memory* resultado =buscar_proceso_sin_lock(pid);
    pthread_mutex_unlock(&procesos_mutex);
    return resultado;
}

t_proceso_memory* buscar_proceso_sin_lock(uint32_t pid){
    for(int i = 0; i < list_size(lista_procesos); i++){
        t_proceso_memory* p = list_get(lista_procesos, i);
        if(p->pid == pid){
            return p;
        }
    }
    return NULL;
}

char* leer_instruccion(t_proceso_memory* proceso, uint32_t pc){
    FILE* f = fopen(proceso->path_pseudocodigo, "r");
    if (!f) {
        log_error(loggerMemory, "PID %d: no se pudo abrir %s", proceso->pid, proceso->path_pseudocodigo);
        return NULL;
    }

    char* linea = NULL;
    size_t capacidad = 0;
    uint32_t linea_actual = 0;

    while(getline(&linea, &capacidad, f) != -1){
        if (linea_actual == pc){
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

void finalizar_proceso(uint32_t pid){
    pthread_mutex_lock(&procesos_mutex);
    t_proceso_memory* proceso = NULL;
    for (int i = 0; i < list_size(lista_procesos); i++) {
        t_proceso_memory* p = list_get(lista_procesos, i);
        if(p->pid == pid){ proceso = p; break;}
    }
    if(!proceso){
        pthread_mutex_unlock(&procesos_mutex);
        log_warning(loggerMemory, "FIN_PROCESO: PID %d no encontrado", pid);
        return;
    }
    list_remove_element(lista_procesos, proceso);

    if(!proceso->en_swap){
        pthread_mutex_lock(&memoria_mutex);
        while(list_size(proceso->contexto->tabla_segmentos) > 0){
            t_segmento* seg = list_remove(proceso->contexto->tabla_segmentos, 0);
            t_hueco* hueco = malloc(sizeof(t_hueco));
            hueco->base = seg->base;
            hueco->limite = seg->limite;
            memoria_libre_size +=seg->limite;
            free(seg);
            insertar_hueco_y_fusionar(hueco);
        }
        pthread_mutex_unlock(&memoria_mutex);
    }else{
        while(list_size(proceso->contexto->tabla_segmentos)>0){
            t_segmento* seg = list_remove(proceso->contexto->tabla_segmentos,0);
            free(seg);
        }
    }

    list_destroy(proceso->contexto->tabla_segmentos);
    free(proceso->contexto);
    free(proceso->path_pseudocodigo);
    free(proceso);
    pthread_mutex_unlock(&procesos_mutex);

    log_info(loggerMemory, "PID: %d - Proceso Finalizado", pid);
}