#include "swap_gestor.h"
#include "inicializar.h"

void inicializar_swap(){
    swap_num_bloques = swap_total_size / swap_block_size;
    tabla_swap = calloc(swap_num_bloques, sizeof(t_bloque_swap));
    pthread_mutex_init(&swap_mutex, NULL);
    log_info(loggerMemory, "Swap inicializado: %d bloques de %d bytes", swap_num_bloques, swap_block_size);
}

int swap_bloque_libre(){
    for (int i = 0; i < (int)swap_num_bloques; i++) {
        if (!tabla_swap[i].ocupado) return i;//osea el primero que tenga True
    }
    return -1;
}

//ESCRIBIR Y LEER BLOQUE

void swap_escribir_bloque(int bloque, char* datos, uint32_t tamanio){
    t_buffer* buf = buffer_create(0);
    buffer_add_uint32(buf, (uint32_t)bloque);
    buffer_add_uint32(buf, tamanio);
    buffer_add(buf, datos, tamanio);
    t_paquete* paq = crear_paquete(ESCRIBIR_SWAP, buf);
    enviar_paquete(socketSwap, paq);
    eliminar_paquete(paq);

    t_paquete* resp = recibir_paquete(socketSwap);
    eliminar_paquete(resp);//damos por hecho la lectura exitosa obligatoria al no tener problasmde desconexion y el memory verifica antes que en ese bloq no haya nada y que entre el segmento
}

char* swap_leer_bloque(int bloque, uint32_t tamanio){
    t_buffer* buf = buffer_create(0);
    buffer_add_uint32(buf, (uint32_t)bloque);
    buffer_add_uint32(buf, tamanio);
    t_paquete* paq = crear_paquete(LEER_SWAP, buf);
    enviar_paquete(socketSwap, paq);
    eliminar_paquete(paq);

    t_paquete* resp = recibir_paquete(socketSwap);
    if (!resp) return NULL;
    char* datos = malloc(tamanio);
    buffer_read(resp->buffer, datos, tamanio);
    eliminar_paquete(resp);
    return datos;
}

//SUSPENDER O DESUSPENDER PROCESO

op_code suspender_proceso(uint32_t pid, uint32_t* bytes_suspendidos){
    *bytes_suspendidos = 0;
    t_proceso_memory* proc = buscar_proceso(pid);
    if(!proc){
        log_error(loggerMemory, "SUSPEND: PID %d no encontrado", pid);
        return MEMORIA_NO_DISPONIBLE;
    }

    pthread_mutex_lock(&memoria_mutex);
    int n_segs = list_size(proc->contexto->tabla_segmentos);
    pthread_mutex_unlock(&memoria_mutex);

    if(n_segs == 0){
        log_info(loggerMemory, "PID %d suspendido (sin segmentos)", pid);
        proc->en_swap = true;
        return SUSPEND_OK;
    }

    pthread_mutex_lock(&memoria_mutex);
    t_segmento** segs = malloc(n_segs * sizeof(t_segmento*));
    for (int i = 0; i < n_segs; i++){
        segs[i] = list_get(proc->contexto->tabla_segmentos, i);
    }
    pthread_mutex_unlock(&memoria_mutex);

    for(int i = 0; i < n_segs; i++){
        uint32_t seg_id  = segs[i]->id_segmento;
        uint32_t tamanio = segs[i]->limite;

        if(tamanio > swap_block_size){
            log_error(loggerMemory, "SUSPEND: PID %d - segmento %d (%d bytes) excede el block size del swap (%d bytes)", pid, seg_id, tamanio, swap_block_size);
            free(segs);
            return MEMORIA_NO_DISPONIBLE;
        }

        pthread_mutex_lock(&swap_mutex);
        int bloque = swap_bloque_libre();
        pthread_mutex_unlock(&swap_mutex);

        if (bloque < 0) {
            log_error(loggerMemory, "SUSPEND: swap lleno, no se puede suspender PID %d", pid);
            free(segs);
            return MEMORIA_NO_DISPONIBLE;
        }

        // Leer segmento de la memory stick
        pthread_mutex_lock(&memoria_mutex);
        t_list* pedazos = traducir_logica_a_fisica(
            seg_id * segment_max_size,
            segment_max_size,
            proc->contexto->tabla_segmentos,
            lista_memory_sticks,
            tamanio
        );
        pthread_mutex_unlock(&memoria_mutex);

        char* datos = malloc(tamanio);
        leer_pedazos(pedazos, datos);
        list_destroy_and_destroy_elements(pedazos, free);

        // Escribir en el bloque del swap y registrar en la tabla
        pthread_mutex_lock(&swap_mutex);
        swap_escribir_bloque(bloque, datos, tamanio);
        tabla_swap[bloque].ocupado = true;
        tabla_swap[bloque].pid = pid;
        tabla_swap[bloque].segmento_id = seg_id;
        tabla_swap[bloque].tamanio = tamanio;
        pthread_mutex_unlock(&swap_mutex);

        free(datos);

        liberar_fisica_segmento(pid, seg_id);
        *bytes_suspendidos += tamanio;

        log_info(loggerMemory, "PID %d - Segmento %d -> Bloque swap %d", pid, seg_id, bloque);
    }

    free(segs);
    proc->en_swap = true;
    log_info(loggerMemory, "PID %d suspendido", pid);
    return SUSPEND_OK;
}

