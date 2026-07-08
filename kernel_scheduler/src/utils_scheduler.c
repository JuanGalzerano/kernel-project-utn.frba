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

t_cpu* obtener_cpu_libre(){
    pthread_mutex_lock(&exec_mutex);
    t_cpu* cpu = NULL;

    for(int i = 0; i<list_size(exec_lista); i++){
        t_cpu* cpuTemporal = list_get(exec_lista, i);
        if (cpuTemporal->pcb == NULL){
            cpu= cpuTemporal;
            break;
        }
    }
    pthread_mutex_unlock(&exec_mutex);
    return cpu;
}


void enviar_proceso_a_cpu(t_cpu* cpu,t_pcb* pcb){//el socket esta en la cpu
    pthread_mutex_lock(&exec_mutex);
    cpu->pcb = pcb;
    cpu->control_enviado = false; 

    t_buffer* buffer = buffer_create(0);
    buffer_add_uint32(buffer, cpu->pcb->pid);
    t_paquete* paquete = crear_paquete(EJECUTAR_PROCESO, buffer);
    enviar_paquete(cpu->socketConexion, paquete);
    //creo q hay destruir el paquete y buffer
    pthread_mutex_unlock(&exec_mutex);

    //hacer log de que se pasa a running
    log_info(loggerScheduler, "## (%d) Pasa del estado READY al estado EXEC", pcb->pid);

    free(buffer->stream);
    free(buffer);
    free(paquete);
}

//me aparece q ya no la uso mas a esta
bool intentar_enviar_control_cpu(t_cpu* cpu, t_paquete* paquete){
    pthread_mutex_lock(&exec_mutex);
    bool enviado = intentar_enviar_control_cpu_bajo_lock(cpu, paquete);
    pthread_mutex_unlock(&exec_mutex);
    return enviado;
}

//version sin tomar el mtx, me deja que si ya se mando otra interrup =>no se mande esta tmb
bool intentar_enviar_control_cpu_bajo_lock(t_cpu* cpu, t_paquete* paquete){
    if(cpu->control_enviado){
        return false;
    }
    cpu->control_enviado = true;
    enviar_paquete(cpu->socketConexion, paquete);
    return true;
}

//si manda->true, si no manda, es pq se mando una interrupcion y se esta esperando el pid y motivo para finalizar el desalojo->false
bool reanudar_proceso_en_cpu(t_cpu* cpu){
    pthread_mutex_lock(&exec_mutex);
    if(cpu->control_enviado){
        pthread_mutex_unlock(&exec_mutex);
        return false;
    }
    t_buffer* buffer = buffer_create(0);
    buffer_add_uint32(buffer, cpu->pcb->pid);
    t_paquete* paquete = crear_paquete(EJECUTAR_PROCESO, buffer);
    enviar_paquete(cpu->socketConexion, paquete);
    pthread_mutex_unlock(&exec_mutex);

    free(buffer->stream);
    free(buffer);
    free(paquete);
    return true;
}

uint32_t generar_pid(){
    pthread_mutex_lock(&mutex_pid);
    uint32_t pid = proximo_pid++;
    pthread_mutex_unlock(&mutex_pid);
    return pid;
}

