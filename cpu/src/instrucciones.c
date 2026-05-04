#include "/home/utnso/tp-2026-1c-Los-Reyes-De-La-Compilaci-n/cpu/src/instrucciones.h"

void ejecutar_set(Codigo_registros_cpu tipoRegistro, uint32_t valor) {
    escribir_valor_en_registro(tipoRegistro, valor);
}
void ejecutar_mov_in(Codigo_registros_cpu tipoRegistro) {
    int valorDeMemoriaLogica = registros_cpu.si;
    escribir_valor_en_registro(tipoRegistro, valorDeMemoriaLogica);
}
/*void ejecutar_mov_out(Codigo_registros_cpu tipoRegistro) {
    int valor = leer_valor_en_registro(tipoRegistro);
    int direccionLogica = leer_valor_en_registro(DI);
    int direccionFisica = traducir_direccion(direccionLogica);
    escribir_memory(direccionFisica, valor);
}*/
void ejecutar_sum(Codigo_registros_cpu registroDestino, Codigo_registros_cpu registroOrigen) {
    int valorRegistroDestino = leer_valor_en_registro(registroDestino);
    int valorRegistroOrigen = leer_valor_en_registro(registroOrigen);
    int suma = valorRegistroDestino + valorRegistroOrigen;
    escribir_valor_en_registro(registroDestino, suma);
}
void ejecutar_sub(Codigo_registros_cpu registroDestino, Codigo_registros_cpu registroOrigen) {
    int valorRegistroDestino = leer_valor_en_registro(registroDestino);
    int valorRegistroOrigen = leer_valor_en_registro(registroOrigen);
    int resta = valorRegistroDestino - valorRegistroOrigen;
    escribir_valor_en_registro(registroDestino, resta);
}
void ejectar_jnz(Codigo_registros_cpu tipoRegistro, uint32_t instruccion) {
    int valorDelRegistro = leer_valor_en_registro(tipoRegistro);
    if (valorDelRegistro != 0) {
        escribir_valor_en_registro(PC, instruccion);
    }
}
/*void ejecutar_copy_mem(Codigo_registros_cpu registroConTamanio) {
    int direccionLogicaSI = leer_valor_en_registro(SI);
    int direccionFisicaSI = traducir_direccion(direccionLogicaSI);
    int tamanio = leer_valor_en_registro(direccionFisicaSI);
    int bytes = leer_memory(direccionFisicaSI, tamanio);

    int direccionLogicaDI = leer_valor_en_registro(DI);
    int direccionFisicaDI = traducir_direccion(direccionLogicaDI);
    escribir_memory(direccionLogicaDI, bytes);
}*/
void solicitar_mutex_create(char *nombreMutex, int socketConexionScheduler, uint32_t pid) {
    t_mutex_syscall* mutexCreate;
    mutexCreate->pid = pid;
    mutexCreate->nombreMutex = nombreMutex;
    t_buffer* buffer = serializar_mutex(mutexCreate);
    t_paquete* paquete_mutex_create = crear_paquete(MUTEX_CREATE, buffer);
    enviar_paquete(socketConexionScheduler, paquete_mutex_create);
}
void solicitar_mutex_lock(char *nombreMutex, int socketConexionScheduler, uint32_t pid) {
    t_mutex_syscall* mutexLock;
    mutexLock->pid = pid;
    mutexLock->nombreMutex = nombreMutex;
    t_buffer* buffer = serializar_mutex(mutexLock);
    t_paquete* paquete_mutex_lock = crear_paquete(MUTEX_LOCK, buffer);
    enviar_paquete(socketConexionScheduler, paquete_mutex_lock); 
}
void solicitar_mutex_unlock(char *nombreMutex, int socketConexionScheduler, uint32_t pid) {
    t_mutex_syscall* mutexUnlock;
    mutexUnlock->pid = pid;
    mutexUnlock->nombreMutex = nombreMutex;
    t_buffer* buffer = serializar_mutex(mutexUnlock);
    t_paquete* paquete_mutex_unlock = crear_paquete(MUTEX_UNLOCK, buffer);
    enviar_paquete(socketConexionScheduler, paquete_mutex_unlock); 
}
void solicitar_mem_alloc(uint32_t segmentoId, uint32_t tamanio, int socketConexionScheduler, uint32_t pid) {
    t_mem_alloc* mem_alloc_struct;
    mem_alloc_struct->pid = pid;
    mem_alloc_struct->segmentoId = segmentoId;
    mem_alloc_struct->tamanio = tamanio;
    t_buffer* buffer = serializar_mem_alloc(mem_alloc_struct);
    t_paquete* paquete_mem_alloc = crear_paquete(MEM_ALLOC, buffer);
    enviar_paquete(socketConexionScheduler, paquete_mem_alloc); 
}
void solicitar_mem_free(uint32_t segmentoId, int socketConexionScheduler, uint32_t pid) {
    t_mem_free* mem_free_struct;
    mem_free_struct->segmentoId = segmentoId;
    mem_free_struct->pid = pid;
    t_buffer* buffer = serializar_mem_free(mem_free_struct);
    t_paquete* paquete_mem_free = crear_paquete(MEM_FREE, buffer);
    enviar_paquete(socketConexionScheduler, paquete_mem_free); 
}
void solicitar_sleep(uint32_t tiempo, int socketConexionScheduler, uint32_t pid) {
    t_sleep* sleep_struct;
    sleep_struct->tiempoADormir = tiempo;
    sleep_struct->pid = pid;
    t_buffer* buffer = serializar_sleep(sleep_struct);
    t_paquete* paquete_sleep = crear_paquete(SLEEP, buffer);
    enviar_paquete(socketConexionScheduler, paquete_sleep); 
}
void solicitar_stdout(Codigo_registros_cpu registroDirLogica, uint32_t tamanio, int socketConexionScheduler, uint32_t pid) {
    t_stdin_stdout* stdout_struct;
    stdout_struct->pid = pid;
    stdout_struct->bytesALeer = tamanio;
    uint32_t direccionLogica = leer_valor_en_registro(registroDirLogica); 
    stdout_struct->direccionLogica = direccionLogica;
    t_buffer* buffer = serializar_stdin(stdout_struct);
    t_paquete* paquete_stdout = crear_paquete(STDOUT, buffer);
    enviar_paquete(socketConexionScheduler, paquete_stdout); 
}
void solicitar_stdin(Codigo_registros_cpu registroDirLogica, uint32_t tamanio, int socketConexionScheduler, uint32_t pid) {
    t_stdin_stdout* stdin_struct;
    stdin_struct->pid = pid;
    stdin_struct->bytesALeer = tamanio;
    uint32_t direccionLogica = leer_valor_en_registro(registroDirLogica); 
    stdin_struct->direccionLogica = direccionLogica;
    t_buffer* buffer = serializar_stdin(stdin_struct);
    t_paquete* paquete_stdin = crear_paquete(STDIN, buffer);
    enviar_paquete(socketConexionScheduler, paquete_stdin); 
}
void solicitar_init_proc(char* pathArchivoInstrucciones, uint32_t prioridad, int socketConexionScheduler) {
    t_init_proc* init_proc_struct;
    init_proc_struct->prioridad = prioridad;
    init_proc_struct->pathArchivoInstrucciones = pathArchivoInstrucciones;
    t_buffer* buffer = serializar_init_proc(init_proc_struct);
    t_paquete* paquete_init_proc = crear_paquete(INIT_PROC, buffer);
    enviar_paquete(socketConexionScheduler, paquete_init_proc); 
}
void solicitar_exit(int socketConexionScheduler) {
    t_buffer* buffer = buffer_create(0);
    t_paquete* paquete_exit = crear_paquete(EXIT, buffer);
    enviar_paquete(socketConexionScheduler, paquete_exit); 
}

