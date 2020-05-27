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

	.file	"kx.s"
	.globl	_send_iac
	.globl 	_interrupt_register_read
	.globl	_interrupt_register_write

_send_iac:
/* G0 is the address to send the iac to (if it is 0, use 0xff000010).  */
/* G1 is the address of the IAC (four words).  Copy the IAC to ensure  */
/* that it is aligned. */

	mov	sp, g2
	addo	0x10, sp, sp
	ldq	(g1), r4

	lda	0xff000010, r3
	cmpobne	0, g0, 1f		# If the specified address is 0,
	mov	r3, g0			# use 0xff000010.

1:	cmpobne	g0, r3, 2f		# If the destination is 0xff000010,

	ldconst	0x93000000, r3		# and it is a reinitialize processor
	cmpobne	r4, r3, 2f		# IAC,

	cmpobne	0, r7, 2f		# and the destination address is 0,
	lda	3f, r7			# set the destination address to 3f.
	flushreg

2:	stq	r4, (g2)
	synmovq g0, g2
3:	ret

_interrupt_register_read:
	lda	0xff000004, r5
	synld	r5, g0
	ret

_interrupt_register_write:
	lda	0xff000004, r5
	synmov	r5, g0
	ret

	.globl	get_imsk
get_imsk:
	bx	(g14)

	.globl	_get_mask
_get_mask:
    ret

	.globl	_set_mask
_set_mask:
    ret

	.globl	_clear_pending
_clear_pending:
    ret

	.globl	_set_step_string
_set_step_string:
	ret

	.globl	save_sfr
save_sfr:
	ret

	.globl	restore_sfr
restore_sfr:
	ret

	.globl	flush_cache
flush_cache:
	lda	0xff000010, r3
	lda	flush_cache_iac, r4
	synmovq	r3, r4
	ret

	.align	4
flush_cache_iac:
	.word	0x89000000, 0, 0, 0