t_cpu* encontrar_cpu_con_pid(uint32_t pid){//entiendo que si se llama a esta funcion, es xq la cpu esta ejecutando, por lo que no contemplo el caso en las lamadas a la funcion que se pueda retornar NULL
    t_cpu* cpu = NULL;
        for(int i = 0; i < list_size(exec_lista); i++){
            t_cpu* cpuEncontrada = list_get(exec_lista, i);
            if(cpuEncontrada->pcb != NULL && cpuEncontrada->pcb->pid == pid){
                cpu = cpuEncontrada;
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
    //tampoco se contempla que una CPU solicite un tipo de IO que no esta corriendo, ypq vi en un issue que se se va a necesitar, va a estar corriendo

}


void bloquear_proceso(t_pcb* pcbBlock){
    pthread_mutex_lock(&exec_mutex);
    t_cpu* cpu = encontrar_cpu_con_pid(pcbBlock->pid);
    if(cpu==NULL){
<<<<<<< HEAD
        pthread_mutex_unlock(&exec_mutex); 
=======
        pthread_mutex_unlock(&exec_mutex);
>>>>>>> 2f22caa0cb2882b4fd540dc7ae34f54864dae51a
        return;
    }
    cpu->pcb = NULL;

    t_buffer* buffer = buffer_create(0);
    buffer_add_uint32(buffer, pcbBlock->pid);
    t_paquete* unPaquete = crear_paquete(PROCESO_BLOQUEADO, buffer);
    intentar_enviar_control_cpu_bajo_lock(cpu, unPaquete);
    pthread_mutex_unlock(&exec_mutex);

    pthread_mutex_lock(&block_mutex);
    list_add(block_lista, pcbBlock); //cuando implemente plani a mediado plazo, aca voy a tener que correr el hilo para ver si va a susp block
    pthread_mutex_unlock(&block_mutex);

    timer_tiempo_bloqueado(pcbBlock);

    log_info(loggerScheduler, "## (%d) Pasa del estado EXEC al estado BLOCK", pcbBlock->pid);

    free(buffer->stream);
    free(buffer);
    free(unPaquete);
}

t_pcb* buscar_y_sacar_de_block(uint32_t pid){
    pthread_mutex_lock(&block_mutex);
    
    t_pcb* pcb = NULL;
    for(int i = 0; i < list_size(block_lista); i++){
        t_pcb* pcbDeLista = list_get(block_lista, i);
        if(pcbDeLista->pid == pid){
            pcb = pcbDeLista;
            list_remove(block_lista, i);
            break;
        }
    }
    pthread_mutex_unlock(&block_mutex);
    return pcb;
}

void enviar_fin_proceso_memory(uint32_t pid){ //esta la voy a usar tmb para el de READY->EXIT Y BLOCK -> EXIT creeeo
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


void enviar_fin_proceso_a_cpu(uint32_t pid, t_cpu* cpu){
    t_buffer* buffer = buffer_create(0);
    buffer_add_uint32(buffer, pid);
    t_paquete* paquete = crear_paquete(FIN_PROCESO, buffer);
    intentar_enviar_control_cpu(cpu, paquete);
    free(buffer->stream);
    free(buffer);
    free(paquete);
}

void* hilo_timer_quantum(void* arg){
    t_parametros_hilo_quantum* argumento = (t_parametros_hilo_quantum*) arg; //recibo pid original y la cpu

    uint32_t pidCpuOriginal = argumento->pid_original;
    uint32_t miToken= argumento->token;
    t_cpu* cpu = argumento->cpu;
    free(argumento);

    usleep(quantum*1000);//usleep recibe microsegundos de parametro VER SI ESTA BIEN USAR ESTA FUNCION

    pthread_mutex_lock(&exec_mutex);
    bool sigueEjecutando = ((cpu->pcb != NULL) && (cpu->pcb->pid == pidCpuOriginal) && (cpu->quantum_token == miToken));

    if(sigueEjecutando){
        //pedir a CPU que finalice por quantum (mandado con el lock tomado para no pisarse con un desalojo/desconexion concurrente)
        t_buffer* buffer = buffer_create(0);
        buffer_add_uint32(buffer, pidCpuOriginal);
        t_paquete* paquete = crear_paquete(FINALIZAR_POR_QUANTUM, buffer);
        intentar_enviar_control_cpu_bajo_lock(cpu, paquete);
        eliminar_paquete(paquete);
    } else {
        log_info(loggerScheduler,"Timer de quantum obsoleto ignorado para pid %d (token %u != %u)", pidCpuOriginal, miToken, cpu->quantum_token);
    }
    pthread_mutex_unlock(&exec_mutex);

    return NULL;
}


void iniciar_timer_quantum(t_cpu* cpu){
    t_parametros_hilo_quantum* argus = malloc(sizeof(t_parametros_hilo_quantum));
    argus->cpu = cpu;

    pthread_mutex_lock(&exec_mutex);
    argus->pid_original = cpu->pcb->pid;
    argus->token = ++cpu->quantum_token;
    pthread_mutex_unlock(&exec_mutex);

    pthread_t hiloQuantum;
    pthread_create(&hiloQuantum, NULL, hilo_timer_quantum, argus);
    pthread_detach(hiloQuantum);
}

void enviar_path_proceso_memory(uint32_t pid, char* path){//decirle a juani que esta va a haber que hacerla con paquete y eso xq sino solo funca para el proceso 0
    uint32_t tamanioPath = strlen(path)+1;
    t_buffer* buffer = buffer_create(0);
    buffer_add_uint32(buffer, pid);
    buffer_add_uint32(buffer, tamanioPath);
    buffer_add_string(buffer, tamanioPath, path);
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
    char* cadena =NULL;// ver sy es bytes +1 y eso

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

void liberar_mutex_y_semaforos(){//si no me equivoxo, nunca se deberia llamar a esto

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
    sem_destroy(&sem_desalojo_compactacion_completo);
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


op_code solicitar_segmento_memory(t_mem_alloc* infoMemAlloc, op_code instanciaDeSolicitud,int socket){//instancia de solicitud seria SOLICITAR_SEGMENTO o RESOLICITAR_SEGMENTO
    t_buffer* buffer = serializar_mem_alloc(infoMemAlloc);//VER SI PPASANDO EL SOCKET FUNCIONA Y SINO VOY A TENER QUE PASAR EL ID DE LA CPU AL HILO GRAL
    t_paquete* paquete = crear_paquete(instanciaDeSolicitud, buffer);
    pthread_mutex_lock(&mutex_socket_memory);
    enviar_paquete(socketConexionMemory, paquete);
    eliminar_paquete(paquete);

    t_paquete* respuesta = recibir_paquete(socketConexionMemory);
    op_code codigo = respuesta->codigo_operacion;
    eliminar_paquete(respuesta);

    if(codigo == COMPACTACION){
        //aca tendria que hacer la func compactacion() que desaloje todos los procesos y no permita enviarle PROCESOS_DESALOJADOS hasta que no se desaloje todo
        compactando = true;
        log_info(loggerScheduler,"## Inicio de compactación");

        despostear_todas_cpus();
        int cpusLiberadas = desalojar_por_compactacion(socket);
        t_paquete* pacComp = crear_paquete(PROCESOS_DESALOJADOS, NULL);
        enviar_paquete(socketConexionMemory, pacComp);
        eliminar_paquete(pacComp);
        t_paquete* compactacionHecha = recibir_paquete(socketConexionMemory);//creo que no tengo que hacer nada con este paquete
        if(compactacionHecha->codigo_operacion!=COMPACTACION_EXITOSA){
            log_error(loggerScheduler,"Compactacion NO exitosa");
            pthread_mutex_unlock(&mutex_socket_memory);
<<<<<<< HEAD
            return -1;
=======
            return MEMORIA_NO_DISPONIBLE;
>>>>>>> 2f22caa0cb2882b4fd540dc7ae34f54864dae51a
        }
        compactando= false;
        for(int i = 0;i<cpusLiberadas;i++){
            liberar_cpu_y_notificar();//ACA VER SI HAY QUE PONER ESTE O SOLO EL SEMPOST
        }
        log_info(loggerScheduler,"## fin de compactación");
        
        despertar_planificador();

/*ACA TENDRIA QUE NOTIFICAR PARA QUE VUELVAN A EJECUTAR TODAS LA CPUS Y LOS PROCESOS*/

        eliminar_paquete(compactacionHecha);
        pthread_mutex_unlock(&mutex_socket_memory);

        //Post-compactacion debe poder y sino es bsod REVISAR SI ES ASI//NO SE DEBERIA LLEGAR NUNCA AL BSOD
        op_code reintentar = solicitar_segmento_memory(infoMemAlloc,RESOLICITAR_SEGMENTO, socket);
        if(reintentar != MEMORIA_DISPONIBLE) manejar_bsod();//realmente imposible pero bueno en casos especiales, pantallazo azul.xq sino es un bucle de solicitar_segmento_memory
        return reintentar;
    }

    pthread_mutex_unlock(&mutex_socket_memory);
    return codigo;
}


//heste es un hilo con socket aparte pq sino el hilo que escuchaba cuando me avisaba juani que habia memoria nueva se robaba paquetes que no le correspondian
void* hilo_escuchar_memory(void* arg){
    //(void)arg; CREO Q INNECESARIO
    while(1){
        t_paquete* paquete = recibir_paquete(socketMemoryNotif);
        if(paquete == NULL){
            log_error(loggerScheduler, "conexion perdida con el socket de escuha a memory");
            break;
        }

        switch(paquete->codigo_operacion){
            case MEMORIA_CORRUPTA:
                eliminar_paquete(paquete);
                manejar_bsod();
                return NULL;
            case NUEVA_MEMORIA_DISPONIBLE: {
                uint32_t cantPidsDesuspendibles = buffer_read_uint32(paquete->buffer);

                if(cantPidsDesuspendibles==0) break;

                t_list* pidsDesuspendibles = list_create();
                for(int i =0; i<cantPidsDesuspendibles;i++){
                    uint32_t* pidAux = malloc(sizeof(uint32_t));
                    *pidAux = buffer_read_uint32(paquete->buffer);
                    list_add(pidsDesuspendibles,pidAux);
                    
                }
                recorrer_y_desuspender(pidsDesuspendibles);
                list_destroy_and_destroy_elements(pidsDesuspendibles, destruir_uint32_t);//es una lista de un solo uso
                eliminar_paquete(paquete);
                break;
            }
            
            default:
                log_warning(loggerScheduler, "Opcode desconocido en hilo de exucha a memory: %d", paquete->codigo_operacion);
                eliminar_paquete(paquete);
                break;
        }
    }
    return NULL;
}


void manejar_bsod(){

    log_info(loggerScheduler, "## BSOD: Corrupcion de memoria detectada");
    pthread_mutex_lock(&exec_mutex);
    for(int i = 0; i < list_size(exec_lista); i++){
        t_cpu* cpu = list_get(exec_lista, i);
        if(cpu->pcb != NULL){
            //ACA ME FALTA AGREGAR AVIDARLE A LA CPU
            t_buffer* buf = buffer_create(0);
            buffer_add_uint32(buf, cpu->pcb->pid);
            t_paquete* paq = crear_paquete(DESALOJAR_POR_BSOD,buf);
            intentar_enviar_control_cpu_bajo_lock(cpu, paq);
            eliminar_paquete(paq);
            log_info(loggerScheduler, "## (%d) Pasa del estado EXEC al estado EXIT", cpu->pcb->pid);
            log_info(loggerScheduler, "## (%d) finalizó su ejecución con motivo de Blue Screen of Death (BSOD)", cpu->pcb->pid);
            free(cpu->pcb);
            cpu->pcb = NULL;
            free(cpu);
            list_remove(exec_lista,i);
            i--;
        }
    }
    pthread_mutex_unlock(&exec_mutex);

    if(algoritmo != CMN){
        pthread_mutex_lock(&ready_mutex);
        while(!queue_is_empty(ready_cola)){
            t_pcb* pcb = queue_pop(ready_cola);
            log_info(loggerScheduler, "## (%d) Pasa del estado READY al estado EXIT", pcb->pid);
            log_info(loggerScheduler, "## (%d) finalizó su ejecución con motivo de Blue Screen of Death (BSOD)", pcb->pid);
            free(pcb);
        }
        pthread_mutex_unlock(&ready_mutex);
    } else {
        pthread_mutex_lock(&mutex_cola_multinivel);
        for(int i = 0; i < cantidad_colas; i++){
            while(!queue_is_empty(cola_multinivel[i])){
                t_pcb* pcb = queue_pop(cola_multinivel[i]);
                log_info(loggerScheduler, "## (%d) Pasa del estado READY al estado EXIT", pcb->pid);
                log_info(loggerScheduler, "## (%d) finalizó su ejecución con motivo de Blue Screen of Death (BSOD)", pcb->pid);
                free(pcb);
            }
        }
        pthread_mutex_unlock(&mutex_cola_multinivel);
    }
    
    pthread_mutex_lock(&block_mutex);
    while(!list_is_empty(block_lista)){
        t_pcb* pcb = list_remove(block_lista, 0);
        log_info(loggerScheduler, "## (%d) Pasa del estado BLOCK al estado EXIT", pcb->pid);
        log_info(loggerScheduler, "## (%d) finalizó su ejecución con motivo de Blue Screen of Death (BSOD)", pcb->pid);
        free(pcb);
    }
    pthread_mutex_unlock(&block_mutex);



    pthread_mutex_lock(&mutex_susp_block);
    while(!list_is_empty(susp_block)){
        t_proc_suspendido* proc = list_remove(susp_block, 0);
        log_info(loggerScheduler, "## (%d) Pasa del estado SUSP BLOCK al estado EXIT", proc->pcb->pid);
        log_info(loggerScheduler, "## (%d) finalizó su ejecución con motivo de Blue Screen of Death (BSOD)", proc->pcb->pid);
        free(proc->cadenaStdin);
        free(proc->pcb);
        free(proc);
    }
    pthread_mutex_unlock(&mutex_susp_block);


    pthread_mutex_lock(&mutex_susp_ready);
    while(!list_is_empty(susp_ready)){
        t_proc_suspendido* proceso = list_remove(susp_ready, 0);
        log_info(loggerScheduler, "## (%d) Pasa del estado SUSP READY al estado EXIT", proceso->pcb->pid);
        log_info(loggerScheduler, "## (%d) finalizó su ejecución con motivo de Blue Screen of Death (BSOD)", proceso->pcb->pid);
        free(proceso->cadenaStdin);
        free(proceso->pcb);
        free(proceso);
    }
    pthread_mutex_unlock(&mutex_susp_ready);

    pthread_mutex_lock(&mutex_lista_mutex);

    for(int i=0;i<list_size(lista_mutex);i++){
        t_mutex_syscall* mutex = list_remove(lista_mutex,0);
        free(mutex->nombreMutex);
        queue_destroy(mutex->colaEspera);
        free(mutex);
    }

    pthread_mutex_unlock(&mutex_lista_mutex);

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
    for(int i = 0; i < cantidad_colas; i++){
        if(!queue_is_empty(cola_multinivel[i])){
            pcb = queue_peek(cola_multinivel[i]);
            break;
        }
    }
    pthread_mutex_unlock(&mutex_cola_multinivel);
    return pcb;
}

t_cpu* hay_cpu_desalojable(t_pcb* pcbCandidato){
    if(pcbCandidato == NULL) return NULL;
    pthread_mutex_lock(&exec_mutex);
    t_cpu* cpuADesalojar = NULL;
    for(int i = 0; i < list_size(exec_lista); i++){
        t_cpu* cpu = list_get(exec_lista, i);
        if(cpu->pcb != NULL && cpu->pcb->prioridad > pcbCandidato->prioridad){
            cpuADesalojar = cpu;
            break;
        }
    }
    pthread_mutex_unlock(&exec_mutex);
    return cpuADesalojar;
}

bool enviar_desalojo_cpu(t_cpu* cpuDesalojable){
    if(cpuDesalojable->pcb == NULL){
        return false;
    }
    t_buffer* buffer = buffer_create(0);
    buffer_add_uint32(buffer, cpuDesalojable->pcb->pid);
    t_paquete* paquete = crear_paquete(DESALOJO, buffer);
    bool enviado = intentar_enviar_control_cpu_bajo_lock(cpuDesalojable, paquete);

    free(buffer->stream);
    free(buffer);
    free(paquete);
    return enviado;
}

void liberar_cpu_y_notificar(){
    sem_post(&sem_hay_cpu_libre);
    pthread_mutex_lock(&mutex_planificador);
    pthread_cond_signal(&cond_planificador);
    pthread_mutex_unlock(&mutex_planificador);
}

//ver bien que onda esta xq si esta en ready en CMN, lo saco de la lista pero en todos los casos no, no creo que eso sea correcto
//USARLA SOLO EN LO DE MUTEX
t_pcb* buscar_pcb_por_pid(uint32_t pid, uint32_t prioridadCandidata, int* estabaEnReady){
    *estabaEnReady = 0;
    pthread_mutex_lock(&exec_mutex);
    for(int i = 0; i < list_size(exec_lista); i++){
        t_cpu* cpu = list_get(exec_lista, i);
        if(cpu->pcb != NULL && cpu->pcb->pid == pid){
            t_pcb* pcb = cpu->pcb;
            pthread_mutex_unlock(&exec_mutex);
            return pcb;
        }
    }
    pthread_mutex_unlock(&exec_mutex);

    //HACER CASO SI HAY CMN


    if(algoritmo != CMN){
        pthread_mutex_lock(&ready_mutex);
        for(int i = 0; i < queue_size(ready_cola); i++){
            t_pcb* pcb = list_get(ready_cola->elements, i);
            if(pcb->pid == pid){
                pthread_mutex_unlock(&ready_mutex);
                return pcb;
            }
        }
        pthread_mutex_unlock(&ready_mutex);
    } else {
        pthread_mutex_lock(&mutex_cola_multinivel);
        for(int i = 0; i < cantidad_colas; i++){
            for(int j = 0; j < queue_size(cola_multinivel[i]); j++){
                t_pcb* pcb = list_get(cola_multinivel[i]->elements, j);
                if(pcb->pid == pid){
                    if(prioridadCandidata < pcb->prioridad){
                        list_remove(cola_multinivel[i]->elements, j);//aca lo saco pq va a heredar
                        *estabaEnReady=1;
                    }
                    pthread_mutex_unlock(&mutex_cola_multinivel);
                    return pcb;
                }
            }
        }
        pthread_mutex_unlock(&mutex_cola_multinivel);
    }
    
    pthread_mutex_lock(&block_mutex);
    for(int i = 0; i<list_size(block_lista);i++){
        t_pcb* pcbBlock = list_get(block_lista, i);
        if(pcbBlock!=NULL && pcbBlock->pid == pid){
            pthread_mutex_unlock(&block_mutex);
            return pcbBlock;
        }
    }
    pthread_mutex_unlock(&block_mutex);
    
    pthread_mutex_lock(&mutex_susp_block);
    for(int i =0;i<list_size(susp_block);i++){
        t_proc_suspendido* proc = list_get(susp_block,i);
        if(proc!=NULL && proc->pcb->pid==pid){
            pthread_mutex_unlock(&mutex_susp_block);
            return proc->pcb;
        }
    }
    pthread_mutex_unlock(&mutex_susp_block);

    pthread_mutex_lock(&mutex_susp_ready);
    for(int i =0;i<list_size(susp_ready);i++){
        t_proc_suspendido* proceso = list_get(susp_ready,i);
        if(proceso!=NULL && proceso->pcb->pid==pid){
            pthread_mutex_unlock(&mutex_susp_ready);
            return proceso->pcb;
        }
    }
    pthread_mutex_unlock(&mutex_susp_ready);

    return NULL;
}

void despertar_planificador(){
    if((algoritmo == CMN) && hay_desalojo){
        pthread_mutex_lock(&mutex_planificador);
        pthread_cond_signal(&cond_planificador);
        pthread_mutex_unlock(&mutex_planificador);
    }
}

void timer_tiempo_bloqueado(t_pcb* pcb){
    uint32_t *pid = malloc(sizeof(uint32_t));
    *pid = pcb->pid;
    pthread_t hiloTimer;
    pthread_create(&hiloTimer,NULL, hilo_timer_bloqueado,pid);
    pthread_detach(hiloTimer);
}

void* hilo_timer_bloqueado(void* arg){
    uint32_t* argu = (uint32_t*) arg;
    uint32_t pid = *argu;
    free(argu);

    usleep(suspensionTimeout*1000);

    pthread_mutex_lock(&block_mutex);
    t_pcb* pcb=NULL;
    for(int i=0;i<list_size(block_lista);i++){
        t_pcb* pcbDeLista = list_get(block_lista,i);
        if(pcbDeLista->pid==pid){
            pcb=pcbDeLista;
            list_remove(block_lista,i);
            break;
        }
    }

    if(pcb!=NULL){
        suspender_proceso(pcb);
    }
    pthread_mutex_unlock(&block_mutex);
    return NULL;
}

void suspender_proceso(t_pcb* pcb){
    //(creo q siempre va a ser de block a susp bloc)
    log_info(loggerScheduler, "## (%d) Pasa del estado BLOCK al estado SUSP BLOCK", pcb->pid);

    //avisarle a juai
    t_buffer* buffer = buffer_create(0);
    buffer_add_uint32(buffer, pcb->pid);
    t_paquete* paquete = crear_paquete(SUSPENDER_PROCESO, buffer);
    pthread_mutex_lock(&mutex_socket_memory);
    enviar_paquete(socketConexionMemory, paquete);
    eliminar_paquete(paquete);
    t_proc_suspendido* proc = malloc(sizeof(t_proc_suspendido));//acordarme de liberar cuando saco de susp
    t_paquete* resp = recibir_paquete(socketConexionMemory);
    pthread_mutex_unlock(&mutex_socket_memory);
    proc->bytesSuspendidos = buffer_read_uint32(resp->buffer);
    proc->cadenaStdin=NULL;
    if(resp->codigo_operacion==MEMORIA_NO_DISPONIBLE){
        /*VER QUE CARAJO DEBERIA HACER ACA (nota de juani, para mi que termine el proceso xd)*/
    }
    eliminar_paquete(resp);
    proc->pcb = pcb;
    pthread_mutex_lock(&mutex_susp_block);
    list_add(susp_block, proc);
    pthread_mutex_unlock(&mutex_susp_block);
    
}


void pasar_des_susp_block_a_ready(uint32_t pid, char* cadenaStdin, uint32_t direccionLogica){//pasa de susp blovk a susp ready. no a ready pasa que sino quedaba full tosco el nombre
//no uso buscar_pcb_por_pid xq ya se que esta en susp block, ahorrandome las iteraciones en otras listas
    pthread_mutex_lock(&mutex_susp_block);
    t_proc_suspendido* proceso=NULL;
    for(int i=0;i<list_size(susp_block);i++){
        t_proc_suspendido* procEnLista = list_get(susp_block,i);
        if(procEnLista->pcb->pid == pid){
            proceso = list_remove(susp_block, i);
            break;
        }
    }
    if(proceso == NULL){
        log_error(loggerScheduler,"no se encontro el pid %d para pasar a SUSP READY", pid);
        return;
    }
    proceso->cadenaStdin = cadenaStdin;
    proceso->direccionLogicaStdin = direccionLogica;
    pthread_mutex_unlock(&mutex_susp_block);

    pthread_mutex_lock(&mutex_susp_ready);
    list_add_sorted(susp_ready,proceso,es_mas_prioritario);
    pthread_mutex_unlock(&mutex_susp_ready);

    log_info(loggerScheduler,"## (%d) Pasa del estado SUSP BLOCK al estado SUSP READY", pid);

    //CREO QUE ACA DEBERIA VERIFICAR SI LA MEMORIA QUE TIENE ES 0, DESUSPENDERLO.
    if(proceso->bytesSuspendidos ==0){
        pthread_mutex_lock(&mutex_susp_ready);
        desuspender_proceso(proceso);
        pthread_mutex_unlock(&mutex_susp_ready);
    }else{
        t_paquete* paqSolicitud = crear_paquete(SOLICITAR_PROCS_DESUSPENDIBLES, NULL);
        pthread_mutex_lock(&mutex_socket_memory);
        enviar_paquete(socketConexionMemory, paqSolicitud);
        pthread_mutex_unlock(&mutex_socket_memory);
        eliminar_paquete(paqSolicitud);
    }

    

}

//igual evr si funciona
bool es_mas_prioritario(void* masPrior, void* menosPrior){//los puse igual que como figura en el list.h con void*
    t_proc_suspendido* procMasPrior = (t_proc_suspendido*) masPrior;
    t_proc_suspendido* procMenosPrior = (t_proc_suspendido*) menosPrior;
    return (procMasPrior->pcb->prioridad < procMenosPrior->pcb->prioridad);//haciendolosolo con mayor, me garantizo que cuando se compare con uno que esta hace mas tiempo en add_sorted, no le robe el puesto
}


//POR AHORA NO LA USO, SI SIGUE ASI, SACARLA
t_proc_suspendido* buscar_en_susp_block(uint32_t pid){//ver si en la func de pasar a susp ready combiene usarla
    pthread_mutex_lock(&mutex_susp_block);
    t_proc_suspendido* proceso=NULL;
    for(int i=0;i<list_size(susp_block);i++){
        proceso = list_get(susp_block,i);
        if(proceso->pcb->pid == pid){
            break;
        }
    }
    pthread_mutex_unlock(&mutex_susp_block);
    return proceso;
}

void recorrer_y_desuspender(t_list* pidsDesuspendibles){
    pthread_mutex_lock(&mutex_susp_ready);
    for(int i=0;i<list_size(susp_ready);i++){
        t_proc_suspendido* proc = list_get(susp_ready,i);
        if(perteneceALalista(proc, pidsDesuspendibles)){ 
            desuspender_proceso(proc);//liberar proc pero no pcb
            break;
        }
    }
    pthread_mutex_unlock(&mutex_susp_ready);
}

void desuspender_proceso(t_proc_suspendido* proc){
    list_remove_element(susp_ready,proc);
    t_pcb* pcb = proc->pcb;
    
    t_buffer* buffer = buffer_create(0);
    buffer_add_uint32(buffer, pcb->pid);
    if(proc->cadenaStdin!=NULL){
        uint32_t tamanioCadena = strlen(proc->cadenaStdin);
        buffer_add_uint32(buffer, tamanioCadena);
        buffer_add_uint32(buffer, proc->direccionLogicaStdin);
        buffer_add_string(buffer, tamanioCadena, proc->cadenaStdin);
    }else{
        buffer_add_uint32(buffer, 0);
    }
    t_paquete* paquete = crear_paquete(DESUSPENDER_PROCESO, buffer);
    pthread_mutex_lock(&mutex_socket_memory);
    enviar_paquete(socketConexionMemory, paquete);
    eliminar_paquete(paquete);
    t_paquete* paq= recibir_paquete(socketConexionMemory);
    pthread_mutex_unlock(&mutex_socket_memory);
    if(paq->codigo_operacion == DESUSPEND_OK){
        eliminar_paquete(paq);
        encolar_pcb_ready(pcb);
        sem_post(&sem_hay_proceso_ready);
        despertar_planificador();
        log_info(loggerScheduler,"## (%d) Pasa del estado SUSP READY al estado READY",pcb->pid);
    }else{
        log_error(loggerScheduler, "Error al querer desuspender el pid: %d", pcb->pid);
        eliminar_paquete(paq);
        //ver si tendria que finalizar el proceso, igual nunca se deberia llegar a esta situacion
    }
    free(proc->cadenaStdin);
    free(proc);
}

bool perteneceALalista(t_proc_suspendido* proc, t_list* lista){
    bool pertenece = false;
    for(int i =0;i<list_size(lista);i++){
        uint32_t* pid= list_get(lista,i);
        if(proc->pcb->pid == *pid){
            pertenece=true;
            break;
        }
    }
    return pertenece;
}








int desalojar_por_compactacion(int socket){
    int totalEvictadas= 0;
    int cpusNotificadas=0;

    pthread_mutex_lock(&exec_mutex);
    for(int i=0; i<list_size(exec_lista);i++){
        t_cpu* cpu= list_get(exec_lista,i);
        if(cpu->pcb == NULL) continue;

        if(cpu->socketConexion == socket){
            //Es la q disparo la compac
            t_pcb* pcbPropio = cpu->pcb;
            uint32_t pidPropio = pcbPropio->pid;
            cpu->pcb = NULL;
            cpu->control_enviado = false;

            log_info(loggerScheduler, "## (%d) - Desalojado por compactacion", pidPropio);
            log_info(loggerScheduler, "## (%d) Pasa del estado EXEC al estado READY", pidPropio);

            enlistar_primero_ready(pcbPropio);
            sem_post(&sem_hay_proceso_ready);
            totalEvictadas++;
            continue;
        }

        t_buffer* buffer=buffer_create(0);
        buffer_add_uint32(buffer, cpu->pcb->pid);
        t_paquete* paquete = crear_paquete(COMPACTACION,buffer);
        bool compactacionEnviada = intentar_enviar_control_cpu_bajo_lock(cpu, paquete);
        eliminar_paquete(paquete);

        if(compactacionEnviada){
            cpu->esperando_ack_compactacion = true;
            cpusNotificadas++;
        }
        totalEvictadas++;
    }
    pthread_mutex_unlock(&exec_mutex);

    for(int i = 0; i<cpusNotificadas; i++){
        sem_wait(&sem_desalojo_compactacion_completo);
    }

    return totalEvictadas;
}

void enlistar_primero_ready(t_pcb* pcb){
    if(algoritmo != CMN){
        pthread_mutex_lock(&ready_mutex);
        list_add_in_index(ready_cola->elements,0,pcb);
        pthread_mutex_unlock(&ready_mutex);
    }else{
        pthread_mutex_lock(&mutex_cola_multinivel);
        list_add_in_index(cola_multinivel[pcb->prioridad]->elements,0,pcb);
        pthread_mutex_unlock(&mutex_cola_multinivel);
    }
}

void destruir_uint32_t(void* nro){
    uint32_t* numero = (uint32_t*) nro;
    free(numero);
}

void despostear_todas_cpus(){
    int valor = 0;//inicializamos 
    sem_getvalue(&sem_hay_cpu_libre, &valor);
    while(valor > 0){
        sem_wait(&sem_hay_cpu_libre);
        sem_getvalue(&sem_hay_cpu_libre, &valor);
    }/*
    while(valor < 0){
        sem_post(&sem_hay_cpu_libre);
        sem_getvalue(&sem_hay_cpu_libre, &valor);
    } */  
}

void liberar_de_mutex_por_muerte(t_pcb* pcb){
    uint32_t pid =pcb->pid;
    pthread_mutex_lock(&mutex_lista_mutex);

    for(int i = 0;i<list_size(lista_mutex);i++){
        t_mutex_syscall* mtx = list_get(lista_mutex,i);
        //Este mutex no pertenece al proceso que murió
        if(mtx->pid != pid || mtx->contador >= 1){
            continue;
        }

        //comoo el proc era dueño del mutex
        mtx->contador++;
        log_info(loggerScheduler,"## (%d) Libera el Mutex %s",pid,mtx->nombreMutex);

        if(queue_is_empty(mtx->colaEspera)){
            mtx->pid = UINT32_MAX;
            continue;
        }

        t_pcb* siguiente = queue_pop(mtx->colaEspera);
        mtx->pid = siguiente->pid;

        log_info(loggerScheduler,"## (%d) Toma el Mutex %s",siguiente->pid,mtx->nombreMutex);
        uint32_t prioridadMaxima = siguiente->prioridad;
        for(int j = 0; j < queue_size(mtx->colaEspera); j++){
            t_pcb* pcbTemporal = list_get(mtx->colaEspera->elements, j);
            if(pcbTemporal->prioridad < prioridadMaxima){
                prioridadMaxima = pcbTemporal->prioridad;
            }
        }

        if(siguiente->prioridad > prioridadMaxima){
            log_info(loggerScheduler,"## %d Cambio de prioridad: %d - %d",siguiente->pid,siguiente->prioridad,prioridadMaxima);
            siguiente->prioridad = prioridadMaxima;
        }

        t_pcb* pcbASacarDeBlock = buscar_y_sacar_de_block(siguiente->pid);

        if(pcbASacarDeBlock != NULL){
            encolar_pcb_ready(siguiente);

            log_info(loggerScheduler,"## (%d) Pasa del estado BLOCK al estado READY",siguiente->pid);
            sem_post(&sem_hay_proceso_ready);
            despertar_planificador();
        }else{
            pasar_des_susp_block_a_ready(siguiente->pid, NULL, 0);
        }
    }

    pthread_mutex_unlock(&mutex_lista_mutex);
}
