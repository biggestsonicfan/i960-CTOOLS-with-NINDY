/*******************************************************************************
 * 
 * Copyright (c) 1993 Intel Corporation
 * 
 * Intel hereby grants you permission to copy, modify, and distribute this
 * software and its documentation.  Intel grants this permission provided
 * that the above copyright notice appears in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  In addition, Intel grants this permission provided that
 * you prominently mark as "not part of the original" any modifications
 * made to this software or documentation, and that the name of Intel
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software or the documentation without specific,
 * written prior permission.
 * 
 * Intel Corporation provides this AS IS, WITHOUT ANY WARRANTY, EXPRESS OR
 * IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY
 * OR FITNESS FOR A PARTICULAR PURPOSE.  Intel makes no guarantee or
 * representations regarding the use of, or the results of the use of,
 * the software and documentation in terms of correctness, accuracy,
 * reliability, currentness, or otherwise; and you rely on the software,
 * documentation and results solely at your own risk.
 *
 * IN NO EVENT SHALL INTEL BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
 * LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
 * OF ANY KIND.  IN NO EVENT SHALL INTEL'S TOTAL LIABILITY EXCEED THE SUM
 * PAID TO INTEL FOR THE PRODUCT LICENSED HEREUNDER.
 * 
 ******************************************************************************/


/*
 *	Power
 *
 * $Header: /ffs/p1/dev/src/lib/libm/common/RCS/pow.c,v 1.2 1993/11/16 17:33:55 alex Exp $
 * $Locker:  $
 */
#include	<fpsl.h>
#include	<errno.h>
#include        <math.h>
#include	"_fpsl.h"
#include	"generic.h"

#ifdef SINGLE
#define	POW_OV_LIM	128.0
#define	POW_UN_LIM	-151.0
#define	do_mul	        _Ls_do_mul
#define ipow            ipowf
#define rpow            rpowf
#endif

#ifdef LIBM
#define	POW_OV_LIM	1024.0
#define	POW_UN_LIM	-1076.0
#define do_mul	        _Ld_do_mul
#define ipow            ipow
#define rpow            rpow
#endif

#ifdef DOUBLEP
#define	POW_OV_LIM	1024.0
#define	POW_UN_LIM	-1076.0
#define do_mul	        _Ld_do_mul
#define ipow            ipow
#define rpow            rpow
#endif

#ifdef EXTENDED
#define	POW_OV_LIM	16384.0
#define	POW_UN_LIM	-16447.0
#define	do_mul	        _Le_do_mul
#define ipow            ipowl
#define rpow            rpowl
#endif

extended	_Ls_do_mul(extended x, extended y);

extended	_Ld_do_mul(extended x, extended y);

extended	_Le_do_mul(extended x, extended y);

void		s_out(extended x);

void		d_out(extended x);

void		e_out(extended x);

