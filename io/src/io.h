#include <utils/utils.h>

//VARIABLES


//ESTRUCTURAS


//FUNCIONES

char* stdin_func(uint32_t cantBytes, uint32_t pid, t_log* logIO);
void stdout_func(char* mensaje,uint32_t cantBytes,uint32_t pid,t_log* logIO);
void sleep_func(uint32_t cantTiempoMicro,uint32_t pid,t_log* logIO);
char* leer_stdin(uint32_t cantidad_bytes);
tipo_IO reconocer_io(char* tipo);
