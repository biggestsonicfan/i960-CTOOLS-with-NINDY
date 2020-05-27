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

	.file	"a_atan.s"
	.globl	_atan
	.globl	_atan2


	.globl	__AFP_NaN_D

	.globl	__errno_ptr		/* returns addr of ERRNO in g0		*/

#define	EDOM	33



#define	DP_Bias         0x3ff
#define	DP_INF          0x7ff


/* Register Name Equates */

#define	arg_lo		g0
#define	arg_hi		g1

#define	y_arg_lo	g0
#define	y_arg_hi	g1
#define	y_arg		y_arg_lo

#define	x_arg_lo	g2
#define	x_arg_hi	g3
#define	x_arg		x_arg_lo


#define	arg_flags	r3
#define	arg_inv_bit	0
#define	arg_refl_bit	1
#define	rmdr_neg_bit	2
#define	rslt_neg_bit	3

#define	arg_mant_lo	r4
#define	arg_mant_hi	r5
#define	arg_mant	arg_mant_lo

#define	rslt_mant_lo	r4
#define	rslt_mant_hi	r5

#define	arg_exp		r6
#define	rslt_exp	r6

#define	tmp		r7

#define	tmp_rp_1_lo	r8
#define tmp_rp_1_hi	r9
#define	tmp_rp_2_lo	r10
#define	tmp_rp_2_hi	r11

#define	mant_sqr_lo	r12
#define	mant_sqr_hi	r13

#define	poly_result_lo	r14
#define	poly_result_hi  r15
#define	poly_result     poly_result_lo

#define	y_exp		arg_exp
#define	y_mant_lo	arg_mant_lo
#define	y_mant_hi	arg_mant_hi
#define	y_mant		y_mant_lo

#define	x_exp		mant_sqr_lo
#define	x_mant_lo	poly_result_lo
#define	x_mant_hi	poly_result_hi
#define	x_mant		poly_result

#define	tmp_r		tmp_rp_1_lo
#define tmp_l		tmp_rp_1_hi

#define	left_shift	g7

#define	step_shift	g6

#define	con_lo		g4
#define	con_hi		g5

#define	mem_ptr		g3

#define	atan_center	g2

#define	out_lo		g0
#define	out_hi		g1
#define	out		out_lo



#define	PI_64_bit_hi	0xC90FDAA2
#define	PI_64_bit_lo	0x2168C235
#define	PI_exp		DP_Bias+1


#define	DP_PI_hi		0x400921FB
#define	DP_PI_lo		0x54442D18
#define	DP_PI_ext		0x469898CC

#define	DP_PI_over_2_hi		0x3FF921FB
#define	DP_PI_over_2_lo		DP_PI_lo

#define	DP_PI_over_4_hi		0x3FE921FB
#define	DP_PI_over_4_lo		DP_PI_lo

#define	DP_3_PI_over_4_hi	0x4002D97C
#define	DP_3_PI_over_4_lo	0x7F3321D2


	.text
	.link_pix

#if	defined(CA_optim)
	.align	5
#else
	.align	4
#endif


/*
 *  Polynomial approximation to ATAN for the interval  [ -1/128, 1/128 ]
 *
 *  Number format:	1/i  .  63/frac		"long" (64-bit) value
 *
 *  Number value:       i.frac
 */

Lpoly_table:
	.word	0x49249249,0x12492492			/* c7 = 1/7 */
	.word	0x9999999a,0x19999999			/* c5 = 1/5 */
	.word	0xaaaaaaab,0x2aaaaaaa			/* c3 = 1/3 */
	.word	0x00000000,0x80000000			/* c1 = 1   */
	.set	Lpoly_table_len,(.-Lpoly_table) >> 3

#define	short_poly_con_1_hi	0x2aaaaaab
#define	short_poly_con_0_lo	0x00000000
#define	short_poly_con_0_hi	0x80000000

/*
 *  Arctangent value table
 *  ----------------------
 *
 *  Number format:	1/1  .  60/frac  3/exp		"long" (64-bit) value
 *
 *  Number value:       1.frac * 2^(exp-0x7)
 *
 *  (Note: exp field is LS 3 bits of IEEE exponent field for value)
 */
