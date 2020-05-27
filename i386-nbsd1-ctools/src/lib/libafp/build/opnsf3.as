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
/*      opnsf3.c - Single Precision Operation Routine (AFP-960)		      */
/*									      */
/******************************************************************************/

#include "asmopt.h"

#if	!defined(KEEP_INTERNAL_LABELS)
#define	ops_as1_special	L_ops_as1_special
#define	ops_as1_04	L_ops_as1_04
#define	ops_as1_05a	L_ops_as1_05a
#define	ops_as1_05b	L_ops_as1_05b
#define	ops_as1_06	L_ops_as1_06
#define	ops_as1_08	L_ops_as1_08
#define	ops_as1_09	L_ops_as1_09
#define	ops_as1_10	L_ops_as1_10
#define	ops_as1_20	L_ops_as1_20
#define	ops_as1_22	L_ops_as1_22
#define	ops_as1_24	L_ops_as1_24
#define	ops_as1_26	L_ops_as1_26
#define	ops_as2_special	L_ops_as2_special
#define	ops_ds2_special	L_ops_ds2_special
#define	ops_ds2_02	L_ops_ds2_02
#define	ops_ds2_04	L_ops_ds2_04
#define	ops_ds2_05	L_ops_ds2_05
#define	ops_ds2_05b	L_ops_ds2_05b
#define	ops_ds2_06	L_ops_ds2_06
#define	ops_ds2_08	L_ops_ds2_08
#define	ops_ds2_10	L_ops_ds2_10
#define	ops_ds2_20	L_ops_ds2_20
#define	ops_ds2_22	L_ops_ds2_22
#define	ops_ds2_24	L_ops_ds2_24
#define	ops_ds2_25	L_ops_ds2_25
#define	ops_ds1_special	L_ops_ds1_special
#define	ops_ms1_special	L_ops_ms1_special
#define	ops_ms1_02	L_ops_ms1_02
#define	ops_mss1_04	L_ops_mss1_04
#define	ops_ms1_05	L_ops_ms1_05
#define	ops_ms1_06	L_ops_ms1_06
#define	ops_mss1_10	L_ops_mss1_10
#define	ops_ms1_20	L_ops_ms1_20
#define	ops_ms1_22	L_ops_ms1_22
#define	ops_ms1_24	L_ops_ms1_24
#define	ops_ms1_25	L_ops_ms1_25
#define	ops_ms2_special	L_ops_ms2_special
#define	ops_ms1_rejoin	L_ops_ms1_rejoin
#define	ops_ms2_rejoin	L_ops_ms2_rejoin
#define	ops_as1_rejoin	L_ops_as1_rejoin
#define	ops_as2_rejoin	L_ops_as2_rejoin
#define	ops_addr_14	L_ops_addr_14
#define	ops_addr_18	L_ops_addr_18
#define	ops_AFP_RRC_S	L_ops_AFP_RRC_S
#define	ops_addr_22	L_ops_addr_22
#define	ops_addr_24	L_ops_addr_24
#define	ops_ds2_rejoin	L_ops_ds2_rejoin
#define	ops_ds1_rejoin	L_ops_ds1_rejoin
#define	ops_divr_10	L_ops_divr_10
#endif


	.file	"opnsf3.as"
	.globl	___addsf3
	.globl	___divsf3
	.globl	___mulsf3
	.globl	___subsf3


#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	AFP_RRC_S_2
	.globl	AFP_NaN_S

	.globl	_AFP_Fault_Invalid_Operation_S
	.globl	_AFP_Fault_Reserved_Encoding_S
	.globl	_AFP_Fault_Zero_Divide_S
	.globl	_AFP_Fault_Underflow_S


#define	FP_Bias         0x7f
#define	FP_INF          0xff

#define	AC_Round_Mode   30
#define	Round_Mode_even  0
#define	Round_Mode_Down  1

#define	AC_Norm_Mode    29

#define	AC_Inex_mask    28
#define	AC_Inex_flag    20

#define	AC_InvO_mask    26
#define	AC_InvO_flag    18

#define	AC_Unfl_mask    25

#define	AC_ZerD_mask    27
#define	AC_ZerD_flag    19

#define	Standard_QNaN   0xffc00000


