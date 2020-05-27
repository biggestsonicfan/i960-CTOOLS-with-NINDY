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
/*      afpnand.c - Double Precision NaN Operand Processing Routine (AFP-960) */
/*									      */
/******************************************************************************/

#include "asmopt.h"


	.file	"afpnand.as"
	.globl	AFP_NaN_D

	.globl	_AFP_Fault_Invalid_Operation_D


#define	DP_INF     0x7FF

#define	AC_InvO_mask     26
#define	AC_InvO_flag     18


#define	tmp        r10
#define	con_1      r11
#define	con_2      r12
#define	tmp2       r13
#define	tmp3       r14

#define	ac         r15

#define	s1         g0
#define	s1_lo      s1
#define	s1_hi      g1
#define	s2         g2
#define	s2_lo      s2
#define	s2_hi      g3
#define	op_type    g4

#define	out        g0
#define	out_lo     out
#define	out_hi     g1


#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

AFP_NaN_D:
	cmpo	s1_lo,0
	shlo	1,s1_hi,tmp		/* drop sign bit */

#if	defined(USE_OPN_CC)
	addone	1,tmp,tmp		/* condense _lo to one bit */
#else
	testne	tmp2			/* condense _lo to one bit */
	or	tmp,tmp2,tmp
#endif

	ldconst	DP_INF << 21,con_1
	cmpobg	tmp,con_1,LNaN_20	/* J/ S1 is a NaN */

	bbc	19,s2_hi,LNaN_10		/* J/ S2 is an SNaN */

	movl	s2,out			/* S2 is a QNaN, quietly return it */
	setbit	31,out_hi,out_hi	/* Insure sign bit = 1 */
	ret

LNaN_10:
	bbc	AC_InvO_mask,ac,LNaN_14	/* J/ Invalid Oper fault not masked */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac	/* Set invalid operation flag */
	st	ac,fpem_CA_AC		/* Update memory image */
#else
	ldconst	1 << AC_InvO_flag,tmp	/* Invalid operation flag bit */
	modac	tmp,tmp,tmp		/* Set invalid operation flag */
#endif

LNaN_12:
	movl	s2,out			/* Return quiet version of S2 */
	setbit	19,out_hi,out_hi
	setbit	31,out_hi,out_hi	/* Insure sign bit = 1 */
	ret

LNaN_14:
	b	_AFP_Fault_Invalid_Operation_D	/* Jump to fault handler */


LNaN_20:
	cmpo	s2_lo,0
	shlo	1,s2_hi,tmp2

#if	defined(USE_OPN_CC)
	addone	1,tmp2,tmp2		/* condense _lo to one bit */
#else
	testne	tmp3			/* condense _lo to one bit */
	or	tmp2,tmp3,tmp2
#endif

	cmpobg	tmp2,con_1,LNaN_30	/* J/ both operations are NaN's */

	bbc	19,s1_hi,LNaN_22		/* J/ S1 is an SNaN */

	movl	s1,out			/* Return S1 */
	setbit	31,out_hi,out_hi	/* Insure sign bit = 1 */
	ret

LNaN_22:
	bbc	AC_InvO_mask,ac,LNaN_14	/* J/ Invalid Operation not masked */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac	/* Set invalid operation flag */
	st	ac,fpem_CA_AC		/* Update memory image */
#else
	ldconst	1 << AC_InvO_flag,tmp	/* Invalid operation flag bit */
	modac	tmp,tmp,tmp		/* Set invalid operation flag */
#endif

LNaN_24:
	movl	s1,out			/* Return S1 */
	setbit	19,out_hi,out_hi	/* Force S1 to be a QNaN */
	setbit	31,out_hi,out_hi	/* Insure sign bit = 1 */
	ret

LNaN_30:
	and	s1_hi,s2_hi,tmp3
	bbs	19,tmp3,LNaN_32		/* J/ both operands are QNaN's */

	bbc	AC_InvO_mask,ac,LNaN_14	/* J/ Invalid Operation not masked */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac	/* Set invalid operation flag */
	st	ac,fpem_CA_AC		/* Update memory image */
#else
	ldconst	1 << AC_InvO_flag,tmp	/* Invalid operation flag bit */
	modac	tmp,tmp,tmp		/* Set invalid operation flag */
#endif

LNaN_32:
	shlo	1,s1_hi,tmp
	shlo	1,s2_hi,tmp2
	cmpobg	tmp,tmp2,LNaN_24		/* J/ return S1 'cause it's "larger" */
	bl	LNaN_12			/* J/ return S2 */
	cmpobge	s1_lo,s2_lo,LNaN_24	/* J/ return S1 'cause it's "larger" */
	b	LNaN_12			/* return S2 'cause it's "larger" */
