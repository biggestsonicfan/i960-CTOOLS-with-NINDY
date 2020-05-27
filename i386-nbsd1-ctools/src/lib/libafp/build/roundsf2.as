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
/*      roundsf2.c - Single Precision Round to Integral Value Routine	      */
/*		     (AFP-960)						      */
/*									      */
/******************************************************************************/


#include "asmopt.h"

#if	!defined(KEEP_INTERNAL_LABELS)
#define	rnd_s_10	L_rnd_s_10
#define	rnd_s_12	L_rnd_s_12
#define	rnd_s_14	L_rnd_s_14
#define	rnd_s_15	L_rnd_s_15
#define	rnd_s_16	L_rnd_s_16
#define	rnd_s_40	L_rnd_s_40
#define	rnd_s_42	L_rnd_s_42
#define	rnd_s_44	L_rnd_s_44
#define	rnd_s_46	L_rnd_s_46
#define	rnd_s_48	L_rnd_s_48
#define	rnd_s_50	L_rnd_s_50
#define	rnd_s_60	L_rnd_s_60
#define	rnd_s_62	L_rnd_s_62
#endif


	.file	"roundsf2.as"
	.globl	___roundsf2,___rintsf2
	.globl	___ceilsf2,___floorsf2
	.globl	_ceilf,_floorf


#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	_AFP_Fault_Reserved_Encoding_S
	.globl	_AFP_Fault_Inexact_S
	.globl	_AFP_Fault_Invalid_Operation_S


#define	FP_Bias    0x7F
#define	FP_INF     0xFF

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

#define	op_type    g2
#define	rint_type  16
#define	round_type 18
#define	ceil_type  19
#define	floor_type 20


	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

_ceilf:
___ceilsf2:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac			/* Fetch AC */
#else
	modac	0,0,ac				/* Get AC */
#endif
	mov	Round_Mode_up,roundmode		/* Force round to up */
	movlda(ceil_type,op_type)
	b	rnd_s_10


	.align	MINOR_CODE_ALIGNMENT

_floorf:
___floorsf2:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac			/* Fetch AC */
#else
	modac	0,0,ac				/* Get AC */
#endif
	mov	Round_Mode_down,roundmode	/* Force round to down */
	movlda(floor_type,op_type)
	b	rnd_s_10


	.align	MINOR_CODE_ALIGNMENT

___rintsf2:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac			/* Fetch AC */
#else
	modac	0,0,ac				/* Get AC */
#endif
	mov	Round_Mode_even,roundmode	/* Force round to nearest */
	movlda(rint_type,op_type)
	b	rnd_s_10


	.align	MINOR_CODE_ALIGNMENT

___roundsf2:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac			/* Fetch AC */
#else
	modac	0,0,ac				/* Get AC */
#endif
	shro	AC_Round_Mode,ac,roundmode	/* Use current rounding mode */
	movlda(round_type,op_type)


rnd_s_10:
	shlo	1,s1,s1_exp			/* Drop sign bit */
	lda	FP_Bias,con_1

	shro	32-8,s1_exp,s1_exp
	lda	FP_INF,con_2

	cmpobl.f s1_exp,con_1,rnd_s_40		/* J/ arg magnitude < 1.0 */

#if	!defined(USE_CMP_BCC)
	cmpo	s1_exp,con_2
	subo	con_1,s1_exp,s1_exp		/* Compute unbiased exponent	*/
#endif

#if	defined(USE_CMP_BCC)
	cmpobe	s1_exp,con_2,rnd_s_60		/* J/ INF or NaN arg		*/
	subo	con_1,s1_exp,s1_exp		/* Compute unbiased exponent	*/
#else
	be.f	rnd_s_60			/* J/ INF or NaN arg		*/
#endif

#if	!defined(USE_CMP_BCC)
	cmpo	23,s1_exp			/* any fraction bits?		*/
#endif

	addlda(9,s1_exp,tmp)			/* left justif shift for R bit	*/

#if	defined(USE_CMP_BCC)
	cmpoble	23,s1_exp,rnd_s_15		/* J/ no fraction bits		*/
#else
	ble.f	rnd_s_15			/* J/ no fraction bits		*/
#endif

	shlo	tmp,s1,r_word			/* left justify R bit		*/

	subo	s1_exp,23,Qbitno		/* Q bit number			*/

#if	!defined(USE_CMP_BCC)
	cmpo	Round_Mode_even,roundmode
#endif

	shro	tmp,r_word,mask			/* Q bit for fraction masking	*/

	xor	mask,s1,s1			/* Strip fraction bits		*/

#if	defined(USE_CMP_BCC)
	cmpobne	Round_Mode_even,roundmode,rnd_s_16	/* J/ not round/even		*/
