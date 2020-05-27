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
/*      roundtf2.c - Extended Precision Round to Integral Value Routine	      */
/*		     (AFP-960)						      */
/*									      */
/******************************************************************************/


#include "asmopt.h"

#if	!defined(KEEP_INTERNAL_LABELS)
#define	rnd_t_10	L_rnd_t_10
#define	rnd_t_12	L_rnd_t_12
#define	rnd_t_14	L_rnd_t_14
#define	rnd_t_15	L_rnd_t_15
#define	rnd_t_16	L_rnd_t_16
#define	rnd_t_20	L_rnd_t_20
#define	rnd_t_22	L_rnd_t_22
#define	rnd_t_26	L_rnd_t_26
#define	rnd_t_40	L_rnd_t_40
#define	rnd_t_42	L_rnd_t_42
#define	rnd_t_44	L_rnd_t_44
#define	rnd_t_46	L_rnd_t_46
#define	rnd_t_48	L_rnd_t_48
#define	rnd_t_50	L_rnd_t_50
#define	rnd_t_60	L_rnd_t_60
#define	rnd_t_62	L_rnd_t_62
#define	rnd_t_98	L_rnd_t_98
#endif


	.file	"roundtf2.as"
	.globl	___roundtf2,___rinttf2
	.globl	___ceiltf2,___floortf2
	.globl	_ceill,_floorl


#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	_AFP_Fault_Reserved_Encoding_T
	.globl	_AFP_Fault_Inexact_T
	.globl	_AFP_Fault_Invalid_Operation_T



#define	TP_Bias    0x3FFF
#define	TP_INF     0x7FFF

#define	AC_Round_Mode    30
#define	Round_Mode_even  0x0
#define	Round_Mode_down  0x1
#define	Round_Mode_up    0x2
#define	Round_Mode_trun  0x3

#define	AC_Norm_Mode     29

#define	AC_Inex_mask     28
#define	AC_Inex_flag     20

#define	AC_InvO_mask     26
#define	AC_InvO_flag     18


#define	s1         g0
#define	s1_mlo     s1
#define	s1_mhi     g1
#define	s1_se      g2

#define	r_word     r3

#define	s1_exp     r6

#define	mask       r8
#define	Qbitno     r9
#define	roundmode  r10
#define	tmp        r11
#define	tmp2       r12
#define	con_1      r13
#define	con_2      r14

#define	ac         r15

#define	op_type    g7
#define	rint_type  16
#define	round_type 18
#define	ceil_type  19
#define	floor_type 20



	.text
	.link_pix


	.align	MAJOR_CODE_ALIGNMENT

_ceill:
___ceiltf2:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac			/* Fetch AC */
#else
	modac	0,0,ac				/* Get AC */
#endif
	mov	Round_Mode_up,roundmode		/* Force round to up */
	movlda(ceil_type,op_type)
	b	rnd_t_10


	.align	MINOR_CODE_ALIGNMENT

_floorl:
___floortf2:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac			/* Fetch AC */
#else
	modac	0,0,ac				/* Get AC */
#endif
	mov	Round_Mode_down,roundmode	/* Force round to down */
	movlda(floor_type,op_type)
	b	rnd_t_10


	.align	MINOR_CODE_ALIGNMENT

___rinttf2:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac			/* Fetch AC */
#else
	modac	0,0,ac				/* Get AC */
#endif
	mov	Round_Mode_even,roundmode	/* Force round to nearest */
	movlda(rint_type,op_type)
	b	rnd_t_10


	.align	MINOR_CODE_ALIGNMENT

___roundtf2:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac			/* Fetch AC */
#else
	modac	0,0,ac				/* Get AC */
#endif
	shro	AC_Round_Mode,ac,roundmode	/* Use current rounding mode */
	movlda(round_type,op_type)


rnd_t_10:
	shlo	16+1,s1_se,s1_exp		/* Drop sign bit */
	lda	TP_Bias,con_1

	shro	16+1,s1_exp,s1_exp
	lda	TP_INF,con_2

	cmpobl.f s1_exp,con_1,rnd_t_40		/* J/ arg magnitude < 1.0 */

	bbc.f	31,s1_mhi,rnd_t_98		/* J/ unnormal -> reserved encd	*/

#if	!defined(USE_CMP_BCC)
	cmpo	s1_exp,con_2
	subo	con_1,s1_exp,s1_exp		/* Compute unbiased exponent	*/
#endif

#if	defined(USE_CMP_BCC)
	cmpobe	s1_exp,con_2,rnd_t_60		/* J/ INF or NaN arg		*/
	subo	con_1,s1_exp,s1_exp		/* Compute unbiased exponent	*/
