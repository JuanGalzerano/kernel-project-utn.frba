#include <utils/hello.h>
#include <commons/log.h>
#include <commons/collections/list.h>



int main(int argc, char* argv[]) {

    //Creo el logger
    t_log* loggerIO=log_create("io.log", "main.c", true, LOG_LEVEL_INFO);
    //Creo el config
    t_config* configIO = config_create(argv[1]);
    //Defino las variables para conectarme al scheduler
    int socketConScheduler;
    char* ip=config_get_string_value(configIO,"IP_SCHEDULER");
    char* puerto= config_get_string_value(configIO,"PUERTO_SCHEDULER");


    //Creo conexion con Scheduler
    socketConScheduler = iniciar_conexion(ip,puerto);
    if(socketConScheduler == EXIT_FAILURE){
        log_info(loggerIO, "no se pudo conectar a Kernel Scheduler");
        //ver si hay que abortar
    }

    log_info(loggerIO, "conexion establecida con Kernel Scheduler");
    
    //handshake_cliente(socketConScheduler,loggerIO);



    return 0;
}



