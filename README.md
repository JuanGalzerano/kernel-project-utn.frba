# Kernel distribuido — TP Sistemas Operativos (UTN FRBA)

Trabajo práctico de la cátedra de Sistemas Operativos de UTN FRBA.

El proyecto es un **kernel didáctico distribuido**: en vez de correr como un
único proceso monolítico, el kernel está partido en varios módulos
independientes que se comunican **exclusivamente por sockets TCP**. Cada módulo
es un ejecutable propio y puede correr en una máquina distinta de la red, así
que lo que en un sistema operativo real sería una llamada a función acá es un
mensaje serializado que viaja por la red.

Sobre ese kernel corre un **lenguaje pseudo-assembler propio**. Los "procesos"
del sistema son archivos `.prc` escritos en ese lenguaje, que la CPU interpreta
instrucción por instrucción siguiendo el ciclo clásico *fetch → decode →
execute → check interrupt*.

El TP apunta a tres ejes:

- **Sockets y serialización.** Todo el sistema se articula sobre un protocolo
  binario propio (`op_code` + buffer serializado) definido en `utils`. Las
  estructuras de datos (contexto de ejecución, tabla de segmentos, syscalls)
  viajan serializadas entre módulos.
- **Concurrencia.** Cada módulo es multihilo: el Scheduler levanta un hilo por
  cliente conectado, más hilos dedicados al planificador, al timer de quantum y
  a cada dispositivo de I/O. La sincronización se resuelve con **mutex,
  semáforos contadores y variables de condición** (`pthread_mutex_t`, `sem_t`,
  `pthread_cond_t`) — evitar deadlocks, race conditions y espera activa es parte
  central del trabajo.
- **Administración de memoria.** Memoria por **segmentación** con asignación
  dinámica (Best Fit / Worst Fit), **compactación** de huecos y **swap** a
  disco, sobre un espacio físico que además está distribuido en varios módulos
  `memory_stick`.

## Arquitectura

| Módulo | Rol |
|---|---|
| `kernel_scheduler` | Planificación de procesos: modelo de 7 estados, colas multinivel, algoritmos FIFO / RR / CMN (colas multinivel por prioridad), desalojo, suspensión por timeout, atención de syscalls y gestión de mutex de usuario. |
| `kernel_memory` | Administra el espacio de direcciones: tabla de segmentos por proceso, lista de huecos libres, Best/Worst Fit, compactación, contextos de ejecución y lectura de los pseudocódigos. |
| `memory_stick` | Memoria física propiamente dicha. Se levantan 1..N instancias; cada una aporta un bloque de memoria contigua y `kernel_memory` las presenta a la CPU como un espacio único. Simula latencia de acceso (`MEMORY_DELAY`). |
| `swap` | Área de intercambio en disco. Guarda y recupera segmentos de procesos suspendidos en un archivo dividido en bloques de tamaño fijo. |
| `cpu` | Ciclo de instrucción y MMU (traducción de dirección lógica a física). Se levantan 1..N instancias, cada una con su identificador. |
| `io` | Dispositivos de entrada/salida. Se levanta una instancia por tipo: `SLEEP`, `STDIN`, `STDOUT`. |
| `utils` | Biblioteca estática compartida: sockets, handshake, protocolo, serialización y MMU. La usan todos los módulos. |
| `pseudocodigos` | Archivos `.prc` con los programas de prueba. |

Quién se conecta con quién:

- `kernel_scheduler` → `kernel_memory` (canal principal + canal de notificaciones)
- `cpu` → `kernel_memory`, `kernel_scheduler`, `memory_stick`
- `memory_stick` → `kernel_memory`
- `swap` → `kernel_memory`
- `io` → `kernel_scheduler`

## Dependencias

Para poder compilar y ejecutar el proyecto, es necesario tener instalada la
biblioteca [so-commons-library] de la cátedra:

```bash
git clone https://github.com/sisoputnfrba/so-commons-library
cd so-commons-library
make debug
make install
```

## Compilación

Cada módulo del proyecto se compila de forma independiente a través de un
archivo `makefile`. Para compilar un módulo, es necesario ejecutar el comando
`make` desde la carpeta correspondiente. El ejecutable resultante se guarda en
la carpeta `bin` del módulo.

> [!IMPORTANT]
> `utils` se compila **primero**: es una biblioteca estática de la que dependen
> todos los demás módulos.

```bash
cd utils && make && cd ..
for m in kernel_memory kernel_scheduler cpu memory_stick swap io; do (cd $m && make) || break; done
```

## Ejecución

El orden importa: `kernel_memory` es servidor de casi todo el sistema, así que
va primero, y `kernel_scheduler` antes que las CPUs. Cada módulo recibe su
archivo de configuración como primer argumento y se ejecuta desde su propia
carpeta.

| # | Módulo | Comando |
|---|---|---|
| 1 | Kernel Memory | `./bin/kernel_memory src/memory.config` |
| 2 | Swap | `./bin/swap src/swap.config` |
| 3 | Memory Stick | `./bin/memory_stick src/memoryStick.config 512` |
| 4 | Kernel Scheduler | `./bin/kernel_scheduler src/kernel.config PMP.prc` |
| 5 | CPU | `./bin/cpu src/cpu.config 1` |
| 6 | IO | `./bin/io src/io.config SLEEP` |

Argumentos extra por módulo:

- **memory_stick**: tamaño en bytes de memoria que aporta esa instancia. Se
  pueden levantar varias (con puertos distintos) para simular memoria
  distribuida.
- **kernel_scheduler**: archivo `.prc` del proceso inicial, resuelto contra el
  `SCRIPTS_BASEPATH` configurado en `memory.config`.
- **cpu**: identificador de la CPU. Se pueden levantar varias con IDs distintos.
- **io**: tipo de dispositivo — `SLEEP`, `STDIN` o `STDOUT`. Una instancia por
  tipo.

Cada módulo escribe su propio log (`memory.log`, `swap.log`, `io.log`,
`MemoryStick.log`, etc.) en el directorio desde el que se lo ejecuta.

## Importar desde Visual Studio Code

Para importar el workspace, debemos abrir el archivo `tp.code-workspace` desde
la interfaz o ejecutando el siguiente comando desde la carpeta raíz del
repositorio:

```bash
code tp.code-workspace
```

Cada módulo trae su `launch.json` con una configuración `run` ya armada, así que
se puede debuggear cualquiera con F5 sin pasar argumentos a mano.

## Guías útiles

- [Cómo interpretar errores de compilación](https://docs.utnso.com.ar/primeros-pasos/primer-proyecto-c#errores-de-compilacion)
- [Cómo utilizar el debugger](https://docs.utnso.com.ar/guias/herramientas/debugger)
- [Cómo configuramos Visual Studio Code](https://docs.utnso.com.ar/guias/herramientas/code)

[so-commons-library]: https://github.com/sisoputnfrba/so-commons-library
