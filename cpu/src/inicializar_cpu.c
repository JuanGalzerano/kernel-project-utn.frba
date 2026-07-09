#include "inicializar_cpu.h"  

t_log* loggerCpu;
t_config* configCpu;

char* puertoKernel;
char* IPKernel;
char* puertoMemory;
char* IPMemory;
char* idCpu;
char* puertoKernelMemoryNotificaciones;
char* puertoMemoryStick;
char* IPMemoryStick;

void inicializar_log_y_config(char* path, char* idDeCpu) {
    idCpu = malloc(strlen(idDeCpu) + 1);
    strcpy(idCpu, idDeCpu);

    configCpu = config_create(path);
    if (configCpu == NULL) {
        fprintf(stderr, "CPU: No se pudo cargar el archivo de configuracion: %s", path);
        abort();
    }

    char configNombre[64];
    snprintf(configNombre, sizeof(configNombre), "cpu_%s.log", idCpu); //Esta funcion escribe al string

    loggerCpu = log_create(configNombre, "cpu.c", true, log_level_from_string(config_get_string_value(configCpu, "LOG_LEVEL")));
    if (loggerCpu == NULL) {
        fprintf(stderr, "CPU: No se pudo crear el logger");
        abort();
    }

    puertoKernel = config_get_string_value(configCpu, "PUERTO_SCHEDULER");
    IPKernel = config_get_string_value(configCpu, "IP_SCHEDULER");
    puertoMemory = config_get_string_value(configCpu, "PUERTO_MEMORY");
    IPMemory = config_get_string_value(configCpu, "IP_MEMORY");
    puertoKernelMemoryNotificaciones = config_get_string_value(configCpu, "PUERTO_KERNEL_MEMORY_AVISOS");
    puertoMemoryStick = config_get_string_value(configCpu, "PUERTO_MEMORYSTICK");
    IPMemoryStick = config_get_string_value(configCpu, "IP_MEMORY_STICK");
    config_destroy(configCpu);
}