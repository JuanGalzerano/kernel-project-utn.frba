#include <utils/utils.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <io.h>

tipo_IO tipo;

int main(int argc, char* argv[]) {

    //Creo el config
    t_config* configIO = config_create(argv[1]);
    //Creo el logger
    t_log* loggerIO = log_create("io.log", "main.c", true, log_level_from_string(config_get_string_value(configIO, "LOG_LEVEL")));
    //Defino las variables para conectarme al scheduler
    int socketConScheduler;
    int pid;
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


    while(1){//agregue el while(1) para que constantemente atienda
        //recibir paquete
        t_paquete* paquete;
        paquete = recibir_paquete(socketConScheduler);
        
        
        switch (tipo)
        {
        case TIPO_SLEEP:
            t_sleep* sl;
            sl = deserializar_sleep(paquete->buffer);
            pid=sl->pid;
            log_info(loggerIO, "## PID: %d - Inicio de IO",pid);
            sleep_func(sl->tiempoADormir,sl->pid,loggerIO);
            paquete->buffer=serializar_sleep(sl);
            paquete->codigo_operacion=FINALIZAR_SLEEP;
            enviar_paquete(socketConScheduler,paquete);
            log_info(loggerIO, "## PID: %d - Fin de IO",pid);
            break;
        case TIPO_STDIN:
            t_stdin_stdout* in;
            in = deserializar_stdin(paquete->buffer);
            pid=in->pid;
            log_info(loggerIO, "## PID: %d - Inicio de IO",pid);
            in->cadenaLeida=stdin_func(in->bytesALeer,in->pid,loggerIO);
            paquete->buffer=serializar_stdin(in);
            paquete->codigo_operacion=FINALIZAR_STDIN;
            enviar_paquete(socketConScheduler,paquete);
            free(in->cadenaLeida);
            free(in);
            log_info(loggerIO, "## PID: %d - Fin de IO",pid);
            break;
        case TIPO_STDOUT:
        
            t_stdin_stdout* out;
            out = deserializar_stdin(paquete->buffer);
            pid=out->pid;
            log_info(loggerIO, "## PID: %d - Inicio de IO",pid);
            stdout_func(out->cadenaLeida,out->pid,out->bytesALeer,loggerIO);
            paquete->buffer=serializar_stdin(out);
            paquete->codigo_operacion=FINALIZAR_STDOUT;
            enviar_paquete(socketConScheduler,paquete);
            log_info(loggerIO, "## PID: %d - Fin de IO",pid);
            break;
        default:
        
            break;
        }
    }
   
    return 0;
}

char* stdin_func(uint32_t cantBytes, uint32_t pid,t_log*logIO){
    char* mensaje;
    log_info(logIO, "## PID: %d - Ingrese %d caracteres:", pid, cantBytes);
    mensaje = leer_stdin(cantBytes);
    return mensaje;
}
void stdout_func(char* mensaje, uint32_t pid, uint32_t cantBytes, t_log* logIO){
    fwrite(mensaje, sizeof(char), cantBytes, stdout);
    printf("\n");

    char* buffer_log = malloc(cantBytes + 1); // +1 para el '\0'
    memcpy(buffer_log, mensaje, cantBytes);
    buffer_log[cantBytes] = '\0';             // terminador explícito
    
    log_info(logIO, "## PID: %d - %s", pid, buffer_log);
    free(buffer_log);                         // también falta liberar
}

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

char* leer_stdin(uint32_t cantidad_bytes) {
   char* buffer = malloc(cantidad_bytes + 1);
    if (buffer == NULL) return NULL;

    // Inicializar todo en '\0'
    memset(buffer, '\0', cantidad_bytes + 1);

    int c;
    uint32_t i = 0;

    while ((c = getchar()) != '\n' && c != EOF) {
        if (i < cantidad_bytes) {
            buffer[i] = (char)c;
            i++;
        }
        // Si i >= cantidad_bytes seguimos leyendo hasta '\n'
        // pero no guardamos (descartamos el exceso)
    }

    // buffer ya tiene '\0' en las posiciones no escritas por el memset
    return buffer;
}
