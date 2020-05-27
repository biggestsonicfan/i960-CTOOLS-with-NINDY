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
/*      tan.c  - Double Precision Tangent Function (AFP-960)		      */
/*									      */
/******************************************************************************/

#include "asmopt.h"

	.file	"a_tan.s"
	.globl	_tan

        .globl  __errno_ptr            /* returns addr of ERRNO in g0          */


#define EDOM    33
#define ERANGE  34

#define	Standard_QNaN_hi  0xfff80000
#define	Standard_QNaN_lo  0x00000000

#define	DP_Bias		0x3Ff
#define	DP_INF		0x7ff



/* Register Name Equates */

#define	s1_lo		g0
#define	s1_hi		g1

#define	s1_exp		r3
#define	s1_mant_lo	r4
#define	s1_mant_hi	r5

#define x_mant_lo	s1_mant_lo
#define	x_mant_hi	s1_mant_hi

#define	x_sqr_lo	r6
#define	x_sqr_hi	r7

#define	con_lo	 	r8
#define	con_hi	 	r9

#define	rp_1_lo		r10
#define	rp_1_hi		r11

#define	numer_poly_lo	r12
#define	numer_poly_hi	r13

#define	denom_poly_lo	r14
#define	denom_poly_hi	r15

#define	quadrant	g2
#define	mem_ptr		g3

#define	tmp_1		g4
#define	tmp_2		g5

#define	rp_2_lo		g6
#define	rp_2_hi		g7

#define	out_lo		g0
#define	out_hi		g1


/*  Overlapping register declarations  */

#define	shift_cnt	s1_exp

#define	x_mant_ext	s1_lo

#define	hold_reg_1	x_sqr_lo
#define	hold_reg_2	x_sqr_hi



#define	TRIG_MAX_EXP	DP_Bias+24

#define	DP_PI_OVER_4_hi	0x3FE921FB	/* DP PI/4	  */
#define	DP_PI_OVER_4_lo	0x54442D18



	.text
	.link_pix

	.align	3



/*	4/PI = 1.27323 95447 35162 68615 10701 06980		*/

#define	USE_66_BIT_PI	1
#if	defined(USE_66_BIT_PI)
#define	FOUR_OVER_PI_hi		0xA2F9836E
#define	FOUR_OVER_PI_mid	0x4E44152A
#define	FOUR_OVER_PI_lo		0x00062BCE

#define	PI_OVER_FOUR_hi		0xC90FDAA2
#define	PI_OVER_FOUR_mid	0x2168C234
#define	PI_OVER_FOUR_lo		0xC0000000
#else
#define	FOUR_OVER_PI_hi		0xA2F9836E
#define	FOUR_OVER_PI_mid	0x4E441529
#define	FOUR_OVER_PI_lo		0xFC2757D2

#define	PI_OVER_FOUR_hi		0xC90FDAA2
#define	PI_OVER_FOUR_mid	0x2168C234
#define	PI_OVER_FOUR_lo		0xC4C6628C
#endif


/*
 *  Special two term evaluation coefficients
 */

#define	C3_hi	0x295779CC

#define	C1_hi	PI_OVER_FOUR_hi
#define	C1_lo	PI_OVER_FOUR_mid


/*
 *  Constants derived from Hart et al TAN 4286
 *  (commonly aligned fixed point representation, maximum value left justified)
 */

LP_poly:
	.word	0x39CC79BA,0x00002DA6	/* P7  4.5649319438665631873961137   E+1  */
	.word	0xB04F1A5B,0x00376DDA	/* P5  1.418985425276177838800394831 E+4  */
	.word	0xA8468764,0x0DAB9070	/* P3  8.958884400676804108729639541 E+5  */
	.word	0xF1B1530B,0xA625986F	/* P1  1.088860043728168752138857983 E+7  */
	.set	LP_poly_len,(.-LP_poly) >> 3

LQ_poly:
	.word	0x00000000,0x00000100	/* Q8  1.                            E+0  */
	.word	0xFC159C19,0x0003F6A7	/* Q6  1.014656190252885338754401947 E+3  */
	.word	0x7A6572EE,0x0210D6B6	/* Q4  1.353827128051190938289294872 E+5  */
	.word	0x9DF3DC80,0x3CE70D84	/* Q2  3.99130951803516515044342794  E+6  */
	.word	0xDF9171F3,0xD38B74A9	/* Q0  1.386379666356762916533913361 E+7  */
	.set	LQ_poly_len,(.-LQ_poly) >> 3


