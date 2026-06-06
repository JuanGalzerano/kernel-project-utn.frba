#include <utils/utils.h>
#include <commons/string.h>

//declaro las variables usadas fuera del main
pthread_mutex_t mutex_memoria = PTHREAD_MUTEX_INITIALIZER;
int delay;
t_log* loggerMemoryStick;
void* memoria;
uint32_t tamanioMemoria;

//prototipos
void escribir_memoria(int fd, t_paquete* paquete);
void leer_memoria(int fd, t_paquete* paquete);
void* atender_cliente(void* arg);

int main(int argc, char* argv[]) {
    // Inicializo log y config
    loggerMemoryStick = log_create("MemoryStick.log","main.c",true,LOG_LEVEL_INFO);
    t_config* configMemoryStick = config_create(argv[1]);

    //traigo de config 
    char* puertoMemory = config_get_string_value(configMemoryStick,"PUERTO_MEMORY");
    char* ipMemory = config_get_string_value(configMemoryStick,"IP_MEMORY");
    char* puertoEscucha =config_get_string_value(configMemoryStick,"PUERTO_MEMORYSTICK");
    delay = config_get_int_value(configMemoryStick, "MEMORY_DELAY");

    //Reservo memoria dependiendo el tamaño recibido
    uint32_t tamano = (uint32_t)atoi (argv[2]);
    tamanioMemoria = tamano;
    memoria =malloc(tamano);
    if (memoria==NULL)
    {   
        log_info(loggerMemoryStick, "Error al reservar memoria");
        abort();
    }

    //Inicio Conexion con la memory
    int socketconexionKernelMemory = iniciar_conexion(ipMemory,puertoMemory);
    if (socketconexionKernelMemory==EXIT_FAILURE)
        {
            log_info(loggerMemoryStick,"Memory Stick no se pudo conectar a la Memory");
            abort();
        }
    log_info(loggerMemoryStick,"Memory stick conectado a Kernel Memory");
    handshake_cliente_id(socketconexionKernelMemory, loggerMemoryStick, MEMORY_STICK);
    
    //Le envio a la memory el tamaño del stick
    send(socketconexionKernelMemory,&tamano,sizeof(uint32_t),0);

    //Recibo el puerto correcto;
    uint32_t puertoAsignado;
    recv(socketconexionKernelMemory, &puertoAsignado, sizeof(uint32_t), MSG_WAITALL);

    //hilo para atender escritura y lectura de la memory
    pthread_t hiloMemory;
    int* fdMemory = malloc(sizeof(int));
    *fdMemory = socketconexionKernelMemory;
    pthread_create(&hiloMemory, NULL, atender_cliente, fdMemory);
    pthread_detach(hiloMemory);

    //abrir socket de escucha para que se conecte la cpu
    int socketEscucha = iniciar_servidor(string_itoa(puertoAsignado)); //string_itoa convierte 7 en "7"
    if(socketEscucha == EXIT_FAILURE){
        log_info(loggerMemoryStick, "no se pudo iniciar servidor");
        abort();
    }
    log_info(loggerMemoryStick, "Servidor iniciado");

    //aceptar cpus
    while(1){
        int socketParaCpu = aceptar_cliente(socketEscucha, loggerMemoryStick); //MAL creo
        if (socketParaCpu== EXIT_FAILURE)
        {
        log_info(loggerMemoryStick, "Error al aceptar conexion con la cpu");
        abort();
        }
        pthread_t hilo;
        int *fd = malloc(sizeof(int));
        *fd = socketParaCpu;
        pthread_create(&hilo, NULL, atender_cliente, fd);
        pthread_detach(hilo);
    }
    

    free(memoria);
    config_destroy(configMemoryStick);
    return 0;
}

