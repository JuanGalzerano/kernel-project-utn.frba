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
/**
@brief Imprime un saludo por consola
@param quien Módulo desde donde se llama a la función
@return No devuelve nada
/
void saludar(char quien);

int iniciar_servidor(char* puerto);

int inciar_conexion(char* ip, char* puerto);

int esperar_cliente(int socket_escucha);


#endif