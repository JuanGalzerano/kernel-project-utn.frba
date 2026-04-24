#include "serializacion.h"
#include "inicializar.h"


// Protocolo: op_code (1B) size (4B)

void enviar_paquete(int socket, uint8_t op, t_buffer* buf) {
    send(socket, &op, sizeof(uint8_t), 0);
    uint32_t size = buf->offset; // offset == bytes escritos en el buffer
    send(socket, &size, sizeof(uint32_t), 0);
    send(socket, buf->stream, size, 0);
}

t_buffer* recibir_paquete(int socket, uint8_t* op_out) {
    recv(socket, op_out, sizeof(uint8_t), MSG_WAITALL);
    uint32_t size;
    recv(socket, &size, sizeof(uint32_t), MSG_WAITALL);
    t_buffer* buf = buffer_create(size);
    buf->stream = malloc(size);
    recv(socket, buf->stream, size, MSG_WAITALL);
    return buf;
}

// 
// SERIALIZACION DEL CONTEXTO DE EJECUCION
// El orden importa para q queden ordenaditos los bytes
// viotti tmb lo tiene q implementar pero no lo queria poner en utils por las dudas q no funcione, desp lo vemos juntos y lo ponemos ahi


static t_buffer* serializar_contexto(t_contexto_ejecucion* ctx) {
    t_buffer* buf = buffer_create(0);
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
    return buf;
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
// opcode:ENVIAR_CONTEXTO=1 
void enviar_contexto_cpu(int socket, uint32_t pid) {
    t_proceso_memory* proceso = buscar_proceso(pid);
    if (!proceso) {
        log_error(loggerMemory, "PID %d no encontrado al enviar contexto", pid);
        return;
    }

    t_buffer* buf = serializar_contexto(proceso->contexto);
    enviar_paquete(socket, ENVIAR_CONTEXTO, buf);
    buffer_destroy(buf);
}


// opcode:ENVIAR_CONTEXTO=1 
void recibir_contexto_cpu(int socket, uint32_t pid) {
    t_proceso_memory* proceso = buscar_proceso(pid);
    if (!proceso) {
        log_error(loggerMemory, "PID %d no encontrado al recibir contexto", pid);
        return;
    }

    uint8_t op;
    t_buffer* buf = recibir_paquete(socket, &op);
    deserializar_contexto(buf, proceso->contexto);
    buffer_destroy(buf);
}
