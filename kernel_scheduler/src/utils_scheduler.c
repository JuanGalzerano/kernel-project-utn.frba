#include "utils_scheduler.h"


t_pcb* crear_proceso(uint32_t pid, char* path, int prioridad){
    if(algoritmo!=CMN || (prioridad<cantidad_colas && prioridad>-1)){
        t_pcb* pcb = malloc(sizeof(t_pcb));
        pcb->pid = pid;
        pcb->prioridad = prioridad;
        pcb->prioridadOriginal=prioridad;

        //loguear que entro a NEW
        log_info(loggerScheduler, "## (%d) Se crea el proceso - Estado: NEW", pcb->pid);

        //Avisar al KM (mandar pid + path)
        pthread_mutex_lock(&mutex_socket_memory);
        enviar_path_proceso_memory(pcb->pid, path);

        //Esperar respuesta OK del KM
        int ok = recibir_ok_memory();

        pthread_mutex_unlock(&mutex_socket_memory);

        //Mover a READY y loguear
        if(ok == 0){
            free(pcb);
            log_error(loggerScheduler, "(%d) Error al crear proceso en KM", pid);
            return NULL;
        }
        else{
            encolar_pcb_ready(pcb);
            sem_post(&sem_hay_proceso_ready);
            log_info(loggerScheduler, "## (%d) Pasa del estado NEW al estado READY", pcb->pid);

            return pcb;
        }
    }
    else{
        log_error(loggerScheduler, "prioridad fuera de rango el pid: %d", pid);
        return NULL;
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

op_code recibir_respuesta_memory(){

    t_paquete* paquete = recibir_paquete(socketConexionMemory);
    op_code codigo = paquete->codigo_operacion;
    free(paquete);
    return codigo;
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
    char* cadena = malloc(bytes);// ver sy es bytes +1 y eso

    t_paquete* respuesta = recibir_paquete(socketConexionMemory);

    
    if(respuesta->codigo_operacion == LECTURA_FALLIDA){
        free(cadena);
        cadena = NULL;
    } else {
        cadena = buffer_read_string(respuesta->buffer, bytes);
    }

    eliminar_paquete(respuesta);
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
    for(int i = 0; i < list_size(lista_mutex); i++){
        t_mutex_syscall* otroMutex = list_get(lista_mutex, i);
        if(strcmp(otroMutex->nombreMutex, nombreMutex) == 0){
            return otroMutex;
        }
    }
    return NULL;
}


op_code solicitar_segmento_memory(t_mem_alloc* infoMemAlloc){
    t_buffer* buffer = serializar_mem_alloc(infoMemAlloc);
    t_paquete* paquete = crear_paquete(SOLICITAR_SEGMENTO, buffer);
    pthread_mutex_lock(&mutex_socket_memory);
    enviar_paquete(socketConexionMemory, paquete);
    eliminar_paquete(paquete);

    t_paquete* respuesta = recibir_paquete(socketConexionMemory);
    op_code codigo = respuesta->codigo_operacion;
    eliminar_paquete(respuesta);

    if(codigo == COMPACTACION) {
        // Hay memoria pero fragmentada: pedir compactacion y esperar que termine
        //aca tendria que hacer la func compactacion() que desaloje todos los procesos y no permita enviarle PROCESOS_DESALOJADOS hasta que no se desaloje todo
        t_paquete* pacComp = crear_paquete(PROCESOS_DESALOJADOS, NULL);
        enviar_paquete(socketConexionMemory, pacComp);
        eliminar_paquete(pacComp);
        t_paquete* compactacionHecha = recibir_paquete(socketConexionMemory);
        eliminar_paquete(compactacionHecha);
        pthread_mutex_unlock(&mutex_socket_memory);

        // Post-compactacion DEBE poder alocar. Si no, BSOD.
        op_code reintentar = solicitar_segmento_memory(infoMemAlloc);
        if(reintentar != MEMORIA_DISPONIBLE) manejar_bsod();//realmente imposible pero bueno en casos especiales, pantallazo azul;xq sino es un bucle de solicitar_segmento_memory
        return reintentar;
    }

    pthread_mutex_unlock(&mutex_socket_memory);
    return codigo;
}



// Escucha el canal dedicado de notificaciones de Kernel Memory (socketMemoryNotif).
// Separado del socketConexionMemory para no interferir con el flujo request/reply.
void* hilo_escuchar_memory(void* arg) {
    (void)arg;
    while(1) {
        t_paquete* paquete = recibir_paquete(socketMemoryNotif);
        if(paquete == NULL) {
            log_error(loggerScheduler, "Se perdio la conexion con el canal de notificaciones de KM");
            break;
        }

        switch(paquete->codigo_operacion) {
            case MEMORIA_CORRUPTA:
                eliminar_paquete(paquete);
                manejar_bsod();
                return NULL;
            case NUEVA_MEMORIA_DISPONIBLE: {
                uint32_t bytes_libres = buffer_read_uint32(paquete->buffer);
                eliminar_paquete(paquete);
                log_info(loggerScheduler, "Memoria disponible: %d bytes libres en KM", bytes_libres);
                //con bytes_libres comparar contra bytes_suspendidos de procesos en SUSP para ver si podes desuspender
                break;
            }
            default:
                log_warning(loggerScheduler, "Opcode desconocido en canal de notificaciones: %d", paquete->codigo_operacion);
                eliminar_paquete(paquete);
                break;
        }
    }
    return NULL;
}


void manejar_bsod() {
    // EXEC
    log_info(loggerScheduler, "## BSOD: Corrupcion de memoria detectada");
    pthread_mutex_lock(&exec_mutex);
    for(int i = 0; i < list_size(exec_lista); i++) {
        t_cpu_exec* cpu = list_get(exec_lista, i);
        if(cpu->pcb != NULL) {
            //ACA ME FALTA AGREGAR AVIDARLE A LA CPU    
            t_paquete* paq = crear_paquete(DESALOJAR_POR_BSOD,NULL);
            enviar_paquete(cpu->socketConexion, paq);
            eliminar_paquete(paq);
            log_info(loggerScheduler, "## (%d) finalizó su ejecución con motivo de Blue Screen of Death (BSOD)", cpu->pcb->pid);
            free(cpu->pcb);
            cpu->pcb = NULL;
        }
    }
    pthread_mutex_unlock(&exec_mutex);

    // READY
    //capaz deberia clavar un mutex entero al while pero me generaria deadlock con el de desencolar_pcb_ready
    while(!queue_is_empty(ready_cola)) {
        t_pcb* pcb = desencolar_pcb_ready();
        log_info(loggerScheduler, "## (%d) finalizó su ejecución con motivo de Blue Screen of Death (BSOD)", pcb->pid);
        free(pcb);
    }
    

    // BLOCK
    pthread_mutex_lock(&block_mutex);
    while(!list_is_empty(block_lista)) {
        t_pcb* pcb = list_remove(block_lista, 0);
        log_info(loggerScheduler, "## (%d) finalizó su ejecución con motivo de Blue Screen of Death (BSOD)", pcb->pid);
        free(pcb);
    }
    pthread_mutex_unlock(&block_mutex);

/*DESCOMENTAR CUANDO HAGA PLANI A MEDIO PLAZO
    // SUSP_BLOCK
    pthread_mutex_lock(&susp_block_mutex);
    while(!list_is_empty(susp_block)) {
        t_pcb* pcb = list_remove(susp_block, 0);
        log_info(loggerScheduler, "## (%d) finalizó su ejecución con motivo de Blue Screen of Death (BSOD)", pcb->pid);
        free(pcb);
    }
    pthread_mutex_unlock(&susp_block_mutex);

    // SUSP_READY
    pthread_mutex_lock(&susp_ready_mutex);
    while(!list_is_empty(susp_ready)) {
        t_pcb* pcb = list_remove(susp_ready, 0);
        log_info(loggerScheduler, "## (%d) finalizó su ejecución con motivo de Blue Screen of Death (BSOD)", pcb->pid);
        free(pcb);
    }
    pthread_mutex_unlock(&susp_ready_mutex);
*/
    log_info(loggerScheduler, "## Kernel Scheduler finalizado por BSOD");
    abort();
}

void encolar_pcb_ready(t_pcb* pcb){
    if(algoritmo == CMN){
        pthread_mutex_lock(&mutex_cola_multinivel);
        queue_push(cola_multinivel[pcb->prioridad], pcb);
        pthread_mutex_unlock(&mutex_cola_multinivel);
    }else{
        pthread_mutex_lock(&ready_mutex);
        queue_push(ready_cola, pcb);
        pthread_mutex_unlock(&ready_mutex);
    }
}

t_pcb* desencolar_pcb_ready(){
    t_pcb* pcb=NULL;
    if(algoritmo==CMN){
        pthread_mutex_lock(&mutex_cola_multinivel);
        for(int i = 0; i< cantidad_colas; i++){
            if(!queue_is_empty(cola_multinivel[i])){
                pcb = queue_pop(cola_multinivel[i]);
                break;
            }
        }
        pthread_mutex_unlock(&mutex_cola_multinivel);
    }else{
        pthread_mutex_lock(&ready_mutex);
        pcb = queue_pop(ready_cola);
        pthread_mutex_unlock(&ready_mutex);
    }
    return pcb;
}

t_pcb* pcb_mas_prioritario(){
    pthread_mutex_lock(&mutex_cola_multinivel);
    t_pcb* pcb = NULL;
    for(int i = 0; i < cantidad_colas; i++) {
        if(!queue_is_empty(cola_multinivel[i])) {
            pcb = queue_peek(cola_multinivel[i]);
            break;
        }
    }
    pthread_mutex_unlock(&mutex_cola_multinivel);
    return pcb;
}

t_cpu_exec* hay_cpu_desalojable(t_pcb* pcbCandidato){
    if(pcbCandidato == NULL) return NULL;
    pthread_mutex_lock(&exec_mutex);
    t_cpu_exec* cpuADesalojar = NULL;
    for(int i = 0; i < list_size(exec_lista); i++) {
        t_cpu_exec* cpu = list_get(exec_lista, i);
        if(cpu->pcb != NULL && cpu->pcb->prioridad > pcbCandidato->prioridad) {
            cpuADesalojar = cpu;
            break;
        }
    }
    pthread_mutex_unlock(&exec_mutex);
    return cpuADesalojar;
}

void enviar_desalojo_cpu(t_cpu_exec* cpuDesalojable){
    t_buffer* buffer = buffer_create(0);
    buffer_add_uint32(buffer, cpuDesalojable->pcb->pid);
    t_paquete* paquete = crear_paquete(DESALOJO, buffer);
    enviar_paquete(cpuDesalojable->socketConexion, paquete);

    free(buffer->stream);
    free(buffer);
    free(paquete);
}

void liberar_cpu_y_notificar() {
    sem_post(&sem_hay_cpu_libre);
    pthread_mutex_lock(&mutex_planificador);
    pthread_cond_signal(&cond_planificador);
    pthread_mutex_unlock(&mutex_planificador);
}


t_pcb* buscar_pcb_por_pid(uint32_t pid, int* estabaEnReady) {
    //buscar en exec_lista
    estabaEnReady = 0; //incializamos en que no estaba
    pthread_mutex_lock(&exec_mutex);
    for(int i = 0; i < list_size(exec_lista); i++) {
        t_cpu_exec* cpu = list_get(exec_lista, i);
        if(cpu->pcb != NULL && cpu->pcb->pid == pid) {
            t_pcb* pcb = cpu->pcb;
            pthread_mutex_unlock(&exec_mutex);
            return pcb;
        }
    }
    pthread_mutex_unlock(&exec_mutex);

    //buscar en ready HACER CASO SI HAY CMN
    
    // buscar en ready (FIFO/RR)
    if(algoritmo != CMN) {
        //aca no tocamos estabaEnReady, pq no necesitariamos reencolar
        pthread_mutex_lock(&ready_mutex);
        for(int i = 0; i < queue_size(ready_cola); i++) {
            t_pcb* pcb = list_get(ready_cola->elements, i);
            if(pcb->pid == pid) {
                pthread_mutex_unlock(&ready_mutex);
                return pcb;
            }
        }
        pthread_mutex_unlock(&ready_mutex);
    } else {
        // buscar en colas multinivel
        pthread_mutex_lock(&mutex_cola_multinivel);
        for(int i = 0; i < cantidad_colas; i++) {
            for(int j = 0; j < queue_size(cola_multinivel[i]); j++) {
                t_pcb* pcb = list_get(cola_multinivel[i]->elements, j);
                if(pcb->pid == pid) {
                    list_remove(cola_multinivel[i]->elements, j);
                    *estabaEnReady=1;//hay que reencolar
                    pthread_mutex_unlock(&mutex_cola_multinivel);
                    return pcb;
                }
            }
        }
        pthread_mutex_unlock(&mutex_cola_multinivel);
    }
    
    //buscar en block
    pthread_mutex_lock(&block_mutex);
    for(int i = 0; i<list_size(block_lista);i++){
        t_pcb* pcbBlock = list_get(block_lista, i);
        if(pcbBlock!=NULL && pcbBlock->pid == pid){
            pthread_mutex_unlock(&block_mutex);
            return pcbBlock;
        }
    }
    pthread_mutex_unlock(&block_mutex);
    //HACER CASOS DE BUSCAR ENTRE LOS SUSPENDIDOS
    return NULL;
}