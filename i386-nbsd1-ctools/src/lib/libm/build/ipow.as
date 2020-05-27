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
/*      ipow.c  - Double Precision POW Function - internal portion (AFP-960)  */
/*									      */
/******************************************************************************/


#include "asmopt.h"

	.file	"ipow.s"
	.globl	__AFP_int_pow


	.globl	__errno_ptr		/* returns addr of ERRNO in g0		*/

#define	ERANGE	34



#define	DP_Bias         0x3ff
#define	DP_INF          0x7ff



/* Register Name Equates */

#define	s1_lo		g0
#define	s1_hi		g1

#define	s2_lo		g2
#define	s2_hi		g3

#define	s_exp		r3
#define	x_exp		s_exp

#define	s_mant_lo	r4
#define	s_mant_hi	r5
#define	s_mant		s_mant_lo

#define	x_mant_lo	s_mant_lo
#define	x_mant_hi	s_mant_hi
#define	x_mant		x_mant_lo

#define	z_mant_lo	s_mant_lo
#define	z_mant_hi	s_mant_hi
#define	z_mant		z_mant_lo

#define	x_sqr_lo	r6
#define	x_sqr_hi	r7
#define	x_sqr		x_sqr_lo

#define	z_sqr_lo	x_sqr_lo
#define	z_sqr_hi	x_sqr_hi
#define	z_sqr		z_sqr_lo

#define	rp_1_lo		r8
#define	rp_1_hi		r9
#define	rp_1		rp_1_lo

#define	rp_2_lo		r10
#define	rp_2_hi		r11
#define	rp_2		rp_2_lo

#define	rp_3_lo		r12
#define	rp_3_hi		r13
#define	rp_3		rp_3_lo

#define	flags		r14
#define	neg_z_bit	  0
#define	neg_exp_bit	  1
#define	neg_rslt_bit	  2
#define	neg_x_bit	  3

#define	shift_cnt	r15

#define	con_lo	 	g0
#define	con_hi	 	g1

#define	rp_keep_lo	g4
#define	rp_keep_hi	g5
#define	rp_keep		rp_keep_lo

#define	rp_4_lo		g6
#define	rp_4_hi		g7
#define	rp_4		rp_4_lo

#define	tmp_1		rp_4_lo
#define	tmp_2		rp_4_hi

#define	out_lo		g0
#define	out_hi		g1
#define	out		out_lo

#define	mem_ptr		g13


/*
 *  Misc constants
 */

#define	SQRT_2_NO_j	0x6A09E668

#define	UNFL_LIMIT	0x86800000



/*
 *  Constants for "short" evaluation of log2
 */

#define	C1_hi		0xB8AA3B29
#define	C1_lo		0x5C17F0BC

#define	C3		0x3D8E13B8


/*
 *  Constants for "short" evaluation of exp2
 */

#define	LN_2_hi		0xB17217F7



	.text
	.link_pix


	.align	4

/*
 *  Hart et al LOGE 2705  -  19.38 digits accuracy (here limited to 64-bits)
 *                           (scaled by LOG2(e) to yield LOG2 result)
 *
 *  LOG2(x) = z * P(z^2)/Q(z^2)   ( 1/sqrt(2) <= x <= sqrt(2) ; z = (x-1)/(x+1) )
 */

Llog2_p_poly:
	.word	0x526D76D9,0x026E14A7	/* P7    0.42108 73712 17979 71450E-01 / Ln(2)  */
	.word	0x80A68ADD,0x379DF36D	/* P5    9.63769 09336 86865 93240E+00 / Ln(2)  */
	.word	0x773051AF,0xB2A5D1B8	/* P3   30.95729 28215 37650 06226E+01 / Ln(2)  */
	.word	0x76048D54,0x8A943C0E	/* P1   24.01391 79559 21050 98684E+01 / Ln(2)  */
	.set	Llog2_p_poly_cnt,(.-Llog2_p_poly) >> 3

Llog2_q_poly:
	.word	0x00000000,0x08000000	/* Q6    1.00000 00000 00000 00000E+00          */
	.word	0xB0AAABC9,0x4749F387	/* Q4    8.91110 90279 37831 23370E+00          */
	.word	0x2C246B73,0x9BD904BD	/* Q2   19.48096 60700 88973 05162E+01          */
	.word	0x36084701,0x600E4082	/* Q0   12.00695 89779 60525 47175E+01          */
	.set	Llog2_q_poly_cnt,(.-Llog2_q_poly) >> 3



/*
 *  Hart et al EXPBC 1324  -  21.54 digits accuracy (here limited to 64-bits)
 *
 *  2^x - 1  =   2xP(x^2)/(Q(x^2)-xP(x^2))    ( |x| <= 1/2 )
 */

Lexp2_p_poly:
	.word	0x037E7A03,0x0000793A	/* P5          60.61330 79074 80042 57484  */
	.word	0x5414E1AD,0x00EC9B3D	/* P3      30 285.61978 21164 59206 24270  */
	.word	0xB0E8D7EC,0x3F7C3612	/* P1   2 080 283.03650 59627 12855 955    */
	.set	Lexp2_p_poly_cnt,(.-Lexp2_p_poly) >> 3

Lexp2_q_poly:
	.word	0x00000000,0x00000200	/* Q6           1.00000 00000 00000        */
	.word	0x08B387B7,0x000DAA71	/* Q4       1 749.22076 95105 71455 89914  */
	.word	0x29B7BDE7,0x0A003B18	/* Q2     327 709.54719 32811 08534 0201   */
	.word	0xE709837E,0xB72DF814	/* Q0   6 002 428.04082 51736 65336 947    */
	.set	Lexp2_q_poly_cnt,(.-Lexp2_q_poly) >> 3


/*
 * Standard exponential evaluation polynomials
 */

Lexp2_s_poly:
	.word	0xC1282FE3,0x071AC235	/*   1 / 3!  /  log2(e) ^ 3   */
	.word	0x82C58EA9,0x1EBFBDFF	/*   1 / 2!  /  log2(e) ^ 2   */
	.word	0xE8E7BCD6,0x58B90BFB	/*   1 / 1!  /  log2(e) ^ 1   */
	.word	0x00000000,0x80000000	/*   1 / 0!  /  log2(e) ^ 0   */
	.set	Lexp2_s_poly_cnt,(.-Lexp2_s_poly) >> 3
	



