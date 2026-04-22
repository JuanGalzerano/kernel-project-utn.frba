#include <cpu.h>

int main(int argc, char* argv[]) {

    //Verifico haber recibido todos los argumentos
    if(argc < 3){
        fprintf(stderr, "Faltan argumentos\n"); 
        return EXIT_FAILURE; 
    }

    //inicializo log y config
    inicializar_log_y_config(argv[1], argv[2]);

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
    
    handshake_cliente_id(socketConexionMemoryStick, loggerCpu, CPU);





    return 0;
}
