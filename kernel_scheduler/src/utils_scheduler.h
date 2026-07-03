#ifndef UTILS_SCHEDULER_H_
#define UTILS_SCHEDULER_H_

#include "gestor_scheduler.h"

op_code recibir_respuesta_memory();

t_pcb* crear_proceso(uint32_t pid, char* path, int prioridad);

int recibir_ok_memory();

t_cpu* obtener_cpu_libre();

void enviar_proceso_a_cpu(t_cpu* cpu,t_pcb* pcb);

void reanudar_proceso_en_cpu(t_cpu* cpu);

uint32_t generar_pid();

t_cpu* encontrar_cpu_con_pid(uint32_t pid);

void recibir_tipo_IO(int socketCliente);

void bloquear_proceso(t_pcb* pcbBlock);

t_pcb* buscar_y_sacar_de_block(uint32_t pid);

void enviar_fin_proceso_memory(uint32_t pid);

void enviar_fin_proceso_a_cpu(uint32_t pid, int socketCPU);

void* hilo_timer_quantum(void* arg);

void iniciar_timer_quantum(t_cpu* cpu);

void enviar_path_proceso_memory(uint32_t pid,char* path);

char* solicitar_cadena_a_memory(uint32_t pid, uint32_t direccionLogica, uint32_t bytes);

void liberar_mutex_y_semaforos();

t_mutex_syscall* buscar_mutex(char* nombreMutex);

op_code solicitar_segmento_memory(t_mem_alloc* infoMemAlloc, op_code instaciaDeSolicitud, int socket);

void* hilo_escuchar_memory(void* arg);

void manejar_bsod();

void encolar_pcb_ready(t_pcb* pcb);

t_pcb* desencolar_pcb_ready();

t_pcb* pcb_mas_prioritario();

t_cpu* hay_cpu_desalojable(t_pcb* pcbCandidato);

void enviar_desalojo_cpu(t_cpu* cpuDesalojable);

void liberar_cpu_y_notificar();//

t_pcb* buscar_pcb_por_pid(uint32_t pid, int* estabaEnReady);

void despertar_planificador();//

void* hilo_timer_bloqueado(void* arg);

void timer_tiempo_bloqueado(t_pcb* pcb);

void suspender_proceso(t_pcb* pcb);

bool es_mas_prioritario(void* masPrior, void* menosPrior);

void pasar_des_susp_block_a_ready(uint32_t pid, char* cadenaStdin,uint32_t direccionLogica);

t_proc_suspendido* buscar_en_susp_block(uint32_t pid);

void recorrer_y_desuspender(t_list* pidsDesuspendibles);

void desuspender_proceso(t_proc_suspendido* proc);

bool perteneceALalista(t_proc_suspendido* proc, t_list* lista);

void desalojar_por_compactacion(int socket);

void enlistar_primero_ready(t_pcb* pcb);

void destruir_uint32_t(void* nro);

void despostear_todas_cpus();

void liberar_de_mutex_por_muerte(t_pcb* pcb);

#endif