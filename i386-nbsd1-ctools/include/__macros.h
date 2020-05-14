#ifndef __MACROS_H__
#define __MACROS_H__

#if defined(__cplusplus)
#define __EXTERN extern "C"
#define __NO_PARMS 
#else
#define __EXTERN extern
#define __NO_PARMS void
#endif

#if defined(__STDC__) || defined(__cplusplus)
#define __CONST const
#define __VOLATILE volatile
#else
#define __CONST __const
#define __VOLATILE __volatile
#endif

#if defined(__STRICT_ANSI__)
#define __CONST_FUNC
#define __VOLATILE_FUNC
#elif defined(__STDC__) || defined(__cplusplus)
#define __CONST_FUNC const
#define __VOLATILE_FUNC volatile
#else
#define __CONST_FUNC __const
#define __VOLATILE_FUNC __volatile
#endif

#if !defined(__i960_KB__)&&!defined(__i960_MC__)&&!defined(__i960_SB__)&&\
    !defined(__i960KB)&&!defined(__i960MC)&&!defined(__i960SB)
#define __NO_FPU
#endif

#if defined(__i960_CA__) || defined(__i960_CF__) ||\
    defined(__i960_JA__) || defined(__i960_JD__) || defined(__i960_JF__) ||\
    defined(__i960_JL__) || defined(__i960_RP__) ||\
    defined(__i960CA)    || defined(__i960CF)    ||\
    defined(__i960JA)    || defined(__i960JD)    || defined(__i960JF) ||\
    defined(__i960JL)    || defined(__i960RP)
#define __SOFT_UNALIGNED_ACCESS
#endif

__EXTERN int* __CONST_FUNC (_errno_ptr)(__NO_PARMS);
#define	__SETERRNO(VAL)	 (*_errno_ptr() = (VAL))
#define __GETERRNO()     (*_errno_ptr())
#define	_SETERRNO(VAL)	 (*_errno_ptr() = (VAL))
#define _GETERRNO()      (*_errno_ptr())
#define __errno          (*_errno_ptr())

#define __EDOM   33
#define __ERANGE 34

/* Support for varargs and stdarg.h, ic960 r4.5 and later.

   To get gcc960 2.2 style varargs and stdargs, uncomment the line below
   which defines __OLD_VARARGS__.

   Note that if you do this, those library routines which take va_list
   parms, such as vsprintf, will not work until the libraries are
   rebuilt with __OLD_VARARGS__ defined.
							tmc 11/10/93
*/

/*#define __OLD_VARARGS__*/

extern int __builtin_argsize();
extern void *__builtin_argptr();

#define __vsiz(t)   (( ((sizeof(t)+3)/4) * 4 ))
#define __vali(t)   (( (unsigned) (__alignof__(*(t *)0)>=4 ? __alignof__(*(t *)0) : 4) ))
#define __vpad(i,t) (( (((i)+__vali(t)-1) /__vali(t)) *__vali(t) +__vsiz(t) ))
#ifdef __i960_BIG_ENDIAN__
#define __vadj(p,t) (sizeof(t)>=4?(p):((p)+4-sizeof(t)))
#else
#define __vadj(p,t) (p)
#endif

#if !defined(__OLD_VARARGS__)
#pragma i960_align __va_list = 8
struct __va_list { char *p; char *regtop; };
typedef struct __va_list _va_list;

#define _va_start(ap)\
(((ap).regtop = ((char*)__builtin_argptr()) + 48),\
 ((ap).p = ((char *)__builtin_argptr())+ (int)__builtin_argsize()))

#define _va_arg(ap,t)\
(((ap).p = (char *) __vpad(((int)(ap).p),t)),				\
									\
 (((ap).p > (ap).regtop && (ap).p - __vsiz(t) < (ap).regtop) ||		\
   (__vsiz(t) > 16 && (ap).p - __vsiz(t) < (ap).regtop)			\
      ? ((ap).p = (ap).regtop + __vsiz(t)) : 0),			\
									\
 (*(t *)(__vadj(((ap).p - __vsiz(t)),t))))
#else
typedef unsigned _va_list[2];	

#define	_va_start(ap)\
(((ap)[1]=(unsigned)__builtin_argsize()),\
 ((ap)[0]=(unsigned)__builtin_argptr()))

#define	_va_arg(ap,t)\
((((ap)[1] <= 48 && (__vpad((ap)[1],t) > 48 || __vsiz(t) > 16))		\
  ?((ap)[1] = 48 + __vsiz(t))						\
  :((ap)[1] = __vpad((ap)[1],t))),					\
									\
  (*((t *)(__vadj(((char*)(ap)[0] + (ap)[1] - __vsiz(t)), t)))))
#endif

#endif /* __MACROS_H__ */
