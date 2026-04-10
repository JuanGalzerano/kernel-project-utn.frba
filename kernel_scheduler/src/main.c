#include <utils/hello.h>

int main(int argc, char* argv[]) { //argv[1]: Path al config, argv[2]: path al proceso inicial. [ejs: ./bin/kernel_scheduler kernel.config ./procesos/init.prog]

    t_log loggerKernel = log_create("kernel.log", "main.c", true, LOG_LEVEL_INFO); //acordarse de cambiar el 2do parametro si cambi el nombre del archivo//Ver si va LOG_LEVEL_INFO o hay que usar lo de las config

    t_config configKernel = config_create(argv[1]);
    char* puertoEscucha= config_get_string_value(configKernel, "PUERTO_ESCUCHA");
    char* puertoMemory= config_get_string_value(configKernel, "PUERTO_MEMORY");
    char* IPMemory = config_get_string_value(configKernel, "IP_MEMORY");


//LEVANTAR SERVIDOR
    int socketEscucha = iniciar_servidor(puertoEscucha);
    if(socketEscucha == EXIT_FAILURE){
        log_info(loggerKernel, "No se pudo iniciar el servidor");
        //ver si hay que abortar
    }
    log_info(loggerKernel, "Servidor iniciado");

    int socketConexion = esperar_cliente(socketEscucha);

    if(socketConexion == EXIT_FAILURE){
        log_info(loggerKernel, "error al conectar cliente");
        //ver si hay que abortar
    }

    //acá agregar para reconocer que se conectó la CPU (se hace con los handshakes) cuando haga lo de los handshakes
    // xq pide de hacer un log especifico para cuando se conecta la CPU



//LEVANTAR CONEXION CON MEMORY
    int socketConexionMemory = iniciar_conexion(IPMemory, puertoMemory);
    if(socketConexionMemory == EXIT_FAILURE){
        log_info(loggerKernel, "no se pudo conectar a Kernel Memory");
        //ver si hay que abortar
    }

    log_info(loggerKernel, "conexion establecida con Kernel Memory");
    //hacer handshake
   


    return 0;
}
