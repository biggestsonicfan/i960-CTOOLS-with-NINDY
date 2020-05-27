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
/*      multf3.c - Extended Precision Multiplication Routine (AFP-960)	      */
/*									      */
/******************************************************************************/

#include "asmopt.h"


	.file	"multf3.s"
	.globl	___multf3

#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	AFP_RRC_T
	.globl	AFP_NaN_T

	.globl	_AFP_Fault_Invalid_Operation_T
	.globl	_AFP_Fault_Reserved_Encoding_T


#define	TP_Bias         0x3fff
#define	TP_INF          0x7fff

#define	AC_Round_Mode   30
#define	Round_Mode_Down 2

#define	AC_Norm_Mode    29

#define	AC_InvO_mask    26
#define	AC_InvO_flag    18

#define	Standard_QNaN_se  0x0000ffff
#define Standard_QNaN_mhi 0xc0000000
#define	Standard_QNaN_mlo 0x00000000


/* Register Name Equates */

#define	s1         g0
#define	s1_mlo     s1
#define s1_mhi     g1
#define	s1_se      g2
#define s2         g4
#define s2_mlo     s2
#define s2_mhi     g5
#define s2_se      g6

#define	rsl_mant_x   r3
#define rsl_mant     r4
#define rsl_mant_lo  rsl_mant
#define rsl_mant_hi  r5
#define	rsl_exp      r6
#define	rsl_sign     r7

#define	rp_A       r8
#define	rp_A_lo    rp_A
#define	rp_A_hi    r9

#define	rp_B       r10
#define	rp_B_lo    rp_B
#define	rp_B_hi    r11

#define	rp_C       r12
#define	rp_C_lo    rp_C
#define	rp_C_hi    r13

#define	tmp        r14

#define	ac         r15

#define	s1_exp     g3

#define	out        g0
#define	out_mlo    out
#define	out_mhi    g1
#define out_se     g2

#define	op_type    g7
#define	mul_type   3


	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

___multf3:
	ldconst	mul_type,op_type

#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	emul	s1_mhi,s2_mhi,rsl_mant		/* Overlap mul (CA) */

	xor	s1_se,s2_se,rsl_sign		/* Compute and isolate ... */
	ldconst	0x00008000,tmp			/* ... result sign	   */
	and	rsl_sign,tmp,rsl_sign

	shlo	32-15,s1_se,s1_exp		/* Delete sign bit */
	shlo	32-15,s2_se,rsl_exp

	emul	s1_mhi,s2_mlo,rp_C

	shri	32-15,s1_exp,tmp
	addo	1,tmp,tmp
	cmpobge	1,tmp,Ls1_special		/* J/ S1 is NaN/INF/0/denorm */

	bbc	31,s1_mhi,Ls1_unnormal		/* J/ S1 is a unnormal! */

	shro	32-15,s1_exp,s1_exp		/* right just, zero fill exp */

	mov	s1_mlo,rp_A_lo			/* for denormalized case */
	mov	s1_mhi,rp_A_hi

Ls1_rejoin:

	emul	rp_A_lo,s2_mhi,rp_B

	shri	32-15,rsl_exp,tmp
	addo	1,tmp,tmp
	cmpobge	1,tmp,Ls2_special		/* J/ S2 is NaN/INF/0/denorm */

	bbc	31,s2_mhi,Ls2_unnormal		/* J/ S2 is a unnormal! */

	emul	rp_A_lo,s2_mlo,rp_A

	shro	32-15,rsl_exp,rsl_exp		/* right just, zero fill exp */

Ls2_rejoin:

/*
 * Partial product multiplies are complete.  Compute result exponent and
 * combine partial products.
 */
	cmpo	1,2				/* clear carry bit */

	addo	s1_exp,rsl_exp,rsl_exp		/* compute result exp    ... */

	addc	rp_C_lo,rp_B_lo,rp_B_lo
	addc	rp_C_hi,rp_B_hi,rp_B_hi
	addc	0,0,tmp
	cmpo	1,2
	addc	rp_A_hi,rp_B_lo,rsl_mant_x
	addc	rsl_mant_lo,rp_B_hi,rsl_mant_lo
	addc	rsl_mant_hi,tmp,rsl_mant_hi

	cmpo	0,rp_A_lo			/* Lo 32 bits = 0? */
	ldconst	TP_Bias-1,tmp

#if	defined(USE_OPN_CC)
	selne	0,1,rp_A_lo			/* Condense 32 bits to 1 bit */
#else
	testne	rp_A_lo				/* Condense 32 bits to 1 bit */
