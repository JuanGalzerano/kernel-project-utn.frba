/*#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>

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
        if ((memory_stick->base_acumulada + memory_stick->size) >= direccion) {
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

uint32_t traducir_logica_a_fisica (uint32_t direccion_logica, uint32_t segment_max_size, t_list* lista_segmentos, t_list* lista_memory_stick, uint32_t tamanio_a_leer) {
    uint32_t numero_de_segmento = floor(direccion_logica / segment_max_size);
    uint32_t desplazamiento_del_segmento = direccion_logica % segment_max_size;

    t_segmento* segmento = buscar_segmento(lista_segmentos, numero_de_segmento);

    uint32_t base_del_segmento = segmento->base;
    uint32_t limite_del_segmento = segmento->limite;

    t_memory_stick_info* stick_base = buscar_stick_base(lista_memory_stick, base_del_segmento + desplazamiento_del_segmento);

    if ((stick_base->base_acumulada + stick_base->size) >= (base_del_segmento + desplazamiento_del_segmento + tamanio)) {
        leer_en_memoria(stick_base->socket, base_del_segmento + desplazamiento_del_segmento, tamanio);
    }
    else {
        cuanto_se_paso = (base_del_segmento + desplazamiento_del_segmento + tamanio) - (stick_base->base_acumulada + stick_base->size);
        leer_en_memoria(stick_base->socket, base_del_segmento + desplazamiento_del_segmento, (tamanio - cuanto_se_paso));

        t_memory_stick_info* siguiente_stick = buscar_siguiente_stick(lista_memory_stick, stick_base);

        leer_en_memoria(stick_base->socket, base_del_segmento + desplazamiento_del_segmento + (tamanio - cuanto_se_paso), cuanto_se_paso);
    }

    return direccion_fisica;
}*/