#if	defined(CA_optim)
	.align	5
#else
	.align	4
#endif


__AFP_int_pow:

#if	defined(CA_optim)
	eshro	32-1-11,s1_lo,s_mant_hi		/* Create left-justified mant	*/
#else
	shlo	1+11,s1_hi,s_mant_hi		/* Create left-justified mant	*/
	shro	32-1-11,s1_lo,tmp_1		/* _lo -> _hi bits		*/
	or	s_mant_hi,tmp_1,s_mant_hi
#endif
	shlo	1+11,s1_lo,s_mant_lo

	addc	s1_hi,s1_hi,s_exp		/* Left justify exp field	*/

	shro	32-11,s_exp,s_exp		/* Biased, right justif exp	*/

	alterbit neg_rslt_bit,0,flags		/* Set rslt sign (carry out)	*/

	cmpobne.f 0,s_exp,Lpow_05		/* J/ not denorm		*/

	scanbit	s_mant_hi,tmp_1
	bno	Lpow_02				/* J/ hi word zero		*/

	subo	31,tmp_1,s_exp			/* Effective exponent value	*/

	subo	tmp_1,31,tmp_2			/* Left shift to normalize	*/
	addlda(1,tmp_1,tmp_1)			/* Right shift for wd -> wd	*/

#if	defined(CA_optim)
	eshro	tmp_1,s_mant_lo,s_mant_hi	/* Left justify denorm value	*/
#else
	shlo	tmp_2,s_mant_hi,s_mant_hi	/* Left justify denorm value	*/
	shro	tmp_1,s_mant_lo,tmp_1
	or	s_mant_hi,tmp_1,s_mant_hi
#endif
	cmpo	1,0				/* Clear carry			*/

	shlo	tmp_2,s_mant_lo,s_mant_lo

	addc	s_mant_lo,s_mant_lo,s_mant_lo	/* Drop MS bit			*/
	addc	s_mant_hi,s_mant_hi,s_mant_hi

	b	Lpow_05

Lpow_02:
	scanbit	s_mant_lo,tmp_1

	subo	tmp_1,31,tmp_2			/* Left shift to normalize	*/
	lda	63,s_exp			/* Begin computing exponent	*/

	subo	s_exp,tmp_1,s_exp
	addlda(1,tmp_2,tmp_2)			/* To drop the MS bit		*/

	shlo	tmp_2,s_mant_lo,s_mant_hi	/* Left justify denorm value	*/
	movlda(0,s_mant_lo)

Lpow_05:
#if	defined(CA_optim)
	eshro	2,s_mant_lo,rp_1_lo		/* Position denom for z calc	*/
#else
	shlo	32-2,s_mant_hi,tmp_1		/* Position denom for z calc	*/
	shro	2,s_mant_lo,rp_1_lo
	or	tmp_1,rp_1_lo,rp_1_lo
#endif

	lda	SQRT_2_NO_j,con_hi

	cmpo	s_mant_hi,con_hi

	shro	2,s_mant_hi,rp_1_hi
	bl.f	Lpow_12				/* J/ < sqrt(2)			*/

	cmpo	0,0				/* Set no borrow		*/

	subc	s_mant_lo,0,s_mant_lo		/* Compute numer = 1 - mant	*/
	setbit	neg_z_bit,flags,flags		/* Signal >= sqrt(2)		*/

	subc	s_mant_hi,0,s_mant_hi

	setbit	30,rp_1_hi,rp_1_hi		/* 1 + mant frac		*/
	addlda(1,s_exp,s_exp)


Lpow_12:
	setbit	31,rp_1_hi,rp_1_hi		/* denom = 2 + mant		*/

	scanbit	s_mant_hi,tmp_1

	subo	tmp_1,31,tmp_2			/* Left shift for bit 31 norm	*/
	addlda(1,tmp_1,tmp_1)			/* Right shift for interword	*/

	bno	Lpow_80				/* direct log(x) approx		*/
	
	shlo	tmp_2,s_mant_hi,s_mant_hi	/* Left norm the numerator	*/
	shro	tmp_1,s_mant_lo,tmp_1		/* _lo -> _hi bits		*/
	or	s_mant_hi,tmp_1,s_mant_hi
	shlo	tmp_2,s_mant_lo,s_mant_lo
	addlda(1,tmp_2,shift_cnt)


#if	defined(CA_optim)
	eshro	1,s_mant_lo,s_mant_lo		/* Prevent division overflow	*/
#else
	shlo	32-1,s_mant_hi,tmp_1		/* Prevent division overflow	*/
	shro	1,s_mant_lo,s_mant_lo
	or	tmp_1,s_mant_lo,s_mant_lo
#endif
	shro	1,s_mant_hi,s_mant_hi


	bal	Lsfbdr				/* s_mant / rp_1 -> z_mant	*/


	emul	z_mant_hi,z_mant_lo,rp_1_lo	/* Compute z^2			*/

	emul	z_mant_hi,z_mant_hi,z_sqr_lo

	addc	rp_1_lo,rp_1_lo,tmp_1		/* Align partial products	*/
	addc	rp_1_hi,rp_1_hi,rp_1_lo
	addc	0,0,rp_1_hi

	shlo	1,shift_cnt,shift_cnt		/* Double shift for z_sqr	*/

	addc	rp_1_lo,z_sqr_lo,z_sqr_lo	/* Combine partial products	*/
	addc	rp_1_hi,z_sqr_hi,z_sqr_hi

	cmpobl.f 31,shift_cnt,Lpow_85		/* J/ single term poly approx	*/

	mov	Llog2_p_poly_cnt,tmp_1
	lda	Llog2_p_poly-.-8(ip),mem_ptr

	bal	Lsfbper1

	emul	rp_1_lo,z_mant_hi,rp_2		/* Apply "odd" power of z	*/
	emul	rp_1_hi,z_mant_lo,rp_3
	emul	rp_1_hi,z_mant_hi,rp_1

	addc	rp_2_lo,rp_3_lo,rp_2_lo		/* Finish poly approx		*/
	addc	rp_2_hi,rp_3_hi,rp_2_lo
	addc	0,0,rp_2_hi

	addc	rp_1_lo,rp_2_lo,rp_keep_lo	/* Retain "P" evaluation	*/
	addc	rp_1_hi,rp_2_hi,rp_keep_hi

	mov	Llog2_q_poly_cnt,tmp_1
