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
#include "i960arch.h"


	.file	"a_sqrt.s"
#if	defined(IEEE_SQRT)

	.globl	__IEEE_sqrt

#if	!defined(_i960KX)
	.globl	fpem_CA_AC
#endif

	.globl	AFP_RRC_D
	.globl	AFP_NaN_D

	.globl	_AFP_Fault_Invalid_Operation_D
	.globl	_AFP_Fault_Reserved_Encoding_D
#else

	.globl	_sqrt

	.globl	__AFP_NaN_D

#endif


#define	Standard_QNaN_hi  0xfff80000
#define	Standard_QNaN_lo  0x00000000

#define	DP_Bias         0x3ff
#define	DP_INF          0x7ff


#if	defined(IEEE_SQRT)
#define	AC_Round_Mode   30
#define	Round_Mode_Down 2

#define	AC_Norm_Mode    29

#define	AC_InvO_mask    26
#define	AC_InvO_flag    18
#endif



/* Register Name Equates */

#define	s1		g0
#define	s1_lo		s1
#define	s1_hi		g1


#define	rsl_mant_x	r3
#define	rsl_mant	r4
#define rsl_mant_lo	rsl_mant
#define rsl_mant_hi	r5
#define	rsl_exp		r6
#define	rsl_sign	r7

#define	s1_mant		r8
#define s1_mant_lo	s1_mant
#define s1_mant_hi	r9
#define	s1_exp		r10

#define	s1_mant_16	r11

#define	rp		r12
#define	rp_lo		r12
#define	rp_hi		r13

#if	defined(IEEE_SQRT)
#define	ac		r15
#endif

#define	con_1		g2
#define	con_2		g3

#define	tmp_1		g5
#define	tmp_2		r14

#define	rp_2		g2
#define	rp_2_lo		g2
#define	rp_2_hi		g3

#define	rp_3		g6
#define	rp_3_lo		g6
#define	rp_3_hi		g7

#define	out		g0
#define	out_lo		out
#define	out_hi		g1

#if	defined(IEEE_SQRT)
#define	op_type		g4
#define	sqrt_type	31
#endif



/*  Polynomial approximation to sqrt for the interval [ 1, 4 ]	*/

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
__IEEE_sqrt:
#else
_sqrt:
#endif
	chkbit	20,s1_hi			/* Odd/even exponent bit	*/

#if	defined(IEEE_SQRT)
#if	defined(_i960KX)
	modac	0,0,ac
#else
	ld	fpem_CA_AC,ac
#endif
#endif

	addc	0,0,tmp_1			/* tmp_1 = 1 if sqrt mant < 2.0	*/

#if	defined(CA_optim)
	eshro	32-11,s1_lo,s1_mant_hi		/* Left justify mantissa bits	*/
#else
	shro	32-11,s1_lo,tmp_2		/* Left justify mantissa bits	*/
	shlo	11,s1_hi,s1_mant_hi
	or	tmp_2,s1_mant_hi,s1_mant_hi
#endif
	shlo	11,s1_lo,s1_mant_lo

	setbit	31,s1_mant_hi,s1_mant_hi	/* Set "j" bit			*/

#if	defined(CA_optim)
	eshro	tmp_1,s1_mant_lo,s1_mant_lo	/* Range [1,2) or [2,4)		*/
#else
	shlo	5,1,tmp_2
	subo	tmp_1,tmp_2,tmp_2
	shlo	tmp_2,s1_mant_hi,tmp_2		/* Range [1,2) or [2,4)		*/
	shro	tmp_1,s1_mant_lo,s1_mant_lo
	or	tmp_2,s1_mant_lo,s1_mant_lo
#endif
	shro	tmp_1,s1_mant_hi,s1_mant_hi

	shro	16,s1_mant_hi,s1_mant_16	/* Short form for root estimate	*/
	lda	E2,con_1

	mulo	s1_mant_16,con_1,rsl_mant_hi	/*                     x  *E2	*/
	shlo1(s1_hi,s1_exp)

	shri	32-11,s1_exp,tmp_1

	shro	18,rsl_mant_hi,rsl_mant_hi	/*  Position for sub from E1	*/
	lda	E1,con_1

	subo	rsl_mant_hi,con_1,rsl_mant_hi

	mulo	s1_mant_16,rsl_mant_hi,rsl_mant_hi /*           x*E1 - x^2*E2	*/
	lda	E0,con_1