#endif

	chkbit	31,rsl_mant_hi			/* Check for normalized */
	or	rsl_mant_x,rp_A_lo,rsl_mant_x
	subo	tmp,rsl_exp,rsl_exp		/* remove excess bias */
	bt	AFP_RRC_T			/* J/ normalized result */

	addc	rsl_mant_x,rsl_mant_x,rsl_mant_x  /* Normalizing shift */
	subo	1,rsl_exp,rsl_exp		/* Adjust result exponent */
	addc	rsl_mant_lo,rsl_mant_lo,rsl_mant_lo
	addc	rsl_mant_hi,rsl_mant_hi,rsl_mant_hi
	b	AFP_RRC_T


/*  S1 is a special case value: +/-0, denormal, +/-INF, NaN; S2 is unknown  */

Ls1_special:
	shlo	1,s1_mhi,tmp		/* Drop j bit */
	or	s1_mlo,tmp,tmp
	bne	Ls1_05			/* J/ NaN or INF */

	cmpobne	0,tmp,LLs1_10		/* J/ S1 is denormal */

	shri	32-15,rsl_exp,tmp	/* S1 is 0; examine S2 */
	addo	1,tmp,tmp
	cmpobge	1,tmp,Ls1_01		/* J/ S2 is NaN/INF/denormal/0 */
	bbs	31,s2_mhi,LLLLLs1_04		/* J/ 0 * finite -> 0 */
	b	Ls1_02			/* J/ S2 unnormal -> rsvd encoding */

Ls1_01:
	shlo	1,s2_mhi,tmp		/* Drop j bit */
	or	s2_mlo,tmp,tmp
	bne	Ls1_23			/* J/ S2 is NaN/INF */

	cmpobe	0,tmp,LLLLLs1_04		/* J/ 0 * 0 -> 0 */

	bbs	AC_Norm_Mode,ac,LLLLLs1_04	/* J/ normalizing mode */
Ls1_02:
Ls1_unnormal:
Ls2_unnormal:
	b	_AFP_Fault_Reserved_Encoding_T

LLLLLs1_04:
	mov	rsl_sign,out_se		/* Return properly signed zero */
	movl	0,out
	ret

Ls1_05:
	bbc	31,s1_mhi,Ls1_02		/* J/ INF/NaN w/o j bit */

	cmpobe	0,tmp,Ls1_20		/* J/ S1 is INF */

	bbs	31,s2_mhi,Ls1_06		/* J/ S2 not reserved encoding -> NaN */
	cmpobne	0,rsl_exp,Ls1_02		/* J/ S2 unnormal (exp <> 0, j = 0) */
	or	s2_mhi,s2_mlo,tmp
	cmpobe	0,tmp,Ls1_06		/* J/ S2 is zero -> process NaN */
	bbc	AC_Norm_Mode,ac,Ls1_02	/* J/ S2 is denormal, not norm mode */

Ls1_06:
	b	AFP_NaN_T		/* Handle NaN operand(s) */


LLs1_10:
	bbc	AC_Norm_Mode,ac,Ls1_02	/* J/ denormal, not norm mode -> fault */

	scanbit	s1_mhi,tmp
	bno	Ls1_14			/* J/ MS word = 0 */

	subo	tmp,31,tmp		/* Top bit num to left shift count */
	subo	tmp,1,s1_exp		/* set s1_exp value */

	shlo	tmp,s1_mhi,rp_A_hi	/* Normalize denorm significand */
	shlo	tmp,s1_mlo,rp_A_lo
	subo	tmp,31,tmp		/* word -> word bit xfer */
	addo	1,tmp,tmp
	shro	tmp,s1_mlo,tmp
	or	tmp,rp_A_hi,rp_A_hi
	emul	rp_A_hi,s2_mhi,rsl_mant	/* redo initial overlapped mult's */
	emul	rp_A_hi,s2_mlo,rp_C
	b	Ls1_rejoin

Ls1_14:
	scanbit	s1_mlo,tmp
	subo	tmp,31,tmp
	shlo	tmp,s1_mlo,rp_A_hi	/* 32+ bit shift */
	emul	rp_A_hi,s2_mhi,rsl_mant	/* redo initial overlapped mult's */
	subo	tmp,0,s1_exp		/* set s1_exp value */
	subo	31,s1_exp,s1_exp
	mov	0,rp_A_lo
	emul	rp_A_hi,s2_mlo,rp_B
	b	Ls1_rejoin


