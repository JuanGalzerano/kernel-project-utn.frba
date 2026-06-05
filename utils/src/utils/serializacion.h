#ifndef SERIALIZACION_H
#define SERIALIZACION_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>

typedef struct {
    uint32_t pc;
    uint8_t  ax;
    uint8_t  bx;
    uint8_t  cx;
    uint8_t  dx;
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t si;              // dirección lógica de origen
    uint32_t di;              // dirección lógica de destino
    t_list*  tabla_segmentos; // lista de t_segmento*, NULL cuando no aplica
} t_contexto_ejecucion;


//para la syscall INIT_PROC
typedef struct{
    uint32_t pid;
    char* pathArchivoInstrucciones;
    uint32_t prioridad;
} t_init_proc;

//para la syscall SLEEP
typedef struct{
    uint32_t pid;
    uint32_t tiempoADormir;
}t_sleep;

//para la syscall STDIN
typedef struct{
    uint32_t pid;
    uint32_t bytesALeer;
    uint32_t direccionLogica;
    char* cadenaLeida;
}t_stdin_stdout;

//para la syscall STDOUT
typedef struct 
{
    uint32_t pid;
    uint32_t cantBytesCadena;
    char* mensajeAMostrar;

}t_stdout;

//para las syscalls de MUTEX
typedef struct{
    uint32_t pid;
    char* nombreMutex;
    t_queue* colaEspera;//esta cola es de PCBs en el scheduler, en la CPU, siempre NULL
    int contador;
}t_mutex_syscall;

//para la syscall MEM_ALLOC
typedef struct{
    uint32_t pid;
    uint32_t segmentoId;
    uint32_t tamanio;
}t_mem_alloc;

//para la syscall MEM_FREE
typedef struct{
    uint32_t pid;
    uint32_t segmentoId;
}t_mem_free;

// Segmento de la tabla de segmentos de un proceso (compartido entre scheduler y CPU)
typedef struct {
    uint32_t id_segmento;
    uint32_t base;
    uint32_t limite;
} t_segmento;

typedef struct {
    uint32_t size;
    uint32_t offset;
    void*    stream;
} t_buffer;

typedef enum {
    OBTENER_CONTEXTO    = 0,
    ENVIAR_CONTEXTO     = 1,
    ACTUALIZAR_CONTEXTO = 2,
    OBTENER_INSTRUCCION = 3,
    MOTIVO_FIN_QUANTUM = 4,//no lo uso mas pero es un bardo sacarla y reordenar
    FINALIZAR_POR_QUANTUM = 5,
    EJECUTAR_PROCESO = 6,
    PATH_PROCESO = 7,
    ENVIAR_INSTRUCCION = 8,
    INIT_PROC = 9, //desp agregar el resto de syscalls
    EXIT = 10,
    FIN_PROCESO = 11,
    STDIN = 12,
    STDOUT = 13,
    SLEEP = 14,
    NOOP    = 15,
    SET     = 16,
    MOV_IN = 17,
    MOV_OUT = 18,
    SUM = 19,
    SUB = 20,
    JNZ = 21,
    COPY_MEM = 22,
    MUTEX_CREATE = 23,
    MUTEX_LOCK = 24,
    MUTEX_UNLOCK = 25,
    MEM_ALLOC = 26,
    MEM_FREE = 27,
    FINALIZAR_STDIN = 28, //esto tmb (solo para stdin)
    ESCRIBIR_BYTES = 29,
    LEER_BYTES = 30,
    FINALIZAR_SLEEP = 31,
    FINALIZAR_STDOUT = 32,
    PROCESO_BLOQUEADO = 33,
    COMPACTACION = 34,
    DESALOJO = 35,
    MEMORY_STICK_CONEXION = 36,
    MEMORIA_DISPONIBLE = 37,
    MEMORIA_NO_DISPONIBLE= 38,
    DISPARAR_COMPACTACION = 39,
    SOLICITAR_SEGMENTO = 40,
    PROCESOS_DESALOJADOS = 41,
    LECTURA_FALLIDA = 42,
    MEMORIA_CORRUPTA = 43,
    NUEVA_MEMORIA_DISPONIBLE = 44,
    ESCRITURA_FALLIDA = 45,
    SUSPENDER_PROCESO = 46,
    DESUSPENDER_PROCESO = 47,
    SWAP_INIT = 48,
    LEER_SWAP = 49,
    ESCRIBIR_SWAP = 50,
    SUSPEND_OK = 51,
    DESUSPEND_OK = 52,
    COMPACTACION_EXITOSA = 53,
    DESALOJAR_POR_BSOD = 54,
    SEG_FAULT = 55,
} op_code;
 
typedef struct {
    uint8_t   codigo_operacion;
    t_buffer* buffer;
} t_paquete;




// ----- Buffer -----

t_buffer* buffer_create(uint32_t size);
void      buffer_destroy(t_buffer *buffer);

void      buffer_add(t_buffer *buffer, void *data, uint32_t size);
void      buffer_read(t_buffer *buffer, void *data, uint32_t size);

void      buffer_add_uint32(t_buffer *buffer, uint32_t data);
uint32_t  buffer_read_uint32(t_buffer *buffer);

void      buffer_add_uint8(t_buffer *buffer, uint8_t data);
uint8_t   buffer_read_uint8(t_buffer *buffer);

void buffer_add_int(t_buffer *buffer, int data);
int buffer_read_int(t_buffer *buffer);

// El caller es responsable de liberar el string devuelto por buffer_read_string
void      buffer_add_string(t_buffer *buffer, uint32_t length, char *string);
char*     buffer_read_string(t_buffer *buffer, uint32_t length);

// ----- Paquete -----

t_paquete* crear_paquete(op_code codigo_operacion, t_buffer* buffer);
void       enviar_paquete(int socket, t_paquete* paquete);
t_paquete* recibir_paquete(int socket);
void       eliminar_paquete(t_paquete* paquete);

// ----- Serialización de contexto de ejecución -----

t_buffer*             serializar_contexto_ctx(t_contexto_ejecucion* ctx);
t_contexto_ejecucion* deserializar_contexto_ctx(t_buffer* buf);

// ----- Serializacion SYSCALLS -----
t_buffer* serializar_init_proc(t_init_proc* proc);

t_init_proc* deserializar_init_proc(t_buffer* buf);

t_buffer* serializar_sleep(t_sleep* sleep);

t_sleep* deserializar_sleep(t_buffer* buf);

t_buffer* serializar_stdin(t_stdin_stdout* ProcesoStdin);

t_stdin_stdout* deserializar_stdin(t_buffer* buf);

t_buffer* serializar_mutex(t_mutex_syscall* mutex_struct);

t_mutex_syscall* deserializar_mutex(t_buffer* buf);

t_buffer* serializar_mem_alloc(t_mem_alloc* mem_alloc_struct);

t_mem_alloc* deserializar_mem_alloc(t_buffer* buf);

t_buffer* serializar_mem_free(t_mem_free* mem_free_struct);

t_mem_free* deserializar_mem_free(t_buffer* buf);

#endif // SERIALIZACION_H