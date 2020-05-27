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
/*      remtf3.c - Extended Precision Remainder Routine (AFP-960)	      */
/*									      */
/******************************************************************************/

#include "asmopt.h"

#if	!defined(KEEP_INTERNAL_LABELS)
#define	rem_t_00		L_rem_t_00
#define	rem_t_s1_rejoin		L_rem_t_s1_rejoin
#define	rem_t_s2_rejoin		L_rem_t_s2_rejoin
#define	rem_t_10		L_rem_t_10
#define	rem_t_12		L_rem_t_12
#define	rem_t_20		L_rem_t_20
#define	rem_t_22		L_rem_t_22
#define	rem_t_30		L_rem_t_30
#define	rem_t_30a		L_rem_t_30a
#define	rem_t_30b		L_rem_t_30b
#define	rem_t_30c		L_rem_t_30c
#define	rem_t_31		L_rem_t_31
#define	rem_t_32		L_rem_t_32
#define	rem_t_33		L_rem_t_33
#define	rem_t_34		L_rem_t_34
#define	rem_t_34a		L_rem_t_34a
#define	rem_t_35		L_rem_t_35
#define	rem_t_36		L_rem_t_36
#define	rem_t_38		L_rem_t_38
#define	rem_t_40		L_rem_t_40
#define	rem_t_45		L_rem_t_45
#define	rem_t_47		L_rem_t_47
#define	rem_t_s1_special	L_rem_t_s1_special
#define	rem_t_s1_unnormal	L_rem_t_s1_unnormal
#define	rem_t_s2_unnormal	L_rem_t_s2_unnormal
#define	rem_t_s1_02		L_rem_t_s1_02
#define	rem_t_s1_04		L_rem_t_s1_04
#define	rem_t_s1_06		L_rem_t_s1_06
#define	rem_t_s1_08		L_rem_t_s1_08
#define	rem_t_s1_10		L_rem_t_s1_10
#define	rem_t_s1_12		L_rem_t_s1_12
#define	rem_t_s1_14		L_rem_t_s1_14
#define	rem_t_s1_16		L_rem_t_s1_16
#define	rem_t_s1_18		L_rem_t_s1_18
#define	rem_t_s1_19		L_rem_t_s1_19
#define	rem_t_s1_20		L_rem_t_s1_20
#define	rem_t_s2_special	L_rem_t_s2_special
#define	rem_t_s2_02		L_rem_t_s2_02
#define	rem_t_s2_04		L_rem_t_s2_04
#endif


	.file	"remtf3.as"
	.globl	___remtf3
	.globl	___rmdtf3

#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	AFP_NaN_T

	.globl	_AFP_Fault_Invalid_Operation_T
	.globl	_AFP_Fault_Reserved_Encoding_T


#define	AC_Norm_Mode    29

#define	AC_InvO_mask    26
#define	AC_InvO_flag    18

#define	Standard_QNaN_se  0x0000ffff
#define Standard_QNaN_mhi 0xc0000000
#define	Standard_QNaN_mlo 0x00000000


/* Register Name Equates */

#define	s1         g0
#define	s1_mlo     s1
#define s1_mhi     g1
#define	s1_se      g2
#define s2         g4
#define s2_mlo     s2
#define s2_mhi     g5
#define s2_se      g6


#define	s2_mant    r4
#define s2_mant_lo s2_mant
#define s2_mant_hi r5
#define	s2_exp     r6


#define	s1_mant    r8
#define s1_mant_lo s1_mant
#define s1_mant_hi r9
#define	s1_exp     r11
#define	s1_sign    r3

#define	loop_cntl  r7

#define	quo        r10

#define	tmp        r12
#define	tmp2       r13
#define	tmp3       r14
#define	ac         r15

#define	s1_mant_x  tmp3

#define	con_1      g3

#define	out        g0
#define	out_mlo    out
#define	out_mhi    g1
#define out_se     g2

#define	out2       g4

#define	op_type    g7
#define	rem_type   15
#define	rmd_type   17


	.text
	.link_pix

___rmdtf3:
	mov	rmd_type,op_type
	b	rem_t_00

___remtf3:
	mov	rem_type,op_type

rem_t_00:
	shlo	32-15,s1_se,s1_exp		/* exp only (no sign bit)	*/
	onebit(15,s1_sign)			/* sign bit mask		*/

	and	s1_se,s1_sign,s1_sign
	shri	32-15,s1_exp,tmp		/* check S1 for potentially	*/

	shlo	32-15,s2_se,s2_exp		/* exp only (no sign bit)	*/
	addlda(1,tmp,tmp)			/* special case handling	*/

