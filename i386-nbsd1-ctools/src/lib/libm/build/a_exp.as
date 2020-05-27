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

	.file	"a_exp.s"
	.globl	_exp

	.globl	__AFP_NaN_D

	.globl	__errno_ptr		/* returns addr of ERRNO in g0		*/


#define	ERANGE	34


#define	DP_Bias         0x3ff
#define	DP_INF          0x7ff



/* Register Name Equates */

#define	s1_lo		g0
#define	s1_hi		g1

#define	s1_exp		r3
#define	s1_mant_lo	r4
#define	s1_mant_hi	r5

#define x_mant_lo	s1_mant_lo
#define	x_mant_hi	s1_mant_hi
#define	twos_pwr	s1_exp

#define	x_sqr_lo	r6
#define	x_sqr_hi	r7

#define	con_lo	 	r8
#define	con_hi	 	r9

#define	rp_1_lo		r10
#define	rp_1_hi		r11

#define	P_poly_lo	r12
#define	P_poly_hi	r13

#define	Q_poly_lo	r14
#define	Q_poly_hi	r15

#define	rp_2_lo		g2
#define	rp_2_hi		g3

#define	tmp_1		g4
#define	tmp_2		g5

#define	x_sign		g6
#define	shift_cnt	g7

#define	out_lo		g0
#define	out_hi		g1

#define	mem_ptr		out_lo


#define	LN_2_hi		0xB17217F7
#define	LN_2_lo		0xD1CF79AC

#define	EXP_min_hi	0xC0874910	/* -1075 * ln 2  (about -744.44)	*/
#define	EXP_min_lo	0xD52D3051

#define	EXP_max_hi	0x40862E42	/* +1024 * ln 2  (about +709.78)	*/
#define	EXP_max_lo	0xFEFA39EF

#define	LN_2_half_mag	0x7FAC5C86	/* HI_WORD(DP LN(2)/2) << 1		*/

#define	INV_LN_2_hi	0xB8AA3B29	/* 1.44269 50408 88963 40736		*/
#define	INV_LN_2_lo	0x5C17F0BC


#define	DP_NEG_INF_hi	0xFFF00000
#define	DP_NEG_INF_lo	0x00000000

#define	DP_POS_INF_hi	0x7FF00000
#define	DP_POS_INF_lo	0x00000000

#define	DP_NaN_hi	0xFFFC0000
#define	DP_NaN_lo	0x00000000



	.text
	.link_pix


/*  Polynomial approximation to exp for the interval  [ -ln(2)/2, ln(2)/2 ]	*/

/*  Constants and algorithms adapted from Hart et al, EXPBC 1323		*/

	.align	4

Lpoly_cons:
	.word	0x1354B9A9,0x00002F4C	/* P5     0.02309432127295385660455 */
	.word	0xECFFF65A,0x07496D04	/* Q2   233.17823205143103579679705 */

	.word	0xE4ABBBB2,0x00A19D14	/* P3    20.20170000695312603946477 */
	.word	0xA6FD8545,0x8880B598	/* Q0  4368.08867006741698646914202 */

	.word	0x8EE7A432,0x2F4EE9D3	/* P1  1513.86417304653561981057161 */

/*	.word	0x00000000,0x00080000	x* Q4     1.00000000000000000000000 */


#if	defined(CA_optim)
	.align	5
#else
	.align	4
#endif


_exp:

/*
 *  Create left-justified mantissa
 */

#if	defined(CA_optim)
	eshro	32-11,s1_lo,s1_mant_hi
#else
	shlo	11,s1_hi,s1_mant_hi
	shro	32-11,s1_lo,tmp_1
	or	s1_mant_hi,tmp_1,s1_mant_hi
#endif
	shlo	11,s1_lo,s1_mant_lo


	lda	EXP_min_hi,con_hi

	cmpobge.f s1_hi,con_hi,Lexp_80		/* J/ may underflow to 0.0	*/

	lda	EXP_max_hi,con_hi

	cmpibge.f s1_hi,con_hi,Lexp_85		/* J/ may overflow to +INF	*/

