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
/*      addtf3.c - Extended Precision Addition Routine (AFP-960)	      */
/*									      */
/******************************************************************************/

#include "asmopt.h"

#if	!defined(KEEP_INTERNAL_LABELS)
#define	atf_s1_rejoin	L_atf_s1_rejoin
#define	atf_s2_rejoin	L_atf_s2_rejoin
#define	atf_s1_sshr	L_atf_s1_sshr
#define	atf_s1_dshr	L_atf_s1_dshr
#define	atf_s1_tshr	L_atf_s1_tshr
#define	atf_s1_zshr	L_atf_s1_zshr
#define	atf_l1		L_atf_l1
#define	atf_s2_sshr	L_atf_s2_sshr
#define	atf_s2_dshr	L_atf_s2_dshr
#define	atf_s2_tshr	L_atf_s2_tshr
#define	atf_s2_zshr	L_atf_s2_zshr
#define	atf_l2		L_atf_l2
#define	atf_l2_a	L_atf_l2_a
#define	atf_l3		L_atf_l3
#define	atf_l4		L_atf_l4
#define	atf_l4_a	L_atf_l4_a
#define	atf_l4_b	L_atf_l4_b
#define	atf_l4_c	L_atf_l4_c
#define	atf_s1_special	L_atf_s1_special
#define	atf_s1_unnormal	L_atf_s1_unnormal
#define	atf_s2_unnormal	L_atf_s2_unnormal
#define	atf_s1_02	L_atf_s1_02
#define	atf_s1_04	L_atf_s1_04
#define	atf_s1_06	L_atf_s1_06
#define	atf_s1_08	L_atf_s1_08
#define	atf_s1_10	L_atf_s1_10
#define	atf_s1_12	L_atf_s1_12
#define	atf_s1_13	L_atf_s1_13
#define	atf_s1_14	L_atf_s1_14
#define	atf_s1_16	L_atf_s1_16
#define	atf_s1_18	L_atf_s1_18
#define	atf_s1_19	L_atf_s1_19
#define	atf_s1_20	L_atf_s1_20
#define	atf_s1_22	L_atf_s1_22
#define	atf_s1_24	L_atf_s1_24
#define	atf_s1_26	L_atf_s1_26
#define	atf_s1_28	L_atf_s1_28
#define	atf_s1_30	L_atf_s1_30
#define	atf_s2_special	L_atf_s2_special
#define	atf_s2_02	L_atf_s2_02
#define	atf_s2_04	L_atf_s2_04
#define	atf_s2_06	L_atf_s2_06
#endif


	.file	"addtf3.as"
	.globl	___addtf3,___subtf3

#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	AFP_RRC_T
	.globl	AFP_NaN_T

	.globl	_AFP_Fault_Invalid_Operation_T
	.globl	_AFP_Fault_Reserved_Encoding_T
	.globl	_AFP_Fault_Underflow_T


#define	AC_Round_Mode   30
#define	Round_Mode_Down 1

#define	AC_Norm_Mode    29

#define	AC_InvO_mask    26
#define	AC_InvO_flag    18

#define	AC_Unfl_mask    25

#define	Standard_QNaN_se  0x0000ffff
#define Standard_QNaN_mhi 0xc0000000
#define	Standard_QNaN_mlo 0x00000000


/* Register Name Equates */

#define	s1         g0
#define	s1_mlo     s1
#define s1_mhi     g1
#define	s1_se      g2
#define s2         g4
#define s2_mlo     s2
#define s2_mhi     g5
#define s2_se      g6

#define	s2_mant_x  r3
#define	s2_mant    r4
#define s2_mant_lo s2_mant
#define s2_mant_hi r5
#define	s2_exp     r6
#define	s2_sign    r7

#define	s1_mant_x  r10
#define	s1_mant    r8
#define s1_mant_lo s1_mant
#define s1_mant_hi r9
#define	s1_exp     r11