/* Register Name Equates */

#define	s1         g0
#define	s2         g1

#define	s2_mant_x  r4
#define	s2_mant    r5
#define	s2_exp     r3
#define	s2_sign    r6

#define	s1_mant_x  r8
#define	s1_mant    r9
#define	s1_exp     r7

#define	rsl_mant_x s2_mant_x
#define	rsl_mant   s2_mant
#define	rsl_exp    s2_exp
#define	rsl_sign   s2_sign

#define	dvsr_mant  s1_mant
#define	dvsr_exp   s1_exp

#define	tmp        r10
#define	tmp2       r11
#define	con_1      r13
#define	con_2      r14
#define	ac         r15

#define	op_type    g2

#define	add_type   1
#define	div_type   2
#define	mul_type   3

#define	out        g0

#define	rp         g4
#define	rp_lo      g4
#define	rp_hi      g5


	.text
	.link_pix

/* *******  A D D I T I O N   S P E C I A L   A R G U M E N T S   *******  */


/*  S1 is a special case value: +/-0, denormal, +/-INF, NaN; S2 is unknown  */

ops_as1_special:
	shlo	1,s1,tmp			/* Drop sign bit */
	ldconst	FP_INF << 24,con_1
	cmpobg	tmp,con_1,ops_as1_06		/* J/ S1 is NaN */
	be	ops_as1_20			/* J/ S1 is INF */
	cmpobne	0,tmp,ops_as1_10		/* J/ S1 is denormal */

	shlo	1,s2,tmp2
	cmpobe	0,tmp2,ops_as1_08		/* J/ 0 + 0 */
	cmpobg	tmp2,con_1,ops_as1_06		/* J/ S2 is NaN */

	ldconst	0x00fffffe,con_1
	cmpobg	tmp2,con_1,ops_as1_04		/* J/ not a denormal */

	bbc	AC_Norm_Mode,ac,ops_as1_05a	/* J/ not normalizing mode */
	bbc	AC_Unfl_mask,ac,ops_as1_05b	/* J/ underflow not masked */

ops_as1_04:
	mov	s2,out				/* Return S2 */
	ret

ops_as1_05a:
	b	_AFP_Fault_Reserved_Encoding_S

ops_as1_05b:
	b	_AFP_Fault_Underflow_S

ops_as1_06:
	b	AFP_NaN_S			/* Handle NaN operand(s) */

ops_as1_08:
	cmpobe	s1,s2,ops_as1_04		/* J/ +0 + +0  or  -0 + -0 */

ops_as1_09:
	shro	AC_Round_Mode,ac,tmp		/* Check round mode for diff sgn 0's */
	cmpo	Round_Mode_Down,tmp
#if	defined(USE_OPN_CC)
	sele	0,1,out
#else
	teste	out
#endif
	shlo	31,out,out			/* -0 iff round down */
	ret


ops_as1_10:
	bbc	AC_Norm_Mode,ac,ops_as1_05a	/* J/ denormal, not norm mode -> fault */

	scanbit	tmp,tmp2
	subo	22+1,tmp2,s1_exp		/* compute denormal exponent */
	subo	tmp2,23,tmp2
	shlo	tmp2,tmp,s1_mant		/* normalize denormal significand */
	lda	0xff800000,tmp2			/* Restore tmp2 mask */
	b	ops_as1_rejoin


ops_as1_20:
	shlo	1,s2,tmp2			/* Drop S2 sign */
	cmpobg	tmp2,con_1,ops_as1_06		/* S2 is NaN -> NaN handler */
	bne	ops_as1_24			/* J/ S2 is not INF (or NaN) */
	cmpobe	s1,s2,ops_as1_04		/* J/ same signed INF's */

	bbc	AC_InvO_mask,ac,ops_as1_22	/* J/ inv oper fault not masked */
#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac		/* Set inv oper flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp		/* Set inv oper flag */
	modac	tmp,tmp,tmp
#endif
	ldconst	Standard_QNaN,out		/* Return a quiet NaN */
	ret

ops_as1_22:
	b	_AFP_Fault_Invalid_Operation_S

