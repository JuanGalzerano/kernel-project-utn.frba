#include "inicializar.h"

void inicializar_log_y_config(char* path){

    loggerCpu = log_create("cpu.log", "main.c", true, LOG_LEVEL_INFO);
    configCpu = config_create(path);
}