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
/*      sincos.c  - Double Precision Sine/Cosine Function (AFP-960)	      */
/*									      */
/******************************************************************************/

#include "asmopt.h"

	.file	"sincos.s"
	.globl	_sin,_cos

        .globl  __errno_ptr            /* returns addr of ERRNO in g0          */


#define EDOM    33
#define ERANGE  34

#define	Standard_QNaN_hi  0xfff80000
#define	Standard_QNaN_lo  0x00000000

#define	DP_Bias		0x3ff
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

#define	rp_2_lo		r12
#define	rp_2_hi		r13

#define	trig_poly_lo	r14
#define	trig_poly_hi	r15

#define	quadrant	g2

#define	tmp_1		g4
#define	tmp_2		g5

#define	x_sign		g6
#define	shift_cnt	g7

#define	out_lo		g0
#define	out_hi		g1

#define	mem_ptr		out_lo

#define	x_mant_ext	x_sqr_lo
#define	hold_reg_1	x_sqr_hi
#define	hold_reg_2	shift_cnt


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
 *  Constants derived from Hart et al SIN 3043
 *  (fixed point fraction representation)
 */
Lsin_poly:
	.word	0x078FBB4E,0x00000000	/*  6.877100349            E-12  */
	.word	0x8C018E67,0x00000007	/*  1.757149292755         E-9  */
	.word	0xE0BF28C8,0x00000541	/*  3.13361621661904       E-7  */
	.word	0x99C5AA5F,0x000265A5	/*  3.6576204158455695     E-5  */
	.word	0x3BAC37FA,0x00A335E3	/*  2.490394570188736117   E-3  */
	.word	0x25BE409F,0x14ABBCE6	/*  8.0745512188280530192  E-2  */
	.word	0x2168C205,0xC90FDAA2	/*  7.85398163397448307014 E-1  */
	.set	Lsin_poly_len,(.-Lsin_poly) >> 3

/*
 *  Constants derived from Hart et al COS 3824
 *  (fixed point fraction representation)
 */
Lcos_poly:
	.word	0x006C961B,0x00000000	/*  3.8577620372             E-13  */
	.word	0x7E730575,0x00000000	/*  1.1500497024263          E-10  */
	.word	0xB47B0F52,0x00000069	/*  2.461136382637005        E-8  */
	.word	0xA0D0607E,0x00003C3E	/*  3.59086044588581953      E-6  */
	.word	0x7E3C8D5C,0x00155D3C	/*  3.2599188692668755044    E-4  */
	.word	0x06D6AA71,0x040F07C2	/*  1.585434424381541089754  E-2  */
	.word	0xF9177918,0x4EF4F326	/*  3.0842513753404245242414 E-1  */
	.word	0xFFFFFFFF,0xFFFFFFFF	/*  9.9999999999999999996415 E-1  */
	.set	Lcos_poly_len,(.-Lcos_poly) >> 3


#if	defined(CA_optim)
	.align	5
#else
	.align	4
#endif

_cos:
	clrbit	31,s1_hi,s1_hi			/* Zap sign bit of arg		*/
	movlda(1,quadrant)
	b	Ltrig_00

	.align	3
_sin:
	mov	0,quadrant			/* Signal SIN function		*/

