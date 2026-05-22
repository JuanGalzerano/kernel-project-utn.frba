#ifndef CICLO_H_
#define CICLO_H_

#include <utils/serializacion.h>

int ejecutar_ciclo_de_instruccion(int socketConexionMemory, int socketConexionScheduler, uint32_t pid);
int decode(op_code tipoInstruccion, int socketConexionScheduler, uint32_t pid);
int interpretar_token(char *token);

#endif