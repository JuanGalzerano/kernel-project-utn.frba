#include "memory_sticks_cpu.h"
#include "inicializar_cpu.h"
#include <sys/socket.h>

t_list* lista_memory_stick; //Variable global
pthread_mutex_t mutex_lista_memory_stick;
static uint32_t memoria_total_size_cpu = 0; // la base_acumulada

void inicializar_memory_sticks_cpu(void) {
    lista_memory_stick = list_create();
    pthread_mutex_init(&mutex_lista_memory_stick, NULL);
}

void agregar_memory_stick_cpu(int socket, uint32_t size) {
    t_memory_stick_info* stick = malloc(sizeof(t_memory_stick_info));
    stick->socket = socket;
    stick->size = size;

    pthread_mutex_lock(&mutex_lista_memory_stick);
    stick->base_acumulada = memoria_total_size_cpu;
    list_add(lista_memory_stick, stick);
    memoria_total_size_cpu += size;
    pthread_mutex_unlock(&mutex_lista_memory_stick);

    log_info(loggerCpu, "## Memory Stick de %d bytes conectada (base %d)", size, stick->base_acumulada);
}

void* hilo_servidor_memory_sticks(void* arg) {
    char* puerto = (char*)arg;

    int socketEscucha = iniciar_servidor(puerto);
    if (socketEscucha == EXIT_FAILURE) {
        log_error(loggerCpu, "CPU: no se pudo iniciar el servidor de memory sticks");
        return NULL;
    }
    log_info(loggerCpu, "CPU: servidor de memory sticks iniciado");

    while (1) {
        int socket = esperar_cliente(socketEscucha);
        int32_t id = handshake_servidor_id(socket, 0);

        if (id == MEMORY_STICK) {
            uint32_t stick_size = 0;
            recv(socket, &stick_size, sizeof(uint32_t), MSG_WAITALL);
            agregar_memory_stick_cpu(socket, stick_size);
        } else {
            log_warning(loggerCpu, "CPU: modulo desconocido conectado al servidor de sticks: %d", id);
            close(socket);
        }
    }
    return NULL;
}