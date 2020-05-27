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
/*      rndsfsi.c - Single Precision to Integer Conversion Routine (AFP-960)  */
/*									      */
/******************************************************************************/

#include "asmopt.h"


	.file	"rndsfsi.as"
	.globl	___roundsfsi

#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif


#define	FP_Bias    0x7F
#define	FP_INF     0xFF

#define	AC_Round_Mode    30
#define	Round_Mode_even  0x0
#define	Round_Mode_trun  0x3

#define	s1         g0

#define	s2_mant_x  r3
#define	s2_mant    r4
#define	s2_exp     r5
#define s2_sign    r6

#define	tmp        r10
#define	con_1      r11
#define	con_2      r12
#define	tmp2       r13

#define	ac         r15

#define	out        g0


	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

___roundsfsi:
	ldconst	0x80000000+((FP_Bias+31) << 23),tmp
	cmpo	s1,tmp
	shlo	1,s1,s2_exp
	bg	LLcnv_90			/* J/ invalid operation -> neg val */
	shro	32-8,s2_exp,s2_exp
	ldconst	FP_Bias+31,con_1
	cmpo	s2_exp,con_1
	shri	31,s1,s2_sign		/* -1 or 0 based on sign */
	bge	Lcnv_80			/* J/ overflow */

LLcnv_10:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac		/* Fetch AC */
#else
	modac	0,0,ac			/* Get AC */
#endif

	ldconst	FP_Bias-1,con_1		/* Compute unbiased power of 2 */
	cmpo	s2_exp,con_1
	subo	con_1,s2_exp,tmp
	bl	Lcnv_20			/* J/ magnitude < 1/2 -> special case */

	shlo	8,s1,s2_mant		/* Left justify significand */
	ldconst	32,tmp2
	subo	tmp,tmp2,tmp		/* Right shift count */
	subo	tmp,tmp2,tmp2		/* Left shift count for rounding */
	setbit	31,s2_mant,s2_mant	/* Explicit "j" bit */
	shlo	tmp2,s2_mant,s2_mant_x
	cmpo	s2_mant_x,0
	shro	tmp,s2_mant,out		/* Position value */
	be	Lcnv_16			/* J/ exact -> apply sign */

Lcnv_12:
	shro	AC_Round_Mode,ac,tmp
	cmpobne	Round_Mode_even,tmp,Lcnv_18	/* J/ if not round/near-even */

	chkbit	0,out
	ldconst	0x7fffffff,tmp

Lcnv_14:
	addc	s2_mant_x,tmp,tmp
	addc	0,out,out
	bg	Lcnv_19			/* J/ potential rounding overflow */

Lcnv_16:
	xor	s2_sign,out,out		/* Apply sign as req'd */
	subo	s2_sign,out,out
	ret

Lcnv_18:
	cmpobe	Round_Mode_trun,tmp,Lcnv_16	/* J/truncate */

	xor	ac,s2_sign,tmp			/* See if directed rounding */
	shri	31,tmp,tmp			/* in this direction */
/*	cmpo	1,0	*/			/* carry bit left clear */
	b	Lcnv_14

Lcnv_19:
	cmpobne	0,s2_sign,Lcnv_16	/* J/ no overflow if negative result */
	b	Lcnv_82			/* Else: int overflow, return +maxint */


Lcnv_20:
	shlo	1,s1,out
	cmpobe	0,out,Lcnv_22		/* J/ signed zero -> return 0 */

	ldconst	1,s2_mant_x
	ldconst	0,out
	b	Lcnv_12			/* Round small value as req'd */


Lcnv_22:
	ret


Lcnv_80:
	shlo	1,s1,tmp
	ldconst	FP_INF << 24,con_1
	cmpobg	tmp,con_1,LLcnv_90	/* J/ invalid operation (NaN) */

	bbs	31,s1,LLcnv_90		/* J/ overflow negative */

Lcnv_82:
	ldconst	0x80000000,out
	subi	1,out,out		/* Integer overflow, +maxint */
	ret

LLcnv_90:					/* Return -maxint */
	ldconst	0x40000000,out
	addi	out,out,out		/* Integer overflow, -maxint */
	ret
