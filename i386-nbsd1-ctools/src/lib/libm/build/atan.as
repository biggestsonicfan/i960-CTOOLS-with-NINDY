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



	.file "atan.s"
	.link_pix

/*
 *  atanf(x)
 *  returns arctangent(x)
 *
 *
 *  Input:	x single
 *  Output:	atan(x)
 */

	.globl	_atanf
_atanf:
	atanr	0f1.0,g0,g0	
	ret

/*
 *  _atan(x)  Unix libm entry
 *  returns arctangent(x)
 *
 *
 *  Input:	x double
 *  Output:	atan(x)
 */

	.globl	_atan
_atan:
	atanrl	0f1.0,g0,g0	
	ret

/*
 *  atanl(x)
 *  returns arctangent(x)
 *
 *
 *  Input:	x long double
 *  Output:	atan(x)
 */

	.globl	_atanl
_atanl:
	movre	g0,fp0
	atanr	0f1.0,fp0,fp0	
	movre	fp0,g0
	ret

/*
 *  atan2f(x,y)
 *  returns arctangent(x/y)
 *

 *  Input:	x single, y single
 *  Output:	atan(x/y) in single
 */

	.globl	_atan2f
_atan2f:
	cmpr	0f0.0,g1
	bne	Latan2f_no_edom
	cmpr	0f0.0,g0
	bne	Latan2f_no_edom
	callj	__errno_ptr
	ldconst	33,r10
	st	r10,(g0)		# errno = EDOM
	mov	0,g0
	ret
Latan2f_no_edom:
	atanr	g1,g0,g0	
	ret


/*
 *  _atan2(x,y) Unix libm entry
 *  returns arctangent(x/y) in double
 *  NOTE change in operand order
 *

 *  Input:	x double, y double
 *  Output:	atan(x/y) in double
 */

	.globl	_atan2
_atan2:
	cmprl	0f0.0,g2
	bne	Latan2_no_edom
	cmprl	0f0.0,g0
	bne	Latan2_no_edom
	callj	__errno_ptr
	ldconst	33,r10
	st	r10,(g0)		#errno = EDOM
	mov	0,g0
	ret
Latan2_no_edom:
	atanrl	g2,g0,g0	
	ret

/*
 *  atan2l(x,y)
 *  returns arctangent(x/y)
 *
 
 *  Input:	x extended, y extended
 *  Output:	atan(x/y) in extended
 *  NOTE change in operand order
 */

	.globl	_atan2l
_atan2l:
	movre	g0,fp0
	movre	g4,fp1
	cmpr	0f0.0,fp1
	bne	Latan2l_no_edom
	cmpr	0f0.0,fp0
	bne	Latan2l_no_edom
	callj	__errno_ptr
	ldconst	33,r10
	st	r10,(g0)		#errno = EDOM
	mov	0,g0
	ret
Latan2l_no_edom:
	atanr	fp1,fp0,fp0	
	movre	fp0,g0
	ret

/*
 * The fp_satan2 is a version of atan2(y/x) where x is always in extended
 * precision.
 */

/*
 *  _Lsatan2f(x,y)
 *  returns arctangent(y/x)
 *

 *  Input:	x long double (in g0-g2), y single (in g4)
 *  Output:	atan(y/x) in single
 */

	.globl	__Lsatan2f
__Lsatan2f:
	movre	g0,fp0
	atanr	fp0,g4,g0	
	ret


/*
 *  _Lsatan2(x,y)
 *  returns arctangent(y/x)
 *

 *  Input:	x long double (in g0-g2), y double (in g4-g5)
 *  Output:	atan(y/x) in double
 */

	.globl	__Lsatan2
__Lsatan2:
	movre	g0,fp0
	atanrl	fp0,g4,g0
	ret

/*
*  _Lsatan2l(x,y)
*  returns arctangent(y/x)
*

*  Input:	x,y are extended_real
*  Output:	atan(y/x) in extended
*/

	.globl	__Lsatan2l
__Lsatan2l:
	movre	g0,fp0
	movre	g4,fp1
	atanr	fp0,fp1,fp0
	movre	fp0,g0
	ret

/*
 * fp_ratan2 is a version of atan2(x/y) where x is always in extended
 * precision.
 */

/*
*  _Lratan2f(x,y)
*  returns arctangent(x/y)
*  NOTE the change in operand order.

*  Input:	x long double, y single (in g4)
*  Output:	atan(x/y) in single
*/

	.globl	__Lratan2f
__Lratan2f:
	movre	g0,fp0
	atanr	g4,fp0,g0	
	ret


/*
*  _Lratan2(x,y)
*  returns arctangent(x/y)
*

*  Input:	x long double, y double
*  Output:	atan(x/y) in double
*  NOTE change in operand order.
*/

	.globl	__Lratan2
__Lratan2:
	movre	g0,fp0
	atanrl	g4,fp0,g0
	ret



/*
*  _Lratan2l(x,y)
*  returns arctangent(x/y)
*

*  Input:	x,y are extended_real
*  Output:	atan(x/y) in extended real
*  NOTE change in operand order.
*/

	.globl	__Lratan2l
__Lratan2l:
	movre	g0,fp0
	movre	g4,fp1
	atanr	fp1,fp0,fp0
	movre	fp0,g0
	ret

