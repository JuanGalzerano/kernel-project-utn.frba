#ifndef CONTEXTO_H_
#define CONTEXTO_H_

#include <utils/serializacion.h>

uint32_t obtener_pid(int socketConexionScheduler);
t_contexto_ejecucion *obtener_contexto(uint32_t pid, int socketConexionMemory);
void actualizar_contexto(t_contexto_ejecucion *ctx, int socketConexionMemory, uint32_t pid);
void actualizar_registros_cpu(t_contexto_ejecucion *ctx);
void liberar_contexto(t_contexto_ejecucion *ctx);

#endif