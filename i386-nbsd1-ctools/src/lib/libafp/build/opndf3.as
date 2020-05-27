/*******************************************************************************
 * 
 * Copyright (c) 1993,1994 Intel Corporation
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
/*      opndf3.c - Double Precision Basic Operation Routine (AFP-960)         */
/*									      */
/******************************************************************************/

#include "asmopt.h"

#if	!defined(KEEP_INTERNAL_LABELS)
#define	opd_as1_rejoin	L_opd_as1_rejoin
#define	opd_as2_rejoin	L_opd_as2_rejoin
#define	opd_a_01	L_opd_a_01
#define	opd_a_02	L_opd_a_02
#define	opd_AFP_RRC_D	L_opd_AFP_RRC_D
#define	opd_a_03	L_opd_a_03
#define	opd_a_04	L_opd_a_04
#define	opd_a_05	L_opd_a_05
#define	opd_ds2_rejoin	L_opd_ds2_rejoin
#define	opd_ds1_rejoin	L_opd_ds1_rejoin
#define	opd_r_adjust	L_opd_r_adjust
#define	opd_no_adjust	L_opd_no_adjust
#define	opd_ms1_rejoin	L_opd_ms1_rejoin
#define	opd_ms2_rejoin	L_opd_ms2_rejoin
#define	opd_a_06	L_opd_a_06
#define	opd_a_07	L_opd_a_07
#define	opd_as1_dshr	L_opd_as1_dshr
#define	opd_as1_tshr	L_opd_as1_tshr
#define	opd_as2_dshr	L_opd_as2_dshr
#define	opd_as2_tshr	L_opd_as2_tshr
#define	opd_as1_special	L_opd_as1_special
#define	opd_as1_04	L_opd_as1_04
#define	opd_as1_05a	L_opd_as1_05a
#define	opd_as1_05b	L_opd_as1_05b
#define	opd_as1_06	L_opd_as1_06
#define	opd_as1_08	L_opd_as1_08
#define	opd_as1_09	L_opd_as1_09
#define	opd_as1_10	L_opd_as1_10
#define	opd_as1_12	L_opd_as1_12
#define	opd_as1_14	L_opd_as1_14
#define	opd_as1_20	L_opd_as1_20
#define	opd_as1_22	L_opd_as1_22
#define	opd_as1_24	L_opd_as1_24
#define	opd_as1_26	L_opd_as1_26
#define	opd_as2_special	L_opd_as2_special
#define	opd_as2_12	L_opd_as2_12
#define	opd_as2_14	L_opd_as2_14
#define	opd_as2_20	L_opd_as2_20
#define	opd_ds2_special	L_opd_ds2_special
#define	opd_ds2_02	L_opd_ds2_02
#define	opd_ds2_04	L_opd_ds2_04
#define	opd_ds2_05	L_opd_ds2_05
#define	opd_ds2_06	L_opd_ds2_06
#define	opd_ds2_08	L_opd_ds2_08
#define	opd_ds2_10	L_opd_ds2_10
#define	opd_ds2_12	L_opd_ds2_12
#define	opd_ds2_14	L_opd_ds2_14
#define	opd_ds2_20	L_opd_ds2_20
#define	opd_ds2_22	L_opd_ds2_22
#define	opd_ds2_24	L_opd_ds2_24
#define	opd_ds2_25	L_opd_ds2_25
#define	opd_ds1_special	L_opd_ds1_special
#define	opd_ds1_12	L_opd_ds1_12
#define	opd_ds1_14	L_opd_ds1_14
#define	opd_ms1_special	L_opd_ms1_special
#define	opd_ms1_02	L_opd_ms1_02
#define	opd_ms1_04	L_opd_ms1_04
#define	opd_ms1_06	L_opd_ms1_06
#define	opd_ms1_10	L_opd_ms1_10
#define	opd_ms1_12	L_opd_ms1_12
#define	opd_ms1_14	L_opd_ms1_14
#define	opd_ms1_20	L_opd_ms1_20
#define	opd_ms1_22	L_opd_ms1_22
#define	opd_ms1_24	L_opd_ms1_24
#define	opd_ms1_25	L_opd_ms1_25
#define	opd_ms2_special	L_opd_ms2_special
#define	opd_ms2_12	L_opd_ms2_12
#define	opd_ms2_14	L_opd_ms2_14
#endif


	.file	"opndf3.as"
	.globl	___adddf3
	.globl	___divdf3
	.globl	___muldf3
	.globl	___subdf3


#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	AFP_RRC_D_2
	.globl	AFP_NaN_D

	.globl	_AFP_Fault_Invalid_Operation_D
	.globl	_AFP_Fault_Reserved_Encoding_D
	.globl	_AFP_Fault_Zero_Divide_D
	.globl	_AFP_Fault_Underflow_D


#define	DP_Bias         0x3ff
#define	DP_INF          0x7ff

#define	AC_Round_Mode   30
#define	Round_Mode_Down  1
#define	Round_Mode_even  0

#define	AC_Norm_Mode    29

#define	AC_Inex_mask    28
#define	AC_Inex_flag    20

#define	AC_InvO_mask    26
#define	AC_InvO_flag    18

#define	AC_Unfl_mask    25

#define	AC_ZerD_mask    27
#define	AC_ZerD_flag    19

#define	Standard_QNaN_hi  0xfff80000
#define	Standard_QNaN_lo  0x00000000


/* Register Name Equates */

#define	s1         g0
#define	s1_lo      s1
#define	s1_hi      g1
#define s2         g2
#define s2_lo      s2
#define s2_hi      g3

#define	s2_mant_x  r3
#define	s2_mant    r4
#define s2_mant_lo s2_mant
#define s2_mant_hi r5
#define	s2_exp     r6
#define	s2_sign    r7

#define	s1_mant_x  r10
#define s1_mant    r8
#define s1_mant_lo s1_mant
#define s1_mant_hi r9
#define	s1_exp     r11

#define	tmp2       r12
#define	tmp3       r13

#define	tmp        r14
#define	ac         r15

#define	con_1      g5

#define	con_2      g6


#define	out        g0
#define	out_lo     out
#define	out_hi     g1

#define	op_type    g4
#define	add_type   1
#define	div_type   2
#define	mul_type   3



/*  Multiplication register pairs  */

#define rp_A       tmp2
#define rp_A_lo    rp_A
#define rp_A_hi    tmp3

#define rp_B       g6
#define rp_B_lo    rp_B
#define rp_B_hi    g7

#define rp_C       g2
#define rp_C_lo    rp_C
#define rp_C_hi    g3



/*  Division register declarations  */

#define	rsl_mant_x   r3
#define	rsl_mant     r4
#define rsl_mant_lo  rsl_mant
#define rsl_mant_hi  r5
#define	rsl_exp      r6
#define	rsl_sign     r7

#define	dvsr_mant_x  r10
#define dvsr_mant    r8
#define dvsr_mant_lo dvsr_mant
#define dvsr_mant_hi r9
#define	dvsr_exp     r11

#define	quo          rsl_mant
#define	quo_lo       rsl_mant_lo
#define	quo_hi       rsl_mant_hi

#define	B            dvsr_mant_hi
#define	C            dvsr_mant_lo

#define	rmndr        rp_B
#define	rmndr_lo     rp_B_lo
#define	rmndr_hi     rp_B_hi



	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

