/*******************************************************************************
 * 
 * Copyright (c) 1993,1994 Intel Corporation
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
/*      environ.c - Floating Point AC Access/Modification Routine (AFP-960)   */
/*									      */
/******************************************************************************/

/**********************************************************************/
/*								      */
/* File "environ.as"						      */
/*								      */
/* This is a list of all the low-level routines used by the C         */
/* functions that modify or access the Arithmetic Controls for the    */
/* 960 processor.  If the underlying processor does not implement     */
/* the floating point bits in its AC register, a memory image         */
/* equivalent to the portion of the 960KB's AC register used for      */
/* floating point is accessed.  Note that these floating point bits   */
/* include the rounding mode bits, the normalization mode bit, the    */
/* floating point exception masks and flags, and the arithmetic       */
/* status bits.                                                       */
/*                                                                    */
/**********************************************************************/

#include "asmopt.h"

.file "environ.as"

#if	defined(USE_SIMULATED_AC)
/* Define address of on-chip RAM word used for Floating Point AC bits */
	.globl	fpem_CA_AC
#endif

	.text
	.link_pix


/*
 *  fp_getround()
 *  gets the current rounding mode
 *

 *  Input: none
 *  Output: integer (current rounding mode)
 */

	.globl	_fp_getround
_fp_getround:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,g0
#else
	modac	0,0,g0
#endif
	shro	30,g0,g0
	ret

/*
 *  fp_setround()
 *  sets the rounding mode specified by the arg and returns the previous one
 *

 *  Input: integer (rounding mode to be set)
 *  Output: integer (previous rounding mode)
 */

	.globl	_fp_setround
_fp_setround:
	ldconst	0xc0000000,g1
	shlo	30,g0,g0
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,g3
	mov	g3,g4
	modify	g1,g0,g4
	st	g4,fpem_CA_AC
	shro	30,g3,g0
#else
	modac	g1,g0,g0
	shro	30,g0,g0
#endif
	ret

/*
 *  fp_getflags()
 *  gets the current exception flags
 *

 *  Input: none
 *  Output: integer (current exception flags)
 */

	.align	MAJOR_CODE_ALIGNMENT

	.globl	_fp_getflags
_fp_getflags:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,g0
#else
	modac	0,0,g0
#endif
	shlo	31-20,g0,g0		/* Top bit in field = 20 */
	shro	32-5,g0,g0		/* Field width = 5 bits */
	ret

/*
 *  fp_setflags()
 *  sets the exception flags as specified by the arg and returns the previous
 *  exception flags
 *  NOTE: Only the flags specified by the argument are set; the other flags
 *	 are preserved

 *  Input: integer (exception flags to be set)
 *  Output: integer (previous exception flags)
 */

	.align	MAJOR_CODE_ALIGNMENT

	.globl	_fp_setflags
_fp_setflags:
	shlo	32-5,g0,g0		/* field width = 5 bits */
	shro	31-20,g0,g0		/* top bit in field = 20 */
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,g3
	or	g0,g3,g4
	st	g4,fpem_CA_AC
	shlo	31-20,g3,g0
#else
	modac	g0,g0,g0
	shlo	31-20,g0,g0
#endif
	shro	32-5,g0,g0
	ret

/*
 *  fp_clrflags()
 *  clears the exception flags as specified by the arg and returns the previous
 *  exception flags
 *  NOTE: Only the flags specified by the argument are cleared; the rest of
 *	 the flags are preserved.

 *  Input: integer (FP exception flags to be cleared)
 *  Output: integer (previous exception flags)
 */

	.align	MAJOR_CODE_ALIGNMENT

	.globl	_fp_clrflags
_fp_clrflags:
	shlo	32-5,g0,g0		/* field width = 5 bits */
	shro	31-20,g0,g0		/* top bit in field = 20 */

#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,g3
	andnot	g0,g3,g4
	st	g4,fpem_CA_AC
	shlo	31-20,g3,g0
