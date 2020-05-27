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

	.file	"a_log.s"
	.globl	_log

        .globl  __errno_ptr            /* returns addr of ERRNO in g0          */


#define EDOM    33
#define ERANGE  34


#define	DP_Bias         0x3ff
#define	DP_INF          0x7ff


/* Register Name Equates */

#define	s1_lo		g0
#define	s1_hi		g1

#define	s1_exp		r3
#define	z_mant_lo	r4
#define	z_mant_hi	r5

#define	tmp_1		r6
#define	tmp_2		r7
#define	tmp_3		r8

#define	s1_flags	r9

#define poly_lo		r10
#define	poly_hi		r11

#define	z_sqr_lo	r12
#define	z_sqr_hi	r13

#define	rp_1_lo		r14
#define	rp_1_hi		r15

#define	rp_2_lo		g6
#define	rp_2_hi		g7

#define	z_shift		g4
#define	step_shift	g5

#define	con_lo		g2
#define	con_hi		g3

#define	out_lo		g0
#define	out_hi		g1



	.text
	.link_pix

	.align	3

/*  Polynomial approximation to LOGe for the interval  [ 1.0, sqrt(2) ]	*/

/*  Constants and algorithms proprietary to Kulus Research, Portland OR  */
/*  Constants and algorithms non-exclusively licensed to Intel Corp	 */

Lpoly_table:
	.word	0xE76F9F7A,0x0AD8CB8D	/* P13 +0.16948 21248 8             E+0 << 62 */ 
	.word	0xFCE9E4CC,0x0B975D9B	/* P11 +0.18111 36267 967           E+0 << 62 */ 
	.word	0x041BB349,0x0E3926B6	/* P9  +0.22223 82333 2791          E+0 << 62 */ 
	.word	0xD8CCC64D,0x124923C1	/* P7  +0.28571 40915 90488 9       E+0 << 62 */ 
	.word	0xE51D7F6D,0x1999999A	/* P5  +0.40000 00012 06045 365     E+0 << 62 */ 
	.word	0xA9C268C1,0x2AAAAAAA	/* P3  +0.66666 66666 63366 0894    E+0 << 62 */ 
	.word	0x00002F05,0x80000000	/* P1  +0.20000 00000 00000 26100 7 E+1 << 62 */ 
	.set	Lpoly_len,(.-Lpoly_table) >> 3

Lpoly_table_2:
	.word	0xD89D89D9,0x09D89D89	/* P13 2/13  */
	.word	0x2E8BA2E9,0x0BA2E8BA	/* P11 2/11  */
	.word	0x38E38E39,0x0E38E38E	/* P9  2/9   */
	.word	0x49249249,0x12492492	/* P7  2/7   */
	.word	0x99999999,0x19999999	/* P5  2/5   */
	.word	0xAAAAAAAB,0x2AAAAAAA	/* P3  2/3   */
	.word	0x00000000,0x80000000	/* P1  2/1   */

#define	one_term_P3	0x2AAAAAAB
#define	one_term_P1	0x80000000

#define	SQRT_2_NO_j	0x6A09E668

#define	ln_2_hi		0xB17217F7
#define	ln_2_mid	0xD1CF79AB
#define	ln_2_lo		0xC9E3B398


/*  (DP_Bias+52) * ln 2  */

#define	bias_int	0x000002E9
#define	bias_frac_hi	0x221AA5A6
#define	bias_frac_lo	0x0A3BEC61


#define	DP_NEG_INF_hi	0xFFF00000
#define	DP_QNaN_hi	0xFFF80000


#if	defined(CA_optim)
	.align	5
#else
	.align	4
#endif


_log:
#if	defined(CA_optim)
	eshro	32-1-11,s1_lo,poly_hi		/* Create left-justified mant	*/
	lda	[s1_hi*2],s1_exp		/* Left justify exp field	*/
