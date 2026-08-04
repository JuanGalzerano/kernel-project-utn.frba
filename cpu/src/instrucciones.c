#include "instrucciones.h"
#include <utils/mmu.h>
#include "kernel_memory_avisos.h"

Registros_cpu registros_cpu;

Registro registros[] = {
    {"PC", PC}, {"AX", AX}, {"BX", BX}, {"CX", CX},
    {"DX", DX}, {"EAX", EAX}, {"EBX", EBX}, {"ECX", ECX},
    {"EDX", EDX}, {"SI", SI}, {"DI", DI},
    {NULL, 0}
};

Instruccion instrucciones[] = {
    {"NOOP", NOOP}, {"SET", SET}, {"MOV_IN", MOV_IN}, {"MOV_OUT", MOV_OUT},
    {"SUM", SUM}, {"SUB", SUB}, {"JNZ", JNZ}, {"COPY_MEM", COPY_MEM},
    {"MUTEX_CREATE", MUTEX_CREATE}, {"MUTEX_LOCK", MUTEX_LOCK}, {"MUTEX_UNLOCK", MUTEX_UNLOCK},
    {"MEM_ALLOC", MEM_ALLOC}, {"MEM_FREE", MEM_FREE}, {"SLEEP", SLEEP}, {"STDOUT", STDOUT},
    {"STDIN", STDIN}, {"INIT_PROC", INIT_PROC}, {"EXIT", EXIT},
    {NULL, 0}
};

