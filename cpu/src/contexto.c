#include "contexto.h"
#include "instrucciones.h"

uint32_t obtener_pid(int socketConexionScheduler) {//el cambio que hice es que si scheduler te bloquea (manda pid tmb) no se siga ejecutando el bloqueado, sino que vuelve a pedir hasta que lo manden a ejecutar
    while(1) {
        t_paquete *paquete = recibir_paquete(socketConexionScheduler);
        
        if(paquete->codigo_operacion == EJECUTAR_PROCESO) {
            uint32_t pid = buffer_read_uint32(paquete->buffer);
            eliminar_paquete(paquete);
            return pid; // solo retorna cuando es un proceso real a ejecutar
        }
        
        if(paquete->codigo_operacion == PROCESO_BLOQUEADO) {
            // el proceso fue bloqueado, seguimos esperando un nuevo PID
            eliminar_paquete(paquete);
            continue; // vuelve al inicio del while a esperar
        }

        if(paquete->codigo_operacion == FIN_PROCESO ) {
            // el proceso fue bloqueado, seguimos esperando un nuevo PID
            eliminar_paquete(paquete);
            continue; // vuelve al inicio del while a esperar
        }

        // cualquier otro caso inesperado
        eliminar_paquete(paquete);
    }
}

t_contexto_ejecucion *obtener_contexto(uint32_t pid, int socketConexionMemory) {
    t_buffer* buffer = buffer_create(0);
    buffer_add_uint32(buffer, pid);
    t_paquete* paqueteConContexto = crear_paquete(OBTENER_CONTEXTO, buffer);
    enviar_paquete(socketConexionMemory, paqueteConContexto);
    eliminar_paquete(paqueteConContexto);

    t_paquete *paquete = recibir_paquete(socketConexionMemory);
    t_contexto_ejecucion *ctx = deserializar_contexto_ctx(paquete->buffer);
    eliminar_paquete(paquete);
    return ctx;
}

void actualizar_contexto(t_contexto_ejecucion *ctx, int socketConexionMemory, uint32_t pid) {  
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
    t_buffer* ctx_buf = serializar_contexto_ctx(ctx);
    t_buffer* buf = buffer_create(0);
    buffer_add_uint32(buf, pid);
    buffer_add(buf, ctx_buf->stream, ctx_buf->size);
    buffer_destroy(ctx_buf);
    t_paquete* paquete_con_contexto = crear_paquete(ACTUALIZAR_CONTEXTO, buf);
    enviar_paquete(socketConexionMemory, paquete_con_contexto);
    eliminar_paquete(paquete_con_contexto);
    if (ctx->tabla_segmentos != NULL) {
        list_destroy_and_destroy_elements(ctx->tabla_segmentos, free);
    }
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