#else
	shlo	1+11,s1_hi,poly_hi		/* Create left-justified mant	*/
	shlo	1,s1_hi,s1_exp			/* Left justify exp field	*/

	shro	32-1-11,s1_lo,tmp_1		/* _lo -> _hi bits		*/
	or	poly_hi,tmp_1,poly_hi
#endif
	shlo	1+11,s1_lo,poly_lo

	shri	32-11,s1_exp,tmp_1		/* Special value testing	*/
	lda	SQRT_2_NO_j,con_hi

	shro	32-11,s1_exp,s1_exp		/* Biased, right justif exp	*/
	addlda(1,tmp_1,tmp_1)			/* Special value testing	*/

	cmpobge.f 1,tmp_1,Llog_80		/* J/ NaN/INF/denorm/0.0	*/

Llog_10:
	bbs.f	31,s1_hi,Llog_90			/* J/ sign bit set		*/

	cmpo	poly_hi,con_hi
	movlda(0,s1_flags)

#if	defined(CA_optim)
	eshro	2,poly_lo,rp_2_lo		/* Position denom for z calc	*/
#else
	shlo	32-2,poly_hi,tmp_1		/* Position denom for z calc	*/
	shro	2,poly_lo,rp_2_lo
	or	tmp_1,rp_2_lo,rp_2_lo
#endif
	shro	2,poly_hi,rp_2_hi
	bl.f	Llog_12				/* J/ < sqrt(2)			*/

	cmpo	0,0				/* Set no borrow		*/

	subc	poly_lo,0,poly_lo		/* Compute numer = 1 - mant	*/
	movlda(1,s1_flags)			/* Signal >= sqrt(2)		*/

	subc	poly_hi,0,poly_hi

	setbit	30,rp_2_hi,rp_2_hi		/* 1 + mant frac		*/
	addlda(1,s1_exp,s1_exp)


Llog_12:
	setbit	31,rp_2_hi,rp_2_hi		/* denom = 2 + mant		*/

	scanbit	poly_hi,tmp_1

	subo	tmp_1,31,tmp_2			/* Left shift for bit 31 norm	*/
	addlda(1,tmp_1,tmp_1)			/* Right shift for interword	*/

	bno	Llog_60				/* direct log(x) approx		*/
	
	shlo	tmp_2,poly_hi,poly_hi		/* Left norm the numerator	*/
	shro	tmp_1,poly_lo,tmp_1		/* _lo -> _hi bits		*/
	or	poly_hi,tmp_1,poly_hi
	shlo	tmp_2,poly_lo,poly_lo
	addlda(2,tmp_2,z_shift)

	cmpo	1,0				/* Set borrow			*/
	subc	poly_lo,rp_2_lo,tmp_1		/* Check for divide overflow	*/
	subc	poly_hi,rp_2_hi,tmp_1
	be.f	Llog_16				/* J/ no divide overflow	*/

#if	defined(CA_optim)
	eshro	1,poly_lo,poly_lo		/* Prevent division overflow	*/
#else
	shlo	32-1,poly_hi,tmp_1		/* Prevent division overflow	*/
	shro	1,poly_lo,poly_lo
	or	tmp_1,poly_lo,poly_lo
#endif
	shro	1,poly_hi,poly_hi
	addlda(1,tmp_2,z_shift)

Llog_16:
	ediv	rp_2_hi,poly_lo,z_mant_lo
	mov	z_mant_hi,tmp_1			/* Division result		*/
	movldar(z_mant_lo,poly_hi)		/* Prepare for next div step	*/
