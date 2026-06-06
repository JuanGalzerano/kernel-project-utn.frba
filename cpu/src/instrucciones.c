#include "instrucciones.h"
#include <utils/mmu.h>
#include "kernel_memory_avisos.h"

Registros_cpu registros_cpu; //LOS REGISTROS DE LA CPU

Registro registros[] = { //EXISTE PARA OBTENER REGISTRO A PARTIR DE STRING
    {"PC", PC}, {"AX", AX}, {"BX", BX}, {"CX", CX},
    {"DX", DX}, {"EAX", EAX}, {"EBX", EBX}, {"ECX", ECX},
    {"EDX", EDX}, {"SI", SI}, {"DI", DI},
    {NULL, 0}
};

Instruccion instrucciones[] = { //EXISTE PARA OBTENER INSTRUCCION A PARTIR DE STRING
    {"NOOP", NOOP}, {"SET", SET}, {"MOV_IN", MOV_IN}, {"MOV_OUT", MOV_OUT},
    {"SUM", SUM}, {"SUB", SUB}, {"JNZ", JNZ}, {"COPY_MEM", COPY_MEM},
    {"MUTEX_CREATE", MUTEX_CREATE}, {"MUTEX_LOCK", MUTEX_LOCK}, {"MUTEX_UNLOCK", MUTEX_UNLOCK},
    {"MEM_ALLOC", MEM_ALLOC}, {"MEM_FREE", MEM_FREE}, {"SLEEP", SLEEP}, {"STDOUT", STDOUT},
    {"STDIN", STDIN}, {"INIT_PROC", INIT_PROC}, {"EXIT", EXIT},
    {NULL, 0}
};

//INSTRUCCIONES QUE YO EJECUTO
void ejecutar_set(Codigo_registros_cpu tipoRegistro, uint32_t valor) {
    escribir_valor_en_registro(tipoRegistro, valor);
}
void ejecutar_mov_in(int socketConexionScheduler, uint32_t pid, Codigo_registros_cpu tipoRegistro, uint32_t segment_max_size, t_list* lista_segmentos) {
    uint32_t direccion_logica = leer_valor_en_registro(SI);
    uint32_t tamanio_dato;
    if (tipoRegistro > 4) {
        tamanio_dato = sizeof(uint32_t);
    } else {
        tamanio_dato = sizeof(uint8_t);
    }

    pthread_mutex_lock(&mutex_lista_memory_stick);
    t_list* lista_memory_sticks = traducir_logica_a_fisica(direccion_logica, segment_max_size, lista_segmentos, lista_memory_stick, tamanio_dato);
    pthread_mutex_unlock(&mutex_lista_memory_stick);
    if (lista_memory_sticks == NULL) {
        t_buffer* buffer = buffer_create(0);
        buffer_add_uint32(buffer, pid);
        t_paquete* paquete_seg_fault =  crear_paquete(SEG_FAULT, buffer);
        enviar_paquete(socketConexionScheduler, paquete_seg_fault);
        eliminar_paquete(paquete_seg_fault);
        return;
    }

    char* dato_leido = leer_en_memoria(lista_memory_sticks);
    if (dato_leido != NULL) {
        if (tipoRegistro > 4) {
            escribir_valor_en_registro(tipoRegistro, *(uint32_t*)dato_leido);
        } else {
            escribir_valor_en_registro(tipoRegistro, *(uint8_t*)dato_leido);
        }
        free(dato_leido);
    }

    list_destroy_and_destroy_elements(lista_memory_sticks, free);
}
void ejecutar_mov_out(int socketConexionScheduler, uint32_t pid, Codigo_registros_cpu tipoRegistro, uint32_t segment_max_size, t_list* lista_segmentos) {
    uint32_t direccion_logica = leer_valor_en_registro(DI);
    uint32_t valor_del_registro = leer_valor_en_registro(tipoRegistro);
    uint32_t tamanio_dato;
    if (tipoRegistro > 4) {
        tamanio_dato = sizeof(uint32_t);
    } else {
        tamanio_dato = sizeof(uint8_t);
    }

    pthread_mutex_lock(&mutex_lista_memory_stick);
    t_list* lista_memory_sticks = traducir_logica_a_fisica(direccion_logica, segment_max_size, lista_segmentos, lista_memory_stick, tamanio_dato);
    pthread_mutex_unlock(&mutex_lista_memory_stick);
    if (lista_memory_sticks == NULL) {
        t_buffer* buffer = buffer_create(0);
        buffer_add_uint32(buffer, pid);
        t_paquete* paquete_seg_fault =  crear_paquete(SEG_FAULT, buffer);
        enviar_paquete(socketConexionScheduler, paquete_seg_fault);
        eliminar_paquete(paquete_seg_fault);
        return;
    }

    uint32_t err = escribir_en_memoria(lista_memory_sticks, (char*)&valor_del_registro);
    list_destroy_and_destroy_elements(lista_memory_sticks, free);
}
void ejecutar_sum(Codigo_registros_cpu registroDestino, Codigo_registros_cpu registroOrigen) {
    uint32_t valorDestino = leer_valor_en_registro(registroDestino);
    uint32_t valorOrigen = leer_valor_en_registro(registroOrigen);
    escribir_valor_en_registro(registroDestino, valorDestino + valorOrigen);
}
void ejecutar_sub(Codigo_registros_cpu registroDestino, Codigo_registros_cpu registroOrigen) {
    uint32_t valorDestino = leer_valor_en_registro(registroDestino);
    uint32_t valorOrigen = leer_valor_en_registro(registroOrigen);
    escribir_valor_en_registro(registroDestino, valorDestino - valorOrigen);
}
void ejecutar_jnz(Codigo_registros_cpu tipoRegistro, uint32_t instruccion) {
    if (leer_valor_en_registro(tipoRegistro) != 0) {
        escribir_valor_en_registro(PC, instruccion);
    }
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