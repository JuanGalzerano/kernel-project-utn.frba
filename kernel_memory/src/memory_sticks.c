#include "memory_sticks.h"
#include "segmentacion.h"
#include <sys/epoll.h>

void agregar_memory_stick(int socket, uint32_t size, char* puertoMemoryStick){
    t_memory_stick_info* stick = malloc(sizeof(t_memory_stick_info));
    stick->socket = socket;
    stick->size  = size;

    pthread_mutex_lock(&memoria_mutex);

    uint32_t puertoDeNuevaStick = (list_size(lista_memory_sticks) + atoi(puertoMemoryStick));
    stick->puerto = puertoDeNuevaStick;
    send(socket,&puertoDeNuevaStick,sizeof(uint32_t),0);

    //Espero confirmacion de que el stick abrio su servidor antes de notificar a las CPUs
    uint32_t listo =0;
    recv(socket, &listo, sizeof(uint32_t), MSG_WAITALL);

    stick->base_acumulada = memoria_total_size;

    list_add(lista_memory_sticks, stick);

    memoria_total_size += size;
    memoria_libre_size += size;
    t_hueco* hueco = malloc(sizeof(t_hueco));
    hueco->base = stick->base_acumulada;
    hueco->limite = size;
    insertar_hueco_y_fusionar(hueco);

    t_buffer* bufSticks = serializar_aviso_nuevo_stick(lista_memory_sticks);

    pthread_mutex_unlock(&memoria_mutex);

    t_paquete* avisoStick = crear_paquete(AVISO_NUEVO_STICK, bufSticks);
    pthread_mutex_lock(&mutex_sockets_cpu_notif);
    int cantCpus = list_size(lista_sockets_cpu_notif);
    for (int i = 0; i < cantCpus;i++){
        int socketCpu = *(int*)list_get(lista_sockets_cpu_notif, i);
        enviar_paquete(socketCpu, avisoStick);
    }
    pthread_mutex_unlock(&mutex_sockets_cpu_notif);
    eliminar_paquete(avisoStick);
    log_info(loggerMemory, "## Notificado a %d CPU(s): AVISO_NUEVO_STICK", cantCpus);

    //registrar con epoll para detectar Ctrl+C
    struct epoll_event ev;
    ev.events = EPOLLRDHUP;
    ev.data.fd = socket;
    epoll_ctl(epoll_fd_sticks, EPOLL_CTL_ADD, socket, &ev);

    //otificar al scheduler que hay nueva memoria disponible, incluyendo el total libre actual
    t_buffer* bufAviso = buffer_create(0);
    buffer_add_uint32(bufAviso, memoria_libre_size);
    t_paquete* aviso = crear_paquete(NUEVA_MEMORIA_DISPONIBLE, bufAviso);
    enviar_paquete(socketSchedulerNotif, aviso);
    eliminar_paquete(aviso);
    log_info(loggerMemory, "## Notificado al Scheduler: NUEVA_MEMORIA_DISPONIBLE (%d bytes libres)", memoria_libre_size);
}

t_memory_stick_info* encontrar_stick_por_dir_fisica(uint32_t dir_fisica){
    for(int i = 0; i < list_size(lista_memory_sticks); i++){
        t_memory_stick_info* stick = list_get(lista_memory_sticks, i);
        if(dir_fisica >= stick->base_acumulada && dir_fisica < stick->base_acumulada + stick->size)
            return stick;
    }
    return NULL;
}

void* hilo_monitor_sticks(void* arg){
    (void)arg;

    struct epoll_event events[64];
    while(1){
        int n = epoll_wait(epoll_fd_sticks, events, 64, -1);//ese -1 es el timeout q seteado en -1 hace q se bloquee hasta q se rompa algun socket de losq  administra el array events
        if (n < 0) {
            log_error(loggerMemory, "epoll_wait error: %s", strerror(errno));
            break;
        }
        for(int i = 0; i < n; i++){
            if (!(events[i].events & EPOLLRDHUP)) continue;
            int fd = events[i].data.fd;

            pthread_mutex_lock(&memoria_mutex);
            for (int j = 0; j < list_size(lista_memory_sticks); j++){
                t_memory_stick_info* s = list_get(lista_memory_sticks, j);
                if (s->socket == fd) {
                    list_remove_and_destroy_element(lista_memory_sticks, j, free);
                    break;
                }
            }
            pthread_mutex_unlock(&memoria_mutex);

            log_warning(loggerMemory, "Memory Stick desconectado (FD %d) - memoria corrupta", fd);

            t_paquete* aviso = crear_paquete(MEMORIA_CORRUPTA, NULL);
            enviar_paquete(socketSchedulerNotif, aviso);
            eliminar_paquete(aviso);

            close(fd);
            abort();
        }
    }
    return NULL;
}

op_code leer_pedazos(t_list* pedazos, char* buffer_out){
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
        if (respuesta == NULL) return LECTURA_FALLIDA;
        buffer_read(respuesta->buffer, buffer_out + bytes_ya_procesados, pedazo->tamanio_a_leer_en_esta_memory_stick);
        eliminar_paquete(respuesta);
        bytes_ya_procesados += pedazo->tamanio_a_leer_en_esta_memory_stick;
    }
    return LEER_BYTES;
}

op_code escribir_pedazos(t_list* pedazos, char* datos) {
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
        if (respuesta == NULL) return ESCRITURA_FALLIDA;
        eliminar_paquete(respuesta);
        bytes_ya_procesados += pedazo->tamanio_a_leer_en_esta_memory_stick;
    }
    return ESCRIBIR_BYTES;
}