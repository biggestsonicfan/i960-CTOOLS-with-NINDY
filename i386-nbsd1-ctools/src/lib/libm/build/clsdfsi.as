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
/*      clsdfsi.c - Classify Double Precision Value (Integer Result)	      */
/*		    Routine (AFP-960)					      */
/*									      */
/******************************************************************************/


	.file	"clsdfsi.s"
	.globl	___clsdfsi


#define	cls_Zero	0
#define	cls_Denormal	1
#define	cls_Normal	2
#define	cls_Infinity	3
#define	cls_QNaN	4
#define	cls_SNaN	5
#define	cls_reserved	6


#define	DP_INF     0x7FF

#define	s1         g0
#define	s1_lo      s1
#define	s1_hi      g1

#define	tmp        r10
#define	con_1      r11
#define	tmp2       r12
#define	rsl_tmp    r13

#define	out        g0


	.text
	.link_pix

___clsdfsi:
	cmpo	s1_lo,0			/* Check for low word eq/ne 0 */
	shro	31-3,s1_hi,rsl_tmp	/* Position sign bit */

	subc	1,0,tmp
	addo	s1_hi,s1_hi,tmp2	/* Drop sign bit, shift left one */

	ornot	tmp,tmp2,tmp		/* Exp w/ mant */
	shro	32-11,tmp2,tmp2		/* Position exponent for zero test */

	and	0x8,rsl_tmp,rsl_tmp	/* Select sign bit */
	lda	DP_INF << 21,con_1

	cmpobg.f tmp,con_1,Lcls_10	/* J/ SNaN or QNaN */
	be	 Lcls_20			/* J/ INF	   */

	cmpobe.f 0,tmp2,Lcls_30		/* J/ zero or denormal */

	or	cls_Normal,rsl_tmp,out	/* Sign normal */
	ret


Lcls_10:
	bbc	19,s1_hi,Lcls_15		/* J/ SNaN */

	or	cls_QNaN,rsl_tmp,out
	ret

Lcls_15:
	or	cls_SNaN,rsl_tmp,out
	ret


Lcls_20:
	or	cls_Infinity,rsl_tmp,out
	ret


Lcls_30:
	cmpobne.f 0,tmp,Lcls_35		/* J/ Denormal */

	or	cls_Zero,rsl_tmp,out
	ret

Lcls_35:
	or	cls_Denormal,rsl_tmp,out
	ret
