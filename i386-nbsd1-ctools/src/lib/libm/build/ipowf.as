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
/*      ipowf.c  - Single Precision POW Function - internal part (AFP-960)    */
/*									      */
/******************************************************************************/


#include "asmopt.h"

	.file	"ipowf.s"
	.globl	__AFP_int_powf


	.globl	__errno_ptr		/* returns addr of ERRNO in g0		*/

#define	ERANGE	34



#define	FP_Bias         0x7f
#define	FP_INF          0xff



/* Register Name Equates */

#define	s1		g0

#define	s2		g1

#define	s_exp		r3
#define	x_exp		s_exp

#define	s_mant_lo	r4
#define	s_mant		r5

#define	x_mant_lo	s_mant_lo
#define	x_mant		s_mant

#define	z_mant_lo	s_mant_lo
#define	z_mant		s_mant

#define	x_sqr_lo	r6
#define	x_sqr		r7

#define	z_sqr_lo	x_sqr_lo
#define	z_sqr		x_sqr

#define	rp_1_lo		r8
#define	rp_1_hi		r9
#define	rp_1		rp_1_lo

#define	P_poly_lo	r10
#define	P_poly		r11

#define	Q_poly_lo	r12
#define	Q_poly		r13

#define	flags		r14
#define	neg_z_bit	  0
#define	neg_exp_bit	  1
#define	neg_rslt_bit	  2
#define	neg_x_bit	  3

#define	shift_cnt	r15

#define	con_1	 	g2
#define	con_2	 	g3

#define	tmp_1		g4
#define	tmp_2		g5

#define	s2_exp		g6

#define	out		g0

#define	mem_ptr		g13


/*
 *  Misc constants
 */

#define	SQRT_2_NO_j	0x6A09E668

#define	UNFL_LIMIT	0x97000000



/*
 *  Constants for "short" evaluation of exp2
 */

#define	LN_2		0xB17217F7
#define	LOG2_e		0xB8AA3B29



	.text
	.link_pix

/*
 *  Hart et al LOGE 2702  -  11.20 digits accuracy (here limited to 32-bits)
 *                           (scaled by LOG2(e) to yield LOG2 result)
 *
 *  LOG2(x) = z * P(z^2)/Q(z^2)   ( 1/sqrt(2) <= x <= sqrt(2) ; z = (x-1)/(x+1) )
 */

	.set	Llog2_P1,0x8460E5F5	/*  22.93944 75477  /  ln(2)   */
	.set	Llog2_P3,0x45EC10A7	/*  12.11658 16588  /  ln(2)   */

	.set	Llog2_Q0,0xB783FD14	/*  11.46972 37757             */
	.set	Llog2_Q2,0x9E1AC244	/*   9.88153 29218             */
	.set	Llog2_Q4,0x10000000	/*   1.00000 00000             */
	.set	Llog2_Q4_shift,4



/*
 *  Hart et al EXPB 1063  -  10.03 digits accuracy
 *
 *  2^x  =   (Q(x^2) + xP(x^2)) / (Q(x^2) - xP(x^2))    ( |x| <= 1/2 )
 */

	.set	Lexp2_P1,0x39B8E985	/*   7.21528 91511  */
	.set	Lexp2_P3,0x00762636	/*   0.05769 00724  */

	.set	Lexp2_Q0,0xA68D27EB	/*  20.81892 37930  */
	.set	Lexp2_Q2,0x08000000	/*   1.00000 00000  */
	.set	Lexp2_Q2_shift,5




#if	defined(CA_optim)
	.align	5
#else
	.align	4
#endif


__AFP_int_powf:
	shlo	1+8,s1,s_mant			/* Create left-justified mant	*/

	addc	s1,s1,s_exp			/* Left justify exp field	*/

	shro	32-8,s_exp,s_exp		/* Biased, right justif exp	*/

	alterbit neg_rslt_bit,0,flags		/* Set rslt sign (carry out)	*/

	cmpobne.f 0,s_exp,Lpow_05		/* J/ not denorm		*/

	scanbit	s_mant,tmp_1

	subo	tmp_1,31,tmp_2			/* Left shift to normalize	*/

	subo	32-1,tmp_1,s_exp		/* Effective exponent value	*/
	addlda(1,tmp_2,tmp_2)			/* To drop the MS bit		*/

	shlo	tmp_2,s_mant,s_mant		/* Left justify denorm value	*/