___subdf3:
	notbit	31,s2_hi,s2_hi			/* Flip the sign bit	*/

___adddf3:
	addo	s1_hi,s1_hi,s1_exp		/* exp + mant (no sign bit)  */

	addc	s2_hi,s2_hi,s2_exp		/* exp + mant, sign -> carry */
	movlda(add_type,op_type)

	alterbit 31,0,s2_sign			/* select S2 sign bit	*/
	movldar(s1_lo,s1_mant_lo)		/* copy mantissa bits        */

	shri	32-11,s1_exp,tmp		/* check S1 for special case */
	lda	0xFFF00000,tmp2

	notand	s1_hi,tmp2,s1_mant_hi

	subo	tmp2,s1_mant_hi,s1_mant_hi	/* set "j" bit		     */
	addlda(1,tmp,tmp)

#if	!defined(USE_CMP_BCC)
	cmpo	1,tmp				/* S1 NaN/INF, 0/Denormal?   */
#endif

#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	shro	32-11,s1_exp,s1_exp		/* right justify exponent    */
#if	defined(USE_CMP_BCC)
	cmpobge	1,tmp,opd_as1_special		/* J/ S1 NaN/INF, 0/Denormal */
#else
	bge.f	opd_as1_special			/* J/ special S1 handling    */
#endif

opd_as1_rejoin:

	shri	32-11,s2_exp,tmp		/* Test S2 for special ...   */
	movldar(s2_lo,s2_mant_lo)		/* ... but assume it's not.  */

	andnot	tmp2,s2_hi,s2_mant_hi
	addlda(1,tmp,tmp)

#if	!defined(USE_CMP_BCC)
	cmpo	2,tmp
#endif
	shro	32-11,s2_exp,s2_exp		/* right justify exponent */

	subo	tmp2,s2_mant_hi,s2_mant_hi	/* set "j" bit		  */
#if	defined(USE_CMP_BCC)
	cmpobg	2,tmp,opd_as2_special		/* J/ NaN/INF, 0/denormal */
#else
	bg.f	opd_as2_special			/* J/ NaN/INF, 0/denormal */
#endif

opd_as2_rejoin:

#if	!defined(USE_CMP_BCC)
	cmpi	s1_exp,s2_exp
#endif
	movlda(0,s1_mant_x)			/* zero mantissa extentions */

	subo	s1_exp,s2_exp,tmp		/* shift count to align */
	movlda(0,s2_mant_x)

	addo	31,1,tmp3			/* Const of 32 for shifting */
#if	defined(USE_CMP_BCC)
	cmpibg	s1_exp,s2_exp,opd_a_01		/* J/ s1_exp > s2_exp     */
#else
	bg	opd_a_01			/* J/ s1_exp > s2_exp     */
#endif

	be	opd_a_02			/* J/ no adjustment req'd */
/*
 * s1_mant = dshr(s1_mant, tmp)
 */
#if	!defined(USE_CMP_BCC)
	cmpo	tmp,tmp3
#endif

	subo	tmp,tmp3,tmp2			/* tmp2 = 32 - tmp */
#if	defined(USE_CMP_BCC)
	cmpobge	tmp,tmp3,opd_as1_dshr		/* J/ >= one word shift */
#else
	bge.f	opd_as1_dshr			/* J/ >= one word shift */
#endif

#if	defined(USE_ESHRO)
	shlo	tmp2,s1_mant_lo,s1_mant_x	/* _lo -> _x bits    */
	eshro	tmp,s1_mant,s1_mant_lo		/* position _lo bits */
	shro	tmp,s1_mant_hi,s1_mant_hi	/* position _hi bits */
#else
	shlo	tmp2,s1_mant_lo,s1_mant_x	/* _lo -> _x bits    */
	shro	tmp,s1_mant_lo,s1_mant_lo	/* position _lo bits */
	shlo	tmp2,s1_mant_hi,tmp3		/* _hi -> _lo bits   */
	shro	tmp,s1_mant_hi,s1_mant_hi	/* position _hi bits */
	or	tmp3,s1_mant_lo,s1_mant_lo	/* put together _lo  */
#endif
	b	opd_a_02			/* re-joint flow */

/*
 * s2_mant = dshr(s2_mant, -tmp)
 */
opd_a_01:
	subo	tmp,0,tmp			/* Neg TMP value */
	mov	s1_exp,s2_exp			/* Max exp value */

	subo	tmp,tmp3,tmp2			/* tmp2 = 32 - tmp */
#if	defined(USE_CMP_BCC)
	cmpobge	tmp,tmp3,opd_as2_dshr		/* J/ >= one word shift */
#else
	cmpo	tmp,tmp3
	bge.f	opd_as2_dshr			/* J/ >= one word shift */
#endif

#if	defined(USE_ESHRO)
	shlo	tmp2,s2_mant_lo,s2_mant_x	/* _lo -> _x bits    */
	eshro	tmp,s2_mant,s2_mant_lo		/* position _lo bits */
	shro	tmp,s2_mant_hi,s2_mant_hi	/* position _hi bits */
#else
	shlo	tmp2,s2_mant_lo,s2_mant_x	/* _lo -> _x bits    */
	shro	tmp,s2_mant_lo,s2_mant_lo	/* position _lo bits */
	shlo	tmp2,s2_mant_hi,tmp3		/* _hi -> _lo bits   */
	shro	tmp,s2_mant_hi,s2_mant_hi	/* position _hi bits */
	or	tmp3,s2_mant_lo,s2_mant_lo	/* put together _lo  */
#endif


/*
 * Operands are aligned; now determine whether to add or subtract
 */

opd_a_02:
	xor	s1_hi,s2_hi,tmp
	addc	tmp,tmp,tmp
	be.f	opd_a_03			/* J/ different signs */

/*
 * Mantissa addition
 */
     /*	cmpo	1,0 */				/* "be" jumps when carry set */
	addc	s1_mant_lo,s2_mant_lo,s2_mant_lo
	or	s1_mant_x,s2_mant_x,s2_mant_x
	addc	s1_mant_hi,s2_mant_hi,s2_mant_hi
	and	1,s2_mant_x,tmp			/* LS bit into tmp */
#if	defined(USE_CMP_BCC)
	bbc	21,s2_mant_hi,opd_AFP_RRC_D	/* J/ no overflow past j bit */
#else
	chkbit	21,s2_mant_hi
	bf.t	opd_AFP_RRC_D			/* J/ no overflow past j bit */
#endif

	shro	1,s2_mant_x,s2_mant_x		/* shift _x word   */
	or	tmp,s2_mant_x,s2_mant_x		/* retain LS bit   */
	shlo	31,s2_mant_lo,tmp		/* _lo -> _x bit   */
	or	s2_mant_x,tmp,s2_mant_x		/* finish _x word  */
#if	defined(USE_ESHRO)
	eshro	1,s2_mant,s2_mant_lo		/* shift _lo word  */
#else
	shro	1,s2_mant_lo,s2_mant_lo		/* shift _lo word  */
	shlo	31,s2_mant_hi,tmp		/* _hi _> _lo bit  */
	or	s2_mant_lo,tmp,s2_mant_lo	/* finish _lo word */
