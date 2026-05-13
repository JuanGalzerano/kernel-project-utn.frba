#ifndef INICIALIZAR_H_
#define INICIALIZAR_H_

#include "memory_gestor.h"

// Inicialización del módulo
void inicializar_log_y_config(char* path);

// Gestión de procesos
int inicializar_proceso(uint32_t pid, char* path);
void finalizar_proceso(uint32_t pid);
t_proceso_memory* buscar_proceso(uint32_t pid);
char* leer_instruccion(t_proceso_memory* proceso, uint32_t pc);

// Memory Sticks
void agregar_memory_stick(int socket, uint32_t size);
t_memory_stick_info* encontrar_stick_por_dir_fisica(uint32_t dir_fisica);

// I/O con Memory Sticks (la comunicación real va por socket; estas funciones encapsulan eso)
// Retornan 1=ok, -1=error
int leer_de_memory_stick(uint32_t dir_fisica, uint32_t tamanio, void* buffer_out);
int escribir_en_memory_stick(uint32_t dir_fisica, uint32_t tamanio, void* datos);

// Gestión de segmentos
// Retorna: 1=ok, 0=no hay hueco contiguo suficiente (se necesita compactación), -1=error
int crear_segmento(uint32_t pid, uint32_t id_segmento, uint32_t tamaño);
int eliminar_segmento(uint32_t pid, uint32_t id_segmento);
t_segmento* buscar_segmento(t_proceso_memory* proceso, uint32_t id_segmento);

// Traducción de dirección lógica a física.
// traducir_direccion_logica: retorna dirección física global, o UINT32_MAX si inválida.
// traducir_y_verificar: además chequea que offset+tamanio no exceda el límite del segmento.
//   Retorna 1=ok (dir_fisica_out cargada), -1=seg_fault.
uint32_t traducir_direccion_logica(uint32_t pid, uint32_t direccion_logica);
int      traducir_y_verificar(uint32_t pid, uint32_t dir_logica, uint32_t tamanio, uint32_t* dir_fisica_out);

#endif