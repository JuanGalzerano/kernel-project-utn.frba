#ifndef INSTRUCCIONES_H_
#define INSTRUCCIONES_H_

#include "/home/utnso/tp-2026-1c-Los-Reyes-De-La-Compilaci-n/utils/src/utils/serializacion.h"

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
} Registros_cpu;

//Declaro la variable global de mis registro
extern Registros_cpu registros_cpu;

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

void ejecutar_set(Codigo_registros_cpu tipoRegistro, uint32_t valor);
void ejecutar_mov_in(Codigo_registros_cpu tipoRegistro);
void ejecutar_sum(Codigo_registros_cpu registroDestino, Codigo_registros_cpu registroOrigen);
void ejecutar_sub(Codigo_registros_cpu registroDestino, Codigo_registros_cpu registroOrigen);
void ejecutar_sub(Codigo_registros_cpu registroDestino, Codigo_registros_cpu registroOrigen);
void solicitar_mutex_create(char *nombreMutex, int socketConexionScheduler, uint32_t pid);
void solicitar_mutex_lock(char *nombreMutex, int socketConexionScheduler, uint32_t pid);
void solicitar_mutex_unlock(char *nombreMutex, int socketConexionScheduler, uint32_t pid);
void solicitar_mem_alloc(uint32_t segmentoId, uint32_t tamanio, int socketConexionScheduler, uint32_t pid);
void solicitar_mem_free(uint32_t segmentoId, int socketConexionScheduler, uint32_t pid);
void solicitar_sleep(uint32_t tiempo, int socketConexionScheduler, uint32_t pid);
void solicitar_stdout(Codigo_registros_cpu registroDirLogica, uint32_t tamanio, int socketConexionScheduler, uint32_t pid);
void solicitar_stdin(Codigo_registros_cpu registroDirLogica, uint32_t tamanio, int socketConexionScheduler, uint32_t pid);
void solicitar_init_proc(char* pathArchivoInstrucciones, uint32_t prioridad, int socketConexionScheduler);
void solicitar_exit(int socketConexionScheduler);

#endif