#endif
	shro	1,s2_mant_hi,s2_mant_hi		/* shift _hi word  */

	addlda(1,s2_exp,s2_exp)			/* bump result exp */


opd_AFP_RRC_D:
	cmpi	s2_exp,2			/* possible range fault test	*/
	lda	DP_INF-2,con_1

	concmpi	s2_exp,con_1			/* possible range fault test	*/
	lda	0x7FFFFFFF,con_2		/* Round/even-nearest constant	*/
	bne.f	AFP_RRC_D_2			/* J/ possible range fault	*/

	cmpo	s2_mant_x,1			/* Check for Inexact fault/flag	*/
	lda	(1 << AC_Inex_mask)+(1 << AC_Inex_flag),con_1

	andnot	ac,con_1,tmp			/* Check for Inexact fault/flag	*/
	concmpo	1,tmp				/* (NZ if tmp = 0)		*/

	shro	AC_Round_Mode,ac,tmp		/* rounding mode to tmp		*/
	be.f	AFP_RRC_D_2			/* J/ inexact + fault/flag set	*/

#if	defined(USE_CMP_BCC)
	cmpobne	Round_Mode_even,tmp,AFP_RRC_D_2	/* J/ not round-to-nearest/even	*/
#else
	cmpo	Round_Mode_even,tmp		/* Round-to-nearest/even?	*/
	bne.f	AFP_RRC_D_2			/* J/ not round-to-nearest/even	*/
#endif

	subo	1,s2_exp,s2_exp			/* Compensate for "j" bit	*/
	chkbit	0,s2_mant_lo			/* Q0 bit into carry		*/
	shlo	20,s2_exp,tmp			/* Position exponent		*/
	addc	con_2,s2_mant_x,tmp2		/* Compute rounding		*/
	or	s2_sign,tmp,tmp			/* Mix exponent with sign	*/
	addc	0,s2_mant_lo,out_lo		/* Round lo word		*/
	addc	tmp,s2_mant_hi,out_hi		/* Round, carry to exp, sign	*/
	ret


/*
 * Mantissa subtraction
 */
opd_a_03:
     /*	cmpo	0,0 */				/* CHKBIT set BORROW/ bit */
	subc	s1_mant_x,s2_mant_x,s2_mant_x
	subc	s1_mant_lo,s2_mant_lo,s2_mant_lo
	subc	s1_mant_hi,s2_mant_hi,s2_mant_hi
	be.t	opd_a_04			/* J/ no sign rev */

	notbit	31,s2_sign,s2_sign		/* Negate mantissa */
	subc	s2_mant_x,1,s2_mant_x		/* "1" for left over borrow */
	subc	s2_mant_lo,0,s2_mant_lo
	subc	s2_mant_hi,0,s2_mant_hi

opd_a_04:
#if	defined(USE_CMP_BCC)
	bbs	20,s2_mant_hi,opd_AFP_RRC_D	/* J/ no shifting to do */
#else
	chkbit	20,s2_mant_hi
	bt.t	opd_AFP_RRC_D			/* J/ no shifting to do */
#endif
	scanbit	s2_mant_hi,tmp
	addo	tmp,12,tmp2
	subo	tmp,20,tmp
	bno.f	opd_a_06			/* J/ norm shift (>= 32)  */

opd_a_05:					/* final normalization shift */
#if	defined(USE_ESHRO)
	eshro	tmp2,s2_mant,s2_mant_hi
#else
	shlo	tmp,s2_mant_hi,s2_mant_hi
	shro	tmp2,s2_mant_lo,tmp3
	or	s2_mant_hi,tmp3,s2_mant_hi
#endif
	shlo	tmp,s2_mant_lo,s2_mant_lo
	shro	tmp2,s2_mant_x,tmp3
	or	s2_mant_lo,tmp3,s2_mant_lo
	shlo	tmp,s2_mant_x,s2_mant_x

	subo	tmp,s2_exp,s2_exp
	b	opd_AFP_RRC_D



___divdf3:

#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	xor	s1_hi,s2_hi,rsl_sign		/* Compute and isolate ... */
	movlda(div_type,op_type)

	shro	31,rsl_sign,rsl_sign		/* ... result sign	   */
	shlo1(s2_hi,dvsr_exp)			/* Delete sign bit */

	shri	32-11,dvsr_exp,tmp
	movldar(s2_lo,dvsr_mant_lo)

	shlo	1,s1_hi,rsl_exp
	addlda(1,tmp,tmp)

#if	!defined(USE_CMP_BCC)
	cmpo	2,tmp
#endif
	lda	0xfff00000,tmp2			/* Extract significand, ...  */

	notand	s2_hi,tmp2,dvsr_mant_hi

	subo	tmp2,dvsr_mant_hi,dvsr_mant_hi	/* ... set "j" bit           */
	shlo	31,rsl_sign,rsl_sign

	shro	32-11,dvsr_exp,dvsr_exp		/* right just, zero fill exp */
#if	defined(USE_CMP_BCC)
	cmpobg	2,tmp,opd_ds2_special		/* J/ S2 is NaN/INF/0/denorm */
#else
	bg.f	opd_ds2_special			/* J/ S2 is NaN/INF/0/denorm */
#endif

opd_ds2_rejoin:

	shri	32-11,rsl_exp,tmp

	shro	32-11,rsl_exp,rsl_exp		/* right just, zero fill exp */
	addlda(1,tmp,tmp)

#if	!defined(USE_CMP_BCC)
	cmpo	2,tmp
#endif
	movldar(s1_lo,rsl_mant_lo)

	notand	s1_hi,tmp2,rsl_mant_hi

	subo	tmp2,rsl_mant_hi,rsl_mant_hi	/* ... set "j" bit           */
#if	defined(USE_CMP_BCC)
	cmpobg	2,tmp,opd_ds1_special		/* J/ S1 is NaN/INF/0/denorm */
#else
	bg.f	opd_ds1_special			/* J/ S1 is NaN/INF/0/denorm */
#endif

opd_ds1_rejoin:

	subo	dvsr_exp,rsl_exp,rsl_exp	/* compute result exp    ... */

/*
 * Normalized source operands - perform divide
 */
#if	defined(USE_ESHRO)
	eshro	32-2,rsl_mant,rp_A_hi
#else
	shlo	2,rsl_mant_hi,rp_A_hi		/* ...  shl(rsl_mant,2) 	 */
	shro	32-2,rsl_mant_lo,tmp
	or	rp_A_hi,tmp,rp_A_hi
#endif
	shlo	2,rsl_mant_lo,rp_A_lo

#if	defined(USE_LDA_REG_OFF)
	lda	DP_Bias-1(rsl_exp),rsl_exp
#else
	lda	DP_Bias-1,tmp
	addo	rsl_exp,tmp,rsl_exp		/* ... with bias             */
#endif

#if	defined(USE_ESHRO)
	eshro	32-10,dvsr_mant,B
#else
	shlo	10,dvsr_mant_hi,B		/* B = H(shl(dvsr_mant,10))*/
	shro	32-10,dvsr_mant_lo,tmp		/* bits xfer'd word-word */
	or	B,tmp,B
