#include "ciclo.h"
#include "auxiliares_ciclo.h"
#include "instrucciones.h"
#include "syscalls.h"
#include "inicializar_cpu.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

//Variable para strtok_r
char *cursor;
  
int ejecutar_ciclo_de_instruccion(int socketConexionMemory, int socketConexionScheduler, uint32_t pid, uint32_t segment_max_size, t_list* lista_segmentos) {
    uint32_t pc = registros_cpu.pc;
    log_info(loggerCpu, "## PID: %d - FETCH - Program Counter: %d", pid, pc);

    log_info(loggerCpu, "CPU: Solicitando linea con instruccion a Kernel Memory");
    char *inicioInstruccion = obtener_linea_con_instruccion(pid, pc, socketConexionMemory);
    if (inicioInstruccion == NULL) {
        log_info(loggerCpu, "CPU: Error al obtener linea con instruccion de Kernel Memory");
        return -1;
    }
    log_info(loggerCpu, "CPU: Obtuve linea con instruccion: %s", inicioInstruccion);

    char *nombreInstruccion = strtok_r(inicioInstruccion, " ", &cursor);
    op_code tipoInstruccion = interpretar_token(nombreInstruccion);
    if (tipoInstruccion < 0) {
        free(inicioInstruccion);
        return -1;
    }

    uint32_t pc_antes = pc;
    log_debug(loggerCpu, "DEBUG_CPU: PC viejo: %d", pc_antes);
    log_debug(loggerCpu, "DEBUG_CPU: Inicio del decode");
    int errorDecode = decode(tipoInstruccion, socketConexionScheduler, pid, segment_max_size, lista_segmentos);
    free(inicioInstruccion);

    if (errorDecode == COD_SEG_FAULT) {
        return COD_SEG_FAULT;
    }
    if (errorDecode == COD_EXIT) {
        return COD_EXIT;
    }
    if (errorDecode == 1) {
        if (registros_cpu.pc == pc_antes) {
            registros_cpu.pc++;
            log_debug(loggerCpu, "DEBUG_CPU: Incremento pc de %d a %d", pc_antes, registros_cpu.pc);
        }
        return 1;
    }
    if (errorDecode == 0) {
        if (registros_cpu.pc == pc_antes) {
            registros_cpu.pc++;
            log_debug(loggerCpu, "DEBUG_CPU: Incremento pc de %d a %d", pc_antes, registros_cpu.pc);
        }
        return 0;
    }
    return -1;
}

