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
/*      cmpdf2.c - Double Precision Comparison Routine (AFP-960)	      */
/*									      */
/******************************************************************************/

#include "asmopt.h"


	.file	"cmpdf2.as"
	.globl	___cmpdf2	/* ?? */

#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

#define	DP_Bias         0x3ff
#define	DP_INF          0x7ff

#define	AC_Norm_Mode    29

#define	AC_InvO_mask    26
#define	AC_InvO_flag    18


/* Register Name Equates */

#define	s1         g0
#define	s1_lo      s1
#define	s1_hi      g1
#define s2         g2
#define s2_lo      s2
#define s2_hi      g3

#define	s2_mant_x  r3
#define	s2_mant    r4
#define s2_mant_lo s2_mant
#define s2_mant_hi r5
#define	s2_exp     r6

#define s1_mant_lo r9
#define s1_mant_hi r10
#define	s1_exp     r11

#define	tmp        r12
#define	tmp2       r13
#define	tmp3       r14

#define	ac         r15

#define	con_1      r14

#define	out        g0

#define	op_type    g4
#define	cmp_type   12


	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

___cmpdf2:
	ldconst	cmp_type,op_type

#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	shlo	1,s1_hi,s1_exp			/* Delete sign bit */
	shlo	1,s2_hi,s2_exp

	shri	32-11,s1_exp,tmp
	addo	1,tmp,tmp
	cmpobg	2,tmp,Ls1_special		/* J/ S1 is NaN/INF/0/denorm */

Ls1_rejoin:

	shri	32-11,s2_exp,tmp
	addo	1,tmp,tmp
	cmpobg	2,tmp,Ls2_special		/* J/ S2 is NaN/INF/0/denorm */

Ls2_rejoin:

/*
 * Produce two results: correct setting of S1 vs S2 in the condition code
 * register and a -1/0/1/3 value returned in g0 (for S1<S2, S1=S2, S1>S2,
 * S1 does not compare to S2).
 */
	shri	31,s1_hi,tmp		/* Convert from sign-mag to two's cmpl */
	clrbit	31,s1_hi,s1_hi
	xor	tmp,s1_hi,s1_hi
	xor	tmp,s1_lo,s1_lo
	cmpo	1,1			/* Set /BORROW */
	subc	tmp,s1_lo,s1_lo
	subc	tmp,s1_hi,s1_hi
	notbit	31,s1_hi,s1_hi		/* Unsigned analog to S1 */

	shri	31,s2_hi,tmp		/* Convert from sign-mag to two's cmpl */
	clrbit	31,s2_hi,s2_hi
	xor	tmp,s2_hi,s2_hi
	xor	tmp,s2_lo,s2_lo
	cmpo	1,1			/* Set /BORROW */
	subc	tmp,s2_lo,s2_lo
	subc	tmp,s2_hi,s2_hi
	notbit	31,s2_hi,s2_hi		/* Unsigned analog to S2 */

	cmpobg	s1_hi,s2_hi,Lcmp_18	/* J/ S1>S2 */
	bl	Lcmp_16			/* J/ S1<S2 */
	cmpobg	s1_lo,s2_lo,Lcmp_18
	bl	Lcmp_16

	ldconst	0,out
	ret

Lcmp_16:
	ldconst	-1,out
	ret

Lcmp_18:
	ldconst	1,out
	ret



/*  S1 is a special case value: +/-0, denormal, +/-INF, NaN; S2 is unknown  */

Ls1_special:
	shlo	1,s1_hi,tmp		/* Drop sign bit */
	cmpo	s1_lo,0			/* Condense lo word into LS bit */

#if	defined(USE_OPN_CC)
	addone	1,tmp,tmp
#else
	testne	tmp2
	or	tmp,tmp2,tmp
#endif

	ldconst	DP_INF << 21,con_1
	cmpobg	tmp,con_1,LLLLLs1_04		/* J/ S1 is NaN */
	be	Ls1_rejoin		/* J/ S1 is INF -> standard compare */
	cmpobe	0,tmp,Ls1_rejoin		/* J/ S1 0 -> standard compare */
	bbs	AC_Norm_Mode,ac,Ls1_rejoin /* J/ denorm, norm mode -> std cmp */

Ls1_02:
	b	_AFP_Fault_Reserved_Encoding_D

LLLLLs1_04:					/* S1 is a NaN */
	shlo	1,s2_hi,tmp2
	cmpo	s2_lo,tmp3

#if	defined(USE_OPN_CC)
	addone	1,tmp2,tmp2
#else
	testne	tmp3
	or	tmp2,tmp3,tmp2
#endif

	cmpobg	tmp2,con_1,Ls1_12	/* J/ S2 is a NaN, too */
	cmpobe	0,tmp2,Ls1_06		/* J/ S2 is a zero */
	ldconst	0x00ffffff,con_1
	cmpobg	tmp2,con_1,Ls1_06	/* J/ S2 is not a denormal */
	bbc	AC_Norm_Mode,ac,Ls1_02	/* J/ fault (denorm, not norm mode */

Ls1_06:
	bbs	19,s1_hi,LLs1_10		/* J/ quiet NaN */
	bbc	AC_InvO_mask,ac,Ls1_14	/* J/ inv oper fault not masked */

LLLLs1_08:
#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac	/* Set inv oper flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp	/* Set inv oper flag */
	modac	tmp,tmp,tmp
#endif
LLs1_10:
	ldconst	3,g0			/* return numeric "does not compare" */
	chkbit	0,0			/* unordered condition code */
	ret

Ls1_12:
	and	s1_hi,s2_hi,tmp
	bbs	19,tmp,LLs1_10		/* J/ S1 and S2 are QNaN's */
	bbs	AC_InvO_mask,ac,LLLLs1_08	/* J/ SNaN(s) w/ invalid opn masked */

Ls1_14:
	b	_AFP_Fault_Invalid_Operation_D	/* Handle NaN operand(s) */



/*  S2 is a special case value: +/-0, denormal, +/-INF, NaN  */

Ls2_special:
	shlo	1,s2_hi,tmp2
	cmpo	s2_lo,0			/* Condense lo word into LS bit */

#if	defined(USE_OPN_CC)
	addone	1,tmp2,tmp2
#else
	testne	tmp3
	or	tmp2,tmp3,tmp2
#endif

	ldconst	DP_INF << 21,con_1
	cmpobg	tmp2,con_1,Ls2_04	/* J/ S2 is a NaN */
	be	Ls2_rejoin		/* J/ S2 is INF -> standard compare */
	cmpobe	0,tmp2,Ls2_rejoin	/* J/ s2 is 0 -> standard compare */
	bbc	AC_Norm_Mode,ac,Ls1_02	/* J/ denormal, not norm mode -> fault */
	b	Ls2_rejoin		/* denorm, norm mode -> std cmp */

Ls2_04:
	bbs	19,s2_hi,LLs1_10		/* J/ quiet NaN */
	bbc	AC_InvO_mask,ac,Ls1_14	/* J/ inv oper fault not masked */
	b	LLLLs1_08
