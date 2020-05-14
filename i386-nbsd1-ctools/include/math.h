#ifndef __MATH_H__
#define __MATH_H__

#include <__macros.h>

#define __M_LN2__	    (( 0.69314718055994530942 ))
#define __M_LOG10_2__	    (( __M_LN2__ * 0.43429448190325182765 ))
#define __M_LOG2E__	    (( 1.4426950408889634074 ))
#define __M_LN_MAXDOUBLE__  (( 2.30258509299404568402 * 308 ))
#define __M_LN_MAXFLOAT__   (( 2.30258509299404568402 * 128 ))

#define __M_INF_LOW__ 0x0L
#define __M_INF_HIGH__ 0x7FF00000L
#define __M_NEG_INF_HIGH__ 0xFFF00000L

#ifndef HUGE_VAL
extern __CONST unsigned int      _d_posinf[2];
#define HUGE_VAL (*((double *)_d_posinf))
#endif

/* 4.5.2 Trigonometric functions */
__EXTERN double (acos)(double);
__EXTERN double (asin)(double);
__EXTERN double (atan)(double);
__EXTERN double (atan2)(double, double);
__EXTERN double (cos)(double);
__EXTERN double (sin)(double);
__EXTERN double (tan)(double);

__EXTERN float  (acosf)(float);
__EXTERN float  (asinf)(float);
__EXTERN float  (atanf)(float);
__EXTERN float  (atan2f)(float, float);
__EXTERN float  (cosf)(float);
__EXTERN float  (sinf)(float);
__EXTERN float  (tanf)(float);

/* 4.5.3 Hyperbolic functions */
__EXTERN double (cosh)(double);
__EXTERN double (sinh)(double);
__EXTERN double (tanh)(double);

__EXTERN float  (coshf)(float);
__EXTERN float  (sinhf)(float);
__EXTERN float  (tanhf)(float);

/* 4.5.4 Exponential and logarithmic functions */
__EXTERN double (exp)(double);
__EXTERN double (frexp)(double, int*);
__EXTERN double (ldexp)(double, int);
__EXTERN double (log)(double);
__EXTERN double (log10)(double);
__EXTERN double (modf)(double, double*);

__EXTERN float  (expf)(float);
__EXTERN float  (logf)(float);
__EXTERN float  (log10f)(float);

/* 4.5.5 Power functions */
__EXTERN double (pow)(double, double);
__EXTERN double (sqrt)(double);

__EXTERN float  (powf)(float, float);
__EXTERN float  (sqrtf)(float);

/* 4.5.6 Nearest integer, absolute value, and remainder functions */
__EXTERN double (ceil)(double);
__EXTERN double (fabs)(double);
__EXTERN double (floor)(double);
__EXTERN double (fmod)(double, double);

__EXTERN float  (ceilf)(float);
__EXTERN float  (fabsf)(float);
__EXTERN float  (floorf)(float);

/* Non-ANSI declarations/definitions */
#if !defined(__STRICT_ANSI) && !defined(__STRICT_ANSI__)

#pragma i960_align complex = 8
struct complex
{
  double x;
  double y;
};

__EXTERN double (square)(double);
__EXTERN double (hypot)(double, double);

#if defined(__NO_FPU)
__EXTERN double (_IEEE_sqrt)(double);
__EXTERN float  (_IEEE_sqrtf)(float);
#endif /* __NO_FPU */

#endif /* ! __STRICT_ANSI */

#if !defined(__NO_BUILTIN)
#define fabs(_x) __builtin_fabs(_x)
#endif

#if !defined(__NO_INLINE)

#define fabsf(_x) __inline_fabsf(_x)
static __inline float __inline_fabsf(float x)
{
  float _t;
  __asm __volatile ("clrbit	31,%1,%0": "=d"(_t) : "dI"(x));
  return _t;
}