/*	lda	Llog2_q_poly-.-8(ip),mem_ptr	\* mem_ptr left here by Lsfbper1	*/

	bal	Lsfbper1

#if	defined(CA_optim)
	eshro	31,rp_1,rp_1_hi
#else
	chkbit	31,rp_1_lo
	shlo	1,rp_1_hi,rp_1_hi
	alterbit 0,rp_1_hi,rp_1_hi
#endif
	shlo	1,rp_1_lo,rp_1_lo

	shro	1,shift_cnt,shift_cnt		/* Return to base z shift	*/

	movl	rp_keep,s_mant			/* P(z2): s_mant, Q(z2): rp_1	*/

	bal	Lsfbdr				/* P(z2)/Q(z2) -> s_mant (=log)	*/

	subo	2,shift_cnt,shift_cnt		/* Adjust for relative exps	*/


Lpow_13:						/* Single term approx re-ent	*/

	lda	-DP_Bias(s_exp),s_exp		/* Remove bias from exp		*/

	bbc	31,s_exp,Lpow_14			/* J/ exp is not negative	*/

	subo	s_exp,0,s_exp			/* Take absolute value		*/

Lpow_14:
	alterbit neg_exp_bit,flags,flags

	scanbit	s_exp,tmp_1

	addlda(1,shift_cnt,shift_cnt)
	bno.f	Lpow_19				/* J/ no integer part		*/

	subo	tmp_1,31,tmp_2

	movlda(0,rp_1_lo)			/* Extension bit for fraction	*/

	shlo	tmp_2,s_exp,rp_1_hi		/* Left just integer part bits	*/

	movldar(tmp_1,s_exp)			/* Exponent left in s_exp	*/

	addo	shift_cnt,s_exp,shift_cnt	/* Compute frac right shift	*/

	lda	32,tmp_1

Lpow_15:
	cmpobge.t 31,shift_cnt,Lpow_17		/* J/ no full word shifts	*/

	mov	s_mant_lo,rp_1_lo
	movldar(s_mant_hi,s_mant_lo)

	subo	tmp_1,shift_cnt,shift_cnt
	movlda(0,s_mant_hi)

	b	Lpow_15
	
Lpow_17:
	subo	shift_cnt,tmp_1,tmp_1		/* wd -> wd shift count		*/

	shlo	tmp_1,s_mant_lo,tmp_2		/* Drop out bits		*/
	shro	shift_cnt,rp_1_lo,rp_1_lo
	or	tmp_2,rp_1_lo,rp_1_lo		/* Extention bits in rp_1_lo	*/

	shlo	tmp_1,s_mant_hi,tmp_2
	shro	shift_cnt,s_mant_lo,s_mant_lo
	or	tmp_2,s_mant_lo,s_mant_lo

	shro	shift_cnt,s_mant_hi,s_mant_hi

	shro	neg_exp_bit-neg_z_bit,flags,tmp_1
	xor	flags,tmp_1,tmp_1
	bbs	neg_z_bit,tmp_1,Lpow_18		/* J/ int/frac different sign	*/


	addc	rp_1_lo,rp_1_lo,rp_1_lo		/* Round and add at once	*/
	addc	0,s_mant_lo,s_mant_lo
	addc	rp_1_hi,s_mant_hi,s_mant_hi
	cmpobne.t 0,s_mant_hi,Lpow_20		/* J/ no carry out		*/

	setbit	31,0,s_mant_hi			/* Handle carry out		*/
	addlda(1,s_exp,s_exp)			/* Adjust exponent		*/
	b	Lpow_20


Lpow_18:
	subc	rp_1_lo,0,rp_1_lo
	subc	s_mant_lo,0,s_mant_lo		/* Subtract fraction from int	*/
	subc	s_mant_hi,rp_1_hi,s_mant_hi
	bbs.t	31,s_mant_hi,Lpow_18b		/* J/ value still normalized	*/

	addc	rp_1_lo,rp_1_lo,rp_1_lo		/* Shift to normalize		*/
	subo	1,s_exp,s_exp			/* Adjust exponent		*/
	addc	s_mant_lo,s_mant_lo,s_mant_lo
	addc	s_mant_hi,s_mant_hi,s_mant_hi

Lpow_18b:
	addc	rp_1_lo,rp_1_lo,rp_1_lo		/* Round result			*/
	addc	0,s_mant_lo,s_mant_lo
	addc	0,s_mant_hi,s_mant_hi
	cmpobne.t 0,s_mant_hi,Lpow_20		/* J/ round didn't carry out	*/

	setbit	31,0,s_mant_hi			/* Handle carry out		*/
	addlda(1,s_exp,s_exp)			/* Adjust exponent		*/
	b	Lpow_20


Lpow_19:
	subo	shift_cnt,0,s_exp		/* Leave fraction alone		*/

	chkbit	neg_z_bit,flags			/* Make frac sign -> exp sign	*/
	alterbit neg_exp_bit,flags,flags

Lpow_20:



/* ************************************************************************** *
 *                                                                            *
 *      I n t e r n a l   6 4 - b i t   x   5 3 - b i t   m u l t i p l y     *
 *                                                                            *
 * ************************************************************************** */

/*
 *  log2(s1)   neg_exp_bit in flags   sign
 *                            s_exp   unbiased exponent value
 *                            s_mant  left justified 64-bit mantissa (w/ j bit set)
 *
 *  s2                        s2      IEEE dp format value
 *
 */

	chkbit	31,s2_hi			/* Compute mult result sign	*/
	alterbit neg_exp_bit,0,tmp_1
	xor	flags,tmp_1,flags

	clrbit	31,s2_hi,s2_hi			/* Zero the sign bit		*/

	shro	32-11-1,s2_hi,rp_2_lo		/* Extract exponent		*/

#if	defined(CA_optim)
	eshro	32-11,s2_lo,s2_hi		/* Create left-justified mant	*/
