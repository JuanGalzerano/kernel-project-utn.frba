#include "swap.h"
#include <unistd.h>
#include <fcntl.h>

uint32_t blockSize;

int main(int argc, char* argv[]) {

    t_config* configSwap = config_create(argv[1]);

    t_log_level logLevelSwap = log_level_from_string(config_get_string_value(configSwap, "LOG_LEVEL"));

    t_log* loggerSwap = log_create("swap.log", "swap", true, logLevelSwap);
    

    char*ip = config_get_string_value(configSwap, "IP_MEMORY");
    char* puerto = config_get_string_value(configSwap, "PUERTO_MEMORY");
    char* swapFilePath = config_get_string_value(configSwap, "SWAP_FILE_PATH");
    uint32_t swapFileSize = (uint32_t)config_get_int_value(configSwap, "SWAP_FILE_SIZE");
    blockSize = (uint32_t)config_get_int_value(configSwap, "BLOCK_SIZE");

    // Abrir/crear el archivo de swap con el tamanio correcto
    FILE* archivo = fopen(swapFilePath, "w+b");
    if (!archivo) {
        log_error(loggerSwap, "No se pudo abrir el archivo de swap: %s", swapFilePath);
        return EXIT_FAILURE;
    }
    ftruncate(fileno(archivo), swapFileSize);//para que el archivo tenga los bytes que dice swapFileSize
    log_info(loggerSwap, "Swap file listo: %s (%d bytes, bloques de %d)", swapFilePath, swapFileSize, blockSize);

    // Conectar con Kernel Memory
    int socketConMemory = iniciar_conexion(ip, puerto);
    if (socketConMemory == EXIT_FAILURE) {
        log_error(loggerSwap, "No se pudo conectar a Kernel Memory");
        return EXIT_FAILURE;
    }
    handshake_cliente_id(socketConMemory, loggerSwap, SWAP);

    //Anunciatamaño total y tamanio de bloque a Kernel Memory
    t_buffer* bufInit = buffer_create(0);
    buffer_add_uint32(bufInit, swapFileSize);
    buffer_add_uint32(bufInit, blockSize);
    t_paquete* paqInit = crear_paquete(SWAP_INIT, bufInit);
    enviar_paquete(socketConMemory, paqInit);
    eliminar_paquete(paqInit);
    log_info(loggerSwap, "SWAP_INIT enviado (%d bytes, bloques de %d)", swapFileSize, blockSize);

    //atender LEER_SWAP y ESCRIBIR_SWAP
    t_paquete* paquete;
    while((paquete = recibir_paquete(socketConMemory)) != NULL){
        switch ((op_code)paquete->codigo_operacion) {

            case ESCRIBIR_SWAP:{
                uint32_t bloque = buffer_read_uint32(paquete->buffer);
                uint32_t tamanio = buffer_read_uint32(paquete->buffer);
                void*    datos = malloc(tamanio);
                buffer_read(paquete->buffer, datos, tamanio);
                eliminar_paquete(paquete);

                escribir_Bloque(bloque, archivo, datos, tamanio);
                free(datos);

                log_info(loggerSwap, "ESCRIBIR_SWAP bloque %d (%d bytes)", bloque, tamanio);

                t_paquete* resp = crear_paquete(ESCRIBIR_SWAP, NULL);
                enviar_paquete(socketConMemory, resp);
                eliminar_paquete(resp);
                break;
            }

            case LEER_SWAP: {
                uint32_t bloque = buffer_read_uint32(paquete->buffer);
                uint32_t tamanio = buffer_read_uint32(paquete->buffer);
                eliminar_paquete(paquete);

                char* datos = leer_Bloque(bloque, archivo, tamanio);

                log_info(loggerSwap, "LEER_SWAP bloque %d (%d bytes)", bloque, tamanio);

                t_buffer* buf = buffer_create(0);
                buffer_add(buf, datos, tamanio);
                free(datos);
                t_paquete* resp = crear_paquete(LEER_SWAP, buf);
                enviar_paquete(socketConMemory, resp);
                eliminar_paquete(resp);
                break;
            }

            default:
                log_warning(loggerSwap, "Opcode desconocido: %d", paquete->codigo_operacion);
                eliminar_paquete(paquete);
                break;
        }
    }

    log_warning(loggerSwap, "Kernel Memory desconectado");
    fclose(archivo);
    config_destroy(configSwap);
    log_destroy(loggerSwap);
    return 0;
}

void escribir_Bloque(uint32_t idBloque, FILE* archivo, void* contenido, uint32_t tamanio){
    long offset = idBloque * blockSize;
    fseek(archivo, offset, SEEK_SET);
    fwrite(contenido, 1, tamanio, archivo);
    fflush(archivo);
}

char* leer_Bloque(uint32_t idBloque, FILE* archivo, uint32_t tamanio){
    char* buffer = malloc(tamanio);
    long offset  = idBloque * blockSize;
    fseek(archivo, offset, SEEK_SET);
    fread(buffer, 1, tamanio, archivo);
    return buffer;
}