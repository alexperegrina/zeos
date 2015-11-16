#ifndef UTILS_H
#define UTILS_H

void copy_data(void *start, void *dest, int size);
int copy_from_user(void *start, void *dest, int size);
int copy_to_user(void *start, void *dest, int size);

#define VERIFY_READ	0
#define VERIFY_WRITE	1

//comprueba que no salgamos del proceso por el puntero pasado, ya que es posible
//poner un valor aleatorio de direccion de memoria y salir del proceso.
int access_ok(int type, const void *addr, unsigned long size);


#define min(a,b)	(a<b?a:b)

unsigned long get_ticks(void);

#endif
