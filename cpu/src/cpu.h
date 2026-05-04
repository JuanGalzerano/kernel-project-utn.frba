#ifndef CPU_H_
#define CPU_H_

#include "cpu_gestor.h"
#include "inicializar_cpu.h"
#include "instrucciones.h"

//DEFINO VARIABLES GLOBALES
t_log* loggerCpu;
t_config* configCpu;

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

Registro registros[] = {
    {"PC", PC}, {"AX", AX}, {"BX", BX }, {"CX", CX},
    {"DX", DX}, {"EAX", EAX}, {"EBX", EBX}, {"ECX", ECX},
    {"EDX", EDX}, {"SI", SI}, {"DI", DI}
};

//INSTRUCCIONES
typedef struct {
    char *nombreDeLaInstruccion;
    op_code instruccion_codigo;
} Instruccion;

Instruccion instrucciones[] = {
    {"NOOP", NOOP}, {"SET", SET}, {"MOV_IN", MOV_IN }, {"MOV_OUT", MOV_OUT},
    {"SUM", SUM}, {"SUB", SUB}, {"JNZ", JNZ}, {"COPY_MEM", COPY_MEM},
    {"MUTEX_CREATE", MUTEX_CREATE}, {"MUTEX_LOCK", MUTEX_LOCK}, {"MUTEX_UNLOCK", MUTEX_UNLOCK},
    {"MEM_ALLOC", MEM_ALLOC}, {"MEM_FREE", MEM_FREE}, {"SLEEP", SLEEP}, {"STDOUT", STDOUT},
    {"STDIN", STDIN}, {"INIT_PROC", INIT_PROC}, {"EXIT", EXIT}
};

uint32_t obtener_pid(int socketConexionScheduler);
t_contexto_ejecucion* obtener_contexto(uint32_t pid, int socketConexionMemory);
Registros_cpu actualizar_registros_cpu(t_contexto_ejecucion* ctx);
int ejecutar_ciclo_de_instruccion(int socketConexionMemory, t_log* loggerCpu, int socketConexionScheduler, uint32_t pid);
int obtener_instruccion_registro_valor(char **string);
void obtener_nombre_mutex_o_path(char** string, char* nombreMutex);
void decode(op_code tipoInstruccion, char **string, int socketConexionScheduler, uint32_t pid);

#endif