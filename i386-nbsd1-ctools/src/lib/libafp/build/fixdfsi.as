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
/*      fixdfsi.c - Double Precision to Integer Conversion Routine (AFP-960)  */
/*									      */
/******************************************************************************/


	.file	"fixdfsi.s"
	.globl	___fixunsdfsi
	.globl	___fixdfsi


#define	DP_Bias    0x3FF
#define	DP_INF     0x7FF

#define	s1         g0
#define	s1_lo      s1
#define	s1_hi      g1

#define	s2_mant    r4
#define	s2_mant_lo s2_mant
#define	s2_mant_hi r5
#define	s2_exp     r6
#define s2_sign    r7

#define	tmp        r10
#define	con_1      r11
#define	tmp2       r12

#define	out        g0


	.text
	.link_pix

___fixunsdfsi:
	shlo	1,s1_hi,s2_exp
	ldconst	DP_Bias+32,con_1
	shro	32-11,s2_exp,s2_exp
	cmpobl.t s2_exp,con_1,LLcnv_10	/* J/ in range */
	b	Lcnv_60			/* J/ overflow */


___fixdfsi:
	cmpo	s1_lo,0
	ldconst	0x80000000+((DP_Bias+31) << 20),tmp
	subc	0,1,tmp2
	or	s1_hi,tmp2,tmp2
	cmpobl.f tmp,tmp2,LLcnv_90	/* J/ invalid operation -> neg val */

	shlo	1,s1_hi,s2_exp
	ldconst	DP_Bias+31,con_1
	shro	32-11,s2_exp,s2_exp
	cmpobge.f s2_exp,con_1,Lcnv_80	/* J/ overflow */

LLcnv_10:
	shri	31,s1_hi,s2_sign	/* Sign of integer */

	ldconst	DP_Bias,con_1		/* Compute unbiased power of 2 */
	cmpo	s2_exp,con_1
	subo	con_1,s2_exp,s2_exp
	bl.f	Lcnv_20			/* J/ magnitude < 1 -> return 0 */

	ldconst	0xfff00000,con_1	/* Extract significand bits */
	andnot	con_1,s1_hi,s2_mant_hi
	subo	s2_exp,20,tmp2
	cmpi	tmp2,0
	subo	con_1,s2_mant_hi,s2_mant_hi
	bl	Lcnv_15			/* Must left shift */
	shro	tmp2,s2_mant_hi,out

	xor	s2_sign,out,out		/* Apply sign as req'd */
	subo	s2_sign,out,out
	ret

Lcnv_15:
	subo	tmp2,0,tmp2		/* Left shift count */
	ldconst	32,tmp
	subo	tmp2,tmp,tmp		/* Right shift for _lo -> _hi bits */

	shro	tmp,s1_lo,tmp
	shlo	tmp2,s2_mant_hi,out
	or	tmp,out,out

	xor	s2_sign,out,out		/* Apply sign as req'd */
	subo	s2_sign,out,out
	ret


Lcnv_20:
	ldconst	0,out			/* Return 0 */
	ret


/* Unsigned special returns */

Lcnv_60:
	shri	31,s1_hi,out		/* rtn 0xFF..FF or 0, based on sign */
	not	out,out
	ret


/* Signed special returns */

Lcnv_80:
	cmpo	s1_lo,0			/* Condense low word into one bit */
	shlo	1,s1_hi,tmp

#if	defined(USE_OPN_CC)
	addone	1,tmp,tmp
#else
	testne	tmp2
	or	tmp2,tmp,tmp		/* One word, unsigned composite */
#endif

	ldconst	DP_INF << 21,con_1
	cmpobg	tmp,con_1,LLcnv_90	/* J/ invalid operation (NaN) */

	bbs	31,s1_hi,LLcnv_90		/* J/ overflow negative */

	ldconst	0x80000000,out		/* Return max positive value */
	subi	1,out,out		/* And signal an integer overflow */
	ret

LLcnv_90:
	ldconst	0x40000000,out		/* Return max negative value */
	addi	out,out,out		/* And signal an integer overflow */
	ret
