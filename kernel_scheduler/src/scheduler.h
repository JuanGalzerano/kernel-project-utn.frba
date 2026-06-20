#ifndef SCHEDULER_H_
#define SCHEDULER_H_



#include "gestor_scheduler.h"
#include "inicializar_scheduler.h"
#include "utils_scheduler.h"
#include "gestion_IO_scheduler.h"

//TYPEDEFS


//COLAS Y LISTAS DEL MODELO DE 7 ESTADOS
//t_list* new_lista;
//pthread_mutex_t new_mutex;
t_queue* ready_cola;
pthread_mutex_t ready_mutex;

t_algortimo_planificacion* algoritmo_por_cola;
pthread_mutex_t mutex_cola_multinivel;
t_queue** cola_multinivel;

t_list* block_lista;
pthread_mutex_t block_mutex;

//despues aplicarle mutex a estas 2 cuando las use
t_list* susp_block;
pthread_mutex_t mutex_susp_block;
t_list* susp_ready; 
pthread_mutex_t mutex_susp_ready;

t_list* exec_lista;
pthread_mutex_t exec_mutex; //lista de t_cpu*, aca estan todas las cpus que tenga, esten ejecutando o no.
//para exit no declamos estructura

//SEMAFOROS CPUs LIBRES Y PROCESOS EN READY 

sem_t sem_hay_proceso_ready;
sem_t sem_hay_cpu_libre;
//para espera activa de planificador en CMN con DESALOJO
pthread_cond_t  cond_planificador;
pthread_mutex_t mutex_planificador;

//VARIABLES GLOBALES

t_log* loggerScheduler;
t_config* configScheduler;
int socketConexionMemory;
int socketMemoryNotif;
pthread_mutex_t mutex_socket_memory;
uint32_t proximo_pid = 1;
pthread_mutex_t mutex_pid;
bool compactando=false;


//Variables de scheduler.config

char* puertoEscucha;
char* puertoMemory;
char* puertoMemoryNotif;
char* IPMemory;
t_algortimo_planificacion algoritmo;
int quantum;
bool hay_desalojo;
int cantidad_colas;
int suspensionTimeout;


//Sockets del las diferentes IOs
int socketSleep;
int socketStdin;
int socketStdout;

bool stdout_ocupado = false;
pthread_mutex_t mutex_stdout_ocupado;

// colas de gestion de IOs
t_queue* cola_sleep;
t_queue* cola_stdin;
t_queue* cola_stdout;

//lista de MUTEX
t_list* lista_mutex;
pthread_mutex_t mutex_lista_mutex;


pthread_mutex_t mutex_cola_sleep;
pthread_mutex_t mutex_cola_stdin;
pthread_mutex_t mutex_cola_stdout;

//semáforos de disponibilidad de IO 
sem_t sem_sleep_disponible;
sem_t sem_stdin_disponible;
sem_t sem_stdout_disponible;
sem_t sem_hay_proc_esperando_sleep;
sem_t sem_hay_proc_esperando_stdin;
sem_t sem_hay_proc_esperando_stdout;

//funciones

void *atender_cliente(void *arg);

int aceptar_cliente_scheduler(int socketEscucha, t_log *logger);

void* planificador(void* arg);


#endif