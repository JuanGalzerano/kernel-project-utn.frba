#include <utils/utils.h>

int main(int argc, char* argv[]) {
    // Inicializo log y config
    t_log* loggerMemoryStick = log_create("MemoryStick.log","main.c",true,LOG_LEVEL_INFO);
    t_config* configMemoryStick = config_create(argv[1]);

    //traigo 
    char* puertoMemory = config_get_string_value(configMemoryStick,"PUERTO_MEMORY");
    char* ipMemory = config_get_string_value(configMemoryStick,"IP_MEMORY");
    char* puertoEscucha =config_get_string_value(configMemoryStick,"PUERTO_MEMORYSTICK");

    //Inicio Conexion con la memory
    int socketconexionKernelMemory = iniciar_conexion(ipMemory,puertoMemory);
    if (socketconexionKernelMemory==EXIT_FAILURE)
        {
            log_info(loggerMemoryStick,"Memory Stick no se pudo conectar a la Memory");
            abort();
        }
    log_info(loggerMemoryStick,"Memory stick conectado a Kernel Memory");
    handshake_cliente_id(socketconexionKernelMemory, loggerMemoryStick, MEMORY_STICK);
    
  

    //abrir socket de escucha para que se conecte la cpu
    int socketEscucha = iniciar_servidor(puertoEscucha);
    if(socketEscucha == EXIT_FAILURE){
        log_info(loggerMemoryStick, "no se pudo iniciar servidor");
        abort();
    }
    log_info(loggerMemoryStick, "Servidor iniciado");

    int socketParaCpu = aceptar_cliente(socketEscucha, loggerMemoryStick);


    
    return 0;
}