/*
 * POW(X,Y)
 * RETURNS X RAISED TO THE POWER OF Y
 *
 * Algorithm:
 *	if y is an positive integral value < pow_int_limit
 *	  compute x**y by calling ipow(a,b);
 *	if y is a negative integral value, with abs(y) < pow_int_limit
 *	  compute (1/(x**-y)) or ((1/x)**-y), if the former results in an
 *	  an error, by calling ipow(a,b);
 *	if (y is an integral value >= pow_int_limit or a non-integral real
 *	  value) compute x**y by calling rpow(a,b); 
 *
 *	ipow(r,i):	(r**i, with i positive integer value)
 *		c := i mod 2;
 *		i := i div 2;
 *		while (c == 0) do
 *		  r := r*r;
 *		  c := i mod 2;
 *		  i := i div 2;
 *		end;
 *		result := r;
 *		while (i <> 0) do
 *		  r := r*r;
 *		  c := i mod 2;
 *		  i := i div 2;
 *		  if (c <> 0) 
 *		    result := result*r;
 *		end;
 *		return(result);
 *
 *	rpow(p,q):
 *		p**q = 2**(qlog2_p)
 *		     = 2**(i+f) = 2**i * [(2**f-1) + 1]
 *		     = scale7(i) * (exp7(f) + 1)
 *
 * Special Cases:
 * 	(anything) ** 0 is 1;
 *	(anything) ** 1 is itself;
 *	(anything) ** NaN is NaN;
 *	NaN ** (anything except 0) is NaN;
 *	+-(anything > 1) ** +inf is +inf;  (** CHECK for x < -1 **)
 *	+-(anything > 1) ** -inf is +0;
 *	+-(anything < 1) ** +inf is +0;
 *	+-(anything < 1) ** -inf is +inf;
 *	+-1 ** +-inf is NaN and signal INVALID;
 *	+0 ** +(anything except 0, NaN) is +0;
 *	-0 ** +(anything except 0, NaN, odd int) is +0;
 *	+0 ** -(anything except 0, NaN) is +inf and signal DIV-BY-ZERO
 *		(** CHECK if signal makes sense... **)
 *	-0 ** -(anything except 0, NaN, odd int) is +inf with signal;
 *	-0 ** (odd integer) = -( +0 ** (oddinteger));
 *	+inf ** +(anything except 0, NaN) is +inf;
 *	+inf ** -(anything except 0, NaN) is +0;
 *	-inf ** (odd integer) = -( +inf ** (odd integer));
 *	-inf ** (even integer) = ( +inf ** (even integer));
 *	-inf ** -(anything except integer, NaN) is SNaN;
 *	-(any other x) ** (k=integer) = (-1)**k * (x**k);
 *	-(anything except 0) ** (non-integer) is SNaN;
 *
 */

#if defined(SINGLE)
static float	ipow(float x, float y);
static float	rpow(float x, float y);
#endif

#if defined(DOUBLEP) || defined(LIBM)
static double	ipow(double x, double y);
static double	rpow(double x, double y);
#endif

#if defined(EXTENDED)
static long double	ipow(long double x, long double y);
static long double	rpow(long double x, long double y);
#endif

