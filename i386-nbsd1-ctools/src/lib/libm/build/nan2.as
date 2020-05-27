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

	.file "nan2.s"
#ifdef __PIC
	.link_pix
#endif
#ifdef __PID
	.pid
#endif
/*
 *  nan2f()
 *  returns the appropriate NaN for a two-argument function
 *

 *  Input: g0 = x, g1 = y, g2 = env, g3 = funct_code
 *  Output: g0: the generated NaN
 */

	.globl	_nan2f
_nan2f:
	setbit	18,g2,g2	/* set INVALID_OP flag */
	lda	0xff1f0000,r4
#if defined(_i960KX)
	modac	r4,g2,g2
#else
	movl	g0,r8
	mov	r4,g0
	mov	g2,g1
	bal	__setac
	movl	r8,g0
#endif
#if defined (_i960XB)
	addr	g0,g1,g0
#else
	call	__fpem_addr_rrr
#endif
	ret		/**  get stub from JimV; ***/

/*
 *  _nan2()
 *  returns the appropriate NaN for a two-argument function
 *

 *  Input: g0,g1 = x, g2,g3 = y, g4 = env, g5 = funct_code
 *  Output: g0,g1: the generated NaN
 */

	.globl	_nan2
_nan2:
	setbit	18,g4,g4/* set INVALID_OP flag */
	lda	0xff1f0000,r4
#if defined(_i960KX)
	modac	r4,g4,g4
#else
	movq	g0,r8
	mov	r4,g0
	mov	g4,g1
	bal	__setac
	movq	r8,g0
#endif
#if defined (_i960XB)
	addrl	g0,g2,g0
#else
	call	__fpem_addrl_rrr
#endif
	ret		/**  get stub from JimV; ***/

/*
 *  nan2l()
 *  returns the appropriate NaN for a two-argument function
 *

 *  Input: g0,g1,g2 = x, g4,g5,g6 = y, g7 = env, g8 = funct_code
 *  Output: g0,g1,g2: the generated NaN (extended_real)
 */

	.globl	_nan2l
_nan2l:
	setbit	18,g7,g7	/* set INVALID_OP flag */
	lda	0xff1f0000,r4
#if defined (_i960KX)
	modac	r4,g7,g7
#else
	movt	g0,r8
	movt	g4,r12
	mov	r4,g0
	mov	g7,g1
	bal	__setac
	movt	r8,g0
	movt	r12,g4
#endif
	movt	g4,r4
	mov	0,g4
	call	__fpem_movre_rf
	movt	r4,g0
	mov	1,g4
	call	__fpem_movre_rf
	mov	0,g0
	mov	1,g1
	mov	0,g2
	call	__fpem_addr_fff
#else
	movre	g0,fp0
	movre	g4,fp1
	addr	fp0,fp1,fp0
#endif
	ret		/**  get stub from JimV; ***/
