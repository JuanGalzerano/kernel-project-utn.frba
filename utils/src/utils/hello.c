#include <utils/hello.h>


void saludar(char* quien) {
    printf("Hola desde %s!!\n", quien);
}

int inciar_conexion(char* ip, char* puerto){
    
    
    struct addrinfo hints, *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo("127.0.0.1", "4444", &hints, &server_info);
    if(err!=0){
        //poner log o printf de que no se pudo conectar
        return EXIT_FAILURE; 
    }

    int fd_conexion = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

    if (fd_conexion == -1){
        //fallo
        return EXIT_FAILURE;
    }

    err = connect(fd_conexion, server_info->ai_addr, server_info->ai_addrlen);

    if(err == -1){
        //fallo
        return EXIT_FAILURE;
    }

    freeaddrinfo(server_info);

    return fd_conexion;
}



int iniciar_servidor(){

}