#if	!defined(USE_CMP_BCC)
	cmpo	1,tmp
#endif

	movldar(s1_mlo,s1_mant_lo)		/* copy mantissa bits		*/

#if	defined(USE_CMP_BCC)
	cmpobge	1,tmp,rem_t_s1_special		/* J/ NaN/INF, 0, denormal	*/
#else
	bge.f	rem_t_s1_special		/* J/ NaN/INF, 0, denormal	*/
#endif

#if	!defined(USE_CMP_BCC)
	chkbit	31,s1_mhi
#endif

	shro	32-15,s1_exp,s1_exp		/* right justify exponent	*/
	movldar(s1_mhi,s1_mant_hi)

#if	defined(USE_CMP_BCC)
	bbc	31,s1_mhi,rem_t_s1_unnormal	/* J/ S1 is an unnormal		*/
#else
	bf.f	rem_t_s1_unnormal		/* J/ S1 is an unnormal		*/
#endif

rem_t_s1_rejoin:

	shri	32-15,s2_exp,tmp
	movldar(s2_mlo,s2_mant_lo)		/* copy mantissa bits		*/

	shro	32-15,s2_exp,s2_exp		/* right justify exponent	*/
	addlda(1,tmp,tmp)			/* check S2 for potentially	*/

#if	!defined(USE_CMP_BCC)
	cmpo	1,tmp
#endif

	movldar(s2_mhi,s2_mant_hi)

#if	defined(USE_CMP_BCC)
	cmpobge	1,tmp,rem_t_s2_special		/* J/ NaN/INF, 0/denormal	*/
#else
	bge.f	rem_t_s2_special		/* J/ NaN/INF, 0/denormal	*/
#endif

#if	defined(USE_CMP_BCC)
	bbc	31,s2_mhi,rem_t_s2_unnormal	/* J/ S2 is an unnormal		*/
#else
	chkbit	31,s2_mhi
	bf.f	rem_t_s2_unnormal		/* J/ S2 is an unnormal		*/
#endif

rem_t_s2_rejoin:

	subo	1,s2_exp,s2_exp			/* Force an extra loop		*/

#if	!defined(USE_CMP_BCC)
	cmpi	s1_exp,s2_exp
#endif

	subo	s2_exp,s1_exp,loop_cntl		/* Number of loops		*/

#if	defined(USE_CMP_BCC)
	cmpibl	s1_exp,s2_exp,rem_t_40		/* J/ no remaindering loops	*/
#else
	bl.f	rem_t_40			/* J/ no remaindering loops	*/
#endif


/*  Non-restoring division alg to get REMRL result  */

	mov	0,quo				/* Quotient register		*/
	movlda(0,s1_mant_x)
rem_t_10:
	cmpo	0,0				/* Set Not Borrow bit		*/
	subc	s2_mant_lo,s1_mant_lo,s1_mant_lo
	subc	s2_mant_hi,s1_mant_hi,s1_mant_hi
	subc	0,s1_mant_x,s1_mant_x
	bno	rem_t_22			/* J/ oversubtract		*/

rem_t_12:
	addc	quo,quo,quo			/* Accumulate quotient		*/
	cmpo	1,0				/* Clear carry			*/
	addc	s1_mant_lo,s1_mant_lo,s1_mant_lo /* Left shift accumulator */
	addc	s1_mant_hi,s1_mant_hi,s1_mant_hi
	addc	s1_mant_x,s1_mant_x,s1_mant_x
	cmpdeci	0,loop_cntl,loop_cntl
	bne	rem_t_10			/* J/ more REMRL loops to perform */

#if	defined(USE_ESHRO)
	eshro	1,s1_mant,s1_mant_lo		/* Undo overshift		*/
#else
	shlo	31,s1_mant_hi,tmp		/* Undo overshift		*/
	shro	1,s1_mant_lo,s1_mant_lo
	or	tmp,s1_mant_lo,s1_mant_lo
#endif

	shlo	31,s1_mant_x,tmp2
	shro	1,s1_mant_hi,s1_mant_hi
	or	tmp2,s1_mant_hi,s1_mant_hi

	shro	1,s1_mant_x,s1_mant_x
	b	rem_t_30			/* Package result		*/