#if	defined(CA_optim)
	.align	5
#else
	.align	4
#endif

_tan:
/*
 *  Create left-justified mant (w/ j bit) in s1_mant_hi/_lo
 */

#if	defined(CA_optim)
	eshro	32-11,s1_lo,s1_mant_hi
#else
	shlo	11,s1_hi,s1_mant_hi
	shro	32-11,s1_lo,tmp_1
	or	s1_mant_hi,tmp_1,s1_mant_hi
#endif
	setbit	31,s1_mant_hi,s1_mant_hi	/* Set j bit			*/

/*
 *  94+ bit scaling for argument reduction
 */

	lda	FOUR_OVER_PI_lo,con_lo

	emul	s1_mant_hi,con_lo,rp_1_lo

	shlo	11,s1_lo,s1_mant_lo
	lda	FOUR_OVER_PI_mid,con_lo

	shlo	1,s1_hi,hold_reg_1		/* Left justify exp field	*/

	emul	s1_mant_lo,con_lo,rp_2_lo

	shro	32-11,hold_reg_1,s1_exp		/* Biased, right justif exp	*/
	movldar(s1_lo,hold_reg_2)		/* Save s1_lo (x_mant_ext ovlp)	*/

	lda	TRIG_MAX_EXP,con_hi
	cmpobge.f s1_exp,con_hi,Ltrig_80		/* J/ out of range (or NaN)	*/

	emul	s1_mant_hi,con_lo,denom_poly_lo

	lda	DP_Bias-28,con_hi
	cmpoble.f s1_exp,con_hi,Ltrig_60		/* J/ small/denorm/zero arg	*/

	lda	FOUR_OVER_PI_hi,con_lo

	emul	s1_mant_lo,con_lo,numer_poly_lo

	addc	rp_1_lo,rp_2_lo,rp_1_lo
	addc	rp_1_hi,rp_2_hi,rp_1_lo
	addc	0,0,rp_1_hi
	addc	rp_1_lo,denom_poly_lo,rp_1_lo

	emul	s1_mant_hi,con_lo,x_mant_lo

	addc	rp_1_hi,denom_poly_hi,rp_1_hi
	movlda(0,quadrant)
	addc	rp_1_lo,numer_poly_lo,x_mant_ext
	addc	rp_1_hi,numer_poly_hi,rp_1_lo
	addc	0,0,rp_1_hi

	addc	rp_1_lo,x_mant_lo,x_mant_lo
	addc	rp_1_hi,x_mant_hi,x_mant_hi


	lda	DP_PI_OVER_4_hi << 1,con_hi
	cmpoble.f hold_reg_1,con_hi,Ltrig_30	/* J/ no argument reduction	*/

Ltrig_01:
	lda	DP_Bias+30,con_lo

	subo	s1_exp,con_lo,tmp_1		/* Compute right shift count	*/
	lda	32,shift_cnt

	subo	tmp_1,shift_cnt,shift_cnt	/* Left shift for fraction	*/

	chkbit	tmp_1,x_mant_hi			/* Rounding/sign bit		*/
	addlda(1,tmp_1,tmp_2)			/* Shift for quadrant number	*/

	shro	tmp_2,x_mant_hi,tmp_2		/* Quadrant number		*/

#if	defined(CA_optim)
	eshro	tmp_1,x_mant_lo,x_mant_hi
#else
	shlo	shift_cnt,x_mant_hi,x_mant_hi
	shro	tmp_1,x_mant_lo,con_hi
	or	x_mant_hi,con_hi,x_mant_hi
#endif
	shlo	shift_cnt,x_mant_lo,x_mant_lo
	shro	tmp_1,x_mant_ext,con_lo
	or	x_mant_lo,con_lo,x_mant_lo

	shlo	shift_cnt,x_mant_ext,x_mant_ext

	alterbit 31,0,tmp_1

	xor	s1_hi,tmp_1,s1_hi		/* Flip arg sign as req'd	*/

	addc	quadrant,tmp_2,quadrant

	shri	31,tmp_1,tmp_1
	xor	x_mant_ext,tmp_1,x_mant_ext
	xor	x_mant_lo,tmp_1,x_mant_lo
	xor	x_mant_hi,tmp_1,x_mant_hi

	scanbit	x_mant_hi,tmp_1

	subo	tmp_1,31,shift_cnt
	addlda(1,tmp_1,tmp_1)

	cmpibg.f 6,tmp_1,Ltrig_40		/* J/ single term approx	*/

	shlo	shift_cnt,x_mant_hi,x_mant_hi
	shro	tmp_1,x_mant_lo,con_hi
	or	x_mant_hi,con_hi,x_mant_hi

	shlo	shift_cnt,x_mant_lo,x_mant_lo
	shro	tmp_1,x_mant_ext,con_lo
	or	x_mant_lo,con_lo,x_mant_lo