ops_as1_24:
	ldconst	0x00fffffe,con_1
	cmpobg	tmp2,con_1,ops_as1_26		/* J/ not denormal, 0 */

	bbc	AC_Norm_Mode,ac,ops_as1_05a	/* J/ not normalizing mode */

ops_as1_26:
	mov	s1,out				/* return s1 */
	ret


/*  S2 is a special case value: +/-0, denormal, +/-INF, NaN; S2 is a non-  */
/*  zero, finite value.                                                    */

ops_as2_special:
	shlo	1,s2,tmp2			/* Drop sign bit */
	ldconst	FP_INF << 24,con_1
	cmpobg	tmp2,con_1,ops_as1_06		/* J/ S2 is a NaN		*/
	be	ops_as1_04			/* J/ INF + finite -> INF	*/
	cmpobe	0,tmp2,ops_as2_02		/* J/ 0				*/
	bbc	AC_Norm_Mode,ac,ops_as1_05a	/* J/ denorm, not norm mode -> fault */

	scanbit	tmp2,tmp			/* Normalize denormal significand */
	subo	22+1,tmp,s2_exp
	subo	tmp,23,tmp
	shlo	tmp,tmp2,s2_mant
	b	ops_as2_rejoin

ops_as2_02:
	bbs	AC_Unfl_mask,ac,ops_as1_26	/* J/ underflow masked	*/
	cmpibl	0,s1_exp,ops_as1_26		/* J/ S1 not denorm	*/
	b	ops_as1_05b			/* J/ underflow fault	*/



/* **  D I V I S I O N   S P E C I A L   A R G U M E N T S  **  */


/*  S2 is a special case value: +/-0, denormal, +/-INF, NaN; S1 is unknown  */

ops_ds2_special:
	shlo	1,s2,tmp			/* Drop sign bit		*/
	ldconst	FP_INF << 24,con_1
	bne	ops_ds2_05b			/* J/ S2 is INF/NaN		*/

	cmpobne.f 0,tmp,ops_ds2_10		/* J/ S2 is denormal		*/

	shlo	1,s1,tmp2
	cmpobe	0,tmp2,ops_ds2_24		/* J/ 0 / 0 -> invalid operation */
	cmpobg	tmp2,con_1,ops_ds2_06		/* J/ S1 is NaN */
	be	ops_ds2_05			/* J/ INF / 0 -> INF w/o ZrDiv fault */

	bbs.t	AC_Norm_Mode,ac,ops_ds2_04	/* J/ normalizing mode */
	ldconst	0x00fffffe,con_1
	cmpobg	tmp2,con_1,ops_ds2_04		/* J/ not a denormal */

ops_ds2_02:
	b	_AFP_Fault_Reserved_Encoding_S

ops_ds2_04:
	bbc	AC_ZerD_mask,ac,ops_ds2_08	/* J/ zero divide fault not masked */
#if	defined(USE_SIMULATED_AC)
	setbit	AC_ZerD_flag,ac,ac		/* Set zero divide flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_ZerD_flag,tmp		/* Set zero divide flag */
	modac	tmp,tmp,tmp
#endif

ops_ds2_05:
	ldconst	FP_INF << 23,out		/* Return properly signed INF */
	or	rsl_sign,out,out
	ret

ops_ds2_05b:
	cmpobe	tmp,con_1,ops_ds2_20		/* J/ S2 is INF */
ops_ds2_06:
	b	AFP_NaN_S			/* Handle NaN operand(s) */

ops_ds2_08:
	b	_AFP_Fault_Zero_Divide_S


ops_ds2_10:
	bbc.f	AC_Norm_Mode,ac,ops_ds2_02	/* J/ denorm, not norm mode -> fault */

	scanbit	tmp,tmp2
	subo	22+1,tmp2,dvsr_exp		/* compute denormal exponent */
	subo	tmp2,23,tmp2
	shlo	tmp2,tmp,dvsr_mant		/* normalize denormal significand */
	lda	0xFF800000,con_1		/* Restore mask			*/
	b	ops_ds2_rejoin