Ls1_20:
	shri	32-15,rsl_exp,tmp	/* S1 is INF; examine S2 */
	addo	1,tmp,tmp
	cmpobge	1,tmp,Ls1_21		/* J/ S2 is NaN/INF/denormal/0 */
	bbs	31,s2_mhi,Ls1_22		/* J/ INF * finite -> INF */
	b	Ls1_02			/* J/ S2 unnormal -> rsvd encoding */

Ls1_21:
	shlo	1,s2_mhi,tmp		/* Drop j bit */
	or	s2_mlo,tmp,tmp
	be	LLs1_21a			/* J/ S2 is denormal/0 */

	bbc	31,s2_mhi,Ls1_02		/* J/ INF/NaN w/o j bit */
	cmpobne	0,tmp,Ls1_06		/* S2 is NaN -> NaN handler */
	b	Ls1_22			/* J/ INF * INF */

LLs1_21a:
	cmpobe	0,tmp,Ls1_24		/* J/ INF * 0 -> inv opn */
	bbc	AC_Norm_Mode,ac,Ls1_02	/* J/ S2 is a denormal -> rsvd encd */
	
Ls1_22:
	ldconst	TP_INF,out_se		/* Return properly signed INF */
	ldconst	0x80000000,out_mhi
	ldconst	0x00000000,out_mlo
	or	rsl_sign,out_se,out_se
	ret


Ls1_23:
	bbc	31,s2_mhi,Ls1_02		/* J/ INF/NaN w/o j bit */
	cmpobne	0,tmp,Ls1_06		/* J/ S2 is NaN */
					/* Fall through for 0 * INF */
Ls1_24:
	bbc	AC_InvO_mask,ac,Ls1_25	/* J/ inv oper fault not masked */

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

Ls1_25:
	b	_AFP_Fault_Invalid_Operation_T


Ls1_30:
	bbs	31,s2_mhi,Ls1_06		/* J/ S2 not reserved encoding -> NaN */
	cmpobne	0,rsl_exp,Ls1_02		/* J/ S2 unnormal */
	or	s2_mhi,s2_mlo,tmp
	cmpobe	0,tmp,Ls1_06		/* J/ S2 is zero -> process NaN */
	bbc	AC_Norm_Mode,ac,Ls1_02	/* J/ S2 is denormal, not norm mode */
	b	Ls1_06


/*  S2 is a special case value: +/-0, denormal, +/-INF, NaN; S1 is a non-  */
/*  zero, finite value.                                                    */

Ls2_special:
	shlo	1,s2_mhi,tmp		/* Drop j bit */
	or	s2_mlo,tmp,tmp
	be	Ls2_10			/* J/ denormal/zero */

	bbc	31,s2_mhi,Ls1_02		/* J/ INF/NaN w/o j bit */
	cmpobne	0,tmp,Ls1_06		/* J/ S2 is a NaN */
	b	Ls1_22			/* J/ INF * finite -> INF */

Ls2_10:
	cmpobe	0,tmp,LLLLLs1_04		/* J/ 0 * finite -> 0 */
	bbc	AC_Norm_Mode,ac,Ls1_02	/* J/ denormal, not norm mode -> fault */

	scanbit	s2_mhi,tmp
	bno	Ls2_14			/* J/ MS word = 0 */

	subo	tmp,31,tmp		/* Top bit num to left shift count */
	subo	tmp,1,rsl_exp		/* set rsl_exp value */

	shlo	tmp,s2_mhi,rp_B_hi	/* Normalize denorm significand */
	shlo	tmp,s2_mlo,rp_B_lo
	subo	tmp,31,tmp		/* word -> word bit xfer */
	addo	1,tmp,tmp
	shro	tmp,s2_mlo,tmp
	or	rp_B_hi,tmp,rp_B_hi

Ls2_12:
	emul	rp_A_hi,rp_B_hi,rsl_mant	/* redo initial overlapped mult's */
	emul	rp_A_lo,rp_B_hi,rp_C
	mov	rp_B_lo,tmp
	emul	rp_A_hi,tmp,rp_B
	emul	rp_A_lo,tmp,rp_A
	b	Ls2_rejoin

Ls2_14:
	scanbit	s2_mlo,tmp
	subo	tmp,31,tmp
	shlo	tmp,s2_mlo,rp_B_hi		/* 32+ bit shift */
	emul	rp_A_hi,rp_B_hi,rsl_mant	/* redo initial overlapped mult's */
	subo	tmp,0,rsl_exp			/* set rsl_exp value */
	subo	31,rsl_exp,rsl_exp
	emul	rp_A_lo,rp_B_hi,rp_B
	movl	0,rp_A				/* since lo word of S2 is 0 */
	movl	0,rp_C
	b	Ls2_rejoin
