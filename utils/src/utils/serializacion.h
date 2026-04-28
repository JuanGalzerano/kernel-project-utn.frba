#ifndef SERIALIZACION_H
#define SERIALIZACION_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

typedef struct {
    uint32_t pc;
    uint8_t  ax;
    uint8_t  bx;
    uint8_t  cx;
    uint8_t  dx;
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t si;   // dirección lógica de origen
    uint32_t di;   // dirección lógica de destino
} t_contexto_ejecucion;

typedef struct {
    uint32_t size;
    uint32_t offset;
    void*    stream;
} t_buffer;

typedef enum {
    OBTENER_CONTEXTO    = 0,
    ENVIAR_CONTEXTO     = 1,
    ACTUALIZAR_CONTEXTO = 2,
    OBTENER_INSTRUCCION = 3,
    MOTIVO_FIN_QUANTUM = 4,
    FINALIZAR_POR_QUANTUM = 5,
    EJECUTAR_PROCESO = 6,
} op_code;

typedef struct {
    uint8_t   codigo_operacion;
    t_buffer* buffer;
} t_paquete;

// ----- Buffer -----

t_buffer* buffer_create(uint32_t size);
void      buffer_destroy(t_buffer *buffer);

void      buffer_add(t_buffer *buffer, void *data, uint32_t size);
void      buffer_read(t_buffer *buffer, void *data, uint32_t size);

void      buffer_add_uint32(t_buffer *buffer, uint32_t data);
uint32_t  buffer_read_uint32(t_buffer *buffer);

void      buffer_add_uint8(t_buffer *buffer, uint8_t data);
uint8_t   buffer_read_uint8(t_buffer *buffer);

// El caller es responsable de liberar el string devuelto por buffer_read_string
void      buffer_add_string(t_buffer *buffer, uint32_t length, char *string);
char*     buffer_read_string(t_buffer *buffer, uint32_t *length);

// ----- Paquete -----

t_paquete* crear_paquete(op_code codigo_operacion, t_buffer* buffer);
void       enviar_paquete(int socket, t_paquete* paquete);
t_paquete* recibir_paquete(int socket);
void       eliminar_paquete(t_paquete* paquete);

// ----- Serialización de contexto de ejecución -----

t_buffer*             serializar_contexto_ctx(t_contexto_ejecucion* ctx);
t_contexto_ejecucion* deserializar_contexto_ctx(t_buffer* buf);

#endif // SERIALIZACION_H
