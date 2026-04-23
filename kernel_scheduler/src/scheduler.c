#include <scheduler.h>



int main(int argc, char* argv[]) { //argv[1]: Path al config, argv[2]: path al proceso inicial. [ejs: ./bin/kernel_scheduler kernel.config ./procesos/init.prog]

    
    inicializar(argv[1]); //loggers y configs

//LEVANTAR CONEXION CON MEMORY
    socketConexionMemory = iniciar_conexion(IPMemory, puertoMemory);
    if(socketConexionMemory == EXIT_FAILURE){
        log_info(loggerScheduler, "no se pudo conectar a Kernel Memory");
        abort();//ver si hay que abortar
    }

    log_info(loggerScheduler, "Conectado a Kernel Memory");
    
    //hacer handshake cuando juani implemente:
    handshake_cliente_id(socketConexionMemory, loggerScheduler, SCHEDULER);
   
//LEVANTAR SERVIDOR
    int socketEscucha = iniciar_servidor(puertoEscucha);
    if(socketEscucha == EXIT_FAILURE){
        log_info(loggerScheduler, "No se pudo iniciar el servidor");
        abort();//ver si hay que abortar
    }
    log_info(loggerScheduler, "Servidor iniciado");

    crear_proceso(0, argv[2], 0); //por el momento asignamos random el de prioridad xq recien lo usamos para CMN, en este proceso debera ser siempre la maxima

    while(1){
        int socketCliente = aceptar_cliente(socketEscucha, loggerScheduler);

        int* socket_ptr = malloc(sizeof(int));
        *socket_ptr = socketCliente;

        pthread_t hilo;
        pthread_create(&hilo, NULL, atender_cliente, socket_ptr);
        pthread_detach(hilo);
    }


    //Liberamos memoria
    close(socketConexionMemory);
    config_destroy(configScheduler);
    log_destroy(loggerScheduler);

    return 0;
}


void *atender_cliente(void *arg){//lo que recibe es el socket cliente (con el que se comunican)
    int socketCliente = *(int*) arg;
    free(arg); 

    //atender peticion o algo asi

    close(socketCliente);
}


//nose si es necec retornar el pcb
t_pcb* crear_proceso(uint32_t pid, char* path, int prioridad){
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->pid = pid;
    pcb->prioridad = prioridad;

    // 2. Meterlo en NEW y loguear
    list_add(new_lista, pcb); 
    log_info(loggerScheduler, "## (%d) Se crea el proceso - Estado: NEW", pcb->pid);

    // 3. Avisar al KM (mandar pid + path por socket)
    enviar_path_proceso_memory(pcb->pid, path);

    // 4. Esperar respuesta OK del KM
    int ok = recibir_ok_memory();

    // 5. Mover a READY y loguear
    if(ok == 0){
        list_remove_element(new_lista, pcb);
        free(pcb);
        log_error(loggerScheduler, "(%d) Error al crear proceso en KM", pid);
        return NULL;
    }
    list_remove_element(new_lista, pcb);
    queue_push(ready_cola, pcb);
    log_info(loggerScheduler, "## (%d) Pasa del estado NEW al estado READY", pcb->pid);

    return pcb;
}


void enviar_path_proceso_memory(uint32_t pid, char* path){
    uint32_t sizePath = strlen(path)+1;
    send(socketConexionMemory, &pid, sizeof(uint32_t), 0);
    send(socketConexionMemory, &sizePath, sizeof(uint32_t),0);
    send(socketConexionMemory, path, sizePath,0);
}

int recibir_ok_memory(){
    int resultado;
    recv(socketConexionMemory, &resultado,sizeof(int),0);
    if(!resultado){
        return 0;
    }
    return 1;
}








