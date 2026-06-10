#ifndef INTERRUPCIONES_H_
#define INTERRUPCIONES_H_

#include <sys/select.h>
#include <stdint.h>
#include <stdbool.h>

extern uint32_t motivo_interrupcion;

bool hay_interrupcion(uint32_t pidActual, int socketConexionScheduler);
void enviar_pid_y_motivo(uint32_t pid, uint32_t motivo, int socketConexionScheduler);

#endif