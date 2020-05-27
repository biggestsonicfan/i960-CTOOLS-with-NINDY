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
/*      logbsf2.c - Single Precision Log-Binary (Integral) Function	      */
/*		     (AFP-960)						      */
/*									      */
/******************************************************************************/


#include "asmopt.h"

#if	!defined(KEEP_INTERNAL_LABELS)
#define	logb_s_05	L_logb_s_05
#define	logb_s_10	L_logb_s_10
#define	logb_s_40	L_logb_s_40
#define	logb_s_45	L_logb_s_45
#define	logb_s_50	L_logb_s_50
#define	logb_s_55	L_logb_s_55
#define	logb_s_60	L_logb_s_60
#define	logb_s_65	L_logb_s_65
#define	logb_s_67	L_logb_s_67
#endif


	.file	"logbsf2.as"
	.globl	___logbsf2


#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	_AFP_Fault_Reserved_Encoding_S
	.globl	_AFP_Fault_Invalid_Operation_S
	.globl	_AFP_Fault_Zero_Divide_S


#define	FP_Bias    0x7F
#define	FP_INF     0xFF

#define	AC_Norm_Mode     29

#define	AC_ZerD_mask    27
#define	AC_ZerD_flag    19

#define	AC_InvO_mask     26
#define	AC_InvO_flag     18


#define	s1         g0

#define	s1_mant    r4
#define	s1_exp     r5
#define	s1_sign    r6

#define	tmp        r11
#define	tmp2       r12
#define	con_1      r13
#define	con_2      r14

#define	ac         r15

#define	out        g0

#define	op_type    g2
#define	logb_type  14


	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

___logbsf2:
	mov	logb_type,op_type		/* Operation type (if fault)	*/

#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac			/* Fetch AC			*/
#else
	modac	0,0,ac				/* Get AC			*/
#endif

	shlo	1,s1,s1_exp			/* Drop sign bit		*/
	lda	FP_Bias,con_1

	shro	32-8,s1_exp,s1_exp
	lda	FP_INF,con_2

	cmpobe.f 0,s1_exp,logb_s_40		/* J/ zero or denorm		*/

#if	!defined(USE_CMP_BCC)
	cmpo	s1_exp,con_2
#endif

	subo	con_1,s1_exp,s1_exp		/* Compute unbiased exponent	*/

#if	defined(USE_CMP_BCC)
	cmpobe	s1_exp,con_2,logb_s_60		/* J/ INF or NaN arg		*/
#else
	be.f	logb_s_60			/* J/ INF or NaN arg		*/
#endif

logb_s_05:
	shri	31,s1_exp,s1_sign		/* Take absolute value		*/
	xor	s1_exp,s1_sign,s1_exp
	subo	s1_sign,s1_exp,s1_mant

	scanbit	s1_mant,tmp
	bno.f	logb_s_10			/* Result = 0			*/

	addo	tmp,con_1,s1_exp		/* Result exponent		*/
	subo	tmp,23,tmp			/* Left shift to normalize	*/

	shlo	tmp,s1_mant,s1_mant
	shlo	23,s1_exp,s1_exp		/* Position exponent		*/

	clrbit	23,s1_mant,s1_mant		/* Delete "j" bit		*/
	or	s1_exp,s1_mant,s1_mant		/* Significand plus exponent	*/

logb_s_10:
	shlo	31,s1_sign,s1_sign		/* Mix in sign bit for result	*/
	or	s1_mant,s1_sign,out
	ret


logb_s_40:
	shlo	1,s1,tmp
	cmpobe	0,tmp,logb_s_50			/* J/ 0.0 input value		*/

	bbc.f	AC_Norm_Mode,ac,logb_s_45	/* J/ not normalizing mode	*/

	scanbit	tmp,tmp
	subo	22+1,tmp,tmp			/* Compute would-have-been exp	*/
	subo	con_1,tmp,s1_exp
	b	logb_s_05			/* Rejoin normal flow		*/


logb_s_45:
	b	_AFP_Fault_Reserved_Encoding_S


logb_s_50:
	bbc	AC_ZerD_mask,ac,logb_s_55	/* J/ zero div fault not masked	*/

#if	defined(USE_SIMULATED_AC)
	setbit	AC_ZerD_flag,ac,ac		/* Set zero divide flag		*/
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_ZerD_flag,tmp		/* Set zero divide flag		*/
	modac	tmp,tmp,tmp
#endif

	lda	(1 << 31)+(FP_INF << 23),out	/* Return -INF		*/
	ret


logb_s_55:
	b	_AFP_Fault_Zero_Divide_S


logb_s_60:
	shlo	9,s1,tmp
	cmpobne.f 0,tmp,logb_s_65		/* J/ NaN arg			*/

	clrbit	31,s1,out			/* Return +INF			*/
	ret


logb_s_65:
	bbc.f	22,s1,logb_s_67			/* J/ SNaN			*/

	setbit	31,s1,out			/* Insure QNaN's sign bit = 1	*/
	ret


logb_s_67:
	chkbit	AC_InvO_mask,ac
	bf.f	_AFP_Fault_Invalid_Operation_S	/* J/ fault on Inv Opn	*/

#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp
	modac	tmp,tmp,tmp
#endif

	setbit	22,s1,s1			/* Set QNaN bit			*/
	setbit	31,s1,out			/* Insure sign bit = 1		*/

	ret
