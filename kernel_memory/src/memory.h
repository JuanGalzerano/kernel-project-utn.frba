#ifndef MEMORY_H_
#define MEMORY_H_

#include "memory_gestor.h"
#include "inicializar.h"
#include "segmentacion.h"
#include "memory_sticks.h"
#include "serializacion_m.h"
//#include "compactacion.h"

// VARIABLES GLOBALES (definiciones — solo en memory.c)

t_log*    loggerMemory;
t_config* configMemory;

char*     puertoEscucha;
char*     scriptsBasePath;
uint32_t  segment_max_size;
char*     allocation_strategy;
uint32_t  instruction_delay;
uint32_t  compaction_delay;

t_list*   lista_procesos;
t_list*   lista_memory_sticks;
t_list*   lista_huecos;
uint32_t  memoria_total_size;
uint32_t  memoria_libre_size;

pthread_mutex_t memoria_mutex;
pthread_mutex_t procesos_mutex;

int socketScheduler;

// FUNCIONES
int   aceptar_cliente_memory(int socketEscucha, modulo* quien_out);
void* atender_scheduler(void* arg);
void* atender_cpu(void* arg);

#endif