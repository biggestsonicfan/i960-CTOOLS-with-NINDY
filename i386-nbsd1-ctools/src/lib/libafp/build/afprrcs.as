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
/*      afprrcs.c - Single Precision Round/Range Check/Pack Routine (AFP-960) */
/*									      */
/******************************************************************************/

#include "asmopt.h"

#if	!defined(KEEP_INTERNAL_LABELS)
#define	rrc_s_00	L_rrc_s_00
#define	rrc_s_02	L_rrc_s_02
#define	rrc_s_10	L_rrc_s_10
#define	rrc_s_12	L_rrc_s_12
#define	rrc_s_20	L_rrc_s_20
#define	rrc_s_21	L_rrc_s_21
#define	rrc_s_30	L_rrc_s_30
#define	rrc_s_40	L_rrc_s_40
#define	rrc_s_43	L_rrc_s_43
#define	rrc_s_48	L_rrc_s_48
#define	rrc_s_50	L_rrc_s_50
#define	rrc_s_51	L_rrc_s_51
#define	rrc_s_52	L_rrc_s_52
#define	rrc_s_54	L_rrc_s_54
#define	rrc_s_56	L_rrc_s_56
#define	rrc_s_57	L_rrc_s_57
#define	rrc_s_58	L_rrc_s_58
#define	rrc_s_60a	L_rrc_s_60a
#define	rrc_s_60b	L_rrc_s_60b
#define	rrc_s_61	L_rrc_s_61
#define	rrc_s_62	L_rrc_s_62
#define	rrc_s_64	L_rrc_s_64
#define	rrc_s_65	L_rrc_s_65
#define	rrc_s_66	L_rrc_s_66
#define	rrc_s_67	L_rrc_s_67
#endif


	.file	"afprrcs.as"
	.globl	AFP_RRC_S
	.globl	AFP_RRC_S_2

	.globl	_AFP_Fault_Inexact_S
	.globl	_AFP_Fault_Overflow_S
	.globl	_AFP_Fault_Underflow_S


#define	FP_INF     0xFF

#define	AC_Inex_mask     28
#define	AC_Inex_flag     20
#define	AC_Ovfl_mask     24
#define	AC_Ovfl_flag     16
#define	AC_Unfl_mask     25
#define	AC_Unfl_flag     17
#define	AC_Round_Mode    30
#define	Round_Mode_even  0x0
#define	Round_Mode_trun  0x3

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
#define	op_type    g1
#define	x_op_type  g2


#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

AFP_RRC_S:
	cmpi	s2_exp,2			/* possible range fault test	*/
	lda	FP_INF-2,con_1

	concmpi	s2_exp,con_1			/* possible range fault test	*/
	lda	0x7FFFFFFF,con_2		/* Round/even-nearest constant	*/
	bne.f	rrc_s_00			/* J/ possible range fault	*/

	cmpo	s2_mant_x,1			/* Check for Inexact fault/flag	*/
	lda	(1 << AC_Inex_mask)+(1 << AC_Inex_flag),con_1

	andnot	ac,con_1,tmp			/* Check for Inexact fault/flag	*/
	concmpo	1,tmp				/* (NZ if tmp = 0)		*/

	shro	AC_Round_Mode,ac,tmp		/* rounding mode to tmp		*/
	be.f	rrc_s_00			/* J/ inexact + fault/flag set	*/

#if	defined(USE_CMP_BCC)
	cmpobne	Round_Mode_even,tmp,rrc_s_00	/* J/ not round-to-nearest/even	*/
#else
	cmpo	Round_Mode_even,tmp		/* Round-to-nearest/even?	*/
	bne.f	rrc_s_00			/* J/ not round-to-nearest/even	*/
#endif

	subo	1,s2_exp,s2_exp			/* Compensate for "j" bit	*/
	chkbit	0,s2_mant			/* Q0 bit into carry		*/
	shlo	23,s2_exp,tmp			/* Position exponent		*/
	addc	con_2,s2_mant_x,tmp2		/* Compute rounding		*/
	addo	s2_sign,tmp,tmp			/* Mix exponent with sign	*/
	addc	tmp,s2_mant,out			/* Combine for result		*/
	ret

rrc_s_00:
AFP_RRC_S_2:
	mov	x_op_type,op_type		/* Transfer op_type value */
	cmpibge	0,s2_exp,rrc_s_40		/* J/ denormal or full underflow */

#if	!defined(USE_CMP_BCC)
	cmpo	s2_mant_x,0
#endif

	ldconst	FP_INF,con_1

#if	defined(USE_CMP_BCC)
	cmpobe	0,s2_mant_x,rrc_s_30		/* J/ no rounding to do */
#else
	be	rrc_s_30			/* J/ no rounding to do */
