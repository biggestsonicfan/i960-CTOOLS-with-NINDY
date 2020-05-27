
/*(cb*/
/**************************************************************************
 *
 *     Copyright (c) 1992 Intel Corporation.  All rights reserved.
 *
 *
 * Intel hereby grants you permission to copy, modify, and distribute this
 * software and its documentation.  Intel grants this permission provided
 * that the above copyright notice appears in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  In addition, Intel grants this permission provided that
 * you prominently mark as not part of the original any modifications made
 * to this software or documentation, and that the name of Intel
 * Corporation not be used in advertising or publicity pertaining to the
 * software or the documentation without specific, written prior
 * permission.
 *
 * Intel provides this AS IS, WITHOUT ANY WARRANTY, INCLUDING THE WARRANTY
 * OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, and makes no
 * guarantee or representations regarding the use of, or the results of the
 * use of, the software and documentation in terms of correctness,
 * accuracy, reliability, currentness, or otherwise, and you rely on the
 * software, documentation, and results solely at your own risk.
 *
 **************************************************************************/
/*)ce*/

	.file	"cyt_asm.s"
#include "std_defs.h"

/*###################################################################
 *
 * Interrupt support routines
 *
 *##################################################################
 */
	.globl	__uart_wrapper
	.globl	__cio_wrapper
	.globl	__sq_irq1_wrapper
	.globl	__sq_irq0_wrapper
	.globl	__parallel_wrapper
	.globl	__pci_wrapper

	.globl	__uart_handler
	.globl	__cio_handler
	.globl	__sq_irq1_handler
	.globl	__sq_irq0_handler
	.globl	__parallel_handler
	.globl	__pci_handler

/*  asm_atadd for cyt_mem.c */
	.text
	.align 4
	.globl _asm_atadd

_asm_atadd:
	atadd g0, g1, g2
    ret


	.align	4
__uart_wrapper:
	lda	__uart_handler, r6
	b	1f

	.align	4
__cio_wrapper:
	lda	__cio_handler, r6
	b	1f

	.align	4
__sq_irq1_wrapper:
	lda	__sq_irq1_handler, r6
	b	1f

	.align	4
__sq_irq0_wrapper:
	/* Hardcode the handler since the Ethernet Squall is
	   the only one tested by MON960 Diags.  By hardcoding
	   interrupt stack space requirements are reduced */

	lda	_i596IntHandler, r6

        /*
         * Store global registers to avoid corruption.
         */
        lda     64(sp), sp
        stq     g0, -64(sp)
        stq     g4, -48(sp)
        stq     g8, -32(sp)
        stq     g12, -16(sp)

        /*
         * Zero g14 in preparation for the 'C' function call.
         * It must be zero unless we have an extra long argument
         * list, and we don't know what it was prior to interrupt.
         */
        mov     0, g14

#
#call the user service routine
#
	callx	(r6)

        /*
         * Restore global registers.
         */
        ldq     -64(sp), g0
        ldq     -48(sp), g4
        ldq     -32(sp), g8
        ldq     -16(sp), g12

	ret

	.align	4
__parallel_wrapper:
	lda	__parallel_handler, r6
	b	1f

	.align	4
__pci_wrapper:
	lda	__pci_handler, r6
	b	1f

1:
	mov	sp, r5
	lda	0x40(sp), sp

	stq	g0, 0(r5)
	stq	g4, 16(r5)
	stq	g8, 32(r5)
	stt	g12, 48(r5)
	mov	0, g14

	callx	(r6)

	ldq	0(r5), g0
	ldq	16(r5), g4
	ldq	32(r5), g8
	ldt	48(r5), g12
	ret


#if CX_CPU
# Misc interrupt support routines

	.globl	_imask_restore
	.globl	_imask_get

# Gets bits in the interrupt mask register that are set returns in g0
	.align	4
_imask_get:
	mov	sf1, g0
	ret

# Restores the interrupt mask register to value in g0
	.align	4
_imask_restore:
	mov	g0, sf1
	ret
#endif /* CX */


#
# quadtest(test_addr, quad_val_addr)
#
#	Tests a quadword write and read to a memory location
#
# Inputs:
#	g0 = test_addr = address to test
#
#	g1 = quad_val_addr = address of quadword value to write to start_addr
#
# Outputs:
#
#	g0 = return value.  0 if the test succeeds, 1 if it fails
#
	.text
	.align 4
	.globl _quadtest

_quadtest:
	mov	g0, r12
	ldq	(g1), r4
	stq	r4,(r12)
	ldq	(r12),r8
	cmpobne	r4,r8,L8
	cmpobne	r5,r9,L8
	cmpobne	r6,r10,L8
	cmpobne	r7,r11,L8
	ldconst 0, g0		# test passed
	ret
L8:
	ldconst	1, g0		# test failed
	ret

/*****************************************************************************
*
* AtomicModify - modify a memory location using an Atomic bus cycle
*
* This routine atomically modifies a memory location using the i960 atmod
* instruction.
*
* Arguments: AtomicModify(a, b, c)
*
*	a: on call contains the value to place in the memory location (before
*	   mask application.
*
*	b: contains the mask value used for memory modification
*
*	c: contains the address to run the bus cycle on
*
* RETURNS: the value that was in the memory location before modification
*/
	.text
	.align 4
	.globl _AtomicModify


_AtomicModify:
	atmod	g2, g1, g0
	ret