ops_ds2_20:
	shlo	1,s1,tmp2			/* Drop S1 sign */
	cmpobg	tmp2,con_1,ops_ds2_06		/* S1 is NaN -> NaN handler */
	be	ops_ds2_24			/* J/ INF / INF -> invalid oper */
	cmpobe	0,tmp2,ops_ds2_22		/* J/ 0 / INF -> 0 */
	bbs	AC_Norm_Mode,ac,ops_ds2_22	/* J/ normalizing mode */
	ldconst	0x00fffffe,con_1
	cmpoble	tmp2,con_1,ops_ds2_02		/* J/ S1 is a denormal */
	
ops_ds2_22:
	mov	rsl_sign,out			/* Return properly signed 0 */
	ret

ops_ds2_24:
	bbc	AC_InvO_mask,ac,ops_ds2_25	/* J/ inv oper fault not masked */
#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac		/* Set inv oper flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp		/* Set inv oper flag */
	modac	tmp,tmp,tmp
#endif
	ldconst	Standard_QNaN,out		/* Return a quiet NaN */
	ret

ops_ds2_25:
	b	_AFP_Fault_Invalid_Operation_S



/*  S1 is a special case value: +/-0, denormal, +/-INF, NaN; S2 is a non-  */
/*  zero, finite value.                                                    */

ops_ds1_special:
	shlo	1,s1,tmp2			/* Drop sign bit */
	cmpobe.t 0,tmp2,ops_ds2_22		/* J/ 0 / finite -> 0 */
	ldconst	FP_INF << 24,con_1
	cmpobg	tmp2,con_1,ops_ds2_06		/* J/ S1 is a NaN */
	be	ops_ds2_05			/* J/ INF / finite -> INF */
	bbc.f	AC_Norm_Mode,ac,ops_ds2_02	/* J/ denorm, not norm mode -> fault */

	scanbit	tmp2,tmp			/* Normalize denormal significand */
	subo	22+1,tmp,rsl_exp
	subo	tmp,23,tmp
	shlo	tmp,tmp2,rsl_mant
	b	ops_ds1_rejoin



/* **  M U L T I P L I C A T I O N   S P E C I A L   A R G U M E N T S  **  */


/*  S1 is a special case value: +/-0, denormal, +/-INF, NaN; S2 is unknown  */

ops_ms1_special:
	shlo	1,s1,tmp			/* Drop sign bit and exp	*/
	lda	FP_INF << 24,con_1
	bne	ops_ms1_05			/* J/ S1 is NaN/INF		*/

	cmpobne.f 0,tmp,ops_mss1_10		/* J/ S1 is denormal		*/

	shlo	1,s2,tmp2
	cmpobe	0,tmp2,ops_mss1_04		/* J/ 0 * 0 */
	cmpobg	tmp2,con_1,ops_ms1_06		/* J/ S2 is NaN */
	be	ops_ms1_24			/* J/ 0 * INF -> invalid operation */

	bbs.t	AC_Norm_Mode,ac,ops_mss1_04	/* J/ normalizing mode */
	ldconst	0x00fffffe,con_1
	cmpobg	tmp2,con_1,ops_mss1_04		/* J/ not a denormal */

ops_ms1_02:
	b	_AFP_Fault_Reserved_Encoding_S

ops_mss1_04:
	mov	s2_sign,out			/* Return properly signed zero */
	ret

ops_ms1_05:
	cmpobe.t tmp,con_1,ops_ms1_20		/* J/ S1 is INF */
ops_ms1_06:
	b	AFP_NaN_S			/* Handle NaN operand(s) */


ops_mss1_10:
	bbc.f	AC_Norm_Mode,ac,ops_ms1_02	/* J/ denorm, not norm mode -> fault */

	scanbit	tmp,tmp2
	subo	22+1,tmp2,s1_exp		/* compute denormal exponent */
	subo	tmp2,31,tmp2
	shlo	tmp2,tmp,s1_mant		/* normalize denormal significand */
	b	ops_ms1_rejoin


ops_ms1_20:
	shlo	1,s2,tmp2			/* Drop S2 sign */
	cmpobg	tmp2,con_1,ops_ms1_06		/* S2 is NaN -> NaN handler */
	cmpobe	0,tmp2,ops_ms1_24		/* J/ INF * 0 */
	bbs.t	AC_Norm_Mode,ac,ops_ms1_22	/* J/ normalizing mode */
	ldconst	0x00fffffe,con_1
	cmpoble	tmp2,con_1,ops_ms1_02		/* J/ S2 is a denormal */
	