#else
	modac	g0,0,g0
	shlo	31-20,g0,g0
#endif
	shro	32-5,g0,g0
	ret

/*
 *  fp_getmasks()
 *  gets the current exception masks
 *

 *  Input: none
 *  Output: integer (current exception masks)
 */

	.align	MAJOR_CODE_ALIGNMENT

	.globl	_fp_getmasks
_fp_getmasks:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,g0
#else
	modac	0,0,g0
#endif
	shlo	31-28,g0,g0		/* top field bit = 20 */
	shro	32-5,g0,g0		/* field width = 5 bits */
	ret

/*
 *  fp_setmasks()
 *  sets the exception masks as specified by the arg and returns the previous
 *  exception masks
 *  NOTE: Only the masks specified by the argument are set; the other exception
 *	 masks are preserved

 *  Input: integer (exception masks to be set)
 *  Output: integer (previous exception masks)
 */

	.align	MAJOR_CODE_ALIGNMENT

	.globl	_fp_setmasks
_fp_setmasks:
	shlo	32-5,g0,g0		/* Field width = 5 bits */
	shro	31-28,g0,g0		/* Top field bit = 28 */

#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,g3
	or	g0,g3,g4
	st	g4,fpem_CA_AC
	shro	31-28,g3,g0
#else
	modac	g0,g0,g0
	shlo	31-28,g0,g0
#endif
	shro	32-5,g0,g0
	ret

/*
 *  fp_clrmasks()
 *  clears the trap masks as specified by the arg and returns the previous
 *  trap masks
 *  NOTE: Only the masks specified by the argument are cleared; the rest of
 *	 the masks are preserved.

 *  Input: integer (FP exception masks to be cleared)
 *  Output: integer (previous exception masks)
 */

	.align	MAJOR_CODE_ALIGNMENT

	.globl	_fp_clrmasks
_fp_clrmasks:
	shlo	32-5,g0,g0		/* field width = 5 bits */
	shro	31-28,g0,g0		/* top bit in field = 28 */

#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,g3
	andnot	g0,g3,g4
	st	g4,fpem_CA_AC
	shlo	31-28,g3,g0
#else
	modac	g0,0,g0
	shlo	31-28,g0,g0
#endif
	shro	32-5,g0,g0
	ret

/*
 *  fp_getenv()
 *  gets the current fp execution environment:
 *  rounding_mode, norm_mode, exception masks and flags
 *

 *  Input: none
 *  Output: integer (current fp execution environment)
 */

	.align	MAJOR_CODE_ALIGNMENT

	.globl	_fp_getenv
_fp_getenv:
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,g0
#else
	modac	0,0,g0		/* masks and flags*/
#endif
	ldconst	0xff1f0000,g1	/* mask for rnd_mode, norm_mode, exception*/
	and	g0,g1,g0
	ret

/*
 *  fp_setenv()
 *  sets the fp execution environment to the argument provided (the
 *  environment includes the exception flags, the exception masks, the
 *  normalizing mode bit, and the rounding mode.
 *

 *  Input: integer (fp execution environment to be set)
 *  Output: integer (previous fp execution environment)
 */

	.align	MAJOR_CODE_ALIGNMENT

	.globl	_fp_setenv
_fp_setenv:
	ldconst	0xff1f0000,g1	/* mask for rnd_mode, norm_mode, exception*/
#if	defined(USE_SIMULATED_AC)
	ld	fpem_CA_AC,g3
	mov	g3,g4
	modify	g1,g0,g4
	st	g4,fpem_CA_AC
	mov	g3,g0
#else
	modac	g1,g0,g0
#endif
	ret


/*
 *  fp_clriflag()
 *  clears the integer overflow flag and returns the previous value
 */

	.align	MAJOR_CODE_ALIGNMENT

	.globl	_fp_clriflag
_fp_clriflag:
	ldconst 0x100,g0
	modac	g0,0,g1
	and	g0,g1,g0
	shro	8,g0,g0	
	ret
