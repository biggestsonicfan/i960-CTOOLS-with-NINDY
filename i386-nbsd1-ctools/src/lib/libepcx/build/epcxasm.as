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

.file "cxasm.s"

	.globl	_store_byte
	.globl	_store_quick_byte
	.globl  _load_byte
	.globl  _send_sysctl
	.globl 	_get_mask
	.globl 	_set_mask
	.globl 	_set_pending
	.globl 	_get_pending
	.globl 	_change_priority
	.globl  _mask_interrupts
	.globl  _unmask_interrupts
	.globl	_eat_time


_store_quick_byte:
	stob	g0, (g1)
	ret


_store_byte:
	stob	g0, (g1)
	ldconst 0, g0
  call	_eat_time
	ret


_load_byte:
	ldob	(g0), r3
	ldconst	0, g0
        call	_eat_time
	mov 	r3, g0
	ret


_send_sysctl:
	sysctl	g0, g1, g2
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
  mov r3,r3 /* Wait for modpc to take affect */
  mov r3,r3 /* Wait for modpc to take affect */
  mov r3,r3 /* Wait for modpc to take affect */
  mov r3,r3 /* Wait for modpc to take affect */
	mov	r5, g0
	ret

_unmask_interrupts:
	calls   5               /* get base address of PRCB */
	addo    4, g0, g0       /* get pointer to Control Table */
	ld      (g0), g1        /* get address of Control Table */
	addo    0x1c, g1, g1    /* get address of ICON register */
	ld      (g1), g2        /* get contents of ICON register */
	lda     0xfffffbff, g0  /* mask bit for enabling interrupts */
	and     g0, g2, g2      /* mask off global interrupts */
	st      g2, (g1)        /* store back to ICON register */
	lda     0x41, g0        /* load control register group 1 */
	sysctl  g0, g0, g0
	ret

_mask_interrupts:
	calls   5               /* get base address of PRCB */
	addo    4, g0, g0       /* get pointer to Control Table */
	ld      (g0), g1        /* get address of Control Table */
	addo    0x1c, g1, g1    /* get address of ICON register */
	ld      (g1), g2        /* get contents of ICON register */
	lda     0x400, g0       /* mask bit for disabling interrupts */
	or      g0, g2, g2      /* mask off global interrupts */
	st      g2, (g1)        /* store back to ICON register */
	lda     0x41, g0        /* load control register group 1 */
	sysctl  g0, g0, g0
	ret

_eat_time:
	ldconst	0,r4
	mov	r4,r3
	b	L2
L1:
	addi	1,r4,r4
	addi	1,r3,r3
L2:
	muli	20,g0,r15
	cmpibl	r3,r15,L1
	ret

/* TIMER INTERRUPT SERVICE ROUTINE
 *
 *
 */

	.globl	_timer_isr
_timer_isr:
#ifdef __PID
        ld      _interrupt_counter(g12),r4
        addo    r4,1,r4
        st      r4,_interrupt_counter(g12)
#else
        ld      _interrupt_counter,r4
        addo    r4,1,r4
        st      r4,_interrupt_counter
#endif
        ret


/*
 * the following routines are from the nindy file cx.s.
 * please make any changes to them there, and propagate
 * to the affected targets.
 */
	.globl	_disable_dcache
	.globl	_enable_dcache
	.globl	_check_dcache
	.globl	_real_check_dcache
	.globl	_restore_dcache
	.globl	_suspend_dcache
	.globl	_invalidate_dcache

/*
 * the following routines are examples of possible implementations of data
 * cache management.  better performance may be available by subsuming these
 * functions into the code requiring them.  if that is done, the 2 clock
 * latency may need to be taken into account explicitly.  it is hidden here
 * by the ret.
 */

/* return true if data cache is enabled, false if disabled */
_check_dcache:
	chkbit		30, sf2
	alterbit	0, 0, g0
	notbit		0, g0, g0
	ret

/* attempt to generate stale data in a possible data cache.  return result. */
/* returns 1 if stale data generated, 0 otherwise. */
_real_check_dcache:
        call	_suspend_dcache
        call	_enable_dcache

	ldconst	0xdeaddead, r4
#ifdef __PID
	lda	scribble(g12), r6
#else /* __PID */
	lda	scribble, r6
#endif	/*PID*/

	st	r4, (r6)	# cache = memory
        call	_disable_dcache
	ldconst	0xfeedface, r4
	st	r4, (r6)	# memory only
	call	_enable_dcache
	ld	(r6), r5	# from cache
	cmpo	r4, r5
	st	r4, (r6)	# cache = memory

	alterbit 0, 0, r5	# indicate 1 if "cache" == mem
        call   _restore_dcache
	notbit	0, r5, g0
	ret

/* save state and disable */
_suspend_dcache:
	mov		sf2, r4
#ifdef __PID
	st		r4, suspended_state(g12)
#else
	st		r4, suspended_state
#endif
	setbit		30, sf2, sf2
	ret

/* restore saved state */
_restore_dcache:
#ifdef __PID
	ld		suspended_state(g12), r4
#else
	ld		suspended_state, r4
#endif
	chkbit		30, r4
	alterbit	30, sf2, sf2
	ret

/* disable */
_disable_dcache:
	setbit	30, sf2, sf2
	ret

/* enable */
_enable_dcache:
	clrbit	30, sf2, sf2
	ret

/* invalidate */
_invalidate_dcache:
	setbit	31, sf2, sf2
	ret

	.bss	suspended_state, 4, 2		# state save area
	.bss	scribble, 4, 2			# state save area


