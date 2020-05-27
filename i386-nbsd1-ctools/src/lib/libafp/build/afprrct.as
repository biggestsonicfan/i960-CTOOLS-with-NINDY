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
/*      afprrct.c - Extended Precision Round/Range Check Routine (AFP-960)    */
/*									      */
/******************************************************************************/

#include "asmopt.h"

#if	!defined(KEEP_INTERNAL_LABELS)
#define	rrc_t_02	L_rrc_t_02
#define	rrc_t_10	L_rrc_t_10
#define	rrc_t_12	L_rrc_t_12
#define	rrc_t_20	L_rrc_t_20
#define	rrc_t_21	L_rrc_t_21
#define	rrc_t_30	L_rrc_t_30
#define	rrc_t_40	L_rrc_t_40
#define	rrc_t_42	L_rrc_t_42
#define	rrc_t_43	L_rrc_t_43
#define	rrc_t_46	L_rrc_t_46
#define	rrc_t_48	L_rrc_t_48
#define	rrc_t_51	L_rrc_t_51
#define	rrc_t_52	L_rrc_t_52
#define	rrc_t_54	L_rrc_t_54
#define	rrc_t_56	L_rrc_t_56
#define	rrc_t_57	L_rrc_t_57
#define	rrc_t_58	L_rrc_t_58
#define	rrc_t_59	L_rrc_t_59
#define	rrc_t_60a	L_rrc_t_60a
#define	rrc_t_60b	L_rrc_t_60b
#define	rrc_t_61	L_rrc_t_61
#define	rrc_t_62	L_rrc_t_62
#define	rrc_t_64	L_rrc_t_64
#define	rrc_t_65	L_rrc_t_65
#define	rrc_t_66	L_rrc_t_66
#define	rrc_t_66b	L_rrc_t_66b
#define	rrc_t_67	L_rrc_t_67
#endif


	.file	"afprrct.as"
	.globl	AFP_RRC_T

	.globl	_AFP_Fault_Inexact_T
	.globl	_AFP_Fault_Overflow_T
	.globl	_AFP_Fault_Underflow_T


#define	TP_INF     0x7fff

#define	AC_Inex_mask     28
#define	AC_Inex_flag     20
#define	AC_Ovfl_mask     24
#define	AC_Ovfl_flag     16
#define	AC_Unfl_mask     25
#define	AC_Unfl_flag     17
#define	AC_Round_Mode    30
#define	Round_Mode_even  0x0
#define	Round_Mode_trun  0x3

#define	s2_mant_x  r3
#define	s2_mant    r4
#define	s2_mant_lo s2_mant
#define	s2_mant_hi r5
#define	s2_exp     r6
#define s2_sign    r7

#define	tmp        r10
#define	tmp2       r11
#define	tmp3       r12
#define	con_1      r13
#define	con_2      r14

#define	ac         r15

#define	out        g0
#define	out_mlo    out
#define	out_mhi    g1
#define out_se     g2

#define	op_type    g4
#define x_op_type  g7


#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif


	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

AFP_RRC_T:
	mov	x_op_type,op_type		/* tranfer op_type in case of fault */
	cmpibge	0,s2_exp,rrc_t_40		/* J/ underflow */

#if	!defined(USE_CMP_BCC)
	cmpo	0,s2_mant_x
#endif

	ldconst	TP_INF,con_1

#if	defined(USE_CMP_BCC)
	cmpobe	0,s2_mant_x,rrc_t_30		/* J/ no rounding to do */
#else
	be	rrc_t_30			/* J/ no rounding to do */
#endif

	shro	AC_Round_Mode,ac,tmp		/* rounding mode to tmp */
	cmpobne	Round_Mode_even,tmp,rrc_t_20	/* J/ not round-to-nearest/even */
	chkbit	0,s2_mant_lo			/* "Q" bit (even) -> carry */
	ldconst	0x7fffffff,con_2
	addc	con_2,s2_mant_x,tmp		/* carry out is rounding bit */

