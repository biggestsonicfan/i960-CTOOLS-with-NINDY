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
/*      rounddf2.c - Double Precision Round to Integral Value Routine	      */
/*		     (AFP-960)						      */
/*									      */
/******************************************************************************/


#include "asmopt.h"

#if	!defined(KEEP_INTERNAL_LABELS)
#define	rnd_d_10	L_rnd_d_10
#define	rnd_d_12	L_rnd_d_12
#define	rnd_d_14	L_rnd_d_14
#define	rnd_d_15	L_rnd_d_15
#define	rnd_d_16	L_rnd_d_16
#define	rnd_d_20	L_rnd_d_20
#define	rnd_d_22	L_rnd_d_22
#define	rnd_d_26	L_rnd_d_26
#define	rnd_d_40	L_rnd_d_40
#define	rnd_d_42	L_rnd_d_42
#define	rnd_d_44	L_rnd_d_44
#define	rnd_d_46	L_rnd_d_46
#define	rnd_d_48	L_rnd_d_48
#define	rnd_d_50	L_rnd_d_50
#define	rnd_d_60	L_rnd_d_60
#define	rnd_d_62	L_rnd_d_62
#endif


	.file	"rounddf2.as"
	.globl	___rounddf2,___rintdf2
	.globl	___ceildf2,___floordf2
	.globl	_ceil,_floor


#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	_AFP_Fault_Reserved_Encoding_D
	.globl	_AFP_Fault_Inexact_D
	.globl	_AFP_Fault_Invalid_Operation_D



#define	DP_Bias    0x3FF
#define	DP_INF     0x7FF

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
#define	s1_lo      s1
#define	s1_hi      g1

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

#define	op_type    g4
#define	rint_type  16
#define	round_type 18
#define	ceil_type  19
#define	floor_type 20



	.text
	.link_pix


	.align	MAJOR_CODE_ALIGNMENT

_ceil:
___ceildf2:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac			/* Fetch AC */
#else
	modac	0,0,ac				/* Get AC */
#endif
	mov	Round_Mode_up,roundmode		/* Force round to up */
	movlda(ceil_type,op_type)
	b	rnd_d_10


	.align	MINOR_CODE_ALIGNMENT

_floor:
___floordf2:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac			/* Fetch AC */
#else
	modac	0,0,ac				/* Get AC */
#endif
	mov	Round_Mode_down,roundmode	/* Force round to down */
	movlda(floor_type,op_type)
	b	rnd_d_10


	.align	MINOR_CODE_ALIGNMENT

___rintdf2:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac			/* Fetch AC */
#else
	modac	0,0,ac				/* Get AC */
#endif
	mov	Round_Mode_even,roundmode	/* Force round to nearest */
	movlda(rint_type,op_type)
	b	rnd_d_10


	.align	MINOR_CODE_ALIGNMENT

___rounddf2:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac			/* Fetch AC */
#else
	modac	0,0,ac				/* Get AC */
#endif
	shro	AC_Round_Mode,ac,roundmode	/* Use current rounding mode */
	movlda(round_type,op_type)


rnd_d_10:
	shlo	1,s1_hi,s1_exp			/* Drop sign bit */
	lda	DP_Bias,con_1

	shro	32-11,s1_exp,s1_exp
	lda	DP_INF,con_2

	cmpobl.f s1_exp,con_1,rnd_d_40		/* J/ arg magnitude < 1.0 */

#if	!defined(USE_CMP_BCC)
	cmpo	s1_exp,con_2
	subo	con_1,s1_exp,s1_exp		/* Compute unbiased exponent	*/
#endif

#if	defined(USE_CMP_BCC)
	cmpobe	s1_exp,con_2,rnd_d_60		/* J/ INF or NaN arg		*/
	subo	con_1,s1_exp,s1_exp		/* Compute unbiased exponent	*/
#else
	be.f	rnd_d_60			/* J/ INF or NaN arg		*/
