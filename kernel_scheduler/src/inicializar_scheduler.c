#include "inicializar_scheduler.h"




void inicializar(char* path){
    inicializar_configs(path);
    
    inicializar_semaforos();

    loggerScheduler = log_create("kernel.log", "kernel_scheduler", true, log_level_from_string(config_get_string_value(configScheduler, "LOG_LEVEL"))); 

    inicializar_listas(); 

    pthread_mutex_init(&mutex_socket_memory, NULL);

    
}

void inicializar_configs(char* path){
    configScheduler = config_create(path); // para que funcione en el launch.json en la linea 9 puse los parametros de lanzamiento adecuados para que se arme bien el config
    puertoEscucha= config_get_string_value(configScheduler, "PUERTO_SCHEDULER");
    puertoMemory= config_get_string_value(configScheduler, "PUERTO_MEMORY");
    puertoMemoryNotif = config_get_string_value(configScheduler, "PUERTO_MEMORY_NOTIF");
    IPMemory = config_get_string_value(configScheduler, "IP_MEMORY");
    quantum = config_get_int_value(configScheduler, "RR_QUANTUM");
    char* hayDesalojo = config_get_string_value(configScheduler, "QUEUE_PREEMPTION");
    hay_desalojo = (strcmp(hayDesalojo, "TRUE") == 0);
    suspensionTimeout = config_get_int_value(configScheduler, "SUSPENSION_TIMEOUT");
    free(hayDesalojo);
    
    char* algoritmoDePlanificacion = config_get_string_value(configScheduler, "PLANIFICATION_ALGORITHM");
    if(strcmp(algoritmoDePlanificacion, "FIFO")==0){
        algoritmo = FIFO;}
    else if(strcmp(algoritmoDePlanificacion, "RR")==0){
        algoritmo=RR;
    }
    else if(strcmp(algoritmoDePlanificacion, "CMN")==0){
        algoritmo=CMN;
    }
    free(algoritmoDePlanificacion);

    if(algoritmo == CMN){
        inicializar_cola_multinivel();
    }
}

void inicializar_semaforos(){
    sem_init(&sem_hay_proceso_ready,0, 0);
    sem_init(&sem_hay_cpu_libre,0, 0);
    sem_init(&sem_desalojo_compactacion_completo,0, 0);
    sem_init(&sem_sleep_disponible, 0, 0);
    sem_init(&sem_stdin_disponible,0, 0);
    sem_init(&sem_stdout_disponible,0, 0);
    sem_init(&sem_hay_proc_esperando_sleep, 0, 0);
    sem_init(&sem_hay_proc_esperando_stdin,0, 0);
    sem_init(&sem_hay_proc_esperando_stdout,0, 0);

    pthread_mutex_init(&mutex_stdout_ocupado, NULL);

    pthread_cond_init(&cond_planificador, NULL);
    pthread_mutex_init(&mutex_planificador, NULL);
}

void inicializar_listas(){
    //new_lista =list_create();
    //pthread_mutex_init(&new_mutex, NULL);
    ready_cola= queue_create();
    pthread_mutex_init(&ready_mutex, NULL);
    block_lista = list_create();
    pthread_mutex_init(&block_mutex, NULL);
    susp_block = list_create();
    pthread_mutex_init(&mutex_susp_block,NULL);
    susp_ready = list_create();
    pthread_mutex_init(&mutex_susp_ready,NULL);
    exec_lista =list_create();
    pthread_mutex_init(&exec_mutex,NULL);
    pthread_mutex_init(&mutex_cola_multinivel, NULL);

    cola_sleep= queue_create();
    pthread_mutex_init(&mutex_cola_sleep,NULL);
    cola_stdin = queue_create();
    pthread_mutex_init(&mutex_cola_stdin,NULL);
    cola_stdout = queue_create();
    pthread_mutex_init(&mutex_cola_stdout, NULL);
    pthread_mutex_init(&mutex_pid, NULL);

    lista_mutex = list_create();
    pthread_mutex_init(&mutex_lista_mutex,NULL);
}

void inicializar_cola_multinivel(){

    //se inicializa el array algoritmo_por_cola
    char** algoritmosArray = config_get_array_value(configScheduler, "QUEUES_ALGORITHMS");

    cantidad_colas = 0;
    while(algoritmosArray[cantidad_colas] != NULL) cantidad_colas++;

    // guardar en un array de enums
    algoritmo_por_cola = malloc(cantidad_colas * sizeof(t_algortimo_planificacion));

    for(int i = 0; i < cantidad_colas; i++){
        if(strcmp(algoritmosArray[i], "FIFO") == 0)
            algoritmo_por_cola[i] = FIFO;
        else if(strcmp(algoritmosArray[i], "RR") == 0)
            algoritmo_por_cola[i] = RR;
        free(algoritmosArray[i]);
    }
    free(algoritmosArray);

    //inicializar cola_multinivel
    cola_multinivel = malloc(sizeof(t_queue*) * cantidad_colas);
    for(int i = 0; i < cantidad_colas; i++){
        cola_multinivel[i] = queue_create();
    }
}