Lexp_05:						/* Return point if in range	*/
	shlo	1,s1_hi,tmp_2			/* Left justify exp field	*/

	shro	20+1,tmp_2,s1_exp		/* Biased, right justif exp	*/

	setbit	31,s1_mant_hi,s1_mant_hi	/* Set j bit			*/
	lda	INV_LN_2_lo,con_lo

	emul	s1_mant_hi,con_lo,rp_1_lo	/* Compute power of 2 factor	*/

	lda	DP_Bias+30,tmp_1

	subo	s1_exp,tmp_1,tmp_1		/* Right shift for twos power	*/

	lda	INV_LN_2_hi,con_lo

	emul	s1_mant_lo,con_lo,rp_2_lo

	ldconst	32,shift_cnt

	subo	tmp_1,shift_cnt,shift_cnt	/* Left shift for fraction pwr	*/

	emul	s1_mant_hi,con_lo,x_mant_lo

	addc	rp_1_lo,rp_2_lo,rp_1_lo		/* Combine mid partial prods	*/
	addc	rp_1_hi,rp_2_hi,rp_1_lo
	addc	0,0,rp_1_hi

	addc	rp_1_lo,x_mant_lo,x_mant_lo	/* Finish comb partial prods	*/
	addc	rp_1_hi,x_mant_hi,x_mant_hi

	lda	LN_2_half_mag,con_lo
	cmpoble.f tmp_2,con_lo,Lexp_40		/* magnitude < (ln 2)/2		*/


	shro	tmp_1,x_mant_hi,twos_pwr	/* Integer power of two		*/

/*
 *  Left justify fraction part in x_mant (possibly a shift of zero)
 */

	shro	tmp_1,x_mant_lo,tmp_1
	shlo	shift_cnt,x_mant_hi,x_mant_hi
	or	tmp_1,x_mant_hi,x_mant_hi

	shlo	shift_cnt,x_mant_lo,x_mant_lo


	shro	31,x_mant_hi,x_sign		/* Rounding bit -> sign bit	*/

	addo	twos_pwr,x_sign,twos_pwr	/* Bump twos power as req'd	*/

	subo	x_sign,0,tmp_1

	xor	x_mant_hi,tmp_1,x_mant_hi	/* Magnitude of reduced arg	*/
	xor	tmp_1,x_mant_lo,x_mant_lo
	subc	tmp_1,x_mant_lo,x_mant_lo
	subc	tmp_1,x_mant_hi,x_mant_hi

	shro	31,s1_hi,tmp_2			/* Incoming argument sign	*/
	xor	x_sign,tmp_2,x_sign		/* Apply arg sign to reduction	*/

	subo	tmp_2,0,tmp_2			/* Apply arg sign to 2's power	*/
	xor	twos_pwr,tmp_2,twos_pwr
	subo	tmp_2,twos_pwr,twos_pwr

	scanbit	x_mant_hi,tmp_1			/* Compute num of leading 0's	*/

	cmpibge.f 3,tmp_1,Lexp_30		/* Single term or no term appx	*/

	subo	tmp_1,31,shift_cnt		/* Left norm fraction		*/
	addlda(1,tmp_1,tmp_1)			/* Right shift for wd -> wd	*/

	shro	tmp_1,x_mant_lo,tmp_1		/* 2-wd left shift		*/ 
	shlo	shift_cnt,x_mant_hi,x_mant_hi	/* (0 shift allowed)		*/
	or	tmp_1,x_mant_hi,x_mant_hi
	shlo	shift_cnt,x_mant_lo,x_mant_lo

/*
 *  Square fraction bits, prepare for poly evaluation
 */
Lexp_08:
	emul	x_mant_hi,x_mant_lo,rp_1_lo

	lda	Lpoly_cons-.-8(ip),mem_ptr

	ldq	(mem_ptr),P_poly_lo		/* Fetch P5 and Q2		*/

	emul	x_mant_hi,x_mant_hi,x_sqr_lo

	shlo	1,shift_cnt,shift_cnt		/* x^2 shift count		*/

	addc	rp_1_lo,rp_1_lo,rp_1_lo		/* Double middle product	*/
	addc	rp_1_hi,rp_1_hi,rp_1_lo
	addc	0,0,rp_1_hi

	addc	rp_1_lo,x_sqr_lo,x_sqr_lo	/* Combine partial products	*/
	addc	rp_1_hi,x_sqr_hi,x_sqr_hi

	cmpobl.f 31,shift_cnt,Lexp_20		/* J/ single X^2 term approx	*/

	emul	x_sqr_lo,P_poly_hi,rp_1_lo

#if	defined(CA_optim)
	eshro	13,x_sqr_lo,con_lo		/* "multiply" X^2 by Q4		*/
