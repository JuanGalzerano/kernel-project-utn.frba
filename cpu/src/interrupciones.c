#include "interrupciones.h"  
#include "inicializar_cpu.h"
#include <utils/serializacion.h>

sem_t sem_interrupcion;
uint32_t pid_interrupcion = 0;
uint32_t motivo_interrupcion = -1;

/*
sem_trywait
sem > 0 => Le resta 1 y retorna 0 (true)
sem == 0 => No lo modifica y retorna -1 (false)
*/
bool hay_interrupcion(uint32_t pidActual) {
    if (sem_trywait(&sem_interrupcion) != 0) { //Era -1
        return false;
    }
    if (pidActual == pid_interrupcion) { //Si llego aca era == 0, entonces sem > 0 (hay interrupcion). Si ademas la interrupcion es en este PID, devuelvo true
        return true;
    }
    //La interrupcion es de otro PID, le vuelvo a sumar 1 pq hay una interrupcion. Pero retorno false pq no es en este PID
    sem_post(&sem_interrupcion);
    return false;
}

void *interrupciones(void *arg) {
    int socketConexionScheduler = *(int *)arg;
    while (1) {
        t_paquete *paquete = recibir_paquete(socketConexionScheduler);
        if (paquete->codigo_operacion == FINALIZAR_POR_QUANTUM) {
            pid_interrupcion = buffer_read_uint32(paquete->buffer); //seteo variable global
            motivo_interrupcion = FINALIZAR_POR_QUANTUM; //seteo variable global
            sem_post(&sem_interrupcion);
            log_info(loggerCpu, "## Interrupción recibida");
        }
        eliminar_paquete(paquete);
    }
    return NULL;  
}

void enviar_pid_y_motivo(uint32_t pid, uint32_t motivo, int socketConexionScheduler) {
    t_buffer *buffer = buffer_create(0);
    buffer_add_uint32(buffer, pid);
    t_paquete *paquete = crear_paquete(motivo, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    eliminar_paquete(paquete);
}