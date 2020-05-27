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
/*      afpcnvt.c - Binary <-> ASCII Decimal Format String (AFP-960)          */
/*									      */
/******************************************************************************/

/*
 *  These routines perform a (near) IEEE binary -> ASCII conversion for
 *  double precision and extended precision values and ASCII -> IEEE binary
 *  for single, double, and extended precision values.  (The qualification
 *  near is used because (a) the current rounding mode is ignored during
 *  scaling operations and (b) the arithmetic status flags are not set
 *  even though C's errno may be set.)
 *
 *  The IEEE binary -> ASCII routines are supplied with two parameters: a
 *  pointer to the output string and the value to be converted.  The
 *  routines convert the value provided to a fixed scientific notation
 *  format "sd.18dEs4d" followed by a nul byte.  In addition, these
 *  routines provide a function return of the tens exponent value as a
 *  two's complement integer.
 *
 *  The ASCII -> IEEE binary routines are based on the standard C library's
 *  "strtod" (string to double) conversion routine.  The corresponding _Lstrtof
 *  (string to float) and _Lstrtoe (string to extended [long double]) are also
 *  provided.  These routines set errno to ERANGE when the string's value
 *  equivalent is outside the representable range.  When no mantissa digits
 *  are found, error is set to EDOM.
 */


#include "asmopt.h"

#define	ASM_SOURCE	1
#include "afpcnvt.h"


	.file	"afpcnvt.s"
	.globl	__AFP_dp2a

/*  extern  int  _AFP_dp2a(char * output_buffer, double conv_value);		*/


	.globl	__AFP_tp2a

/*  extern  int  _AFP_tp2a(char * output_buffer, long double conv_value);	*/


	.globl	_strtod
/*  extern  double       strtod(char * in_string,  char * *  end_pntr);		*/

	.globl	__Lstrtoe
/*  extern  long double  _Lstrtoe(char * in_string,  char * *  end_pntr);		*/

	.globl	__Lstrtof
/*  extern  float        _Lstrtof(char * in_string,  char * *  end_pntr);		*/



	.globl	__errno_ptr		/* returns addr of ERRNO in g0		*/

#define	EDOM	33
#define	ERANGE	34



#define	FP_Bias         0x7f
#define	FP_denorm_ofs	22
#define	FP_INF		0xff

#define	DP_Bias         0x3ff
#define	DP_denorm_ofs	51
#define	DP_INF		0x7ff

#define	TP_Bias		0x3fff
#define	TP_denorm_ofs	63
#define	TP_INF		0x7fff


#define	com_log_2	0x4d104d42	/* log10(2) * 2^32			*/

#define	DP_log_bias_hi	0x00000143	/* log10(2) * (DP_Bias+DP_denorm_ofs)	*/
#define	DP_log_bias_lo	0x4e6420f4

#define	TP_log_bias_hi	0x00001356	/* log10(2) * (TP_Bias+TP_denorm_ofs)	*/
#define	TP_log_bias_lo	0xbd435594


#define	max_val_exp		TP_Bias+63	/* Value which would be rounded	*/
#define	max_val_mant_hi		0x8ac72304	/* up to 10^AFP_MANT_DIGS (and	*/
#define	max_val_mant_mid	0x89e7ffff	/* thus overflow during digit	*/
#define	max_val_mant_lo		0x80000000	/* conversion)			*/



/* Register Name Equates */

#define	str_pntr	g0			/* Incoming string pntr		*/
#define	rtn_pntr_pntr	g1			/* String pntr pntr (strto-)	*/

#define	dp_arg		g2			/* Incoming double prec value	*/
#define	dp_arg_lo	dp_arg
#define	dp_arg_hi	g3

#define	tp_arg_lo	g4			/* Incoming ext prec value	*/
#define	tp_arg_mid	g5
#define	tp_arg_se	g6

#define	twos_exp	r3			/* Twos exp of incoming arg	*/
#define	mant		r4			/* 64-bit mantissa of arg	*/
#define	mant_lo		mant
#define	mant_hi		r5

#define	rp_1		r6			/* temp + ten power return	*/
#define	rp_1_lo		rp_1
#define	rp_1_hi		r7

#define	rp_2		r8
#define	rp_2_lo		rp_2
#define	rp_2_hi		r9

#define	rp_3		r10
#define	rp_3_lo		rp_3
#define	rp_3_hi		r11

#define	rp_4		r12
#define	rp_4_lo		rp_4
#define	rp_4_hi		r13

#define	rp_5		r14
#define	rp_5_lo		rp_5
#define	rp_5_hi		r15


#define	tens_exp	g2			/* Tens exp of argument		*/

#define	ten_pwr		g3			/* Power of ten for scaling	*/
#define	scaled_exp	g4			/* Arg exponent after scaling	*/


#define	tmp_1		g5

#define	tmp_2		g6
#define	tmp_3		g7
#define	rp_6		g6
#define	rp_6_lo		rp_6
#define	rp_6_hi		g7

#define	ten_pwr_sign	g13


/*
 *  Registers used by strto- routines
 */

#define	flags		rp_5_hi			/* strto- flag bits		*/
#define	DP_mode			0
#define	TP_mode			1
#define	Neg_mant		2
#define	DP_found		3
#define Neg_exp			4
#define	Signif_mant_digit	5
#define	Any_mant_digits		6

#define	hold_flags	g0

#define	digit_limit	rp_5_lo			/* strto- digit limit		*/
#define	MAX_SIGNIF_MANT_DIGS	21
#define	MAX_EXP_DIGS		 4

#define	cur_char	rp_4_hi			/* strto- current character	*/
#define	dig_value	rp_4_lo			/* strto- digit value		*/
#define	lit_0		rp_2_hi			/* strto- ASCII "0"		*/


#define	out_tens_exp	g0			/* Return of exponent value	*/

#define	out_fp		g0

#define	out_dp_lo	g0
#define	out_dp_hi	g1

#define	out_tp_mant_lo	g0
#define	out_tp_mant_hi	g1
#define	out_tp_se	g2



	.text
	.link_pix

	.align	3

/*
 *  Powers of ten table
 *
 *  Note that unlike IEEE format values, the double word mantissa is
 *  a fractional value (between 1/2 and 1-).
 *
 */

