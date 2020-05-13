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
	.globl  _send_sysctl
	.globl 	_get_prcb
	.globl 	_get_mask
	.globl 	_set_mask
	.globl 	_set_pending
	.globl 	_get_pending
	.globl 	_change_priority

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

_send_sysctl:
	sysctl	g0, g1, g2
	ret

_get_prcb:
	calls	5
	ret

_get_mask:
	mov	sf1, g0
	ret
	
_set_mask:
	mov	g0, sf1
	ret
	
_set_pending:
	mov	g0, sf0
	ret

_get_pending:
	mov	sf0, g0
	ret

_change_priority:
	lda	0x001f0000, r4
	shlo	16, g0, r5
	modpc	r4, r4, r5
	mov	r5, g0
	ret

/* TIMER INTERRUPT SERVICE ROUTINE
 *	Must be in assembler to guarantee that global registers
 *	aren't clobbered.
 */
	.globl	_timer_isr
_timer_isr:
        ld      _interrupt_counter,r4
        addo    r4,1,r4
        st      r4,_interrupt_counter
        ret
