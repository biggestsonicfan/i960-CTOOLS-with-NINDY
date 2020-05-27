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
/*      afprrcd.c - Double Precision Round/Range Check/Pack Routine (AFP-960) */
/*									      */
/******************************************************************************/

#include "asmopt.h"

#if	!defined(KEEP_INTERNAL_LABELS)
#define	rrc_d_02	L_rrc_d_02
#define	rrc_d_10	L_rrc_d_10
#define	rrc_d_12	L_rrc_d_12
#define	rrc_d_20	L_rrc_d_20
#define	rrc_d_21	L_rrc_d_21
#define	rrc_d_30	L_rrc_d_30
#define	rrc_d_40	L_rrc_d_40
#define	rrc_d_42	L_rrc_d_42
#define	rrc_d_43	L_rrc_d_43
#define	rrc_d_46	L_rrc_d_46
#define	rrc_d_48	L_rrc_d_48
#define	rrc_d_50	L_rrc_d_50
#define	rrc_d_51	L_rrc_d_51
#define	rrc_d_52	L_rrc_d_52
#define	rrc_d_54	L_rrc_d_54
#define	rrc_d_56	L_rrc_d_56
#define	rrc_d_57	L_rrc_d_57
#define	rrc_d_58	L_rrc_d_58
#define	rrc_d_60a	L_rrc_d_60a
#define	rrc_d_60b	L_rrc_d_60b
#define	rrc_d_61	L_rrc_d_61
#define	rrc_d_62	L_rrc_d_62
#define	rrc_d_64	L_rrc_d_64
#define	rrc_d_65	L_rrc_d_65
#define	rrc_d_66	L_rrc_d_66
#define	rrc_d_67	L_rrc_d_67
#endif


	.file	"afprrcd.as"
	.globl	AFP_RRC_D
	.globl	AFP_RRC_D_2

	.globl	_AFP_Fault_Inexact_D
	.globl	_AFP_Fault_Overflow_D
	.globl	_AFP_Fault_Underflow_D


#define	DP_INF     0x7ff

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

#define	con_1      g5
#define	con_2      g6

#define	ac         r15

#define	out        g0
#define	out_lo     out
#define	out_hi     g1

#define	op_type    g2
#define	x_op_type  g4


#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

AFP_RRC_D:
	cmpi	s2_exp,2		/* possible range fault test	*/
	lda	DP_INF-2,con_1

	concmpi	s2_exp,con_1		/* possible range fault test	*/
	lda	0x7FFFFFFF,con_2	/* Round/even-nearest constant	*/
	bne.f	AFP_RRC_D_2		/* J/ possible range fault	*/

	cmpo	s2_mant_x,1		/* Check for Inexact fault/flag	*/
	lda	(1 << AC_Inex_mask)+(1 << AC_Inex_flag),con_1

	andnot	ac,con_1,tmp		/* Check for Inexact fault/flag	*/
	concmpo	1,tmp			/* (NZ if tmp = 0)		*/

	shro	AC_Round_Mode,ac,tmp	/* rounding mode to tmp		*/
	be.f	AFP_RRC_D_2		/* J/ inexact + fault/flag set	*/

#if	defined(USE_CMP_BCC)
	cmpobne	Round_Mode_even,tmp,AFP_RRC_D_2	/* J/ not round-to-nearest/even	*/
#else
	cmpo	Round_Mode_even,tmp	/* Round-to-nearest/even?	*/
	bne.f	AFP_RRC_D_2		/* J/ not round-to-nearest/even	*/
#endif

	subo	1,s2_exp,s2_exp		/* Compensate for "j" bit	*/
	chkbit	0,s2_mant_lo		/* Q0 bit into carry		*/
	shlo	20,s2_exp,tmp		/* Position exponent		*/
	addc	con_2,s2_mant_x,tmp2	/* Compute rounding		*/
	or	s2_sign,tmp,tmp		/* Mix exponent with sign	*/
	addc	0,s2_mant_lo,out_lo	/* Round lo word		*/
	addc	tmp,s2_mant_hi,out_hi	/* Round, carry to exp, sign	*/
	ret


