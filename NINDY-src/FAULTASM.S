/******************************************************************/
/* 		Copyright (c) 1989, Intel Corporation

   Intel hereby grants you permission to copy, modify, and 
   distribute this software and its documentation.  Intel grants
   this permission provided that the above copyright notice 
   appears in all copies and that both the copyright notice and
   this permission notice appear in supporting documentation.  In
   addition, Intel grants this permission provided that you
   prominently mark as not part of the original any modifications
   made to this software or documentation, and that the name of 
   Intel Corporation not be used in advertising or publicity 
   pertaining to distribution of the software or the documentation 
   without specific, written prior permission.  

   Intel Corporation does not warrant, guarantee or make any 
   representations regarding the use of, or the results of the use
   of, the software and documentation in terms of correctness, 
   accuracy, reliability, currentness, or otherwise; and you rely
   on the software, documentation and results solely at your own 
   risk.							  */
/******************************************************************/

/* Offsets into external '_register_set' array
 * of some important register values
 *
 * UPDATE 'regs.h' IF THESE CHANGE!
 */
	.set	REG_G0,64
	.set	REG_G4,80
	.set	REG_G8,96
	.set	REG_G12,112
	.set	REG_FP,124
	.set	REG_R0,0
	.set	REG_RIP,8
	.set	REG_R4,16
	.set	REG_R8,32
	.set	REG_R12,48
	.set	REG_PC,128
	.set	REG_AC,132
	.set	REG_IP,136
	.set	REG_TC,140


/********************************************************/ 
/* Switch Stack On Fault					
				
   This code is the first code invoked from the fault
   procedure table.  It switches stacks from the application's
   stack to the NINDY stack, then calls the C-level fault
   handler to service the fault.  The stack switch keeps
   NINDY from being intrusive on the user's stack space.

   The following is done:	
				
  1)	All global/local registers at the time of the fault
	are copied into a global structure.  This structure
	is an array of 36 unsigned ints, which will then
	contain g0-g15, r0-r15, ac, pc, tc, and ip.

  2) 	If the fault occurred on the user stack, the fault record
	is copied to the NINDY stack and the stacks are switched,
	linking the new stack back to the old one.

  On entry the (user) stack appears as follows:

		  LOWER MEMORY
	pfp  ->	+-----------------+
		| faulting        |
		|    frame        |
		+-----------------+ <--| fault data starts here
		| resumption      |    |
		| record          |    |
		+-----------------+    |
		| fault record	  |    |
		|                 |    |
	fp  ->	+-----------------+ ---| 
		| this procedure  |
		|                 |
		+-----------------+
		  HIGHER MEMORY

  The fault data is copied over to the NINDY stack, and
  the current stack is switched:

		   LOWER MEMORY
	 	+-----------------+
		| fault info      |
		|                 |
	 fp  ->	+-----------------+
		| this            |
		| procedure       |
	 sp  ->	+-----------------+ 
		| subsequent      |
		| routine's frame |
		| goes here       |
		+-----------------+ 
		   HIGHER MEMORY

  (The next available space on the NINDY stack is determined
  by checking the variable begin_frame, which gets
  set by the 'begin' routine just before it invokes the
  application.)

  When the C-level code returns, the application's registers,
  including the FP, are restored;  a simple 'ret' then
  returns us to the application and its stack.
  
  There is one exception:  if the external flag _user_is_running
  has gone FALSE, the higher-level code detected a user exit.
  Then, instead of returning to the application we pull some
  hanky-panky with the stack to return us to the point
  where the application was first invoked, from the 'begin'
  entry point (see below).

*********************************************************/ 
	.globl _switch_stack_on_fault