Ltrig_02:
	emul	x_mant_lo,x_mant_hi,rp_1_lo

	mov	LP_poly_len,tmp_1
	lda	LP_poly-.-8(ip),mem_ptr

	ldl	(mem_ptr),numer_poly_lo

	emul	x_mant_hi,x_mant_hi,x_sqr_lo

	addc	rp_1_lo,rp_1_lo,rp_1_lo

	addc	rp_1_hi,rp_1_hi,rp_1_lo
	addlda(8,mem_ptr,mem_ptr)

	addc	0,0,rp_1_hi

	addc	rp_1_lo,x_sqr_lo,x_sqr_lo
	addc	rp_1_hi,x_sqr_hi,x_sqr_hi

	shlo	1,shift_cnt,shift_cnt

	cmpobl.f 31,shift_cnt,Ltrig_50		/* J/ >= one word shift		*/

#if	!defined(CA_optim)
	lda	32,tmp_2
	subo	shift_cnt,tmp_2,tmp_2
#endif

Ltrig_04:
	emul	x_sqr_lo,numer_poly_hi,rp_1_lo
	ldl	(mem_ptr),con_lo
	addo	8,mem_ptr,mem_ptr
	emul	x_sqr_hi,numer_poly_lo,rp_2_lo

	subo	1,tmp_1,tmp_1

	emul	x_sqr_hi,numer_poly_hi,numer_poly_lo

	addc	rp_1_lo,rp_2_lo,rp_1_lo
	addc	rp_1_hi,rp_2_hi,rp_1_lo
	addc	0,0,rp_1_hi

	addc	rp_1_lo,numer_poly_lo,numer_poly_lo
	addc	rp_1_hi,numer_poly_hi,numer_poly_hi

#if	defined(CA_optim)
	eshro	shift_cnt,numer_poly_lo,numer_poly_lo
#else
	shro	shift_cnt,numer_poly_lo,numer_poly_lo
	shlo	tmp_2,numer_poly_hi,rp_1_lo
	or	numer_poly_lo,rp_1_lo,numer_poly_lo
#endif
	shro	shift_cnt,numer_poly_hi,numer_poly_hi

	subc	numer_poly_lo,con_lo,numer_poly_lo
	subc	numer_poly_hi,con_hi,numer_poly_hi

	cmpobne	1,tmp_1,Ltrig_04

	emul	x_mant_lo,numer_poly_hi,rp_1_lo
	emul	x_mant_hi,numer_poly_lo,rp_2_lo
	emul	x_mant_hi,numer_poly_hi,numer_poly_lo

	addc	rp_1_lo,rp_2_lo,rp_1_lo
	lda	8(mem_ptr),mem_ptr		/* Skip Q8 coefficient		*/
	addc	rp_1_hi,rp_2_hi,rp_1_lo
	ldl	(mem_ptr),con_lo		/* Fetch Q6 coefficient		*/
	addc	0,0,rp_1_hi
	addlda(8,mem_ptr,mem_ptr)

	addc	rp_1_lo,numer_poly_lo,numer_poly_lo
	addc	rp_1_hi,numer_poly_hi,numer_poly_hi


/*
 *  Use shifts to "multiply" by Q8
 */

#if	defined(CA_optim)
	eshro	24,x_sqr_lo,denom_poly_lo
#else
	shro	24,x_sqr_lo,denom_poly_lo
	shlo	32-24,x_sqr_hi,tmp_1
	or	denom_poly_lo,tmp_1,denom_poly_lo
#endif
	shro	24,x_sqr_hi,denom_poly_hi

#if	defined(CA_optim)
	eshro	shift_cnt,denom_poly_lo,denom_poly_lo
#else
	shro	shift_cnt,denom_poly_lo,denom_poly_lo
	shlo	tmp_2,denom_poly_hi,rp_1_lo
	or	denom_poly_lo,rp_1_lo,denom_poly_lo
