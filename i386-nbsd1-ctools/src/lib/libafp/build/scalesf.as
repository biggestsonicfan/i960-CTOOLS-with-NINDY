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
/*      scalesf.c - Scale Single Precision Routine (AFP-960)		      */
/*									      */
/******************************************************************************/

#include "asmopt.h"


	.file	"scalesf.as"
	.globl	___scalesfsisf

#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	AFP_RRC_S

	.globl	_AFP_Fault_Invalid_Operation_S
	.globl	_AFP_Fault_Reserved_Encoding_S

#define	FP_INF          0xff

#define	AC_Norm_Mode    29

#define	AC_InvO_mask    26
#define	AC_InvO_flag    18


/* Register Name Equates */

#define	s1         g0
#define	s2         g1

#define	s1_mant_x  r4
#define	s1_mant    r5
#define	s1_exp     r3
#define	s1_sign    r6

#define	tmp        r10
#define	tmp2       r11
#define	con_1      r14
#define	ac         r15

#define	op_type    g2
#define	scale_type 13

#define	out        g0


	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

___scalesfsisf:
	ldconst	scale_type,op_type	/* for fault handler operation info */
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	shro	31,s1,s1_sign
	shlo	31,s1_sign,s1_sign

	shlo	1,s1,s1_exp
	shri	24,s1_exp,tmp
	addo	1,tmp,tmp
	cmpobg	2,tmp,Ls1_special	/* J/ s1 is zero/denormal/INF/NaN */

	ldconst	0xff800000,tmp		/* Extract significand and exp */
	andnot	tmp,s1,s1_mant
	subo	tmp,s1_mant,s1_mant
	shro	24,s1_exp,s1_exp
Ls1_rejoin:

/*
 * Finite, non-zero number
 */
	addo	s2,s1_exp,s1_exp	/* Scale exponent by integer src1 */
	ldconst	0,s1_mant_x		/* Exact operation */
	b	AFP_RRC_S		/* Range check */


/*  s1 is a special case value: +/-0, denormal, +/-INF, NaN  */

Ls1_special:
	shlo	1,s1,tmp		/* Drop sign bit */
	ldconst	FP_INF << 24,con_1
	cmpobg	tmp,con_1,Ls1_20		/* J/ s1 is NaN */
	be	LLLLLs1_04			/* J/ s1 is INF */
	cmpobne	0,tmp,LLs1_10		/* J/ s1 is denormal */

LLLLLs1_04:
	mov	s1,out			/* Scale of 0 or INF  ->  return s1 */
	ret


LLs1_10:
	bbc	AC_Norm_Mode,ac,Ls1_12	/* J/ denormal, not norm mode -> fault */

	scanbit	tmp,tmp2
	subo	22+1,tmp2,s1_exp	/* compute denormal exponent */
	subo	tmp2,23,tmp2
	shlo	tmp2,tmp,s1_mant	/* normalize denormal significand */
	b	Ls1_rejoin

Ls1_12:
	b	_AFP_Fault_Reserved_Encoding_S


Ls1_20:
	bbs	22,s1,LLLLLs1_04		/* J/ quiet NaN -> just return it */

	bbc	AC_InvO_mask,ac,Ls1_22	/* J/ inv oper fault not masked */
#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac	/* Set inv oper flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp	/* Set inv oper flag */
	modac	tmp,tmp,tmp
#endif

	setbit	22,s1,out		/* Convert SNaN to QNaN, return it */
	ret

Ls1_22:
	b	_AFP_Fault_Invalid_Operation_S
