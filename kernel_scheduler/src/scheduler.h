#ifndef SCHEDULER_H_
#define SCHEDULER_H_



#include "gestor_scheduler.h"
#include "inicializar_scheduler.h"
#include "utils_scheduler.h"


//COLAS Y LISTAS DEL MODELO DE 7 ESTADOS
t_list* new_lista;
pthread_mutex_t new_mutex;
t_queue* ready_cola;
pthread_mutex_t ready_mutex;
//t_queue** ready_colas_multinivel; implemnetar para el 3er check

t_list* block_lista;
pthread_mutex_t block_mutex;

//despues aplicarle mutex a estas 2 cuando las use
t_list* susp_block;
t_list* susp_ready; 

t_list* exec_lista;
pthread_mutex_t exec_mutex; // lista de t_cpu_exec*, aca estan todas las cpus que tenga, esten ejecutando o no.
//para exit no declamos estructura

//SEMAFOROS CPUs LIBRES Y PROCESOS EN READY 

sem_t sem_hay_proceso_ready;
sem_t sem_hay_cpu_libre;

//VARIABLES GLOBALES

t_log* loggerScheduler;
t_config* configScheduler;
int socketConexionMemory;
uint32_t proximo_pid = 0;
pthread_mutex_t mutex_pid;

//Variables de scheduler.config

char* puertoEscucha;
char* puertoMemory;
char* IPMemory;
t_planification_algorithm algoritmo;
int quantum;

//Sockets del las diferentes IOs
int socketSleep;
int socketStdin;
int socketStdout;

//funciones

void *atender_cliente(void *arg);

int aceptar_cliente_scheduler(int socketEscucha, t_log *logger);

void* planificador(void* arg);

void* hilo_timer_quantum(void* arg);

#endif