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

	.file "log_sup.s"
#define CSTEP

#ifdef __PIC
	.link_pix
#endif
#ifdef __PID
	.pid
#endif

/*
*  _Lylog2xf(x,y)
*  returns (y *  log2_x) 
*

*  Input:	x,y are single
*  Output:	y *  log2_x
*/

	.globl	__Lylog2xf
__Lylog2xf:
#ifndef CSTEP
	logr	g0,0f1.0,g0
	mulr	g0,g1,g0
#else
	logr	g0,g1,g0
#endif
	ret

/*
*  _Lylog2x(x,y)
*  returns (y *  log2_x) 
*

*  Input:	x,y are double
*  Output:	y *  log2_x
*/

	.globl	__Lylog2x
__Lylog2x:
#ifndef	CSTEP
	logrl	g0,0f1.0,g0
	mulrl	g0,g2,g0
#else
	logrl	g0,g2,g0
#endif
	ret

/*
*  _Lylog2xl(x,y)
*  returns (y *  log2_x) 
*

*  Input:	x,y are extended_real
*  Output:	y *  log2_x
*/

	.globl	__Lylog2xl
__Lylog2xl:
	movre	g0,fp0
	movre	g4,fp1
#ifndef CSTEP
	logr	fp0,0f1.0,fp0
	mulr	fp0,fp1,fp0
	movre	fp0,g0
#else
	logr	fp0,fp1,fp0
	movre	fp0,g0
#endif
	ret

/*
*  _Lclog2xf(x,c)
*  returns (c *  log2_x) 
*

*  Input:	x (in g0) is float, c (in g4..g6) is extended_real
*  Output:	c *  log2_x : float
*/

	.globl	__Lclog2xf
__Lclog2xf:
	movre	g4,fp0
#ifndef	CSTEP
	logr	g0,0f1.0,g0
	mulr	g0,fp0,g0
#else
	logr	g0,fp0,g0
#endif
	ret

/*
*  _Lclog2x(x,c)
*  returns (c *  log2_x) 
*

*  Input:	x (in g0,g1) is double, c (in g4..g6) is extended_real
*  Output:	c *  log2_x : double
*/

	.globl	__Lclog2x
__Lclog2x:
	movre	g4,fp0
#ifndef CSTEP
	logrl	g0,0f1.0,g0
	mulrl	g0,fp0,g0
#else
	logrl	g0,fp0,g0
#endif
	ret

/*
*  _Lclog2xl(x,c)
*  returns (c *  log2_x) 
*

*  Input:	x (in g0..g2) and c (in g4..g6) are extended_real
*  Output:	c *  log2_x : extended_real
*/

	.globl	__Lclog2xl
__Lclog2xl:
	movre	g0,fp0
	movre	g4,fp1
#ifndef CSTEP
	logr	fp0,0f1.0,fp0
	mulr	fp0,fp1,fp0
	movre	fp0,g0
#else
	logr	fp0,fp1,fp0
	movre	fp0,g0
#endif
	ret

/*
*  _Lclogep2xf(x,c)
*  returns (c *  log2_(1+x)) 
*

*  Input:	x (in g0) is float, c (in g4..g6) is extended_real
*  Output:	c *  log2_(1+x) : float
*/

	.globl	__Lclogep2xf
__Lclogep2xf:
	movre	g4,fp0
	logepr	g0,fp0,g0
	ret

/*
*  _Lclogep2x(x,c)
*  returns (c *  log2_x) 
*

*  Input:	x (in g0,g1) is double, c (in g4..g6) is extended_real
*  Output:	c *  log2_(1+x) : double
*/

	.globl	__Lclogep2x
__Lclogep2x:
	movre	g4,fp0
	logeprl	g0,fp0,g0
	ret

/*
*  _Lclogep2xl(x,c)
*  returns (c *  log2_x) 
*

*  Input:	x (in g0..g2) and c (in g4..g6) are extended_real
*  Output:	c *  log2_(1+x) : extended_real
*/

	.globl	__Lclogep2xl
__Lclogep2xl:
	movre	g0,fp0
	movre	g4,fp1
	logepr	fp0,fp1,fp0
	movre	fp0,g0
	ret

