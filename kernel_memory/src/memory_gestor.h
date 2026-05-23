#ifndef MEMORY_GESTOR_H_
#define MEMORY_GESTOR_H_

#include <utils/utils.h>
#include <commons/collections/list.h>

// Información de un Memory Stick conectado
typedef struct {
    int      socket;
    uint32_t size;
    uint32_t base_acumulada;
} t_memory_stick_info;

// Hueco libre contiguo en el espacio de memoria física global
typedef struct {
    uint32_t base;   // dirección física global donde empieza el hueco
    uint32_t limite; // tamaño en bytes del hueco
} t_hueco;

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

extern t_list*   lista_procesos;       //ista de t_proceso_memory*
extern t_list*   lista_memory_sticks;  // lista de t_memory_stick_info*
extern t_list*   lista_huecos;         // lista de t_hueco* ordenada por base física
extern uint32_t  memoria_total_size;   //suma de tamaños de todos los sticks conectados

extern pthread_mutex_t memoria_mutex;   //protege lista_huecos, lista_memory_sticks y tablas de segmentos
extern pthread_mutex_t procesos_mutex;  //protege lista_procesos y proceso->contexto

extern int socketScheduler;

#endif