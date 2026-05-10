#include <scheduler.h>

int main(int argc, char* argv[]){ //argv[1]: Path al config, argv[2]: path al proceso inicial. [ejs: ./bin/kernel_scheduler kernel.config ./procesos/init.prog]

    inicializar(argv[1]); 

//LEVANTAR CONEXION CON MEMORY
    socketConexionMemory = iniciar_conexion(IPMemory, puertoMemory);
    if(socketConexionMemory == EXIT_FAILURE){
        log_error(loggerScheduler, "no se pudo conectar a Kernel Memory");
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


//GESTION DE PROCESOS
    pthread_t hiloPlanificador;
    pthread_create(&hiloPlanificador, NULL, planificador, NULL);
    pthread_detach(hiloPlanificador);

//GESTION DE IOs
    pthread_t hilo_sleep, hilo_stdin, hilo_stdout;
    pthread_create(&hilo_sleep, NULL, hilo_io_sleep, NULL);
    pthread_detach(hilo_sleep);
    pthread_create(&hilo_stdin, NULL, hilo_io_stdin, NULL);
    pthread_detach(hilo_stdin);
    pthread_create(&hilo_stdout, NULL, hilo_io_stdout, NULL);
    pthread_detach(hilo_stdout);

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
    liberar_mutex_y_semaforos();

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
            case MOTIVO_FIN_QUANTUM:
            //ME PARECE QUE LO QUE ME TIENE QUE MANDAR VIOTTI ES EL PID Y YO AHI BUSCO LA CPU EN LA QUE ESTA EJECUTANDO. REVISAR
                
                uint32_t pidInterrumpido;
                buffer_read(paquete->buffer, &pidInterrumpido, sizeof(uint32_t));

                pthread_mutex_lock(&exec_mutex);

                t_cpu_exec* cpuFinQuantum = encontrar_cpu_con_pid(pidInterrumpido);

               

                log_info(loggerScheduler, "## (%d) - Desalojado por fin de quantum", cpuFinQuantum->pcb->pid);

                
                t_pcb* pcb = cpuFinQuantum->pcb;
                cpuFinQuantum->pcb = NULL;
                pthread_mutex_unlock(&exec_mutex);

                pthread_mutex_lock(&ready_mutex);
                queue_push(ready_cola, pcb);
                pthread_mutex_unlock(&ready_mutex);

                sem_post(&sem_hay_cpu_libre);
                sem_post(&sem_hay_proceso_ready);

                break;
            case INIT_PROC:
                t_init_proc* proc = deserializar_init_proc(paquete->buffer);

                //CREO QIUE TENGO QUE HACER EL LOG DE SOLICITO SYSCALL

                uint32_t nuevoPid = generar_pid();

                crear_proceso(nuevoPid, proc->pathArchivoInstrucciones, proc->prioridad);

                free(proc->pathArchivoInstrucciones);
                free(proc);
                break;
            case EXIT: //aca solo recibir el pid, me parece innecesario serializar un bufffer solo para esto
            //despues ver los casos en los que se pasa de READY-> EXIT Y BLOCK->EXIT 
                uint32_t pid;
                buffer_read(paquete->buffer, &pid, sizeof(uint32_t));

                //CREO QIUE TENGO QUE HACER EL LOG DE SOLICITO SYSCALL

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
                t_cpu_exec* cpuSleep = encontrar_cpu_con_pid(sleep->pid); 
                pthread_mutex_unlock(&exec_mutex);
                
                bloquear_proceso(cpuSleep->pcb);

                pthread_mutex_lock(&mutex_cola_sleep);
                queue_push(cola_sleep, sleep);
                pthread_mutex_unlock(&mutex_cola_sleep);

                sem_post(&sem_hay_proc_esperando_sleep);
                sem_post(&sem_hay_cpu_libre);
              
                break;
            case STDIN:
            
                t_stdin_stdout* procesoStdin = deserializar_stdin(paquete->buffer);//cuando viotti me lo mande, cadenaLeida=NULL

                pthread_mutex_lock(&exec_mutex);
                t_cpu_exec* cpuUsada = encontrar_cpu_con_pid(procesoStdin->pid); 
                pthread_mutex_unlock(&exec_mutex);  

                log_info(loggerScheduler, "## (%d) - Solicitó syscall: STDIN", procesoStdin->pid);
                   
                bloquear_proceso(cpuUsada->pcb);           
             
                pthread_mutex_lock(&mutex_cola_stdin);
                queue_push(cola_stdin, procesoStdin);
                pthread_mutex_unlock(&mutex_cola_stdin);

                sem_post(&sem_hay_proc_esperando_stdin);

                sem_post(&sem_hay_cpu_libre);
                break;
            case STDOUT:
                t_stdin_stdout* procesoStdout = deserializar_stdin(paquete->buffer); //cuando viotti me lo mande, cadenaLeida=NULL

                pthread_mutex_lock(&exec_mutex);
                t_cpu_exec* cpuUsadaStdout = encontrar_cpu_con_pid(procesoStdout->pid); 

                log_info(loggerScheduler, "## (%d) - Solicitó syscall: STDOUT", procesoStdout->pid);

                pthread_mutex_unlock(&exec_mutex); 

                bloquear_proceso(cpuUsadaStdout->pcb);

                procesoStdout->cadenaLeida = solicitar_cadena_a_memory(procesoStdout->direccionLogica, procesoStdout->bytesALeer);

                pthread_mutex_lock(&mutex_cola_stdout);
                queue_push(cola_stdout, procesoStdout);
                pthread_mutex_unlock(&mutex_cola_stdout);

                sem_post(&sem_hay_proc_esperando_stdout);

                sem_post(&sem_hay_cpu_libre);
                
                break;
            case FINALIZAR_SLEEP: 
                
                uint32_t pidTerminado;

                buffer_read(paquete->buffer, &pidTerminado, sizeof(uint32_t));

                t_pcb* pcbADesbloquear = buscar_y_sacar_de_block(pidTerminado);
                
                pthread_mutex_lock(&ready_mutex);
                queue_push(ready_cola, pcbADesbloquear);
                pthread_mutex_unlock(&ready_mutex);

                log_info(loggerScheduler, "## (%d) finalizó IO y pasa a READY", pidTerminado);
                log_info(loggerScheduler, "## (%d) Pasa del estado BLOCK al estado READY", pidTerminado);
                sem_post(&sem_sleep_disponible);
                sem_post(&sem_hay_proceso_ready);

                break;
            case FINALIZAR_STDOUT: 
                
                uint32_t pidFinalizado;

                buffer_read(paquete->buffer, &pidFinalizado, sizeof(uint32_t));

                t_pcb* pcbParaDesbloquear = buscar_y_sacar_de_block(pidFinalizado);
                
                pthread_mutex_lock(&ready_mutex);
                queue_push(ready_cola, pcbParaDesbloquear);
                pthread_mutex_unlock(&ready_mutex);

                log_info(loggerScheduler, "## (%d) finalizó IO y pasa a READY", pidFinalizado);
                log_info(loggerScheduler, "## (%d) Pasa del estado BLOCK al estado READY", pidFinalizado);
                sem_post(&sem_stdout_disponible);
                sem_post(&sem_hay_proceso_ready);

                break;
            case FINALIZAR_STDIN:
                //recibir pid, bytes, cadenaLeida y  del IO
                
                t_stdin_stdout* resultado = deserializar_stdin(paquete->buffer);

                //pedirle al KM que escriba en memoria
                pthread_mutex_lock(&mutex_socket_memory);
                t_paquete* paqueteKM = crear_paquete(ESCRIBIR_BYTES, paquete->buffer);
                enviar_paquete(socketConexionMemory, paqueteKM);
                free(paqueteKM); // solo el paquete, no el buffer porque es del paquete original

                //esperar OK del KM
                int ok = recibir_ok_memory();
                pthread_mutex_unlock(&mutex_socket_memory);

                if(!ok){
                    //loguear error al escrribir en memoria
                    log_error(loggerScheduler, "El proceso (%d) no pudo escribir en memoria", resultado->pid);
                    free(resultado->cadenaLeida);
                    free(resultado);
                    break;
                }

                //mover proceso BLOCK -> READY
                t_pcb* pcbDesblockeado = buscar_y_sacar_de_block(resultado->pid);
                
                pthread_mutex_lock(&ready_mutex);
                queue_push(ready_cola, pcbDesblockeado);
                pthread_mutex_unlock(&ready_mutex);

                log_info(loggerScheduler, "## (%d) finalizó IO y pasa a READY", resultado->pid);
                log_info(loggerScheduler, "## (%d) Pasa del estado BLOCK al estado READY", resultado->pid);
                sem_post(&sem_stdin_disponible);
                sem_post(&sem_hay_proceso_ready);

                free(resultado->cadenaLeida);
                free(resultado);

                //ver si hay que eliminar paquete aca
                break;
            case MUTEX_CREATE:
                t_mutex_syscall* mutexNuevo = deserializar_mutex(paquete->buffer);

                log_info(loggerScheduler, "## (%d) - Solicitó syscall: MUTEX_CREATE", mutexNuevo->pid);

                pthread_mutex_lock(&mutex_lista_mutex);

                t_mutex_syscall* otroMutex = buscar_mutex(mutexNuevo->nombreMutex);

                if(otroMutex == NULL){
                    t_mutex_syscall* mutexParaLista = malloc(sizeof(t_mutex_syscall));
                    mutexParaLista->colaEspera=queue_create();
                    mutexParaLista->nombreMutex=strdup(mutexNuevo->nombreMutex);
                    mutexParaLista->pid = UINT32_MAX;//significa LIBRE, xq no podemos usar -1 y tampoco podemos usar NULL. Desp ver si esta bien
                    list_add(lista_mutex, mutexParaLista);
                    pthread_mutex_unlock(&mutex_lista_mutex);
                    
                }else{
                    //ya esxiste =>
                    pthread_mutex_unlock(&mutex_lista_mutex);
                }
                
                uint32_t existe=1;
                send(socketCliente, &existe, sizeof(uint32_t), 0);//decirle a viotti y ver si es necesario
                free(mutexNuevo->nombreMutex);
                free(mutexNuevo);
                break;
            /*
            case MUTEX_LOCK:
                t_mutex_syscall* mutexABloquear = deserializar_mutex(paquete->buffer);

                log_info(loggerScheduler, "## (%d) - Solicitó syscall: MUTEX_LOCK", mutexABloquear->pid);

                pthread_mutex_lock(&mutex_lista_mutex);
                t_mutex_syscall* mutexABloquearEnLista = buscar_mutex(mutexABloquear->nombreMutex);

                if(mutexABloquearEnLista->pid != UINT32_MAX){
                    //esta tomado
                    t_cpu_exec* cpuALiberar = encontrar_cpu_con_pid(mutexABloquear->pid);

                    queue_push(mutexABloquearEnLista->colaEspera, cpuALiberar->pcb);

                    bloquear_proceso(cpuALiberar->pcb);

                    pthread_mutex_unlock(&mutex_lista_mutex);

                    sem_post(&sem_hay_cpu_libre);
                }
                else{
                    //no esta tomado
                    mutexABloquearEnLista->pid = mutexABloquear->pid;
                    pthread_mutex_unlock(&mutex_lista_mutex);

                    uint32_t ok = 1;
                    send(socketCliente, &ok, sizeof(uint32_t), 0);//mediante un 0 se avisa que ya fue tomado el recurso
                }

                free(mutexABloquear->nombreMutex);
                free(mutexABloquear);
                break;
            case MUTEX_UNLOCK:
                t_mutex_syscall* mutexADesbloquear = deserializar_mutex(paquete->buffer);

                pthread_mutex_lock(&lista_mutex);
                t_mutex_syscall* mutexADesbloquearEnLista = buscar_mutex(mutexADesbloquear->nombreMutex);

                if(mutexADesbloquearEnLista->colaEspera != NULL){

                }

                pthread_mutex_unlock(&lista_mutex);

                break;*/

            //agregar caso que no coincida con nada
            
        }

        free(paquete->buffer->stream);
        free(paquete->buffer);
        free(paquete);
        }

    close(socketCliente);

    return NULL;
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
        log_info(logger, "## CPU %s CONECTADA", idCPU);

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



