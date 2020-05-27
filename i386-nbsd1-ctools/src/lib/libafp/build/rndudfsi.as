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
/*      rndudfsi.c - Double Precision to Unsigned Integer Conversion Routine  */
/*		   (AFP-960)						      */
/*									      */
/******************************************************************************/

#include "asmopt.h"


	.file	"rndudfsi.as"
	.globl	___roundunsdfsi

#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif


#define	DP_Bias    0x3FF
#define	DP_INF     0x7FF

#define	AC_Round_Mode    30
#define	Round_Mode_even  0x0
#define	Round_Mode_trun  0x3

#define	s1         g0
#define	s1_lo      s1
#define	s1_hi      g1

#define	s2_mant_x  r4
#define	s2_mant_hi r5
#define	s2_exp     r6
#define	s2_sign    r7

#define	tmp        r10
#define	con_1      r11
#define	con_2      r12
#define	tmp2       r13
#define	tmp3       r14

#define	ac         r15

#define	out        g0


	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

___roundunsdfsi:
	shlo	1,s1_hi,s2_exp
	shro	32-11,s2_exp,s2_exp
	ldconst	DP_Bias+32,con_1
	cmpo	s2_exp,con_1
	bge.f	Lcnv_60			/* J/ overflow */

#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac		/* Fetch AC */
#else
	modac	0,0,ac			/* Get AC */
#endif

	shri	31,s1_hi,s2_sign	/* Extract sign */
	ldconst	DP_Bias-1,con_1		/* Compute unbiased power of 2 + 1*/
	cmpo	s2_exp,con_1
	subo	con_1,s2_exp,tmp
	bl.f	Lcnv_20			/* J/ magnitude < 1/2 -> limit shift */

	ldconst	0xfff00000,con_1	/* Extract significand bits */
	andnot	con_1,s1_hi,s2_mant_hi
	subo	tmp,21,tmp		/* Compute right shift count */
	cmpi	tmp,0
	subo	con_1,s2_mant_hi,s2_mant_hi
	ldconst	32,con_1
	bl	Lcnv_19			/* Must left shift */

	subo	tmp,con_1,tmp2		/* left shift for _hi -> _lo bits */
	shlo	tmp2,s1_lo,tmp3		/* Bits shifted off the right end */
	cmpo	tmp3,0
	shro	tmp,s1_lo,s2_mant_x	/* Use care since out overlaps s1_lo */

#if	defined(USE_OPN_CC)
	selne	0,1,tmp3			/* 1 if bits shifted off right end */
#else
	testne	tmp3			/* 1 if bits shifted off right end */
#endif

	shro	tmp,s2_mant_hi,out	/* Position integer portion */
	or	s2_mant_x,tmp3,s2_mant_x
	shlo	tmp2,s2_mant_hi,tmp
	or	s2_mant_x,tmp,s2_mant_x

	cmpo	0,s2_mant_x
	be	Lcnv_16			/* J/ exact -> apply sign */

Lcnv_12:
	shro	AC_Round_Mode,ac,tmp
	cmpobne.f Round_Mode_even,tmp,Lcnv_18	/* J/ if not round/near-even */

	chkbit	0,out
	ldconst	0x7fffffff,tmp

Lcnv_14:
	addc	s2_mant_x,tmp,tmp
	addc	0,out,out
	be.f	Lcnv_60			/* J/ rounding overflow */

Lcnv_16:
	xor	s2_sign,out,out		/* apply sign */
	subo	s2_sign,out,out
	ret

Lcnv_18:
	cmpobe	Round_Mode_trun,tmp,Lcnv_16	/* J/truncate */

	xor	ac,s1_hi,tmp		/* See if directed rounding */
	shri	31,tmp,tmp		/* in this direction */
/*	cmpo	1,0	*/		/* carry bit left clear */
	b	Lcnv_14

Lcnv_19:
	subo	tmp,0,tmp2		/* Left shift count */
	subo	tmp2,con_1,tmp		/* Right shift for _lo -> _hi bits */

	shlo	tmp2,s1_lo,s2_mant_x	/* Rounding bits */
	shro	tmp,s1_lo,tmp		/* Integer portion from lo word */
	shlo	tmp2,s2_mant_hi,out	/* Upper integer portion */
	cmpo	0,s2_mant_x
	or	out,tmp,out
	bne	Lcnv_12			/* J/ inexact -> round */
	b	Lcnv_16			/* J/ exact -> apply sign */


Lcnv_20:
	cmpo	s1_lo,0			/* test for signed zero */
	shlo	1,s1_hi,out

#if	defined(USE_OPN_CC)
	selne	0,1,tmp
#else
	testne	tmp
#endif

	cmpobe	out,tmp,Lcnv_22		/* J/ signed zero -> return 0 */

	ldconst	1,s2_mant_x		/* kludge to use rounding code */
	ldconst	0,out
	b	Lcnv_12			/* Round small value as req'd */


Lcnv_22:
	ret



Lcnv_60:
	shri	31,s1_hi,out		/* rtn 0xFF..FF or 0, based on sign */
	not	out,out
	ret
