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

	.file "rmd.s"
#ifdef __PIC
	.link_pix
#endif
#ifdef __PID
	.pid
#endif
/*
 *  rmdf(x,y)	CHECK if OK
 *  returns the ieee remainder 
 *  NOTE: This is the entry-point for the RMD function and not a support
 *		routine
 *

 *  Input:	x,y are single
 *  Output:	ieee rmd
 */

	.globl _fp_rmdf
_fp_rmdf:
#if defined (_i960XB)
	remr	g1,g0,g0
	modac	0,0,r4
	chkbit	4,r4
	bne	2f		/* QR=0  =>  g0 < y/2  =>  z = g0 */
	chkbit	3,r4
	be	1f		/* QR=1,QS=1  =>  g0 > y/2  =>  z = g0 - y */
	chkbit	5,r4
	bne	2f		/* Q0=0,QR=1,QS=0  =>  g0 = y/2  =>  z = g0 */
1:	clrbit	31,g1,g1
	subr	g1,g0,g0
#else
	call ___rmdsf3
#endif
2:	ret


/*
 *  _fp_rmd(x,y)		
 *  returns the ieee remainder 
 *  NOTE: This is the entry-point for the RMD function and not a support
 *		routine
 *

 *  Input:	x,y are double
 *  Output:	ieee rmd
 */

	.globl _fp_rmd
_fp_rmd:
#if defined (_i960XB)
	remrl	g2,g0,g0
	modac	0,0,r4
	chkbit	4,r4
	bne	2f		/* QR=0  =>  g0 < y/2  =>  z = g0 */
	chkbit	3,r4
	be	1f		/* QR=1,QS=1  =>  g0 > y/2  =>  z = g0 - y*/
	chkbit	5,r4
	bne	2f		/* Q0=0,QR=1,QS=0  =>  g0 = y/2  =>  z = g0*/
1:	clrbit	31,g3,g3
	subrl	g2,g0,g0
#else
	call ___rmddf3
#endif
2:	ret

/*
 *  rmdl(x,y)
 *  returns the ieee remainder 
 *  NOTE: This is the entry-point for the RMD function and not a support
 *		routine
 *

 *  Input:	x,y are extended_real
 *  Output:	ieee rmd
 */

	.globl _fp_rmdl
_fp_rmdl:
#if defined (_i960XB)
	movre	g0,fp0
	movre	g4,fp1
	remr	fp1,fp0,fp0
	modac	0,0,r4
	chkbit	4,r4
	bne	2f	/* QR=0  =>  fp0 < y/2  =>  z = fp0 */
	chkbit	3,r4
	be	1f 	/* QR=1,QS=1  =>  fp0 > y/2  =>  z = fp0 - y */
	chkbit	5,r4
	bne	2f	/* Q0=0,QR=1,QS=0  =>  fp0 = y/2  =>  z = fp0*/
1:	cpysre	fp1,0f1.0,fp1
	subrl	fp1,fp0,fp0
	movre	fp0,g0
#else
	call ___rmdtf3
#endif
2:	ret
