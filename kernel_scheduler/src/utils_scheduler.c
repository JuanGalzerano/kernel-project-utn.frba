#include "utils_scheduler.h"


t_pcb* crear_proceso(uint32_t pid, char* path, int prioridad){
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->pid = pid;
    pcb->prioridad = prioridad;

    //Meterlo en NEW y loguear
    //DESPUES VER POR QUE ESTA LISTA ES BASTANTE INNECESARIA
    /*pthread_mutex_lock(&new_mutex);
    list_add(new_lista, pcb); 
    pthread_mutex_unlock(&new_mutex);*/
    log_info(loggerScheduler, "## (%d) Se crea el proceso - Estado: NEW", pcb->pid);

    //Avisar al KM (mandar pid + path)
    pthread_mutex_lock(&mutex_socket_memory);
    enviar_path_proceso_memory(pcb->pid, path);

    //Esperar respuesta OK del KM
    int ok = recibir_ok_memory();

    pthread_mutex_unlock(&mutex_socket_memory);

    //Mover a READY y loguear
    if(ok == 0){/*
        pthread_mutex_lock(&new_mutex);
        list_remove_element(new_lista, pcb);
        pthread_mutex_unlock(&new_mutex);*/
        free(pcb);
        log_error(loggerScheduler, "(%d) Error al crear proceso en KM", pid);
        return NULL;
    }
    else{
        pthread_mutex_lock(&ready_mutex);
        queue_push(ready_cola, pcb);
        pthread_mutex_unlock(&ready_mutex);
        sem_post(&sem_hay_proceso_ready);
        log_info(loggerScheduler, "## (%d) Pasa del estado NEW al estado READY", pcb->pid);

        return pcb;
    }
    
}


int recibir_ok_memory(){
    int resultado;
    recv(socketConexionMemory, &resultado,sizeof(int),0);//el OK no me lo mandes por paquete juani, pq si o si es lo siguiente a recibir
    if(!resultado){
        return 0;
    }
    return 1;
}

t_cpu_exec* obtener_cpu_libre(){
    pthread_mutex_lock(&exec_mutex);
    t_cpu_exec* cpu = NULL;

    for(int i = 0; i<list_size(exec_lista); i++){
        t_cpu_exec* entrada = list_get(exec_lista, i);
        if (entrada->pcb == NULL) {
            cpu= entrada;
            break;
        }
    }
    pthread_mutex_unlock(&exec_mutex);
    return cpu;
}


void enviar_proceso_a_cpu(t_cpu_exec* cpu,t_pcb* pcb){//el socket esta en la cpu
    cpu->pcb = pcb;

    //se le comunica a la CPU que debe correr este proceso
    t_buffer* buffer = buffer_create(0);
    buffer_add_uint32(buffer, cpu->pcb->pid);
    t_paquete* paquete = crear_paquete(EJECUTAR_PROCESO, buffer);
    enviar_paquete(cpu->socketConexion, paquete);
    //creo q hay destruir el paquete y buffer

    //hacer log de que se pasa a running
    log_info(loggerScheduler, "## (%d) Pasa del estado READY al estado EXEC", pcb->pid);

    free(buffer->stream);
    free(buffer);
    free(paquete);
}

// Re-despacha al mismo CPU el PID que ya tiene asignado, sin cambiar su estado.
// Se usa para syscalls no bloqueantes (ej. INIT_PROC) donde el proceso padre
// debe seguir ejecutando inmediatamente.
void reanudar_proceso_en_cpu(t_cpu_exec* cpu) {
    t_buffer* buffer = buffer_create(0);
    buffer_add_uint32(buffer, cpu->pcb->pid);
    t_paquete* paquete = crear_paquete(EJECUTAR_PROCESO, buffer);
    enviar_paquete(cpu->socketConexion, paquete);

    free(buffer->stream);
    free(buffer);
    free(paquete);
}

uint32_t generar_pid() {
    pthread_mutex_lock(&mutex_pid);
    uint32_t pid = proximo_pid++;
    pthread_mutex_unlock(&mutex_pid);
    return pid;
}

t_cpu_exec* encontrar_cpu_con_pid(uint32_t pid){//entiendo que si se llama a esta funcion, es xq la cpu esta ejecutando, por lo que no contemplo el caso en las lamadas a la funcion que se pueda retornar NULL
    t_cpu_exec* cpu = NULL;
        for(int i = 0; i < list_size(exec_lista); i++){
            t_cpu_exec* entrada = list_get(exec_lista, i);
            if(entrada->pcb != NULL && entrada->pcb->pid == pid){
                cpu = entrada;
                break;
            }
        }

    return cpu;
}

