#ifndef MEMORY_STICKS_H_
#define MEMORY_STICKS_H_

#include "memory_gestor.h"

void                 agregar_memory_stick(int socket, uint32_t size);
t_memory_stick_info* encontrar_stick_por_dir_fisica(uint32_t dir_fisica);
op_code              leer_pedazos(t_list* pedazos, char* buffer_out);
op_code              escribir_pedazos(t_list* pedazos, char* datos);

#endif
