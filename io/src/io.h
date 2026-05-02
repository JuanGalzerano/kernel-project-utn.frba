#include <utils/utils.h>

//VARIABLES


//ESTRUCTURAS


//FUNCIONES

char* STDIN(uint32_t cantBytes, uint32_t pid, t_log* logIO);
void STDOUT(char* mensaje,uint32_t pid,uint32_t cantBytes,t_log* logIO);
void SLEEP(uint32_t cantTiempo,iuint32_t pid, t_log* logIO);
char* leer_stdin(uint32_t cantidad_bytes);
