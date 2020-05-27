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
/*      rndtfsi.c - Extended Precision to Integer Conversion Routine	      */
/*		   (AFP-960)						      */
/*									      */
/******************************************************************************/

#include "asmopt.h"


	.file	"rndtfsi.as"
	.globl	___roundtfsi

#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif


#define	EP_Bias    0x3FFF
#define	EP_INF     0x7FFF

#define	AC_Round_Mode    30
#define	Round_Mode_even  0x0
#define	Round_Mode_trun  0x3

#define	s1         g0
#define	s1_mlo     s1
#define	s1_mhi     g1
#define s1_se      g2

#define	s2_mant_x  r4
#define	s2_mant_hi r5
#define	s2_exp     r6
#define s2_sign    r7

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

___roundtfsi:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac		/* Fetch AC */
#else
	modac	0,0,ac			/* Get AC */
#endif

	shlo	32-15,s1_se,tmp3	/* strip sign bit  */
	shro	32-15,tmp3,s2_exp	/* biased exponent */

	ldconst	EP_Bias+31,con_1
	cmpobg	s2_exp,con_1,Lcnv_70	/* J/ magnitude >= 2^32 */
	
	shlo	16,s1_se,s2_sign	/* sign word */
	shri	31,s2_sign,s2_sign

	subo	s2_exp,con_1,tmp	/* right shift count */
	ldconst	32,con_2
	cmpobg	tmp,con_2,Lcnv_20	/* J/ right shift count > 32 */

	subo	tmp,con_2,tmp2		/* left shift count for dropped bits */
	shlo	tmp2,s1_mlo,tmp3
	cmpo	tmp3,0
	shro	tmp,s1_mlo,tmp3

#if	defined(USE_OPN_CC)
	selne	0,1,s2_mant_x		/* set <> 0 if bit(s) lost */
#else
	testne	s2_mant_x		/* set <> 0 if bit(s) lost */
#endif

	or	tmp3,s2_mant_x,s2_mant_x
	shlo	tmp2,s1_mhi,tmp3	/* bits shifted out of top word */
	or	s2_mant_x,tmp3,s2_mant_x
	shro	tmp,s1_mhi,out
	cmpobe	0,s2_mant_x,Lcnv_15	/* J/ exact -> ovfl check, apply sign */

Lcnv_12:
	shro	AC_Round_Mode,ac,tmp
	cmpobne	Round_Mode_even,tmp,Lcnv_18	/* J/ if not round/near-even */

	chkbit	0,out
	ldconst	0x7fffffff,tmp

Lcnv_14:
	addc	s2_mant_x,tmp,tmp
	addc	0,out,out
	be	Lcnv_80			/* J/ overflow */

Lcnv_15:
	bbs	31,out,Lcnv_19		/* J/ possible overflow */

Lcnv_16:
	xor	s2_sign,out,out		/* Apply sign as req'd */
	subo	s2_sign,out,out
	ret

Lcnv_18:
	cmpobe	Round_Mode_trun,tmp,Lcnv_15	/* J/truncate */

	shlo	16,s1_se,tmp
	xor	ac,tmp,tmp		/* See if directed rounding */
	shri	31,tmp,tmp		/* in this direction */
/*	cmpo	1,0	*/		/* carry bit left clear */
	b	Lcnv_14

Lcnv_19:
	shlo	1,out,tmp
	cmpobne	0,tmp,Lcnv_80		/* J/ overflow */
	bbc	15,s1_se,Lcnv_85		/* J/ positive overflow */
	ret				/* result = 0x80000000 (!!) */


Lcnv_20:
	or	s1_mhi,s1_mlo,out	/* test for signed zero */
	cmpobe	0,out,Lcnv_22		/* J/ signed zero -> return 0 */

	ldconst	1,s2_mant_x		/* kludge to use rounding code */
	ldconst	0,out
	b	Lcnv_12			/* Round small value as req'd */


Lcnv_22:
	ret



Lcnv_70:
	ldconst	EP_INF,con_1
	cmpobne	s2_exp,con_1,Lcnv_80	/* J/ not NaN/INF */
	shlo	1,s1_mhi,tmp
	or	s1_mlo,tmp,tmp
	cmpobne	0,tmp,LLcnv_90		/* J/ invalid operation (NaN) */


Lcnv_80:
	bbs	15,s1_se,LLcnv_90		/* J/ overflow negative */

Lcnv_85:
	ldconst	0x80000000,out
	subi	1,out,out		/* Integer overflow, +maxint */
	ret

LLcnv_90:					/* Return -maxint */
	ldconst	0x40000000,out
	addi	out,out,out		/* Integer overflow, -maxint */
	ret