#endif

	ediv	B,rp_A,rp_A			/* 64 bit of A/B -> quo	 */

	shlo	10,dvsr_mant_lo,C		/* C = L(shl(dvsr_mant,10))*/

	mov	rp_A_hi,quo_hi
	mov	rp_A_lo,rp_A_hi			/* zero ext remdr for	 */
	mov	0,rp_A_lo			/* ... next 32 bits	 */
	ediv	B,rp_A,rp_A
	mov	rp_A_hi,quo_lo

	shro	2,C,rp_A_hi			/* (prevent div overflow) */
	mov	0,rp_A_lo
	ediv	B,rp_A,rp_A

	shlo	7,quo_hi,tmp			/* extract A/B bits to tmp */
	shro	32-7,quo_lo,rp_A_lo
	or	tmp,rp_A_lo,tmp
	emul	tmp,rp_A_hi,rp_A		/* C/B*A/B -> rp_A	 */

	shro	7-2,rp_A_hi,rp_A_hi		/* align with A/B */
	addo	1,rp_A_hi,rp_A_hi		/* ceiling function */

	cmpo	0,0
	subc	rp_A_hi,quo_lo,quo_lo
	subc	0,quo_hi,quo_hi

	shro	1,quo_lo,quo_lo			/* Delete approx guard bit */
	shlo	32-1,quo_hi,tmp
	or	quo_lo,tmp,quo_lo
	shro	1,quo_hi,quo_hi

	emul	C,quo_lo,rmndr			/* Compute remainder	 */
	emul	C,quo_hi,rp_A
	addo	rp_A_lo,rmndr_hi,rmndr_hi
	emul	B,quo_lo,rp_A
	addo	rp_A_lo,rmndr_hi,rmndr_hi

    /*	cmpo	0,0  */				/* BORROW/ set by subc	 */
	subc	rmndr_lo,0,rmndr_lo		/* Negate for true rmndr */
	subc	rmndr_hi,0,rmndr_hi

#if	defined(USE_CMP_BCC)
	cmpobg	rmndr_hi,B,opd_r_adjust		/* J/ rmndr > divisor	 */
#else
	cmpo	rmndr_hi,B
	bg	opd_r_adjust			/* J/ rmndr > divisor	 */
#endif

	bl	opd_no_adjust			/* J/ rmndr < divisor	 */

#if	defined(USE_CMP_BCC)
	cmpobl	rmndr_lo,C,opd_no_adjust	/* J/ rmndr < divisor	 */
#else
	cmpo	rmndr_lo,C
	bl	opd_no_adjust			/* J/ rmndr < divisor	 */
#endif

opd_r_adjust:
	cmpo	0,0				/* Set BORROW/ bit	 */
	subc	C,rmndr_lo,rmndr_lo		/* Reduce remainder	 */
	subc	B,rmndr_hi,rmndr_hi
	addc	0,quo_lo,quo_lo			/* Incr quotient by 1	 */
	addc	0,quo_hi,quo_hi

opd_no_adjust:
	or	rmndr_hi,rmndr_lo,rmndr_lo
	cmpo	0,rmndr_lo
	shro	23,quo_hi,tmp			/* compute mant shift	 */
	subc	1,0,tmp2
	notor	quo_lo,tmp2,quo_lo		/* inexact sticky bit	 */

	addo	rsl_exp,tmp,rsl_exp		/* adj exp as required	 */

	subo	tmp,30,tmp2			/* shift counts		 */
	addlda(2,tmp,tmp)
	shlo	tmp2,quo_lo,rsl_mant_x		/* RS bits		 */
	shlo	tmp2,quo_hi,tmp3		/* Position result	 */
	shro	tmp,quo_lo,rsl_mant_lo
	or	tmp3,rsl_mant_lo,rsl_mant_lo
	shro	tmp,quo_hi,rsl_mant_hi
	b	opd_AFP_RRC_D			/* Round and range check */



___muldf3:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	xor	s1_hi,s2_hi,s2_sign		/* Compute and isolate ... */
	movlda(mul_type,op_type)

	shro	31,s2_sign,s2_sign		/* ... result sign	   */
	shlo1(s1_hi,s1_exp)			/* Delete sign bit	*/

	shri	32-11,s1_exp,tmp		/* Position for testing	*/
	movldar(s1_lo,s1_mant_lo)

	shlo	1,s2_hi,s2_exp			/* Delete sign bit	*/
	addlda(1,tmp,tmp)

#if	!defined(USE_CMP_BCC)
	cmpo	2,tmp
#endif
	lda	0xfff00000,tmp2

	notand	s1_hi,tmp2,s1_mant_hi		/* Extract significand,	*/

	subo	tmp2,s1_mant_hi,s1_mant_hi	/* ... set "j" bit	*/
	shro	32-11,s1_exp,s1_exp		/* right just, zero fill exp */

	shlo	31,s2_sign,s2_sign
#if	defined(USE_CMP_BCC)
	cmpobg	2,tmp,opd_ms1_special		/* J/ S1 is NaN/INF/0/denorm */
#else
	bg.f	opd_ms1_special			/* J/ S1 is NaN/INF/0/denorm */
#endif

opd_ms1_rejoin:
	shri	32-11,s2_exp,tmp3

	setbit	20,s2_hi,s2_mant_hi		/* Set "j" bit (ignore exp) */
	addlda(1,tmp3,tmp)

#if	!defined(USE_CMP_BCC)
	cmpo	2,tmp
#endif
	movldar(s2_lo,s2_mant_lo)

	shro	32-11,s2_exp,s2_exp		/* right just, zero fill exp */
#if	defined(USE_CMP_BCC)
	cmpobg	2,tmp,opd_ms2_special		/* J/ S2 is NaN/INF/0/denorm */
#else
	bg.f	opd_ms2_special			/* J/ S2 is NaN/INF/0/denorm */
#endif

opd_ms2_rejoin:

/*
 * Overlap partial product multiplies while computing result exponent and
 * left justifying s2 operand (to simplify normalization).  On the 960CA,
 * this code runs faster when the integer overflow fault is masked.
 */

	shlo	11,s2_mant_lo,tmp		/* Left just lo word */
	emul	tmp,s1_mant_lo,rp_A		/* (overlap multiply) */

	shro	32-11,s2_mant_lo,s2_mant_lo	/* _lo -> _hi xfer	*/
	shlo	32-21,s2_mant_hi,s2_mant_hi	/* Left just _hi word */
	or	s2_mant_lo,s2_mant_hi,s2_mant_hi

	emul	tmp,s1_mant_hi,rp_B

	cmpo	0,rp_A_lo			/* Set Borrow if <> 0	*/
	addo	s1_exp,s2_exp,s2_exp		/* compute result exp	*/
	subc	1,0,rp_A_lo			/* clear Carry, keep NZ	*/
	lda	DP_Bias-1,con_1

	emul	s1_mant_lo,s2_mant_hi,rp_C

	addc	rp_A_hi,rp_B_lo,rp_A_hi		/* combine partial products */
	subo	con_1,s2_exp,s2_exp		/* compute result exponent  */
	addc	0,rp_B_hi,rp_B_hi

	emul	s1_mant_hi,s2_mant_hi,s2_mant

	addc	rp_A_hi,rp_C_lo,s2_mant_x
	ornot	rp_A_lo,s2_mant_x,s2_mant_x
	addc	rp_B_hi,rp_C_hi,rp_B_hi
	addc	0,0,tmp

	addc	rp_B_hi,s2_mant_lo,s2_mant_lo	/* combine partial products */
	addc	tmp,s2_mant_hi,s2_mant_hi

