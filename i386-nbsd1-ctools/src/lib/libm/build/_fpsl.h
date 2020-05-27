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

/*	iC-960 $Revision: 1.3 $ */

/*
 *	Floating-Point Support Library
 *
 * This is everything which is visible to the FPSL user.
 */


/* Check for Multiple includes */
#ifndef __fpslh
#define __fpslh

#ifndef _extended
#define _extended
typedef	long double	extended;
#endif

#define FP_DEFENV  0x3f009000 /* Used by pow.c and exp.c */

	/* FPSL function codes for exception handler */
#define	FPSL_ASIN	  1
#define	FPSL_ACOS	  2
#define	FPSL_EXP	  3
#define	FPSL_SINH	  4
#define	FPSL_COSH	  5
#define	FPSL_TANH	  6
#define	FPSL_LOG	  7
#define	FPSL_LOG10	  8
#define	FPSL_POW	  9
#define	FPSL_LOG1P	  10
#define	FPSL_EXPM1	  11

/*************************************/

/* Floating-Point types */

#define	FP_ZERO   	0
#define	FP_DENORM       1+0
#define	FP_NORM         2+0
#define	FP_INF          3+0
#define	FP_QNAN         4+0
#define	FP_SNAN         5+0
#define	FP_RESERVED     6+0
/* negative */
#define	FP_N_ZERO       0+8
#define	FP_N_DENORM     1+8
#define	FP_N_NORM       2+8
#define	FP_N_INF        3+8
#define	FP_N_QNAN       4+8
#define	FP_N_SNAN       5+8
#define	FP_N_RESERVED   6+8

short	_Lclassf(float);
short	_Lclass(double);
short	_Lclassl(extended);

#define	uclassf(x)	(classf(x) & 7)	/* equiv to class(abs(x)) */
#define	uclass(x)	(class(x) & 7)
#define	uclassl(x)	(classl(x) & 7)


int	_Lisnanf(float);
int	_Lisnan(double);
int	_Lisnanl(extended);

float		__Lnan1f(float, unsigned int, short);
double		__Lnan1(double, unsigned int, short);
extended	__Lnan1l(extended, unsigned int, short);

float		nan2f(float, float, unsigned int, short);
double		nan2(double, double, unsigned int, short);
extended	nan2l(extended, extended, unsigned int, short);

extended	fabsl(extended);

float		copysignf(float, float);
double		copysign(double, double);
extended	copysignl(extended, extended);


/* Exit routines */

float		_Lfps_exit(float, unsigned int, short);
double		_Lfpd_exit(double, unsigned int, short);
extended	_Lfpe_exit(extended, unsigned int, short);

float		_Lquickexitf(float, unsigned int, short);
double		_Lquickexit(double, unsigned int, short);
extended	_Lquickexitl(extended, unsigned int, short);

float		_Lfaultexitf(float, short, unsigned int, short);
double		_Lfaultexit(double, short, unsigned int, short);
extended	_Lfaultexitl(extended, short, unsigned int, short);

int		_Lfpi_exit(int, unsigned int, short);

#if     defined(__i960KB) || defined(__i960SB)
/*
 * 80960 Instructions
 */
extended	sqrtl(extended);
extended	ceill(extended);
extended	floorl(extended);
extended	sinl(extended);
extended	cosl(extended);
extended	tanl(extended);
extended	atanl(extended);
extended	atan2l(extended, extended);
float		_Lsatan2f(extended, float);
double		_Lsatan2(extended, double);
extended	_Lsatan2l(extended, extended);

float		_Lratan2f(extended, float);
double		_Lratan2(extended, double);
extended	_Lratan2l(extended, extended);

float		_Lexp2m1f(float);
double		_Lexp2m1(double);
extended	_Lexp2m1l(extended);

float		_Lylog2xf(float, float);
double		_Lylog2x(double, double);
extended	_Lylog2xl(extended, extended);

float		_Lclog2xf(float, extended);
double		_Lclog2x(double, extended);
extended	_Lclog2xl(extended, extended);

float		_Lclogep2xf(float, extended);
double		_Lclogep2x(double, extended);
extended	_Lclogep2xl(extended, extended);

/*
 * The following are declarations for the FPSL functions coded in C.
 */

extended	asinl(extended);

extended	acosl(extended);

extended	expl(extended);

float		expm1f(float);
double		expm1(double);
extended	expm1l(extended);

extended	logl(extended);

float		log1pf(float);
double		log1p(double);
extended	log1pl(extended);

extended	log10l(extended);

extended	sinhl(extended);

extended	coshl(extended);

extended	fpe_expm1i(extended, int *);

extended	fpe_exp2m1i(extended, int *);

extended	tanhl(extended);

extended	powl(extended, extended);

/*************************************/


/*
 * CONSTANTS required by FPSL routines.
 */

#define	SCL_OV_LIM	33	/* Used in sinh.c, */
#define	pow_int_limit	64	/* Used in pow.c */

