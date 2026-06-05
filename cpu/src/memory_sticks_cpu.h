#ifndef MEMORY_STICKS_CPU_H_
#define MEMORY_STICKS_CPU_H_

#include <utils/utils.h>
#include <utils/mmu.h>
#include <pthread.h>

extern t_list* lista_memory_stick;
extern pthread_mutex_t mutex_lista_memory_stick;

void inicializar_memory_sticks_cpu(void);
void agregar_memory_stick_cpu(int socket, uint32_t size);
void* hilo_servidor_memory_sticks(void* arg);

#endif