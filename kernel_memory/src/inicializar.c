#include "inicializar.h"

void inicializar_log_y_config(char* path){

    loggerMemory = log_create("memory.log", "memory.c", true, LOG_LEVEL_INFO);
    configMemory = config_create(path);

}

int inicializar_proceso(uint32_t pid, char* path) {
    // Construir path completo: SCRIPTS_BASEPATH + "/" + path_relativo
    // +2 por el '/' separador y el '\0' terminador de string
    char* full_path = malloc(strlen(scriptsBasePath) + strlen(path) + 2);
    sprintf(full_path, "%s/%s", scriptsBasePath, path);

    // Verificar que el archivo existe y se puede leer
    FILE* f = fopen(full_path, "r");
    if (!f) {
        log_error(loggerMemory, "No se pudo abrir el archivo: %s", full_path);
        free(full_path);
        return 0;
    }
    fclose(f);

    // Crear la estructura del proceso
    t_proceso_memory* proceso = malloc(sizeof(t_proceso_memory));
    proceso->pid = pid;
    proceso->path_pseudocodigo = full_path;

    // Inicializar el contexto de ejecucion con TODOS los registros en 0
    proceso->contexto = malloc(sizeof(t_contexto_ejecucion));
    memset(proceso->contexto, 0, sizeof(t_contexto_ejecucion));

    list_add(lista_procesos, proceso);

    log_info(loggerMemory, "## PID: %d - Proceso Creado", pid);
    return 1;
}

t_proceso_memory* buscar_proceso(uint32_t pid) {
    for (int i = 0; i < list_size(lista_procesos); i++) {
        t_proceso_memory* p = list_get(lista_procesos, i);
        if (p->pid == pid) return p;
    }
    return NULL;
}
