#ifndef INTERRUPCIONES_H_ 
#define INTERRUPCIONES_H_

#include <semaphore.h>
#include <stdint.h>
#include <stdbool.h>

//VARIABLES GLOBALES
extern sem_t sem_interrupcion;
extern uint32_t pid_interrupcion;
extern uint32_t motivo_interrupcion;

bool hay_interrupcion(uint32_t pidActual);
void *interrupciones(void *arg);
void enviar_pid_y_motivo(uint32_t pid, uint32_t motivo, int socketConexionScheduler);

#endif