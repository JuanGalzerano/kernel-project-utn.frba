#include "interrupciones.h"
#include "inicializar_cpu.h"
#include <utils/serializacion.h>

uint32_t motivo_interrupcion = -1;

// Chequea (sin bloquear) si llegó un paquete de interrupción del scheduler.
// Se llama entre instrucciones, por lo que el main thread es el único lector del socket.
bool hay_interrupcion(uint32_t pidActual, int socketConexionScheduler) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(socketConexionScheduler, &readfds);
    struct timeval tv = {0, 0};

    if (select(socketConexionScheduler + 1, &readfds, NULL, NULL, &tv) <= 0)
        return false;

    t_paquete *paquete = recibir_paquete(socketConexionScheduler);
    if (paquete->codigo_operacion == FINALIZAR_POR_QUANTUM) {
        uint32_t pidInterrumpido = buffer_read_uint32(paquete->buffer);
        eliminar_paquete(paquete);
        if (pidInterrumpido == pidActual) {
            motivo_interrupcion = FINALIZAR_POR_QUANTUM;
            log_info(loggerCpu, "## Interrupción recibida");
            return true;
        }
    } else {
        eliminar_paquete(paquete);
    }
    return false;
}

void enviar_pid_y_motivo(uint32_t pid, uint32_t motivo, int socketConexionScheduler) {
    t_buffer *buffer = buffer_create(0);
    buffer_add_uint32(buffer, pid);
    t_paquete *paquete = crear_paquete(motivo, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    eliminar_paquete(paquete);
}