Latan_table:
	.word	0x8-8 + 0xDB94D5C0, 0xFFFAAADD	/*  ATAN( 1/64)  */
	.word	0x9-8 + 0x4BB12540, 0xFFEAADDD	/*  ATAN( 2/64)  */
	.word	0xA-8 + 0x86D14FD0, 0xBFDC0C21	/*  ATAN( 3/64)  */
	.word	0xA-8 + 0x67EF4E38, 0xFFAADDB9	/*  ATAN( 4/64)  */
	.word	0xB-8 + 0xE2ACEB58, 0x9FACF873	/*  ATAN( 5/64)  */
	.word	0xB-8 + 0x17887460, 0xBF70C130	/*  ATAN( 6/64)  */
	.word	0xB-8 + 0x783E1BF0, 0xDF1CF5F3	/*  ATAN( 7/64)  */
	.word	0xB-8 + 0x617B6E30, 0xFEADD4D5	/*  ATAN( 8/64)  */
	.word	0xC-8 + 0x21B93728, 0x8F0FD7D8	/*  ATAN( 9/64)  */
	.word	0xC-8 + 0x331362C0, 0x9EB77746	/*  ATAN(10/64)  */
	.word	0xC-8 + 0xF6134EF8, 0xAE4C08F1	/*  ATAN(11/64)  */
	.word	0xC-8 + 0x72D81138, 0xBDCBDA5E	/*  ATAN(12/64)  */
	.word	0xC-8 + 0x643130E8, 0xCD35474B	/*  ATAN(13/64)  */
	.word	0xC-8 + 0x93051020, 0xDC86BA94	/*  ATAN(14/64)  */
	.word	0xC-8 + 0x02B9B390, 0xEBBEAEF9	/*  ATAN(15/64)  */
	.word	0xC-8 + 0x6406EB18, 0xFADBAFC9	/*  ATAN(16/64)  */
	.word	0xD-8 + 0xC31B12C8, 0x84EE2CBE	/*  ATAN(17/64)  */
	.word	0xD-8 + 0x5F8BC130, 0x8C5FAD18	/*  ATAN(18/64)  */
	.word	0xD-8 + 0xBF7A2DF0, 0x93C1B902	/*  ATAN(19/64)  */
	.word	0xD-8 + 0x3F5E5E68, 0x9B13B9B8	/*  ATAN(20/64)  */
	.word	0xD-8 + 0x15784D48, 0xA25521B6	/*  ATAN(21/64)  */
	.word	0xD-8 + 0x8E6A4ED8, 0xA9856CCA	/*  ATAN(22/64)  */
	.word	0xD-8 + 0x4E7F0CB0, 0xB0A42018	/*  ATAN(23/64)  */
	.word	0xD-8 + 0x26F78478, 0xB7B0CA0F	/*  ATAN(24/64)  */
	.word	0xD-8 + 0x1D9FBAD8, 0xBEAB025B	/*  ATAN(25/64)  */
	.word	0xD-8 + 0x50D92B70, 0xC59269CA	/*  ATAN(26/64)  */
	.word	0xD-8 + 0x6B58C340, 0xCC66AA2A	/*  ATAN(27/64)  */
	.word	0xD-8 + 0x611FE5B8, 0xD327761E	/*  ATAN(28/64)  */
	.word	0xD-8 + 0x32E36360, 0xD9D488ED	/*  ATAN(29/64)  */
	.word	0xD-8 + 0x764F7C68, 0xE06DA64A	/*  ATAN(30/64)  */
	.word	0xD-8 + 0x609A84B8, 0xE6F29A19	/*  ATAN(31/64)  */
	.word	0xD-8 + 0x0DDA7B48, 0xED63382B	/*  ATAN(32/64)  */
	.word	0xD-8 + 0xBAD1A220, 0xF3BF5BF8	/*  ATAN(33/64)  */
	.word	0xD-8 + 0xA0A0BE60, 0xFA06E85A	/*  ATAN(34/64)  */
	.word	0xE-8 + 0x0D205C98, 0x801CE39E	/*  ATAN(35/64)  */
	.word	0xE-8 + 0xD9867E28, 0x832BF4A6	/*  ATAN(36/64)  */
	.word	0xE-8 + 0xDA1ED068, 0x8630A2DA	/*  ATAN(37/64)  */
	.word	0xE-8 + 0xDE9547B8, 0x892AECDF	/*  ATAN(38/64)  */
	.word	0xE-8 + 0xF3E09B90, 0x8C1AD445	/*  ATAN(39/64)  */
	.word	0xE-8 + 0xF7F59F98, 0x8F005D5E	/*  ATAN(40/64)  */
	.word	0xE-8 + 0x64F350E0, 0x91DB8F16	/*  ATAN(41/64)  */
	.word	0xE-8 + 0x847186F8, 0x94AC72C9	/*  ATAN(42/64)  */
	.word	0xE-8 + 0x365E5390, 0x97731420	/*  ATAN(43/64)  */
	.word	0xE-8 + 0x71BDDA20, 0x9A2F80E6	/*  ATAN(44/64)  */
	.word	0xE-8 + 0xA0B8CDB8, 0x9CE1C8E6	/*  ATAN(45/64)  */
	.word	0xE-8 + 0xF4B7A1F0, 0x9F89FDC4	/*  ATAN(46/64)  */
	.word	0xE-8 + 0xCADAAE08, 0xA22832DB	/*  ATAN(47/64)  */
	.word	0xE-8 + 0x34F70928, 0xA4BC7D19	/*  ATAN(48/64)  */
	.word	0xE-8 + 0xB7602298, 0xA746F2DD	/*  ATAN(49/64)  */
	.word	0xE-8 + 0x4830F5C8, 0xA9C7ABDC	/*  ATAN(50/64)  */
	.word	0xE-8 + 0x997DD6A0, 0xAC3EC0FB	/*  ATAN(51/64)  */
	.word	0xE-8 + 0xB4D8C080, 0xAEAC4C38	/*  ATAN(52/64)  */
	.word	0xE-8 + 0xEBDC6F68, 0xB110688A	/*  ATAN(53/64)  */
	.word	0xE-8 + 0x1F043690, 0xB36B31C9	/*  ATAN(54/64)  */
	.word	0xE-8 + 0x59ECC4B0, 0xB5BCC490	/*  ATAN(55/64)  */
	.word	0xE-8 + 0xC2319E78, 0xB8053E2B	/*  ATAN(56/64)  */
	.word	0xE-8 + 0xD4707830, 0xBA44BC7D	/*  ATAN(57/64)  */
	.word	0xE-8 + 0xE98AF280, 0xBC7B5DEA	/*  ATAN(58/64)  */
	.word	0xE-8 + 0xFD049AB0, 0xBEA94144	/*  ATAN(59/64)  */
	.word	0xE-8 + 0xAC526640, 0xC0CE85B8	/*  ATAN(60/64)  */
	.word	0xE-8 + 0x661628B8, 0xC2EB4ABB	/*  ATAN(61/64)  */
	.word	0xE-8 + 0xBF8FBD58, 0xC4FFAFFA	/*  ATAN(62/64)  */
	.word	0xE-8 + 0xE602EE18, 0xC70BD54C	/*  ATAN(63/64)  */
	.word	0xE-8 + 0x2168C238, 0xC90FDAA2	/*  ATAN(64/64)  */



_atan2:
	addc	y_arg_hi,y_arg_hi,y_exp		/* 1 bit left, sign -> carry	*/

#if	defined(CA_optim)
	eshro	32-11,y_arg,y_mant_hi		/* Left justify mantissa	*/
#else
	shlo	11,y_arg_hi,y_mant_hi		/* Left justify mantissa	*/
	shro	32-11,y_arg_lo,tmp
	or	y_mant_hi,tmp,y_mant_hi
#endif
	shlo	11,y_arg_lo,y_mant_lo

	shri	32-11,y_exp,tmp

	alterbit rslt_neg_bit,0,arg_flags	/* Retain Y sign bit		*/
	addlda(1,tmp,tmp)

	cmpo	tmp,1
	shro	32-11,y_exp,y_exp
	ble.f	Ly_special			/* J/ Y is 0/denorm/INF/NaN	*/

	setbit	31,y_mant_hi,y_mant_hi		/* Set j bit			*/

Ly_rejoin:
	addc	x_arg_hi,x_arg_hi,x_exp		/* 1 bit left, sign -> carry	*/

#if	defined(CA_optim)
	eshro	32-11,x_arg,x_mant_hi		/* Left justify mantissa	*/
#else
	shlo	11,x_arg_hi,x_mant_hi		/* Left justify mantissa	*/
	shro	32-11,x_arg_lo,tmp
	or	x_mant_hi,tmp,x_mant_hi
#endif
	shlo	11,x_arg_lo,x_mant_lo

	shri	32-11,x_exp,tmp

	alterbit arg_refl_bit,arg_flags,arg_flags	/* Retain X sign bit	*/
	addlda(1,tmp,tmp)

	cmpo	tmp,1
	shro	32-11,x_exp,x_exp
	ble.f	Lx_special			/* J/ Y is 0/denorm/INF/NaN	*/

	setbit	31,x_mant_hi,x_mant_hi		/* Set j bit			*/

