#ifndef KERNEL_MEMORY_AVISOS_H_
#define KERNEL_MEMORY_AVISOS_H_

#include <utils/utils.h>
#include <utils/mmu.h>
#include <pthread.h>
#include <semaphore.h>

extern t_list* lista_memory_stick;
extern pthread_mutex_t mutex_lista_memory_stick;
extern sem_t sem_lista_cargada;

void* hilo_kernel_memory_avisos(void* arg);
int conectarme_nuevas_sticks(t_list* nueva_lista_memory_stick, int memory_sticks_conectadas);

#endif