void recibir_tipo_IO(int socketCliente){
    tipo_IO tipo;
    recv(socketCliente, &tipo, sizeof(tipo_IO), MSG_WAITALL); // ni bien se conecta la IO, envia que tipo es

    if(tipo == TIPO_SLEEP){
        socketSleep = socketCliente;
        sem_post(&sem_sleep_disponible);
    }
    if(tipo == TIPO_STDIN){
        socketStdin = socketCliente;
        sem_post(&sem_stdin_disponible);
    }
    if(tipo == TIPO_STDOUT){
        socketStdout = socketCliente;
        sem_post(&sem_stdout_disponible); 
    }

    //no se usa mutex aunque sean globales xq solo se conectara 1 IO x cada tipo
    //tampoco se contempla que una CPU solicite un tipo de IO que no esta corriendo, ya que se dijo que en caso de ser necesaria, esta estara corriendo

}


void bloquear_proceso(t_pcb* pcbBlock){
    pthread_mutex_lock(&exec_mutex);
    t_cpu_exec* cpu = encontrar_cpu_con_pid(pcbBlock->pid); 
    
    //t_pcb* pcbBlockeado = cpu->pcb; comentado pq por ahora creo que no se usa, creo qie en el list add es lo mismo cual use.
    cpu->pcb = NULL;
    pthread_mutex_unlock(&exec_mutex);

    //le avisamos a la cpu que se bloqueo el proceso
    t_buffer* buf = buffer_create(0);
    buffer_add_uint32(buf, pcbBlock->pid);
    t_paquete* unPaquete = crear_paquete(PROCESO_BLOQUEADO, buf);
    enviar_paquete(cpu->socketConexion, unPaquete);//cuidado con la race condition del cpu->socketConexion

    pthread_mutex_lock(&block_mutex);
    list_add(block_lista, pcbBlock); //cuando implemente plani a mediado plazo, aca voy a tener que correr el hilo para ver si va a susp block
    pthread_mutex_unlock(&block_mutex);
    
    log_info(loggerScheduler, "## (%d) Pasa del estado EXEC al estado BLOCK", pcbBlock->pid);

    free(buf->stream);
    free(buf);
    free(unPaquete);
}

t_pcb* buscar_y_sacar_de_block(uint32_t pid){
    pthread_mutex_lock(&block_mutex);
    
    t_pcb* pcb = NULL;
    for(int i = 0; i < list_size(block_lista); i++){
        t_pcb* entrada = list_get(block_lista, i);
        if(entrada->pid == pid){
            pcb = entrada;
            list_remove(block_lista, i);
            break;
        }
    }
    
    pthread_mutex_unlock(&block_mutex);
    return pcb;
}

void enviar_fin_proceso_memory(uint32_t pid){ //esta la voy a usar tmb para el de READY->EXIT Y BLOCK -> EXIT
    t_buffer* buffer = buffer_create(0);
    buffer_add_uint32(buffer, pid);
    t_paquete* paquete = crear_paquete(FIN_PROCESO, buffer); //avisarle a juani que haga este case para  que libere todos los segmentos y estructuras asociadas a ese PID
    pthread_mutex_lock(&mutex_socket_memory);
    enviar_paquete(socketConexionMemory, paquete);
    pthread_mutex_unlock(&mutex_socket_memory);
    free(buffer->stream);
    free(buffer);
    free(paquete);
}

void enviar_fin_proceso_a_cpu(uint32_t pid, int socketCPU){ 
    t_buffer* buffer = buffer_create(0);
    buffer_add_uint32(buffer, pid);
    t_paquete* paquete = crear_paquete(FIN_PROCESO, buffer); 
    //pthread_mutex_lock(&exec_mutex); desp ver si se genera race condition aca
    enviar_paquete(socketCPU, paquete);
    //pthread_mutex_unlock(&exec_mutex);
    free(buffer->stream);
    free(buffer);
    free(paquete);
}

