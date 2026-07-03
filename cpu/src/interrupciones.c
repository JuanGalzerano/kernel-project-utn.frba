#include "interrupciones.h"
#include "inicializar_cpu.h"
#include <utils/serializacion.h>

uint32_t motivo_interrupcion = -1;

// select() es un recv() con timeout 0, es decir, no bloqueante.
bool hay_interrupcion(uint32_t pidActual, int socketConexionScheduler) {
    fd_set readfds; //Lista de sockets donde escuchar
    FD_ZERO(&readfds); //Limpio la lista
    FD_SET(socketConexionScheduler, &readfds); //Agrego el socket de scheduler pq quiero escuchar ahi
    struct timeval tv = {0, 0}; //Timeout 0, es decir, no bloqueante

    //select(numero mas alto de descriptor, lista, es de escritura?, es de error?, timeout)
    if (select(socketConexionScheduler + 1, &readfds, NULL, NULL, &tv) <= 0) {
        //Si devuelve > 0, hay datos para leer, si devuelve 0, no hay datos (timeout), y si devuelve < 0, hubo un error.
        return false;
    }

    t_paquete *paquete = recibir_paquete(socketConexionScheduler);
    if (paquete == NULL) {
        log_error(loggerCpu, "CPU: Se desconecto el Scheduler al esperar interrupciones");
        abort();
    }
    op_code interrupcionRecibida = paquete->codigo_operacion;
    if ((interrupcionRecibida == FINALIZAR_POR_QUANTUM) || (interrupcionRecibida == COMPACTACION) || (interrupcionRecibida == DESALOJO) || (interrupcionRecibida == DESALOJAR_POR_BSOD)) {
        uint32_t pidInterrumpido = buffer_read_uint32(paquete->buffer);
        eliminar_paquete(paquete);
        if (pidInterrumpido == pidActual) {
            motivo_interrupcion = interrupcionRecibida;
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

    log_info(loggerCpu,
        "ENVIANDO motivo=%d pid=%u size=%u",
        motivo,
        pid,
        buffer->offset);

    t_paquete *paquete = crear_paquete(motivo, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    eliminar_paquete(paquete);
}