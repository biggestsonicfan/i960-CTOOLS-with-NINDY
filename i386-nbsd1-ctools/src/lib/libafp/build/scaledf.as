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
/*      scaledf.c - Scale Double Precision Routine (AFP-960)		      */
/*									      */
/******************************************************************************/

#include "asmopt.h"


	.file	"scaledf.as"
	.globl	___scaledfsidf

#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	AFP_RRC_D

	.globl	_AFP_Fault_Invalid_Operation_D
	.globl	_AFP_Fault_Reserved_Encoding_D

#define	DP_INF          0x7ff

#define	AC_Norm_Mode    29

#define	AC_InvO_mask    26
#define	AC_InvO_flag    18


/* Register Name Equates */

#define	s1         g0
#define s1_lo      s1
#define s1_hi      g1

#define s2         g2

#define	s1_mant_x  r3
#define	s1_mant    r4
#define s1_mant_lo s1_mant
#define s1_mant_hi r5
#define	s1_exp     r6
#define	s1_sign    r7

#define	tmp        r12
#define	tmp2       r13
#define	ac         r15

#define	con_1      g7

#define	out        g0
#define	out_lo     out
#define	out_hi     g1

#define	op_type    g4
#define	scale_type 13

	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

___scaledfsidf:
	ldconst	scale_type,op_type

#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	shro	31,s1_hi,s1_sign		/* isolate sign bit */
	shlo	31,s1_sign,s1_sign

	shlo	1,s1_hi,s1_exp			/* exp + mant (no sign bit) */
	shri	32-11,s1_exp,tmp
	addo	1,tmp,tmp			/* check s1 for potentially */
	cmpobg	2,tmp,Ls1_special		/* J/ NaN/INF, 0/denormal   */

	ldconst	0xFFF00000,tmp
	mov	s1_lo,s1_mant_lo		/* copy mantissa bits      */
	andnot	tmp,s1_hi,s1_mant_hi
	subo	tmp,s1_mant_hi,s1_mant_hi
	shro	32-11,s1_exp,s1_exp		/* right justify exponent  */
Ls1_rejoin:

/*
 * Finite, non-zero number
 */
	addo	s2,s1_exp,s1_exp	/* Scale exponent by integer src1 */
	ldconst	0,s1_mant_x		/* Exact operation */
	b	AFP_RRC_D		/* Range check */



/*  s1 is a special case value: +/-0, denormal, +/-INF, NaN  */

Ls1_special:
	cmpo	s1_lo,0			/* Condense lo word into LS bit */
	shlo	1,s1_hi,tmp		/* Drop sign bit */

#if	defined(USE_OPN_CC)
	addone	1,tmp,tmp
#else
	testne	tmp2
	or	tmp,tmp2,tmp
#endif

	ldconst	DP_INF << 21,con_1
	cmpobg	tmp,con_1,Ls1_20	/* J/ s1 is NaN */
	be	LLLLLs1_04		/* J/ s1 is INF */
	cmpobne	0,tmp,LLs1_10		/* J/ s1 is denormal */

LLLLLs1_04:				/* Scale of 0/INF/QNaN -> return s1 */
	ret

LLs1_10:
	bbc	AC_Norm_Mode,ac,Ls1_16	/* J/ denorm, not norm mode -> fault */

	shro	1,tmp,tmp		/* tmp = s1_hi w/o sign bit */
	scanbit	tmp,tmp
	bno	Ls1_14			/* J/ MS word = 0 */

Ls1_12:
	subo	tmp,20,tmp		/* Top bit num to left shift count */
	and	0x1f,tmp,tmp		/* (when MS word = 0) */
	subo	tmp,1,s1_exp		/* set s1_exp value */

	shlo	tmp,s1_hi,s1_mant_hi	/* Normalize denorm significand */
	shlo	tmp,s1_lo,s1_mant_lo
	subo	tmp,31,tmp		/* word -> word bit xfer */
	addo	1,tmp,tmp
	shro	tmp,s1_lo,tmp
	or	tmp,s1_mant_hi,s1_mant_hi
	b	Ls1_rejoin

Ls1_14:
	scanbit	s1_lo,tmp
	cmpoble	21,tmp,Ls1_12		/* J/ not a full word shift */

	subo	tmp,20,tmp
	subo	tmp,0,s1_exp		/* set s1_exp value */
	subo	31,s1_exp,s1_exp
	shlo	tmp,s1_lo,s1_mant_hi	/* 32+ bit shift */
	mov	0,s1_mant_lo
	b	Ls1_rejoin

Ls1_16:
	b	_AFP_Fault_Reserved_Encoding_D


Ls1_20:
	bbs	19,s1_hi,LLLLLs1_04		/* J/ quiet NaN -> just return it */

	bbc	AC_InvO_mask,ac,Ls1_22	/* J/ inv oper fault not masked */
#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac	/* Set inv oper flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp	/* Set inv oper flag */
	modac	tmp,tmp,tmp
#endif
	setbit	19,out_hi,out_hi	/* Convert SNaN to QNaN, return it */
	ret

Ls1_22:
	b	_AFP_Fault_Invalid_Operation_D