op_code desuspender_proceso(uint32_t pid) {
    pthread_mutex_lock(&swap_mutex);
    int cantidad = 0;
    for (int i = 0; i < (int)swap_num_bloques; i++)
        if (tabla_swap[i].ocupado && tabla_swap[i].pid == pid) cantidad++;

    if (cantidad == 0) {
        pthread_mutex_unlock(&swap_mutex);
        t_proceso_memory* proc0 = buscar_proceso(pid);
        if(proc0){
            proc0->en_swap = false;
        }
        log_info(loggerMemory, "PID %d no tiene segmentos en swap", pid);
        return DESUSPEND_OK;
    }

    typedef struct { int bloque; uint32_t seg_id; uint32_t tamanio; } t_info;//estructura para enlistar los segmentos leidos de los bloques
    t_info* info = malloc(cantidad * sizeof(t_info));
    int k = 0;
    for (int i = 0; i < (int)swap_num_bloques && k < cantidad; i++) {
        if (tabla_swap[i].ocupado && tabla_swap[i].pid == pid) {
            info[k].bloque = i;
            info[k].seg_id = tabla_swap[i].segmento_id;
            info[k].tamanio = tabla_swap[i].tamanio;
            k++;
        }
    }
    pthread_mutex_unlock(&swap_mutex);

    t_proceso_memory* proc = buscar_proceso(pid);
    for(int i = 0; i < cantidad; i++){
        // Leer datos del swap
        char* datos = swap_leer_bloque(info[i].bloque, info[i].tamanio);

        op_code resultado = asignar_fisica_segmento(pid, info[i].seg_id, info[i].tamanio);
        if (resultado != MEMORIA_DISPONIBLE){
            log_error(loggerMemory, "DESUSPEND: no hay memoria para PID %d seg %d", pid, info[i].seg_id);
            free(datos);
            free(info);
            return resultado;
        }
        pthread_mutex_lock(&memoria_mutex);
        t_list* pedazos = traducir_logica_a_fisica(
            info[i].seg_id * segment_max_size,
            segment_max_size,
            proc->contexto->tabla_segmentos,
            lista_memory_sticks,
            info[i].tamanio
        );
        pthread_mutex_unlock(&memoria_mutex);

        escribir_pedazos(pedazos, datos);
        list_destroy_and_destroy_elements(pedazos, free);
        free(datos);

        pthread_mutex_lock(&swap_mutex);
        tabla_swap[info[i].bloque].ocupado = false;
        pthread_mutex_unlock(&swap_mutex);

        log_info(loggerMemory, "PID %d - Bloque swap %d -> Segmento %d", pid, info[i].bloque, info[i].seg_id);
    }

    free(info);
    if(proc) proc->en_swap = false;
    log_info(loggerMemory, "PID %d desuspendido", pid);
    return DESUSPEND_OK;
}