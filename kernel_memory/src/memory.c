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
    int socketSwap      = aceptar_cliente(socketEscucha, loggerMemory);
    (void)socketSwap; //usar cuando se implemente swap

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
                // La desconexion se detecta en leer/escribir_en_memory_stick cuando recibir_paquete devuelve NULL.
                free(arg);
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

                uint32_t dir_fisica = 0;
                int ok = traducir_y_verificar(req->pid, req->direccionLogica, req->bytesALeer, &dir_fisica);
                if (ok > 0) {
                    ok = escribir_en_memory_stick(dir_fisica, req->bytesALeer, req->cadenaLeida);
                }

                log_info(loggerMemory, "PID: %d - Escritura - Dir. Fisica: %d - Tamanio: %d", req->pid, dir_fisica, req->bytesALeer);
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

                uint32_t dir_fisica = 0;
                int ok = traducir_y_verificar(pid, dir_logica, bytes, &dir_fisica);

                if (ok > 0) {
                    char* datos = calloc(bytes, 1);
                    leer_de_memory_stick(dir_fisica, bytes, datos);
                    log_info(loggerMemory, "PID: %d - Lectura - Dir. Fisica: %d - Tamanio: %d", pid, dir_fisica, bytes);

                    t_buffer* buf = buffer_create(0);
                    buffer_add(buf, datos, bytes);
                    free(datos);
                    t_paquete* respuesta = crear_paquete(LEER_BYTES, buf);
                    enviar_paquete(socket, respuesta);
                    eliminar_paquete(respuesta);
                } else {
                    t_paquete* respuesta = crear_paquete(LECTURA_FALLIDA, NULL);
                    enviar_paquete(socket, respuesta);
                    eliminar_paquete(respuesta);
                }
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
