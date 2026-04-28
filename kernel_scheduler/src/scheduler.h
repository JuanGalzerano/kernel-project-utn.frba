#ifndef SCHEDULER_H_
#define SCHEDULER_H_



#include "gestor_scheduler.h"
#include "inicializar_scheduler.h"

//TYPESDEFS






//COLAS Y LISTAS DEL MODELO DE 7 ESTADOS
t_list* new_lista;
t_queue* ready_cola;
pthread_mutex_t ready_mutex;
//t_queue** ready_colas_multinivel; implemnetar para el 3er check
t_list* block;
t_list* susp_block;
t_list* susp_ready;
t_list* exec_lista;
pthread_mutex_t exec_mutex; // lista de t_cpu_exec*, aca estan todas las cpus que tenga, esten ejecutando o no.
//dep ver si tendria que hacer algo con exit

//SEMAFOROS CPUs LIBRES Y PROCESOS EN READY 

sem_t sem_hay_proceso_ready;
sem_t sem_hay_cpu_libre;

//VARIABLES GLOBALES

t_log* loggerScheduler;
t_config* configScheduler;


//Variables de scheduler.config

char* puertoEscucha;
char* puertoMemory;
char* IPMemory;
int socketConexionMemory;
t_planification_algorithm algoritmo;
int quantum;

void *atender_cliente(void *arg);

t_pcb* crear_proceso(uint32_t pid, char* path, int prioridad);

void enviar_path_proceso_memory(uint32_t pid,char* path);

int recibir_ok_memory();

int aceptar_cliente_scheduler(int socketEscucha, t_log *logger);

void* planificador(void* arg);

t_cpu_exec* obtener_cpu_libre();

void enviar_proceso_a_cpu(t_cpu_exec* cpu, t_pcb* pcb);

void iniciar_timer_quantum(t_cpu_exec* cpu);

void* hilo_timer_quantum(void* arg);


#endif