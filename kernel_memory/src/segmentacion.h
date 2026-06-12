#ifndef SEGMENTACION_H_
#define SEGMENTACION_H_

#include "memory_gestor.h"

op_code crear_segmento(uint32_t pid, uint32_t id_segmento, uint32_t tamaño);
int eliminar_segmento(uint32_t pid, uint32_t id_segmento);
t_segmento* buscar_segmento_proceso(t_proceso_memory* proceso, uint32_t id_segmento);
void insertar_hueco_y_fusionar(t_hueco* nuevo);
t_hueco* encontrar_hueco_worst_fit(t_list* huecos, uint32_t tamanio);
t_hueco* encontrar_hueco_best_fit(t_list* huecos, uint32_t tamanio);
t_hueco* encontrar_hueco(t_list* huecos, uint32_t tamanio);
void liberar_fisica_segmento(uint32_t pid, uint32_t seg_id);
op_code asignar_fisica_segmento(uint32_t pid, uint32_t seg_id, uint32_t tamanio);

#endif