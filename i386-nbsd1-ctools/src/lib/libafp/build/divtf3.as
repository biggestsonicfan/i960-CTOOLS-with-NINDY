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
/*      divtf3.c - Extended Precision Division Routine (AFP-960)	      */
/*									      */
/******************************************************************************/

#include "asmopt.h"


	.file	"divtf3.as"
	.globl	___divtf3

#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	AFP_RRC_T
	.globl	AFP_NaN_T

	.globl	_AFP_Fault_Invalid_Operation_T
	.globl	_AFP_Fault_Reserved_Encoding_T
	.globl	_AFP_Fault_Zero_Divide_T


#define	TP_Bias         0x3fff
#define	TP_INF          0x7fff


#define	AC_Norm_Mode    29

#define	AC_InvO_mask    26
#define	AC_InvO_flag    18

#define	AC_ZerD_mask    27
#define	AC_ZerD_flag    19

#define	Standard_QNaN_se  0x8000+TP_INF
#define	Standard_QNaN_mhi 0xc0000000
#define	Standard_QNaN_mlo 0x00000000


/* Register Name Equates */

#define	s1           g0
#define	s1_mlo       s1
#define	s1_mhi       g1
#define	s1_se        g2
#define s2           g4
#define s2_mlo       s2
#define s2_mhi       g5
#define	s2_se        g6

#define	quo_lo       r3
#define	quo_mid      r4
#define	quo_hi       r5
#define quo_exp      r6
#define quo_sign     r7

#define	s2_exp       r8

#define hld_exp      g3

#define	lt_2         r6
#define	lt_2_lo      lt_2
#define	lt_2_hi      r7

#define	rmndr        r8
#define	rmndr_lo     rmndr
#define	rmndr_mid    r9

#define	con_1        rmndr_lo

#define	lt_3         r10
#define	lt_3_lo      lt_3
#define	lt_3_hi      r11

#define	B            s2_mhi
#define	C            s2_mlo

#define	tmp          r14
#define	ac           r15

#define	tmp2         r12
#define	tmp3         r13
#define	long_temp    tmp2
#define	long_temp_lo long_temp
#define	long_temp_hi tmp3

#define	rmndr_hi     tmp2


#define	out          g0
#define	out_mlo      out
#define	out_mhi      g1
#define	out_se       g2

#define	op_type      g7
#define	div_type     2


	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

___divtf3:

	ldconst	div_type,op_type

#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	xor	s1_se,s2_se,quo_sign		/* Compute and isolate ... */
	ldconst	0x00008000,tmp			/* ... result sign	   */
	and	quo_sign,tmp,quo_sign

	shlo	32-15,s1_se,quo_exp		/* Delete sign bit */
	shlo	32-15,s2_se,s2_exp

	shri	32-15,s2_exp,tmp
	addo	1,tmp,tmp
	cmpobge	1,tmp,Ls2_special		/* J/ S2 is NaN/INF/0/denorm */

	bbc	31,s2_mhi,Ls2_unnormal
	shro	32-15,s2_exp,s2_exp

Ls2_rejoin:

	shri	32-15,quo_exp,tmp
	addo	1,tmp,tmp
	cmpobge	1,tmp,Ls1_special		/* J/ S1 is NaN/INF/0/denorm */

	bbc	31,s1_mhi,Ls1_unnormal
	shro	32-15,quo_exp,quo_exp

Ls1_rejoin:

	subo	s2_exp,quo_exp,hld_exp		/* compute result exp    ... */
	ldconst	TP_Bias,tmp			/* (save exp in spare reg)   */
	addo	hld_exp,tmp,hld_exp		/* ... with bias             */

