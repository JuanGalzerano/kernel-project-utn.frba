#include "gestion_IO_scheduler.h"


/*YA ESTAN TODOS LOS SEMAFOROS DECLARADOS E INICIALIZADOS Y EL DOCUMENTO CONECTADO


FALTARIA:
    - HACER LOS HILOS POR CADA IO
    - LLAMAR LOS SEMAFOROS EN CADA CASE
    - REVISAR CORRECTO FUNCIONAMIENTO
*/

void* hilo_io_sleep(void* arg){
    while(1){
        sem_wait(&sem_hay_proc_esperando_sleep);
        sem_wait(&sem_sleep_disponible);

        pthread_mutex_lock(&mutex_cola_sleep);
        t_sleep* sleep = queue_pop(cola_sleep);
        pthread_mutex_unlock(&mutex_cola_sleep);

        t_buffer* buffer = serializar_sleep(sleep);

        t_paquete* paquete = crear_paquete(SLEEP, buffer);

        enviar_paquete(socketSleep, paquete);

        free(sleep);
        free(buffer->stream);
        free(buffer);
        free(paquete);
    }

}

void* hilo_io_stdout(void* arg){
    while(1){
        sem_wait(&sem_hay_proc_esperando_stdout);
        sem_wait(&sem_stdout_disponible);

        pthread_mutex_lock(&mutex_cola_stdout);
        t_stdin_stdout* procesoStdout = queue_pop(cola_stdout);
        pthread_mutex_unlock(&mutex_cola_stdout);       


        t_buffer* unBuffer = serializar_stdin(procesoStdout);

        t_paquete* paqueteStdout = crear_paquete(STDOUT, unBuffer);
        enviar_paquete(socketStdout, paqueteStdout);

        pthread_mutex_lock(&mutex_stdout_ocupado);
        stdout_ocupado=true;
        pthread_mutex_unlock(&mutex_stdout_ocupado);

        free(unBuffer->stream);
        free(unBuffer);
        free(paqueteStdout);
        free(procesoStdout->cadenaLeida);
        free(procesoStdout);
    }

}

void* hilo_io_stdin(void* arg){
    while(1){
        sem_wait(&sem_hay_proc_esperando_stdin);
        sem_wait(&sem_stdin_disponible);

        pthread_mutex_lock(&mutex_cola_stdin);
        t_stdin_stdout* procesoStdin = queue_pop(cola_stdin);
        pthread_mutex_unlock(&mutex_cola_stdin);

        t_buffer* unBuffer = serializar_stdin(procesoStdin);

        t_paquete* paqueteStdin = crear_paquete(STDIN, unBuffer);
        enviar_paquete(socketStdin, paqueteStdin);
        free(procesoStdin);
        free(unBuffer->stream);
        free(unBuffer);
        free(paqueteStdin);
    }
}