#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <__macros.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef _wchar_t
#define _wchar_t
typedef char wchar_t;
#endif

#ifndef _size_t
#define _size_t
typedef unsigned size_t;  /* result of sizeof operator */
#endif

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0
#define RAND_MAX     0x7fff
#define MB_CUR_MAX   1

#pragma i960_align __div_t = 8
typedef struct __div_t
{
  int rem;
  int quot;
} div_t;

#pragma i960_align __ldiv_t = 8
typedef struct __ldiv_t
{
  long rem;
  long quot;
} ldiv_t;

#pragma i960_align __ulong_ulong_t = 8
typedef struct __ulong_ulong_t
{
  unsigned long lo;
  unsigned long hi;
} ulong_ulong_t;

/* 4.10.1 String conversion functions */
__EXTERN double		(atof)(__CONST char*);
__EXTERN int		(atoi)(__CONST char*);
__EXTERN long		(atol)(__CONST char*);
__EXTERN double		(strtod)(__CONST char*,char**);
__EXTERN long		(strtol)(__CONST char*,char**,int);
__EXTERN unsigned long 	(strtoul)(__CONST char*,char**,int);

/* 4.10.2 Pseudo-random sequence generation functions */
__EXTERN int		(rand)(__NO_PARMS);
__EXTERN void		(srand)(unsigned int);

/* 4.10.3 Memory management functions */
__EXTERN void*		(calloc)(size_t,size_t);
__EXTERN void		(free)(void*);
__EXTERN void*		(malloc)(size_t);
__EXTERN void*		(realloc)(void*,size_t);

/* 4.10.4 Communication with the environment */
__EXTERN void __VOLATILE_FUNC (abort)(__NO_PARMS);
__EXTERN int 	        (atexit)(void (*)(__NO_PARMS));
__EXTERN void __VOLATILE_FUNC (exit)(int);
__EXTERN char*		(getenv)(__CONST char*);
__EXTERN int		(system)(__CONST char*);

/* 4.10.5 Searching and sorting utilities */
__EXTERN void*		(bsearch)(__CONST void*,__CONST void*,size_t,size_t,int(*)(__CONST void*, __CONST void*) );
__EXTERN void		(qsort)(void*,size_t,size_t,int(*)(__CONST void*, __CONST void*) );

/* 4.10.6 Integer arithmetic functions */
__EXTERN int __CONST_FUNC	(abs)(int);
__EXTERN div_t __CONST_FUNC	(div)(int,int);
__EXTERN long __CONST_FUNC	(labs)(long);
__EXTERN ldiv_t __CONST_FUNC	(ldiv)(long,long);

/* 4.10.7 Multibyte character functions */
__EXTERN int		(mblen)(__CONST char*,size_t);
__EXTERN int		(mbtowc)(wchar_t*,__CONST char*,size_t);
__EXTERN int		(wctomb)(char*,wchar_t);

/* 4.10.8 Multibyte string functions */
__EXTERN size_t		(mbstowcs)(wchar_t*,__CONST char*,size_t);
__EXTERN size_t		(wcstombs)(char*,__CONST wchar_t*,size_t);

/* EMULATION Libraries for MDU operations */
#ifdef __i960P80

__EXTERN int _muli(int,int);
__EXTERN int _modi(int,int);
__EXTERN ldiv_t _ediv(long, ulong_ulong_t);
__EXTERN ulong_ulong_t _emul(unsigned long, unsigned long);   
__EXTERN int __divsi3(int,int);      
__EXTERN unsigned int __udivsi3(unsigned int,unsigned int);
__EXTERN int __modsi(int,int);
__EXTERN unsigned int __umodsi3(unsigned int,unsigned int);

#endif


/* Non-ANSI functions */
#if !defined(__STRICT_ANSI__) && !defined(__STRICT_ANSI)
__EXTERN char *		(ecvt)(double, int, int *, int *);
__EXTERN char *		(fcvt)(double, int, int *, int *);
__EXTERN char *		(gcvt)(double, int, char *);
__EXTERN char *		(itoa)(int, char *, int);
__EXTERN char *		(itoh)(int, char *);
__EXTERN char *		(ltoa)(long, char *, int);
__EXTERN char *		(ltoh)(unsigned long, char *);
__EXTERN char *		(ltos)(long, char *, int);
__EXTERN char *		(ultoa)(unsigned long, char *, int);
__EXTERN char *		(utoa)(unsigned int, char *, int);
__EXTERN int		(getopt)(int, char **, char *);
#endif /* ! __STRICT_ANSI */

#if !defined(__NO_BUILTIN)
#define abs(i)  __builtin_abs(i)
#define labs(i) __builtin_labs(i)
#endif /* ! __NO_BUILTIN */

#if !defined(__NO_INLINE)
#define div(n,d)  __inline_div(n,d)
static __inline div_t __inline_div(int _n, int _d)
{
  div_t _v; _v.quot = _n / _d; _v.rem = _n % _d; return _v;
}

#define ldiv(n,_d) __inline_ldiv(n,_d)
static __inline ldiv_t __inline_ldiv(long _n, long _d)
{
  ldiv_t _v; _v.quot = _n / _d; _v.rem = _n % _d; return _v;
}
#endif /* ! __NO_INLINE */

#endif /* __STDLIB_H__ */
