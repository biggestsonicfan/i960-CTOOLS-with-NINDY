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

/******************************************************************************/
/*									      */
/*      tanf.c  - Single Precision Tangent Function (AFP-960)		      */
/*									      */
/******************************************************************************/

#include "asmopt.h"

	.file	"a_tanf.s"
	.globl	_tanf

        .globl  __errno_ptr            /* returns addr of ERRNO in g0          */


#define EDOM    33
#define ERANGE  34

#define	Standard_QNaN   0xffc00000

#define	FP_Bias         0x7f
#define	FP_INF          0xff



/* Register Name Equates */

#define	s1		g0

#define	s1_mant		r3
#define	s1_exp		r4

#define	tmp_1		r5
#define	tmp_2		r6
#define	tmp_3		r7
#define	con_1	 	r8

#define	mem_ptr		r9

#define x_mant		r10
#define	quadrant	r11

#define	x_sqr_lo	r12
#define	x_sqr		r13

#define	x_sign		r14
#define	x_shift		r15

#define	numer_lo	g6
#define	numer_hi	g7

#define	denom_lo	g2
#define	denom_hi	g3

#define	step_shift	g1

#define	out		g0


#define	TRIGF_MAX_EXP	FP_Bias+24

#define	FP_PI_OVER_4	0x3F490FDB	/* FP PI/4	  */



	.text
	.link_pix



/*	4/PI = 1.27323 95447 35162 68615 10701 06980		*/

#define	FOUR_OVER_PI_hi		0xA2F9836E
#define	FOUR_OVER_PI_mid	0x4E44152A

#define	PI_OVER_FOUR_hi		0xC90FDAA2
#define	PI_OVER_FOUR_mid	0x2168C234
#define	PI_OVER_FOUR_lo		0xC4C6628C


/*  Polynomial approximation to tan for the interval  [ -PI/4, +PI/4 ]	*/

/*
 *  Constants derived from Hart et al TAN 4282
 *  (commonly aligned fixed point representation, maximum value left justified)
 */
	.set	LP3,0x0646D273	/*  1.255329742424  E+1  */
	.set	LP1,0x6A3654A0	/*  2.1242445758263 E+2  */

	.set	LQ4,0x00800000	/*  1.              E+0  */
	.set	LQ2,0x23CC4BB6	/*  7.159606050466  E+1  */
	.set	LQ0,0x873BCDFB	/*  2.7046722349399 E+2  */



#if	defined(CA_optim)
	.align	5
#else
	.align	4
#endif

_tanf:
	shlo	8,s1,s1_mant			/* Create left-justified mant	*/
	shlo1(s1,tmp_3)				/* Left justify exp field	*/

	setbit	31,s1_mant,s1_mant		/* Set j bit			*/
	lda	FOUR_OVER_PI_hi,con_1

	emul	s1_mant,con_1,numer_lo		/* Begin scaling operation	*/

	shro	24,tmp_3,s1_exp			/* Biased, right justif exp	*/
	lda	TRIGF_MAX_EXP,con_1

	cmpo	s1_exp,con_1
	bge.f	Ltrigf_80			/* J/ out of range (or NaN)	*/

	lda	FP_Bias-13,con_1

	cmpo	s1_exp,con_1
	lda	FOUR_OVER_PI_mid,con_1

	emul	s1_mant,con_1,denom_lo

	mov	0,quadrant			/* Default to std tan		*/
	ble	Ltrigf_60			/* J/ small/denorm/zero arg	*/

	addc	denom_hi,numer_lo,numer_lo	/* Combine partial products	*/
	lda	FP_PI_OVER_4 << 1,con_1

	addc	0,numer_hi,numer_hi

	cmpo	tmp_3,con_1
	lda	FP_Bias+30,con_1

	subo	s1_exp,con_1,tmp_1		/* Compute right shift count	*/
	bl	Ltrigf_30			/* J/ no reduction		*/

	lda	32,tmp_3

	subo	tmp_1,tmp_3,tmp_3		/* Left shift for fraction	*/

	chkbit	tmp_1,numer_hi			/* Rounding/x_sign bit		*/
	addlda(1,tmp_1,tmp_2)			/* Shift for quadrant number	*/

	shro	tmp_2,numer_hi,tmp_2		/* Quadrant number		*/

#if	defined(CA_optim)
	eshro	tmp_1,numer_lo,numer_hi
#else
	shlo	tmp_3,numer_hi,numer_hi
	shro	tmp_1,numer_lo,tmp_1
	or	numer_hi,tmp_1,numer_hi
#endif
	shlo	tmp_3,numer_lo,numer_lo

	alterbit 31,0,x_sign

	addc	quadrant,tmp_2,quadrant

	shri	31,x_sign,x_sign
	xor	numer_lo,x_sign,numer_lo
	xor	numer_hi,x_sign,numer_hi

	scanbit	numer_hi,tmp_1

	subo	tmp_1,31,x_shift
	addlda(1,tmp_1,tmp_1)

	shlo	x_shift,numer_hi,numer_hi
	shro	tmp_1,numer_lo,tmp_1
	or	numer_hi,tmp_1,x_mant