ops_ms1_22:
	ldconst	FP_INF << 23,out		/* Return properly signed INF */
	or	s2_sign,out,out
	ret

ops_ms1_24:
	bbc.f	AC_InvO_mask,ac,ops_ms1_25	/* J/ inv oper fault not masked */
#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac		/* Set inv oper flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp		/* Set inv oper flag */
	modac	tmp,tmp,tmp
#endif
	ldconst	Standard_QNaN,out		/* Return a quiet NaN */
	ret

ops_ms1_25:
	b	_AFP_Fault_Invalid_Operation_S


/*  S2 is a special case value: +/-0, denormal, +/-INF, NaN; S1 is a non-  */
/*  zero, finite value.                                                    */

ops_ms2_special:
	shlo	1,s2,tmp2			/* Drop sign bit */
	lda	FP_INF << 24,con_1
	cmpobg	tmp2,con_1,ops_ms1_06		/* J/ S2 is a NaN */
	be	ops_ms1_22			/* J/ INF * finite -> INF */
	cmpobe	0,tmp2,ops_mss1_04		/* J/ 0 * finite -> 0 */
	bbc.f	AC_Norm_Mode,ac,ops_ms1_02	/* J/ denorm, not norm mode -> fault */

	scanbit	tmp2,tmp			/* Normalize denormal significand */
	subo	22+1,tmp,s2_exp
	subo	tmp,23,tmp
	shlo	tmp,tmp2,s2_mant

	emul	s1_mant,s2_mant,s2_mant_x	/* Start for overlap	*/
	b	ops_ms2_rejoin



	.align	MAJOR_CODE_ALIGNMENT


/* ******  M U L T I P L I C A T I O N   P R O C E S S I N G  ****** */

___mulsf3:
	xor	s1,s2,s2_sign			/* Compute result sign		*/

#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	shlo	8,s1,s1_mant			/* Left justify signif bits	*/
	shlo1(s1,s1_exp)

	shri	24,s1_exp,tmp
	shlo1(s2,s2_exp)

	shro	31,s2_sign,s2_sign
	addlda(1,tmp,tmp)

#if	!defined(USE_CMP_BCC)
	cmpo	1,tmp
#endif
	movlda(mul_type,op_type)

	shlo	31,s2_sign,s2_sign		/* Isolate result sign		*/

#if	defined(USE_CMP_BCC)
	cmpobge	1,tmp,ops_ms1_special		/* J/ S1 is 0/Denorm/INF/NaN	*/
#else
	bge.f	ops_ms1_special			/* J/ S1 is 0/Denorm/INF/NaN	*/
#endif

	setbit	31,s1_mant,s1_mant		/* Force "j" bit		*/
	shro	24,s1_exp,s1_exp

ops_ms1_rejoin:

	shri	24,s2_exp,tmp
	lda	0xff800000,con_1		/* Inverse mask for signif	*/

	notand	s2,con_1,s2_mant
	addlda(1,tmp,tmp)

	subo	con_1,s2_mant,s2_mant

	emul	s1_mant,s2_mant,s2_mant_x	/* Start for overlap	*/

#if	!defined(USE_CMP_BCC)
	cmpo	1,tmp
#endif

	shro	24,s2_exp,s2_exp
#if	defined(USE_CMP_BCC)
	cmpobge	1,tmp,ops_ms2_special		/* J/ S2 is 0/Denorm/INF/NaN	*/
#else
	bge.f	ops_ms2_special			/* J/ S2 is 0/Denorm/INF/NaN	*/
#endif

ops_ms2_rejoin:

/*
 * s1_mant is aligned as xxxxxx00, s2_mant is aligned as 00xxxxxx.
 * The result of the emul will be 00rrrrrr ssssss00.  Thus, the
 * result rrrrrr is correctly aligned and ssssss is the round
 * and sticky information for rounding.
 */
	addo	s1_exp,s2_exp,s2_exp
	lda	FP_Bias-1,tmp

	subo	tmp,s2_exp,s2_exp

#if	defined(USE_CMP_BCC)
	bbs	23,s2_mant,ops_AFP_RRC_S