//funcion para escribir
void escribir_memoria(int fd, t_paquete* paquete){
    uint32_t direccion = buffer_read_uint32(paquete->buffer);
    uint32_t tamanio = buffer_read_uint32(paquete->buffer);

    // Validacion: la escritura no puede salirse del stick
    if (direccion > tamanioMemoria || tamanio > tamanioMemoria - direccion) {
        log_error(loggerMemoryStick, "Escritura fuera de rango: dir %d + tam %d > memoria %d", direccion, tamanio, tamanioMemoria);
        t_paquete* respuesta = crear_paquete(ESCRITURA_FALLIDA, NULL);
        enviar_paquete(fd, respuesta);
        eliminar_paquete(respuesta);
        return;
    }

    void* datos = malloc(tamanio);
    if (datos == NULL) {
        log_error(loggerMemoryStick, "No se pudo reservar %d bytes para la escritura", tamanio);
        t_paquete* respuesta = crear_paquete(ESCRITURA_FALLIDA, NULL);
        enviar_paquete(fd, respuesta);
        eliminar_paquete(respuesta);
        return;
    }
    buffer_read(paquete->buffer, datos, tamanio);

    pthread_mutex_lock(&mutex_memoria);
    memcpy(memoria + direccion, datos, tamanio);
    pthread_mutex_unlock(&mutex_memoria);
    free(datos);

    t_buffer* buf = buffer_create(0);
    buffer_add_uint32(buf, 0);
    t_paquete* respuesta = crear_paquete(ESCRIBIR_BYTES, buf);
    enviar_paquete(fd, respuesta);
    eliminar_paquete(respuesta);

    log_info(loggerMemoryStick, "## Escritura de %d bytes", tamanio);
}
//funcion para leer
void leer_memoria(int fd, t_paquete* paquete){
    uint32_t direccion = buffer_read_uint32(paquete->buffer);
    uint32_t tamanio = buffer_read_uint32(paquete->buffer);

    // Validacion: la lectura no puede salirse del stick
    if (direccion > tamanioMemoria || tamanio > tamanioMemoria - direccion) {
        log_error(loggerMemoryStick, "Lectura fuera de rango: dir %d + tam %d > memoria %d", direccion, tamanio, tamanioMemoria);
        t_paquete* respuesta = crear_paquete(LECTURA_FALLIDA, NULL);
        enviar_paquete(fd, respuesta);
        eliminar_paquete(respuesta);
        return;
    }

    void* datos = malloc(tamanio);
    if (datos == NULL) {
        log_error(loggerMemoryStick, "No se pudo reservar %d bytes para la lectura", tamanio);
        t_paquete* respuesta = crear_paquete(LECTURA_FALLIDA, NULL);
        enviar_paquete(fd, respuesta);
        eliminar_paquete(respuesta);
        return;
    }

    pthread_mutex_lock(&mutex_memoria);
    memcpy(datos, memoria + direccion, tamanio);
    pthread_mutex_unlock(&mutex_memoria);

    t_buffer* buf = buffer_create(0);
    buffer_add(buf, datos, tamanio);
    t_paquete* respuesta = crear_paquete(LEER_BYTES, buf);
    enviar_paquete(fd, respuesta);
    eliminar_paquete(respuesta);
    free(datos);

    log_info(loggerMemoryStick, "## Lectura de %d bytes", tamanio);
}
//funcion atender los
void* atender_cliente(void* arg){
    int fd = *(int*)arg;
    free(arg);

    while(1){
        t_paquete* paquete = recibir_paquete(fd);
        if(paquete == NULL) break;

        usleep(delay * 1000);

        switch(paquete->codigo_operacion){
            case LEER_BYTES:
                leer_memoria(fd, paquete);
                break;
            case ESCRIBIR_BYTES:
                escribir_memoria(fd, paquete);
                break;
            default:
                log_error(loggerMemoryStick, "Op code desconocido: %d", paquete->codigo_operacion);
                break;
        }
        eliminar_paquete(paquete);
    }
    log_info(loggerMemoryStick, "Cliente desconectado");
    close(fd);
    return NULL;
}