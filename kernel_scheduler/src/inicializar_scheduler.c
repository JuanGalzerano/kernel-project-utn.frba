#include "inicializar_scheduler.h"




void inicializar(char* path){
    configScheduler = config_create(path); // para que funcione en el launch.json en la linea 9 puse los parametros de lanzamiento adecuados para que se arme bien el config
    puertoEscucha= config_get_string_value(configScheduler, "PUERTO_SCHEDULER");
    puertoMemory= config_get_string_value(configScheduler, "PUERTO_MEMORY");
    IPMemory = config_get_string_value(configScheduler, "IP_MEMORY");
    


    loggerScheduler = log_create("kernel.log", "main.c", true, LOG_LEVEL_INFO); //acordarse de cambiar el 2do parametro si cambi el nombre del archivo//Ver si va LOG_LEVEL_INFO o hay que usar lo de las config

    new_lista    = list_create();
    ready_cola   = queue_create();
    block        = list_create();
    susp_block   = list_create();
    susp_ready   = list_create();
    exec_lista   = list_create();
}