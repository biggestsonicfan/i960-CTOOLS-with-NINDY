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
/*      sincosf.c  - Single Precision Sine/Cosine Function (AFP-960)	      */
/*									      */
/******************************************************************************/

#include "asmopt.h"

	.file	"sincosf.s"
	.globl	_sinf,_cosf

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

#define	trig_poly_lo	g6
#define	trig_poly	g7

#define	rp_1_lo		g2
#define	rp_1_hi		g3

#define	rp_2_lo		g4
#define	rp_2_hi		g5

#define	step_shift	g1

#define	out		g0


#define	TRIGF_MAX_EXP	FP_Bias+24

#define	FP_PI_OVER_4	0x3F490FDB	/* FP PI/4	  */



	.text
	.link_pix

	.align	2


/*  Polynomial approximation to sin for the interval  [ -ln(2)/2, ln(2)/2 ]	*/

/*  Constants and algorithms adapted from Hart et al, EXPBC 1321		*/


/*	4/PI = 1.27323 95447 35162 68615 10701 06980		*/

#define	FOUR_OVER_PI_hi		0xA2F9836E
#define	FOUR_OVER_PI_mid	0x4E441529
#define	FOUR_OVER_PI_lo		0xFC2757D2

#define	PI_OVER_FOUR_hi		0xC90FDAA2
#define	PI_OVER_FOUR_mid	0x2168C234
#define	PI_OVER_FOUR_lo		0xC4C6628C


/*
 *  Constants derived from Hart et al SIN 3040
 *  (fixed point fraction representation)
 */
	.set	Lsinf_poly_init,0x00025B26	/*  3.5950439     E-5  */
Lsinf_poly:
	.word	0x00A32F49			/*  2.490001007   E-2  */
	.word	0x14ABBB90			/*  8.0745432524  E-2  */
	.word	0xC90FDA97			/*  7.85398160854 E-1  */
	.set	Lsinf_poly_len,(.-Lsinf_poly) >> 2

/*
 *  Constants derived from Hart et al COS 3821
 *  (fixed point fraction representation)
 */
	.set	Lcosf_poly_init,0x00003B34	/*  3.52876162      E-6  */
Lcosf_poly:
	.word	0x00155C4F			/*  3.2593650043    E-4  */
	.word	0x040F076B			/*  1.585432391225  E-2  */
	.word	0x4EF4F31C			/*  3.0842513489118 E-1  */
	.word	0x00000000			/*  9.9999999994393 E-1  */
	.set	Lcosf_poly_len,(.-Lcosf_poly) >> 2


#if	defined(CA_optim)
	.align	5
#else
	.align	4
#endif

_cosf:
	clrbit	31,s1,s1			/* Zap sign bit of arg		*/
	movlda(1,quadrant)
	b	Ltrigf_00

_sinf:
	mov	0,quadrant			/* Signal SIN function		*/

Ltrigf_00:
	shlo	8,s1,s1_mant			/* Create left-justified mant	*/
	shlo1(s1,tmp_3)				/* Left justify exp field	*/

	setbit	31,s1_mant,s1_mant		/* Set j bit			*/
	lda	FOUR_OVER_PI_hi,con_1

	emul	s1_mant,con_1,rp_1_lo		/* Begin scaling operation	*/

	shro	24,tmp_3,s1_exp			/* Biased, right justif exp	*/
	lda	TRIGF_MAX_EXP,con_1

	cmpo	s1_exp,con_1
	lda	FP_Bias-12,con_1
	bge.f	Ltrigf_80			/* J/ out of range (or NaN)	*/

	cmpo	s1_exp,con_1
	lda	FOUR_OVER_PI_mid,con_1

	emul	s1_mant,con_1,rp_2_lo

	lda	FP_PI_OVER_4 << 1,con_1
	bl.f	Ltrigf_60			/* J/ small/denorm/zero arg	*/

	cmpobl	tmp_3,con_1,Ltrigf_30		/* J/ no reduction		*/

	addc	rp_2_hi,rp_1_lo,rp_1_lo		/* Combine partial products	*/
	lda	FP_Bias+30,con_1

	subo	s1_exp,con_1,tmp_1		/* Compute right shift count	*/
	lda	32,tmp_3

	subo	tmp_1,tmp_3,tmp_3		/* Left shift for fraction	*/

	addc	0,rp_1_hi,rp_1_hi

	chkbit	tmp_1,rp_1_hi			/* Rounding/x_sign bit		*/
	addlda(1,tmp_1,tmp_2)			/* Shift for quadrant number	*/

	shro	tmp_2,rp_1_hi,tmp_2		/* Quadrant number		*/

#if	defined(CA_optim)
	eshro	tmp_1,rp_1_lo,rp_1_hi
