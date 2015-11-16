#include <libc.h>

char buff[24];

int pid;

#define SECS_PER_MIN 60
#define SECS_PER_HOUR 3600


long inner(long n)
{
  int i;
  long suma;
  suma = 0;
  for (i=0; i<n; i++) suma = suma + i;
  return suma;
}
long outer(long n)
{
  int i;
  long acum;
  acum = 0;
  for (i=0; i<n; i++) acum = acum + inner(i);
  return acum;
}


int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

    long count, acum;
    count = 75;
    acum = 0;
    acum = outer(count);
    acum += 0;  //para quitar un warning  warning: variable ‘acum’ set but not used [-Wunused-but-set-variable]


    int pid = getpid();
    int pidh = fork();

     char buff1[10];
     char buff2[10];
     itoa(pid,buff1);
     itoa(pidh,buff2);

     int a = 1;
     int b = 10;
     write(a,buff1,b);
     write(a,buff2,b);

  while(1) {
    }


  return 0;
}