#define	tmp        r12
#define	tmp2       r13
#define	tmp3       r14
#define	ac         r15

#define	con_1      g3

#define	out        g0
#define	out_mlo    out
#define	out_mhi    g1
#define out_se     g2

#define	op_type    g7
#define	add_type   1


	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

___subtf3:
	notbit	15,s2_se,s2_se
___addtf3:
	mov	add_type,op_type

#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	shlo	32-15,s1_se,s1_exp		/* exp only (no sign bit) */
	setbit	15,0,s2_sign			/* sign bit mask */

	shri	32-15,s1_exp,tmp		/* check S1 for potentially */
	and	s2_se,s2_sign,s2_sign

	shlo	32-15,s2_se,s2_exp		/* exp only (no sign bit)   */
	addlda(1,tmp,tmp)			/* special case handling    */

#if	!defined(USE_CMP_BCC)
	cmpo	1,tmp
#endif
	movldar(s1_mlo,s1_mant_lo)		/* copy mantissa bits      */

#if	defined(USE_CMP_BCC)
	cmpobge	1,tmp,atf_s1_special		/* J/ NaN/INF, 0, denormal  */
#else
	bge.f	atf_s1_special			/* J/ NaN/INF, 0, denormal  */
#endif

#if	!defined(USE_CMP_BCC)
	chkbit	31,s1_mhi
#endif

	shro	32-15,s1_exp,s1_exp		/* right justify exponent  */
	movldar(s1_mhi,s1_mant_hi)

#if	defined(USE_CMP_BCC)
	bbc	31,s1_mhi,atf_s1_unnormal	/* J/ S1 is an unnormal */
#else
	bf.f	atf_s1_unnormal			/* J/ S1 is an unnormal */
#endif

atf_s1_rejoin:

	shri	32-15,s2_exp,tmp
	movldar(s2_mlo,s2_mant_lo)		/* copy mantissa bits      */

	shro	32-15,s2_exp,s2_exp		/* right justify exponent  */
	addlda(1,tmp,tmp)			/* check S2 for potentially */

#if	!defined(USE_CMP_BCC)
	cmpo	1,tmp
#endif

	movldar(s2_mhi,s2_mant_hi)

#if	defined(USE_CMP_BCC)
	cmpobge	1,tmp,atf_s2_special		/* J/ NaN/INF, 0/denormal   */
#else
	bge.f	atf_s2_special			/* J/ NaN/INF, 0/denormal   */
#endif

#if	defined(USE_CMP_BCC)
	bbc	31,s2_mhi,atf_s2_unnormal	/* J/ S2 is an unnormal */
#else
	chkbit	31,s2_mhi
	bf.f	atf_s2_unnormal			/* J/ S2 is an unnormal */
#endif

atf_s2_rejoin:

#if	!defined(USE_CMP_BCC)
	cmpi	s1_exp,s2_exp
#endif
	movlda(0,s1_mant_x)			/* zero mantissa extentions */

	subo	s1_exp,s2_exp,tmp		/* shift count to align */
	movlda(0,s2_mant_x)

	addo	31,1,tmp3
#if	defined(USE_CMP_BCC)
	cmpibg	s1_exp,s2_exp,atf_l1		/* J/ s1_exp > s2_exp     */
#else
	bg	atf_l1				/* J/ s1_exp > s2_exp     */
#endif

	be	atf_l2				/* J/ no adjustment req'd */
/*
 * s1_mant = dshr(s1_mant, tmp)
 */

#if	!defined(USE_CMP_BCC)
	cmpo	tmp,tmp3
#endif

	subo	tmp,tmp3,tmp2			/* tmp2 = 32 - tmp */

#if	defined(USE_CMP_BCC)
	cmpobge	tmp,tmp3,atf_s1_dshr		/* J/ >= one word shift */
#else
	bge.f	atf_s1_dshr			/* J/ >= one word shift */
