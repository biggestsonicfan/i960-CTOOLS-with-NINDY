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
/*      cmptf2.c - Extended Precision Comparison Routine (AFP-960)	      */
/*									      */
/******************************************************************************/

#include "asmopt.h"


	.file	"cmptf2.s"
	.globl	___cmptf2

#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	_AFP_Fault_Invalid_Operation_T
	.globl	_AFP_Fault_Reserved_Encoding_T


#define	AC_Norm_Mode    29

#define	AC_InvO_mask    26
#define	AC_InvO_flag    18


/* Register Name Equates */

#define	s1         g0
#define	s1_mlo     s1
#define	s1_mhi     g1
#define s1_se      g2

#define s2         g4
#define s2_mlo     s2
#define s2_mhi     g5
#define s2_se      g6


#define	s2_x_lo    r3
#define	s2_x_mid   r4
#define s2_x_hi    r5

#define	s2_exp     r6

#define s1_x_lo    r9
#define s1_x_mid   r10
#define s1_x_hi    r11

#define	s1_exp     r12

#define	tmp        r13
#define	tmp2       r14

#define	ac         r15


#define	out        g0

#define	op_type    g7
#define	cmp_type   12



	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

___cmptf2:
	ldconst	cmp_type,op_type

#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	shlo	32-15,s1_se,s1_exp	/* Delete sign bit */
	shlo	32-15,s2_se,s2_exp

	shri	32-15,s1_exp,tmp
	addo	1,tmp,tmp
	cmpobge	1,tmp,Ls1_special	/* J/ S1 is NaN/INF/0/denorm */

	bbc	31,s1_mhi,Ls1_unnormal

Ls1_rejoin:

	shri	32-15,s2_exp,tmp
	addo	1,tmp,tmp
	cmpobge	1,tmp,Ls2_special	/* J/ S2 is NaN/INF/0/denorm */

	bbc	31,s2_mhi,Ls2_unnormal

Ls2_rejoin:

/*
 * Produce two results: correct setting of S1 vs S2 in the condition code
 * register and a -1/0/1/3 value returned in g0 (for S1<S2, S1=S2, S1>S2,
 * S1 does not compare to S2).
 */
	shlo	16,s1_se,tmp		/* Convert from sign-mag to two's cmpl */
	shri	31,tmp,tmp
	shlo	17,s1_se,s1_x_hi	/* Strip sign bit, zero top halfword */
	shro	17,s1_x_hi,s1_x_hi

	xor	tmp,s1_x_hi,s1_x_hi
	xor	tmp,s1_mhi,s1_x_mid
	xor	tmp,s1_mlo,s1_x_lo
	cmpo	1,1			/* Set /BORROW */
	subc	tmp,s1_x_lo,s1_x_lo
	subc	tmp,s1_x_mid,s1_x_mid
	subc	tmp,s1_x_hi,s1_x_hi

	notbit	31,s1_x_hi,s1_x_hi	/* Unsigned analog to S1 */

	shlo	16,s2_se,tmp		/* Convert from sign-mag to two's cmpl */
	shri	31,tmp,tmp
	shlo	17,s2_se,s2_x_hi	/* Strip sign bit, zero top halfword */
	shro	17,s2_x_hi,s2_x_hi

	xor	tmp,s2_x_hi,s2_x_hi
	xor	tmp,s2_mhi,s2_x_mid
	xor	tmp,s2_mlo,s2_x_lo
	cmpo	1,1			/* Set /BORROW */
	subc	tmp,s2_x_lo,s2_x_lo
	subc	tmp,s2_x_mid,s2_x_mid
	subc	tmp,s2_x_hi,s2_x_hi

	notbit	31,s2_x_hi,s2_x_hi	/* Unsigned analog to S2 */

	cmpobg	s1_x_hi,s2_x_hi,Lcmp_18	/* J/ S1>S2 */
	bl	Lcmp_16			/* J/ S1<S2 */
	cmpobg	s1_x_mid,s2_x_mid,Lcmp_18
	bl	Lcmp_16			/* J/ S1<S2 */
	cmpobg	s1_x_lo,s2_x_lo,Lcmp_18
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
	shlo	1,s1_mhi,tmp		/* Drop j bit */
	or	s1_mlo,tmp,tmp

	bne	LLLLLs1_04			/* J/ NaN/INF */

	cmpobe	0,tmp,Ls1_rejoin		/* J/ S1 0 -> standard compare */

	bbs	AC_Norm_Mode,ac,Ls1_rejoin /* J/ denorm, norm mode -> std cmp */