Lx_rejoin:
	cmpi	y_exp,x_exp			/* Set no borrow		*/
	bg.f	Latan2_10			/* J/ |y_arg| > |x_arg|		*/
	bl	Latan2_05			/* J/ |y_arg| < |x_arg|		*/

	subc	x_mant_lo,y_mant_lo,tmp
	subc	x_mant_hi,y_mant_hi,tmp

	be.f	Latan2_10			/* J/ |y_arg| >= |x_arg|	*/

Latan2_05:
#if	defined(CA_optim)
	eshro	1,y_mant,y_mant_lo
#else
	shro	1,y_mant_lo,y_mant_lo
	shlo	31,y_mant_hi,tmp
	or	y_mant_lo,tmp,y_mant_lo
#endif
	shro	1,y_mant_hi,y_mant_hi

	ediv	x_mant_hi,y_mant,tmp_rp_1_lo	/* Begin Y/X division		*/

	subo	x_exp,y_exp,arg_exp		/* Compute result exponent	*/

	lda	DP_Bias(arg_exp),arg_exp

	mov    	0,tmp_rp_2_lo
	mov	tmp_rp_1_lo,tmp_rp_2_hi

	ediv	x_mant_hi,tmp_rp_2_lo,tmp_rp_2_lo

	mov	tmp_rp_1_hi,arg_mant_hi

	shro	1,x_mant_lo,tmp_rp_1_hi		/* Compute correction term	*/
	shlo	31,x_mant_lo,tmp_rp_1_lo

	ediv	x_mant_hi,tmp_rp_1_lo,tmp_rp_1_lo	/* Compute C/B		*/

	emul	tmp_rp_1_hi,arg_mant_hi,tmp_rp_1_lo	/* Compute C/B * A/B	*/

	addc	tmp_rp_1_lo,tmp_rp_1_lo,tmp_rp_1_lo	/* Align to A/B		*/
	addc	tmp_rp_1_hi,tmp_rp_1_hi,tmp_rp_1_lo
	addc	0,0,tmp_rp_1_hi

	cmpo	0,0					/* A/B - C/B * A/B	*/
	subc	tmp_rp_1_lo,tmp_rp_2_hi,arg_mant_lo
	subc	tmp_rp_1_hi,arg_mant_hi,arg_mant_hi

	b	Latan2_12

Latan2_10:
#if	defined(CA_optim)
	eshro	1,x_mant,x_mant_lo
#else
	shro	1,x_mant_lo,x_mant_lo
	shlo	31,x_mant_hi,tmp
	or	x_mant_lo,tmp,x_mant_lo
#endif
	shro	1,x_mant_hi,x_mant_hi		/* Begin X/Y division		*/

	ediv	y_mant_hi,x_mant,tmp_rp_1_lo

	setbit	arg_inv_bit,arg_flags,arg_flags

	subo	y_exp,x_exp,arg_exp		/* Compute result exponent	*/

	lda	DP_Bias(arg_exp),arg_exp

	mov    	0,tmp_rp_2_lo
	mov	tmp_rp_1_lo,tmp_rp_2_hi

	ediv	y_mant_hi,tmp_rp_2_lo,tmp_rp_2_lo

	mov	tmp_rp_1_hi,tmp

	shro	1,y_mant_lo,tmp_rp_1_hi		/* Compute correction term	*/
	shlo	31,y_mant_lo,tmp_rp_1_lo

	ediv	y_mant_hi,tmp_rp_1_lo,tmp_rp_1_lo	/* Compute C/B		*/

	emul	tmp_rp_1_hi,tmp,tmp_rp_1_lo		/* Compute C/B * A/B	*/

	addc	tmp_rp_1_lo,tmp_rp_1_lo,tmp_rp_1_lo	/* Align to A/B		*/
	addc	tmp_rp_1_hi,tmp_rp_1_hi,tmp_rp_1_lo
	addc	0,0,tmp_rp_1_hi

	cmpo	0,0					/* A/B - C/B * A/B	*/
	subc	tmp_rp_1_lo,tmp_rp_2_hi,arg_mant_lo
	subc	tmp_rp_1_hi,tmp,arg_mant_hi

Latan2_12:
	chkbit	31,arg_mant_hi
	bo.f	Latan_10					/* J/ normalized	*/

	addc	arg_mant_lo,arg_mant_lo,arg_mant_lo	/* Single norm shift	*/
	subo	1,arg_exp,arg_exp			/* Adjust exponent	*/
	addc	arg_mant_hi,arg_mant_hi,arg_mant_hi
	b	Latan_10



/*  Y_ARG is a special case (0.0, denormalized value, INF, or NaN)  */

Ly_special:
	bne.f	Ly_sp_20				/* J/ INF/NaN			*/

	scanbit	y_mant_hi,tmp
	bno	Ly_sp_02				/* J/ hi word zero		*/

	subo	tmp,31,tmp_rp_1_lo		/* Left shift to normalize	*/
	addlda(1,tmp,tmp_rp_1_hi)		/* Right shift for wd -> wd	*/

#if	defined(CA_optim)
	eshro	tmp_rp_1_hi,y_mant,y_mant_hi	/* Left justify denorm value	*/
#else
	shlo	tmp_rp_1_lo,y_mant_hi,y_mant_hi	/* Left justify denorm value	*/
	shro	tmp_rp_1_hi,y_mant_lo,tmp
	or	y_mant_hi,tmp,y_mant_hi
#endif
	shlo	tmp_rp_1_lo,y_mant_lo,y_mant_lo

	subo	tmp_rp_1_lo,1,y_exp		/* Effective exponent value	*/
	b	Ly_rejoin

Ly_sp_02:
	scanbit	y_mant_lo,tmp
	bno	Ly_sp_04				/* J/ y_mant is zero		*/

	subo	tmp,31,tmp_rp_1_lo		/* Left shift to normalize	*/

	shlo	tmp_rp_1_lo,y_mant_lo,y_mant_hi	/* Left justify denorm value	*/
	movlda(0,y_mant_lo)

	subo	tmp_rp_1_lo,0,y_exp		/* Effective exponent		*/
	subo	31,y_exp,y_exp
	b	Ly_rejoin

Ly_sp_04:					/* Y is 0.0			*/
	addo	x_arg_hi,x_arg_hi,tmp_rp_1_hi
	or	x_arg_lo,tmp_rp_1_hi,tmp
	cmpobe.f 0,tmp,Ly_sp_10			/* J/ atan2(0,0) -> set errno	*/

	lda	DP_INF << 21,con_hi
	cmpo	0,0
	subc	x_arg_lo,0,tmp
	subc	tmp_rp_1_hi,con_hi,tmp
	be.t	Ly_sp_08				/* J/ not a NaN			*/