#endif

	shro	AC_Round_Mode,ac,tmp		/* rounding mode to tmp */
	cmpobne	Round_Mode_even,tmp,rrc_s_20	/* J/ not round-to-nearest/even */
	chkbit	0,s2_mant			/* "Q" bit (even) -> carry */
	ldconst	0x7fffffff,con_2
	addc	s2_mant_x,con_2,tmp		/* carry out is rounding bit */

rrc_s_02:					/* (rejoin for directed rounding) */
	addc	0,s2_mant,out			/* round to out (s2_mant unchanged) */

	shro	24,out,tmp			/* tmp = 1 if rounding carry out */
	addo	tmp,s2_exp,s2_exp		/* bump exponent as req'd */

	cmpible	con_1,s2_exp,rrc_s_60b		/* J/ overflow */

	shlo	9,out,out			/* Pack result to out register */
	or	s2_exp,out,out
	rotate	23,out,out
	or	s2_sign,out,out

	ldconst	(1 << AC_Inex_mask)+(1 << AC_Inex_flag),con_1
	and	con_1,ac,tmp
	cmpobne	con_1,tmp,rrc_s_10		/* J/ inexact fault or set inex flag */
	ret

rrc_s_10:
	bbc	AC_Inex_mask,ac,rrc_s_12	/* J/ Inexact fault not masked */
#if	defined(USE_SIMULATED_AC)
	setbit	AC_Inex_flag,ac,ac		/* Set inexact flag */
	st	ac,fpem_CA_AC			/* Update memory image */
#else
	ldconst	1 << AC_Inex_flag,tmp		/* Inexact flag bit */
	modac	tmp,tmp,tmp			/* Set inexact flag */
#endif
	ret

rrc_s_12:
	b	_AFP_Fault_Inexact_S		/* Go to the inexact fault handler */


rrc_s_20:					/* Inexact result, not round/nearest */
	cmpobe	Round_Mode_trun,tmp,rrc_s_21	/* J/ truncate (tmp sign bit = 0) */

	xor	ac,s2_sign,tmp			/* tmp sign bit = 1 if round "up" */
rrc_s_21:
	addc	tmp,tmp,tmp			/* tmp sign bit -> carry bit */
	b	rrc_s_02			/* round as req'd */


rrc_s_30:					/* Exact result (significand) */
	cmpibge	0,s2_exp,rrc_s_40		/* J/ denormal or full underflow */
	cmpible	con_1,s2_exp,rrc_s_60a		/* J/ overflow */

	shlo	9,s2_mant,out			/* Pack result to out register */
	or	s2_exp,out,out
	rotate	23,out,out
	or	s2_sign,out,out
        ret


rrc_s_40:					/* Exponent value underflow */
	bbc	AC_Unfl_mask,ac,rrc_s_56	/* J/ Underflow not masked */

	subo	s2_exp,1,tmp
	cmpobge	24,tmp,rrc_s_43			/* J/ shift in range */
	ldconst	25,tmp				/* limit shift count */
rrc_s_43:
	subo	tmp,31,tmp2			/* conjugate left shift count */
	addo	1,tmp2,tmp2

	cmpo	s2_mant_x,0			/* old _x bit(s) to CC reg */
	shlo	tmp2,s2_mant,s2_mant_x		/* new _x bits from denorm */
	shro	tmp,s2_mant,s2_mant		/* denorm mant */

#if	defined(USE_OPN_CC)
	selne	0,1,tmp				/* old _x bits */
#else
	testne	tmp				/* old _x bits */
#endif

	or	s2_mant_x,tmp,s2_mant_x		/* old + new _x bits */

	cmpobe	0,s2_mant_x,rrc_s_54		/* J/ exact: no rounding */

	shro	AC_Round_Mode,ac,tmp		/* rounding mode to tmp */
	cmpobne	Round_Mode_even,tmp,rrc_s_58	/* J/ not round-to-nearest/even */
	chkbit	0,s2_mant			/* "Q" bit (even) -> carry */
	ldconst	0x7fffffff,con_2
	addc	con_2,s2_mant_x,tmp		/* carry out is rounding bit */

rrc_s_48:					/* (rejoin for directed rounding) */
	addc	0,s2_mant,s2_mant

rrc_s_50:					/* (rejoin for truncate) */
	or	s2_sign,s2_mant,out		/* pack denorm result */

/* ***	bbs	23,out,rrc_s_51  *** */		/* J/ rounded up to "E" */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_Unfl_flag,ac,ac		/* Set underflow flag */
	st	ac,fpem_CA_AC			/* Update memory image */
#else
	ldconst	1 << AC_Unfl_flag,tmp		/* Underflow flag bit */
	modac	tmp,tmp,ac			/* Set inexact flag */
