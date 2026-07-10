#include "memory.h"
#include <sys/epoll.h>//para captar desconexiones de sticks y mandar mem corrupta
#include <signal.h>//para ignorar socket cerrado de cpu al mandarle un nuevo stick


int main(int argc, char* argv[]) {

    if (argc < 2) {
        printf("Falta path de configuracion");
        return EXIT_FAILURE;
    }

    //esta es una biblioteca que es especialmente para que ignore si le mando a un socket cerrado un mensaje. 
    //El unico caso donde pasa esto es como le aviso de un stick al cpu, entonces me ahorro que termine el proceso 
    signal(SIGPIPE, SIG_IGN);//dice signal haciendo referencia a la señal del so, no a sem wait/signal


    configMemory = config_create(argv[1]);
    logLevel = log_level_from_string(config_get_string_value(configMemory, "LOG_LEVEL"));
    loggerMemory = log_create("memory.log", "memory.c", true, logLevel);

    puertoEscucha = config_get_string_value(configMemory, "PUERTO_MEMORY");
    puertoEscuchaNotif = config_get_string_value(configMemory, "PUERTO_MEMORY_NOTIF");
    scriptsBasePath = config_get_string_value(configMemory, "SCRIPTS_BASEPATH");
    segment_max_size = (uint32_t)config_get_int_value(configMemory, "SEGMENT_MAX_SIZE");
    allocation_strategy = config_get_string_value(configMemory, "ALLOCATION_STRATEGY");
    instruction_delay = (uint32_t)config_get_int_value(configMemory, "INSTRUCTION_DELAY");
    compaction_delay = (uint32_t)config_get_int_value(configMemory, "COMPACTION_DELAY");
    puertoMemoryStick = config_get_string_value(configMemory, "PUERTO_MEMORYSTICK");
    lista_procesos = list_create();
    lista_memory_sticks = list_create();
    lista_sockets_cpu_notif = list_create();
    lista_huecos = list_create();
    memoria_total_size  = 0;
    memoria_libre_size  = 0;
    socketScheduler = -1;
    socketSchedulerNotif = -1;
    socketSwap = -1;
    epoll_fd_sticks = -1;
    pthread_mutex_init(&memoria_mutex, NULL);
    pthread_mutex_init(&mutex_sockets_cpu_notif, NULL);
    pthread_mutex_init(&procesos_mutex, NULL);

    int socketEscucha = iniciar_servidor(puertoEscucha);
    if (socketEscucha == EXIT_FAILURE) {
        log_error(loggerMemory, "No se pudo iniciar el servidor");
        return EXIT_FAILURE;
    }
    log_info(loggerMemory, "Servidor iniciado en puerto %s", puertoEscucha);

    socketEscuchaNotif = iniciar_servidor(puertoEscuchaNotif);
    if (socketEscuchaNotif == EXIT_FAILURE) {
        log_error(loggerMemory, "No se pudo iniciar el servidor de notificaciones");
        return EXIT_FAILURE;
    }
    log_info(loggerMemory, "Servidor de notificaciones iniciado en puerto %s", puertoEscuchaNotif);

    //El scheduler se conecta primero en ambos puertos (uno para tema procesos y otra para gestion de conexion o desconexion de sticks)
    socketScheduler = aceptar_cliente(socketEscucha, loggerMemory);
    log_info(loggerMemory, "## Kernel Scheduler Conectado - FD del socket: %d", socketScheduler);

    socketSchedulerNotif = esperar_cliente(socketEscuchaNotif);
    log_info(loggerMemory, "Kernel Scheduler Conectado al canal de notificaciones - FD: %d", socketSchedulerNotif);

    socketSwap = aceptar_cliente(socketEscucha, loggerMemory);
    log_info(loggerMemory, "Swap Conectado - FD del socket: %d", socketSwap);

    t_paquete* swapInit = recibir_paquete(socketSwap);
    swap_total_size = buffer_read_uint32(swapInit->buffer);
    swap_block_size = buffer_read_uint32(swapInit->buffer);
    eliminar_paquete(swapInit);
    inicializar_swap();

    //monitoreo de desconexion de memory sticks sin espera activa
    epoll_fd_sticks = epoll_create1(0);
    if (epoll_fd_sticks < 0) {
        log_error(loggerMemory, "No se pudo crear el epoll para memory sticks");
        return EXIT_FAILURE;
    }

    // Thread monitor: detecta desconexion de sticks via epoll (EPOLLRDHUP)
    pthread_t hilo_monitor;
    pthread_create(&hilo_monitor, NULL, hilo_monitor_sticks, NULL);
    pthread_detach(hilo_monitor);

    // Thread dedicado al scheduler: recibe nuevos procesos en loop
    int* argSched = malloc(sizeof(int));
    *argSched = socketScheduler;
    pthread_t hilo_scheduler;
    pthread_create(&hilo_scheduler, NULL, atender_scheduler, argSched);
    pthread_detach(hilo_scheduler);

    // Thread Notificaciones
    pthread_t hilo_notif_cpu;
    pthread_create(&hilo_notif_cpu, NULL, hilo_notificaciones_cpu, NULL);
    pthread_detach(hilo_notif_cpu);

    // Loop principal: acepta CPUs y Memory Sticks
    while (1) {
        int socket = esperar_cliente(socketEscucha);
        int32_t id = handshake_servidor_id(socket, 0);

        switch (id){
            case CPU:{
                int sizeCpuId;
                recv(socket, &sizeCpuId, sizeof(int), MSG_WAITALL);
                char* cpuId = malloc(sizeCpuId);
                recv(socket, cpuId, sizeCpuId, MSG_WAITALL);
                log_info(loggerMemory, "## CPU %s Conectada", cpuId);

                send(socket, &segment_max_size, sizeof(uint32_t), 0);

                t_cpu_conexiones* entrada = malloc(sizeof(t_cpu_conexiones));
                entrada->cpu_id = cpuId;
                entrada->socket_main = socket;
                entrada->socket_notif = -1;
                pthread_mutex_lock(&mutex_sockets_cpu_notif);
                list_add(lista_sockets_cpu_notif,entrada);
                pthread_mutex_unlock(&mutex_sockets_cpu_notif);                

                int* arg = malloc(sizeof(int));
                *arg = socket;
                pthread_t hilo;
                pthread_create(&hilo, NULL, atender_cpu, arg);
                pthread_detach(hilo);
                break;
            }
            case MEMORY_STICK:{
                uint32_t stick_size = 0;
                recv(socket, &stick_size, sizeof(uint32_t), MSG_WAITALL);
                agregar_memory_stick(socket, stick_size, puertoMemoryStick);
                log_info(loggerMemory, "## Memory Stick de %d bytes Conectada", stick_size);
                break;
            }
            default:
                log_warning(loggerMemory, "Modulo desconocido conectado: %d", id);
                close(socket);
                break;
        }
    }

    config_destroy(configMemory);
    return 0;
}