Lten_table_mant:
	.word	0x00000000,0xA0000000		/* * 2 ^ 4       10 ^ 1		*/
	.word	0x00000000,0xC8000000		/* * 2 ^ 7       10 ^ 2		*/
	.word	0x00000000,0xFA000000		/* * 2 ^ 10      10 ^ 3		*/
	.word	0x00000000,0x9C400000		/* * 2 ^ 14      10 ^ 4		*/
	.word	0x00000000,0xC3500000		/* * 2 ^ 17      10 ^ 5		*/
	.word	0x00000000,0xF4240000		/* * 2 ^ 20      10 ^ 6		*/
	.word	0x00000000,0x98968000		/* * 2 ^ 24      10 ^ 7		*/
	.word	0x00000000,0xBEBC2000		/* * 2 ^ 27      10 ^ 8		*/
	.word	0x00000000,0xEE6B2800		/* * 2 ^ 30      10 ^ 9		*/
	.word	0x00000000,0x9502F900		/* * 2 ^ 34      10 ^ 10	*/
	.word	0x00000000,0xBA43B740		/* * 2 ^ 37      10 ^ 11	*/
	.word	0x00000000,0xE8D4A510		/* * 2 ^ 40      10 ^ 12	*/
	.word	0x00000000,0x9184E72A		/* * 2 ^ 44      10 ^ 13	*/
	.word	0x80000000,0xB5E620F4		/* * 2 ^ 47      10 ^ 14	*/
	.word	0xA0000000,0xE35FA931		/* * 2 ^ 50      10 ^ 15	*/
	.word	0x04000000,0x8E1BC9BF		/* * 2 ^ 54      10 ^ 16	*/
	.word	0xC5000000,0xB1A2BC2E		/* * 2 ^ 57      10 ^ 17	*/
	.word	0x76400000,0xDE0B6B3A		/* * 2 ^ 60      10 ^ 18	*/
	.word	0x89E80000,0x8AC72304		/* * 2 ^ 64      10 ^ 19	*/
	.word	0xAC620000,0xAD78EBC5		/* * 2 ^ 67      10 ^ 20	*/
	.word	0x177A8000,0xD8D726B7		/* * 2 ^ 70      10 ^ 21	*/
	.word	0x6EAC9000,0x87867832		/* * 2 ^ 74      10 ^ 22	*/
	.word	0x0A57B400,0xA968163F		/* * 2 ^ 77      10 ^ 23	*/
	.word	0xCCEDA100,0xD3C21BCE		/* * 2 ^ 80      10 ^ 24	*/
	.word	0x401484A0,0x84595161		/* * 2 ^ 84      10 ^ 25	*/
	.word	0x9019A5C8,0xA56FA5B9		/* * 2 ^ 87      10 ^ 26	*/
Lten_power_27_mant:
	.word	0xF4200F3A,0xCECB8F27		/* * 2 ^ 90      10 ^ 27	*/


Lten_power_55_mant:
	.word	0xCFE20766,0xD0CF4B50		/* * 2 ^ 183     10 ^ 55	*/
#define	ten_power_55_exp	183

Lten_power_110_mant:
	.word	0x34A7EEDF,0xAA51823E		/* * 2 ^ 366     10 ^ 110	*/
#define	ten_power_110_exp	366

Lten_power_220_mant:
	.word	0x971F303A,0xE2A0B5DC		/* * 2 ^ 731     10 ^ 220	*/
#define	ten_power_220_exp	731

Lten_power_440_mant:
	.word	0x4FC1A3E9,0xC8A025FD		/* * 2 ^ 1462    10 ^ 440	*/
#define	ten_power_440_exp	1462

Lten_power_880_mant:
	.word	0x4EE575DE,0x9D3A9F8B		/* * 2 ^ 2924    10 ^ 880	*/
#define	ten_power_880_exp	2924

Lten_power_1760_mant:
	.word	0x1AA714B7,0xC121EA3B		/* * 2 ^ 5847    10 ^ 1760	*/
#define	ten_power_1760_exp	5847

Lten_power_3520_mant:
	.word	0x57BCE6AE,0x91B427AB		/* * 2 ^ 11694   10 ^ 3520	*/
#define	ten_power_3520_exp	11694


Lten_table_exp:
	.byte	4
	.byte	7
	.byte	10
	.byte	14
	.byte	17
	.byte	20
	.byte	24
	.byte	27
	.byte	30
	.byte	34
	.byte	37
	.byte	40
	.byte	44
	.byte	47
	.byte	50
	.byte	54
	.byte	57
	.byte	60
	.byte	64
	.byte	67
	.byte	70
	.byte	74
	.byte	77
	.byte	80
	.byte	84
	.byte	87
	.byte	90
#define	ten_power_27_exp	90

	.align	2


/*
 * No special alignment since speed isn't an issue (cache is sure to be worked
 * over by the xprintf processing sequence).
 */

__AFP_dp2a:
#if	defined(CA_optim)
	eshro	32-11,dp_arg_lo,mant_hi		/* Create left-justified mant	*/
#else
	shlo	11,dp_arg_hi,mant_hi		/* Create left-justified mant	*/
	shro	32-11,dp_arg_lo,tmp_1		/* _lo -> _hi bits		*/
	or	mant_hi,tmp_1,mant_hi
#endif
	shlo	11,dp_arg_lo,mant_lo

	shlo	1,dp_arg_hi,twos_exp		/* Left justify exp field	*/

	shro	32-11,twos_exp,twos_exp		/* Biased, right justif exp	*/

	chkbit	31,dp_arg_hi
	lda	'+',tmp_1
	bno	Ldp2a_05				/* J/ positive			*/
	lda	'-',tmp_1
Ldp2a_05:
	stob	tmp_1,(str_pntr)		/* Issue sign character		*/

	cmpobe.f 0,twos_exp,Ldp2a_80		/* J/ denorm/0.0		*/

	setbit	31,mant_hi,mant_hi		/* Set j bit			*/

Ldp2a_10:
	clrbit	31,mant_hi,tmp_1		/* Prepare to est log10(mant)	*/
	lda	DP_denorm_ofs(twos_exp),tmp_3	/* Bias to non-neg value	*/

	lda	com_log_2,tmp_2

	emul	tmp_3,tmp_2,rp_1		/* Tens exp from number's exp	*/

	lda	DP_log_bias_hi,rp_2_hi
	cmpo	0,0				/* Clear borrow			*/
	lda	DP_log_bias_lo,rp_2_lo

	emul	tmp_1,tmp_2,rp_3		/* Est tens exps of mant	*/

	subc	rp_2_lo,rp_1_lo,rp_1_lo		/* Remove exponent bias		*/
	subc	rp_2_hi,rp_1_hi,rp_1_hi

	lda	TP_Bias-DP_Bias(twos_exp),twos_exp	/* Convert to TP bias	*/

	addc	rp_3_hi,rp_1_lo,rp_1_lo
	addc	0,rp_1_hi,tens_exp		/* Tens exp estimate		*/

	b	Lxp2a_10				/* Join standard processing	*/


Ldp2a_80:
	scanbit	mant_hi,tmp_1
	bno	Ldp2a_86				/* J/ hi mantissa word is zero	*/

	subo	30,tmp_1,twos_exp		/* Compute effective exponent	*/

	subo	tmp_1,31,tmp_2			/* Left shift			*/
	addlda(1,tmp_1,tmp_3)			/* Right shift (wd -> wd)	*/

#if	defined(CA_optim)
	eshro	tmp_3,mant_lo,mant_hi		/* Left just denormal mant	*/
#else
	shlo	tmp_2,mant_hi,mant_hi		/* Left just denormal mant	*/
	shro	tmp_3,mant_lo,tmp_3
	or	mant_hi,tmp_3,mant_hi
#endif
	shlo	tmp_2,mant_lo,mant_lo
	b	Ldp2a_10

Ldp2a_86:
	scanbit	mant_lo,tmp_1
	bno	Lxpa_zero_10			/* J/ argument is zero		*/

	ldconst	32,tmp_3

	subo	30,tmp_1,twos_exp		/* Compute effective exponent	*/
	subo	tmp_3,twos_exp,twos_exp

	subo	tmp_1,31,tmp_1			/* Left shift			*/

	shlo	tmp_1,mant_lo,mant_hi
	movlda(0,mant_lo)
	b	Ldp2a_10


/*
 *  Zero arg -> issue the requested number of zero characters
 */

