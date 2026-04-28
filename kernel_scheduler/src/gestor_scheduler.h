#ifndef GESTOR_SCHEDULER_H_
#define GESTOR_SCHEDULER_H_



#include <utils/utils.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <semaphore.h>


typedef enum {
    FIFO,
    RR,
    CMN
} t_planification_algorithm;

//VARIABLES GLOBALES

extern t_log* loggerScheduler;
extern t_config* configScheduler;
extern int socketConexionMemory;


//Variables del memory.config

extern char* puertoEscucha;
extern char* puertoMemory;
extern char* IPMemory;
extern t_planification_algorithm algoritmo;
extern int quantum;

//listas de los 7 estados
extern t_list* new_lista;
extern t_queue* ready_cola;
extern pthread_mutex_t ready_mutex;
//extern t_queue** ready_colas_multinivel; implemnetar para el 3er check
extern t_list* block;
extern t_list* susp_block;
extern t_list* susp_ready;
extern t_list* exec_lista; 
extern pthread_mutex_t exec_mutex; 

//SEMAFOROS
extern sem_t sem_hay_proceso_ready;
extern sem_t sem_hay_cpu_libre;




#endif
