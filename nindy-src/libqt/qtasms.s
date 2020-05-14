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
	.globl	_store_byte
	.globl	_store_quick_byte
	.globl  _load_byte
	.globl  _send_iac
	.globl  _lower_priority
	.globl	_default_isr
	.globl	_timer_isr
	.globl	_interrupt_register_write
	.globl 	_interrupt_register_read

_store_quick_byte:
	stob	g0, (g1)
	ret

_store_byte:
	stob	g0, (g1)
	ldconst 0, g0
	call	_eat_time
	ret

_load_byte:
	ldob	(g0), g1
	ldconst	0, g0
	call	_eat_time
	mov 	g1, g0
	ret

_send_iac:
	lda	0xff000010, r5
	synmovq	r5, g0
	ret

_interrupt_register_read:
	lda	0xff000004, r5
	synld	r5, g0
	ret

_interrupt_register_write:
	lda	0xff000004, r5
	synmov	r5, g0
	ret

_lower_priority:
	lda	0x001f0000, r5
	shlo	16, g0, g0
	modpc	r5, r5, g0
	ret

	.text
	.align 4
_default_isr:
	ret

	.align 4
_timer_isr:
	ld	_bentime_interrupts,r5
	addo	1,r5,r5
	st	r5,_bentime_interrupts
	ret







