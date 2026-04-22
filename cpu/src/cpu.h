#ifndef CPU_H_
#define CPU_H_

#include "cpu_gestor.h"
#include "inicializar_cpu.h"

//DEFINO VARIABLES GLOBALES
t_log* loggerCpu;
t_config* configCpu;

// VARIABLES cpu.config
extern char* puertoKernel;
extern char* IPKernel;
extern char* puertoMemory;
extern char* IPMemory;
extern char* puertoMemoryStick;
extern char* IPMemoryStick;
extern char* idCpu;

#endif