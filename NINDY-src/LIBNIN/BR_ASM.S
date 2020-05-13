
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
	.globl __init_branch
	.globl __inc_branch
	.globl __alt_rip


	/*
	 * This function is necessary to avoid a CA A-step bug
	 * which makes RIP alteration a difficult task.
	 */
__alt_rip:

	/*
	 * Use the current frame's pfp get the previous frame's
	 * pfp.
	 */
	flushreg
	notand	pfp, 0xf, r3
	ld	(r3), r4
	notand	r4, 0xf, r4

	/*
	 * Use the previous frame's pfp to get it's RIP, then bump
	 * the RIP one word to avoid a branch table address or a
	 * branch counter. Don't forget to clear the low order two
	 * bits of the RIP to avoid yet-another CA bug.
	 */
	ld	8(r4), r5
	notand	r5, 0x3, r5
	addo	r5, 4, r5
	st	r5, 8(r4)
	ret


__init_branch:

	/*
	 * Store global registers to avoid corruption.
	 */
	lda	64(sp), sp
	stq	g0, -64(sp)
	stq	g4, -48(sp)
	stq	g8, -32(sp)
	stq	g12, -16(sp)
	flushreg

	/*
	 * Pass the contents of memory at the RIP, which actually does
	 * not contain an instruction but the branch table address, to
	 * the function which will link together the branch tables from
	 * each module.
	 */
	notand	pfp, 0xf, r3
	ld	8(r3), g1
	notand	g1, 0x3, g1
	ld	(g1), g0
	call	___branch_store

	/*
	 * Call the function which will modify our RIP so when we
	 * return we skip past the branch table address.
	 */
	call	__alt_rip

	/*
	 * Restore global registers and return. We should now
	 * return to the instruction just past the branch table word.
	 */
	ldq	-64(sp), g0
	ldq	-48(sp), g4
	ldq	-32(sp), g8
	ldq	-16(sp), g12
	ret


__inc_branch:

	/*
	 * The function's RIP actually points to the branch's
	 * counter word. Increment that word by one.
	 */
	flushreg
	notand	pfp, 0xf, r3
	ld	8(r3), r4
	notand	r4, 0x3, r4
	ld	(r4), r5
	addo	r5, 1, r5
	st	r5, (r4)

	/*
	 * Call the function which will actually modify the RIP
	 * so when we return we skip past the branch counter.
	 */
	call	__alt_rip
	ret