#else
	be.f	rnd_t_60			/* J/ INF or NaN arg		*/
#endif

	cmpoble.f 31,s1_exp,rnd_t_20		/* J/ "R" bit not hi word	*/

	cmpo	s1_mlo,0			/* Check for bits in lo word	*/
	addlda(1,s1_exp,tmp)			/* left justif shift for R bit	*/

	subc	1,0,tmp2
	shlo	tmp,s1_mhi,r_word		/* left justify R bit		*/

	subo	s1_exp,31,Qbitno		/* Q bit number			*/
	ornot	tmp2,r_word,r_word		/* Set r_word LS bit if lo <> 0	*/

	shlo	Qbitno,1,mask			/* Q bit for fraction masking	*/

#if	!defined(USE_CMP_BCC)
	cmpo	Round_Mode_even,roundmode
#endif

	subo	1,mask,tmp2
	movlda(0,s1_mlo)			/* Result lo word = 0		*/

	andnot	tmp2,s1_mhi,s1_mhi		/* Strip fraction bits		*/

#if	defined(USE_CMP_BCC)
	cmpobne	Round_Mode_even,roundmode,rnd_t_16	/* J/ not round/even		*/
#else
	bne.f	rnd_t_16			/* J/ not round/even		*/
#endif

	chkbit	Qbitno,s1_mhi			/* Q bit to carry		*/
	lda	0x7fffffff,tmp

rnd_t_12:
	addc	tmp,r_word,tmp			/* Round up bit in carry	*/

	alterbit Qbitno,0,tmp
	cmpo	0,1				/* clear carry			*/

	addc	s1_mhi,tmp,s1_mhi		/* Incr signif, carry to exp	*/

	addc	0,s1_se,s1_se			/* Carry out to exponent	*/
	setbit	31,s1_mhi,s1_mhi		/* Insure "j" bit set		*/

rnd_t_14:
#if	!defined(USE_CMP_BCC)
	cmpo	0,r_word			/* exact result?		*/
#endif

	lda	(1 << AC_Inex_mask)+(1 << AC_Inex_flag),mask

	and	ac,mask,tmp

#if	defined(USE_CMP_BCC)
	cmpobe	0,r_word			/* exact result?		*/,rnd_t_15			/* J/ exact result		*/
#else
	be.f	rnd_t_15			/* J/ exact result		*/
#endif

	cmpobne.f mask,tmp,rnd_t_50		/* J/ either fault or flag set	*/

rnd_t_15:
	ret


rnd_t_16:
	cmpobe.f Round_Mode_trun,roundmode,rnd_t_14	/* J/ trunc mode	*/

	shlo	16,s1_se,tmp2			/* Sign bit to MS bit in reg	*/
	shlo	AC_Round_Mode,roundmode,tmp
	xor	tmp2,tmp,tmp			/* Sign bit set if round up dir	*/
	shri	31,tmp,tmp			/* tmp = f..f if round up dir	*/
	b	rnd_t_12			/* CMPOBE left carry clear	*/


rnd_t_20:
	subo	31,s1_exp,s1_exp
	cmpobl.f 31,s1_exp,rnd_t_15		/* J/ no fraction bits		*/

	shlo	s1_exp,s1_mlo,r_word		/* left justify R bit		*/
	addldax(31,1,Qbitno)			/* Constant of 32		*/

#if	!defined(USE_CMP_BCC)
	cmpo	Round_Mode_even,roundmode
#endif

	subo	s1_exp,Qbitno,Qbitno		/* Q bit number			*/

	shlo	Qbitno,1,mask
	and	1,s1_mhi,tmp			/* Kludge for Qbitno = 32	*/

	subo	1,mask,mask			/* fraction bit field mask	*/
	or	tmp,s1_mlo,tmp			/* Mix in LS bit of hi word	*/

	andnot	mask,s1_mlo,s1_mlo		/* Strip fraction bits		*/

#if	defined(USE_CMP_BCC)
	cmpobne	Round_Mode_even,roundmode,rnd_t_26	/* J/ not round/even		*/
#else
	bne	rnd_t_26			/* J/ not round/even		*/
#endif

	chkbit	Qbitno,tmp			/* Q bit to carry		*/
	lda	0x7fffffff,tmp

rnd_t_22:
	addc	tmp,r_word,tmp			/* Round up bit into carry	*/

	addc	s1_mlo,mask,s1_mlo		/* (addc/andnot round up only	*/

	andnot	mask,s1_mlo,s1_mlo		/* when carry = 1)		*/
	addc	s1_mhi,0,s1_mhi

	setbit	31,s1_mhi,s1_mhi		/* Insure "j" bit set		*/
	addc	s1_se,0,s1_se
	b	rnd_t_14			/* Inexact result processing	*/