//INSTRUCCIONES QUE YO EJECUTO
uint32_t ejecutar_set(Codigo_registros_cpu tipoRegistro, uint32_t valor) {
    log_debug(loggerCpu, "DEBUG_CPU: Iniciando ejecucion SET");
    escribir_valor_en_registro(tipoRegistro, valor);
    if (valor == leer_valor_en_registro(tipoRegistro)) {
        log_debug(loggerCpu, "DEBUG_CPU: SET exitoso");
        return 0;
    } else {
        log_debug(loggerCpu, "DEBUG_CPU: SET no exitoso");
        return 1;
    }
}
uint32_t ejecutar_mov_in(int socketConexionScheduler, uint32_t pid, Codigo_registros_cpu tipoRegistro, uint32_t segment_max_size, t_list* lista_segmentos) {
    log_debug(loggerCpu, "DEBUG_CPU: Iniciando ejecucion MOV_IN");
    uint32_t direccion_logica = leer_valor_en_registro(SI);
    uint32_t tamanio_dato;
    if (tipoRegistro > 4) {
        log_debug(loggerCpu, "DEBUG_CPU: Se leera un dato de 4 bytes");
        tamanio_dato = sizeof(uint32_t);
    } else {
        log_debug(loggerCpu, "DEBUG_CPU: Se leera un dato de 1 byte");
        tamanio_dato = sizeof(uint8_t);
    }

    pthread_mutex_lock(&mutex_lista_memory_stick);
    t_list* lista_memory_sticks = traducir_logica_a_fisica(direccion_logica, segment_max_size, lista_segmentos, lista_memory_stick, tamanio_dato);
    pthread_mutex_unlock(&mutex_lista_memory_stick);
    if (lista_memory_sticks == NULL) {
        log_debug(loggerCpu, "DEBUG_CPU: Hubo SEG_FAULT en direccion logica: %d", direccion_logica);
        lanzar_seg_fault(socketConexionScheduler, pid);
        return COD_SEG_FAULT;
    }

    log_debug(loggerCpu, "DEBUG_CPU: Inicio lectura en memoria");
    t_resultado_lectura* resultado = leer_en_memoria(lista_memory_sticks, pid, loggerCpu);
    list_destroy_and_destroy_elements(lista_memory_sticks, free);
    if (resultado != NULL) {
        interpretar_y_escribir_dato_en_registro(tipoRegistro, resultado->dato, resultado->dir_fisica, pid);
        free(resultado);
        return 0;
    }

    log_debug(loggerCpu, "DEBUG_CPU: Fallo la lectura en Memory Stick (MOV_IN), lanzo SEG_FAULT");
    lanzar_seg_fault(socketConexionScheduler, pid);
    return COD_SEG_FAULT;
}
uint32_t ejecutar_mov_out(int socketConexionScheduler, uint32_t pid, Codigo_registros_cpu tipoRegistro, uint32_t segment_max_size, t_list* lista_segmentos) {
    log_debug(loggerCpu, "DEBUG_CPU: Iniciando ejecucion MOV_OUT");
    uint32_t direccion_logica = leer_valor_en_registro(DI);
    uint32_t valor_del_registro = leer_valor_en_registro(tipoRegistro);
    uint32_t tamanio_dato;
    if (tipoRegistro > 4) {
        log_debug(loggerCpu, "DEBUG_CPU: Se escribira un dato de 4 bytes");
        tamanio_dato = sizeof(uint32_t);
    } else {
        log_debug(loggerCpu, "DEBUG_CPU: Se escribira un dato de 1 byte");
        tamanio_dato = sizeof(uint8_t);
    }

    pthread_mutex_lock(&mutex_lista_memory_stick);
    t_list* lista_memory_sticks = traducir_logica_a_fisica(direccion_logica, segment_max_size, lista_segmentos, lista_memory_stick, tamanio_dato);
    pthread_mutex_unlock(&mutex_lista_memory_stick);
    if (lista_memory_sticks == NULL) {
        log_debug(loggerCpu, "DEBUG_CPU: Hubo SEG_FAULT en direccion logica: %d", direccion_logica);
        lanzar_seg_fault(socketConexionScheduler, pid);
        return COD_SEG_FAULT;
    }

    log_debug(loggerCpu, "DEBUG_CPU: Inicio escritura en memoria");
    int primera_dir_fisica = escribir_en_memoria(lista_memory_sticks, (char*)&valor_del_registro, pid, loggerCpu);
    list_destroy_and_destroy_elements(lista_memory_sticks, free);
    if (primera_dir_fisica == -1) {
        log_debug(loggerCpu, "DEBUG_CPU: Hubo SEG_FAULT se desconecto memory stick");
        lanzar_seg_fault(socketConexionScheduler, pid);
        return COD_SEG_FAULT;
    }

    log_info(loggerCpu, "## PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %d", pid, primera_dir_fisica, valor_del_registro);

    return 0;
}
uint32_t ejecutar_sum(Codigo_registros_cpu registroDestino, Codigo_registros_cpu registroOrigen) {
    log_debug(loggerCpu, "DEBUG_CPU: Iniciando ejecucion SUM");
    uint32_t valorDestino = leer_valor_en_registro(registroDestino);
    uint32_t valorOrigen = leer_valor_en_registro(registroOrigen);
    escribir_valor_en_registro(registroDestino, valorDestino + valorOrigen);
    log_debug(loggerCpu, "DEBUG_CPU: Se escribio: %d en el registro %s", valorDestino + valorOrigen, registro_a_string(registroDestino));
    return 0;
}
uint32_t ejecutar_sub(Codigo_registros_cpu registroDestino, Codigo_registros_cpu registroOrigen) {
    log_debug(loggerCpu, "DEBUG_CPU: Iniciando ejecucion SUB");
    uint32_t valorDestino = leer_valor_en_registro(registroDestino);
    uint32_t valorOrigen = leer_valor_en_registro(registroOrigen);
    escribir_valor_en_registro(registroDestino, valorDestino - valorOrigen);
    log_debug(loggerCpu, "DEBUG_CPU: Se escribio: %d en el registro %s", valorDestino - valorOrigen, registro_a_string(registroDestino));
    return 0;
}
uint32_t ejecutar_jnz(Codigo_registros_cpu tipoRegistro, uint32_t instruccion) {
    log_debug(loggerCpu, "DEBUG_CPU: Iniciando ejecucion JNZ");
    if (leer_valor_en_registro(tipoRegistro) != 0) {
        escribir_valor_en_registro(PC, instruccion);
        log_debug(loggerCpu, "DEBUG_CPU: JNZ salta, PC actualizado a: %d", instruccion);
    } else {
        log_debug(loggerCpu, "DEBUG_CPU: JNZ no salta, registro en 0");
    }
    return 0;
}
uint32_t ejecutar_copy_mem(int socketConexionScheduler, uint32_t pid, Codigo_registros_cpu tipoRegistro, uint32_t segment_max_size, t_list* lista_segmentos) {
    log_debug(loggerCpu, "DEBUG_CPU: Iniciando ejecucion COPY_MEM");
    uint32_t direccion_logica_si = leer_valor_en_registro(SI);
    uint32_t tamanio_dato = leer_valor_en_registro(tipoRegistro);

    pthread_mutex_lock(&mutex_lista_memory_stick);
    t_list* lista_memory_sticks = traducir_logica_a_fisica(direccion_logica_si, segment_max_size, lista_segmentos, lista_memory_stick, tamanio_dato);
    pthread_mutex_unlock(&mutex_lista_memory_stick);
    if (lista_memory_sticks == NULL) {
        log_debug(loggerCpu, "DEBUG_CPU: Hubo SEG_FAULT en direccion logica: %d", direccion_logica_si);
        lanzar_seg_fault(socketConexionScheduler, pid);
        return COD_SEG_FAULT;
    }

    log_debug(loggerCpu, "DEBUG_CPU: Inicio lectura en memoria");
    t_resultado_lectura* resultado = leer_en_memoria(lista_memory_sticks, pid, loggerCpu);
    list_destroy_and_destroy_elements(lista_memory_sticks, free);
    if (resultado != NULL) {
        log_info(loggerCpu, "## PID: %d - Acción: LEER - Dirección Física: %d - Valor: %.*s", pid, resultado->dir_fisica, tamanio_dato, resultado->dato);
        uint32_t direccion_logica_di = leer_valor_en_registro(DI);
        pthread_mutex_lock(&mutex_lista_memory_stick);
        t_list* lista_sticks_destino = traducir_logica_a_fisica(direccion_logica_di, segment_max_size, lista_segmentos, lista_memory_stick, tamanio_dato);
        pthread_mutex_unlock(&mutex_lista_memory_stick);
        if (lista_sticks_destino == NULL) {
            log_debug(loggerCpu, "DEBUG_CPU: Hubo SEG_FAULT en direccion logica: %d", direccion_logica_di);
            lanzar_seg_fault(socketConexionScheduler, pid);
            free(resultado->dato);
            free(resultado);
            return COD_SEG_FAULT;
        }

        log_debug(loggerCpu, "DEBUG_CPU: Inicio escritura en memoria");
        int primera_dir_fisica = escribir_en_memoria(lista_sticks_destino, resultado->dato, pid, loggerCpu);
        list_destroy_and_destroy_elements(lista_sticks_destino, free);
        if (primera_dir_fisica == -1) {
            log_debug(loggerCpu, "DEBUG_CPU: Hubo SEG_FAULT se desconecto memory stick");
            lanzar_seg_fault(socketConexionScheduler, pid);
            return COD_SEG_FAULT;
        }

        log_info(loggerCpu, "## PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %.*s", pid, primera_dir_fisica, tamanio_dato, resultado->dato);
        free(resultado->dato);
        free(resultado);

        return 0;
    }

    log_debug(loggerCpu, "DEBUG_CPU: Fallo la lectura en Memory Stick (COPY_MEM), lanzo SEG_FAULT");
    lanzar_seg_fault(socketConexionScheduler, pid);
    return COD_SEG_FAULT;
}

