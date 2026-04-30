#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <errno.h> 
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/config.h>
#include <pthread.h>
#include "serializacion.h"

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

int aceptar_cliente(int socketEscucha, t_log *logger);

//TYPEDEF CONEXIONES
typedef enum{
    SCHEDULER,
    MEMORY,
    CPU,
    MEMORY_STICK,
    IO,
    SWAP
}modulo;


#endif