rem_t_20:
	addc	s1_mant_lo,s1_mant_lo,s1_mant_lo /* Left shift accumulator */
	addc	s1_mant_hi,s1_mant_hi,s1_mant_hi
	addc	s1_mant_x,s1_mant_x,s1_mant_x
	cmpo	1,0				/* Clear carry			*/
	addc	s2_mant_lo,s1_mant_lo,s1_mant_lo /* Non-restor oversub corr */
	addc	s2_mant_hi,s1_mant_hi,s1_mant_hi
	addc	0,s1_mant_x,s1_mant_x
	be	rem_t_12			/* J/ correction completed	*/

rem_t_22:
	addc	quo,quo,quo			/* Accumulate quotient		*/
	cmpdeci	0,loop_cntl,loop_cntl
	bne	rem_t_20			/* J/ more REMRL loops		*/

	cmpo	1,0				/* Clear carry			*/
	addc	s2_mant_lo,s1_mant_lo,s1_mant_lo /* Complete correction	*/
	addc	s2_mant_hi,s1_mant_hi,s1_mant_hi
	addc	0,s1_mant_x,s1_mant_x


/* Compute the sticky bit so that the quotient value is:		*/
/*									*/
/*			qq..qqrs					*/
/*									*/
/* Where	qq..qq  LS 30 bits of the integral quotient magnitude	*/
/*		r	is the rounding bit (the next quotient bit)	*/
/*		s	set if a remainder exists after qq..qqr		*/

rem_t_30:
	or	s1_mant_lo,s1_mant_hi,tmp	/* Check for exact result	*/
	cmpo	0,tmp				/* Carry set if Z result	*/
	addc	quo,quo,quo			/* Retain "inexact" bit		*/
	xor	1,quo,quo			/* Correct polarity of bit	*/

/* Adjust the result based on the type of operation rem_type (called	*/
/* MOD in the IEEE test suite) or the rmd_type (the IEEE remainder)	*/

	cmpobe	rmd_type,op_type,rem_t_30c	/* J/ RMD operation		*/

	and	0xF,quo,quo			/* Trunc quo return value	*/

	bbc	1,quo,rem_t_31			/* J/ extra quo bit not set	*/

rem_t_30a:
	cmpo	1,0				/* Undo extra loop sub		*/
	addc	s2_mant_lo,s1_mant_lo,s1_mant_lo
	addc	s2_mant_hi,s1_mant_hi,s1_mant_hi
	be	rem_t_30b			/* J/ carry out -> right shift	*/
	b	rem_t_34			/* J/ no mant norm shifting	*/

rem_t_30b:					/* Right normalization shift	*/
#if	defined(USE_ESHRO)
	eshro	1,s1_mant,s1_mant_lo
#else
	shlo	31,s1_mant_hi,tmp
	shro	1,s1_mant_lo,s1_mant_lo
	or	tmp,s1_mant_lo,s1_mant_lo
#endif

	shro	1,s1_mant_hi,s1_mant_hi
	setbit	31,s1_mant_hi,s1_mant_hi

	addo	1,s2_exp,s2_exp			/* Bump exponent		*/
	b	rem_t_34			/* J/ no mant norm shifting	*/

/*  RMD processing: force to nearest/even integral quotient		*/

rem_t_30c:
	mov	quo,tmp				/* save a copy of the raw quo	*/
	shro	2,quo,quo			/* RMD: only int part of quo	*/
	bbc	1,tmp,rem_t_31			/* J/ R bit = 0 -> no RMD adj	*/
	and	5,tmp,tmp
	cmpobe	0,tmp,rem_t_30a			/* J/ Q0 and S bits = 0, undo	*/

	cmpo	0,0				/* Subtract for neg result	*/
	subc	s1_mant_lo,s2_mant_lo,s1_mant_lo
	subc	s1_mant_hi,s2_mant_hi,s1_mant_hi
	notbit	15,s1_sign,s1_sign
	addlda(1,quo,quo)			/* Bump integral quotient	*/


/* Normalize result significand then check exponent for result building	*/

rem_t_31:
	bbs	31,s1_mant_hi,rem_t_34		/* J/ normalized		*/

	scanbit	s1_mant_hi,tmp			/* Look for MS bit		*/
	bno	rem_t_36			/* J/ top word = 0		*/

rem_t_32:
	subo	tmp,31,tmp			/* Normalization shift count	*/

rem_t_33:
	ldconst	32,tmp2				/* Normalize rslt (64-bit SHLO)	*/
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

rem_t_34:
	cmpibge.f 0,s2_exp,rem_t_45		/* J/ denormalized result	*/