#endif

	cmpoble.f 20,s1_exp,rnd_d_20		/* J/ "R" bit not hi word	*/

	cmpo	s1_lo,0				/* Check for bits in lo word	*/
	addlda(12,s1_exp,tmp)			/* left justif shift for R bit	*/

	subc	1,0,tmp2
	shlo	tmp,s1_hi,r_word		/* left justify R bit		*/

	subo	s1_exp,20,Qbitno		/* Q bit number			*/
	ornot	tmp2,r_word,r_word		/* Set r_word LS bit if lo <> 0	*/

	shlo	Qbitno,1,mask			/* Q bit for fraction masking	*/

#if	!defined(USE_CMP_BCC)
	cmpo	Round_Mode_even,roundmode
#endif

	subo	1,mask,tmp2
	movlda(0,s1_lo)				/* Result lo word = 0		*/

	andnot	tmp2,s1_hi,s1_hi		/* Strip fraction bits		*/

#if	defined(USE_CMP_BCC)
	cmpobne	Round_Mode_even,roundmode,rnd_d_16	/* J/ not round/even		*/
#else
	bne.f	rnd_d_16			/* J/ not round/even		*/
#endif

/* Since DP_Bias is odd, the following chkbit works even when Qbitno = 20 */

	chkbit	Qbitno,s1_hi			/* Q bit to carry		*/
	lda	0x7fffffff,tmp

rnd_d_12:
	addc	tmp,r_word,tmp			/* Round up bit in carry	*/

	alterbit Qbitno,0,tmp
	addo	s1_hi,tmp,s1_hi			/* Incr signif, carry to exp	*/

rnd_d_14:
#if	!defined(USE_CMP_BCC)
	cmpo	0,r_word			/* exact result?		*/
#endif

	lda	(1 << AC_Inex_mask)+(1 << AC_Inex_flag),mask

	and	ac,mask,tmp

#if	defined(USE_CMP_BCC)
	cmpobe	0,r_word,rnd_d_15		/* J/ exact result		*/
#else
	be.f	rnd_d_15			/* J/ exact result		*/
#endif

	cmpobne.f mask,tmp,rnd_d_50		/* J/ either fault or flag set	*/

rnd_d_15:
	ret


rnd_d_16:
	cmpobe.f Round_Mode_trun,roundmode,rnd_d_14	/* J/ trunc mode	*/

	shlo	AC_Round_Mode,roundmode,tmp
	xor	s1_hi,tmp,tmp			/* Sign bit set if round up dir	*/
	shri	31,tmp,tmp			/* tmp = f..f if round up dir	*/
	b	rnd_d_12			/* CMPOBE left carry clear	*/


rnd_d_20:
	subo	20,s1_exp,s1_exp
	cmpobl.f 31,s1_exp,rnd_d_15		/* J/ no fraction bits		*/

	shlo	s1_exp,s1_lo,r_word		/* left justify R bit		*/
	addldax(31,1,Qbitno)			/* Constant of 32		*/

#if	!defined(USE_CMP_BCC)
	cmpo	Round_Mode_even,roundmode
#endif

	subo	s1_exp,Qbitno,Qbitno		/* Q bit number			*/

	shlo	Qbitno,1,mask
	and	1,s1_hi,tmp			/* Kludge for Qbitno = 32	*/

	subo	1,mask,mask			/* fraction bit field mask	*/
	or	tmp,s1_lo,tmp			/* Mix in LS bit of hi word	*/

	andnot	mask,s1_lo,s1_lo		/* Strip fraction bits		*/

#if	defined(USE_CMP_BCC)
	cmpobne	Round_Mode_even,roundmode,rnd_d_26	/* J/ not round/even		*/
#else
	bne	rnd_d_26			/* J/ not round/even		*/
#endif

	chkbit	Qbitno,tmp			/* Q bit to carry		*/
	lda	0x7fffffff,tmp

rnd_d_22:
	addc	tmp,r_word,tmp			/* Round up bit into carry	*/
	addc	s1_lo,mask,s1_lo		/* (addc/andnot round up only	*/
	andnot	mask,s1_lo,s1_lo		/* when carry = 1)		*/
	addc	s1_hi,0,s1_hi
	b	rnd_d_14			/* Inexact result processing	*/


