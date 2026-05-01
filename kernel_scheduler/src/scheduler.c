#include <scheduler.h>



int main(int argc, char* argv[]){ //argv[1]: Path al config, argv[2]: path al proceso inicial. [ejs: ./bin/kernel_scheduler kernel.config ./procesos/init.prog]

    inicializar(argv[1]); 

//LEVANTAR CONEXION CON MEMORY
    socketConexionMemory = iniciar_conexion(IPMemory, puertoMemory);

    if(socketConexionMemory == EXIT_FAILURE){
        log_info(loggerScheduler, "no se pudo conectar a Kernel Memory");
        abort();
    }

    log_info(loggerScheduler, "Conectado a Kernel Memory");
    
    
    handshake_cliente_id(socketConexionMemory, loggerScheduler, SCHEDULER);
   
//LEVANTAR SERVIDOR
    int socketEscucha = iniciar_servidor(puertoEscucha);
    if(socketEscucha == EXIT_FAILURE){
        log_info(loggerScheduler, "No se pudo iniciar el servidor");
        abort();
    }
    log_info(loggerScheduler, "Servidor iniciado");

    crear_proceso(0, argv[2], 0); //por el momento asignamos random el de prioridad xq recien lo usamos para CMN, en este proceso debera ser siempre la maxima

    pthread_t hiloPlanificador;
    pthread_create(&hiloPlanificador, NULL, planificador, NULL);
    pthread_detach(hiloPlanificador);


    while(1){
        int socketCliente = aceptar_cliente_scheduler(socketEscucha, loggerScheduler);

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
    //LIBERAR RECURSOS DE SEMAFOROS Y MUTEX

    return 0;
}


void *atender_cliente(void *arg){//lo que recibe es el socket cliente (con el que se comunican)
    int socketCliente = *(int*) arg;
    free(arg); 



    while(1){
        //recibir paquete 
        t_paquete* paquete;
        paquete = recibir_paquete(socketCliente);//fijarse si hay que liberar memoria
        if(paquete==NULL){
            break; //cliente se desconecto
        }
    


        switch(paquete->codigo_operacion){//le tengo que decir a viotti que cuando le notifico que corte por fin de quantum, me mande este paquete
            case MOTIVO_FIN_QUANTUM://en el buffer que me manden la CPU 
                t_cpu_exec* cpuFinQuantum = malloc(sizeof(t_cpu_exec));
                buffer_read(paquete->buffer, cpuFinQuantum, sizeof(t_cpu_exec));

                log_info(loggerScheduler, "## (%d) - Desalojado por fin de quantum", cpuFinQuantum->pcb->pid);

                pthread_mutex_lock(&exec_mutex);
                t_pcb* pcb = cpuFinQuantum->pcb;
                cpuFinQuantum->pcb = NULL;
                pthread_mutex_unlock(&exec_mutex);

                pthread_mutex_lock(&ready_mutex);
                queue_push(ready_cola, pcb);
                pthread_mutex_unlock(&ready_mutex);

                sem_post(&sem_hay_cpu_libre);
                sem_post(&sem_hay_proceso_ready);

                free(cpuFinQuantum);
                break;
            case INIT_PROC:
                t_init_proc* proc = deserializar_init_proc(paquete->buffer);

                uint32_t nuevoPid = generar_pid();

                crear_proceso(nuevoPid, proc->pathArchivoInstrucciones, proc->prioridad);

                free(proc->pathArchivoInstrucciones);
                free(proc);
                break;
            case EXIT: //aca solo recibir el pid
            //despues ver los casos en los que se pasa de READY-> EXIT Y BLOCK->EXIT 
                uint32_t pid;
                buffer_read(paquete->buffer, &pid, sizeof(uint32_t));

                // encontrar el pcb en exec_lista y liberarlo
                pthread_mutex_lock(&exec_mutex);
                t_cpu_exec* cpu = encontrar_cpu_con_pid(pid);

                t_pcb* pcbFin = cpu->pcb;
                cpu->pcb = NULL;
                pthread_mutex_unlock(&exec_mutex);

                // notificar al KM que libere los recursos del proceso
                enviar_fin_proceso_memory(pid);

                // 3. loguear y liberar el PCB
                log_info(loggerScheduler, "## (%d) Pasa del estado EXEC al estado EXIT", pid);
                log_info(loggerScheduler, "## (%d) finalizó su ejecución con motivo de EXIT", pid);
                free(pcbFin);

                // 4. la CPU quedó libre
                sem_post(&sem_hay_cpu_libre);
                break;
            case SLEEP://se recibe tiempo a dormir y pid
                
                t_sleep* sleep = deserializar_sleep(paquete->buffer);

                pthread_mutex_lock(&exec_mutex);
                t_cpu_exec* cpuUsada = encontrar_cpu_con_pid(sleep->pid); 

                log_info(loggerScheduler, "## (%d) - Solicitó syscall: SLEEP", sleep->pid);

                t_pcb* pcbBlock = cpuUsada->pcb;
                cpuUsada->pcb = NULL;
                pthread_mutex_unlock(&exec_mutex);

                bloquear_proceso(pcbBlock);

                log_info(loggerScheduler, "## (%d) Pasa del estado EXEC al estado BLOCK", sleep->pid);

                //entiendo que aca se manda a el IO el paquete y que despues cuando termine me avise con un op_code tipo IO_TERMINADO y yo lo mando a ready
                enviar_paquete(socketSleep, paquete);

                sem_post(&sem_hay_cpu_libre);
                free(sleep);
                break;
            case STDIN:
            
                t_stdin* procesoStdin = deserializar_stdin(paquete->buffer);

                pthread_mutex_lock(&exec_mutex);
                t_cpu_exec* cpuUsada = encontrar_cpu_con_pid(procesoStdin->pid); 

                log_info(loggerScheduler, "## (%d) - Solicitó syscall: STDIN", procesoStdin->pid);

                t_pcb* pcbBlock = cpuUsada->pcb;
                cpuUsada->pcb = NULL;
                pthread_mutex_unlock(&exec_mutex);     

                bloquear_proceso(pcbBlock);           

                log_info(loggerScheduler, "## (%d) Pasa del estado EXEC al estado BLOCK", procesoStdin->pid);

             
                enviar_paquete(socketStdin,paquete); //la IO stdin ni bien se conecta, se queda esperando a recibir los bytes a leer

                //ver si hay que eliminar paquete aca                

                sem_post(&sem_hay_cpu_libre);
                free(procesoStdin);

                break;
            case STDOUT:

                break;
            case IO_TERMINADO: //esta es util para STDOUT y SLEEP, pero para STDIN necesito desp pedirle a KM que lo guarde en una dire

                break;
            case FINALIZAR_STDIN:
                //recibir pid, bytes, cadenaLeida y  del IO
                
                t_stdin* resultado = deserializar_stdin(paquete->buffer);

                //pedirle al KM que escriba en memoria
                enviar_paquete(socketConexionMemory, crear_paquete(ESCRIBIR_BYTES, paquete->buffer));

                //esperar OK del KM
                int ok = recibir_ok_memory();

                if(!ok){
                    //loguear error al escrribir en memoria
                    log_error(loggerScheduler, "El proceso (%d) no pudo escribir en memoria", resultado->pid);
                    free(resultado->cadenaLeida);
                    free(resultado);
                    break;
                }

                //mover proceso BLOCK -> READY
                t_pcb* pcb = buscar_y_sacar_de_block(resultado->pid);
                
                pthread_mutex_lock(&ready_mutex);
                queue_push(ready_cola, pcb);
                pthread_mutex_unlock(&ready_mutex);

                log_info(loggerScheduler, "## (%d) finalizó IO y pasa a READY", resultado->pid);
                log_info(loggerScheduler, "## (%d) Pasa del estado BLOCK al estado READY", resultado->pid);
                sem_post(&sem_hay_proceso_ready);

                free(resultado->cadenaLeida);
                free(resultado);


                //ver si hay que eliminar paquete aca
                
                break;

            
        }

        free(paquete->buffer->stream);
        free(paquete->buffer);
        free(paquete);
        }

    close(socketCliente);
}



t_pcb* crear_proceso(uint32_t pid, char* path, int prioridad){
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->pid = pid;
    pcb->prioridad = prioridad;

    //Meterlo en NEW y loguear
    pthread_mutex_lock(&new_mutex);
    list_add(new_lista, pcb); 
    pthread_mutex_unlock(&new_mutex);
    log_info(loggerScheduler, "## (%d) Se crea el proceso - Estado: NEW", pcb->pid);

    //Avisar al KM (mandar pid + path)
    enviar_path_proceso_memory(pcb->pid, path);

    //Esperar respuesta OK del KM
    int ok = recibir_ok_memory();

    //Mover a READY y loguear
    if(ok == 0){
        pthread_mutex_lock(&new_mutex);
        list_remove_element(new_lista, pcb);
        pthread_mutex_unlock(&new_mutex);
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


void enviar_path_proceso_memory(uint32_t pid, char* path){//decirle a juani que esta va a haber que hacerla con paquete y eso xq sino solo funca para el proceso 0
    uint32_t sizePath = strlen(path)+1;
    t_buffer* buffer = buffer_create(0);
    buffer_add_uint32(buffer, pid);
    buffer_add_uint32(buffer, sizePath);
    buffer_add_string(buffer, sizePath, path);
    t_paquete* paquete = crear_paquete(PATH_PROCESO, buffer);
    enviar_paquete(socketConexionMemory, paquete);

/*
    send(socketConexionMemory, &pid, sizeof(uint32_t), 0);
    send(socketConexionMemory, &sizePath, sizeof(uint32_t),0);
    send(socketConexionMemory, path, sizePath,0);*/
}

int recibir_ok_memory(){
    int resultado;
    recv(socketConexionMemory, &resultado,sizeof(int),0);//el OK no me lo mandes por paquete juani, pq si o si es lo siguiente a recibir
    if(!resultado){
        return 0;
    }
    return 1;
}


int aceptar_cliente_scheduler(int socketEscucha, t_log *logger){

    int socketCliente = esperar_cliente(socketEscucha);

    int32_t idCliente=0;
    idCliente = handshake_servidor_id(socketCliente, idCliente);


    switch (idCliente)
    {
    case CPU:
        //se solicita id cpu
        char* idCPU;
        int sizeIdCpu;
        recv(socketCliente, &sizeIdCpu, sizeof(int), MSG_WAITALL);
        idCPU = malloc(sizeIdCpu);
        recv(socketCliente, idCPU, sizeIdCpu,MSG_WAITALL);
        log_info(logger, "CPU %s CONECTADA", idCPU);

        //creo cpu
        t_cpu_exec* nueva_cpu = malloc(sizeof(t_cpu_exec));
        nueva_cpu->cpu_id = atoi(idCPU);//transforma a int
        nueva_cpu->socketConexion = socketCliente;
        nueva_cpu->pcb=NULL;

        //agrego CPU a lista de CPUs
        pthread_mutex_lock(&exec_mutex);
        list_add(exec_lista, nueva_cpu);
        pthread_mutex_unlock(&exec_mutex);
        
        //sem post cpu libre
        sem_post(&sem_hay_cpu_libre);

        free(idCPU);
        break;
    case IO:
        log_info(logger, "IO CONECTADO"); 
        recibir_tipo_IO(socketCliente);
        break;
    case -1:

        log_info(logger, "Error en la conexion con el cliente");
        abort();
        break;
    }

    return socketCliente;



}


void* planificador(void* arg) {
    while (1) {
        //hasta que no tengamos cpu disponible ni proceso, no continuamos
        sem_wait(&sem_hay_proceso_ready); 
        sem_wait(&sem_hay_cpu_libre);     

        //mutex funciona como candado para que solo uno pueda modificar en un cierto momento
        pthread_mutex_lock(&ready_mutex);
        t_pcb* pcb = queue_pop(ready_cola);
        pthread_mutex_unlock(&ready_mutex);

        t_cpu_exec* cpu = obtener_cpu_libre();
        enviar_proceso_a_cpu(cpu, pcb); 



        if (algoritmo == RR) {
            iniciar_timer_quantum(cpu);
        }

        //Implementar mas adelante para CMN
    }


    //cuando un proceso termina de ejcutar, hacer el log que se pasa a exit y volar el PCB de ese proceso (nose si es que va aca)
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
    log_info(loggerScheduler, "## (%d) Pasa del estado READY al estado RUNNING", pcb->pid);
}

void iniciar_timer_quantum(t_cpu_exec* cpu){
    pthread_t hiloQuantum;
    pthread_create(&hiloQuantum, NULL, hilo_timer_quantum, cpu);
    pthread_detach(hiloQuantum);
}

void* hilo_timer_quantum(void* arg){
    t_cpu_exec* cpu = (t_cpu_exec*) arg;
    free(arg);
    uint32_t pidCpuOriginal = cpu->pcb->pid;

    usleep(quantum*1000);//usleep recibe microsegundos de parametro VER SI ESTA BIEN USAR ESTA FUNCION

    pthread_mutex_lock(&exec_mutex);
    bool sigueEjecutando = (cpu->pcb != NULL && cpu->pcb->pid != pidCpuOriginal);
    pthread_mutex_unlock(&exec_mutex);

    if(sigueEjecutando){
        //pedir a CPU que finalice por quantum
        t_buffer* buffer = buffer_create(0);
        buffer_add_uint32(buffer, cpu->pcb->pid);
        t_paquete* paquete = crear_paquete(FINALIZAR_POR_QUANTUM, buffer);
        enviar_paquete(cpu->socketConexion, paquete);
        //creo q hay destruir el paquete y buffer
    }
}
uint32_t generar_pid() {
    pthread_mutex_lock(&mutex_pid);
    uint32_t pid = proximo_pid++;
    pthread_mutex_unlock(&mutex_pid);
    return pid;
}

void enviar_fin_proceso_memory(uint32_t pid){ //esta la voy a usar tmb para el de READY->EXIT Y BLOCK -> EXIT
    t_buffer* buffer = buffer_create(0);
    buffer_add_uint32(buffer, pid);
    t_paquete* paquete = crear_paquete(FIN_PROCESO, buffer); //avisarle a juani que haga este case para  que libere todos los segmentos y estructuras asociadas a ese PID
    enviar_paquete(socketConexionMemory, paquete);
    //creo q hay destruir el paquete y buffer
}

t_cpu_exec* encontrar_cpu_con_pid(uint32_t pid){
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
    }
    if(tipo == TIPO_STDIN){
        socketStdin = socketCliente;
    }
    if(tipo == TIPO_STDOUT){
        socketStdout = socketCliente;
    }

    //no se usa mutex aunque sean globales xq solo se conectara 1 IO x cada tipo
    //tampoco se contempla que una CPU solicite un tipo de IO que no esta corriendo, ya que se dijo que en caso de ser necesaria, esta estara corriendo

}

void bloquear_proceso(t_pcb* pcbBlock){
    pthread_mutex_lock(&block_mutex);
    list_add(block_lista, pcbBlock); //cuando implemente plani a mediado plazo, aca voy a tener que correr el hilo para ver si va a susp block
    pthread_mutex_unlock(&block_mutex);
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