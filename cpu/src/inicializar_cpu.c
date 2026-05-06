#include "inicializar_cpu.h"

t_log* loggerCpu;
t_config* configCpu;

char* puertoKernel;
char* IPKernel;
char* puertoMemory;
char* IPMemory;
char* puertoMemoryStick;
char* IPMemoryStick;
char* idCpu;

void inicializar_log_y_config(char* path, char* idDeCpu){
    idCpu = malloc(strlen(idDeCpu) + 1);
    strcpy(idCpu,idDeCpu);
    char configNombre[15];
    strcpy(configNombre, "cpu_");
    strcat(configNombre, idCpu);
    strcat(configNombre, ".log");

    loggerCpu = log_create(configNombre, "cpu.c", true, LOG_LEVEL_INFO);
    configCpu = config_create(path);

    //Meto todo lo del config
    puertoKernel = config_get_string_value(configCpu, "PUERTO_SCHEDULER");
    IPKernel = config_get_string_value(configCpu, "IP_SCHEDULER");
    puertoMemory = config_get_string_value(configCpu, "PUERTO_MEMORY");
    IPMemory = config_get_string_value(configCpu, "IP_MEMORY");
    puertoMemoryStick = config_get_string_value(configCpu, "PUERTO_MEMORYSTICK");
    IPMemoryStick = config_get_string_value(configCpu, "IP_MEMORYSTICK");
}