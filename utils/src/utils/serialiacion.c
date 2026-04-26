#include <utils/utils.h>


t_buffer *buffer_create(uint32_t size){
    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer->size = size;
    buffer->offset = 0;
    buffer->stream=NULL;
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
    buffer_add(buffer, data, sizeof(uint32_t));
}

uint32_t buffer_read_uint32(t_buffer *buffer){
    uint32_t numero;
    buffer_read(buffer, &numero, sizeof(uint32_t));
    return numero;
}

void buffer_add_uint8(t_buffer *buffer, uint8_t data){
    buffer_add(buffer, data, sizeof(uint8_t));
}

uint8_t buffer_read_uint8(t_buffer *buffer){
    uint8_t numero;
    buffer_read(buffer, &numero, sizeof(uint8_t));
    return numero;
}

void buffer_add_string(t_buffer *buffer, uint32_t length, char *string){
    buffer_add(buffer, string, length +1);
}

char *buffer_read_string(t_buffer *buffer, uint32_t *length){
    char *cadena = malloc(*length + 1);
    buffer_read(buffer, cadena, *length+1);
    return cadena;
    //hacer el free en donde se usa la func
}


//Funciones de paquete


//Crear paquete
t_paquete* crear_paquete(op_code codigo_operacion) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = codigo_operacion;
    paquete->buffer = buffer_create(0);
    return paquete;
}

//Agregar datos al buffer de un paquete
void agregar_a_paquete(t_paquete* paquete, void* valor, uint32_t size) {
	buffer_add(paquete->buffer, valor, size);
}

//Enviar paquete
void enviar_paquete(int socket, t_paquete* paquete) {
    uint8_t op = paquete->codigo_operacion;
    uint32_t size = paquete->buffer->offset;
    send(socket, &op;, sizeof(uint8_t), 0);
    send(socket, &size;, sizeof(uint32_t), 0);
    if (size > 0) {
        send(socket, paquete->buffer->stream, size, 0);
    }
}

//Recibir paquete
t_paquete* recibir_paquete(int socket, uint8_t* op_out) {
	recv(socket, op_out, sizeof(uint8_t), MSG_WAITALL);
	uint32_t size;
	recv(socket, &size;, sizeof(uint32_t), MSG_WAITALL);
	t_buffer* buf = buffer_create(size);
	buf->stream = malloc(size);
	recv(socket, buf->stream, size, MSG_WAITALL);

    t_paquete* paquete = crear_paquete(op_out);
    paquete->buffer = buf;

    return paquete;
}

//Eliminar paquete
void eliminar_paquete(t_paquete* paquete) {
    buffer_destroy(paquete->buffer);
    free(paquete);
}