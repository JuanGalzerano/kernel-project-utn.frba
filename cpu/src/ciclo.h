#ifndef CICLO_H_
#define CICLO_H_

#include <utils/serializacion.h>

int ejecutar_ciclo_de_instruccion(int socketConexionMemory, int socketConexionScheduler, uint32_t pid);
int decode(op_code tipoInstruccion, char **string, int socketConexionScheduler, uint32_t pid);
int obtener_instruccion_registro_valor(char **string);
char *obtener_nombre_mutex_o_path(char **string);

#endif