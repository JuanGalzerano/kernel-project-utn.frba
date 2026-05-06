#include "ciclo.h"
#include "instrucciones.h"
#include "syscalls.h"
#include "inicializar_cpu.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
  
int ejecutar_ciclo_de_instruccion(int socketConexionMemory, int socketConexionScheduler, uint32_t pid) {
    uint32_t pc = registros_cpu.pc;
    log_info(loggerCpu, "## PID: %d - FETCH - Program Counter: %d", pid, pc);

    op_code opCode = OBTENER_INSTRUCCION;
    send(socketConexionMemory, &opCode, sizeof(op_code), 0);
    send(socketConexionMemory, &pc, sizeof(uint32_t), 0);

    t_paquete *paquete = recibir_paquete(socketConexionMemory);
    uint32_t longitud = paquete->buffer->size;
    char *inicioInstruccion = buffer_read_string(paquete->buffer, longitud);
    char *cursor = inicioInstruccion;
    eliminar_paquete(paquete);

    op_code tipoInstruccion = obtener_instruccion_registro_valor(&cursor);
    if (tipoInstruccion < 0) {
        free(inicioInstruccion);
        return -1;
    }

    uint32_t pc_antes = registros_cpu.pc;
    int errorDecode = decode(tipoInstruccion, &cursor, socketConexionScheduler, pid);
    free(inicioInstruccion);

    if (errorDecode == 1) return 1;
    if (errorDecode == 0) {
        if (registros_cpu.pc == pc_antes) registros_cpu.pc++;
        return 0;
    }
    return -1;
}

//Basicamente me da los "tokens validos"
int obtener_instruccion_registro_valor(char **string) {
    int i = 0;
    while ((*string)[i] != ' ' && (*string)[i] != '\0') i++;

    char *token = malloc(i + 1);
    if (token == NULL) return -1;
    strncpy(token, *string, i);
    token[i] = '\0';

    if ((*string)[i] == ' ') *string += i + 1;

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

    free(token);
    return resultado;
}

char *obtener_nombre_mutex_o_path(char **string) {
    int i = 0;
    while ((*string)[i] == ' ') i++;
    int start = i;
    while ((*string)[i] != ' ' && (*string)[i] != '\0') i++;

    int len = i - start;
    char *resultado = malloc(len + 1);
    strncpy(resultado, (*string) + start, len);
    resultado[len] = '\0';

    if ((*string)[i] == ' ') *string += i + 1;
    else *string += i;

    return resultado;
}

int decode(op_code tipoInstruccion, char **string, int socketConexionScheduler, uint32_t pid) {
    Codigo_registros_cpu tipoRegistro, registroDestino, registroOrigen, registroDirLogica, registroTamanio;
    uint32_t segmentoId, valor;

    switch (tipoInstruccion) {
    case NOOP:
        log_info(loggerCpu, "## PID: %d - Ejecutando: NOOP", pid);
        break;  
    case SET:
        tipoRegistro = obtener_instruccion_registro_valor(string);
        valor = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: SET - %s %d", pid, registro_a_string(tipoRegistro), valor);
        ejecutar_set(tipoRegistro, valor);
        break;
    case MOV_IN:
        tipoRegistro = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: MOV_IN - %s", pid, registro_a_string(tipoRegistro));
        break;
    case MOV_OUT:
        tipoRegistro = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: MOV_OUT - %s", pid, registro_a_string(tipoRegistro));
        break;
    case SUM:  
        registroDestino = obtener_instruccion_registro_valor(string);
        registroOrigen = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: SUM - %s %s", pid, registro_a_string(registroDestino), registro_a_string(registroOrigen));
        ejecutar_sum(registroDestino, registroOrigen);
        break;
    case SUB:
        registroDestino = obtener_instruccion_registro_valor(string);
        registroOrigen = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: SUB - %s %s", pid, registro_a_string(registroDestino), registro_a_string(registroOrigen));
        ejecutar_sub(registroDestino, registroOrigen);
        break;
    case JNZ: {  
        tipoRegistro = obtener_instruccion_registro_valor(string);
        uint32_t instruc = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: JNZ - %s %d", pid, registro_a_string(tipoRegistro), instruc);
        ejectar_jnz(tipoRegistro, instruc);
        break;
    }
    case COPY_MEM:
        tipoRegistro = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: COPY_MEM - %s", pid, registro_a_string(tipoRegistro));
        break;
    case MUTEX_CREATE: {
        char *nombre = obtener_nombre_mutex_o_path(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: MUTEX_CREATE - %s", pid, nombre);
        solicitar_mutex_create(nombre, socketConexionScheduler, pid);
        free(nombre);
        return 1;
    }
    case MUTEX_LOCK: {
        char *nombre = obtener_nombre_mutex_o_path(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: MUTEX_LOCK - %s", pid, nombre);
        solicitar_mutex_lock(nombre, socketConexionScheduler, pid);
        free(nombre);
        return 1;
    }
    case MUTEX_UNLOCK: {
        char *nombre = obtener_nombre_mutex_o_path(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: MUTEX_UNLOCK - %s", pid, nombre);
        solicitar_mutex_unlock(nombre, socketConexionScheduler, pid);
        free(nombre);
        return 1;
    }
    case MEM_ALLOC: {  
        segmentoId = obtener_instruccion_registro_valor(string);
        uint32_t tam = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: MEM_ALLOC - %d %d", pid, segmentoId, tam);
        solicitar_mem_alloc(segmentoId, tam, socketConexionScheduler, pid);
        return 1;
    }
    case MEM_FREE:
        segmentoId = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: MEM_FREE - %d", pid, segmentoId);
        solicitar_mem_free(segmentoId, socketConexionScheduler, pid);
        return 1;
    case SLEEP: {
        uint32_t tiempo = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: SLEEP - %d", pid, tiempo);
        solicitar_sleep(tiempo, socketConexionScheduler, pid);
        return 1;
    }
    case STDOUT:  
        registroDirLogica = obtener_instruccion_registro_valor(string);
        registroTamanio = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: STDOUT - %s %s", pid, registro_a_string(registroDirLogica), registro_a_string(registroTamanio));
        solicitar_stdout(registroDirLogica, registroTamanio, socketConexionScheduler, pid);
        return 1;
    case STDIN:
        registroDirLogica = obtener_instruccion_registro_valor(string);
        registroTamanio = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: STDIN - %s %s", pid, registro_a_string(registroDirLogica), registro_a_string(registroTamanio));
        solicitar_stdin(registroDirLogica, registroTamanio, socketConexionScheduler, pid);
        return 1;
    case INIT_PROC: {  
        char *path = obtener_nombre_mutex_o_path(string);
        int prioridad = obtener_instruccion_registro_valor(string);
        log_info(loggerCpu, "## PID: %d - Ejecutando: INIT_PROC - %s %d", pid, path, prioridad);
        solicitar_init_proc(path, prioridad, socketConexionScheduler);
        free(path);
        return 1;
    }
    case EXIT:
        log_info(loggerCpu, "## PID: %d - Ejecutando: EXIT", pid);
        solicitar_exit(socketConexionScheduler);
        return 1;
    default:
        log_info(loggerCpu, "## PID: %d - Instruccion NO RECONOCIDA", pid);
        return -1;
    }
    return 0;
}