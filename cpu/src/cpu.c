#include <cpu.h>

//PROTOTIPOS

int main(int argc, char* argv[]) {

    //Verifico haber recibido todos los argumentos
    if(argc < 3){
        fprintf(stderr, "Faltan argumentos\n"); 
        return EXIT_FAILURE; 
    }
#include "cpu.h"
#include "kernel_memory_avisos.h"
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: ./bin/cpu [Archivo Config] [Identificador]");
        return EXIT_FAILURE;
    }
    inicializar_log_y_config(argv[1], argv[2]);

    uint32_t sizeIdCpu = strlen(idCpu) + 1;

    int socketConexionMemory = iniciar_conexion(IPMemory, puertoMemory);
    if(socketConexionMemory == EXIT_FAILURE){
    if (socketConexionMemory == EXIT_FAILURE) {
        log_info(loggerCpu, "CPU: No se pudo conectar con Kernel Memory");
        abort();
    }
    log_info(loggerCpu, "CPU: Conexion establecida con Kernel Memory");
    handshake_cliente_id(socketConexionMemory, loggerCpu, CPU);
    
    //LEVANTAR CONEXION CON SCHEDULER
    int socketConexionKernel = iniciar_conexion(IPKernel, puertoKernel);
    if(socketConexionKernel == EXIT_FAILURE){
        log_info(loggerCpu, "CPU: No se pudo conectar con Kernel Scheduler");
        abort();
    }
    log_info(loggerCpu, "CPU: Conexion establecida con Kernel Scheduler");
    handshake_cliente_id(socketConexionKernel, loggerCpu, CPU);

    if ()//si hay memory sticks conectarme a todas
    {
        //LEVANTAR CONEXION CON MEMORY STICK
        int socketConexionMemoryStick = iniciar_conexion(IPMemoryStick, puertoMemoryStick);
        if(socketConexionMemoryStick == EXIT_FAILURE){
            log_info(loggerCpu, "CPU: No se pudo conectar con Memory Stick");
            abort();
        }
        log_info(loggerCpu, "CPU: Conexion establecida con Memory Stick");
        handshake_cliente_id(socketConexionMemoryStick, loggerCpu, CPU);
    }
//1
    //Una vez establecidas todas las conexiones
    esperar_pid_del_scheduler();
//2
    //Una vez recibido
    solicitar_contexto_a_memory();
//3
    //Una vez recibido
    ejecutar_ciclo_de_instruccion();

    //Liberamos memoria
    close(socketConexionMemory);
    close(socketConexionKernel);
    close(socketConexionMemoryStick);
    config_destroy(configCpu);
    liberar_log(loggerCpu);

    return 0;
}

//DECLARACION FUNCIONES
void enviar_peticion_scheduler(void) {}
void esperar_pid_del_scheduler(void) {}
void recibir_solicitud_de_desalojo(void) {}

void enviar_peticion_io(void) {} //sleep, stdin o stdout

void solicitar_contexto_a_memory(void) {}
void solicitar_instruccion_a_memory(pc, pid) {} //Le paso el IP?
void recibir_nuevas_memory_sticks(void) {} //Ni idea

void ejecutar_ciclo_de_instruccion(void) {}
void fetch(void) {}
void decode(void) {}
void execute(instruccion) {
    switch (instruccion)
    {
        case NOOP:
            break;
        case SET:
            break;
        case MOV_IN:
            break;
        case MOV_OUT:
            break;
        case SUM:
            break;
        case SUB:
            break;
        case JNZ:
            break;
        case COPY_MEM:
            break;
        //syscall
        case MUTEX_CREATE:
            break;
        case MUTEX_LOCK:
            break;
        case MUTEX_UNLOCK:
            break;
        case MEM_ALLOC:
            break;
        case MEM_FREE:
            break;
        case SLEEP:
            break;
        case STDOUT:
            break;
        case STDIN:
            break;
        case INIT_PROC:
            break;
        case EXIT:
            break;
    }
    //Siempre y cuando el PC no haya sido modificado por la instruccion
    actualizar_pc();
}
void actualizar_pc(void) {}
void check_interrupt(void){}

