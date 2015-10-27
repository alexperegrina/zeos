/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>
#include <vars_global.h>
#include <mm_address.h>



extern struct list_head blocked;

// Estas variables estan en vars_global
struct list_head freequeue;
struct list_head readyqueue;
struct task_struct * idle_task;

int nextPID = 2;
int quantumCPU;

/**
 * Container for the Task array and 2 additional pages (the first and the last one)
 * to protect against out of bound accesses.
 */
union task_union protected_tasks[NR_TASKS+2]
  __attribute__((__section__(".data.task")));

union task_union *task = &protected_tasks[1]; /* == union task_union task[NR_TASKS] */

// ******************************* ESTO PORQUE SI NUNCA ENTRARA
//#if 0
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}
//#endif

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
  /*if (list_empty(&freequeue)){
	   return -EPERM;
  }*/

  //cojemos el primer PCB libre para asignarle el idle
  struct list_head * listHead = list_first(&freequeue);

  //cojemos el container del elemento
  //struct task_union * realelement = list_entry(listHead, struct task_union, task.list);
  //V2
  struct task_struct * taskStruct = list_head_to_task_struct(listHead);

  //eliminamos el elemento de la freequeue ya que no esta libre
  list_del(listHead);

  //añadimo el proceso en la cola de ready
  //list_add_tail( &(listHead.task.list), &readyqueue );

  //Asignamos el PID 0 al proceso
  taskStruct->PID = 0;
  //indicamos el valor del quantum
  taskStruct->quantum = DEFAULT_QUANTUM;
  //inicializamos el quantum de la CPU para el proceso init
  quantumCPU = taskStruct->quantum;

  //inicializamos la variable global para acceder de una forma facil al idle
  idle_task = taskStruct;

  //inicializamos la estructura task_union
  union task_union * taskUnion = (union task_union *)taskStruct;

  //inicializamos la tabla de paginas
  allocate_DIR(taskStruct);


  /* INICIALIZAMOS EL CONTEXTO DE EJECUCION */

  //añadimos en la pila del proceso la direccion de memoria de la funcion
  //que queremos que se ejecute.
  //list_add(&cpu_idle, taskUnion->stack);
  taskUnion->stack[KERNEL_STACK_SIZE-1] = (unsigned long)&cpu_idle;

  //añadimos en la pila del proceso el valor 0 (No se para que????)
  //list_add(0, &(taskUnion->stack));
  taskUnion->stack[KERNEL_STACK_SIZE-2] = 0;

  //inicializamos los estado
  taskUnion->task.stats.user_ticks = 0;
  taskUnion->task.stats.system_ticks = 0;
  taskUnion->task.stats.blocked_ticks = 0;
  taskUnion->task.stats.ready_ticks = 0;
  taskUnion->task.stats.elapsed_total_ticks = get_ticks();
  taskUnion->task.stats.total_trans = 0;
  taskUnion->task.stats.remaining_ticks = 0;
  taskUnion->task.actualState = ST_READY;

}

void init_task1(void)
{
  //comprobamos que no este vacia la lista
  /*if (list_empty(&freequeue)){
	   return -EPERM;
  }*/

  //cojemos el primer PCB libre para asignarle el idle
  struct list_head * listHead = list_first(&freequeue);

  //cojemos el container del elemento
  struct task_struct * taskStruct = list_head_to_task_struct(listHead);

  //inicializamos la estructura task_union
  union task_union * taskUnion = (union task_union *)taskStruct;

  //eliminamos el elemento de la freequeue ya que no esta libre
  list_del(listHead);

  //añadimo el proceso en la cola de ready
  //list_add_tail( &(listHead.task.list), &readyqueue );

  //Asignamos el PID 1 al proceso
  taskStruct->PID = 1;
  //indicamos el valor del quantum
  taskStruct->quantum = DEFAULT_QUANTUM;
  //inicializamos el quantum de la CPU para el proceso init
  quantumCPU = taskStruct->quantum;

  //inicializamos la tabla de paginas
  allocate_DIR(taskStruct);

  //Asigna páginas físicas para sostener el espacio de direcciones del usuario
  set_user_pages(taskStruct);

  // hacemos que tss.esp0 apunte abajo de la pila
  tss.esp0 = (DWord)&(taskUnion->stack[KERNEL_STACK_SIZE]);

  // Indicamos donde esta la tabla de paginas del proceso y realizamos un flush del TLB
  set_cr3(taskStruct->dir_pages_baseAddr);

  //inicializamos los estado
  taskUnion->task.stats.user_ticks = 0;
  taskUnion->task.stats.system_ticks = 0;
  taskUnion->task.stats.blocked_ticks = 0;
  taskUnion->task.stats.ready_ticks = 0;
  taskUnion->task.stats.elapsed_total_ticks = get_ticks();
  taskUnion->task.stats.total_trans = 0;
  taskUnion->task.stats.remaining_ticks = 0;
  taskUnion->task.actualState = ST_READY;
}

void init_freequeue(void) {
  //inicializamos freequeue
  INIT_LIST_HEAD(&freequeue);
  int i = 0;
  for(i = 0; i < NR_TASKS; i++) {
    // encolamos los procesos libres
    list_add_tail( &(task[i].task.list), &freequeue );
  }
}

