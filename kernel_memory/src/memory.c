#include "memory.h"
#include <sys/epoll.h>


int main(int argc, char* argv[]) {

    if (argc < 2) {
        printf("Falta path de configuracion");
        return EXIT_FAILURE;
    }

    inicializar_log_y_config(argv[1]);

    puertoEscucha = config_get_string_value(configMemory, "PUERTO_MEMORY");
    puertoEscuchaNotif = config_get_string_value(configMemory, "PUERTO_MEMORY_NOTIF");
    scriptsBasePath = config_get_string_value(configMemory, "SCRIPTS_BASEPATH");
    segment_max_size = (uint32_t)config_get_int_value(configMemory, "SEGMENT_MAX_SIZE");
    allocation_strategy = config_get_string_value(configMemory, "ALLOCATION_STRATEGY");
    instruction_delay = (uint32_t)config_get_int_value(configMemory, "INSTRUCTION_DELAY");
    compaction_delay = (uint32_t)config_get_int_value(configMemory, "COMPACTION_DELAY");

    lista_procesos = list_create();
    lista_memory_sticks = list_create();
    lista_huecos = list_create();
    memoria_total_size  = 0;
    memoria_libre_size  = 0;
    socketScheduler = -1;
    socketSchedulerNotif = -1;
    socketSwap = -1;
    epoll_fd_sticks = -1;
    pthread_mutex_init(&memoria_mutex, NULL);
    pthread_mutex_init(&procesos_mutex, NULL);
    pthread_cond_init(&cond_hay_stick, NULL);

    int socketEscucha = iniciar_servidor(puertoEscucha);
    if (socketEscucha == EXIT_FAILURE) {
        log_error(loggerMemory, "No se pudo iniciar el servidor");
        return EXIT_FAILURE;
    }
    log_info(loggerMemory, "Servidor iniciado en puerto %s", puertoEscucha);

    int socketEscuchaNotif = iniciar_servidor(puertoEscuchaNotif);
    if (socketEscuchaNotif == EXIT_FAILURE) {
        log_error(loggerMemory, "No se pudo iniciar el servidor de notificaciones");
        return EXIT_FAILURE;
    }
    log_info(loggerMemory, "Servidor de notificaciones iniciado en puerto %s", puertoEscuchaNotif);

    // El scheduler se conecta primero en ambos puertos (uno para tema procesos y otra para gestion de conexion o desconexion de sticks)
    socketScheduler = aceptar_cliente(socketEscucha, loggerMemory);
    log_info(loggerMemory, "## Kernel Scheduler Conectado - FD del socket: %d", socketScheduler);

    socketSchedulerNotif = esperar_cliente(socketEscuchaNotif);
    log_info(loggerMemory, "## Kernel Scheduler Conectado al canal de notificaciones - FD: %d", socketSchedulerNotif);

    socketSwap = aceptar_cliente(socketEscucha, loggerMemory);
    log_info(loggerMemory, "## Swap Conectado - FD del socket: %d", socketSwap);

    // El swap envía su tamaño total y el tamaño de cada bloque al conectarse
    t_paquete* swapInit = recibir_paquete(socketSwap);
    swap_total_size = buffer_read_uint32(swapInit->buffer);
    swap_block_size = buffer_read_uint32(swapInit->buffer);
    eliminar_paquete(swapInit);
    inicializar_swap();

    // Inicializar epoll para monitoreo de desconexion de memory sticks sin espera activa
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

    // Loop principal: acepta CPUs y Memory Sticks
    while (1) {
        int socket = esperar_cliente(socketEscucha);
        int32_t id = handshake_servidor_id(socket, 0);

        switch (id) {
            case CPU: {
                int sizeCpuId;
                recv(socket, &sizeCpuId, sizeof(int), MSG_WAITALL);
                char* cpuId = malloc(sizeCpuId);
                recv(socket, cpuId, sizeCpuId, MSG_WAITALL);
                log_info(loggerMemory, "## CPU %s Conectada", cpuId);
                free(cpuId);
                send(socket, &segment_max_size, sizeof(uint32_t), 0);
                int* arg = malloc(sizeof(int));
                *arg = socket;
                pthread_t hilo;
                pthread_create(&hilo, NULL, atender_cpu, arg);
                pthread_detach(hilo);
                break;
            }
            case MEMORY_STICK: {
                uint32_t stick_size = 0;
                recv(socket, &stick_size, sizeof(uint32_t), MSG_WAITALL);
                agregar_memory_stick(socket, stick_size);
                log_info(loggerMemory, "## Memory Stick de %d bytes Conectada", stick_size);
                break;
            }
            default:
                log_warning(loggerMemory, "Modulo desconocido conectado: %d", id);
                close(socket);
                break;
        }
    }

    return 0;
}

