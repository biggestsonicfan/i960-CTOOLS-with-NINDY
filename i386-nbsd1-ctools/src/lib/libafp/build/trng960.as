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
/*      trng960.c - Double Precision to Single Precision Conversion Routine   */
/*                  Wrapper specially for gcc960.                             */
/*		   (AFP-960)						      */
/*									      */
/******************************************************************************/

#include "asmopt.h"

	.file	"trng960.as"
	.globl	___truncdfsf2
	.globl	___truncdfsf2_g960

/*
 * The entry point ___truncdfsf2_g960 can only modify g0,g1 and the
 * local register set.  AFP-960 does not honor this restriction for
 * truncdfsf2, so we have created this wrapper routine.
 */
	.text
	.link_pix

	.align	MAJOR_CODE_ALIGNMENT

___truncdfsf2_g960:
	movl	g2,r6
	movq	g4,r8
	call	___truncdfsf2
	movq	r8,g4
	movl	r6,g2
	ret