/*
traducir_direccion() {

}
escribir_memory(direccionFisica, valor) {

}
leer_memory(direccionFisica, valor) {

}
*/





void escribir_valor_en_registro(Codigo_registros_cpu tipoRegistro, int valor) {
    switch (tipoRegistro)
    {
        case PC: {
            registros_cpu.pc = valor;
            break;
        }
        case AX: {
            registros_cpu.ax = valor;
            break;
        }
        case BX: {
            registros_cpu.bx = valor;
            break;
        }
        case CX: {
            registros_cpu.cx = valor;
            break;
        }
        case DX: {
            registros_cpu.dx = valor;
            break;
        }
        case EAX: {
            registros_cpu.eax = valor;
            break;
        }
        case EBX: {
            registros_cpu.ebx = valor;
            break;
        }
        case ECX: {
            registros_cpu.ecx = valor;
            break;
        }
        case EDX: {
            registros_cpu.edx = valor;
            break;
        }
        case SI: {
            registros_cpu.si = valor;
            break;
        }
        case DI: {
            registros_cpu.di = valor;
            break;
        }
        default: {
            break;
        }
    }
}
int leer_valor_en_registro(Codigo_registros_cpu tipoRegistro) {
    int valor;
    switch (tipoRegistro)
    {
        case PC: {
            valor = registros_cpu.pc;
            break;
        }
        case AX: {
            valor = registros_cpu.ax;
            break;
        }
        case BX: {
            valor = registros_cpu.bx;
            break;
        }
        case CX: {
            valor = registros_cpu.cx;
            break;
        }
        case DX: {
            valor = registros_cpu.dx;
            break;
        }
        case EAX: {
            valor = registros_cpu.eax;
            break;
        }
        case EBX: {
            valor = registros_cpu.ebx;
            break;
        }
        case ECX: {
            valor = registros_cpu.ecx;
            break;
        }
        case EDX: {
            valor = registros_cpu.edx;
            break;
        }
        case SI: {
            valor = registros_cpu.si;
            break;
        }
        case DI: {
            valor = registros_cpu.di;
            break;
        }
        default: {
            break;
        }
    }
    return valor;
}