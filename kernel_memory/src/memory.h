#ifndef MEMORY_H_
#define MEMORY_H_

#include "memory_gestor.h"
#include "inicializar.h"
#include "serializacion_m.h"

//VARIABLES GLOBALES

t_log* loggerMemory;
t_config* configMemory;


//Variables de memory.config

char* puertoEscucha;

//FUNCIONES
char* recibir_path(int socket, uint32_t* pid_out);
void* hacerAlgo(void* arg);



#endif