Lxpa_zero_10:					/* 0.0 argument			*/
	lda	'0',tmp_1			/* Issue "0."			*/
	stob	tmp_1,1(str_pntr)
	lda	'.',tmp_2
	stob	tmp_2,2(str_pntr)

	addo	3,str_pntr,str_pntr		/* Loop to issue mant digits	*/
	movlda(AFP_MANT_DIGS-1,tmp_3)
Lxpa_zero_20:
	cmpdeco	1,tmp_3,tmp_3
	stob	tmp_1,(str_pntr)
	addlda(1,str_pntr,str_pntr)
	bne	Lxpa_zero_20			/* J/ more to issue		*/

	lda	'E',tmp_2			/* Issue E+00000<nul>		*/
	stob	tmp_2,(str_pntr)
	lda	'+',tmp_2
	stob	tmp_2,1(str_pntr)

	addo	2,str_pntr,str_pntr		/* Loop to issue exp zeroes	*/
	movlda(AFP_EXP_DIGS,tmp_3)
Lxpa_zero_30:
	cmpdeco	1,tmp_3,tmp_3
	stob	tmp_1,(str_pntr)
	addlda(1,str_pntr,str_pntr)
	bne	Lxpa_zero_30

	mov	0,tmp_1				/* NUL byte termination		*/
	stob	tmp_1,(str_pntr)

	mov	0,out_tens_exp			/* Tens exp of 0.0 = 0		*/
	ret



__AFP_tp2a:
	movl	tp_arg_lo,mant_lo		/* Copy the mantissa bits	*/

	shlo	1+16,tp_arg_se,twos_exp		/* Left justify exp field	*/

	shro	32-15,twos_exp,twos_exp		/* Biased, right justif exp	*/

	chkbit	15,tp_arg_se
	lda	'+',tmp_1
	bno	Ltp2a_05				/* J/ positive			*/
	lda	'-',tmp_1
Ltp2a_05:
	stob	tmp_1,(str_pntr)		/* Issue sign character		*/

	cmpobe.f 0,twos_exp,Ltp2a_80		/* J/ denorm/0.0		*/

Ltp2a_10:
	clrbit	31,mant_hi,tmp_1		/* Prepare to est log10(mant)	*/
	lda	TP_denorm_ofs(twos_exp),tmp_3	/* Bias to non-neg value	*/

	lda	com_log_2,tmp_2

	emul	tmp_3,tmp_2,rp_1		/* Tens exp from number's exp	*/

	lda	TP_log_bias_hi,rp_2_hi
	cmpo	0,0				/* Clear borrow			*/
	lda	TP_log_bias_lo,rp_2_lo

	emul	tmp_1,tmp_2,rp_3		/* Est tens exps of mant	*/

	subc	rp_2_lo,rp_1_lo,rp_1_lo		/* Remove exponent bias		*/
	subc	rp_2_hi,rp_1_hi,rp_1_hi

	addc	rp_3_hi,rp_1_lo,rp_1_lo
	addc	0,rp_1_hi,tens_exp		/* Tens exp estimate		*/

	b	Lxp2a_10				/* Join standard processing	*/


Ltp2a_80:
	scanbit	mant_hi,tmp_1
	bno	Ltp2a_86				/* J/ hi mantissa word is zero	*/

	subo	30,tmp_1,twos_exp		/* Compute effective exponent	*/

	subo	tmp_1,31,tmp_2			/* Left shift			*/
	addlda(1,tmp_1,tmp_3)			/* Right shift (wd -> wd)	*/

#if	defined(CA_optim)
	eshro	tmp_3,mant_lo,mant_hi		/* Left just denormal mant	*/
#else
	shlo	tmp_2,mant_hi,mant_hi		/* Left just denormal mant	*/
	shro	tmp_3,mant_lo,tmp_3
	or	mant_hi,tmp_3,mant_hi
#endif
	shlo	tmp_2,mant_lo,mant_lo
	b	Ltp2a_10

Ltp2a_86:
	scanbit	mant_lo,tmp_1
	bno	Lxpa_zero_10			/* J/ argument is zero		*/

	ldconst	32,tmp_3

	subo	30,tmp_1,twos_exp		/* Compute effective exponent	*/
	subo	tmp_3,twos_exp,twos_exp

	subo	tmp_1,31,tmp_1			/* Left shift			*/

	shlo	tmp_1,mant_lo,mant_hi
	movlda(0,mant_lo)
	b	Ltp2a_10


Lxp2a_10:
	subo	tens_exp,AFP_MANT_DIGS-1,ten_pwr	/* Scale to 10^18 -> 10^19	*/


	bal	Lten_scale			/* Scale mant by ten_pwr -> rp1	*/

	mov	0,g14				/* Zero BAL's ret addr		*/

	lda	max_val_exp,tmp_1
	cmpobl	scaled_exp,tmp_1,Lxp2a_30	/* J/ < 10^19			*/
	bg	Lxp2a_14				/* J/ > 10^19			*/

	lda	max_val_mant_hi,tmp_1
	cmpobl	rp_1_hi,tmp_1,Lxp2a_30		/* J/ < 10^19			*/
	bg	Lxp2a_14				/* J/ > 10^19			*/

	lda	max_val_mant_mid,tmp_1
	cmpobl	rp_1_lo,tmp_1,Lxp2a_30		/* J/ < 10^19			*/
	bg	Lxp2a_14				/* J/ > 10^19			*/

	bbc	31,rp_3_lo,Lxp2a_30		/* J/ < 10^19			*/

Lxp2a_14:
	addo	1,tens_exp,tens_exp		/* Adjust estimated tens exp	*/
	b	Lxp2a_10				/* Go try again			*/


Lxp2a_30:

/*
 *  Shift to form an integer value, round-to-nearest (ignore current rounding
 *  mode at present), convert integer to scientific format, append tens
 *  exponent, complete with a zero byte, the return
 */

	lda	TP_Bias+63,tmp_1
	subo	scaled_exp,tmp_1,tmp_1		/* Right shift (0 to 4)		*/
	lda	32,tmp_2
	subo	tmp_1,tmp_2,tmp_2		/* Left shift for wd -> wd 	*/

	shro	tmp_1,rp_3_lo,rp_3_lo		/* (just drop low order bits)	*/
	shlo	tmp_2,rp_1_lo,tmp_3
	or	rp_3_lo,tmp_3,rp_3_lo		/* Rounding word		*/

#if	defined(CA_optim)
	eshro	tmp_1,rp_1,rp_1_lo
#else
	shro	tmp_1,rp_1_lo,rp_1_lo
	shlo	tmp_2,rp_1_hi,tmp_3
	or	rp_1_lo,tmp_3,rp_1_lo
#endif

	shro	tmp_1,rp_1_hi,rp_1_hi

/*
 *  Rounding mode fixed at round to nearest (not round-to-nearest even!)
 */

	chkbit	31,rp_3_lo
	addc	0,rp_1_lo,rp_1_lo
	addc	0,rp_1_hi,rp_1_hi

/*
 *  Convert to string in three stages: 64-bit division stage (9 digits),
 *                                     32-bit division stage (9 digits),
 *                                     + 1 residual digit
 */

#define	long_word_divide	9

	mov	long_word_divide,tmp_3		/* 64-bit division stage cntr	*/
	lda	1+1+AFP_MANT_DIGS-1(str_pntr),str_pntr	/* Point to last digit	*/