#if	defined(USE_CMP_BCC)
	bbs	20,s2_mant_hi,opd_AFP_RRC_D	/* J/ normalized result */
#else
	chkbit	20,s2_mant_hi
	bt.t	opd_AFP_RRC_D			/* J/ normalized result */
#endif

	addc	s2_mant_x,s2_mant_x,s2_mant_x	/* Normalizing shift needed */
	addc	s2_mant_lo,s2_mant_lo,s2_mant_lo
	subo	1,s2_exp,s2_exp			/* Adjust result exponent */
	addc	s2_mant_hi,s2_mant_hi,s2_mant_hi
	b	opd_AFP_RRC_D



/******************************************************************************/
/*                                                                            */
/*  Addition/subtraction long alignment/normalization shift routines          */
/*                                                                            */
/******************************************************************************/

opd_a_06:					/* Sub rslt's MS bit not... */
	scanbit	s2_mant_lo,tmp			/* ...it top word           */
	subo	tmp,26,tmp
	bf.f	opd_a_07			/* J/ check for zero/R	 */
	subo	tmp,6,tmp2
#if	!defined(USE_CMP_BCC)
	cmpi	tmp,6				/* Shift ? 36-26	 */
#endif
	addo	tmp,26,tmp			/* Shift = 52 - msbit	 */
#if	defined(USE_CMP_BCC)
	cmpibge	31,tmp,opd_a_05			/* J/ norm shift (31 >= shft) */
#else
	bl.t	opd_a_05			/* J/ norm shift (< 32)	      */
#endif

	addo	31,1,tmp3
	movldar(s2_mant_lo,s2_mant_hi)		/* Full word shift	 */
	subo	tmp3,s2_exp,s2_exp		/* reduce exponent by 32    */
	movldar(s2_mant_x,s2_mant_lo)
	mov	0,s2_mant_x
	be	opd_AFP_RRC_D			/* J/ exactly 32 bit shift  */

	subo	tmp3,tmp,tmp			/* reduce shift count by 32 */
	subo	tmp,tmp3,tmp2			/* word -> word right shift */
	b	opd_a_05

opd_a_07:
	cmpobe	0,s2_mant_x,opd_as1_09		/* J/ zero result */

	subo	26,s2_exp,s2_exp		/* Result = R bit */
	shlo	20,1,s2_mant_hi
	movlda(0,s2_mant_lo)

	subo	53-26,s2_exp,s2_exp
	movlda(0,s2_mant_x)
	b	opd_AFP_RRC_D



opd_as1_dshr:					/* Align shift >= 32 bits */
	subo	tmp3,tmp,tmp
#if	!defined(USE_CMP_BCC)
	cmpo	54-32,tmp
#endif
	subo	tmp,tmp3,tmp2
#if	defined(USE_CMP_BCC)
	cmpobl	54-32,tmp,opd_as1_tshr			/* J/ shift beyond R bit */
#else
	cmpo	54-32,tmp
	bl.f	opd_as1_tshr			/* J/ shift beyond R bit */
#endif

#if	defined(USE_ESHRO)
	shlo	tmp2,s1_mant_lo,tmp3		/* _lo to oblivion bits */
	cmpo	0,tmp3
	eshro	tmp,s1_mant,s1_mant_x		/* _lo -> _x bits */
	subc	1,0,tmp3			/* if any lost _lo bits */
	shro	tmp,s1_mant_hi,s1_mant_lo	/* _hi -> _lo bits */
	ornot	tmp3,s1_mant_x,s1_mant_x	/* lost bits to LS bit of _x */
	movlda(0,s1_mant_hi)
#else
	shlo	tmp2,s1_mant_lo,tmp3		/* _lo to oblivion bits */
	shro	tmp,s1_mant_lo,s1_mant_x	/* _lo -> _x bits */
	cmpo	0,tmp3
	shlo	tmp2,s1_mant_hi,s1_mant_lo	/* _hi -> _x bits */
	subc	1,0,tmp3			/* if any lost _lo bits */
	or	s1_mant_x,s1_mant_lo,s1_mant_x	/* put together _x */
	shro	tmp,s1_mant_hi,s1_mant_lo	/* _hi -> _lo bits */
	ornot	tmp3,s1_mant_x,s1_mant_x	/* lost bits to LS bit of _x */
	movlda(0,s1_mant_hi)
#endif
	b	opd_a_02

opd_as1_tshr:
	movl	0,s1_mant			/* set for inexact */
	movlda(1,s1_mant_x)
	b	opd_a_02


opd_as2_dshr:					/* Align shift >= 32 bits  */
	subo	tmp3,tmp,tmp
#if	!defined(USE_CMP_BCC)
	cmpo	54-32,tmp
#endif
	subo	tmp,tmp3,tmp2
#if	defined(USE_CMP_BCC)
	cmpobl	54-32,tmp,opd_as2_tshr			/* J/ shift beyond R bit */
#else
	bl.f	opd_as2_tshr			/* J/ shift beyond R bit */
#endif

#if	defined(USE_ESHRO)
	shlo	tmp2,s2_mant_lo,tmp3		/* _lo to oblivion bits */
	cmpo	0,tmp3
	eshro	tmp,s2_mant,s2_mant_x		/* _lo -> _x bits */
	subc	1,0,tmp3			/* if any lost _lo bits */
	shro	tmp,s2_mant_hi,s2_mant_lo	/* _hi -> _lo bits */
	ornot	tmp3,s2_mant_x,s2_mant_x	/* lost bits to LS bit of _x */
	lda	0,s2_mant_hi
#else
	shlo	tmp2,s2_mant_lo,tmp3		/* _lo to oblivion bits */
	cmpo	0,tmp3
	shro	tmp,s2_mant_lo,s2_mant_x	/* _lo -> _x bits */
	shlo	tmp2,s2_mant_hi,s2_mant_lo	/* _hi -> _x bits */
	or	s2_mant_x,s2_mant_lo,s2_mant_x	/* put together _x */
	subc	1,0,tmp3			/* if any lost _lo bits */
	shro	tmp,s2_mant_hi,s2_mant_lo	/* _hi -> _lo bits */
	ornot	tmp3,s2_mant_x,s2_mant_x	/* lost bits to LS bit of _x */
	mov	0,s2_mant_hi
#endif
	b	opd_a_02

opd_as2_tshr:
	movl	0,s2_mant			/* set for inexact */
	movlda(1,s2_mant_x)
	b	opd_a_02



/******************************************************************************/
/*                                                                            */
/*  Addition/subtraction special case handlers                                */
/*                                                                            */
/******************************************************************************/


/*  S1 is a special case value: +/-0, denormal, +/-INF, NaN; S2 is unknown  */

