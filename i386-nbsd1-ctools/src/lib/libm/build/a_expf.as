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

	.file	"a_expf.s"
	.globl	_expf

        .globl  __AFP_NaN_S

        .globl  __errno_ptr            /* returns addr of ERRNO in g0          */


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

#define x_mant		r10
#define	twos_power	r11

#define	x_sqr_lo	r12
#define	x_sqr		r13

#define	P_poly_lo	r14
#define	P_poly		r15

#define	Q_poly_lo	g6
#define	Q_poly		g7

#define	x_sign		g5
#define	x_shift		g4

#define	rp_1_lo		g2
#define	rp_1_hi		g3

#define	step_shift	g1

#define	out		g0


/*  Polynomial approximation to exp for the interval  [ -ln(2)/2, ln(2)/2 ]	*/

/*  Constants and algorithms adapted from Hart et al, EXPBC 1321		*/

#define	Q0	0xA68BBB27	/* 20.818228058972077  */
#define	Q2	0x08000000	/*  1.000000000000000  */

#define	P1	0x39B86B1B	/*  7.215048037358433  */
#define	P3	0x00762B33	/*  0.057699581512902  */


#define	LN_2		0xB17217F7

#define	EXPF_min	0xC2CFF1B4	/* -103.972077 */
#define	EXPF_max	0x42B17217	/*  +88.722832 */

#define	LN_2_half_mag	0x7D62E42F	/* (FP LN(2)/2) << 1 */

#define	INV_LN_2	0xB8AA3B29	/* 1.44269504 */


#define	FP_NEG_INF	0xFF800000
#define	FP_POS_INF	0x7F800000
#define	FP_NaN		0xFFE00000



	.text
	.link_pix

#if	defined(CA_optim)
	.align	5
#else
	.align	4
#endif


_expf:
	shlo	8,s1,s1_mant			/* Create left-justified mant	*/
	lda	EXPF_min,con_1

	cmpo	con_1,s1			/* s1 <= con_1 if in range	*/
	shlo1(s1,tmp_3)				/* Left justify exp field	*/

	shro	24,tmp_3,s1_exp			/* Biased, right justif exp	*/
	lda	EXPF_max,con_1

	concmpi	s1,con_1			/* s1 >= con_1 if in range	*/
	lda	LN_2_half_mag,con_1

	setbit	31,s1_mant,s1_mant		/* Set j bit			*/
	bne	Lexpf_80				/* J/ out of range (or NaN)	*/

	lda	INV_LN_2,tmp_1

	emul	s1_mant,tmp_1,rp_1_lo		/* Compute power of 2 factor	*/

	cmpoble.f tmp_3,con_1,Lexpf_50		/* magnitude < (ln 2)/2		*/

	lda	FP_Bias+30,tmp_1

	subo	s1_exp,tmp_1,tmp_1		/* Right shift for twos power	*/
	lda	32,tmp_3

	subo	tmp_1,tmp_3,tmp_3		/* Left shift for fraction pwr	*/

	shro	tmp_1,rp_1_hi,twos_power	/* Integer power of two		*/

	shlo	tmp_3,rp_1_hi,x_mant		/* Build fract part in x_mant	*/

	shro	31,x_mant,x_sign		/* Rounding bit -> sign bit	*/

	addo	twos_power,x_sign,twos_power	/* Bump power w/ rounding bit	*/

	shro	31,s1,tmp_2			/* Incoming argument sign	*/

	subo	x_sign,0,tmp_1

	xor	tmp_2,x_sign,x_sign		/* Apply arg sign to reduction	*/

	subo	tmp_2,0,tmp_2			/* Apply arg sign to 2's power	*/
	xor	tmp_2,twos_power,twos_power
	subo	tmp_2,twos_power,twos_power

	xor	tmp_1,x_mant,x_mant		/* Magnitude of reduced arg	*/
	subo	tmp_1,x_mant,x_mant

	scanbit	x_mant,x_shift			/* Compute num of leading 0's	*/

	subo	x_shift,31,x_shift		/* Left norm fraction		*/
	shlo	x_shift,x_mant,x_mant