Lxpa2d_40:
	mov	rp_1_hi,rp_2_lo			/* 64-bit div of rp_1 by 10	*/
	movlda(0,rp_2_hi)
	ediv	10,rp_2,rp_2
	mov	rp_2_hi,rp_1_hi
	mov	rp_2_lo,rp_2_hi
	movldar(rp_1_lo,rp_2_lo)
	ediv	10,rp_2,rp_2
	cmpdeco	1,tmp_3,tmp_3
	lda	'0'(rp_2_lo),rp_2_lo		/* Convert remainder to digit	*/
	mov	rp_2_hi,rp_1_lo
	stob	rp_2_lo,(str_pntr)		/* Store digit			*/
	subo	1,str_pntr,str_pntr
	bne	Lxpa2d_40

	mov	AFP_MANT_DIGS-long_word_divide-1,tmp_3	/* 32-bit stage cntr	*/
Lxpa2d_42:
	ediv	10,rp_1,rp_1
	cmpdeco	1,tmp_3,tmp_3
	lda	'0'(rp_1_lo),tmp_1		/* Convert remainder to digit	*/
	mov	rp_1_hi,rp_1_lo
	stob	tmp_1,(str_pntr)		/* Store digit			*/
	subo	1,str_pntr,str_pntr
	movlda(0,rp_1_hi)
	bne	Lxpa2d_42

	lda	'.',tmp_1			/* Decimal point then MS digit	*/
	stob	tmp_1,(str_pntr)
	subo	1,str_pntr,str_pntr
	lda	'0'(rp_1_lo),tmp_1
	stob	tmp_1,(str_pntr)

	lda	AFP_MANT_DIGS+1(str_pntr),str_pntr	/* Issue E		*/
	lda	'E',tmp_1
	stob	tmp_1,(str_pntr)

	cmpibg	0,tens_exp,Lxpa2d_44		/* J/ tens_exp < 0		*/

	mov	tens_exp,rp_1_lo
	lda	'+',tmp_1
	b	Lxpa2d_46

Lxpa2d_44:
	subo	tens_exp,0,rp_1_lo
	lda	'-',tmp_1

Lxpa2d_46:
	mov	0,rp_1_hi
	stob	tmp_1,1(str_pntr)

	mov	AFP_EXP_DIGS-1,tmp_3		/* Exponent issue stage cntr	*/
	addlda(AFP_EXP_DIGS+1,str_pntr,str_pntr)
Lxpa2d_48:
	ediv	10,rp_1,rp_1
	cmpdeco	1,tmp_3,tmp_3
	lda	'0'(rp_1_lo),tmp_1		/* Convert remainder to digit	*/
	mov	rp_1_hi,rp_1_lo
	stob	tmp_1,(str_pntr)		/* Store digit			*/
	subo	1,str_pntr,str_pntr
	movlda(0,rp_1_hi)
	bne	Lxpa2d_48

	lda	'0'(rp_1_lo),tmp_1		/* Convert residue to digit	*/
	stob	tmp_1,(str_pntr)		/* Store digit			*/

	stob	g14,AFP_EXP_DIGS(str_pntr)	/* Add zero byte terminator	*/

	mov	tens_exp,out_tens_exp		/* Return tens exponent		*/
	ret

/*
 *  String-to-floating-point Conversion Routines
 *
 */

__Lstrtof:
	mov	0,flags				/* Default to single precision	*/
	b	Lstx_10

__Lstrtoe:
	setbit	TP_mode,0,flags			/* Signal extended precision	*/
	b	Lstx_10

_strtod:
	setbit	DP_mode,0,flags			/* Signal double precision	*/


Lstx_10:
	cmpobe.t 0,rtn_pntr_pntr,Lstx_10a	/* J/ don't return pntr		*/
	st	str_pntr,(rtn_pntr_pntr)	/* Default for invalid numbers	*/
Lstx_10a:

	mov	0,ten_pwr
	ldob	(str_pntr),cur_char		/* Fetch the first character	*/

	movl	0,mant				/* Init mantissa (96-bit)	*/
	lda	' ',tmp_1

Lstx_11a:
	addo	1,str_pntr,str_pntr		/* Bump the input pointer	*/
	cmpobe.f cur_char,tmp_1,Lstx_11b		/* J/ leading blank		*/

	cmpo	cur_char,0x09			/* Check if less than low limit	*/
	concmpo	cur_char,0x0D			/* ... and high lim of isspace	*/
	bne.t	Lstx_11c				/* J/ not an isspace char	*/

Lstx_11b:
	ldob	(str_pntr),cur_char		/* Fetch the next byte		*/
	b	Lstx_11a

Lstx_11c:
	mov	0,tens_exp			/* Init tens exponent field	*/
	lda	'0',lit_0

	mov	0,rp_3_lo			/* 0 low 32-bits of mantissa	*/
	lda	'+',tmp_1

	cmpobe.f cur_char,tmp_1,Lstx_12		/* J/ leading + sign (ignore)	*/

	lda	'-',tmp_1
	cmpobne.t cur_char,tmp_1,Lstx_14		/* J/ no leading sign		*/

	setbit	Neg_mant,flags,flags

Lstx_12:
	ldob	(str_pntr),cur_char
	addo	1,str_pntr,str_pntr

Lstx_14:
	lda	-MAX_SIGNIF_MANT_DIGS-1,digit_limit


Lstx_20:
	subo	lit_0,cur_char,dig_value

	cmpoble.f 10,dig_value,Lstx_30		/* J/ not a digit		*/

	setbit	Any_mant_digits,flags,flags	/* Signal a mant digit found	*/
	ldob	(str_pntr),cur_char		/* Fetch the next character	*/

	cmpo	dig_value,0			/* Check for signif vs placehld	*/
	addlda(1,str_pntr,str_pntr)

	alterbit Signif_mant_digit,0,tmp_1	/* Set signif flag as req'd	*/
	notbit	 Signif_mant_digit,tmp_1,tmp_1
	or	flags,tmp_1,flags

	chkbit	Signif_mant_digit,flags		/* Set carry if signif dig fnd	*/

	addc	0,digit_limit,digit_limit	/* Cond decr of signif dig cnt	*/

	bbc.f	31,digit_limit,Lstx_22		/* Too many digits		*/
	
	movl	mant,rp_1			/* Multiply old val by 10	*/
	movldar(rp_3_lo,rp_2_lo)

#if	defined(CA_optim)
	eshro	32-2,mant,mant_hi		/* times 4			*/
#else
	shlo	2,mant_hi,mant_hi		/* times 4			*/
	shro	32-2,mant_lo,tmp_1
	or	mant_hi,tmp_1,mant_hi
#endif
	shlo	2,mant_lo,mant_lo
	shro	32-2,rp_3_lo,tmp_1
	or	mant_lo,tmp_1,mant_lo
	shlo	2,rp_3_lo,rp_3_lo

	cmpo	0,1				/* Clear carry			*/

	addc	rp_2_lo,rp_3_lo,rp_3_lo		/* Add 1 + 4 -> 5		*/
	addc	rp_1_lo,mant_lo,mant_lo
	addc	rp_1_hi,mant_hi,mant_hi

	addc	rp_3_lo,rp_3_lo,rp_3_lo		/* 2 * 5 -> 10			*/
	addc	mant_lo,mant_lo,mant_lo
	addc	mant_hi,mant_hi,mant_hi

	addc	dig_value,rp_3_lo,rp_3_lo	/* dig_value + 10*mant		*/
	addc	0,mant_lo,mant_lo
	addc	0,mant_hi,mant_hi

	chkbit	DP_found,flags
	addc	0,ten_pwr,ten_pwr		/* Bump count if right of DP	*/

	b	Lstx_20

