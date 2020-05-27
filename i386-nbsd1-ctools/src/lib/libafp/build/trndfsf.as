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
/*      trndfsf.c - Double Precision to Single Precision Conversion Routine   */
/*		   (AFP-960)						      */
/*									      */
/******************************************************************************/

#include "asmopt.h"

#if	!defined(KEEP_INTERNAL_LABELS)
#define	trn_ds_special	L_trn_ds_special
#define	trn_ds_02	L_trn_ds_02
#define	trn_ds_06	L_trn_ds_06
#define	trn_ds_08	L_trn_ds_08
#define	trn_ds_09	L_trn_ds_09
#define	trn_ds_10	L_trn_ds_10
#define	trn_ds_12	L_trn_ds_12
#define	trn_ds_13	L_trn_ds_13
#define	trn_ds_14	L_trn_ds_14
#define	trn_ds_20	L_trn_ds_20
#endif


	.file	"trndfsf.as"
	.globl	___truncdfsf2

#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	AFP_RRC_S
	.globl	_AFP_Fault_Reserved_Encoding_D
	.globl	_AFP_Fault_Invalid_Operation_D


#define	DP_Bias    0x3FF
#define	DP_INF     0x7FF
#define	FP_Bias    0x7F
#define	FP_INF     0xFF

#define	AC_Norm_Mode    29
#define	AC_InvO_mask    26
#define	AC_InvO_flag    18
#define	AC_Round_Mode    30
#define	Round_Mode_even  0x0
#define	Round_Mode_trun  0x3

#define	s1         g0
#define	s1_lo      s1
#define	s1_hi      g1

#define	s2_mant_x  r4
#define	s2_mant    r5
#define	s2_exp     r3
#define	s2_sign    r6

#define	tmp        r10
#define	con_1      r11
#define	con_2      r12
#define	tmp2       r13

#define	ac         r15

#define	out        g0

#define	DP_op_type  g4
#define	FP_op_type  g2
#define	cnv_ds_type 10


	.text
	.link_pix


	.align	MAJOR_CODE_ALIGNMENT

___truncdfsf2:
	addc	s1_hi,s1_hi,s2_exp		/* Sign bit to carry	*/

#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	shri	32-11,s2_exp,tmp
	movlda(cnv_ds_type,FP_op_type)

	alterbit 31,0,s2_sign			/* extract sign			*/
	addlda(1,tmp,tmp)

#if	!defined(USE_CMP_BCC)
	cmpo	1,tmp				/* is S1 NaN/INF/denorm/ ?	*/
#endif

	lda	0xff800000,con_1		/* Mask to extract mant		*/

	shlo	23-20,s1_lo,s2_mant_x		/* Position S1 mant as FP ..	*/

#if	defined(USE_CMP_BCC)
	cmpobge	1,tmp,trn_ds_special		/* J/ S1 is NaN/INF/denorm/0	*/
#else
	bge.f	trn_ds_special			/* J/ S1 is NaN/INF/denorm/0	*/
#endif

#if	defined(USE_ESHRO)
	eshro	32-(23-20),s1,s2_mant		/* Position S1 mant as FP ..	*/
#else
	shlo	23-20,s1_hi,s2_mant		/* Position S1 mant as FP .. */
	shro	32-23+20,s1_lo,tmp
	or	s2_mant,tmp,s2_mant
#endif

	andnot	con_1,s2_mant,s2_mant		/* extract mant bits */
	subo	con_1,s2_mant,s2_mant		/* set "j" bit */
	lda	DP_Bias-FP_Bias,con_2
	shro	32-11,s2_exp,s2_exp		/* Extract biased DP exp */
	subo	con_2,s2_exp,s2_exp		/* Convert to biased FP exp */
	b	AFP_RRC_S


trn_ds_special:
	cmpo	s1_lo,0				/* Condense lo word into LS bit */
	shlo	1,s1_hi,tmp			/* Drop sign bit */
	subc	1,0,tmp2
	notor	tmp,tmp2,tmp
	ldconst	DP_INF << 21,con_1
	cmpobg	tmp,con_1,trn_ds_06		/* J/ S1 is NaN */
	be	trn_ds_20			/* J/ S1 is INF */
	cmpobne	0,tmp,trn_ds_10			/* J/ S1 is denormal */

	mov	s1_hi,out			/* Signed zero -> xfer hi word */
	ret


trn_ds_02:
	mov	FP_op_type,DP_op_type
	b	_AFP_Fault_Reserved_Encoding_D


trn_ds_06:
	bbs	19,s1_hi,trn_ds_08		/* J/ QNaN */
	bbc	AC_InvO_mask,ac,trn_ds_09	/* J/ Unmasked fault w/ SNaN */

#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac		/* Set inv oper flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp		/* Set inv oper flag */
	modac	tmp,tmp,tmp
#endif

trn_ds_08:
	shlo	1+3,s1_hi,tmp			/* FP QNaN from DP NaN source */
	shro	1,tmp,tmp			/* lo 8 bits of DP NaN exp = ... */
	shro	32-3,s1_lo,tmp2			/* ... FP NaN exp */
	or	tmp,tmp2,tmp			/* Retain 22 bits from incoming NaN */
	setbit	22,tmp,tmp			/* (force QNaN) */
	or	s2_sign,tmp,out			/* retain original sign */
	ret


trn_ds_09:
	mov	FP_op_type,DP_op_type
	b	_AFP_Fault_Invalid_Operation_D	/* Handle NaN operand(s) */


trn_ds_10:
	bbc	AC_Norm_Mode,ac,trn_ds_02	/* J/ denormal, not norm mode -> fault */

	shro	1,tmp,tmp
	scanbit	tmp,tmp
	bno	trn_ds_14			/* J/ MS word = 0 */

trn_ds_12:
	subo	tmp,23,tmp			/* Top bit num to left shift count */
	and	0x1f,tmp,tmp			/* (when MS word = 0) */
	shlo	tmp,s1_hi,s2_mant		/* Normalize denorm significand */
	shlo	tmp,s1_lo,s2_mant_x
	subo	tmp,31,tmp			/* word -> word bit xfer */
	addo	1,tmp,tmp
	shro	tmp,s1_lo,tmp
	or	tmp,s2_mant,s2_mant
trn_ds_13:
	ldconst	0-191,s2_exp			/* Minimum exponent after unfl proc */
	b	AFP_RRC_S			/* Underflow processing */


trn_ds_14:
	scanbit	s1_lo,tmp
	cmpoble	25,tmp,trn_ds_12		/* J/ not a full word shift */

	subo	tmp,24,tmp
	shlo	tmp,s1_lo,s2_mant		/* 32+ bit shift */
	mov	0,s2_mant_x
	b	trn_ds_13


trn_ds_20:
	ldconst	FP_INF << 23,con_1
	or	s2_sign,con_1,out		/* Return corresponding infinity */
	ret