void notificar_desuspendibles(){

    pthread_mutex_lock(&procesos_mutex);

    bool hay_suspendidos = false;//intento hacer que termine rapido la funcion verificando que no haya suspendidos
    for(int i = 0; i < list_size(lista_procesos) && !hay_suspendidos; i++)
        if(((t_proceso_memory*)list_get(lista_procesos, i))->en_swap) hay_suspendidos = true;
    if(!hay_suspendidos){
        pthread_mutex_unlock(&procesos_mutex);
        return;
    }

    pthread_mutex_lock(&swap_mutex);
    pthread_mutex_lock(&memoria_mutex);

    t_list* huecos_base = list_create();//hago esto asi no retenog tantas veces el mutex de memoria
    for(int j = 0; j < list_size(lista_huecos); j++){
        t_hueco* orig = list_get(lista_huecos, j);
        t_hueco* copia = malloc(sizeof(t_hueco));
        *copia = *orig;
        list_add(huecos_base, copia);
    }
    pthread_mutex_unlock(&memoria_mutex);

    t_list* pids_ok = list_create();

    for(int i=0; i<list_size(lista_procesos); i++){
        t_proceso_memory* proc = list_get(lista_procesos, i);
        if(!proc->en_swap)continue;

//copio lista de huecos
        t_list* huecos_sim =list_create();
        for(int j = 0; j < list_size(huecos_base); j++){
            t_hueco* orig = list_get(huecos_base, j);
            t_hueco* copia = malloc(sizeof(t_hueco));
            *copia = *orig;
            list_add(huecos_sim, copia);
        }

        //intentara alocar cada segmento del proceso sobre la copia de huecos, cada iteracion es mas facil copiarla
        bool entra = true;
        for(int j = 0; j < (int)swap_num_bloques && entra;j++){
            if(!tabla_swap[j].ocupado || tabla_swap[j].pid != proc->pid)continue;
            uint32_t tam = tabla_swap[j].tamanio;
            t_hueco* h = encontrar_hueco(huecos_sim, tam);
            if(!h){
                entra =false;
                break;
            }
            h->base += tam;
            h->limite -= tam;
            if(h->limite==0){
                list_remove_element(huecos_sim, h);
                free(h);
            }
        }
        list_destroy_and_destroy_elements(huecos_sim, free);

        if(entra){
            uint32_t* pid_ptr = malloc(sizeof(uint32_t));
            *pid_ptr = proc->pid;
            list_add(pids_ok, pid_ptr);
        }
    }

    pthread_mutex_unlock(&swap_mutex);
    pthread_mutex_unlock(&procesos_mutex);
    list_destroy_and_destroy_elements(huecos_base, free);

    t_buffer* buf = buffer_create(0);
    uint32_t count = (uint32_t)list_size(pids_ok);
    buffer_add_uint32(buf, count);//primero le madno la cantidad de ints, y desp voy agregando los ints
    for (int i = 0; i < (int)count; i++){//xq count esta como uint32 y para hacer el <, por las dudas lo convierto a int
        buffer_add_uint32(buf, *(uint32_t*)list_get(pids_ok, i));
    }
    list_destroy_and_destroy_elements(pids_ok, free);

    t_paquete* aviso = crear_paquete(NUEVA_MEMORIA_DISPONIBLE, buf);
    enviar_paquete(socketSchedulerNotif, aviso);
    eliminar_paquete(aviso);
}