#endif

rrc_s_51:
	ldconst	(1 << AC_Inex_mask)+(1 << AC_Inex_flag),con_1
	and	con_1,ac,tmp
	cmpobne	con_1,tmp,rrc_s_52		/* J/ inexact fault or set inex flag */
	ret

rrc_s_52:
	bbc	AC_Inex_mask,ac,rrc_s_12	/* J/ Inexact fault not masked */
#if	defined(USE_SIMULATED_AC)
	setbit	AC_Inex_flag,ac,ac		/* Set inexact flag */
	st	ac,fpem_CA_AC			/* Update memory image */
#else
	ldconst	1 << AC_Inex_flag,tmp		/* Inexact flag bit */
	modac	tmp,tmp,tmp			/* Set inexact flag */
#endif
	ret

rrc_s_54:					/* exact denorm result */
	or	s2_sign,s2_mant,out		/* pack denorm result */
	ret

rrc_s_56:
	ldconst	192,tmp				/* Exponent adjustment */
	addo	tmp,s2_exp,s2_exp

	cmpibge.f 0,s2_exp,rrc_s_57		/* J/ massive underflow */

	shlo	9,s2_mant,out			/* Pack the adjusted result */
	or	s2_exp,out,out
	rotate	23,out,out
	or	s2_sign,out,out

	b	_AFP_Fault_Underflow_S		/* Go to the underflow fault handler */

rrc_s_57:
	mov	s2_sign,out			/* Massive underflow -> 0 */
	b	_AFP_Fault_Underflow_S		/* Go to the underflow fault handler */


rrc_s_58:
	cmpobe	Round_Mode_trun,tmp,rrc_s_50	/* J/ truncate result */
	xor	ac,s2_sign,tmp			/* tmp sign bit = 1 if round "up" */
	addc	tmp,tmp,tmp			/* tmp sign bit -> carry bit */
	b	rrc_s_48			/* round as req'd */

	
rrc_s_60a:					/* Exponent value overflow */
	mov	s2_mant,out
rrc_s_60b:
	bbc	AC_Ovfl_mask,ac,rrc_s_66	/* J/ overflow not masked */
	bbc	AC_Inex_mask,ac,rrc_s_64	/* J/ inexact fault on overflow */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_Ovfl_flag,ac,ac		/* Set Overflow flag */
	setbit	AC_Inex_flag,ac,ac		/* Set inexact flag */
	st	ac,fpem_CA_AC			/* Update memory image */
#else
	ldconst	(1 << AC_Ovfl_flag)+(1 << AC_Inex_flag),tmp
	modac	tmp,tmp,tmp			/* Set inexact and overflow flag */
#endif
	
	ldconst	FP_INF << 23,out		/* Default to +/-INF */
	or	s2_sign,out,out

	shro	AC_Round_Mode,ac,tmp
	cmpobe	Round_Mode_even,tmp,rrc_s_62
	cmpobe	Round_Mode_trun,tmp,rrc_s_61
	xor	s2_sign,ac,tmp			/* XOR sign with MS round mode bit */
	bbs	31,tmp,rrc_s_62			/* J/ leave +/- INF */

rrc_s_61:
	subo	1,out,out			/* Max magnitude finite value */

rrc_s_62:
	ret

rrc_s_64:
	ldconst	0x80000000,tmp
	or	tmp,op_type,op_type		/* Use op type for inexact from ovfl */

	ldconst	192,tmp				/* Exponent adjustment */
	subo	tmp,s2_exp,s2_exp

	cmpible.f con_1,s2_exp,rrc_s_65		/* J/ Massive overflow */

	shlo	9,out,out			/* Pack the adjusted result */
	or	s2_exp,out,out
	rotate	23,out,out
	or	s2_sign,out,out

	b	_AFP_Fault_Inexact_S		/* Go to the inexact fault handler */

rrc_s_65:
	lda	FP_INF << 23,out		/* Massive overflow -> INF */
	or	s2_sign,out,out
	b	_AFP_Fault_Inexact_S		/* Go to the inexact fault handler */

rrc_s_66:
	ldconst	192,tmp				/* Exponent adjustment */
	subo	tmp,s2_exp,s2_exp

	cmpible.f con_1,s2_exp,rrc_s_67		/* J/ Massive overflow */

	shlo	9,out,out			/* Pack the adjusted result */
	or	s2_exp,out,out
	rotate	23,out,out
	or	s2_sign,out,out

	b	_AFP_Fault_Overflow_S		/* Go to the overflow fault handler */

rrc_s_67:
	lda	FP_INF << 23,out		/* Massive overflow -> INF */
	or	s2_sign,out,out
	b	_AFP_Fault_Overflow_S		/* Go to the overflow fault handler */