#endif
	shro	shift_cnt,denom_poly_hi,denom_poly_hi

	subc	denom_poly_lo,con_lo,denom_poly_lo
	subc	denom_poly_hi,con_hi,denom_poly_hi

	movlda(LQ_poly_len-1,tmp_1)

Ltrig_06:
	emul	x_sqr_lo,denom_poly_hi,rp_1_lo
	ldl	(mem_ptr),con_lo
	addo	8,mem_ptr,mem_ptr
	emul	x_sqr_hi,denom_poly_lo,rp_2_lo

	subo	1,tmp_1,tmp_1

	emul	x_sqr_hi,denom_poly_hi,denom_poly_lo

	addc	rp_1_lo,rp_2_lo,rp_1_lo
	addc	rp_1_hi,rp_2_hi,rp_1_lo
	addc	0,0,rp_1_hi

	addc	rp_1_lo,denom_poly_lo,denom_poly_lo
	addc	rp_1_hi,denom_poly_hi,denom_poly_hi

#if	defined(CA_optim)
	eshro	shift_cnt,denom_poly_lo,denom_poly_lo
#else
	shro	shift_cnt,denom_poly_lo,denom_poly_lo
	shlo	tmp_2,denom_poly_hi,rp_1_lo
	or	denom_poly_lo,rp_1_lo,denom_poly_lo
#endif
	shro	shift_cnt,denom_poly_hi,denom_poly_hi

	subc	denom_poly_lo,con_lo,denom_poly_lo
	subc	denom_poly_hi,con_hi,denom_poly_hi

	cmpobne	1,tmp_1,Ltrig_06

	shro	1,shift_cnt,shift_cnt

	bbc.t	0,quadrant,Ltrig_22		/* J/ standard tan evaluation	*/

/*
 *  Inverted tangent evaluation
 *    1. exchange denom and numer values
 *    2. shift "new" numer right to prevent division overflow
 *    3. norm "new" denom as needed, also to prevent division overflow
 *    4. compute new shift_cnt, ultimately for result exp computation
 */

#if	defined(CA_optim)
	eshro	1,denom_poly_lo,rp_1_lo
#else
	shro	1,denom_poly_lo,rp_1_lo
	shlo	32-1,denom_poly_hi,tmp_1
	or	rp_1_lo,tmp_1,rp_1_lo
#endif
	shro	1,denom_poly_hi,rp_1_hi

Ltrig_21:
	shro	31,numer_poly_hi,tmp_1		/* Left norm numer		*/

	subo	tmp_1,1,tmp_2			/* Norm left shift count	*/
	addlda(31,tmp_1,tmp_1)			/* wd->wd norm shift count	*/

	shlo	tmp_2,numer_poly_hi,denom_poly_hi
	shro	tmp_1,numer_poly_lo,tmp_1
	or	denom_poly_hi,tmp_1,denom_poly_hi

	shlo	tmp_2,numer_poly_lo,denom_poly_lo
	addlda(1,shift_cnt,shift_cnt)		/* Exp reduct for right shift	*/

	subo	shift_cnt,0,shift_cnt
	movldar(rp_1_lo,numer_poly_lo)
	subo	tmp_2,shift_cnt,shift_cnt	/* Exp incr for left shift	*/
	movldar(rp_1_hi,numer_poly_hi)


/*
 *  Compute tangent result by division, normalize, and pack for return
 */

Ltrig_22:
	cmpobg.t denom_poly_hi,numer_poly_hi,Ltrig_22a	/* J/ no div ovfl	*/

#if	defined(CA_optim)
	eshro	1,numer_poly_lo,numer_poly_lo		/* Shift to avoid ovfl	*/
#else
	shlo	31,numer_poly_hi,tmp_1			/* Shift to avoid ovfl	*/
	shro	1,numer_poly_lo,numer_poly_lo
	or	tmp_1,numer_poly_lo,numer_poly_lo
#endif
	shro	1,numer_poly_hi,numer_poly_hi

	subo	1,shift_cnt,shift_cnt			/* Bump exponent	*/