void* atender_scheduler(void* arg){
    int socket = *(int*)arg;
    free(arg);

    t_paquete* paquete;
    while ((paquete = recibir_paquete(socket)) != NULL) {
        switch ((op_code)paquete->codigo_operacion) {
            case PATH_PROCESO:{
                uint32_t pid = buffer_read_uint32(paquete->buffer);
                uint32_t sizePath = buffer_read_uint32(paquete->buffer);
                char*    path = buffer_read_string(paquete->buffer, sizePath);
                eliminar_paquete(paquete);

                int ok = inicializar_proceso(pid, path);
                free(path);
                send(socket, &ok, sizeof(int), 0);
                break;
            }
            case FIN_PROCESO:{
                uint32_t pid = buffer_read_uint32(paquete->buffer);
                eliminar_paquete(paquete);
                finalizar_proceso(pid);
                notificar_desuspendibles();
                break;
            }
            case ESCRIBIR_BYTES:{
                t_stdin_stdout* datosAEscribir = deserializar_stdin(paquete->buffer);
                eliminar_paquete(paquete);
                op_code ok = escribir_bytes_en_memoria(datosAEscribir->pid, datosAEscribir->direccionLogica, datosAEscribir->bytesALeer, datosAEscribir->cadenaLeida);
                free(datosAEscribir->cadenaLeida);
                free(datosAEscribir);
                t_paquete* paqueteResp = crear_paquete(ok,NULL);
                enviar_paquete(socket,paqueteResp);
                eliminar_paquete(paqueteResp);
                break;
            }
            case LEER_BYTES:{
                uint32_t pid = buffer_read_uint32(paquete->buffer);
                uint32_t dir_logica = buffer_read_uint32(paquete->buffer);
                uint32_t bytes = buffer_read_uint32(paquete->buffer);
                eliminar_paquete(paquete);

                t_proceso_memory* proc = buscar_proceso(pid);
                if (proc== NULL) {
                    t_paquete* respuesta = crear_paquete(LECTURA_FALLIDA, NULL);
                    enviar_paquete(socket, respuesta);
                    eliminar_paquete(respuesta);
                    break;
                }

                pthread_mutex_lock(&memoria_mutex);
                t_list* pedazos = traducir_logica_a_fisica(dir_logica, segment_max_size, proc->contexto->tabla_segmentos, lista_memory_sticks, bytes);
                pthread_mutex_unlock(&memoria_mutex);

                if (pedazos ==NULL) {
                    t_paquete* respuesta = crear_paquete(LECTURA_FALLIDA, NULL);
                    enviar_paquete(socket, respuesta);
                    eliminar_paquete(respuesta);
                    break;
                }

                char* datos = calloc(bytes, 1);
                op_code ok = leer_pedazos(pedazos, datos);
                list_destroy_and_destroy_elements(pedazos, free);

                if (ok == LEER_BYTES) {
                    uint32_t seg_id_leer = dir_logica / segment_max_size;
                    uint32_t desp_leer = dir_logica % segment_max_size;
                    t_segmento* seg_l = buscar_segmento_proceso(proc, seg_id_leer);
                    uint32_t dir_fisica = seg_l->base + desp_leer;
                    log_info(loggerMemory, "## PID: %d - Lectura - Dir. Física: %d - Tamaño: %d", pid, dir_fisica, bytes);
                    t_buffer* buf = buffer_create(0);
                    buffer_add(buf, datos, bytes);
                    t_paquete* respuesta = crear_paquete(LEER_BYTES, buf);
                    enviar_paquete(socket, respuesta);
                    eliminar_paquete(respuesta);
                } else {
                    t_paquete* respuesta = crear_paquete(LECTURA_FALLIDA, NULL);
                    enviar_paquete(socket, respuesta);
                    eliminar_paquete(respuesta);
                }
                free(datos);
                break;
            }
            case SOLICITAR_SEGMENTO:{
                t_mem_alloc* req = deserializar_mem_alloc(paquete->buffer);
                eliminar_paquete(paquete);
                op_code resultado = crear_segmento(req->pid, req->segmentoId, req->tamanio);
                free(req);
                t_paquete* resp = crear_paquete(resultado, NULL);
                enviar_paquete(socket, resp);
                eliminar_paquete(resp);
                break;
            }
            case RESOLICITAR_SEGMENTO:{
                t_mem_alloc* req = deserializar_mem_alloc(paquete->buffer);
                eliminar_paquete(paquete);
                op_code resultado = crear_segmento(req->pid, req->segmentoId, req->tamanio);
                free(req);
                t_paquete* resp = crear_paquete(resultado, NULL);
                enviar_paquete(socket, resp);
                eliminar_paquete(resp);
                if (resultado == MEMORIA_DISPONIBLE)notificar_desuspendibles();
                break;
            }
            case MEM_FREE:{
                t_mem_free* infoFree = deserializar_mem_free(paquete->buffer);
                eliminar_paquete(paquete);
                eliminar_segmento(infoFree->pid, infoFree->segmentoId);
                free(infoFree);
                notificar_desuspendibles();
                break;
            }
            case PROCESOS_DESALOJADOS:{
                eliminar_paquete(paquete);
                usleep(compaction_delay * 1000);
                compactar();
                t_paquete* resp = crear_paquete(COMPACTACION_EXITOSA, NULL);
                enviar_paquete(socket, resp);
                eliminar_paquete(resp);
                break;
            }
            case SUSPENDER_PROCESO:{
                uint32_t pid = buffer_read_uint32(paquete->buffer);
                eliminar_paquete(paquete);
                uint32_t bytes_suspendidos = 0;
                op_code resultado = suspender_proceso(pid, &bytes_suspendidos);//aca se suspende y se modifica el valor de bytes suspendidos
                if (resultado == SUSPEND_OK) notificar_desuspendibles();
                t_buffer* bufResp = buffer_create(0);
                buffer_add_uint32(bufResp, bytes_suspendidos);
                t_paquete* resp = crear_paquete(resultado, bufResp);
                enviar_paquete(socket, resp);
                eliminar_paquete(resp);
                break;
            }
            case DESUSPENDER_PROCESO:{
                uint32_t pid = buffer_read_uint32(paquete->buffer);
                uint32_t tamCadena = buffer_read_uint32(paquete->buffer);
                uint32_t dirLogica = 0;
                char* cadena = NULL;
                if (tamCadena > 0) {
                    dirLogica = buffer_read_uint32(paquete->buffer);
                    cadena = buffer_read_string(paquete->buffer, tamCadena);
                }
                eliminar_paquete(paquete);
                op_code resultado = desuspender_proceso(pid);
                if (resultado == DESUSPEND_OK && cadena != NULL) {
                    escribir_bytes_en_memoria(pid, dirLogica, tamCadena, cadena);
                }
                free(cadena);
                t_paquete* resp = crear_paquete(resultado, NULL);//desp tendria q poner el caso donde no se pudo guiardar en swap, ahi nose que tendria q hacer
                enviar_paquete(socket, resp);
                eliminar_paquete(resp);
                break;
            }
            case SOLICITAR_PROCS_DESUSPENDIBLES:{
                notificar_desuspendibles();
                eliminar_paquete(paquete);
                break;
            }
            default:
                log_warning(loggerMemory, "Opcode desconocido del Scheduler: %d", paquete->codigo_operacion);
                eliminar_paquete(paquete);
                break;
        }
    }

    log_warning(loggerMemory, "Scheduler desconectado");
    return NULL;
}

