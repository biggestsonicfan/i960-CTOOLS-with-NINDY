/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994 Intel Corporation
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
/*)ce*/

	.file	"ui_float.s"

	.globl	_class_f
_class_f:
	classr	g0
	modac	0, r5, r5
	shro	3, r5, r5
	and	r5, 0x07, g0
	ret

	.globl	_class_d
_class_d:
	classrl	g0
	modac	0, r5,	r5
	shro	3, r5,	r5
	and	r5, 0x07, g0
	ret

	.globl	_class_e
_class_e:
	ldt	(g0), g0
	movre	g0, fp0
	classrl	fp0
	modac	0, r5, r5
	shro	3, r5, r5
	and	r5, 0x07, g0
	ret

	.globl	_re_to_rl
_re_to_rl:
	ldt	(g0), g4
	movre	g4, fp0
	movrl	fp0, g4
	stl	g4, (g1)
	ret
