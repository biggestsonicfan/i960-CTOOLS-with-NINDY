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
/*      scaletf.c - Scale Extended Precision Routine (AFP-960)		      */
/*									      */
/******************************************************************************/

#include "asmopt.h"


	.file	"scaletf.as"
	.globl	___scaletfsitf

#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	AFP_RRC_T

	.globl	_AFP_Fault_Invalid_Operation_T
	.globl	_AFP_Fault_Reserved_Encoding_T


#define	AC_Norm_Mode    29

#define	AC_InvO_mask    26
#define	AC_InvO_flag    18


/* Register Name Equates */

#define	s1         g0
#define s1_mlo     s1
#define s1_mhi     g1
#define s1_se      g2

#define s2         g4

#define	s1_mant_x  r3
#define	s1_mant    r4
#define s1_mant_lo s1_mant
#define s1_mant_hi r5
#define	s1_exp     r6
#define	s1_sign    r7

#define	tmp        r12
#define	tmp2       r13
#define	ac         r15

#define	con_1      g3

#define	out        g0
#define	out_mlo    out
#define	out_mhi    g1
#define out_se     g2

#define	op_type    g7
#define	scale_type 13

	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

___scaletfsitf:
	ldconst	scale_type,op_type
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	ldconst	0x00008000,s1_sign		/* isolate sign bit */
	and	s1_se,s1_sign,s1_sign

	shlo	32-15,s1_se,s1_exp		/* exp only (no sign bit) */
	shri	32-15,s1_exp,tmp
	addo	1,tmp,tmp			/* check s1 for potentially */
	cmpobge	1,tmp,Ls1_special		/* J/ NaN/INF, 0/denormal   */

	bbc	31,s1_mhi,Ls1_unnormal		/* J/ s1 is an unnormal */

	mov	s1_mlo,s1_mant_lo		/* copy mantissa bits      */
	mov	s1_mhi,s1_mant_hi

	shro	32-15,s1_exp,s1_exp		/* right justify exponent  */

Ls1_rejoin:

/*
 * Finite, non-zero number
 */
	addo	s2,s1_exp,s1_exp	/* Scale exponent by integer src1 */
	ldconst	0,s1_mant_x		/* Exact operation */
	b	AFP_RRC_T



/*  s1 is a special case value: +/-0, denormal, +/-INF, NaN  */

Ls1_special:
	shlo	1,s1_mhi,tmp		/* Drop j bit */
	or	s1_mlo,tmp,tmp
	bne	Ls1_20			/* J/ NaN or INF */

	cmpobne	0,tmp,LLs1_10		/* J/ s1 is denormal */

LLLLLs1_04:					/* Scale 0/INF/QNaN  ->  return s1 */
	ret


LLs1_10:
	bbc	AC_Norm_Mode,ac,Ls1_14	/* J/ denormal, not norm mode -> fault */

	scanbit	s1_mhi,tmp
	bno	Ls1_12			/* J/ MS word = 0 */

	subo	tmp,31,tmp		/* Top bit num to left shift count */
	subo	tmp,1,s1_exp		/* set s1_exp value */

	shlo	tmp,s1_mhi,s1_mant_hi	/* Normalize denorm significand */
	shlo	tmp,s1_mlo,s1_mant_lo
	subo	tmp,31,tmp		/* word -> word bit xfer */
	addo	1,tmp,tmp
	shro	tmp,s1_mlo,tmp
	or	tmp,s1_mant_hi,s1_mant_hi
	b	Ls1_rejoin

Ls1_12:
	scanbit	s1_mlo,tmp
	subo	tmp,31,tmp
	shlo	tmp,s1_mlo,s1_mant_hi	/* 32+ bit shift */
	subo	tmp,0,s1_exp		/* set s1_exp value */
	subo	31,s1_exp,s1_exp
	mov	0,s1_mant_lo
	b	Ls1_rejoin

Ls1_unnormal:
Ls1_14:
	b	_AFP_Fault_Reserved_Encoding_T


Ls1_20:
	bbc	31,s1_mhi,Ls1_14		/* J/ INF/NaN w/o j bit */

	cmpobe	0,tmp,LLLLLs1_04		/* J/ s1 is INF */

	bbs	30,s1_mhi,LLLLLs1_04		/* J/ s1 is a QNaN */

	bbc	AC_InvO_mask,ac,Ls1_28	/* J/ inv oper fault not masked */
#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac	/* Set inv oper flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp	/* Set inv oper flag */
	modac	tmp,tmp,tmp
#endif
	setbit	30,out_mhi,out_mhi	/* Convert SNaN to SNaN, return */
	ret

Ls1_28:
	b	_AFP_Fault_Invalid_Operation_T
