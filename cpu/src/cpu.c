#include <cpu.h>

int main(int argc, char* argv[]) {

    //inicializo log y config
    inicializar_log_y_config(argv[1]);

    //Meto todo lo del config
    char* puertoKernel= config_get_string_value(configCpu, "PUERTO_SCHEDULER");
    char* IPKernel = config_get_string_value(configCpu, "IP_SCHEDULER");
    char* puertoMemory= config_get_string_value(configCpu, "PUERTO_MEMORY");
    char* IPMemory = config_get_string_value(configCpu, "IP_MEMORY");
    char* puertoMemoryStick= config_get_string_value(configCpu, "PUERTO_MEMORYSTICK");
    char* IPMemoryStick = config_get_string_value(configCpu, "IP_MEMORYSTICK");


    //LEVANTAR CONEXION CON MEMORY
    int socketConexionMemory = iniciar_conexion(IPMemory, puertoMemory);
    if(socketConexionMemory == EXIT_FAILURE){
        log_info(loggerCpu, "CPU no se pudo conectar a Kernel Memory");
        abort();
    }
    log_info(loggerCpu, "CPU: conexion establecida con Kernel Memory");

    handshake_cliente_id(socketConexionMemory, loggerCpu, CPU);
    
    //LEVANTAR CONEXION CON SCHEDULER
    int socketConexionKernel = iniciar_conexion(IPKernel, puertoKernel);
    if(socketConexionKernel == EXIT_FAILURE){
        log_info(loggerCpu, "CPU no se pudo conectar a Kernel Scheduler");
        abort();
    }
    log_info(loggerCpu, "CPU: conexion establecida con Kernel Scheduler");

    handshake_cliente_id(socketConexionKernel, loggerCpu, CPU);


        //LEVANTAR CONEXION CON MEMORY STICK
    int socketConexionMemoryStick = iniciar_conexion(IPMemoryStick, puertoMemoryStick);
    if(socketConexionMemoryStick == EXIT_FAILURE){
        log_info(loggerCpu, "CPU no se pudo conectar a Memory Stick");
        abort();
    }

    log_info(loggerCpu, "CPU: conexion establecida con Memory Stick");
    
    //handshake_cliente_id(socketConexionMemory, loggerCpu, CPU);





    return 0;
}
