#ifndef SCHEDULER_H_
#define SCHEDULER_H_



#include "gestor_scheduler.h"
#include "inicializar_scheduler.h"

//TYPESDEFS
typedef struct {
    uint32_t pid;
    int prioridad;//esto va para CMN
    //despues se van a agregar cosas de mutex
} t_pcb;

typedef struct {
    int cpu_id;
    t_pcb* pcb; // NULL si la CPU está libre
} t_cpu_exec; 



//COLAS Y LISTAS DEL MODELO DE 7 ESTADOS
t_list* new_lista;
t_queue* ready_cola;
//t_queue** ready_colas_multinivel; implemnetar para el 3er check
t_list* block;
t_list* susp_block;
t_list* susp_ready;
t_list* exec_lista; // lista de t_cpu_exec*
//dep ver si tendria que hacer algo con exit

//VARIABLES GLOBALES

t_log* loggerScheduler;
t_config* configScheduler;


//Variables de scheduler.config

char* puertoEscucha;
char* puertoMemory;
char* IPMemory;
int socketConexionMemory;

void *atender_cliente(void *arg);

t_pcb* crear_proceso(uint32_t pid, char* path, int prioridad);

void enviar_path_proceso_memory(uint32_t pid,char* path);

int recibir_ok_memory();




#endif