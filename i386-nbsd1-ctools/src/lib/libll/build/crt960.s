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

.file "crt960.s"

/* Mark module as PIC */
.link_pix

/*
 * The following macro allows you to conditionally call a target 
 * adjusting the call for pic.
 *
 * We make TARGET a common symbol here, so that by default the
 * TARGET is zeroed (since common symbols are allocated to .bss by
 * default).  But if the user includes a special magic file that
 * resolves TARGET in a strong reference, then it will have that
 * definition's address.
 */

#define CONDITIONAL_CALL(TARGET)                                            \
                                                                            \
	.comm TARGET,4 ;                                                    \
                                                                            \
	ld	TARGET, r6 ;                                                \
	cmpobe	0, r6, 0f	;/* This compares the unbiased address*/    \
				 /* of the TARGET routine against NULL. */  \
	/*                                                                  \
	 * We will call the target. Make call after adjusting r6 for        \
         * optional position-independent code.                              \
	 */                                                                 \
	lda	0(ip), g0    ;/* g0 <- runtime addr of next instruction  */ \
	lda	., g1	     ;/* g1 <- linktime addr of same instruction */ \
	subo	g1, g0, g0   ;/* g0 <- PIC bias (= 0 if PIC not used)    */ \
	addo	g0, r6, r6   ;/* r6 += PIC bias                          */ \
	callx	(r6) ;                                                      \
0: ;                                                                        \


/* crt960.s -- C runtime for 80960 */

	.globl	start
	.text
start:
	/*
	 * Set up frame on our stack
	 */
	lda	_stackbase, g0
	cmpobe	g0, fp, 0f	/* check if we're already there */
	mov	g0, sp
	call	0f		/* create the first stack frame on our stack */

0:	flushreg		/* discard old stack frames */
	lda	0, pfp		/* no previous frame */
	flushreg
	mov	0, g14		/* C compiler expects zero in g14 */


	/*
	 * clear bss
	 */
	lda	__Bbss, g0
	lda	__Ebss, g1
1:	st	g14, (g0)
	addo	4, g0, g0
	cmpobl	g0, g1, 1b

        /*
         * Conditionally call the flash copy routine,
         * This must be done before calling other stuff, and after bss
         * has been zeroed.
         */
	CONDITIONAL_CALL(___flash_copy_from_text_to_data)


	ldconst	0x3b001000, g0	/* AC codes: Suppress ... 
				  	integer overflow fault,
				  	float overflow fault,
				  	float underflow fault,
				  	float zero-divide fault,
				  	float inexact fault.
				     Allow denormalized numbers.
				     Round to nearest. */
	call	__setac 	/* set arithmetic controls in fp lib */

	call	___sram_init	/* clear sram (code made by the linker) */

	call	__LL_init	/* Initialize low level library */
	call	__HL_init    	/* Initialize high level library */

        /* Conditionally call the 2-pass profiling dump initialization. */
	CONDITIONAL_CALL(___profile_init_ptr)

        /* Conditionally call the ghist960 profiling initialization. */
	CONDITIONAL_CALL(___ghist_profile_init_ptr)

        /* Call the named bss section clearing code (made by the linker). */
        call ___clear_named_bss_sections

	call	__arg_init    	/* Initialize arguments to main */
				/* This call can be omitted if command */
				/* line arguments are not required. */

	call	_main    	/* Start user program. */

	b	_exit		/* No return from exit. */