rnd_t_26:
	cmpobe.f Round_Mode_trun,roundmode,rnd_t_14	/* J/ trunc mode	*/

	shlo	16,s1_se,tmp2			/* Sign bit to MS bit in reg	*/
	shlo	AC_Round_Mode,roundmode,tmp
	xor	tmp2,tmp,tmp			/* Produce FFFFFFF word if arg	*/
	shri	31,tmp,tmp			/* sign aligns with dirct rnd	*/
	b	rnd_t_22


rnd_t_40:
	cmpobne.t 0,s1_exp,rnd_t_42		/* J/ exp <> 0			*/

	or	s1_mlo,s1_mhi,tmp
	cmpobe.t 0,tmp,rnd_t_15			/* J/ 0.0 value -> return same	*/

	bbc.f	AC_Norm_Mode,ac,rnd_t_98	/* J/ resvd encd fault (denorm)	*/

rnd_t_42:
#if	!defined(USE_CMP_BCC)
	cmpo	Round_Mode_even,roundmode	/* round nearest/even?	*/
#endif

	movlda(1,r_word)			/* Force r_word to inexact rslt	*/

#if	defined(USE_CMP_BCC)
	cmpobne	Round_Mode_even,roundmode,rnd_t_48	/* J/ not round nearest/even	*/
#else
	bne.f	rnd_t_48			/* J/ not round nearest/even	*/
#endif

	clrbit	31,s1_mhi,tmp			/* Drop "j" bit			*/
	or	s1_mlo,tmp,tmp			/* Accum any non-j frac bits	*/
	cmpo	0,tmp				/* Signif = 1.0?		*/
	lda	(TP_Bias-1)<<1,con_1		/* 0.5 magnitude threshold	*/
	addc	s1_exp,s1_exp,tmp		/* Combine exponent with signif	*/
	xor	1,tmp,tmp			/* Correct polarity of LS bit	*/
	cmpobl.t con_1,tmp,rnd_t_46		/* J/ magnitude > 0.5		*/

rnd_t_44:
	setbit	15,0,mask			/* To isolate sign bit		*/
	movl	0,s1_mlo			/* Significand is zero		*/

	and	mask,s1_se,s1_se		/* Signed zero return		*/
	b	rnd_t_14			/* Inexact result processing	*/


rnd_t_46:					/* Return signed 1.0		*/
	chkbit	15,s1_se			/* Copy arg sign bit ...	*/
	lda	TP_Bias,s1_se

	alterbit 15,s1_se,s1_se			/* ... to result sign bit	*/
	movlda(0,s1_mlo)

	setbit	31,0,s1_mhi
	b	rnd_t_14			/* Inexact result processing	*/


rnd_t_48:
	cmpobe.f Round_Mode_trun,roundmode,rnd_t_44	/* Trunc mode -> +/-0.0	*/

	shlo	16,s1_se,tmp2			/* Sign bit to MS bit of reg	*/
	shlo	AC_Round_Mode,roundmode,tmp
	xor	tmp2,tmp,tmp
	bbs	31,tmp,rnd_t_46			/* Directed rounding -> +/-1.0	*/
	b	rnd_t_44			/* Directed rounding -> +/-0.0	*/


rnd_t_50:
	chkbit	AC_Inex_mask,ac
	bf.f	_AFP_Fault_Inexact_T		/* J/ Fault on inexact */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_Inex_flag,ac,ac
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_Inex_flag,tmp
	modac	tmp,tmp,tmp
#endif
	ret


rnd_t_60:
	shlo	1,s1_mhi,tmp
	or	s1_mlo,tmp,tmp
	cmpobe.t 0,tmp,rnd_t_15			/* INF arg -> return same	*/

	bbc.f	30,s1_mhi,rnd_t_62		/* J/ SNaN			*/

	setbit	15,s1_se,s1_se			/* Insure sign bit = 1		*/
	ret


rnd_t_62:
	chkbit	AC_InvO_mask,ac			/* Inv Opn fault masked?	*/
	bf.f	_AFP_Fault_Invalid_Operation_T	/* J/ fault on Inv Opn	*/

#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp
	modac	tmp,tmp,tmp
#endif

	setbit	30,s1_mhi,s1_mhi		/* Set QNaN bit			*/
	setbit	15,s1_se,s1_se			/* Insure sign bit = 1		*/

	ret


rnd_t_98:
	b	_AFP_Fault_Reserved_Encoding_T	/* J/ denormal fault	*/
