//Necesitamos la inclusion porque sino no podemos definir (list_head,task_struct)
//como dentro de shed.h esta declarado type.h no hace falta declara type.h
#include <sched.h>

// variables
extern long long int zeos_ticks;
extern struct list_head freequeue;
extern struct list_head readyqueue;
extern struct task_struct * idle_task;
extern TSS tss;
