#include "compactacion.h"

typedef struct{//estructura para cargar los segmentos si nperder su pid
    uint32_t pid;
    t_segmento* seg;
} t_seg_entry;

//para el sort
bool comparar_base(void* a, void* b){
    return ((t_seg_entry*)a)->seg->base < ((t_seg_entry*)b)->seg->base;
}

t_list* pedazos_fisico(uint32_t dir_fisica, uint32_t tamanio){//crea los pedazos de un segmento para poder leerlos o escribirlos
    t_list* pedazos = list_create();
    t_memory_stick_info* stick = encontrar_stick_por_dir_fisica(dir_fisica);
    if (!stick){ list_destroy(pedazos); return NULL;}

    uint32_t offset = dir_fisica - stick->base_acumulada;
    uint32_t restantes = tamanio;

//normalmente siempre va a ser menor a 0 en la segunda iteracion
//ya que pocas veces va a coincidir que se haya pasado del limite de lstick, el segmento
    while(restantes > 0){
        uint32_t espacio  = stick->size - offset;
        uint32_t a_copiar = espacio < restantes ? espacio : restantes;

        struct_control_mmu* p = malloc(sizeof(struct_control_mmu));
        p->socketMemoryStick = stick->socket;
        p->desde_donde_leer = offset;
        p->tamanio_a_leer_en_esta_memory_stick = a_copiar;
        list_add(pedazos, p);

        restantes -= a_copiar; //resto para verificar que ya haya bastado el stick para leer todo el segmento (mi redaccion es terrible)
        if (restantes > 0){
            stick = buscar_siguiente_stick(lista_memory_sticks, stick); //seteo el sig stick para crear el nuevo pedazo 
            if (!stick){
                list_destroy_and_destroy_elements(pedazos, free);
                return NULL;
            }
            offset = 0;//porque si o si el sig pedazo va a estar en la primer pos del sig stick
        }
    }
    return pedazos;
}

//esta funcion se hizo con mucho cariño
void compactar(){
    pthread_mutex_lock(&procesos_mutex);
    pthread_mutex_lock(&memoria_mutex);

    // 1.Enlisto todos los segmentos en nodos de tipo t_seg_entry
    t_list* todos = list_create();
    for(int i = 0; i < list_size(lista_procesos); i++){
        t_proceso_memory* proc = list_get(lista_procesos, i);
        for (int j = 0; j < list_size(proc->contexto->tabla_segmentos); j++) {
            t_seg_entry* e = malloc(sizeof(t_seg_entry));
            e->pid = proc->pid;
            e->seg = list_get(proc->contexto->tabla_segmentos, j);
            list_add(todos, e);
        }
    }

    //2.Ordeno por base fisica actual(izquierda a derecha)
    list_sort(todos, comparar_base); //no pierdo el pid de cada segmento ya que esta almacenado en t_seg_entry

    //3. Muevo cada segmento al cursor y actualizo su base
    uint32_t cursor = 0;
    for(int i = 0; i < list_size(todos); i++){
        t_seg_entry* e = list_get(todos, i);
        uint32_t tamanio = e->seg->limite;

        if(e->seg->base != cursor){//esto es porque si un segmento ya esta bien posicionado, no tiene sentido leerlo y escribirlo donde esta el cursor
            char* contenido = malloc(tamanio);
            t_list* a_leer = pedazos_fisico(e->seg->base, tamanio);
            if(a_leer==NULL){
                log_error(loggerMemory, "Compactacion: pedazos_fisico NULL en base=%d", e->seg->base);
                free(contenido);
                continue;
            }
            leer_pedazos(a_leer, contenido);
            list_destroy_and_destroy_elements(a_leer, free);
            t_list* a_escribir = pedazos_fisico(cursor, tamanio);
            if(a_escribir ==NULL){
                log_error(loggerMemory, "Compactacion: pedazos_fisico NULL en cursor=%d", cursor);
                free(contenido);
                continue;
            }
            escribir_pedazos(a_escribir, contenido);
            list_destroy_and_destroy_elements(a_escribir, free);
            free(contenido);

            log_info(loggerMemory, "Compactacion - PID %d seg %d: base %d -> %d",
                     e->pid, e->seg->id_segmento, e->seg->base, cursor);
            e->seg->base = cursor; //ACA ES DONDE MODIFICO LA TABLA DE SEGMENTOS, PORQUE e->seg apunta al t_segmento, entonces modifica su dato de "base"
        }
        cursor += tamanio;//MUEVO EL CURSOR PARA QUE EL PROXSEGMENTO ESTE PEGADO AL ANTERIOR
    }

    list_destroy_and_destroy_elements(todos, free);

    //Borro la lista de huecos y agrego el hueco grande que hay al final, en el cual va a estar esa mem dispo continua
    list_destroy_and_destroy_elements(lista_huecos, free);
    lista_huecos =list_create();
    if(cursor < memoria_total_size){//chequea q no haya sido que lit mem total size es == al cursor, en ese caso habria 0 huecos
        t_hueco* hueco  = malloc(sizeof(t_hueco));
        hueco->base = cursor;
        hueco->limite = memoria_total_size - cursor;
        list_add(lista_huecos, hueco);
    }

    pthread_mutex_unlock(&memoria_mutex);
    pthread_mutex_unlock(&procesos_mutex);

    log_info(loggerMemory, "Compactacion finalizada - datos hasta %d, hueco libre: %d bytes", cursor, memoria_total_size - cursor);
}