Ltrig_00:
/*
 *  Create left-justified mant
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

	emul	s1_mant_hi,con_lo,trig_poly_lo

	addc	rp_1_lo,rp_2_lo,rp_1_lo
	addc	rp_1_hi,rp_2_hi,rp_1_lo
	addc	0,0,rp_1_hi

	lda	FOUR_OVER_PI_hi,con_lo

	emul	s1_mant_lo,con_lo,rp_2_lo

	lda	DP_Bias-28,con_hi
	cmpoble.f s1_exp,con_hi,Ltrig_60		/* J/ small/denorm/zero arg	*/

	addc	rp_1_lo,trig_poly_lo,rp_1_lo

	emul	s1_mant_hi,con_lo,x_mant_lo

	addc	rp_1_hi,trig_poly_hi,rp_1_hi
	addc	rp_1_lo,rp_2_lo,x_mant_ext
	addc	rp_1_hi,rp_2_hi,rp_1_lo
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

	chkbit	tmp_1,x_mant_hi			/* Rounding/x_sign bit		*/
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

	alterbit 31,0,x_sign

	addc	quadrant,tmp_2,quadrant

	shri	31,x_sign,x_sign
	xor	x_mant_ext,x_sign,x_mant_ext
	xor	x_mant_lo,x_sign,x_mant_lo
	xor	x_mant_hi,x_sign,x_mant_hi

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

	chkbit	0,quadrant
	lda	Lsin_poly-.-8(ip),mem_ptr

	mov	Lsin_poly_len,tmp_1

	emul	x_mant_hi,x_mant_hi,x_sqr_lo

	bno	Ltrig_03
	mov	Lcos_poly_len,tmp_1
	lda	Lcos_poly-Lsin_poly(mem_ptr),mem_ptr
	mov	0,x_sign
Ltrig_03:

	addc	rp_1_lo,rp_1_lo,rp_1_lo
	ldl	(mem_ptr),trig_poly_lo

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

Ltrig_eval_01:
	emul	x_sqr_lo,trig_poly_hi,rp_1_lo
	ldl	(mem_ptr),con_lo
	addo	8,mem_ptr,mem_ptr
	emul	x_sqr_hi,trig_poly_lo,rp_2_lo

	subo	1,tmp_1,tmp_1

	emul	x_sqr_hi,trig_poly_hi,trig_poly_lo

	addc	rp_1_lo,rp_2_lo,rp_1_lo
	addc	rp_1_hi,rp_2_hi,rp_1_lo
	addc	0,0,rp_1_hi

	addc	rp_1_lo,trig_poly_lo,trig_poly_lo
	addc	rp_1_hi,trig_poly_hi,trig_poly_hi

#if	defined(CA_optim)
	eshro	shift_cnt,trig_poly_lo,trig_poly_lo
#else
	shro	shift_cnt,trig_poly_lo,trig_poly_lo
	shlo	tmp_2,trig_poly_hi,rp_1_lo
	or	trig_poly_lo,rp_1_lo,trig_poly_lo
#endif
	shro	shift_cnt,trig_poly_hi,trig_poly_hi

	subc	trig_poly_lo,con_lo,trig_poly_lo
	subc	trig_poly_hi,con_hi,trig_poly_hi

	cmpobne	1,tmp_1,Ltrig_eval_01

	bbc.t	0,quadrant,Ltrig_15

	mov	0,shift_cnt
	b	Ltrig_22

Ltrig_15:
	emul	x_mant_lo,trig_poly_hi,rp_1_lo
	shro	1,shift_cnt,shift_cnt
	emul	x_mant_hi,trig_poly_lo,rp_2_lo
	emul	x_mant_hi,trig_poly_hi,trig_poly_lo

	addc	rp_1_lo,rp_2_lo,rp_1_lo
	addc	rp_1_hi,rp_2_hi,rp_1_lo
	addc	0,0,rp_1_hi

	addc	rp_1_lo,trig_poly_lo,trig_poly_lo
	addc	rp_1_hi,trig_poly_hi,trig_poly_hi

Ltrig_20:
	bbs.f	31,trig_poly_hi,Ltrig_22

	addc	trig_poly_lo,trig_poly_lo,trig_poly_lo
	addc	trig_poly_hi,trig_poly_hi,trig_poly_hi

	addo	1,shift_cnt,shift_cnt

Ltrig_22:
	chkbit	10,trig_poly_lo

#if	defined(CA_optim)
	eshro	11,trig_poly_lo,trig_poly_lo
#else
	shro	11,trig_poly_lo,trig_poly_lo
	shlo	32-11,trig_poly_hi,tmp_1
	or	trig_poly_lo,tmp_1,trig_poly_lo