#define frexp(_x,y) __inline_frexp(_x,y)
static __inline double __inline_frexp(double _x, int* _exp)
{
#if defined(__i960_BIG_ENDIAN__)
#pragma i960_align __x = 8
  typedef struct __x {
    unsigned long _frac_low;
    unsigned _sign     : 1;
    unsigned _exponent : 11;
    unsigned _frac_hi  : 20;
  } __DOUBLE;
#else
#pragma i960_align __x = 8
  typedef struct __x {
    unsigned long _frac_low;
    unsigned _frac_hi  : 20;
    unsigned _exponent : 11;
    unsigned _sign     : 1; } __DOUBLE;
#endif /* __i960_BIG_ENDIAN__ */
  union {
    double _d;
    __DOUBLE _D; } un;

  if (_x == 0.0)
  { *_exp = 0;
    return 0.0;
  }
  un._d = _x;
  *_exp = un._D._exponent - 1022;	/* extract _exponent,subtract bias */
  un._D._exponent = 1022;		/* _fractional part in range 0.5 .. <1.0 */
  return un._d;
}

#if !defined(__NO_FPU)
/*
 * These are all the routines we define as inlined if we have an
 * onchip FPU.
 */
#define expf(_x) __inline_expf(_x)
#define sinf(_x) __inline_sinf(_x)
#define cosf(_x) __inline_cosf(_x)
#define tanf(_x) __inline_tanf(_x)
#define atanf(_x) __inline_atanf(_x)
#define atan2f(_x,y) __inline_atan2f(_x,y)
#define log10f(_x) __inline_log10f(_x)
#define sqrtf(_x) __inline_sqrtf(_x)
#define logf(_x) __inline_logf(_x)
#define ceilf(_x) __inline_ceilf(_x)
#define floorf(_x) __inline_floorf(_x)

#define exp(x) __inline_exp(x)
#define sin(x) __inline_sin(x)
#define cos(x) __inline_cos(x)
#define tan(x) __inline_tan(x)
#define atan(x) __inline_atan(x)
#define atan2(x,y) __inline_atan2(x,y)
#define log10(x) __inline_log10(x)
#define sqrt(x) __inline_sqrt(x)
#define log(x) __inline_log(x)
#define ldexp(x,y) __inline_ldexp(x,y)
#define ceil(x) __inline_ceil(x)
#define floor(x) __inline_floor(x)

static __inline float __inline_expf(float _x)
{
  float _ret,_ipart,_frac;
  float _fipart;
  unsigned long _scale;
  unsigned long _ac,_rnd=0,_mask=3<<30;

  /* check for overflow & underflow */
#ifndef __NOERRCHECK__
  if (_x > __M_LN_MAXFLOAT__)
    return (__SETERRNO(__ERANGE),HUGE_VAL);
  else if (_x < -__M_LN_MAXFLOAT__)
    return (__SETERRNO(__ERANGE),0.0);
#endif

  /* compute 2 ** (log2(e)*x) */
  _x *= __M_LOG2E__;
  __asm __volatile ("modac %2,%1,%0"    : "=d" (_ac) : "dI" (_rnd),"d" (_mask));
  __asm __volatile ("roundr %1,%0 "    : "=dGH" (_ipart)       : "dGH" (_x));
  _frac = _x - _ipart;
  __asm __volatile ("expr %1,%0"       : "=dGH" (_ret)        : "dGH" (_frac));
  __asm __volatile ("addr 0f1.0,%1,%0" : "=dGH" (_ret)        : "dGH" (_ret));
  _fipart = _ipart;
  __asm __volatile ("cvtri %1,%0"       : "=d"  (_scale)      : "d" (_fipart));
  __asm __volatile ("scaler %2,%1,%0"  : "=dGH" (_ret) :"dGH" (_ret),"d" (_scale));

  /* restore rounding mode.  */
  __asm __volatile ("modac %2,%1,%0"  : "=d" (_ac) : "d" (_ac),"d" (_mask));
  return _ret;
}

static __inline float __inline_sinf(float _x)
{ float _value;
  __asm ("sinr	%1,%0" : "=dGH" (_value) : "dGH" (_x));
  return _value;
}

static __inline float __inline_cosf(float _x)
{ float _value;
  __asm ("cosr	%1,%0" : "=dGH" (_value) : "dGH" (_x));
  return _value;
}

static __inline float __inline_tanf(float _x)
{ float _value;
  __asm ("tanr	%1,%0" : "=dGH" (_value) : "dGH" (_x));
  return _value;
}

static __inline float __inline_atanf(float _x)
{ float _value;
  __asm ("atanr 0f1.0,%1,%0" : "=dGH" (_value) : "dGH" (_x));
  return _value;
}

