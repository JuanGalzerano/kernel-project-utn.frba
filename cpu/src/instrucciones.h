#ifndef INSTRUCCIONES_H_  
#define INSTRUCCIONES_H_

#include <utils/serializacion.h>  
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

//STRUCT PARA CONVERTIR LOS STRINGS EN REGISTROS
typedef struct {
    char* nombreDelRegistro;
    Codigo_registros_cpu codigo_registros_cpu;
} Registro;
//STRUCT PARA CONVERTIR LAS INSTRUCCIONES EN REGISTROS
typedef struct {
    char* nombreDeLaInstruccion;
    op_code instruccion_codigo;
} Instruccion;

//VARIABLES GLOBALES
extern Registros_cpu registros_cpu;
extern Registro registros[];
extern Instruccion instrucciones[];

void ejecutar_set(Codigo_registros_cpu tipoRegistro, uint32_t valor);
void ejecutar_mov_in(int socketConexionScheduler, uint32_t pid, Codigo_registros_cpu tipoRegistro, uint32_t segment_max_size, t_list* lista_segmentos);
void ejecutar_mov_out(int socketConexionScheduler, uint32_t pid, Codigo_registros_cpu tipoRegistro, uint32_t segment_max_size, t_list* lista_segmentos);
void ejecutar_sum(Codigo_registros_cpu registroDestino, Codigo_registros_cpu registroOrigen);
void ejecutar_sub(Codigo_registros_cpu registroDestino, Codigo_registros_cpu registroOrigen);
void ejecutar_jnz(Codigo_registros_cpu tipoRegistro, uint32_t instruccion);
void escribir_valor_en_registro(Codigo_registros_cpu tipoRegistro, int valor);
int leer_valor_en_registro(Codigo_registros_cpu tipoRegistro);
const char* registro_a_string(Codigo_registros_cpu tipoRegistro);

#endif