#else
	chkbit	23,s2_mant
	bt.t	ops_AFP_RRC_S
#endif

	addc	s2_mant_x,s2_mant_x,s2_mant_x	/* Normalization shift */
	subo	1,s2_exp,s2_exp
	addc	s2_mant,s2_mant,s2_mant
	b	ops_AFP_RRC_S


	.align	MINOR_CODE_ALIGNMENT


/* **  A D D I T I O N / S U B T R A C T I O N   P R O C E S S I N G  ** */

___subsf3:
	notbit	31,s2,s2

___addsf3:
	addo	s1,s1,s1_exp			/* exp + mant (no sign bit)  */

	addc	s2,s2,s2_exp			/* exp + mant (sign -> carry)	*/

	shri	24,s1_exp,tmp			/* check S1 for special case */
	lda	0xff800000,tmp2			/* Extract significand, exp  */

	alterbit 31,0,s2_sign			/* select S2 sign bit        */
	addlda(1,tmp,tmp)

#if	!defined(USE_CMP_BCC)
	cmpo	1,tmp				/* S1 NaN/INF, 0/Denormal?   */
#endif
	movlda(add_type,op_type)		/* for fault handler operation info */

	andnot	tmp2,s1,s1_mant			/* extract S1 significand    */
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	shro	24,s1_exp,s1_exp		/* extract S1 biased exp     */
	subo	tmp2,s1_mant,s1_mant		/* set "j" bit		     */

#if	defined(USE_CMP_BCC)
	cmpobge	1,tmp,ops_as1_special		/* J/ S1 is 0/denorm/INF/NaN */
#else
	bge.f	ops_as1_special			/* J/ S1 is 0/denorm/INF/NaN */
#endif

ops_as1_rejoin:

	shri	24,s2_exp,tmp			/* Compute S2 exp test value */
	shro	24,s2_exp,s2_exp		/* extract S2 exp            */

	andnot	tmp2,s2,s2_mant			/* extract S2 significand    */
	addlda(1,tmp,tmp)

#if	!defined(USE_CMP_BCC)
	cmpo	1,tmp
#endif
	subo	tmp2,s2_mant,s2_mant		/* Set "j" bit */

#if	defined(USE_CMP_BCC)
	cmpobge	1,tmp,ops_as2_special		/* J/ S2 is zero/denormal/INF/NaN */
#else
	bge.f	ops_as2_special			/* J/ S2 is zero/denormal/INF/NaN */
#endif

ops_as2_rejoin:

/*
 * Finite, non-zero numbers
 */
#if	!defined(USE_CMP_BCC)
	cmpi	s1_exp,s2_exp
#endif
	movlda(0,s1_mant_x)

	subo	s1_exp,s2_exp,tmp
	movlda(0,s2_mant_x)
#if	!defined(USE_ESHRO)
	addo	31,1,tmp2
#endif

#if	defined(USE_CMP_BCC)
	cmpibg	s1_exp,s2_exp,ops_addr_14
#else
	bg	ops_addr_14
#endif

	be	ops_addr_18

/*
 * s1_mant = sshr(s1_mant, tmp)
 */

#if	!defined(USE_CMP_BCC)
	cmpo	25,tmp
#endif

#if	defined(USE_ESHRO)
	eshro	tmp,s1_mant_x,s1_mant_x
	shro	tmp,s1_mant,s1_mant
#else
	subo	tmp,tmp2,tmp2
	shlo	tmp2,s1_mant,s1_mant_x
	shro	tmp,s1_mant,s1_mant
#endif

#if	defined(USE_CMP_BCC)
	cmpobge	25,tmp,ops_addr_18
#else
	bge.t	ops_addr_18
#endif

	mov	1,s1_mant_x
	b	ops_addr_18

/*
 * s2_mant = sshr(s2_mant, -tmp)
 */
	.align	MINOR_CODE_ALIGNMENT

ops_addr_14:
	subo	tmp,0,tmp
#if	!defined(USE_CMP_BCC)
	cmpo	25,tmp
#endif
	movldar(s1_exp,s2_exp)

#if	defined(USE_ESHRO)
	eshro	tmp,s2_mant_x,s2_mant_x
	shro	tmp,s2_mant,s2_mant