static __inline float __inline_atan2f(float _y,float _x)
{ float _value;
  
#ifndef __NOERRCHECK__
  if (_y==0.0 && _x==0.0)
    return (__SETERRNO(__EDOM),0.0);
#endif
  __asm ("atanr %2,%1,%0" : "=dGH" (_value) : "dGH" (_y),"dGH" (_x));
  return _value;
}

static __inline float __inline_log10f(float _x)
{
  float _value,_log10=__M_LOG10_2__;

#ifndef __NOERRCHECK__
  if (_x < 0)
    return (__SETERRNO(__EDOM),0.0);
  else if (_x == 0.0)
    return (__SETERRNO(__ERANGE),-HUGE_VAL);
#endif
  __asm ("logr %1,%2,%0" : "=dGH" (_value) : "dGH" (_x),"dGH" (_log10));
  return _value;
}

static __inline float __inline_sqrtf(float _x)
{ float _value;

#ifndef __NOERRCHECK__
  if (_x < 0)
    return (__SETERRNO(__EDOM),0.0);
#endif

  __asm("sqrtr	%1,%0" : "=dGH" (_value) : "dGH" (_x));
  return _value;
}

static __inline float __inline_logf(float _x)
{ float _value,_ln_2=__M_LN2__;

#ifndef __NOERRCHECK__
  if (_x == 0.0)
    return (__SETERRNO(__ERANGE),-HUGE_VAL);
  else if (_x < 0)
    return (__SETERRNO(__EDOM),0.0);
#endif

  __asm ("logr	%1,%2,%0" : "=dGH" (_value) : "dGH" (_x),"dGH" (_ln_2));
  return _value;
}

static __inline float __inline_ceilf(float _x)
{ unsigned long _ac;
  unsigned long _rnd=1<<31,_mask=3<<30;
  float _value;

  /* set the arith. controls to round toward positive infinity.  */
  __asm __volatile ("modac %1,%2,%0" : "=d" (_ac) : "d" (_mask),"d" (_rnd));
  __asm __volatile ("roundr %1,%0"  : "=dGH" (_value) : "dGH" (_x));

  /* restore previous mode.  */
  __asm __volatile ("modac %1,%2,%0" : "=d" (_ac) : "d" (_mask),"d" (_ac));

  return _value;
}

static __inline float __inline_floorf(float _x)
{ unsigned long _ac;
  unsigned long _rnd=0x40000000L,_mask=0xc0000000L;
  float _value;

  /* set the arith. controls to round toward _negative infinity.  */
  __asm __volatile ("modac %1,%2,%0" : "=d" (_ac) : "d" (_mask),"d" (_rnd));
  __asm __volatile ("roundr %1,%0"  : "=dGH" (_value) : "dGH" (_x));

  /* * restore previous mode.  */
  __asm __volatile ("modac %1,%2,%0" : "=d" (_ac) : "d" (_mask),"d" (_ac));

  return _value;
}


static __inline double __inline_exp(double _x)
{ double _ret,_ipart,_frac;
  float _fipart;
  unsigned long _scale;
  unsigned long _ac,_rnd=0,_mask=3<<30;

#ifndef __NOERRCHECK__
  if (_x > __M_LN_MAXDOUBLE__)
    return (__SETERRNO(__ERANGE),HUGE_VAL);
  else if (_x < -__M_LN_MAXDOUBLE__)
    return (__SETERRNO(__ERANGE),0.0);
#endif

  /* compute 2 ** (log2(e)*x) */
  _x *= __M_LOG2E__;
  __asm __volatile ("modac %2,%1,%0"    : "=d" (_ac) : "dI" (_rnd),"d" (_mask));
  __asm __volatile ("roundrl %1,%0 "    : "=dGH" (_ipart)       : "dGH" (_x));
  _frac = _x - _ipart;
  __asm __volatile ("exprl %1,%0"       : "=dGH" (_ret)        : "dGH" (_frac));
  __asm __volatile ("addrl 0f1.0,%1,%0" : "=dGH" (_ret)        : "dGH" (_ret));
  _fipart = _ipart;
  __asm __volatile ("cvtri %1,%0"       : "=d"  (_scale)      : "d" (_fipart));
  __asm __volatile ("scalerl %2,%1,%0"  : "=dGH" (_ret):"dGH" (_ret),"d" (_scale));

  /* restore rounding mode.  */
  __asm __volatile ("modac %2,%1,%0"  : "=d" (_ac) : "d" (_ac),"d" (_mask));
  return _ret;
}