#endif

atf_s1_sshr:
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
	b	atf_l2

atf_s1_dshr:
	subo	tmp3,tmp,tmp			/* Reduce shift count by 32 */

#if	!defined(USE_CMP_BCC)
	cmpo	tmp3,tmp
#endif

	subo	tmp,tmp3,tmp2			/* tmp2 = 32 - tmp */

#if	defined(USE_CMP_BCC)
	cmpoble	tmp3,tmp,atf_s1_tshr		/* J/ >= two word shift */
#else
	ble.f	atf_s1_tshr			/* J/ >= two word shift */
#endif

#if	defined(USE_ESHRO)
	shlo	tmp2,s1_mant_lo,tmp3		/* _lo to oblivion bits */
	cmpo	0,tmp3
	eshro	tmp,s1_mant,s1_mant_x		/* _x bits */
	subc	1,0,tmp3			/* if any lost _lo bits */
	shro	tmp,s1_mant_hi,s1_mant_lo	/* _hi -> _lo bits */
	ornot	tmp3,s1_mant_x,s1_mant_x	/* lost bits to LS bit of _x */
	lda	0,s1_mant_hi
#else
	shlo	tmp2,s1_mant_lo,tmp3		/* _lo to oblivion bits */
	cmpo	0,tmp3
	shro	tmp,s1_mant_lo,s1_mant_x	/* _lo -> _x bits */
	shlo	tmp2,s1_mant_hi,s1_mant_lo	/* _hi -> _x bits */
	or	s1_mant_x,s1_mant_lo,s1_mant_x	/* put together _x */
	subc	1,0,tmp3			/* if any lost _lo bits */
	shro	tmp,s1_mant_hi,s1_mant_lo	/* _hi -> _lo bits */
	ornot	tmp3,s1_mant_x,s1_mant_x	/* lost bits to LS bit of _x */
	mov	0,s1_mant_hi
#endif
	b	atf_l2

atf_s1_tshr:
	subo	tmp3,tmp,tmp			/* Reduce shift count by 32 */

#if	!defined(USE_CMP_BCC)
	cmpo	2,tmp				/* Max valid shift count: 65 */
#endif

	subo	tmp,tmp3,tmp2
	shlo	tmp2,s1_mant_hi,tmp3		/* Drop out shift count */
	or	s1_mant_lo,tmp3,tmp3

#if	defined(USE_CMP_BCC)
	cmpoble	2,tmp,atf_s1_zshr		/* J/ > 65 bit shift */
#else
	ble.t	atf_s1_zshr			/* J/ > 65 bit shift */
#endif

	cmpo	tmp3,0				/* Set C if no dropped bits */
	shro	tmp,s1_mant_hi,s1_mant_x
	movlda(0,s1_mant_hi)
	subc	1,0,tmp3
	notor	s1_mant_x,tmp3,s1_mant_x	/* Set LS bit if dropped bits */
	movlda(0,s1_mant_lo)
	b	atf_l2

atf_s1_zshr:
	movl	0,s1_mant
	movlda(1,s1_mant_x)
	b	atf_l2


/*
 * s2_mant = dshr(s2_mant, -tmp)
 */
	.align	MINOR_CODE_ALIGNMENT
atf_l1:
	subo	tmp,0,tmp			/* Neg TMP value */
	mov	s1_exp,s2_exp			/* Max exp value */

#if	!defined(USE_CMP_BCC)
	cmpo	tmp,tmp3
#endif

	subo	tmp,tmp3,tmp2			/* tmp2 = 32 - tmp */

#if	defined(USE_CMP_BCC)
	cmpobge	tmp,tmp3,atf_s2_dshr		/* J/ >= one word shift */
#else
	bge.f	atf_s2_dshr			/* J/ >= one word shift */
#endif

atf_s2_sshr:
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
	b	atf_l2

