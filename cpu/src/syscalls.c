#include "syscalls.h"
#include "kernel_memory_avisos.h"

//INSTRUCCIONES SYSCALLS
uint32_t solicitar_mutex_create(char *nombreMutex, int socketConexionScheduler, uint32_t pid) {
    t_mutex_syscall *mutex_struct = malloc(sizeof(t_mutex_syscall));
    mutex_struct->contador = 1;
    mutex_struct->pid = pid;
//    mutex_struct->colaEspera=queue_create();
    mutex_struct->nombreMutex = nombreMutex;
    t_buffer *buffer = serializar_mutex(mutex_struct);
    t_paquete *paquete = crear_paquete(MUTEX_CREATE, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    eliminar_paquete(paquete);
    free(mutex_struct);
    return 0;
}
uint32_t solicitar_mutex_lock(char *nombreMutex, int socketConexionScheduler, uint32_t pid) {
    t_mutex_syscall *mutex_struct = malloc(sizeof(t_mutex_syscall));
    mutex_struct->pid = pid;
    mutex_struct->nombreMutex = nombreMutex;
    t_buffer *buffer = serializar_mutex(mutex_struct);
    t_paquete *paquete = crear_paquete(MUTEX_LOCK, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    eliminar_paquete(paquete);
    free(mutex_struct);
    return 0;
}
uint32_t solicitar_mutex_unlock(char *nombreMutex, int socketConexionScheduler, uint32_t pid) {
    t_mutex_syscall *mutex_struct = malloc(sizeof(t_mutex_syscall));
    mutex_struct->pid = pid;
    mutex_struct->nombreMutex = nombreMutex;
    t_buffer *buffer = serializar_mutex(mutex_struct);
    t_paquete *paquete = crear_paquete(MUTEX_UNLOCK, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    eliminar_paquete(paquete);
    free(mutex_struct);
    return 0;
}
uint32_t solicitar_mem_alloc(uint32_t segmentoId, uint32_t tamanio, int socketConexionScheduler, uint32_t pid) {
    t_mem_alloc *mem_alloc_struct = malloc(sizeof(t_mem_alloc));
    mem_alloc_struct->pid = pid;
    mem_alloc_struct->segmentoId = segmentoId;
    mem_alloc_struct->tamanio = tamanio;
    t_buffer *buffer = serializar_mem_alloc(mem_alloc_struct);
    t_paquete *paquete = crear_paquete(MEM_ALLOC, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    eliminar_paquete(paquete);
    free(mem_alloc_struct);
    return 0;
}
uint32_t solicitar_mem_free(uint32_t segmentoId, int socketConexionScheduler, uint32_t pid) {
    t_mem_free *mem_free_struct = malloc(sizeof(t_mem_free));
    mem_free_struct->segmentoId = segmentoId;
    mem_free_struct->pid = pid;
    t_buffer *buffer = serializar_mem_free(mem_free_struct);
    t_paquete *paquete = crear_paquete(MEM_FREE, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    
    eliminar_paquete(paquete);
    free(mem_free_struct);

    return 0;
}
uint32_t solicitar_sleep(uint32_t tiempo, int socketConexionScheduler, uint32_t pid) {
    t_sleep *sleep_struct = malloc(sizeof(t_sleep));
    sleep_struct->tiempoADormir = tiempo;
    sleep_struct->pid = pid;
    t_buffer *buffer = serializar_sleep(sleep_struct);
    t_paquete *paquete = crear_paquete(SLEEP, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    eliminar_paquete(paquete);
    free(sleep_struct);

    return 0;
}
uint32_t solicitar_stdout(Codigo_registros_cpu registroDirLogica, Codigo_registros_cpu registroTamanio, int socketConexionScheduler, uint32_t pid, uint32_t segment_max_size, t_list* lista_segmentos) {
    uint32_t direccion_logica = leer_valor_en_registro(registroDirLogica);
    uint32_t tamanio = leer_valor_en_registro(registroTamanio);
    pthread_mutex_lock(&mutex_lista_memory_stick);
    t_list* lista_memory_sticks = traducir_logica_a_fisica(direccion_logica, segment_max_size, lista_segmentos, lista_memory_stick, tamanio);
    pthread_mutex_unlock(&mutex_lista_memory_stick);
    if (lista_memory_sticks == NULL) {
        log_debug(loggerCpu, "DEBUG_CPU: Hubo SEG_FAULT en direccion logica: %d", direccion_logica);
        lanzar_seg_fault(socketConexionScheduler, pid);
        return COD_SEG_FAULT;
    }
    list_destroy_and_destroy_elements(lista_memory_sticks, free);

    t_stdin_stdout *stdout_struct = malloc(sizeof(t_stdin_stdout));
    stdout_struct->pid = pid;
    stdout_struct->bytesALeer = tamanio;
    stdout_struct->direccionLogica = direccion_logica;
    stdout_struct->cadenaLeida = NULL;
    t_buffer *buffer = serializar_stdin(stdout_struct);
    t_paquete *paquete = crear_paquete(STDOUT, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    eliminar_paquete(paquete);
    free(stdout_struct);

    return 0;
}
uint32_t solicitar_stdin(Codigo_registros_cpu registroDirLogica, Codigo_registros_cpu registroTamanio, int socketConexionScheduler, uint32_t pid, uint32_t segment_max_size, t_list* lista_segmentos) {
    uint32_t direccion_logica = leer_valor_en_registro(registroDirLogica);
    uint32_t tamanio = leer_valor_en_registro(registroTamanio);
    pthread_mutex_lock(&mutex_lista_memory_stick);
    t_list* lista_memory_sticks = traducir_logica_a_fisica(direccion_logica, segment_max_size, lista_segmentos, lista_memory_stick, tamanio);
    pthread_mutex_unlock(&mutex_lista_memory_stick);
    if (lista_memory_sticks == NULL) {
        log_debug(loggerCpu, "DEBUG_CPU: Hubo SEG_FAULT en direccion logica: %d", direccion_logica);
        lanzar_seg_fault(socketConexionScheduler, pid);
        return COD_SEG_FAULT;
    }
    list_destroy_and_destroy_elements(lista_memory_sticks, free);

    t_stdin_stdout *stdin_struct = malloc(sizeof(t_stdin_stdout));
    stdin_struct->pid = pid;
    stdin_struct->bytesALeer = tamanio;
    stdin_struct->direccionLogica = direccion_logica;
    stdin_struct->cadenaLeida = NULL;
    t_buffer *buffer = serializar_stdin(stdin_struct);
    t_paquete *paquete = crear_paquete(STDIN, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    eliminar_paquete(paquete);
    free(stdin_struct);

    return 0;
}
uint32_t solicitar_init_proc(char *pathArchivoInstrucciones, uint32_t prioridad, int socketConexionScheduler, uint32_t pid) {
    t_init_proc *init_proc_struct = malloc(sizeof(t_init_proc));
    init_proc_struct->pid = pid;
    init_proc_struct->prioridad = prioridad;
    init_proc_struct->pathArchivoInstrucciones = pathArchivoInstrucciones;
    t_buffer *buffer = serializar_init_proc(init_proc_struct);
    t_paquete *paquete = crear_paquete(INIT_PROC, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    eliminar_paquete(paquete);
    free(init_proc_struct);

    return 0;
}
uint32_t solicitar_exit(int socketConexionScheduler, uint32_t pid) {
    t_buffer *buffer = buffer_create(0);
    buffer_add_uint32(buffer, pid);
    t_paquete *paquete = crear_paquete(EXIT, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    eliminar_paquete(paquete);

    return 0;
}