/*	mov	0,poly_lo	*/		/* (don't care about ls bit	*/
	ediv	rp_2_hi,poly_lo,z_mant_lo
	mov	z_mant_hi,z_mant_lo		/* 64-bit "A/B" result		*/
	movldar(tmp_1,z_mant_hi)

	shro	1,rp_2_lo,z_sqr_hi		/* Prepare for C/B		*/
	shlo	31,rp_2_lo,z_sqr_lo
	ediv	rp_2_hi,z_sqr_lo,rp_2_lo

	emul	rp_2_hi,z_mant_hi,rp_2_lo	/*  C/B * A/B			*/

	addc	rp_2_lo,rp_2_lo,rp_2_lo		/*  align to A/B		*/
	addc	rp_2_hi,rp_2_hi,rp_2_lo
	addc	0,0,rp_2_hi

	cmpo	0,0				/* Clear borrow bit		*/
	subc	rp_2_lo,z_mant_lo,z_mant_lo	/* A/B - (C/B * A/B)		*/
	subc	rp_2_hi,z_mant_hi,z_mant_hi

	emul	z_mant_hi,z_mant_lo,rp_1_lo	/* Compute z^2			*/

	cmpo	z_shift,4			/* Check for second table	*/

	shlo	1,z_shift,step_shift
	lda	Lpoly_table-.-8(ip),tmp_3	/* Table of constants		*/

	bl.t	Llog_18				/* J/ use first table		*/

	lda	Lpoly_len*8(tmp_3),tmp_3	/* Use second table		*/

Llog_18:
	emul	z_mant_hi,z_mant_hi,z_sqr_lo

	ldl	(tmp_3),poly_lo			/* Fetch first constant		*/

	addc	rp_1_lo,rp_1_lo,tmp_1		/* Align partial products	*/

	addc	rp_1_hi,rp_1_hi,rp_1_lo
	addlda(8,tmp_3,tmp_3)

	addc	0,0,rp_1_hi

	addc	rp_1_lo,z_sqr_lo,z_sqr_lo
	movlda(Lpoly_len-1,tmp_1)

	addc	rp_1_hi,z_sqr_hi,z_sqr_hi

	cmpobl.f 31,step_shift,Llog_70		/* J/ single term poly approx	*/

#if	!defined(CA_optim)
	ldconst	32,tmp_2
	subo	step_shift,tmp_2,tmp_2
#endif

Llog_20:						/* Polynomial approximation	*/
	emul	poly_lo,z_sqr_hi,rp_1_lo
	ldl	(tmp_3),con_lo
	addo	8,tmp_3,tmp_3
	emul	poly_hi,z_sqr_lo,rp_2_lo
	emul	poly_hi,z_sqr_hi,poly_lo

	addc	rp_2_lo,rp_1_lo,rp_1_lo
	addc	rp_2_hi,rp_1_hi,rp_1_lo
	addc	0,0,rp_1_hi

	addc	rp_1_lo,poly_lo,poly_lo
	addc	rp_1_hi,poly_hi,poly_hi

#if	defined(CA_optim)
	eshro	step_shift,poly_lo,poly_lo
	shro	step_shift,poly_hi,poly_hi
#else
	shlo	tmp_2,poly_hi,rp_2_lo
	shro	step_shift,poly_hi,poly_hi
	shro	step_shift,poly_lo,poly_lo
	or	rp_2_lo,poly_lo,poly_lo
#endif
	addc	poly_lo,con_lo,poly_lo
	addc	poly_hi,con_hi,poly_hi

	cmpdeco	1,tmp_1,tmp_1
	bne.t	Llog_20

Llog_22:
	emul	poly_lo,z_mant_hi,rp_1_lo
	emul	poly_hi,z_mant_lo,rp_2_lo
	emul	poly_hi,z_mant_hi,poly_lo

	addc	rp_2_lo,rp_1_lo,rp_1_lo		/* Finish poly approx		*/
	addc	rp_2_hi,rp_1_hi,rp_1_lo
	addc	0,0,rp_1_hi

	addc	rp_1_lo,poly_lo,poly_lo
	addc	rp_1_hi,poly_hi,poly_hi

	lda	DP_Bias,con_hi
	cmpobne.t s1_exp,con_hi,Llog_50		/* J/ ln 2 factors		*/

	lda	DP_Bias-1+2,tmp_1		/* Compute exp of approx	*/
	subo	z_shift,tmp_1,s1_exp

	bbs.f	31,poly_hi,Llog_42		/* J/ normalized result		*/

	subo	1,s1_exp,s1_exp			/* Left shift to normalize	*/
	addc	poly_lo,poly_lo,poly_lo
	addc	poly_hi,poly_hi,poly_hi

