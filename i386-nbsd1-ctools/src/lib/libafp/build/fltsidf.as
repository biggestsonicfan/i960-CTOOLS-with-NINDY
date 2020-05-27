/*******************************************************************************
 * 
 * Copyright (c) 1993,1994 Intel Corporation
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
/*      fltsidf.c - Integer to Double Precision Conversion Routine (AFP-960)  */
/*									      */
/******************************************************************************/

#include "asmopt.h"

#if	!defined(KEEP_INTERNAL_LABELS)
#define	sidf_rejoin	L_sidf_rejoin
#define	sidf_05		L_sidf_05
#define	sidf_10		L_sidf_10
#endif


	.file	"fltsidf.as"
	.globl	___floatunssidf
	.globl	___floatsidf


#define	DP_Bias    0x3FF

#define	s1         g0

#define	s2_mant_lo r4
#define	s2_exp     r6
#define s2_sign    r7

#define	tmp        r10
#define	con_1      r11
#define	tmp2       r13

#define	ac         r15

#define	out        g0
#define	out_lo     out
#define	out_hi     g1


	.text
	.link_pix


	.align	MAJOR_CODE_ALIGNMENT

___floatunssidf:
	mov	0,s2_sign		/* Positive result only */
	movldar(s1,s2_mant_lo)
	b	sidf_rejoin


___floatsidf:
	shri	31,s1,tmp		/* -1 or 0 based on sign */
	shlo	31,tmp,s2_sign		/* sign of result */

	xor	tmp,s1,s2_mant_lo
	subo	tmp,s2_mant_lo,s2_mant_lo

sidf_rejoin:
	scanbit	s2_mant_lo,tmp		/* Find MS bit */

	subo	tmp,31,tmp2		/* Shift count to left justify	*/
	bno.f	sidf_05			/* J/ zero value */

#if	!defined(USE_CMP_BCC)
	cmpo	20,tmp
#endif

	addlda(1,tmp2,tmp2)		/* Shift to left just + 1	*/

	shlo	tmp2,s2_mant_lo,out_hi	/* Shift dropping MS bit	*/

#if	defined(USE_LDA_REG_OFF)
	lda	DP_Bias(tmp),s2_exp	/* Result exponent		*/
#else
	lda	DP_Bias,con_1
	addo	tmp,con_1,s2_exp	/* Result exponent */
#endif

#if	defined(USE_CMP_BCC)
	cmpobl	20,tmp,sidf_10		/* J/ bits spill into lo word	*/
#else
	bl.f	sidf_10			/* J/ bits spill into lo word	*/
#endif

	addo	s2_exp,out_hi,out_hi	/* Normalized signif + exp */
        rotate	20,out_hi,out_hi	/* Position exp/significand */
	or	s2_sign,out_hi,out_hi	/* Mix in sign */
	movlda(0,out_lo)
	ret


sidf_05:
	ldconst	0,out_hi		/* +0 result (s1 overlaps out_lo) */
	ret


sidf_10:
	shlo	20,out_hi,out_lo	/* Lo word bits			*/
	shlo	20,s2_exp,tmp		/* Position exponent		*/
	addo	s2_sign,tmp,tmp		/* Sign w/ exponent		*/
	shro	12,out_hi,out_hi	/* Hi word bits			*/
	or	tmp,out_hi,out_hi	/* Mix in sign & exponent	*/
	ret
