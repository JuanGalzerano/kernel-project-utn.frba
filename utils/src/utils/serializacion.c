#include "serializacion.h"
#include <utils/utils.h>


t_buffer *buffer_create(uint32_t size){
    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer->size = size;
    buffer->offset = 0;
    buffer->stream = NULL;
    return buffer;
}

void buffer_destroy(t_buffer *buffer){
    if (buffer)
	{
		if (buffer->stream)
			free(buffer->stream);
		free(buffer);
	}
}

void buffer_add(t_buffer *buffer, void *data, uint32_t size){
    buffer->stream = realloc(buffer->stream, buffer->offset + size);

    memcpy(buffer->stream + buffer->offset, data, size);

    buffer->offset += size;
    buffer->size += size;
}

void buffer_read(t_buffer *buffer, void *data, uint32_t size){//creemos que esta bien
    memcpy(data, (uint8_t*)buffer->stream + buffer->offset, size);//ponemos el uint_8 para que se pueda sumar de a 1 byte
    buffer->offset+=size;
}

void buffer_add_uint32(t_buffer *buffer, uint32_t data){
    buffer_add(buffer, &data, sizeof(uint32_t));
}

uint32_t buffer_read_uint32(t_buffer *buffer){
    uint32_t numero;
    buffer_read(buffer, &numero, sizeof(uint32_t));
    return numero;
}

void buffer_add_uint8(t_buffer *buffer, uint8_t data){
    buffer_add(buffer, &data, sizeof(uint8_t));
}

uint8_t buffer_read_uint8(t_buffer *buffer){
    uint8_t numero;
    buffer_read(buffer, &numero, sizeof(uint8_t));
    return numero;
}

void buffer_add_int(t_buffer *buffer, int data){
    buffer_add(buffer, &data, sizeof(int));
}

int buffer_read_int(t_buffer *buffer){
    int numero;
    buffer_read(buffer, &numero, sizeof(int));
    return numero;
}

void buffer_add_string(t_buffer *buffer, uint32_t length, char *string){
    buffer_add(buffer, string, length);
}

char *buffer_read_string(t_buffer *buffer, uint32_t length){
    char *cadena = malloc(length + 1);
    buffer_read(buffer, cadena, length);
    cadena[length] = '\0';
    return cadena;
    //hacer el free en donde se usa la func
}


//Funciones de paquete


//Crear paquete
t_paquete* crear_paquete(op_code codigo_operacion, t_buffer* buffer) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = codigo_operacion;
    paquete->buffer = buffer;
    return paquete;
}


//Enviar paquete
void enviar_paquete(int socket, t_paquete* paquete) {
    op_code op = paquete->codigo_operacion;
    uint32_t size = 0;
    if (paquete->buffer != NULL) {
        size = paquete->buffer->offset;
    }
    send(socket, &op, sizeof(op_code), 0);
    send(socket, &size, sizeof(uint32_t), 0);
    if (size > 0) {
        send(socket, paquete->buffer->stream, size, 0);
    }
    //creo q aca hay que eliminar paquete
}
//recibir paquete
t_paquete* recibir_paquete(int socket) {


    t_paquete* paqueteRecibido = malloc(sizeof(t_paquete));
    
    int bytes = recv(socket, &paqueteRecibido->codigo_operacion, sizeof(op_code), MSG_WAITALL);
    if(bytes <= 0) {  // 0 = desconexión, -1 = error
        free(paqueteRecibido);
        return NULL;
    }

    uint32_t size;
    recv(socket, &size, sizeof(uint32_t), MSG_WAITALL);
    t_buffer *buffer = buffer_create(size);
    buffer->size = size;
    if (size > 0) {//para que pueda mandar streams vacios porque sino quedaba un malloc(0)
        buffer->stream = malloc(size);
        recv(socket, buffer->stream, size, MSG_WAITALL);
    }
    paqueteRecibido->buffer = buffer;
    return paqueteRecibido;
}

