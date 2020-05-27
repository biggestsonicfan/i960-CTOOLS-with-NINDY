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
/*      extsftf.c - Single Precision to Extended Precision Conversion Routine */
/*		    (AFP-960)						      */
/*									      */
/******************************************************************************/

#include "asmopt.h"


	.file	"extsftf.s"
	.globl	___extendsftf2


#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	_AFP_Fault_Reserved_Encoding_T
	.globl	_AFP_Fault_Invalid_Operation_T


#define	TP_Bias    0x3FFF
#define	TP_INF     0x7FFF
#define	FP_Bias    0x7F
#define	FP_INF     0xFF

#define	AC_Norm_Mode    29
#define	AC_InvO_mask    26
#define	AC_InvO_flag    18

#define	s1         g0

#define	s2_exp     r6
#define s2_sign    r7

#define	tmp        r10
#define	con_1      r11
#define	con_2      r12
#define	tmp2       r13
#define	tmp3       r14

#define	ac         r15

#define	out        g0
#define	out_mlo    out
#define	out_mhi    g1
#define out_se     g2

#define	TP_op_type  g7
#define	cnv_se_type 9


	.text
	.link_pix

___extendsftf2:
	shro	31,s1,s2_sign		/* extract sign */
	shlo	15,s2_sign,s2_sign

	shlo	1,s1,s2_exp		/* Check for special value */
	shri	32-8,s2_exp,tmp
	addo	1,tmp,tmp
	cmpobg	2,tmp,Ls1_special	/* J/ S1 is NaN/INF/denorm/0 */

/* Exact conversion of finite value */

	shlo	1+8-1,s1,out_mhi	/* Position S1 mant as TP .. */
	setbit	31,out_mhi,out_mhi	/* Force j bit */
	ldconst	0,out_mlo		/* LS mant word always = 0 */

	shro	32-8,s2_exp,s2_exp	/* Extract biased FP exp */

LLcnv_10:					/* Denorm junction */
	ldconst	TP_Bias-FP_Bias,con_2	/* Exp conversion factor */
	addo	con_2,s2_exp,out_se	/* Convert to biased DP exp */
	or	s2_sign,out_se,out_se	/* Use incoming sign */
	ret


Ls1_special:
	shlo	1,s1,tmp		/* Drop sign bit */
	ldconst	FP_INF << 24,con_1
	cmpobg	tmp,con_1,Ls1_06		/* J/ S1 is NaN */
	be	Ls1_20			/* J/ S1 is INF */
	cmpobne	0,tmp,LLs1_10		/* J/ S1 is denormal */

	shro	16,s1,out_se		/* Signed zero -> xfer hi word */
	movl	0,out_mlo
	ret

Ls1_02:
	ldconst	cnv_se_type,TP_op_type
	b	_AFP_Fault_Reserved_Encoding_T

Ls1_06:
	bbs	22,s1,LLLLs1_08		/* J/ QNaN */
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	bbc	AC_InvO_mask,ac,Ls1_09	/* J/ Unmasked fault w/ SNaN */
#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac	/* Set inv oper flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp	/* Set inv oper flag */
	modac	tmp,tmp,tmp
#endif

LLLLs1_08:					/* TP QNaN from FP NaN source */
	shlo	8,s1,out_mhi		/* Delete exponent, sign bit */
	ldconst	TP_INF,out_se
	setbit	15,out_se,out_se	/* force sign bit */
	setbit	31,out_mhi,out_mhi	/* Set j bit */
	setbit	30,out_mhi,out_mhi	/* (force QNaN) */
	ldconst	0,out_mlo
	ret

Ls1_09:
	ldconst	cnv_se_type,TP_op_type
	b	_AFP_Fault_Invalid_Operation_T	/* Handle NaN operand(s) */


LLs1_10:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	bbc	AC_Norm_Mode,ac,Ls1_02	/* J/ denormal, not norm mode -> fault */

	shlo	9,s1,tmp2		/* Unsigned S1 in tmp2 */
	scanbit	tmp2,tmp
	subo	31,tmp,s2_exp		/* FP biased exponent value */
	subo	tmp,31,tmp		/* Top bit num to left shift count */

	shlo	tmp,tmp2,out_mhi	/* Normalize unsigned denorm signif */
	ldconst	0,out_mlo
	b	LLcnv_10			/* Use std code to pack MS word */


Ls1_20:
	ldconst	TP_INF,out_se
	or	s2_sign,out_se,out_se	/* Return corresponding infinity */
	ldconst	0x80000000,out_mhi
	ldconst	0x00000000,out_mlo
	ret
