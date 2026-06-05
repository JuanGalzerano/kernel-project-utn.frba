#include "mmu.h"
#include <utils/utils.h>

t_list* traducir_logica_a_fisica(uint32_t direccion_logica, uint32_t segment_max_size, t_list* lista_segmentos, t_list* lista_memory_stick, uint32_t tamanio_dato) {
    uint32_t numero_de_segmento = direccion_logica / segment_max_size; //No hace falta el floor pq los dos son enteros
    uint32_t desplazamiento_del_segmento = direccion_logica % segment_max_size;

    t_segmento* segmento = buscar_segmento(lista_segmentos, numero_de_segmento);
    if (segmento == NULL) {
        // El proceso no tiene ese segmento: lo tratamos como segmentation fault
        return NULL;
    }

    uint32_t base_del_segmento = segmento->base;
    uint32_t limite_del_segmento = segmento->limite;

    if (hay_segmentation_fault(desplazamiento_del_segmento, limite_del_segmento, tamanio_dato)) {
        return NULL;
    }

    t_memory_stick_info* stick_base = buscar_stick_base(lista_memory_stick, base_del_segmento + desplazamiento_del_segmento);
    if (stick_base == NULL) {
        // La direccion fisica esta fuera del rango total de memoria: error del sistema
        return NULL;
    }

    t_list* lista_de_memories_stick_a_llamar = list_create();
    uint32_t err = crear_lista_con_memories_stick_a_llamar(stick_base, base_del_segmento, desplazamiento_del_segmento, tamanio_dato, lista_de_memories_stick_a_llamar, lista_memory_stick);

    if (err != 0) {
        list_destroy_and_destroy_elements(lista_de_memories_stick_a_llamar, free);
        return NULL;
    }

    return lista_de_memories_stick_a_llamar;
}

bool hay_segmentation_fault(uint32_t desplazamiento_del_segmento, uint32_t limite_del_segmento, uint32_t tamanio_dato) {
    if (desplazamiento_del_segmento + tamanio_dato > limite_del_segmento) {
        return true;
    }
    else {
        return false;
    }
}

t_segmento* buscar_segmento(t_list* lista_segmentos, uint32_t numero_de_segmento) {
    for (int i = 0; i < list_size(lista_segmentos); i++) {
        t_segmento* seg = list_get(lista_segmentos, i);
        if (seg->id_segmento == numero_de_segmento) {
            return seg;
        }
    }
    return NULL;
}

t_memory_stick_info* buscar_stick_base(t_list* lista_memory_stick, uint32_t direccion) {
    for (int k = 0; k < list_size(lista_memory_stick); k++) {
        t_memory_stick_info* memory_stick = list_get(lista_memory_stick, k);
        if ((memory_stick->base_acumulada + memory_stick->size) > direccion) {
            return memory_stick;
        }
    }
    return NULL;
}

t_memory_stick_info* buscar_siguiente_stick(t_list* lista_memory_stick, t_memory_stick_info* stick_base) {
    for (int j = 0; j < list_size(lista_memory_stick); j++) {
        t_memory_stick_info* ms = list_get(lista_memory_stick, j);
        if (ms->base_acumulada == stick_base->base_acumulada + stick_base->size) {
            return ms;
        }
    }
    return NULL;
}

uint32_t crear_lista_con_memories_stick_a_llamar(t_memory_stick_info* stick, uint32_t base_del_segmento, uint32_t desplazamiento_del_segmento, uint32_t tamanio, t_list* lista_de_memories_stick_a_llamar, t_list* lista_memory_stick) {
    uint32_t direccion_fisica_global = base_del_segmento + desplazamiento_del_segmento;
    uint32_t offset_en_stick = direccion_fisica_global - stick->base_acumulada;
    uint32_t bytes_restantes = tamanio;

    while (bytes_restantes > 0) {
        // Cuanto espacio queda en este stick desde el offset donde arranco
        uint32_t espacio_disponible_en_stick = stick->size - offset_en_stick;

        // Decido cuanto leo en este stick: o todo lo que me falta, o lo que entra
        uint32_t bytes_a_leer_en_este_stick;
        if (espacio_disponible_en_stick >= bytes_restantes) {
            bytes_a_leer_en_este_stick = bytes_restantes;
        } else {
            bytes_a_leer_en_este_stick = espacio_disponible_en_stick;
        }

        // Creo la estructura de control para este stick y la agrego a la lista
        struct_control_mmu* control = malloc(sizeof(struct_control_mmu));
        control->socketMemoryStick = stick->socket;
        control->desde_donde_leer = offset_en_stick;
        control->tamanio_a_leer_en_esta_memory_stick = bytes_a_leer_en_este_stick;
        list_add(lista_de_memories_stick_a_llamar, control);

        // Descuento lo que ya planifique leer
        bytes_restantes = bytes_restantes - bytes_a_leer_en_este_stick;

        // Si todavia queda dato, paso al siguiente stick y reseteo el offset
        if (bytes_restantes > 0) {
            t_memory_stick_info* siguiente = buscar_siguiente_stick(lista_memory_stick, stick);

            if (siguiente == NULL) {
                // No hay mas sticks pero todavia falta dato: error del sistema
                return 1;
            }

            stick = siguiente;
            offset_en_stick = 0;
        }
    }
    return 0;
}

