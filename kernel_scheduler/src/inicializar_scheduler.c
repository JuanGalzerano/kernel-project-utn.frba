#include "inicializar_scheduler.h"




void inicializar(char* path){
    inicializar_configs(path);
    
    inicializar_semaforos();

    loggerScheduler = log_create("kernel.log", "main.c", true, LOG_LEVEL_INFO); //acordarse de cambiar el 2do parametro si cambi el nombre del archivo//Ver si va LOG_LEVEL_INFO o hay que usar lo de las config

    inicializar_listas(); 

    pthread_mutex_init(&mutex_socket_memory, NULL);

    
}

void inicializar_configs(char* path){
    configScheduler = config_create(path); // para que funcione en el launch.json en la linea 9 puse los parametros de lanzamiento adecuados para que se arme bien el config
    puertoEscucha= config_get_string_value(configScheduler, "PUERTO_SCHEDULER");
    puertoMemory= config_get_string_value(configScheduler, "PUERTO_MEMORY");
    IPMemory = config_get_string_value(configScheduler, "IP_MEMORY");
    quantum = config_get_int_value(configScheduler, "RR_QUANTUM");
    
    char* algoritmoDePlanificacion = config_get_string_value(configScheduler, "PLANIFICATION_ALGORITHM");
    if(strcmp(algoritmoDePlanificacion, "FIFO")==0) {
        algoritmo = FIFO;}
    else if(strcmp(algoritmoDePlanificacion, "RR")==0){
        algoritmo=RR;
    }
    else if(strcmp(algoritmoDePlanificacion, "CMN")==0){
        algoritmo=CMN;
    }
    free(algoritmoDePlanificacion);
}

void inicializar_semaforos(){
    sem_init(&sem_hay_proceso_ready, 0, 0); // 0 procesos al inicio
    sem_init(&sem_hay_cpu_libre,     0, 0); // 0 CPUs al inicio
    sem_init(&sem_sleep_disponible,          0, 0);
    sem_init(&sem_stdin_disponible,          0, 0);
    sem_init(&sem_stdout_disponible,         0, 0);
    sem_init(&sem_hay_proc_esperando_sleep,  0, 0);
    sem_init(&sem_hay_proc_esperando_stdin,  0, 0);
    sem_init(&sem_hay_proc_esperando_stdout, 0, 0);
}

void inicializar_listas(){
    new_lista    = list_create();
    pthread_mutex_init(&new_mutex, NULL);
    ready_cola   = queue_create();
    pthread_mutex_init(&ready_mutex, NULL);
    block_lista    = list_create();
    pthread_mutex_init(&block_mutex, NULL);
    susp_block   = list_create();
    susp_ready   = list_create();
    exec_lista   = list_create();
    pthread_mutex_init(&exec_mutex,NULL);

    cola_sleep  = queue_create();
    pthread_mutex_init(&mutex_cola_sleep,  NULL);
    cola_stdin  = queue_create();
    pthread_mutex_init(&mutex_cola_stdin,  NULL);
    cola_stdout = queue_create();
    pthread_mutex_init(&mutex_cola_stdout, NULL);
    pthread_mutex_init(&mutex_pid,         NULL);
}