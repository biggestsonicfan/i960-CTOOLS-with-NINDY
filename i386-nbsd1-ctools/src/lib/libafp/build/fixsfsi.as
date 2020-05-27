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
/******************************************************************************/
/*									      */
/*      fixsfsi.c - Single Precision to Integer Conversion Routine (AFP-960)  */
/*									      */
/******************************************************************************/


	.file	"fixsfsi.s"
	.globl	___fixunssfsi
	.globl	___fixsfsi


#define	FP_Bias    0x7F
#define	FP_INF     0xFF

#define	s1         g0

#define	s2_mant    r4
#define	s2_exp     r5
#define s2_sign    r6

#define	tmp        r10
#define	con_1      r11

#define	out        g0


	.text
	.link_pix

___fixunssfsi:
	shlo	1,s1,s2_exp
	ldconst	FP_Bias+32,con_1
	shro	32-8,s2_exp,s2_exp
	cmpobl.t s2_exp,con_1,LLcnv_10	/* J/ in range */
	b	Lcnv_60			/* J/ overflow */


___fixsfsi:
	ldconst	0x80000000+((FP_Bias+31) << 23),tmp
	cmpobg.f s1,tmp,LLcnv_90		/* J/ invalid operation -> neg val */

	shlo	1,s1,s2_exp
	ldconst	FP_Bias+31,con_1
	shro	32-8,s2_exp,s2_exp
	cmpobge.f s2_exp,con_1,Lcnv_80	/* J/ overflow */

LLcnv_10:
	shri	31,s1,s2_sign		/* Sign of integer */

	ldconst	FP_Bias,con_1		/* Compute unbiased power of 2 */
	cmpo	s2_exp,con_1
	subo	con_1,s2_exp,tmp
	bl.f	Lcnv_20			/* J/ magnitude < 1 -> return 0 */

	shlo	8,s1,s2_mant		/* Left justify significand */
	subo	tmp,31,tmp		/* Right shift count */
	setbit	31,s2_mant,s2_mant	/* Explicit "j" bit */
	shro	tmp,s2_mant,out		/* Position value */

	xor	s2_sign,out,out		/* Apply sign as req'd */
	subo	s2_sign,out,out
	ret

Lcnv_20:
	ldconst	0,out			/* Return 0 */
	ret


Lcnv_60:
	shri	31,s1,out		/* rtn 0xFF..FF or 0, based on sign */
	not	out,out
	ret


Lcnv_80:
	shlo	1,s1,tmp
	ldconst	FP_INF << 24,con_1
	cmpobg	tmp,con_1,LLcnv_90	/* J/ invalid operation (NaN) */

	bbs	31,s1,LLcnv_90		/* J/ overflow negative */

	ldconst	0x80000000,out
	subi	1,out,out		/* Integer overflow, +maxint */
	ret

LLcnv_90:					/* Return -maxint */
	ldconst	0x40000000,out
	addi	out,out,out		/* Integer overflow, -maxint */
	ret
