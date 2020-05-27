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

	.file	"a_atanf.s"
	.globl	_atanf
	.globl	_atan2f


	.globl	__AFP_NaN_S

	.globl	__errno_ptr		/* returns addr of ERRNO in g0		*/

#define	EDOM	33


#define	FP_Bias         0x7f
#define	FP_INF          0xff


/* Register Name Equates */

#define	s1		g0

#define	arg_flags	r3
#define	arg_inv_bit	0
#define	arg_refl_bit	1
#define	rmdr_neg_bit	2
#define	rslt_neg_bit	3

#define	arg_mant	r4
#define	arg_exp		r5

#define	tmp_1		r6
#define	tmp_2		r7
#define	tmp_3		r8
#define	con_1	 	r9

#define	step_shift	r10
#define	mem_ptr		r11

#define	mant_sqr_lo	r12
#define	mant_sqr_hi	r13

#define	poly_result_lo	r14
#define	poly_result     r15

#define	result_se	g6

#define	x_arg		g1
#define	x_mant		g5
#define	x_exp		g4

#define	y_arg		s1
#define	y_mant		arg_mant
#define	y_exp		arg_exp

#define	out		g0


#define	FP_one		0x3f800000

#define	FP_PI_over_4	0x3f490fdb
#define	FP_PI_over_2	0x3fc90fdb
#define	FP_3_PI_over_4	0x4016cbe4
#define	FP_PI		0x40490fdb

#define	PI_32_bit	0xC90FDAA2
#define	PI_exp		FP_Bias+1


	.text
	.link_pix

/*  Polynomial approximation to ATAN for the interval  [ -1.0 , 1.0 ]	*/

	.set	Lpoly_init,0x00BAC537	/* C17 +0.28498896208  E-2 */
Lpoly_table:
	.word	0x041D12DC		/* C15 -0.160686289604 E-1 */
	.word	0x0AEDD4D7		/* C13 +0.426915192711 E-1 */
	.word	0x133603B4		/* C11 -0.750429453889 E-1 */
	.word	0x1B3DA47C		/* C09 +0.106409340253 E+0 */
	.word	0x245C801D		/* C07 -0.1420364446652E+0 */
	.word	0x332E5CF0		/* C05 +0.1999261939166E+0 */
	.word	0x555529B7		/* C03 -0.3333307334505E+0 */
	.word	0xFFFFFFBF		/* C01 +0.9999999847657E+0 */
	.set	Lpoly_table_len,(.-Lpoly_table) >> 2


#if	defined(CA_optim)
	.align	5
#else
	.align	4
#endif

_atan2f:
	addc	y_arg,y_arg,y_exp		/* 1 bit left, sign -> carry	*/

	shlo	1+8-1,y_arg,y_mant		/* Left justify mantissa	*/

	shri	32-8,y_exp,tmp_1

	alterbit rslt_neg_bit,0,arg_flags	/* Retain Y sign bit		*/
	addlda(1,tmp_1,tmp_1)

	cmpo	tmp_1,1
	shro	32-8,y_exp,y_exp
	ble.f	Ly_special			/* J/ Y is 0/denorm/INF/NaN	*/

	setbit	31,y_mant,y_mant		/* Set j bit			*/

Ly_rejoin:
	addc	x_arg,x_arg,x_exp		/* 1 bit left, sign -> carry	*/

	shlo	1+8-1,x_arg,x_mant		/* Left justify mantissa	*/

	shri	32-8,x_exp,tmp_1

	alterbit arg_refl_bit,arg_flags,arg_flags	/* Retain X sign bit	*/
	addlda(1,tmp_1,tmp_1)

	cmpo	tmp_1,1
	shro	32-8,x_exp,x_exp
	ble.f	Lx_special			/* J/ Y is 0/denorm/INF/NaN	*/

	setbit	31,x_mant,x_mant		/* Set j bit			*/

Lx_rejoin:
	cmpi	y_exp,x_exp			/* Set no borrow		*/
	bg.f	Latan2f_10			/* J/ |y_arg| > |x_arg|		*/
	bl	Latan2f_05			/* J/ |y_arg| < |x_arg|		*/

	cmpo	y_mant,x_mant
	bg.f	Latan2f_10			/* J/ |y_arg| > |x_arg|		*/
	be	Latanf_50			/* J/ |y_arg| = |x_arg|		*/