//Eliminar paquete
void eliminar_paquete(t_paquete* paquete) {
    buffer_destroy(paquete->buffer);
    free(paquete);
}

t_buffer* serializar_contexto_ctx(t_contexto_ejecucion* ctx){
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

    uint32_t n = 0;
    if (ctx->tabla_segmentos != NULL) {
        n = list_size(ctx->tabla_segmentos);
    }

    buffer_add_uint32(buf, n);
    for (uint32_t i = 0; i < n; i++) {
        t_segmento* seg = list_get(ctx->tabla_segmentos, i);
        buffer_add_uint32(buf, seg->id_segmento);
        buffer_add_uint32(buf, seg->base);
        buffer_add_uint32(buf, seg->limite);
    }
    
    return buf;
}

t_contexto_ejecucion* deserializar_contexto_ctx(t_buffer* buf) {
    t_contexto_ejecucion* ctx = malloc(sizeof(t_contexto_ejecucion));
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

    uint32_t n = buffer_read_uint32(buf);
    ctx->tabla_segmentos = list_create(); 
    for (uint32_t i = 0; i < n; i++) {
        t_segmento* seg = malloc(sizeof(t_segmento));
        seg->id_segmento = buffer_read_uint32(buf);
        seg->base        = buffer_read_uint32(buf);
        seg->limite      = buffer_read_uint32(buf);
        list_add(ctx->tabla_segmentos, seg);
    }
    return ctx;
}

/*
t_contexto_ejecucion* ctx;

t_buffer* buffer_serializado = serializar_contexto_ctx(ctx);

t_paquete* paquete_contexto = crear_paquete(MENSAJE_CONTEXTO, buffer_serializado); Mensaje_contexto es el op_code

// Le aviso que le envio un paquete? Como sabe que debe recibir un paquete?

enviar_paquete(socketCpu, paquete_contexto);
*/

t_buffer* serializar_init_proc(t_init_proc* proc){
    t_buffer* buf = buffer_create(0);
    buffer_add_uint32(buf, proc->pid);
    uint32_t tamanioPath = strlen(proc->pathArchivoInstrucciones)+1;
    buffer_add_uint32(buf, tamanioPath);
    buffer_add_string(buf,tamanioPath, proc->pathArchivoInstrucciones);
    buffer_add_uint32(buf, proc->prioridad);
    return buf;
}

t_init_proc* deserializar_init_proc(t_buffer* buf){
    t_init_proc* proc = malloc(sizeof(t_init_proc));
    proc->pid = buffer_read_uint32(buf);
    uint32_t tamanioPath = buffer_read_uint32(buf);
    proc->pathArchivoInstrucciones = buffer_read_string(buf, tamanioPath);
    proc->prioridad = buffer_read_uint32(buf);
    return proc;
}


t_buffer* serializar_sleep(t_sleep* sleep){
    t_buffer* buf = buffer_create(0);
    buffer_add_uint32(buf, sleep->pid);
    buffer_add_uint32(buf, sleep->tiempoADormir);

    return buf;
}

t_sleep* deserializar_sleep(t_buffer* buf){
    t_sleep* sleep = malloc(sizeof(t_sleep));
    sleep->pid = buffer_read_uint32(buf);
    sleep->tiempoADormir = buffer_read_uint32(buf);
    return sleep;
}


//------ ESTAS TAMBIEN FUNCIONAN PARA STDOUT, NO SOLO STDIN, NO QUIERO CAMBIAR EL NOMBRE EN TODAS LAS LLAMADAS A LA FUNCION

t_buffer* serializar_stdin(t_stdin_stdout* ProcesoStdin){
    t_buffer* buf = buffer_create(0);
    buffer_add_uint32(buf, ProcesoStdin->pid);
    buffer_add_uint32(buf, ProcesoStdin->bytesALeer);
    buffer_add_uint32(buf, ProcesoStdin->direccionLogica);
    if (ProcesoStdin->cadenaLeida != NULL) {
        uint32_t tamanioActualCadena = strlen(ProcesoStdin->cadenaLeida) + 1;
        buffer_add_uint32(buf, tamanioActualCadena);
        buffer_add_string(buf, tamanioActualCadena, ProcesoStdin->cadenaLeida);
    } else {
        buffer_add_uint32(buf, 0);
    }
    return buf;
}