Ly_sp_06:					/* At least one arg is a NaN	*/
	b	__AFP_NaN_D

Ly_sp_08:					/* atan2(0,non-zero/non-NaN)	*/
	bbs	31,x_arg_hi,Lx_sp_24		/* J/ x is negative -> +/- PI	*/
/* ***	movl	y_arg,out  *** */
	ret

Ly_sp_10:					/* atan2(0,0) or atan2(INF,INF)	*/
	callj	__errno_ptr			/* returns addr of ERRNO in g0	*/
	ldconst	EDOM,tmp
	st	tmp,(g0)
	movl	0,out
	ret

Ly_sp_20:					/* Y is INF/NaN			*/
	shlo	1,y_mant_hi,y_mant_hi		/* Drop bottom exp bit		*/
	or	y_mant_lo,y_mant_hi,tmp
	cmpobne.f 0,tmp,Ly_sp_06			/* J/ Y is a NaN		*/

	addo	x_arg_hi,x_arg_hi,tmp		/* Drop sign bit		*/

	lda	DP_INF << 21,con_hi

	cmpo	x_arg_lo,0
	alterbit 0,tmp,tmp
	xor	1,tmp,tmp			/* LS bit set if lo word <> 0	*/

	cmpobg.f tmp,con_hi,Ly_sp_06		/* J/ atan2(INF,NaN)		*/
	be.f	Ly_sp_10				/* J/ atan2(INF,INF)		*/
	b	Latan_91				/* J/ atan2(INF,finite)		*/



/*  X_ARG is a special case (0.0, denormalized value, INF, or NaN), Y_arg  */
/*  is a non-zero, finite value.					   */

Lx_special:
	bne.f	Lx_sp_20				/* J/ INF/NaN			*/

	scanbit	x_mant_hi,tmp
	bno	Lx_sp_02				/* J/ hi word zero		*/

	subo	tmp,31,tmp_rp_1_lo		/* Left shift to normalize	*/
	addlda(1,tmp,tmp_rp_1_hi)		/* Right shift for wd -> wd	*/

#if	defined(CA_optim)
	eshro	tmp_rp_1_hi,x_mant,x_mant_hi	/* Left justify denorm value	*/
#else
	shlo	tmp_rp_1_lo,x_mant_hi,x_mant_hi	/* Left justify denorm value	*/
	shro	tmp_rp_1_hi,x_mant_lo,tmp
	or	x_mant_hi,tmp,x_mant_hi
#endif
	shlo	tmp_rp_1_lo,x_mant_lo,x_mant_lo

	subo	tmp_rp_1_lo,1,x_exp		/* Effective exponent value	*/
	b	Lx_rejoin

Lx_sp_02:
	scanbit	x_mant_lo,tmp
	bno	Latan_91				/* J/ x_arg is zero (+/- PI/2)	*/

	subo	tmp,31,tmp_rp_1_lo		/* Left shift to normalize	*/

	shlo	tmp_rp_1_lo,x_mant_lo,x_mant_hi	/* Left justify denorm value	*/
	movlda(0,x_mant_lo)

	subo	tmp_rp_1_lo,0,x_exp		/* Effective exponent		*/
	subo	31,x_exp,x_exp
	b	Lx_rejoin

Lx_sp_20:
	shlo	1,x_mant_hi,x_mant_hi		/* Drop bottom exp bit		*/
	or	x_mant_lo,x_mant_hi,tmp
	cmpobne.f 0,tmp,Ly_sp_06			/* J/ X is a NaN		*/

	bbs.f	arg_refl_bit,arg_flags,Lx_sp_24	/* J/ atan2(finite,-INF)	*/

	movl	0,out				/* atan2(finite,+INF) -> +/- 0	*/

Lx_sp_22:
	chkbit	rslt_neg_bit,arg_flags		/* Y_arg sign to result		*/
	alterbit 31,out_hi,out_hi
	ret

Lx_sp_24:
	lda	DP_PI_hi,out_hi			/* |result| = PI		*/
	lda	DP_PI_lo,out_lo
	b	Lx_sp_22				/* Transfer Y_arg sign		*/



#if	defined(CA_optim)
	.align	5
#else
	.align	4
#endif

_atan:
#if	defined(CA_optim)
	eshro	32-11,arg_lo,arg_mant_hi
#else
	shro	32-11,arg_lo,con_lo
	shlo	11,arg_hi,con_hi
	or	con_lo,con_hi,arg_mant_hi
#endif
	setbit	31,arg_mant_hi,arg_mant_hi	/* Force j bit			*/

	shlo	11,arg_lo,arg_mant_lo		/* Create left-justified mant	*/

	addc	arg_hi,arg_hi,tmp		/* Sign bit -> "carry"		*/

	alterbit rslt_neg_bit,0,arg_flags
	ldconst	DP_INF,con_lo

	shro	32-11,tmp,arg_exp		/* Biased, right justif exp	*/
	lda	DP_Bias,con_hi

	cmpobe.f arg_exp,con_lo,Latan_90		/* J/ NaN/INF			*/

	lda	DP_Bias-28,con_lo
	cmpoble.f arg_exp,con_lo,Latan_78	/* J/ zero/denorm/small		*/

	cmpobg.t con_hi,arg_exp,Latan_10		/* J/ no arg inversion		*/

	setbit	30,0,tmp_rp_1_hi		/* Constant of 1.0		*/
	movlda(0,tmp_rp_1_lo)

	ediv	arg_mant_hi,tmp_rp_1_lo,tmp_rp_2_lo	/* Invert argument	*/

	mov	tmp_rp_2_hi,tmp
	movldar(tmp_rp_2_lo,tmp_rp_1_hi)

	ediv	arg_mant_hi,tmp_rp_1_lo,tmp_rp_2_lo	/* 64-bit 1/mant_hi	*/

	shro	1,arg_mant_lo,tmp_rp_1_hi
	movlda(0,tmp_rp_1_lo)

	ediv	arg_mant_hi,tmp_rp_1_lo,tmp_rp_1_lo

	lda	2*DP_Bias,con_lo
	subo	arg_exp,con_lo,arg_exp			/* Inverted exp value	*/
	setbit	arg_inv_bit,arg_flags,arg_flags		/* Indicate inversion	*/

	emul	tmp,tmp_rp_1_hi,tmp_rp_1_lo		/* 64-bit div apx term	*/

	addc	tmp_rp_1_lo,tmp_rp_1_lo,tmp_rp_1_lo	/* Left shift apx term	*/
	addc	tmp_rp_1_hi,tmp_rp_1_hi,tmp_rp_1_lo
	addc	0,0,tmp_rp_1_hi

	cmpo	0,0

	subc	tmp_rp_1_lo,tmp_rp_2_hi,arg_mant_lo	/* 64-bit invert apx	*/
	subc	tmp_rp_1_hi,tmp,arg_mant_hi

	bbs.f	31,arg_mant_hi,Latan_10		/* J/ normalized value		*/

	addc	arg_mant_lo,arg_mant_lo,arg_mant_lo
	subo	1,arg_exp,arg_exp
	addc	arg_mant_hi,arg_mant_hi,arg_mant_hi

