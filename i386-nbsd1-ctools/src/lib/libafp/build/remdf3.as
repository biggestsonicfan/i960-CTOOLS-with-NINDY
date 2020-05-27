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
/*      remdf3.c - Double Precision Remainder Routine (AFP-960)		      */
/*									      */
/******************************************************************************/

#include "asmopt.h"

#if	!defined(KEEP_INTERNAL_LABELS)
#define	rem_d_00		L_rem_d_00
#define	rem_d_s1_rejoin		L_rem_d_s1_rejoin
#define	rem_d_s2_rejoin		L_rem_d_s2_rejoin
#define	rem_d_10		L_rem_d_10
#define	rem_d_12		L_rem_d_12
#define	rem_d_20		L_rem_d_20
#define	rem_d_22		L_rem_d_22
#define	rem_d_30		L_rem_d_30
#define	rem_d_30a		L_rem_d_30a
#define	rem_d_30c		L_rem_d_30c
#define	rem_d_31		L_rem_d_31
#define	rem_d_32		L_rem_d_32
#define	rem_d_33		L_rem_d_33
#define	rem_d_34		L_rem_d_34
#define	rem_d_35		L_rem_d_35
#define	rem_d_36		L_rem_d_36
#define	rem_d_37		L_rem_d_37
#define	rem_d_40		L_rem_d_40
#define	rem_d_45		L_rem_d_45
#define	rem_d_47		L_rem_d_47
#define	rem_d_s1_special	L_rem_d_s1_special
#define	rem_d_s1_02		L_rem_d_s1_02
#define	rem_d_s1_04		L_rem_d_s1_04
#define	rem_d_s1_06		L_rem_d_s1_06
#define	rem_d_s1_10		L_rem_d_s1_10
#define	rem_d_s1_12		L_rem_d_s1_12
#define	rem_d_s1_14		L_rem_d_s1_14
#define	rem_d_s1_20		L_rem_d_s1_20
#define	rem_d_s1_21		L_rem_d_s1_21
#define	rem_d_s1_22		L_rem_d_s1_22
#define	rem_d_s2_special	L_rem_d_s2_special
#define	rem_d_s2_02		L_rem_d_s2_02
#define	rem_d_s2_04		L_rem_d_s2_04
#endif


	.file	"remdf3.as"
	.globl	___remdf3
	.globl	___rmddf3

#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	AFP_NaN_D

	.globl	_AFP_Fault_Invalid_Operation_D
	.globl	_AFP_Fault_Reserved_Encoding_D


#define	DP_INF          0x7ff

#define	AC_Norm_Mode    29

#define	AC_InvO_mask    26
#define	AC_InvO_flag    18

#define	Standard_QNaN_hi  0xfff80000
#define	Standard_QNaN_lo  0x00000000


/* Register Name Equates */

#define	s1         g0
#define	s1_lo      s1
#define	s1_hi      g1
#define s2         g2
#define s2_lo      s2
#define s2_hi      g3

#define	s2_mant    r4
#define s2_mant_lo s2_mant
#define s2_mant_hi r5
#define	s2_exp     r6

#define s1_mant    r8
#define s1_mant_lo s1_mant
#define s1_mant_hi r9
#define	s1_exp     r10
#define	s1_sign    r11

#define	tmp        r12
#define	tmp2       r13
#define	tmp3       r14
#define	ac         r15

#define	loop_cntl  r7

#define	con_1      r3

#define	quo        g5

#define	out        g0
#define	out_lo     out
#define	out_hi     g1

#define	out2       g2

#define	op_type    g4
#define	rem_type   15
#define	rmd_type   17


	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

___rmddf3:
	ldconst	rmd_type,op_type		/* for fault handler operation info */
	b	rem_d_00

___remdf3:
	ldconst	rem_type,op_type		/* for fault handler operation info */

rem_d_00:
	shlo	1,s1_hi,s1_exp			/* exp + mant (no sign bit)	*/
	onebit(31,s1_sign)			/* mask for sign bit		*/

	shri	32-11,s1_exp,tmp		/* check S1 for special case	*/
	lda	0xFFF00000,tmp2

	and	s1_hi,s1_sign,s1_sign		/* select S2 sign bit		*/
	addlda(1,tmp,tmp)

#if	!defined(USE_CMP_BCC)
	cmpo	2,tmp				/* S1 NaN/INF, 0/Denormal?	*/
