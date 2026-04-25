#include <cpu.h>

//PROTOTIPOS

int main(int argc, char* argv[]) {

    //Verifico haber recibido todos los argumentos
    if(argc < 3){
        fprintf(stderr, "Faltan argumentos\n"); 
        return EXIT_FAILURE; 
    }

    //inicializo log y config
    inicializar_log_y_config(argv[1], argv[2]);

    //LEVANTAR CONEXION CON MEMORY
    int socketConexionMemory = iniciar_conexion(IPMemory, puertoMemory);
    if(socketConexionMemory == EXIT_FAILURE){
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