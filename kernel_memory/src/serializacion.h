#ifndef SERIALIZACION_H_
#define SERIALIZACION_H_

#include "memory_gestor.h"

void      enviar_paquete(int socket, uint8_t op, t_buffer* buf);
t_buffer* recibir_paquete(int socket, uint8_t* op_out);

// Envio y recepcion del contexto de ejecucion hacia/desde la CPU
void enviar_contexto_cpu(int socket, uint32_t pid);
void recibir_contexto_cpu(int socket, uint32_t pid);

#endif