void avisar_nueva_memoria() {
    t_buffer* buf = buffer_create(0);
    buffer_add_uint32(buf, memoria_libre_size);
    t_paquete* aviso = crear_paquete(NUEVA_MEMORIA_DISPONIBLE, buf);
    enviar_paquete(socketSchedulerNotif, aviso);
    eliminar_paquete(aviso); 
}

void* atender_scheduler(void* arg) {
    int socket = *(int*)arg;
    free(arg);

    t_paquete* paquete;
    while ((paquete = recibir_paquete(socket)) != NULL) {
        switch ((op_code)paquete->codigo_operacion) {
            case PATH_PROCESO: {
                uint32_t pid = buffer_read_uint32(paquete->buffer);
                uint32_t sizePath = buffer_read_uint32(paquete->buffer);
                char*    path = buffer_read_string(paquete->buffer, sizePath);
                eliminar_paquete(paquete);

                int ok = inicializar_proceso(pid, path);
                free(path);
                send(socket, &ok, sizeof(int), 0);
                break;
            }
            case FIN_PROCESO: {
                uint32_t pid = buffer_read_uint32(paquete->buffer);
                eliminar_paquete(paquete);
                finalizar_proceso(pid);
                avisar_nueva_memoria();
                break;
            }
            case ESCRIBIR_BYTES: {
                t_stdin_stdout* req = deserializar_stdin(paquete->buffer);
                eliminar_paquete(paquete);

                t_proceso_memory* proc = buscar_proceso(req->pid);
                op_code ok = ESCRITURA_FALLIDA;
                if (proc == NULL) {
                    log_error(loggerMemory, "PID: %d - proceso no encontrado en ESCRIBIR_BYTES", req->pid);
                }
                if (proc != NULL) {
                    pthread_mutex_lock(&memoria_mutex);
                    log_info(loggerMemory, "PID: %d - ESCRIBIR_BYTES: tabla tiene %d segmentos, sticks: %d", req->pid, list_size(proc->contexto->tabla_segmentos), list_size(lista_memory_sticks));
                    for (int _i = 0; _i < list_size(proc->contexto->tabla_segmentos); _i++) {
                        t_segmento* _s = list_get(proc->contexto->tabla_segmentos, _i);
                        log_info(loggerMemory, "  seg[%d]: id=%d base=%d limite=%d", _i, _s->id_segmento, _s->base, _s->limite);
                    }
                    uint32_t _seg_id = req->direccionLogica / segment_max_size;
                    uint32_t _desp   = req->direccionLogica % segment_max_size;
                    log_info(loggerMemory, "  busca seg_id=%d desp=%d bytes=%d segment_max_size=%d", _seg_id, _desp, req->bytesALeer, segment_max_size);
                    t_list* pedazos = traducir_logica_a_fisica(req->direccionLogica, segment_max_size, proc->contexto->tabla_segmentos, lista_memory_sticks, req->bytesALeer);
                    pthread_mutex_unlock(&memoria_mutex);

                    if (pedazos == NULL) {
                        log_error(loggerMemory, "PID: %d - traducir_logica_a_fisica retorno NULL para dir %d, bytes %d", req->pid, req->direccionLogica, req->bytesALeer);
                    }
                    if (pedazos != NULL) {
                        ok = escribir_pedazos(pedazos, req->cadenaLeida);
                        if (ok == ESCRIBIR_BYTES)
                            log_info(loggerMemory, "PID: %d - Escritura - Dir. Logica: %d - Tamanio: %d", req->pid, req->direccionLogica, req->bytesALeer);
                        list_destroy_and_destroy_elements(pedazos, free);
                    }
                }
                free(req->cadenaLeida);
                free(req);
                t_paquete* paqueteResp = crear_paquete(ok,NULL);
                enviar_paquete(socket,paqueteResp);
                break;
            }
            case LEER_BYTES: {
                uint32_t pid = buffer_read_uint32(paquete->buffer);
                uint32_t dir_logica = buffer_read_uint32(paquete->buffer);
                uint32_t bytes = buffer_read_uint32(paquete->buffer);
                eliminar_paquete(paquete);

                t_proceso_memory* proc = buscar_proceso(pid);
                if (proc == NULL) {
                    t_paquete* respuesta = crear_paquete(LECTURA_FALLIDA, NULL);
                    enviar_paquete(socket, respuesta);
                    eliminar_paquete(respuesta);
                    break;
                }

                pthread_mutex_lock(&memoria_mutex);
                t_list* pedazos = traducir_logica_a_fisica(dir_logica, segment_max_size, proc->contexto->tabla_segmentos, lista_memory_sticks, bytes);
                pthread_mutex_unlock(&memoria_mutex);

                if (pedazos == NULL) {
                    t_paquete* respuesta = crear_paquete(LECTURA_FALLIDA, NULL);
                    enviar_paquete(socket, respuesta);
                    eliminar_paquete(respuesta);
                    break;
                }

                char* datos = calloc(bytes, 1);
                int ok = leer_pedazos(pedazos, datos);
                list_destroy_and_destroy_elements(pedazos, free);

                if (ok > 0) {
                    log_info(loggerMemory, "PID: %d - Lectura - Dir. Logica: %d - Tamanio: %d", pid, dir_logica, bytes);
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
            case SOLICITAR_SEGMENTO: {
                t_mem_alloc* req = deserializar_mem_alloc(paquete->buffer);
                eliminar_paquete(paquete);
                op_code resultado = crear_segmento(req->pid, req->segmentoId, req->tamanio);
                free(req);
                t_paquete* resp = crear_paquete(resultado, NULL);
                enviar_paquete(socket, resp);
                eliminar_paquete(resp);
                break;
            }
            case MEM_FREE: {
                t_mem_free* infoFree = deserializar_mem_free(paquete->buffer);
                eliminar_paquete(paquete);
                eliminar_segmento(infoFree->pid, infoFree->segmentoId);
                free(infoFree);
                avisar_nueva_memoria();
                break;
            }
            case PROCESOS_DESALOJADOS: {
                eliminar_paquete(paquete);
                usleep(compaction_delay * 1000);
                compactar();
                t_paquete* resp = crear_paquete(COMPACTACION_EXITOSA, NULL);
                enviar_paquete(socket, resp);
                eliminar_paquete(resp);
                break;
            }
            case SUSPENDER_PROCESO: {
                uint32_t pid = buffer_read_uint32(paquete->buffer);
                eliminar_paquete(paquete);
                uint32_t bytes_suspendidos = 0;
                op_code resultado = suspender_proceso(pid, &bytes_suspendidos);//aca se suspende y se modifica el valor de bytes suspendidos
                if (resultado == SUSPEND_OK) avisar_nueva_memoria();
                t_buffer* bufResp = buffer_create(0);
                buffer_add_uint32(bufResp, bytes_suspendidos);
                t_paquete* resp = crear_paquete(resultado, bufResp);
                enviar_paquete(socket, resp);
                eliminar_paquete(resp);
                break;
            }
            case DESUSPENDER_PROCESO: {
                uint32_t pid = buffer_read_uint32(paquete->buffer);
                eliminar_paquete(paquete);
                op_code resultado = desuspender_proceso(pid);
                t_paquete* resp = crear_paquete(resultado, NULL);//desp tendria q poner el caso donde no se pudo guiardar en swap, ahi nose que tendria q hacer
                enviar_paquete(socket, resp);
                eliminar_paquete(resp);
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
void* atender_cpu(void* arg) {
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
            case OBTENER_INSTRUCCION: {
                uint32_t pc = buffer_read_uint32(paquete->buffer);
                // Bloquear sin espera activa hasta que haya al menos un memory stick conectado
                pthread_mutex_lock(&memoria_mutex);
                while (list_size(lista_memory_sticks) == 0)
                    pthread_cond_wait(&cond_hay_stick, &memoria_mutex);
                pthread_mutex_unlock(&memoria_mutex);
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

    log_warning(loggerMemory, "CPU desconectado");
    return NULL;
}
