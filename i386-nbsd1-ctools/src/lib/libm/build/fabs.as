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

	.file "fabs.s"
#ifdef __PIC
	.link_pix
#endif
#ifdef __PID
	.pid
#endif

/*
 *  _fabsf(x)
 *  returns the absolute value of the real arg x 
 *
 
 *  Input:	single x
 *  Output:	absolute value of x
 */

	.globl	_fabsf
_fabsf:
	clrbit	31,g0,g0
	ret
/*
 *  _fabs(x) -
 *  returns the absolute value of the arg x 
 *
 
 *  Input:	double x
 *  Output:	absolute value of x
 */
	.globl _fabs
_fabs:
	clrbit	31,g1,g1
	ret

/*
 *  _fabsl(x)
 *  returns the absolute value of the extended_real arg x 
 *
 
 *  Input:	extended_real x
 *  Output:	absolute value of x
 */

	.globl	_fabsl
_fabsl:
	clrbit	15,g2,g2
	ret


