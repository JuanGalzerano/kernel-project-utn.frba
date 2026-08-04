#include "auxiliares_ciclo.h"
#include "instrucciones.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

char* obtener_linea_con_instruccion(uint32_t pid, uint32_t pc, int socketConexionMemory) {
    t_buffer* buf_instr = buffer_create(0);
    buffer_add_uint32(buf_instr, pid);
    buffer_add_uint32(buf_instr, pc);
    t_paquete* req_instr = crear_paquete(OBTENER_INSTRUCCION, buf_instr);
    enviar_paquete(socketConexionMemory, req_instr);
    eliminar_paquete(req_instr);

    t_paquete *paquete = recibir_paquete(socketConexionMemory);
    if (paquete == NULL) {
        return NULL;
    }
    uint32_t longitud = paquete->buffer->size;
    char *inicioInstruccion = buffer_read_string(paquete->buffer, longitud);
    eliminar_paquete(paquete);

    return inicioInstruccion;
}

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