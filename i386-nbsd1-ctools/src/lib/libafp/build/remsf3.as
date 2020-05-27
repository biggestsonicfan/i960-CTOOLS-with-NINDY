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
/*      remsf3.c - Single Precision Remainder Routine (AFP-960)		      */
/*									      */
/******************************************************************************/

#include "asmopt.h"

#if	!defined(KEEP_INTERNAL_LABELS)
#define	rem_s_00		L_rem_s_00
#define	rem_s_s1_rejoin		L_rem_s_s1_rejoin
#define	rem_s_s2_rejoin		L_rem_s_s2_rejoin
#define	rem_s_10		L_rem_s_10
#define	rem_s_12		L_rem_s_12
#define	rem_s_20		L_rem_s_20
#define	rem_s_22		L_rem_s_22
#define	rem_s_30		L_rem_s_30
#define	rem_s_30a		L_rem_s_30a
#define	rem_s_30c		L_rem_s_30c
#define	rem_s_31		L_rem_s_31
#define	rem_s_33		L_rem_s_33
#define	rem_s_34		L_rem_s_34
#define	rem_s_40		L_rem_s_40
#define	rem_s_45		L_rem_s_45
#define	rem_s_s1_special	L_rem_s_s1_special
#define	rem_s_s1_02		L_rem_s_s1_02
#define	rem_s_s1_04		L_rem_s_s1_04
#define	rem_s_s1_06		L_rem_s_s1_06
#define	rem_s_s1_10		L_rem_s_s1_10
#define	rem_s_s1_20		L_rem_s_s1_20
#define	rem_s_s1_21		L_rem_s_s1_21
#define	rem_s_s1_22		L_rem_s_s1_22
#define	rem_s_s2_special	L_rem_s_s2_special
#endif


	.file	"remsf3.as"
	.globl	___remsf3
	.globl	___rmdsf3

#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	AFP_NaN_S

	.globl	_AFP_Fault_Invalid_Operation_S
	.globl	_AFP_Fault_Reserved_Encoding_S


#define	FP_INF          0xff


#define	AC_Norm_Mode    29

#define	AC_InvO_mask    26
#define	AC_InvO_flag    18


#define	Standard_QNaN   0xffc00000


/* Register Name Equates */

#define	s1         g0
#define	s2         g1

#define	s2_mant    r4
#define	s2_exp     r5
#define	s1_sign    r6
#define	loop_cntl  r7
#define	s1_mant    r8
#define	s1_exp     r9
#define	tmp        r10
#define	tmp2       r11
#define	con_1      r12
#define	quo        r14
#define	ac         r15

#define	op_type    g2
#define	rem_type   15
#define	rmd_type   17

#define	out        g0
#define	out2       g1


	.text
	.link_pix

___rmdsf3:
	ldconst	rmd_type,op_type		/* for fault handler operation info */
	b	rem_s_00


___remsf3:
	ldconst	rem_type,op_type		/* for fault handler operation info */

rem_s_00:
	shlo	1,s1,s1_exp			/* Drop the sign bit		*/
	onebit(31,s1_sign)			/* Mask for result sign bit	*/

	and	s1,s1_sign,s1_sign		/* Extract result sign bit	*/
	shri	24,s1_exp,tmp			/* For special case detection	*/

	shlo	1,s2,s2_exp			/* Drop the sign bit		*/
	addlda(1,tmp,tmp)			/* Special case testing		*/

#if	!defined(USE_CMP_BCC)
	cmpo	2,tmp				/* S1 zero/denormal/INF/NaN?	*/
#endif

	lda	0xff800000,tmp2			/* Extract significand		*/

	notand	s1,tmp2,s1_mant			/* Extract significand bits	*/

	subo	tmp2,s1_mant,s1_mant		/* Set "j" bit			*/
	shro	24,s1_exp,s1_exp		/* Standard exponent		*/

#if	defined(USE_CMP_BCC)
	cmpobg	2,tmp,rem_s_s1_special		/* J/ S1 special case		*/
#else
	bg.f	rem_s_s1_special		/* J/ S1 special case		*/
#endif

rem_s_s1_rejoin:

	shri	24,s2_exp,tmp			/* Special case testing		*/
	andnot	tmp2,s2,s2_mant			/* Extract significand		*/

	subo	tmp2,s2_mant,s2_mant		/* Set "j" bit			*/
	addlda(1,tmp,tmp)			/* Special case detection	*/

#if	!defined(USE_CMP_BCC)
	cmpo	2,tmp				/* S2 zero/denormal/INF/NaN?	*/
#endif

	shro	24,s2_exp,s2_exp		/* Standard exponent		*/

