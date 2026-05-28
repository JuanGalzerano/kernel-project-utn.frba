#include "memory.h"


int main(int argc, char* argv[]) {

    if (argc < 2) {
        printf("Falta path de configuracion");
        return EXIT_FAILURE;
    }

    inicializar_log_y_config(argv[1]);

    puertoEscucha = config_get_string_value(configMemory, "PUERTO_MEMORY");
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
    pthread_mutex_init(&memoria_mutex, NULL);
    pthread_mutex_init(&procesos_mutex, NULL);

    int socketEscucha = iniciar_servidor(puertoEscucha);
    if (socketEscucha == EXIT_FAILURE) {
        log_error(loggerMemory, "No se pudo iniciar el servidor");
        return EXIT_FAILURE;
    }
    log_info(loggerMemory, "Servidor iniciado en puerto %s", puertoEscucha);

    // El scheduler y el swap son los primeros en conectarse (arranque del sistema)
    // Ya sabemos quienes son, no necesitamos identificarlos
    socketScheduler = aceptar_cliente(socketEscucha, loggerMemory);
    log_info(loggerMemory, "## Kernel Scheduler Conectado - FD del socket: %d", socketScheduler);
    // int socketSwap = aceptar_cliente(socketEscucha, loggerMemory); // descomentar cuando swap este implementado

    // Thread dedicado al scheduler: recibe nuevos procesos en loop
    int* argSched = malloc(sizeof(int));
    *argSched = socketScheduler;
    pthread_t hilo_scheduler;
    pthread_create(&hilo_scheduler, NULL, atender_scheduler, argSched);
    pthread_detach(hilo_scheduler);

    // Loop principal: acepta CPUs y Memory Sticks, identifica quien es y le da su thread
    modulo quien;
    while (1) {
        int socket_cliente = aceptar_cliente_memory(socketEscucha, &quien);

        int* arg = malloc(sizeof(int));
        *arg = socket_cliente;

        pthread_t hilo;
        switch (quien) {
            case CPU:
                pthread_create(&hilo, NULL, atender_cpu, arg);
                pthread_detach(hilo);
                break;
            case MEMORY_STICK:
                free(arg);
                /*  EJEMPLO DE ESCRITURA Y LECTURA DE STICK TOTALMENTE FUNCIONAL DEW
                log_info(loggerMemory, "TEST - case MEMORY_STICK alcanzado, sticks en lista: %d", list_size(lista_memory_sticks));

                if (list_size(lista_memory_sticks) >= 1) {
                    t_memory_stick_info* stick = list_get(lista_memory_sticks, 0);
                    char* texto   = "Hola Memory Stick!";
                    uint32_t tam  = strlen(texto) + 1;

                    struct_control_mmu* pedazo = malloc(sizeof(struct_control_mmu));
                    pedazo->socketMemoryStick = stick->socket;
                    pedazo->desde_donde_leer = 0;
                    pedazo->tamanio_a_leer_en_esta_memory_stick = tam;

                    t_list* pedazos = list_create();
                    list_add(pedazos, pedazo);

                    escribir_pedazos(pedazos, texto);
                    log_info(loggerMemory, "TEST escritura - '%s' (%d bytes en offset 0)", texto, tam);

                    char* leido = calloc(tam, 1);
                    leer_pedazos(pedazos, leido);
                    log_info(loggerMemory, "TEST lectura  - '%s'", leido);
                    free(leido);

                    list_destroy_and_destroy_elements(pedazos, free);
                }
                */
                break;
            default:
                log_warning(loggerMemory, "Modulo desconocido conectado: %d", quien);
                free(arg);
                break;
        }
    }

    return 0;
}

// Acepta un cliente, completa el handshake y devuelve quien se conecto via *quien_out.
// Para CPU consume tambien el string de ID que el cliente manda (para no desincronizar el socket).
// Usa loggerMemory directamente por ser global en este modulo.
int aceptar_cliente_memory(int socketEscucha, modulo* quien_out) {
    int socket = esperar_cliente(socketEscucha);

    int32_t id = 0;
    id = handshake_servidor_id(socket, id);
    *quien_out = (modulo)id;

    switch (id) {
        case CPU: {
            int sizeCpuId;
            recv(socket, &sizeCpuId, sizeof(int), MSG_WAITALL);
            char* cpuId = malloc(sizeCpuId);
            recv(socket, cpuId, sizeCpuId, MSG_WAITALL);
            log_info(loggerMemory, "## CPU %s Conectada", cpuId);
            free(cpuId);
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
            log_error(loggerMemory, "Modulo inesperado intento conectarse al loop: %d", id);
            close(socket);
            break;
    }

    return socket;
}



void compactar(void) {
    //implementar compactacion real
}

// Loop que atiende al scheduler.
// PATH_PROCESO → inicializar proceso, responde int ok
// FIN_PROCESO → liberar proceso, sin respuesta
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
                break;
            }
            case ESCRIBIR_BYTES: {
                t_stdin_stdout* req = deserializar_stdin(paquete->buffer);
                eliminar_paquete(paquete);

                t_proceso_memory* proc = buscar_proceso(req->pid);
                int ok = -1;
                if (proc != NULL) {
                    pthread_mutex_lock(&memoria_mutex);
                    t_list* pedazos = traducir_logica_a_fisica(req->direccionLogica, segment_max_size, proc->contexto->tabla_segmentos, lista_memory_sticks, req->bytesALeer);
                    pthread_mutex_unlock(&memoria_mutex);

                    if (pedazos != NULL) {
                        ok = escribir_pedazos(pedazos, req->cadenaLeida);
                        if (ok > 0)
                            log_info(loggerMemory, "PID: %d - Escritura - Dir. Logica: %d - Tamanio: %d", req->pid, req->direccionLogica, req->bytesALeer);
                        list_destroy_and_destroy_elements(pedazos, free);
                    }
                }
                free(req->cadenaLeida);
                free(req);
                send(socket, &ok, sizeof(int), 0);
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
                break;
            }
            case PROCESOS_DESALOJADOS: {
                eliminar_paquete(paquete);
                usleep(compaction_delay * 1000);
                compactar();
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