opd_as1_special:
	cmpo	s1_lo,0				/* Condense lo word to carry bit */
	lda	DP_INF << 21,con_1
	addc	s1_hi,s1_hi,tmp			/* Drop sign bit, mix in carry bit */
	xor	1,tmp,tmp			/* Correct polarity of LB bit */
	cmpobl	con_1,tmp,opd_as1_06		/* J/ S1 is NaN */
	be	opd_as1_20			/* J/ S1 is INF */
	cmpobne	0,tmp,opd_as1_10		/* J/ S1 is denormal */

	cmpo	s2_lo,0				/* Condense lo word into LS bit */
	addc	s2_hi,s2_hi,tmp2		/* S1 is zero - examine s2 */
	xor	1,tmp2,tmp2
	cmpobe	0,tmp2,opd_as1_08		/* J/ 0 + 0 */
	cmpobg	tmp2,con_1,opd_as1_06		/* J/ S2 is NaN */

	ldconst	0x001fffff,con_1
	cmpobg	tmp2,con_1,opd_as1_04		/* J/ not a denormal */

	bbc	AC_Norm_Mode,ac,opd_as1_05a	/* J/ not normalizing mode	*/
	bbc	AC_Unfl_mask,ac,opd_as1_05b	/* J/ underflow masked		*/

opd_as1_04:
	movl	s2,out				/* Return S2 */
	ret

opd_as1_05a:
	b	_AFP_Fault_Reserved_Encoding_D

opd_as1_05b:
	b	_AFP_Fault_Underflow_D

opd_as1_06:
	b	AFP_NaN_D			/* Handle NaN operand(s) */

opd_as1_08:
	cmpobe	s1_hi,s2_hi,opd_as1_04		/* J/ +0 + +0  or  -0 + -0 */

opd_as1_09:
	shro	AC_Round_Mode,ac,tmp		/* Check round mode for diff sgn 0's */
	cmpo	Round_Mode_Down,tmp
#if	defined(USE_OPN_CC)
	sele	0,1,out_hi
#else
	teste	out_hi
#endif
	shlo	31,out_hi,out_hi		/* -0 iff round down */
	ldconst	0,out_lo
	ret


opd_as1_10:
	bbc.f	AC_Norm_Mode,ac,opd_as1_05a	/* J/ denorm, not norm mode -> fault */

	shro	1,tmp,tmp			/* tmp = s1_hi w/o sign bit */
	scanbit	tmp,tmp
	bno	opd_as1_14			/* J/ MS word = 0 */

opd_as1_12:
	subo	tmp,20,tmp			/* Top bit num to left shift count */
	and	0x1f,tmp,tmp			/* (when MS word = 0) */
	subo	tmp,1,s1_exp			/* set s1_exp value */

	shlo	tmp,s1_hi,s1_mant_hi		/* Normalize denorm significand */
	shlo	tmp,s1_lo,s1_mant_lo
	subo	tmp,31,tmp			/* word -> word bit xfer */
	addo	1,tmp,tmp
	shro	tmp,s1_lo,tmp
	or	tmp,s1_mant_hi,s1_mant_hi
	lda	0xFFF00000,tmp2			/* restore TMP2 mask */
	b	opd_as1_rejoin

opd_as1_14:
	scanbit	s1_lo,tmp
	cmpoble	21,tmp,opd_as1_12		/* J/ not a full word shift */

	subo	tmp,20,tmp
	subo	tmp,0,s1_exp			/* set s1_exp value */
	subo	31,s1_exp,s1_exp
	shlo	tmp,s1_lo,s1_mant_hi		/* 32+ bit shift */
	mov	0,s1_mant_lo
	lda	0xFFF00000,tmp2			/* restore TMP2 mask */
	b	opd_as1_rejoin


opd_as1_20:
	cmpo	s2_lo,0				/* Condense lo word into LS bit */
	shlo	1,s2_hi,tmp2			/* Drop S2 sign */

#if	defined(USE_OPN_CC)
	addone	1,tmp2,tmp2
#else
	testne	tmp3
	or	tmp2,tmp3,tmp2
#endif

	cmpobg	tmp2,con_1,opd_as1_06		/* S2 is NaN -> NaN handler */
	bne	opd_as1_24			/* J/ S2 is not INF (or NaN) */
	cmpobe	s1_hi,s2_hi,opd_as1_04		/* J/ same signed INF's */

	bbc	AC_InvO_mask,ac,opd_as1_22	/* J/ inv oper fault not masked */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac		/* Set inv oper flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp		/* Set inv oper flag */
	modac	tmp,tmp,tmp
#endif
	ldconst	Standard_QNaN_hi,out_hi		/* Return a quiet NaN */
	ldconst	Standard_QNaN_lo,out_lo
	ret

opd_as1_22:
	b	_AFP_Fault_Invalid_Operation_D

opd_as1_24:
	bbs.t	AC_Norm_Mode,ac,opd_as1_26	/* J/ denormals allowed */
	ldconst	0x00ffffff,con_1
	cmpobg	tmp2,con_1,opd_as1_26		/* J/ not denormal, 0 */
	cmpobne	0,tmp2,opd_as1_05a		/* J/ denormal -> fault */

opd_as1_26:
	movl	s1,out				/* return s1 */
	ret



/*  S2 is a special case value: +/-0, denormal, +/-INF, NaN; S2 is a non-  */
/*  zero, finite value.                                                    */

opd_as2_special:
	cmpo	s2_lo,0				/* Condense lo word into LS bit */
	ldconst	DP_INF << 21,con_1
	addc	s2_hi,s2_hi,tmp2		/* Drop sign bit */
	xor	1,tmp2,tmp2
	cmpobg	tmp2,con_1,opd_as1_06		/* J/ S2 is a NaN */
	be	opd_as1_04			/* J/ INF + finite -> INF */
	cmpobe	0,tmp2,opd_as2_20		/* J/ 0 (check for underflow)	*/
	bbc.f	AC_Norm_Mode,ac,opd_as1_05a	/* J/ denorm, not norm mode -> fault */

	shro	1,tmp2,tmp2			/* tmp = s2_hi w/o sign bit */
	scanbit	tmp2,tmp
	bno	opd_as2_14			/* J/ MS word = 0 */

opd_as2_12:
	subo	tmp,20,tmp			/* Top bit num to left shift count */
	and	0x1f,tmp,tmp			/* (when MS word = 0) */
	subo	tmp,1,s2_exp			/* set s2_exp value */

	shlo	tmp,s2_hi,s2_mant_hi		/* Normalize denorm significand */
	shlo	tmp,s2_lo,s2_mant_lo
	subo	tmp,31,tmp			/* word -> word bit xfer */
	addo	1,tmp,tmp
	shro	tmp,s2_lo,tmp
	or	tmp,s2_mant_hi,s2_mant_hi
	b	opd_as2_rejoin

opd_as2_14:
	scanbit	s2_lo,tmp
	cmpoble	21,tmp,opd_as2_12		/* J/ not a full word shift */

	subo	tmp,20,tmp
	subo	tmp,0,s2_exp			/* set s2_exp field */
	subo	31,s2_exp,s2_exp
	shlo	tmp,s2_lo,s2_mant_hi		/* 32+ bit shift */
	mov	0,s2_mant_lo
	b	opd_as2_rejoin


opd_as2_20:
	bbs	AC_Unfl_mask,ac,opd_as1_26	/* J/ underflow masked	*/
	cmpibl	0,s1_exp,opd_as1_26		/* J/ S1 not denorm	*/
	b	opd_as1_05b			/* J/ underflow fault	*/


/******************************************************************************/
/*                                                                            */
/*  Division special case handlers                                            */
/*                                                                            */
/******************************************************************************/