rem_t_34a:
	movl	s1_mant,out			/* Copy significand		*/
	movldar(s2_exp,out_se)			/* Copy exponent		*/

rem_t_35:					/* (ent for zero/denorm rslt)	*/
	addo	s1_sign,out_se,out_se		/* REM result			*/
	movldar(quo,out2)
	ret


rem_t_36:
	scanbit	s1_mant_lo,tmp
	bno	rem_t_38			/* J/ result zero		*/

	mov	s1_mant_lo,s1_mant_hi		/* Perform a 32-bit shift	*/
	mov	0,s1_mant_lo
	subo	31,s2_exp,s2_exp		/* Adjust exponent		*/
	subo	1,s2_exp,s2_exp
	b	rem_t_32			/* J/ for remaining norm shift	*/

rem_t_38:
	mov	0,s2_exp
	b	rem_t_34a


rem_t_40:					/* EXP(src1) < EXP(src2)-1	*/
	mov	1,out2				/* Inexact result		*/
	ret					/* Return S1 in place		*/


rem_t_45:
	mov	0,out_se			/* Denorm exponent = 0		*/
#if	!defined(USE_ESHRO)
	addo	16,16,tmp2			/* Constant of 32		*/
#endif
	subo	s2_exp,1,tmp			/* Shift to create denormalized	*/
	cmpobl	31,tmp,rem_t_47			/* J/ >= 32-bit shift		*/

#if	defined(USE_ESHRO)
	eshro	tmp,s1_mant,out_mlo		/* 64-bit shro			*/
#else
	subo	tmp,tmp2,tmp2			/* Conjugate shift count	*/
	shro	tmp,s1_mant_lo,s1_mant_lo
	shlo	tmp2,s1_mant_hi,tmp2
	or	s1_mant_lo,tmp2,out_mlo
#endif

	shro	tmp,s1_mant_hi,out_mhi
	b	rem_t_35

rem_t_47:
	and	0x1f,tmp,tmp			/* Chop shift count		*/

	shro	tmp,s1_mant_hi,out_mlo
	movlda(0,out_mhi)
	b	rem_t_35



/*  S1 is a special case value: +/-0, denormal, +/-INF, NaN; S2 is unknown  */

rem_t_s1_special:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	shlo	1,s1_mhi,tmp			/* Drop j bit			*/
	or	s1_mlo,tmp,tmp
	bne	rem_t_s1_20			/* J/ NaN or INF		*/

	cmpobne	0,tmp,rem_t_s1_18		/* J/ S1 is denormal		*/

	shri	32-15,s2_exp,tmp		/* S1 is 0; examine S2		*/
	addo	1,tmp,tmp
	cmpobge	1,tmp,rem_t_s1_06		/* J/ S2 is NaN/INF/denormal/0	*/

	bbs	31,s2_mhi,rem_t_s1_04		/* J/ 0 REM finite -> 0		*/

rem_t_s1_unnormal:
rem_t_s2_unnormal:
rem_t_s1_02:
	b	_AFP_Fault_Reserved_Encoding_T


rem_t_s1_04:
/*	movt	s1,out	*/			/* return S1 (in place)		*/
	movlda(0,out2)				/* Exact			*/
	ret


rem_t_s1_06:
	shlo	1,s2_mhi,tmp			/* Drop j bit			*/
	or	s2_mlo,tmp,tmp
	bne	rem_t_s1_14			/* J/ S2 is NaN/INF		*/

	cmpobne	0,tmp,rem_t_s1_12		/* J/ S2 is denormal		*/

rem_t_s1_08:
	bbc	AC_InvO_mask,ac,rem_t_s1_10	/* J/ inv oper fault not masked	*/
#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac		/* Set inv oper flag		*/
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp		/* Set inv oper flag		*/
	modac	tmp,tmp,tmp
#endif
	ldconst	Standard_QNaN_se,out_se		/* Return a quiet NaN		*/
	ldconst	Standard_QNaN_mhi,out_mhi
	ldconst	Standard_QNaN_mlo,out_mlo
	ret

rem_t_s1_10:
	b	_AFP_Fault_Invalid_Operation_T


rem_t_s1_12:
	bbs	AC_Norm_Mode,ac,rem_t_s1_04	/* J/ norm mode -> ret S1	*/
	b	_AFP_Fault_Reserved_Encoding_T


rem_t_s1_14:
	bbc	31,s2_mhi,rem_t_s1_02		/* J/ INF/NaN w/o j bit		*/

	cmpobe	0,tmp,rem_t_s1_04		/* 0 REM INF -> return S1	*/