Latan_10:
	lda	DP_Bias-28,con_hi
	cmpible.f arg_exp,con_hi,Latan_70	/* J/ direct approximation	*/

	mov	0,atan_center			/* Default to 0 center point	*/
	lda	DP_Bias-8,con_hi

	cmpoble.f arg_exp,con_hi,Latan_20	/* J/ center value = 0 / 64	*/

	ldconst	DP_Bias+24,con_hi

	subo	arg_exp,con_hi,tmp		/* right shift cnt for 128ths	*/
	shro	tmp,arg_mant_hi,atan_center	/* 128ths of argument		*/
	shlo	tmp,2,con_lo			/* For negative remainder	*/
	subo	tmp,31,tmp			/* Shift count for remainder	*/
	chkbit	0,atan_center			/* Round-up bit to carry flag	*/
	addlda(1,atan_center,atan_center)	/* Round to nearest 64th	*/
	shro	1,atan_center,atan_center

	shlo	tmp,arg_mant_hi,tmp_rp_1_hi	/* Delete fraction bits		*/
	shro	tmp,tmp_rp_1_hi,tmp_rp_1_hi
	movldar(arg_mant_lo,tmp_rp_1_lo)

	bno	Latan_15				/* J/ not a negative remainder	*/

	setbit	rmdr_neg_bit,arg_flags,arg_flags	/* Negative remainder	*/

	subc	tmp_rp_1_lo,0,tmp_rp_1_lo	/* Negative remainder		*/
	subc	tmp_rp_1_hi,con_lo,tmp_rp_1_hi

Latan_15:
	ldconst	DP_Bias-1-(32-7),tmp		/* Compute shift for arg*cntr	*/
	subo	tmp,arg_exp,tmp
	shlo	tmp,atan_center,tmp
	emul	tmp,arg_mant_hi,tmp_rp_2_lo	/* Partial muls of arg*cntr	*/
	emul	tmp,arg_mant_lo,mant_sqr_lo

	addc	mant_sqr_lo,mant_sqr_lo,tmp	/* Combine partial products	*/
	addc	mant_sqr_hi,tmp_rp_2_lo,tmp_rp_2_lo
	addc	0,tmp_rp_2_hi,tmp_rp_2_hi
	setbit	31,tmp_rp_2_hi,tmp_rp_2_hi	/* 1 + arg*cntr			*/


	/* compute (arg-cntr)/(1 + arg*cntr) w/ division approximation */

	ediv	tmp_rp_2_hi,tmp_rp_1_lo,tmp_rp_1_lo
	mov	tmp_rp_1_hi,tmp			/* Division result		*/
	movldar(tmp_rp_1_lo,tmp_rp_1_hi)	/* Prepare for next div step	*/
/*	mov	0,tmp_rp_1_lo	*/		/* (don't care about ls bit	*/
	ediv	tmp_rp_2_hi,tmp_rp_1_lo,tmp_rp_1_lo
	mov	tmp_rp_1_hi,tmp_rp_1_lo		/* 64-bit "A/B" result		*/
	movldar(tmp,tmp_rp_1_hi)

	shro	1,tmp_rp_2_lo,mant_sqr_hi	/* Prepare for C/B		*/
	shlo	31,tmp_rp_2_lo,mant_sqr_lo
	ediv	tmp_rp_2_hi,mant_sqr_lo,mant_sqr_lo

	emul	mant_sqr_hi,tmp_rp_1_hi,mant_sqr_lo	/*  C/B * A/B		*/

	addc	mant_sqr_lo,mant_sqr_lo,mant_sqr_lo	/*  align to A/B	*/
	addc	mant_sqr_hi,mant_sqr_hi,mant_sqr_lo
	addc	0,0,mant_sqr_hi

	cmpo	0,0				/* Clear borrow bit		*/
	subc	mant_sqr_lo,tmp_rp_1_lo,arg_mant_lo	/* A/B - (C/B * A/B)	*/
	subc	mant_sqr_hi,tmp_rp_1_hi,arg_mant_hi

	scanbit	arg_mant_hi,tmp			/* Compute new exponent		*/

	subo	tmp,31,tmp_rp_1_lo		/* Normalizing shift counts	*/
	addlda(1,tmp,tmp_rp_1_hi)

	addo	arg_exp,tmp,tmp
	lda	DP_Bias-28+32,con_hi

	cmpoble.f tmp,con_hi,Latan_60		/* J/ direct appx of rmdr ATAN	*/

	subo	31,tmp,arg_exp			/* adjusted exponent		*/
	subo	1,arg_exp,arg_exp


/*  Normalize adjusted argument value  */

	shlo	tmp_rp_1_lo,arg_mant_hi,arg_mant_hi	/* (zero shift ok)	*/
	shro	tmp_rp_1_hi,arg_mant_lo,tmp
	or	arg_mant_hi,tmp,arg_mant_hi
	shlo	tmp_rp_1_lo,arg_mant_lo,arg_mant_lo

Latan_20:					/* Re-join for center = 0	*/
	emul	arg_mant_hi,arg_mant_hi,mant_sqr_lo

	lda	DP_Bias-1,con_hi

	subo	arg_exp,con_hi,step_shift	/* begin computing step shift	*/
	lda	Lpoly_table-.-8(ip),mem_ptr	/* Compute denom poly		*/

	movl	arg_mant_lo,poly_result_lo	/* Just in case ...		*/
	movlda(Lpoly_table_len-1,tmp)		/* Num of consts in memory	*/

	emul	arg_mant_hi,arg_mant_lo,tmp_rp_1_lo

	shlo	1,step_shift,step_shift
	ldl	(mem_ptr),poly_result_lo	/* First constant from table	*/

	cmpobl.f 31,step_shift,Latan_64		/* J/ single approximation	*/

	addlda(8,mem_ptr,mem_ptr)

	addc	tmp_rp_1_lo,tmp_rp_1_lo,tmp_rp_1_lo	/* Finish computing	*/
	addc	tmp_rp_1_hi,tmp_rp_1_hi,tmp_rp_1_lo	/* mant squared		*/
	addc	0,0,tmp_rp_1_hi

	addc	tmp_rp_1_lo,mant_sqr_lo,mant_sqr_lo
	addc	tmp_rp_1_hi,mant_sqr_hi,mant_sqr_hi

