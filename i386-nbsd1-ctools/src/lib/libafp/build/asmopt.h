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

#if	defined(__i960CA) || defined(__i960CF)
#define	MAJOR_CODE_ALIGNMENT	5
#define	MINOR_CODE_ALIGNMENT	4
#define	USE_SIMULATED_AC
#define	USE_ESHRO
#define	USE_LDA_REG_OFF

#elif	defined(__i960JA) || defined(__i960JD) || defined(__i960JF) || defined(__i960JL) || defined(__i960RP)
#define	MAJOR_CODE_ALIGNMENT	3
#define	MINOR_CODE_ALIGNMENT	2
#define	USE_SIMULATED_AC
#define	USE_LDA_REG_OFF
#define	USE_CMP_BCC
#define	USE_OPN_CC

#elif	defined(__i960KA)
#define	MAJOR_CODE_ALIGNMENT	4
#define	MINOR_CODE_ALIGNMENT	3

#elif	defined(__i960SA)
#define	MAJOR_CODE_ALIGNMENT	4
#define	MINOR_CODE_ALIGNMENT	3

#else
#error unknown/invalid architecture
#endif



#if	defined(__i960CA) && !defined(CK)
#define	CA_optim	1
#endif

#if	defined(__i960CA) || defined(__i960CF)
#define	addlda(a,b,c)	lda	a(b),c
#else
#define addlda(a,b,c)	addo	a,b,c
#endif

#if	defined(__i960CA) || defined(__i960CF)
#define	addldax(a,b,c)	lda	a+b,c
#else
#define addldax(a,b,c)	addo	a,b,c
#endif

#if	defined(__i960CA) || defined(__i960CF)
#define	movlda(a,c)	lda	a,c
#else
#define movlda(a,c)	mov	a,c
#endif

#if	defined(__i960CA) || defined(__i960CF)
#define	movldar(a,b)	lda	(a),b
#else
#define	movldar(a,b)	mov	a,b
#endif

#if	defined(__i960CA) || defined(__i960CF)
#define	onebit(a,b)	setbit	a,0,b
#else
#define	onebit(a,b)	shlo	a,1,b
#endif

#if	defined(__i960CA) || defined(__i960CF)
#define	shlo1(a,b)	lda	[a*2],b
#else
#define	shlo1(a,b)	shlo	1,a,b
#endif

#if	defined(__i960CA) || defined(__i960CF)
#define	shlo2(a,b)	lda	[a*4],b
#else
#define	shlo2(a,b)	shlo	2,a,b
#endif
