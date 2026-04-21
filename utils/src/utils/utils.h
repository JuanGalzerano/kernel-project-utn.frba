#ifndef UTILS_HELLOH
#define UTILS_HELLOH

#include <stdlib.h>
#include <stdio.h>
#include <errno.h> 
#include <string.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<commons/log.h>
#include <commons/config.h>

/**
@brief Imprime un saludo por consola
@param quien Módulo desde donde se llama a la función
@return No devuelve nada
**/

//FUNCIONES CONEXIONES
void saludar(char* quien);

int iniciar_servidor(char* puerto);

int iniciar_conexion(char* ip, char* puerto);

int esperar_cliente(int socket_escucha);

void handshake_cliente_id(int socket_conexion, t_log *log, int32_t id);

int32_t handshake_servidor_id(int socket_conexion, int32_t id);

int atender_cliente(int socketEscucha, t_log *logger);

//TYPEDEF CONEXIONES
typedef enum{
    SCHEDULER,
    MEMORY,
    CPU,
    MEMORY_STICK,
    IO,
    SWAP
}modulo;

//FUNCIONES SERIALIZACION

// Crea un buffer vacío de tamaño size y offset 0
t_buffer *buffer_create(uint32_t size);

// Libera la memoria asociada al buffer
void buffer_destroy(t_buffer *buffer);

// Agrega un stream al buffer en la posición actual y avanza el offset
void buffer_add(t_buffer *buffer, void *data, uint32_t size);

// Guarda size bytes del principio del buffer en la dirección data y avanza el offset
void buffer_read(t_buffer *buffer, void *data, uint32_t size);

// Agrega un uint32_t al buffer
void buffer_add_uint32(t_buffer *buffer, uint32_t data);

// Lee un uint32_t del buffer y avanza el offset
uint32_t buffer_read_uint32(t_buffer *buffer);

// Agrega un uint8_t al buffer
void buffer_add_uint8(t_buffer *buffer, uint8_t data);

// Lee un uint8_t del buffer y avanza el offset
uint8_t buffer_read_uint8(t_buffer *buffer);

// Agrega string al buffer con un uint32_t adelante indicando su longitud
void buffer_add_string(t_buffer *buffer, uint32_t length, char *string);

// Lee un string y su longitud del buffer y avanza el offset
char *buffer_read_string(t_buffer *buffer, uint32_t *length);

//Crea un paquete vacio y con CODE como codigo de operacion
t_paquete paquete_create(op_code code);

//TYPEDEF SERIALIZACION

typedef struct {
    uint32_t size; // Tamaño del payload
    uint32_t offset; // Desplazamiento dentro del payload
    void* stream; // Payload
} t_buffer;

typedef struct {
    uint8_t codigo_operacion;
    t_buffer* buffer;
} t_paquete;

typedef enum{

}op_code;

#endif