/*  S2 (divisor) - special value: +/-0, denormal, +/-INF, NaN; S1 - unknown  */

opd_ds2_special:
	shlo	1,s2_hi,tmp			/* Drop sign bit */
	cmpo	s2_lo,0				/* Condense lo word into LS bit */
#if	defined(USE_OPN_CC)
	addone	tmp,1,tmp
#else
	testne	tmp2
	or	tmp,tmp2,tmp
#endif
	ldconst	DP_INF << 21,con_1
	cmpobg	tmp,con_1,opd_ds2_06		/* J/ S2 is NaN */
	be	opd_ds2_20			/* J/ S2 is INF */
	cmpobne	0,tmp,opd_ds2_10		/* J/ S2 is denormal */

	shlo	1,s1_hi,tmp2
	cmpo	s1_lo,0				/* Condense lo word into LS bit */
#if	defined(USE_OPN_CC)
	addone	tmp2,1,tmp2
#else
	testne	tmp3
	or	tmp2,tmp3,tmp2
#endif
	cmpobe	0,tmp2,opd_ds2_24		/* J/ 0 / 0 -> invalid operation */
	cmpobg	tmp2,con_1,opd_ds2_06		/* J/ S1 is NaN */
	be	opd_ds2_05			/* J/ INF / 0 -> INF w/o ZrDiv fault */

	bbs	AC_Norm_Mode,ac,opd_ds2_04	/* J/ normalizing mode */
	ldconst	0x00ffffff,con_1
	cmpobg	tmp2,con_1,opd_ds2_04		/* J/ not a denormal */

opd_ds2_02:
	b	_AFP_Fault_Reserved_Encoding_D

opd_ds2_04:
	bbc	AC_ZerD_mask,ac,opd_ds2_08	/* J/ zero divide fault not masked */
#if	defined(USE_SIMULATED_AC)
	setbit	AC_ZerD_flag,ac,ac		/* Set zero divide flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_ZerD_flag,tmp		/* Set zero divide flag */
	modac	tmp,tmp,tmp
#endif

opd_ds2_05:
	ldconst	DP_INF << 20,out_hi		/* Return properly signed INF */
	ldconst	0,out_lo
	or	rsl_sign,out_hi,out_hi
	ret

opd_ds2_06:
	b	AFP_NaN_D			/* Handle NaN operand(s) */

opd_ds2_08:
	b	_AFP_Fault_Zero_Divide_D


opd_ds2_10:
	bbc	AC_Norm_Mode,ac,opd_ds2_02	/* J/ denormal, not norm mode -> fault */

	shro	1,tmp,tmp
	scanbit	tmp,tmp
	bno	opd_ds2_14			/* J/ MS word = 0 */

opd_ds2_12:
	subo	tmp,20,tmp			/* Top bit num to left shift count */
	and	0x1f,tmp,tmp			/* (when MS word = 0) */
	subo	tmp,1,dvsr_exp			/* set dvsr_exp value */

	shlo	tmp,s2_hi,dvsr_mant_hi		/* Normalize denorm significand */
	shlo	tmp,s2_lo,dvsr_mant_lo
	subo	tmp,31,tmp			/* word -> word bit xfer */
	addo	1,tmp,tmp
	shro	tmp,s2_lo,tmp
	or	tmp,dvsr_mant_hi,dvsr_mant_hi
	lda	0xfff00000,tmp2			/* Extract significand, ...  */
	b	opd_ds2_rejoin

opd_ds2_14:
	scanbit	s2_lo,tmp
	cmpoble	21,tmp,opd_ds2_12		/* J/ not a full word shift */

	subo	tmp,20,tmp
	subo	tmp,0,dvsr_exp			/* set dvsr_exp value */
	subo	31,dvsr_exp,dvsr_exp
	shlo	tmp,s2_lo,dvsr_mant_hi		/* 32+ bit shift */
	mov	0,dvsr_mant_lo
	lda	0xfff00000,tmp2			/* Extract significand, ...  */
	b	opd_ds2_rejoin


opd_ds2_20:
	shlo	1,s1_hi,tmp2
	cmpo	s1_lo,0				/* Condense lo word into LS bit */
#if	defined(USE_OPN_CC)
	addone	tmp2,1,tmp2
#else
	testne	tmp3
	or	tmp2,tmp3,tmp2
#endif
	cmpobg	tmp2,con_1,opd_ds2_06		/* S1 is NaN -> NaN handler */
	be	opd_ds2_24			/* J/ INF / INF -> invalid oper */
	cmpobe	0,tmp2,opd_ds2_22		/* J/ 0 / INF -> 0 */
	bbs	AC_Norm_Mode,ac,opd_ds2_22	/* J/ normalizing mode */
	ldconst	0x00ffffff,con_1
	cmpoble	tmp2,con_1,opd_ds2_02		/* J/ S1 is a denormal */
	
opd_ds2_22:
	mov	rsl_sign,out_hi			/* Return properly signed 0 */
	ldconst	0,out_lo
	ret

opd_ds2_24:
	bbc	AC_InvO_mask,ac,opd_ds2_25	/* J/ inv oper fault not masked */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac		/* Set inv oper flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp		/* Set inv oper flag */
	modac	tmp,tmp,tmp
#endif
	ldconst	Standard_QNaN_hi,out_hi		/* Return a quiet NaN */
	ldconst	Standard_QNaN_lo,out_lo
	ret

opd_ds2_25:
	b	_AFP_Fault_Invalid_Operation_D



/*  S1 is a special case value: +/-0, denormal, +/-INF, NaN; S2 is a non-  */
/*  zero, finite value.                                                    */

opd_ds1_special:
	shlo	1,s1_hi,tmp2
	cmpo	s1_lo,0				/* Condense lo word into LS bit */
#if	defined(USE_OPN_CC)
	addone	tmp2,1,tmp2
#else
	testne	tmp3
	or	tmp2,tmp3,tmp2
#endif
	ldconst	DP_INF << 21,con_1
	cmpobg	tmp2,con_1,opd_ds2_06		/* J/ S1 is a NaN */
	be	opd_ds2_05			/* J/ INF / finite -> INF */
	cmpobe	0,tmp2,opd_ds2_22		/* J/ 0 / finite -> 0 */
	bbc	AC_Norm_Mode,ac,opd_ds2_02	/* J/ denormal, not norm mode -> fault */

	shro	1,tmp2,tmp2
	scanbit	tmp2,tmp			/* Normalize denormal significand */
	bno	opd_ds1_14			/* J/ MS word = 0 */

opd_ds1_12:
	subo	tmp,20,tmp			/* Top bit num to left shift count */
	and	0x1f,tmp,tmp			/* (when MS word = 0) */
	subo	tmp,1,rsl_exp			/* set rsl_exp value */

	shlo	tmp,s1_hi,rsl_mant_hi		/* Normalize denorm significand */
	shlo	tmp,s1_lo,rsl_mant_lo
	subo	tmp,31,tmp			/* word -> word bit xfer */
	addo	1,tmp,tmp
	shro	tmp,s1_lo,tmp
	or	tmp,rsl_mant_hi,rsl_mant_hi
	b	opd_ds1_rejoin

