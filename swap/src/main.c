#include <utils/hello.h>

int main(int argc, char* argv[]) {
    
    
    //Creo el logger
    t_log* loggerSwap=log_create("swap.log", "main.c", true, LOG_LEVEL_INFO);
    //Creo el config
    t_config* configSwap = config_create(argv[1]);
    //Defino las variables para conectarme al scheduler
    int socketConMemory;
    char* ip=config_get_string_value(configSwap,"IP_MEMORY");
    char* puerto= config_get_string_value(configSwap,"PUERTO_MEMORY");


    //Creo conexion con Memory
    socketConMemory= iniciar_conexion(ip,puerto);
    if(socketConMemory == EXIT_FAILURE){
        log_info(loggerSwap, "no se pudo conectar a Kernel Memory");
        abort();
    }

    log_info(loggerSwap, "conexion establecida con Kernel Memory");
    handshake_cliente_id(socketConMemory, loggerSwap, SWAP);





    return 0;

}