#else
	shlo	11,s2_hi,s2_hi			/* Create left-justified mant	*/
	shro	32-11,s2_lo,tmp_2		/* _lo -> _hi bits		*/
	or	s2_hi,tmp_2,s2_hi
#endif
	shlo	11,s2_lo,s2_lo

	setbit	31,s2_hi,s2_hi			/* Set implicit j bit		*/

	cmpobne.t 0,rp_2_lo,Lpow_24		/* J/ not denormal		*/

	clrbit	31,s2_hi,s2_hi			/* Reverse setting j bit	*/

	scanbit	s2_hi,tmp_2
	bno	Lpow_22				/* J/ top 32 bits are zero	*/

	subo	tmp_2,31,rp_2_lo
	addlda(1,tmp_2,tmp_2)

	shlo	rp_2_lo,s2_hi,s2_hi		/* Left justify the value	*/
	shro	tmp_2,s2_lo,tmp_2
	or	s2_hi,tmp_2,s2_hi

	shlo	rp_2_lo,s2_lo,s2_lo

	subo	rp_2_lo,1,rp_2_lo		/* Effective exponent		*/

	b	Lpow_24

Lpow_22:
	scanbit	s2_lo,tmp_2

	subo	tmp_2,31,rp_2_lo		/* Shift to left just value	*/

	shlo	rp_2_lo,s2_lo,s2_hi
	movlda(0,s2_lo)

	subo	tmp_2,0,rp_2_lo			/* Compute effective exponent	*/
	subo	31,rp_2_lo,rp_2_lo

Lpow_24:
	emul	s_mant_lo,s2_lo,rp_4		/* 64 x 53 bit multiply		*/
	addo	rp_2_lo,s_exp,x_exp		/* Biased exp of multiply	*/
	addo	1,x_exp,x_exp			/* Assume no norm shift		*/
	emul	s_mant_lo,s2_hi,rp_3
	cmpo	1,0
	emul	s_mant_hi,s2_lo,rp_2
	addc	rp_4_hi,rp_3_lo,rp_3_lo		/* Combine pp/lo w/ pp/mid-1	*/
	addc	0,rp_3_hi,rp_3_hi
	emul	s_mant_hi,s2_hi,s_mant
	addc	rp_3_lo,rp_2_lo,rp_2_lo		/* Combine w/ pp/mid-2		*/
	addc	rp_3_hi,rp_2_hi,rp_3_lo
	addc	0,0,rp_3_hi
	addc	rp_3_lo,s_mant_lo,x_mant_lo	/* Result in s_mant::rp_4_lo	*/
	addc	rp_3_hi,s_mant_hi,x_mant_hi

	bbs.t	31,x_mant_hi,Lpow_26		/* J/ normalized		*/

	addc	rp_2_lo,rp_2_lo,rp_2_lo
	subo	1,x_exp,x_exp			/* Reduce exponent		*/
	addc	x_mant_lo,x_mant_lo,x_mant_lo
	addc	x_mant_hi,x_mant_hi,x_mant_hi

Lpow_26:
	cmpo	1,0
	addc	rp_2_lo,rp_2_lo,rp_2_lo		/* Round result			*/
	addc	0,x_mant_lo,x_mant_lo
	addc	0,x_mant_hi,x_mant_hi

	cmpobne.t 0,x_mant_hi,Lpow_28		/* J/ no carry-out from round	*/

	setbit	31,0,x_mant_hi			/* Result = 1000...000		*/
	addlda(1,x_exp,x_exp)

Lpow_28:


/* ************************************************************************** */
/*                                                                            */
/*                 s g n  *   2 ^ x   e v a l u a t i o n                     */
/*                                                                            */
/* ************************************************************************** */

/*
 *  sgn  neg_rslt_bit  in  flags   sign of result value
 *
 *  x    neg_exp_bit   in  flags   sign
 *                         x_exp   biased exponent value
 *                         x_mant  left justified 64-bit mantissa (w/ j bit set)
 *
 */


	lda	DP_Bias+10,con_lo

	cmpibl.t x_exp,con_lo,Lpow_40		/* J/ no limit testing		*/

	mov	0,g14				/* Zero the bal ret addr reg	*/

	be.f	Lpow_34				/* J/ further limit testing	*/

	bbs	neg_exp_bit,flags,Lpow_32	/* J/ underflow			*/

Lpow_31:						/* Overflow			*/
	callj	__errno_ptr

	lda	ERANGE,r4			/* Set errno to ERANGE		*/
	st	r4,(g0)

	mov	0,out_lo			/* Return zero			*/
	lda	DP_INF << 20,out_hi

	chkbit	neg_rslt_bit,flags
	alterbit 31,out_hi,out_hi
	ret

Lpow_32:						/* Underflow			*/
	callj	__errno_ptr

	lda	ERANGE,r4
	st	r4,(g0)

	movl	0,out

	chkbit	neg_rslt_bit,flags
	alterbit 31,out_hi,out_hi
	ret

Lpow_34:
	bbc	neg_exp_bit,flags,Lpow_31	/* Overflow if positive		*/

	cmpo	x_mant_lo,0
	subc	1,0,tmp_1
	ornot	tmp_1,x_mant_hi,tmp_1
	lda	UNFL_LIMIT,con_hi
	cmpobge	tmp_1,con_hi,Lpow_32		/* J/ underflows even denorm	*/



Lpow_40:
	lda	-DP_Bias+1(x_exp),shift_cnt	/* Left shift for fraction pwr	*/
	bbs.f	31,shift_cnt,Lpow_75		/* J/ magnitude < 1/2		*/

	lda	32,tmp_1
	subo	shift_cnt,tmp_1,tmp_1		/* Right shift			*/

	shro	tmp_1,x_mant_hi,x_exp		/* Integer power of two		*/

/*
 *  Left justify fraction part in x_mant (possibly a shift of zero)
 */

	shro	tmp_1,x_mant_lo,tmp_1
	shlo	shift_cnt,x_mant_hi,x_mant_hi
	or	tmp_1,x_mant_hi,x_mant_hi

	shlo	shift_cnt,x_mant_lo,x_mant_lo