Llog_42:
	chkbit	64-53-1,poly_lo			/* Rounding bit to carry	*/

	clrbit	31,poly_hi,poly_hi		/* Drop j bit			*/

#if	defined(CA_optim)
	eshro	11,poly_lo,poly_lo
#else
	shlo	32-11,poly_hi,tmp_1
	shro	11,poly_lo,poly_lo
	or	tmp_1,poly_lo,poly_lo
#endif
	shro	11,poly_hi,poly_hi

	shlo	32-1-11,s1_exp,s1_exp		/* Position exponent		*/
	addc	0,poly_lo,out_lo
	addc	s1_exp,poly_hi,out_hi		/* Round, combine exp w/ mant	*/

	chkbit	0,s1_flags
	alterbit 31,out_hi,out_hi		/* Set sign bit			*/
	ret

/*
 *  Log w/ ln 2 factors to be combined
 */

Llog_50:
	lda	52(s1_exp),s1_exp		/* Insure s1_exp is not neg	*/

	lda	ln_2_lo,con_hi			/* Compute biased ln 2 factor	*/
	emul	con_hi,s1_exp,rp_1_lo

	subo	2,z_shift,z_shift		/* "Multiply" poly approx by 4	*/

	subo	s1_flags,0,s1_flags		/* 0 if positive, -1 if neg	*/
	lda	ln_2_mid,con_hi

	emul	con_hi,s1_exp,rp_2_lo
	
/*
 *  align poly result to binary point immediately to the left of the _hi word
 */

#if	defined(CA_optim)
	eshro	z_shift,poly_lo,poly_lo
#else
	ldconst	32,tmp_2
	subo	z_shift,tmp_2,tmp_2
	shlo	tmp_2,poly_hi,tmp_1
	shro	z_shift,poly_lo,poly_lo
	or	tmp_1,poly_lo,poly_lo
#endif
	shro	z_shift,poly_hi,poly_hi

	addc	rp_1_hi,rp_2_lo,rp_2_lo		/* Combine partial ln 2 prdcts	*/
	lda	ln_2_hi,con_hi

	emul	con_hi,s1_exp,rp_1_lo

	addc	rp_2_hi,rp_1_lo,rp_1_lo		/* Combine partial ln 2 prdcts	*/
	lda	bias_frac_lo,con_lo
	addc	0,rp_1_hi,rp_1_hi		/* Combine partial ln 2 prdcts	*/
	lda	bias_frac_hi,con_hi

	subc	con_lo,rp_2_lo,rp_2_lo		/* Remove exp bias of ln 2	*/
	lda	bias_int,con_lo
	subc	con_hi,rp_1_lo,rp_1_lo
	subc	con_lo,rp_1_hi,rp_1_hi

	xor	s1_flags,poly_lo,poly_lo	/* Apply sign to poly rslts	*/
	addc	rp_2_lo,poly_lo,rp_2_lo		/* Add in poly results		*/
	xor	s1_flags,poly_hi,poly_hi
	addc	rp_1_lo,poly_hi,rp_1_lo
	addc	s1_flags,rp_1_hi,rp_1_hi

	shri	31,rp_1_hi,con_hi		/* Absolute value of result	*/
	xor	rp_2_lo,con_hi,rp_2_lo
	xor	rp_1_lo,con_hi,rp_1_lo
	xor	rp_1_hi,con_hi,rp_1_hi

	addc	rp_2_lo,rp_2_lo,rp_2_lo		/* Position for scanbit		*/
	addc	rp_1_lo,rp_1_lo,rp_1_lo
	addc	rp_1_hi,rp_1_hi,rp_1_hi

	scanbit	rp_1_hi,tmp_1

	addo	32-20,tmp_1,tmp_2		/* "Right" shift for norm	*/