#if	defined(USE_CMP_BCC)
	cmpobg	2,tmp,rem_s_s2_special		/* J/ S2 special case		*/
#else
	bg.f	rem_s_s2_special		/* J/ S2 special case		*/
#endif

rem_s_s2_rejoin:

	subo	1,s2_exp,s2_exp			/* Force an extra loop		*/

#if	!defined(USE_CMP_BCC)
	cmpi	s1_exp,s2_exp
#endif

	subo	s2_exp,s1_exp,loop_cntl		/* Number of REMR loops		*/
	movlda(0,quo)				/* Init quotient		*/

#if	defined(USE_CMP_BCC)
	cmpibl	s1_exp,s2_exp,rem_s_40		/* J/ no remaindering loops	*/
#else
	bl.f	rem_s_40			/* J/ no remaindering loops	*/
#endif


/*  Non-restoring division alg to get REMR result  */

rem_s_10:
	cmpo	0,0				/* Set Not Borrow bit		*/
	subc	s2_mant,s1_mant,s1_mant
	bno	rem_s_22			/* J/ oversubtract		*/

rem_s_12:
	addc	quo,quo,quo			/* Accumulate quotient		*/
	cmpdeci	0,loop_cntl,loop_cntl
	shlo	1,s1_mant,s1_mant		/* rem_s_eft shift accumulator	*/
	bne.t	rem_s_10			/* J/ more REMR loops		*/

	shro	1,s1_mant,s1_mant		/* Correct overshift		*/
	b	rem_s_30			/* Package result		*/

rem_s_20:
	shlo	1,s1_mant,s1_mant		/* rem_s_eft shift accumulator	*/
	addc	s2_mant,s1_mant,s1_mant		/* Non-restoring oversub corr	*/
	be	rem_s_12			/* J/ correction completed	*/

rem_s_22:
	addc	quo,quo,quo			/* Accumulate quotient		*/
	cmpdeci	0,loop_cntl,loop_cntl
	bne.t	rem_s_20			/* J/ more REMR loops		*/

	addo	s2_mant,s1_mant,s1_mant		/* Complete correction		*/


/* Compute the sticky bit so that the quotient value is:		*/
/*									*/
/*			qq..qqrs					*/
/*									*/
/* Where	qq..qq  LS 30 bits of the integral quotient magnitude	*/
/*		r	is the rounding bit (the next quotient bit)	*/
/*		s	set if a remainder exists after qq..qqr		*/


rem_s_30:
	cmpo	s1_mant,0			/* Carry set if Z result	*/
	addc	quo,quo,quo			/* Retain "inexact" bit		*/
	xor	1,quo,quo			/* Correct polarity of bit	*/

/* Adjust the result based on the type of operation rem_type (called	*/
/* MOD in the IEEE test suite) or the rmd_type (the IEEE remainder)	*/

	cmpobe	rmd_type,op_type,rem_s_30c	/* J/ RMD operation		*/

	and	0xF,quo,quo			/* Trunc quo return value	*/

	bbc	1,quo,rem_s_31			/* J/ no adjustment (R not set)	*/

rem_s_30a:
	addo	s2_mant,s1_mant,s1_mant		/* Undo extra bit's subtract	*/

	bbc	24,s1_mant,rem_s_33		/* J/ normalized result		*/

	shro	1,s1_mant,s1_mant		/* Right normalization shift	*/
	addo	1,s2_exp,s2_exp			/* Bump exponent		*/
	b	rem_s_33			/* J/ normalized result		*/

rem_s_30c:
	mov	quo,tmp				/* save a copy of the raw quo	*/
	shro	2,quo,quo			/* RMD: only int part of quo	*/
	bbc	1,tmp,rem_s_31			/* J/ R bit = 0 -> no RMD adj	*/
	and	5,tmp,tmp
	cmpobe	0,tmp,rem_s_30a			/* J/ Q0 and S bits = 0, undo	*/

	subo	s1_mant,s2_mant,s1_mant		/* Subtract again (neg result)	*/
	notbit	31,s1_sign,s1_sign
	addlda(1,quo,quo)			/* Bump integral quotient	*/


/* Normalize result significand then check exponent for result building	*/

rem_s_31:
	scanbit	s1_mant,tmp			/* rem_s_ook for MS bit		*/
	bno.f	rem_s_34			/* J/ zero REMR result		*/

	subo	tmp,31-8,tmp			/* Shift for 8 MS zero bits	*/
	subo	tmp,s2_exp,s2_exp		/* Adjust exponent as req'd	*/
	shlo	tmp,s1_mant,s1_mant		/* Normalize result		*/

