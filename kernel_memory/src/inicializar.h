#ifndef INICIALIZAR_H_
#define INICIALIZAR_H_

#include "memory_gestor.h"

int               inicializar_proceso(uint32_t pid, char* path);
void              finalizar_proceso(uint32_t pid);
t_proceso_memory* buscar_proceso(uint32_t pid);
t_proceso_memory* buscar_proceso_sin_lock(uint32_t pid);
char*             leer_instruccion(t_proceso_memory* proceso, uint32_t pc);

#endif