#if	defined(CA_optim)
	eshro	tmp_2,rp_1_lo,rp_1_hi
	lda	(rp_1_lo),rp_2_hi
	eshro	tmp_2,rp_2_lo,rp_1_lo
#else
	subo	tmp_1,20,tmp_3			/* Left shift for bit 20 norm	*/
	shlo	tmp_3,rp_1_hi,rp_1_hi
	shro	tmp_2,rp_1_lo,rp_2_hi
	or	rp_1_hi,rp_2_hi,rp_1_hi		/* MS word of result		*/
	shlo	tmp_3,rp_1_lo,rp_1_lo
	shro	tmp_2,rp_2_lo,rp_2_hi
	or	rp_1_lo,rp_2_hi,rp_1_lo		/* LS word of result		*/
#endif

	lda	DP_Bias-1(tmp_1),s1_exp		/* Result exponent		*/

	addo	31-20,tmp_1,tmp_2		/* Rounding bit number		*/
	chkbit	tmp_2,rp_2_lo			/* Rounding bit to carry	*/
	clrbit	20,rp_1_hi,rp_1_hi		/* Zap j bit			*/
	addc	0,rp_1_lo,out_lo
	shlo	20,s1_exp,s1_exp		/* Position exponent		*/
	addc	rp_1_hi,s1_exp,out_hi		/* Round, combine exp and mant	*/
	shlo	31,con_hi,con_hi		/* Sign bit			*/
	or	out_hi,con_hi,out_hi		/* Mix in sign bit, mov		*/
	ret


/*
 *  Mantissa value very close to 1.0 (top word of mant-1 = 00...00), so
 *  approximate log(mant) = (mant-1) - (mant-1)^2/2
 */

Llog_60:
	mov	2,z_shift			/* Prepare for Llog_50		*/
	lda	DP_Bias,tmp_1

	cmpobne.t s1_exp,tmp_1,Llog_50		/* J/ some ln 2 factor		*/

	scanbit	poly_lo,tmp_1			/* Search for MS bit		*/

	bno.f	Llog_62				/* J/ mant = 1.0 -> return 0.0	*/

	subo	tmp_1,31,tmp_1			/* left shift to norm		*/

	shlo	tmp_1,poly_lo,poly_hi		/* Normalizing shift (0 works)	*/

	emul	poly_hi,poly_hi,rp_1_lo

	addo	s1_flags,tmp_1,tmp_1		/* Adj shift for neg x		*/

	cmpo	1,1				/* Set no borrow		*/
	addlda(1,tmp_1,tmp_2)			/* x^2/2 shift			*/

	shro	tmp_2,rp_1_hi,rp_1_lo

	subo	s1_flags,0,rp_1_hi		/* Inflict sign on second term	*/
	xor	rp_1_lo,rp_1_hi,rp_1_lo
	subo	rp_1_hi,rp_1_lo,rp_1_lo

	subc	rp_1_lo,0,poly_lo
	lda	DP_Bias-32-1-1,s1_exp		/* Compute result exponent	*/

	subc	rp_1_hi,poly_hi,poly_hi

	subo	tmp_1,s1_exp,s1_exp

	bbs.t	31,poly_hi,Llog_61		/* J/ normalized		*/

	addc	poly_lo,poly_lo,poly_lo
	subo	1,s1_exp,s1_exp
	addc	poly_hi,poly_hi,poly_hi

Llog_61:
	chkbit	10,poly_lo			/* Rounding bit			*/

#if	defined(CA_optim)
	eshro	11,poly_lo,poly_lo		/* Position mantissa		*/
#else
	shlo	32-11,poly_hi,tmp_1		/* Position mantissa		*/
	shro	11,poly_lo,poly_lo
	or	tmp_1,poly_lo,poly_lo
#endif
	shro	11,poly_hi,poly_hi

	addc	poly_lo,0,out_lo		/* Round/copy lo word		*/

	shlo	20,s1_exp,s1_exp		/* Create sign/exp fields	*/
	shlo	31,s1_flags,s1_flags
	or	s1_exp,s1_flags,out_hi

	addc	poly_hi,out_hi,out_hi		/* Finish round, pack hi word	*/
	ret