Lstx_22:
	chkbit	DP_found,flags			/* Set borrow if no DP yet	*/
	subc	0,ten_pwr,ten_pwr		/* x 10 for each extra LOD dig	*/
	b	Lstx_20

Lstx_30:
	lda	'.',tmp_1
	bbs.f	DP_found,flags,Lstx_40		/* J/ DP already found		*/

	cmpobne.f cur_char,tmp_1,Lstx_40		/* J/ not a DP			*/

	ldob	(str_pntr),cur_char		/* Get the next character	*/
	addo	1,str_pntr,str_pntr

	setbit	DP_found,flags,flags
	b	Lstx_20

Lstx_40:
	lda	'E',tmp_1
	cmpobe	cur_char,tmp_1,Lstx_42		/* J/ start of exponent field?	*/
	lda	'e',tmp_1
	cmpobne	cur_char,tmp_1,Lstx_60		/* J/ no exponent field		*/

Lstx_42:
	ldob	(str_pntr),cur_char
	addo	1,str_pntr,str_pntr

	lda	'+',tmp_1
	cmpobe	cur_char,tmp_1,Lstx_44		/* J/ leading + in exp fld	*/
	lda	'-',tmp_1
	cmpobne	cur_char,tmp_1,Lstx_46		/* J/ no sign in exp fld	*/

	setbit	Neg_exp,flags,flags
Lstx_44:
	ldob	(str_pntr),cur_char		/* Advance past the sign char	*/
	addo	1,str_pntr,str_pntr

Lstx_46:
	mov	0,digit_limit			/* Counter for digits		*/
	lda	999,tmp_1			/* Pre-mul limit for exp val	*/

Lstx_48:
	subo	lit_0,cur_char,dig_value	/* compute digit value		*/
	cmpoble.f 10,dig_value,Lstx_50		/* J/ not an exp digit		*/

	addo	1,digit_limit,digit_limit	/* Count exponent digit		*/
	ldob	(str_pntr),cur_char		/* Fetch the next character	*/

	cmpo	tens_exp,tmp_1			/* Check for exp val in range	*/
	addlda(1,str_pntr,str_pntr)		/* Bump input pointer		*/

	mulo	tens_exp,10,tens_exp

	addo	dig_value,tens_exp,tens_exp
	bl	Lstx_48				/* J/ not out of range		*/

	shlo	13,1,tens_exp			/* Top out exp value (8192)	*/
	b	Lstx_48

Lstx_50:
	shlo	32-Neg_exp-1,flags,tmp_1	/* Neg exp bit -> sign bit	*/
	shri	31,tmp_1,tmp_1			/* tmp_1 = sgn(exp)  [0 or -1]	*/

	cmpobe.f 0,digit_limit,Lstx_edom		/* J/ no exp digits -> ret 0.0	*/

	xor	tens_exp,tmp_1,tens_exp
	subo	tmp_1,tens_exp,tens_exp

Lstx_60:

	bbc.f	Any_mant_digits,flags,Lstx_edom	/* J/ no mant digits		*/

	subo	ten_pwr,tens_exp,ten_pwr	/* ROD count w/ exp field	*/

	cmpobe	0,rtn_pntr_pntr,Lstx_62		/* J/ don't return string pntr	*/
	subo	1,str_pntr,str_pntr		/* Point to terminating char	*/
	st	str_pntr,(rtn_pntr_pntr)	/* Write updated str pntr	*/
Lstx_62:

/*
 *  Left justify value in  mant_hi::mant_lo::rp_3_lo  using the left shift
 *  count to create a twos exponent for the mantissa "integer"
 */

	mov	2,twos_exp			/* Max num of full word shifts	*/
Lstx_64:
	cmpo	mant_hi,0
	bne.f	Lstx_70				/* J/ found a non-zero word (!)	*/
	cmpdeco	0,twos_exp,twos_exp
	movldar(mant_lo,mant_hi)		/* Left shift a word		*/
	mov	rp_3_lo,mant_lo
	movlda(0,rp_3_lo)
	bne	Lstx_64				/* J/ more words to check	*/

/*
 *  Signed zero return (because mantissa is zero)
 */
Lstx_66:						/* (underflow entry)		*/
	bbs.f	TP_mode,flags,Lstx_68
	bbs.t	DP_mode,flags,Lstx_67

	chkbit	Neg_mant,flags			/* Single precision zero	*/
	alterbit 31,0,out_fp
	ret

Lstx_67:
	chkbit	Neg_mant,flags			/* Double precision zero	*/
	movlda(0,out_dp_lo)
	alterbit 31,0,out_dp_hi
	ret

Lstx_68:
	chkbit	Neg_mant,flags
	alterbit 15,0,out_tp_se
	movl	0,out_tp_mant_lo
	ret


Lstx_70:						/* Finish left justification	*/
	shlo	5,twos_exp,twos_exp		/* 32 * num words not shifted	*/

	scanbit	mant_hi,tmp_1

	addo	twos_exp,tmp_1,twos_exp		/* Twos exponent of mant int	*/
	subo	tmp_1,31,tmp_2			/* Left shift to justify	*/
	addlda(1,tmp_1,tmp_3)			/* Right shift for wd -> wd	*/

	chkbit	tmp_1,rp_3_lo			/* Rounding bit			*/

	shlo	tmp_2,mant_hi,mant_hi
	shro	tmp_3,mant_lo,tmp_1
	or	mant_hi,tmp_1,mant_hi

	shlo	tmp_2,mant_lo,mant_lo
	shro	tmp_3,rp_3_lo,tmp_1
	or	mant_lo,tmp_1,mant_lo

	addc	0,mant_lo,mant_lo		/* Round after shift		*/
	addc	0,mant_hi,mant_hi

	alterbit 31,0,tmp_1			/* Handle carry out		*/
	or	mant_hi,tmp_1,mant_hi
	addc	0,twos_exp,twos_exp

	movldar(flags,hold_flags)

	bal	Lten_scale

	mov	0,g14				/* Zero BAL's ret addr		*/

	bbs.f	TP_mode,hold_flags,Lstx_90	/* J/ extended precision result	*/
	bbs.t	DP_mode,hold_flags,Lstx_80

	lda	FP_Bias-1(scaled_exp),twos_exp
	cmpibg.f 0,twos_exp,Lstx_72		/* J/ denorm form		*/

	lda	FP_INF-1,tmp_1
	cmpobge.f twos_exp,tmp_1,Lstx_ovfl	/* J/ overflow			*/

	chkbit	7,rp_1_hi			/* Round bit to carry		*/
	shro	8,rp_1_hi,rp_1_hi		/* Position mantissa		*/
	shlo	23,twos_exp,twos_exp		/* Position exponent		*/
	addc	rp_1_hi,twos_exp,rp_1_hi	/* Round + mix w/ exponent	*/
	shro	23,rp_1_hi,tmp_2		/* Check rounded val's exponent	*/
	cmpobg.f tmp_2,tmp_1,Lstx_ovfl		/* J/ rounded up to INF		*/
	chkbit	Neg_mant,hold_flags		/* Transfer sign bit		*/
	alterbit 31,rp_1_hi,out_fp
	ret

