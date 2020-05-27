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
/*      fltsisf.c - Integer to Single Precision Conversion Routine (AFP-960)  */
/*									      */
/******************************************************************************/

#include "asmopt.h"

#if	!defined(KEEP_INTERNAL_LABELS)
#define	sisf_rejoin	L_sisf_rejoin
#define	sisf_05		L_sisf_05
#define	sisf_10		L_sisf_10
#endif


	.file	"fltsisf.as"
	.globl	___floatunssisf
	.globl	___floatsisf


#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	AFP_RRC_S

#define	FP_Bias    0x7F

#define	s1         g0

#define	s2_mant_x  r4
#define	s2_mant    r5
#define	s2_exp     r3
#define	s2_sign    r6

#define	tmp        r10
#define	con_1      r11
#define	con_2      r12
#define	tmp2       r13

#define	ac         r15

#define	out        g0

#define	op_type    g2
#define	cnv_i_type 4
#define	cnv_u_type 5


	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

___floatunssisf:
	mov	cnv_u_type,op_type
	movlda(0,s2_sign)		/* Positive result only */

	mov	s1,s2_mant
	b	sisf_rejoin


___floatsisf:
	shri	31,s1,tmp		/* -1 or 0 based on sign */
	movlda(cnv_i_type,op_type)

	shlo	31,tmp,s2_sign		/* sign of result */

	xor	tmp,s1,s2_mant
	subo	tmp,s2_mant,s2_mant

sisf_rejoin:
	scanbit	s2_mant,tmp		/* Find MS bit */

	ldconst	32,con_2
	bno.f	sisf_05			/* J/ zero value */

#if	!defined(USE_CMP_BCC)
	cmpo	23,tmp
#endif

#if	defined(USE_LDA_REG_OFF)
	lda	FP_Bias(tmp),s2_exp	/* Result exponent */
#else
	ldconst	FP_Bias,con_1
	addo	con_1,tmp,s2_exp	/* Result exponent */
#endif

	subo	tmp,con_2,tmp2		/* Left shift for exact conversion */

#if	defined(USE_CMP_BCC)
	cmpobl	23,tmp,sisf_10		/* J/ possibly inexact result */
#else
	bl.f	sisf_10			/* J/ possibly inexact result */
#endif

	shlo	tmp2,s2_mant,s2_mant	/* Shift dropping MS bit */
	addo	s2_exp,s2_mant,out	/* Normalized signif + exp */
        rotate	23,out,out		/* Position exp/significand */
	or	s2_sign,out,out		/* mix in sign */
	ret


sisf_05:
	ldconst	0,out			/* +0 result */
	ret


sisf_10:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	subo	23,tmp,tmp		/* Right shift count */
	subo	tmp,con_2,tmp2		/* Left shift count */
	shlo	tmp2,s2_mant,s2_mant_x	/* Rounding bits */
	shro	tmp,s2_mant,s2_mant	/* Normalize significand */
	b	AFP_RRC_S		/* Round/inexact check */
