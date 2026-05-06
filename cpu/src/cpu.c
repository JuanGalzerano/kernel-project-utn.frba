#include <cpu.h>

Registros_cpu registros_cpu;
Registro registros[] = {
    {"PC", PC}, {"AX", AX}, {"BX", BX }, {"CX", CX},
    {"DX", DX}, {"EAX", EAX}, {"EBX", EBX}, {"ECX", ECX},
    {"EDX", EDX}, {"SI", SI}, {"DI", DI}
};
Instruccion instrucciones[] = {
    {"NOOP", NOOP}, {"SET", SET}, {"MOV_IN", MOV_IN }, {"MOV_OUT", MOV_OUT},
    {"SUM", SUM}, {"SUB", SUB}, {"JNZ", JNZ}, {"COPY_MEM", COPY_MEM},
    {"MUTEX_CREATE", MUTEX_CREATE}, {"MUTEX_LOCK", MUTEX_LOCK}, {"MUTEX_UNLOCK", MUTEX_UNLOCK},
    {"MEM_ALLOC", MEM_ALLOC}, {"MEM_FREE", MEM_FREE}, {"SLEEP", SLEEP}, {"STDOUT", STDOUT},
    {"STDIN", STDIN}, {"INIT_PROC", INIT_PROC}, {"EXIT", EXIT}
};

sem_t sem_interrupcion;
uint32_t pid_interrupcion = 0;
uint32_t motivo_interrupcion = -1;

