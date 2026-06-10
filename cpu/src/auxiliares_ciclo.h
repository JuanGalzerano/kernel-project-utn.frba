#ifndef AUXILIARES_CICLO_H_
#define AUXILIARES_CICLO_H_

#include <stdint.h>

char* obtener_linea_con_instruccion(uint32_t pid, uint32_t pc, int socketConexionMemory);
int interpretar_token(char *token);

#endif