#endif

	movldar(s1_lo,s1_mant_lo)		/* copy mantissa bits		*/

	shlo	1,s2_hi,s2_exp			/* exp + mant (no sign bit)	*/
	andnot	tmp2,s1_hi,s1_mant_hi

	subo	tmp2,s1_mant_hi,s1_mant_hi	/* set "j" bit		*/
	shro	32-11,s1_exp,s1_exp		/* right justify exponent	*/

#if	defined(USE_CMP_BCC)
	cmpobg.f	2,tmp,rem_d_s1_special	/* J/ special S1 handling	*/
#else
	bg.f	rem_d_s1_special		/* J/ special S1 handling	*/
#endif

rem_d_s1_rejoin:

	shri	32-11,s2_exp,tmp		/* Test S2 for special ...	*/
	movldar(s2_lo,s2_mant_lo)		/* ... but assume it's not.	*/

	andnot	tmp2,s2_hi,s2_mant_hi
	addlda(1,tmp,tmp)

#if	!defined(USE_CMP_BCC)
	cmpo	2,tmp
#endif

	shro	32-11,s2_exp,s2_exp		/* right justify exponent	*/

	setbit	20,s2_mant_hi,s2_mant_hi	/* set "j" bit			*/

#if	defined(USE_CMP_BCC)
	cmpobg.f	2,tmp,rem_d_s2_special	/* J/ NaN/INF, 0/denormal	*/
#else
	bg.f	rem_d_s2_special		/* J/ NaN/INF, 0/denormal	*/
#endif

rem_d_s2_rejoin:

	subo	1,s2_exp,s2_exp			/* Force an extra loop		*/

#if	!defined(USE_CMP_BCC)
	cmpi	s1_exp,s2_exp
#endif

	subo	s2_exp,s1_exp,loop_cntl		/* Number of loops		*/

#if	defined(USE_CMP_BCC)
	cmpibl	s1_exp,s2_exp,rem_d_40		/* J/ no remaindering loops	*/
#else
	bl.f	rem_d_40			/* J/ no remaindering loops	*/
#endif


/*  Non-restoring division alg to get REMRL result  */

	mov	0,quo				/* Quotient register		*/
rem_d_10:
	cmpo	0,0				/* Set Not Borrow bit		*/
	subc	s2_mant_lo,s1_mant_lo,s1_mant_lo
	subc	s2_mant_hi,s1_mant_hi,s1_mant_hi
	bno	rem_d_22			/* J/ oversubtract		*/

rem_d_12:
	addc	quo,quo,quo			/* Accumulate quotient		*/
	cmpo	1,0				/* Clear carry			*/
	addc	s1_mant_lo,s1_mant_lo,s1_mant_lo /* Left shift accumulator */
	addc	s1_mant_hi,s1_mant_hi,s1_mant_hi
	cmpdeci	0,loop_cntl,loop_cntl
	bne	rem_d_10			/* J/ more REMRL loops to perform */

#if	defined(USE_ESHRO)
	eshro	1,s1_mant,s1_mant_lo
#else
	shlo	31,s1_mant_hi,tmp		/* Undo overshift		*/
	shro	1,s1_mant_lo,s1_mant_lo
	or	tmp,s1_mant_lo,s1_mant_lo
#endif
	shro	1,s1_mant_hi,s1_mant_hi
	b	rem_d_30			/* Package result		*/


rem_d_20:
	addc	s1_mant_lo,s1_mant_lo,s1_mant_lo /* Left shift accum	*/
	addc	s1_mant_hi,s1_mant_hi,s1_mant_hi
	cmpo	1,0				 /* Clear carry		*/
	addc	s2_mant_lo,s1_mant_lo,s1_mant_lo /* Non-rstr osub corr	*/
	addc	s2_mant_hi,s1_mant_hi,s1_mant_hi
	be	rem_d_12			 /* J/ corr completed	*/

rem_d_22:
	addc	quo,quo,quo			 /* Accumulate quotient		*/
	cmpdeci	0,loop_cntl,loop_cntl
	bne	rem_d_20			 /* J/ more REMRL loops		*/

	cmpo	1,0				 /* Clear carry			*/
	addc	s2_mant_lo,s1_mant_lo,s1_mant_lo /* Complete correction	*/
	addc	s2_mant_hi,s1_mant_hi,s1_mant_hi