/*
 * Normalized source operands - perform divide
 */
	shlo	31,s1_mhi,tmp			/* ...  shr(s1_mant,1) 	 */
	shro	1,s1_mhi,long_temp_hi
	shro	1,s1_mlo,long_temp_lo
	or	tmp,long_temp_lo,long_temp_lo

	ediv	B,long_temp,long_temp		/* 96 bit of A/B -> quo	 */
	mov	long_temp_hi,quo_hi
	mov	long_temp_lo,long_temp_hi	/* (append LS s1 bit to	 */
	shlo	31,s1_mlo,long_temp_lo		/* EDIV remainder)	 */
	ediv	B,long_temp,long_temp
	mov	long_temp_hi,quo_mid
	mov	long_temp_lo,long_temp_hi	/* zero extend for	 */
	mov	0,long_temp_lo			/* ... next 32 bits	 */
	ediv	B,long_temp,long_temp
	mov	long_temp_hi,quo_lo

	shro	1,C,long_temp_hi		/* 64 bits adj value	 */
	shlo	32-1,C,long_temp_lo		/* (shift avoids ovfl)	 */
	ediv	B,long_temp,long_temp
	emul	long_temp_hi,long_temp_hi,lt_2	/* (compute 2nd order)	 */
	mov	long_temp_hi,tmp
	mov	long_temp_lo,long_temp_hi
	mov	0,long_temp_lo
	ediv	B,long_temp,long_temp
	shlo	1,lt_2_hi,lt_2_lo		/* position 2nd order	 */
	shro	31,lt_2_hi,lt_2_hi
	cmpo	0,0
	subc	lt_2_lo,long_temp_hi,long_temp_lo
	subc	lt_2_hi,tmp,long_temp_hi	/* combined 1st & 2nd	 */

	emul	quo_hi,long_temp_lo,lt_2	/* compute adjustment	 */
	cmpo	0,1				/* clear carry */
	emul	quo_mid,long_temp_hi,lt_3
	addc	lt_2_lo,lt_3_lo,lt_2_lo
	addc	lt_2_hi,lt_3_hi,lt_2_hi
	addc	0,0,tmp
	emul	quo_hi,long_temp_hi,lt_3
	addc	lt_2_hi,lt_3_lo,lt_3_lo
	addc	tmp,lt_3_hi,lt_3_hi

	addc	lt_3_lo,lt_3_lo,lt_2_lo		/* alignment shift */
	addc	lt_3_hi,lt_3_hi,lt_2_hi
	addc	0,0,tmp
	addc	6,lt_2_lo,lt_2_lo		/* max compensate for trunc */
	addc	0,lt_2_hi,lt_2_hi
	addc	0,tmp,tmp
	cmpo	0,0				/* set /borrow */
	subc	lt_2_lo,quo_lo,quo_lo		/* adjust quotient estimate */
	ldconst	0xc0000000,lt_2_lo
	subc	lt_2_hi,quo_mid,quo_mid
	and	quo_lo,lt_2_lo,quo_lo		/* retain guard + round bit */
	subc	tmp,quo_hi,quo_hi


	emul	s2_mlo,quo_lo,rmndr_lo		/* compute dvnd - quo*dvsr */
	emul	s2_mlo,quo_mid,lt_2
	cmpo	0,1
	emul	s2_mhi,quo_lo,lt_3
	addc	lt_2_lo,rmndr_mid,rmndr_mid
	addc	0,lt_2_hi,rmndr_hi
	emul	s2_mlo,quo_hi,lt_2
	addc	lt_3_lo,rmndr_mid,rmndr_mid
	addc	lt_3_hi,rmndr_hi,rmndr_hi
	emul	s2_mhi,quo_mid,lt_3
	addo	lt_2_lo,rmndr_hi,rmndr_hi
	cmpo	0,0
	addo	lt_3_lo,rmndr_hi,rmndr_hi
	subc	rmndr_lo,0,rmndr_lo
	subc	rmndr_mid,0,rmndr_mid
	subc	rmndr_hi,0,rmndr_hi

	shro	30,rmndr_lo,rmndr_lo		/* align rmndr w/ divisor */
	shlo	2,rmndr_mid,tmp
	or	rmndr_lo,tmp,rmndr_lo
	shro	30,rmndr_mid,rmndr_mid
	shlo	2,rmndr_hi,tmp
	or	rmndr_mid,tmp,rmndr_mid

	bbs	30,rmndr_hi,Lr_adjust		/* J/ rmndr > divisor	 */
	cmpobg	rmndr_mid,B,Lr_adjust		/* J/ rmndr > divisor	 */
	bl	Lno_adjust			/* J/ rmndr < divisor	 */
	cmpobl	rmndr_lo,C,Lno_adjust		/* J/ rmndr < divisor	 */

Lr_adjust:
	cmpo	0,0				/* Set BORROW/ bit	 */
	subc	C,rmndr_lo,rmndr_lo		/* Reduce remainder	 */
	subc	B,rmndr_mid,rmndr_mid

	ldconst	0x40000000,tmp
	cmpo	0,1
	addc	tmp,quo_lo,quo_lo		/* Incr quotient's LSB	 */
	addc	0,quo_mid,quo_mid
	addc	0,quo_hi,quo_hi