Latan2f_05:
	shro	1,y_mant,poly_result		/* Begin Y/X division		*/
	movlda(0,poly_result_lo)

	ediv	x_mant,poly_result_lo,poly_result_lo

	subo	x_exp,y_exp,arg_exp		/* Compute result exponent	*/

	b	Latan2f_12

Latan2f_10:
	shro	1,x_mant,poly_result		/* Begin X/Y division		*/
	movlda(0,poly_result_lo)

	ediv	y_mant,poly_result_lo,poly_result_lo

	setbit	arg_inv_bit,arg_flags,arg_flags

	subo	y_exp,x_exp,arg_exp		/* Compute result exponent	*/

Latan2f_12:
	shro	31,poly_result,tmp_1		/* Check for normalized		*/
	lda	FP_Bias(arg_exp),arg_exp
	subo	tmp_1,1,tmp_1			/* 1 or 0 left shift to norm	*/
	shlo	tmp_1,poly_result,arg_mant	/* Move w/ or w/o norm shift	*/
	subo	tmp_1,arg_exp,arg_exp		/* Adjust exp as required	*/
	b	Latanf_10



/*  Y_ARG is a special case (0.0, denormalized value, INF, or NaN)  */

Ly_special:
	bne.f	Ly_sp_20				/* J/ INF/NaN			*/

	scanbit	y_mant,tmp_1
	bno	Ly_sp_02				/* J/ zero			*/

	subo	tmp_1,31,tmp_1			/* Left shift to normalize	*/
	shlo	tmp_1,y_mant,y_mant

	subo	tmp_1,1,y_exp			/* Effective exponent value	*/
	b	Ly_rejoin

Ly_sp_02:
	addo	x_arg,x_arg,tmp_1
	cmpobe.f 0,tmp_1,Ly_sp_10		/* J/ atan2f(0,0) -> set errno	*/

	lda	FP_INF << 24,con_1
	cmpoble.t tmp_1,con_1,Ly_sp_08		/* J/ not a NaN			*/

Ly_sp_06:					/* At least one arg is a NaN	*/
	b	__AFP_NaN_S

Ly_sp_08:					/* atan2(0,non-zero/non-NaN)	*/
	bbs	31,x_arg,Lx_sp_24		/* J/ x is negative -> +/- PI	*/
/* ***	mov	y_arg,out  *** */
	ret

Ly_sp_10:					/* atan2(0,0) or atan2(INF,INF)	*/
	callj	__errno_ptr			/* returns addr of ERRNO in g0	*/
	ldconst	EDOM,tmp_1
	st	tmp_1,(g0)
	mov	0,out
	ret

Ly_sp_20:					/* Y is INF/NaN			*/
	shlo	1,y_mant,y_mant			/* Drop bottom exp bit		*/
	cmpobne.f 0,y_mant,Ly_sp_06		/* J/ Y is a NaN		*/

	addo	x_arg,x_arg,tmp_1		/* Drop sign bit		*/

	lda	FP_INF << 24,con_1
	cmpobg.t tmp_1,con_1,Ly_sp_06		/* J/ atan2f(INF,NaN)		*/
	be.f	Ly_sp_10				/* J/ atan2f(INF,INF)		*/
	b	Latanf_sp_02			/* J/ atan2f(INF,finite)	*/



/*  X_ARG is a special case (0.0, denormalized value, INF, or NaN), Y_arg  */
/*  is a non-zero, finite value.					   */

Lx_special:
	bne.f	Lx_sp_20				/* J/ INF/NaN			*/

	scanbit	x_mant,tmp_1
	bno	Latanf_sp_02			/* J/ x_arg is zero (+/- PI/2)	*/

	subo	tmp_1,31,tmp_1			/* Left shift to normalize	*/
	shlo	tmp_1,x_mant,x_mant

	subo	tmp_1,1,x_exp			/* Effective exponent value	*/
	b	Lx_rejoin

