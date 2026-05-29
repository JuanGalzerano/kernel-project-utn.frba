#ifndef UTILS_SCHEDULER_H_
#define UTILS_SCHEDULER_H_

#include "gestor_scheduler.h"

op_code recibir_respuesta_memory();

t_pcb* crear_proceso(uint32_t pid, char* path, int prioridad);

int recibir_ok_memory();

t_cpu_exec* obtener_cpu_libre();

void enviar_proceso_a_cpu(t_cpu_exec* cpu,t_pcb* pcb);

void reanudar_proceso_en_cpu(t_cpu_exec* cpu);

uint32_t generar_pid();

t_cpu_exec* encontrar_cpu_con_pid(uint32_t pid);

void recibir_tipo_IO(int socketCliente);

void bloquear_proceso(t_pcb* pcbBlock);

t_pcb* buscar_y_sacar_de_block(uint32_t pid);

void enviar_fin_proceso_memory(uint32_t pid);

void enviar_fin_proceso_a_cpu(uint32_t pid, int socketCPU);

void* hilo_timer_quantum(void* arg);

void iniciar_timer_quantum(t_cpu_exec* cpu);

void enviar_path_proceso_memory(uint32_t pid,char* path);

char* solicitar_cadena_a_memory(uint32_t pid, uint32_t direccionLogica, uint32_t bytes);

void liberar_mutex_y_semaforos();

t_mutex_syscall* buscar_mutex(char* nombreMutex);

op_code solicitar_segmento_memory(t_mem_alloc* infoMemAlloc);

void* hilo_escuchar_memory(void* arg);

void manejar_bsod();

void encolar_pcb_ready(t_pcb* pcb);

t_pcb* desencolar_pcb_ready();
#endif