t_stdin_stdout* deserializar_stdin(t_buffer* buf){
    t_stdin_stdout* ProcesoStdin = malloc(sizeof(t_stdin_stdout));
    ProcesoStdin->pid = buffer_read_uint32(buf);
    ProcesoStdin->bytesALeer = buffer_read_uint32(buf);
    ProcesoStdin->direccionLogica = buffer_read_uint32(buf);
    uint32_t tamanioActualCadena = buffer_read_uint32(buf);
    if (tamanioActualCadena > 0) {
        ProcesoStdin->cadenaLeida = buffer_read_string(buf, tamanioActualCadena);
    } else {
        ProcesoStdin->cadenaLeida = NULL;
    }
    return ProcesoStdin;
}

//Mutex
//Esto esta medio jugado, pq la variable queda libre. Supuestamente como al toque q queda libre se reserva aca no pasa nada
t_buffer* serializar_mutex(t_mutex_syscall* mutex_struct){
    t_buffer* buf = buffer_create(0);
    uint32_t tamanioChar = strlen(mutex_struct->nombreMutex)+1;
    buffer_add_int(buf, mutex_struct->contador);
    buffer_add_uint32(buf, mutex_struct->pid);
    buffer_add_uint32(buf, tamanioChar);
    buffer_add_string(buf,tamanioChar, mutex_struct->nombreMutex);
    
    //uint32_t tamanioCola = queue_size(mutex_struct->colaEspera);
    //buffer_add_uint32(buf, tamanioCola);
    //buffer_add(buf, mutex_struct->colaEspera, tamanioCola);
    return buf;
}
t_mutex_syscall* deserializar_mutex(t_buffer* buf){
    t_mutex_syscall* mutex_struct = malloc(sizeof(t_mutex_syscall));
    mutex_struct->contador = buffer_read_int(buf);
    mutex_struct->pid = buffer_read_uint32(buf);
    uint32_t tamanioChar = buffer_read_uint32(buf);
    mutex_struct->nombreMutex = buffer_read_string(buf, tamanioChar);
    
    //uint32_t tamanioCola = buffer_read_uint32(buf);
    //buffer_read(buf, mutex_struct->colaEspera, tamanioCola); //desp ver si funca esto
    return mutex_struct;
}

//Memo alloc
t_buffer* serializar_mem_alloc(t_mem_alloc* mem_alloc_struct){
    t_buffer* buf = buffer_create(0);
    buffer_add_uint32(buf, mem_alloc_struct->pid);
    buffer_add_uint32(buf, mem_alloc_struct->segmentoId);
    buffer_add_uint32(buf, mem_alloc_struct->tamanio);
    return buf;
}
t_mem_alloc* deserializar_mem_alloc(t_buffer* buf){
    t_mem_alloc* mem_alloc_struct = malloc(sizeof(t_mem_alloc));
    mem_alloc_struct->pid = buffer_read_uint32(buf);
    mem_alloc_struct->segmentoId = buffer_read_uint32(buf);
    mem_alloc_struct->tamanio = buffer_read_uint32(buf);
    return mem_alloc_struct;
}

//Mem free
t_buffer* serializar_mem_free(t_mem_free* mem_free_struct){
    t_buffer* buf = buffer_create(0);
    buffer_add_uint32(buf, mem_free_struct->pid);
    buffer_add_uint32(buf, mem_free_struct->segmentoId);
    return buf;
}
t_mem_free* deserializar_mem_free(t_buffer* buf){
    t_mem_free* mem_free_struct = malloc(sizeof(t_mem_free));
    mem_free_struct->pid = buffer_read_uint32(buf);
    mem_free_struct->segmentoId = buffer_read_uint32(buf);
    return mem_free_struct;
}

