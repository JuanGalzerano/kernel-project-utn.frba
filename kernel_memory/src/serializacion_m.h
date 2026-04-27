#ifndef SERIALIZACION_H_
#define SERIALIZACION_H_

#include "memory_gestor.h"

// Envio y recepcion del contexto de ejecucion hacia/desde la CPU
void enviar_contexto_cpu(int socket, uint32_t pid);
void recibir_contexto_cpu(int socket, uint32_t pid);

#endif