GENERIC
(pow)(GENERIC x,GENERIC y)
{
	unsigned int env;
	GENERIC  t;

	GENERIC  two = 2.0;
	int sign;
	short yc;

	yc = fp_class(y);
	env = fp_setenv(FP_DEFENV);

	if ((yc & 0x7) == FP_QNAN) 
		return fp_faultexit(y,FPX_INVOP, env, FPSL_POW);

	if ((yc & 0x7) == FP_INF) { /* y=+/-inf */
		t = fp_abs(x);
		if (t==1.0)	/* return NaN and signal INV_OP */
			return fp_faultexit((yc==FP_INF)?negqnan:qnan, FPX_INVOP, env, FPSL_POW);
		else if (t>1.0)  /* x > 1.0 or x < -1.0 */
			return fp_exit((yc==FP_INF)?y:0.0,env,FPSL_POW);
		 else if (t==0.0) /* x = +0e00 or x = -0e00 */ 
			return fp_exit(x,env,FPSL_POW);
		 else  /* -1.0 < x < 0.0 or 0.0 < x < 1.0  */
		/*
		 * If x=+/-0.0 and y = -inf, return +inf with DIV_BY_ZERO
		 * exception.
		 */
			return fp_exit((yc==FP_N_INF)?(1.0/t):0.0,env,FPSL_POW);
	}

	if ((x == 0.0) && (y <= 0.0))
		errno = EDOM;
	env = fp_setenv(FP_DEFENV);
	if (y==0.0)
		return fp_quickexit(1.0,env,FPSL_POW);
	else if (y==1.0) {
		if (fp_isnan(x))
			return fp_nan1(x,env,FPSL_POW);
		else
			return fp_quickexit(x,env,FPSL_POW);
	     }	
	else if (fp_isnan(y))
		return fp_nan1(y,env,FPSL_POW);
	     else if (fp_isnan(x))
		return fp_nan1(x,env,FPSL_POW);
	
	sign = fp_copysign(1.0,x);
	if (fp_round(y)==y)	/* y is integral value */
/*
 * For integral values of the exponent y < pow_int_limit (here 64), it is
 * more effecient to compute by repeated multiplication instead of logarithms.
 * We'll need to determine this cut-off value by estimating the individual
 * execution times; for now, we use the value used by CEL.
 */
		if (fp_abs(y) < pow_int_limit)  /* compute by repeated mult */
			if (sign==1)  /* sign of x is + */
				return fp_exit(ipow(x,y),env,FPSL_POW);
			else  /* sign of x is - */
		/* check for odd/even int */
				if (fp_abs(fp_rem(y,two)) == 1.0) {  /* odd power */
#ifndef ACTBUG
				   register long double r;
				   r = ipow(-x,y);
				   r = -r;
			           return fp_exit(r,env,FPSL_POW);
#else	
				   return fp_exit(-ipow(-x,y),env,FPSL_POW);
#endif
				}
				else  /* even power */
			           return fp_exit(ipow(-x,y),env,FPSL_POW);
		else  /* if y >= pow_int_limit, compute using log */ 
			if (sign==1)  /* sign of x is + */
				return fp_exit(rpow(x,y),env,FPSL_POW);
			else  /* sign of x is - */
		  /* check for odd/even int */
				if (fp_abs(fp_rem(y,two)) == 1.0) {  /* odd power */
#ifndef ACTBUG
			 	    register long double r;	
				    r = rpow(-x,y);
				    r = -r;	
				    return fp_exit(r,env,FPSL_POW);
#else
				    return fp_exit(-rpow(-x,y),env,FPSL_POW);
#endif
				}
				else  /* even power */
				    return fp_exit(rpow(-x,y),env,FPSL_POW);
	else  {	/* y is not an integral value */

	/*
	 * Note that if we have come here, the INEXACT flag would have
	 * been set by the previous round operation, since y is not an
	 * integral value. This has to be cleared.
	 */
		fp_clrflags(FPX_INEX);
		if (sign == 1)  /* sign of x is + */
			return fp_exit(rpow(x,y),env,FPSL_POW);
		else	/* sign of x is - */
			if (x==0.0)  /* x is -0 */
        /* if y > 0, return +0, else return +inf with signal */
			    return fp_exit((y>0.0)?-x:1.0/-x,env,FPSL_POW);
	/*
	 * Note: 1.0/0.0 will return +inf with the DIV_BY_ZERO flag set.
	 */
			else {	/* x < 0 , return NaN with signal */
			    errno = EDOM;
			    return fp_faultexit(qnan,FPX_INVOP,env,FPSL_POW);
			}
		}

}	/** End of POW(X,Y) **/

/*
 * The following function computes pow(x,y) for finite integral values 
 * of y that are less than pow_int_limit; the sign of x is assumed to be
 * positive. The computation is done by repeated multiplication, the 
 * number of operations being minimized by successive squaring.
 */

static GENERIC
ipow(GENERIC x,GENERIC y)
{
	long double r;
	extended do_mul();
	short xc;

#ifdef DEBUG

#ifdef SINGLE
	printf("**ipow: x = ");
	s_out(x);
	printf("**ipow: y = ");
	s_out(y);
#endif

#ifdef LIBM
	printf("**ipow: x = ");
	d_out(x);
	printf("**ipow: y = ");
	d_out(y);
#endif

#ifdef DOUBLEP
	printf("**ipow: x = ");
	d_out(x);
	printf("**ipow: y = ");
	d_out(y);
#endif

#ifdef EXTENDED
	printf("**ipow: x = ");
	e_out(x);
	printf("**ipow: y = ");
	e_out(y);
#endif

#endif
	xc = fp_class(x);
	if (x==0.0 || ((xc & 0x7) == FP_INF))  /* if x = +0 or x = +inf */
		return (y>0.0)?x:(1.0/x);
	/*
	 * Note: For (0.0^-i), the above returns +inf with DIV_BY_ZERO flag
	 * set.
	 */

	if (y < 0.0) {	/* negative integer: compute 1/(x**-y)..*/
		r = do_mul( (extended) x, (extended) -y);
		r = 1.0/r;
#ifdef DEBUG
	printf("**ipow: y < 0.0\n");
	printf("**ipow: r = 1/x**-y using do_mul is: ");
	e_out(r);
#endif
		return (GENERIC)r;
	}
	else	/* y > 0 */
		return (GENERIC)do_mul( (extended) x, (extended) y);

}	/** End of IPOW(X,Y) **/


