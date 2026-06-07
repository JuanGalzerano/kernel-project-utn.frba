#ifndef MEMORY_GESTOR_H_
#define MEMORY_GESTOR_H_

#include <utils/utils.h>
#include <commons/collections/list.h>

// Hueco libre contiguo en el espacio de memoria física global
typedef struct {
    uint32_t base;   // dirección física global donde empieza el hueco
    uint32_t limite; // tamaño en bytes del hueco
} t_hueco;

//Entrada en la tabla de bloques del swap
typedef struct {
    bool ocupado;
    uint32_t pid;
    uint32_t segmento_id;
    uint32_t tamanio;
} t_bloque_swap;

// Proceso residente en Kernel Memory
typedef struct {
    uint32_t              pid;
    char*                 path_pseudocodigo;
    t_contexto_ejecucion* contexto; // contexto->tabla_segmentos es la tabla de segmentos del proceso
} t_proceso_memory;

// VARIABLES GLOBALES

extern t_log*    loggerMemory;
extern t_config* configMemory;

// Variables del memory.config
extern char*     puertoEscucha;
extern char*     scriptsBasePath;
extern uint32_t  segment_max_size;
extern char*     allocation_strategy; //BEST o WORST
extern uint32_t  instruction_delay; 
extern uint32_t  compaction_delay;

extern t_list*   lista_procesos;       //ista de t_proceso_memory*
extern t_list*   lista_memory_sticks;  // lista de t_memory_stick_info*
extern t_list*   lista_sockets_cpu_notif; // sockets del canal de notificaciones de cada CPU conectada
extern t_list*   lista_huecos;         // lista de t_hueco* ordenada por base física
extern uint32_t  memoria_total_size;   // suma de tamaños de todos los sticks conectados
extern uint32_t  memoria_libre_size;   // suma de bytes libres en todos los huecos (se mantiene actualizada)

extern pthread_mutex_t mutex_sockets_cpu_notif; //protege lista_sockets_cpu_notif
extern pthread_mutex_t memoria_mutex;   //protege lista_huecos, lista_memory_sticks y tablas de segmentos
extern pthread_mutex_t procesos_mutex;  //protege lista_procesos y proceso->contexto

extern int socketScheduler;
extern int socketSchedulerNotif;
extern int socketSwap;
extern int socketEscuchaNotif;
extern int epoll_fd_sticks;
extern char* puertoEscuchaNotif;

extern t_bloque_swap* tabla_swap;
extern uint32_t swap_total_size;
extern uint32_t swap_block_size;
extern uint32_t swap_num_bloques;
extern pthread_mutex_t swap_mutex;

#endif