atf_s2_dshr:
	subo	tmp3,tmp,tmp			/* Reduce shift count by 32 */

#if	!defined(USE_CMP_BCC)
	cmpo	tmp3,tmp
#endif

	subo	tmp,tmp3,tmp2			/* tmp2 = 32 - tmp */

#if	defined(USE_CMP_BCC)
	cmpoble	tmp3,tmp,atf_s2_tshr		/* J/ >= two word shift */
#else
	ble.f	atf_s2_tshr			/* J/ >= two word shift */
#endif

#if	defined(USE_ESHRO)
	shlo	tmp2,s2_mant_lo,tmp3		/* _lo to oblivion bits */
	cmpo	0,tmp3
	eshro	tmp,s2_mant,s2_mant_x		/* _x bits */
	subc	1,0,tmp3
	shro	tmp,s2_mant_hi,s2_mant_lo	/* _hi -> _lo bits */
	ornot	tmp3,s2_mant_x,s2_mant_x	/* lost bits to LS bit of _x */
	lda	0,s2_mant_hi
#else
	shlo	tmp2,s2_mant_lo,tmp3		/* _lo to oblivion bits */
	cmpo	0,tmp3
	shro	tmp,s2_mant_lo,s2_mant_x	/* _lo -> _x bits */
	shlo	tmp2,s2_mant_hi,s2_mant_lo	/* _hi -> _x bits */
	or	s2_mant_x,s2_mant_lo,s2_mant_x	/* put together _x */
	shro	tmp,s2_mant_hi,s2_mant_lo	/* _hi -> _lo bits */

#if	defined(USE_OPN_CC)
	selne	0,1,tmp				/* if any lost _lo bits */
#else
	testne	tmp				/* if any lost _lo bits */
#endif

	or	s2_mant_x,tmp,s2_mant_x		/* lost bits to LS bit of _x */
	ldconst	0,s2_mant_hi
#endif
	b	atf_l2

atf_s2_tshr:
	subo	tmp3,tmp,tmp			/* Reduce shift count by 32 */

#if	!defined(USE_CMP_BCC)
	cmpo	2,tmp				/* Max valid shift count: 65 */
#endif

	subo	tmp,tmp3,tmp2
	shlo	tmp2,s2_mant_hi,tmp3		/* Drop out shift count */
	or	s2_mant_lo,tmp3,tmp3

#if	defined(USE_CMP_BCC)
	cmpoble	2,tmp,atf_s2_zshr		/* J/ > 65 bit shift */
#else
	ble.t	atf_s2_zshr			/* J/ > 65 bit shift */
#endif

	cmpo	tmp3,0				/* Set C if no dropped bits */
	shro	tmp,s2_mant_hi,s2_mant_x
	movlda(0,s2_mant_hi)
	subc	1,0,tmp3
	notor	s2_mant_x,tmp3,s2_mant_x	/* Set LS bit if dropped bits */
	movlda(0,s2_mant_lo)
	b	atf_l2

atf_s2_zshr:
	movl	0,s2_mant
	movlda(1,s2_mant_x)
/* **	b	atf_l2		** */


/*
 * Operands are aligned; now determine whether to add or subtract
 */
atf_l2:
	xor	s1_se,s2_se,tmp
#if	defined(USE_CMP_BCC)
	bbs	15,tmp,atf_l3			/* J/ different signs */
#else
	chkbit	15,tmp
	be.f	atf_l3				/* J/ different signs */
#endif

/*
 * Mantissa addition
 */
     /*	cmpo	1,0 */				/* CHKBIT/BBS cleared carry bit */
	addc	s1_mant_lo,s2_mant_lo,s2_mant_lo
	or	s1_mant_x,s2_mant_x,s2_mant_x
	addc	s1_mant_hi,s2_mant_hi,s2_mant_hi
	and	1,s2_mant_x,tmp			/* LS bit into tmp */
	be.f	atf_l2_a			/* J/ overflow */
	b	AFP_RRC_T			/* J/ no overflow past j bit */