Lstx_72:
	subo	twos_exp,7,tmp_1		/* Right shift for denorm/round	*/

	cmpobl.f 31,tmp_1,Lstx_unfl		/* J/ too small -> underflow	*/

	shro	tmp_1,rp_1_hi,rp_1_hi		/* Denormal form		*/
	addo	1,rp_1_hi,rp_1_hi		/* Round it			*/
	shro	1,rp_1_hi,rp_1_hi		/* Final position		*/
	chkbit	Neg_mant,hold_flags		/* Copy sign bit over		*/
	alterbit 31,rp_1_hi,out_fp
	ret


Lstx_80:
	lda	DP_Bias-1(scaled_exp),twos_exp
	cmpibg.f 0,twos_exp,Lstx_82		/* J/ denorm form		*/

	lda	DP_INF-1,tmp_1
	cmpobge.f twos_exp,tmp_1,Lstx_ovfl	/* J/ overflow			*/

	chkbit	10,rp_1_lo			/* Round bit to carry		*/
#if	defined(CA_optim)
	eshro	11,rp_1,rp_1_lo			/* Position mantissa		*/
#else
	shro	11,rp_1_lo,rp_1_lo		/* Position mantissa		*/
	shlo	32-11,rp_1_hi,tmp_2
	or	rp_1_lo,tmp_2,rp_1_lo
#endif
	shro	11,rp_1_hi,rp_1_hi

	shlo	20,twos_exp,twos_exp		/* Position exponent		*/
	addc	0,rp_1_lo,rp_1_lo
	addc	rp_1_hi,twos_exp,rp_1_hi	/* Round + mix w/ exponent	*/
	shro	20,rp_1_hi,tmp_2		/* Check rounded val's exponent	*/
	cmpobg.f tmp_2,tmp_1,Lstx_ovfl		/* J/ rounded up to INF		*/
	chkbit	Neg_mant,hold_flags		/* Transfer sign bit		*/
	alterbit 31,rp_1_hi,out_dp_hi
	movldar(rp_1_lo,out_dp_lo)
	ret

Lstx_82:
	subo	twos_exp,10,tmp_1		/* Right shift for denorm/round	*/

Lstx_83:						/* Entry for extended precision	*/
	lda	32,tmp_2

	cmpobge.f 31,tmp_1,Lstx_84		/* J/ less than a word shift	*/

	mov	rp_1_hi,rp_1_lo			/* Do a word shift		*/
	movlda(0,rp_1_hi)

	subo	tmp_2,tmp_1,tmp_1
	cmpobl.f 31,tmp_1,Lstx_unfl		/* J/ underflow			*/

Lstx_84:
	cmpo	1,0				/* Clear carry			*/

#if	defined(CA_optim)
	eshro	tmp_1,rp_1,rp_1_lo
	shro	tmp_1,rp_1_hi,rp_1_hi
	addc	1,rp_1_lo,rp_1_lo
	addc	0,rp_1_hi,rp_1_hi
	eshro	1,rp_1,rp_1_lo
	shro	1,rp_1_hi,rp_1_hi
#else
	subo	tmp_1,tmp_2,tmp_2		/* Left shift for wd -> wd	*/
	shro	tmp_1,rp_1_lo,rp_1_lo
	shlo	tmp_2,rp_1_hi,tmp_2
	or	rp_1_lo,tmp_2,rp_1_lo
	shro	tmp_1,rp_1_hi,rp_1_hi
	addc	1,rp_1_lo,rp_1_lo
	addc	0,rp_1_hi,rp_1_hi
	shro	1,rp_1_lo,rp_1_lo
	shlo	32-1,rp_1_hi,tmp_2
	or	rp_1_lo,tmp_2,rp_1_lo
	shro	1,rp_1_hi,rp_1_hi
#endif

	bbs.f	TP_mode,hold_flags,Lstx_86	/* J/ extended precision	*/

	chkbit	Neg_mant,hold_flags		/* Copy sign bit over		*/
	alterbit 31,rp_1_hi,out_dp_hi
	movldar(rp_1_lo,out_dp_lo)
	ret


Lstx_86:
	chkbit	Neg_mant,hold_flags		/* Copy sign bit over		*/
	alterbit 15,0,out_tp_se			/* To sign/exp word		*/
	movl	rp_1,out_tp_mant_lo		/* Copy mantissa long word	*/
	ret


Lstx_90:
	lda	TP_Bias(scaled_exp),twos_exp
	cmpibge.f 0,twos_exp,Lstx_92		/* J/ denorm form		*/

	lda	TP_INF,tmp_1

	addc	rp_3_lo,rp_3_lo,rp_3_lo		/* Round result			*/
	addc	0,rp_1_lo,rp_1_lo
	addc	0,rp_1_hi,rp_1_hi
	alterbit 31,0,tmp_2			/* Handle carry out		*/
	or	rp_1_hi,tmp_2,rp_1_hi
	addc	0,twos_exp,twos_exp
	cmpobge.f twos_exp,tmp_1,Lstx_ovfl	/* J/ overflow			*/

	chkbit	Neg_mant,hold_flags			/* Transfer sign bit		*/
	movl	rp_1,out_tp_mant_lo
	alterbit 15,twos_exp,out_tp_se
	ret

Lstx_92:
	subo	twos_exp,0,tmp_1		/* Right shift for denorm/round	*/
	b	Lstx_83				/* Common DP/TP shift code	*/


/*
 *  Overflow/Underflow Returns
 */

Lstx_edom:
	movl	0,out_tp_mant_lo		/* Return +0.0			*/
	movlda(0,out_tp_se)			/* (all number formats)		*/
	ret


Lstx_unfl:
	mov	hold_flags,flags

	callj	__errno_ptr			/* returns addr of ERRNO in g0	*/
	ldconst	ERANGE,tmp_1
	st	tmp_1,(g0)

	bbs.f	TP_mode,flags,Lstx_u_20		/* J/ +/-0 in long double form	*/
	bbs.t	DP_mode,flags,Lstx_u_10		/* J/ +/-0 in double form	*/

	chkbit	Neg_mant,flags
	alterbit 31,0,out_fp
	ret

Lstx_u_10:
	chkbit	Neg_mant,flags
	alterbit 31,0,out_dp_hi
	movlda(0,out_dp_lo)
	ret

Lstx_u_20:
	chkbit	Neg_mant,flags
	alterbit 15,0,out_tp_se
	movl	0,out_tp_mant_lo
	ret


Lstx_ovfl:
	mov	hold_flags,flags

	callj	__errno_ptr			/* returns addr of ERRNO in g0	*/
	ldconst	ERANGE,tmp_1
	st	tmp_1,(g0)

	bbs.f	TP_mode,flags,Lstx_o_20		/* J/ INF in long double form	*/
	bbs.t	DP_mode,flags,Lstx_o_10		/* J/ INF in double form	*/

	chkbit	Neg_mant,flags
	lda	FP_INF << 23,out_fp
	alterbit 31,out_fp,out_fp
	ret

Lstx_o_10:
	chkbit	Neg_mant,flags
	lda	DP_INF << 20,out_dp_hi
	alterbit 31,out_dp_hi,out_dp_hi
	movlda(0,out_dp_lo)
	ret

Lstx_o_20:
	chkbit	Neg_mant,flags
	setbit	31,0,out_tp_mant_hi
	lda	TP_INF,out_tp_se
	alterbit 15,out_tp_se,out_tp_se
	movlda(0,out_tp_mant_lo)
	ret



