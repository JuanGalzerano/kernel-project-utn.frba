#include <scheduler.h>

int main(int argc, char* argv[]){ //argv[1]: Path al config, argv[2]: path al proceso inicial. [ejs: ./bin/kernel_scheduler kernel.config ./procesos/init.prog]

    inicializar(argv[1]); 

//LEVANTAR CONEXION CON MEMORY
    socketConexionMemory = iniciar_conexion(IPMemory, puertoMemory);
    if(socketConexionMemory == EXIT_FAILURE){
        log_error(loggerScheduler, "no se pudo conectar a Kernel Memory");
        abort();
    }
    log_info(loggerScheduler, "## Conectado a Kernel Memory");
    handshake_cliente_id(socketConexionMemory, loggerScheduler, SCHEDULER);

    socketMemoryNotif = iniciar_conexion(IPMemory, puertoMemoryNotif);
    if(socketMemoryNotif == EXIT_FAILURE){
        log_error(loggerScheduler, "no se pudo conectar al canal de notificaciones de Kernel Memory");
        abort();
    }
    log_info(loggerScheduler, "Conectado al canal de notificaciones de Kernel Memory");

    pthread_t hilo_memory_notif;
    pthread_create(&hilo_memory_notif, NULL, hilo_escuchar_memory, NULL);
    pthread_detach(hilo_memory_notif);
   
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
    //pthread_t hilo_memory;
    //pthread_create(&hilo_memory, NULL, hilo_escuchar_memory, NULL);
    //pthread_detach(hilo_memory);

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
        if(paquete==NULL){//yo creo q siempre van a ser CPUs las que se pueden desonectar, no creo que evaluen la desconexion de IOs
            pthread_mutex_lock(&exec_mutex);
            for(int i =0; i<list_size(exec_lista);i++){
                t_cpu* cpu = list_get(exec_lista, i);
                if(socketCliente == cpu->socketConexion){
                    log_info(loggerScheduler,"## Se desconectó la CPU ID: %d", cpu->cpu_id);
                    list_remove_element(exec_lista, cpu);//ACACREO QUE PODIRA HACER REMOVE_ELEMNT_BY_CONDITION Y ME AHORRARIA TODO EL FOR Y ESO

                    if(cpu->pcb!=NULL){
                        liberar_de_mutex_por_muerte(cpu->pcb);
                        log_info(loggerScheduler, "## (%d) Pasa del estado EXEC al estado EXIT", cpu->pcb->pid);
                        log_info(loggerScheduler, "## (%d) finalizó su ejecución con motivo de Desconexion de CPU", cpu->pcb->pid);
                        enviar_fin_proceso_memory(cpu->pcb->pid);//LINEA ESCRITA DESDE EL CELU DE JUANI!!! FIJARSE SI FUNCA

                        
                        free(cpu->pcb);
                    }
                
                    free(cpu);
                }
                    
            }
            pthread_mutex_unlock(&exec_mutex);
            break; 
        }

        switch(paquete->codigo_operacion){
            case FINALIZAR_POR_QUANTUM:
                
                uint32_t pidInterrumpido;

                buffer_read(paquete->buffer, &pidInterrumpido, sizeof(uint32_t));

                pthread_mutex_lock(&exec_mutex);
                t_cpu* cpuFinQuantum = encontrar_cpu_con_pid(pidInterrumpido);
                if(cpuFinQuantum==NULL){
                    pthread_mutex_unlock(&exec_mutex);
                    break;
                }
                

                log_info(loggerScheduler, "## (%d) - Desalojado por fin de quantum", cpuFinQuantum->pcb->pid);
                log_info(loggerScheduler, "## (%d) Pasa del estado EXEC al estado READY", pidInterrumpido);


                
                t_pcb* pcb = cpuFinQuantum->pcb;
                cpuFinQuantum->pcb = NULL;
                pthread_mutex_unlock(&exec_mutex);

                encolar_pcb_ready(pcb);

                liberar_cpu_y_notificar();
                sem_post(&sem_hay_proceso_ready);

                break;
            case INIT_PROC:
                t_init_proc* proc = deserializar_init_proc(paquete->buffer);
                uint32_t nuevoPid = generar_pid();

                log_info(loggerScheduler, "## (%d) - Solicitó syscall: INIT_PROC", proc->pid);

                pthread_mutex_lock(&exec_mutex);
                t_cpu* cpuPadre = encontrar_cpu_con_pid(proc->pid);
                if(cpuPadre==NULL){
                    pthread_mutex_unlock(&exec_mutex);
                    break;
                }

                t_pcb* procPcb = cpuPadre->pcb;
                cpuPadre->pcb = NULL;
                pthread_mutex_unlock(&exec_mutex);
                liberar_cpu_y_notificar();

                log_info(loggerScheduler, "## (%d) Pasa del estado EXEC al estado READY", proc->pid);

                encolar_pcb_ready(procPcb);

                sem_post(&sem_hay_proceso_ready);
                

                crear_proceso(nuevoPid, proc->pathArchivoInstrucciones, proc->prioridad);

                

                free(proc->pathArchivoInstrucciones);
                free(proc);
                break;
            case EXIT: //aca solo recibir el pid, me parece innecesario serializar un bufffer solo para esto
            //despues ver los casos en los que se pasa de READY-> EXIT Y BLOCK->EXIT 

                uint32_t pid;
                buffer_read(paquete->buffer, &pid, sizeof(uint32_t));

                log_info(loggerScheduler, "## (%d) - Solicitó syscall: EXIT", pid);

                // encontrar el pcb en exec_lista y liberarlo
                pthread_mutex_lock(&exec_mutex);
                t_cpu* cpu = encontrar_cpu_con_pid(pid);

                if(cpu==NULL){
                    pthread_mutex_unlock(&exec_mutex);
                    break;
                }

                t_pcb* pcbFin = cpu->pcb;
                cpu->pcb = NULL;
                pthread_mutex_unlock(&exec_mutex);

                //notificar al KM que libere los recursos del proceso
                enviar_fin_proceso_memory(pid);

                //revisar si va esta de aca abajo
                enviar_fin_proceso_a_cpu(pid, socketCliente);

                //loguear y liberar el PCB
                liberar_de_mutex_por_muerte(pcbFin);
                log_info(loggerScheduler, "## (%d) Pasa del estado EXEC al estado EXIT", pid);
                log_info(loggerScheduler, "## (%d) finalizó su ejecución con motivo de EXIT", pid);
               
                free(pcbFin);

                //la CPU quedó libre
                liberar_cpu_y_notificar();
                break;
            case SLEEP://se recibe tiempo a dormir y pid
                
                t_sleep* sleep = deserializar_sleep(paquete->buffer);

                pthread_mutex_lock(&exec_mutex);
                t_cpu* cpuSleep = encontrar_cpu_con_pid(sleep->pid); 
                if(cpuSleep==NULL){
                    pthread_mutex_unlock(&exec_mutex);
                    break;
                }
                pthread_mutex_unlock(&exec_mutex);

                log_info(loggerScheduler, "## (%d) - Solicitó syscall: SLEEP", sleep->pid);
                
                bloquear_proceso(cpuSleep->pcb);

                pthread_mutex_lock(&mutex_cola_sleep);
                queue_push(cola_sleep, sleep);
                pthread_mutex_unlock(&mutex_cola_sleep);

                sem_post(&sem_hay_proc_esperando_sleep);
                liberar_cpu_y_notificar();
              
                break;
            case STDIN:
            
                t_stdin_stdout* procesoStdin = deserializar_stdin(paquete->buffer);//cuando viotti me lo mande, cadenaLeida=NULL

                pthread_mutex_lock(&exec_mutex);
                t_cpu* cpuUsada = encontrar_cpu_con_pid(procesoStdin->pid); 
                if(cpuUsada==NULL){
                    pthread_mutex_unlock(&exec_mutex);
                    break;
                }
                pthread_mutex_unlock(&exec_mutex);  

                log_info(loggerScheduler, "## (%d) - Solicitó syscall: STDIN", procesoStdin->pid);
                   
                bloquear_proceso(cpuUsada->pcb);           
             
                pthread_mutex_lock(&mutex_cola_stdin);
                queue_push(cola_stdin, procesoStdin);
                pthread_mutex_unlock(&mutex_cola_stdin);

                sem_post(&sem_hay_proc_esperando_stdin);

                liberar_cpu_y_notificar();
                break;
            case STDOUT:
                t_stdin_stdout* procesoStdout = deserializar_stdin(paquete->buffer); //cuando viotti me lo mande, cadenaLeida=NULL

                pthread_mutex_lock(&exec_mutex);
                t_cpu* cpuUsadaStdout = encontrar_cpu_con_pid(procesoStdout->pid); 
                if(cpuUsadaStdout==NULL){
                    pthread_mutex_unlock(&exec_mutex);
                    break;
                }

                log_info(loggerScheduler, "## (%d) - Solicitó syscall: STDOUT", procesoStdout->pid);

                //ACA TENGO QUE DIFERENCIAR SI HAY IO DISPONIBLE O NO PARA VER SI BLOQUEEO O NO
                pthread_mutex_lock(&mutex_stdout_ocupado);
                if(stdout_ocupado){
                    t_pcb* pcbStdout = cpuUsadaStdout ? cpuUsadaStdout->pcb : NULL;
                    pthread_mutex_unlock(&exec_mutex);
                    if(pcbStdout !=NULL){bloquear_proceso(cpuUsadaStdout->pcb);}
                } else{
                    t_pcb* stdoutPcb = cpuUsadaStdout->pcb;
                    
                    pthread_mutex_unlock(&exec_mutex);

                    bloquear_proceso(stdoutPcb);
                    liberar_cpu_y_notificar();
                }
                pthread_mutex_unlock(&mutex_stdout_ocupado);

                

                procesoStdout->cadenaLeida = solicitar_cadena_a_memory(procesoStdout->pid, procesoStdout->direccionLogica, procesoStdout->bytesALeer);
                if(procesoStdout->cadenaLeida != NULL){
                    pthread_mutex_lock(&mutex_cola_stdout);
                    queue_push(cola_stdout, procesoStdout);
                    pthread_mutex_unlock(&mutex_cola_stdout);
                    sem_post(&sem_hay_proc_esperando_stdout);
                }else{
                    log_error(loggerScheduler ,"no se pudieron leer los bytes que solicito el pid: (%d)", procesoStdout->pid);
                }
                
                break;
            case FINALIZAR_SLEEP: 
                
                uint32_t pidTerminado;

                buffer_read(paquete->buffer, &pidTerminado, sizeof(uint32_t));

                t_pcb* pcbADesbloquear = buscar_y_sacar_de_block(pidTerminado);
                
                if(pcbADesbloquear!=NULL){
                    encolar_pcb_ready(pcbADesbloquear);

                    log_info(loggerScheduler, "## (%d) finalizó IO y pasa a READY", pidTerminado);
                    log_info(loggerScheduler, "## (%d) Pasa del estado BLOCK al estado READY", pidTerminado);
                    sem_post(&sem_sleep_disponible);
                    sem_post(&sem_hay_proceso_ready);

                    despertar_planificador();
                }else{
                    log_info(loggerScheduler, "## (%d) finalizó IO y pasa a SUSP READY", pidTerminado);
                    pasar_des_susp_block_a_ready(pidTerminado, NULL, 0);
                    sem_post(&sem_sleep_disponible);
                }
                break;
            case FINALIZAR_STDOUT: 
                
                uint32_t pidFinalizado = buffer_read_uint32(paquete->buffer);

                t_pcb* pcbParaDesbloquear = buscar_y_sacar_de_block(pidFinalizado);
                t_proc_suspendido* procSusp = buscar_en_susp_block(pidFinalizado);
                
                if(pcbParaDesbloquear!=NULL){
                    encolar_pcb_ready(pcbParaDesbloquear);
                    log_info(loggerScheduler,"## (%d) finalizó IO y pasa a READY", pidFinalizado);
                    log_info(loggerScheduler,"## (%d) Pasa del estado BLOCK al estado READY", pidFinalizado);
                    sem_post(&sem_stdout_disponible);
                    sem_post(&sem_hay_proceso_ready);
                    despertar_planificador();
                }else if (procSusp !=NULL)
                {
                    //t_proc_suspendido* enSuspBlock = buscar_en_susp_block(pidFinalizado);
                    //imposible qe sa NULL si se lelgo hasta aca asi q comento la de arriba y ni verifico si da NULLCREO Q COMENT VIEJO
                    log_info(loggerScheduler,"## (%d) finalizó IO y pasa a SUSP READY",pidFinalizado);
                    pasar_des_susp_block_a_ready(pidFinalizado, NULL, 0);
                    sem_post(&sem_stdout_disponible);   
                }

                pthread_mutex_lock(&mutex_stdout_ocupado);
                stdout_ocupado=false;
                pthread_mutex_unlock(&mutex_stdout_ocupado);

                break;
            case FINALIZAR_STDIN:
                //recibir pid, bytes, cadenaLeida y  del IO

                t_stdin_stdout* resultado = deserializar_stdin(paquete->buffer);
                t_pcb* pcbDesblockeado = buscar_y_sacar_de_block(resultado->pid);

                if(pcbDesblockeado != NULL){

                    pthread_mutex_lock(&mutex_socket_memory);
                    t_paquete* paqueteKM = crear_paquete(ESCRIBIR_BYTES, paquete->buffer);
                    enviar_paquete(socketConexionMemory, paqueteKM);
                    free(paqueteKM);

                    //recibo ok
                    op_code ok = recibir_respuesta_memory();
                    pthread_mutex_unlock(&mutex_socket_memory);
                    if(ok == ESCRITURA_FALLIDA){
                        log_error(loggerScheduler, "## (%d) - Escritura fallida en KM", resultado->pid);
                        free(resultado->cadenaLeida);
                        free(resultado);
                        break;
                    }
                    encolar_pcb_ready(pcbDesblockeado);
                    log_info(loggerScheduler, "## (%d) finalizó IO y pasa a READY", resultado->pid);
                    log_info(loggerScheduler, "## (%d) Pasa del estado BLOCK al estado READY", resultado->pid);
                    sem_post(&sem_stdin_disponible);
                    sem_post(&sem_hay_proceso_ready);
                    despertar_planificador();
                }else{
                    //segmento en swap,guarda para escribir al desuspender
                    log_info(loggerScheduler, "## (%d) finalizó IO y pasa a SUSP READY", resultado->pid);
                    pasar_des_susp_block_a_ready(resultado->pid,resultado->cadenaLeida,resultado->direccionLogica);
                    sem_post(&sem_stdin_disponible);//SERA???
                    free(resultado);
                    break;
                }
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
                    mutexParaLista->pid = UINT32_MAX;//significaria LIBRE (igual usamos el contador), xq no podemos usar -1 y tampoco podemos usar NULL. Desp ver si esta bien
                    mutexParaLista->contador = 1;
                    list_add(lista_mutex, mutexParaLista);
                    pthread_mutex_unlock(&mutex_lista_mutex);
                    
                }else{
                    //ya esxiste =>
                    pthread_mutex_unlock(&mutex_lista_mutex);
                }

                pthread_mutex_lock(&exec_mutex);
                t_cpu* unaCpuPadre = encontrar_cpu_con_pid(mutexNuevo->pid);
                if(unaCpuPadre==NULL){
                    pthread_mutex_unlock(&exec_mutex);
                    break;
                }
                reanudar_proceso_en_cpu(unaCpuPadre);
                pthread_mutex_unlock(&exec_mutex);
                
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
                //log_debug(loggerScheduler, "encontro el mutex: %s", mutexABloquearEnLista->nombreMutex);
                
                if(mutexABloquearEnLista ==NULL){
                    pthread_mutex_unlock(&mutex_lista_mutex);
                    log_error(loggerScheduler, "Mutex no encontrado: %s", mutexABloquear->nombreMutex);
                    break;
                }

                pthread_mutex_lock(&exec_mutex);
                t_cpu* cpuALiberar = encontrar_cpu_con_pid(mutexABloquear->pid);
                if(cpuALiberar==NULL){
                    pthread_mutex_unlock(&exec_mutex);
                    break;
                }
                //log_debug(loggerScheduler, "encontro la cpu id: %d   con el pid:  %d", cpuALiberar->cpu_id, cpuALiberar->pcb->pid);
                pthread_mutex_unlock(&exec_mutex);

                if(mutexABloquearEnLista->contador < 1){
                    //esta tomado

                    mutexABloquearEnLista->contador--;
                    queue_push(mutexABloquearEnLista->colaEspera, cpuALiberar->pcb);

                    int estabaEnReady = 0;//es si estaba en ready de CMN
                    t_pcb* pcbDueño = buscar_pcb_por_pid(mutexABloquearEnLista->pid,  &estabaEnReady);
                    if(pcbDueño != NULL && pcbDueño->prioridad > cpuALiberar->pcb->prioridad){
                        //el dueño tiene menor prioridad (número mayor = menor prioridad)
                        log_info(loggerScheduler, "## %d Cambio de prioridad: %d - %d",pcbDueño->pid, pcbDueño->prioridad, cpuALiberar->pcb->prioridad);
                        pcbDueño->prioridad = cpuALiberar->pcb->prioridad;
                        //ACA TENGO QUE METER FUNCION PARA QUE SI ESTE ESTABA EN OTRA COLA DE UN MUTEX, VER SI EL DUEÑO DE ESE MUTEX TIENE QUE HEREDAR

                        for(int i=0;i<list_size(lista_mutex);i++){
                            t_mutex_syscall* unMtxEnLista =list_get(lista_mutex,i);
                            int enReady=0;
                            t_pcb* otroDueño = buscar_pcb_por_pid(unMtxEnLista->pid, &enReady);
                            if(unMtxEnLista->pid !=pcbDueño->pid && !queue_is_empty(unMtxEnLista->colaEspera)){//si es el que heredo, el que lo tiene tomado, no tengo que actualizar nada
                                for(int j=0;j<queue_size(unMtxEnLista->colaEspera);j++){
                                    t_pcb* posiblePcb =list_get(unMtxEnLista->colaEspera->elements,j);
                                    if(posiblePcb->pid ==pcbDueño->pid && posiblePcb->prioridad < otroDueño->prioridad){
                                        log_info(loggerScheduler, "## %d Cambio de prioridad: %d - %d",otroDueño->pid, otroDueño->prioridad, posiblePcb->prioridad);
                                        if(enReady){
                                            encolar_pcb_ready(otroDueño);
                                        }
                                    }
                                }
                            }
                        }
                        if(estabaEnReady){//guarda, aco hago referencia al dueño, no al que solicito la syscall, a ese lo bloqueo abajo
                            encolar_pcb_ready(pcbDueño);
                        }
                        despertar_planificador();
                    }

                    bloquear_proceso(cpuALiberar->pcb);

                    pthread_mutex_unlock(&mutex_lista_mutex);//tendria q ir mas arriba creo

                    liberar_cpu_y_notificar();
                }
                else{
                    //no esta tomado
                    mutexABloquearEnLista->contador--;
                    mutexABloquearEnLista->pid = mutexABloquear->pid;
                    pthread_mutex_unlock(&mutex_lista_mutex);
                    log_info(loggerScheduler, "## (%d) Toma el Mutex %s", mutexABloquear->pid, mutexABloquear->nombreMutex);
                    pthread_mutex_lock(&exec_mutex);
                    reanudar_proceso_en_cpu(cpuALiberar); 
                    pthread_mutex_unlock(&exec_mutex);
                    
                }

                free(mutexABloquear->nombreMutex);
                free(mutexABloquear);
                break;
                
            case MUTEX_UNLOCK:
                t_mutex_syscall* mutexALiberar = deserializar_mutex(paquete->buffer);
                log_info(loggerScheduler, "## (%d) - Solicitó syscall: MUTEX_UNLOCK", mutexALiberar->pid);

                pthread_mutex_lock(&mutex_lista_mutex);
                t_mutex_syscall* mutexALiberarEnLista = buscar_mutex(mutexALiberar->nombreMutex);
                if(mutexALiberarEnLista ==NULL){
                    pthread_mutex_unlock(&mutex_lista_mutex);
                    log_error(loggerScheduler, "Mutex no encontrado: %s", mutexABloquear->nombreMutex);
                    break;
                }

                mutexALiberarEnLista->contador++;

                log_info(loggerScheduler, "## (%d) Libera el Mutex %s", mutexALiberar->pid, mutexALiberar->nombreMutex);

                if(mutexALiberarEnLista->contador==1){
                    //nadie esperando, queda libre
                    pthread_mutex_unlock(&mutex_lista_mutex);
                } else {
                    //hay alguien esperando, le damos el mutex
                    
                    t_pcb* siguiente = queue_pop(mutexALiberarEnLista->colaEspera);
                    mutexALiberarEnLista->pid = siguiente->pid;
                    
                    log_info(loggerScheduler, "## (%d) Toma el Mutex %s", siguiente->pid, mutexALiberar->nombreMutex);

                    if(!queue_is_empty(mutexALiberarEnLista->colaEspera)){
                        uint32_t prioridadMaxima = siguiente->prioridad;

                        for(int i=0; i<queue_size(mutexALiberarEnLista->colaEspera);i++){
                            t_pcb* pcbTemporal = list_get(mutexALiberarEnLista->colaEspera->elements,i);
                            if(prioridadMaxima > pcbTemporal->prioridad){
                                prioridadMaxima = pcbTemporal->prioridad;
                            }
                        }
                        if(siguiente->prioridad > prioridadMaxima){
                            log_info(loggerScheduler, "## %d Cambio de prioridad: %d - %d",siguiente->pid, siguiente->prioridad, prioridadMaxima);
                            siguiente->prioridad = prioridadMaxima;
                        }
                        
                    }


                    pthread_mutex_unlock(&mutex_lista_mutex);
                    //mover de BLOCK-> READY
                    t_pcb* pcbASacarDeBlock =  buscar_y_sacar_de_block(siguiente->pid);
                    if(pcbASacarDeBlock!=NULL){
                        encolar_pcb_ready(siguiente);

                        log_info(loggerScheduler, "## (%d) Pasa del estado BLOCK al estado READY", siguiente->pid);
                        sem_post(&sem_hay_proceso_ready);
                        despertar_planificador();
                    }else{
                        pasar_des_susp_block_a_ready(siguiente->pid,NULL,0);
                    }
                }

                pthread_mutex_lock(&exec_mutex);
                t_cpu* otraCpuPadre = encontrar_cpu_con_pid(mutexALiberar->pid);
                if(otraCpuPadre==NULL){
                    pthread_mutex_unlock(&exec_mutex);
                    break;
                }
                pthread_mutex_unlock(&exec_mutex);

                //
                uint32_t prioridadRecalculada =otraCpuPadre->pcb->prioridadOriginal;
                pthread_mutex_lock(&mutex_lista_mutex);
                for(int i=0;i<list_size(lista_mutex);i++){
                    t_mutex_syscall* mtx = list_get(lista_mutex,i);
                    if(mtx->pid == otraCpuPadre->pcb->pid && !queue_is_empty(mtx->colaEspera)){
                        for(int j=0;j<queue_size(mtx->colaEspera);j++){
                            t_pcb* posiblePadrePrior =list_get(mtx->colaEspera->elements,j);

                                if(posiblePadrePrior->prioridad<prioridadRecalculada){
                                    prioridadRecalculada=posiblePadrePrior->prioridad;
                                }
                            
                        }
                    }
                }

                pthread_mutex_unlock(&mutex_lista_mutex);

                uint32_t prioridadAnterior = otraCpuPadre->pcb->prioridad;
                otraCpuPadre->pcb->prioridad = prioridadRecalculada;

                if(prioridadAnterior != otraCpuPadre->pcb->prioridad){
                    log_info(loggerScheduler, "## %d Cambio de prioridad: %d - %d", otraCpuPadre->pcb->pid, prioridadAnterior, otraCpuPadre->pcb->prioridad);
                }

                log_info(loggerScheduler, "## (%d) Pasa del estado EXEC al estado READY", otraCpuPadre->pcb->pid);

                pthread_mutex_lock(&exec_mutex);
                reanudar_proceso_en_cpu(otraCpuPadre);
                pthread_mutex_unlock(&exec_mutex);

                free(mutexALiberar->nombreMutex);
                free(mutexALiberar);
                break;
            case MEM_ALLOC:
                //por lo que entiendo aca solo funciono como intermediario para loguear que se solicito la syscall
                t_mem_alloc* infoMemAlloc = deserializar_mem_alloc(paquete->buffer);
                log_info(loggerScheduler, "## (%d) - Solicitó syscall: MEM_ALLOC", infoMemAlloc->pid);

                pthread_mutex_lock(&exec_mutex);
                t_cpu* cpuAlloc = encontrar_cpu_con_pid(infoMemAlloc->pid);
                if(cpuAlloc==NULL){
                    pthread_mutex_unlock(&exec_mutex);
                    break;
                }
                pthread_mutex_unlock(&exec_mutex);
                //sollicitar segmento a KM(acordarme de avisar a cpu que se ejecuto syscall, tipo reaundar_proc y eso)
                op_code rtaKM = solicitar_segmento_memory(infoMemAlloc,SOLICITAR_SEGMENTO,socketCliente);//aca dentro se gestiona las posibles compactaciones y reintentos


                //LA BUSCAMOS DE NUEVO POR SI HUBO COMPACTACION AL SOLICITAR EL SEGMENTO
                cpuAlloc = encontrar_cpu_con_pid(infoMemAlloc->pid);
                if(cpuAlloc==NULL){
                    pthread_mutex_unlock(&exec_mutex);
                    break;
                }

                if(rtaKM == MEMORIA_DISPONIBLE){
                //HAY MEMORIA DISPONIBLE => confirmar creacion a CPU, no se bloquea
                    if(cpuAlloc!=NULL){
                            reanudar_proceso_en_cpu(cpuAlloc);
                    }
                }
                if(rtaKM==MEMORIA_NO_DISPONIBLE){
                //NO HAY MEMORIA DISPONIBLE => FINALIZA POR OUT OF MEMORY COMO DICE EL ISSUE #5206

                    pthread_mutex_lock(&exec_mutex);
                    t_pcb* pcbOut = cpuAlloc->pcb;
                    cpuAlloc->pcb = NULL;
                    pthread_mutex_unlock(&exec_mutex);
                    liberar_de_mutex_por_muerte(pcbOut);
                    log_info(loggerScheduler, "## (%d) finalizó su ejecución con motivo de Out Of Memory", infoMemAlloc->pid);
                    log_info(loggerScheduler, "## (%d) Pasa del estado EXEC al estado EXIT", infoMemAlloc->pid);
                    enviar_fin_proceso_memory(pcbOut->pid);
                    enviar_fin_proceso_a_cpu(pcbOut->pid, cpuAlloc->socketConexion);
                    
                    free(pcbOut);
                    liberar_cpu_y_notificar();
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
                


                pthread_mutex_lock(&exec_mutex);
                t_cpu* CpuFree = encontrar_cpu_con_pid(infoMemFree->pid);
                if(CpuFree==NULL){
                    pthread_mutex_unlock(&exec_mutex);
                    break;
                }
                pthread_mutex_unlock(&exec_mutex);

                if (CpuFree != NULL){
                    reanudar_proceso_en_cpu(CpuFree);
                }

                free(paqueteFree);//no libero el buffer pq es del otro paquete
                free(infoMemFree);
                break;
            case DESALOJO:
                uint32_t pidDesalojado;
                buffer_read(paquete->buffer, &pidDesalojado, sizeof(uint32_t));

                pthread_mutex_lock(&exec_mutex);
                t_cpu* cpuDesalojada = encontrar_cpu_con_pid(pidDesalojado);
                if(cpuDesalojada==NULL){
                    pthread_mutex_unlock(&exec_mutex);
                    break;
                }
                t_pcb* pcbDesalojado = cpuDesalojada->pcb;
                uint32_t causante_pid = cpuDesalojada->pid_desalojador;
                int causante_prioridad = cpuDesalojada->prioridad_desalojador;
                cpuDesalojada->pcb = NULL;
                pthread_mutex_unlock(&exec_mutex);

                log_info(loggerScheduler, "## (%d) Prioridad: %d - Desalojado por cola más prioritaria por el proceso %d con prioridad %d", pcbDesalojado->pid, pcbDesalojado->prioridad, causante_pid, causante_prioridad);

                encolar_pcb_ready(pcbDesalojado);
                sem_post(&sem_hay_proceso_ready);
                liberar_cpu_y_notificar();
                break;
            case SEG_FAULT:
                uint32_t pidFault = buffer_read_uint32(paquete->buffer);
                pthread_mutex_lock(&exec_mutex);
                t_cpu* cpuFault = encontrar_cpu_con_pid(pidFault);
                if(cpuFault==NULL){
                    pthread_mutex_unlock(&exec_mutex);
                    break;
                }
                t_pcb* pcbFault = cpuFault->pcb;
                cpuFault->pcb=NULL;
                pthread_mutex_unlock(&exec_mutex);

                //notificar al KM que libere los recursos del proceso
                enviar_fin_proceso_memory(pidFault);

                //revisar si va esta de aca abajo
                enviar_fin_proceso_a_cpu(pidFault, socketCliente);
                
                liberar_de_mutex_por_muerte(pcbFault);
                log_info(loggerScheduler, "## (%d) finalizó su ejecución con motivo de Error: Segmentation Fault (SEG_FAULT)", pidFault);
                log_info(loggerScheduler, "## (%d) Pasa del estado EXEC al estado EXIT", pidFault);
                
                free(pcbFault);
                liberar_cpu_y_notificar();
                break;
            case DESALOJAR_POR_BSOD:
                uint32_t pidBsod;
                pidBsod = buffer_read_uint32(paquete->buffer);
                // NO LOGUEO ACA QUE PASAN A EXIT Y EL MOTIVO DE FINALIZACION X SI ME PASA QUE SE EJECUTE EL abort() ANTES DE QUE SE LLEGUE A ESTE CASE
                log_debug(loggerScheduler, "desalojo el pid (%d) por bsod. DESP SACAR ESTE LOG", pidBsod);
                pthread_mutex_lock(&exec_mutex);
                t_cpu* cpuBsod = encontrar_cpu_con_pid(pidBsod);
                if(cpuBsod!=NULL){
                    free(cpuBsod->pcb);
                    cpuBsod->pcb =NULL;
                    free(cpuBsod);//DESP VER SI ESTA VA O NO
                }
                pthread_mutex_unlock(&exec_mutex);
                //liberar_cpu_y_notificar(); DESP VER SI ESTA LINEA VA O NO, YO CREO Q NO PQ YA SE TERMINA TODO

                break;
            
            default:
                log_error(loggerScheduler,"------recibi %d , y no lo entiendo", paquete->codigo_operacion);
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
        t_cpu* nueva_cpu = malloc(sizeof(t_cpu));
        nueva_cpu->cpu_id = atoi(idCPU);//transforma a int
        nueva_cpu->socketConexion = socketCliente;
        nueva_cpu->pcb=NULL;
        nueva_cpu->pid_desalojador = 0;
        nueva_cpu->prioridad_desalojador = 0;
        nueva_cpu->quantum_token = 0;

        //agrego CPU a lista de CPUs
        pthread_mutex_lock(&exec_mutex);
        list_add(exec_lista, nueva_cpu);
        pthread_mutex_unlock(&exec_mutex);
        
        //sem post cpu libre
        if(compactando==false){
            liberar_cpu_y_notificar();//hace el sempost
        }
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



//DESPUES VER XQ CREO QUE TIENE UNA RE ESPERA ACTIVA EL PLANIFICADOR
void* planificador(void* arg){
    while (1){
        //hasta que no tengamos cpu disponible ni proceso, no continuamos
        sem_wait(&sem_hay_proceso_ready);

        if(algoritmo==FIFO || algoritmo == RR){ 
            sem_wait(&sem_hay_cpu_libre);
            if(compactando){
                sem_post(&sem_hay_proceso_ready); 
                sem_post(&sem_hay_cpu_libre);
                pthread_mutex_lock(&mutex_planificador);
                while(compactando) pthread_cond_wait(&cond_planificador, &mutex_planificador);
                pthread_mutex_unlock(&mutex_planificador);
                continue;
            }     
            
            
            t_pcb* pcb =desencolar_pcb_ready();
            

            t_cpu* cpu = obtener_cpu_libre();
            enviar_proceso_a_cpu(cpu, pcb); 

            if (algoritmo == RR){
                iniciar_timer_quantum(cpu);
            }
        }
        if(algoritmo == CMN){
            /*desalojo habilitado y hay uno de menor prior ejecutando*/
            if(hay_desalojo){

                

                pthread_mutex_lock(&mutex_planificador);
                while(compactando){
                    sem_post(&sem_hay_proceso_ready);
                    pthread_cond_wait(&cond_planificador, &mutex_planificador);
                }
                pthread_mutex_unlock(&mutex_planificador);

                t_pcb* prox_pcb = pcb_mas_prioritario();

                t_cpu* cpuDesalojable = hay_cpu_desalojable(prox_pcb);

                if((sem_trywait(&sem_hay_cpu_libre)==0) && (prox_pcb!=NULL)){


                    t_pcb* unPcb = desencolar_pcb_ready();
                    t_cpu* cpuAUsar = obtener_cpu_libre();
                    enviar_proceso_a_cpu(cpuAUsar, unPcb);
                    if(algoritmo_por_cola[unPcb->prioridad] == RR){
                        iniciar_timer_quantum(cpuAUsar);
                    }
                
                }else if ((cpuDesalojable!=NULL) && (prox_pcb!=NULL))
                {
                    t_pcb* otroPcb = desencolar_pcb_ready();
                    
                    pthread_mutex_lock(&exec_mutex);
                    cpuDesalojable->pid_desalojador = otroPcb->pid;
                    cpuDesalojable->prioridad_desalojador = otroPcb->prioridad;
                    pthread_mutex_unlock(&exec_mutex);

                    enviar_desalojo_cpu(cpuDesalojable);

                    //esperamos a que selibere la CPU tras el desalojo
                    sem_wait(&sem_hay_cpu_libre);
                    t_cpu* laCpu = obtener_cpu_libre();
                    enviar_proceso_a_cpu(laCpu, otroPcb);
                    if(algoritmo_por_cola[otroPcb->prioridad] == RR){
                        iniciar_timer_quantum(laCpu);
                    }
                }
                else{
                    //no hay CPU libre ni desalojable
                    sem_post(&sem_hay_proceso_ready);
                    pthread_mutex_lock(&mutex_planificador);
                    pthread_cond_wait(&cond_planificador, &mutex_planificador);
                    pthread_mutex_unlock(&mutex_planificador);
                }

            }else{
                //NO HAY DESALOJO
                sem_wait(&sem_hay_cpu_libre);

                pthread_mutex_lock(&mutex_planificador);
                while(compactando){
                    sem_post(&sem_hay_cpu_libre);
                    pthread_cond_wait(&cond_planificador,&mutex_planificador);
                    //aaca esta dormido y cuando vuelve intenta tomar el sem nuevamente
                    pthread_mutex_unlock(&mutex_planificador);
                    sem_wait(&sem_hay_cpu_libre);
                    pthread_mutex_lock(&mutex_planificador);
                }
                pthread_mutex_unlock(&mutex_planificador);

                t_pcb* elPcb = desencolar_pcb_ready();
                t_cpu* cpuEncontrada = obtener_cpu_libre();
                enviar_proceso_a_cpu(cpuEncontrada, elPcb);
                if(algoritmo_por_cola[elPcb->prioridad] == RR){
                    iniciar_timer_quantum(cpuEncontrada);
                }
            }
        }
        
    }

    //cuando un proceso termina de ejcutar, hacer el log que se pasa a exit y volar el PCB de ese proceso (nose si es que va aca)
}



