#include "serializacion_m.h"
#include "inicializar.h"

// KM → CPU: serializa el contexto del proceso y lo manda como paquete ENVIAR_CONTEXTO
void enviar_contexto_cpu(int socket, uint32_t pid) {
    t_proceso_memory* proceso = buscar_proceso(pid);
    if (!proceso) {
        log_error(loggerMemory, "PID %d no encontrado al enviar contexto", pid);
        return;
    }

    t_buffer*  buffer  = serializar_contexto_ctx(proceso->contexto);
    t_paquete* paquete = crear_paquete(ENVIAR_CONTEXTO, buffer);
    enviar_paquete(socket, paquete);
    eliminar_paquete(paquete); // libera buffer y paquete
}

// CPU → KM: recibe el contexto actualizado y lo guarda en el proceso
void recibir_contexto_cpu(int socket, uint32_t pid) {
    t_proceso_memory* proceso = buscar_proceso(pid);
    if (!proceso) {
        log_error(loggerMemory, "PID %d no encontrado al recibir contexto", pid);
        return;
    }

    t_paquete* paquete = recibir_paquete(socket);
    t_contexto_ejecucion* ctx_nuevo = deserializar_contexto_ctx(paquete->buffer);

    free(proceso->contexto);
    proceso->contexto = ctx_nuevo;

    eliminar_paquete(paquete); // libera buffer y paquete
}