rem_s_33:
	cmpibge.f 0,s2_exp,rem_s_45		/* J/ denormalized result	*/

	shlo	23,s2_exp,s2_exp		/* Position the exponent	*/
	clrbit	23,s1_mant,s1_mant		/* Strip "j" bit		*/

	or	s2_exp,s1_mant,s1_mant		/* Mix in the result exponent	*/

rem_s_34:					/* (ent for zero/denorm rslt)	*/
	addo	s1_sign,s1_mant,out		/* REM result			*/
	movldar(quo,out2)			/* Return the quotient bits	*/
	ret


rem_s_40:					/* EXP(src1) < EXP(src2)-1	*/
	mov	1,out2				/* Inexact result		*/
	ret					/* Return S1 in place		*/


rem_s_45:
	subo	s2_exp,1,tmp			/* Shift to create denormilized	*/
	shro	tmp,s1_mant,s1_mant		/* result value			*/
	b	rem_s_34			/* J/ return completion		*/


/*  S1 is a special case value: +/-0, denormal, +/-INF, NaN; S2 is unknown  */

rem_s_s1_special:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	shlo	1,s1,tmp			/* Drop sign bit		*/
	lda	FP_INF << 24,con_1

	cmpobe	tmp,con_1,rem_s_s1_20		/* J/ S1 is INF			*/
	bg	AFP_NaN_S			/* J/ S1 is NaN			*/

	cmpobne	0,tmp,rem_s_s1_10		/* J/ S1 is denormal		*/

	shlo	1,s2,tmp2
	cmpobe	0,tmp2,rem_s_s1_21		/* J/ 0 REM 0 -> Invalid opn	*/
	cmpobg	tmp2,con_1,rem_s_s1_06		/* J/ S2 is NaN			*/

	bbs	AC_Norm_Mode,ac,rem_s_s1_04	/* J/ normalizing mode		*/
	ldconst	0x00fffffe,con_1
	cmpobg	tmp2,con_1,rem_s_s1_04		/* J/ not a denormal		*/

rem_s_s1_02:
	b	_AFP_Fault_Reserved_Encoding_S

rem_s_s1_04:
/*	mov	s1,out	*/			/* Return S1 (in place)		*/
	mov	0,out2				/* Quo is 0			*/
	ret

rem_s_s1_06:
	b	AFP_NaN_S			/* Handle NaN operand(s)	*/



rem_s_s1_10:
	bbc	AC_Norm_Mode,ac,rem_s_s1_02	/* J/ denorm, not norm mode	*/

	scanbit	tmp,tmp2
	subo	22+1,tmp2,s1_exp		/* compute denormal exponent	*/
	subo	tmp2,23,tmp2
	shlo	tmp2,tmp,s1_mant		/* normalize denorm significand	*/

	lda	0xFF800000,tmp2			/* restore tmp2 mask value	*/
	b	rem_s_s1_rejoin


rem_s_s1_20:
	shlo	1,s2,tmp2			/* Drop S2 sign			*/
	cmpobg	tmp2,con_1,rem_s_s1_06		/* S2 is NaN -> NaN handler	*/

rem_s_s1_21:
	bbc	AC_InvO_mask,ac,rem_s_s1_22	/* J/ inv oper fault not masked	*/

#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac		/* Set inv oper flag		*/
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp		/* Set inv oper flag		*/
	modac	tmp,tmp,tmp
#endif

	ldconst	Standard_QNaN,out		/* Return a quiet NaN		*/
	ret

rem_s_s1_22:
	b	_AFP_Fault_Invalid_Operation_S



/*  S2 is a special case value: +/-0, denormal, +/-INF, NaN; S2 is a non-  */
/*  zero, finite value.                                                    */

/*  Note: S2 = 0 is processed as an invalid operation.  The 80960KB prog.  */
/*  indicates that the part handles it as a zero divide (!).               */

rem_s_s2_special:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	shlo	1,s2,tmp2			/* Drop sign bit		*/
	ldconst	FP_INF << 24,con_1
	cmpobe	tmp2,con_1,rem_s_s1_04		/* J/ S2 is a INF -> return S1	*/
	bg	rem_s_s1_06			/* J/ S2 is NaN			*/
	cmpobe	0,tmp2,rem_s_s1_21		/* J/ finite REM 0 -> inv opn	*/
	bbc	AC_Norm_Mode,ac,rem_s_s1_02	/* J/ denorm, not norm mode	*/

	scanbit	tmp2,tmp			/* Normalize denorm significand	*/
	subo	22+1,tmp,s2_exp
	subo	tmp,22+1,tmp
	shlo	tmp,tmp2,s2_mant
	b	rem_s_s2_rejoin
