#ifndef GESTION_IO_SCHEDULER_H_
#define GESTION_IO_SCHEDULER_H_

#include "gestor_scheduler.h"

void* hilo_io_sleep(void* arg);

void* hilo_io_stdout(void* arg);

void* hilo_io_stdin(void* arg);

#endif