opd_ds1_14:
	scanbit	s1_lo,tmp
	cmpoble	21,tmp,opd_ds1_12		/* J/ not a full word shift */

	subo	tmp,20,tmp
	subo	tmp,0,rsl_exp			/* set rsl_exp value */
	subo	31,rsl_exp,rsl_exp
	shlo	tmp,s1_lo,rsl_mant_hi		/* 32+ bit shift */
	mov	0,rsl_mant_lo
	b	opd_ds1_rejoin



/******************************************************************************/
/*                                                                            */
/*  Multiplication special case handlers                                      */
/*                                                                            */
/******************************************************************************/


/*  S1 is a special case value: +/-0, denormal, +/-INF, NaN; S2 is unknown  */

opd_ms1_special:
	shlo	1,s1_hi,tmp			/* Drop sign bit */
	cmpo	s1_lo,0				/* Condense lo word into LS bit */
#if	defined(USE_OPN_CC)
	addone	tmp,1,tmp
#else
	testne	tmp2
	or	tmp,tmp2,tmp
#endif
	ldconst	DP_INF << 21,con_1
	cmpobg	tmp,con_1,opd_ms1_06		/* J/ S1 is NaN */
	be	opd_ms1_20			/* J/ S1 is INF */
	cmpobne	0,tmp,opd_ms1_10		/* J/ S1 is denormal */

	shlo	1,s2_hi,tmp2
	cmpo	s2_lo,0				/* Condense lo word into LS bit */
#if	defined(USE_OPN_CC)
	addone	tmp2,1,tmp2
#else
	testne	tmp3
	or	tmp2,tmp3,tmp2
#endif
	cmpobe	0,tmp2,opd_ms1_04		/* J/ 0 * 0 */
	cmpobg	tmp2,con_1,opd_ms1_06		/* J/ S2 is NaN */
	be	opd_ms1_24			/* J/ 0 * INF -> invalid operation */

	bbs	AC_Norm_Mode,ac,opd_ms1_04	/* J/ normalizing mode */
	ldconst	0x00ffffff,con_1
	cmpobg	tmp2,con_1,opd_ms1_04		/* J/ not a denormal */

opd_ms1_02:
	b	_AFP_Fault_Reserved_Encoding_D

opd_ms1_04:
	mov	s2_sign,out_hi			/* Return properly signed zero */
	ldconst	0,out_lo
	ret

opd_ms1_06:
	b	AFP_NaN_D			/* Handle NaN operand(s) */


opd_ms1_10:
	bbc	AC_Norm_Mode,ac,opd_ms1_02	/* J/ denormal, not norm mode -> fault */

	shro	1,tmp,tmp
	scanbit	tmp,tmp
	bno	opd_ms1_14			/* J/ MS word = 0 */

opd_ms1_12:
	subo	tmp,20,tmp			/* Top bit num to left shift count */
	and	0x1f,tmp,tmp			/* (when MS word = 0) */
	subo	tmp,1,s1_exp			/* set s1_exp value */

	shlo	tmp,s1_hi,s1_mant_hi		/* Normalize denorm significand */
	shlo	tmp,s1_lo,s1_mant_lo
	subo	tmp,31,tmp			/* word -> word bit xfer */
	addo	1,tmp,tmp
	shro	tmp,s1_lo,tmp
	or	tmp,s1_mant_hi,s1_mant_hi
	b	opd_ms1_rejoin

opd_ms1_14:
	scanbit	s1_lo,tmp
	cmpoble	21,tmp,opd_ms1_12		/* J/ not a full word shift */

	subo	tmp,20,tmp
	subo	tmp,0,s1_exp			/* set s1_exp value */
	subo	31,s1_exp,s1_exp
	shlo	tmp,s1_lo,s1_mant_hi		/* 32+ bit shift */
	mov	0,s1_mant_lo
	b	opd_ms1_rejoin


opd_ms1_20:
	shlo	1,s2_hi,tmp2
	cmpo	s2_lo,0				/* Condense lo word into LS bit */
#if	defined(USE_OPN_CC)
	addone	tmp2,1,tmp2
#else
	testne	tmp3
	or	tmp2,tmp3,tmp2
#endif
	cmpobg	tmp2,con_1,opd_ms1_06		/* S2 is NaN -> NaN handler */
	cmpobe	0,tmp2,opd_ms1_24		/* J/ INF * 0 */
	bbs	AC_Norm_Mode,ac,opd_ms1_22	/* J/ normalizing mode */
	ldconst	0x00ffffff,con_1
	cmpoble	tmp2,con_1,opd_ms1_02		/* J/ S2 is a denormal */
	
opd_ms1_22:
	ldconst	DP_INF << 20,out_hi		/* Return properly signed INF */
	ldconst	0,out_lo
	or	s2_sign,out_hi,out_hi
	ret

opd_ms1_24:
	bbc	AC_InvO_mask,ac,opd_ms1_25	/* J/ inv oper fault not masked */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac		/* Set inv oper flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp		/* Set inv oper flag */
	modac	tmp,tmp,tmp
#endif
	ldconst	Standard_QNaN_hi,out_hi		/* Return a quiet NaN */
	ldconst	Standard_QNaN_lo,out_lo
	ret

opd_ms1_25:
	b	_AFP_Fault_Invalid_Operation_D




/*  S2 is a special case value: +/-0, denormal, +/-INF, NaN; S2 is a non-  */
/*  zero, finite value.                                                    */

opd_ms2_special:
	shlo	1,s2_hi,tmp2
	cmpo	s2_lo,0				/* Condense lo word into LS bit */
#if	defined(USE_OPN_CC)
	addone	tmp2,1,tmp2
#else
	testne	tmp3
	or	tmp2,tmp3,tmp2
#endif
	ldconst	DP_INF << 21,con_1
	cmpobg	tmp2,con_1,opd_ms1_06		/* J/ S2 is a NaN */
	be	opd_ms1_22			/* J/ INF * finite -> INF */
	cmpobe	0,tmp2,opd_ms1_04		/* J/ 0 * finite -> 0 */
	bbc	AC_Norm_Mode,ac,opd_ms1_02	/* J/ denormal, not norm mode -> fault */

	shro	1,tmp2,tmp2
	scanbit	tmp2,tmp			/* Normalize denormal significand */
	bno	opd_ms2_14			/* J/ MS word = 0 */

opd_ms2_12:
	subo	tmp,20,tmp			/* Top bit num to left shift count */
	and	0x1f,tmp,tmp			/* (when MS word = 0) */
	subo	tmp,1,s2_exp			/* set s2_exp value */

	shlo	tmp,s2_hi,s2_mant_hi		/* Normalize denorm significand */
	shlo	tmp,s2_lo,s2_mant_lo
	subo	tmp,31,tmp			/* word -> word bit xfer */
	addo	1,tmp,tmp
	shro	tmp,s2_lo,tmp
	or	tmp,s2_mant_hi,s2_mant_hi
	b	opd_ms2_rejoin

opd_ms2_14:
	scanbit	s2_lo,tmp
	cmpoble	21,tmp,opd_ms2_12		/* J/ not a full word shift */

	subo	tmp,20,tmp
	subo	tmp,0,s2_exp			/* set s2_exp value */
	subo	31,s2_exp,s2_exp
	shlo	tmp,s2_lo,s2_mant_hi		/* 32+ bit shift */
	mov	0,s2_mant_lo
	b	opd_ms2_rejoin