// Loop que atiende a un CPU: todos los mensajes llegan como paquete con pid en el buffer.
void* atender_cpu(void* arg){
    int socket = *(int*)arg;
    free(arg);

    t_paquete* paquete;
    while ((paquete = recibir_paquete(socket)) != NULL) {
        op_code opcode = paquete->codigo_operacion;
        uint32_t pid   = buffer_read_uint32(paquete->buffer);

        switch (opcode) {
            case OBTENER_CONTEXTO:
                enviar_contexto_cpu(socket, pid);
                eliminar_paquete(paquete);
                break;
            case ACTUALIZAR_CONTEXTO:
                recibir_contexto_cpu(pid, paquete->buffer);
                eliminar_paquete(paquete);
                break;
            case OBTENER_INSTRUCCION:{
                uint32_t pc = buffer_read_uint32(paquete->buffer);
                usleep(instruction_delay * 1000);
                enviar_instruccion_cpu(socket, pid, pc);
                eliminar_paquete(paquete);
                break;
            }
            default:
                log_warning(loggerMemory, "Opcode desconocido del CPU: %d", opcode);
                eliminar_paquete(paquete);
                break;
        }
    }

    pthread_mutex_lock(&mutex_sockets_cpu_notif);
    for (int i = 0; i < list_size(lista_sockets_cpu_notif); i++) {
        t_cpu_conexiones* cpuDesconectado = list_get(lista_sockets_cpu_notif, i);
        if (cpuDesconectado->socket_main == socket) {
            if (cpuDesconectado->socket_notif != -1) close(cpuDesconectado->socket_notif);
            free(cpuDesconectado->cpu_id);
            list_remove_and_destroy_element(lista_sockets_cpu_notif, i, free);
            break;
        }
    }
    pthread_mutex_unlock(&mutex_sockets_cpu_notif);

    close(socket);

    log_warning(loggerMemory, "CPU desconectado");
    return NULL;
}

