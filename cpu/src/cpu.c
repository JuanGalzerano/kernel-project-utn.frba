#include "cpu.h"
#include "kernel_memory_avisos.h"
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: ./bin/cpu [Archivo Config] [Identificador]\n");
        return EXIT_FAILURE;
    }

    inicializar_log_y_config(argv[1], argv[2]);

    uint32_t sizeIdCpu = strlen(idCpu) + 1;
//1) CONECTARME
    int socketConexionMemory = iniciar_conexion(IPMemory, puertoMemory);
    if (socketConexionMemory == EXIT_FAILURE) {
        log_info(loggerCpu, "CPU: No se pudo conectar con Kernel Memory");
        abort();
    }
    log_info(loggerCpu, "CPU: Conexion establecida con Kernel Memory");
    handshake_cliente_id(socketConexionMemory, loggerCpu, CPU);
    send(socketConexionMemory, &sizeIdCpu, sizeof(int), 0);
    send(socketConexionMemory, idCpu, sizeIdCpu, 0);

    uint32_t segment_max_size = 0; //Esto que me lo pase juani siempre despues del handshake
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

    // Levanto un hilo para escuchar al kernel_memory para que me avise de las memory sticks
    int* argSocketAvisos = malloc(sizeof(int));
    *argSocketAvisos = socketConexionMemoryAvisos;
    pthread_t hiloKernelMemoryAvisos;
    pthread_create(&hiloKernelMemoryAvisos, NULL, hilo_kernel_memory_avisos, argSocketAvisos);
    pthread_detach(hiloKernelMemoryAvisos);

    sem_wait(&sem_lista_cargada); //Espero a cargar la lista de memories sticks

//2) LUEGO DE CONECTARME, CUMPLO LA FUNCION DE CPU REAL
    while (1) {
        //1) SOLICITO PID (bloquea hasta recibir EJECUTAR_PROCESO)
        uint32_t pid = obtener_pid(socketConexionScheduler);
        log_info(loggerCpu, "CPU: Obtuve PID: %d", pid);

        //2) CON ESE PID SOLICITO CONTEXTO
        t_contexto_ejecucion *ctx = obtener_contexto(pid, socketConexionMemory);
        log_info(loggerCpu, "CPU: Obtuve contexto de ejecucion");
        actualizar_registros_cpu(ctx);
        log_info(loggerCpu, "CPU: Contexto cargado, iniciando ciclo");

        //3) MIENTRAS NO HAYA INTERRUPCION, EJECUTO CICLO
        // hay_interrupcion se llama entre instrucciones (select con timeout 0)
        int errorCiclo = 0;
        while (!hay_interrupcion(pid, socketConexionScheduler)) {
            errorCiclo = ejecutar_ciclo_de_instruccion(socketConexionMemory, socketConexionScheduler, pid, segment_max_size, ctx->tabla_segmentos);
            if (errorCiclo < 0) {
                log_info(loggerCpu, "CPU: Error en el ciclo de instruccion");
                abort();
            }
            if (errorCiclo == 1) {
                log_info(loggerCpu, "CPU: Syscall ejecutada");
                break;
            }
        }
        actualizar_contexto(ctx, socketConexionMemory, pid);
        if (errorCiclo != 1) { //Si es distinto de 1 es pq sali por interrupcion
            enviar_pid_y_motivo(pid, motivo_interrupcion, socketConexionScheduler);
        }
    }

    return 0;
}