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

int sys_fork()
{
  int PID=-1;

  // creates the child process

  return PID;
}

void sys_exit()
{
}

int sys_write(int fd, char * buffer, int size) {
  int error = check_fd(fd, ESCRIPTURA);
  char new_buffer[1024];

  if(error == 0){
    if(buffer == NULL)  return EFAULT;  //bad address
    if( size > -1){
      if(size > 0 ){
        if(size > 1024){
          while(size > 0){
            int i = 0;
            //igualamos a error para quitar un warning en la compilacion.
            error = copy_from_user(&buffer[i], &new_buffer, 1024);
            error = sys_write_console(new_buffer, 1024);
            size = size - 1024;
            i = i + 1024;
            if(error > 0) error = error + 1024;
            else return EINVAL;
          }
        }
        else{
          copy_from_user(buffer, new_buffer, 1024);
          error = sys_write_console(new_buffer, 1024);
        }
      }
    }
    else return EINVAL; //invalid argument
  }
  return error;
}

int sys_gettime() {
	return zeos_ticks;
}
