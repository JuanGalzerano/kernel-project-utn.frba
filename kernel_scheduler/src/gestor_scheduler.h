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
    uint32_t prioridadOriginal;
    //despues tengo que agregar cosas de mutex creo
} t_pcb;

typedef struct {
    int cpu_id;
    int socketConexion;
    t_pcb* pcb; // NULL si la CPU está libre
    //estos de abajo solo los uso cuando hay desalojo habiliatdo los necesito pq asi lo logueo cuando me devuelven el pid y motivo y no cuando todavia realmente la cpu no lo desalojoo
    uint32_t pid_desalojador;       
    int prioridad_desalojador; 
} t_cpu; 

typedef enum {
    FIFO,
    RR,
    CMN
} t_algortimo_planificacion;

typedef struct {
    t_cpu* cpu;
    uint32_t pid_original;
} t_parametros_hilo_quantum; //eate  atruct lo usamos para que antes de correr el hilo timer de quantum, podamos guardar el pid, evitando que el de la CPU se ponga en NULL 
//xq el proceso termino de ejecutar antes y ya dejo en NULL cpu->pcb 

//VARIABLES GLOBALES

extern t_log* loggerScheduler;
extern t_config* configScheduler;
extern int socketConexionMemory;
extern int socketMemoryNotif;
extern uint32_t proximo_pid;
extern pthread_mutex_t mutex_pid;
extern pthread_mutex_t mutex_socket_memory;

extern bool hay_desalojo;
extern int cantidad_colas;
extern t_algortimo_planificacion* algoritmo_por_cola;
extern pthread_mutex_t mutex_cola_multinivel;
extern t_queue** cola_multinivel;


//Variables del memory.config

extern char* puertoEscucha;
extern char* puertoMemory;
extern char* puertoMemoryNotif;
extern char* IPMemory;
extern t_algortimo_planificacion algoritmo;
extern int quantum;
extern int suspensionTimeout;

//listas de los 7 estados
//extern t_list* new_lista;
//extern pthread_mutex_t new_mutex;
extern t_queue* ready_cola;
extern pthread_mutex_t ready_mutex;
//extern t_queue** ready_colas_multinivel; implemnetar para el 3er check
extern t_list* block_lista;
extern pthread_mutex_t block_mutex;

extern t_list* susp_block;
extern t_list* susp_ready;
extern pthread_mutex_t mutex_susp_block;
extern pthread_mutex_t mutex_susp_ready;

extern t_list* exec_lista; 
extern pthread_mutex_t exec_mutex; 

//SEMAFOROS
extern sem_t sem_hay_proceso_ready;
extern sem_t sem_hay_cpu_libre;

//eatos son para eliminar espera activa de CMN DESALOJO
extern pthread_cond_t  cond_planificador;
extern pthread_mutex_t mutex_planificador;

//sockets de las IOs

extern int socketSleep;
extern int socketStdin;
extern int socketStdout;


//colas de IOs
extern t_queue* cola_sleep;
extern t_queue* cola_stdin;
extern t_queue* cola_stdout;

extern pthread_mutex_t mutex_cola_sleep;
extern pthread_mutex_t mutex_cola_stdin;
extern pthread_mutex_t mutex_cola_stdout;

//cola MUTEX

extern t_list* lista_mutex;
extern pthread_mutex_t mutex_lista_mutex;

//semáforos de disponibilidad de IO 
extern sem_t sem_sleep_disponible;
extern sem_t sem_stdin_disponible;
extern sem_t sem_stdout_disponible;
extern sem_t sem_hay_proc_esperando_sleep;
extern sem_t sem_hay_proc_esperando_stdin;
extern sem_t sem_hay_proc_esperando_stdout;

extern bool stdout_ocupado;
extern pthread_mutex_t mutex_stdout_ocupado;




#endif
