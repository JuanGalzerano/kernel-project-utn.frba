#include "ciclo.h"
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

    t_buffer* buf_instr = buffer_create(0);
    buffer_add_uint32(buf_instr, pid);
    buffer_add_uint32(buf_instr, registros_cpu.pc);
    t_paquete* req_instr = crear_paquete(OBTENER_INSTRUCCION, buf_instr);
    enviar_paquete(socketConexionMemory, req_instr);
    eliminar_paquete(req_instr);

    t_paquete *paquete = recibir_paquete(socketConexionMemory);
    uint32_t longitud = paquete->buffer->size;
    char *inicioInstruccion = buffer_read_string(paquete->buffer, longitud);
    eliminar_paquete(paquete);

    char *nombreInstruccion = strtok_r(inicioInstruccion, " ", &cursor);
    op_code tipoInstruccion = interpretar_token(nombreInstruccion);
    if (tipoInstruccion < 0) {
        free(inicioInstruccion);
        return -1;
    }

    uint32_t pc_antes = registros_cpu.pc;
    int errorDecode = decode(tipoInstruccion, socketConexionScheduler, pid, segment_max_size, lista_segmentos);
    free(inicioInstruccion);

    if (errorDecode == 1) {
        registros_cpu.pc++;//agregado por marotti pq si venia una syscall no se incrementaba el PC
        return 1;
    }
    if (errorDecode == 0) {
        if (registros_cpu.pc == pc_antes) registros_cpu.pc++;
        return 0;
    }
    return -1;
}

//Basicamente me da los "tokens validos"
int interpretar_token(char *token) {
     int resultado = -1;

    if (isdigit((unsigned char)token[0])) {
        resultado = atoi(token);
    }

    if (resultado == -1) {
        for (int j = 0; instrucciones[j].nombreDeLaInstruccion != NULL; j++) {
            if (strcmp(token, instrucciones[j].nombreDeLaInstruccion) == 0) {
                resultado = instrucciones[j].instruccion_codigo;
                break;
            }
        }
    }

    if (resultado == -1) {
        for (int k = 0; registros[k].nombreDelRegistro != NULL; k++) {
            if (strcmp(token, registros[k].nombreDelRegistro) == 0) {
                resultado = registros[k].codigo_registros_cpu;
                break;
            }
        }
    }

    return resultado;
}

