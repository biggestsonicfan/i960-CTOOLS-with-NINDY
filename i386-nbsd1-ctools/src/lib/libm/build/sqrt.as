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

	.file "sqrt.s"
	.link_pix
/*
*  sqrtf(x)
*  returns the square-root of x 
*

*  Input:	x is single
*  Output:	sqrt(x)
*/

	.globl	_sqrtf
_sqrtf:
	cmpr	0f0.0,g0
	ble	Llab1
	callj	__errno_ptr
	ldconst	33,g1
	st	g1,(g0)		#errno = EDOM
	movr	0f0.0,g0
	b	Lend1
Llab1:
	sqrtr	g0,g0
Lend1:
	ret

/*
*  _sqrt(x) UNIX libm entry
*

*  Input:	x is double
*  Output:	sqrt(x)
*/

	.globl	_sqrt
_sqrt:
	cmprl	0f0.0,g0
	ble	Llab2
	callj	__errno_ptr
	ldconst	33,g1
	st	g1,(g0)		#errno = EDOM
	movrl	0f0.0,g0
	b	Lend2
Llab2:
	sqrtrl	g0,g0
Lend2:
	ret

/*
*  sqrtl(x)
*  returns the square-root of x 
*

*  Input:	x is extended_real
*  Output:	sqrt(x)
*/

	.globl	_sqrtl
_sqrtl:
	movre	g0,fp0
	cmprl	0f0.0,fp0
	ble	Llab3
	callj	__errno_ptr
	ldconst	33,g1
	st	g1,(g0)		#errno = EDOM
	movre	0f0.0,g0
	b	Lend3
Llab3:
	sqrtrl	fp0,fp0
	movre	fp0,g0
Lend3:
	ret