atf_l2_a:
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

	setbit	31,s2_mant_hi,s2_mant_hi	/* set j bit */
	addlda(1,s2_exp,s2_exp)			/* bump result exp */
	b	AFP_RRC_T

/*
 * Mantissa subtraction
 */
atf_l3:
     /*	cmpo	0,0 */				/* CHKBIT set BORROW/ bit */
	subc	s1_mant_x,s2_mant_x,s2_mant_x
	subc	s1_mant_lo,s2_mant_lo,s2_mant_lo
	subc	s1_mant_hi,s2_mant_hi,s2_mant_hi
	be.t	atf_l4				/* J/ no sign rev */

	notbit	15,s2_sign,s2_sign		/* Negate mantissa */
	subc	s2_mant_x,1,s2_mant_x		/* "1" for left over borrow */
	subc	s2_mant_lo,0,s2_mant_lo
	subc	s2_mant_hi,0,s2_mant_hi

atf_l4:
	bbs.t	31,s2_mant_hi,atf_l4_c		 /* J/ no shifting to do */

	scanbit	s2_mant_hi,tmp

	bf.f	atf_l4_a			/* J/ norm shift (>= 32)  */

	subo	tmp,31,tmp2			/* Left shift needed	 */
	addlda(1,tmp,tmp)			/* Right shift (wd->wd)	 */

#if	defined(USE_ESHRO)
	eshro	tmp,s2_mant,s2_mant_hi
#else
	shlo	tmp2,s2_mant_hi,s2_mant_hi
	shro	tmp,s2_mant_lo,tmp3
	or	s2_mant_hi,tmp3,s2_mant_hi
#endif

	shlo	tmp2,s2_mant_lo,s2_mant_lo
	shro	tmp,s2_mant_x,tmp3
	or	s2_mant_lo,tmp3,s2_mant_lo
	shlo	tmp2,s2_mant_x,s2_mant_x

	subo	tmp2,s2_exp,s2_exp
	b	AFP_RRC_T

atf_l4_a:
	scanbit	s2_mant_lo,tmp
	bf.f	atf_l4_b			/* J/ check for zero/R	 */

	subo	tmp,31,tmp2			/* Left shift needed	 */
	addlda(1,tmp,tmp)			/* Right shift (wd->wd)	 */

	shlo	tmp2,s2_mant_lo,s2_mant_hi
	shlo	tmp2,s2_mant_x,s2_mant_lo
	shro	tmp,s2_mant_x,tmp
	or	s2_mant_hi,tmp,s2_mant_hi
	mov	0,s2_mant_x

	subo	tmp2,s2_exp,s2_exp		/* adjust exponent */
	subo	31,s2_exp,s2_exp
	subo	1,s2_exp,s2_exp
	b	AFP_RRC_T			/* Round/Range Check */

atf_l4_b:
	cmpobe	0,s2_mant_x,atf_s1_08		/* J/ zero result */

	shlo	6,1,tmp				/* Result = R bit */
	subo	tmp,s2_exp,s2_exp
	setbit	31,0,s2_mant_hi
	movlda(0,s2_mant_x)

atf_l4_c:
	b	AFP_RRC_T



/*  S1 is a special case value: +/-0, denormal, +/-INF, NaN; S2 is unknown  */

atf_s1_special:
	shlo	1,s1_mhi,tmp			/* Drop j bit */
	or	s1_mlo,tmp,tmp
	bne	atf_s1_20			/* J/ NaN or INF */

	cmpobne	0,tmp,atf_s1_18			/* J/ S1 is denormal */

	shri	32-15,s2_exp,tmp		/* S1 is 0; examine S2 */
	addo	1,tmp,tmp
	cmpobge	1,tmp,atf_s1_06			/* J/ S2 is NaN/INF/denormal/0 */

	bbs	31,s2_mhi,atf_s1_04		/* J/ 0 + finite -> finite */