char* leer_en_memoria(t_list* lista_de_memories_stick_a_llamar) {
    uint32_t tamanio_total = 0;
    for (int i = 0; i < list_size(lista_de_memories_stick_a_llamar); i++) {
        struct_control_mmu* memoria_a_llamar = list_get(lista_de_memories_stick_a_llamar, i);
        tamanio_total += memoria_a_llamar->tamanio_a_leer_en_esta_memory_stick;
    }

    char* valor_leido = malloc(tamanio_total);
    uint32_t leido = 0;

    for (int i = 0; i < list_size(lista_de_memories_stick_a_llamar); i++) {
        struct_control_mmu* memoria_a_llamar = list_get(lista_de_memories_stick_a_llamar, i);

        t_buffer* buf = buffer_create(0);
        buffer_add_uint32(buf, memoria_a_llamar->desde_donde_leer);
        buffer_add_uint32(buf, memoria_a_llamar->tamanio_a_leer_en_esta_memory_stick);
        t_paquete* paquete_para_leer = crear_paquete(LEER_BYTES, buf);
        enviar_paquete(memoria_a_llamar->socketMemoryStick, paquete_para_leer);
        eliminar_paquete(paquete_para_leer);

        t_paquete* paquete_con_lo_leido = recibir_paquete(memoria_a_llamar->socketMemoryStick);
        if (paquete_con_lo_leido == NULL || paquete_con_lo_leido->codigo_operacion != LEER_BYTES) {
            if (paquete_con_lo_leido != NULL) {
                eliminar_paquete(paquete_con_lo_leido);
            }
            free(valor_leido);
            return NULL;
        }

        buffer_read(paquete_con_lo_leido->buffer, valor_leido + leido, memoria_a_llamar->tamanio_a_leer_en_esta_memory_stick);
        eliminar_paquete(paquete_con_lo_leido);

        leido += memoria_a_llamar->tamanio_a_leer_en_esta_memory_stick;
    }
    return valor_leido;
}

uint32_t escribir_en_memoria(t_list* lista_de_memories_stick_a_llamar, char* valor_a_escribir) {
    uint32_t escrito = 0;
    for (int i = 0; i < list_size(lista_de_memories_stick_a_llamar); i++) {
        struct_control_mmu* memoria_a_llamar = list_get(lista_de_memories_stick_a_llamar, i);
        uint32_t tamanio = memoria_a_llamar->tamanio_a_leer_en_esta_memory_stick;

        t_buffer* buf = buffer_create(0);
        buffer_add_uint32(buf, memoria_a_llamar->desde_donde_leer);
        buffer_add_uint32(buf, tamanio);
        buffer_add(buf, valor_a_escribir + escrito, tamanio);
        t_paquete* paquete_para_escribir = crear_paquete(ESCRIBIR_BYTES, buf);
        enviar_paquete(memoria_a_llamar->socketMemoryStick, paquete_para_escribir);
        eliminar_paquete(paquete_para_escribir);

        t_paquete* respuesta = recibir_paquete(memoria_a_llamar->socketMemoryStick);
        if (respuesta == NULL || respuesta->codigo_operacion != ESCRIBIR_BYTES) {
            if (respuesta != NULL) {
                eliminar_paquete(respuesta);
            }
            return 1;
        }
        eliminar_paquete(respuesta);

        escrito += tamanio;
    }
    return 0;
}