Llog_62:
	movl	0,out_lo			/* Return +0.0			*/
	ret


/*
 *  Single term approximation:
 *
 *               log(x) ~=  z * ( 2 + z^2/3 )  [ where z = (x-1)/(x+1) ]
 */

Llog_70:
	subo	31,step_shift,step_shift	/* Subtract 32 from step_shift	*/
	lda	one_term_P3,con_hi
	emul	z_sqr_hi,con_hi,poly_lo
	subo	1,step_shift,step_shift
	shro	step_shift,poly_hi,poly_lo
	ldconst	one_term_P1,poly_hi
	b	Llog_22				/* Rejoin after poly approx	*/
	

/*
 *  NaN/INF/denorm/0.0
 */

Llog_80:
	or	poly_lo,poly_hi,tmp_1
	be	Llog_85				/* J/ denorm/0.0		*/

	cmpobne.f 0,tmp_1,Llog_82		/* J/ NaN			*/

	bbs.f	31,s1_hi,Llog_90			/* J/ -INF -> negative value	*/

/*
 *  log(+INF) -> +INF w/ ERANGE
 */

	movl	g0,r4				/* Save +INF value		*/

        callj   __errno_ptr                    /* returns addr of ERRNO in g0  */
        ldconst ERANGE,r8
        st      r8,(g0)

	movl	r4,g0				/* log(+INF) -> +INF		*/

	ret


Llog_82:						/* NaN argument			*/
	movl	g0,r4				/* Save incoming value		*/

	callj	__errno_ptr			/* returns addr of ERRNO in g0	*/
	ldconst	EDOM,r8
	st	r8,(g0)

	movl	r4,out_lo			/* Restore incoming value	*/

	lda	DP_QNaN_hi,tmp_1

	or	out_hi,tmp_1,out_hi

	ret


Llog_85:
	scanbit	poly_hi,tmp_1
	bno	Llog_86				/* J/ hi mantissa word is zero	*/

	subo	31,tmp_1,s1_exp			/* Compute effective exponent	*/

	subo	tmp_1,31,tmp_2			/* Left shift			*/
	addlda(1,tmp_1,tmp_3)			/* Right shift (wd -> wd)	*/

	shlo	tmp_2,poly_hi,poly_hi		/* Left just denormal mant	*/
	shro	tmp_3,poly_lo,tmp_3
	or	poly_hi,tmp_3,poly_hi
	shlo	tmp_2,poly_lo,poly_lo

	addc	poly_lo,poly_lo,poly_lo		/* Drop MS bit of mant		*/
	addc	poly_hi,poly_hi,poly_hi
	b	Llog_10

Llog_86:
	scanbit	poly_lo,tmp_1
	bno	Llog_88				/* J/ argument is zero		*/

	ldconst	32,con_lo

	subo	31,tmp_1,s1_exp			/* Compute effective exponent	*/
	subo	con_lo,s1_exp,s1_exp

	subo	tmp_1,con_lo,tmp_1		/* Left shift (w/ MS bit drop)	*/

	shlo	tmp_1,poly_lo,poly_hi		/* Shift and MS bit drop	*/
	movlda(0,poly_lo)
	b	Llog_10


Llog_88:						/* log(0) -> -INF w/ ERANGE	*/
	callj	__errno_ptr			/* returns addr of ERRNO in g0	*/
	ldconst	ERANGE,r8
	st	r8,(g0)

	mov	0,out_lo			/* -INF				*/
	lda	DP_NEG_INF_hi,out_hi
	ret


/*
 *  Negative (non-zero) numbers handled here:  return QNaN w/ EDOM
 */

Llog_90:
	callj	__errno_ptr			/* returns addr of ERRNO in g0	*/
	ldconst	EDOM,r8
	st	r8,(g0)

	mov	0,out_lo			/* QNaN				*/
	lda	DP_QNaN_hi,out_hi
	ret
