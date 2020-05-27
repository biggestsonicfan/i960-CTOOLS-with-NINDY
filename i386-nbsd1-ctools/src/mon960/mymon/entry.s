/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994 Intel Corporation
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
/*)ce*/
/* This module contains the entry points to the monitor from the application.
 * There are separate entry points for faults, interrupts (including BREAK),
 * the exit system call, unknown system call (reserved but undefined).
 * The fault and interrupt entry points must handle faults and interrupts
 * which occur during the monitor as well as those which occur during the
 * application.  Each of these entry points saves the global registers,
 * performs some processing specific to the type of entry, and branches
 * to switch_stack, which saves the rest of the registers, switches from
 * the application's stack to the monitor's stack, then calls the C-level
 * routine to set the values in the stop record.
 */

	.file	"entry.s"

/* Offsets into external '_register_set' array
 * of some important register values
 * These should change only if hdi_regs.h changes.
 */
	.set	REG_G0, 64
	.set	REG_G4, 80
	.set	REG_G8, 96
	.set	REG_G12, 112
	.set	REG_FP, 124
	.set	REG_R0, 0
	.set	REG_SP, 4
	.set	REG_RIP, 8
	.set	REG_R4, 16
	.set	REG_R8, 32
	.set	REG_R12, 48
	.set	REG_PC, 128
	.set	REG_AC, 132
	.set	REG_TC, 136

	.set	SYS_EXIT_USER, 258
	.set	SYS_GO_USER, 259

	.bss	in_runtime, 4, 2
	.bss	save_priority, 4, 2
	.bss	user_priority, 4, 2

	.set FAULT_RECORD_SIZE, 16
	.globl	_fault_entry
_fault_entry:
	ld	_user_code, r4
	cmpo	0, r4
	be	_fatal_fault

	/*
	 * We must prevent single-stepping into the monitor (except the
	 * runtime support).  The only entry points at which stepping is
	 * possibly enabled are those that can be "calls"ed by the user.
	 */
	/* Check if it was a Trace fault. */
	ld	-8(fp), r4
	extract	16, 8, r4
	cmpobne	0x01, r4, 0f

	/* Get the RIP. */
	flushreg
	andnot	0xf, pfp, r4
	ld	8(r4), r5

	/* Now see if RIP is one of the steppable monitor entry points. */
	lda	_sdm_exit, r4
	cmpobe	r4, r5, 1f
	lda	_mon_entry, r4
	cmpobe	r4, r5, 1f
	lda	_unknown, r4
	cmpobe	r4, r5, 1f
	lda	_fault_entry, r4
	cmpobe	r4, r5, 1f
	b	0f

1:	/* So, this is a Trace fault on one of our entry points. */
	/* Clear the Trace enable bits in TC, and ret, allowing 
	 * "calls" routine to continue without the trace faults. */
	ldconst	0x0ffe00fe, r4
	modtc	r4, 0, r4
	ret

0:	/* Interrupt occurred during user code */
	/* Raise processor priority to 31 and clear trace bit. */
	ldconst	0x001f0000, r8
	or	1, r8, r7
	modpc	r7, r7, r8

	lda	_register_set, r5
	stq 	g0, REG_G0(r5)	/* store global registers */
	stq	g4, REG_G4(r5)	
	stq	g8, REG_G8(r5)	
	stt	g12, REG_G12(r5)

	/* Get the address of the fault record into g1 and get
	 * the pc and ac from the fault record into g4 and g5. */
	lda	-FAULT_RECORD_SIZE(fp), g1
	ldl	(g1), g4

	ldconst	0, g0		/* fault entry */
	b	switch_stack_priority_set


        .globl  _intr_entry
_intr_entry:
/*
 *	if (!user_code)
 *          if (vector_number != break_vector)
 *	        go to fatal_intr
 *	    if (clear_break_condition())
 *              break_flag = TRUE
 *	    return
 *	else if (vector_number == break_vector && !clear_break_condition())
 *	    return
 *	else if (in_runtime)
 *	    break_pending = vector_number
 *	    return
 *	else
 *	    go to switch_stack
 */
	/* Don't use r3 in this routine; it may contain the imsk register */
	ld	_user_code, r4
	cmpobne	0, r4, 2f

	mov	sp, r4
	lda	0x40(sp), sp
	stq	g0, 0(r4)
	stq	g4, 16(r4)
	stq	g8, 32(r4)
	stt	g12, 48(r4)
	mov	0, g14

	ld  	-8(fp), r6
	ld  	_break_vector, r5
	cmpo	r5, r6
	be      0f

	ldconst 0xf8, r5         /* assume nmi is uart break */  
	cmpo	r6, r5
	bne	    _fatal_intr		/* not a interrupt from the uart */
	    			/* This shouldn't really be fatal;
		    		 * the monitor should print a message
			    		 * and ignore the interrupt. */