#else
	shlo	tmp_3,rp_1_hi,rp_1_hi
	shro	tmp_1,rp_1_lo,tmp_1
	or	rp_1_hi,tmp_1,rp_1_hi
#endif
	shlo	tmp_3,rp_1_lo,rp_1_lo

	alterbit 31,0,x_sign

	addc	quadrant,tmp_2,quadrant

	shri	31,x_sign,x_sign
	xor	rp_1_lo,x_sign,rp_1_lo
	xor	rp_1_hi,x_sign,rp_1_hi

	scanbit	rp_1_hi,tmp_1

	subo	tmp_1,31,x_shift
	addlda(1,tmp_1,tmp_1)

	shlo	x_shift,rp_1_hi,rp_1_hi
	shro	tmp_1,rp_1_lo,tmp_1
	or	rp_1_hi,tmp_1,x_mant

Ltrigf_02:
	emul	x_mant,x_mant,x_sqr_lo

	cmpo	x_shift,12
	lda	Lsinf_poly-.-8(ip),mem_ptr
	bg.f	Ltrigf_40			/* J/ single term approx	*/

	chkbit	0,quadrant
	shlo	1,x_shift,step_shift

	bo	Ltrigf_cos			/* J/ cos evaluation		*/

	movlda(Lsinf_poly_len,tmp_1)
	lda	Lsinf_poly_init,trig_poly

Ltrigf_sin_01:
	emul	x_sqr,trig_poly,trig_poly_lo
	ld	(mem_ptr),con_1
	cmpdeco	1,tmp_1,tmp_1
	addlda(4,mem_ptr,mem_ptr)
	shro	step_shift,trig_poly,trig_poly
	subo	trig_poly,con_1,trig_poly
	bne	Ltrigf_sin_01

	emul	x_mant,trig_poly,trig_poly_lo

Ltrigf_20:
	shro	31,trig_poly,tmp_1
	subo	tmp_1,1,tmp_1
	shlo	tmp_1,trig_poly,trig_poly
	addo	tmp_1,x_shift,x_shift
	lda	FP_Bias-2,s1_exp
	subo	x_shift,s1_exp,s1_exp
	chkbit	7,trig_poly
	shro	8,trig_poly,trig_poly
	shlo	23,s1_exp,s1_exp
	addc	trig_poly,s1_exp,trig_poly
	xor	s1,x_sign,tmp_1
	shlo	30,quadrant,quadrant
	xor	tmp_1,quadrant,tmp_1
	chkbit	31,tmp_1
	alterbit 31,trig_poly,out
	ret


Ltrigf_cos:
	mov	Lcosf_poly_len,tmp_1
	lda	Lcosf_poly_init,trig_poly
	addo	Lcosf_poly-Lsinf_poly,mem_ptr,mem_ptr

Ltrigf_cos_01:
	emul	x_sqr,trig_poly,trig_poly_lo
	ld	(mem_ptr),con_1
	cmpdeco	1,tmp_1,tmp_1
	addlda(4,mem_ptr,mem_ptr)
	shro	step_shift,trig_poly,trig_poly
	subo	trig_poly,con_1,trig_poly
	bne	Ltrigf_cos_01

	mov	0,x_sign
	movlda(0,x_shift)
	b	Ltrigf_20



/*
 *  No reduction step -> set up for normal evaluation
 */

Ltrigf_30:
	chkbit	31,rp_1_hi			/* Check for norm shift		*/
	lda	FP_Bias-2,con_1

	subo	s1_exp,con_1,x_shift		/* Derive shift from arg exp	*/
	movlda(0,x_sign)

	mov	rp_1_hi,x_mant
	be	Ltrigf_02			/* J/ resume flow if norm	*/

	shlo	1,x_mant,x_mant			/* Norm shift			*/
	addlda(1,x_shift,x_shift)
	b	Ltrigf_02


/*
 * Shift count >= 1/2 mantissa bit width -> single or zero term approximation
 */

Ltrigf_40:
	bbs.f	0,quadrant,Ltrigf_45		/* J/ cosine evaluation		*/

	lda	PI_OVER_FOUR_hi,con_1

	emul	x_mant,con_1,trig_poly_lo	/* single term sin conversion	*/

	b	Ltrigf_20

Ltrigf_45:
	shlo	31,1,trig_poly			/* Return 1.0			*/
	subo	1,0,x_shift
	movlda(0,x_sign)
	b	Ltrigf_20


/*
 *  Very small argument -> return arg if sin, return 1.0 if cosine
 */

Ltrigf_60:
	bbs	0,quadrant,Ltrigf_65		/* J/ cosine -> return 1.0	*/

/* ***	mov	s1,out  *** */			/* Return argument		*/
	ret

Ltrigf_65:
	lda	FP_Bias << 23,out		/* Return 1.0			*/
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