/*
 *  Adjust for (1) negative values, and for (2) mantissa >= 1/2
 */

	bbc	neg_exp_bit,flags,Lpow_42	/* J/ positive x value		*/

	subc	x_mant_lo,0,x_mant_lo		/* Magnitude of reduced arg	*/
	subc	x_mant_hi,0,x_mant_hi
	subc	x_exp,0,x_exp			/* Apply arg sign to 2's power	*/

Lpow_42:

	bbc	31,x_mant_hi,Lpow_44

	subc	x_mant_lo,0,x_mant_lo		/* Magnitude of reduced arg	*/
	subc	x_mant_hi,0,x_mant_hi

	cmpo	0,0				/* Insure neg_x_bit set		*/

Lpow_44:
	alterbit neg_x_bit,flags,flags


/*
 *  Normalize fractional power of 2, then decide on evaluation technique
 */

	scanbit	x_mant_hi,tmp_1			/* Compute num of leading 0's	*/

	bno	Lpow_72				/* Single term or no term appx	*/

	subo	tmp_1,31,shift_cnt		/* Left norm fraction		*/
	addlda(1,tmp_1,tmp_1)			/* Right shift for wd -> wd	*/

	shro	tmp_1,x_mant_lo,tmp_1		/* 2-wd left shift		*/ 
	shlo	shift_cnt,x_mant_hi,x_mant_hi	/* (0 shift allowed)		*/
	or	tmp_1,x_mant_hi,x_mant_hi
	shlo	shift_cnt,x_mant_lo,x_mant_lo

/*
 *  Choose polynomial evaluation algorithm
 */
Lpow_46:						/* re-entry for |x| < 1/2	*/

	cmpoble.f 16,shift_cnt,Lpow_70		/* J/ short direct poly eval	*/

	emul	x_mant_hi,x_mant_lo,rp_1	/* Square x for poly eval	*/

	lda	Lexp2_p_poly-.-8(ip),mem_ptr

	emul	x_mant_hi,x_mant_hi,x_sqr

	shlo	1,shift_cnt,shift_cnt		/* x^2 shift count		*/

	addc	rp_1_lo,rp_1_lo,rp_1_lo		/* Double middle product	*/
	addc	rp_1_hi,rp_1_hi,rp_1_lo
	addc	0,0,rp_1_hi

	addc	rp_1_lo,x_sqr_lo,x_sqr_lo	/* Combine partial products	*/
	addc	rp_1_hi,x_sqr_hi,x_sqr_hi

	mov	Lexp2_p_poly_cnt,tmp_1

	bal	Lsfbper2

	emul	rp_1_lo,x_mant_hi,rp_2		/* Apply "odd" power of z	*/
	emul	rp_1_hi,x_mant_lo,rp_3
	emul	rp_1_hi,x_mant_hi,rp_1

	addc	rp_2_lo,rp_3_lo,rp_2_lo		/* Finish poly approx		*/
	addc	rp_2_hi,rp_3_hi,rp_2_lo
	addc	0,0,rp_2_hi

	addc	rp_1_lo,rp_2_lo,rp_keep_lo	/* Retain "P" evaluation	*/
	addc	rp_1_hi,rp_2_hi,rp_keep_hi

	mov	Lexp2_q_poly_cnt,tmp_1
/*	lda	Lexp2_q_poly-.-8(ip),mem_ptr	\* mem_ptr left here by Lsfbper1	*/

	bal	Lsfbper2

	shro	1,shift_cnt,shift_cnt		/* Return the "x*" shift	*/

#if	defined(CA_optim)
	eshro	shift_cnt,rp_keep_lo,rp_keep_lo	/* Align P poly to Q poly	*/
#else
	lda	32,tmp_1
	subo	shift_cnt,tmp_1,tmp_1
	shlo	tmp_1,rp_keep_hi,tmp_2
	shro	shift_cnt,rp_keep_lo,rp_keep_lo
	or	tmp_2,rp_keep_lo,rp_keep_lo
#endif
	shro	shift_cnt,rp_keep_hi,rp_keep_hi

	addc	rp_keep_lo,rp_1_lo,rp_2_lo	/* LQ_poly + LP_poly		*/
	addc	rp_keep_hi,rp_1_hi,rp_2_hi

	subc	rp_keep_lo,rp_1_lo,rp_1_lo	/* LQ_poly - LP_poly		*/
	subc	rp_keep_hi,rp_1_hi,rp_1_hi

	bbs	neg_x_bit,flags,Lpow_52		/* J/ inverted division		*/


/*
 *  Prevent division overflow by shifting divisor or dividend
 */

	bbc.f	31,rp_1_hi,Lpow_51		/* J/ can left shift Q poly	*/

#if	defined(CA_optim)
	eshro	1,rp_2_lo,s_mant_lo
#else
	shro	1,rp_2_lo,s_mant_lo
	shlo	32-1,rp_2_hi,tmp_2
	or	s_mant_lo,tmp_2,s_mant_lo
#endif
	shro	1,rp_2_hi,s_mant_hi
	b	Lpow_54

Lpow_51:
	addc	rp_1_lo,rp_1_lo,rp_1_lo
	addc	rp_1_hi,rp_1_hi,rp_1_hi
	movl	rp_2_lo,s_mant
	b	Lpow_54

Lpow_52:
	movl	rp_1_lo,s_mant
	movl	rp_2_lo,rp_1_lo

Lpow_54:
	bal	Lsfbdr


/*
 *  Construct the result
 */

Lpow_60:
	mov	0,g14				/* Zero BAL ret addr reg	*/

	lda	DP_Bias-1(s_exp),s_exp		/* (-1 for j bit)		*/
	cmpibg.f 0,s_exp,Lpow_62			/* J/ denormalized result	*/

	shlo	20,s_exp,s_exp

	chkbit	64-53-1,s_mant_lo		/* Rounding bit to carry	*/

#if	defined(CA_optim)
	eshro	64-53,s_mant_lo,s_mant_lo
#else
	shro	64-53,s_mant_lo,s_mant_lo
	shlo	32-(64-53),s_mant_hi,tmp_1
	or	s_mant_lo,tmp_1,s_mant_lo
#endif
	addc	0,s_mant_lo,out_lo		/* Start rounding		*/

	shro	64-53,s_mant_hi,s_mant_hi
	addc	s_exp,s_mant_hi,out_hi		/* Mix exp & mant; move		*/

	chkbit	neg_rslt_bit,flags
	alterbit 31,out_hi,out_hi
	ret