Ls1_02:
Ls1_unnormal:
Ls2_unnormal:
	b	_AFP_Fault_Reserved_Encoding_T

LLLLLs1_04:					/* S1 is NaN/INF */
	bbc	31,s1_mhi,Ls1_02		/* J/ no j bit -> rsvd encoding */

	cmpobe	0,tmp,Ls1_rejoin		/* J/ INF -> use normal compare */

	shri	17,s2_exp,tmp
	addo	1,tmp,tmp
	cmpobge	1,tmp,Ls1_06		/* J/ S2 is NaN/INF/denormal/0 */

	bbc	31,s2_mhi,Ls1_02		/* J/ no j bit -> rsvd encoding */
	b	LLs1_10

Ls1_06:
	shlo	1,s2_mhi,tmp
	or	s2_mlo,tmp,tmp
	bne	LLLLs1_08			/* J/ S2 is INF or NaN */

	cmpobe	0,tmp,LLs1_10		/* J/ S2 is a zero */

	bbc	AC_Norm_Mode,ac,Ls1_02	/* J/ fault (denorm, not norm mode */
	b	LLs1_10

LLLLs1_08:
	bbc	31,s2_mhi,Ls1_02		/* NaN/INF w/o j bit -> rsvd encd */
	cmpobne	0,tmp,Ls1_16		/* J/ S2 is NaN, too */

LLs1_10:
	bbs	30,s1_mhi,Ls1_14		/* J/ quiet NaN */
	bbc	AC_InvO_mask,ac,Ls1_18	/* J/ inv oper fault not masked */

Ls1_12:
#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac	/* Set inv oper flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp	/* Set inv oper flag */
	modac	tmp,tmp,tmp
#endif
Ls1_14:
	ldconst	3,g0			/* return numeric "does not compare" */
	chkbit	0,0			/* unordered condition code */
	ret

Ls1_16:
	and	s1_mhi,s2_mhi,tmp
	bbs	30,tmp,Ls1_14		/* J/ S1 and S2 are QNaN's */
	bbs	AC_InvO_mask,ac,Ls1_12	/* J/ SNaN(s) w/ invalid opn masked */

Ls1_18:
	b	_AFP_Fault_Invalid_Operation_T	/* Handle NaN operand(s) */



/*  S2 is a special case value: +/-0, denormal, +/-INF, NaN  */

Ls2_special:
	shlo	1,s2_mhi,tmp
	or	s2_mlo,tmp,tmp

	bne	Ls2_02			/* J/ S2 is NaN/INF */

	cmpobe	0,tmp,Ls2_rejoin		/* J/ s2 is 0 -> standard compare */
	bbc	AC_Norm_Mode,ac,Ls1_02	/* J/ denormal, not norm mode -> fault */
	b	Ls2_rejoin		/* denorm, norm mode -> std cmp */

Ls2_02:
	bbc	31,s2_mhi,Ls1_02		/* J/ NaN/INF w/o j bit set */

	cmpobe	0,tmp,Ls2_rejoin		/* J/ S2 is INF -> standard compare */

	bbs	30,s2_mhi,Ls1_14		/* J/ quiet NaN */
	bbc	AC_InvO_mask,ac,Ls1_18	/* J/ inv oper fault not masked */
	b	Ls1_12
