#include "memory_sticks.h"
#include "segmentacion.h"

void agregar_memory_stick(int socket, uint32_t size) {
    t_memory_stick_info* stick = malloc(sizeof(t_memory_stick_info));
    stick->socket = socket;
    stick->size   = size;

    pthread_mutex_lock(&memoria_mutex);

    stick->base_acumulada = memoria_total_size;

    list_add(lista_memory_sticks, stick);

    memoria_total_size += size;
    memoria_libre_size += size;
    //crear el hueco gigante que se generar al conectarse una memory stick
    t_hueco* hueco = malloc(sizeof(t_hueco));
    hueco->base = stick->base_acumulada;
    hueco->limite = size;
    insertar_hueco_y_fusionar(hueco);

    pthread_mutex_unlock(&memoria_mutex);
}

t_memory_stick_info* encontrar_stick_por_dir_fisica(uint32_t dir_fisica) {
    for (int i = 0; i < list_size(lista_memory_sticks); i++) {
        t_memory_stick_info* stick = list_get(lista_memory_sticks, i);
        if (dir_fisica >= stick->base_acumulada && dir_fisica < stick->base_acumulada + stick->size)
            return stick;
    }
    return NULL;
}

static void manejar_desconexion_stick(int socket) {
    log_warning(loggerMemory, "Memory Stick desconectado (FD %d) - memoria corrupta", socket);

    pthread_mutex_lock(&memoria_mutex);
    for (int i = 0; i < list_size(lista_memory_sticks); i++) {
        t_memory_stick_info* s = list_get(lista_memory_sticks, i);
        if (s->socket == socket) {
            list_remove_and_destroy_element(lista_memory_sticks, i, free);
            break;
        }
    }
    pthread_mutex_unlock(&memoria_mutex);

    t_buffer* buf = buffer_create(0);
    buffer_add_uint32(buf, (uint32_t)socket);
    t_paquete* aviso = crear_paquete(MEMORIA_CORRUPTA, buf);
    enviar_paquete(socketScheduler, aviso);
    eliminar_paquete(aviso);

    close(socket);
}

int leer_pedazos(t_list* pedazos, char* buffer_out) {
    uint32_t bytes_ya_procesados = 0;
    for (int i = 0; i < list_size(pedazos); i++) {
        struct_control_mmu* pedazo = list_get(pedazos, i);

        t_buffer* buf = buffer_create(0);
        buffer_add_uint32(buf, pedazo->desde_donde_leer);
        buffer_add_uint32(buf, pedazo->tamanio_a_leer_en_esta_memory_stick);
        t_paquete* pedido = crear_paquete(LEER_BYTES, buf);
        enviar_paquete(pedazo->socketMemoryStick, pedido);
        eliminar_paquete(pedido);

        t_paquete* respuesta = recibir_paquete(pedazo->socketMemoryStick);
        if (respuesta == NULL) {
            manejar_desconexion_stick(pedazo->socketMemoryStick);
            return -1;
        }
        buffer_read(respuesta->buffer, buffer_out + bytes_ya_procesados, pedazo->tamanio_a_leer_en_esta_memory_stick);
        eliminar_paquete(respuesta);
        bytes_ya_procesados += pedazo->tamanio_a_leer_en_esta_memory_stick;
    }
    return 1;
}

int escribir_pedazos(t_list* pedazos, char* datos) {
    uint32_t bytes_ya_procesados = 0;
    for (int i = 0; i < list_size(pedazos); i++) {
        struct_control_mmu* pedazo = list_get(pedazos, i);

        t_buffer* buf = buffer_create(0);
        buffer_add_uint32(buf, pedazo->desde_donde_leer);
        buffer_add_uint32(buf, pedazo->tamanio_a_leer_en_esta_memory_stick);
        buffer_add(buf, datos + bytes_ya_procesados, pedazo->tamanio_a_leer_en_esta_memory_stick);
        t_paquete* pedido = crear_paquete(ESCRIBIR_BYTES, buf);
        enviar_paquete(pedazo->socketMemoryStick, pedido);
        eliminar_paquete(pedido);

        t_paquete* respuesta = recibir_paquete(pedazo->socketMemoryStick);
        if (respuesta == NULL) {
            manejar_desconexion_stick(pedazo->socketMemoryStick);
            return -1;
        }
        eliminar_paquete(respuesta);
        bytes_ya_procesados += pedazo->tamanio_a_leer_en_esta_memory_stick;
    }
    return 1;
}
