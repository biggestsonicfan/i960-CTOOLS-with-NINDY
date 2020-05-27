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

	.file "fp_exits.s"
#ifdef __PIC
	.link_pix
#endif
#ifdef __PID
	.pid
#endif
/*
 *  _Lfpe_exit() : returns extended_real value
 *

 *  An entry point for exiting without any check for exceptions is available:
 *  fpquickexit: this entry point can be called if we need to just restore the
 *		       previous fp environment and exit (i.e. no checks for
 *		       exceptions will occur)
 *

 *  Input: integer: g4 = previous fp environment, 
 *			  g5 = FPSL function code
 *			  g0,g1,g2 = extended_real value of the FPSL function to 
 *			  be returned
 *  Output: the (extended_real) value of the function
 */

	.globl	__Lfpe_exit
__Lfpe_exit:
#if defined(_i960KX)
	modac	0,0,r4
#else
	movt	g0,r8
	movl	g4,r12
	bal	_fp_getenv
	mov	g0,r4
	movt	r8,g0
	movl	r12,g4
#endif
	shlo	11,r4,r4
	shro	27,r4,r4	/* current exception flags */
	shlo	3,g4,g6
	shro	27,g6,g6	/* initial exception masks*/
	andnot	g6,r4,g6   /* to determine if exception is to be raised..*/
	cmpo	0,g6
	shlo	16,r4,r4	/* get the exception flags into position */
	bne	Lraise_except	/* for the previous compare */
	or	g4,r4,g4   /* propagate the exceptions of this function */

	.globl	__Lquickexitl
__Lquickexitl:
	lda	0xff1f0000,r4
#if defined(_i960KX)
	modac	r4,g4,g4	/* restore previous fp environment */
#else
	movt	g0,r8
	mov	r4,g0
	mov	g4,g1
	bal	__setac
	movt	r8,g0
#endif
	ret

/*
 *  _Lfpd_exit()
 *

 *  Input: integer: g2 = previous fp environment, 
 *			  g3 = FPSL function code
 *			  g0,g1 = double_real value of the FPSL function to 
 *			  be returned
 *  Output: the (extended_real) value of the function
 */

	.globl	__Lfpd_exit
__Lfpd_exit:
#if defined(_i960KX)
	modac	0,0,r4
#else
	movl	g0,r8
	movl	g2,r12
	bal	_fp_getenv
	mov	g0,r4
	movl	r8,g0
	movl	r12,g2
#endif
	shlo	11,r4,r4
	shro	27,r4,r4	/* current exception flags */
	shlo	3,g2,g4
	shro	27,g4,g4	/* initial exception masks */
	andnot	g4,r4,g4   /* to determine if exception is to be raised.. */
	cmpo	0,g4
	shlo	16,r4,r4	/* get the exception flags into position */
	bne	Lraise_except	/* for the previous compare*/
	or	g2,r4,g2  /* propagate the exceptions of this function */

	.globl	__Lquickexit
__Lquickexit:
	lda	0xff1f0000,r4
#if defined(_i960KX)
	modac	r4,g2,g2	/* restore previous fp environment */
#else
	movl	g0,r8
	mov	r4,g0
	mov	g2,g1
	bal	__setac
	movl	r8,g0
#endif
	ret

/*
 *  _Lfps_exit()
 *

 *  Input: integer: g1 = previous fp environment,
 *			  g2 = FPSL function code
 *			  g0 = (float) value of the FPSL function to 
 *			  be returned
 *  Output: the (float) value of the function
 */

	.globl	__Lfps_exit
__Lfps_exit:
#if defined(_i960KX)
	modac	0,0,r4
#else
	mov	g0,r8
	mov	g1,r12
	mov	g2,r13
	bal	_fp_getenv
	mov	g0,r4
	mov	r8,g0
	mov	r12,g1
	mov	r13,g2
#endif
	shlo	11,r4,r4
	shro	27,r4,r4	/* current exception flags */
	shlo	3,g1,g3
	shro	27,g3,g3	/* initial exception masks */
	andnot	g3,r4,g3  /* to determine if exception is to be raised.. */
	cmpo	0,g3
	shlo	16,r4,r4	/* get the exception flags into position */
	bne	Lraise_except	/* for the previous compare */
	or	g1,r4,g1   /* propagate the exceptions of this function */

	.globl	__Lquickexitf
__Lquickexitf:
	lda	0xff1f0000,r4
#if defined(_i960KX)
	modac	r4,g1,g1	/* restore previous fp environment */
#else
	mov	g0,r8
	mov	r4,g0
	bal	__setac
	mov	r8,g0
#endif
	ret

/*
 *  _Lfaultexitf()
 *  This routine raises the exception (if enabled) passed as a parameter
 *  to the function. Otherwise, the exception flag is or'ed into the previous
 *  fp environment before exiting.
 *

 *  Input: integer: g2 = previous fp environment,
 *			  g1 = exception to be raised,
 *			  g3 = FPSL function code,
 *			  g0 = (float) value of the FPSL function to 
 *			  be returned
 *  Output: the (float) value of the function
 */

	.globl	__Lfaultexitf
