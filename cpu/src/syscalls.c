#include "syscalls.h"  

//INSTRUCCIONES SYSCALLS
void solicitar_mutex_create(char *nombreMutex, int socketConexionScheduler, uint32_t pid) {
    t_mutex_syscall *mutex_struct = malloc(sizeof(t_mutex_syscall));
    mutex_struct->pid = pid;
    mutex_struct->colaEspera=NULL;
    mutex_struct->nombreMutex = nombreMutex;
    t_buffer *buffer = serializar_mutex(mutex_struct);
    t_paquete *paquete = crear_paquete(MUTEX_CREATE, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    eliminar_paquete(paquete);
    free(mutex_struct);
}
void solicitar_mutex_lock(char *nombreMutex, int socketConexionScheduler, uint32_t pid) {
    t_mutex_syscall *mutex_struct = malloc(sizeof(t_mutex_syscall));
    mutex_struct->pid = pid;
    mutex_struct->colaEspera=NULL;
    mutex_struct->nombreMutex = nombreMutex;
    t_buffer *buffer = serializar_mutex(mutex_struct);
    t_paquete *paquete = crear_paquete(MUTEX_LOCK, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    eliminar_paquete(paquete);
    free(mutex_struct);
}
void solicitar_mutex_unlock(char *nombreMutex, int socketConexionScheduler, uint32_t pid) {
    t_mutex_syscall *mutex_struct = malloc(sizeof(t_mutex_syscall));
    mutex_struct->pid = pid;
    mutex_struct->colaEspera=NULL;
    mutex_struct->nombreMutex = nombreMutex;
    t_buffer *buffer = serializar_mutex(mutex_struct);
    t_paquete *paquete = crear_paquete(MUTEX_UNLOCK, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    eliminar_paquete(paquete);
    free(mutex_struct);
}
void solicitar_mem_alloc(uint32_t segmentoId, uint32_t tamanio, int socketConexionScheduler, uint32_t pid) {
    t_mem_alloc *mem_alloc_struct = malloc(sizeof(t_mem_alloc));
    mem_alloc_struct->pid = pid;
    mem_alloc_struct->segmentoId = segmentoId;
    mem_alloc_struct->tamanio = tamanio;
    t_buffer *buffer = serializar_mem_alloc(mem_alloc_struct);
    t_paquete *paquete = crear_paquete(MEM_ALLOC, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    eliminar_paquete(paquete);
    free(mem_alloc_struct);
}
void solicitar_mem_free(uint32_t segmentoId, int socketConexionScheduler, uint32_t pid) {
    t_mem_free *mem_free_struct = malloc(sizeof(t_mem_free));
    mem_free_struct->segmentoId = segmentoId;
    mem_free_struct->pid = pid;
    t_buffer *buffer = serializar_mem_free(mem_free_struct);
    t_paquete *paquete = crear_paquete(MEM_FREE, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    //agregado por marotti->
    uint32_t ok;
    recv(socketConexionScheduler, &ok, sizeof(uint32_t),MSG_WAITALL);
    if(ok!=1){
        log_error(loggerCpu, "no se pudo hacer el MEM_FREE");
    }
    eliminar_paquete(paquete);
    free(mem_free_struct);
}
void solicitar_sleep(uint32_t tiempo, int socketConexionScheduler, uint32_t pid) {
    t_sleep *sleep_struct = malloc(sizeof(t_sleep));
    sleep_struct->tiempoADormir = tiempo;
    sleep_struct->pid = pid;
    t_buffer *buffer = serializar_sleep(sleep_struct);
    t_paquete *paquete = crear_paquete(SLEEP, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    eliminar_paquete(paquete);
    free(sleep_struct);
}
void solicitar_stdout(Codigo_registros_cpu registroDirLogica, Codigo_registros_cpu registroTamanio, int socketConexionScheduler, uint32_t pid) {
    t_stdin_stdout *stdout_struct = malloc(sizeof(t_stdin_stdout));
    stdout_struct->pid = pid;
    stdout_struct->bytesALeer = leer_valor_en_registro(registroTamanio);
    stdout_struct->direccionLogica = leer_valor_en_registro(registroDirLogica);
    stdout_struct->cadenaLeida = NULL;
    t_buffer *buffer = serializar_stdin(stdout_struct);
    t_paquete *paquete = crear_paquete(STDOUT, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    eliminar_paquete(paquete);
    free(stdout_struct);
}
void solicitar_stdin(Codigo_registros_cpu registroDirLogica, Codigo_registros_cpu registroTamanio, int socketConexionScheduler, uint32_t pid) {
    t_stdin_stdout *stdin_struct = malloc(sizeof(t_stdin_stdout));
    stdin_struct->pid = pid;
    stdin_struct->bytesALeer = leer_valor_en_registro(registroTamanio);
    stdin_struct->direccionLogica = leer_valor_en_registro(registroDirLogica);
    stdin_struct->cadenaLeida = NULL;
    t_buffer *buffer = serializar_stdin(stdin_struct);
    t_paquete *paquete = crear_paquete(STDIN, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    eliminar_paquete(paquete);
    free(stdin_struct);
}
void solicitar_init_proc(char *pathArchivoInstrucciones, uint32_t prioridad, int socketConexionScheduler) {
    t_init_proc *init_proc_struct = malloc(sizeof(t_init_proc));
    init_proc_struct->prioridad = prioridad;
    init_proc_struct->pathArchivoInstrucciones = pathArchivoInstrucciones;
    t_buffer *buffer = serializar_init_proc(init_proc_struct);
    t_paquete *paquete = crear_paquete(INIT_PROC, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    //agregado por marotti->
    /*uint32_t ok;
    recv(socketConexionScheduler, &ok, sizeof(uint32_t),MSG_WAITALL);
    if(ok!=1){
        log_error(loggerCpu, "no se pudo crear el proceso");
    }*/
    eliminar_paquete(paquete);
    free(init_proc_struct);
}
void solicitar_exit(int socketConexionScheduler, uint32_t pid) {
    t_buffer *buffer = buffer_create(0);
    buffer_add_uint32(buffer, pid);
    t_paquete *paquete = crear_paquete(EXIT, buffer);
    enviar_paquete(socketConexionScheduler, paquete);
    eliminar_paquete(paquete);
}