AFP_RRC_D_2:
	mov	x_op_type,op_type		/* tranfer op_type in case of fault */
	cmpibge	0,s2_exp,rrc_d_40		/* J/ underflow */

#if	!defined(USE_CMP_BCC)
	cmpo	0,s2_mant_x
#endif

	ldconst	DP_INF,con_1

#if	defined(USE_CMP_BCC)
	cmpobe	0,s2_mant_x,rrc_d_30		/* J/ no rounding to do */
#else
	be	rrc_d_30			/* J/ no rounding to do */
#endif

	shro	AC_Round_Mode,ac,tmp		/* rounding mode to tmp */
	cmpobne	Round_Mode_even,tmp,rrc_d_20	/* J/ not round-to-nearest/even */
	chkbit	0,s2_mant_lo			/* "Q" bit (even) -> carry */
	addc	con_2,s2_mant_x,tmp		/* carry out is rounding bit */

rrc_d_02:						/* (rejoin for directed rounding) */
	addc	0,s2_mant_lo,out_lo		/* round, move to out */
	addc	0,s2_mant_hi,out_hi		/* keep s2_mant for underflow */

	shro	21,out_hi,tmp			/* tmp = 1 if rounding carry out */
	addo	tmp,s2_exp,s2_exp		/* bump exponent as req'd */
	cmpible	con_1,s2_exp,rrc_d_60b		/* J/ overflow */

	shlo	12,out_hi,out_hi		/* Pack result */
	or	s2_exp,out_hi,out_hi
	rotate	20,out_hi,out_hi
	or	s2_sign,out_hi,out_hi

	ldconst	(1 << AC_Inex_mask)+(1 << AC_Inex_flag),con_1
	and	con_1,ac,tmp
	cmpobne	con_1,tmp,rrc_d_10		/* J/ inexact fault or set inex flag */
	ret

rrc_d_10:
	bbc	AC_Inex_mask,ac,rrc_d_12	/* J/ Inexact fault not masked */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_Inex_flag,ac,ac		/* Set inexact flag */
	st	ac,fpem_CA_AC			/* Update memory image */
#else
	ldconst	1 << AC_Inex_flag,tmp		/* Inexact flag bit */
	modac	tmp,tmp,tmp			/* Set inexact flag */
#endif
	ret

rrc_d_12:
	b	_AFP_Fault_Inexact_D		/* Go to the inexact fault handler */


rrc_d_20:					/* Inexact result, not round/nearest */
	cmpobe	Round_Mode_trun,tmp,rrc_d_21 	/* J/ truncate (tmp sign bit = 0) */

	xor	ac,s2_sign,tmp			/* tmp sign bit = 1 if round "up" */
rrc_d_21:
	addc	tmp,tmp,tmp			/* tmp sign bit -> carry bit */
	b	rrc_d_02			/* round as req'd */


rrc_d_30:					/* Exact result (significand) */
	cmpibge	0,s2_exp,rrc_d_40		/* J/ underflow */
	cmpible	con_1,s2_exp,rrc_d_60a		/* J/ overflow */

	shlo	12,s2_mant_hi,out_hi		/* Pack result to out register */
	or	s2_exp,out_hi,out_hi
	rotate	20,out_hi,out_hi
	or	s2_sign,out_hi,out_hi
	mov	s2_mant_lo,out_lo
        ret


rrc_d_40:					/* Exponent value underflow */
	bbc	AC_Unfl_mask,ac,rrc_d_56	/* J/ Underflow not masked */

	subo	s2_exp,1,tmp			/* shift right count */
	ldconst	32,tmp3

#if	!defined(USE_CMP_BCC)
	cmpo	31,tmp
#endif

	subo	tmp,tmp3,tmp2			/* shift left conjugate */

#if	defined(USE_CMP_BCC)
	cmpobl	31,tmp,rrc_d_42			/* J/ >= 32-bit shift */