#else
	bne.f	rnd_s_16			/* J/ not round/even		*/
#endif

/* Since FP_Bias is odd, the following chkbit works even when Qbitno = 20 */

	chkbit	Qbitno,s1			/* Q bit to carry		*/
	lda	0x7fffffff,tmp

rnd_s_12:
	addc	tmp,r_word,tmp			/* Round up bit in carry	*/

	alterbit Qbitno,0,tmp
	addo	s1,tmp,s1			/* Incr signif, carry to exp	*/

rnd_s_14:
#if	!defined(USE_CMP_BCC)
	cmpo	0,r_word			/* exact result?		*/
#endif

	lda	(1 << AC_Inex_mask)+(1 << AC_Inex_flag),mask

	and	ac,mask,tmp

#if	defined(USE_CMP_BCC)
	cmpobe	0,r_word,rnd_s_15		/* J/ exact result		*/
#else
	be.f	rnd_s_15			/* J/ exact result		*/
#endif

	cmpobne.f mask,tmp,rnd_s_50		/* J/ either fault or flag set	*/

rnd_s_15:
	ret


rnd_s_16:
	cmpobe.f Round_Mode_trun,roundmode,rnd_s_14	/* J/ trunc mode	*/

	shlo	AC_Round_Mode,roundmode,tmp
	xor	s1,tmp,tmp			/* Sign bit set if round up dir	*/
	shri	31,tmp,tmp			/* tmp = f..f if round up dir	*/
	b	rnd_s_12			/* CMPOBE left carry clear	*/


rnd_s_40:
	cmpobne.t 0,s1_exp,rnd_s_42		/* J/ exp <> 0			*/

	clrbit	31,s1,tmp			/* Drop sign bit		*/
	cmpobe.t 0,tmp,rnd_s_15			/* J/ 0.0 value -> return same	*/

	chkbit	AC_Norm_Mode,ac
	bf.f	_AFP_Fault_Reserved_Encoding_S	/* J/ denormal fault	*/

rnd_s_42:
#if	!defined(USE_CMP_BCC)
	cmpo	Round_Mode_even,roundmode	/* round nearest/even?	*/
#endif

	movlda(1,r_word)			/* Force r_word to inexact rslt	*/

#if	defined(USE_CMP_BCC)
	cmpobne	Round_Mode_even,roundmode,rnd_s_48	/* J/ not round nearest/even	*/
#else
	bne.f	rnd_s_48			/* J/ not round nearest/even	*/
#endif

	addo	s1,s1,tmp2			/* Drop sign bit		*/
	lda	(FP_Bias-1)<<24,con_1		/* 0.5 magnitude threshold	*/

	cmpobl.t con_1,tmp2,rnd_s_46		/* J/ magnitude > 0.5		*/

rnd_s_44:
	setbit	31,0,mask			/* To isolate sign bit		*/
	and	s1,mask,s1			/* Signed zero return		*/
	b	rnd_s_14			/* Inexact result processing	*/


rnd_s_46:					/* Return signed 1.0		*/
	chkbit	31,s1				/* Copy arg sign bit ...	*/
	lda	FP_Bias << 23,s1

	alterbit 31,s1,s1			/* ... to result sign bit	*/
	b	rnd_s_14			/* Inexact result processing	*/


rnd_s_48:
	cmpobe.f Round_Mode_trun,roundmode,rnd_s_44	/* Trunc mode -> +/-0.0	*/

	shlo	AC_Round_Mode,roundmode,tmp
	xor	s1,tmp,tmp
	bbs	31,tmp,rnd_s_46			/* Directed rounding -> +/-1.0	*/
	b	rnd_s_44			/* Directed rounding -> +/-0.0	*/


rnd_s_50:
	chkbit	AC_Inex_mask,ac
	bf.f	_AFP_Fault_Inexact_S		/* J/ Fault on inexact */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_Inex_flag,ac,ac
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_Inex_flag,tmp
	modac	tmp,tmp,tmp
#endif
	ret


rnd_s_60:
	shlo	9,s1,tmp
	cmpobe.t 0,tmp,rnd_s_15			/* INF arg -> return same	*/

	bbc.f	22,s1,rnd_s_62			/* J/ SNaN			*/

	setbit	31,s1,s1			/* Insure sign bit = 1		*/
	ret


rnd_s_62:
	chkbit	AC_InvO_mask,ac			/* Inv Opn fault masked?	*/
	bf.f	_AFP_Fault_Invalid_Operation_S	/* J/ fault on Inv Opn	*/

#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp
	modac	tmp,tmp,tmp
#endif

	setbit	22,s1,s1			/* Set QNaN bit			*/
	setbit	31,s1,s1			/* Insure sign bit = 1		*/

	ret