rem_t_s1_16:
	b	AFP_NaN_T


rem_t_s1_18:
	bbc	AC_Norm_Mode,ac,rem_t_s1_02	/* J/ denormal, not norm mode	*/

	scanbit	s1_mhi,tmp2
	bno	rem_t_s1_19			/* J/ MS word = 0		*/

	subo	tmp2,31,tmp			/* Left shift to normalize	*/
	addlda(1,tmp2,tmp2)			/* Right shft for wd -> wd xfer	*/
	subo	31,tmp2,s1_exp			/* set s1_exp value		*/

#if	defined(USE_ESHRO)
	eshro	tmp2,s1,s1_mant_hi		/* Normalize denorm significand */
#else
	shlo	tmp,s1_mhi,s1_mant_hi		/* Normalize denorm significand */
	shro	tmp2,s1_mlo,tmp2
	or	tmp2,s1_mant_hi,s1_mant_hi
#endif

	shlo	tmp,s1_mlo,s1_mant_lo
	b	rem_t_s1_rejoin

rem_t_s1_19:
	scanbit	s1_mlo,tmp
	subo	tmp,31,tmp
	shlo	tmp,s1_mlo,s1_mant_hi		/* 32+ bit shift		*/
	subo	tmp,0,s1_exp			/* set s1_exp value		*/
	subo	31,s1_exp,s1_exp
	mov	0,s1_mant_lo
	b	rem_t_s1_rejoin

rem_t_s1_20:
	bbc	31,s1_mhi,rem_t_s1_02		/* J/ INF/NaN w/o j bit		*/

	cmpobne	0,tmp,rem_t_s1_16		/* J/ S1 is a NaN		*/

	shri	32-15,s2_exp,tmp		/* S1 is INF; examine S2	*/
	addo	1,tmp,tmp
	cmpobe	1,tmp,rem_t_s1_08		/* J/ INF REM 0|D -> Inv opn	*/
	bbc	31,s2_mhi,rem_t_s1_02		/* J/ S2 is an unnormal		*/
	cmpobl	1,tmp,rem_t_s1_08		/* J/ INF REM finite -> Inv opn	*/

	shlo	1,s2_mhi,tmp
	or	s2_mlo,tmp,tmp
	cmpobe	0,tmp,rem_t_s1_08		/* J/ INF REM INF -> Inv opn	*/
	b	AFP_NaN_T



/*  S2 is a special case value: +/-0, denormal, +/-INF, NaN; S2 is a non-  */
/*  zero, finite value.                                                    */

rem_t_s2_special:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	shlo	1,s2_mhi,tmp
	or	s2_mlo,tmp,tmp
	bne	rem_t_s2_04			/* J/ S2 is NaN/INF		*/

	cmpobe	0,tmp,rem_t_s1_08		/* J/ finite REM 0 -> inv opn	*/

	bbc	AC_Norm_Mode,ac,rem_t_s1_02	/* J/ denormal, not norm mode	*/

	scanbit	s2_mhi,tmp2
	bno	rem_t_s2_02			/* J/ MS word = 0		*/

	subo	tmp2,31,tmp			/* Left shift to normalize	*/
	addlda(1,tmp2,tmp2)			/* Right shft for wd -> wd xfer	*/
	subo	31,tmp2,s2_exp			/* set s2_exp value		*/

#if	defined(USE_ESHRO)
	eshro	tmp2,s2,s2_mant_hi		/* Normalize denorm significand */
#else
	shlo	tmp,s2_mhi,s2_mant_hi		/* Normalize denorm significand	*/
	shro	tmp2,s2_mlo,tmp
	or	tmp,s2_mant_hi,s2_mant_hi
#endif
	shlo	tmp,s2_mlo,s2_mant_lo
	b	rem_t_s2_rejoin

rem_t_s2_02:
	scanbit	s2_mlo,tmp
	subo	tmp,31,tmp
	shlo	tmp,s2_mlo,s2_mant_hi		/* 32+ bit shift		*/
	subo	tmp,0,s2_exp			/* set s2_exp value		*/
	subo	31,s2_exp,s2_exp
	mov	0,s2_mant_lo
	b	rem_t_s2_rejoin


rem_t_s2_04:
	bbc	31,s2_mhi,rem_t_s1_02		/* J/ INF/NaN w/o j bit		*/

	cmpobe	0,tmp,rem_t_s1_04		/* J/ finite REM INF -> ret S1	*/
	b	AFP_NaN_T			/* J/ process NaN operand */
