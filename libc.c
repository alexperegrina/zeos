/*
 * libc.c 
 */

#include <libc.h>

#include <types.h>

#include <errno.h>

int errno;

void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}

int write (int fd, char * buffer, int size){
  int result;
  char msg[] = "Este es el mensaje de error";
  _asm_(
    "movw 8(%ebp), %ebx"
    "movw 12(%ebp), %ecx"
    "movw 16(%ebp), %edx"
    "movw $4, %eax"
    "int $0x80"
    "movw %eax, -4(%ebp)"
  );

  if(result < 0) {
    errno = result * -1;
    perrno(msg);
    result = -1;
  }
  
  return result;
}

