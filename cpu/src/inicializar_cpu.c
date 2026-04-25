#include "inicializar_cpu.h"

char* puertoKernel;
char* IPKernel;
char* puertoMemory;
char* IPMemory;
char* puertoMemoryStick;
char* IPMemoryStick;
char* idCpu;
char* archivoLogNombre;

void inicializar_log_y_config(char* path, char* idCpu){
    idCpu = idCpu;
    archivoLogNombre = malloc(strlen("cpu_") + strlen(idCpu) + strlen(".log") + 1);
    sprintf(archivoLogNombre, "cpu_%s.log", idCpu);

    loggerCpu = log_create(archivoLogNombre, "cpu.c", true, LOG_LEVEL_INFO);
    configCpu = config_create(path);

    //Meto todo lo del config
    puertoKernel = config_get_string_value(configCpu, "PUERTO_SCHEDULER");
    IPKernel = config_get_string_value(configCpu, "IP_SCHEDULER");
    puertoMemory = config_get_string_value(configCpu, "PUERTO_MEMORY");
    IPMemory = config_get_string_value(configCpu, "IP_MEMORY");
    puertoMemoryStick = config_get_string_value(configCpu, "PUERTO_MEMORYSTICK");
    IPMemoryStick = config_get_string_value(configCpu, "IP_MEMORYSTICK");
}

void liberar_log(t_log* loggerCpu) {
    log_destroy(loggerCpu);
    free(archivoLogNombre);
}