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
/*      afpnant.c - Extended Precision NaN Operand Processing Routine	      */
/*		    (AFP-960)						      */
/*									      */
/******************************************************************************/

#include "asmopt.h"


	.file	"afpnant.as"
	.globl	AFP_NaN_T

	.globl	_AFP_Fault_Invalid_Operation_T


#define	TP_INF     0x7FFF

#define	AC_InvO_mask     26
#define	AC_InvO_flag     18


#define	tmp        r10
#define	con_1      r11
#define	con_2      r12
#define	tmp2       r13
#define	tmp3       r14

#define	ac         r15

#define	s1         g0
#define	s1_mlo     s1
#define	s1_mhi     g1
#define s1_se      g2

#define	s2         g4
#define	s2_mlo     s2
#define	s2_mhi     g5
#define s2_se      g6

#define	op_type    g7

#define	out        g0
#define	out_mlo    out
#define	out_mhi    g1
#define out_se     g2


#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

AFP_NaN_T:
	shlo	1,s1_mhi,tmp		/* drop j bit */
	or	tmp,s1_mlo,tmp
	cmpo	tmp,0
	shlo	32-15,s1_se,tmp

#if	defined(USE_OPN_CC)
	addone	1,tmp,tmp
#else
	testne	tmp2
	or	tmp,tmp2,tmp
#endif

	ldconst	TP_INF << (32-15),con_1
	cmpobg	tmp,con_1,LNaN_20	/* J/ S1 is a NaN */

	bbc	30,s2_mhi,LNaN_10	/* J/ S2 is an SNaN */

	movt	s2,out			/* S2 is a QNaN, quietly return it */
	setbit	15,out_se,out_se	/* Insure sign bit = 1 */
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
	movt	s2,out			/* Return quiet version of S2 */
	setbit	30,out_mhi,out_mhi
	setbit	15,out_se,out_se	/* Insure sign bit = 1 */
	ret

LNaN_14:
	b	_AFP_Fault_Invalid_Operation_T	/* Jump to fault handler */


LNaN_20:
	shlo	1,s2_mhi,tmp		/* drop j bit */
	or	tmp,s2_mlo,tmp
	cmpo	tmp,0
	shlo	32-15,s2_se,tmp

#if	defined(USE_OPN_CC)
	addone	1,tmp,tmp
#else
	testne	tmp2
	or	tmp,tmp2,tmp
#endif

	cmpobg	tmp,con_1,LNaN_30	/* J/ both operations are NaN's */

	bbc	30,s1_mhi,LNaN_22	/* J/ S1 is an SNaN */

	movt	s1,out			/* Return S1 */
	setbit	15,out_se,out_se	/* Insure sign bit = 1 */
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
	movt	s1,out			/* Return S1 */
	setbit	30,out_mhi,out_mhi	/* Force S1 to be a QNaN */
	setbit	15,out_se,out_se	/* Insure sign bit = 1 */
	ret

LNaN_30:
	and	s1_mhi,s2_mhi,tmp3
	bbs	30,tmp3,LNaN_32		/* J/ both operands are QNaN's */

	bbc	AC_InvO_mask,ac,LNaN_14	/* J/ Invalid Operation not masked */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac	/* Set invalid operation flag */
	st	ac,fpem_CA_AC		/* Update memory image */
#else
	ldconst	1 << AC_InvO_flag,tmp	/* Invalid operation flag bit */
	modac	tmp,tmp,tmp		/* Set invalid operation flag */
#endif

LNaN_32:
	cmpobg	s1_mhi,s2_mhi,LNaN_24	/* J/ return S1 'cause it's "larger" */
	bl	LNaN_12			/* J/ return S2 */
	cmpobge	s1_mlo,s2_mlo,LNaN_24	/* J/ return S1 'cause it's "larger" */
	b	LNaN_12			/* return S2 'cause it's "larger" */