atf_s1_unnormal:
atf_s2_unnormal:
atf_s1_02:
	b	_AFP_Fault_Reserved_Encoding_T


atf_s1_04:
	movt	s2,out				/* return S2 */
	ret


atf_s1_06:
	shlo	1,s2_mhi,tmp			/* Drop j bit */
	or	s2_mlo,tmp,tmp
	bne	atf_s1_14			/* J/ S2 is NaN/INF */

	cmpobne	0,tmp,atf_s1_12			/* J/ S2 is denormal */

	cmpobe	s1_se,s2_se,atf_s1_10		/* J/ +0 + +0  or  -0 + -0 */

atf_s1_08:
	shro	AC_Round_Mode,ac,tmp		/* Check round mode for diff sgn 0's */
	cmpo	Round_Mode_Down,tmp

#if	defined(USE_OPN_CC)
	sele	0,1,out_se
#else
	teste	out_se
#endif

	shlo	15,out_se,out_se		/* -0 iff round down */
	ldconst	0,out_mhi
	ldconst	0,out_mlo


atf_s1_10:					/* Default return of S1 */
	ret


atf_s1_12:
	bbc	AC_Norm_Mode,ac,atf_s1_02	/* J/ not norm mode -> fault */
	bbs	AC_Unfl_mask,ac,atf_s1_04	/* J/ unfl masked -> rtn S2  */

atf_s1_13:
	b	_AFP_Fault_Underflow_T


atf_s1_14:
	bbc	31,s2_mhi,atf_s1_02		/* J/ INF/NaN w/o j bit */

	cmpobe	0,tmp,atf_s1_04			/* 0 + INF -> INF */
atf_s1_16:
	b	AFP_NaN_T


atf_s1_18:
	bbc	AC_Norm_Mode,ac,atf_s1_02	/* J/ denormal, not norm mode -> fault */

	scanbit	s1_mhi,tmp
	bno	atf_s1_19			/* J/ MS word = 0 */

	subo	tmp,31,tmp			/* Top bit num to left shift count */
	subo	tmp,1,s1_exp			/* set s1_exp value */

	shlo	tmp,s1_mhi,s1_mant_hi		/* Normalize denorm significand */
	shlo	tmp,s1_mlo,s1_mant_lo
	subo	tmp,31,tmp			/* word -> word bit xfer */
	addo	1,tmp,tmp
	shro	tmp,s1_mlo,tmp
	or	tmp,s1_mant_hi,s1_mant_hi
	b	atf_s1_rejoin

atf_s1_19:
	scanbit	s1_mlo,tmp
	subo	tmp,31,tmp
	shlo	tmp,s1_mlo,s1_mant_hi		/* 32+ bit shift */
	subo	tmp,0,s1_exp			/* set s1_exp value */
	subo	31,s1_exp,s1_exp
	mov	0,s1_mant_lo
	b	atf_s1_rejoin

atf_s1_20:
	bbc	31,s1_mhi,atf_s1_02		/* J/ INF/NaN w/o j bit */

	cmpobne	0,tmp,atf_s1_30			/* J/ S1 is a NaN */

	shri	32-15,s2_exp,tmp		/* S1 is INF; examine S2 */
	addo	1,tmp,tmp
	cmpobge	1,tmp,atf_s1_22			/* J/ S2 is NaN/INF/denormal/0 */

	bbs	31,s2_mhi,atf_s1_10		/* J/ INF + finite -> INF (S1) */
	b	_AFP_Fault_Reserved_Encoding_T	/* S2 is an unnormal */


