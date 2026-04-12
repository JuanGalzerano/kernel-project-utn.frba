#include <utils/hello.h>

int main(int argc, char* argv[]) {


    //inicializo log y config, desp lo pongo en archivo aparte

    t_log* loggerMemory = log_create("memory.log", "main.c", true, LOG_LEVEL_INFO);
    t_config* configMemory = config_create(argv[1]);
    

    //guardo el valor del puerto de escucha que esta en el config
    char* puertoEscucha = config_get_string_value(configMemory, "PUERTO_ESCUCHA");

    // LEVANTAR SERVIDOR (espera conexiones de Kernel, CPU, Memory Stick y SWAP)
    int socketEscucha = iniciar_servidor(puertoEscucha);
    if(socketEscucha == EXIT_FAILURE){
        log_info(loggerMemory, "No se pudo iniciar el servidor");
    }
    log_info(loggerMemory, "Servidor iniciado");

    // ESPERAR KERNEL SCHEDULER
    int socketKernel = esperar_cliente(socketEscucha);
    if(socketKernel == EXIT_FAILURE){
    log_info(loggerMemory, "error al conectar con Kernel-Scheduler");
    }
    log_info(loggerMemory, "Kernel-Scheduler conectado");

    // ESPERAR CPU
    int socketCPU = esperar_cliente(socketEscucha);
    if(socketCPU == EXIT_FAILURE){
        log_info(loggerMemory, "error al conectar con CPU");
    }
    log_info(loggerMemory, "CPU conectado");

    // ESPERAR MEMORY STICK
    int socketMemoryStick = esperar_cliente(socketEscucha);
    if(socketMemoryStick == EXIT_FAILURE){
        log_info(loggerMemory, "error al conectar con Memory Stick");
    }
    log_info(loggerMemory, "Memory Stick conectado");

    // ESPERAR SWAP
    int socketSwap = esperar_cliente(socketEscucha);
    if(socketSwap == EXIT_FAILURE){
        log_info(loggerMemory, "error al conectar con Swap");
    }
    log_info(loggerMemory, "Swap conectado");

    return 0;
    }
