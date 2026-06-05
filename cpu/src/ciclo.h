#ifndef CICLO_H_
#define CICLO_H_

#include <utils/serializacion.h>

int ejecutar_ciclo_de_instruccion(int socketConexionMemory, int socketConexionScheduler, uint32_t pid, uint32_t segment_max_size, t_list* lista_segmentos);
int decode(op_code tipoInstruccion, int socketConexionScheduler, uint32_t pid, uint32_t segment_max_size, t_list* lista_segmentos);
int interpretar_token(char *token);

#endif