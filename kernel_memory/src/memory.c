#include "memory.h"

// Definicion de variables globales declaradas en memory_gestor.h
char*   puertoEscucha;
char*   scriptsBasePath;
t_list* lista_procesos;

int main(int argc, char* argv[]) {

    if (argc < 2) {
        printf("Falta path de configuracion");
        return EXIT_FAILURE;
    }

    inicializar_log_y_config(argv[1]);

    puertoEscucha   = config_get_string_value(configMemory, "PUERTO_MEMORY");
    scriptsBasePath = config_get_string_value(configMemory, "SCRIPTS_BASEPATH");

    lista_procesos = list_create();

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
                // TODO: implementar atender_memory_stick cuando este listo con hilos me imagino
                log_warning(loggerMemory, "Memory Stick conectado - num nose arce PONETE A LABURAR");
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
    *quien_out = (modulo)id; // est oes para que la viable modulo del main se modifique, para 

    switch (id) {
        case CPU: {
            // El cliente CPU manda ademas un string con su identificador
            int sizeCpuId;
            recv(socket, &sizeCpuId, sizeof(int), MSG_WAITALL);
            char* cpuId = malloc(sizeCpuId);
            recv(socket, cpuId, sizeCpuId, MSG_WAITALL);
            log_info(loggerMemory, "## CPU %s Conectada", cpuId);
            free(cpuId);
            break;
        }
        case MEMORY_STICK:
            log_info(loggerMemory, "Memory Stick conectado");
            break;
        default:
            log_error(loggerMemory, "Modulo inesperado intentó conectarse al loop: %d", id);
            close(socket);
            break;
    }

    return socket;
}

//Loop que atiende al scheduler:
//KM → Scheduler: ok (int, 1=exito 0=error)
void* atender_scheduler(void* arg) {
    int socket = *(int*)arg;
    free(arg);

    while (1) {
        uint32_t pid;
        char* path = recibir_path(socket, &pid);
        if (!path) break; // scheduler cerro conexion

        int ok = inicializar_proceso(pid, path);
        free(path);
        send(socket, &ok, sizeof(int), 0);
    }

    log_warning(loggerMemory, "Scheduler desconectado");
    return NULL;
}

// Loop que atiende a un CPU:
// Respeustas:
//     OBTENER_CONTEXTO    → KM responde con paquete
//     ACTUALIZAR_CONTEXTO → CPU envia ademas un paquete completo con el contexto
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
                // recibir_contexto_cpu llama a recibir_paquete internamente para leer el contexto
                recibir_contexto_cpu(socket, pid);
                break;
            case OBTENER_INSTRUCCION:
                enviar_instruccion_cpu(socket, pid);
                break;
            default:
                // TODO: manejar OBTENER_INSTRUCCION y otros cuando esten implementados
                log_warning(loggerMemory, "Opcode desconocido del CPU: %d", opcode);
                break;
        }
    }

    log_warning(loggerMemory, "CPU desconectado");
    return NULL;
}

char* recibir_path(int socket, uint32_t* pid_out) {
    uint32_t pid, sizeDePath;

    if (recv(socket, &pid, sizeof(uint32_t), MSG_WAITALL) <= 0) return NULL;
    if (recv(socket, &sizeDePath, sizeof(uint32_t), MSG_WAITALL) <= 0) return NULL;

    char* path = malloc(sizeDePath);
    if (recv(socket, path, sizeDePath, MSG_WAITALL) <= 0) {
        free(path);
        return NULL;
    }

    *pid_out = pid;
    return path;
}