0:
	call	_clear_break_condition
	cmpobe	0, g0, 1f		/* not caused by a break - ignore */

	/* We have received a break on the uart and we are not in user code.
	 * Set break_flag and return.  */
	ldconst	1, g0
	st	g0, _break_flag

1:
	ldq	0(r4), g0
	ldq	16(r4), g4
	ldq	32(r4), g8
	ldt	48(r4), g12
	ret

2:	/* Interrupt occurred during user code */
	/* Raise processor priority to 31 and clear trace bit. */
	ldconst	0x001f0000, r8
	or	1, r8, r7
	modpc	r7, r7, r8
	st r8, user_priority

	lda	_register_set, r4
	stq	g0, REG_G0(r4)
	stq	g4, REG_G4(r4)
	stq	g8, REG_G8(r4)
	stt	g12, REG_G12(r4)
	mov	0, g14

	ld	-8(fp), r5
	ld	_break_vector, g1
	cmpobe	r5, g1, 8f		/* an interrupt from the uart */

	ldconst 0xf8, g1        /* assume nmi is uart break */  
	cmpobne	r5, g1, 3f		/* not a interrupt from the uart */
	ld	_break_vector, g1
	st   g1, -8(fp)
8:
	call	_clear_break_condition
	cmpobe	0, g0, 4f		/* not caused by a break - ignore */

3:	/* This is an interrupt we want, unless appl is in the runtime */
	ld	in_runtime, g0
	cmpobe	0, g0, 5f		/* not in runtime - enter monitor */
	st	r5, _break_pending	/* in runtime - save interrupt */
					/* to be handled by exit_runtime */

4:	/* This is not an interrupt we want.  Return to the interrupted code. */
	ldq	REG_G0(r4), g0
	ldq	REG_G4(r4), g4
	ldq	REG_G8(r4), g8
	ldt	REG_G12(r4), g12

	/* Reset processor priority and trace control to what it was. */
	ldconst	0x001f0001, r7
	ld user_priority, r8
	modpc	r7, r7, r8
	ret

5:	/* An interrupt occurred during user code (either a break on
	 * the uart or some other (unclaimed) interrupt).
	 * Enter the monitor.  */
	ldt	-16(fp), g4	/* Get the interrupt record for switch_stack */
	mov	r3, g7		/* Save r3, which may contain IMSK */

	/* We want to do an interrupt return, so the monitor doesn't run in
	 * the interrupted state; also if the interrupt was an NMI, further
	 * NMI's are locked out until the interrupt return is done.  Fake up
	 * the stack so we can do the interrupt return without actually
	 * returning to the application; then branch to switch_stack.
	 * Both the ret and the call in this code go to the next instruction.
	 */
	andnot	1, g4, r4	/* Clear the trace enable bit in the saved */
	st	r4, -16(fp)	/* pc, so we don't reenable tracing when we */
				/* do the interrupt return. */
	flushreg
	andnot	0xf, pfp, r4
	ld	8(r4), g3	/* Save rip; this is replaced below */
	lda	6f, r5		/* Fake frame rip */
	st	r5, 8(r4)
	ret			/* Do interrupt return */
6:	call	7f		/* Make new frame for switch_stack */
7:	flushreg
	st	g3, 8(pfp)	/* restore rip */
	ld user_priority, r8
	ldconst	1, g0		/* interrupt entry */
	b	switch_stack_priority_set

	.globl	_enter_runtime
_enter_runtime:
	/* Raise priority to mon_priority if it's greater than the
	 * current priority.  This must be done to avoid hitting a
	 * breakpoint in an interrupt handler while we are in the
	 * runtime.  Only interrupt handlers whose priority is less
	 * than mon_priority should have breakpoints set in them. */
	ld	_mon_priority, r9
	modpc	0, 0, r8
	extract	0x10, 5, r8	/* Get current priority */
	st	r8, save_priority
	cmpoble	r9, r8, 1f
	shlo	0x10, r9, r9
	ldconst	0x001f0000, r8
	modpc	r8, r8, r9