rrc_t_02:					/* (rejoin for directed rounding) */
	addc	0,s2_mant_lo,out_mlo		/* round, move to out */
	addc	0,s2_mant_hi,out_mhi		/* keep s2_mant for underflow */
	addc	0,s2_exp,out_se			/* bump exponent as req'd */

	setbit	31,out_mhi,out_mhi		/* insure j bit set */

	cmpible	con_1,out_se,rrc_t_60b		/* J/ overflow */

	or	s2_sign,out_se,out_se		/* Result to out register set */

	ldconst	(1 << AC_Inex_mask)+(1 << AC_Inex_flag),con_1
	and	con_1,ac,tmp
	cmpobne	con_1,tmp,rrc_t_10		/* J/ inexact fault or set inex flag */
	ret

rrc_t_10:
	bbc	AC_Inex_mask,ac,rrc_t_12	/* J/ Inexact fault not masked */
#if	defined(USE_SIMULATED_AC)
	setbit	AC_Inex_flag,ac,ac		/* Set inexact flag */
	st	ac,fpem_CA_AC			/* Update memory image */
#else
	ldconst	1 << AC_Inex_flag,tmp		/* Inexact flag bit */
	modac	tmp,tmp,tmp			/* Set inexact flag */
#endif
	ret

rrc_t_12:
	b	_AFP_Fault_Inexact_T		/* Go to the inexact fault handler */


rrc_t_20:					/* Inexact result, not round/nearest */
	cmpobe	Round_Mode_trun,tmp,rrc_t_21	/* J/ truncate (tmp sign bit = 0) */

	shlo	16,s2_sign,tmp
	xor	ac,tmp,tmp			/* tmp sign bit = 1 if round "up" */
rrc_t_21:
	addc	tmp,tmp,tmp			/* tmp sign bit -> carry bit */
	b	rrc_t_02			/* round as req'd */


rrc_t_30:					/* Exact result (significand) */
	cmpibge	0,s2_exp,rrc_t_40		/* J/ underflow */
	cmpible	con_1,s2_exp,rrc_t_60a		/* J/ overflow */

	mov	s2_mant_lo,out_mlo		/* Pack result to out register */
	mov	s2_mant_hi,out_mhi
	or	s2_sign,s2_exp,out_se
        ret


rrc_t_40:					/* Exponent value underflow */
	bbc	AC_Unfl_mask,ac,rrc_t_56	/* J/ Underflow not masked */

	subo	s2_exp,1,tmp			/* shift right count */
	ldconst	32,tmp3

#if	!defined(USE_CMP_BCC)
	cmpo	31,tmp
#endif

	subo	tmp,tmp3,tmp2			/* shift left conjugate */

#if	defined(USE_CMP_BCC)
	cmpobl	31,tmp,rrc_t_42			/* J/ >= 32-bit shift */
#else
	bl	rrc_t_42			/* J/ >= 32-bit shift */
#endif

	cmpo	s2_mant_x,0			/* Retain _x bits */
	shlo	tmp2,s2_mant_lo,s2_mant_x	/* _lo -> _x bits */

#if	defined(USE_OPN_CC)
	addone	s2_mant_x,1,s2_mant_x
#else
	testne	tmp3
	or	s2_mant_x,tmp3,s2_mant_x	/* retain dropped bits */
#endif

	shlo	tmp2,s2_mant_hi,tmp3		/* _hi -> _lo bits */
	shro	tmp,s2_mant_hi,s2_mant_hi	/* _hi right shift */
	shro	tmp,s2_mant_lo,s2_mant_lo	/* _lo right shift */
	or	tmp3,s2_mant_lo,s2_mant_lo	/* combine _lo bits */
	b	rrc_t_46			/* Round denorm as req'd */

rrc_t_42:
	subo	tmp3,tmp,tmp			/* reduce shift by 32 bits */

#if	!defined(USE_CMP_BCC)
	cmpo	tmp,tmp3
#endif

	addo	tmp3,tmp2,tmp2			/* left shift adjustment */

#if	defined(USE_CMP_BCC)
	cmpobg	tmp,tmp3,rrc_t_43		/* J/ max'ed out shift */
