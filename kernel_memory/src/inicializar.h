#ifndef INICIALIZAR_H_
#define INICIALIZAR_H_

#include "memory_gestor.h"

// Inicialización del módulo
void inicializar_log_y_config(char* path);

// Gestión de procesos
int inicializar_proceso(uint32_t pid, char* path);
void finalizar_proceso(uint32_t pid);
t_proceso_memory* buscar_proceso(uint32_t pid);
char* leer_instruccion(t_proceso_memory* proceso);

// Memory Sticks
void agregar_memory_stick(int socket, uint32_t size);

// Gestión de segmentos
// Retorna: 1=ok, 0=no hay hueco contiguo suficiente (se necesita compactación), -1=error
int crear_segmento(uint32_t pid, uint32_t id_segmento, uint32_t tamaño);
int eliminar_segmento(uint32_t pid, uint32_t id_segmento);
t_segmento* buscar_segmento(t_proceso_memory* proceso, uint32_t id_segmento);

#endif