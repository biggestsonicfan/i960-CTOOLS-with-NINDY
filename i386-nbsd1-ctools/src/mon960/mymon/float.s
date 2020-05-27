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

	.file	"float.s"

    .globl  _arch
/*#include "hdi_arch.h"*/
	.set    ARCH_SB, 2
	.set    ARCH_KB, 2

/* Offsets into external '_fp_register_set' array
 * of values of individual floating point registers.
 */
	.set	FP_REG_SIZE, 32
	.set	REG_FP0,0
	.set	REG_FP1,32
	.set	REG_FP2,64
	.set	REG_FP3,96

	.set	FP_80BIT, 0
	.set	FP_64BIT, 16
	.set	FP_64BIT_FLAGS, 24

/* Save user floating point registers */
	.globl	save_fpr
save_fpr:
    ld _arch, g0
	cmpibe ARCH_SB, g0, save_fpr_true
	cmpibe ARCH_KB, g0, save_fpr_true
	ret
save_fpr_true:
	lda	_fp_register_set, r3

	lda	REG_FP0(r3), g0
	movre 	fp0, r4
	movrl	fp0, r8
	bal	save_1_fpr

	lda	REG_FP1(r3), g0
	movre 	fp1, r4
	movrl	fp1, r8
	bal	save_1_fpr

	lda	REG_FP2(r3), g0
	movre 	fp2, r4
	movrl	fp2, r8
	bal	save_1_fpr

	lda	REG_FP3(r3), g0
	movre 	fp3, r4
	movrl	fp3, r8
	bal	save_1_fpr

	mov	0, g14
	ret

save_1_fpr:
	/* This routine is called with bal, and shares local registers with
	 * its caller.  It must not use r3. */
	stt	r4, FP_80BIT(g0)

	ldconst	0x001f0000, r4	/* load mask for arithmetic controls */
	modac	r4, 0, r4	/* clear sticky flags */

	stl	r8, FP_64BIT(g0)

	modac	0, 0, r4	/* Get the flags from the AC register */
	extract	16, 5, r4
	stob	r4, FP_64BIT_FLAGS(g0)

	bx	(g14)

	.globl	_update_fpr
_update_fpr:
    ld _arch, g4
	cmpibe ARCH_SB, g4, update_fpr_true
	cmpibe ARCH_KB, g4, update_fpr_true
	ret
update_fpr_true:

	cmpobne	1, g1, 1f

	ldt	(g2), r4

	cmpobne	0, g0, 4f
	movre	r4, fp0
	movrl   fp0, r8
    b 2f
4:
	cmpobne	1, g0, 5f
	movre	r4, fp1
	movrl   fp1, r8
    b 2f
5:
	cmpobne	2, g0, 6f
	movre	r4, fp2
	movrl   fp2, r8
    b 2f
6:
	movre	r4, fp3
	movrl   fp3, r8
	b	2f

1:
	ldl	(g2), r8

	cmpobne	0, g0, 7f
	movrl	r8, fp0
	movre   fp0, r4
    b 2f
7:
	cmpobne	1, g0, 8f
	movrl	r8, fp1
	movre   fp1, r4
    b 2f
8:
	cmpobne	2, g0, 9f
	movrl	r8, fp2
	movre   fp2, r4
    b 2f
9:
	movrl	r8, fp3
	movre   fp3, r4
2:
	ldconst	FP_REG_SIZE, r3
	mulo	r3, g0, g0
	lda	_fp_register_set(g0), g0
	bal	save_1_fpr

	mov	0, g14
	ret


/* Restore user floating point registers */
	.globl	restore_fpr
restore_fpr:
    ld _arch, g0
	cmpibe ARCH_SB, g0, restore_fpr_true
	cmpibe ARCH_KB, g0, restore_fpr_true
	ret
restore_fpr_true:
	lda	_fp_register_set, r3
	ldt	REG_FP0+FP_80BIT(r3), r4
	movre	r4, fp0
	ldt	REG_FP1+FP_80BIT(r3), r4
	movre	r4, fp1
	ldt	REG_FP2+FP_80BIT(r3), r4
	movre	r4, fp2
	ldt	REG_FP3+FP_80BIT(r3), r4
	movre	r4, fp3
	ret
