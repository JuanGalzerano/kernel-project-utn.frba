#include <cpu.h>
//pfidasfhidsuopahfaip
int main(int argc, char* argv[]) {

    //Verifico haber recibido todos los argumentos
    if(argc < 3){
        fprintf(stderr, "➜ ~ ./bin/cpu [Archivo Config] [Identificador]\n"); 
        return EXIT_FAILURE; 
    }

    //inicializo log y config
    inicializar_log_y_config(argv[1], argv[2]);

    int sizeIdCpu = strlen(idCpu)+1;
//Establezco conexiones
    //LEVANTAR CONEXION CON MEMORY
    int socketConexionMemory = iniciar_conexion(IPMemory, puertoMemory);
    if(socketConexionMemory == EXIT_FAILURE){
        log_info(loggerCpu, "CPU: No se pudo conectar con Kernel Memory");
        abort();
    }
    log_info(loggerCpu, "CPU: Conexion establecida con Kernel Memory");
    handshake_cliente_id(socketConexionMemory, loggerCpu, CPU);
    send(socketConexionMemory, &sizeIdCpu, sizeof(int), 0);
    send(socketConexionMemory, idCpu, sizeIdCpu, 0);
    
    //LEVANTAR CONEXION CON SCHEDULER
    int socketConexionScheduler = iniciar_conexion(IPKernel, puertoKernel);
    if(socketConexionScheduler == EXIT_FAILURE){
        log_info(loggerCpu, "CPU: No se pudo conectar con Kernel Scheduler");
        abort();
    }
    log_info(loggerCpu, "CPU: Conexion establecida con Kernel Scheduler");
    handshake_cliente_id(socketConexionScheduler, loggerCpu, CPU);
    send(socketConexionScheduler, &sizeIdCpu, sizeof(int), 0);
    send(socketConexionScheduler, idCpu, sizeIdCpu, 0);
//Espero PID
    while(1) {
        // 1) Obtener PID
        uint32_t pid = obtener_pid(socketConexionScheduler);
        log_info(loggerCpu, "CPU: Obtuve PID: %d", pid);

        // 2) Pedir Contexto a Juani
        t_contexto_ejecucion* ctx = obtener_contexto(pid, socketConexionMemory);
        log_info(loggerCpu, "CPU: Obtuve contexto de ejecucion");

        // 3) Iniciar Ciclo de Instruccion
        while (true/*!hay_interrupcion()*/) {
            int err = ejecutar_ciclo_de_instruccion(ctx, socketConexionMemory, loggerCpu);
            if (err == EXIT_FAILURE) {
                log_info(loggerCpu, "CPU: No se pudo ejecutar correctamente el ciclo de instruccion");
                abort();
            }
            log_info(loggerCpu, "CPU: Se ejecuto el ciclo de instruccion");

            // 4) Check Interrupt
        }
    }

/*
    //LEVANTAR CONEXION CON MEMORY STICK
    int socketConexionMemoryStick = iniciar_conexion(IPMemoryStick, puertoMemoryStick);
    if(socketConexionMemoryStick == EXIT_FAILURE){
        log_info(loggerCpu, "CPU no se pudo conectar a Memory Stick");
        abort();
    }
    log_info(loggerCpu, "CPU: conexion establecida con Memory Stick");
    handshake_cliente_id(socketConexionMemoryStick, loggerCpu, CPU);
    send(socketConexionMemoryStick, &sizeIdCpu, sizeof(int), 0);
    send(socketConexionMemoryStick, idCpu, sizeIdCpu, 0);
*/

    return 0;
}

uint32_t obtener_pid(int socketConexionScheduler) {
    t_paquete* paqueteConPid = recibir_paquete(socketConexionScheduler);
    t_buffer *buffer = paqueteConPid->buffer;
    uint32_t pid = buffer_read_uint32(buffer);
    eliminar_paquete(paqueteConPid);
    return pid;
}