#else
	shlo	32-13,x_sqr_hi,tmp_2
	shro	13,x_sqr_lo,con_lo
	or	tmp_2,con_lo,con_lo
#endif
	shro	13,x_sqr_hi,con_hi

#if	!defined(CA_optim)
	lda	32,tmp_1
	subo	shift_cnt,tmp_1,tmp_1
#endif

	emul	x_sqr_hi,P_poly_lo,rp_2_lo

#if	defined(CA_optim)
	eshro	shift_cnt,con_lo,con_lo		/* Align x^2*Q4 to Q2		*/
#else
	shlo	tmp_1,con_hi,tmp_2
	shro	shift_cnt,con_lo,con_lo
	or	tmp_2,con_lo,con_lo
#endif
	shro	shift_cnt,con_hi,con_hi

	addc	Q_poly_lo,con_lo,Q_poly_lo	/*      Q2     + Q4*x^2		*/
	addc	Q_poly_hi,con_hi,Q_poly_hi

	addo	8+8,mem_ptr,mem_ptr
	ldl	(mem_ptr),con_lo		/* Fetch P3			*/

	emul	x_sqr_hi,P_poly_hi,P_poly_lo

	addc	rp_1_lo,rp_2_lo,rp_2_lo
	addc	rp_1_hi,rp_2_hi,rp_2_lo
	addc	0,0,rp_2_hi

	emul	x_sqr_lo,Q_poly_hi,rp_1_lo	/* Begin multiplying by x^2	*/

	addc	P_poly_lo,rp_2_lo,P_poly_lo
	addc	P_poly_hi,rp_2_hi,P_poly_hi

#if	defined(CA_optim)
	eshro	shift_cnt,P_poly_lo,P_poly_lo	/* Align x^2*P5 to P3		*/
#else
	shlo	tmp_1,P_poly_hi,tmp_2
	shro	shift_cnt,P_poly_lo,P_poly_lo
	or	tmp_2,P_poly_lo,P_poly_lo
#endif
	addc	con_lo,P_poly_lo,P_poly_lo	/*        P3     + P5*x^2	*/

	emul	x_sqr_hi,Q_poly_lo,rp_2_lo

	shro	shift_cnt,P_poly_hi,P_poly_hi
	addc	con_hi,P_poly_hi,P_poly_hi

	addo	8,mem_ptr,mem_ptr
	ldl	(mem_ptr),con_lo		/* Fetch Q0			*/

	emul	x_sqr_hi,Q_poly_hi,Q_poly_lo

	addc	rp_1_lo,rp_2_lo,rp_2_lo
	addc	rp_1_hi,rp_2_hi,rp_2_lo
	addc	0,0,rp_2_hi

	emul	x_sqr_lo,P_poly_hi,rp_1_lo

	addc	Q_poly_lo,rp_2_lo,Q_poly_lo	/* Finish x^2 multiplication	*/
	addc	Q_poly_hi,rp_2_hi,Q_poly_hi

#if	defined(CA_optim)
	eshro	shift_cnt,Q_poly_lo,Q_poly_lo	/* Align LQ_poly to Q0		*/
#else
	shlo	tmp_1,Q_poly_hi,tmp_2
	shro	shift_cnt,Q_poly_lo,Q_poly_lo
	or	tmp_2,Q_poly_lo,Q_poly_lo
#endif
	addc	con_lo,Q_poly_lo,Q_poly_lo	/* Q0   + Q2*x^2 + Q4*x^4	*/

	emul	x_sqr_hi,P_poly_lo,rp_2_lo

	shro	shift_cnt,Q_poly_hi,Q_poly_hi
	addc	con_hi,Q_poly_hi,Q_poly_hi

	addo	8,mem_ptr,mem_ptr
	ldl	(mem_ptr),con_lo		/* Fetch P1			*/

	emul	x_sqr_hi,P_poly_hi,P_poly_lo

	addc	rp_1_lo,rp_2_lo,rp_2_lo
	addc	rp_1_hi,rp_2_hi,rp_2_lo
	addc	0,0,rp_2_hi

	addc	P_poly_lo,rp_2_lo,P_poly_lo
	addc	P_poly_hi,rp_2_hi,P_poly_hi

#if	defined(CA_optim)
	eshro	shift_cnt,P_poly_lo,P_poly_lo	/* Align P poly to P1		*/
#else
	shlo	tmp_1,P_poly_hi,tmp_2
	shro	shift_cnt,P_poly_lo,P_poly_lo
	or	tmp_2,P_poly_lo,P_poly_lo
