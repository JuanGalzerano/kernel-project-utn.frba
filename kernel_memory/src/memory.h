#ifndef MEMORY_H_
#define MEMORY_H_

#include "memory_gestor.h"
#include "inicializar.h"

//VARIABLES GLOBALES

t_log* loggerMemory;
t_config* configMemory;


//Variables de memory.config

char* puertoEscucha;

//FUNCIONES
char* recibir_path(int socket);
void crear_proceso(int pid, char* path);
void* hacerAlgo(void* arg);



#endif