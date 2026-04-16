#ifndef SCHEDULER_H_
#define SCHEDULER_H_



#include "gestor_scheduler.h"
#include "inicializar_scheduler.h"

//VARIABLES GLOBALES

t_log* loggerScheduler;
t_config* configScheduler;


//Variables de scheduler.config

char* puertoEscucha;
char* puertoMemory;
char* IPMemory;




int atender_cliente(int socketEscucha);



#endif