#ifndef SWAP_H_
#define SWAP_H_

#include <utils/utils.h>
#include <stdio.h>

void  escribir_Bloque(uint32_t idBloque, FILE* archivo, void* contenido, uint32_t tamanio);
char* leer_Bloque(uint32_t idBloque, FILE* archivo, uint32_t tamanio);

#endif