atf_s1_22:
	shlo	1,s2_mhi,tmp
	or	s2_mlo,tmp,tmp
	bne	atf_s1_24			/* J/ S2 is NaN/INF */

	bbs	AC_Norm_Mode,ac,atf_s1_10	/* J/ norm mode -> return S1 (INF) */
	cmpobe	0,tmp,atf_s1_10			/* J/ INF + 0 -> return S1 (INF) */
	b	_AFP_Fault_Reserved_Encoding_T	/* J/ S2 is denormal */
	

atf_s1_24:
	bbc	31,s2_mhi,atf_s1_02		/* J/ INF/NaN w/o j bit */

	cmpobne	0,tmp,atf_s1_16			/* J/ S2 is a NaN */

	cmpobe	s1_se,s2_se,atf_s1_10		/* J/ same signed INF -> return S1 */

atf_s1_26:
	bbc	AC_InvO_mask,ac,atf_s1_28	/* J/ inv oper fault not masked */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac		/* Set inv oper flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp		/* Set inv oper flag */
	modac	tmp,tmp,tmp
#endif

	ldconst	Standard_QNaN_se,out_se		/* Return a quiet NaN */
	ldconst	Standard_QNaN_mhi,out_mhi
	ldconst	Standard_QNaN_mlo,out_mlo
	ret

atf_s1_28:
	b	_AFP_Fault_Invalid_Operation_T


atf_s1_30:
	bbs	31,s2_mhi,atf_s1_16		/* J/ S2 not reserved encoding -> NaN */
	cmpobne	0,s2_exp,atf_s1_02		/* J/ S2 unnormal */
	or	s2_mhi,s2_mlo,tmp
	cmpobe	0,tmp,atf_s1_16			/* J/ S2 is zero -> process NaN */
	bbc	AC_Norm_Mode,ac,atf_s1_02	/* J/ S2 is denormal, not norm mode */
	b	atf_s1_16



/*  S2 is a special case value: +/-0, denormal, +/-INF, NaN; S2 is a non-  */
/*  zero, finite value.                                                    */

atf_s2_special:
	shlo	1,s2_mhi,tmp
	or	s2_mlo,tmp,tmp
	bne	atf_s2_04			/* J/ S2 is NaN/INF */

	cmpobe	0,tmp,atf_s2_06			/* J/ S2 is 0 */

	bbc	AC_Norm_Mode,ac,atf_s1_02	/* J/ denormal, not norm mode -> fault */

	scanbit	s2_mhi,tmp
	bno	atf_s2_02			/* J/ MS word = 0 */

	subo	tmp,31,tmp			/* Top bit num to left shift count */
	subo	tmp,1,s2_exp			/* set s2_exp value */

	shlo	tmp,s2_mhi,s2_mant_hi		/* Normalize denorm significand */
	shlo	tmp,s2_mlo,s2_mant_lo
	subo	tmp,31,tmp			/* word -> word bit xfer */
	addo	1,tmp,tmp
	shro	tmp,s2_mlo,tmp
	or	tmp,s2_mant_hi,s2_mant_hi
	b	atf_s2_rejoin

atf_s2_02:
	scanbit	s2_mlo,tmp
	subo	tmp,31,tmp
	shlo	tmp,s2_mlo,s2_mant_hi		/* 32+ bit shift */
	subo	tmp,0,s2_exp			/* set s2_exp value */
	subo	31,s2_exp,s2_exp
	mov	0,s2_mant_lo
	b	atf_s2_rejoin


atf_s2_04:
	bbc	31,s2_mhi,atf_s1_02		/* J/ INF/NaN w/o j bit */

	cmpobe	0,tmp,atf_s1_04			/* J/ finite + INF -> return INF (S2) */
	b	AFP_NaN_T			/* J/ process NaN operand */

atf_s2_06:
	bbs	AC_Unfl_mask,ac,atf_s1_10	/* J/ unfl masked -> don't check */
	cmpibl	0,s1_exp,atf_s1_10		/* J/ S1 not denorm -> rtn it */
	b	atf_s1_13			/* Underflow trap */
