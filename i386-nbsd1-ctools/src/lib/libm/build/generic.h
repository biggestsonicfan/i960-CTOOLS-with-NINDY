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


/*
 *	Generic mapping for FPSL
 *
 * $Header: /ffs/p1/dev/src/lib/libm/common/RCS/generic.h,v 1.2 1993/11/16 18:13:22 alex Exp $
 * $Locker:  $
 *
 */

#ifdef	SINGLE
#define	fp_class	_Lclassf
#define	fp_copysign	copysignf
#define	fp_abs		fabsf
#define	fp_isnan	_Lisnanf
#define	fp_unordered	unorderedf
#define	fp_scale	fp_scalef
#define	fp_logb		fp_logbf
#define	fp_round	fp_roundf
#define	fp_ceil		ceilf
#define	fp_floor	floorf
#define	fp_rmd		fp_rmdf
#define	_fp_rem		_fp_remf
#define	fp_sqrt		sqrtf
#define	fp_sin		sinf
#define	fp_cos		cosf
#define	fp_tan		tanf
#define	fp_atan2	atan2f
#define	fp_satan2	_Lsatan2f
#define	fp_ratan2	_Lratan2f
#define	fp_exp2m1	_Lexp2m1f
#define	fp_ylog2x	_Lylog2xf
#define	fp_clogep2x	_Lclogep2xf
#define	fp_clog2x	_Lclog2xf
#define	fp_nan1		__Lnan1f
#define	fp_exit		_Lfps_exit
#define	fp_quickexit	_Lquickexitf
#define	fp_faultexit	_Lfaultexitf
#define	GENERIC		float
#define	asin	asinf
#define	acos	acosf
#define	exp	expf
#define	expm1	expm1f
#define	log	logf
#define	log10	log10f
#define	log1p	log1pf
#define	sinh	sinhf
#define	cosh	coshf
#define	tanh	tanhf
#define	pow	powf
#endif

#ifdef	DOUBLEP
#define	fp_class	_Lclass
#define	fp_copysign	copysign
#define	fp_abs		fabs
#define	fp_isnan	_Lisnan
#define	fp_unordered	unordered
#define	fp_scale	fp_scale
#define	fp_logb		fp_logb
#define	fp_round	fp_round
#define	fp_ceil		ceil
#define	fp_floor	floor
#define	fp_rmd		fp_rmd
#define	_fp_rem		_fp_rem
#define	fp_sqrt		sqrt
#define	fp_sin		sin
#define	fp_cos		cos
#define	fp_tan		tan
#define	fp_atan2	atan2
#define	fp_satan2	_Lsatan2
#define	fp_ratan2	_Lratan2
#define	fp_exp2m1	_Lexp2m1
#define	fp_clogep2x	_Lclogep2x
#define	fp_ylog2x	_Lylog2x
#define	fp_clog2x	_Lclog2x
#define	fp_nan1		__Lnan1
#define	fp_exit		_Lfpd_exit
#define	fp_quickexit	_Lquickexit
#define	fp_faultexit	_Lfaultexit
#define	GENERIC		double
#define	asin	asin
#define	acos	acos
#define	exp	exp
#define	expm1	expm1
#define	log	log
#define	log10	log10
#define	log1p	log1p
#define	sinh	sinh
#define	cosh	cosh
#define	tanh	tanh
#define	pow	pow
#endif

#ifdef	EXTENDED
#define	fp_class	_Lclassl
#define	fp_copysign	copysignl
#define	fp_abs		fabsl
#define	fp_isnan	_Lisnanl
#define	fp_unordered	unorderedl
#define	fp_scale	fp_scalel
#define	fp_logb		fp_logbl
#define	fp_round	fp_roundl
#define	fp_ceil		ceill
#define	fp_floor	floorl
#define	fp_rmd		fp_rmdl
#define	_fp_rem		_fp_reml
#define	fp_sqrt		sqrtl
#define	fp_sin		sinl
#define	fp_cos		cosl
#define	fp_tan		tanl
#define	fp_atan2	atan2l
#define	fp_satan2	_Lsatan2l
#define	fp_ratan2	_Lratan2l
#define	fp_exp2m1	_Lexp2m1l
#define	fp_clogep2x	_Lclogep2xl
#define	fp_ylog2x	_Lylog2xl
#define fp_clog2x	_Lclog2xl
#define fp_nan1		__Lnan1l
#define	fp_exit		_Lfpe_exit
#define	fp_quickexit	_Lquickexitl
#define	fp_faultexit	_Lfaultexitl
#define	GENERIC		long double
#define	asin	asinl
#define	acos	acosl
#define	exp	expl
#define	expm1	expm1l
#define	log	logl
#define	log10	log10l
#define	log1p	log1pl
#define	sinh	sinhl
#define	cosh	coshl
#define	tanh	tanhl
#define	pow	powl
#endif