static __inline double __inline_sin(double _x)
{ double _value;
  __asm ("sinrl	%1,%0" : "=dGH" (_value) : "dGH" (_x));
  return _value;
}

static __inline double __inline_cos(double _x)
{ double _value;
  __asm ("cosrl	%1,%0" : "=dGH" (_value) : "dGH" (_x));
  return _value;
}

static __inline double __inline_tan(double _x)
{ double _value;
  __asm ("tanrl	%1,%0" : "=dGH" (_value) : "dGH" (_x));
  return _value;
}

static __inline double __inline_atan(double _x)
{ double _value;
  __asm ("atanrl 0f1.0,%1,%0" : "=dGH" (_value) : "dGH" (_x));
  return _value;
}

static __inline double __inline_atan2(double _y, double _x)
{ double _value;
  
#ifndef __NOERRCHECK__
  if (_y==0.0 && _x==0.0)
    return (__SETERRNO(__EDOM),0.0);
#endif
  __asm ("atanrl %2,%1,%0" : "=dGH" (_value) : "dGH" (_y),"dGH" (_x));
  return _value;
}

static __inline double __inline_log10(double _x)
{ double _value,_log10=__M_LOG10_2__;

#ifndef __NOERRCHECK__
  if (_x < 0)
    return (__SETERRNO(__EDOM),0.0);
  else if (_x == 0.0)
    return (__SETERRNO(__ERANGE),-HUGE_VAL);
#endif
  __asm ("logrl %1,%2,%0" : "=dGH" (_value) : "dGH" (_x),"dGH" (_log10));
  return _value;
}

static __inline double __inline_sqrt(double _x)
{ double _value;

#ifndef __NOERRCHECK__
  if (_x < 0)
    return (__SETERRNO(__EDOM),0.0);
#endif

  __asm("sqrtrl	%1,%0" : "=dGH" (_value) : "dGH" (_x));
  return _value;
}


static __inline double __inline_log(double _x)
{ double _value,_ln_2=__M_LN2__;

#ifndef __NOERRCHECK__
  if (_x == 0.0)
    return (__SETERRNO(__ERANGE),-HUGE_VAL);
  else if (_x < 0)
    return (__SETERRNO(__EDOM),0.0);
#endif

  __asm ("logrl	%1,%2,%0" : "=dGH" (_value) : "dGH" (_x),"dGH" (_ln_2));
  return _value;
}

static __inline double __inline_ldexp(double _x, int _n)
{ union {
    double _value;
    long   _l[2];} _d;

  __asm ("scalerl %2,%1,%0" : "=rGH" (_d._value) : "rGH" (_x),"dI" (_n));
#ifndef __NOERRCHECK__
  if (_d._l[0]==__M_INF_LOW__ && 
      ((_d._l[1]==__M_INF_HIGH__) || (_d._l[1]==__M_NEG_INF_HIGH__)))
    return (__SETERRNO(__ERANGE),HUGE_VAL);
#endif
  return _d._value;
}

static __inline double __inline_ceil(double _x)
{ unsigned long _ac;
  unsigned long _rnd=1<<31,_mask=3<<30;
  double _value;

  /* set the arith. controls to round toward positive infinity.  */
  __asm __volatile ("modac %1,%2,%0" : "=d" (_ac) : "d" (_mask),"d" (_rnd));
  __asm __volatile ("roundrl %1,%0"  : "=dGH" (_value) : "dGH" (_x));

  /* restore previous mode.  */
  __asm __volatile ("modac %1,%2,%0" : "=d" (_ac) : "d" (_mask),"d" (_ac));

  return _value;
}

static __inline double __inline_floor(double _x)
{ unsigned long _ac;
  unsigned long _rnd=0x40000000L,_mask=0xc0000000L;
  double _value;

  /* set the arith. controls to round toward _negative infinity.  */
  __asm __volatile ("modac %1,%2,%0" : "=d" (_ac) : "d" (_mask),"d" (_rnd));
  __asm __volatile ("roundrl %1,%0"  : "=dGH" (_value) : "dGH" (_x));

  /* restore previous mode.  */
  __asm __volatile ("modac %1,%2,%0" : "=d" (_ac) : "d" (_mask),"d" (_ac));

  return _value;
}
#endif /* ! __NO_FPU */
#endif /* ! __NO_INLINE */
#endif /* __MATH_H */
