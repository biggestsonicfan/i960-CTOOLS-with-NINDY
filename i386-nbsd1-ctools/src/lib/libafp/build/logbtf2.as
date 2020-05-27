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
/*      logbtf2.c - Extended Precision Log-Binary (Integral) Function	      */
/*		     (AFP-960)						      */
/*									      */
/******************************************************************************/


#include "asmopt.h"

#if	!defined(KEEP_INTERNAL_LABELS)
#define	logb_t_05	L_logb_t_05
#define	logb_t_10	L_logb_t_10
#define	logb_t_40	L_logb_t_40
#define	logb_t_42	L_logb_t_42
#define	logb_t_45	L_logb_t_45
#define	logb_t_50	L_logb_t_50
#define	logb_t_55	L_logb_t_55
#define	logb_t_60	L_logb_t_60
#define	logb_t_65	L_logb_t_65
#define	logb_t_67	L_logb_t_67
#endif


	.file	"logbtf2.as"
	.globl	___logbtf2


#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	_AFP_Fault_Reserved_Encoding_T
	.globl	_AFP_Fault_Invalid_Operation_T
	.globl	_AFP_Fault_Zero_Divide_T


#define	TP_Bias    0x3FFF
#define	TP_INF     0x7FFF

#define	AC_Norm_Mode   29

#define	AC_ZerD_mask   27
#define	AC_ZerD_flag   19

#define	AC_InvO_mask   26
#define	AC_InvO_flag   18


#define	s1         g0
#define	s1_mlo     s1
#define	s1_mhi     g1
#define	s1_se      g2

#define	s1_mant    r4
#define	s1_exp     r5
#define	s1_sign    r6

#define	tmp        r11
#define	tmp2       r12
#define	con_1      r13
#define	con_2      r14

#define	ac         r15


#define	out        g0
#define	out_mlo    out
#define	out_mhi    g1
#define	out_se     g2


#define	op_type    g7
#define	logb_type  14


	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

___logbtf2:
	mov	logb_type,op_type		/* Operation type (if fault)	*/

#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac			/* Fetch AC			*/
#else
	modac	0,0,ac				/* Get AC			*/
#endif

	shlo	16+1,s1_se,s1_exp		/* Drop sign bit		*/
	lda	TP_Bias,con_1

	shro	32-15,s1_exp,s1_exp
	lda	TP_INF,con_2

	cmpobe.f 0,s1_exp,logb_t_40		/* J/ zero or denorm		*/

	bbc.f	31,s1_mhi,logb_t_45		/* J/ unnormal			*/

#if	!defined(USE_CMP_BCC)
	cmpo	s1_exp,con_2
#endif

	subo	con_1,s1_exp,s1_exp		/* Compute unbiased exponent	*/

#if	defined(USE_CMP_BCC)
	cmpobe	s1_exp,con_2,logb_t_60		/* J/ INF or NaN arg		*/
#else
	be.f	logb_t_60			/* J/ INF or NaN arg		*/
#endif

logb_t_05:
	shri	31,s1_exp,s1_sign		/* Take absolute value		*/
	xor	s1_exp,s1_sign,s1_exp
	subo	s1_sign,s1_exp,s1_mant

	scanbit	s1_mant,tmp
	bno.f	logb_t_10			/* Result = 0			*/

	addo	tmp,con_1,s1_exp		/* Result exponent		*/
	subo	tmp,31,tmp			/* Left shift to normalize	*/

	shlo	tmp,s1_mant,s1_mant

logb_t_10:
	shlo	31,s1_sign,s1_sign		/* Mix in sign bit for result	*/
	shro	16,s1_sign,s1_sign
	or	s1_exp,s1_sign,out_se
	mov	s1_mant,out_mhi
	movlda(0,out_mlo)
	ret


logb_t_40:
	or	s1_mlo,s1_mhi,tmp
	cmpobe	0,tmp,logb_t_50			/* J/ 0.0 input value		*/

	bbc.f	AC_Norm_Mode,ac,logb_t_45	/* Denorm, not norm mode	*/

	scanbit	s1_mhi,tmp
	bno	logb_t_42

	subo	30,tmp,tmp			/* Compute would-have-been exp	*/
	subo	con_1,tmp,s1_exp
	b	logb_t_05			/* Rejoin normal flow		*/


logb_t_42:
	scanbit	s1_mlo,tmp

	subo	31,tmp,s1_exp			/* Compute would-have-been exp	*/
	subo	1+30,s1_exp,s1_exp
	subo	con_1,s1_exp,s1_exp
	b	logb_t_05


logb_t_45:
	b	_AFP_Fault_Reserved_Encoding_T


logb_t_50:
	bbc	AC_ZerD_mask,ac,logb_t_55	/* J/ zero div fault not masked	*/

#if	defined(USE_SIMULATED_AC)
	setbit	AC_ZerD_flag,ac,ac		/* Set zero divide flag		*/
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_ZerD_flag,tmp		/* Set zero divide flag		*/
	modac	tmp,tmp,tmp
#endif

	mov	0,out_mlo
	onebit(31,out_mhi)
	lda	(1 << 15)+TP_INF,out_se		/* Return -INF			*/

	ret


logb_t_55:
	b	_AFP_Fault_Zero_Divide_T


logb_t_60:
	shlo	1,s1_mhi,tmp
	or	s1_mlo,tmp,tmp
	cmpobne.f 0,tmp,logb_t_65		/* J/NaN arg			*/

	clrbit	15,s1_se,out_se			/* Return +INF			*/
/* **	movlda(0,out_mlo)  ** */
	ret


logb_t_65:
	bbc.f	30,s1_mhi,logb_t_67		/* J/ SNaN			*/

	setbit	15,s1_se,s1_se			/* Insure QNaN's sign bit = 1	*/
	ret


logb_t_67:
	chkbit	AC_InvO_mask,ac
	bf.f	_AFP_Fault_Invalid_Operation_T	/* J/ fault on Inv Opn	*/

#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp
	modac	tmp,tmp,tmp
#endif

	setbit	31,s1_mhi,s1_mhi		/* Set QNaN bits		*/
	setbit	30,s1_mhi,s1_mhi
	setbit	15,s1_se,s1_se			/* Insure sign bit = 1		*/

	ret
