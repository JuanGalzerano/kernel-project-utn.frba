#ifndef CPU_H_
#define CPU_H_

#include "cpu_gestor.h"
#include "inicializar_cpu.h"

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

//REGISTROS
typedef struct {
    uint32_t pc;
    uint8_t  ax;
    uint8_t  bx;
    uint8_t  cx;
    uint8_t  dx;
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t si;   // dirección lógica de origen
    uint32_t di;   // dirección lógica de destino
} registros_cpu;

typedef enum {
    PC = 0,
    AX = 1,
    BX = 2,
    CX = 3,
    DX = 4,
    EAX = 5,
    EBX = 6,
    ECX = 7,
    EDX = 8,
    SI = 9,
    DI = 10
} Codigo_registros_cpu;

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
typedef enum {
    NOOP    = 0,
    SET     = 1,
    MOV_IN = 2,
    MOV_OUT = 3,
    SUM = 4,
    SUB = 5,
    JNZ = 6,
    COPY_MEM = 7,
    MUTEX_CREATE = 8,
    MUTEX_LOCK = 9,
    MUTEX_UNLOCK = 10,
    MEM_ALLOC = 11,
    MEM_FREE = 12,
    SLEEP  = 13,
    STDOUT  = 14,
    STDIN  = 15,
    INIT_PROC = 16,
    EXIT = 17
} Instruccion_codigo;
typedef struct {
    char *nombreDeLaInstruccion;
    Instruccion_codigo instruccion_codigo;
} Instruccion;

Instruccion instrucciones[] = {
    {"NOOP", NOOP}, {"SET", SET}, {"MOV_IN", MOV_IN }, {"MOV_OUT", MOV_OUT},
    {"SUM", SUM}, {"SUB", SUB}, {"JNZ", JNZ}, {"COPY_MEM", COPY_MEM},
    {"MUTEX_CREATE", MUTEX_CREATE}, {"MUTEX_LOCK", MUTEX_LOCK}, {"MUTEX_UNLOCK", MUTEX_UNLOCK},
    {"MEM_ALLOC", MEM_ALLOC}, {"MEM_FREE", MEM_FREE}, {"SLEEP", SLEEP}, {"STDOUT", STDOUT},
    {"STDIN", STDIN}, {"INIT_PROC", INIT_PROC}, {"EXIT", EXIT}
};

//Funciones
uint32_t obtener_pid(int socketConexionScheduler);
t_contexto_ejecucion* obtener_contexto(uint32_t pid, int socketConexionMemory);
int ejecutar_ciclo_de_instruccion(t_contexto_ejecucion* ctx, int socketConexionMemory, t_log* loggerCpu);
int obtener_instruccion_registro_valor(char *string);
void decode(Instruccion_codigo tipoInstruccion, char *instruccionFormatoString);

    //Por el momento asi
void ejecutar_noop(void);
void ejecutar_set(Codigo_registros_cpu tipoRegistro, int valor);
void ejecutar_mov_in(Codigo_registros_cpu tipoRegistro);
void ejecutar_mov_out(Codigo_registros_cpu tipoRegistro);
void ejecutar_sum(Codigo_registros_cpu registroDestino, Codigo_registros_cpu registroOrigen);
void ejecutar_sub(Codigo_registros_cpu registroDestino, Codigo_registros_cpu registroOrigen);
void ejectar_jnz(Codigo_registros_cpu tipoRegistro, int instruccion);
void ejecutar_copy_mem(int tamanio);
void ejecutar_mutex_create(char *nombreMutex);
void ejecutar_mutex_lock(char *nombreMutex);
void ejecutar_mutex_unlock(char *nombreMutex);
void ejecutar_mem_alloc(int segmentoId, int tamanio);
void ejecutar_mem_free(int segmentoId);
void ejecutar_sleep(int tiempo);
void ejecutar_stdout(int registroDirLogica, int tamanio);
void ejecutar_stdin(int registroDirLogica, int tamanio);
void ejecutar_init_proc(char *pathArchivoInstrucciones, int prioridad);
void ejecutar_exit(void);

#endif