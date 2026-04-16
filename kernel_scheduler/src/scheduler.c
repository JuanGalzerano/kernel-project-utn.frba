#include <scheduler.h>

int main(int argc, char* argv[]) { //argv[1]: Path al config, argv[2]: path al proceso inicial. [ejs: ./bin/kernel_scheduler kernel.config ./procesos/init.prog]

    
    inicializar(argv[1]);




//LEVANTAR CONEXION CON MEMORY
    int socketConexionMemory = iniciar_conexion(IPMemory, puertoMemory);
    if(socketConexionMemory == EXIT_FAILURE){
        log_info(loggerScheduler, "no se pudo conectar a Kernel Memory");
        abort();//ver si hay que abortar
    }

    log_info(loggerScheduler, "Conectado a Kernel Memory");
    
    //handshake_cliente(socketConexionMemory, loggerScheduler);
   


//LEVANTAR SERVIDOR
    int socketEscucha = iniciar_servidor(puertoEscucha);
    if(socketEscucha == EXIT_FAILURE){
        log_info(loggerScheduler, "No se pudo iniciar el servidor");
        abort();//ver si hay que abortar
    }
    log_info(loggerScheduler, "Servidor iniciado");



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



    //Liberamos memoria
    close(socketConexionMemory);
    config_destroy(configScheduler);
    log_destroy(loggerScheduler);

    return 0;
}