Lexpf_10:
	emul	x_mant,x_mant,x_sqr_lo		/* Square fraction bits		*/

	cmpoble.f 24,x_shift,Lexpf_30		/* J/ 2^reduced_arg ~= 1.0	*/

	subo	x_sign,twos_power,twos_power

	cmpoble.f 12,x_shift,Lexpf_20		/* J/ single term approx	*/

	lda	P3,P_poly			/* Begin computing P poly	*/

	emul	x_sqr,P_poly,P_poly_lo		/*         P3*x^2		*/

	shlo	1,x_shift,step_shift

	addo	step_shift,5,tmp_3		/*  shift for Q2*x^2		*/
	lda	P1,con_1

	shro	step_shift,P_poly,P_poly

	addo	con_1,P_poly,P_poly		/*  P1   + P3*x^2		*/

	emul	x_mant,P_poly,P_poly_lo		/*  P1*x + P3*x^3		*/

	shro	tmp_3,x_sqr,Q_poly		/*         Q2*x^2		*/
	lda	Q0,con_1

	addo	Q_poly,con_1,Q_poly		/*  Q0   + Q2*x^2		*/

	shro	x_shift,P_poly,P_poly		/* Align P_poly with Q_poly	*/

	cmpo	x_sign,0

	addo	P_poly,Q_poly,tmp_1		/* Q_poly + P_poly		*/

	subo	P_poly,Q_poly,Q_poly		/* Q_poly - P_poly		*/

	bne	Lexpf_12				/* J/ < 1.0			*/

	shro	1,tmp_1,P_poly			/* Prevent division overflow	*/

	ediv	Q_poly,P_poly_lo,P_poly_lo
	b	Lexpf_14

Lexpf_12:
	ediv	tmp_1,Q_poly_lo,P_poly_lo

Lexpf_14:
	lda	FP_Bias-1(twos_power),twos_power	/* (-1 for j bit)	*/
	cmpibg.f 0,twos_power,Lexpf_16		/* J/ denormal result		*/

	shlo	23,twos_power,twos_power

	chkbit	7,P_poly			/* Rounding bit to carry	*/
	shro	8,P_poly,P_poly			/* Position mantissa		*/
	addc	twos_power,P_poly,out		/* Mix exp & mant; round; mov	*/
	ret

Lexpf_16:
	subo	twos_power,1+8-1,tmp_1		/* Right shift to denom		*/
	addlda(32-1-8+1,twos_power,tmp_2)	/* Left shift for rounding	*/

	shro	tmp_1,P_poly,out		/* Denormalize mantissa		*/
	shlo	tmp_2,P_poly,tmp_2		/* Rounding sequence		*/
	addc	tmp_2,tmp_2,tmp_2
	addc	0,out,out
	ret


/*
 *  Exp estimate w/ a single factor
 */

Lexpf_20:
	shro	x_shift,x_mant,x_mant
	lda	(LN_2 >> 1) & 0x7fffffff,con_1

	emul	x_mant,con_1,P_poly_lo

	cmpobne	0,x_sign,Lexpf_22		/* J/ less than 1.0		*/

	setbit	31,P_poly,P_poly		/* 1 + frac			*/
	b	Lexpf_14

Lexpf_22:
	subo	P_poly,0,P_poly			/* 1 - frac			*/
	shlo	1,P_poly,P_poly			/* Normalize			*/
	b	Lexpf_14


/*
 *  Exp estimate an exact power of two
 */

Lexpf_30:
	setbit	31,0,P_poly			/* 1.0 result			*/
	b	Lexpf_14


/*
 *  Magnitude of argument < (ln 2)/2
 */

Lexpf_50:
	chkbit	31,rp_1_hi
	lda	FP_Bias-2,con_1

	shro	31,s1,x_sign			/* Sign of argument		*/
	movldar(rp_1_hi,x_mant)			/* Scaled mant -> approx mant	*/

	subo	s1_exp,con_1,x_shift		/* Equivalent x_shift value	*/
	movlda(0,twos_power)			/* Default to 2^0 exponent	*/

	bo	Lexpf_10				/* Resume standard flow		*/

	shlo	1,x_mant,x_mant			/* Normalize scaled value	*/
	addo	1,x_shift,x_shift
	b	Lexpf_10
	


/*
 *  NaN, too large, or too small
 */

Lexpf_80:
	lda	FP_INF << 24,con_1

	cmpobg.f tmp_3,con_1,Lexpf_90		/* J/ NaN argument		*/

	bbs.f	31,s1,Lexpf_85			/* J/ negative argument		*/

        callj   __errno_ptr                    /* returns addr of ERRNO in g0  */
        ldconst ERANGE,r8
        st      r8,(g0)

	lda	FP_INF << 23,out		/* Return +INF			*/
	ret

Lexpf_85:
        callj   __errno_ptr                    /* returns addr of ERRNO in g0  */
        ldconst ERANGE,r8
        st      r8,(g0)

	mov	0,out				/* Return +0.0			*/
	ret


Lexpf_90:
	mov	0,g1			/* Fakes second parameter		*/
	b	__AFP_NaN_S		/* Use non-signaling NaN handler	*/