#if	!defined(CA_optim)
	ldconst	32,left_shift
	subo	step_shift,left_shift,left_shift
#endif

Latan_30:					/* Polynomial approximation	*/
	emul	poly_result_lo,mant_sqr_hi,tmp_rp_1_lo
	ldl	(mem_ptr),con_lo
	addo	8,mem_ptr,mem_ptr
	emul	poly_result_hi,mant_sqr_lo,tmp_rp_2_lo
	emul	poly_result_hi,mant_sqr_hi,poly_result_lo

	addc	tmp_rp_2_lo,tmp_rp_1_lo,tmp_rp_1_lo
	addc	tmp_rp_2_hi,tmp_rp_1_hi,tmp_rp_1_lo
	addc	0,0,tmp_rp_1_hi

	addc	tmp_rp_1_lo,poly_result_lo,poly_result_lo
	addc	tmp_rp_1_hi,poly_result_hi,poly_result_hi

#if	defined(CA_optim)
	eshro	step_shift,poly_result_lo,poly_result_lo
	shro	step_shift,poly_result_hi,poly_result_hi
#else
	shlo	left_shift,poly_result_hi,tmp_rp_2_lo
	shro	step_shift,poly_result_hi,poly_result_hi
	shro	step_shift,poly_result_lo,poly_result_lo
	or	tmp_rp_2_lo,poly_result_lo,poly_result_lo
#endif
	subc	poly_result_lo,con_lo,poly_result_lo
	subc	poly_result_hi,con_hi,poly_result_hi

	cmpdeco	1,tmp,tmp
	bne.t	Latan_30

Latan_32:
	emul	poly_result_lo,arg_mant_hi,tmp_rp_1_lo
	lda	Latan_table-8-.-8(ip),mem_ptr	/* Fetch center atan	*/
	emul	poly_result_hi,arg_mant_lo,tmp_rp_2_lo
	ldl	(mem_ptr)[atan_center*8],con_lo
	emul	poly_result_hi,arg_mant_hi,poly_result_lo

	addc	tmp_rp_2_lo,tmp_rp_1_lo,tmp_rp_1_lo	/* Finish poly approx	*/
	addc	tmp_rp_2_hi,tmp_rp_1_hi,tmp_rp_1_lo
	addc	0,0,tmp_rp_1_hi

	addc	tmp_rp_1_lo,poly_result_lo,poly_result_lo
	addc	tmp_rp_1_hi,poly_result_hi,poly_result_hi

/*
 *  Left shift and move poly result to insure non-negative alignment shift
 */
	addc	poly_result_lo,poly_result_lo,rslt_mant_lo
	addc	poly_result_hi,poly_result_hi,rslt_mant_hi

	cmpobe.f 0,atan_center,Latan_38		/* J/ center point = 0.0	*/


	shro	1,step_shift,step_shift		/* Compute alignment shift	*/

	and	7,con_lo,rslt_exp		/* Table value exponent snipit	*/

	addo	step_shift,rslt_exp,step_shift
	subo	6,step_shift,step_shift		/* Relative shift count		*/

	lda	DP_Bias-7(rslt_exp),rslt_exp	/* Reconstituted rslt exponent	*/

	chkbit	rmdr_neg_bit,arg_flags

#if	defined(CA_optim)
	eshro	step_shift,rslt_mant_lo,rslt_mant_lo
	shro	step_shift,rslt_mant_hi,rslt_mant_hi
#else
	ldconst	32,left_shift
	subo	step_shift,left_shift,left_shift
	shlo	left_shift,rslt_mant_hi,tmp_rp_2_lo
	shro	step_shift,rslt_mant_lo,rslt_mant_lo
	or	tmp_rp_2_lo,rslt_mant_lo,rslt_mant_lo
	shro	step_shift,rslt_mant_hi,rslt_mant_hi
#endif

Latan_33:
	bo	Latan_34			/* J/ subtract value			*/

	addc	rslt_mant_lo,con_lo,rslt_mant_lo
	addc	rslt_mant_hi,con_hi,rslt_mant_hi

	addc	0,0,tmp
	cmpobe.t 0,tmp,Latan_40		/* J/ no carry out			*/

#if	defined(CA_optim)
	eshro	1,rslt_mant_lo,rslt_mant_lo
#else
	shro	1,rslt_mant_lo,rslt_mant_lo
	shlo	31,rslt_mant_hi,tmp
	or	rslt_mant_lo,tmp,rslt_mant_lo
#endif
	shro	1,rslt_mant_hi,rslt_mant_hi
	setbit	31,rslt_mant_hi,rslt_mant_hi

	addlda(1,rslt_exp,rslt_exp)

	b	Latan_40

Latan_34:
	subc	rslt_mant_lo,con_lo,rslt_mant_lo
	subc	rslt_mant_hi,con_hi,rslt_mant_hi


Latan_38:					/* c = 0 (arg_exp -> rslt_exp)	*/

	bbs.t	31,rslt_mant_hi,Latan_40		/* J/ still normalized		*/

	addc	rslt_mant_lo,rslt_mant_lo,rslt_mant_lo
	subo	1,rslt_exp,rslt_exp
	addc	rslt_mant_hi,rslt_mant_hi,rslt_mant_hi


Latan_40:
	bbs.f	arg_inv_bit,arg_flags,Latan_48	/* J/ inverted argument		*/
	bbs.f	arg_refl_bit,arg_flags,Latan_46	/* J/ reflected result		*/

Latan_45:
	chkbit	10,rslt_mant_lo			/* Rounding bit to carry	*/
	clrbit	31,rslt_mant_hi,rslt_mant_hi	/* Zero j bit			*/

#if	defined(CA_optim)
	eshro	11,rslt_mant_lo,rslt_mant_lo
#else
	shro	11,rslt_mant_lo,rslt_mant_lo
	shlo	32-11,rslt_mant_hi,tmp
	or	rslt_mant_lo,tmp,rslt_mant_lo
#endif
	shro	11,rslt_mant_hi,rslt_mant_hi
	shlo	20,rslt_exp,rslt_exp

	addc	0,rslt_mant_lo,out_lo		/* Round/move/combine		*/
	addc	rslt_exp,rslt_mant_hi,out_hi

	chkbit	rslt_neg_bit,arg_flags		/* Set sign bit			*/
	alterbit 31,out_hi,out_hi
	ret

Latan_46:
	lda	PI_64_bit_hi,con_hi
	lda	PI_64_bit_lo,con_lo

	mov	rslt_exp,step_shift
	lda	PI_exp,rslt_exp

	subo	step_shift,rslt_exp,step_shift	/* alignment shift count	*/