Ltrig_22a:
	ediv	denom_poly_hi,numer_poly_lo,numer_poly_lo
	mov	numer_poly_hi,tmp_1			/* Division result	*/
	movldar(numer_poly_lo,numer_poly_hi)		/* Next division step	*/
    /*	mov	0,numer_poly_lo	 */			/* ignore lsb of rslt	*/
	ediv	denom_poly_hi,numer_poly_lo,numer_poly_lo
	mov	numer_poly_hi,numer_poly_lo		/* 64-bit result	*/
	movldar(tmp_1,numer_poly_hi)

	shro	1,denom_poly_lo,rp_1_hi			/* Set up for C/B div	*/
	shlo	31,denom_poly_lo,rp_1_lo
	ediv	denom_poly_hi,rp_1_lo,rp_1_lo

	emul	rp_1_hi,numer_poly_hi,rp_1_lo		/* C/B * A/B corr term	*/

	addc	rp_1_lo,rp_1_lo,rp_1_lo			/* align result		*/
	addc	rp_1_hi,rp_1_hi,rp_1_lo
	addc	0,0,rp_1_hi

	cmpo	0,0					/* Clear borrow bit 	*/
	subc	rp_1_lo,numer_poly_lo,numer_poly_lo	/* A/B - (C/B * A/B)	*/
	subc	rp_1_hi,numer_poly_hi,numer_poly_hi

Ltrig_23:
	bbs.t	31,numer_poly_hi,Ltrig_24		/* J/ normalized	*/

	addc	numer_poly_lo,numer_poly_lo,numer_poly_lo
	addlda(1,shift_cnt,shift_cnt)			/* Reduce result exp	*/
	addc	numer_poly_hi,numer_poly_hi,numer_poly_hi

Ltrig_24:

/*
 *  Pack result
 *    1. Extract rounding bit
 *    2. Position mantissa bits
 *    3. Position exponent
 *    4. Combine exponent, mantissa, round
 *    5. Compute sign bit, impose and move result
 */

Ltrig_25:
	chkbit	10,numer_poly_lo		/* Rounding bit			*/

#if	defined(CA_optim)
	eshro	11,numer_poly_lo,numer_poly_lo
#else
	shro	11,numer_poly_lo,numer_poly_lo
	shlo	32-11,numer_poly_hi,tmp_1
	or	numer_poly_lo,tmp_1,numer_poly_lo
#endif
	addc	0,numer_poly_lo,numer_poly_lo	/* Begin rounding		*/
	shro	11,numer_poly_hi,numer_poly_hi	/* Finish mantissa positioning	*/
	lda	DP_Bias-2,con_lo		/* Begin computing result exp	*/
	subo	shift_cnt,con_lo,s1_exp
	shlo	20,s1_exp,s1_exp		/* Position exponent		*/
	addc	numer_poly_hi,s1_exp,numer_poly_hi

	shlo	31,quadrant,quadrant
	xor	s1_hi,quadrant,tmp_1
	chkbit	31,tmp_1
	alterbit 31,numer_poly_hi,out_hi
	movldar(numer_poly_lo,out_lo)
	ret


/*
 *  No reduction step -> set up for normal evaluation
 */

Ltrig_30:
	bl	Ltrig_32				/* J/ |arg_hi| < PI/4_hi	*/

	lda	DP_PI_OVER_4_lo,con_lo
	cmpobg.f hold_reg_2,con_lo,Ltrig_01	/* J/ |arg| > PI/4		*/

Ltrig_32:
	chkbit	31,x_mant_hi			/* Check for norm shift		*/
	lda	DP_Bias-2,con_lo

	subo	s1_exp,con_lo,shift_cnt		/* Derive shift from arg exp	*/
	bo	Ltrig_35				/* J/ x_mant normalized		*/

	addc	x_mant_ext,x_mant_ext,x_mant_ext
	addc	x_mant_lo,x_mant_lo,x_mant_lo
	addc	x_mant_hi,x_mant_hi,x_mant_hi

	addlda(1,shift_cnt,shift_cnt)

Ltrig_35:
	cmpobg.t 32-5,shift_cnt,Ltrig_02		/* J/ can use std poly eval	*/
	b	Ltrig_44				/* J/ tan (x_mant norm'd)	*/


/*
 * Shift count >= 1/2 mantissa bit width -> single or zero term approximation
 */

Ltrig_40:
	cmpobe.t 0,x_mant_hi,Ltrig_42		/* J/ _hi word zero		*/

	shlo	shift_cnt,x_mant_hi,x_mant_hi	/* Norm from _hi word bits	*/
	shro	tmp_1,x_mant_lo,con_hi
	or	x_mant_hi,con_hi,x_mant_hi

	shlo	shift_cnt,x_mant_lo,x_mant_lo
	shro	tmp_1,x_mant_ext,con_lo
	or	x_mant_lo,con_lo,x_mant_lo
	b	Ltrig_44