void traducir_direcciones_logicas_a_fisicas(void) {}
    send(socketConexionMemory, &sizeIdCpu, sizeof(int), 0);
    send(socketConexionMemory, idCpu, sizeIdCpu, 0);

    uint32_t segment_max_size = 0;
    recv(socketConexionMemory, &segment_max_size, sizeof(uint32_t), MSG_WAITALL);
    if (segment_max_size == 0) {
        log_info(loggerCpu, "CPU: Error, recibi segment_max_size = 0");
        abort();
    }

    int socketConexionScheduler = iniciar_conexion(IPKernel, puertoKernel);
    if (socketConexionScheduler == EXIT_FAILURE) {
        log_info(loggerCpu, "CPU: No se pudo conectar con Kernel Scheduler");
        abort();
    }
    log_info(loggerCpu, "CPU: Conexion establecida con Kernel Scheduler");
    handshake_cliente_id(socketConexionScheduler, loggerCpu, CPU);
    send(socketConexionScheduler, &sizeIdCpu, sizeof(int), 0);
    send(socketConexionScheduler, idCpu, sizeIdCpu, 0);

    int socketConexionMemoryAvisos = iniciar_conexion(IPMemory, puertoKernelMemoryNotificaciones);
    if (socketConexionMemoryAvisos == EXIT_FAILURE) {
        log_info(loggerCpu, "CPU: No se pudo conectar con Kernel Memory de Avisos");
        abort();
    }
    log_info(loggerCpu, "CPU: Conexion establecida con Kernel Memory Avisos");
    handshake_cliente_id(socketConexionMemoryAvisos, loggerCpu, CPU);
    send(socketConexionMemoryAvisos, &sizeIdCpu, sizeof(int), 0);
    send(socketConexionMemoryAvisos, idCpu, sizeIdCpu, 0);

    lista_memory_stick = list_create();
    pthread_mutex_init(&mutex_lista_memory_stick, NULL);
    sem_init(&sem_lista_cargada, 0, 0);

    log_info(loggerCpu, "CPU: Creo hilo de avisos de Kernel Memory");
    int* argSocketAvisos = malloc(sizeof(int));
    *argSocketAvisos = socketConexionMemoryAvisos;
    pthread_t hiloKernelMemoryAvisos;
    pthread_create(&hiloKernelMemoryAvisos, NULL, hilo_kernel_memory_avisos, argSocketAvisos);
    pthread_detach(hiloKernelMemoryAvisos);

    log_info(loggerCpu, "CPU: Espero a que haya al menos 1 stick");
    sem_wait(&sem_lista_cargada);

    log_info(loggerCpu, "CPU: Comienzo como cpu");
    while (1) {
        log_debug(loggerCpu, "CPU_DEBUG: Solicitando PID a Scheduler");
        uint32_t pid = obtener_pid(socketConexionScheduler);
        log_info(loggerCpu, "CPU: Obtuve PID: %d", pid);

        log_debug(loggerCpu, "CPU_DEBUG: Solicitando contexto a Kernel Memory");
        t_contexto_ejecucion *ctx = obtener_contexto(pid, socketConexionMemory);
        log_debug(loggerCpu, "CPU_DEBUG: Obtuve contexto de ejecucion");
        actualizar_registros_cpu(ctx);
        log_info(loggerCpu, "CPU: Contexto actualizado");

        int errorCiclo = 0;
        log_info(loggerCpu, "CPU: Iniciando ciclo de instruccion");
        while (!hay_interrupcion(pid, socketConexionScheduler)) {
            errorCiclo = ejecutar_ciclo_de_instruccion(socketConexionMemory, socketConexionScheduler, pid, segment_max_size, ctx->tabla_segmentos);
            if (errorCiclo < 0) {
                log_info(loggerCpu, "CPU: Error en el ciclo de instruccion");
                break;
            }
            if (errorCiclo == 1) {
                log_info(loggerCpu, "CPU: Syscall ejecutada");
                break;
            }
            if (errorCiclo == COD_SEG_FAULT) {
                log_info(loggerCpu, "CPU: Segmentation Fault, proceso desalojado");
                break;
            }
            if (errorCiclo == COD_EXIT) {
                log_info(loggerCpu, "CPU: EXIT ejecutado, descartando contexto");
                break;
            }
        }

        if (errorCiclo == COD_SEG_FAULT || errorCiclo == COD_EXIT || errorCiclo == -1) {
            log_debug(loggerCpu, "DEBUG_CPU: PID %d - Contexto descartado (SEG_FAULT, EXIT o BSOD, no se actualiza KM)", pid);
            liberar_contexto(ctx);
            continue;
        }
        if (motivo_interrupcion != DESALOJAR_POR_BSOD) {
            log_info(loggerCpu, "CPU: Envio el contexto a Kernel Memory");
            actualizar_contexto(ctx, socketConexionMemory, pid);
        }
        if (errorCiclo != 1) {
            log_info(loggerCpu, "CPU: Envio PID y motivo de interrupcion a Scheduler");
            enviar_pid_y_motivo(pid, motivo_interrupcion, socketConexionScheduler);
            if (motivo_interrupcion == DESALOJAR_POR_BSOD) {
                abort();
            }
        }
    }
    config_destroy(configCpu);
    return 0;
}
