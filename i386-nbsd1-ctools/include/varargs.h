#ifndef __VARARGS_H__
#define __VARARGS_H__

#include <__macros.h>
typedef _va_list va_list;
#define	va_arg(ap,t) _va_arg(ap,t)
#define	va_end(ap)

/* For varargs, va_start has no 2nd parameter;  __builtin_va_alist tells
   the compiler to set up the function for varargs */
#define	va_start(ap) _va_start(ap)
#define	va_alist __builtin_va_alist
#define	va_dcl	 char *__builtin_va_alist;

#endif
