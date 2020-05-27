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
 *	Arcsine, Arccosine
 *
 * $Header: /ffs/p1/dev/src/lib/libm/common/RCS/acos.c,v 1.2 1993/11/16 17:33:55 alex Exp $
 * $Locker:  $
 *
 */
#include	<fpsl.h>
#include	<math.h>
#include	"_fpsl.h"
#include	"generic.h"
#include	<errno.h>

/*
 * The approximations for arcsine and arccosine are very similar, and both
 * require an accurate computation of sqrt(1.0 - x*x).  The strategy of
 * choice is to compute the approximation entirely in extended precision,
 * and convert back to the destination precision only after the final
 * arctangent is computed.  Note that in some cases the expression
 *	sqrt( (1.0 - x) * (1.0 + x) )
 * is more precise than sqrt(1.0 - x*x).  When, you ask?
 *
 * The idea is to avoid computing (1.0 - x*x) when cancellation can cause
 * the relative error to become significant.  The interesting property of
 * the second sqrt() expression is that either (1.0 - x) or (1.0 + x) is
 * exact for any x where 0.5 <= abs(x) <= 1.0; when this happens the
 * argument to sqrt() has only two rounding errors and no problems with
 * cancellation.  However, when abs(x) <= 0.5, then (1.0 - x*x) will
 * always yield the infinitely precise result rounded to the destination
 * precision.
 */

/*
 * The arccosine is approximated using the mathematical identity
 *	arccos(x) = arctan(sqrt(1.0 - x*x) / x)
 *
 * Note that the numerator is always >= 0, so the atan2() function
 * used in the approximation will return a result in the range
 * [0, Pi].
 */
GENERIC
(acos)(GENERIC x)
{
	unsigned int env;
	register GENERIC px;

	env = fp_setenv(FP_DEFENV);

	px = fp_abs(x);
	if (px <= 1.0) {
		register long double rad;

		if (px <= 0.5) {
			rad = sqrtl(1.0 - x*x);
			fp_clrflags(FPX_UNFL);
	/*
	 * We need to clear the underflow flag which could be set
	 * for denormalized and very small x. This was detected by
	 * one of the test vectors...
	 */
		}
		else
			rad = sqrtl( (1.0 - x) * (1.0 + x) );
		x = fp_ratan2(rad, x);
		return fp_exit(x, env, FPSL_ACOS);
	}

	/* <<< is it legal for compiler to optimize cmpr out? >>> */
	if (fp_isnan(px))
		return fp_nan1(x, env, FPSL_ACOS);

	/*
	 * Domain error: abs(x) > 1.0
	 */
	errno = EDOM;
	return fp_faultexit(fp_copysign(qnan,x), FPX_INVOP, env, FPSL_ACOS);
}
