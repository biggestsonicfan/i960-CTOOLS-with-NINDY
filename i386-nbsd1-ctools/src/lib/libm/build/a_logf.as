/*******************************************************************************
 * 
 * Copyright (c) 1993 Intel Corporation
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


#include "asmopt.h"

	.file	"a_logf.s"
	.globl	_logf

        .globl  __errno_ptr            /* returns addr of ERRNO in g0          */


#define EDOM    33
#define ERANGE  34

#define	FP_Bias         0x7f
#define	FP_INF          0xff


/* Register Name Equates */

#define	s1		g0

#define	s1_mant		r3
#define	s1_exp		r4
#define	s1_flags	r5

#define	tmp_1		r6
#define	tmp_2		r7
#define	tmp_3		r8
#define	con_1	 	r9

#define z_mant		r10
#define	z_exp		r11

#define	z_sqr_lo	r12
#define	z_sqr		r13

#define	numer_lo	r14
#define	numer		r15

#define	denom_lo	g6
#define	denom		g7

#define	result_se	g5

#define	z_shift		g4
#define	step_shift	g3

#define	out		g0



	.text
	.link_pix


/*  Polynomial approximation to LOGe for the interval  [ 1.0, sqrt(2) ]	*/

/*  Constants and algorithms proprietary to Kulus Research, Portland OR  */
/*  Constants and algorithms non-exclusively licensed to Intel Corp	 */


#define	P1		0x22981142
#define	P0		0x80000085

#define	Q1		0x9A844BC4
#define	Q0		0x100000000

#define	SQRT_2_NO_j	0x6A09E600

#define	ln_2_hi		0xB17217F7
#define	ln_2_lo		0xD1CF79AC


/*  (FP_Bias+22) * ln 2  */

#define	bias_int	0x00000067
#define	bias_frac	0x4767F33D


#define	FP_NEG_INF	0xFF800000
#define	FP_QNaN		0xFFC00000


#if	defined(CA_optim)
	.align	5
#else
	.align	4
#endif

_logf:
	shlo	9,s1,numer			/* Create left-justified mant	*/
	shlo1(s1,s1_exp)			/* Left justify exp field	*/

	shri	24,s1_exp,tmp_1			/* Special value testing	*/
	lda	SQRT_2_NO_j,con_1

	shro	24,s1_exp,s1_exp		/* Biased, right justif exp	*/
	addlda(1,tmp_1,tmp_1)			/* Special value testing	*/

	cmpobge.f 1,tmp_1,Llogf_80		/* J/ NaN/INF/denorm/0.0	*/

Llogf_10:
	bbs.f	31,s1,Llogf_90			/* J/ sign bit set		*/

	cmpo	numer,con_1
	movlda(0,s1_flags)

	shro	2,numer,denom
	bl.f	Llogf_12				/* J/ < sqrt(2)			*/

	subo	numer,0,numer			/* Compute numer = 1 - mant	*/
	movlda(1,s1_flags)			/* Signal >= sqrt(2)		*/

	setbit	30,denom,denom			/* 1 + mant frac		*/
	addlda(1,s1_exp,s1_exp)


Llogf_12:
	setbit	31,denom,denom			/* denom = 2 + mant		*/

	scanbit	numer,tmp_1

	subo	tmp_1,31,tmp_1			/* Shift for bit 31 norm	*/
	lda	0,numer_lo

	shlo	tmp_1,numer,numer		/* Left norm the numerator	*/
	addlda(2,tmp_1,z_shift)

	cmpobl.f numer,denom,Llogf_16		/* J/ no divide overflow	*/

	shro	1,numer,numer			/* Prevent division overflow	*/
	addlda(1,tmp_1,z_shift)

Llogf_16:
	ediv	denom,numer_lo,numer_lo		/* Compute "z"			*/

	setbit	31,0,z_mant			/* In case no poly needed	*/

	cmpoble.f 12,z_shift,Llogf_17		/* J/ no poly approx needed	*/

	lda	P0,tmp_1			/* Polynomial constants		*/
	lda	P1,tmp_2
	lda	Q1,tmp_3

	emul	numer,numer,z_sqr_lo		/* Compute z^2			*/

	shlo	1,z_shift,step_shift
	movldar(numer,z_mant)

	emul	z_sqr,tmp_2,numer_lo		/* Compute P1 * z^2		*/

	emul	z_sqr,tmp_3,denom_lo		/* Compute Q1 * z^2		*/

	shro	step_shift,numer,numer		/* Align terms			*/
	subo	numer,tmp_1,numer		/* P0 - P1 * z^2		*/

	movlda(0,numer_lo)

	shro	step_shift,denom,denom		/* Align terms			*/
	subo	denom,0,denom			/* Q0 - Q1 * z^2  (Q0 = 1.0)	*/

	ediv	denom,numer_lo,numer_lo		/* P-poly / Q-poly		*/

Llogf_17:
	emul	numer,z_mant,numer_lo		/* Compute approximation	*/

	lda	FP_Bias,tmp_1
	cmpobne.t s1_exp,tmp_1,Llogf_20		/* J/ some ln 2 scaling		*/

	lda	FP_Bias-1+2,tmp_1		/* Compute exp of approx	*/
	subo	z_shift,tmp_1,s1_exp

	bbs.f	31,numer,Llogf_18		/* J/ normalized result		*/

	cmpo	numer,0				/* Check for log of 1.0		*/
	shlo1(numer,numer)

	subo	1,s1_exp,s1_exp			/* Left shift to normalize	*/
	be.f	Llogf_19				/* J/ log of 1.0 -> 0.0		*/