1:

	/* Clear the trace bit so we won't stop in the runtime. */
	/* (This is not effective against INSTRUCTION trace.)	*/
	mov	0, r3
	modpc	1, 1, r3

	mov	1, r3
	st	r3, in_runtime

	/* Call application-specific handler; pass NULL indicating
	 * runtime request rather than breakpoint */
	mov	0, g1
	ldconst	SYS_EXIT_USER, g0
	calls	g0

	call    _timer_suspend

	ret

	.globl	_exit_runtime
_exit_runtime:
	mov	g0, r6

	call    _timer_resume

	/* Call application-specific handler; pass 0 indicating
	 * runtime request is finished */
	mov	0, g1
	ldconst	SYS_GO_USER, g0
	calls	g0

	mov	0, r3
	st	r3, in_runtime

	ld	save_priority, r5	/* Restore priority and enable trace */
	ldconst	0x001f0001, r4
	shlo	0x10, r5, r5
	or	1, r5, r5
	modpc	r4, r4, r5

	/* If an interrupt was saved and ignored by intr_entry while
	 * application was in the runtime, force it to be handled now. */
	ld	_break_pending, g0
	st	r3, _break_pending
	cmpobne	0, g0, _force_interrupt

	/* If flag argument is TRUE, force an interrupt at break_vector */
	cmpobe	0, r6, 1f
	ld	_break_vector, g0
	b	_force_interrupt

1:	ret

	.globl	_force_interrupt
_force_interrupt:
	/*
	 * Jump to the monitor entry point as if an interrupt occurred.
	 * G0 is the interrupt vector number.
	 */
	lda	_register_set, r5
	stq	g0, REG_G0(r5)
	stq	g4, REG_G4(r5)
	stq	g8, REG_G8(r5)
	stt	g12, REG_G12(r5)

	/* create fields as in an interrupt frame */
	/* this is expected by switch_stack */
	modpc	0, 0, g4	/* pcw */
	modac	0, 0, g5	/* ac */
	mov	g0, g6		/* interrupt vector */

/* For the CA, get the imsk register in g7, where switch_stack expects it. */
/* For the Kx, get_imsk is a nop. */
	bal	get_imsk
	mov	g0, g7		/* imsk value */

	/* Don't need to make a new frame here - we are still on the frame for
	 * exit_runtime, which switch_stack will assume is the interrupt frame
	 * which is okay. */
	ldconst	1, g0		/* interrupt entry */
	b	switch_stack


	.globl	_sdm_exit
_sdm_exit:
	ldconst	2, r3		/* exit entry */
	b	1f

	.globl	_mon_entry
_mon_entry:
	ldconst	3, r3		/* general purpose monitor entry */
	b	1f

	.globl	_unknown
_unknown:
	ldconst	4, r3		/* unknown system call */
	b	1f

1:	lda	_register_set, r5
	stq 	g0, REG_G0(r5)	/* store global registers */
	stq	g4, REG_G4(r5)	
	stq	g8, REG_G8(r5)	
	stt	g12, REG_G12(r5)

/* Get pc and ac into g4, g5. */
	modpc	0, 0, g4
	modac	0, 0, g5

	and	6, pfp, g0
	cmpobne	2, g0, 1f	/* If call was made from user mode, */
	andnot	2, g4, g4	/* clear the mode bit (set to user mode), */
	modify	1, pfp, g4	/* and set the trace bit if it was set
				 * before the call */
1:
	mov	r3, g0		/* type of monitor entry call */
	b	switch_stack


/* This is the main entry point to the monitor from the application.
 * All the various entry points branch here.  They have already
 * saved the global registers.
 * G0 indicates the type of monitor entry:  0 = fault, 1 = interrupt,
 *    2 = exit, 3 = unknown system call.
 * G1 points to the the fault record.
 * G4 and g5 contain the PC and AC.
 * G6 contains the interrupt vector.
 * G7 contains the value of r3 (which may be the value of IMSK)
 */
	.globl switch_stack
switch_stack:
	/* Raise processor priority to 31 and clear trace bit. */
	ldconst	0x001f0000, r8
	or	1, r8, r7
	modpc	r7, r7, r8

	/* entry if modpc already done */
	/* r8 contains the pc returned value */
