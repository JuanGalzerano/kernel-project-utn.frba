#include <utils/utils.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>


int main(int argc, char* argv[]) {
    
    int tipo;
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

    FILE* archivoProcesos;
    archivoProcesos = fopen(config_get_string_value(configSwap,"SWAP_FILE_PATH"), "r+b");
    ftruncate(fileno(archivoProcesos), config_get_int_value(configSwap,"SWAP_FILE_SIZE"));
    //aca podriamos con un paquete mandar el tipo de operacion que queres hacer y despues en un switchque haga lo que tenga que hacer
    while (1)
    {
        switch (tipo)
        {
        case 1 :
            //escribir_Bloque(idBloque, archivoProcesos, loggerSwap);
            break;
        
        case 2 :
            //leer_Bloque(idBloque, archivoProcesos, loggerSwap);
            break;
        }
    }
    



    fclose(archivoProcesos);
    return 0;

}

void escribir_Bloque(uint32_t  idBloque, FILE* archivo, void* contenido, t_config* configSwap ){
    
    
    int offset = idBloque * config_get_int_value(configSwap,"BLOCK_SIZE");
    fseek(archivo, offset, SEEK_SET);
    fwrite(contenido, 1, config_get_int_value(configSwap,"BLOCK_SIZE"), archivo);
}
uint32_t leer_Bloque(uint32_t  idBloque,FILE* archivo, t_config* configSwap ){
  
    void* buffer = malloc(config_get_int_value(configSwap,"BLOCK_SIZE"));
    int offset = idBloque * config_get_int_value(configSwap,"BLOCK_SIZE");
    fseek(archivo, offset, SEEK_SET);           // me posiciono al inicio del bloque
    fread(buffer, 1, config_get_int_value(configSwap,"BLOCK_SIZE"), archivo);      // leo exactamente un bloque
    
    return buffer;
}