#else
	bl	rrc_d_42			/* J/ >= 32-bit shift */
#endif

	cmpo	s2_mant_x,0			/* Retain _x bits */
	shlo	tmp2,s2_mant_lo,s2_mant_x	/* _lo -> _x bits */

#if	defined(USE_OPN_CC)
	selne	0,1,tmp3
#else
	testne	tmp3
#endif

	or	s2_mant_x,tmp3,s2_mant_x	/* retain dropped bits */
	shlo	tmp2,s2_mant_hi,tmp3		/* _hi -> _lo bits */
	shro	tmp,s2_mant_hi,s2_mant_hi	/* _hi right shift */
	shro	tmp,s2_mant_lo,s2_mant_lo	/* _lo right shift */
	or	tmp3,s2_mant_lo,s2_mant_lo	/* combine _lo bits */
	b	rrc_d_46			/* Round denorm as req'd */

rrc_d_42:
	subo	tmp3,tmp,tmp			/* reduce shift by 32 bits */

#if	!defined(USE_CMP_BCC)
	cmpo	21,tmp
#endif

	addo	tmp3,tmp2,tmp2			/* left shift adjustment */

#if	defined(USE_CMP_BCC)
	cmpobge	21,tmp,rrc_d_43			/* J/ inrange shift */
#else
	bge	rrc_d_43			/* J/ inrange shift */
#endif

	ldconst	22,tmp				/* limit shift count */
	ldconst	32-22,tmp2
rrc_d_43:
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

rrc_d_46:					/* Round after denorm shift */
	cmpobe	0,s2_mant_x,rrc_d_54		/* J/ exact: no rounding */
	shro	AC_Round_Mode,ac,tmp		/* rounding mode to tmp */
	cmpobne	Round_Mode_even,tmp,rrc_d_58	/* J/ not round-to-nearest/even */
	chkbit	0,s2_mant_lo			/* "Q" bit (even) -> carry */
	addc	con_2,s2_mant_x,tmp		/* carry out is rounding bit */

rrc_d_48:					/* (rejoin for directed rounding) */
	addc	0,s2_mant_lo,s2_mant_lo
	addc	0,s2_mant_hi,s2_mant_hi

rrc_d_50:					/* (rejoin for truncate) */
	or	s2_sign,s2_mant_hi,out_hi	/* mix sign with denorm */
	mov	s2_mant_lo,out_lo

/* ***	bbs	20,out_hi,rrc_d_51  *** */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_Unfl_flag,ac,ac		/* Set underflow flag */
	st	ac,fpem_CA_AC			/* Update memory image */
#else
	ldconst	1 << AC_Unfl_flag,tmp		/* Underflow flag bit */
	modac	tmp,tmp,ac			/* Set inexact flag */
#endif

rrc_d_51:
	ldconst	(1 << AC_Inex_mask)+(1 << AC_Inex_flag),con_1
	and	con_1,ac,tmp
	cmpobne	con_1,tmp,rrc_d_52		/* J/ inexact fault or set inex flag */
	ret

rrc_d_52:
	bbc	AC_Inex_mask,ac,rrc_d_12	/* J/ Inexact fault not masked */
#if	defined(USE_SIMULATED_AC)
	setbit	AC_Inex_flag,ac,ac		/* Set inexact flag */
	st	ac,fpem_CA_AC			/* Update memory image */
#else
	ldconst	1 << AC_Inex_flag,tmp		/* Inexact flag bit */
	modac	tmp,tmp,tmp			/* Set inexact flag */
#endif
	ret


rrc_d_54:					/* exact denorm result */
	or	s2_sign,s2_mant_hi,out_hi	/* mix sign with denorm */
	mov	s2_mant_lo,out_lo
	ret


rrc_d_56:
	ldconst	1536,tmp			/* Exponent adjustment */
	addo	tmp,s2_exp,s2_exp

	cmpibge.f 0,s2_exp,rrc_d_57		/* J/ Massive undeflow	*/

	shlo	1+11,s2_mant_hi,out_hi		/* Pack the adjusted result */
	or	s2_exp,out_hi,out_hi
	rotate	20,out_hi,out_hi
	or	s2_sign,out_hi,out_hi
	mov	s2_mant_lo,out_lo

	b	_AFP_Fault_Underflow_D		/* Go to the underflow fault handler */

