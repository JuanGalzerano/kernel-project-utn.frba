#ifndef INSTRUCCIONES_H_  
#define INSTRUCCIONES_H_

#include <utils/serializacion.h>
#include "inicializar_cpu.h"

#define COD_SEG_FAULT 2
#define COD_EXIT 3

typedef struct {
    uint32_t pc;
    uint8_t ax;
    uint8_t bx;
    uint8_t cx;
    uint8_t dx;
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t si;
    uint32_t di;
} Registros_cpu;

typedef enum {
    PC = 0,
    AX = 1,
    BX = 2,
    CX = 3,
    DX = 4,
    EAX = 5,
    EBX = 6,
    ECX = 7,
    EDX = 8,
    SI = 9,
    DI = 10
} Codigo_registros_cpu;

typedef struct {
    char* nombreDelRegistro;
    Codigo_registros_cpu codigo_registros_cpu;
} Registro;

typedef struct {
    char* nombreDeLaInstruccion;
    op_code instruccion_codigo;
} Instruccion;

extern Registros_cpu registros_cpu;
extern Registro registros[];
extern Instruccion instrucciones[];

uint32_t ejecutar_set(Codigo_registros_cpu tipoRegistro, uint32_t valor);
uint32_t ejecutar_mov_in(int socketConexionScheduler, uint32_t pid, Codigo_registros_cpu tipoRegistro, uint32_t segment_max_size, t_list* lista_segmentos);
uint32_t ejecutar_mov_out(int socketConexionScheduler, uint32_t pid, Codigo_registros_cpu tipoRegistro, uint32_t segment_max_size, t_list* lista_segmentos);
uint32_t ejecutar_sum(Codigo_registros_cpu registroDestino, Codigo_registros_cpu registroOrigen);
uint32_t ejecutar_sub(Codigo_registros_cpu registroDestino, Codigo_registros_cpu registroOrigen);
uint32_t ejecutar_jnz(Codigo_registros_cpu tipoRegistro, uint32_t instruccion);
uint32_t ejecutar_copy_mem(int socketConexionScheduler, uint32_t pid, Codigo_registros_cpu tipoRegistro, uint32_t segment_max_size, t_list* lista_segmentos);
void escribir_valor_en_registro(Codigo_registros_cpu tipoRegistro, int valor);
int leer_valor_en_registro(Codigo_registros_cpu tipoRegistro);
const char* registro_a_string(Codigo_registros_cpu tipoRegistro);
void lanzar_seg_fault(int socketConexionScheduler, uint32_t pid);
void interpretar_y_escribir_dato_en_registro(Codigo_registros_cpu tipoRegistro, char* dato_leido);

#endif