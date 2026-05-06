#ifndef INICIALIZAR_CPU_H_
#define INICIALIZAR_CPU_H_

#include <utils/utils.h>

extern t_log* loggerCpu;
extern t_config* configCpu;

extern char* puertoKernel;
extern char* IPKernel;
extern char* puertoMemory;
extern char* IPMemory;
extern char* puertoMemoryStick;
extern char* IPMemoryStick;
extern char* idCpu;

void inicializar_log_y_config(char* path, char* idDeCpu);

#endif