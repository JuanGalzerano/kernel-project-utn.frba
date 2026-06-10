#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <stdbool.h>
#include <commons/log.h>
#include "serializacion.h"

typedef struct {
    int socketMemoryStick;
    uint32_t desde_donde_leer;
    uint32_t tamanio_a_leer_en_esta_memory_stick;
} struct_control_mmu;

t_list* traducir_logica_a_fisica(uint32_t direccion_logica, uint32_t segment_max_size, t_list* lista_segmentos, t_list* lista_memory_stick, uint32_t tamanio_dato);
bool hay_segmentation_fault(uint32_t desplazamiento_del_segmento, uint32_t limite_del_segmento, uint32_t tamanio_dato);
t_segmento* buscar_segmento(t_list* lista_segmentos, uint32_t numero_de_segmento);
t_memory_stick_info* buscar_stick_base(t_list* lista_memory_stick, uint32_t direccion);
t_memory_stick_info* buscar_siguiente_stick(t_list* lista_memory_stick, t_memory_stick_info* stick_base);
uint32_t crear_lista_con_memories_stick_a_llamar(t_memory_stick_info* stick, uint32_t base_del_segmento, uint32_t desplazamiento_del_segmento, uint32_t tamanio, t_list* lista_de_memories_stick_a_llamar, t_list* lista_memory_stick);
char* leer_en_memoria(t_list* lista_de_memories_stick_a_llamar, uint32_t pid, t_log* logger);
uint32_t escribir_en_memoria(t_list* lista_de_memories_stick_a_llamar, char* valor_a_escribir, uint32_t pid, t_log* logger);

#endif