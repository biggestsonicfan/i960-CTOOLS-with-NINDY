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
/*      sqrtf.c  - Single Precision Square Root Function (AFP-960)	      */
/*									      */
/******************************************************************************/

#include "asmopt.h"
#include "i960arch.h"


	.file	"a_sqrtf.s"
#if	defined(IEEE_SQRT)

	.globl	__IEEE_sqrtf

#if	!defined(_i960KX)
	.globl	fpem_CA_AC
#endif

	.globl	AFP_RRC_S
	.globl	AFP_NaN_S

	.globl	_AFP_Fault_Invalid_Operation_S
	.globl	_AFP_Fault_Reserved_Encoding_S

#else

	.globl	_sqrtf

	.globl	__AFP_NaN_S


#endif


#define	Standard_QNaN   0xffc00000

#define	FP_Bias         0x7f
#define	FP_INF          0xff


#define	AC_Round_Mode   30
#define	Round_Mode_Down 2

#if	defined(IEEE_SQRT)
#define	AC_Norm_Mode    29

#define	AC_InvO_mask    26
#define	AC_InvO_flag    18
#endif


/* Register Name Equates */

#define	s1		g0

#define	rsl_mant_x	r4
#define	rsl_mant	r5
#define	rsl_exp		r3
#define	rsl_sign	r6

#define	s1_exp		r7
#define	s1_mant_x	r8
#define	s1_mant		r9

#define	tmp_1		r10
#define	tmp_2		r11

#define	rp		r12
#define	rp_lo		r12
#define	rp_hi		r13

#define	s1_mant_16	r14

#if	defined(IEEE_SQRT)
#define	ac		r15

#define	op_type		g2
#define	sqrt_type	31
#endif

#define	con_1		g4
#define	con_2		g5

#define	out		g0




/*  Polynomial approximation to sqrt for the interval  [ 1, 4 ]	*/

/*  Constants and algorithms adapted from Hart et al, SQRT 0071	*/

#define	E2	    0xA1F5	/*  0.03954 02679 << 20  */
#define	E1	    0x86A9	/*  0.52601 05935 << 16  */
#define	E0	0x212FFC14	/*  0.51855 37526 << 30  */


	.text
	.link_pix

#if	defined(CA_optim)
	.align	5
#else
	.align	4
#endif


#if	defined(IEEE_SQRT)
__IEEE_sqrtf:
#else
_sqrtf:
#endif
	chkbit	23,s1				/* Odd/even exponent bit	*/

#if	defined(IEEE_SQRT)
#if	defined(_i960KX)
	modac	0,0,ac
#else
	ld	fpem_CA_AC,ac
#endif
#endif

	addc	0,0,tmp_1			/* tmp_1 = 1 if sqrt mant < 2.0	*/

	shlo	8,s1,s1_mant			/* Left justify mantissa bits	*/

	setbit	31,s1_mant,s1_mant		/* Set "j" bit			*/
	movlda(0,s1_mant_x)			/* Zero extent just for luck	*/

	shro	tmp_1,s1_mant,s1_mant		/* Range [1,2) or [2,4)		*/

	shro	16,s1_mant,s1_mant_16		/* Short form for root estimate	*/
	lda	E2,con_1

	mulo	s1_mant_16,con_1,rsl_mant	/*              x  *E2		*/
	shlo1(s1,s1_exp)

	shri	24,s1_exp,tmp_1

	shro	18,rsl_mant,rsl_mant		/*  Position for sub from E1	*/
	lda	E1,con_1

	subo	rsl_mant,con_1,rsl_mant

	mulo	s1_mant_16,rsl_mant,rsl_mant	/*       x*E1 - x^2*E2		*/
	lda	E0,con_1

	shro	3,s1_mant,s1_mant		/* Position for iteration	*/

	addo	rsl_mant,con_1,rsl_mant		/*   R0  =  E0 + x*E1 - x^2*E2	*/

	ediv	rsl_mant,s1_mant_x,rp		/* First iteration division	*/

	shro	24,s1_exp,s1_exp		/* Biased exponent to s1_exp	*/
	addlda(1,tmp_1,tmp_1)

	cmpo	tmp_1,1				/* NaN/INF/denorm/zero test	*/
#if	defined(IEEE_SQRT)
	movlda(sqrt_type,op_type)
#endif

	shro	1,rsl_mant,rsl_mant		/* Prepare for iter averaging	*/
	ble.f	Ls1_special			/* J/ special handling required	*/

Lsqrtf_10:
	mov	0,rsl_sign			/* Set result sign bit		*/
#if	defined(IEEE_SQRT)
	lda	FP_Bias(s1_exp),rsl_exp
#else
	lda	FP_Bias-2(s1_exp),rsl_exp
#endif

	shro	1,rsl_exp,rsl_exp		/* Compute result exponent	*/

	bbs.f	31,s1,Lsqrtf_80			/* J/ negative finite value	*/

	addo	rsl_mant,rp_hi,rsl_mant		/* R1 iteration			*/

	ediv	rsl_mant,s1_mant_x,rp		/* Second iteration		*/

