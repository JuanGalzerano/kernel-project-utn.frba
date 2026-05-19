#include <utils/utils.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <io.h>

tipo_IO tipo;

int main(int argc, char* argv[]) {

    //Creo el logger
    t_log* loggerIO=log_create("io.log", "main.c", true, LOG_LEVEL_INFO);
    //Creo el config
    t_config* configIO = config_create(argv[1]);
    //Defino las variables para conectarme al scheduler
    int socketConScheduler;
    char* ip=config_get_string_value(configIO,"IP_SCHEDULER");
    char* puerto= config_get_string_value(configIO,"PUERTO_SCHEDULER");
    tipo = reconocer_io(argv[2]);


    //Creo conexion con Scheduler
    socketConScheduler = iniciar_conexion(ip,puerto);
    if(socketConScheduler == EXIT_FAILURE){
        log_info(loggerIO, "no se pudo conectar a Kernel Scheduler");
        abort();
    }

    log_info(loggerIO, "conexion establecida con Kernel Scheduler");
    handshake_cliente_id(socketConScheduler, loggerIO, IO);
    send(socketConScheduler, &tipo, sizeof(tipo_IO), 0);

    //recibir paquete
    t_paquete* paquete;
    paquete = recibir_paquete(socketConScheduler);
    //aca tendria que deserializarlo pero ni idea como, lo veo mas adelatne
    log_info(loggerIO, "## PID: <PID> - Inicio de IO");
    switch (tipo)
    {
    case TIPO_SLEEP:
        t_sleep* sl;
        sl = deserializar_sleep(paquete->buffer);
        sleep_func(sl->tiempoADormir,sl->pid,loggerIO);
        send(socketConScheduler,FINALIZAR_SLEEP,sizeof(op_code),0);// tenes que mandarme el PID en un paquete
        break;
    case TIPO_STDIN:
        t_stdin_stdout* in;
        in = deserializar_stdin(paquete->buffer);
        in->cadenaLeida=stdin_func(in->bytesALeer,in->pid,loggerIO);
        paquete->buffer=serializar_stdin(in);
        enviar_paquete(socketConScheduler,paquete);
        free(in->cadenaLeida);
        free(in);
        break;
    case TIPO_STDOUT:
       
        t_stdout out;
        //out = deserializar_out(paquete->buffer)
        //stdout_func();//faltan argumentos
        send(socketConScheduler,FINALIZAR_STDOUT,sizeof(op_code),0);// tenes que mandarme el PID en un paquete
        break;
    default:
       
        break;
    }
    log_info(loggerIO, "## PID: <PID> - Fin de IO");//falta poner que pid termino
    return 0;
}
/*
void stdin_func(int cantBytes,int pid, t_log* logIO){
    char* mensaje;
    log_info(logIO, "## PID: %d - Ingrese %d caracteres:", pid, cantBytes);
    mensaje = leer_stdin(cantBytes);
}
void stdout_func(char* mensaje,int pid ,int cantBytes,t_log* logIO){
    // Crear buffer temporal con '\0' al final para el log
    char* buffer_log = malloc(out->bytesAEscribir + 1);
    memcpy(buffer_log, mensaje, cantBytes);
    fwrite(mensaje, sizeof(char), cantBytes, stdout);
    printf("\n"); // salto de línea al final
    log_info(logIO, "## PID: %d - %s",pid, buffer_log);

}*/

void sleep_func(uint32_t cantTiempoMicro,uint32_t pid,t_log* logIO){
    usleep(cantTiempoMicro*1000);
    log_info(logIO, "## PID: %d - Haciendo sleep por %d milisegundos.", pid, cantTiempoMicro);
}

tipo_IO reconocer_io(char* tipo){
    tipo_IO io;
   if (strcmp(tipo, "SLEEP") == 0) {
        io = TIPO_SLEEP;
    } else if (strcmp(tipo, "STDIN") == 0) {
        io = TIPO_STDIN;
    } else if (strcmp(tipo, "STDOUT") == 0) {
        io = TIPO_STDOUT;
    } else {
        //tendria que agregar algo para controlar pero xd lol equisde
    }
    return io;
}

char* leer_stdin(int cantidad_bytes) {
    char* buffer = malloc(cantidad_bytes + 1); // +1 para el '\0' de fgets
    if (buffer == NULL) return NULL;

    // Inicializar todo en '\0'
    memset(buffer, '\0', cantidad_bytes + 1);

    // Leer línea completa (fgets incluye el '\n' si hay espacio)
    char* linea = malloc(4096 + 1); // buffer temporal grande para capturar todo el input
    if (linea == NULL) {
        free(buffer);
        return NULL;
    }
    memset(linea, '\0', 4097);

    fgets(linea, 4097, stdin); // lee hasta Enter o 4096 chars

    // Sacar el '\n' del final si está
    int len = strlen(linea);
    if (len > 0 && linea[len - 1] == '\n') {
        linea[len - 1] = '\0';
        len--;
    }

    // Caso 1: input mayor al tamaño → cortar
    // Caso 2: input menor al tamaño → el memset ya llenó de '\0'
    // Caso 3: input igual → copia exacta
    int copiar = len < cantidad_bytes ? len : cantidad_bytes;
    memcpy(buffer, linea, copiar);

    free(linea);
    return buffer; // el caller debe hacer free()
}