Lx_sp_20:
	shlo	1,x_mant,x_mant			/* Drop bottom exp bit		*/
	cmpobne.f 0,x_mant,Ly_sp_06		/* J/ X is a NaN		*/

	bbs.f	arg_refl_bit,arg_flags,Lx_sp_24	/* J/ atan2f(finite,-INF)	*/

	mov	0,out				/* atan2f(finite,+INF) -> +/- 0	*/

Lx_sp_22:
	chkbit	rslt_neg_bit,arg_flags		/* Y_arg sign to result		*/
	alterbit 31,out,out
	ret

Lx_sp_24:
	lda	FP_PI,out			/* |result| = PI		*/
	b	Lx_sp_22				/* Transfer Y_arg sign		*/


#if	defined(CA_optim)
	.align	5
#else
	.align	4
#endif

_atanf:
	cmpo	1,0				/* Clear carry			*/
	shlo	8,s1,arg_mant			/* Create left-justified mant	*/
	addc	s1,s1,tmp_3			/* Sign bit to carry		*/
	shri	24,tmp_3,tmp_1			/* Special value testing	*/
	alterbit rslt_neg_bit,0,arg_flags	/* Save the sign bit		*/
	addlda(1,tmp_1,tmp_1)			/* Special value testing	*/

	cmpobge.f 1,tmp_1,Latanf_special		/* J/ NaN/INF/denorm/0.0	*/

	setbit	31,arg_mant,arg_mant		/* Force j bit			*/
	lda	FP_Bias-13,con_1

	shro	24,tmp_3,arg_exp		/* Biased, right justif exp	*/

	cmpoble.f arg_exp,con_1,Latanf_70	/* J/ zero/denorm/small		*/

	lda	FP_one << 1,con_1		/* Inversion testing		*/

	cmpobl.f tmp_3,con_1,Latanf_10		/* J/ < 1.0			*/
	be	Latanf_50			/* J/ s1 = 1.0			*/

	setbit	31,0,mant_sqr_hi		/* Invert argument's mantissa	*/
	movlda(0,mant_sqr_lo)

	or	1,arg_mant,arg_mant		/* Prevent division overflow	*/

	ediv	arg_mant,mant_sqr_lo,poly_result_lo

	lda	2*FP_Bias-1,con_1		/* Inverted arg's exponent	*/
	subo	arg_exp,con_1,arg_exp

	setbit	arg_inv_bit,arg_flags,arg_flags	/* Signal inversion	*/

	mov	poly_result,arg_mant


Latanf_10:
	emul	arg_mant,arg_mant,mant_sqr_lo	/* Square mant bits		*/
	lda	FP_Bias-1,con_1
	subo	arg_exp,con_1,step_shift	/* Poly step shifting		*/
	shlo	1,step_shift,step_shift
	lda	Lpoly_table-.-8(ip),mem_ptr	/* Compute poly			*/
	mov	Lpoly_table_len,tmp_1		/* Num of consts in memory	*/
	lda	Lpoly_init,poly_result		/* First constant from instr	*/

Latanf_14:
	emul	poly_result,mant_sqr_hi,poly_result_lo
	ld	(mem_ptr),con_1
	addo	4,mem_ptr,mem_ptr
	cmpdeco	1,tmp_1,tmp_1
	shro	step_shift,poly_result,poly_result
	subo	poly_result,con_1,poly_result
	bne	Latanf_14

	emul	arg_mant,poly_result,poly_result_lo	/* "Odd" power		*/

Latanf_20:
	bbs.f	arg_inv_bit,arg_flags,Latanf_42	/* J/ inverted argument		*/
	bbs.f	arg_refl_bit,arg_flags,Latanf_40	/* J/ reflected result		*/

Latanf_25:
	bbs.t	31,poly_result,Latanf_30		/* J/ no norm shift for result	*/

	subo	1,arg_exp,arg_exp		/* Decr result exponent		*/
	shlo1(poly_result,poly_result)		/* Left shift result mant	*/

Latanf_30:
	cmpibge.f 0,arg_exp,Latanf_32		/* J/ denormal or zero result	*/