Ltrig_42:
	scanbit x_mant_lo,tmp_1			/* Norm from _lo word bits	*/

	subo	tmp_1,31,shift_cnt
	addlda(1,tmp_1,tmp_1)

	shlo	shift_cnt,x_mant_lo,x_mant_hi
	shro	tmp_1,x_mant_ext,con_hi
	or	x_mant_hi,con_hi,x_mant_hi

	shlo	shift_cnt,x_mant_ext,x_mant_lo

	setbit	5,shift_cnt,shift_cnt		/* add 32 to shift count	*/

Ltrig_44:
	lda	PI_OVER_FOUR_mid,con_lo		/* single term tan conversion	*/
	emul	x_mant_hi,con_lo,rp_1_lo

	lda	PI_OVER_FOUR_hi,con_hi
	emul	x_mant_lo,con_hi,rp_2_lo

	emul	x_mant_hi,con_hi,numer_poly_lo

	addc	rp_1_lo,rp_2_lo,rp_1_lo
	addc	rp_1_hi,rp_2_hi,rp_1_lo
	addc	0,0,rp_1_hi

	addc	rp_1_lo,numer_poly_lo,numer_poly_lo
	addc	rp_1_hi,numer_poly_hi,numer_poly_hi

	bbc.t	0,quadrant,Ltrig_23		/* Done if standard tan eval	*/

	subo	1,0,rp_1_lo			/* Set denom to scaled 1.0	*/
	shro	1,rp_1_lo,rp_1_hi		/* rp_1 is denom for invert	*/

	b	Ltrig_21				/* Inverted tan evaluation	*/


/*
 *  Two term evaluation
 */

Ltrig_50:
	lda	C3_hi,numer_poly_hi
	emul	x_sqr_hi,numer_poly_hi,numer_poly_lo

	clrbit	5,shift_cnt,tmp_1		/* Subtract 32 from shift count	*/
	lda	C1_hi,con_hi
	shro	1,shift_cnt,shift_cnt		/* Estimated result exp		*/
	lda	C1_lo,con_lo

	shro	tmp_1,numer_poly_hi,numer_poly_lo
	movlda(0,numer_poly_hi)

	addc	numer_poly_lo,con_lo,numer_poly_lo
	addc	numer_poly_hi,con_hi,numer_poly_hi

	emul	x_mant_lo,numer_poly_hi,rp_1_lo
	emul	x_mant_hi,numer_poly_lo,rp_2_lo
	emul	x_mant_hi,numer_poly_hi,numer_poly_lo

	addc	rp_1_lo,rp_2_lo,rp_2_lo		/* Combine lo order prtl prod	*/
	subo	1,0,rp_1_lo			/* Set denom to scaled 1.0	*/
	addc	rp_1_hi,rp_2_hi,rp_2_lo
	shro	1,rp_1_lo,rp_1_hi		/* rp_1 is denom for invert	*/
	addc	0,0,rp_2_hi

	addc	rp_2_lo,numer_poly_lo,numer_poly_lo
	addc	rp_2_hi,numer_poly_hi,numer_poly_hi

	bbc.t	0,quadrant,Ltrig_23		/* Done if standard tan eval	*/

	b	Ltrig_21				/* Inverted tan evaluation	*/



/*
 *  Very small argument -> return arg
 */

Ltrig_60:
/* ***	movl	s1,out  *** */			/* Return argument		*/
	ret



/*
 *  NaN or too large
 */

Ltrig_80:
	movl	s1_lo,r4			/* Save incoming value		*/

	callj	__errno_ptr			/* returns addr of ERRNO in g0	*/
	ldconst	EDOM,r8				/* Set errno to EDOM		*/
	st	r8,(g0)

	movl	r4,out_lo			/* Default -> return arg	*/

	cmpo	out_lo,0
	lda	DP_INF << 21,con_hi

	addc	out_hi,out_hi,tmp_2
	xor	1,tmp_2,tmp_2

	cmpobg.f tmp_2,con_hi,Ltrig_82		/* J/ NaN argument		*/

	movl	0,out_lo			/* Return 0.0 (non-NaN arg)	*/

	bne.t	Ltrig_84				/* J/ finite argument		*/

Ltrig_82:
	lda	Standard_QNaN_hi,tmp_1		/* Force Quiet NaN		*/

	or	out_hi,tmp_1,out_hi

Ltrig_84:	
	ret