void* hilo_timer_quantum(void* arg){
    t_timer_args* argumento = (t_timer_args*) arg;

    uint32_t pidCpuOriginal = argumento->pid_original;
    t_cpu_exec* cpu = argumento->cpu;
    free(argumento);

    usleep(quantum*1000);//usleep recibe microsegundos de parametro VER SI ESTA BIEN USAR ESTA FUNCION

    pthread_mutex_lock(&exec_mutex);
    bool sigueEjecutando = (cpu->pcb != NULL && cpu->pcb->pid == pidCpuOriginal);
    pthread_mutex_unlock(&exec_mutex);

    if(sigueEjecutando){
        //pedir a CPU que finalice por quantum
        t_buffer* buffer = buffer_create(0);
        buffer_add_uint32(buffer, pidCpuOriginal);
        t_paquete* paquete = crear_paquete(FINALIZAR_POR_QUANTUM, buffer);
        enviar_paquete(cpu->socketConexion, paquete);
        //creo q hay destruir el paquete y buffer
        free(buffer->stream);
        free(buffer);
        free(paquete);
    }

    return NULL;
}


void iniciar_timer_quantum(t_cpu_exec* cpu){
    t_timer_args* args = malloc(sizeof(t_timer_args));
    args->cpu = cpu;
    args->pid_original=cpu->pcb->pid;

    pthread_t hiloQuantum;
    pthread_create(&hiloQuantum, NULL, hilo_timer_quantum, args);
    pthread_detach(hiloQuantum);
}

void enviar_path_proceso_memory(uint32_t pid, char* path){//decirle a juani que esta va a haber que hacerla con paquete y eso xq sino solo funca para el proceso 0
    uint32_t sizePath = strlen(path)+1;
    t_buffer* buffer = buffer_create(0);
    buffer_add_uint32(buffer, pid);
    buffer_add_uint32(buffer, sizePath);
    buffer_add_string(buffer, sizePath, path);
    t_paquete* paquete = crear_paquete(PATH_PROCESO, buffer);
    enviar_paquete(socketConexionMemory, paquete);
    free(buffer->stream);
    free(buffer);
    free(paquete);

/*
    send(socketConexionMemory, &pid, sizeof(uint32_t), 0);
    send(socketConexionMemory, &sizePath, sizeof(uint32_t),0);
    send(socketConexionMemory, path, sizePath,0);*/
}

char* solicitar_cadena_a_memory(uint32_t pid, uint32_t direccionLogica, uint32_t bytes){
    t_buffer* buffer = buffer_create(0);

    buffer_add_uint32(buffer, pid);
    buffer_add_uint32(buffer, direccionLogica);
    buffer_add_uint32(buffer, bytes);

    t_paquete* unPaquete = crear_paquete(LEER_BYTES, buffer);


    pthread_mutex_lock(&mutex_socket_memory);
    enviar_paquete(socketConexionMemory, unPaquete);

    free(buffer->stream);
    free(buffer);
    free(unPaquete);

    //ver si hay que elimiinar paquete

    char* cadena = malloc(bytes+1);

    recv(socketConexionMemory, cadena, bytes+1, MSG_WAITALL);
    pthread_mutex_unlock(&mutex_socket_memory);

    return cadena;
}

void liberar_mutex_y_semaforos(){

    //pthread_mutex_destroy(&new_mutex);
    pthread_mutex_destroy(&ready_mutex);
    pthread_mutex_destroy(&block_mutex);
    pthread_mutex_destroy(&exec_mutex);
    pthread_mutex_destroy(&mutex_pid);
    pthread_mutex_destroy(&mutex_socket_memory);
    pthread_mutex_destroy(&mutex_cola_sleep);
    pthread_mutex_destroy(&mutex_cola_stdin);
    pthread_mutex_destroy(&mutex_cola_stdout);

    
    sem_destroy(&sem_hay_proceso_ready);
    sem_destroy(&sem_hay_cpu_libre);
    sem_destroy(&sem_sleep_disponible);
    sem_destroy(&sem_stdin_disponible);
    sem_destroy(&sem_stdout_disponible);
    sem_destroy(&sem_hay_proc_esperando_sleep);
    sem_destroy(&sem_hay_proc_esperando_stdin);
    sem_destroy(&sem_hay_proc_esperando_stdout);
}


t_mutex_syscall* buscar_mutex(char* nombreMutex){
    t_mutex_syscall* mutex = NULL;

    for(int i = 0; i<list_size(lista_mutex);i++){
        t_mutex_syscall* otroMutex = list_get(lista_mutex,i);
        if(strcmp(otroMutex->nombreMutex, nombreMutex)){
            mutex = otroMutex;
            return mutex;
        }
    }
    return NULL;
}