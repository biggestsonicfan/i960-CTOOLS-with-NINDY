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
 *  _Lclassf(x)
 *  Classify the real number
 *  Used by other routines to check for NaN's,infinities,etc.
 *
 *  Input:	x is single
 *  Output:	integer result indicating the class
 *			0:	+0
 *			1:	+Denormalized number
 *			2:	+Normal finite number
 *			3:	+infinity
 *			4,12:	QNaN
 *			5,13:	SNaN
 *			6,14:	Reserved operand
 *			8:	-0
 *			9:	-Denormalized number
 *			10:	-Normal finite number
 *			11:	-infinity
 */

#include "i960arch.h"

	.file "_class.s"
#ifdef __PIC
	.link_pix
#endif
#ifdef __PID
	.pid
#endif
	.globl	__Lclassf
__Lclassf:
#if defined(_i960XB)
	classr	g0
	modac	0,0,g0
	shlo	25,g0,g0
	shro	28,g0,g0
#else
	call	___clssfsi
#endif
	ret

/*
 *  _Lclass(x)
 *
 *  Input:	x is double
 *  Output:	integer result indicating the class
 */

	.globl	__Lclass
__Lclass:
#if defined(_i960XB)
	classrl	g0
	modac	0,0,g0
	shlo	25,g0,g0
	shro	28,g0,g0
#else
	call	___clsdfsi
#endif
	ret

/*
 *  _Lclassl(x)
 *
 *  Input:	x is extended_real
 *  Output:	integer result indicating the class
 */

	.globl	__Lclassl
__Lclassl:
#if defined(_i960XB)
	movre	g0,fp0
	classr	fp0
	modac	0,0,g0
	shlo	25,g0,g0
	shro	28,g0,g0
#else
	call	___clstfsi
#endif
	ret