Lpow_62:
	subo	s_exp,12-1,tmp_1		/* Right shift for hi wd denorm	*/
	addo	s_exp,32-12+1,tmp_2		/* Left shift wd -> wd xfer	*/
	cmpobl	31,tmp_1,Lpow_64			/* Top word of denorm is zero	*/

#if	defined(CA_optim)
	eshro	tmp_1,s_mant_lo,out_lo		/* Shift to denormalize		*/
#else
	shro	tmp_1,s_mant_lo,out_lo		/* Shift to denormalize		*/
	shlo	tmp_2,s_mant_hi,rp_1_lo
	or	out_lo,rp_1_lo,out_lo
#endif
	shro	tmp_1,s_mant_hi,out_hi

	shlo	tmp_2,s_mant_lo,rp_1_lo		/* MS bit is round bit		*/
	b	Lpow_66

Lpow_64:
	ldconst	32,rp_1_lo

	subo	rp_1_lo,tmp_1,tmp_1		/* Reduce right shift count	*/
	addo	rp_1_lo,tmp_2,tmp_2		/* Adjust wd -> wd shift count	*/

	shro	tmp_1,s_mant_hi,out_lo
	movlda(0,out_hi)
	shlo	tmp_2,s_mant_hi,rp_1_lo

Lpow_66:
	addc	rp_1_lo,rp_1_lo,rp_1_lo		/* Rounding bit to carry	*/
	addc	0,out_lo,out_lo			/* Round the result		*/
	addc	0,out_hi,out_hi

	chkbit	neg_rslt_bit,flags
	alterbit 31,out_hi,out_hi
	ret



/*
 *  Exp estimate w/ short direct poly (3 term standard exp expansion)
 */

Lpow_70:
	movl	x_mant,x_sqr			/* Use x for poly factor	*/

	mov	Lexp2_s_poly_cnt,tmp_1
	lda	Lexp2_s_poly-.-8(ip),mem_ptr

	bbs	neg_x_bit,flags,Lpow_71		/* Use alternating add/sub	*/

	bal	Lsfbper2				/* Result > 1.0			*/

	movl	rp_1,s_mant

	b	Lpow_60

Lpow_71:
	bal	Lsfbper1				/* Result < 1.0			*/

	addc	rp_1_lo,rp_1_lo,s_mant_lo	/* Normalize the result		*/
	addc	rp_1_hi,rp_1_hi,s_mant_hi

	b	Lpow_60



/*
 *  Estimate exp(x) with 1+x when |x| < 2^-31
 */

Lpow_72:
	lda	LN_2_hi,con_hi
	emul	x_mant_lo,con_hi,rp_1_lo


	bbs	neg_x_bit,flags,Lpow_73		/* J/ less than 1.0		*/

	setbit	31,0,s_mant_hi			/* 1 + frac			*/
	shro	1,rp_1_hi,s_mant_lo
	b	Lpow_60

Lpow_73:
	subc	rp_1_hi,0,s_mant_lo		/* 1 - frac			*/
	subc	0,0,s_mant_hi
	b	Lpow_60


/*
 *  Special handling for |x| < 1/2
 */

Lpow_75:
	lda	DP_Bias-1,con_hi

	subo	s_exp,con_hi,shift_cnt		/* Equivalent shift_cnt value	*/

	chkbit	neg_exp_bit,flags		/* 2^0 or 2^-1 exp from sign	*/
	alterbit 0,0,tmp_1
	alterbit neg_x_bit,flags,flags		/* Choose poly eval correctly	*/
	subo	tmp_1,0,s_exp

	cmpobge.t 31,shift_cnt,Lpow_46		/* J/ use standard sequence	*/

	ldconst	32,tmp_1
	subo	tmp_1,shift_cnt,shift_cnt

	cmpoble.f 32-9,shift_cnt,Lpow_83		/* J/ result is 1.0		*/

	shro	shift_cnt,x_mant_hi,x_mant_lo	/* Position for 1+x eval	*/
	movlda(0,x_mant_hi)
	b	Lpow_72





/*
 *  Mantissa value very close to 1.0 (top 33 bits = 1000...000)
 *
 *      approximate log2(mant) = log2(e)*((mant-1) - (mant-1)^2/2)
 */

Lpow_80:
	scanbit	s_mant_lo,tmp_1			/* Search for MS bit		*/

	bno	Lpow_82				/* J/ mantissa = 1.0 exactly	*/

	subo	tmp_1,31,shift_cnt		/* left shift to norm (0 to 20)	*/

	shlo	shift_cnt,s_mant_lo,s_mant_hi	/* Norm the value		*/

	emul	s_mant_hi,s_mant_hi,rp_3	/* Compute x^2 term		*/

	chkbit	neg_z_bit,flags
	addc	1,shift_cnt,tmp_2		/* Shift for squaring + /2	*/
	lda	C1_lo,tmp_1

	emul	s_mant_hi,tmp_1,rp_1		/* Lo partial prod, first term	*/

#if	defined(CA_optim)
	eshro	tmp_2,rp_3,rp_3_lo		/* Position second term		*/
	shro	tmp_2,rp_3_hi,rp_3_hi
#else
	shro	tmp_2,rp_3_lo,rp_3_lo		/* Position second term		*/
	subo	tmp_2,31,tmp_1
	addo	1,tmp_1,tmp_1
	shlo	tmp_1,rp_3_hi,tmp_1
	or	rp_3_lo,tmp_1,rp_3_lo
	shro	tmp_2,rp_3_hi,rp_3_hi
#endif

	chkbit	neg_z_bit,flags
	lda	C1_hi,tmp_1

	addc	31,shift_cnt,shift_cnt		/* Compute effective shift cnt	*/

	emul	s_mant_hi,tmp_1,s_mant

	cmpo	1,0				/* Clear carry			*/

	emul	rp_3_hi,tmp_1,rp_3		/* Scale second term		*/

	addc	rp_1_hi,s_mant_lo,s_mant_lo	/* Compute adjusted first term	*/
	addc	0,s_mant_hi,s_mant_hi

	cmpo	0,0				/* Set borrow			*/

	subc	rp_3_lo,rp_1_lo,rp_1_lo		/* Sub second term from first	*/
	subc	rp_3_hi,s_mant_lo,s_mant_lo
	subc	0,s_mant_hi,s_mant_hi


	bbs.t	31,s_mant_hi,Lpow_81		/* J/ normalized		*/

	addc	rp_1_lo,rp_1_lo,rp_1_lo		/* Left shift to normalize	*/
	addlda(1,shift_cnt,shift_cnt)
	addc	s_mant_lo,s_mant_lo,s_mant_lo
	addc	s_mant_hi,s_mant_hi,s_mant_hi

