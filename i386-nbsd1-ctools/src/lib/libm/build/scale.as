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

	.file "scale.s"
#ifdef __PIC
	.link_pix
#endif
#ifdef __PID
	.pid
#endif
/*
 *  scalef(x,i)
 *  returns x *  2**i 
 *

 *  Input:	x is single (in g0), i is an integer (in g1)
 *  Output:	x *  2**i
 *  CHECK: x in g0, i in g1
 */

	.globl	_fp_scalef
_fp_scalef:
#if defined (_i960XB)
	scaler	g1,g0,g0
#else
	call	___scalesfsisf
#endif
	ret

/*
 *  _fp_scale(x,i)
 *  returns x *  2**i 
 *

 *  Input:	x is double, i is an integer
 *  Output:	x *  2**i
 *****  CHECK: input arguments: x in g0,g1, i in g2 assumed
 */

	.globl	_fp_scale
_fp_scale:
#if defined (_i960XB)
	scalerl	g2,g0,g0
#else
	call	___scaledfsidf
#endif
	ret

/*
 *  scalel(x,i)
 *  returns x *  2**i 
 *

 *  Input:	x is extended_real (in g0-g2), i is an integer (in g4)
 *  Output:	x *  2**i
 *****  CHECK: input arguments: x in g0,g1,g2, i in g4 assumed
 */

	.globl	_fp_scalel
_fp_scalel:
#if defined (_i960XB)
	movre	g0,fp0
	scaler	g4,fp0,fp0
	movre	fp0,g0
#else
	call	___scaletfsitf
#endif
	ret
