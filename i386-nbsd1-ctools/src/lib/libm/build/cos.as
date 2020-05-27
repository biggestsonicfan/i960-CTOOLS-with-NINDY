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


	.file "cos.s"
	.link_pix
/*
*  cosf(x)
*  returns cos(x) 
*
*
*  Input:	x is single
*  Output:	cos(x) in float
*/

	.globl	_cosf
_cosf:
	cosr	g0,g0
	ret

/*
*  cos(x) UNIX libm entry.
*  returns cos(x) 
*
*
*  Input:	x is double
*  Output:	cos(x) in double
*/

	.globl	_cos
_cos:
	cosrl	g0,g0
	ret


/*
*  cosl(x)
*  returns cos(x) in extended 
*
*
*  Input:	x is extended_real
*  Output:	cos(x) in extended real
*/

	.globl	_cosl
_cosl:
	movre	g0,fp0
	cosr	fp0,fp0
	movre	fp0,g0
	ret