_switch_stack_on_fault:

	ldconst	0x001f0000, r8	/* load pc mask        */
	ldconst	0x001f0001, r9	/* load pc mask        */
	modpc	r8, r9, r8	/* set priority to MAX */
				/* to avoid interrupts */
	flushreg		/* make stack current  */

	ldconst	_register_set, r5
	stq 	g0, REG_G0(r5)	/* store global registers */
	stq	g4, REG_G4(r5)	
	stq	g8, REG_G8(r5)	
	stq	g12, REG_G12(r5)
	
	ldconst	0xfffffff0, r13 /* PFP mask for return bits */
	mov	pfp, r6		/* chain back past previous call */
	and	r6, r13, r6	/* mask off return bits  */
	st	r6, REG_FP(r5)	/* store "real" frame ptr*/

	ldq 	REG_R0(r6), r8	/* save user's r0-r3	*/
	stq 	r8, REG_R0(r5)	/* .			*/
	ldq 	REG_R4(r6), r8	/* save user's r4-r7	*/
	stq 	r8, REG_R4(r5)	/* .			*/
	ldq 	REG_R8(r6), r8	/* save user's r8-r11	*/
	stq 	r8, REG_R8(r5)	/* .			*/
	ldq 	REG_R12(r6), r8	/* save user's r12-r15	*/
	stq 	r8, REG_R12(r5)	/* .			*/

	ldconst 0, g14		/* zero g14 for C compiler */

	ldq	-16(fp), r8	/* get pc/ac/ip from fault data */
	st	r8, REG_PC(r5)	/* store pc	        */
	st	r9, REG_AC(r5)	/* store ac		*/
	st	r11, REG_IP(r5)	/* store ip		*/

/* if nested fault, skip stack switch */
	lda	_fault_cnt, r15
	atadd	r15, 1, r8
	cmpibne	0, r8, right_stack 

/* Move resumption record and fault record to NINDY stack */
	ld	begin_frame, g1
	lda	64(g1), g1	/* g1 -> next word on NINDY stack */
	ldq	-64(fp), r8	/* load resumption record	*/
	stq	r8, (g1)	/* store resumption record	*/
	ldq	-48(fp), r8
	stq	r8, 16(g1)
	ldq	-32(fp), r8	
	stq	r8, 32(g1)
	ldq	-16(fp), r8	
	stq	r8, 48(g1)

/* Actual change of our fp and sp to new stack */
	call	swap

right_stack:
	ldconst	0x0ffe00fe, r13	/* load mask                 */
	ldconst	0, r14		/* turn off trace in monitor */
	modtc	r13, r14, r14	/* get old trace controls    */

/* if single step was enabled, the next instruction
 * will automatically disable it by changing the single 
 * step bit in the trace controls to OFF
 */
	notand	r14, 2, r14	/* turn off single step */
	st	r14, REG_TC(r5)	/* and store to memory  */

/* Turn off NIF flag, in case single step was enabled.
 */
	ld	REG_AC(r5), r13	/* get old AC           */
	clrbit	0xf, r13, r13	/* zero out NIF bit 15  */
	st	r13, REG_AC(r5)	/* store back to memory */

/* Save floating pt registers, if any */
	call	_save_fpr

/* From here a C level fault handler will report the fault
 * to the user and invoke the monitor.
 */
	lda	-48(fp), g0	/* pass fault data */
	callx	_fault_handler

	/***********************/
	/* RETURN FROM MONITOR */
	/***********************/

	ldib	_user_is_running, r3
	cmpobne	0, r3, continue
	flushreg
	ld	begin_frame, pfp
	ld	REG_RIP(pfp), rip	/* Cover for CA A-step bug */
	flushreg
	ret
continue:

/* Restore floating pt registers, if any */
	call	_restore_fpr	

/* Restore local registers to application's stack frame */

	flushreg		/* Make stack current, invalidate stack frame*/

	lda	_register_set,r5/* r5 -> saved values of registers */
	ld	REG_FP(r5), r4	/* r4 -> application's frame	*/

	ldq	REG_R0(r5), r8	/* copy r0-r3		*/
	stq	r8, REG_R0(r4)	/* .			*/
	ldq	REG_R4(r5), r8	/* copy r4-r7		*/
	stq	r8, REG_R4(r4)	/* .			*/
	ldq	REG_R8(r5), r8	/* copy r8-r11		*/
	stq	r8, REG_R8(r4)	/* .			*/
	ldq	REG_R12(r5), r8	/* copy r12-r15		*/
	stq	r8, REG_R12(r4)	/* .			*/