int decode(op_code tipoInstruccion, int socketConexionScheduler, uint32_t pid, uint32_t segment_max_size, t_list* lista_segmentos) {
    Codigo_registros_cpu tipoRegistro, registroDestino, registroOrigen, registroDirLogica, registroTamanio;
    uint32_t segmentoId, valor;

    switch (tipoInstruccion) {
    case NOOP:
        log_info(loggerCpu, "## PID: %d - Ejecutando: NOOP", pid);
        break;  
    case SET:
        tipoRegistro = interpretar_token(strtok_r(NULL, " ", &cursor));
        valor = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: SET - %s %d", pid, registro_a_string(tipoRegistro), valor);
        ejecutar_set(tipoRegistro, valor);
        break;
    case MOV_IN:
        tipoRegistro = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: MOV_IN - %s", pid, registro_a_string(tipoRegistro));
        ejecutar_mov_in(socketConexionScheduler, pid, tipoRegistro, segment_max_size, lista_segmentos);
        break;
    case MOV_OUT:
        tipoRegistro = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: MOV_OUT - %s", pid, registro_a_string(tipoRegistro));
        ejecutar_mov_out(socketConexionScheduler, pid, tipoRegistro, segment_max_size, lista_segmentos);
        break;
    case SUM:  
        registroDestino = interpretar_token(strtok_r(NULL, " ", &cursor));
        registroOrigen = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: SUM - %s %s", pid, registro_a_string(registroDestino), registro_a_string(registroOrigen));
        ejecutar_sum(registroDestino, registroOrigen);
        break;
    case SUB:
        registroDestino = interpretar_token(strtok_r(NULL, " ", &cursor));
        registroOrigen = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: SUB - %s %s", pid, registro_a_string(registroDestino), registro_a_string(registroOrigen));
        ejecutar_sub(registroDestino, registroOrigen);
        break;
    case JNZ: {  
        tipoRegistro = interpretar_token(strtok_r(NULL, " ", &cursor));
        uint32_t instruccion = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: JNZ - %s %d", pid, registro_a_string(tipoRegistro), instruccion);
        ejecutar_jnz(tipoRegistro, instruccion);
        break;
    }
    case COPY_MEM:
        tipoRegistro = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: COPY_MEM - %s", pid, registro_a_string(tipoRegistro));
        break;
    case MUTEX_CREATE: {
        char *nombre = strtok_r(NULL, " ", &cursor);
        log_info(loggerCpu, "## PID: %d - Ejecutando: MUTEX_CREATE - %s", pid, nombre);
        solicitar_mutex_create(nombre, socketConexionScheduler, pid);
        return 1;
    }
    case MUTEX_LOCK: {
        char *nombre = strtok_r(NULL, " ", &cursor);
        log_info(loggerCpu, "## PID: %d - Ejecutando: MUTEX_LOCK - %s", pid, nombre);
        solicitar_mutex_lock(nombre, socketConexionScheduler, pid);
        return 1;
    }
    case MUTEX_UNLOCK: {
        char *nombre = strtok_r(NULL, " ", &cursor);
        log_info(loggerCpu, "## PID: %d - Ejecutando: MUTEX_UNLOCK - %s", pid, nombre);
        solicitar_mutex_unlock(nombre, socketConexionScheduler, pid);
        return 1;
    }
    case MEM_ALLOC: {  
        segmentoId = interpretar_token(strtok_r(NULL, " ", &cursor));
        uint32_t tam = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: MEM_ALLOC - %d %d", pid, segmentoId, tam);
        solicitar_mem_alloc(segmentoId, tam, socketConexionScheduler, pid);
        return 1;
    }
    case MEM_FREE:
        segmentoId = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: MEM_FREE - %d", pid, segmentoId);
        solicitar_mem_free(segmentoId, socketConexionScheduler, pid);
        return 1;
    case SLEEP: {
        uint32_t tiempo = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: SLEEP - %d", pid, tiempo);
        solicitar_sleep(tiempo, socketConexionScheduler, pid);
        return 1;
    }
    case STDOUT:  
        registroDirLogica = interpretar_token(strtok_r(NULL, " ", &cursor));
        registroTamanio = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: STDOUT - %s %s", pid, registro_a_string(registroDirLogica), registro_a_string(registroTamanio));
        solicitar_stdout(registroDirLogica, registroTamanio, socketConexionScheduler, pid);
        return 1;
    case STDIN:
        registroDirLogica = interpretar_token(strtok_r(NULL, " ", &cursor));
        registroTamanio = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: STDIN - %s %s", pid, registro_a_string(registroDirLogica), registro_a_string(registroTamanio));
        solicitar_stdin(registroDirLogica, registroTamanio, socketConexionScheduler, pid);
        return 1;
    case INIT_PROC: {  
        char *path = strtok_r(NULL, " ", &cursor);
        int prioridad = interpretar_token(strtok_r(NULL, " ", &cursor));
        log_info(loggerCpu, "## PID: %d - Ejecutando: INIT_PROC - %s %d", pid, path, prioridad);
        solicitar_init_proc(path, prioridad, socketConexionScheduler, pid);
        return 1;
    }
    case EXIT:
        log_info(loggerCpu, "## PID: %d - Ejecutando: EXIT", pid);
        solicitar_exit(socketConexionScheduler, pid);
        return 1;
    default:
        log_info(loggerCpu, "## PID: %d - Instruccion NO RECONOCIDA", pid);
        return -1;
    }
    return 0;
}