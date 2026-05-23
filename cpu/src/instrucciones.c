#include "instrucciones.h"  

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
/*ejecutar_mov_in(tipoRegistro) {
    uint32_t direccion_logica = leer_valor_en_registro(SI);
    direccion_fisica = traducir_logica_a_fisica(direccion_logica);
    if (tipoRegistro > 3) {
        char* valor = leer_memoria(direccion_fisica, uint32_t);
    }
    else {
        char* valor = leer_memoria(direccion_fisica, uint8_t);
    }
    escribir_valor_en_registro(tipoRegistro, valor);
}*/
/*ejecutar_mov_out(tipoRegistro) {

}*/
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