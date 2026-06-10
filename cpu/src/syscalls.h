#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#include "instrucciones.h"
#include <commons/log.h>
#include "inicializar_cpu.h"

uint32_t solicitar_mutex_create(char *nombreMutex, int socketConexionScheduler, uint32_t pid);
uint32_t solicitar_mutex_lock(char *nombreMutex, int socketConexionScheduler, uint32_t pid);
uint32_t solicitar_mutex_unlock(char *nombreMutex, int socketConexionScheduler, uint32_t pid);
uint32_t solicitar_mem_alloc(uint32_t segmentoId, uint32_t tamanio, int socketConexionScheduler, uint32_t pid);
uint32_t solicitar_mem_free(uint32_t segmentoId, int socketConexionScheduler, uint32_t pid);
uint32_t solicitar_sleep(uint32_t tiempo, int socketConexionScheduler, uint32_t pid);
uint32_t solicitar_stdout(Codigo_registros_cpu registroDirLogica, Codigo_registros_cpu registroTamanio, int socketConexionScheduler, uint32_t pid);
uint32_t solicitar_stdin(Codigo_registros_cpu registroDirLogica, Codigo_registros_cpu registroTamanio, int socketConexionScheduler, uint32_t pid);
uint32_t solicitar_init_proc(char *pathArchivoInstrucciones, uint32_t prioridad, int socketConexionScheduler, uint32_t pid);
uint32_t solicitar_exit(int socketConexionScheduler, uint32_t pid);

#endif