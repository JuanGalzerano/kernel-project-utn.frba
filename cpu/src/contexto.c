#include "contexto.h"
#include "instrucciones.h"  

uint32_t obtener_pid(int socketConexionScheduler) {
    t_paquete *paquete = recibir_paquete(socketConexionScheduler);
    uint32_t pid = buffer_read_uint32(paquete->buffer);
    eliminar_paquete(paquete);
    return pid;
}

t_contexto_ejecucion *obtener_contexto(uint32_t pid, int socketConexionMemory) {
    op_code opCode = OBTENER_CONTEXTO;
    send(socketConexionMemory, &opCode, sizeof(op_code), 0);
    send(socketConexionMemory, &pid, sizeof(uint32_t), 0);
    t_paquete *paquete = recibir_paquete(socketConexionMemory);
    t_contexto_ejecucion *ctx = deserializar_contexto_ctx(paquete->buffer);
    eliminar_paquete(paquete);
    return ctx;
}

void actualizar_contexto(t_contexto_ejecucion *ctx, int socketConexionMemory) {  
    ctx->pc = registros_cpu.pc;
    ctx->ax = registros_cpu.ax;
    ctx->bx = registros_cpu.bx;
    ctx->cx = registros_cpu.cx;
    ctx->dx = registros_cpu.dx;
    ctx->eax = registros_cpu.eax;
    ctx->ebx = registros_cpu.ebx;
    ctx->ecx = registros_cpu.ecx;
    ctx->edx = registros_cpu.edx;
    ctx->si = registros_cpu.si;
    ctx->di = registros_cpu.di;
    t_buffer *buffer = serializar_contexto_ctx(ctx);
    t_paquete *paquete = crear_paquete(ACTUALIZAR_CONTEXTO, buffer);
    enviar_paquete(socketConexionMemory, paquete);
    eliminar_paquete(paquete);  
    free(ctx);
}

void actualizar_registros_cpu(t_contexto_ejecucion *ctx) {
    registros_cpu.pc = ctx->pc;
    registros_cpu.ax = ctx->ax;
    registros_cpu.bx = ctx->bx;
    registros_cpu.cx = ctx->cx;
    registros_cpu.dx = ctx->dx;
    registros_cpu.eax = ctx->eax;
    registros_cpu.ebx = ctx->ebx;
    registros_cpu.ecx = ctx->ecx;
    registros_cpu.edx = ctx->edx;
    registros_cpu.si = ctx->si;
    registros_cpu.di = ctx->di;
}