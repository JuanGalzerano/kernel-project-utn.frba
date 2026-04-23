#ifndef GESTOR_SCHEDULER_H_
#define GESTOR_SCHEDULER_H_



#include <utils/utils.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>

//VARIABLES GLOBALES

extern t_log* loggerScheduler;
extern t_config* configScheduler;
extern int socketConexionMemory;

//Variables del memory.config

extern char* puertoEscucha;
extern char* puertoMemory;
extern char* IPMemory;

//listas de los 7 estados
extern t_list* new_lista;
extern t_queue* ready_cola;
//extern t_queue** ready_colas_multinivel; implemnetar para el 3er check
extern t_list* block;
extern t_list* susp_block;
extern t_list* susp_ready;
extern t_list* exec_lista; 




#endif