/* Compute the sticky bit so that the quotient value is:		*/
/*									*/
/*			qq..qqrs					*/
/*									*/
/* Where	qq..qq  LS 30 bits of the integral quotient magnitude	*/
/*		r	is the rounding bit (the next quotient bit)	*/
/*		s	set if a remainder exists after qq..qqr		*/

rem_d_30:
	or	s1_mant_lo,s1_mant_hi,tmp	/* Check for exact result	*/
	cmpo	0,tmp				/* Carry set if Z result	*/
	addc	quo,quo,quo			/* Retain "inexact" bit		*/
	xor	1,quo,quo			/* Correct polarity of bit	*/

/* Adjust the result based on the type of operation rem_type (called	*/
/* MOD in the IEEE test suite) or the rmd_type (the IEEE remainder)	*/

	cmpobe	rmd_type,op_type,rem_d_30c	/* J/ RMD operation		*/

	and	0xF,quo,quo			/* Trunc quo return value	*/

	bbc	1,quo,rem_d_31			/* J/ no adjustment (R not set)	*/

rem_d_30a:
	cmpo	1,0				/* Undo extra bit's subtract	*/
	addc	s2_mant_lo,s1_mant_lo,s1_mant_lo
	addc	s2_mant_hi,s1_mant_hi,s1_mant_hi

	bbc	21,s1_mant_hi,rem_d_31		/* J/ No forced right shift	*/
#if	defined(USE_ESHRO)
	eshro	1,s1_mant,s1_mant_lo
#else
	shlo	31,s1_mant_hi,tmp		/* Undo extra loop's shift	*/
	shro	1,s1_mant_lo,s1_mant_lo
	or	tmp,s1_mant_lo,s1_mant_lo
#endif
	shro	1,s1_mant_hi,s1_mant_hi

	addo	1,s2_exp,s2_exp			/* Adjust result exponent	*/
	b	rem_d_34			/* J/ normalized		*/


rem_d_30c:
	mov	quo,tmp				/* save a copy of the raw quo	*/
	shro	2,quo,quo			/* RMD: only int part of quo	*/
	bbc	1,tmp,rem_d_31			/* J/ R bit = 0 -> no RMD adj	*/
	and	5,tmp,tmp
	cmpobe	0,tmp,rem_d_30a			/* J/ Q0 and S bits = 0, undo	*/

	cmpo	0,0				/* Subtract for neg result	*/
	subc	s1_mant_lo,s2_mant_lo,s1_mant_lo
	subc	s1_mant_hi,s2_mant_hi,s1_mant_hi
	notbit	31,s1_sign,s1_sign
	addlda(1,quo,quo)			/* Bump integral quotient	*/


/* Normalize result significand then check exponent for result building	*/

rem_d_31:
	scanbit	s1_mant_hi,tmp			/* Look for MS bit		*/
	bno	rem_d_36			/* J/ top word = 0		*/

rem_d_32:
	subo	tmp,31-11,tmp			/* Normalization shift count	*/
	cmpobe	0,tmp,rem_d_34			/* J/ no normalizing shift	*/

rem_d_33:
	ldconst	32,tmp2				/* Normalize rslt (64-bit SHRL)	*/
	subo	tmp,tmp2,tmp2
#if	defined(USE_ESHRO)
	eshro	tmp2,s1_mant,s1_mant_hi
#else
	shro	tmp2,s1_mant_lo,tmp2
	shlo	tmp,s1_mant_hi,s1_mant_hi
	or	tmp2,s1_mant_hi,s1_mant_hi
#endif
	shlo	tmp,s1_mant_lo,s1_mant_lo

	subo	tmp,s2_exp,s2_exp		/* Adjust exponent		*/

rem_d_34:
	cmpibge	0,s2_exp,rem_d_45		/* J/ denormalized result	*/

	clrbit	20,s1_mant_hi,s1_mant_hi	/* Strip "j" bit		*/
	shlo	20,s2_exp,s2_exp		/* Position and mix in exponent	*/

	or	s2_exp,s1_mant_hi,s1_mant_hi

rem_d_35:					/* (ent for zero/denorm rslt)	*/
	or	s1_sign,s1_mant_hi,out_hi	/* Result			*/
	movldar(s1_mant_lo,out_lo)
	
	mov	quo,out2
	ret