Ltrigf_02:
	cmpo	x_shift,12

	emul	x_mant,x_mant,x_sqr_lo

	bg.f	Ltrigf_40			/* J/ single term approx	*/

	shlo	1,x_shift,step_shift
	lda	LP3,con_1

	emul	x_sqr,con_1,numer_lo		/* LP3 * x^2			*/

	shro	9,x_sqr,denom_hi		/* LQ4 * x^2			*/
	shro	step_shift,denom_hi,denom_hi

	lda	LQ2,con_1

	subo	denom_hi,con_1,denom_hi		/* LQ2 - LQ4 * x^2		*/

	emul	x_sqr,denom_hi,denom_lo		/* LQ2 * x^2 - LQ4 * x^4		*/

	shro	step_shift,numer_hi,numer_hi
	lda	LP1,con_1

	subo	numer_hi,con_1,numer_hi		/* LP1 - LP3 * x^2		*/

	emul	x_mant,numer_hi,numer_lo

	chkbit	0,quadrant
	lda	LQ0,con_1

	shro	step_shift,denom_hi,denom_hi

	subo	denom_hi,con_1,denom_hi

	bo	Ltrigf_15			/* J/ inv tan evaluation	*/

	ediv	denom_hi,numer_lo,numer_lo
	b	Ltrigf_20


Ltrigf_14:					/* Division ovfl prevention	*/
	shro	1,denom_hi,denom_hi
	ediv	numer_hi,denom_lo,numer_lo

	addo	1,tmp_1,tmp_1
	b	Ltrigf_17

Ltrigf_15:
	scanbit	numer_hi,tmp_1			/* Left norm numer to ...	*/
	subo	tmp_1,31,tmp_1
	shlo	tmp_1,numer_hi,numer_hi		/* ... prevent ediv overflow	*/

	cmpobge.f denom_hi,numer_hi,Ltrigf_14	/* J/ must also shift denom	*/

	ediv	numer_hi,denom_lo,numer_lo	/* Inverse tan evaluation	*/

Ltrigf_17:
	subo	x_shift,0,x_shift		/* Invert result exponent	*/
	subo	tmp_1,x_shift,x_shift		/* Account for norm shift	*/

Ltrigf_20:
	shro	31,numer_hi,tmp_1		/* Normalize as req'd		*/
	subo	tmp_1,1,tmp_1
	shlo	tmp_1,numer_hi,numer_hi
	addo	tmp_1,x_shift,x_shift
	lda	FP_Bias-2,s1_exp		/* Result exponent from x_shift	*/
	subo	x_shift,s1_exp,s1_exp
	chkbit	7,numer_hi			/* Rounding bit			*/
	shro	8,numer_hi,numer_hi
	shlo	23,s1_exp,s1_exp
	addc	numer_hi,s1_exp,numer_hi	/* Round, combine exp w/ mant	*/
	xor	s1,x_sign,tmp_1			/* Compute result sign		*/
	shlo	31,quadrant,quadrant
	xor	tmp_1,quadrant,tmp_1
	chkbit	31,tmp_1
	alterbit 31,numer_hi,out		/* Set sign bit, move result	*/
	ret


/*
 *  No reduction step -> set up for normal evaluation
 */

Ltrigf_30:
	chkbit	31,numer_hi			/* Check for norm shift		*/
	lda	FP_Bias-2,con_1

	subo	s1_exp,con_1,x_shift		/* Derive shift from arg exp	*/
	movlda(0,x_sign)

	mov	numer_hi,x_mant
	bo	Ltrigf_02			/* J/ resume flow if norm	*/

	shlo	1,x_mant,x_mant			/* Norm shift			*/
	addlda(1,x_shift,x_shift)
	b	Ltrigf_02


/*
 * Shift count >= 1/2 mantissa bit width -> single or zero term approximation
 */

Ltrigf_40:
	lda	PI_OVER_FOUR_hi,con_1

	emul	x_mant,con_1,numer_lo		/* single term tan conversion	*/

	bbs.f	0,quadrant,Ltrigf_45		/* J/ inv tan evaluation		*/

	b	Ltrigf_20

Ltrigf_45:
	shlo	30,1,denom_hi			/* Invert the value		*/
	movlda(0,denom_lo)

	ediv	numer_hi,denom_lo,numer_lo

	subo	x_shift,0,x_shift
	subo	2,x_shift,x_shift

	b	Ltrigf_20



/*
 *  Very small argument -> return arg
 */

Ltrigf_60:
/* ***	mov	s1,out  *** */			/* Return argument		*/
	ret



/*
 *  NaN or too large
 */

Ltrigf_80:
	mov	s1,r4				/* Save incoming value		*/

	callj	__errno_ptr			/* returns addr of ERRNO in g0	*/
	ldconst	EDOM,r8
	st	r8,(g0)

	mov	r4,out				/* Default -> return arg	*/

	shlo	1,out,tmp_3

	lda	FP_INF << 24,tmp_1

	cmpobg.f tmp_3,tmp_1,Ltrigf_82		/* J/ NaN argument		*/

	mov	0,out				/* Return 0.0			*/

	bne.t	Ltrigf_84			/* J/ finite argument		*/

Ltrigf_82:
	lda	Standard_QNaN,tmp_1		/* Force Quiet NaN		*/

	or	out,tmp_1,out

Ltrigf_84:	
	ret