rrc_d_57:
	mov	s2_sign,out_hi			/* Massive underflow -> 0.0 */
	movlda(0,out_lo)
	b	_AFP_Fault_Underflow_D
	
rrc_d_58:					/* Inexact result, not round/nearest */
	cmpobe	Round_Mode_trun,tmp,rrc_d_50 	/* J/ truncate result */

	xor	ac,s2_sign,tmp			/* tmp sign bit = 1 if round "up" */
	addc	tmp,tmp,tmp			/* tmp sign bit -> carry bit */
	b	rrc_d_48			/* round as req'd */


rrc_d_60a:					/* Exponent value overflow */
	movl	s2_mant,out
rrc_d_60b:
	bbc	AC_Ovfl_mask,ac,rrc_d_66	/* J/ overflow not masked */
	bbc	AC_Inex_mask,ac,rrc_d_64	/* J/ inexact fault on overflow */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_Ovfl_flag,ac,ac		/* Set overflow flag */
	setbit	AC_Inex_flag,ac,ac		/* Set inexact flag */
	st	ac,fpem_CA_AC			/* Update memory image */
#else
	ldconst	(1 << AC_Ovfl_flag)+(1 << AC_Inex_flag),tmp
	modac	tmp,tmp,tmp			/* Set inexact, overflow flag */
#endif
	
	ldconst	DP_INF << 20,out_hi		/* Default to +/-INF */
	or	s2_sign,out_hi,out_hi
	ldconst	0,out_lo

	shro	AC_Round_Mode,ac,tmp
	cmpobe	Round_Mode_even,tmp,rrc_d_62
	cmpobe	Round_Mode_trun,tmp,rrc_d_61
	xor	s2_sign,ac,tmp			/* XOR sign with MS round mode bit */
	bbs	31,tmp,rrc_d_62			/* J/ leave +/- INF */

rrc_d_61:
	subo	1,out_hi,out_hi			/* Max magnitude finite value */
	subo	1,out_lo,out_lo

rrc_d_62:
	ret

rrc_d_64:
	ldconst	0x80000000,tmp
	or	tmp,op_type,op_type		/* Use op type for inexact from ovfl */

	ldconst	1536,tmp			/* Exponent adjustment */
	subo	tmp,s2_exp,s2_exp

	cmpible.f con_1,s2_exp,rrc_d_65		/* J/ massive overflow */

	shlo	12,out_hi,out_hi		/* Pack the adjusted result */
	or	s2_exp,out_hi,out_hi
	rotate	23,out_hi,out_hi
	or	s2_sign,out_hi,out_hi

	b	_AFP_Fault_Inexact_D		/* Go to the inexact fault handler */

rrc_d_65:
	lda	DP_INF << 20,out_hi		/* Massive overflow -> INF */
	or	s2_sign,out_hi,out_hi
	movlda(0,out_lo)
	b	_AFP_Fault_Inexact_D		/* Go to the inexact fault handler */

rrc_d_66:
	ldconst	1536,tmp			/* Exponent adjustment */
	subo	tmp,s2_exp,s2_exp

	cmpible.f con_1,s2_exp,rrc_d_67		/* J/ massive overflow */

	shlo	12,out_hi,out_hi		/* Pack the adjusted result */
	or	s2_exp,out_hi,out_hi
	rotate	23,out_hi,out_hi
	or	s2_sign,out_hi,out_hi

	b	_AFP_Fault_Overflow_D		/* Go to the overflow fault handler */

rrc_d_67:
	lda	DP_INF << 20,out_hi		/* Massive overflow -> INF */
	or	s2_sign,out_hi,out_hi
	movlda(0,out_lo)
	b	_AFP_Fault_Overflow_D		/* Go to the overflow fault handler */
