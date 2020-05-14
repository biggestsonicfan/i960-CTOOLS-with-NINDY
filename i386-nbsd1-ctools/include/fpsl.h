#ifndef __FPSL_H__
#define __FPSL_H__

#include <__macros.h>
/*
 *	Floating-Point Support Library
 *
 */

/*
 * Rounding Control
 */
#define	FP_RN  0	/* round to nearest, tie -> even */
#define	FP_RM  1	/* round toward minus infinity */
#define	FP_RP  2	/* round toward plus infinity */
#define	FP_RZ  3	/* round toward zero (truncate) */

__EXTERN
int (fp_getround)(__NO_PARMS);	/* return current rounding mode */
__EXTERN
int (fp_setround)(int);  /* set rounding mode, return previous */

/*
 * Exception Control
 */
#define	FPX_INVOP  0x04	/* invalid operation */
#define	FPX_ZDIV   0x08	/* divide-by-zero */
#define	FPX_OVFL   0x01	/* overflow */
#define	FPX_UNFL   0x02	/* underflow */
#define	FPX_INEX   0x10	/* inexact result */
#define	FPX_CLEX   0x00 /* all exception flags are clear */
#define	FPX_ALL	   0x1f	/* all exception flags set */

__EXTERN
int (fp_getflags)(__NO_PARMS);  /* return current exception flags */
__EXTERN
int (fp_setflags)(int);         /* set flags, return previous */
__EXTERN
int (fp_clrflags)(int);         /* clear flags, return previous */
__EXTERN
int (fp_clriflag)(__NO_PARMS);  /* clears INT OVFL flag */
__EXTERN
int (fp_getmasks)(__NO_PARMS);  /* return current exception masks */
__EXTERN
int (fp_setmasks)(int);         /* set masks, return previous */

__EXTERN
unsigned int (_getac)(__NO_PARMS);
__EXTERN
unsigned int (_setac)(unsigned int);

/*
 * Environment
 *
 * This is essentially the 80960 arithmetic controls.
 */
#pragma i960_align _ac = 4
struct	_ac {
#if defined(__i960_BIG_ENDIAN__)
	unsigned int   rndmode : 2; /* rounding mode */
	unsigned int  normmode : 1; /* normalizing mode */
	unsigned int   fpmasks : 5; /* FP exception masks */
	unsigned int           : 3;
	unsigned int   fpflags : 5; /* FP exception flags */
	unsigned int       nif : 1; /* no imprecise faults */
	unsigned int           : 2;
	unsigned int iovfl_msk : 1; /* integer overflow mask */
	unsigned int           : 3;
	unsigned int iovfl_flg : 1; /* integer overflow flag */
	unsigned int           : 1;
	unsigned int        as : 4; /* arithmetic status */
	unsigned int        cc : 3; /* condition code */
#else
	unsigned int        cc : 3; /* condition code */
	unsigned int        as : 4; /* arithmetic status */
	unsigned int           : 1;
	unsigned int iovfl_flg : 1; /* integer overflow flag */
	unsigned int           : 3;
	unsigned int iovfl_msk : 1; /* integer overflow mask */
	unsigned int           : 2;
	unsigned int       nif : 1; /* no imprecise faults */
	unsigned int   fpflags : 5; /* FP exception flags */
	unsigned int           : 3;
	unsigned int   fpmasks : 5; /* FP exception masks */
	unsigned int  normmode : 1; /* normalizing mode */
	unsigned int   rndmode : 2; /* rounding mode */
#endif
};

__EXTERN
unsigned int	fp_getenv(__NO_PARMS);	/* return current environment */
__EXTERN
unsigned int	fp_setenv(unsigned int);/* set environment, return previous */

__EXTERN
float		fp_logbf(float);
__EXTERN
double		fp_logb(double);
__EXTERN
long double	fp_logbl(long double);

__EXTERN
float		fp_roundf(float);		/* round to integral value */
__EXTERN
double		fp_round(double);
__EXTERN
long double	fp_roundl(long double);

__EXTERN
float		fp_remf(float, float);	/* 80960 remainder */
__EXTERN
double		fp_rem(double, double);
__EXTERN
long double	fp_reml(long double, long double);

__EXTERN
float		fp_rmdf(float, float);	/* IEEE remainder */
__EXTERN
double		fp_rmd(double, double);
__EXTERN
long double	fp_rmdl(long double, long double);

__EXTERN
float		fp_scalef(float, int);
__EXTERN
double		fp_scale(double, int);
__EXTERN
long double	fp_scalel(long double, int);

#if !defined(__NO_FPU) && !defined(__NO_INLINE)

#define fp_roundf(x)	_ASM_fp_roundf(x)
#define fp_scalef(x,y)	_ASM_fp_scalef(x,y)
#define fp_clrflags(x)	_ASM_fp_clrflags(x)
#define fp_round(x)	_ASM_fp_round(x)
#define fp_scale(x,y)	_ASM_fp_scale(x,y)
#define fp_roundl(x)	_ASM_fp_roundl(x)

/*
 * float asm functions
 */
static __inline float _ASM_fp_roundf( float x )
{
  float _t;
  __asm ("roundr	%1,%0": "=d"(_t) : "dGH"(x));
  return _t;
}

static __inline float _ASM_fp_scalef( float x, int i)
{
  float _t;
  __asm ("scaler	%2,%1,%0": "=d"(_t) : "dGH"(x), "dI"(i));
  return _t;
}

static __inline short _ASM_fp_clrflags( short flags )
{
  unsigned int _t;
  __asm ("modac	%2,%1,%0": "=d"(_t) : "dI"(0), "dI"(flags<<16));
  return ((_t << 11) >> 27);
}

/*
 * double asm functions
 */

static __inline double _ASM_fp_round( double x )
{
  double _t;
  __asm ("roundrl	%1,%0" : "=t"(_t) : "tGH"(x));
  return _t;
}

static __inline double _ASM_fp_scale ( double x, int i )
{
  double _t;
  __asm ("scalerl        %2,%1,%0": "=t"(_t) : "tGH"(x), "dI"(i));
  return _t;
}

/*
 * long double asm functions
 */
static __inline long double _ASM_fp_roundl (long double x)
{
  long double _t;
  __asm ("roundr	%1,%0": "=f"(_t) : "f"(x));
  return _t;
}

#endif
#endif