#if	defined(CA_optim)
	eshro	3,s1_mant_lo,s1_mant_lo		/* Position for iteration	*/
#else
	shlo	32-3,s1_mant_hi,tmp_2		/* Position for iteration	*/
	shro	3,s1_mant_lo,s1_mant_lo
	or	tmp_2,s1_mant_lo,s1_mant_lo
#endif
	shro	3,s1_mant_hi,s1_mant_hi

	addo	rsl_mant_hi,con_1,rsl_mant_hi	/*  R0  =  E0 + x*E1 - x^2*E2	*/

	ediv	rsl_mant_hi,s1_mant_lo,rp	/* First iteration division	*/

	shro	32-11,s1_exp,s1_exp		/* Biased exponent to s1_exp	*/
	addlda(1,tmp_1,tmp_1)

	cmpo	tmp_1,1				/* NaN/INF/denorm/zero test	*/
#if	defined(IEEE_SQRT)
	movlda(sqrt_type,op_type)
#endif

	shro	1,rsl_mant_hi,rsl_mant_hi	/* Prepare for iter averaging	*/
	ble.f	Ls1_special			/* J/ special handling required	*/

Lsqrt_10:
	mov	0,rsl_sign			/* Set result sign bit		*/
#if	defined(IEEE_SQRT)
	lda	DP_Bias(s1_exp),rsl_exp
#else
	lda	DP_Bias-2(s1_exp),rsl_exp	/* "-2" for j bit		*/
#endif

	shro	1,rsl_exp,rsl_exp		/* Compute result exponent	*/

	bbs.f	31,s1_hi,Lsqrt_80		/* J/ negative finite value	*/

	addo	rsl_mant_hi,rp_hi,rsl_mant_hi	/* R1 iteration			*/

	ediv	rsl_mant_hi,s1_mant_lo,rp	/* Second iteration		*/

	shro	1,rsl_mant_hi,rsl_mant_hi

	addo	rsl_mant_hi,rp_hi,rsl_mant_hi

	ediv	rsl_mant_hi,s1_mant_lo,rp	/* Third iteration		*/

	mov	rsl_mant_hi,tmp_1		/* Copy the divisor		*/

	shlo	32-1,rsl_mant_hi,rsl_mant_lo	/* Right shift to long word	*/
	shro	1,rsl_mant_hi,rsl_mant_hi

	addo	rsl_mant_hi,rp_hi,rsl_mant_hi	/* Begin computing result	*/

	mov	rp_lo,rp_hi
	movlda(0,rp_lo)

	ediv	tmp_1,rp_lo,rp_lo


#if	defined(IEEE_SQRT)

	cmpo	1,0				/* Clear carry bit		*/

	shlo	8,1,con_1			/* Four rounding to R bit	*/
	lda	0x1FF,con_2			/* mask after rounding		*/

	addc	rsl_mant_lo,con_1,rsl_mant_lo	/* Start rounding steps		*/
	addc	rsl_mant_hi,0,rsl_mant_hi

	addc	rsl_mant_lo,rp_hi,rsl_mant_lo	/* Final approximation		*/
	andnot	con_2,rsl_mant_lo,rsl_mant_lo	/* Zap below R bit		*/
	addc	rsl_mant_hi,0,rsl_mant_hi

	emul	rsl_mant_hi,rsl_mant_lo,rp_2