#else
	subo	tmp,tmp2,tmp2
	shlo	tmp2,s2_mant,s2_mant_x
	shro	tmp,s2_mant,s2_mant
#endif

#if	defined(USE_CMP_BCC)
	cmpobge	25,tmp,ops_addr_18		/* J/ 25 >= shift	*/
#else
	bge.t	ops_addr_18			/* J/ 25 >= shift	*/
#endif

	mov	1,s2_mant_x			/* Sticky bit		*/

ops_addr_18:
/*
 * Operands are aligned; now determine whether to add or subtract
 */
	xor	s1,s2,tmp
	addc	tmp,tmp,tmp
	be.f	ops_addr_22
/*
 * Mantissa addition
 */
	addc	s1_mant_x,s2_mant_x,s2_mant_x
	addc	s1_mant,s2_mant,s2_mant

#if	defined(USE_CMP_BCC)
	bbc	24,s2_mant,ops_AFP_RRC_S	/* J/ no shift req'd */
#else
	chkbit	24,s2_mant
	bf.t	ops_AFP_RRC_S			/* J/ no shift req'd */
#endif

#if	defined(USE_ESHRO)
	eshro	1,s2_mant_x,s2_mant_x		/* two word, 1 bit right shft */
	shro	1,s2_mant,s2_mant
#else
	shlo	31,s2_mant,tmp			/* LS bit into tmp */
	shro	1,s2_mant,s2_mant		/* two word, 1 bit right shft */
	shro	1,s2_mant_x,s2_mant_x
	or	tmp,s2_mant_x,s2_mant_x
#endif

	addlda(1,s2_exp,s2_exp)			/* bump exponent */


ops_AFP_RRC_S:					/* round and range check */
	cmpi	s2_exp,2			/* possible range fault test	*/
	lda	FP_INF-2,con_1

	concmpi	s2_exp,con_1			/* possible range fault test	*/
	lda	0x7FFFFFFF,con_2		/* Round/even-nearest constant	*/
	bne.f	AFP_RRC_S_2			/* J/ possible range fault	*/

	cmpo	s2_mant_x,1			/* Check for Inexact fault/flag	*/
	lda	(1 << AC_Inex_mask)+(1 << AC_Inex_flag),con_1

	andnot	ac,con_1,tmp			/* Check for Inexact fault/flag	*/
	concmpo	1,tmp				/* (NZ if tmp = 0)		*/

	shro	AC_Round_Mode,ac,tmp		/* rounding mode to tmp		*/
	be.f	AFP_RRC_S_2			/* J/ inexact + fault/flag set	*/

#if	defined(USE_CMP_BCC)
	cmpobne	Round_Mode_even,tmp,AFP_RRC_S_2	/* J/ not round-to-nearest/even	*/
#else
	cmpo	Round_Mode_even,tmp		/* Round-to-nearest/even?	*/
	bne.f	AFP_RRC_S_2			/* J/ not round-to-nearest/even	*/
#endif

	subo	1,s2_exp,s2_exp			/* Compensate for "j" bit	*/
	chkbit	0,s2_mant			/* Q0 bit into carry		*/
	shlo	23,s2_exp,tmp			/* Position exponent		*/
	addc	con_2,s2_mant_x,tmp2		/* Compute rounding		*/
	addo	s2_sign,tmp,tmp			/* Mix exponent with sign	*/
	addc	tmp,s2_mant,out			/* Combine for result		*/
	ret


/*
 * Mantissa subtraction
 */

	.align	MINOR_CODE_ALIGNMENT
ops_addr_22:
	subc	s1_mant_x,s2_mant_x,s2_mant_x
	subc	s1_mant,s2_mant,s2_mant
	be.t	ops_addr_24			/* J/ no borrow out */
	subc	s2_mant_x,1,s2_mant_x
	subc	s2_mant,0,s2_mant
	notbit	31,s2_sign,s2_sign
ops_addr_24:
	or	s2_mant,s2_mant_x,tmp

#if	defined(USE_CMP_BCC)
	cmpobe	0,tmp,ops_as1_09		/* J/ zero result */