t_contexto_ejecucion* obtener_contexto(uint32_t pid, int socketConexionMemory) {
    send(socketConexionMemory, OBTENER_CONTEXTO, sizeof(op_code), 0);
    send(socketConexionMemory, &pid, sizeof(uint32_t), 0);
    t_paquete* paqueteConContexto = recibir_paquete(socketConexionMemory);
    t_buffer *buffer = paqueteConContexto->buffer;
    t_contexto_ejecucion* ctx = deserializar_contexto_ctx(buffer);
    eliminar_paquete(paqueteConContexto);
    return ctx;
}

int ejecutar_ciclo_de_instruccion(t_contexto_ejecucion* ctx, int socketConexionMemory, t_log* loggerCpu) {
    uint32_t pc = ctx->pc;
    send(socketConexionMemory, OBTENER_INSTRUCCION, sizeof(op_code), 0);
    send(socketConexionMemory, &pc, sizeof(uint32_t), 0);
    t_paquete* paqueteConInstruccion = recibir_paquete(socketConexionMemory);
    t_buffer *buffer = paqueteConInstruccion->buffer;
    uint32_t longitudInstruccion = paqueteConInstruccion->buffer->size;
    char *inicioInstruccionFormatoString = buffer_read_string(buffer, longitudInstruccion);
    char *cursor = inicioInstruccionFormatoString; //Para avanzar sobre el string entero
    eliminar_paquete(paqueteConInstruccion);

    log_info(loggerCpu, "CPU: Se obtuvo el string instruccion");

    op_code tipoInstruccion = obtener_instruccion_registro_valor(&cursor); //Esto me modifica mi instruccionFormatoString
    if (tipoInstruccion == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    log_info(loggerCpu, "CPU: Se obtuvo el tipo instruccion");

    log_info(loggerCpu, "CPU: Iniciando decode...");
    decode(tipoInstruccion, inicioInstruccionFormatoString);
    free(inicioInstruccionFormatoString);
    return EXIT_SUCCESS;
}

int obtener_instruccion_registro_valor(char **string) {
    int i = 0;

    //Cuento los caracteres del string
    while ((*string)[i] != ' ' && (*string)[i] != '\0') {
        i++;
    }

    //Creo el buffer donde voy a ir copiando cada char
    char *bufferTemporal = malloc(i + 1);
    if (bufferTemporal == NULL) {
        return EXIT_FAILURE;
    }

    //FUncion que copia i caracteres de *string en bufferTemporal
    strncpy(bufferTemporal, *string, i);

    //El string despues quiero que me de parametros. Quiero que avance desde donde se quedo
    *string += ((*string)[i] == ' ') ? i + 1 : i;

    
    //Identificar que es
    int resultado = EXIT_FAILURE;

    if (isdigit(bufferTemporal[0])) { //Si el primero es n numero asumo que todo es un numero
        resultado = atoi(bufferTemporal);
    }

    if (resultado == EXIT_FAILURE) { //Si no era un numero, veo si era una instruccion
        for (int j = 0; instrucciones[j].nombreDeLaInstruccion != NULL; j++) {
            if (strcmp(bufferTemporal, instrucciones[j].nombreDeLaInstruccion) == 0) {
                resultado = instrucciones[j].instruccion_codigo;
                break;
            }
        }
    }

    if (resultado == EXIT_FAILURE) { //Si no era num ni instruccion, veo si era un registro
            for (int k = 0; registros[k].nombreDelRegistro != NULL; k++) {
                if (strcmp(bufferTemporal, registros[k].nombreDelRegistro) == 0) {
                    resultado = registros[k].codigo_registros_cpu;
                    break;
                }
            }
    }

    free(bufferTemporal); // ya no lo necesitás, guardaste el resultado
    return resultado;
}

void decode(op_code tipoInstruccion, char *instruccionFormatoString) {
    Codigo_registros_cpu tipoRegistro;
    Codigo_registros_cpu registroDestino;
    Codigo_registros_cpu registroOrigen;
    Codigo_registros_cpu registroDirLogica;
    Codigo_registros_cpu tamanio;
    char *nombreMutex;
    int segmentoId;
    int valor;
    switch (tipoInstruccion)
    {
        case NOOP:
            {
                //ejecutar_noop();
                break;
            }
        case SET:
            {
                tipoRegistro = obtener_instruccion_registro_valor(instruccionFormatoString);
                valor = obtener_instruccion_registro_valor(instruccionFormatoString);
                //ejecutar_set(tipoRegistro, valor);
                break;
            }
        case MOV_IN:
            {
                tipoRegistro = obtener_instruccion_registro_valor(instruccionFormatoString);
                //ejecutar_mov_in(tipoRegistro);
                break;
            }
        case MOV_OUT:
            {
                tipoRegistro = obtener_instruccion_registro_valor(instruccionFormatoString);
                //ejecutar_mov_out(tipoRegistro);
                break;
            }
        case SUM:
           {
                registroDestino = obtener_instruccion_registro_valor(instruccionFormatoString);
                registroOrigen = obtener_instruccion_registro_valor(instruccionFormatoString);
                //ejecutar_sum(registroDestino, registroOrigen);
                break;
            }
        case SUB:
            {
                registroDestino = obtener_instruccion_registro_valor(instruccionFormatoString);
                registroOrigen = obtener_instruccion_registro_valor(instruccionFormatoString);
                //ejecutar_sub(registroDestino, registroOrigen);
                break;
            }
        case JNZ:
            {
                tipoRegistro = obtener_instruccion_registro_valor(instruccionFormatoString);
                int instruccion = obtener_instruccion_registro_valor(instruccionFormatoString);
                //ejectar_jnz(tipoRegistro, instruccion);
                break;
            }
        case COPY_MEM:
            {           
                valor = obtener_instruccion_registro_valor(instruccionFormatoString);
                //ejecutar_copy_mem(tamanio);
                break;
            }
        case MUTEX_CREATE:
            {
                //nombreMutex = obtener_nombre_mutex(instruccionFormatoString);
                //ejecutar_mutex_create(nombreMutex);
                break;
            }
        case MUTEX_LOCK:
            {
                //nombreMutex = obtener_nombre_mutex(instruccionFormatoString);
                //ejecutar_mutex_lock(nombreMutex);
                break;
            }
        case MUTEX_UNLOCK:
            {
                //nombreMutex = obtener_nombre_mutex(instruccionFormatoString);
                //ejecutar_mutex_unlock(nombreMutex);
                break;
            }
        case MEM_ALLOC:
            {
                segmentoId = obtener_instruccion_registro_valor(instruccionFormatoString);
                int tamanio = obtener_instruccion_registro_valor(instruccionFormatoString);
                //ejecutar_mem_alloc(segmentoId, tamanio);
                break;
            }
        case MEM_FREE:
            {
                segmentoId = obtener_instruccion_registro_valor(instruccionFormatoString);
                //ejecutar_mem_free(segmentoId);
                break;
            }
        case SLEEP:
            {
                int tiempo = obtener_instruccion_registro_valor(instruccionFormatoString);
                //ejecutar_sleep(tiempo);
                break;
            }
        case STDOUT:
            {
                registroDirLogica = obtener_instruccion_registro_valor(instruccionFormatoString);
                tamanio = obtener_instruccion_registro_valor(instruccionFormatoString);
                //ejecutar_stdout(registroDirLogica, tamanio);
                break;
            }
        case STDIN:
            {
                registroDirLogica = obtener_instruccion_registro_valor(instruccionFormatoString);
                tamanio = obtener_instruccion_registro_valor(instruccionFormatoString);
                //ejecutar_stdin(registroDirLogica, tamanio);
                break;
            }
        case INIT_PROC:
            {
                //char *pathArchivoInstrucciones = obtener_archivo_instrucciones(instruccionFormatoString);
                int prioridad = obtener_instruccion_registro_valor(instruccionFormatoString);
                //ejecutar_init_proc(pathArchivoInstrucciones, prioridad);
                break;
            }
        case EXIT:
            {
                //ejecutar_exit();
                break;
            }
        default:
            {
                break;
                // Cuando agregue validacion para cada funcion ejecutar veo de hacer return EXIT_FAILURE;
            }
    }
}

/*char *obtener_nombre_mutex(char *string) {

}
char *obtener_archivo_instrucciones(char *string) {

}*/

bool hay_interrupcion(void) {
    return true;
}