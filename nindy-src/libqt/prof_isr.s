
/******************************************************************
 *
 * 		Copyright (c) 1989, Intel Corporation
 *
 * Intel hereby grants you permission to copy, modify, and 
 * distribute this software and its documentation.  Intel grants
 * this permission provided that the above copyright notice 
 * appears in all copies and that both the copyright notice and
 * this permission notice appear in supporting documentation.  In
 * addition, Intel grants this permission provided that you
 * prominently mark as not part of the original any modifications
 * made to this software or documentation, and that the name of 
 * Intel Corporation not be used in advertising or publicity 
 * pertaining to distribution of the software or the documentation 
 * without specific, written prior permission.  
 *
 * Intel Corporation does not warrant, guarantee or make any 
 * representations regarding the use of, or the results of the use
 * of, the software and documentation in terms of correctness, 
 * accuracy, reliability, currentness, or otherwise; and you rely
 * on the software, documentation and results solely at your own 
 * risk.
 *
 ******************************************************************/

	.text
	.align	4
	.globl _profile_isr

_profile_isr:

	/*
	 * Store global registers to avoid corruption.
	 */
	lda	64(sp), sp
	stq	g0, -64(sp)
	stq	g4, -48(sp)
	stq	g8, -32(sp)
	stq	g12, -16(sp)
	flushreg
	notand	pfp, 0xf, r3

	/*
	 * Zero g14 in preparation for the 'C' function call.
	 * It must be zero unless we have an extra long argument
	 * list, and we don't know what it was prior to interrupt.
	 */
	mov	0, g14

	/*
	 * Get the IP of the interrupted instruction, then call
	 * the function which translates it into a bucket hit.
	 */
	ld	8(r3), g0
	callx	_store_prof_data

	/*
	 * Restore global registers.
	 */
	ldq	-64(sp), g0
	ldq	-48(sp), g4
	ldq	-32(sp), g8
	ldq	-16(sp), g12
	ret