/*
 *  Ten Scale Routine (entered via an internal BAL!)
 *
 *  Scales the incoming by the power of ten specified in ten_pwr.
 *
 *      Incoming value in:      twos_exp::mant_hi::mant_lo
 *      Return value in:        scaled_exp::rp_1_hi::rp_1_lo::rp_3_lo
 *
 *  Note that this routine destroys the contents of the following registers:
 *
 *      rp_1          rp_2          rp_3          rp_4          rp_5
 *      rp_6          tmp_1        +tmp_2        +tmp_3         ten_pwr
 *      ten_pwr_sign  scaled_exp    g14
 *
 *  The following registers are specifically preserved:
 *
 *      str_pntr      rtn_pntr_pntr mant_hi       mant_lo       twos_exp
 *      tens_exp     *flags
 *
 *  (+ -> register overlaps rp_6, * -> register overlaps tens_exp)
 *
 */

Lten_scale:
	movl	mant,rp_1			/* Just in case no scaling	*/
	movldar(twos_exp,scaled_exp)

	mov	g14,tmp_1			/* Save ret addr in tmp_1	*/

	cmpobe.f 0,ten_pwr,Lts_130		/* J/ no scaling		*/


	shri	31,ten_pwr,ten_pwr_sign		/* -1 or 0 from ten_pwr's sign	*/
	xor	ten_pwr,ten_pwr_sign,ten_pwr
	subo	ten_pwr_sign,ten_pwr,ten_pwr	/* abs(ten_pwr) -> ten_pwr	*/

	mov	0,rp_1_hi			/* Signal no value yet		*/
	lda	Lten_table_mant-.-8(ip),tmp_2	/* Base addr (rel computation)	*/

	lda	3520,tmp_3
	cmpobl	ten_pwr,tmp_3,Lts_010		/* J/ power of ten < 3520	*/

	ldl	Lten_power_3520_mant-Lten_table_mant(tmp_2),rp_1
	subo	tmp_3,ten_pwr,ten_pwr
	lda	ten_power_3520_exp,scaled_exp

/*
 *  Try for 3520 a second time to handle strto- exp overflow
 */
	cmpobl	ten_pwr,tmp_3,Lts_010		/* J/ power of ten < 3520	*/

	subo	tmp_3,ten_pwr,ten_pwr

	movl	rp_1,rp_2			/* Square mantissa		*/
	shlo	1,scaled_exp,scaled_exp		/* Double exponent		*/

	bal	Lten_power_mult


Lts_010:
	lda	1760,tmp_3
	cmpobl	ten_pwr,tmp_3,Lts_020		/* J/ power of ten < 1760	*/

	subo	tmp_3,ten_pwr,ten_pwr

	cmpobne	0,rp_1_hi,Lts_015		/* J/ rp_1 in use		*/

	ldl	Lten_power_1760_mant-Lten_table_mant(tmp_2),rp_1
	lda	ten_power_1760_exp,scaled_exp
	b	Lts_020

Lts_015:
	ldl	Lten_power_1760_mant-Lten_table_mant(tmp_2),rp_2
	lda	ten_power_1760_exp(scaled_exp),scaled_exp

	bal	Lten_power_mult


Lts_020:
	lda	880,tmp_3
	cmpobl	ten_pwr,tmp_3,Lts_030		/* J/ power of ten < 880	*/

	subo	tmp_3,ten_pwr,ten_pwr

	cmpobne	0,rp_1_hi,Lts_025		/* J/ rp_1 in use		*/

	ldl	Lten_power_880_mant-Lten_table_mant(tmp_2),rp_1
	lda	ten_power_880_exp,scaled_exp
	b	Lts_030

Lts_025:
	ldl	Lten_power_880_mant-Lten_table_mant(tmp_2),rp_2
	lda	ten_power_880_exp(scaled_exp),scaled_exp

	bal	Lten_power_mult


Lts_030:
	lda	440,tmp_3
	cmpobl	ten_pwr,tmp_3,Lts_040		/* J/ power of ten < 440	*/

	subo	tmp_3,ten_pwr,ten_pwr

	cmpobne	0,rp_1_hi,Lts_035		/* J/ rp_1 in use		*/

	ldl	Lten_power_440_mant-Lten_table_mant(tmp_2),rp_1
	lda	ten_power_440_exp,scaled_exp
	b	Lts_040

Lts_035:
	ldl	Lten_power_440_mant-Lten_table_mant(tmp_2),rp_2
	lda	ten_power_440_exp(scaled_exp),scaled_exp

	bal	Lten_power_mult


Lts_040:
	lda	220,tmp_3
	cmpobl	ten_pwr,tmp_3,Lts_050		/* J/ power of ten < 220	*/

	subo	tmp_3,ten_pwr,ten_pwr

	cmpobne	0,rp_1_hi,Lts_045		/* J/ rp_1 in use		*/

	ldl	Lten_power_220_mant-Lten_table_mant(tmp_2),rp_1
	lda	ten_power_220_exp,scaled_exp
	b	Lts_050

Lts_045:
	ldl	Lten_power_220_mant-Lten_table_mant(tmp_2),rp_2
	lda	ten_power_220_exp(scaled_exp),scaled_exp

	bal	Lten_power_mult


Lts_050:
	lda	110,tmp_3
	cmpobl	ten_pwr,tmp_3,Lts_060		/* J/ power of ten < 110	*/

	subo	tmp_3,ten_pwr,ten_pwr

	cmpobne	0,rp_1_hi,Lts_055		/* J/ rp_1 in use		*/

	ldl	Lten_power_110_mant-Lten_table_mant(tmp_2),rp_1
	lda	ten_power_110_exp,scaled_exp
	b	Lts_060

Lts_055:
	ldl	Lten_power_110_mant-Lten_table_mant(tmp_2),rp_2
	lda	ten_power_110_exp(scaled_exp),scaled_exp

	bal	Lten_power_mult


Lts_060:
	lda	55,tmp_3
	cmpobl	ten_pwr,tmp_3,Lts_070		/* J/ power of ten < 55	*/

	subo	tmp_3,ten_pwr,ten_pwr

	cmpobne	0,rp_1_hi,Lts_065		/* J/ rp_1 in use		*/

	ldl	Lten_power_55_mant-Lten_table_mant(tmp_2),rp_1
	lda	ten_power_55_exp,scaled_exp
	b	Lts_070

Lts_065:
	ldl	Lten_power_55_mant-Lten_table_mant(tmp_2),rp_2
	lda	ten_power_55_exp(scaled_exp),scaled_exp

	bal	Lten_power_mult


Lts_070:
	lda	27,tmp_3
	cmpobl	ten_pwr,tmp_3,Lts_080		/* J/ power of ten < 27	*/

	subo	tmp_3,ten_pwr,ten_pwr

	cmpobne	0,rp_1_hi,Lts_075		/* J/ rp_1 in use		*/

	ldl	Lten_power_27_mant-Lten_table_mant(tmp_2),rp_1
	lda	ten_power_27_exp,scaled_exp
	b	Lts_080

Lts_075:
	ldl	Lten_power_27_mant-Lten_table_mant(tmp_2),rp_2
	lda	ten_power_27_exp(scaled_exp),scaled_exp

	bal	Lten_power_mult


