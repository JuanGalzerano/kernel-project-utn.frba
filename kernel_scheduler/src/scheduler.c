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

//P/A ESCUCHAR A SCHEDULER
    pthread_t hilo_memory;
    pthread_create(&hilo_memory, NULL, hilo_escuchar_memory, NULL);
    pthread_detach(hilo_memory);

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
    //cerrar conexiones de IOs
    close(socketSleep);
    close(socketStdin);
    close(socketStdout);

    //ver si aca tendria que cerrar las conexiones con las CPUs que quedarin conectadas

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
            case FINALIZAR_POR_QUANTUM://creo que lo tendria que cambiar a FINALIZAR_POR_QUANTUM
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
                uint32_t nuevoPid = generar_pid();
                crear_proceso(nuevoPid, proc->pathArchivoInstrucciones, proc->prioridad);

                // INIT_PROC no bloquea al proceso padre: lo devolvemos a ejecutar
                pthread_mutex_lock(&exec_mutex);
                t_cpu_exec* cpuPadre = encontrar_cpu_con_pid(proc->pid);
                pthread_mutex_unlock(&exec_mutex);

                if (cpuPadre != NULL) {
                    reanudar_proceso_en_cpu(cpuPadre);
                }

                free(proc->pathArchivoInstrucciones);
                free(proc);
                break;
            case EXIT: //aca solo recibir el pid, me parece innecesario serializar un bufffer solo para esto
            //despues ver los casos en los que se pasa de READY-> EXIT Y BLOCK->EXIT 

                uint32_t pid;
                buffer_read(paquete->buffer, &pid, sizeof(uint32_t));

                log_info("## (%d) - Solicitó syscall: EXIT", pid);

                // encontrar el pcb en exec_lista y liberarlo
                pthread_mutex_lock(&exec_mutex);
                t_cpu_exec* cpu = encontrar_cpu_con_pid(pid);

                t_pcb* pcbFin = cpu->pcb;
                cpu->pcb = NULL;
                pthread_mutex_unlock(&exec_mutex);

                // notificar al KM que libere los recursos del proceso
                enviar_fin_proceso_memory(pid);

                //revisar si va esta de aca abajo
                enviar_fin_proceso_a_cpu(pid, socketCliente);

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

                log_info(loggerScheduler, "## (%d) - Solicitó syscall: SLEEP", sleep->pid);
                
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

                //ACA TENGO QUE DIFERENCIAR SI HAY IO DISPONIBLE O NO PARA VER SI BLOQUEEO O NO
                pthread_mutex_lock(&mutex_stdout_ocupado);
                if(stdout_ocupado){
                    bloquear_proceso(cpuUsadaStdout->pcb);
                } else{
                    reanudar_proceso_en_cpu(cpuUsadaStdout);
                    sem_post(&sem_hay_cpu_libre);
                }
                pthread_mutex_unlock(&mutex_stdout_ocupado);

                pthread_mutex_unlock(&exec_mutex);

                procesoStdout->cadenaLeida = solicitar_cadena_a_memory(procesoStdout->pid, procesoStdout->direccionLogica, procesoStdout->bytesALeer);
                if(procesoStdout->cadenaLeida != NULL){
                    pthread_mutex_lock(&mutex_cola_stdout);
                    queue_push(cola_stdout, procesoStdout);
                    pthread_mutex_unlock(&mutex_cola_stdout);
                    sem_post(&sem_hay_proc_esperando_stdout);
                }else{
                    log_error("no se pudieron leer los bytes que solicito el pid: (%d)", procesoStdout->pid);
                }
                
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
                
                if(pcbParaDesbloquear!=NULL){
                    pthread_mutex_lock(&ready_mutex);
                    queue_push(ready_cola, pcbParaDesbloquear);
                    pthread_mutex_unlock(&ready_mutex);
                    sem_post(&sem_hay_proceso_ready);
                }
                pthread_mutex_lock(&mutex_stdout_ocupado);
                stdout_ocupado=false;
                pthread_mutex_unlock(&mutex_stdout_ocupado);

                log_info(loggerScheduler, "## (%d) finalizó IO y pasa a READY", pidFinalizado);
                log_info(loggerScheduler, "## (%d) Pasa del estado BLOCK al estado READY", pidFinalizado);
                sem_post(&sem_stdout_disponible);
                
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
                    mutexParaLista->contador = 1;
                    list_add(lista_mutex, mutexParaLista);
                    pthread_mutex_unlock(&mutex_lista_mutex);
                    
                }else{
                    //ya esxiste =>
                    pthread_mutex_unlock(&mutex_lista_mutex);
                }

                pthread_mutex_lock(&exec_mutex);
                t_cpu_exec* unaCpuPadre = encontrar_cpu_con_pid(mutexNuevo->pid);
                pthread_mutex_unlock(&exec_mutex);

                if (unaCpuPadre != NULL) {
                    reanudar_proceso_en_cpu(unaCpuPadre);
                }
                
                //uint32_t existe=1;
                //send(socketCliente, &existe, sizeof(uint32_t), 0);decidimos que no es necesario avisarle, ya que si ya esta creado, no realizamos nada y listo
                free(mutexNuevo->nombreMutex);
                free(mutexNuevo);
                break;
            
            case MUTEX_LOCK://creeeo que esta bien
                t_mutex_syscall* mutexABloquear = deserializar_mutex(paquete->buffer);

                log_info(loggerScheduler, "## (%d) - Solicitó syscall: MUTEX_LOCK", mutexABloquear->pid);
                
                pthread_mutex_lock(&mutex_lista_mutex);
                t_mutex_syscall* mutexABloquearEnLista = buscar_mutex(mutexABloquear->nombreMutex);
                

                pthread_mutex_lock(&exec_mutex);
                t_cpu_exec* cpuALiberar = encontrar_cpu_con_pid(mutexABloquear->pid);
                pthread_mutex_unlock(&exec_mutex);

                if(mutexABloquearEnLista->contador < 1){
                    //esta tomado

                    mutexABloquearEnLista->contador--;
                    queue_push(mutexABloquearEnLista->colaEspera, cpuALiberar->pcb);

                    bloquear_proceso(cpuALiberar->pcb);

                    pthread_mutex_unlock(&mutex_lista_mutex);

                    sem_post(&sem_hay_cpu_libre);
                }
                else{
                    //no esta tomado
                    mutexABloquearEnLista->contador--;
                    mutexABloquearEnLista->pid = mutexABloquear->pid;
                    pthread_mutex_unlock(&mutex_lista_mutex);
                    log_info(loggerScheduler, "## (%d) Toma el Mutex %s", mutexABloquear->pid, mutexABloquear->nombreMutex);
                    if(cpuALiberar!=NULL){
                        reanudar_proceso_en_cpu(cpuALiberar);
                    }
                }

                free(mutexABloquear->nombreMutex);
                free(mutexABloquear);
                break;
                
            case MUTEX_UNLOCK:
                t_mutex_syscall* mutexALiberar = deserializar_mutex(paquete->buffer);
                log_info(loggerScheduler, "## (%d) - Solicitó syscall: MUTEX_UNLOCK", mutexALiberar->pid);

                pthread_mutex_lock(&mutex_lista_mutex);
                t_mutex_syscall* mutexALiberarEnLista = buscar_mutex(mutexALiberar->nombreMutex);

                mutexALiberarEnLista->contador++;

                log_info(loggerScheduler, "## (%d) Libera el Mutex %s", mutexALiberar->pid, mutexALiberar->nombreMutex);

                if(mutexALiberarEnLista->contador==1) {
                    // nadie esperando, queda libre
                    pthread_mutex_unlock(&mutex_lista_mutex);
                } else {
                    // hay alguien esperando, le damos el mutex
                    
                    t_pcb* siguiente = queue_pop(mutexALiberarEnLista->colaEspera);
                    mutexALiberarEnLista->pid = siguiente->pid;
                    pthread_mutex_unlock(&mutex_lista_mutex);

                    log_info(loggerScheduler, "## (%d) Toma el Mutex %s", siguiente->pid, mutexALiberar->nombreMutex);

                    // mover de BLOCK → READY
                    buscar_y_sacar_de_block(siguiente->pid);
                    pthread_mutex_lock(&ready_mutex);
                    queue_push(ready_cola, siguiente);
                    pthread_mutex_unlock(&ready_mutex);

                    log_info(loggerScheduler, "## (%d) Pasa del estado BLOCK al estado READY", siguiente->pid);
                    sem_post(&sem_hay_proceso_ready);
                }

                pthread_mutex_lock(&exec_mutex);
                t_cpu_exec* otraCpuPadre = encontrar_cpu_con_pid(mutexALiberar->pid);
                pthread_mutex_unlock(&exec_mutex);

                if (otraCpuPadre != NULL) {
                    reanudar_proceso_en_cpu(otraCpuPadre);
                }

                free(mutexALiberar->nombreMutex);
                free(mutexALiberar);
                break;
            case MEM_ALLOC:
                //por lo que entiendo aca solo funciono como intermediario para loguear que se solicito la syscall
                t_mem_alloc* infoMemAlloc = deserializar_mem_alloc(paquete->buffer);
                log_info(loggerScheduler, "## (%d) - Solicitó syscall: MEM_ALLOC", infoMemAlloc->pid);

                pthread_mutex_lock(&exec_mutex);
                t_cpu_exec* cpuAlloc = encontrar_cpu_con_pid(infoMemAlloc->pid);
                pthread_mutex_unlock(&exec_mutex);
                //sollicitar segmento a KM(acordarme de avisar a cpu que se ejecuto syscall, tipo reaundar_proc y eso)
                op_code rtaKM = solicitar_segmento_memory(infoMemAlloc);

                if(rtaKM == MEMORIA_DISPONIBLE){
                //HAY MEMORIA DISPONIBLE => confirmar creacion a CPU, no se bloquea
                    if(cpuAlloc!=NULL){
                        reanudar_proceso_en_cpu(cpuAlloc);
                    }
                }
                if(rtaKM==MEMORIA_NO_DISPONIBLE){
                //NO HAY MEMORIA DISPONIBLE => se bloquea el proceso, prestar atencian a cuando KM me avisa que hay nuevo memory stick conectado y a la planificacion de mediano plazo
                    bloquear_proceso(cpuAlloc->pcb);
                    sem_post(&sem_hay_cpu_libre);
                }
                if(rtaKM==COMPACTACION){
                //HAY MEMORIA PERO NO DISPONIBLE => se dispara compactacion (todavia no lo implemente a eso)
                    //compactacion();
                    t_paquete* pacProcsDesalojados = crear_paquete(PROCESOS_DESALOJADOS, NULL);
                    enviar_paquete(socketConexionMemory,pacProcsDesalojados);
                    free(pacProcsDesalojados);
                }
                
                
                
                free(infoMemAlloc);

                break;
            case MEM_FREE:
                t_mem_free* infoMemFree = deserializar_mem_free(paquete->buffer);

                log_info(loggerScheduler, "## (%d) - Solicitó syscall: MEM_FREE", infoMemFree->pid);

                //avisarle a KM que libere el segmento
                t_paquete* paqueteFree = crear_paquete(MEM_FREE, paquete->buffer);
                pthread_mutex_lock(&mutex_socket_memory);
                enviar_paquete(socketConexionMemory, paqueteFree);
                pthread_mutex_unlock(&mutex_socket_memory);
                //No espero el OK de memory pq se puede liberar sin restriccion.
                //confirmarle a CPU que se libero (no hay restriccion para liberar)
                uint32_t okFree = 1;
                send(socketCliente, &okFree,sizeof(uint32_t),0);


                pthread_mutex_lock(&exec_mutex);
                t_cpu_exec* CpuFree = encontrar_cpu_con_pid(infoMemFree->pid);
                pthread_mutex_unlock(&exec_mutex);

                if (CpuFree != NULL) {
                    reanudar_proceso_en_cpu(CpuFree);
                }

                free(paqueteFree);//no libero el buffer pq es del otro paquete
                free(infoMemFree);
                break;
            default:
                log_error("------recibi %d , y no lo entiendo", paquete->codigo_operacion);
                break;
            
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



