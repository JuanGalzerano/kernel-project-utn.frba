#ifndef UTILS_SCHEDULER_H_
#define UTILS_SCHEDULER_H_

#include "gestor_scheduler.h"

t_pcb* crear_proceso(uint32_t pid, char* path, int prioridad);

int recibir_ok_memory();

t_cpu_exec* obtener_cpu_libre();

void enviar_proceso_a_cpu(t_cpu_exec* cpu,t_pcb* pcb);

uint32_t generar_pid();

t_cpu_exec* encontrar_cpu_con_pid(uint32_t pid);

void recibir_tipo_IO(int socketCliente);

void bloquear_proceso(t_pcb* pcbBlock);

t_pcb* buscar_y_sacar_de_block(uint32_t pid);

void enviar_fin_proceso_memory(uint32_t pid);

void iniciar_timer_quantum(t_cpu_exec* cpu);

void enviar_path_proceso_memory(uint32_t pid,char* path);

#endif