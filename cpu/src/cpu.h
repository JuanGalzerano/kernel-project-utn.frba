#ifndef CPU_H_
#define CPU_H_

#include "cpu_gestor.h"
#include "inicializar_cpu.h"
#include "instrucciones.h"
#include <semaphore.h>

//VARIABLES cpu.config
extern char* puertoKernel;
extern char* IPKernel;
extern char* puertoMemory;
extern char* IPMemory;
extern char* puertoMemoryStick;
extern char* IPMemoryStick;
extern char* idCpu;
typedef struct {
    char *nombreDelRegistro;
    Codigo_registros_cpu codigo_registros_cpu;
} Registro;

extern Registro registros[];

//INSTRUCCIONES
typedef struct {
    char *nombreDeLaInstruccion;
    op_code instruccion_codigo;
} Instruccion;

extern Instruccion instrucciones[];

uint32_t obtener_pid(int socketConexionScheduler);
t_contexto_ejecucion* obtener_contexto(uint32_t pid, int socketConexionMemory);
void actualizar_contexto(t_contexto_ejecucion *ctx, int socketConexionMemory, uint32_t pid);
void actualizar_registros_cpu(t_contexto_ejecucion* ctx);
int ejecutar_ciclo_de_instruccion(int socketConexionMemory, t_log* loggerCpu, int socketConexionScheduler, uint32_t pid);
int obtener_instruccion_registro_valor(char **string);
char* obtener_nombre_mutex_o_path(char** string);
int decode(op_code tipoInstruccion, char **string, int socketConexionScheduler, uint32_t pid, t_log *loggerCpu);

extern sem_t sem_interrupcion;
extern uint32_t pid_interrupcion;
bool hay_interrupcion(uint32_t pidActual);
void* interrupciones(void* arg);
void enviar_pid_y_motivo(uint32_t pid, uint32_t motivo, int socketConexionScheduler);
const char* registro_a_string(Codigo_registros_cpu tipoRegistro);

extern uint32_t motivo_interrupcion;

#endif