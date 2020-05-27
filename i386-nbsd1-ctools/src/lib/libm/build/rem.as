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

	.file "rem.s"
#ifdef __PIC
	.link_pix
#endif
#ifdef __PID
	.pid
#endif
/*
 *  remf(x,y)
 *  returns the modulus remainder 
 *

 *  Input:	x,y are single
 *  Output:	P7 remainder
 */

	.globl _fp_remf
_fp_remf:
#if defined (_i960XB)
	remr	g1,g0,g0
#else
	call	___remsf3
#endif
	ret

/*
 *  _fp_rem(x,y)
 *  returns the modulus remainder 
 *

 *  Input:	x,y are double
 *  Output:	P7 remainder
 */

	.globl _fp_rem
_fp_rem:
#if defined (_i960XB)
	remrl	g2,g0,g0
#else
	call	___remdf3
#endif
	ret

/*
 *  reml(x,y)
 *  returns the modulus remainder 
 *

 *  Input:	x,y are extended_real
 *  Output:	P7 remainder
 */

	.globl _fp_reml
_fp_reml:
#if defined (_i960XB)
	movre	g0,fp0
	movre	g4,fp1
	remr	fp1,fp0,fp0
	movre	fp0,g0
#else
	call	___remtf3
#endif
	ret