#endif
	addc	con_lo,P_poly_lo,P_poly_lo	/* P1   + P3*x^2 + P5*x^4	*/

	emul	x_mant_hi,P_poly_lo,rp_1_lo

	shro	shift_cnt,P_poly_hi,P_poly_hi
	addc	con_hi,P_poly_hi,P_poly_hi

	emul	x_mant_lo,P_poly_hi,rp_2_lo

Lexp_10:
	emul	x_mant_hi,P_poly_hi,P_poly_lo

	shro	1,shift_cnt,shift_cnt		/* Return the "x*" shift	*/

	addc	rp_1_lo,rp_2_lo,rp_2_lo
#if	!defined(CA_optim)
	shlo	5,1,tmp_1
#endif
	addc	rp_1_hi,rp_2_hi,rp_2_lo
#if	!defined(CA_optim)
	subo	shift_cnt,tmp_1,tmp_1
#endif
	addc	0,0,rp_2_hi

	addc	P_poly_lo,rp_2_lo,P_poly_lo
	addc	P_poly_hi,rp_2_hi,P_poly_hi

#if	defined(CA_optim)
	eshro	shift_cnt,P_poly_lo,P_poly_lo	/* Align P poly to Q poly	*/
#else
	shlo	tmp_1,P_poly_hi,tmp_2
	shro	shift_cnt,P_poly_lo,P_poly_lo
	or	tmp_2,P_poly_lo,P_poly_lo
#endif
	shro	shift_cnt,P_poly_hi,P_poly_hi

	addc	P_poly_lo,Q_poly_lo,rp_1_lo	/* LQ_poly + LP_poly		*/
	addc	P_poly_hi,Q_poly_hi,rp_1_hi

	subc	P_poly_lo,Q_poly_lo,Q_poly_lo	/* LQ_poly - LP_poly		*/
	subc	P_poly_hi,Q_poly_hi,Q_poly_hi

	cmpobne	0,x_sign,Lexp_12			/* J/ < 1.0			*/


/*
 *  Prevent division overflow by shifting divisor or dividend
 */

	bbc.f	31,Q_poly_hi,Lexp_11		/* J/ can left shift Q poly	*/

#if	defined(CA_optim)
	eshro	1,rp_1_lo,P_poly_lo
#else
	shro	1,rp_1_lo,P_poly_lo
	shlo	32-1,rp_1_hi,tmp_2
	or	P_poly_lo,tmp_2,P_poly_lo
#endif
	shro	1,rp_1_hi,P_poly_hi
	b	Lexp_14

Lexp_11:
	addc	Q_poly_lo,Q_poly_lo,Q_poly_lo
	addc	Q_poly_hi,Q_poly_hi,Q_poly_hi
	movl	rp_1_lo,P_poly_lo
	b	Lexp_14

Lexp_12:
	movl	Q_poly_lo,P_poly_lo
	movl	rp_1_lo,Q_poly_lo

Lexp_14:
	ediv	Q_poly_hi,P_poly_lo,P_poly_lo
	mov	P_poly_hi,tmp_1			/* Division result		*/
	movldar(P_poly_lo,P_poly_hi)		/* Prepare for next div step	*/
/*	mov	0,P_poly_lo	*/		/* (don't care about ls bit	*/
	ediv	Q_poly_hi,P_poly_lo,P_poly_lo
	mov	P_poly_hi,P_poly_lo		/* 64-bit "A/B" result		*/
	movldar(tmp_1,P_poly_hi)

	shro	1,Q_poly_lo,rp_2_hi		/* Prepare for C/B		*/
	shlo	31,Q_poly_lo,rp_2_lo
	ediv	Q_poly_hi,rp_2_lo,rp_2_lo

	emul	rp_2_hi,P_poly_hi,rp_2_lo	/*  C/B * A/B			*/

	addc	rp_2_lo,rp_2_lo,rp_2_lo		/*  align to A/B		*/
	addc	rp_2_hi,rp_2_hi,rp_2_lo
	addc	0,0,rp_2_hi

	subc	rp_2_lo,P_poly_lo,P_poly_lo	/* A/B - (C/B * A/B)		*/
	subc	rp_2_hi,P_poly_hi,P_poly_hi

/*
 *  Construct the result
 */