#if	defined(CA_optim)
	eshro	step_shift,rslt_mant_lo,rslt_mant_lo
	shro	step_shift,rslt_mant_hi,rslt_mant_hi
#else
	ldconst	32,left_shift
	subo	step_shift,left_shift,left_shift
	shro	step_shift,rslt_mant_lo,rslt_mant_lo
	shlo	left_shift,rslt_mant_hi,tmp
	or	rslt_mant_lo,tmp,rslt_mant_lo
	shro	step_shift,rslt_mant_hi,rslt_mant_hi
#endif

	cmpo	0,0

	subc	rslt_mant_lo,con_lo,rslt_mant_lo	/* Note: this subtract	*/
	subc	rslt_mant_hi,con_hi,rslt_mant_hi	/* can't denorm mant	*/

	b	Latan_45				/* J/ round/sign/move - done	*/

Latan_48:
	lda	PI_64_bit_hi,con_hi
	lda	PI_64_bit_lo,con_lo

	mov	rslt_exp,step_shift
	lda	PI_exp-1,rslt_exp

	subo	step_shift,rslt_exp,step_shift	/* alignment shift count	*/

	chkbit	arg_refl_bit,arg_flags

#if	defined(CA_optim)
	eshro	step_shift,rslt_mant_lo,rslt_mant_lo
	shro	step_shift,rslt_mant_hi,rslt_mant_hi
#else
	ldconst	32,left_shift
	subo	step_shift,left_shift,left_shift
	shro	step_shift,rslt_mant_lo,rslt_mant_lo
	shlo	left_shift,rslt_mant_hi,tmp
	or	rslt_mant_lo,tmp,rslt_mant_lo
	shro	step_shift,rslt_mant_hi,rslt_mant_hi
#endif

	bno	Latan_49			/* J/ inv set, refl clear		*/

	addc	con_lo,rslt_mant_lo,rslt_mant_lo
	addc	con_hi,rslt_mant_hi,rslt_mant_hi

	addc	0,0,tmp
	cmpobe.t 0,tmp,Latan_45		/* J/ assume no carry out		*/

#if	defined(CA_optim)
	eshro	1,rslt_mant_lo,rslt_mant_lo
#else
	shro	1,rslt_mant_lo,rslt_mant_lo
	shlo	31,rslt_mant_hi,tmp
	or	rslt_mant_lo,tmp,rslt_mant_lo
#endif

	shro	1,rslt_mant_hi,rslt_mant_hi
	setbit	31,rslt_mant_hi,rslt_mant_hi

	addlda(1,rslt_exp,rslt_exp)

	b	Latan_45

Latan_49:
	subc	rslt_mant_lo,con_lo,rslt_mant_lo
	subc	rslt_mant_hi,con_hi,rslt_mant_hi

	bbs.t	31,rslt_mant_hi,Latan_45		/* J/ value still normalized	*/

	addc	rslt_mant_lo,rslt_mant_lo,rslt_mant_lo
	subo	1,rslt_exp,rslt_exp
	addc	rslt_mant_hi,rslt_mant_hi,rslt_mant_hi

	b	Latan_45



/*
 *  Remainder (after division by 1+arg*center) atan is direct approximation.
 *  Fetch the center's atan, align the bits in arg_mant to that atan value,
 *  then resume flow for normal add/sub and completion processing
 */

Latan_60:
	lda	Latan_table-8-.-8(ip),mem_ptr	/* Base address of ATAN table	*/
	ldl	(mem_ptr)[atan_center*8],con_lo	/* Fetch ATAN constant		*/

	subo	1,arg_exp,tmp			/* Compute MS bit's exponent	*/

	and	7,con_lo,rslt_exp		/* Extract exponent field	*/
	lda	DP_Bias-7(rslt_exp),rslt_exp	/* Compute result exponent	*/

#if	defined(CA_optim)
	subo	tmp,rslt_exp,step_shift		/* Compute right shft (0 or 1)	*/

	eshro	step_shift,arg_mant_lo,rslt_mant_lo	/* Shift and move	*/
	shro	step_shift,arg_mant_hi,rslt_mant_hi
#else
	subo	tmp,rslt_exp,step_shift
	ldconst	32,tmp
	subo	step_shift,tmp,left_shift	/* Compute right shft (0 or 1)	*/

	shlo	left_shift,arg_mant_hi,tmp	/* Alignment shift and move	*/
	shro	step_shift,arg_mant_hi,rslt_mant_hi
	shro	step_shift,arg_mant_lo,rslt_mant_lo
	or	tmp,rslt_mant_lo,rslt_mant_lo
#endif

	chkbit	rmdr_neg_bit,arg_flags		/* Test add/sub bit to rejoin	*/
	b	Latan_33


/*
 *  When the atan approx has an argument with magnitude > 2^-27 and <=2^-16,
 *  a single term is used (i.e.,  atan(x') = x' - x'^3/3).  A separate
 *  evaluation is used because the term shift is greater than 31 bits (so
 *  the shift technique used in the basic loop would fail).
 */

Latan_64:				/* poly approx w/ step shift >= 32	*/
	lda	short_poly_con_1_hi,poly_result_hi	/* Const ld of C3 term	*/

	emul	poly_result_hi,mant_sqr_hi,poly_result_lo

	addo	1,31,tmp
	lda	short_poly_con_0_hi,con_hi		/* Const ld of C1 term	*/

	subo	tmp,step_shift,tmp			/* Excess 32 shift cnt	*/
	lda	short_poly_con_0_lo,con_lo

	shro	tmp,poly_result_hi,poly_result_hi

	subc	poly_result_hi,con_lo,poly_result_lo	/* Compute approx	*/
	subc	0,con_hi,poly_result_hi

	b	Latan_32


/*
 *  Argument magnitude < 2^-26.  No ATAN approximation is used (since only
 *  the first term is significant).  However, depending on the inversion
 *  flag and the quadrant, this magnitude may need to be subtracted from
 *  PI or added to/subtracted from PI/2.
 */

Latan_70:
	bbs.f	arg_inv_bit,arg_flags,Latan_76	/* J/ inverted arg (use PI/2)	*/
	bbs.f	arg_refl_bit,arg_flags,Latan_72	/* Near PI or -PI result	*/