#if	defined(IEEE_SQRT)

	shro	1,rsl_mant,rsl_mant
	lda	0x20,con_1

	addo	rsl_mant,con_1,rsl_mant		/* Begin rounding to R bit	*/
	lda	0x3F,con_1

	addc	rsl_mant,rp_hi,rsl_mant		/* R2 iteration + rnding to R	*/

	andnot	con_1,rsl_mant,rsl_mant		/* Strip to mant + R only	*/

	emul	rsl_mant,rsl_mant,rp		/* Square adjusted R2		*/

	cmpo	0,0				/* Set no borrow		*/

	shlo	1,s1_mant,s1_mant		/* Adjust for correction step	*/

	subc	rp_lo,s1_mant_x,rp_lo
	subc	rp_hi,s1_mant,rp_hi

	or	rp_lo,rp_hi,tmp_1
	cmpo	0,tmp_1
	testne	tmp_1
	shri	31,rp_hi,tmp_2			/* 0xFF..FF if negative		*/
	xor	tmp_1,tmp_2,tmp_1
	subo	tmp_2,tmp_1,tmp_1
	addo	rsl_mant,tmp_1,rsl_mant		/* Correction step		*/

	shlo	32-7,rsl_mant,rsl_mant_x
	shro	7,rsl_mant,rsl_mant

	b	AFP_RRC_S

#else

	shlo	23,rsl_exp,rsl_exp		/* Position exponent-1		*/

	shro	1,rsl_mant,rsl_mant		/* Prepare for final approx	*/

	addo	rsl_mant,rp_hi,rsl_mant		/* R2 iteration + rnding to R	*/

	chkbit	7-1,rsl_mant			/* Rounding bit			*/

	shro	8-1,rsl_mant,rsl_mant		/* Right just double mantissa	*/

	addc	rsl_exp,rsl_mant,out		/* Round/combine mant w/ exp	*/

	ret
#endif


/*
 *  Negative finite value
 */

Lsqrtf_80:
#if	!defined(IEEE_SQRT)
	ldconst	Standard_QNaN,g0		/* Create a quiet NaN		*/
	mov	0,g1				/* Fake second operand		*/
	b	__AFP_NaN_S			/* Non-signaling NaN handler	*/
#else
	bbc.f	AC_InvO_mask,ac,Lsqrtf_85	/* J/ inv oper fault not masked	*/

#if	defined(_i960KX)
	ldconst	1 << AC_InvO_flag,tmp_1		/* Set inv oper flag */
	modac	tmp_1,tmp_1,tmp_1
#else
	setbit	AC_InvO_flag,ac,ac		/* Set inv oper flag */
	st	ac,fpem_CA_AC
#endif

	ldconst	Standard_QNaN,out		/* Return a quiet NaN		*/
	ret

Lsqrtf_85:
	mov	0,g1				/* Fake second operand		*/
	b	_AFP_Fault_Invalid_Operation_S
#endif



/*
 *  NaN, INF, denorm, or zero
 */

Ls1_special:
	shlo	1+8,s1,tmp_1

	bne	Ls1_20				/* J/ NaN/INF value		*/

	cmpobne.f 0,tmp_1,Ls1_10			/* J/ denomalized value		*/

/*	mov	s1,out		*/		/* sqrt(+/-0) = +/- 0		*/
	ret

Ls1_10:
#if	defined(IEEE_SQRT)
	bbc.f	AC_Norm_Mode,ac,Ls1_18		/* J/ not norm mode -> fault	*/
#endif

/*
 *  Normalizing algorithm fails with a negative denormal.  However, shortly
 *  after returning to the normal flow, the negative value will be detected
 *  and shunted off to special processing.
 */
	scanbit	s1,tmp_1			/* Normalize denorm significand	*/

	subo	tmp_1,31,tmp_1			/* MS bit num to left shift cnt	*/

	subo	tmp_1,1+8,s1_exp		/* set "effective" s1_exp value	*/

	shlo	tmp_1,s1,s1_mant		/* Normalize significand	*/

/*
 *  After normalizing denormal value, put everything the way it would have
 *  been (were the value not denormal) at the point of departure.
 */
	and	1,s1_exp,tmp_1			/* tmp_1 = 1 if sqrt mant < 2.0	*/

	shro	tmp_1,s1_mant,s1_mant		/* Range [1,2) or [2,4)		*/

	shro	16,s1_mant,s1_mant_16		/* Short form for root estimate	*/
	lda	E2,con_1
	mulo	s1_mant_16,con_1,rsl_mant	/*                     x  *E2	*/
	shro	18,rsl_mant,rsl_mant		/*  Position for sub from E1	*/
	lda	E1,con_1
	subo	rsl_mant,con_1,rsl_mant
	mulo	s1_mant_16,rsl_mant,rsl_mant	/*           x*E1 - x^2*E2	*/
	lda	E0,con_1

	shro	3,s1_mant,s1_mant		/* Position for iteration	*/

	addo	rsl_mant,con_1,rsl_mant		/*  R0  =  E0 + x*E1 - x^2*E2	*/

	ediv	rsl_mant,s1_mant_x,rp		/* First iteration division	*/

	shro	1,rsl_mant,rsl_mant		/* Prepare for iter averaging	*/

	b	Lsqrtf_10			/* Resume execution		*/

#if	defined(IEEE_SQRT)
Ls1_18:
	mov	0,g1				/* Fake second parameter	*/
	b	_AFP_Fault_Reserved_Encoding_S
#endif


Ls1_20:
	cmpobne.f 0,tmp_1,Ls1_30			/* J/ NaN			*/

	bbs.f	31,s1,Lsqrtf_80			/* J/ -INF -> sqrt of neg num	*/

/*	mov	s1,out		*/		/* sqrt(+INF) = +INF		*/
	ret

Ls1_30:
	mov	0,g1				/* Fake second operand		*/
#if	defined(IEEE_SQRT)
	b	AFP_NaN_S			/* Handle NaN Operand		*/
#else
	b	__AFP_NaN_S			/* Non-signaling NaN handler	*/
#endif
