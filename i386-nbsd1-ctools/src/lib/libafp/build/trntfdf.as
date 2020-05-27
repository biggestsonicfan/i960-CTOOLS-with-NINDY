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
/*      trntfdf.c - Extended Precision to Double Precision Conversion Routine */
/*		   (AFP-960)						      */
/*									      */
/******************************************************************************/

#include "asmopt.h"


	.file	"trntfdf.as"
	.globl	___trunctfdf2

#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	AFP_RRC_D
	.globl	_AFP_Fault_Reserved_Encoding_T
	.globl	_AFP_Fault_Invalid_Operation_T


#define	EP_Bias    0x3FFF
#define	EP_INF     0x7FFF
#define	DP_Bias    0x3FF
#define	DP_INF     0x7FF

#define	AC_Norm_Mode    29
#define	AC_InvO_mask    26
#define	AC_InvO_flag    18
#define	AC_Round_Mode    30
#define	Round_Mode_even  0x0
#define	Round_Mode_trun  0x3

#define	s1         g0
#define	s1_mlo     s1
#define	s1_mhi     g1
#define s1_se      g2

#define	s2_mant_x  r3
#define	s2_mant_lo r4
#define s2_mant_hi r5
#define	s2_exp     r6
#define s2_sign    r7

#define	tmp        r10
#define	con_1      r11
#define	con_2      r12
#define	tmp2       r13

#define	ac         r15

#define	out_lo     g0
#define out_hi     g1

#define	EP_op_type  g7
#define	DP_op_type  g4
#define	cnv_ed_type 6


	.text
	.link_pix


	.align	MAJOR_CODE_ALIGNMENT

___trunctfdf2:

#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	shro	15,s1_se,s2_sign	/* extract sign */
	shlo	31,s2_sign,s2_sign

	shlo	32-15,s1_se,s2_exp
	shri	32-15,s2_exp,tmp
	addo	1,tmp,tmp
	cmpobge	1,tmp,Ls1_special	/* J/ S1 is NaN/INF/denorm/0 */
	bbc	31,s1_mhi,Ls1_unnormal	/* J/ S1 is an unnormal */

	ldconst	cnv_ed_type,DP_op_type  /* Will use RRC_S -> set op type */

	shlo	32-(64-53),s1_mlo,s2_mant_x	/* rounding info to s2_mant_x */
	shro	64-53,s1_mlo,s2_mant_lo		/* Position significand */
	shro	64-53,s1_mhi,s2_mant_hi
	shlo	32-(64-53),s1_mhi,tmp
	or	s2_mant_lo,tmp,s2_mant_lo

	ldconst	EP_Bias-DP_Bias,con_2
	shro	32-15,s2_exp,s2_exp	/* Extract biased DP exp */
	subo	con_2,s2_exp,s2_exp	/* Convert to biased FP exp */
	ldconst	DP_INF+1533,con_1
	cmpibg	s2_exp,con_1,LLcnv_10	/* Massive overflow thresh */
	ldconst	0-1533,con_1
	cmpibl	s2_exp,con_1,LLcnv_10	/* Massive underflow thresh */
	b	AFP_RRC_D

LLcnv_10:
	mov	con_1,s2_exp		/* Limit over/underflow */
	b	AFP_RRC_D



Ls1_special:
	shlo	1,s1_mhi,tmp		/* Drop j bit */
	or	s1_mlo,tmp,tmp

	bne	LLs1_10			/* J/ S1 is NaN/INF */

	cmpobne	0,tmp,Ls1_02		/* J/ S1 is denormal */

	shlo	16,s1_se,out_hi		/* Signed zero conversion */
	ldconst	0,out_lo
	ret

Ls1_02:
	bbc	AC_Norm_Mode,ac,LLLLs1_08	/* J/ denormal, not norm mode -> fault */

	scanbit	s1_mhi,tmp
	bno	Ls1_06			/* J/ MS word = 0 */

	subo	tmp,31,tmp		/* Top bit num to left shift count */
	shlo	tmp,s1_mhi,s2_mant_hi	/* Normalize denorm significand */
	shlo	tmp,s1_mlo,s2_mant_lo
	subo	tmp,31,tmp		/* word -> word bit xfer */
	addo	1,tmp,tmp
	shro	tmp,s1_mlo,tmp
	or	tmp,s2_mant_hi,s2_mant_hi

LLLLLs1_04:
	shlo	32-(64-53),s1_mlo,s2_mant_x	/* rounding info to s2_mant_x */
	shro	64-53,s1_mlo,s2_mant_lo		/* Position significand */
	shro	64-53,s1_mhi,s2_mant_hi
	shlo	32-(64-53),s1_mhi,tmp
	or	s2_mant_lo,tmp,s2_mant_lo

	ldconst	0-1533,s2_exp		/* Minimum exponent after unfl proc */
	ldconst	cnv_ed_type,DP_op_type
	b	AFP_RRC_D		/* Underflow processing */

Ls1_06:
	scanbit	s1_mlo,tmp

	subo	tmp,31,tmp
	shlo	tmp,s1_mlo,s2_mant_hi	/* 32+ bit shift */
	ldconst	0,s2_mant_lo
	ldconst	0,s2_mant_x
	b	LLLLLs1_04


LLLLs1_08:
Ls1_unnormal:
	ldconst	cnv_ed_type,EP_op_type
	b	_AFP_Fault_Reserved_Encoding_T


LLs1_10:
	cmpobne	0,tmp,Ls1_12		/* J/ NaN */

	ldconst	DP_INF << 20,con_1
	or	s2_sign,con_1,out_hi	/* Return corresponding infinity */
	ldconst	0,out_lo
	ret


Ls1_12:
	bbc	30,s1_mhi,Ls1_16		/* J/ SNaN */

Ls1_14:
	shlo	1,s1_mhi,tmp		/* DP QNaN from DP NaN source */
	shro	12,tmp,out_hi		/* Retain 20 info bits in MS word */
	shlo	32-12+1,s1_mhi,tmp	/* ... and 32 more in LS word */
	shro	12-1,s1_mlo,out_lo
	or	tmp,out_lo,out_lo
	shlo	32-12,s1_se,tmp		/* lo 11 bits of EP NaN exp = ... */
	or	tmp,out_hi,out_hi	/* ... DP NaN exp */
	setbit	19,out_hi,out_hi	/* Force QNaN */
/* **	setbit	31,out_hi,out_hi ** */	/* Force sign */	/* by exp */
	ret

Ls1_16:
	bbc	AC_InvO_mask,ac,Ls1_18	/* J/ Unmasked fault w/ SNaN */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac	/* Set inv oper flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp	/* Set inv oper flag */
	modac	tmp,tmp,tmp
#endif
	b	Ls1_14			/* Return a QNaN */

Ls1_18:
	ldconst	cnv_ed_type,EP_op_type
	b	_AFP_Fault_Invalid_Operation_T	/* Handle NaN operand(s) */
