/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>
#include <vars_global.h>
#include <utils.h>
#include <io.h>
#include <mm.h>
#include <mm_address.h>
#include <sched.h>
#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1



int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -38; /*ENOSYS*/
}

int sys_getpid()
{
	return current()->PID;
}


int ret_from_fork() {
  return 0;
}

int sys_fork()
{
  
  int PID=-1;
  int i,j;
  /*vector que contendra el conjunto de numeros de paginas libres para el
  nuevo proceso*/
  int frameFree[NUM_PAG_DATA];
  unsigned int frame;

  //comprobamos que no este vacia la lista
  if (list_empty(&freequeue)){
	   return -EPERM;
  }

  /*Comprobamos que hay suficientes frames para en nuevo
  proceso y las reservamos, en caso contrario las volvemos a liberar.
  Solo utilizamos NUM_PAG_DATA=20 ya que seran estas paginas las que se tienes
  que hacer las traducciones de logicas a fisicas*/
  for(i = 0; i < NUM_PAG_DATA; i++) {
    //solicitamos un nuevo frame y lo reservamos, en caso que no haya --> -1
    frameFree[i] = alloc_frame();

    if(frameFree[i] == -1) { //si cumple la condicion es que no hay frame libres
      //hay que restarle 1 al index i porque si no liberamos la memoria
      // de phys_mem[(frameFree[i] == -1)] --> mm.h
      for(j = i-1; j >= 0; j--) {
        free_frame(frameFree[j]);
      }
      return -ENOMEM;
    }
  }

  //  ************************* Buscar PCB libre
  //cojemos un PCB libre
  struct list_head * listHead = list_first(&freequeue);

  //cojemos el container del nuevo PCB (hijo)
  struct task_struct * taskStruct_new = list_head_to_task_struct(listHead);
  //cojemos el container del current (padre)
  struct task_struct * taskStruct_current = current();

  //inicializamos la estructura task_union
  union task_union * taskUnion_new = (union task_union *)taskStruct_new;
  union task_union * taskUnion_current = (union task_union *)taskStruct_current;

  //eliminamos el elemento de la freequeue ya que no esta libre
  list_del(listHead);

  //copiamo la estructura task_union del proceso padre al proceso hijo
  /*ponemos el task_struct mejor con el cast del task_union para simplificar
  el codigo*/
  //copy_data(taskStruct_current, taskStruct_new, sizeof(union task_union));
  copy_data(taskUnion_current, taskUnion_new, sizeof(union task_union));

  //inicializamos la tabla de paginas del hijo
  allocate_DIR(taskStruct_new);

  //  ************************* Asignar PID
  taskStruct_new->PID = nextPID;
  PID = nextPID;
  nextPID++; //preparamos para le siguiente proceso

  //  ************************* Inicializar espacio de direcciones
  page_table_entry *table_current = get_PT(taskStruct_current);
  page_table_entry *table_new = get_PT(taskStruct_new);

  /* Copiamos kernel_code and kernel_data y user_code, este espacio de
  direcciones sera igual para los dos procesos. */
  for(i = 0; i < NUM_PAG_KERNEL+NUM_PAG_CODE; i++) {
    //cojemos el frame del proceso padre
    frame = get_frame(table_current, i);
    // Copiamos el frame al proceso nuevo.
    set_ss_pag(table_new, i, frame);
  }

  /* Copiamos data */
  j = 0;
  for(i = NUM_PAG_KERNEL+NUM_PAG_CODE; i < NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; i++) {
    // asignamos el frame libre al proceso hijo.
    set_ss_pag(table_new, i, frameFree[j]);

    /* asignamos el frame al padre para que tenga acceso a los datos
    Hay que sumarle NUM_PAG_DATA para que las nuevas asignaciones vayan a
    continuacion del bloque de data porque si no perderiamos informacion*/
    set_ss_pag(table_current, i+NUM_PAG_DATA, frameFree[j]);

    /* Como ya tenemos la direccion logica de la pagina --> i, tenemos que
    desplazar a la izquierda 12 bits ya que son los bits de offset para movernos
    dentro de la paguina, ZEOS DOC Pag. 41*/
    /* Se realiza la copia de datos entre frames */
    copy_data((void*)(i<<12), (void*)((i+NUM_PAG_DATA)<<12), PAGE_SIZE);

    // quitamos la relacion entre el proceso padre y el frame del proceso hijo
    del_ss_pag(table_current, i+NUM_PAG_DATA);
  }

  // Indicamos donde esta la tabla de paginas del proceso y realizamos un flush del TLB
  set_cr3(taskStruct_current->dir_pages_baseAddr);

  /* introducimos los valores arriba del todo de la pila ya que no sabemos si al
  momento de copiar los datos hay mas datos por debajo de estas 2 posiciones*/
  /*sabemos que por debajo hay el contexto hardware, el contexto software y la direccion
  de retorno de la system_call*/

  /*
  De la forma que he implementado el enlaze dinamico el S.O. no tiene usabilidad
  ya que si nos cambian la CPU con diferentes registros (mas o menos registros)
  no funcionara correctamente este fragmento de codigo
  */
  //********************** Alex ************************
  // introducimos el valor de retorno para el proceso hijo
  //taskUnion_new->stack[KERNEL_STACK_SIZE-19] = 0;
  // introducimos la direccion de retorno del handler
  //taskUnion_new->stack[KERNEL_STACK_SIZE-18] = (unsigned long)&ret_from_fork;

  // hacemos que el registro del esp de la estructura task_struct apunte a la
  // cima de la pila
  //taskStruct_new->kernel_esp = (unsigned long)&taskUnion_new->stack[KERNEL_STACK_SIZE-19];
  //********************** Fin Alex ************************

  /*
  Con este fragmento de codigo asumimos la usabilidad del S.O.
  */
  //********************** Profe ************************
  int register_ebp;		/* frame pointer */
  /* Map Parent's ebp to child's stack */
  __asm__ __volatile__ (
    "movl %%ebp, %0\n\t"
      : "=g" (register_ebp)
      : );
  register_ebp = (register_ebp - (int)taskStruct_current) + (int)(taskUnion_new);

  taskStruct_new->kernel_esp = register_ebp + sizeof(DWord);
  DWord temp_ebp=*(DWord*)register_ebp;

  /* Prepare child stack for context switch */
  taskStruct_new->kernel_esp-=sizeof(DWord);
  *(DWord*)(taskStruct_new->kernel_esp)=(DWord)&ret_from_fork;
  taskStruct_new->kernel_esp-=sizeof(DWord);
  *(DWord*)(taskStruct_new->kernel_esp)=temp_ebp;

  //********************** Fin Profe ************************

  //inicializamos los estado
  taskUnion_new->task.stats.user_ticks = 0;
  taskUnion_new->task.stats.system_ticks = 0;
  taskUnion_new->task.stats.blocked_ticks = 0;
  taskUnion_new->task.stats.ready_ticks = 0;
  taskUnion_new->task.stats.elapsed_total_ticks = get_ticks();
  taskUnion_new->task.stats.total_trans = 0;
  taskUnion_new->task.stats.remaining_ticks = 0;
  taskUnion_new->task.actualState = ST_READY;

  //  ************************* Encolar PCB en la cola de ready
  //añadimo el proceso en la cola de ready
  list_add_tail( &(taskStruct_new->list), &readyqueue );

  return PID;
}

