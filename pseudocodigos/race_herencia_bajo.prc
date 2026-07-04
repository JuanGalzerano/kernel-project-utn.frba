MEM_ALLOC 0 64
MUTEX_LOCK mutex_h
INIT_PROC race_herencia_medio.prc 2
INIT_PROC race_herencia_alto.prc 1
SET AX 50
SET BX 1
SUB AX BX
JNZ AX 1
MUTEX_UNLOCK mutex_h
MEM_FREE 0
EXIT
