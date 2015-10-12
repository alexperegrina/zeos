/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>
#include <vars_global.h>

/**
 * Container for the Task array and 2 additional pages (the first and the last one)
 * to protect against out of bound accesses.
 */
union task_union protected_tasks[NR_TASKS+2]
  __attribute__((__section__(".data.task")));

union task_union *task = &protected_tasks[1]; /* == union task_union task[NR_TASKS] */

// ******************************* ESTO PORQUE SI NUNCA ENTRARA
#if 0
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}
#endif

extern struct list_head blocked;

// Estas variables estan en vars_global
struct list_head freequeue;
struct list_head readyqueue;
struct task_struct * idle_task;


/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t)
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t)
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t)
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos];

	return 1;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");

	while(1)
	{
	;
	}
}

void init_idle (void)
{
  //comprobamos que no este vaci la lista
  if(!list_empty(&freequeue)) {
    //cojemos el primer PCB libre para asignarle el idle
    struct list_head * listHead = list_first(&freequeue);

    //cojemos el container del elemento
    //struct task_union * realelement = list_entry(listHead, struct task_union, task.list);
    //V2
    struct task_struct * taskStruct = list_head_to_task_struct(listHead);

    //eliminamos el elemento de la freequeue ya que no esta libre
    list_del(listHead);

    //Asignamos el PID 0 al proceso
    taskStruct->PID = 0;
    //inicializamos la variable global para acceder de una forma facil al idle
    idle_task = taskStruct;

    //inicializamos la estructura task_union
    union task_union * taskUnion = taskStruct;

    //inicializamos la tabla de paginas
    allocate_DIR(taskStruct);


    /* INICIALIZAMOS EL CONTEXTO DE EJECUCION */

    //añadimos en la pila del proceso la direccion de memoria de la funcion
    //que queremos que se ejecute.
    list_add(&cpu_idle, &(taskUnion->stack));

    //añadimos en la pila del proceso el valor 0 (No se para que????)
    list_add(0, &(taskUnion->stack));

    // Creo que esto hace lo mismo que las 2 lineas anterirores pero de
    // forma manual
    //taskUnion->stack[KERNEL_STACK_SIZE-1] = &cpu_idle;
    //taskUnion->stack[KERNEL_STACK_SIZE-2] = 0;

  }

}

void init_task1(void)
{
  //comprobamos que no este vaci la lista
  if(!list_empty(&freequeue)) {
    //cojemos el primer PCB libre para asignarle el idle
    struct list_head * listHead = list_first(&freequeue);

    //cojemos el container del elemento
    //struct task_union * realelement = list_entry(listHead, struct task_union, task.list);
    //V2
    struct task_struct * taskStruct = list_head_to_task_struct(listHead);

    //eliminamos el elemento de la freequeue ya que no esta libre
    list_del(listHead);

    //Asignamos el PID 0 al proceso
    taskStruct->PID = 1;

    //inicializamos la tabla de paginas
    allocate_DIR(taskStruct);

    //Asigna páginas físicas para sostener el espacio de direcciones del usuario
    set_user_pages(taskStruct);

    // hacemos que tss.esp0 apunte abajo de la pila
    tss.esp0 = &taskUnion->stack[KERNEL_STACK_SIZE];

    // Indicamos donde esta la tabla de paginas del proceso y realizamos un flush del TLB
    set_cr3(taskStruct->page_table_entry);

  }
}


void init_sched(){
  //inicializamos freequeue
  INIT_LIST_HEAD(&freequeue);
  //inicializamos readyqueue
  INIT_LIST_HEAD(&readyqueue);

  int i = 0;
  for(i = 0; i < NR_TASKS; i++) {
    // encolamos los procesos libres
    list_add_tail( &(task[i].task.list), &freequeue );
  }
}

struct task_struct* current()
{
  int ret_value;

  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}
