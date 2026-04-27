#ifndef MEMORY_GESTOR_H_
#define MEMORY_GESTOR_H_


#include <utils/utils.h>
#include <commons/collections/list.h>


// Estructura que representa un proceso en Kernel Memory
typedef struct {
    uint32_t pid;
    char* path_pseudocodigo;        // path completo al archivo .prog
    t_contexto_ejecucion* contexto; // todos los registros inicializados en 0
    // tabla de segmentos se agrega mas adelante me imagino, todo lo q es stack todo eso, por ahora asi me queda prolijo
} t_proceso_memory;


//VARIABLES GLOBALES

extern t_log* loggerMemory;
extern t_config* configMemory;


//Variables del memory.config

extern char* puertoEscucha;
extern char* scriptsBasePath; // SCRIPTS_BASEPATH del config
extern t_list* lista_procesos; // lista de t_proceso_memory*

#endif