/* Copy the (possibly modified) ip into the rip on
 * application's stack frame so execution will resume there.
 *
 * Using the rip as an intermediate register causes it to
 * hold the same value as is stored in the caller's frame,
 * which works around a bug in the A-step 80960CA.
 */
	ld	REG_IP(r5), rip
	st	rip, REG_RIP(r4)


/* Restore global registers directly
 *
 *	Note that g15 is the frame pointer(fp).  DON'T load
 *	g15 directly:  the user's fp will be restored out of
 *	our pfp, so load it there, being careful to preserve
 *	the return type bits (bits 0-3).
 */
	ldq	REG_G0(r5), g0	/* load g0-g3			*/
	ldq	REG_G4(r5), g4	/* load g4-g7			*/
	ldq	REG_G8(r5), g8	/* load g8-g11			*/
	ldt	REG_G12(r5), g12/* load g12-g14			*/
	and	0xf, pfp, pfp	/* Preserve low 4 bits of pfp	*/
	ld	REG_FP(r5), r8	/* r8 = new fp			*/
	andnot	0xf, r8, r8	/* Clear low 4 bits of new fp	*/
	or	r8, pfp, pfp	/* pfp = new fp			*/

/* Restore PC, AC into fault record */

	ld	REG_PC(r5), r9
	st	r9, -16(fp)
	ld	REG_AC(r5), r9
	st	r9, -12(fp)

/* Restore TC directly */

	ld	REG_TC(r5), r14	/* load program trace	*/
	ldconst	0xff01ffff, r7	/* clear event flags	*/
	and	r7, r14, r14	/* .			*/
	ldconst	0x0ffe00fe, r7	/* load TC mask		*/
	modtc	r7, r14, r14	/* set trace controls	*/

/* Decrement fault nesting count*/
	lda	_fault_cnt, r5
	ldconst	-1, r6
	atadd	r5, r6, r8

	ret			/* return to application*/

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

/* Point r3  at callers frame
 * Point pfp at frame on new stack
 */
	mov	pfp, r3
	ld	begin_frame, r4
	lda	128(r4), r4	/* + 64 for nindy frame + 64 for fault record */
	andnot	0xf, r4, r4
	and	0xf, pfp, pfp
	or	r4, pfp, pfp

/* Move copies of 16 registers from old frame to new one
 */
	ldq	(r3), r4	/* Copy r0-r3		*/
	lda	64(pfp), r5	/* Modify caller's sp (r1)*/
	stq	r4, (pfp)
	ldq	16(r3), r4	/* Copy r4-r7		*/
	stq	r4, 16(pfp)
	ldq	32(r3), r4	/* Copy r8-r11		*/
	stq	r4, 32(pfp)
	ldq	48(r3), r4	/* Copy r12-r15		*/
	stq	r4, 48(pfp)

	ret

/************************************************************
 *   begin:
 *	The following routine actually begins execution of an 
 *	application program.  The application entry address
 *	must be passed in g0.
 ************************************************************/

	.globl _begin
_begin:
	ldconst	_register_set, r5
	ld	REG_TC(r5), r14	/* load program trace	*/
	ldconst	0xff01ffff, r7	/* clear event flags	*/
	and	r7, r14, r14	/* .			*/
	ldconst	0x0ffe00fe, r7	/* load TC mask		*/
	modtc	r7, r14, r14	/* set trace controls	*/
	ldconst	1, r7		/* load pc mask		*/
	modpc	r7, r7, r7	/* set trace-enable bit	*/

	st	fp, begin_frame

	ldconst	1,r3
	stib	r3,_user_is_running
	callx	(g0)		/* call application	*/
	ret			/* return to monitor	*/


	/* The frame pointer for the begin routine will be the last
	 * thing on the NINDY stack before the application is invoked
	 * The application will set up and use its own stack.
	 *
	 * If the application returns to NINDY before it exits
	 * (e.g., due to trace or fault or service request), NINDY
	 * should use it's own stack, beginning with the next
	 * available frame above the begin_frame.
	 *
	 * To exit the application, NINDY merely restores the
	 * begin_frame as the current frame and returns to it.
	 */
				
	.bss	begin_frame,4,2
