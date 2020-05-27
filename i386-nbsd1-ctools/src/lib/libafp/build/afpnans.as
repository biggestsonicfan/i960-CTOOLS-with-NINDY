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
/*      afpnans.c - Single Precision NaN Operand Processing Routine (AFP-960) */
/*									      */
/******************************************************************************/

#include "asmopt.h"


	.file	"afpnans.as"
	.globl	AFP_NaN_S

	.globl	_AFP_Fault_Invalid_Operation_S


#define	FP_INF     0xFF

#define	AC_InvO_mask     26
#define	AC_InvO_flag     18


#define	tmp        r10
#define	con_1      r11
#define	con_2      r12
#define	tmp2       r13
#define	tmp3       r14

#define	ac         r15

#define	s1         g0
#define	s2         g1

#define	out        g0


#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

AFP_NaN_S:
	shlo	1,s1,tmp		/* drop sign bit */
	ldconst	FP_INF << 24,con_1
	cmpobg	tmp,con_1,LNaN_20	/* J/ S1 is a NaN */

	bbc	22,s2,LNaN_10		/* J/ S2 is an SNaN */

	mov	s2,out			/* S2 is a QNaN, quietly return it */
	setbit	31,out,out		/* Insure sign bit = 1 */
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
	mov	s2,out			/* Return quiet version of S2 */
	setbit	22,out,out
	setbit	31,out,out		/* Insure sign bit = 1 */
	ret

LNaN_14:
	b	_AFP_Fault_Invalid_Operation_S	/* Jump to fault handler */


LNaN_20:
	shlo	1,s2,tmp2
	cmpobg	tmp2,con_1,LNaN_30	/* J/ both operations are NaN's */

	bbc	22,s1,LNaN_22		/* J/ S1 is an SNaN */

	mov	s1,out			/* Return S1 */
	setbit	31,out,out		/* Insure sign bit = 1 */
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
	mov	s1,out			/* Return S1 */
	setbit	22,out,out		/* Force S1 to be a QNaN */
	setbit	31,out,out		/* Insure sign bit = 1 */
	ret

LNaN_30:
	and	s1,s2,tmp3
	bbs	22,tmp3,LNaN_32		/* J/ both operands are QNaN's */

	bbc	AC_InvO_mask,ac,LNaN_14	/* J/ Invalid Operation not masked */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac	/* Set invalid operation flag */
	st	ac,fpem_CA_AC		/* Update memory image */
#else
	ldconst	1 << AC_InvO_flag,tmp	/* Invalid operation flag bit */
	modac	tmp,tmp,tmp		/* Set invalid operation flag */
#endif

LNaN_32:
	cmpobge	tmp,tmp2,LNaN_24		/* J/ return S1 'cause it's "larger" */
	b	LNaN_12			/* return S2 'cause it's "larger" */
