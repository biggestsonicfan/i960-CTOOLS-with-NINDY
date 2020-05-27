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
 *	Exponential (e^x)
 *
 * $Header: /ffs/p1/dev/src/lib/libm/common/RCS/exp.c,v 1.2 1993/11/16 17:33:55 alex Exp $
 * $Locker:  $
 *
 */

#include	<fpsl.h>
#include	<errno.h>
#include	<math.h>
#include	"_fpsl.h"
#include	"generic.h"

void	e_out(long double x);

/*
 * The exponential function e^x is approximated using the mathematical
 * identity:
 *	e^x = 2^(log2(e) * x)
 * Since the underlying approximation function is 2^f - 1, for abs(f) <= 0.5,
 * the actual algorithm used is
 *	e^x = scale( (2^f - 1) + 1, I)
 * where (log2(e) * x) = I+f and I is the (IEEE) nearest integer to
 * (log2(e) * x).
 */
GENERIC
(exp)(GENERIC x)
{
	unsigned int env;

	switch (fp_class(x)) {

	case FP_ZERO:
	case FP_N_ZERO:
		return 1.0;

	case FP_INF:
		return x;

	case FP_N_INF:
		return 0.0;

	case FP_QNAN:
	case FP_N_QNAN:
	case FP_SNAN:
	case FP_N_SNAN:
		env = fp_setenv(FP_DEFENV);
		return fp_nan1(x, env, FPSL_EXP);

	case FP_NORM:
	case FP_N_NORM:
	case FP_DENORM:
	case FP_N_DENORM:
		break;

	default:
		/* NOTREACHED */
		break;
	}

	/*
	 * We make a range check here to avoid two different overflow
	 * conditions.  If x is very large, then x*log2(e) can overflow
	 * to infinity, which will then precipitate an invalid operation
	 * exception when computing ex-ei.  The other overflow avoided
	 * by this check is when ei is too large to fit in an integer --
	 * in no case should exp() ever signal integer overflow.
	 */
	if (fp_abs(x) < EXP_OV_LIM) {
		register long double ex, ei;

		/*
		 * Two subtle points here:
		 * (1) Exp2m1() might generate a spurious underflow
		 *     when ei = 0.  The spurious flag must be cleared.
		 *     The true underflow (and overflow) indication
		 *     comes from the scale() operation.
		 * (2) The inexact exception will always be signaled
		 *     because either the multiplication or the round()
		 *     operation (and usually both) will signal inexact.
		 */
		ex = x * log2_e;
		ei = fp_roundl(ex);
#ifdef DEBUG
	printf("**exp: ex = ");
	e_out(ex);
	printf("**exp: ei = ");
	e_out(ei);
	x = _Lexp2m1l(ex - ei);
	printf("**exp: _Lexp2m1(ex-ei) = ");
	e_out(x);
	x = x + 1.0;
	printf("**exp: _Lexp2m1(ex-ei) + 1.0 = ");
	e_out(x);
#else
		x = 1.0 + _Lexp2m1l(ex - ei);
#endif
		fp_clrflags(FPX_UNFL);
		return fp_scale(x, (int)ei);
	}

	/*
	 * When x >= EXP_OV_LIM, overflow is certain.
	 * When x <= -EXP_OV_LIM, underflow is certain.
	 */
	env = fp_setenv(FP_DEFENV);
	errno = ERANGE;
	if (x > 0)
		return fp_faultexit(HUGE_VAL, FPX_OVFL|FPX_INEX, env, FPSL_EXP);
	else
		return fp_faultexit(0.0, FPX_UNFL|FPX_INEX, env, FPSL_EXP);
}

/*
 * Berkeley's exp(x-1) function.
 *
 * <<This is largely untested, and was added at the last minute for Alex Liu's
 *   tests.>>
 */
GENERIC
expm1(GENERIC x)
{
	unsigned int env;

	switch (fp_class(x)) {

	case FP_ZERO:
	case FP_N_ZERO:
	case FP_INF:
		return x;

	case FP_N_INF:
		return -1.0;

	case FP_QNAN:
	case FP_N_QNAN:
	case FP_SNAN:
	case FP_N_SNAN:
		env = fp_setenv(FP_DEFENV);
		return fp_nan1(x, env, FPSL_EXP);

	case FP_NORM:
	case FP_N_NORM:
	case FP_DENORM:
	case FP_N_DENORM:
		break;

	default:
		/* NOTREACHED */
		break;
	}

	/*
	 * We make a range check here to avoid two different overflow
	 * conditions.  If x is very large, then x*log2(e) can overflow
	 * to infinity, which will then precipitate an invalid operation
	 * exception when computing ex-ei.  The other overflow avoided
	 * by this check is when ei is too large to fit in an integer --
	 * in no case should exp() ever signal integer overflow.
	 */
	if (fp_abs(x) < EXP_OV_LIM) {
		register long double ex, ei;

		/*
		 * Two subtle points here:
		 * (1) Exp2m1() might generate a spurious underflow
		 *     when ei = 0.  The spurious flag must be cleared.
		 *     The true underflow (and overflow) indication
		 *     comes from the scale() operation.
		 * (2) The inexact exception will always be signaled
		 *     because either the multiplication or the round()
		 *     operation (and usually both) will signal inexact.
		 */
		ex = x * log2_e;

		/*
		 * Here we try to take advantage of the undocumented
		 * chip feature that _Lexp2m1() is accurate for -1 <= x <= 1.
		 */
		if (fabsl(ex) <= 1.0) {
			x = (GENERIC)_Lexp2m1l(ex);
		} else {
			ei = fp_roundl(ex);
#ifdef DEBUG
	printf("**exp: ex = ");
	e_out(ex);
	printf("**exp: ei = ");
	e_out(ei);
	x = _Lexp2m1l(ex - ei);
	printf("**exp: _Lexp2m1(ex-ei) = ");
	e_out(x);
#else
			ex = 1.0 + _Lexp2m1l(ex - ei);
#endif
			fp_clrflags(FPX_UNFL);
			x = (GENERIC)(fp_scalel(ex, (int)ei) - 1.0);
		}
		return x;
	}

	/*
	 * When x >= EXP_OV_LIM, overflow is certain.
	 * When x <= -EXP_OV_LIM, "underflow" to -1 is certain.
	 */
	env = fp_setenv(FP_DEFENV);
	if (x > 0)
		return fp_faultexit(HUGE_VAL, FPX_OVFL|FPX_INEX, env, FPSL_EXPM1);
	else
		return fp_faultexit(-1.0, FPX_INEX, env, FPSL_EXPM1);
}