//FUNCIONES AUXILIARES PARA LAS INSTRUCCIONES
void escribir_valor_en_registro(Codigo_registros_cpu tipoRegistro, int valor) {
    switch (tipoRegistro) {
        case PC: registros_cpu.pc = valor; break;
        case AX: registros_cpu.ax = valor; break;
        case BX: registros_cpu.bx = valor; break;
        case CX: registros_cpu.cx = valor; break;
        case DX: registros_cpu.dx = valor; break;
        case EAX: registros_cpu.eax = valor; break;
        case EBX: registros_cpu.ebx = valor; break;
        case ECX: registros_cpu.ecx = valor; break;
        case EDX: registros_cpu.edx = valor; break;
        case SI: registros_cpu.si = valor; break;
        case DI: registros_cpu.di = valor; break;
        default: break;
    }
}
int leer_valor_en_registro(Codigo_registros_cpu tipoRegistro) {
    switch (tipoRegistro) {
        case PC: return registros_cpu.pc;
        case AX: return registros_cpu.ax;
        case BX: return registros_cpu.bx;
        case CX: return registros_cpu.cx;
        case DX: return registros_cpu.dx;
        case EAX: return registros_cpu.eax;
        case EBX: return registros_cpu.ebx;
        case ECX: return registros_cpu.ecx;
        case EDX: return registros_cpu.edx;
        case SI: return registros_cpu.si;
        case DI: return registros_cpu.di;
        default: return 0;
    }
}
const char* registro_a_string(Codigo_registros_cpu tipoRegistro) { //FUNCION UTIL SOLO PARA LOS LOGS
    switch (tipoRegistro) {
        case PC: return "PC";
        case AX: return "AX";
        case BX: return "BX";
        case CX: return "CX";
        case DX: return "DX";
        case EAX: return "EAX";
        case EBX: return "EBX";
        case ECX: return "ECX";
        case EDX: return "EDX";
        case SI: return "SI";
        case DI: return "DI";
        default: return "noReconocido";
    }
}
void lanzar_seg_fault(int socketConexionScheduler, uint32_t pid) {
    t_buffer* buffer = buffer_create(0);
    buffer_add_uint32(buffer, pid);
    t_paquete* paquete_seg_fault = crear_paquete(SEG_FAULT, buffer);
    enviar_paquete(socketConexionScheduler, paquete_seg_fault);
    eliminar_paquete(paquete_seg_fault);
    log_debug(loggerCpu, "DEBUG_CPU: SEG_FAULT lanzado para PID: %d", pid);
}
void interpretar_y_escribir_dato_en_registro(Codigo_registros_cpu tipoRegistro, char* dato_leido, uint32_t primera_dir_fisica, uint32_t pid) {
    uint32_t valor_leido;
    if (tipoRegistro > 4) {
        valor_leido = *(uint32_t*)dato_leido;
        log_info(loggerCpu, "## PID: %d - Acción: LEER - Dirección Física: %d - Valor: %d", pid, primera_dir_fisica, valor_leido);
        escribir_valor_en_registro(tipoRegistro, valor_leido);
    } else {
        valor_leido = *(uint8_t*)dato_leido;
        log_info(loggerCpu, "## PID: %d - Acción: LEER - Dirección Física: %d - Valor: %d", pid, primera_dir_fisica, valor_leido);
        escribir_valor_en_registro(tipoRegistro, valor_leido);
    }
    free(dato_leido);
}