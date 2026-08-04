#ifndef INICIALIZAR_CPU_H_
#define INICIALIZAR_CPU_H_

#include <utils/utils.h>

void inicializar_log_y_config(char* path, char* idCpu);
void liberar_log(t_log* loggerCpu);
extern t_log* loggerCpu;
extern t_config* configCpu;

extern char* puertoKernel;
extern char* IPKernel;
extern char* puertoMemory;
extern char* IPMemory;
extern char* idCpu;
extern char* puertoKernelMemoryNotificaciones;
extern char* puertoMemoryStick;

void inicializar_log_y_config(char* path, char* idDeCpu);

#endif