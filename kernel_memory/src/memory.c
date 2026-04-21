#include "memory.h"

int main(int argc, char* argv[]) {


    //inicializo log y config, desp lo pongo en archivo aparte
    inicializar_log_y_config(argv[1]);
    

    //guardo el valor del puerto de escucha que esta en el config
    puertoEscucha = config_get_string_value(configMemory, "PUERTO_MEMORY");

    // LEVANTAR SERVIDOR (espera conexiones de Kernel, CPU, Memory Stick y SWAP)
    int socketEscucha = iniciar_servidor(puertoEscucha);
    if(socketEscucha == EXIT_FAILURE){
        log_info(loggerMemory, "No se pudo iniciar el servidor");
    }
    log_info(loggerMemory, "Servidor iniciado");

    int unSocket = atender_cliente(socketEscucha, loggerMemory);
    int otroSocket = atender_cliente(socketEscucha, loggerMemory);
    int masSocket = atender_cliente(socketEscucha, loggerMemory);
    int elUltimoSocket = atender_cliente(socketEscucha, loggerMemory);



/*
        // ESPERAR KERNEL SCHEDULER
    int socketKernel = esperar_cliente(socketEscucha);
    if(socketKernel == EXIT_FAILURE){
        log_info(loggerMemory, "error al conectar con Kernel-Scheduler");
    }
    log_info(loggerMemory, "Kernel Scheduler Conectado - FD del socket: <FD_DEL_SOCKET>"); //a implementar el <FD_DEL_SOCKET>
    //handshake_servidor(socketKernel);//agregao por marotti


        // ESPERAR CPU
    int socketCPU = esperar_cliente(socketEscucha);
    if(socketCPU == EXIT_FAILURE){
        log_info(loggerMemory, "error al conectar con CPU");
    }
    log_info(loggerMemory, "CPU <ID CPU> Conectada"); //a implementar el <ID CPU>


        // ESPERAR MEMORY STICK
    int socketMemoryStick = esperar_cliente(socketEscucha);
    if(socketMemoryStick == EXIT_FAILURE){
        log_info(loggerMemory, "error al conectar con Memory Stick");
    }
    log_info(loggerMemory, "Memory Stick de <TAMAÑO> bytes Conectada");//a implementar el <TAMAÑO>


        // ESPERAR SWAP
    int socketSwap = esperar_cliente(socketEscucha);
    if(socketSwap == EXIT_FAILURE){
        log_info(loggerMemory, "error al conectar con Swap");
    }
    log_info(loggerMemory, "Swap conectado");*/

    return 0;
}
