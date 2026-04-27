#include "inicializar_scheduler.h"




void inicializar(char* path){// desp separar en dif tipos de inicializar (config, semaforos, logger, etc..)
    configScheduler = config_create(path); // para que funcione en el launch.json en la linea 9 puse los parametros de lanzamiento adecuados para que se arme bien el config
    puertoEscucha= config_get_string_value(configScheduler, "PUERTO_SCHEDULER");
    puertoMemory= config_get_string_value(configScheduler, "PUERTO_MEMORY");
    IPMemory = config_get_string_value(configScheduler, "IP_MEMORY");
    
    char* algoritmoDePlanificacion = config_get_string_value(configScheduler, "PLANIFICATION_ALGORITHM");
    if(strcmp(algoritmoDePlanificacion, "FIFO")==0) {
        algoritmo = FIFO;}
    else if(strcmp(algoritmoDePlanificacion, "RR")==0){
        algoritmo=RR;
    }
    else if(strcmp(algoritmoDePlanificacion, "CMN")==0){
        algoritmo=CMN;
    }


    sem_init(&sem_hay_proceso_ready, 0, 0); // 0 procesos al inicio
    sem_init(&sem_hay_cpu_libre,     0, 0); // 0 CPUs al inicio
    


    loggerScheduler = log_create("kernel.log", "main.c", true, LOG_LEVEL_INFO); //acordarse de cambiar el 2do parametro si cambi el nombre del archivo//Ver si va LOG_LEVEL_INFO o hay que usar lo de las config

    new_lista    = list_create();
    ready_cola   = queue_create();
    pthread_mutex_init(&ready_mutex, NULL);
    block        = list_create();
    susp_block   = list_create();
    susp_ready   = list_create();
    exec_lista   = list_create();
    pthread_mutex_init(&exec_mutex,NULL);
}