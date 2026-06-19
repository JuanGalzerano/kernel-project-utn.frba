#include "serializacion_m.h"
#include "inicializar.h"

// KM → CPU: serializa el contexto del proceso y lo manda como paquete ENVIAR_CONTEXTO
void enviar_contexto_cpu(int socket, uint32_t pid) {
    pthread_mutex_lock(&procesos_mutex);
    t_proceso_memory* proceso = buscar_proceso_sin_lock(pid);
    if(!proceso){
        pthread_mutex_unlock(&procesos_mutex);
        log_error(loggerMemory, "PID %d no encontrado al enviar contexto", pid);
        return;
    }
    t_buffer* buffer = serializar_contexto_ctx(proceso->contexto);
    pthread_mutex_unlock(&procesos_mutex);

    t_paquete* paquete = crear_paquete(ENVIAR_CONTEXTO, buffer);
    enviar_paquete(socket, paquete);
    eliminar_paquete(paquete); // libera buffer y paquete
}

// CPU → KM: recibe el contexto actualizado y lo guarda en el proceso.
// El buffer ya fue extraido del paquete recibido en atender_cpu (pid ya leido).
void recibir_contexto_cpu(uint32_t pid, t_buffer* buffer) {
    t_contexto_ejecucion* ctx_nuevo = deserializar_contexto_ctx(buffer);

    pthread_mutex_lock(&procesos_mutex);
    t_proceso_memory* proceso =buscar_proceso_sin_lock(pid);
    if(!proceso){
        pthread_mutex_unlock(&procesos_mutex);
        log_warning(loggerMemory, "PID %d no encontrado al recibir contexto (posible EXIT)", pid);
        list_destroy_and_destroy_elements(ctx_nuevo->tabla_segmentos, free);
        free(ctx_nuevo);
        return;
    }
    list_destroy_and_destroy_elements(ctx_nuevo->tabla_segmentos, free);
    ctx_nuevo->tabla_segmentos = proceso->contexto->tabla_segmentos;
    free(proceso->contexto);
    proceso->contexto = ctx_nuevo;
    pthread_mutex_unlock(&procesos_mutex);
}

// KM → CPU: lee la instruccion en el PC actual y la manda como paquete ENVIAR_INSTRUCCION.
// El CPU sabe por el opcode que el payload es el string de la instruccion,
// y sabe el tamaño por el campo size del header del paquete.
void enviar_instruccion_cpu(int socket, uint32_t pid, uint32_t pc) {
    t_proceso_memory* proceso = buscar_proceso(pid);
    if (!proceso) {
        log_error(loggerMemory, "PID %d no encontrado al enviar instruccion", pid);
        return;
    }

    char* instruccion = leer_instruccion(proceso, pc);
    if (!instruccion) return;

    log_info(loggerMemory, "## PID: %d - Obtener instrucción: %d - Instrucción: %s", pid, pc, instruccion);

    t_buffer* buffer = buffer_create(0);
    buffer_add_string(buffer, strlen(instruccion), instruccion);
    free(instruccion);

    t_paquete* paquete = crear_paquete(ENVIAR_INSTRUCCION, buffer);
    enviar_paquete(socket, paquete);
    eliminar_paquete(paquete);
}