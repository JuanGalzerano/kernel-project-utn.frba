#ifndef INICIALIZAR_H_
#define INICIALIZAR_H_

#include "memory_gestor.h"

void inicializar_log_y_config(char* path);
int inicializar_proceso(uint32_t pid, char* path);
t_proceso_memory* buscar_proceso(uint32_t pid);
char* leer_instruccion(t_proceso_memory* proceso);


#endif