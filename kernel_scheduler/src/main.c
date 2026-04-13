#include <utils/hello.h>

int main(int argc, char* argv[]) { //argv[1]: Path al config, argv[2]: path al proceso inicial. [ejs: ./bin/kernel_scheduler kernel.config ./procesos/init.prog]

    t_log* loggerKernel = log_create("kernel.log", "main.c", true, LOG_LEVEL_INFO); //acordarse de cambiar el 2do parametro si cambi el nombre del archivo//Ver si va LOG_LEVEL_INFO o hay que usar lo de las config

    t_config* configKernel = config_create(argv[1]); // para que funcione en el launch.json en la linea 9 puse los parametros de lanzamiento adecuados para que se arme bien el config
    char* puertoEscucha= config_get_string_value(configKernel, "PUERTO_ESCUCHA");
    char* puertoMemory= config_get_string_value(configKernel, "PUERTO_MEMORY");
    char* IPMemory = config_get_string_value(configKernel, "IP_MEMORY");



//LEVANTAR CONEXION CON MEMORY
    int socketConexionMemory = iniciar_conexion(IPMemory, puertoMemory);
    if(socketConexionMemory == EXIT_FAILURE){
        log_info(loggerKernel, "no se pudo conectar a Kernel Memory");
        //ver si hay que abortar
    }

    log_info(loggerKernel, "conexion establecida con Kernel Memory");
    
    handshake_cliente(socketConexionMemory, loggerKernel);
   


//LEVANTAR SERVIDOR
    int socketEscucha = iniciar_servidor(puertoEscucha);
    if(socketEscucha == EXIT_FAILURE){
        log_info(loggerKernel, "No se pudo iniciar el servidor");
        //ver si hay que abortar
    }
    log_info(loggerKernel, "Servidor iniciado");



    //SUPONIENDO QUE CPU SE CONECTA ANTES QUE IO (cuando aprenda hilos lo cambiamos)

    int socketCPU = esperar_cliente(socketEscucha);

    if(socketCPU == EXIT_FAILURE){
        log_info(loggerKernel, "error al conectar CPU");
        //ver si hay que abortar
    }
    log_info(loggerKernel, "CPU <id cpu> CONECTADA"); //agregar despues lo de loggear el id de la cpu 

    handshake_servidor(socketCPU);


    int socketIO = esperar_cliente(socketEscucha);
    if(socketIO == EXIT_FAILURE){
        log_info(loggerKernel, "error al conectar IO");
        //ver si hay que abortar
    }
    log_info(loggerKernel, "IO CONECTADO"); 

    handshake_servidor(socketIO);





    return 0;
}
