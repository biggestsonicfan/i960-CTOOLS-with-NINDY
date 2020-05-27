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

#include "i960arch.h"

	.file "_nan.s"
#define	_e_nan_bit	30	/* bit 30 of reg g1 */
#define	_d_nan_bit	19	/* bit 19 of reg g1 */
#define	_s_nan_bit	22	/* bit 22 of reg g0 */

#ifdef __PIC
	.link_pix
#endif
#ifdef __PID
	.pid
#endif
/*
 *  __Lnan1f()
 *  returns the appropriate NaN for a one-argument function
 *

 *  Input: g0 = x, g1 = env, g2 = funct_code
 *  Output: g0 = predicted NaN
 */

	.globl	___Lnan1f
___Lnan1f:	
	chkbit	_s_nan_bit,g0	/* check for NaN type */
	be	__Lquickexitf	/* If QNaN, return the same value, exit */
/*
 * At this point, we have a SNaN; if the INVOP mask is set then convert it 
 * to a  QNaN, set the INVOP flag and exit; else, raise FPSL exception
 */
	chkbit	26,g1		/* check if INVOP mask set */
	setbit	18,g1,g1	/* set INVALID_OP flag */
	lda	0xff1f0000,r4
#if defined(_i960XB)
	modac	r4,g1,g1
#else
	mov	g0,r8
	mov	r4,g0
	bal	__setac
	mov	r8,g0
#endif
	setbit	_s_nan_bit,g0,g0	/*convert to a QNaN */
	ret

/*
 *  __Lnan1()
 *  returns the appropriate NaN for a one-argument function
 *

 *  Input: g0,g1 = x, g2 = env, g3 = funct_code
 *  Output: g0,g1 = predicted NaN (double)
 */

	.globl	___Lnan1
___Lnan1:	
	chkbit	_d_nan_bit,g1	/* check for NaN type */
	be	__Lquickexit	/* If QNaN, return the same value, exit */
/*
 * At this point, we have a SNaN; if the INVOP mask is set then convert it 
 * to a  QNaN, set the INVOP flag and exit; else, raise FPSL exception
 */
	chkbit	26,g2		/* check if INVOP mask set */
	setbit	18,g2,g2	/* set INVALID_OP flag */
	lda	0xff1f0000,r4
#if defined(_i960XB)
	modac	r4,g2,g2
#else
	movl	g0,r8
	mov	r4,g0
	mov	g2,g1
	bal	__setac
	movl	r8,g0
#endif
	setbit	_d_nan_bit,g1,g1	/*convert to a QNaN */
	ret

/*
 *  __Lnan1l()
 *  returns the appropriate NaN for a one-argument function
 *

 *  Input: g0,g1,g2 = x, g4 = env, g5 = funct_code
 *  Output: g0,g1,g2 = predicted NaN (extended_real)
 */

	.globl	___Lnan1l
___Lnan1l:	
	chkbit	_e_nan_bit,g1		/* Check for NaN type.. */
	be	__Lquickexitl	/* If QNaN, return the same value, exit */
/*
 * At this point, we have a SNaN; if the INVOP mask is set then convert it 
 * to a  QNaN, set the INVOP flag and exit; else, raise FPSL exception
 */
	chkbit	26,g4		/* check if INVOP mask set */
	setbit	18,g4,g4	/* set INVALID_OP flag */
	lda	0xff1f0000,r4
#if defined(_i960XB)
	modac	r4,g4,g4
#else
	movt	g0,r8
	mov	r4,g0
	mov	g4,g1
	bal	__setac
	movt	r8,g0
#endif
	setbit	_e_nan_bit,g1,g1	/* convert to a QNaN */	
	ret
