#include <utils/hello.h>

void saludar(char* quien) {
    printf("Hola desde %s!!\n", quien);
}


int esperar_cliente(int socket_escucha){


	int socket_conexion = accept(socket_escucha, NULL, NULL);
    if(socket_conexion == -1){
        printf("Error al aceptar al cliente\n%s", strerror(errno)); //ver si es print f o log
        return EXIT_FAILURE;
    }
    

	return socket_conexion;
}

int iniciar_servidor(char* puerto){

    int err;

    struct addrinfo hints, *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;


    err = getaddrinfo(NULL, puerto, &hints, &server_info);
    if(err != 0){
        printf("Error en getaddrinfo() %s\n", gai_strerror(err)); // ver si es printf o log
        return EXIT_FAILURE;
    }


    int socket_escucha = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (socket_escucha == -1) {
        printf("Error en creacion de socket de escucha\n%s", strerror(errno)); // ver si es printf o log
        return EXIT_FAILURE;
    }


    err = setsockopt(socket_escucha, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int));
    if(err == -1){
        printf("Error en setsockopt()\n%s", strerror(errno)); // preguntar como ponerle al error // ver si es printf o log
        return EXIT_FAILURE;
    }


    err = bind(socket_escucha, server_info->ai_addr, server_info->ai_addrlen);
    if(err == -1){
        printf("Error al conectarse con el puerto\n%s", strerror(errno)); // ver si es printf o log
        return EXIT_FAILURE;
    }


    err = listen(socket_escucha, SOMAXCONN);
    if(err == -1){
        printf("Error al intentar escuchar conexiones\n%s", strerror(errno)); // ver si es printf o log
        return EXIT_FAILURE;
    }


    freeaddrinfo(server_info);

    return socket_escucha;
}




int iniciar_conexion(char* ip, char* puerto){


    struct addrinfo hints, *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;


    int err = getaddrinfo(ip, puerto, &hints, &server_info);
    if(err!=0){
        printf("getaddrinfo error: %s\n", gai_strerror(err)); // ver si es printf o log
        return EXIT_FAILURE; 
    }


    int socket_conexion = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (socket_conexion == -1){
        printf("Socket creation error\n%s", strerror(errno)); // ver si es printf o log
        return EXIT_FAILURE;
    }


    err = connect(socket_conexion, server_info->ai_addr, server_info->ai_addrlen);
    if(err == -1){
        printf("Connect error\n%s", strerror(errno));  // ver si es printf o log
        // agregamos un close(socket_conexion) por no haberse podido conectar?
        return EXIT_FAILURE;
    }

    freeaddrinfo(server_info);

    return socket_conexion;
}


//FUNCIONES HANDSHAKE (TODAVIA EVALLUANDO FUNCIONAMIENTO)
/*
IMPORTANTE:
    EL unico problema que le encuentro a las funciones de handshake de aca abajo es que en caso de fallar el handshake, el cliente hara exit(1)
y el servidor se quedara esperando el id por el recv(... WAITALL), por lo que el servidor nunca se enterara del fallo
*/


void handshake_cliente_id(int socket_conexion, t_log *log, int32_t id)
{
	int32_t handshake = 1;
	int32_t result = 0;
  
	send(socket_conexion, &handshake, sizeof(int32_t), 0);
	recv(socket_conexion, &result, sizeof(int32_t), MSG_WAITALL);

	if (result == 0)
	{
		log_info(log, "Handshake OK");
	}
	else
	{
		perror("Error al realizar Handshake");
		exit(1);
	}
    send(socket_conexion, &id, sizeof(int32_t), 0);
}

int32_t handshake_servidor_id(int socket_conexion, int32_t id)
{
	int32_t handshake;
	int32_t resultOk = 0;
	int32_t resultError = -1;

	recv(socket_conexion, &handshake, sizeof(int32_t), MSG_WAITALL);
	if (handshake == 1)
	{
		send(socket_conexion, &resultOk, sizeof(int32_t), 0);
	}
	else
	{
		send(socket_conexion, &resultError, sizeof(int32_t), 0);
	}
    recv(socket_conexion, &id, sizeof(int32_t), MSG_WAITALL);
    return id;
}