Lts_080:
	cmpobe.f 0,ten_pwr,Lts_090		/* J/ no table look-up!		*/

	cmpobne	0,rp_1_hi,Lts_085		/* J/ rp_1 in use		*/

	ldl	-8(tmp_2)[ten_pwr*8],rp_1
	ldob	Lten_table_exp-Lten_table_mant-1(tmp_2)[ten_pwr],scaled_exp
	b	Lts_090

Lts_085:
	ldl	-8(tmp_2)[ten_pwr*8],rp_2
	ldob	Lten_table_exp-Lten_table_mant-1(tmp_2)[ten_pwr],tmp_3
	addo	tmp_3,scaled_exp,scaled_exp

	bal	Lten_power_mult

Lts_090:


	mov	tmp_1,g14			/* Lten_scale ret addr -> g14	*/


	cmpibne	0,ten_pwr_sign,Lts_110		/* Negative power of ten	*/


/*
 *  Complete scaling by multiplying by the power of ten
 */

	emul	mant_lo,rp_1_lo,rp_4

	cmpo	1,0				/* Clear carry			*/

	emul	mant_lo,rp_1_hi,rp_3

	addo	scaled_exp,twos_exp,scaled_exp

	emul	mant_hi,rp_1_lo,rp_2

	addc	rp_4_hi,rp_3_lo,rp_3_lo
	addc	0,rp_3_hi,rp_3_hi

	emul	mant_hi,rp_1_hi,rp_1

	addc	rp_2_lo,rp_3_lo,rp_3_lo
	addc	rp_2_hi,rp_3_hi,rp_3_hi
	addc	0,0,tmp_1

	addc	rp_3_hi,rp_1_lo,rp_1_lo
	addc	tmp_1,rp_1_hi,rp_1_hi

	b	Lts_120

/*
 *  Division approximation using long division technique:
 *
 *    a / (b+c) ~= a/b - a/b*c/b + a/b*c/b*c/b
 *
 *  Choosing b as the hi word of the divisor and c as the low word of
 *  these division, the estimate is accurate to well over 64 bits with
 *  three terms.
 */

Lts_110:

#if	defined(CA_optim)
	eshro	1,mant_lo,rp_2_lo		/* Prevent division overflow	*/
#else
	shro	1,mant_lo,rp_2_lo		/* Prevent division overflow	*/
	shlo	32-1,mant_hi,tmp_1
	or	rp_2_lo,tmp_1,rp_2_lo
#endif
	shro	1,mant_hi,rp_2_hi

	ediv	rp_1_hi,rp_2,rp_2		/* Top word of a/b		*/

	shro	31,mant_lo,rp_3_lo
	mov	rp_2_lo,rp_3_hi

	ediv	rp_1_hi,rp_3,rp_3		/* Middle word of a/b		*/

	mov	rp_3_hi,rp_2_lo			/* For a/b in rp2::rp_3_hi	*/
	mov	rp_3_lo,rp_3_hi
	movlda(0,rp_3_lo)

	ediv	rp_1_hi,rp_3,rp_3		/* Low word of a/b		*/

	shro	1,rp_1_lo,rp_4_hi		/* Prepare to compute c/b	*/
	shlo	32-1,rp_1_lo,rp_4_lo

	ediv	rp_1_hi,rp_4,rp_4		/* High word of c/b		*/

	mov	rp_4_hi,tmp_1
	mov	rp_4_lo,rp_4_hi
	movlda(0,rp_4_lo)

	ediv	rp_1_hi,rp_4,rp_4		/* Low word of c/b		*/

	subo	scaled_exp,twos_exp,scaled_exp
	addo	1,scaled_exp,scaled_exp

	emul	rp_4_hi,rp_2_lo,rp_5		/* Begin computing c/b * a/b	*/

	mov	rp_4_hi,rp_4_lo			/* Form c/b lo product in rp_4	*/
	mov	tmp_1,rp_4_hi

	emul	rp_4_hi,rp_2_lo,rp_1		/* Form c/b mid/a prod in rp_1	*/

	cmpo	1,0				/* clear carry			*/

	emul	rp_4_lo,rp_2_hi,rp_6		/* Form c/b mid/b prod in rp_6	*/

	addc	rp_5_hi,rp_1_lo,rp_1_lo		/* Combine lo with mid/a	*/
	addc	0,rp_1_hi,rp_1_hi

	emul	rp_4_hi,rp_2_hi,rp_5

	addc	rp_6_lo,rp_1_lo,rp_1_lo		/* (lo+mid/a) + mid/b		*/
	addc	rp_6_hi,rp_1_hi,rp_1_hi
	addc	0,0,tmp_1

	addc	rp_1_hi,rp_5_lo,rp_1_lo		/* Finish combining partials	*/
	addc	tmp_1,rp_5_hi,rp_1_hi		/* c/b * a/b in  rp_1		*/

	emul	rp_1_hi,rp_4_hi,rp_4		/* Compute c/b * c/b * a/b	*/

	cmpo	0,0				/* Set no borrow		*/

	shlo	1,rp_4_hi,rp_4_lo		/* Align term 3 to term 2	*/
	shro	32-1,rp_4_hi,rp_4_hi

	subc	rp_4_lo,rp_1_lo,rp_1_lo		/* Calc a/b*c/b - a/b*c/b*c/b	*/
	subc	rp_4_hi,rp_1_hi,rp_1_hi

	cmpo	0,1				/* Clear carry			*/

	addc	rp_1_lo,rp_1_lo,tmp_1		/* Align corr term to a/b	*/
	addc	rp_1_hi,rp_1_hi,rp_1_lo
	addc	0,0,rp_1_hi

	cmpo	0,0				/* Clear borrow			*/

	subc	tmp_1,rp_3_hi,rp_3_lo
	subc	rp_1_lo,rp_2_lo,rp_1_lo
	subc	rp_1_hi,rp_2_hi,rp_1_hi


Lts_120:
	bbs.t	31,rp_1_hi,Lts_130		/* J/ normalized		*/

	addc	rp_3_lo,rp_3_lo,rp_3_lo		/* Normalize value		*/
	subo	1,scaled_exp,scaled_exp		/* Adjust exponent		*/
	addc	rp_1_lo,rp_1_lo,rp_1_lo
	addc	rp_1_hi,rp_1_hi,rp_1_hi


Lts_130:
	bx	(g14)



Lten_power_mult:
	emul	rp_1_lo,rp_2_lo,rp_3
	emul	rp_1_hi,rp_2_lo,rp_4

	cmpo	1,0				/* Insure carry = 0		*/

	emul	rp_1_lo,rp_2_hi,rp_5

	addc	rp_3_hi,rp_4_lo,rp_4_lo
	addc	0,rp_4_hi,rp_4_hi

	emul	rp_1_hi,rp_2_hi,rp_1

	addc	rp_5_lo,rp_4_lo,rp_4_lo
	addc	rp_5_hi,rp_4_hi,rp_4_hi
	addc	0,0,tmp_3

	addc	rp_4_hi,rp_1_lo,rp_1_lo
	addc	tmp_3,rp_1_hi,rp_1_hi

	bbs	31,rp_1_hi,Ltpm_10

	addc	rp_4_lo,rp_4_lo,rp_4_lo
	addc	rp_1_lo,rp_1_lo,rp_1_lo
	addc	rp_1_hi,rp_1_hi,rp_1_hi

	subo	1,scaled_exp,scaled_exp

Ltpm_10:
	chkbit	31,rp_4_lo
	addc	0,rp_1_lo,rp_1_lo
	addc	0,rp_1_hi,rp_1_hi

	bx	(g14)