__Lfaultexitf:
	shlo	3,g2,r4
	shro	27,r4,r4	/* initial exception masks */
	andnot	r4,g1,r4  /* to determine if exception is to be raised.. */
	cmpo	0,r4
	shlo	16,g1,g1	/* get the exception flags into position */
	bne	Lraise_except	/* for the previous compare */
	or	g1,g2,g2
	lda	0xff1f0000,r4
#if defined(_i960KX)
	modac	r4,g2,g2	/* restore previous fp environment */
#else
	mov	g0,r8
	mov	r4,g0
	mov	g2,g1
	bal	__setac
	mov	r8,g0
#endif
	ret

/*
 *  __Lfaultexit()
 *
 *
 *  Input: integer: g3 = previous fp environment,
 *			  g2 = exception to be raised,
 *			  g4 = FPSL function code,
 *			  g0,g1 = (double) value of the FPSL function to 
 *			  be returned
 *  Output: the (double) value of the function
 */

	.globl	__Lfaultexit
__Lfaultexit:
	shlo	3,g3,r4
	shro	27,r4,r4	/* initial exception masks */
	andnot	r4,g2,r4  /* to determine if exception is to be raised.. */
	cmpo	0,r4
	shlo	16,g2,g2	/* get the exception flags into position */
	bne	Lraise_except	/* for the previous compare */
	or	g2,g3,g3
	lda	0xff1f0000,r4
#if defined(_i960KX)
	modac	r4,g3,g3	/* restore previous fp environment */
#else
	movl	g0,r8
	mov	r4,g0
	mov	g3,g1
	bal	__setac
	movl	r8,g0
#endif
	ret

/*
 *  _Lfaultexitl()
 *

 *  Input: integer: g5 = previous fp environment,
 *			  g4 = exception to be raised,
 *			  g6 = FPSL function code,
 *			  g0,g1,g2 = (ext_real) value of the FPSL function to 
 *			  be returned
 *  Output: the (extended) value of the function
 */


	.globl	__Lfaultexitl
__Lfaultexitl:
	shlo	3,g5,r5
	shro	27,r5,r5	/* initial exception masks */
	andnot	r5,g4,r5  /* to determine if exception is to be raised.. */
	cmpo	0,r5
	shlo	16,g4,g4  /* get the exception flags into position */
	bne	Lraise_except	/* for the previous compare */
	or	g4,g5,g5
	lda	0xff1f0000,r5
#if defined(_i960KX)
	modac	r5,g5,g5	/* restore previous fp environment */
#else
	movt	g0,r8
	mov	r5,g0
	mov	g5,g1
	bal	__setac
	movt	r8,g0
#endif
	ret

/*
 *  _Lfpi_exit() : returns integer value
 *  exits the FPSL function and does the following:
 *  if any exception flags are set and the exception enabled, goes to
 *  the exception-handling code ("raise_exeception")
 *  else the exception flags are or'ed in with the initial a_ctr (the input
 *  arg) to form the new fp environment (a_ctr)
 *

 *  An entry point for exiting without any check for exceptions is available:
 *  fpquickexit: this entry point can be called if we need to just restore the
 *		       previous fp environment and exit (i.e. no checks for
 *		       exceptions will occur)
 *

 *  Input: integer: g1 = previous fp environment, 
 *			  g2 = FPSL function code
 *			  g0 = value of the FPSL function to be returned (integer)
 *  Output: the (integer) value of the function
 */

	.globl	__Lfpi_exit
__Lfpi_exit:
#if defined(_i960KX)
	modac	0,0,r4
#else
	mov	g0,r8
	mov	g1,r12
	mov	g2,r13
	bal	_fp_getenv
	mov	g0,r4
	mov	r8,g0
	mov	r12,g1
	mov	r13,g2
#endif
	modac	0,0,r4
	shro	27,r4,r4	/* current exception flags */
	shlo	3,g1,g3
	shro	27,g3,g3	/* initial exception masks */
	andnot	r4,g3,g3   /* to determine if exception is to be raised.. */
	cmpo	0,g3
	shlo	16,r4,r4	/* get the exception flags into position */
	bne	Lraise_except   	/* for the previous compare */ 
	or	g1,r4,g1   /* propagate the exceptions of this function */

	.globl	__Lfpi_quickexit
__Lfpi_quickexit:
	lda	0xff1f0000,r4
#if defined(_i960KX)
	modac	r4,g1,g1	/* restore previous fp environment */
#else
	mov	g0,r8
	mov	r4,g0
	bal	__setac
	mov	r8,g0
#endif
	ret

Lraise_except:	/* Interface to Jimv's code */
	ret		/* assuming this is a "dummy" for other formats */