Latanf_31:
	chkbit	rslt_neg_bit,arg_flags		/* Negative result bit		*/
	shlo	23,arg_exp,arg_exp		/* Position result exp		*/
	alterbit 31,arg_exp,arg_exp		/* Add sign bit			*/
	clrbit	31,poly_result,poly_result	/* Delete j bit			*/
	chkbit	7,poly_result			/* Rounding bit into carry	*/
	shro	8,poly_result,poly_result	/* Position result mantissa	*/
	addc	poly_result,arg_exp,out		/* Combine/round result		*/
	ret

Latanf_32:
	subo	arg_exp,1+8,tmp_1
	addo	arg_exp,32-1-8,tmp_2
	lda	32,tmp_3
	cmpobg.f tmp_1,tmp_3,Latanf_34		/* J/ unfl to zero		*/

	chkbit	rslt_neg_bit,arg_flags		/* Negative result bit		*/
	shro	tmp_1,poly_result,out		/* Denormalize mantissa		*/
	alterbit 31,out,out			/* Impose sign bit		*/

	shlo	tmp_2,poly_result,tmp_2		/* MS bit is round bit		*/
	addc	tmp_2,tmp_2,tmp_2		/* Rounding bit to carry	*/
	addc	0,out,out			/* Round result			*/
	ret

Latanf_34:
	chkbit	rslt_neg_bit,arg_flags
	alterbit 31,0,out			/* Signed zero result		*/
	ret


Latanf_40:
	lda	PI_32_bit,con_1

	mov	arg_exp,step_shift
	lda	PI_exp,arg_exp

	subo	step_shift,arg_exp,step_shift	/* alignment shift count	*/

	shro	step_shift,poly_result,poly_result
	subo	poly_result,con_1,poly_result
	b	Latanf_31			/* Round/sign/move - done	*/


Latanf_42:
	lda	PI_32_bit,con_1

	mov	arg_exp,step_shift
	lda	PI_exp-1,arg_exp

	chkbit	arg_refl_bit,arg_flags
	subo	step_shift,arg_exp,step_shift	/* alignment shift count	*/
	shro	step_shift,poly_result,poly_result
	bno	Latanf_44			/* J/ inv set, relf clear	*/

	cmpo	1,0
	addc	poly_result,con_1,poly_result
	addc	0,0,tmp_1
	shro	tmp_1,poly_result,poly_result
	addo	tmp_1,arg_exp,arg_exp
	b	Latanf_31			/* Round/sign/move - done	*/

Latanf_44:
	subo	poly_result,con_1,poly_result
	b	Latanf_25			/* Round/sign/move - done	*/



Latanf_50:					/* arg magnitude = 1.0		*/
	bbs.f	arg_refl_bit,arg_flags,Latanf_52	/* J/ quadrant 2 or 3		*/

	chkbit	rslt_neg_bit,arg_flags
	lda	FP_PI_over_4,out		/* PI/4 result magnitude	*/
	alterbit 31,out,out			/* Copy arg sign		*/
	ret

Latanf_52:
	chkbit	rslt_neg_bit,arg_flags
	lda	FP_3_PI_over_4,out		/* 3/4 PI result magnitude	*/
	alterbit 31,out,out			/* Copy arg sign		*/
	ret



Latanf_70:					/* atan(x) = x approx		*/
/* ***	mov	s1,out  *** */
	ret


Latanf_special:
	bne.f	Latanf_sp_01			/* J/ NaN/INF			*/

/* ***	mov	s1,out  *** */			/* approx atan(x) = x		*/
	ret

Latanf_sp_01:
	shlo	9,s1,tmp_1			/* Strip sign bit and exp	*/
	cmpobne.f 0,tmp_1,Latanf_sp_03		/* J/ NaN			*/

Latanf_sp_02:
	chkbit	rslt_neg_bit,arg_flags		/* INF arg			*/
	lda	FP_PI_over_2,out		/* Result magnitude = PI/2	*/

	alterbit 31,out,out			/* Copy sign of arg to result	*/
	ret

Latanf_sp_03:
	mov	0,g1				/* Fake second operand		*/
	b	__AFP_NaN_S			/* Non-signaling NaN handler	*/
