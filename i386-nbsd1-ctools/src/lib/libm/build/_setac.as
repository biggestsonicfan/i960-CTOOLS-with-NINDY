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

#include "i960arch.h"

	.file "_setac"
#ifdef __PIC
	.link_pix
#endif
#ifdef __PID
	.pid
#endif
	.text

	.globl	__setac

__setac:
#if	defined(_i960KX)
	ldconst	0xffffffff,g1
	modac	g1,g0,g0
#else
	ldconst	0xffffffff,g1
	modac	g1,g0,g4		/* update AC reg, get former value */

/* NOTE: fpem_CA_AC is not biased by g12.  It is always in a fixed location.*/
	ld	fpem_CA_AC,g1
	ldconst	0xff1f0000,g2
	modify	g2,g1,g4		/* fpem_CA_AC combined with old AC */
	and	g2,g0,g1		/* extract fp control from param */

/* NOTE: fpem_CA_AC is not biased by g12.  It is always in a fixed location.*/
	st	g1,fpem_CA_AC		/* update fpem_CA_AC */
	mov	g4,g0			/* return old composite AC value */
#endif
	ret