rem_d_36:
	scanbit	s1_mant_lo,tmp
	bno	rem_d_35			/* J/ result zero		*/

	cmpibge	31-11,tmp,rem_d_37		/* J/ >= 32 bit shift		*/
	ldconst	63-11,tmp2			/* Compute norm shift count	*/
	subo	tmp,tmp2,tmp
	b	rem_d_33			/* J/ rejoin standard flow	*/


rem_d_37:
	mov	s1_mant_lo,s1_mant_hi		/* Perform a 32-bit shift	*/
	mov	0,s1_mant_lo
	subo	31,s2_exp,s2_exp		/* Adjust exponent		*/
	subo	1,s2_exp,s2_exp
	b	rem_d_32			/* J/ for remaining norm shift	*/


rem_d_40:					/* EXP(src2) < EXP(src1)	*/
	mov	1,out2				/* Inexact result		*/
	ret					/* Return S1 in place		*/


rem_d_45:
#if	!defined(USE_ESHRO)
	addo	16,16,tmp2			/* Constant of 32		*/
#endif
	subo	s2_exp,1,tmp			/* Shift to create denormalized	*/
	cmpobl	31,tmp,rem_d_47			/* J/ >= 32-bit shift		*/

#if	defined(USE_ESHRO)
	eshro	tmp,s1_mant,s1_mant_lo		/* 64-bit shro			*/
#else
	subo	tmp,tmp2,tmp2			/* Conjugate shift count	*/
	shro	tmp,s1_mant_lo,s1_mant_lo
	shlo	tmp2,s1_mant_hi,tmp2
	or	s1_mant_lo,tmp2,s1_mant_lo
#endif
	shro	tmp,s1_mant_hi,s1_mant_hi
	b	rem_d_35


rem_d_47:
	and	0x1f,tmp,tmp			/* Chop shift count		*/

	shro	tmp,s1_mant_hi,s1_mant_lo
	movlda(0,s1_mant_hi)
	b	rem_d_35



/*  S1 is a special case value: +/-0, denormal, +/-INF, NaN; S2 is unknown  */

rem_d_s1_special:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	cmpo	s1_lo,0				/* Condense lo word to carry	*/
	lda	DP_INF << 21,con_1

	addc	s1_hi,s1_hi,tmp			/* Drop sign bit, add carry bit	*/
	xor	1,tmp,tmp			/* Correct polarity of LS bit	*/
	cmpobe	con_1,tmp,rem_d_s1_20		/* J/ S1 is INF			*/
	bl	AFP_NaN_D			/* J/ S1 is NaN			*/

	cmpobne	0,tmp,rem_d_s1_10		/* J/ S1 is denormal		*/

	cmpo	s2_lo,0				/* Condense lo word into LS bit */
	addc	s2_hi,s2_hi,tmp2		/* S1 is zero - examine s2 */
	xor	1,tmp2,tmp2
	cmpobe	0,tmp2,rem_d_s1_21		/* J/ 0 REM 0 -> Invalid opn	*/
	cmpobg	tmp2,con_1,rem_d_s1_06		/* J/ S2 is NaN			*/

	bbs.t	AC_Norm_Mode,ac,rem_d_s1_04	/* J/ normalizing mode		*/
	ldconst	0x001fffff,con_1
	cmpobg	tmp2,con_1,rem_d_s1_04		/* J/ not a denormal		*/

rem_d_s1_02:
	b	_AFP_Fault_Reserved_Encoding_D


rem_d_s1_04:
/*	movl	s1,out	*/			/* Return S1 (in place)		*/
	mov	0,out2				/* Quo is 0			*/
	ret


rem_d_s1_06:
	b	AFP_NaN_D			/* Handle NaN operand(s) */


rem_d_s1_10:
	bbc.f	AC_Norm_Mode,ac,rem_d_s1_02	/* J/ denorm, not norm mode	*/

	shro	1,tmp,tmp			/* tmp = s1_hi w/o sign bit 	*/
	scanbit	tmp,tmp
	bno	rem_d_s1_14			/* J/ MS word = 0		*/