/*
 * The following function does the actual computation of pow(x,y) for
 * positive x and integral values of y (less than pow_int_limit). It is
 * called by IPOW(x,y) which makes sure that y is positive.
 * This calculation is forced in extended_precision for better accuracy.
 */

/*static*/ extended
do_mul(x,y)
extended x,y;
{
	register extended  r;
	int  iy=(int) y,c;

	c = iy % 2;	/* iy mod 2 */
	iy = iy/2;	/* iy div 2 */
	while (c==0) {
		x = x*x;
		c = iy % 2;
		iy = iy/2;
	}
	r = x;
	while (iy != 0) {
		x = x*x;
		c = iy % 2;
		iy = iy/2;
		if (c != 0)
			r = r*x;
	}
	return r;

}	/** End of DO_MUL(X,Y) **/


/*
 * The following function computes pow(x,y) for finite y (real and integral
 * values greater than pow_int_limit) and positive x. This computation is
 * done by using the logarithm and exponential functions.
 */

static GENERIC
rpow(GENERIC x,GENERIC y)
{
	register extended t,tr;
	short xc;

	xc = fp_class(x);

	if (x==0.0 || ((xc & 0x7) == FP_INF))  /* if x = +0 or x = +inf */
		return (y>0.0)?x:(1.0/x);
	/*
	 * Note: For (0.0^-r), the above returns +inf with DIV_BY_ZERO flag
	 * set.
	 */

	/*
	 * We do the intermediate computations in extended precision for
	 * better accuracy.
	 */

	t = _Lylog2xl((long double)x, (long double)y);		/* ylog2_x */

	if (t < POW_OV_LIM && t > POW_UN_LIM) {

	/*
	 * The above limit ensures that no overflow/underflow would occur
	 * in the computation below. See "expnote" for insights into how
	 * we deduce this limit.
	 * We make a range check here to avoid two different overflow
	 * conditions.  If y*log2_x overflows to infinity, it will 
	 * precipitate an invalid operation when computing (t - tr).
	 * The other overflow avoided by this check is when tr is too 
	 * large to fit in an integer -- in no case should pow() ever
	 * signal integer overflow.
	 *
	 * Two subtle points here:
	 * (1) Exp2m1() might generate a spurious underflow
	 *     when tr = 0.  The spurious flag must be cleared.
	 *     The true underflow (and overflow) indication
	 *     comes from the scale() operation.
	 * (2) The inexact exception will always be signaled
	 *     because either the multiplication or the round()
	 *     operation (and usually both) will signal inexact.
	 */

		tr = fp_roundl(t);
		t = 1.0 + _Lexp2m1l(t - tr);
		fp_clrflags(FPX_UNFL);
		x = (GENERIC)fp_scalel(t, (int)tr);
		 /* in effect, 2**(ylog2_x) = x^y */
		return x;
	}

	/*
	 * At this point, we know for sure that the result would overflow
	 * to +inf (if t is positive) or underflow to 0.0 (if t is
	 * negative). For the latter case, the OVFL flag should be cleared
	 * before returning the result, since the computation of t in the
	 * prior step could have overflowed. In both cases, the INEX flag
	 * should also be set.
	 */

	if (t > 0.0) {
		errno = ERANGE;
		fp_setflags(FPX_OVFL | FPX_INEX); /* INEX must have been set */
		return (HUGE_VAL);
	}
	else {
		errno = ERANGE;
		fp_clrflags(FPX_OVFL);
		fp_setflags(FPX_UNFL | FPX_INEX); /* INEX must have been set */
		return (0.0);
	}

}	/* End of RPOW(X,Y) */