switch_stack_priority_set:
	/* Now set priority to mon_priority or to application's
	 * current priority, whichever is greater. */
	ld	_mon_priority, r9
	extract	0x10, 5, r8	/* Get application's current priority */
	cmpobge	r9, r8, 1f
	mov	r8, r9		/* If current priority is greater, use that */
1:	shlo	0x10, r9, r9
	ldconst	0x001f0000, r8
	modpc	r8, r8, r9

	ldconst 0, g14		/* zero g14 for C compiler */
	flushreg		/* make stack current  */
	lda	_register_set, r5
	st	g14, _user_code	/* This must be >= 4 instructions after modpc */

	andnot	0xf, pfp, r6	/* mask return status bits */
	st	r6, REG_FP(r5)	/* store frame ptr	*/

	ldq 	REG_R0(r6), r8	/* save user's r0-r3	*/
	stq 	r8, REG_R0(r5)	/* .			*/
	ldq 	REG_R4(r6), r8	/* save user's r4-r7	*/
	stq 	r8, REG_R4(r5)	/* .			*/
	ldq 	REG_R8(r6), r8	/* save user's r8-r11	*/
	stq 	r8, REG_R8(r5)	/* .			*/
	ldq 	REG_R12(r6), r8	/* save user's r12-r15	*/
	stq 	r8, REG_R12(r5)	/* .			*/

	ldconst	0x0ffe00fe, r13	/* load mask for trace controls */
	modtc	r13, 0, r14	/* clear trace controls, and get old value */
	st	r14, REG_TC(r5)	/* to store to memory  */

	ldconst	0x3f001000, r13	/* load mask for arithmetic controls */
	modac	r13, r13, r13	/* mask floating point and integer faults */

/* Store the pc and ac from g4 and g5.  They were loaded from the fault record
 * or interrupt record.  */
	st	g4, REG_PC(r5)	/* store pc	        */
	clrbit	15, g5, g5	/* zero out NIF, bit 15 */
	st	g5, REG_AC(r5)	/* store ac		*/

/* Actual change of our fp and sp to new stack */
/* NOTE: local registers are NOT preserved across this call! */
	call	swap

/* Save values in globals before calling external functions. */
	mov	g0, r8
	mov	g1, r9
	mov	g6, r10

/* Save special function registers, if any */
/* Pass in stop cause and value of r3 so it can restore IMSK if necessary */
	mov	g6, g1
	mov	g7, g2
	call	save_sfr

/* Save floating point registers, if any */
	call	save_fpr

/* Clear flag to indicate the register set came from the user, not the host
 * or monitor. */
	st	g14, _default_regs

/* Call a C routine to fill in the stop record.  */
	cmpobne	0, r8, 1f
	mov	r9, g0		/* pass fault record */
	call	_fault_handler
	b	9f
1:
	cmpobne	1, r8, 2f
	mov	r10, g0		/* pass interrupt vector */
	call	_intr_handler
	b	9f
2:
	mov	r8, g0
	call	_exit_handler
9:
	mov	g0, r3		/* save pointer to stop record */

	lda	(g0), g1	/* Pass pointer to stop record to */
	ldconst	SYS_EXIT_USER, g0
	calls	g0		/* application-specific stop handler */


	/* pass stop record returned by _fault_handler, _intr_handler,
	 * or _exit_handler to monitor */
	mov	r3, g0
	b	_exit_user


/*********************************************************************
 * The following routine actually swaps the fault handler's stack, 
 * by copying its frame to the new stack, changing the pfp to point
 * to the copied frame, and returning to it.  Along the way, the sp of
 * the caller must be changed to allocate the frame below the new fp.
 *
 * It's necessary to do this as a subroutine, because a routine cannot
 * change it's own frame on the i960:  it will pass the modified
 * frame pointer on to the called routine (as its pfp), but it will
 * store its registers to the address the fp contained on entry
 *********************************************************************/
swap:
	flushreg
	ld	8(pfp), r6	/* save caller's rip (r2) */

	/* Point pfp at frame on new stack */
	/* i960 core EAS 7.3.3 flush reg */
	flushreg
	lda	_monitor_stack, pfp
	flushreg

	ldconst	0, r4		/* Set caller's pfp (r0) */
	lda	64(pfp), r5	/* Set caller's sp (r1) */

	stt	r4, (pfp)
	ret