Lpow_05:
	shro	2,s_mant,rp_1_hi		/* Position denom for z calc	*/

	lda	SQRT_2_NO_j,con_1

	cmpobl.f s_mant,con_1,Lpow_12		/* J/ < sqrt(2)			*/

	subo	s_mant,0,s_mant			/* Compute numer = 1 - mant	*/
	setbit	neg_z_bit,flags,flags		/* Signal >= sqrt(2)		*/

	setbit	30,rp_1_hi,rp_1_hi		/* 1 + mant frac		*/
	addlda(1,s_exp,s_exp)


Lpow_12:
	setbit	31,rp_1_hi,rp_1_hi		/* denom = 2 + mant		*/

	scanbit	s_mant,tmp_1

	subo	tmp_1,31,tmp_2			/* Left shift for bit 31 norm	*/

	cmpibge.f 16,tmp_1,Lpow_80		/* log(x) approx = x-1 (!!)	*/
	
	shlo	tmp_2,s_mant,s_mant		/* Left norm the numerator	*/
	addlda(2,tmp_2,shift_cnt)

	movlda(0,s_mant_lo)

	cmpobl	s_mant,rp_1_hi,Lpow_12a		/* J/ division won't overflow	*/

	shro	1,s_mant,s_mant			/* Prevent division overflow	*/

	subo	1,shift_cnt,shift_cnt


Lpow_12a:
	ediv	rp_1_hi,s_mant_lo,z_mant_lo	/* s_mant / rp_1 -> z_mant	*/


	emul	z_mant,z_mant,z_sqr_lo		/* Compute z^2			*/

	shlo	1,shift_cnt,shift_cnt		/* Double shift for z_sqr	*/

	lda	Llog2_P3,con_1			/* Compute z^2 * P3		*/
	emul	con_1,z_sqr,P_poly_lo

	shro	shift_cnt,z_sqr,Q_poly		/* Compute Q2 - z^2 * Q4	*/
	shro	Llog2_Q4_shift,Q_poly,Q_poly
	lda	Llog2_Q2,con_2
	subo	Q_poly,con_2,Q_poly
	
	lda	Llog2_P1,con_1

	emul	z_sqr,Q_poly,Q_poly_lo		/* Compute  z^2*Q2 - z^4*Q4	*/

	shro	shift_cnt,P_poly,P_poly
	subo	P_poly,con_1,P_poly

	lda	Llog2_Q0,con_2

	emul	z_mant,P_poly,P_poly_lo		/* Apply "odd" power of z	*/

	shro	shift_cnt,Q_poly,Q_poly
	subo	Q_poly,con_2,Q_poly

	shro	1,shift_cnt,shift_cnt		/* Return to base z shift	*/

	bbs.f	31,P_poly,Lpow_12b		/* J/ no pre-divide norm shift	*/

	addc	P_poly_lo,P_poly_lo,rp_1_lo	/* Try a norm shift		*/
	addc	P_poly,P_poly,rp_1_hi
	cmpobge rp_1_hi,Q_poly,Lpow_12b		/* J/ no pre-divide norm shift	*/

	movl	rp_1_lo,P_poly_lo
	addlda(1,shift_cnt,shift_cnt)

Lpow_12b:
	ediv	Q_poly,P_poly_lo,s_mant_lo	/* P(z2)/Q(z2) -> s_mant (=log)	*/

	subo	2,shift_cnt,shift_cnt		/* Adjust for relative exps	*/


