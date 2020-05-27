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
 *	Natural (base e) logarithm, Common (base 10) logarithm
 *
 * $Header: /ffs/p1/dev/src/lib/libm/common/RCS/log.c,v 1.2 1993/11/16 17:33:55 alex Exp $
 * $Locker:  $
 *
 */

#include	<fpsl.h>
#include	<math.h>
#include	"_fpsl.h"
#include	<errno.h>
#include	"generic.h"

/*
 * Can we make use of the LOGEP instruction, here, for small x?
 * No! The internal implementation of the "normal" logarithm does the
 * argument reduction to a value close to 1.0 before the computation.
 * So, we do not gain anything by checking for the case of x close to
 * 1.0 and using the logep instruction with (x - 1.0) as argument.
 */

GENERIC
(log)(GENERIC x)
{
	unsigned int env;

	env = fp_setenv(FP_DEFENV);

	if (x < 0.0)
		errno = EDOM;
	else if (x == 0.0)
		errno = ERANGE; 
	if (fp_isnan(x))
		return fp_nan1(x, env, FPSL_LOG);

	/*
	 * fp_clog2x is a special entry point defined so that the constant
	 * 'c' (here loge_2) is always in extended precision.
	 */
	return fp_exit(fp_clog2x(x, loge_2), env, FPSL_LOG);

	/*
	 * For x < 0, the P7 logarithm instruction will set the INVALID_OP
	 * flag, since all faults are masked.
	 * The case x = 0 is also handled correctly, since the P7 logarithm
	 * instruction generates a -inf if the DIV-BY-ZERO fault is masked.
	 */

}

GENERIC
(log10)(GENERIC x)
{
	unsigned int env;

	env = fp_setenv(FP_DEFENV);

	if (x < 0.0)
		errno = EDOM;
	else if (x == 0.0)
		errno = ERANGE;
	if (fp_isnan(x))
		return fp_nan1(x, env, FPSL_LOG10);
	/*
	 * fp_clog2x is a special entry point defined so that the constant
	 * 'c' (here log10_2) is always in extended precision.
	 */
	return fp_exit(fp_clog2x(x, log10_2), env, FPSL_LOG10);

	/*
	 * For x < 0, the P7 logarithm instruction will set the INVALID_OP
	 * flag, since all faults are masked.
	 * The case x = 0 is also handled correctly, since the P7 logarithm
	 * instruction generates a -inf if the DIV-BY-ZERO fault is masked.
	 */

}

/*
 * Berkeley's log(1+x) function.
 *
 * We assume the argument is in the required range [sqrt(1/2)-1,sqrt(2)-1].
 * <<Should check this and call regular log function when is out of range.>>
 */
GENERIC
(log1p)(GENERIC x)
{
	unsigned int env;

	env = fp_setenv(FP_DEFENV);

	if (fp_isnan(x))
		return fp_nan1(x, env, FPSL_LOG1P);

	/*
	 * fp_clogep2x is a special entry point defined so that the constant
	 * 'c' (here loge_2) is always in extended precision.
	 */
	return fp_exit(fp_clogep2x(x, loge_2), env, FPSL_LOG1P);
}
