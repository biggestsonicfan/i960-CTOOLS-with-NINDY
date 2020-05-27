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

#define RZ		1
#define qtc_dacos	acos
#define qtc_dasin	asin
#define qtc_datan	atan
#define qtc_dcos	cos
#define qtc_dcosh	cosh
#define qtc_dexp	exp
#ifdef __i960KB
#define qtc_dexpm1	_Lqexpm1
#else
#define qtc_dexpm1	expm1
#endif
#define qtc_dhypot	hypot
#define qtc_dlog	log
#define qtc_dpow	pow
#define qtc_dsin	sin
#define qtc_dsinh	sinh
#define qtc_dsqrt	sqrt
#define qtc_dtan	tan
#define qtc_dtanh	tanh
#define qtc_error	_Lqerror
#define qtc_util_dhypot	_Lhypot_util

#define qtc_fatan	atanf
#define qtc_fcos	cosf
#define qtc_fexp	expf
#define qtc_flog	logf
#define qtc_fpow	powf
#define qtc_fsin	sinf
#define qtc_fsqrt	sqrtf
#define qtc_ftan	tanf
#define qtc_ferror	_Lqerrorf
float _Lqerrorf(int, float, float);
