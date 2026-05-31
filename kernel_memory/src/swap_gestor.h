#ifndef SWAP_GESTOR_H_
#define SWAP_GESTOR_H_

#include "memory_gestor.h"
#include "segmentacion.h"
#include "memory_sticks.h"

void inicializar_swap();
int swap_bloque_libre();
op_code suspender_proceso(uint32_t pid);
op_code desuspender_proceso(uint32_t pid);

#endif