/*	cmpo	1,0	*/			/* Clear carry			*/
	addc	s1_mant_lo,s1_mant_lo,s1_mant_lo	/* Shift left one slot	*/
	addc	s1_mant_hi,s1_mant_hi,s1_mant_hi

	emul	rsl_mant_lo,rsl_mant_lo,rp_3

	addc	rp_2_lo,rp_2_lo,rp_2_lo		/* Double cross product		*/
	addc	rp_2_hi,rp_2_hi,rp_2_hi
	addc	0,0,tmp_1

	emul	rsl_mant_hi,rsl_mant_hi,rp	/* Square adjusted R3		*/

	addc	rp_3_hi,rp_2_lo,rp_3_hi		/* Combine to   rp :: rp_3	*/
	addc	rp_2_hi,rp_lo,rp_lo
	addc	tmp_1,rp_hi,rp_hi

	cmpo	0,0				/* Set no borrow		*/

	subc	rp_3_lo,0,rp_3_lo
	subc	rp_3_hi,0,rp_3_hi
	subc	rp_lo,s1_mant_lo,rp_lo
	subc	rp_hi,s1_mant_hi,rp_hi

	or	rp_lo,rp_hi,tmp_1
	or	rp_3_lo,tmp_1,tmp_1
	or	rp_3_hi,tmp_1,tmp_1
	cmpo	0,tmp_1
	testne	tmp_1
	cmpo	1,0				/* Clear carry			*/
	shri	31,rp_hi,tmp_2			/* 0xFF..FF if negative		*/
	xor	tmp_1,tmp_2,tmp_1
	subo	tmp_2,tmp_1,tmp_1
	addc	rsl_mant_lo,tmp_1,rsl_mant_lo	/* Correction step		*/
	shri	1,tmp_1,tmp_1
	addc	rsl_mant_hi,tmp_1,rsl_mant_hi

	shlo	32-10,rsl_mant_lo,rsl_mant_x
#if	defined(CA_optim)
	eshro	10,rsl_mant_lo,rsl_mant_lo
#else
	shro	10,rsl_mant_lo,rsl_mant_lo
	shlo	32-10,rsl_mant_hi,tmp_1
	or	rsl_mant_lo,tmp_1,rsl_mant_lo
#endif
	shro	10,rsl_mant_hi,rsl_mant_hi

	b	AFP_RRC_D

#else

	shlo	20,rsl_exp,rsl_exp		/* Position exponent-1		*/

	cmpo	1,0				/* Clear carry bit		*/

	addc	rsl_mant_lo,rp_hi,rsl_mant_lo	/* Final approximation		*/
	addc	rsl_mant_hi,0,rsl_mant_hi

	chkbit	10-1,rsl_mant_lo		/* Rounding bit			*/

#if	defined(CA_optim)
	eshro	11-1,rsl_mant,rsl_mant_lo	/* Right just double mantissa	*/
#else
	shro	11-1,rsl_mant_lo,rsl_mant_lo	/* Right just double mantissa	*/
	shlo	32-11+1,rsl_mant_hi,tmp_1
	or	rsl_mant_lo,tmp_1,rsl_mant_lo
#endif

	addc	0,rsl_mant_lo,out_lo		/* Round lo word		*/

	shro	11-1,rsl_mant_hi,rsl_mant_hi	/* Position hi 21 bits of mant	*/

	addc	rsl_exp,rsl_mant_hi,out_hi	/* Combine mant w/ exp, round	*/

	ret
#endif


/*
 *  Negative finite value
 */

Lsqrt_80:
#if	!defined(IEEE_SQRT)
	ldconst	Standard_QNaN_hi,out_hi		/* Return a quiet NaN		*/
	ldconst	Standard_QNaN_lo,out_lo
	movl	0,g2				/* Fake second operand		*/
	b	__AFP_NaN_D			/* Non-signaling NaN handler	*/
#else
	bbc.f	AC_InvO_mask,ac,Lsqrt_85		/* J/ inv oper fault not masked	*/

#if	defined(_i960KX)
	ldconst	1 << AC_InvO_flag,tmp_1		/* Set inv oper flag */
	modac	tmp_1,tmp_1,tmp_1
#else
	setbit	AC_InvO_flag,ac,ac		/* Set inv oper flag */
	st	ac,fpem_CA_AC
#endif

	ldconst	Standard_QNaN_hi,out_hi		/* Return a quiet NaN		*/
	ldconst	Standard_QNaN_lo,out_lo
	ret

Lsqrt_85:
	movl	0,g2				/* Fake second operand		*/
	b	_AFP_Fault_Invalid_Operation_D
#endif



/*
 *  NaN, INF, denorm, or zero
 */

Ls1_special:
	shlo	1+11,s1_hi,tmp_1
	or	s1_lo,tmp_1,tmp_1

	bne	Ls1_20				/* J/ NaN/INF value		*/

	cmpobne.f 0,tmp_1,Ls1_10			/* J/ denomalized value		*/

