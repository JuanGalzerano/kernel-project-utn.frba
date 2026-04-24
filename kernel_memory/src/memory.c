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
    
    //Traer del config
    puertoEscucha   = config_get_string_value(configMemory, "PUERTO_MEMORY");
    scriptsBasePath = config_get_string_value(configMemory, "SCRIPTS_BASEPATH");

    //creo lista donde van a estar los procesos con su id y todos los punteros a sus datos
    lista_procesos = list_create();

    // LEVANTAR SERVIDOR (espera conexiones de Kernel, CPU, Memory Stick y SWAP)
    int socketEscucha = iniciar_servidor(puertoEscucha);
    if (socketEscucha == EXIT_FAILURE) {
        log_info(loggerMemory, "No se pudo iniciar el servidor");
    }
    log_info(loggerMemory, "Servidor iniciado");

    // Estas conexiones son primordiales al principio del run
    int socketScheduler = aceptar_cliente(socketEscucha, loggerMemory);
    int socketSwap = aceptar_cliente(socketEscucha, loggerMemory);

    // Primer proceso: recibir path del Scheduler, inicializar y responder OK
    uint32_t pid;
    char* path = recibir_path(socketScheduler, &pid);
    int ok = inicializar_proceso(pid, path);
    free(path);
    send(socketScheduler, &ok, sizeof(int), 0);

    // while para recibir CPUs y STICKs
    while (1) {
        int socket_cliente = aceptar_cliente(socketEscucha, loggerMemory);

        int* arg = malloc(sizeof(int));
        *arg = socket_cliente;

        pthread_t hilo;
        pthread_create(&hilo, NULL, hacerAlgo, arg);
        pthread_detach(hilo);
    }

    return 0;
}

void* hacerAlgo(void* arg) {
    int socket = *(int*)arg;
    free(arg);

    // logica aca, me imagino que aca recivo y mando ni idea

    return NULL;
}

char* recibir_path(int socket, uint32_t* pid_out) {

    //marotti estuvo aqui
    uint32_t pid, sizeDePath;
    recv(socket, &pid, sizeof(uint32_t), MSG_WAITALL);
    recv(socket, &sizeDePath, sizeof(uint32_t), MSG_WAITALL);
    char* path = malloc(sizeDePath);
    recv(socket, path, sizeDePath, MSG_WAITALL);

    *pid_out = pid;

    //necesito que cuando crees el proceso me hagas un send de 1 si se creo correctamente o 0 si no
    //xq yo necesito ver eso para ver si lo meto en ready o no
    return path;
}

