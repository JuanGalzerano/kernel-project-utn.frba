#include <utils/hello.h>

int main(int argc, char* argv[]) {
    // Inicializo log y config
    t_log* loggerMemoryStick = log_create("MemoryStick.log","main.c",true,LOG_LEVEL_INFO);
    t_config* configMemoryStick = config_create(argv[1]);

    //traigo 
    char* puertoMemory = config_get_string_value(configMemoryStick,"PUERTO_MEMORY");
    char* ipMemory = config_get_string_value(configMemoryStick,"IP_MEMORY");
    char* puertoEscucha =config_get_string_value(configMemoryStick,"PUERTO_ESCUCHA");

    //Inicio Conexion con la memory
    int socketconexionMemory = iniciar_conexion(ipMemory,puertoMemory);
    if (socketconexionMemory==EXIT_FAILURE)
        {
            log_info(loggerMemoryStick,"Memory Stick no se pudo conectar a la Memory");
            abort();
        }
    log_info(loggerMemoryStick,"Memory stick conectado a Kernel Memory");
    
    //handshake
    handshake_cliente(socketconexionMemory,loggerMemoryStick);

    //abrir socket de escucha para que se conecte la cpu
    int socketEscucha = iniciar_servidor(puertoEscucha);

    log_info(loggerMemoryStick, "Servidor iniciado");
    //esperar cpu
    int socketCPU = esperar_cliente(socketEscucha);
    if(socketCPU==EXIT_FAILURE)
        {
            log_info(loggerMemoryStick,"Error al iniciar Sevidor(Memory Stick)");
            abort();
        }
    
    log_info(loggerMemoryStick,"Memory Stick: CPU <ID CPU> Conectada");
    //handshake cpu
    handshake_servidor(socketCPU);
    return 0;
}