Llogf_18:
	chkbit	7,numer				/* Rounding bit to carry	*/
	shlo1(numer,numer)			/* Drop j bit			*/

	shro	9,numer,numer			/* Right just mant bits		*/
	shlo	23,s1_exp,s1_exp		/* Position exponent		*/
	addc	s1_exp,numer,numer		/* Round, combine exp w/ mant	*/

Llogf_19:					/* for log of 1.0		*/
	chkbit	0,s1_flags
	alterbit 31,numer,out			/* Set sign bit, move result	*/
	ret

/*
 * Log w/ ln 2 factors to be combined
 */

Llogf_20:
	lda	22(s1_exp),s1_exp		/* Insure s1_exp is not neg	*/

	lda	ln_2_hi,tmp_1			/* Compute biased ln 2 factor	*/
	emul	tmp_1,s1_exp,denom_lo

	subo	s1_flags,0,s1_flags		/* 0 if positive, -1 if neg	*/
	lda	bias_int,tmp_2			/* To remove DP_Bias factors	*/

	subo	2,z_shift,z_shift

	shro	z_shift,numer,numer		/* Align poly results		*/
	lda	bias_frac,tmp_3

	xor	s1_flags,numer,numer		/* Apply signed to poly rslts	*/
	lda	ln_2_lo,tmp_1			/* Eight lsb's of biased adj	*/

	cmpo	0,0				/* Clear borrow			*/

	emul	tmp_1,s1_exp,z_sqr_lo		/* Lo-order biasing bits	*/

	subc	tmp_3,denom_lo,denom_lo		/* Remove bias			*/
	subc	tmp_2,denom,denom

	cmpo	1,0				/* Clear carry bit		*/
	addc	numer,denom_lo,denom_lo		/* Add in poly results		*/
	addc	s1_flags,denom,denom

	cmpo	1,0				/* Clear carry bit		*/
	addc	z_sqr,denom_lo,denom_lo
	addc	0,denom,denom

	shri	31,denom,con_1			/* Absolute value of result	*/
	xor	denom_lo,con_1,denom_lo
	xor	denom,con_1,denom

	addc	denom_lo,denom_lo,denom_lo	/* Position for scanbit		*/
	addc	denom,denom,denom

	scanbit	denom,tmp_1

	addo	32-24,tmp_1,tmp_2		/* "Right" shift for norm	*/

#if	defined(CA_optim)
	eshro	tmp_2,denom_lo,denom
#else
	shro	tmp_2,denom_lo,denom_lo
	subo	tmp_1,24,tmp_2			/* Left shift for bit 24 norm	*/
	shlo	tmp_2,denom,denom
	or	denom_lo,denom,denom
#endif
	lda	FP_Bias-1(tmp_1),s1_exp		/* Result exponent		*/

	chkbit	0,denom				/* Rounding bit			*/
	shro	1,denom,denom			/* Position mantissa		*/
	clrbit	23,denom,denom			/* Zap j bit			*/
	shlo	23,s1_exp,s1_exp		/* Position exponent		*/
	addc	denom,s1_exp,denom		/* Round, combine exp and mant	*/
	shlo	31,con_1,con_1			/* Sign bit			*/
	or	denom,con_1,out			/* Mix in sign bit, mov		*/
	ret


/*
 *  NaN/INF/denorm/0.0
 */

Llogf_80:
	be	Llogf_85				/* J/ denorm/0.0		*/

	cmpobne.f 0,numer,Llogf_82		/* J/ NaN			*/

	bbs.f	31,s1,Llogf_90			/* J/ -INF -> negative value	*/

/*
 *  logf(+INF) -> +INF w/ ERANGE
 */

	mov	g0,r4				/* Save +INF value		*/

        callj   __errno_ptr                    /* returns addr of ERRNO in g0  */
        ldconst ERANGE,r8
        st      r8,(g0)

	mov	r4,g0				/* logf(+INF) -> +INF		*/

	ret					/* logf(+INF) -> +INF		*/


/*
 *  logf(NaN) -> QNaN w/ EDOM
 */

Llogf_82:					/* NaN argument			*/
	mov	g0,r4				/* Save incoming value		*/

	callj	__errno_ptr			/* returns addr of ERRNO in g0	*/
	ldconst	EDOM,r8
	st	r8,(g0)

	mov	r4,out				/* Restore incoming value	*/
	lda	FP_QNaN,tmp_1

	or	out,tmp_1,out			/* Force QNaN			*/

	ret


Llogf_85:
	scanbit	numer,tmp_1
	bno	Llogf_88				/* J/ mantissa is zero		*/

	subo	31,tmp_1,s1_exp			/* Compute effective exponent	*/

	subo	s1_exp,1,tmp_2			/* Left shift (w/ MS bit drop)	*/

	shlo	tmp_2,numer,numer		/* Left just w/ MS bit drop	*/
	b	Llogf_10


Llogf_88:					/* logf(0) -> -INF w/ ERANGE	*/
	callj	__errno_ptr			/* returns addr of ERRNO in g0	*/
	ldconst	ERANGE,r8
	st	r8,(g0)

	lda	FP_NEG_INF,out			/* Return -INF			*/
	ret


/*
 *  Negative (non-zero) numbers handled here:  return QNaN w/ EDOM
 */

Llogf_90:
	callj	__errno_ptr			/* returns addr of ERRNO in g0	*/
	ldconst	EDOM,r8
	st	r8,(g0)

	lda	FP_QNaN,out			/* QNaN				*/

	ret