Lno_adjust:
	xor	s1_se,s2_se,quo_sign		/* Compute and isolate ... */
	ldconst	0x00008000,tmp			/* ... result sign	   */
	and	quo_sign,tmp,quo_sign

	mov	hld_exp,quo_exp

	or	rmndr_lo,rmndr_mid,rmndr_lo
	cmpo	0,rmndr_lo

#if	defined(USE_OPN_CC)
	selne	0,1,tmp2
#else
	testne	tmp2
#endif

	or	quo_lo,tmp2,quo_lo		/* inexact sticky bit	 */
	cmpibg	0,quo_hi,Lno_norm		/* J/ already normalized */

	subo	1,quo_exp,quo_exp		/* reduce exponent	 */

	addc	quo_lo,quo_lo,quo_lo		/* single left shift 	 */
	addc	quo_mid,quo_mid,quo_mid
	addc	quo_hi,quo_hi,quo_hi

Lno_norm:
	b	AFP_RRC_T			/* Round and range check */


/*  S2 is a special case value: +/-0, denormal, +/-INF, NaN; S1 is unknown  */

Ls2_special:
	shlo	1,s2_mhi,tmp		/* Drop integer mantissa bit */
	or	s2_mlo,tmp,tmp

	bne	Ls2_30			/* J/ S2 is NaN or INF */

	cmpobne	0,tmp,Ls2_20		/* J/ S2 is denormal */

	shri	32-15,quo_exp,tmp	/* S2 is zero - examine S1 */
	addo	1,tmp,tmp
	cmpobge	1,tmp,Ls2_10		/* J/ S1 is NaN/Inf/Denormal/0 */

	bbc	31,s1_mhi,Ls2_08		/* J/ unnormal -> reserved encoding */

Ls2_02:
	bbc	AC_ZerD_mask,ac,Ls2_06	/* J/ zero divide fault not masked */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_ZerD_flag,ac,ac	/* Set zero divide flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_ZerD_flag,tmp	/* Set zero divide flag */
	modac	tmp,tmp,tmp
#endif

Ls2_04:
	ldconst	TP_INF,out_se		/* Return properly signed INF */
	ldconst	0x80000000,out_mhi
	ldconst	0x00000000,out_mlo
	or	quo_sign,out_se,out_se
	ret

Ls2_06:
	b	_AFP_Fault_Zero_Divide_T

Ls2_08:
Ls1_unnormal:
Ls2_unnormal:
	b	_AFP_Fault_Reserved_Encoding_T


Ls2_10:
	shlo	1,s1_mhi,tmp
	or	s1_mlo,tmp,tmp
	bne	Ls2_16			/* J/ S1 is INF/NaN */

	cmpobe	0,tmp,Ls2_12		/* J/ S1 is 0 */

	bbc	AC_Norm_Mode,ac,Ls2_08	/* J/ denormal, not norm mode -> fault */
	b	Ls2_02			/* J/ denormal / 0 -> zero divide */

Ls2_12:
	bbc	AC_InvO_mask,ac,Ls2_14	/* J/ inv oper fault not masked */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac	/* Set inv oper flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp	/* Set inv oper flag */
	modac	tmp,tmp,tmp
#endif
	ldconst	Standard_QNaN_se,out_se	/* Return a quiet NaN */
	ldconst	Standard_QNaN_mhi,out_mhi
	ldconst	Standard_QNaN_mlo,out_mlo
	ret

Ls2_14:
	b	_AFP_Fault_Invalid_Operation_T

	
Ls2_16:
	bbc	31,s1_mhi,Ls2_08		/* J/ NaN,INF w/o j bit -> Rsvd Encd */

	cmpobe	0,tmp,Ls2_04		/* J/ INF / 0 -> signed INF return */

Ls2_18:
	b	AFP_NaN_T		/* Handle NaN operand(s) */


Ls2_20:
	bbc	AC_Norm_Mode,ac,Ls2_08	/* J/ denormal, not norm mode -> fault */

	scanbit	s2_mhi,tmp
	bno	Ls2_22			/* J/ MS word = 0 */

	subo	tmp,31,tmp		/* Top bit num to left shift count */
	subo	tmp,1,s2_exp		/* set s2_exp value */

	subo	tmp,31,tmp2		/* word -> word bit xfer */
	addo	1,tmp2,tmp2
	shlo	tmp,s2_mhi,s2_mhi	/* Normalize denorm significand */
	shro	tmp2,s2_mlo,tmp2
	shlo	tmp,s2_mlo,s2_mlo
	or	s2_mhi,tmp2,s2_mhi
	b	Ls2_rejoin

