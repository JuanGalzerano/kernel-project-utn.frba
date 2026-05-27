#include "segmentacion.h"
#include "inicializar.h"

static t_hueco* encontrar_hueco_best_fit(uint32_t tamaño) {
    t_hueco* mejor = NULL;
    for (int i = 0; i < list_size(lista_huecos); i++) {
        t_hueco* h = list_get(lista_huecos, i);
        if (h->limite >= tamaño && (!mejor || h->limite < mejor->limite))
            mejor = h;
    }
    return mejor;
}

static t_hueco* encontrar_hueco_worst_fit(uint32_t tamaño) {
    t_hueco* peor = NULL;
    for (int i = 0; i < list_size(lista_huecos); i++) {
        t_hueco* h = list_get(lista_huecos, i);
        if (h->limite >= tamaño && (!peor || h->limite > peor->limite))
            peor = h;
    }
    return peor;
}

static t_hueco* encontrar_hueco(uint32_t tamaño) {
    if (strcmp(allocation_strategy, "BEST") == 0)
        return encontrar_hueco_best_fit(tamaño);
    return encontrar_hueco_worst_fit(tamaño);
}

// Inserta un hueco en lista_huecos (ordenada por base) y fusiona con adyacentes.
// Llamar con memoria_mutex tomado.
void insertar_hueco_y_fusionar(t_hueco* nuevo) {
    int pos = list_size(lista_huecos);
    for (int i = 0; i < list_size(lista_huecos); i++) {
        t_hueco* h = list_get(lista_huecos, i);
        if (h->base > nuevo->base) { pos = i; break; }
    }
    list_add_in_index(lista_huecos, pos, nuevo);

    if (pos + 1 < list_size(lista_huecos)) {
        t_hueco* sig = list_get(lista_huecos, pos + 1);
        if (nuevo->base + nuevo->limite == sig->base) {
            nuevo->limite += sig->limite;
            list_remove_and_destroy_element(lista_huecos, pos + 1, free);
        }
    }

    if (pos > 0) {
        t_hueco* ant = list_get(lista_huecos, pos - 1);
        if (ant->base + ant->limite == nuevo->base) {
            ant->limite += nuevo->limite;
            list_remove_and_destroy_element(lista_huecos, pos, free);
        }
    }
}

t_segmento* buscar_segmento_proceso(t_proceso_memory* proceso, uint32_t id_segmento) {
    for (int i = 0; i < list_size(proceso->contexto->tabla_segmentos); i++) {
        t_segmento* seg = list_get(proceso->contexto->tabla_segmentos, i);
        if (seg->id_segmento == id_segmento) return seg;
    }
    return NULL;
}

op_code crear_segmento(uint32_t pid, uint32_t id_segmento, uint32_t tamaño) {
    t_proceso_memory* proceso = buscar_proceso(pid);
    if (!proceso) {
        log_error(loggerMemory, "PID %d no encontrado al crear segmento %d", pid, id_segmento);
        return MEMORIA_NO_DISPONIBLE;
    }
    if (tamaño > segment_max_size) {
        log_error(loggerMemory, "PID %d: segmento %d excede tamaño maximo (%d > %d)", pid, id_segmento, tamaño, segment_max_size);
        return MEMORIA_NO_DISPONIBLE;
    }

    pthread_mutex_lock(&memoria_mutex);

    t_hueco* hueco = encontrar_hueco(tamaño);

    if (hueco != NULL) {
        t_segmento* seg  = malloc(sizeof(t_segmento));
        seg->id_segmento = id_segmento;
        seg->base        = hueco->base;
        seg->limite      = tamaño;
        list_add(proceso->contexto->tabla_segmentos, seg);

        hueco->base   += tamaño;
        hueco->limite -= tamaño;
        if (hueco->limite == 0) {
            list_remove_element(lista_huecos, hueco);
            free(hueco);
        }
        memoria_libre_size -= tamaño;

        pthread_mutex_unlock(&memoria_mutex);
        log_info(loggerMemory, "PID: %d - Segmento Creado - Id: %d - Base: %d - Tamaño: %d", pid, id_segmento, seg->base, tamaño);
        return MEMORIA_DISPONIBLE;
    }

    if (memoria_libre_size >= tamaño) {
        pthread_mutex_unlock(&memoria_mutex);
        log_info(loggerMemory, "PID: %d - Segmento %d - Memoria suficiente pero no contigua, requiere compactacion", pid, id_segmento);
        return COMPACTACION;
    }

    pthread_mutex_unlock(&memoria_mutex);
    log_info(loggerMemory, "PID: %d - Segmento %d - Memoria insuficiente (libre: %d, pedido: %d)", pid, id_segmento, memoria_libre_size, tamaño);
    return MEMORIA_NO_DISPONIBLE;
}

int eliminar_segmento(uint32_t pid, uint32_t id_segmento) {
    t_proceso_memory* proceso = buscar_proceso(pid);
    if (!proceso) return -1;

    pthread_mutex_lock(&memoria_mutex);

    t_segmento* seg = buscar_segmento_proceso(proceso, id_segmento);
    if (!seg) {
        pthread_mutex_unlock(&memoria_mutex);
        return -1;
    }

    t_hueco* hueco_liberado  = malloc(sizeof(t_hueco));
    hueco_liberado->base     = seg->base;
    hueco_liberado->limite   = seg->limite;

    memoria_libre_size += seg->limite;

    list_remove_element(proceso->contexto->tabla_segmentos, seg);
    free(seg);

    insertar_hueco_y_fusionar(hueco_liberado);

    pthread_mutex_unlock(&memoria_mutex);
    return 1;
}
