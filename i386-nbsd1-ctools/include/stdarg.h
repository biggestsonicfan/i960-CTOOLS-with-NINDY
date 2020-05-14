#ifndef __STDARG_H__
#define __STDARG_H__

#include <__macros.h>
typedef _va_list va_list;
#define	va_arg(ap,t) _va_arg(ap,t)
#define	va_end(ap)

/* We ignore the 2nd parm of va_start;  "..." tells
   the compiler to set up the function for stdarg */
#define	va_start(ap,a) _va_start(ap)

#endif