Lpow_13:						/* Single term approx re-ent	*/

	lda	-FP_Bias(s_exp),s_exp		/* Remove bias from exp		*/

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

	shro	shift_cnt,s_mant,s_mant

	shro	neg_exp_bit-neg_z_bit,flags,tmp_1
	xor	flags,tmp_1,tmp_1
	bbs	neg_z_bit,tmp_1,Lpow_18		/* J/ int/frac different sign	*/


	addo	rp_1_hi,s_mant,s_mant
	b	Lpow_20


Lpow_18:
	subo	s_mant,rp_1_hi,s_mant		/* Subtract fraction from int	*/
	bbs.t	31,s_mant,Lpow_20		/* J/ value still normalized	*/

	shlo	1,s_mant,s_mant			/* Shift to normalize		*/
	subo	1,s_exp,s_exp			/* Adjust exponent		*/
	b	Lpow_20


Lpow_19:
	subo	shift_cnt,0,s_exp		/* Leave fraction alone		*/

	chkbit	neg_z_bit,flags			/* Make frac sign -> exp sign	*/
	alterbit neg_exp_bit,flags,flags

Lpow_20:



/* ************************************************************************** *
 *                                                                            *
 *      I n t e r n a l   3 2 - b i t   x   2 4 - b i t   m u l t i p l y     *
 *                                                                            *
 * ************************************************************************** */

/*
 *  log2(s1)   neg_exp_bit in flags   sign
 *                            s_exp   unbiased exponent value
 *                            s_mant  left justified 32-bit mantissa (w/ j bit set)
 *
 *  s2                        s2      IEEE sp format value
 *
 */

	chkbit	31,s2				/* Compute mult result sign	*/
	alterbit neg_exp_bit,0,tmp_1
	xor	flags,tmp_1,flags

	clrbit	31,s2,s2			/* Zero the sign bit		*/

	shro	32-8-1,s2,s2_exp		/* Extract exponent		*/

	shlo	8,s2,s2				/* Create left-justified mant	*/

	setbit	31,s2,s2			/* Set implicit j bit		*/

	cmpobne.t 0,s2_exp,Lpow_24		/* J/ not denormal		*/


	clrbit	31,s2,s2			/* Reverse setting j bit	*/

	scanbit	s2,tmp_2

	subo	tmp_2,31,tmp_2

	shlo	tmp_2,s2,s2			/* Left justify the value	*/

	subo	tmp_2,1,s2_exp			/* Effective exponent		*/

Lpow_24:
	emul	s_mant,s2,x_mant_lo		/* 64 x 53 bit multiply		*/
	addo	s2_exp,s_exp,x_exp		/* Biased exp of multiply	*/
	addo	1,x_exp,x_exp			/* Assume no norm shift		*/

	bbs.t	31,x_mant,Lpow_26		/* J/ normalized		*/

	addc	x_mant_lo,x_mant_lo,x_mant_lo
	subo	1,x_exp,x_exp			/* Reduce exponent		*/
	addc	x_mant,x_mant,x_mant

Lpow_26:
	cmpo	1,0
	addc	x_mant_lo,x_mant_lo,x_mant_lo
	addc	0,x_mant,x_mant

	cmpobne.t 0,x_mant,Lpow_28		/* J/ no carry-out from round	*/

	setbit	31,0,x_mant			/* Result = 1000...000		*/
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
 *                         x_mant  left justified 32-bit mantissa (w/ j bit set)
 *
 */


	lda	FP_Bias+7,con_1

	cmpibl.t x_exp,con_1,Lpow_40		/* J/ no limit testing		*/

	mov	0,g14				/* Zero the bal ret addr reg	*/

	be.f	Lpow_34				/* J/ further limit testing	*/

	bbs	neg_exp_bit,flags,Lpow_32	/* J/ underflow			*/

Lpow_31:						/* Overflow			*/
	callj	__errno_ptr

	lda	ERANGE,r4			/* Set errno to ERANGE		*/
	st	r4,(g0)

	lda	FP_INF << 23,out		/* Return INF			*/

	chkbit	neg_rslt_bit,flags
	alterbit 31,out,out
	ret

