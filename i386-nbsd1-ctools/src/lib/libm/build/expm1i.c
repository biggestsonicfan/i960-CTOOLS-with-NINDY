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
 * Support routine.
 * Computes t and i where, e^x = (t + 1.0) * 2^i
 * Beware that underflow may be signaled.
 */

#include <fpsl.h>
#include <math.h>
#include "_fpsl.h"

long double
expm1il(x, ip)
register long double x;
register int *ip;
{

	register long double xi;

	x = x * log2_e;

	/*
	 * Could have generated a spurious underflow for tiny x; this has to
	 * be cleared. The correct underflow, if any, resultd from the
	 * _Lexp2m1l computation.
	 */

	fp_clrflags(FPX_UNFL);
	xi = fp_roundl(x);

	*ip = xi;
	fp_clriflag();	/* Clear the INT OVFL flag */
	return _Lexp2m1l(x - xi);

}