void sys_exit() {
  int i;
  struct task_struct * taskStruct_current= current();
  page_table_entry* table_current = get_PT((void*)&taskStruct_current);
  for(i = 0; i < NUM_PAG_DATA; i++) {
    //Marcamos como libre el frame seleccionado
    free_frame(get_frame(table_current, PAG_LOG_INIT_DATA+i));
    //Eliminamos asociacion del frame con el proceso actual
    del_ss_pag(table_current, PAG_LOG_INIT_DATA+i);
  }

  /* Añadimos el task_struct a la cola freequeue*/
  list_add_tail(&(current()->list), &freequeue);

  current()->PID=-1;

  // hacemos que entre un nuevo proceso en RUN
  sched_next_rr();
}

int sys_write(int fd, char * buffer, int size) {
  int error = check_fd(fd, ESCRIPTURA);
  char new_buffer[1024];

  if(error == 0){
    if(buffer == NULL || !access_ok(VERIFY_READ, buffer, size))  return EFAULT;  //bad address
    if( size > -1){
      if(size > 0 ){
        if(size > 1024){
          while(size > 1024){
            int i = 0;
            //igualamos a error para quitar un warning en la compilacion.
            error = copy_from_user(&buffer[i], new_buffer, 1024);
            error = sys_write_console(new_buffer, 1024);
            size = size - 1024;
            i = i + 1024;
            if(error > 0) error = error + 1024;
            else return EINVAL;
          }
        }
        if(size > 0) {
          copy_from_user(buffer, new_buffer, size);
          error = sys_write_console(new_buffer, size);
        }
      }
    }
    else return EINVAL; //invalid argument
  }
  return error;
}

/*#define TAM_BUFFER 512

int sys_write(int fd, char *buffer, int nbytes) {
char localbuffer [TAM_BUFFER];
int bytes_left;
int ret;

	if ((ret = check_fd(fd, ESCRIPTURA)))
		return ret;
	if (nbytes < 0)
		return -EINVAL;
	if (!access_ok(VERIFY_READ, buffer, nbytes))
		return -EFAULT;

	bytes_left = nbytes;
	while (bytes_left > TAM_BUFFER) {
		copy_from_user(buffer, localbuffer, TAM_BUFFER);
		ret = sys_write_console(localbuffer, TAM_BUFFER);
		bytes_left-=ret;
		buffer+=ret;
	}
	if (bytes_left > 0) {
		copy_from_user(buffer, localbuffer,bytes_left);
		ret = sys_write_console(localbuffer, bytes_left);
		bytes_left-=ret;
	}
	return (nbytes-bytes_left);
}*/


int sys_gettime() {
	return zeos_ticks;
}

/*
void update_statistics_user(struct task_struct *task) {
  task->stats.user_ticks += get_ticks() - task->stats.elapsed_total_ticks;
  task->stats.elapsed_total_ticks = get_ticks();
}

void update_statistics_system(struct task_struct *task) {
  task->stats.system_ticks += get_ticks() - task->stats.elapsed_total_ticks;
  task->stats.elapsed_total_ticks = get_ticks();
}

void update_statistics_ready(struct task_struct *task) {
  task->stats.ready_ticks += get_ticks() - task->stats.elapsed_total_ticks;
  task->stats.elapsed_total_ticks = get_ticks();
}
*/
/**
Funcion para modificar las estadisticas del proceso
pre: task, task_struck; opt: si 0 == user_ticks, si 1 == system_ticks, si 2 == ready_ticks
**/
void update_statistics(struct task_struct *task, int opt) {
  unsigned long *increment;
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
}

extern int quantumCPU;

int sys_get_stats(int pid, struct stats *st)
{
  int i;

  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) return -EFAULT;

  if (pid<0) return -EINVAL;
  for (i=0; i<NR_TASKS; i++)
  {
    if (task[i].task.PID==pid)
    {
      task[i].task.stats.remaining_ticks=quantumCPU;
      copy_to_user(&(task[i].task.stats), st, sizeof(struct stats));
      return 0;
    }
  }
  return -ESRCH; /*ESRCH */
}