int main(int argc, char *argv[])
{

    // Verifico haber recibido todos los argumentos
    if (argc < 3)
    {
        fprintf(stderr, "➜ ~ ./bin/cpu [Archivo Config] [Identificador]\n");
        return EXIT_FAILURE;
    }

    // inicializo log y config
    inicializar_log_y_config(argv[1], argv[2]);

    sem_init(&sem_interrupcion, 0, 0);

    uint32_t sizeIdCpu = strlen(idCpu) + 1;
    // Establezco conexiones
    // LEVANTAR CONEXION CON MEMORY
    int socketConexionMemory = iniciar_conexion(IPMemory, puertoMemory);
    if (socketConexionMemory == EXIT_FAILURE)
    {
        log_info(loggerCpu, "CPU: No se pudo conectar con Kernel Memory");
        abort();
    }
    log_info(loggerCpu, "CPU: Conexion establecida con Kernel Memory");
    handshake_cliente_id(socketConexionMemory, loggerCpu, CPU);
    send(socketConexionMemory, &sizeIdCpu, sizeof(int), 0);
    send(socketConexionMemory, idCpu, sizeIdCpu, 0);

    // LEVANTAR CONEXION CON SCHEDULER
    int socketConexionScheduler = iniciar_conexion(IPKernel, puertoKernel);
    if (socketConexionScheduler == EXIT_FAILURE)
    {
        log_info(loggerCpu, "CPU: No se pudo conectar con Kernel Scheduler");
        abort();
    }
    log_info(loggerCpu, "CPU: Conexion establecida con Kernel Scheduler");
    handshake_cliente_id(socketConexionScheduler, loggerCpu, CPU);
    send(socketConexionScheduler, &sizeIdCpu, sizeof(int), 0);
    send(socketConexionScheduler, idCpu, sizeIdCpu, 0);

//ESCUCHO INTERRUPCIONES
    pthread_t hiloInterrupciones;
    pthread_create(&hiloInterrupciones, NULL, interrupciones, &socketConexionScheduler);
    pthread_detach(hiloInterrupciones);

    // Espero PID
    while (1)
    {
        // 1) Obtener PID
        uint32_t pid = obtener_pid(socketConexionScheduler);
        log_info(loggerCpu, "CPU: Obtuve PID: %d", pid);

        // 2) Pedir Contexto a Juani
        t_contexto_ejecucion *ctx = obtener_contexto(pid, socketConexionMemory);
        log_info(loggerCpu, "CPU: Obtuve contexto de ejecucion");
        actualizar_registros_cpu(ctx); // Parece q no tiene sentido pero en realidad el contexto tiene mas cosas q no estan implementadas.
        log_info(loggerCpu, "CPU: Contexto cargado, iniciando ciclo");
        // 3) Iniciar Ciclo de Instruccion
        int errorCiclo;
        while (!hay_interrupcion(pid))
        {
            errorCiclo = ejecutar_ciclo_de_instruccion(socketConexionMemory, loggerCpu, socketConexionScheduler, pid);
            if (errorCiclo == EXIT_FAILURE) {
                log_info(loggerCpu, "CPU: No se pudo ejecutar correctamente el ciclo de instruccion");
                abort();
            }
            if (errorCiclo == 1) {
                log_info(loggerCpu, "CPU: Syscall ejecutada");
                break;
            }
        }
        if (errorCiclo == 1) {
            actualizar_contexto(ctx, socketConexionMemory, pid);
        }
        else {
            actualizar_contexto(ctx, socketConexionMemory, pid);
            enviar_pid_y_motivo(pid, motivo_interrupcion, socketConexionScheduler);
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

uint32_t obtener_pid(int socketConexionScheduler)
{
    t_paquete *paqueteConPid = recibir_paquete(socketConexionScheduler);
    t_buffer *buffer = paqueteConPid->buffer;
    uint32_t pid = buffer_read_uint32(buffer);
    eliminar_paquete(paqueteConPid);
    return pid;
}

t_contexto_ejecucion *obtener_contexto(uint32_t pid, int socketConexionMemory)
{
    t_buffer* buf = buffer_create(0);
    buffer_add_uint32(buf, pid);
    t_paquete* req = crear_paquete(OBTENER_CONTEXTO, buf);
    enviar_paquete(socketConexionMemory, req);
    eliminar_paquete(req);
    t_paquete *paqueteConContexto = recibir_paquete(socketConexionMemory);
    t_buffer *buffer = paqueteConContexto->buffer;
    t_contexto_ejecucion *ctx = deserializar_contexto_ctx(buffer);
    eliminar_paquete(paqueteConContexto);
    return ctx;
}

void actualizar_contexto(t_contexto_ejecucion *ctx, int socketConexionMemory, uint32_t pid) {
    ctx->ax = registros_cpu.ax;
    ctx->bx = registros_cpu.bx;
    ctx->cx = registros_cpu.cx;
    ctx->dx = registros_cpu.dx;
    ctx->eax = registros_cpu.eax;
    ctx->ebx = registros_cpu.ebx;
    ctx->ecx = registros_cpu.ecx;
    ctx->edx = registros_cpu.edx;
    ctx->pc = registros_cpu.pc;
    ctx->si = registros_cpu.si;
    ctx->di = registros_cpu.di;
    //Asumo q tendre q actualizar algo de segmentos
    t_buffer* ctx_buf = serializar_contexto_ctx(ctx);
    t_buffer* buf = buffer_create(0);
    buffer_add_uint32(buf, pid);
    buffer_add(buf, ctx_buf->stream, ctx_buf->size);
    buffer_destroy(ctx_buf);
    t_paquete* paquete_con_contexto = crear_paquete(ACTUALIZAR_CONTEXTO, buf);
    enviar_paquete(socketConexionMemory, paquete_con_contexto);
    eliminar_paquete(paquete_con_contexto);
    free(ctx);
}

void actualizar_registros_cpu(t_contexto_ejecucion *ctx)
{
    registros_cpu.ax = ctx->ax;
    registros_cpu.bx = ctx->bx;
    registros_cpu.cx = ctx->cx;
    registros_cpu.dx = ctx->dx;
    registros_cpu.eax = ctx->eax;
    registros_cpu.ebx = ctx->ebx;
    registros_cpu.ecx = ctx->ecx;
    registros_cpu.edx = ctx->edx;
    registros_cpu.pc = ctx->pc;
    registros_cpu.si = ctx->si;
    registros_cpu.di = ctx->di;
}

void enviar_pid_y_motivo(uint32_t pid, uint32_t motivo_interrupcion, int socketConexionScheduler) {
    t_buffer* buffer_interrupcion = buffer_create(0);
    buffer_add_uint32(buffer_interrupcion, pid);
    t_paquete* paquete_interrupcion = crear_paquete(motivo_interrupcion, buffer_interrupcion);
    enviar_paquete(socketConexionScheduler, paquete_interrupcion);
    eliminar_paquete(paquete_interrupcion);
}

int ejecutar_ciclo_de_instruccion(int socketConexionMemory, t_log *loggerCpu, int socketConexionScheduler, uint32_t pid)
{
    uint32_t pc = registros_cpu.pc;
    log_info(loggerCpu, "## PID: %d - FETCH - Program Counter: %d", pid, pc);
    t_buffer* buf_instr = buffer_create(0);
    buffer_add_uint32(buf_instr, pid);
    t_paquete* req_instr = crear_paquete(OBTENER_INSTRUCCION, buf_instr);
    enviar_paquete(socketConexionMemory, req_instr);
    eliminar_paquete(req_instr);
    t_paquete *paqueteConInstruccion = recibir_paquete(socketConexionMemory);
    t_buffer *buffer = paqueteConInstruccion->buffer;
    uint32_t longitudInstruccion = paqueteConInstruccion->buffer->size;
    char *inicioInstruccionFormatoString = buffer_read_string(buffer, longitudInstruccion);
    char *cursor = inicioInstruccionFormatoString; // Para avanzar sobre el string entero
    eliminar_paquete(paqueteConInstruccion);
    log_info(loggerCpu, "CPU: Instruccion Obtenida");

    op_code tipoInstruccion = obtener_instruccion_registro_valor(&cursor); // Esto me modifica mi instruccionFormatoString
    if (tipoInstruccion < 0)
    {
        free(inicioInstruccionFormatoString);
        return EXIT_FAILURE;
    }
    log_info(loggerCpu, "CPU: Se obtuvo el tipo instruccion");

    log_info(loggerCpu, "CPU: Iniciando decode...");
    uint32_t pc_antes = registros_cpu.pc;
    int errorDecode = decode(tipoInstruccion, &cursor, socketConexionScheduler, pid, loggerCpu);
    if (errorDecode == 1)
    {
        free(inicioInstruccionFormatoString);
        return 1;
    }
    if (errorDecode == 0)
    {
        if (registros_cpu.pc == pc_antes) {
            registros_cpu.pc++;
        }
        free(inicioInstruccionFormatoString);
        return EXIT_SUCCESS;
    }
    free(inicioInstruccionFormatoString);
    return EXIT_FAILURE;
}

int obtener_instruccion_registro_valor(char **string)
{
    int i = 0;

    // Cuento los caracteres del string
    while ((*string)[i] != ' ' && (*string)[i] != '\0')
    {
        i++;
    }

    // Creo el buffer donde voy a ir copiando cada char
    char *bufferTemporal = malloc(i + 1);
    if (bufferTemporal == NULL)
    {
        return -1;
    }

    // FUncion que copia i caracteres de *string en bufferTemporal
    strncpy(bufferTemporal, *string, i);
    bufferTemporal[i] = '\0'; // Agrego el '\0' para que strcmp no lea basura

    // El string despues quiero que me de parametros. Quiero que avance desde donde se quedo
    if ((*string)[i] == ' ')
    {
        *string += i + 1; // saltamos el token + el espacio
    }

    // Identificar que es
    int resultado = -1;

    if (isdigit(bufferTemporal[0]))
    { // Si el primero es n numero asumo que todo es un numero
        resultado = atoi(bufferTemporal);
    }

    if (resultado == -1)
    { // Si no era un numero, veo si era una instruccion
        for (int j = 0; instrucciones[j].nombreDeLaInstruccion != NULL; j++)
        {
            if (strcmp(bufferTemporal, instrucciones[j].nombreDeLaInstruccion) == 0)
            {
                resultado = instrucciones[j].instruccion_codigo;
                break;
            }
        }
    }

    if (resultado == -1)
    { // Si no era num ni instruccion, veo si era un registro
        for (int k = 0; registros[k].nombreDelRegistro != NULL; k++)
        {
            if (strcmp(bufferTemporal, registros[k].nombreDelRegistro) == 0)
            {
                resultado = registros[k].codigo_registros_cpu;
                break;
            }
        }
    }

    free(bufferTemporal); // ya no lo necesitás, guardaste el resultado
    return resultado;
}
char* obtener_nombre_mutex_o_path(char** string)
{
    int i = 0;

    // Saltar espacios iniciales
    while ((*string)[i] == ' ') {
        i++;
    }

    int start = i;

    // Copiar hasta espacio o fin
    while ((*string)[i] != ' ' && (*string)[i] != '\0') {
        i++;
    }

    int len = i - start;
    char* resultado = malloc(len + 1);
    strncpy(resultado, (*string) + start, len); 

    resultado[len] = '\0';

    // Avanzar el puntero original
    if ((*string)[i] == ' ')
    {
        *string += i + 1;
    }
    else
    {
        *string += i;
    }

    return resultado;
}

int decode(op_code tipoInstruccion, char **string, int socketConexionScheduler, uint32_t pid, t_log *loggerCpu)
{
    Codigo_registros_cpu tipoRegistro;
    Codigo_registros_cpu registroDestino;
    Codigo_registros_cpu registroOrigen;
    Codigo_registros_cpu registroDirLogica;
    Codigo_registros_cpu registroTamanio;
    uint32_t segmentoId;
    uint32_t valor;
    switch (tipoInstruccion)
    {
    case NOOP:
    {
        // Creo q no tengo q hacer nada
        log_info(loggerCpu, "## PID: %d - Ejecutando: NOOP", pid);
        break;
    }
    case SET:
    {
        tipoRegistro = obtener_instruccion_registro_valor(string);
        valor = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: SET - %s %d", pid, registro_a_string(tipoRegistro), valor);
        ejecutar_set(tipoRegistro, valor);
        break;
    }
    case MOV_IN:
    {
        tipoRegistro = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: MOV_IN - %s", pid, registro_a_string(tipoRegistro));
        // ejecutar_mov_in(tipoRegistro);
        break;
    }
    case MOV_OUT:
    {
        tipoRegistro = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: MOV_OUT - %s", pid, registro_a_string(tipoRegistro));
        // ejecutar_mov_out(tipoRegistro);
        break;
    }
    case SUM:
    {
        registroDestino = obtener_instruccion_registro_valor(string);
        registroOrigen = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: SUM - %s %s", pid, registro_a_string(registroDestino), registro_a_string(registroOrigen));
        ejecutar_sum(registroDestino, registroOrigen);
        break;
    }
    case SUB:
    {
        registroDestino = obtener_instruccion_registro_valor(string);
        registroOrigen = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: SUB - %s %s", pid, registro_a_string(registroDestino), registro_a_string(registroOrigen));
        ejecutar_sub(registroDestino, registroOrigen);
        break;
    }
    case JNZ:
    {
        tipoRegistro = obtener_instruccion_registro_valor(string);
        uint32_t instruccion = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: JNZ - %s %d", pid, registro_a_string(tipoRegistro), instruccion);
        ejectar_jnz(tipoRegistro, instruccion);
        break;
    }
    case COPY_MEM:
    {
        tipoRegistro = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: COPY_MEM - %s", pid, registro_a_string(tipoRegistro));
        // ejecutar_copy_mem(tipoRegistro);
        break;
    }
    case MUTEX_CREATE:
    {
        char* nombreMutex = obtener_nombre_mutex_o_path(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: MUTEX_CREATE - %s", pid, nombreMutex);
        solicitar_mutex_create(nombreMutex, socketConexionScheduler, pid);
        free(nombreMutex);
        return 1;
    }
    case MUTEX_LOCK:
    {
        char* nombreMutex = obtener_nombre_mutex_o_path(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: MUTEX_LOCK - %s", pid, nombreMutex);
        solicitar_mutex_lock(nombreMutex, socketConexionScheduler, pid);
        free(nombreMutex);
        return 1;
    }
    case MUTEX_UNLOCK:
    {
        char* nombreMutex = obtener_nombre_mutex_o_path(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: MUTEX_UNLOCK - %s", pid, nombreMutex);
        solicitar_mutex_unlock(nombreMutex, socketConexionScheduler, pid);
        free(nombreMutex);
        return 1;
    }
    case MEM_ALLOC:
    {
        segmentoId = obtener_instruccion_registro_valor(string);
        uint32_t tamanio = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: MEM_ALLOC - %d %d", pid, segmentoId, tamanio);
        solicitar_mem_alloc(segmentoId, tamanio, socketConexionScheduler, pid);
        return 1;
    }
    case MEM_FREE:
    {
        segmentoId = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: MEM_FREE - %d", pid, segmentoId);
        solicitar_mem_free(segmentoId, socketConexionScheduler, pid);
        return 1;
    }
    case SLEEP:
    {
        uint32_t tiempo = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: SLEEP - %d", pid, tiempo);
        solicitar_sleep(tiempo, socketConexionScheduler, pid);
        return 1;
    }
    case STDOUT:
    {
        registroDirLogica = obtener_instruccion_registro_valor(string);
        registroTamanio = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: STDOUT - %s %s", pid, registro_a_string(registroDirLogica), registro_a_string(registroTamanio));
        solicitar_stdout(registroDirLogica, registroTamanio, socketConexionScheduler, pid);
        return 1;
    }
    case STDIN:
    {
        registroDirLogica = obtener_instruccion_registro_valor(string);
        registroTamanio = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: STDIN - %s %s", pid, registro_a_string(registroDirLogica), registro_a_string(registroTamanio));
        solicitar_stdin(registroDirLogica, registroTamanio, socketConexionScheduler, pid);
        return 1;
    }
    case INIT_PROC:
    {
        char* pathArchivoInstrucciones = obtener_nombre_mutex_o_path(string);
        int prioridad = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: INIT_PROC - %s %d", pid, pathArchivoInstrucciones, prioridad);
        solicitar_init_proc(pathArchivoInstrucciones, prioridad, socketConexionScheduler);
        free(pathArchivoInstrucciones);
        return 1;
    }
    case EXIT:
    {
        log_info(loggerCpu, "## PID: %d - Ejecutando: EXIT", pid);
        solicitar_exit(socketConexionScheduler);
        return 1;
    }
    default:
    {
        log_info(loggerCpu, "## PID: %d - Instruccion NO RECONOCIDA", pid);
        return -1;
        // Cuando agregue validacion para cada funcion ejecutar veo de hacer return EXIT_FAILURE;
    }
    }
    return 0;
}

bool hay_interrupcion(uint32_t pidActual) {
    // Si el semaforo > 0 lo decrementa y retorna 0
    // Si el semaforo == 0 no lo modifica y retorna -1
    bool hayInterrupcion = sem_trywait(&sem_interrupcion) == 0;
    if(hayInterrupcion) {
        if (pidActual == pid_interrupcion) {
            return hayInterrupcion;
        }
        else {
            sem_post(&sem_interrupcion); //como le reste 1, le vuelvo a sumar 1
            return false;
        }
    }
    else {
        return false;
    }
}

void* interrupciones(void* arg) {
    int socketConexionScheduler = *(int*) arg;
    while (1) {
        t_paquete *paqueteConInterrupcion = recibir_paquete(socketConexionScheduler);
        if (paqueteConInterrupcion->codigo_operacion == FINALIZAR_POR_QUANTUM) {
            pid_interrupcion = buffer_read_uint32(paqueteConInterrupcion->buffer);
            motivo_interrupcion = FINALIZAR_POR_QUANTUM;
            sem_post(&sem_interrupcion);
            log_info(loggerCpu, "## Interrupción recibida");
        }
        eliminar_paquete(paqueteConInterrupcion);
    }
    return NULL;
}


//FUNCION EXTRA PARA LOGS SOLO
const char* registro_a_string (Codigo_registros_cpu tipoRegistro) {
    switch (tipoRegistro) {
        case PC: return "PC";
        case AX: return "AX";
        case BX: return "BX";
        case CX: return "CX";
        case DX: return "DX";
        case EAX: return "EAX";
        case EBX: return "EBX";
        case ECX: return "ECX";
        case EDX: return "EDX";
        case SI: return "SI";
        case DI: return "DI";
        default: return "tipoDesconocido";
    }
}