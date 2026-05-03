#ifndef GESTOR_SCHEDULER_H_
#define GESTOR_SCHEDULER_H_



#include <utils/utils.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <semaphore.h>



//TYPESDEFS
typedef struct {
    uint32_t pid;
    uint32_t prioridad;//esto va para CMN
    //despues se van a agregar cosas de mutex creo
} t_pcb;

typedef struct {
    int cpu_id;
    int socketConexion;
    t_pcb* pcb; // NULL si la CPU está libre
} t_cpu_exec; 

typedef enum {
    FIFO,
    RR,
    CMN
} t_planification_algorithm;

//VARIABLES GLOBALES

extern t_log* loggerScheduler;
extern t_config* configScheduler;
extern int socketConexionMemory;
extern uint32_t proximo_pid;
extern pthread_mutex_t mutex_pid;
extern pthread_mutex_t mutex_socket_memory;


//Variables del memory.config

extern char* puertoEscucha;
extern char* puertoMemory;
extern char* IPMemory;
extern t_planification_algorithm algoritmo;
extern int quantum;

//listas de los 7 estados
extern t_list* new_lista;
extern pthread_mutex_t new_mutex;
extern t_queue* ready_cola;
extern pthread_mutex_t ready_mutex;
//extern t_queue** ready_colas_multinivel; implemnetar para el 3er check
extern t_list* block_lista;
extern pthread_mutex_t block_mutex;

extern t_list* susp_block;
extern t_list* susp_ready;
extern t_list* exec_lista; 
extern pthread_mutex_t exec_mutex; 

//SEMAFOROS
extern sem_t sem_hay_proceso_ready;
extern sem_t sem_hay_cpu_libre;

//sockets de las IOs

extern int socketSleep;
extern int socketStdin;
extern int socketStdout;


// colas de IOs
extern t_queue* cola_sleep;
extern t_queue* cola_stdin;
extern t_queue* cola_stdout;

extern pthread_mutex_t mutex_cola_sleep;
extern pthread_mutex_t mutex_cola_stdin;
extern pthread_mutex_t mutex_cola_stdout;

// semáforos de disponibilidad de IO 
extern sem_t sem_sleep_disponible;
extern sem_t sem_stdin_disponible;
extern sem_t sem_stdout_disponible;
extern sem_t sem_hay_proc_esperando_sleep;
extern sem_t sem_hay_proc_esperando_stdin;
extern sem_t sem_hay_proc_esperando_stdout;




#endif
