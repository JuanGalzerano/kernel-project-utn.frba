#include "serializacion.h"
#include "inicializar.h"

// Protocolo: [op_code (1B)] [size (4B)] [payload (size bytes)]

void enviar_paquete(int socket, t_paquete* paquete) {
    send(socket, &paquete->codigo_operacion, sizeof(uint8_t), 0);
    uint32_t size = paquete->buffer->offset;
    send(socket, &size, sizeof(uint32_t), 0);
    send(socket, paquete->buffer->stream, size, 0);
}

// El caller es responsable de liberar: buffer_destroy(p->buffer); free(p);
t_paquete* recibir_paquete(int socket) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    recv(socket, &paquete->codigo_operacion, sizeof(uint8_t), MSG_WAITALL);
    uint32_t size;
    recv(socket, &size, sizeof(uint32_t), MSG_WAITALL);
    paquete->buffer = buffer_create(size);
    paquete->buffer->stream = malloc(size);
    recv(socket, paquete->buffer->stream, size, MSG_WAITALL);
    return paquete;
}

//
// SERIALIZACION DEL CONTEXTO DE EJECUCION
// El orden importa para q queden ordenaditos los bytes
// viotti tmb lo tiene q implementar pero no lo queria poner en utils por las dudas q no funcione, desp lo vemos juntos y lo ponemos ahi

static void serializar_contexto(t_buffer* buf, t_contexto_ejecucion* ctx) {
    buffer_add_uint32(buf, ctx->pc);
    buffer_add_uint8 (buf, ctx->ax);
    buffer_add_uint8 (buf, ctx->bx);
    buffer_add_uint8 (buf, ctx->cx);
    buffer_add_uint8 (buf, ctx->dx);
    buffer_add_uint32(buf, ctx->eax);
    buffer_add_uint32(buf, ctx->ebx);
    buffer_add_uint32(buf, ctx->ecx);
    buffer_add_uint32(buf, ctx->edx);
    buffer_add_uint32(buf, ctx->si);
    buffer_add_uint32(buf, ctx->di);
}

static void deserializar_contexto(t_buffer* buf, t_contexto_ejecucion* ctx) {
    ctx->pc  = buffer_read_uint32(buf);
    ctx->ax  = buffer_read_uint8 (buf);
    ctx->bx  = buffer_read_uint8 (buf);
    ctx->cx  = buffer_read_uint8 (buf);
    ctx->dx  = buffer_read_uint8 (buf);
    ctx->eax = buffer_read_uint32(buf);
    ctx->ebx = buffer_read_uint32(buf);
    ctx->ecx = buffer_read_uint32(buf);
    ctx->edx = buffer_read_uint32(buf);
    ctx->si  = buffer_read_uint32(buf);
    ctx->di  = buffer_read_uint32(buf);
}

// La CPU pidio el contexto (OBTENER_CONTEXTO). Respondemos con ENVIAR_CONTEXTO.
// opcode: ENVIAR_CONTEXTO=1
void enviar_contexto_cpu(int socket, uint32_t pid) {
    t_proceso_memory* proceso = buscar_proceso(pid);
    if (!proceso) {
        log_error(loggerMemory, "PID %d no encontrado al enviar contexto", pid);
        return;
    }

    t_paquete paquete = paquete_create(ENVIAR_CONTEXTO);
    serializar_contexto(paquete.buffer, proceso->contexto);
    enviar_paquete(socket, &paquete);
    buffer_destroy(paquete.buffer);
}

// La CPU mando el contexto actualizado (ACTUALIZAR_CONTEXTO). Lo guardamos.
// opcode: ACTUALIZAR_CONTEXTO=2
void recibir_contexto_cpu(int socket, uint32_t pid) {
    t_proceso_memory* proceso = buscar_proceso(pid);
    if (!proceso) {
        log_error(loggerMemory, "PID %d no encontrado al recibir contexto", pid);
        return;
    }

    t_paquete* paquete = recibir_paquete(socket);
    deserializar_contexto(paquete->buffer, proceso->contexto);
    buffer_destroy(paquete->buffer);
    free(paquete);
}