Lpow_81:						/* Round result			*/
	chkbit	31,rp_1_lo
	addc	0,s_mant_lo,s_mant_lo
	addc	0,s_mant_hi,s_mant_hi
	bbs.t	31,s_mant_hi,Lpow_13		/* No overflow from rounding	*/

	setbit	31,0,s_mant_hi			/* Result = 1000...000		*/
	subo	1,shift_cnt,shift_cnt
	b	Lpow_13


Lpow_82:						/* mant = 1.0			*/
	lda	DP_Bias,con_lo
	cmpobe.f s_exp,con_lo,Lpow_83		/* mant = 1.0, exp = 0		*/

	movl	0,s_mant			/* Set log frac to 0		*/
	movlda(0,shift_cnt)
	b	Lpow_13


Lpow_83:						/* Result is signed 1.0		*/
	mov	0,g14				/* Zero BAL ret addr reg	*/

	chkbit	neg_rslt_bit,flags

	mov	0,out_lo
	lda	DP_Bias << 20,out_hi

	alterbit 31,out_hi,out_hi
	ret	




/*
 *  Single term approximation:
 *
 *      log2(x) ~= z * ( 2*log2(e) + z^2*log2(e)/3 )  [ where z = (x-1)/(x+1) ]
 */

Lpow_85:
	lda	C3,con_hi

	emul	z_sqr_hi,con_hi,rp_1

	ldconst	32,tmp_1
	subo	tmp_1,shift_cnt,shift_cnt	/* Subtract 32 from shift_cnt	*/

	lda	C1_hi,con_hi
	cmpo	1,0
	lda	C1_lo,con_lo
	shro	shift_cnt,rp_1_hi,rp_1_lo

	addc	rp_1_lo,con_lo,rp_1_lo
	addc	0,con_hi,rp_1_hi

	emul	rp_1_lo,s_mant_lo,rp_3
	shro	1,shift_cnt,shift_cnt		/* Compute assumed shift cnt	*/
	addo	16-2,shift_cnt,shift_cnt	/* Reverse CLRBIT, poly adj	*/
	emul	rp_1_lo,s_mant_hi,rp_2
	emul	rp_1_hi,s_mant_lo,rp_4
	addc	rp_3_hi,rp_2_lo,rp_2_lo
	addc	0,rp_2_hi,rp_2_hi
	emul	rp_1_hi,s_mant_hi,rp_1
	addc	rp_4_lo,rp_2_lo,rp_3_lo
	addc	rp_4_hi,rp_2_hi,rp_2_lo
	addc	0,0,rp_2_hi
	addc	rp_2_lo,rp_1_lo,rp_1_lo
	addc	rp_2_hi,rp_1_hi,rp_1_hi
	bbs.t	31,rp_1_hi,Lpow_86		/* J/ result normalized		*/

	addc	rp_3_lo,rp_3_lo,rp_3_lo		/* Left shift to normalize	*/
	addlda(1,shift_cnt,shift_cnt)
	addc	rp_1_lo,rp_1_lo,rp_1_lo
	addc	rp_1_hi,rp_1_hi,rp_1_hi

Lpow_86:
	addc	rp_3_lo,rp_3_lo,rp_3_lo		/* Round the result		*/
	addc	0,rp_1_lo,s_mant_lo
	addc	0,rp_1_hi,s_mant_hi
	bbs.t	31,s_mant_hi,Lpow_13		/* J/ no carry out from round	*/

	setbit	31,0,s_mant_hi			/* React to carry out		*/
	subo	1,shift_cnt,shift_cnt
	b	Lpow_13



/*
 *  64-bit division routine
 *
 *  s_mant / rp_1 -> s_mant
 *
 *  Temps used:  rp_1, rp_2, rp_3, rp_4
 *
 *  The result is rounded and normalized.  Both operands are assumed left
 *  justified.  The value in shift_cnt is incremented when a normalization
 *  shift is required.  The result should be accurate to 64-bits.
 *
 */

Lsfbdr:
	ediv	rp_1_hi,s_mant,s_mant		/* Top word of a/b		*/

	mov	s_mant_lo,rp_2_hi
	movlda(0,rp_2_lo)

	ediv	rp_1_hi,rp_2,rp_2		/* Middle word of a/b		*/

	mov	rp_2_hi,s_mant_lo		/* Form a/b in s_mant::rp_2_hi	*/

	mov	rp_2_lo,rp_2_hi
	movlda(0,rp_2_lo)

	ediv	rp_1_hi,rp_2,rp_2		/* Lo word of a/b (96-bit rslt)	*/

	shro	1,rp_1_lo,rp_3_hi		/* Prepare to compute c/b	*/
	shlo	32-1,rp_1_lo,rp_3_lo

	ediv	rp_1_hi,rp_3,rp_3		/* High word of c/b		*/

	mov	rp_3_hi,rp_2_lo			/* Use rp_2_lo as a temp	*/
	mov	rp_3_lo,rp_3_hi
	movlda(0,rp_3_lo)

	ediv	rp_1_hi,rp_3,rp_3		/* Low word of c/b		*/

	emul	rp_2_lo,s_mant_hi,rp_1		/* Begin computing c/b * a/b	*/

	mov	rp_3_hi,rp_3_lo			/* Collect c/b in rp_3		*/
	mov	rp_2_lo,rp_3_hi

	emul	rp_3_hi,s_mant_lo,rp_4		/* Form c/b mid/a prod in rp_1	*/

	cmpo	1,0				/* clear carry			*/

	addc	rp_4_hi,rp_1_lo,rp_1_lo		/* Combine c/b * a/b partials	*/

	emul	rp_3_lo,s_mant_lo,rp_4

	addc	0,rp_1_hi,rp_1_hi

	addc	rp_4_hi,rp_1_lo,rp_1_lo
	addc	0,rp_1_hi,rp_1_hi

	emul	rp_1_hi,rp_3_hi,rp_4		/* Compute c/b * c/b * a/b	*/

	cmpo	0,0				/* Set no borrow		*/

	shlo	1,rp_4_hi,rp_4_lo		/* Align term 3 to term 2	*/
	shro	32-1,rp_4_hi,rp_4_hi

	subc	rp_4_lo,rp_1_lo,rp_1_lo		/* Calc a/b*c/b - a/b*c/b*c/b	*/
	subc	rp_4_hi,rp_1_hi,rp_1_hi

	cmpo	0,1				/* Clear carry			*/

	addc	rp_1_lo,rp_1_lo,rp_2_lo		/* Align corr term to a/b	*/
	addc	rp_1_hi,rp_1_hi,rp_1_lo
	addc	0,0,rp_1_hi

	cmpo	0,0				/* Clear borrow			*/

	subc	rp_2_lo,rp_2_hi,rp_2_hi
	subc	rp_1_lo,s_mant_lo,s_mant_lo
	subc	rp_1_hi,s_mant_hi,s_mant_hi

	bbs	31,s_mant_hi,Lsfdr_10		/* J/ normalized		*/

	addc	rp_2_hi,rp_2_hi,rp_2_hi		/* Right shift to normalize	*/
	addc	s_mant_lo,s_mant_lo,s_mant_lo
	addlda(1,shift_cnt,shift_cnt)
	addc	s_mant_hi,s_mant_hi,s_mant_hi