/*	movl	s1,out		*/		/* sqrt(+/-0) = +/- 0		*/
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
	scanbit	s1_hi,tmp_1			/* Normalize denorm significand	*/
	bno	Ls1_14				/* J/ MS word = 0		*/

	subo	tmp_1,31,tmp_1			/* MS bit num to left shift cnt	*/

Ls1_12:
	subo	tmp_1,12,s1_exp			/* set "effective" s1_exp value	*/

	xor	s1_hi,s1_lo,s1_mant_hi		/* Normalize significand	*/
	rotate	tmp_1,s1_mant_hi,s1_mant_hi
	shlo	tmp_1,s1_lo,s1_mant_lo
	xor	s1_mant_hi,s1_mant_lo,s1_mant_hi

/*
 *  After normalizing denormal value, put everything the way it would have
 *  been (were the value not denormal) at the point of departure.
 */
	and	1,s1_exp,tmp_1			/* tmp_1 = 1 if sqrt mant < 2.0	*/

#if	defined(CA_optim)
	eshro	tmp_1,s1_mant_lo,s1_mant_lo	/* Range [1,2) or [2,4)		*/
#else
	shlo	5,1,tmp_2			/* 32 -> tmp_2			*/
	subo	tmp_1,tmp_2,tmp_2
	shlo	tmp_2,s1_mant_hi,tmp_2		/* Range [1,2) or [2,4)		*/
	shro	tmp_1,s1_mant_lo,s1_mant_lo
	or	tmp_2,s1_mant_lo,s1_mant_lo
#endif
	shro	tmp_1,s1_mant_hi,s1_mant_hi

	shro	16,s1_mant_hi,s1_mant_16	/* Short form for root estimate	*/
	lda	E2,con_1
	mulo	s1_mant_16,con_1,rsl_mant_hi	/*                     x  *E2	*/
	shro	18,rsl_mant_hi,rsl_mant_hi	/*  Position for sub from E1	*/
	lda	E1,con_1
	subo	rsl_mant_hi,con_1,rsl_mant_hi
	mulo	s1_mant_16,rsl_mant_hi,rsl_mant_hi /*           x*E1 - x^2*E2	*/
	lda	E0,con_1

#if	defined(CA_optim)
	eshro	3,s1_mant_lo,s1_mant_lo		/* Position for iteration	*/
#else
	shlo	32-3,s1_mant_hi,tmp_2		/* Position for iteration	*/
	shro	3,s1_mant_lo,s1_mant_lo
	or	tmp_2,s1_mant_lo,s1_mant_lo
#endif
	shro	3,s1_mant_hi,s1_mant_hi

	addo	rsl_mant_hi,con_1,rsl_mant_hi	/*  R0  =  E0 + x*E1 - x^2*E2	*/

	ediv	rsl_mant_hi,s1_mant_lo,rp	/* First iteration division	*/

	shro	1,rsl_mant_hi,rsl_mant_hi	/* Prepare for iter averaging	*/

	b	Lsqrt_10				/* Resume execution		*/

Ls1_14:
	scanbit	s1_lo,tmp_1
	subo	tmp_1,31,tmp_1
	setbit	5,tmp_1,tmp_1
	b	Ls1_12				/* J/ compute exp, norm mant	*/


#if	defined(IEEE_SQRT)
Ls1_18:
	movl	0,g2				/* Fake second parameter	*/
	b	_AFP_Fault_Reserved_Encoding_D
#endif


Ls1_20:
	cmpobne.f 0,tmp_1,Ls1_30			/* J/ NaN			*/

	bbs.f	31,s1_hi,Lsqrt_80		/* J/ -INF -> sqrt of neg num	*/

/*	movl	s1,out		*/		/* sqrt(+INF) = +INF		*/
	ret

Ls1_30:
#if	defined(IEEE_SQRT)
	movl	0,g2				/* Fake second operand		*/
	b	AFP_NaN_D			/* Handle NaN Operand		*/
#else
	movl	0,g2				/* Fake second operand		*/
	b	__AFP_NaN_D			/* Non-signaling NaN handler	*/
#endif