#else
	cmpo	0,tmp
	be.f	ops_as1_09			/* J/ zero result */
#endif

#if	defined(USE_CMP_BCC)
	bbs	23,s2_mant,ops_AFP_RRC_S	/* J/ no shift req'd */
#else
	chkbit	23,s2_mant
	bt.t	ops_AFP_RRC_S			/* J/ no shift req'd */
#endif

	scanbit	s2_mant,tmp
	addo	9,tmp,tmp2			/* right shift for _lo -> _hi */
	subo	tmp,23,tmp			/* left shift for norm shift */
#if	defined(USE_ESHRO)
	eshro	tmp2,s2_mant_x,s2_mant		/* inter-word bits */
#else
	shro	tmp2,s2_mant_x,tmp2		/* inter-word bits */
	shlo	tmp,s2_mant,s2_mant
	or	tmp2,s2_mant,s2_mant
#endif
	shlo	tmp,s2_mant_x,s2_mant_x
	subo	tmp,s2_exp,s2_exp

	b	ops_AFP_RRC_S


	.align	MINOR_CODE_ALIGNMENT

/* ******  D I V I S I O N   P R O C E S S I N G  ****** */

___divsf3:
	xor	s1,s2,rsl_sign			/* Compute result sign		*/

#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	shro	31,rsl_sign,rsl_sign
	shlo1(s2,dvsr_exp)

	shri	24,dvsr_exp,tmp
	movlda(div_type,op_type)

	shlo	31,rsl_sign,rsl_sign		/* Isolate result sign bit	*/
	addlda(1,tmp,tmp)

#if	!defined(USE_CMP_BCC)
	cmpo	1,tmp
#endif
	lda	0xff800000,con_1		/* Mask to unpack s2		*/

	notand	s2,con_1,dvsr_mant		/* Extract significand bits	*/
	shlo1(s1,rsl_exp)

#if	defined(USE_CMP_BCC)
	cmpobge	1,tmp,ops_ds2_special		/* J/ s2 is 0/denorm/INF/NaN	*/
#else
	bge.f	ops_ds2_special			/* J/ s2 is 0/denorm/INF/NaN	*/
#endif

	shro	24,dvsr_exp,dvsr_exp
	subo	con_1,dvsr_mant,dvsr_mant	/* Set "j" bit			*/

ops_ds2_rejoin:

	shri	24,rsl_exp,tmp
	movlda(0,rp_lo)

	andnot	con_1,s1,rsl_mant		/* Extract significand bits	*/
	addlda(1,tmp,tmp)

#if	!defined(USE_CMP_BCC)
	cmpo	1,tmp
#endif
	subo	con_1,rsl_mant,rsl_mant		/* Set "j" bit			*/

	shro	24,rsl_exp,rsl_exp

#if	defined(USE_CMP_BCC)
	cmpobge	1,tmp,ops_ds1_special		/* J/ s1 is 0/denorm/INF/NaN */
#else
	bge.f	ops_ds1_special			/* J/ s1 is 0/denorm/INF/NaN */
#endif

ops_ds1_rejoin:

	cmpo	dvsr_mant,rsl_mant

#if	defined(USE_LDA_REG_OFF)
	lda	FP_Bias-1(rsl_exp),rsl_exp
#else
	lda	FP_Bias-1,tmp
	addo	tmp,rsl_exp,rsl_exp
#endif

	shlo	7,dvsr_mant,dvsr_mant		/* Position MS result bit	*/
	movldar(rsl_mant,rp_hi)			/* Zero extend to long		*/

	subo	dvsr_exp,rsl_exp,rsl_exp	/* Compute result exp		*/
	bg	ops_divr_10

	shlo	1,dvsr_mant,dvsr_mant		/* Do norm shifting now		*/
	addlda(1,rsl_exp,rsl_exp)
ops_divr_10:

	ediv	dvsr_mant,rp,rp

	shro	1,rp_lo,rsl_mant_x		/* Will not drop any bits	*/
	shlo	31,rp_hi,tmp			/* R bit			*/
	addo	rsl_mant_x,tmp,rsl_mant_x
	shro	1,rp_hi,rsl_mant		/* Position result		*/

	b	ops_AFP_RRC_S			/* Round and range the result	*/
