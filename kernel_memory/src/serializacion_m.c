#include "serializacion_m.h"
#include "inicializar.h"

// La CPU pidio el contexto (OBTENER_CONTEXTO). Respondemos con ENVIAR_CONTEXTO.
// opcode: ENVIAR_CONTEXTO=1
/*void enviar_contexto_cpu(int socket, uint32_t pid) {
    t_proceso_memory* proceso = buscar_proceso(pid);
    if (!proceso) {
        log_error(loggerMemory, "PID %d no encontrado al enviar contexto", pid);
        return;
    }

    t_paquete paquete = paquete_create(ENVIAR_CONTEXTO);
    serializar_contexto(paquete.buffer, proceso->contexto);
    enviar_paquete(socket, &paquete);
    buffer_destroy(paquete.buffer);
}*/

// La CPU mando el contexto actualizado (ACTUALIZAR_CONTEXTO). Lo guardamos.
// opcode: ACTUALIZAR_CONTEXTO=2
/*void recibir_contexto_cpu(int socket, uint32_t pid) {
    t_proceso_memory* proceso = buscar_proceso(pid);
    if (!proceso) {
        log_error(loggerMemory, "PID %d no encontrado al recibir contexto", pid);
        return;
    }

    t_paquete* paquete = recibir_paquete(socket);
    deserializar_contexto(paquete->buffer, proceso->contexto);
    buffer_destroy(paquete->buffer);
    free(paquete);
}*/