#define	log2_e	*((long double *)_Le_log2_e)   /* Used by exp.c, */
#define	loge_2	*((long double *)_e_loge_2)   /* Used by log in log.c */
#define	log10_2	*((long double *)_Le_log10_2)  /* Used by log10 in log.c */

#ifdef SINGLE			/** ALL CONSTANTS ARE SINGLE PRECISION **/
#define	posinf	*((float *)_Ls_posinf)   /* Used by exp.c, */
#define	neginf	*((float *)_Ls_neginf)   /* Used by exp.c, */
#define	qnan	*((float *)_Ls_qnan)     /* Used by pow.c, */
#define	negqnan	*((float *)_Ls_negqnan)     /* Used by pow.c, */
#define	EXP_OV_LIM	90.0	       /* Used by sinh.c, */
#define	logep_hlim	*((float *)_Ls_logep_hlim)  /* Used by log.c */
#define	logep_llim	*((float *)_Ls_logep_llim)  /* Used by log.c */
#define	tanh_hlim	*((float *)_Ls_tanh_hlim)   /* Used by tanh.c */
#define	tanh_llim	*((float *)_Ls_tanh_llim)   /* Used by tanh.c */
#endif

#if defined(LIBM) || defined(DOUBLEP) /* ALL CONSTANTS ARE DOUBLE PRECISION */
#define	posinf	*((double *)_d_posinf)   /* Used by exp.c, */
#define	neginf	*((double *)_Ld_neginf)   /* Used by exp.c, */
#define	qnan	*((double *)_Ld_qnan)     /* Used by pow.c, */
#define	negqnan	*((double *)_Ld_negqnan)     /* Used by pow.c, */
#define	EXP_OV_LIM	711.0	        /* Used by sinh.c, */
#define	logep_hlim	*((double *)_Ld_logep_hlim)  /* Used by log.c */
#define	logep_llim	*((double *)_Ld_logep_llim)  /* Used by log.c */
#define	tanh_hlim	*((double *)_Ld_tanh_hlim)  /* Used by tanh.c */
#define	tanh_llim	*((double *)_Ld_tanh_llim)  /* Used by tanh.c */
#endif


#ifdef EXTENDED			/** ALL CONSTANTS ARE EXTENDED PRECISION **/
#define	posinf	*((long double *)_Le_posinf)   /* Used by exp.c, */
#define	neginf	*((long double *)_Le_neginf)   /* Used by exp.c, */
#define	qnan	*((long double *)_Le_qnan)     /* Used by pow.c, */
#define	negqnan	*((long double *)_Le_negqnan)     /* Used by pow.c, */
#define	EXP_OV_LIM	11358.0		     /* Used by sinh.c, */
#define	logep_hlim	*((long double *)_Le_logep_hlim)  /* Used by log.c */
#define	logep_llim	*((long double *)_Le_logep_llim)  /* Used by log.c */
#define	tanh_hlim	*((long double *)_Le_tanh_hlim)  /* Used by tanh.c */
#define	tanh_llim	*((long double *)_Le_tanh_llim)  /* Used by tanh.c */
#endif

/*
 * The following are the declarations of constants used by FPSL.
 */

extern	const unsigned int	_Le_log2_e[4];
extern	const unsigned int	_e_loge_2[4];
extern	const unsigned int	_Le_log10_2[4];
extern	const unsigned int	_Le_posinf[4];
extern	const unsigned int	_d_posinf[2];
extern	const unsigned int	_Ls_posinf[1];

extern	const unsigned int	_Le_neginf[4];
extern	const unsigned int	_Ld_neginf[4];
extern	const unsigned int	_Ls_neginf[1];

extern	const unsigned int	_Le_qnan[4];
extern	const unsigned int	_Ld_qnan[2];
extern	const unsigned int	_Ls_qnan[1];

extern	const unsigned int	_Le_negqnan[4];
extern	const unsigned int	_Ld_negqnan[2];
extern	const unsigned int	_Ls_negqnan[1];

extern	const unsigned int	_Le_logep_hlim[4];
extern	const unsigned int	_Ld_logep_hlim[2];
extern	const unsigned int	_Ls_logep_hlim[1];

extern	const unsigned int	_Le_logep_llim[4];
extern	const unsigned int	_Ld_logep_llim[2];
extern	const unsigned int	_Ls_logep_llim[1];

extern	const unsigned int	_Le_tanh_hlim[4];
extern	const unsigned int	_Ld_tanh_hlim[2];
extern	const unsigned int	_Ls_tanh_hlim[1];

extern	const unsigned int	_Le_tanh_llim[4];
extern	const unsigned int	_Ld_tanh_llim[2];
extern	const unsigned int	_Ls_tanh_llim[1];
#endif /* defined(__i960KB) || defined(__i960SB) */
#endif	/* __fpslh - Check for multiple includes */