Lexp_18:
	subo	x_sign,twos_pwr,twos_pwr	/* Reduce exp for neg poly	*/
	lda	DP_Bias-1(twos_pwr),twos_pwr	/* (-1 for j bit)		*/
	cmpibg.f 0,twos_pwr,Lexp_19		/* J/ denormalized result	*/

	shlo	20,twos_pwr,twos_pwr

	chkbit	64-53-1,P_poly_lo		/* Rounding bit to carry	*/

#if	defined(CA_optim)
	eshro	64-53,P_poly_lo,P_poly_lo
#else
	shro	64-53,P_poly_lo,P_poly_lo
	shlo	32-(64-53),P_poly_hi,tmp_1
	or	P_poly_lo,tmp_1,P_poly_lo
#endif
	addc	0,P_poly_lo,out_lo		/* Start rounding		*/

	shro	64-53,P_poly_hi,P_poly_hi
	addc	twos_pwr,P_poly_hi,out_hi	/* Mix exp & mant; move		*/
	ret

Lexp_19:
	subo	twos_pwr,12-1,tmp_1		/* Right shift for hi wd denorm	*/
	addo	twos_pwr,32-12+1,tmp_2		/* Left shift wd -> wd xfer	*/
	cmpobl	31,tmp_1,Lexp_19b		/* Top word of denorm is zero	*/

#if	defined(CA_optim)
	eshro	tmp_1,P_poly_lo,out_lo		/* Shift to denormalize		*/
#else
	shro	tmp_1,P_poly_lo,out_lo		/* Shift to denormalize		*/
	shlo	tmp_2,P_poly_hi,con_lo
	or	out_lo,con_lo,out_lo
#endif
	shro	tmp_1,P_poly_hi,out_hi

	shlo	tmp_2,P_poly_lo,con_lo		/* MS bit of tmp is round bit	*/
	b	Lexp_19d

Lexp_19b:
	ldconst	32,con_lo

	subo	con_lo,tmp_1,tmp_1		/* Reduce right shift count	*/
	addo	con_lo,tmp_2,tmp_2		/* Adjust wd -> wd shift count	*/

	shro	tmp_1,P_poly_hi,out_lo
	movlda(0,out_hi)
	shlo	tmp_2,P_poly_hi,con_lo

Lexp_19d:
	addc	con_lo,con_lo,con_lo		/* Rounding bit to carry	*/
	addc	0,out_lo,out_lo			/* Round the result		*/
	addc	0,out_hi,out_hi
	ret



/*
 *  Exp estimate w/ shorter poly's (Q = Q0 + Q2*x^2; P = P1*x)
 */

Lexp_20:
	ldl	32(mem_ptr),P_poly_lo		/* Fetch P1 into LP_poly		*/

	clrbit	5,shift_cnt,tmp_1		/* Subtract 32 from shift_cnt	*/

	ldl	24(mem_ptr),con_lo		/* Fetch Q0			*/

	emul	x_sqr_hi,Q_poly_hi,Q_poly_lo	/* Q2*x^2			*/

	emul	x_mant_lo,P_poly_hi,rp_1_lo	/* Begin computing P1*x		*/

	shro	tmp_1,Q_poly_hi,Q_poly_lo	/* Align Q2*x^2 to Q0		*/

	emul	x_mant_hi,P_poly_lo,rp_2_lo

	addc	con_lo,Q_poly_lo,Q_poly_lo	/* Compute Q0 + Q2*x^2		*/
	addc	con_hi,0,Q_poly_hi
	b	Lexp_10


/*
 *  Estimate exp(x) with 1+x when |x| < 2^-28
 */

Lexp_30:
	lda	LN_2_hi,con_hi
	emul	x_mant_lo,con_hi,rp_1_lo
	lda	LN_2_lo,con_lo
	emul	x_mant_hi,con_lo,rp_2_lo
	emul	x_mant_hi,con_hi,P_poly_lo

	addc	rp_1_lo,rp_2_lo,rp_1_lo
	addc	rp_1_hi,rp_2_hi,rp_1_lo
	addc	0,0,rp_1_hi

	addc	P_poly_lo,rp_1_lo,P_poly_lo
	addc	P_poly_hi,rp_1_hi,P_poly_hi

	cmpobne	0,x_sign,Lexp_32			/* J/ less than 1.0		*/

#if	defined(CA_optim)
	eshro	1,P_poly_lo,P_poly_lo		/* Shift for 1.0+ below		*/
#else
	shro	1,P_poly_lo,P_poly_lo		/* Shift for 1.0+ below		*/
	shlo	31,P_poly_hi,tmp_1
	or	P_poly_lo,tmp_1,P_poly_lo