int decode(op_code tipoInstruccion, int socketConexionScheduler, uint32_t pid, uint32_t segment_max_size, t_list* lista_segmentos) {
    Codigo_registros_cpu tipoRegistro, registroDestino, registroOrigen, registroDirLogica, registroTamanio;
    uint32_t segmentoId, valor;
    uint32_t control_error = 0;

    switch (tipoInstruccion) {
    case NOOP:
        log_info(loggerCpu, "## PID: %d - Ejecutando: NOOP", pid);
        break;  
    case SET:
        tipoRegistro = interpretar_token(strtok_r(NULL, " ", &cursor));
        valor = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: SET - %s %d", pid, registro_a_string(tipoRegistro), valor);
        control_error = ejecutar_set(tipoRegistro, valor);
        break;
    case MOV_IN:
        tipoRegistro = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: MOV_IN - %s", pid, registro_a_string(tipoRegistro));
        control_error = ejecutar_mov_in(socketConexionScheduler, pid, tipoRegistro, segment_max_size, lista_segmentos);
        break;
    case MOV_OUT:
        tipoRegistro = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: MOV_OUT - %s", pid, registro_a_string(tipoRegistro));
        control_error = ejecutar_mov_out(socketConexionScheduler, pid, tipoRegistro, segment_max_size, lista_segmentos);
        break;
    case SUM:  
        registroDestino = interpretar_token(strtok_r(NULL, " ", &cursor));
        registroOrigen = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: SUM - %s %s", pid, registro_a_string(registroDestino), registro_a_string(registroOrigen));
        control_error = ejecutar_sum(registroDestino, registroOrigen);
        break;
    case SUB:
        registroDestino = interpretar_token(strtok_r(NULL, " ", &cursor));
        registroOrigen = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: SUB - %s %s", pid, registro_a_string(registroDestino), registro_a_string(registroOrigen));
        control_error = ejecutar_sub(registroDestino, registroOrigen);
        break;
    case JNZ: {  
        tipoRegistro = interpretar_token(strtok_r(NULL, " ", &cursor));
        uint32_t instruccion = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: JNZ - %s %d", pid, registro_a_string(tipoRegistro), instruccion);
        control_error = ejecutar_jnz(tipoRegistro, instruccion);
        break;
    }
    case COPY_MEM:
        tipoRegistro = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: COPY_MEM - %s", pid, registro_a_string(tipoRegistro));
        control_error = ejecutar_copy_mem(socketConexionScheduler, pid, tipoRegistro, segment_max_size, lista_segmentos);
        break;
    case MUTEX_CREATE: {
        char *nombre = strtok_r(NULL, " ", &cursor);
        log_info(loggerCpu, "## PID: %d - Ejecutando: MUTEX_CREATE - %s", pid, nombre);
        control_error = solicitar_mutex_create(nombre, socketConexionScheduler, pid);
        return 1;
    }
    case MUTEX_LOCK: {
        char *nombre = strtok_r(NULL, " ", &cursor);
        log_info(loggerCpu, "## PID: %d - Ejecutando: MUTEX_LOCK - %s", pid, nombre);
        control_error = solicitar_mutex_lock(nombre, socketConexionScheduler, pid);
        return 1;
    }
    case MUTEX_UNLOCK: {
        char *nombre = strtok_r(NULL, " ", &cursor);
        log_info(loggerCpu, "## PID: %d - Ejecutando: MUTEX_UNLOCK - %s", pid, nombre);
        control_error = solicitar_mutex_unlock(nombre, socketConexionScheduler, pid);
        return 1;
    }
    case MEM_ALLOC: {  
        segmentoId = interpretar_token(strtok_r(NULL, " ", &cursor));
        uint32_t tam = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: MEM_ALLOC - %d %d", pid, segmentoId, tam);
        control_error = solicitar_mem_alloc(segmentoId, tam, socketConexionScheduler, pid);
        return 1;
    }
    case MEM_FREE:
        segmentoId = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: MEM_FREE - %d", pid, segmentoId);
        control_error = solicitar_mem_free(segmentoId, socketConexionScheduler, pid);
        return 1;
    case SLEEP: {
        uint32_t tiempo = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: SLEEP - %d", pid, tiempo);
        control_error = solicitar_sleep(tiempo, socketConexionScheduler, pid);
        return 1;
    }
    case STDOUT:
        registroDirLogica = interpretar_token(strtok_r(NULL, " ", &cursor));
        registroTamanio = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: STDOUT - %s %s", pid, registro_a_string(registroDirLogica), registro_a_string(registroTamanio));
        control_error = solicitar_stdout(registroDirLogica, registroTamanio, socketConexionScheduler, pid, segment_max_size, lista_segmentos);
        if (control_error == COD_SEG_FAULT) return COD_SEG_FAULT;
        return 1;
    case STDIN:
        registroDirLogica = interpretar_token(strtok_r(NULL, " ", &cursor));
        registroTamanio = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: STDIN - %s %s", pid, registro_a_string(registroDirLogica), registro_a_string(registroTamanio));
        control_error = solicitar_stdin(registroDirLogica, registroTamanio, socketConexionScheduler, pid, segment_max_size, lista_segmentos);
        if (control_error == COD_SEG_FAULT) return COD_SEG_FAULT;
        return 1;
    case INIT_PROC: {  
        char *path = strtok_r(NULL, " ", &cursor);
        int prioridad = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: INIT_PROC - %s %d", pid, path, prioridad);
        control_error = solicitar_init_proc(path, prioridad, socketConexionScheduler, pid);
        return 1;
    }
    case EXIT:
        log_info(loggerCpu, "## PID: %d - Ejecutando: EXIT", pid);
        control_error = solicitar_exit(socketConexionScheduler, pid);
        return COD_EXIT;
    default:
        log_info(loggerCpu, "## PID: %d - Instruccion NO RECONOCIDA", pid);
        return -1;
    }
    
    if (control_error == COD_SEG_FAULT) {
        return COD_SEG_FAULT;
    }
    if (control_error == 1) {
        log_info(loggerCpu, "CPU: Error al ejecutar instruccion");
        return -1;
    }
    return 0;
}