/************************************************************
 *   int check_stack_size():
 *	The following routin checks for stack overflow.
 *     retuns stack size  
 ************************************************************/
    .set MON_STACK_SIZE, 0x1000 
	.text
	.globl _check_stack_size
_check_stack_size:
	lda _monitor_stack, g0
	lda MON_STACK_SIZE, g1
	subo g0, sp, g0
	subo g0, g1, g0
	ret


/************************************************************
 *   exit_mon:
 *	The following routine actually begins execution of an 
 *	application program.
 ************************************************************/

	.text
	.globl _exit_mon
_exit_mon:
	mov	g0, r15		/* Save shadow parameter */

	ldconst	0x001f0000, r4	/* Raise priority to 31; we don't  */
	modpc	r4, r4, r4	/* want interrupts when we restore */
				/* the IMSK register.  Priority is */
				/* restored by the 'ret'.          */

	mov	1, g1		/* Pass 1 indicating go rather than */
	ldconst	SYS_GO_USER, g0 /* the end of a runtime request.    */
	calls	g0

/* Restore floating point registers, if any */
	call	restore_fpr	

/* Restore special function registers, if any */
	call	restore_sfr

/* Flush processor instruction cache.  It may be inconsistent due to
 * new code loaded, patches, or new or removed software breakpoints. */
	call	flush_cache

/* Copy application's local registers to its stack frame */
	flushreg		/* required before changing pfp */
	lda	_register_set, r5    /* r5 -> saved values of registers */

	ld	REG_FP(r5), r4	/* r4 -> application's frame	*/
	andnot	0xf, r4, r4

	flushreg		/* required before changing pfp */
	or	1, r4, pfp	/* set return status of pfp to fault return */
	flushreg		/* required after changing pfp i960 eas 7.3.3 */

/* Set fp to a frame above the user sp, leaving room for the fault record. */
	ld	REG_SP(r5), r8	/* Get user sp */
	ldconst	0x3f, r9
	lda	0x50(r8), r8	/* Leave room for fault record, and */
	andnot	r9, r8, fp	/* round up to 0x40 byte boundary.  */
	lda	0x40(fp), sp

	ldq	REG_R0(r5), r8	/* copy r0-r3 to	*/
	stq	r8, REG_R0(r4)	/* application's frame. */
	ldq	REG_R4(r5), r8	/* copy r4-r7		*/
	stq	r8, REG_R4(r4)	/* .			*/
	ldq	REG_R8(r5), r8	/* copy r8-r11		*/
	stq	r8, REG_R8(r4)	/* .			*/
	ldq	REG_R12(r5), r8	/* copy r12-r15		*/
	stq	r8, REG_R12(r4)	/* .			*/

/* Restore global registers directly
 *	Note that g15 is the frame pointer(fp).  DON'T load
 *	g15 directly:  the user's fp will be restored out of
 *	our pfp, loaded above.
 */
	ldq	REG_G0(r5), g0	/* load g0-g3			*/
	ldq	REG_G4(r5), g4	/* load g4-g7			*/
	ldq	REG_G8(r5), g8	/* load g8-g11			*/
	ldt	REG_G12(r5), g12/* load g12-g14			*/

/* Restore PC, AC into fault record */
	ld	in_runtime, r3
	cmpo	0, r3
	teste	r3

	ld	REG_PC(r5), r8
	or	r3, r8, r8	/* enable trace if we're not in runtime*/
	ld	REG_AC(r5), r9
	stl	r8, -16(fp)

/* Restore TC directly */
	ld	REG_TC(r5), r14	/* load TC - only low byte is relevant */
	extract	0, 8, r14
	ldconst 0x0ffe00fe, r7  /* TC mask		*/
	modtc	r7, r14, r14	/* set trace controls	*/

	ldconst	1, r4
	st	r4, _user_code

	cmpobe	0, r15, 1f	/* Check shadow flag */
	ld	_shadow, r15	/* Check if board supports shadow */
	cmpobe	0, r15, 1f
	balx	(r15), r15	/* Call shadow if requested and supported */
1:	ret			/* return to application*/

    
	.globl	_shadow
	.bss	_shadow, 4, 2
	.globl	brksav
	.bss	brksav, 4, 2
	.globl	_monitor_stack
	.bss	_monitor_stack, MON_STACK_SIZE, 6	# monitor stack 