#endif
	addc	0,trig_poly_lo,trig_poly_lo

	shro	11,trig_poly_hi,trig_poly_hi
	lda	DP_Bias-2,s1_exp
	subo	shift_cnt,s1_exp,s1_exp
	shlo	20,s1_exp,s1_exp
	addc	trig_poly_hi,s1_exp,trig_poly_hi

	xor	s1_hi,x_sign,tmp_1
	shlo	30,quadrant,quadrant
	xor	tmp_1,quadrant,tmp_1
	chkbit	31,tmp_1
	alterbit 31,trig_poly_hi,out_hi
	movldar(trig_poly_lo,out_lo)
	ret


/*
 *  No reduction step -> set up for normal evaluation
 */

Ltrig_30:
	bl	Ltrig_32				/* J/ |arg_hi| < PI/4_hi	*/

	lda	DP_PI_OVER_4_lo,con_lo
	cmpobg.f s1_lo,con_lo,Ltrig_01		/* J/ |arg| > PI/4		*/

Ltrig_32:
	chkbit	31,x_mant_hi			/* Check for norm shift		*/
	lda	DP_Bias-2,con_lo

	subo	s1_exp,con_lo,shift_cnt		/* Derive shift from arg exp	*/
	movlda(0,x_sign)

	bo	Ltrig_35				/* J/ x_mant normalized		*/

	addc	x_mant_ext,x_mant_ext,x_mant_ext
	addc	x_mant_lo,x_mant_lo,x_mant_lo
	addc	x_mant_hi,x_mant_hi,x_mant_hi

	addlda(1,shift_cnt,shift_cnt)

Ltrig_35:
	cmpobge.t 27,shift_cnt,Ltrig_02		/* J/ must use std poly eval	*/

	bbs.f	0,quadrant,Ltrig_45		/* J/ cosine (1.0 result)	*/

	b	Ltrig_44				/* J/ sine (x_mant norm'd)	*/


/*
 * Shift count >= 1/2 mantissa bit width -> single or zero term approximation
 */

Ltrig_40:
	bbs.f	0,quadrant,Ltrig_45		/* J/ cosine evaluation		*/

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
	lda	PI_OVER_FOUR_mid,con_lo		/* single term sin conversion	*/
	emul	x_mant_hi,con_lo,rp_1_lo

	lda	PI_OVER_FOUR_hi,con_hi
	emul	x_mant_lo,con_hi,rp_2_lo

	emul	x_mant_hi,con_hi,trig_poly_lo

	addc	rp_1_lo,rp_2_lo,rp_1_lo
	addc	rp_1_hi,rp_2_hi,rp_1_lo
	addc	0,0,rp_1_hi

	addc	rp_1_lo,trig_poly_lo,trig_poly_lo
	addc	rp_1_hi,trig_poly_hi,trig_poly_hi

	b	Ltrig_20

Ltrig_45:
	shlo	31,1,trig_poly_hi		/* Return 1.0			*/
	movlda(0,trig_poly_lo)
	subo	1,0,shift_cnt
	movlda(0,x_sign)
	b	Ltrig_22


/*
 *  Two term evaluation
 */

Ltrig_50:
	ld	-20(mem_ptr)[tmp_1*8],trig_poly_hi /* Fetch _hi of penult coef	*/
	ldl	-16(mem_ptr)[tmp_1*8],con_lo	   /* Fetch last coef		*/

	clrbit	5,shift_cnt,tmp_1		/* Subtract 32 from shift cnt	*/

	emul	x_sqr_hi,trig_poly_hi,trig_poly_lo

	shro	tmp_1,trig_poly_hi,trig_poly_lo

	subc	trig_poly_lo,con_lo,trig_poly_lo
	subc	0,con_hi,trig_poly_hi

	bbc.t	0,quadrant,Ltrig_15		/* J/ finish sin evaluation	*/

	mov	0,shift_cnt
	b	Ltrig_22				/* J/ finish cosine evaluation	*/


/*
 *  Very small argument -> return arg if sin, return 1.0 if cosine
 */

Ltrig_60:
	bbs	0,quadrant,Ltrig_65		/* J/ cosine -> return 1.0	*/

/* ***	movl	s1,out  *** */			/* Return argument		*/
	ret

Ltrig_65:
	mov	0,out_lo			/* Return 1.0			*/
	lda	DP_Bias << 20,out_hi
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