#else
	bg	rrc_t_43			/* J/ max'ed out shift */
#endif

	cmpo	s2_mant_x,0
	shro	tmp,s2_mant_lo,s2_mant_x
	shlo	tmp2,s2_mant_lo,tmp3		/* dropped bits */
	shro	tmp,s2_mant_hi,s2_mant_lo
	shlo	tmp2,s2_mant_hi,s2_mant_hi	/* _hi -> _x bits */
	or	s2_mant_x,s2_mant_hi,s2_mant_x	/* Update _x */

#if	defined(USE_OPN_CC)
	selne	0,1,s2_mant_hi			/* dropped bits from _x */
#else
	testne	s2_mant_hi			/* dropped bits from _x */
#endif

	cmpo	tmp3,0
	or	s2_mant_x,s2_mant_hi,s2_mant_x

#if	defined(USE_OPN_CC)
	selne	0,1,tmp3			/* dropped bits from _lo */
#else
	testne	tmp3				/* dropped bits from _lo */
#endif

	or	s2_mant_x,tmp3,s2_mant_x
	ldconst	0,s2_mant_hi
	b	rrc_t_46			/* round denorm as req'd */

rrc_t_43:
	ldconst	0,s2_mant_hi			/* kludge for rounding */
	ldconst	0,s2_mant_lo
	ldconst	1,s2_mant_x

rrc_t_46:					/* Round after denorm shift */
	cmpobe	0,s2_mant_x,rrc_t_54		/* J/ exact: no rounding */
	shro	AC_Round_Mode,ac,tmp		/* rounding mode to tmp */
	cmpobne	Round_Mode_even,tmp,rrc_t_58	/* J/ not round-to-nearest/even */
	chkbit	0,s2_mant_lo			/* "Q" bit (even) -> carry */
	ldconst	0x7fffffff,con_2
	addc	con_2,s2_mant_x,tmp		/* carry out is rounding bit */

rrc_t_48:					/* (rejoin for directed rounding) */
	addc	0,s2_mant_lo,out_mlo
	addc	0,s2_mant_hi,out_mhi
	chkbit	31,out_mhi			/* rounded to norm value? */
	alterbit 0,s2_sign,out_se		/* exp = 0|1 for denorm|norm */

/* ***	bt	rrc_t_51  *** */		/* J/ not a denormal result */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_Unfl_flag,ac,ac		/* Set underflow flag */
	st	ac,fpem_CA_AC			/* Update memory image */
#else
	ldconst	1 << AC_Unfl_flag,tmp		/* Underflow flag bit */
	modac	tmp,tmp,ac			/* Set inexact flag */
#endif

rrc_t_51:
	ldconst	(1 << AC_Inex_mask)+(1 << AC_Inex_flag),con_1
	and	con_1,ac,tmp
	cmpobne	con_1,tmp,rrc_t_52		/* J/ inexact fault or set inex flag */
	ret

rrc_t_52:
	bbc	AC_Inex_mask,ac,rrc_t_12	/* J/ Inexact fault not masked */
#if	defined(USE_SIMULATED_AC)
	setbit	AC_Inex_flag,ac,ac		/* Set inexact flag */
	st	ac,fpem_CA_AC			/* Update memory image */
#else
	ldconst	1 << AC_Inex_flag,tmp		/* Inexact flag bit */
	modac	tmp,tmp,tmp			/* Set inexact flag */
#endif
	ret


rrc_t_54:					/* exact denorm result */
	mov	s2_mant_lo,out_mlo		/* xfer denorm significand */
	mov	s2_mant_hi,out_mhi
	mov	s2_sign,out_se			/* denorm exp = 0 */
	ret


rrc_t_56:
	ldconst	3*8192,tmp			/* Exponent adjustment */
	addo	tmp,out_se,out_se

	cmpibge.f 0,out_se,rrc_t_57		/* J/ massive overflow */

	or	s2_sign,out_se,out_se

	b	_AFP_Fault_Underflow_T		/* Go to the underflow fault handler */

