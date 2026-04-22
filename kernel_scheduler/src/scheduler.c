#include <scheduler.h>



int main(int argc, char* argv[]) { //argv[1]: Path al config, argv[2]: path al proceso inicial. [ejs: ./bin/kernel_scheduler kernel.config ./procesos/init.prog]

    
    inicializar(argv[1]); //loggers y configs

//LEVANTAR CONEXION CON MEMORY
    int socketConexionMemory = iniciar_conexion(IPMemory, puertoMemory);
    if(socketConexionMemory == EXIT_FAILURE){
        log_info(loggerScheduler, "no se pudo conectar a Kernel Memory");
        abort();//ver si hay que abortar
    }

    log_info(loggerScheduler, "Conectado a Kernel Memory");
    
    //hacer handshake cuando juani implemente:
    handshake_cliente_id(socketConexionMemory, loggerScheduler, SCHEDULER);
   
//LEVANTAR SERVIDOR
    int socketEscucha = iniciar_servidor(puertoEscucha);
    if(socketEscucha == EXIT_FAILURE){
        log_info(loggerScheduler, "No se pudo iniciar el servidor");
        abort();//ver si hay que abortar
    }
    log_info(loggerScheduler, "Servidor iniciado");


    while(1){
        int socketCliente = aceptar_cliente(socketEscucha, loggerScheduler);

        int* socket_ptr = malloc(sizeof(int));
        *socket_ptr = socketCliente;

        pthread_t hilo;
        pthread_create(&hilo, NULL, atender_cliente, socket_ptr);
        pthread_detach(hilo);
    }


//esto va con hilos cuando lo aprenda
    //int unSocket = aceptar_cliente(socketEscucha, loggerScheduler);
    //int otroSocket = aceptar_cliente(socketEscucha, loggerScheduler);

    //Liberamos memoria
    close(socketConexionMemory);
    config_destroy(configScheduler);
    log_destroy(loggerScheduler);

    return 0;
}


void *atender_cliente(void *arg){//lo que recibe es el socket cliente (con el que se comunican)
    int socketCliente = *(int*) arg;
    free(arg); 

    //atender peticion o algo asi

    close(socketCliente);
}



















/*

    //SUPONIENDO QUE CPU SE CONECTA ANTES QUE IO (cuando aprenda hilos lo cambiamos)

    int socketCPU = esperar_cliente(socketEscucha);

    if(socketCPU == EXIT_FAILURE){
        log_info(loggerScheduler, "error al conectar CPU");
        abort();//ver si hay que abortar
    }
    log_info(loggerScheduler, "CPU <id cpu> CONECTADA");//OBLIGATORIO //agregar despues lo de loggear el id de la cpu 

    //handshake_servidor(socketCPU);


    int socketIO = esperar_cliente(socketEscucha);
    if(socketIO == EXIT_FAILURE){
        log_info(loggerScheduler, "error al conectar IO");
        abort();//ver si hay que abortar
    }
    log_info(loggerScheduler, "IO CONECTADO"); 

    //handshake_servidor(socketIO);

*/