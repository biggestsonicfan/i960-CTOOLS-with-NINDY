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
/*      extsfdf.c - Single Precision to Double Precision Conversion Routine   */
/*		    (AFP-960)						      */
/*									      */
/******************************************************************************/

#include "asmopt.h"

#if	!defined(KEEP_INTERNAL_LABELS)
#define	esd_s1_special	L_esd_s1_special
#define	esd_s1_05	L_esd_s1_05
#define	esd_s1_06	L_esd_s1_06
#define	esd_s1_08	L_esd_s1_08
#define	esd_s1_10	L_esd_s1_10
#define	esd_s1_11	L_esd_s1_11
#define	esd_s1_12	L_esd_s1_12
#endif

	.file	"extsfdf.s"
	.globl	___extendsfdf2


#if	defined(USE_SIMULATED_AC)
	.globl	fpem_CA_AC
#endif

	.globl	_AFP_Fault_Reserved_Encoding_S
	.globl	_AFP_Fault_Invalid_Operation_S


#define	DP_Bias    0x3FF
#define	DP_INF     0x7FF
#define	FP_Bias    0x7F

#define	AC_Norm_Mode    29
#define	AC_InvO_mask    26
#define	AC_InvO_flag    18

#define	s1         g0

#define	s2_exp     r6

#define	tmp        r10
#define	con_1      r11
#define	con_2      r12
#define	tmp2       r13
#define	tmp3       r14

#define	ac         r15

#define	out        g0
#define	out_lo     out
#define	out_hi     g1

#define	FP_op_type  g2
#define	cnv_sd_type 11


	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

___extendsfdf2:
	addc	s1,s1,tmp			/* Move sign bit into carry	*/

	shri	32-8,tmp,s2_exp			/* Prepare to test for special	*/
	lda	(DP_Bias-FP_Bias) << 20,con_1

	alterbit 31,con_1,con_1			/* Retain sign bit in exp adj	*/
	addlda(1,s2_exp,s2_exp)

#if	!defined(USE_CMP_BCC)
	cmpo	1,s2_exp			/* Check against magic value	*/
#endif

	shro	1+3,tmp,out_hi			/* Begin creating a result	*/

	addo	con_1,out_hi,out_hi		/* Adjust exp, mix in sign bit	*/

#if	defined(USE_CMP_BCC)
	cmpobge	1,s2_exp,esd_s1_special		/* J/ S1 is 0/denormal/INF/NaN	*/
#else
	bge.f	esd_s1_special			/* J/ S1 is 0/denormal/INF/NaN	*/
#endif

	shlo	32-3,s1,out_lo			/* Create lo word		*/
	ret


esd_s1_special:
	bne	esd_s1_05			/* J/ INF or NaN		*/

#if	!defined(USE_CMP_BCC)
	cmpo	1,tmp				/* (1 because of addc carry in)	*/
#endif

	movlda(cnv_sd_type,FP_op_type)

#if	defined(USE_CMP_BCC)
	cmpobl	1,tmp,esd_s1_10			/* J/ S1 is denormal		*/
#else
	bl.f	esd_s1_10			/* J/ S1 is denormal		*/
#endif

	mov	s1,out_hi			/* Signed zero -> xfer hi word	*/
	movlda(0,out_lo)
	ret



esd_s1_05:
	shlo	1+8,s1,tmp

#if	defined(USE_CMP_BCC)
	cmpobne	0,tmp,esd_s1_06			/* J/ S1 is NaN			*/
#else
	cmpo	0,tmp
	bne.f	esd_s1_06			/* J/ S1 is NaN			*/
#endif

	ldconst	DP_INF << 20,con_1
	or	s1,con_1,out_hi			/* Return corresponding infinity */
	movlda(0,out_lo)
	ret

esd_s1_06:
	bbs	22,s1,esd_s1_08			/* J/ QNaN */
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	chkbit	AC_InvO_mask,ac			/* Check for InvO fault masked	*/
	movlda(cnv_sd_type,FP_op_type)
	bf.f	_AFP_Fault_Invalid_Operation_S	/* Handle NaN operand	*/

#if	defined(USE_SIMULATED_AC)
	setbit	AC_InvO_flag,ac,ac		/* Set inv oper flag */
	st	ac,fpem_CA_AC
#else
	ldconst	1 << AC_InvO_flag,tmp		/* Set inv oper flag */
	modac	tmp,tmp,tmp
#endif

esd_s1_08:					/* DP QNaN from FP NaN source */
	shro	3,s1,out_hi
	lda	(1 << 31)+(DP_INF << 20)+(1 << 19),con_1

	or	con_1,out_hi,out_hi		/* Set sign, NaN exp, QNaN bit	*/

	shlo	32-3,s1,out_lo			/* last cause s1 overlaps out_lo */
	ret



esd_s1_10:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,ac
#else
	modac	0,0,ac
#endif

	chkbit	AC_Norm_Mode,ac
	bf.f	_AFP_Fault_Reserved_Encoding_S	/* J/ fault	*/

	shro	1,tmp,tmp2			/* Unsigned S1 in tmp2 */
	scanbit	tmp2,tmp
	subo	22,tmp,s2_exp			/* FP biased exponent value */
	subo	tmp,20,tmp			/* Top bit num to left shift count */
	cmpibg.f 0,tmp,esd_s1_12		/* J/ right shift to convert */

	shlo	tmp,tmp2,out_hi			/* Normalize unsigned denorm signif */
	movlda(0,out_lo)
	clrbit	20,out_hi,out_hi		/* Zero "j" bit */

esd_s1_11:					/* Denorm junction */
	addo	con_1,out_hi,out_hi		/* Sign bit with exp adjustment	*/
	shlo	32-12,s2_exp,s2_exp		/* Position exponent */
	addo	out_hi,s2_exp,out_hi		/* Finish packing result */
	ret

esd_s1_12:
	subo	tmp,0,tmp			/* Neg left shift to pos right shift */
	ldconst	32,con_2
	subo	tmp,con_2,tmp3			/* Conj left shift for _lo word */
	shro	tmp,tmp2,out_hi			/* Position MS 21 bits */
	shlo	tmp3,tmp2,out_lo		/* Position LS 1 or 2 bits */
	clrbit	20,out_hi,out_hi		/* Zero "j" bit */
	b	esd_s1_11