rnd_d_26:
	cmpobe.f Round_Mode_trun,roundmode,rnd_d_14	/* J/ trunc mode	*/

	shlo	AC_Round_Mode,roundmode,tmp
	xor	s1_hi,tmp,tmp			/* Produce FFFFFFF word if arg	*/
	shri	31,tmp,tmp			/* sign aligns with dirct rnd	*/
	b	rnd_d_22


rnd_d_40:
	cmpobne.t 0,s1_exp,rnd_d_42		/* J/ exp <> 0			*/

	clrbit	31,s1_hi,tmp			/* Drop sign bit		*/
	or	s1_lo,tmp,tmp
	cmpobe.t 0,tmp,rnd_d_15			/* J/ 0.0 value -> return same	*/

	chkbit	AC_Norm_Mode,ac
	bf.f	_AFP_Fault_Reserved_Encoding_D	/* J/ denormal fault	*/

rnd_d_42:
#if	!defined(USE_CMP_BCC)
	cmpo	Round_Mode_even,roundmode	/* round nearest/even?	*/
#endif

	movlda(1,r_word)			/* Force r_word to inexact rslt	*/

#if	defined(USE_CMP_BCC)
	cmpobne	Round_Mode_even,roundmode,rnd_d_48	/* J/ not round nearest/even	*/
#else
	bne.f	rnd_d_48			/* J/ not round nearest/even	*/
#endif

	cmpo	s1_lo,0				/* Condense lo word to one bit	*/
	lda	(DP_Bias-1)<<21,con_1		/* 0.5 magnitude threshold	*/

	subc	1,0,tmp
	addo	s1_hi,s1_hi,tmp2		/* Drop sign bit		*/

	ornot	tmp,tmp2,tmp2			/* Set LS bit if lo <> 0	*/

	cmpobl.t con_1,tmp2,rnd_d_46		/* J/ magnitude > 0.5		*/

rnd_d_44:
	setbit	31,0,mask			/* To isolate sign bit		*/
	movlda(0,s1_lo)				/* Lo word is zero		*/

	and	mask,s1_hi,s1_hi		/* Signed zero return		*/
	b	rnd_d_14			/* Inexact result processing	*/


rnd_d_46:					/* Return signed 1.0		*/
	chkbit	31,s1_hi			/* Copy arg sign bit ...	*/
	lda	DP_Bias << 20,s1_hi

	alterbit 31,s1_hi,s1_hi			/* ... to result sign bit	*/
	movlda(0,s1_lo)
	b	rnd_d_14			/* Inexact result processing	*/


rnd_d_48:
	cmpobe.f Round_Mode_trun,roundmode,rnd_d_44	/* Trunc mode -> +/-0.0	*/

	shlo	AC_Round_Mode,roundmode,tmp
	xor	s1_hi,tmp,tmp
	bbs	31,tmp,rnd_d_46			/* Directed rounding -> +/-1.0	*/
	b	rnd_d_44			/* Directed rounding -> +/-0.0	*/


rnd_d_50:
	chkbit	AC_Inex_mask,ac
	bf.f	_AFP_Fault_Inexact_D		/* J/ Fault on inexact */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_Inex_flag,ac,ac
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_Inex_flag,tmp
	modac	tmp,tmp,tmp
#endif
	ret


rnd_d_60:
	shlo	12,s1_hi,tmp
	or	s1_lo,tmp,tmp
	cmpobe.t 0,tmp,rnd_d_15			/* INF arg -> return same	*/

	bbc.f	19,s1_hi,rnd_d_62		/* J/ SNaN			*/

	setbit	31,s1_hi,s1_hi			/* Insure sign bit = 1		*/
	ret


rnd_d_62:
	chkbit	AC_InvO_mask,ac			/* Inv Opn fault masked?	*/
	bf.f	_AFP_Fault_Invalid_Operation_D	/* J/ fault on Inv Opn	*/

#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp
	modac	tmp,tmp,tmp
#endif

	setbit	19,s1_hi,s1_hi			/* Set QNaN bit			*/
	setbit	31,s1_hi,s1_hi			/* Insure sign bit = 1		*/

	ret
