#include <utils/hello.h>

int main(int argc, char* argv[]) {

    //inicializo log y config
    t_log* loggerCpu = log_create("cpu.log", "main.c", true, LOG_LEVEL_INFO);
    t_config* configCpu = config_create(argv[1]);
    //Meto todo lo del config
    char* puertoKernel= config_get_string_value(configCpu, "PUERTO_KERNEL");
    char* IPKernel = config_get_string_value(configCpu, "IP_KERNEL");
    char* puertoMemory= config_get_string_value(configCpu, "PUERTO_MEMORY");
    char* IPMemory = config_get_string_value(configCpu, "IP_MEMORY");


    //LEVANTAR CONEXION CON MEMORY
    int socketConexionMemory = iniciar_conexion(IPMemory, puertoMemory);
    if(socketConexionMemory == EXIT_FAILURE){
        log_info(loggerCpu, "CPU no se pudo conectar a Kernel Memory");
        //ver si hay que abortar
    }

    log_info(loggerCpu, "CPU: conexion establecida con Kernel Memory");
    //hacer handshake


    //LEVANTAR CONEXION CON KERNEL
    int socketConexionKernel = iniciar_conexion(IPKernel, puertoKernel);
    if(socketConexionKernel == EXIT_FAILURE){
        log_info(loggerCpu, "CPU no se pudo conectar a Kernel");
        //ver si hay que abortar
    }

    log_info(loggerCpu, "CPU: conexion establecida con Kernel");
    //hacer handshake

    return 0;
}