void* hilo_notificaciones_cpu(void* arg){
    while (1) {
        int socket = esperar_cliente(socketEscuchaNotif);
        handshake_servidor_id(socket, 0);
        int sizeCpuId;
        recv(socket, &sizeCpuId, sizeof(int), MSG_WAITALL);
        char* cpuId = malloc(sizeCpuId);
        recv(socket, cpuId, sizeCpuId, MSG_WAITALL);
        log_info(loggerMemory, "CPU %s Conectada a Kernel Memory Notificaciones", cpuId);


        pthread_mutex_lock(&mutex_sockets_cpu_notif);
        for(int i =0; i<list_size(lista_sockets_cpu_notif);i++){
            t_cpu_conexiones* cpu = list_get(lista_sockets_cpu_notif, i);
            if(strcmp(cpu->cpu_id, cpuId) ==0){
                cpu->socket_notif = socket;
                break;
            }
        }
        pthread_mutex_unlock(&mutex_sockets_cpu_notif);
        free(cpuId);

        //Mando la lista actual de sticks para que la CPU no se pierda los que ya estaban conectados
        pthread_mutex_lock(&memoria_mutex);
        t_buffer* bufSticks = serializar_aviso_nuevo_stick(lista_memory_sticks);
        pthread_mutex_unlock(&memoria_mutex);
        t_paquete* avisoInicial = crear_paquete(AVISO_NUEVO_STICK, bufSticks);
        enviar_paquete(socket, avisoInicial);
        eliminar_paquete(avisoInicial);
    }
}