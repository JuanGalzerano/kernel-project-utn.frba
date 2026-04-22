/*#include <utils/utils.h>


t_buffer *buffer_create(uint32_t size){
    t_buffer buffer = malloc(sizeof(t_buffer));
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
*/