Lsfdr_10:
	chkbit	31,rp_2_hi			/* Round the result		*/
	addc	0,s_mant_lo,s_mant_lo
	addc	0,s_mant_hi,s_mant_hi

	be.f	Lsfdr_20				/* J/ carry out			*/
	bx	(g14)				/* BAL/return			*/

Lsfdr_20:
	setbit	31,0,s_mant_hi			/* Result = 1000...000		*/

	subo	1,shift_cnt,shift_cnt		/* Reduce the shift count	*/

	bx	(g14)				/* BAL/return			*/



/*
 *  64-bit Polynomial Evaluation routine
 *
 *  mem_ptr     points to polynomial constants table
 *  z_sqr	polynomial factor
 *  shift_cnt   right shift per degree
 *  tmp_1       number of coefficients
 *
 *  rp_1        result return
 *
 *  Temps used  rp_2, rp_3, tmp_2
 *
 *
 *  Lsfbper1:    Alternating coefficient signs
 *  Lsfbper2:	All coefficients positive
 *
 */

Lsfbper1:
	ldl	(mem_ptr),rp_1			/* Fetch the initial constant	*/

	addo	8,mem_ptr,mem_ptr

#if	!defined(CA_optim)
	ldconst	32,tmp_2
	subo	shift_cnt,tmp_2,tmp_2
#endif

Lsfbper1_10:					/* Polynomial approximation	*/
	emul	rp_1_lo,z_sqr_hi,rp_2		/* Apply poly factor power	*/
	ldl	(mem_ptr),con_lo
	addo	8,mem_ptr,mem_ptr
	emul	rp_1_hi,z_sqr_lo,rp_3
	emul	rp_1_hi,z_sqr_hi,rp_1

	addc	rp_2_lo,rp_3_lo,rp_2_lo
	addc	rp_2_hi,rp_3_hi,rp_2_lo
	addc	0,0,rp_2_hi

	addc	rp_1_lo,rp_2_lo,rp_1_lo		/* Combine partial products	*/
	addc	rp_1_hi,rp_2_hi,rp_1_hi

	cmpo	0,0

#if	defined(CA_optim)
	eshro	shift_cnt,rp_1,rp_1_lo
	shro	shift_cnt,rp_1_hi,rp_1_hi
#else
	shlo	tmp_2,rp_1_hi,rp_2_lo
	shro	shift_cnt,rp_1_hi,rp_1_hi
	shro	shift_cnt,rp_1_lo,rp_1_lo
	or	rp_2_lo,rp_1_lo,rp_1_lo
#endif
	subc	rp_1_lo,con_lo,rp_1_lo		/* Subtract next coefficient	*/
	subc	rp_1_hi,con_hi,rp_1_hi

	cmpdeco	2,tmp_1,tmp_1
	bne.t	Lsfbper1_10

	bx	(g14)



Lsfbper2:
	ldl	(mem_ptr),rp_1			/* Fetch the initial constant	*/

	addo	8,mem_ptr,mem_ptr

#if	!defined(CA_optim)
	ldconst	32,tmp_2
	subo	shift_cnt,tmp_2,tmp_2
#endif

Lsfbper2_10:					/* Polynomial approximation	*/
	emul	rp_1_lo,z_sqr_hi,rp_2_lo	/* Apply poly factor power	*/
	ldl	(mem_ptr),con_lo
	addo	8,mem_ptr,mem_ptr
	emul	rp_1_hi,z_sqr_lo,rp_3_lo
	emul	rp_1_hi,z_sqr_hi,rp_1

	addc	rp_2_lo,rp_3_lo,rp_2_lo
	addc	rp_2_hi,rp_3_hi,rp_2_lo
	addc	0,0,rp_2_hi

	addc	rp_1_lo,rp_2_lo,rp_1_lo		/* Combine partial products	*/
	addc	rp_1_hi,rp_2_hi,rp_1_hi

#if	defined(CA_optim)
	eshro	shift_cnt,rp_1_lo,rp_1_lo
	shro	shift_cnt,rp_1_hi,rp_1_hi
#else
	shlo	tmp_2,rp_1_hi,rp_2_lo
	shro	shift_cnt,rp_1_hi,rp_1_hi
	shro	shift_cnt,rp_1_lo,rp_1_lo
	or	rp_2_lo,rp_1_lo,rp_1_lo
#endif
	addc	rp_1_lo,con_lo,rp_1_lo		/* Add next coefficient		*/
	addc	rp_1_hi,con_hi,rp_1_hi

	cmpdeco	2,tmp_1,tmp_1
	bne.t	Lsfbper2_10

	bx	(g14)
