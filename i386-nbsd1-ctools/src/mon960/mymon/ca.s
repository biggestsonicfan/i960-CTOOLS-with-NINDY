/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994, 1995 Intel Corporation
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
/* This file contains routines to access registers or
 * features specific to the CA processor. */

	.file	"ca.s"

/* Offsets into _register_set array.  These values should change only
 * if hdi_regs.h changes. */
	.set	REG_SF0, 35*4
	.set	REG_SF1, 36*4
	.set	REG_SF2, 37*4


	.bss	save_pfp, 4, 4		# state save area

	.globl	_send_sysctl
/* Issue a sysctl instruction */
_send_sysctl:
	/* If this is a reinitialize sysctl, and no new start address has
	 * been supplied, return to the caller, by doing a flushreg before
	 * the sysctl instruction and a ret after.  The reinitialize sysctl
	 * does not change pfp.
	 */
	ldconst	0x300, r3
	cmpobne	g0, r3, 1f
	cmpobne	0, g1, 3f

    /*  The pfp is lost on reinits for the CF, so we must restore it */
	lda	2f, g1
3:  st  pfp, save_pfp
	flushreg
	sysctl	g0, g1, g2

2:  ld  save_pfp, pfp
	flushreg
	ret

1:	sysctl	g0, g1, g2
	ret

/*
 * Get_imsk is called only from entry.s in the interrupt entry handler.  It
 * is called with a bal instruction.
 */
	.globl	get_imsk
get_imsk:
	mov	sf1, g0
	bx	(g14)

	.globl	set_imsk
set_imsk:
	mov	sf1, g0
	bx	(g14)

	.globl 	_get_mask
_get_mask:
	mov	sf1, g0
	ret

	.globl 	_set_mask
_set_mask:
	mov	g0, sf1
	ret

/* In the following INPD register manipulation routines, we need
 * to force the processor to empty out the bus request queue.  If
 * the software performs an access to a hardware device register
 * to acknowledge/cancel the interrupt, this access may be posted
 * in the bus execution unit for later processing.  It is possible
 * that the CPU then checks the IPND register BEFORE the bus unit
 * has issued the access to the hardware device (we have seen this
 * happen).  In order to prevent this problem, we need to force the
 * instruction execution unit to wait for the bus unit to complete
 * the device access.  On the CA, a load from external memory,
 * followed by use of that data, forces the execution unit to wait
 * for the bus execution pipeline to empty out and the load to
 * complete.  By the time the load has completed, our dvice access
 * will be done.  On the CF, matters are a bit more complicated -
 * what if the data we try to fetch is in the data queue?  The CF
 * documentation specifies that all atomic accesses (atadd, atmod)
 * are considered uncachable, and are forced to access external
 * memory.  Thus, we use an atmod to force the processor to perform
 * the read-modify-write and wait for it to complete before it
 * moves on.  We use a mask of 0 so that the memory location is not
 * really altered.
 */
	.bss	nop_pending, 4, 2		# state save area

	.globl 	_set_pending
_set_pending:
	lda	nop_pending, g1
	atmod	g1, 0, g1
	mov	g0, sf0
	ret

    .globl  _clear_pending
_clear_pending:
    clrbit  g0, sf0, sf0
    lda     nop_pending, g1
    atmod   g1, 0, g1

    /* As per i960 Cx microprocessor user's guide, loop until pend bit clears */
    bbs     g0, sf0, _clear_pending
    ret

	.globl 	_get_pending
_get_pending:
	lda	nop_pending, g1
	atmod	g1, 0, g1
	mov	sf0, g0
	ret

	.globl 	_enable_dcache
_enable_dcache:
	clrbit	30, sf2, sf2
	ret

	.globl 	_disable_dcache
_disable_dcache:
	setbit	30, sf2, sf2
	ret

	.globl 	_set_dmac
_set_dmac:
	mov	g0, sf2
	ret

	.globl 	_get_dmac
_get_dmac:
	mov	sf2, g0
	ret

	.globl	save_sfr
/* This routine is called on entry to the monitor to save the special
 * function registers.  They are saved in the same register save area
 * used for the general purpose registers and other registers.  IPND
 * and DMAC are also saved in variables local to this module, because
 * they require special processing when they are restored.  See restore_sfr
 * for information.
 */
	.bss	ipnd_save, 4, 2
	.bss	dmac_save, 4, 2

save_sfr:
	mov	sf0,r4
        st	r4,_register_set+REG_SF0
	st	r4,ipnd_save

	/* If the monitor was entered with a maskable interrupt, and imsk is 0,
	 * retrieve the value of imsk from r3.  The stop cause is passed
	 * in g0, the vector number in g1 (if the stop cause was interrupt),
	 * and the saved value of r3 in g2.  */
	mov	sf1, r4
	cmpobne 1, g0, 1f		/* Check for interrupt entry */
	ldconst	248, r5
	cmpobe	r5, g1, 1f		/* Check for not NMI */
	cmpobne	0, r4, 1f		/* Check whether IMSK was cleared */
	mov	g2, r4			/* Use the saved value of r3 */
1:	st	r4,_register_set+REG_SF1

	mov	sf2,r4
    st	r4,_register_set+REG_SF2
	st	r4,dmac_save
	ret

	.globl	restore_sfr
/* Restore sfr's when exiting to application */
restore_sfr:
        ld     _register_set+REG_SF0,r4
        ld     _register_set+REG_SF1,r5
        ld     _register_set+REG_SF2,r6

	/* Only change bits in IPND that the user changed since we stopped. */
	ld	ipnd_save,r7
	xor	r4,r7,r7	/* bits to change */
	/* Can't use modify with an sf register */
	/* We also can't move ipnd into another register to operate on it; */
	/* we might miss an interrupt.  Note: the processor only sets bits */
	/* in the IPND register; it never clears them. */
	/* modify	r7,r4,sf0 */
	and	r4,r7,r8	/* bits to set */
	andnot	r4,r7,r7	/* bits to clear */
	andnot	r7,sf0,sf0
	or	r8,sf0,sf0

	/* IMSK requires no special treatment */
	mov	r5,sf1

	/* Only change bits in DMAC that the user changed since we stopped. */
	ld	dmac_save,r7
	xor	r6,r7,r7	/* bits to change */
	/* Can't use modify with an sf register */
	/* We also can't move dmac into another register to operate on it; */
	/* we might miss a dma event. Note: the processor only sets bits in */
	/* the DMAC register; it never clears them. */
	/* modify	r7,r6,sf2 */
	and	r6,r7,r8	/* bits to set */
	andnot	r6,r7,r7	/* bits to clear */
	andnot	r7,sf2,sf2
	or	r8,sf2,sf2

	ret

	.globl	flush_cache
/* Clear the instruction cache. */
flush_cache:
	ldconst	0x100, r3
	/* g0,g0 ignored */
	sysctl	r3, g0, g0
	ret

	.globl	_enable_interrupt
_enable_interrupt:
	setbit	g0, sf1, sf1
	ret

	.globl	_disable_interrupt
_disable_interrupt:
	clrbit	g0, sf1, sf1
	ret
