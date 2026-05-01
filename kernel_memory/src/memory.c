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
    int socketScheduler = aceptar_cliente(socketEscucha, loggerMemory);
    log_info(loggerMemory, "## Kernel Scheduler Conectado - FD del socket: %d", socketScheduler);
    int socketSwap      = aceptar_cliente(socketEscucha, loggerMemory);
    (void)socketSwap; // TODO: usar cuando se implemente swap

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
                // La conexion y registro del stick ya se hizo en aceptar_cliente_memory.
                // El socket queda abierto; por ahora no hay loop de atencion (Memory Stick
                // es pasivo: KM le manda pedidos de lectura/escritura cuando los necesite).
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
            break;
        }
        default:
            log_error(loggerMemory, "Modulo inesperado intento conectarse al loop: %d", id);
            close(socket);
            break;
    }

    return socket;
}

// Loop que atiende al scheduler.
// PATH_PROCESO → inicializar proceso, responde int ok
// FIN_PROCESO  → liberar proceso, sin respuesta
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
            default:
                log_warning(loggerMemory, "Opcode desconocido del Scheduler: %d", paquete->codigo_operacion);
                eliminar_paquete(paquete);
                break;
        }
    }

    log_warning(loggerMemory, "Scheduler desconectado");
    return NULL;
}

// Loop que atiende a un CPU:
// OBTENER_CONTEXTO    → KM responde con paquete
// ACTUALIZAR_CONTEXTO → CPU envia ademas un paquete completo con el contexto
void* atender_cpu(void* arg) {
    int socket = *(int*)arg;
    free(arg);

    op_code opcode;
    uint32_t pid;

    while (recv(socket, &opcode, sizeof(op_code), MSG_WAITALL) > 0) {
        if (recv(socket, &pid, sizeof(uint32_t), MSG_WAITALL) <= 0) break;

        switch (opcode) {
            case OBTENER_CONTEXTO:
                enviar_contexto_cpu(socket, pid);
                break;
            case ACTUALIZAR_CONTEXTO:
                recibir_contexto_cpu(socket, pid);
                break;
            case OBTENER_INSTRUCCION:
                enviar_instruccion_cpu(socket, pid);
                break;
            default:
                log_warning(loggerMemory, "Opcode desconocido del CPU: %d", opcode);
                break;
        }
    }

    log_warning(loggerMemory, "CPU desconectado");
    return NULL;
}