#endif
	shro	1,P_poly_hi,P_poly_hi

	setbit	31,P_poly_hi,P_poly_hi		/* 1 + frac			*/
	b	Lexp_18

Lexp_32:
	subc	P_poly_lo,0,P_poly_lo		/* 1 - frac			*/
	subc	P_poly_hi,0,P_poly_hi
	b	Lexp_18


/*
 *  Magnitude of argument < (ln 2)/2
 */

Lexp_40:
	shro	31,s1_hi,x_sign			/* Sign of argument		*/
	lda	DP_Bias-2,con_hi

	subo	s1_exp,con_hi,shift_cnt		/* Equivalent shift_cnt value	*/
	movlda(0,twos_pwr)			/* Default to 2^0 exponent	*/

	cmpoble.f 28,shift_cnt,Lexp_45		/* J/ escape for one term aprx	*/

	bbs.f	31,x_mant_hi,Lexp_08		/* J/ normalized		*/

	addc	x_mant_lo,x_mant_lo,x_mant_lo	/* Left shift to normalize	*/
	addc	x_mant_hi,x_mant_hi,x_mant_hi

	addlda(1,shift_cnt,shift_cnt)
	b	Lexp_08


Lexp_45:						/* bin pt align for 1+x aprx	*/
	addo	1,shift_cnt,shift_cnt		/* Adjust for binary point	*/
						/* Before undoing scaling	*/

	cmpobl.f 31,shift_cnt,Lexp_47		/* J/ at least one word shift	*/

#if	defined(CA_optim)
	eshro	shift_cnt,x_mant_lo,x_mant_lo
#else
	shlo	5,1,tmp_1
	subo	shift_cnt,tmp_1,tmp_1
	shro	shift_cnt,x_mant_lo,x_mant_lo
	shlo	tmp_1,x_mant_hi,tmp_1
	or	x_mant_lo,tmp_1,x_mant_lo
#endif
	shro	shift_cnt,x_mant_hi,x_mant_hi
	b	Lexp_30

Lexp_47:
	shlo	5,1,tmp_1
	subo	tmp_1,shift_cnt,tmp_1
	shro	tmp_1,x_mant_hi,x_mant_lo
	movlda(0,x_mant_hi)
	b	Lexp_30


/*
 *  NaN or too small
 */

Lexp_80:
	be	Lexp_82				/* Borderline underflow		*/

	cmpo	1,0
	subo	1,0,tmp_1
	addc	s1_lo,tmp_1,tmp_1		/* Carry set if _lo <> 0	*/
	addc	s1_hi,s1_hi,tmp_1		/* Special NaN tester		*/
	lda	DP_INF << 21, con_hi
	cmpobg.f tmp_1,con_hi,Lexp_90		/* J/ NaN			*/

Lexp_81:
	callj	__errno_ptr			/* returns addr of ERRNO in g0	*/
	ldconst	ERANGE,r8
	st	r8,(g0)

	movl	0,out_lo
	ret

Lexp_82:
	lda	EXP_min_lo,tmp_1
	cmpoble.t s1_lo,tmp_1,Lexp_05		/* J/ no underflow		*/
	b	Lexp_81


/*
 *  NaN or too large
 */

Lexp_85:
	be	Lexp_87				/* Borderline overflow		*/

	cmpo	1,0
	subo	1,0,tmp_1
	addc	s1_lo,tmp_1,tmp_1		/* Carry set if _lo <> 0	*/
	addc	s1_hi,s1_hi,tmp_1		/* Special NaN tester		*/
	lda	DP_INF << 21, con_hi
	cmpobg.f tmp_1,con_hi,Lexp_90		/* J/ NaN			*/

Lexp_86:
	callj	__errno_ptr			/* returns addr of ERRNO in g0	*/
	ldconst	ERANGE,r8
	st	r8,(g0)

	mov	0,out_lo			/* Return +INF			*/
	lda	DP_POS_INF_hi,out_hi
	ret

Lexp_87:
	lda	EXP_max_lo,tmp_1
	cmpoble.t s1_lo,tmp_1,Lexp_05		/* J/ no overflow		*/
	b	Lexp_86


/*
 *  NaN handling
 */

Lexp_90:
	movl	0,g2				/* Fake second parameter	*/
	b	__AFP_NaN_D			/* Non-signaling NaN handler	*/

	ret
