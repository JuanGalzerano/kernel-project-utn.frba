#include "memory.h"

int main(int argc, char* argv[]) {

    if (argc < 2) {
    printf("Falta path de configuracion");
    return EXIT_FAILURE; //no va a llegar a menos q alguien 
    }
    //inicializo log y config, desp lo pongo en archivo aparte
    inicializar_log_y_config(argv[1]);
    

    //guardo el valor del puerto de escucha que esta en el config
    puertoEscucha = config_get_string_value(configMemory, "PUERTO_MEMORY");

    // LEVANTAR SERVIDOR (espera conexiones de Kernel, CPU, Memory Stick y SWAP)
    int socketEscucha = iniciar_servidor(puertoEscucha);
    if(socketEscucha == EXIT_FAILURE){
        log_info(loggerMemory, "No se pudo iniciar el servidor");
    }
    log_info(loggerMemory, "Servidor iniciado");

//Estas conexiones son primordiales al principio del run osea que si o si se van a conectar en este orden
//desp cpu y stick ahi si que se van a esperar en un while(1)
    int socketScheduler = aceptar_cliente(socketEscucha, loggerMemory);
    int socketSwap = aceptar_cliente(socketEscucha, loggerMemory);

    

    char* path = recibir_path(socketScheduler); //scheduler me manda la ubicacion del archivo q contiene las instrucciones que va a ejecutar el PID 0
    crear_proceso(0, path); //este tiene que guardar el archivo para cuadno el cpu pida instrucciones e inicializar el contexto de ejecucion (ax,bx, etc)

//while para recibir CPUs y STICKs
    while (1) {
        int socket_cliente = aceptar_cliente(socketEscucha, loggerMemory);

        int* arg = malloc(sizeof(int)); 
        *arg = socket_cliente;  //argumento random para pasarle al hilo generico

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

char* recibir_path(int socket) {
    
//marotti estuvo aqui
    uint32_t pid, sizeDePath;
    recv(socket, &pid, sizeof(uint32_t), MSG_WAITALL);
    recv(socket, &sizeDePath, sizeof(uint32_t), MSG_WAITALL);
    char* path = malloc(sizeDePath);
    recv(socket, path, sizeDePath, MSG_WAITALL);

    //necesito que cuando crees el proceso me hagas un send de 1 si se creo correctamente o 0 si no
    //xq yo necesito ver eso para ver si lo meto en ready o no
  
    return path; //no te quiero cambiar esto pero creo que tmb vas a necesitar retornar el PID, capaz te combiene recibirlos por referencia
}

void crear_proceso(int pid, char* path) {
    // implementación vacía por ahora
    printf("Creando proceso PID %d con path %s\n", pid, path);
}