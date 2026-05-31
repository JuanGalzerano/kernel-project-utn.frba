#include <utils/utils.h>

//VARIABLES 

//FUNCIONES
void escribir_Bloque(uint32_t  idBloque, FILE* archivo, void* contenido, t_config* configSwap ); 
uint32_t leer_Bloque(uint32_t  idBloque, FILE* archivo, t_config* configSwap );