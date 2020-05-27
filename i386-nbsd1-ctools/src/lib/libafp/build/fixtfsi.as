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
/*      fixtfsi.c - Extended Precision to Integer Conversion Routine	      */
/*		    (AFP-960)						      */
/*									      */
/******************************************************************************/


	.file	"fixtfsi.s"
	.globl	___fixunstfsi
	.globl	___fixtfsi


#define	TP_Bias    0x3FFF
#define	TP_INF     0x7FFF

#define	s1         g0
#define	s1_mlo     s1
#define	s1_mhi     g1
#define s1_se      g2

#define s2_exp     r6
#define s2_sign    r7

#define	tmp        r10
#define	con_1      r11
#define	tmp2       r12

#define	out        g0


	.text
	.link_pix

___fixunstfsi:
	shlo	32-15,s1_se,s2_exp
	ldconst	TP_Bias+32,con_1
	shro	32-15,s2_exp,s2_exp
	cmpobl.t s2_exp,con_1,LLcnv_10	/* J/ in range */
	b	Lcnv_60			/* J/ overflow */


___fixtfsi:
	shlo	32-15,s1_se,s2_exp
	ldconst	TP_Bias+31,con_1
	shro	32-15,s2_exp,s2_exp
	cmpobl.t s2_exp,con_1,LLcnv_10	/* J/ in range */
	bg	Lcnv_80			/* J/ out of range */
	bbc	15,s1_se,Lcnv_85		/* J/ positive overflow */
	
	shlo	1,s1_mhi,tmp
	cmpobne	0,tmp,LLcnv_90		/* J/ negative overflow */


LLcnv_10:
	shlo	32-16,s1_se,s2_sign
	shri	31,s2_sign,s2_sign	/* Sign of integer */

	ldconst	TP_Bias,con_1		/* Compute unbiased power of 2 */
	cmpo	s2_exp,con_1
	subo	con_1,s2_exp,s2_exp
	bl	Lcnv_20			/* J/ magnitude < 1 -> return 0 */

	subo	s2_exp,31,tmp
	shro	tmp,s1_mhi,out
	xor	s2_sign,out,out		/* Apply sign as req'd */
	subo	s2_sign,out,out
	ret


Lcnv_20:
	ldconst	0,out			/* Return 0 */
	ret


/* Unsigned special returns */

Lcnv_60:
	shlo	16,s1_se,out		/* rtn 0xFF..FF or 0, based on sign */
	shri	31,out,out
	not	out,out
	ret


/* Signed special returns */

Lcnv_80:
	bbs	15,s1_se,LLcnv_90		/* J/ negative */

Lcnv_85:
	ldconst	0x80000000,out		/* Return max positive value */
	subi	1,out,out		/* And signal an integer overflow */
	ret

LLcnv_90:
	ldconst	0x40000000,out		/* Return max negative value */
	addi	out,out,out		/* And signal an integer overflow */
	ret