Lpow_32:						/* Underflow			*/
	callj	__errno_ptr

	lda	ERANGE,r4
	st	r4,(g0)

	mov	0,out

	chkbit	neg_rslt_bit,flags
	alterbit 31,out,out
	ret

Lpow_34:
	bbc	neg_exp_bit,flags,Lpow_31	/* Overflow if positive		*/

	lda	UNFL_LIMIT,con_1
	cmpobge	x_mant,con_1,Lpow_32		/* J/ underflows even denorm	*/



Lpow_40:
	lda	-FP_Bias+1(x_exp),shift_cnt	/* Left shift for fraction pwr	*/
	bbs.f	31,shift_cnt,Lpow_75		/* J/ magnitude < 1/2		*/

	lda	32,tmp_1
	subo	shift_cnt,tmp_1,tmp_1		/* Right shift			*/

	shro	tmp_1,x_mant,x_exp		/* Integer power of two		*/

/*
 *  Left justify fraction part in x_mant (possibly a shift of zero)
 */

	shlo	shift_cnt,x_mant,x_mant

/*
 *  Adjust for (1) negative values, and for (2) mantissa >= 1/2
 */

	bbc	neg_exp_bit,flags,Lpow_42	/* J/ positive x value		*/

	subc	x_mant,0,x_mant			/* Magnitude of reduced arg	*/
	subc	x_exp,0,x_exp			/* Apply arg sign to 2's power	*/

Lpow_42:

	bbc	31,x_mant,Lpow_44

	subo	x_mant,0,x_mant			/* Magnitude of reduced arg	*/

	cmpo	0,0				/* Insure neg_x_bit set		*/

Lpow_44:
	alterbit neg_x_bit,flags,flags


/*
 *  Normalize fractional power of 2, then decide on evaluation technique
 */

	scanbit	x_mant,tmp_1			/* Compute num of leading 0's	*/

	cmpibge.f 15,tmp_1,Lpow_72		/* Single term or no term appx	*/

	subo	tmp_1,31,shift_cnt		/* Left norm fraction		*/

	shlo	shift_cnt,x_mant,x_mant


/*
 *  Polynomial evaluation algorithm
 */
Lpow_46:						/* re-entry for |x| < 1/2	*/

	emul	x_mant,x_mant,x_sqr_lo		/* Square x for poly eval	*/

	shlo	1,shift_cnt,shift_cnt		/* x^2 shift count		*/

	lda	Lexp2_P3,P_poly

	emul	x_sqr,P_poly,P_poly_lo

	shro	shift_cnt,x_sqr,Q_poly

	shro	Lexp2_Q2_shift,Q_poly,Q_poly

	lda	Lexp2_Q0(Q_poly),Q_poly

	shro	shift_cnt,P_poly,P_poly

	lda	Lexp2_P1(P_poly),P_poly

	emul	x_mant,P_poly,P_poly_lo		/* Apply "odd" power of z	*/

	shro	1,shift_cnt,shift_cnt		/* Return the "x*" shift	*/

	shro	shift_cnt,P_poly,P_poly		/* Align P poly to Q poly	*/

	addc	P_poly,Q_poly,tmp_1		/* Q_poly + P_poly		*/

	subo	P_poly,Q_poly,Q_poly		/* Q_poly - P_poly		*/

	bbs	neg_x_bit,flags,Lpow_52		/* J/ inverted division		*/


/*
 *  Prevent division overflow by shifting divisor or dividend
 */

	shlo	32-1,tmp_1,s_mant_lo
	shro	1,tmp_1,s_mant
	b	Lpow_54

Lpow_52:
	mov	Q_poly,s_mant
	movldar(tmp_1,Q_poly)

Lpow_54:
	ediv	Q_poly,s_mant_lo,s_mant_lo


/*
 *  Construct the result
 */

