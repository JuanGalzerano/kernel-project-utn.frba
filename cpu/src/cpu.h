#ifndef CPU_H_
#define CPU_H_

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
#include "instrucciones.h"
#include "syscalls.h"
#include "contexto.h"
#include "ciclo.h"
#include "interrupciones.h"

//REGISTROS CPU
typedef struct {
    uint32_t pc; //proxima instruccion
    uint8_t ax;
    uint8_t bx;
    uint8_t cx;
    uint8_t dx;
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t si; //direccion logica de memoria de origen
    uint32_t di; //direccion logica de memoria de destino
} registroCpu;

#endif