/*  Pack (or re-pack) rslt_neg_bit/arg_exp/arg_mant for return  */

	cmpibge.f 0,arg_exp,Latan_70a		/* J/ denormal or unfl to zero	*/

	clrbit	31,arg_mant_hi,arg_mant_hi	/* Zap the j bit		*/
	chkbit	rslt_neg_bit,arg_flags		/* Sign bit of the result	*/
	shlo	32-1-11,arg_exp,arg_exp		/* Position the exponent	*/
	alterbit 31,arg_exp,arg_exp		/* Sign bit with exponent	*/

	chkbit	63-53,arg_mant_lo		/* Rounding bit			*/

#if	defined(CA_optim)
	eshro	1+11-1,arg_mant,arg_mant_lo	/* Position the mantissa bits	*/
#else
	shro	1+11-1,arg_mant_lo,arg_mant_lo	/* Position the mantissa bits	*/
	shlo	32-1-11+1,arg_mant_hi,tmp
	or	arg_mant_lo,tmp,arg_mant_lo
#endif
	shro	1+11-1,arg_mant_hi,arg_mant_hi

	addc	arg_mant_lo,0,out_lo		/* Combine sign/exp/mantissa	*/
	addc	arg_mant_hi,arg_exp,out_hi
	ret

Latan_70a:
	subo	arg_exp,12,tmp_r		/* Right shift for hi wd denorm	*/
	addo	arg_exp,20,tmp_l		/* Left shift wd -> wd xfer	*/
	cmpobl	31,tmp_r,Latan_70b		/* Top word of denorm is zero	*/

#if	defined(CA_optim)
	eshro	tmp_r,arg_mant,out_lo		/* Shift to denormalize		*/
#else
	shro	tmp_r,arg_mant_lo,out_lo	/* Shift to denormalize		*/
	shlo	tmp_l,arg_mant_hi,tmp
	or	out_lo,tmp,out_lo
#endif
	shro	tmp_r,arg_mant_hi,out_hi

	shlo	tmp_l,arg_mant_lo,tmp		/* MS bit of tmp is round bit	*/
	b	Latan_70d

Latan_70b:
	ldconst	32,tmp

	subo	tmp,tmp_r,tmp_r			/* Reduce right shift count	*/
	addo	tmp,tmp_l,tmp_l			/* Adjust wd -> wd shift count	*/

	cmpobl	31,tmp_r,Latan_70c		/* J/ shifted beyond lo word	*/

	shro	tmp_r,arg_mant_hi,out_lo
	movlda(0,out_hi)
	shlo	tmp_l,arg_mant_hi,tmp
	b	Latan_70d

Latan_70c:
	cmpo	tmp,tmp_r
	movl	0,out				/* All mant bits are zero	*/
	movlda(0,tmp)
	bne	Latan_70d			/* J/ beyond next lower bit	*/

	mov	arg_mant_hi,tmp			/* MS bit is rounding bit	*/

Latan_70d:
	addc	tmp,tmp,tmp			/* Rounding bit to carry	*/
	addc	0,out_lo,out_lo			/* Round the result		*/
	addc	0,out_hi,out_hi
	chkbit	rslt_neg_bit,arg_flags		/* Copy result sign		*/
	alterbit 31,out_hi,out_hi
	ret

Latan_72:
	clrbit	arg_refl_bit,arg_flags,arg_flags	/* for common code	*/

	lda	DP_Bias+1-21-2,tmp		/* PI Exp - MS wd mant bits-2	*/
	lda	DP_PI_hi,out_hi			/* DP PI w/ extention word	*/

Latan_73:					/* Common align-add/sub-rnd	*/
	subo	arg_exp,tmp,step_shift		/* Alignment shift		*/

	lda	DP_PI_lo,out_lo
	lda	DP_PI_ext,tmp_rp_1_hi		/* Extension bits for rounding	*/

	cmpobl.f 31,step_shift,Latan_75		/* J/ shifted to oblivion	*/

#if	defined(CA_optim)
	eshro	2,arg_mant_lo,arg_mant_lo
	shro	2,arg_mant_hi,arg_mant_hi
	eshro	step_shift,arg_mant_lo,arg_mant_lo
	shro	step_shift,arg_mant_hi,arg_mant_hi
#else
	ldconst	32,left_shift
	subo	step_shift,left_shift,left_shift

	shlo	32-2,arg_mant_hi,tmp
	shro	2,arg_mant_hi,arg_mant_hi
	shro	2,arg_mant_lo,arg_mant_lo
	or	tmp,arg_mant_lo,arg_mant_lo
	shlo	left_shift,arg_mant_hi,tmp
	shro	step_shift,arg_mant_hi,arg_mant_hi
	shro	step_shift,arg_mant_lo,arg_mant_lo
	or	tmp,arg_mant_lo,arg_mant_lo
#endif

	bbs	arg_refl_bit,arg_flags,Latan_74	/* J/ add the delta		*/

	subc	arg_mant_lo,tmp_rp_1_hi,tmp_rp_1_hi
	subc	arg_mant_hi,out_lo,out_lo
	subc	0,out_hi,out_hi
	b	Latan_75

Latan_74:
	addc	arg_mant_lo,tmp_rp_1_hi,tmp_rp_1_hi
	addc	arg_mant_hi,out_lo,out_lo
	addc	0,out_hi,out_hi

Latan_75:
	addc	tmp_rp_1_hi,tmp_rp_1_hi,tmp_rp_1_hi
	addc	0,out_lo,out_lo

	chkbit	rslt_neg_bit,arg_flags
	alterbit 31,out_hi,out_hi
	ret

Latan_76:
	lda	DP_Bias-21-2,tmp		/* PI/2 Exp-MS wd mant bits-2	*/
	lda	DP_PI_over_2_hi,out_hi		/* DP PI w/ extention word	*/
	b	Latan_73


Latan_78:					/* atan(x) = x approx		*/
/* ***	movl	s1,out  *** */
	ret


/*
 *  Argument is NaN or INF.  If INF, return +/- PI/2 depending on INF sign.
 *  If NaN, return that NaN after insuring that the two MS mantissa bits are
 *  set (i.e., force a quiet NaN return).  ATAN does not - at present - do
 *  any exception signaling.
 */

Latan_90:					/* NaN/INF argument		*/
	shlo	1+11,arg_hi,tmp			/* Drop sign bit and exponent	*/
	or	arg_lo,tmp,tmp
	cmpobne.f 0,tmp,Latan_92			/* J/ NaN argument		*/

Latan_91:
	chkbit	rslt_neg_bit,arg_flags		/* SIgn of result		*/
	lda	DP_PI_over_2_hi,out_hi		/* MS word of result		*/

	alterbit 31,out_hi,out_hi		/* Impose result sign		*/
	lda	DP_PI_over_2_lo,out_lo		/* LS word of result		*/
	ret

Latan_92:
	movl	0,g2				/* Fake second operand		*/
	b	__AFP_NaN_D			/* Non-signaling NaN handler	*/