Lpow_60:
	lda	FP_Bias-1(s_exp),s_exp		/* (-1 for j bit)		*/
	cmpibg.f 0,s_exp,Lpow_62			/* J/ denormalized result	*/

	shlo	23,s_exp,s_exp

	chkbit	32-24-1,s_mant			/* Rounding bit to carry	*/

	shro	32-24,s_mant,s_mant
	addc	s_exp,s_mant,out		/* Mix exp & mant; move		*/

	chkbit	neg_rslt_bit,flags
	alterbit 31,out,out
	ret

Lpow_62:
	subo	s_exp,1+8-1,tmp_1		/* Right shift for denorm	*/
	subo	1,tmp_1,tmp_2			/* Rounding bit			*/

	shro	tmp_1,s_mant,out
	chkbit	tmp_2,s_mant
	addc	0,out,out			/* Round result			*/

	chkbit	neg_rslt_bit,flags
	alterbit 31,out,out
	ret



/*
 *  Estimate exp(x) with 1+x when |x| < 2^-15
 */

Lpow_72:
	lda	LN_2,con_1
	emul	x_mant,con_1,x_mant_lo

	bbs	neg_x_bit,flags,Lpow_73		/* J/ less than 1.0		*/

	shro	1,s_mant,s_mant
	setbit	31,s_mant,s_mant		/* 1 + frac			*/
	b	Lpow_60

Lpow_73:
	subo	s_mant,0,s_mant			/* 1 - frac			*/
	b	Lpow_60


/*
 *  Special handling for |x| < 1/2
 */

Lpow_75:
	lda	FP_Bias-1,con_1

	subo	s_exp,con_1,shift_cnt		/* Equivalent shift_cnt value	*/

	chkbit	neg_exp_bit,flags		/* 2^0 or 2^-1 exp from sign	*/
	alterbit 0,0,tmp_1
	alterbit neg_x_bit,flags,flags
	subo	tmp_1,0,s_exp

	cmpobge.t 15,shift_cnt,Lpow_46		/* J/ use standard sequence	*/

	cmpoble.f 32-7,shift_cnt,Lpow_83		/* J/ result is 1.0		*/

	shro	shift_cnt,x_mant,x_mant		/* Position for 1+x eval	*/
	b	Lpow_72



/*
 *  Mantissa value very close to 1.0 (top 16 bits = 1000...000)
 *
 *      approximate log2(mant) = log2(e)*(mant-1)
 */

Lpow_80:
	scanbit	s_mant,tmp_1			/* Search for MS bit		*/

	bno	Lpow_82				/* J/ mantissa = 1.0 exactly	*/

	subo	tmp_1,31,shift_cnt		/* left shift to norm		*/

	shlo	shift_cnt,s_mant,s_mant		/* Norm the value		*/
	lda	LOG2_e,con_1

	emul	s_mant,con_1,s_mant_lo

	chkbit	neg_x_bit,flags

	subo	1,shift_cnt,shift_cnt
	addc	0,shift_cnt,shift_cnt		/* Compute effective shift cnt	*/

	bbs.t	31,s_mant,Lpow_81		/* J/ normalized		*/

	addc	s_mant_lo,s_mant_lo,s_mant_lo	/* Left shift to normalize	*/
	addlda(1,shift_cnt,shift_cnt)
	addc	s_mant,s_mant,s_mant

Lpow_81:						/* Round result			*/
	chkbit	31,s_mant_lo
	addc	0,s_mant,s_mant
	bbs.t	31,s_mant,Lpow_13		/* No overflow from rounding	*/

	setbit	31,0,s_mant			/* Result = 1000...000		*/
	subo	1,shift_cnt,shift_cnt
	b	Lpow_13


Lpow_82:						/* mant = 1.0			*/
	lda	FP_Bias,con_1
	cmpobe.f s_exp,con_1,Lpow_83		/* mant = 1.0, exp = 0		*/

	mov	0,s_mant			/* Set log frac to 0		*/
	movlda(0,shift_cnt)
	b	Lpow_13


Lpow_83:						/* Result is signed 1.0		*/
	chkbit	neg_rslt_bit,flags

	lda	FP_Bias << 23,out

	alterbit 31,out,out
	ret	