void init_sched(){
  //inicializamos freequeue
  init_freequeue();
  //INIT_LIST_HEAD(&freequeue);

  //inicializamos readyqueue
  INIT_LIST_HEAD(&readyqueue);

  /*int i = 0;
  for(i = 0; i < NR_TASKS; i++) {
    // encolamos los procesos libres
    list_add_tail( &(task[i].task.list), &freequeue );
  }*/

  //inicializamos el quantum para el proceso init
  //quantumCPU = DEFAULT_QUANTUM;
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

// doc: https://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html
void inner_task_switch(union task_union*t){
  tss.esp0 = (DWord) &t->stack[KERNEL_STACK_SIZE];
  set_cr3(t->task.dir_pages_baseAddr);

  struct task_struct *taskCurrent = current();
  //void * old = PH_PAGE(*(int*)(void *)(&taskCurrent));
  //void * new = PH_PAGE((int)&(t->task)); // cojemos la posicion incial del task_union
  void * old = &taskCurrent->kernel_esp;
  void * new = &t->task.kernel_esp;

  __asm__ __volatile__(
    "movl %%ebp,%0;" //guardamos el valor de ebp en la estructura
    "movl %1,%%esp;" //cojemos el nuevo esp del proceso nuevo
    "popl %%ebp;" //desempilamos ebp para poder hacer la llamada de retorno
    "ret;" //volvemos a la funcion desde donde nos han llamado
    :
    : "g" (old), "g" (new)
  );
}

// t: pointer to the task_union of the process that will be executed
// metodo para cambiar un proceso
void task_switch(union task_union*t) {
  __asm__ __volatile__(
	   "pushl %esi;"
     "pushl %edi;"
     "pushl %ebx;"
  );
	inner_task_switch(t);
  __asm__ __volatile__(
	   "popl %esi;"
	   "popl %edi;"
	   "popl %ebx;"
   );
}

/*Funcion para ir modificando la informacion del quantum que lleva procesado la CPU*/
void update_sched_data_rr(void) {
  quantumCPU--;
}

/*
Funcion para decidir si es necesario el cambio del proceso actual
post: 1 --> es necesario cambio de proceso,
      0 --> no es necesario cambio de proceso
*/
int needs_sched_rr(void) {
  /*casos a contemplar para el cambio de proceso:
  1_ El quantum del proceso esta a 0
  2_ Proceso de E/S --> Sale solo del proceso (politica no apropiativa)
  */
  return quantumCPU == 0;

  /************************ PREGUNTAR PROFE ***********************/
  /*El codigo del profe tiene mas logica*/
  /* Porque debemos controlar desde aqui si es el unico proceso para procesar?
  si se hace de la manera que yo propongo no saldra de la CPU y volvera a
  entrar entiendo que sera un overhead por realizar el cambio*/

  /*if((quantumCPU == 0) && (!list_empty(&readyqueue))) return 1;
  if(quantumCPU == 0) quantumCPU = get_quantum(current());
  return 0;*/
}

/*
Función para actualizar el estado de un proceso. Si el estado actual del proceso
no se está ejecutando, entonces esta función elimina el proceso de su cola actual.
Si el nuevo estado del proceso no se está ejecutando, entonces esta función
inserta el proceso en una cola adecuada (por ejemplo, la cola libre o la cola
de listos). Los parámetros de esta función son el task_struct del proceso y la
cola de acuerdo con el nuevo estado del proceso. Si el nuevo estado del proceso
se está ejecutando, el parámetro cola shoud ser NULL.
*/
void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue) {
  if(t->actualState != ST_RUN) list_del(&(t->list));
  if(dst_queue == NULL) t->actualState = ST_RUN;
  else {
    list_add_tail(&t->list, dst_queue);
    if(dst_queue == &readyqueue) {
      // caso 'c' del documento
      update_statistics(t, 1);
      t->actualState = ST_READY;
    }
    else {
      t->actualState = ST_BLOCKED;
    }
  }
}

/*Funcion que selecciona el siguiente proceso a ejecutar, se extrae de
la cola readyqueue*/
void sched_next_rr(void) {
  struct task_struct *newTask;

  if(list_empty(&readyqueue)) {
    newTask = idle_task;
  }
  else {
    //cojemos el primer proceso para ejecutar
    struct list_head * listHead = list_first(&readyqueue);

    //cojemos el container del elemento
    newTask = list_head_to_task_struct(listHead);

    //inicializamos la estructura task_union
    //union task_union * taskUnion = (union task_union *)taskStruct;

    //eliminamos el elemento de la readyqueue ya que se va ha ejecutar
    list_del(listHead);
  }

  //actualizamos el quantum de la CPU
  quantumCPU = get_quantum(newTask);

  // Estado 'd' del documento estadisticas de proceso
  update_statistics(newTask,2);

  // Estado 'b' del documento estadisticas de proceso
  update_statistics(current(),1);

  //cambiamos el estado del proceso
  newTask->actualState = ST_RUN;

  //cambiamos al nuevo proceso
  task_switch((union task_union*)newTask);
}

int get_quantum(struct task_struct *t) {
  return t->quantum;
}

void set_quantum(struct task_struct *t, int new_quantum) {
  t->quantum = new_quantum;
}

void schedule()
{
  update_sched_data_rr();
  if (needs_sched_rr())
  {
    update_process_state_rr(current(), &readyqueue);
    sched_next_rr();
  }
}

/**
Funcion para modificar las estadisticas del proceso
pre: task, task_struck; opt: si 0 == user_ticks, si 1 == system_ticks, si 2 == ready_ticks
**/
/*void update_statistics(struct task_struct *task, int opt) {
  int *increment;
  switch (opt) {
    case 0:
      increment = &task->stats.user_ticks;
      break;
    case 1:
      increment = &task->stats.system_ticks;
      break;
    case 2:
      increment = &task->stats.ready_ticks;
      break;
  }
  increment += get_ticks() - task->stats.elapsed_total_ticks;
  task->stats.elapsed_total_ticks = get_ticks();
}*/
