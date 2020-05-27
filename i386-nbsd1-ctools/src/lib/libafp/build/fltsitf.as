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
/*      fltsitf.c - Integer to Extended Precision Conversion Routine	      */
/*		    (AFP-960)						      */
/*									      */
/******************************************************************************/


	.file	"fltsitf.s"
	.globl	___floatunssitf
	.globl	___floatsitf


#define	TP_Bias    0x3FFF

#define s1         g0

#define s2_sign    r7

#define	tmp        r10

#define	out        g0
#define	out_mlo    out
#define	out_mhi    g1
#define out_se     g2


	.text
	.link_pix

___floatunssitf:
	ldconst	0,s2_sign		/* Positive result only */
	b	Lcnv_rejoin


___floatsitf:
	shro	31,s1,s2_sign		/* sign of result */
	shri	31,s1,tmp		/* -1 or 0 based on sign */
	shlo	15,s2_sign,s2_sign

	xor	tmp,s1,s1
	subo	tmp,s1,s1

Lcnv_rejoin:
	scanbit	s1,tmp			/* Find MS bit */
	ldconst	TP_Bias,out_se
	bno	Lcnv_05			/* J/ zero value */

	addo	tmp,out_se,out_se	/* Result exponent */
	or	s2_sign,out_se,out_se
	subo	tmp,31,tmp		/* Normalization shift count */
	shlo	tmp,s1,out_mhi
	ldconst	0,out_mlo
	ret


Lcnv_05:
	movt	0,out			/* +0 result */
	ret