rrc_t_57:
	mov	s2_sign,out_se			/* Massive underflow -> 0 */
	movlda(0,out_mhi)
	mov	0,out_mlo
	b	_AFP_Fault_Underflow_T		/* Go to the underflow fault handler */
	
rrc_t_58:					/* Inexact result, not round/nearest */
	cmpobe	Round_Mode_trun,tmp,rrc_t_59 	/* J/ truncate result */

	shlo	16,s2_sign,tmp			/* Result sign to tmp's MS bit */
	xor	ac,tmp,tmp			/* tmp sign bit = 1 if round "up" */
rrc_t_59:
	addc	tmp,tmp,tmp			/* tmp sign bit -> carry bit */
	b	rrc_t_48			/* round as req'd */


rrc_t_60a:					/* Exponent value overflow */
	movl	s2_mant,out_mlo
rrc_t_60b:
	bbc	AC_Ovfl_mask,ac,rrc_t_66	/* J/ overflow not masked */
	bbc	AC_Inex_mask,ac,rrc_t_64	/* J/ inexact fault on overflow */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_Ovfl_flag,ac,ac		/* Set overflow flag */
	setbit	AC_Inex_flag,ac,ac		/* Set inexact flag */
	st	ac,fpem_CA_AC			/* Update memory image */
#else
	ldconst	(1 << AC_Ovfl_flag)+(1 << AC_Inex_flag),tmp
	modac	tmp,tmp,tmp			/* Set inexact, overflow flag */
#endif
	
	ldconst	TP_INF,out_se			/* Default to +/-INF */
	or	s2_sign,out_se,out_se
	ldconst	0x80000000,out_mhi
	ldconst	0x00000000,out_mlo

	shro	AC_Round_Mode,ac,tmp
	cmpobe	Round_Mode_even,tmp,rrc_t_62
	cmpobe	Round_Mode_trun,tmp,rrc_t_61
	shlo	16,s2_sign,tmp
	xor	ac,tmp,tmp			/* XOR sign with MS round mode bit */
	bbs	31,tmp,rrc_t_62			/* J/ leave +/- INF */

rrc_t_61:
	subo	1,out_se,out_se			/* Max magnitude finite value */
	ldconst	0xFFFFFFFF,out_mhi
	ldconst	0xFFFFFFFF,out_mlo

rrc_t_62:
	ret

rrc_t_64:
	ldconst	0x80000000,tmp
	or	tmp,op_type,op_type		/* Use op type for inexact from ovfl */

	ldconst	3*8192,tmp			/* Exponent adjustment */
	subo	tmp,out_se,out_se

	cmpible.f con_1,out_se,rrc_t_65		/* J/ massive overflow */

	or	s2_sign,out_se,out_se
	b	_AFP_Fault_Inexact_T		/* Go to the inexact fault handler */

rrc_t_65:
	lda	TP_INF,out_se			/* Massive overflow -> INF */
	or	s2_sign,out_se,out_se
	movlda(0,out_mlo)
	shlo	31,1,out_mhi
	b	_AFP_Fault_Inexact_T		/* Go to the inexact fault handler */

rrc_t_66:
	ldconst	3*8192,tmp			/* Exponent adjustment */
	subo	tmp,s2_exp,out_se

#if	defined(USE_OPN_CC)
	cmpi	con_1,out_se			/* massive overflow?   */
	sell	con_1,out_se,out_se		/* limit exp to TF_INF */
#else
	cmpibg.f con_1,out_se,rrc_t_66b		/* J/ not a massive overflow */
	lda	TP_INF,out_se			/* Massive overflow -> INF */
rrc_t_66b:
#endif

	or	s2_sign,out_se,out_se
	b	_AFP_Fault_Overflow_T		/* Go to the overflow fault handler */

rrc_t_67:
	lda	TP_INF,out_se			/* Massive overflow -> INF */
	or	s2_sign,out_se,out_se
	movlda(0,out_mlo)
	shlo	31,1,out_mhi
	b	_AFP_Fault_Overflow_T		/* Go to the overflow fault handler */