rem_d_s1_12:
	subo	tmp,20,tmp			/* Top bit num to shift count	*/
	and	0x1f,tmp,tmp			/* (when MS word = 0)		*/
	subo	tmp,1,s1_exp			/* set s1_exp value		*/

	shlo	tmp,s1_hi,s1_mant_hi		/* Normalize denorm significand	*/
	shlo	tmp,s1_lo,s1_mant_lo
	subo	tmp,31,tmp			/* word -> word bit xfer	*/
	addo	1,tmp,tmp
	shro	tmp,s1_lo,tmp
	or	tmp,s1_mant_hi,s1_mant_hi
	lda	0xFFF00000,tmp2			/* restore TMP2 mask		*/
	b	rem_d_s1_rejoin


rem_d_s1_14:
	scanbit	s1_lo,tmp
	cmpoble	21,tmp,rem_d_s1_12		/* J/ not a full word shift	*/

	subo	tmp,20,tmp
	subo	tmp,0,s1_exp			/* set s1_exp value		*/
	subo	31,s1_exp,s1_exp
	shlo	tmp,s1_lo,s1_mant_hi		/* 32+ bit shift		*/
	mov	0,s1_mant_lo
	lda	0xFFF00000,tmp2			/* restore TMP2 mask		*/
	b	rem_d_s1_rejoin


rem_d_s1_20:
	cmpo	s2_lo,0				/* Condense lo word into LS bit	*/
	addc	s2_hi,s2_hi,tmp2		/* Drop sign, append "EQ" bit	*/
	xor	tmp2,1,tmp2			/* Correct sense of LS bit	*/

	cmpobg	tmp2,con_1,rem_d_s1_06		/* S2 is NaN -> NaN handler	*/

rem_d_s1_21:
	bbc	AC_InvO_mask,ac,rem_d_s1_22	/* J/ inv oper fault not masked */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac		/* Set inv oper flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp		/* Set inv oper flag */
	modac	tmp,tmp,tmp
#endif
	ldconst	Standard_QNaN_hi,out_hi		/* Return a quiet NaN */
	ldconst	Standard_QNaN_lo,out_lo
	ret


rem_d_s1_22:
	b	_AFP_Fault_Invalid_Operation_D


/*  S2 is a special case value: +/-0, denormal, +/-INF, NaN; S2 is a non-  */
/*  zero, finite value.                                                    */

/*  Note: S2 = 0 is processed as an invalid operation.  The 80960KB prog.  */
/*  indicates that the part handles it as a zero divide (!).               */

rem_d_s2_special:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	cmpo	s2_lo,0				/* Condense lo word into LS bit	*/
	ldconst	DP_INF << 21,con_1
	addc	s2_hi,s2_hi,tmp2		/* Drop sign bit		*/
	xor	1,tmp2,tmp2
	cmpobe	con_1,tmp2,rem_d_s1_04		/* J/ S2 is INF -> return S1	*/
	bl	AFP_NaN_D			/* J/ S2 is NaN			*/
	cmpobe	0,tmp2,rem_d_s1_21		/* J/ finite REM 0 -> inv opn	*/
	bbc.f	AC_Norm_Mode,ac,rem_d_s1_02	/* J/ denorm, not norm mode	*/

	shro	1,tmp2,tmp2			/* tmp = s2_hi w/o sign bit	*/
	scanbit	tmp2,tmp
	bno	rem_d_s2_04			/* J/ MS word = 0		*/

rem_d_s2_02:
	subo	tmp,20,tmp			/* Top bit num to shift count	*/
	and	0x1f,tmp,tmp			/* (when MS word = 0)		*/
	subo	tmp,1,s2_exp			/* set s2_exp value		*/

	shlo	tmp,s2_hi,s2_mant_hi		/* Normalize denorm significand	*/
	shlo	tmp,s2_lo,s2_mant_lo
	subo	tmp,31,tmp			/* word -> word bit xfer	*/
	addo	1,tmp,tmp
	shro	tmp,s2_lo,tmp
	or	tmp,s2_mant_hi,s2_mant_hi
	b	rem_d_s2_rejoin

rem_d_s2_04:
	scanbit	s2_lo,tmp
	cmpoble	21,tmp,rem_d_s2_02		/* J/ not a full word shift	*/

	subo	tmp,20,tmp
	subo	tmp,0,s2_exp			/* set s2_exp field		*/
	subo	31,s2_exp,s2_exp
	shlo	tmp,s2_lo,s2_mant_hi		/* 32+ bit shift		*/
	mov	0,s2_mant_lo
	b	rem_d_s2_rejoin
