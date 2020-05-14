/******************************************************************/
/* 		Copyright (c) 1989, 1990 Intel Corporation

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
	.text
	.globl start
	.globl __exit

#ifdef PROFILE
        .globl _default_pstart
    _default_pstart:
#endif

start:
	mov	1, r12
	modpc	r12, r12, r12	/* enable tracing */
	mov	g5, g5

#ifdef FLASH
	/* For Flash version, copy the .data section from Flash to RAM.
	 * (When the code is burned into Flash, the data area is placed
	 * immediately following the executable code.)
	 * This must be done first before anything else to
	 * insure that the global variables are initialized.
	 */
        lda     _ram, r4    /* r4-> next word of RAM to load into  */
        lda     _etext, r6  /* r6-> next word of data to load from */
        lda     _edata, r5  /* r5-> first word past end of data    */

move_floop:
        cmpobge	r4, r5, zerobss /* see if done             */
        ld	(r6), r7        /* load data word from ROM */
        st	r7, (r4)        /* store data to memory    */
        addo	4, r6, r6       /* increment pointer       */
        addo	4, r4, r4       /* increment destination   */
        b	move_floop
#endif

	/*
	 * zero out uninitialized data area
	 */
zerobss:
	lda 	_end, r4	/* find end of .bss */
	lda 	_bss_start, r5	/* find beginning of .bss */
	ldconst 0, r6		

loop:	st	r6, (r5)	/* to zero out uninitialized */
	addo	4, r5, r5	/* data area                 */
	cmpobl	r5, r4, loop	/* loop until _end reached   */

/* set up stack pointer:
 *	The heap will begin at '_end';  its length is 'heap_size'
 *	bytes.  The stack will begin at the first 64-byte-aligned
 *	block after the heap.
 *
 *	A default value of 'heap_size' is set by linking with libnindy.a
 *	The default can be overridden by redefining this symbol at link
 *	time (with a line of the form 'heap_size=XXXX;' in the lnk960
 *	linker specification file; or one of the form
 *	"-defsym heap_size=XXXX" on the gld960 invocation line).
 */

	ldconst	_end, sp	 /* set sp = address of end of heap */
	lda	heap_size(sp),sp
	lda	64(sp), sp	 /* Now round up to 64-byte boundary */
	ldconst 0xffffffc0, r12
	and     r12, sp, sp
	st	sp, _stack_start /* Save for brk() routine */

	call	init_frames
	ret			 /* return to monitor */

init_frames:
	mov	0, g14		 /* initialize constant for C */
	ldconst	0x3b001000, g0
	ldconst	0x00009107, g1
	modac	g1, g0, g0	 /* set AC controls */

	/*
	 * insert "C" startup code 
	 */
	callx _init_c

	/*
	 * remember the frame, so that we can set it up if necessary
	 */

	st	fp, _start_frame


#ifdef PROFILE
	/*
	 * Call profiler initialization function.
	 */
	callx _init_profile
#endif

	/*
	 * Call application mainline.
	 *	Someday, real values of argc and argv will be set up.
	 *	For now, they are set to 0.
	 */
	ldconst	0,g0
	ldconst	0,g1
	callx 	_main

	/* 
	 * if we return from main, we have "fallen" off the end
	 * of the program, therefore status is 0
	 * so move 0 to g0 (exit parameter)
	 */
	mov	0, g0
	callx	_exit

return_to_nindy:
	ret			/* _exit() will return to this point */


__exit:

#ifdef BR_PREDICT
	/*
	 * Call the branch-prediction data dump function.
	 */
	callx ___branch_dump
#endif

#ifdef PROFILE
	/*
	 * Call profiler termination function.
	 */
	callx _term_profile
#endif

	fmark
	syncf		 /* added for the CA faulting but nop for Kx */
	.word 0xfeedface /* magic word for break, defined in NINDY   */

	/*
	 * Code below this point is needed only with versions of
	 * NINDY older than 2.13;  the newer versions never return
	 * from the fmark above.
	 */
	flushreg
	ld	_start_frame, pfp
	lda	return_to_nindy, r4	
	st	r4, 8(pfp)	/* store return address to frame */
	ret		
	
	.data
	.globl	_start_frame
	.globl	_stack_start
_start_frame:
	.word	0	/* addr of first user frame: for gdb960 */
_stack_start:
	.word	0	/* addr of first (lowest) byte of stack */


#ifdef PROFILE
	/*
	 * These references are just here to make sure that printf, fopen,
	 * and fwrite, which are needed by _term_profile(), get linked in.
	 */
	.word	_printf
	.word	_fopen
	.word	_fwrite
#endif

#ifdef BR_PREDICT
	/*
	 * These references are just here to make sure that printf, fopen,
	 * and fwrite, which are needed by __branch_dump(), get linked in.
	 */
	.word	_printf
	.word	_fopen
	.word	_fwrite
#endif
