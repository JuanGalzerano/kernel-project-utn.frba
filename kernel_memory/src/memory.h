#ifndef MEMORY_H_
#define MEMORY_H_

#include "memory_gestor.h"
#include "inicializar.h"
#include "segmentacion.h"
#include "memory_sticks.h"
#include "serializacion_m.h"
#include "compactacion.h"
#include "swap_gestor.h"

// VARIABLES GLOBALES (definiciones — solo en memory.c)

t_log*    loggerMemory;
t_config* configMemory;

char*     puertoMemoryStick;
char*     puertoEscucha;
char*     puertoEscuchaNotif;
char*     scriptsBasePath;
uint32_t  segment_max_size;
char*     allocation_strategy;
uint32_t  instruction_delay;
uint32_t  compaction_delay;
t_log_level logLevel;
t_list*   lista_procesos;
t_list*   lista_memory_sticks;
t_list*   lista_sockets_cpu_notif;
t_list*   lista_huecos;
uint32_t  memoria_total_size;
uint32_t  memoria_libre_size;

pthread_mutex_t memoria_mutex;
pthread_mutex_t mutex_sockets_cpu_notif;
pthread_mutex_t procesos_mutex;

int socketScheduler;
int socketSchedulerNotif;
int socketSwap;
int socketEscuchaNotif;
int epoll_fd_sticks;

t_bloque_swap* tabla_swap;
uint32_t swap_total_size;
uint32_t swap_block_size;
uint32_t swap_num_bloques;
pthread_mutex_t swap_mutex;

// FUNCIONES
void notificar_desuspendibles();
void* atender_scheduler(void* arg);
void* atender_cpu(void* arg);
void* hilo_notificaciones_cpu(void* arg);

#endif