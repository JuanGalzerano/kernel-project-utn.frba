#include "inicializar.h"

void inicializar_log_y_config(char* path){

    loggerMemory = log_create("memory.log", "memory.c", true, LOG_LEVEL_INFO);
    configMemory = config_create(path);
    
}



