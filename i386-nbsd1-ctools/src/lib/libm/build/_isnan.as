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
 *  _Lisnanf(x)
 *  determines if the arg is a NaN (QNaN,SNaN)
 *

 *  Input:	x is single 
 *  Output:	integer (boolean): 1 = true, 0 = false
 */

#include "i960arch.h"

	.file "_isnan.s"
#ifdef __PIC
	.link_pix
#endif
#ifdef __PID
	.pid
#endif
	.globl	__Lisnanf
__Lisnanf:
#if defined (_i960XB)
	cmpr	g0,g0		/* x != x determines a NaN */
#else
	mov	g0,g1
	call	___cmpsf2
#endif
	mov	1,g0
	bno	1f
	mov	0,g0
     1:	ret

/*
 *  _Lisnan(x)
 *

 *  Input:	x is double
 *  Output:	integer (boolean): 1 = true, 0 = false
 */

	.globl	__Lisnan
__Lisnan:
#if defined (_i960XB)
	cmprl	g0,g0		/* x != x determines a NaN */
#else
	movl	g0,g2
	call	___cmpdf2
#endif
	mov	1,g0
	bno	1f
	mov	0,g0
     1:	ret

/*
 *  _Lisnanl(x)
 *  determines if the arg is a NaN (QNaN,SNaN)
 *

 *  Input:	x is extended_real
 *  Output:	integer (boolean): 1 = true, 0 = false
 */

	.globl	__Lisnanl
__Lisnanl:
#if defined (_i960XB)
	movre	g0,fp0
	cmpr	fp0,fp0		/* x != x determines a NaN */
#else
	movt	g0,g4
	call	___cmptf2
#endif
	mov	1,g0
	bno	1f
	mov	0,g0
     1:	ret