Ls2_22:
	scanbit	s2_mlo,tmp
	subo	tmp,31,tmp
	shlo	tmp,s2_mlo,s2_mhi
	ldconst	0,s2_mlo
	subo	tmp,0,s2_exp
	subo	31,s2_exp,s2_exp
	b	Ls2_rejoin


Ls2_30:
	bbc	31,s2_mhi,Ls2_08		/* J/ NaN,INF w/o j bit -> Rsvd Encd */

	cmpobne	0,tmp,Ls2_38		/* J/ S2 is NaN */

	shri	32-15,quo_exp,tmp	/* S2 is INF - examine S1 */
	addo	1,tmp,tmp
	cmpobge	1,tmp,Ls2_34		/* J/ S1 is NaN/Inf/Denormal/0 */

	bbc	31,s1_mhi,Ls2_08		/* J/ unnormal -> reserved encoding */

Ls2_32:
	mov	quo_sign,out_se		/* Return properly signed 0 */
	movl	0,out_mlo
	ret

	
Ls2_34:
	shlo	1,s1_mhi,tmp		/* Drop integer mantissa bit */
	or	s1_mlo,tmp,tmp

	bne	Ls2_36			/* J/ S1 is NaN/INF */

	cmpobe	0,tmp,Ls2_32		/* J/ 0 / INF -> 0 */
	bbs	AC_Norm_Mode,ac,Ls2_32	/* J/ normalizing mode */
	b	Ls2_08			/* J/ S1 is denormal, not norm mode */

Ls2_36:
	bbc	31,s1_mhi,Ls2_08		/* J/ NaN,INF w/o j bit -> Rsvd Encd */

	cmpobe	0,tmp,Ls2_12		/* J/ INF / INF -> Inv Opn */
	b	Ls2_16			/* J/ S1 is NaN */

Ls2_38:
	bbs	31,s1_mhi,Ls2_18		/* J/ S1 not reserved encoding -> NaN */
	cmpobne	0,quo_exp,Ls2_08		/* J/ S1 unnormal */
	or	s1_mhi,s1_mlo,tmp
	cmpobe	0,tmp,Ls2_18		/* J/ S1 is zero -> process NaN */
	bbc	AC_Norm_Mode,ac,Ls2_08	/* J/ S1 is denormal, not norm mode */
	b	Ls2_18



/*  S1 is a special case value: +/-0, denormal, +/-INF, NaN; S2 is a non-  */
/*  zero, finite value.                                                    */

Ls1_special:
	shlo	1,s1_mhi,tmp		/* Drop integer mantissa bit */
	or	s1_mlo,tmp,tmp

	bne	LLLLLs1_04			/* J/ S1 is NaN/INF */

	cmpobe	0,tmp,Ls2_32		/* J/ 0 / finite -> 0 */
	bbc	AC_Norm_Mode,ac,Ls2_08	/* J/ S1 is denormal, not norm mode */

	scanbit	s1_mhi,tmp
	bno	Ls1_02			/* J/ MS word = 0 */

	subo	tmp,31,tmp		/* Top bit num to left shift count */
	subo	tmp,1,quo_exp		/* set quo_exp value */

	subo	tmp,31,tmp2		/* word -> word bit xfer */
	addo	1,tmp2,tmp2
	shlo	tmp,s1_mhi,s1_mhi	/* Normalize denorm significand */
	shro	tmp2,s1_mlo,tmp2
	shlo	tmp,s1_mlo,s1_mlo
	or	s1_mhi,tmp2,s1_mhi
	b	Ls1_rejoin

Ls1_02:
	scanbit	s1_mlo,tmp
	subo	tmp,31,tmp
	shlo	tmp,s1_mlo,s1_mhi
	ldconst	0,s1_mlo
	subo	tmp,0,quo_exp
	subo	31,quo_exp,quo_exp
	b	Ls1_rejoin


LLLLLs1_04:
	cmpobe	0,tmp,Ls2_04		/* J/ INF / finite -> INF */
	b	Ls2_18			/* J/ S1 is NaN */
