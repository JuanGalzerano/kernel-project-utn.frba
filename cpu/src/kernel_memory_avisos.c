#include "inicializar_cpu.h"
#include <sys/socket.h>
#include <pthread.h>
#include <commons/string.h>
#include "kernel_memory_avisos.h"

t_list* lista_memory_stick; //Variable global
pthread_mutex_t mutex_lista_memory_stick;
sem_t sem_lista_cargada;

void* hilo_kernel_memory_avisos(void* arg) {
    int socketConexionMemoryAvisos = *(int*)arg;
    free(arg);

    int memory_sticks_conectadas = 0;

    uint32_t ya_avise = 0;

    while(1) {
        t_paquete* paquete_sticks = recibir_paquete(socketConexionMemoryAvisos);
        if (paquete_sticks == NULL) {
            log_debug(loggerCpu, "CPU: Se desconecto Kernel Memory Avisos");
            abort();
        }
        if (paquete_sticks->codigo_operacion == AVISO_NUEVO_STICK) {
            t_list* nueva_lista_memory_stick = deserializar_aviso_nuevo_stick(paquete_sticks->buffer);
            pthread_mutex_lock(&mutex_lista_memory_stick);
            memory_sticks_conectadas = conectarme_nuevas_sticks(nueva_lista_memory_stick, memory_sticks_conectadas);
            pthread_mutex_unlock(&mutex_lista_memory_stick);
            if (memory_sticks_conectadas == -1) {
                log_info(loggerCpu, "CPU: Error al conectar con las nuevas Memory Sticks");
                abort();
            }
            else {
                if (!ya_avise && memory_sticks_conectadas > 0) {
                    ya_avise = 1;
                    sem_post(&sem_lista_cargada);
                }
            }
        }
        eliminar_paquete(paquete_sticks);
    }
}

int conectarme_nuevas_sticks(t_list* nueva_lista_memory_stick, int memory_sticks_conectadas) {
    int cantidad_sticks = list_size(nueva_lista_memory_stick);
    if (memory_sticks_conectadas == cantidad_sticks) {
        list_destroy_and_destroy_elements(nueva_lista_memory_stick, free);
        return memory_sticks_conectadas;
    }
    if (cantidad_sticks > memory_sticks_conectadas) {
        for (int i = memory_sticks_conectadas; i < cantidad_sticks; i++) {
            t_memory_stick_info* stick = list_get(nueva_lista_memory_stick, i);

            char* puertoStr = string_itoa(stick->puerto);
            int socketConexionMemoryStick = iniciar_conexion(IPMemoryStick, puertoStr);
            free(puertoStr);
            if (socketConexionMemoryStick == EXIT_FAILURE) {
                log_info(loggerCpu, "CPU: No se pudo conectar con Memory Stick %d", stick->puerto);
                abort();
            }
            uint32_t sizeIdCpu = strlen(idCpu) + 1;
            send(socketConexionMemoryStick, &sizeIdCpu, sizeof(int), 0);
            send(socketConexionMemoryStick, idCpu, sizeIdCpu, 0);
            stick->socket = socketConexionMemoryStick;
            log_info(loggerCpu, "CPU: Conexion establecida con Memory Stick %d", stick->puerto);

            list_add(lista_memory_stick, stick);
        }
        for (int i = 0; i < memory_sticks_conectadas; i++) {
            free(list_get(nueva_lista_memory_stick, i));
        }
        list_destroy(nueva_lista_memory_stick);
        return cantidad_sticks;
    }
    return -1;
}