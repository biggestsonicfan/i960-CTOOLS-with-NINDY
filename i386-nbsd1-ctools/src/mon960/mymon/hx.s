/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1995 Intel Corporation
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

	.file	"hx.s"

/* define offest into register array matching hdi_regs.h */
    .set    REG_IPND, 35*4
    .set    REG_IMSK, 36*4
    .set    REG_SF2,  37*4
    .set    REG_SF3,  38*4
    .set    REG_SF4,  39*4

/* define memory mapped regiseter addresses */
#define    DLMCON_ADDR    0xff008100
#define    LMAR0_ADDR     0xff008108
#define    LMMR0_ADDR     0xff00810c
#define    LMAR1_ADDR     0xff008110
#define    LMMR1_ADDR     0xff008114
#define    IPB0_ADDR      0xff008400
#define    IPB1_ADDR      0xff008404
#define    IPB2_ADDR      0xff008408
#define    IPB3_ADDR      0xff00840c
#define    IPB4_ADDR      0xff008410
#define    IPB5_ADDR      0xff008414
#define    DAB0_ADDR      0xff008420
#define    DAB1_ADDR      0xff008424
#define    DAB2_ADDR      0xff008428
#define    DAB3_ADDR      0xff00842c
#define    DAB4_ADDR      0xff008430
#define    DAB5_ADDR      0xff008434
#define    BPCON_ADDR     0xff008440
#define    XBPCON_ADDR    0xff008444
#define    IPND_ADDR      0xff008500
#define    IMSK_ADDR      0xff008504
#define    ICON_ADDR      0xff008510
#define    GCON_ADDR      0xff008000
#define    IMAP0_ADDR     0xff008520
#define    IMAP1_ADDR     0xff008524
#define    IMAP2_ADDR     0xff008528
#define    PMCON0_ADDR    0xff008600
#define    PMCON2_ADDR    0xff008608
#define    PMCON4_ADDR    0xff008610
#define    PMCON6_ADDR    0xff008618
#define    PMCON8_ADDR    0xff008620
#define    PMCON10_ADDR   0xff008628
#define    PMCON12_ADDR   0xff008630
#define    PMCON14_ADDR   0xff008638
#define    BCON_ADDR      0xff0086fc
#define    PRCB_ADDR      0xff008700
#define    ISP_ADDR       0xff008704
#define    SSP_ADDR       0xff008708
#define    DEVID_ADDR     0xff008700
#define    TRR0_ADDR      0xff000300
#define    TCR0_ADDR      0xff000304
#define    TMR0_ADDR      0xff000308
#define    TRR1_ADDR      0xff000310
#define    TCR1_ADDR      0xff000314
#define    TMR1_ADDR      0xff000318

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

    /* jx loses pfp on reinitalize, so save it then restore it*/
	lda	2f, g1
3:  st      pfp, save_pfp
	flushreg
	sysctl	g0, g1, g2

2:  ld      save_pfp, pfp
	flushreg
	ret

1:	sysctl	g0, g1, g2
	ret

/* used by Hdi_set_mmr_reg call */
	.globl 	_mmr_sysctl
_mmr_sysctl:
	sysctl	g0, g1, g2
	mov     g2, g0
	ret

/*
 * Get_imsk is called only from entry.s in the interrupt entry handler.  It
 * is called with a bal instruction.
 */

	.globl	get_imsk
get_imsk:
	lda IMSK_ADDR, g1
	ld  (g1), g0
	bx	(g14)

	.globl	set_imsk
set_imsk:
	lda IMSK_ADDR, g1
	st  g0, (g1)
	bx	(g14)

	.globl 	_get_mask
_get_mask:
	lda IMSK_ADDR, g1
	ld  (g1), g0
	ret

	.globl 	_set_mask
_set_mask:
	lda IMSK_ADDR, g1
	st  g0, (g1)
	ret

	.globl 	_get_icon
_get_icon:
	lda ICON_ADDR, g1
	ld  (g1), g0
	ret

	.globl 	_get_imap
_get_imap:
    cmpobne  0, g0, 1f
	lda IMAP0_ADDR, g2
	b   3f
1:
    cmpobne  1, g0, 2f
	lda IMAP1_ADDR, g2
	b   3f
2:	
	lda IMAP2_ADDR, g2
3:
	ld  (g2), g0
	ret

	.globl 	_set_imap
_set_imap:
    cmpobne  0, g0, 1f
	lda IMAP0_ADDR, g2
	b   3f
1:
    cmpobne  1, g0, 2f
	lda IMAP1_ADDR, g2
	b   3f
2:	
	lda IMAP2_ADDR, g2
3:
	st  g1, (g2)
	ret

	.globl 	_set_tcr
_set_tcr:
    cmpobne  0, g0, 1f
	lda TCR0_ADDR, g2
	b   2f
1:
	lda TCR1_ADDR, g2
2:	
	st  g1, (g2)
	ret

	.globl 	_get_tcr
_get_tcr:
    cmpobne  0, g0, 1f
	lda TCR0_ADDR, g2
	b   2f
1:
	lda TCR1_ADDR, g2
2:	
	ld  (g2), g0
	ret

	.globl 	_set_tmr
_set_tmr:
    cmpobne  0, g0, 1f
	lda TMR0_ADDR, g2
	b   2f
1:
	lda TMR1_ADDR, g2
2:	
	st  g1, (g2)
	ret

	.globl 	_get_tmr
_get_tmr:
    cmpobne  0, g0, 1f
	lda TMR0_ADDR, g2
	b   2f
1:
	lda TMR1_ADDR, g2
2:	
	ld  (g2), g0
	ret

	.globl 	_set_trr
_set_trr:
    cmpobne  0, g0, 1f
	lda TRR0_ADDR, g2
	b   2f
1:
	lda TRR1_ADDR, g2
2:	
	st  g1, (g2)
	ret

	.globl 	_get_trr
_get_trr:
    cmpobne  0, g0, 1f
	lda TRR0_ADDR, g2
	b   2f
1:
	lda TRR1_ADDR, g2
2:	
	ld  (g2), g0
	ret

	.globl 	_get_brkreq
_get_brkreq:
    ldconst 0x600, g2
	sysctl  g2, g2, g0
    ret

	.globl 	_get_ipb
_get_ipb:
    cmpobne  0, g0, 1f
	lda IPB0_ADDR, g2
	b   9f
1:  cmpobne  1, g0, 2f
	lda IPB1_ADDR, g2
	b   9f
2:  cmpobne  2, g0, 3f
	lda IPB2_ADDR, g2
	b   9f
3:  cmpobne  3, g0, 4f
	lda IPB3_ADDR, g2
	b   9f
4:  cmpobne  4, g0, 5f
	lda IPB4_ADDR, g2
	b   9f
5:
	lda IPB5_ADDR, g2
9:	
	ld  (g2), g0
	ret

	.globl 	_set_ipb
_set_ipb:
/*    ldconst 0x600, g2
	sysctl  g2, g2, g2
    cmpobne  0, g2, 0f
    ret  */  /* not available */
    
0:    cmpobne  0, g0, 1f
	lda IPB0_ADDR, g2
	b   9f
1:  cmpobne  1, g0, 2f
	lda IPB1_ADDR, g2
	b   9f
2:  cmpobne  2, g0, 3f
	lda IPB2_ADDR, g2
	b   9f
3:  cmpobne  3, g0, 4f
	lda IPB3_ADDR, g2
	b   9f
4:  cmpobne  4, g0, 5f
	lda IPB4_ADDR, g2
	b   9f
5:
	lda IPB5_ADDR, g2
9:	
	st  g1, (g2)
	ret

	.globl 	_get_dab
_get_dab:
    cmpobne  0, g0, 1f
	lda DAB0_ADDR, g2
	b   9f
1:  cmpobne  1, g0, 2f
	lda DAB1_ADDR, g2
	b   9f
2:  cmpobne  2, g0, 3f
	lda DAB2_ADDR, g2
	b   9f
3:  cmpobne  3, g0, 4f
	lda DAB3_ADDR, g2
	b   9f
4:  cmpobne  4, g0, 5f
	lda DAB4_ADDR, g2
	b   9f
5:
	lda DAB5_ADDR, g2
9:	
	ld  (g2), g0
	ret

	.globl 	_set_dab
_set_dab:
/*    ldconst 0x600, g2
	sysctl  g2, g2, g2
    cmpobne  0, g2, 0f
    ret  */   /* not available */

0:  cmpobne  0, g0, 1f
	lda DAB0_ADDR, g2
	b   9f
1:  cmpobne  1, g0, 2f
	lda DAB1_ADDR, g2
	b   9f
2:  cmpobne  2, g0, 3f
	lda DAB2_ADDR, g2
	b   9f
3:  cmpobne  3, g0, 4f
	lda DAB3_ADDR, g2
	b   9f
4:  cmpobne  4, g0, 5f
	lda DAB4_ADDR, g2
	b   9f
5:
	lda DAB5_ADDR, g2
9:	
	st  g1, (g2)
	ret

	.globl 	_get_bpcon
_get_bpcon:
	lda BPCON_ADDR, g1
	ld  (g1), g0
	ret

	.globl 	_get_xbpcon
_get_xbpcon:
	lda XBPCON_ADDR, g1
	ld  (g1), g0
	ret

	.globl 	_set_bpcon
_set_bpcon:
/*   ldconst 0x600, g2
	sysctl  g2, g2, g2
    cmpobne  0, g2, 0f
    ret    */ /* not available */

0:	lda BPCON_ADDR, g1
	st  g0, (g1)
	ret
    
	.globl 	_set_xbpcon
_set_xbpcon:
/*    ldconst 0x600, g2
	sysctl  g2, g2, g2
    cmpobne  0, g2, 0f
    ret  */   /* not available */

0:	lda XBPCON_ADDR, g1
	st  g0, (g1)
	ret

	.globl 	_get_dlmcon
_get_dlmcon:
	lda DLMCON_ADDR, g1
	ld  (g1), g0
	ret

	.globl 	_set_dlmcon
_set_dlmcon:
	lda DLMCON_ADDR, g1
	st  g0, (g1)
	ret

	.globl 	_set_lmar0
_set_lmar0:
	lda LMAR0_ADDR, g1
	st  g0, (g1)
	ret

	.globl 	_set_lmmr0
_set_lmmr0:
	lda LMMR0_ADDR, g1
	st  g0, (g1)
	ret

	.globl 	_disable_dcache
_disable_dcache:
	dcctl 0, g0, g0 
	ret

	.globl 	_enable_dcache
_enable_dcache:
	dcctl 1, g0, g0 
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
	lda IPND_ADDR, g1
    lda    0xffffffff, g2
    atmod  g1, g2, g0
	ret

	.globl 	_clear_pending
_clear_pending:
    lda      IPND_ADDR, g1
    ldconst  1, r10
    shlo     g0, r10, r10
clear_pend_wait:
    ldconst  0, g2
    atmod    g1, r10, g2

    /* As per i960 Hx microprocessor user's guide, loop until pend bit clears */
    ld       (g1), r11
    bbs      g0, r11, clear_pend_wait
	ret

	.globl 	_get_pending
_get_pending:
	lda	nop_pending, g1
	atmod	g1, 0, g1
	lda IPND_ADDR, g1
	ld  (g1), g0
	ret

	.globl	save_sfr
/* This routine is called on entry to the monitor to save the special
 * function registers.  They are saved in the same register save area
 * used for the general purpose registers and other registers.  IPND
 * is saved in variables local to this module, because
 * they require special processing when they are restored.  See restore_sfr
 * for information.
 */
	.bss	ipnd_save, 4, 2

save_sfr:
	lda GCON_ADDR, r5
	ld  (r5), r4
	st  r4, _register_set+REG_SF4
	lda ICON_ADDR, r5
	ld  (r5), r4
	st  r4, _register_set+REG_SF3
	mov sf2, r5
	st  r5, _register_set+REG_SF2 /* REG_CCON */
	lda IPND_ADDR, r5
	ld  (r5), r4
	st  r4, _register_set+REG_IPND
	st	r4, ipnd_save

	/* If the monitor was entered with a maskable interrupt, and imsk is 0,
	 * retrieve the value of imsk from r3.  The stop cause is passed
	 * in g0, the vector number in g1 (if the stop cause was interrupt),
	 * and the saved value of r3 in g2.  */
	lda IMSK_ADDR, r5
	ld  (r5), r4           /* IMASK */
	cmpobne 1, g0, 1f		/* Check for interrupt entry */
	ldconst	248, r5
	cmpobe	r5, g1, 1f		/* Check for not NMI */
	cmpobne	0, r4, 1f		/* Check whether IMSK was cleared */
	mov	g2, r4			/* Use the saved value of r3 */
1:	st  r4, _register_set+REG_IMSK
	ret

	.globl	restore_sfr
/* Restore sfr's when exiting to application */
restore_sfr:
	lda GCON_ADDR, r5    /* (r) = IPND register */
	ld  _register_set+REG_SF4, r4
	st  r4, (r5)

	lda ICON_ADDR, r5    /* (r) = IPND register */
	ld  _register_set+REG_SF3, r4
	st  r4, (r5)

	ld  _register_set+REG_SF2, r4 /* REG_CCON*/
	mov r4, sf2

	/* IMSK requires no special treatment */
	lda IMSK_ADDR, r5 /* r5 = IMSK reg */
	ldconst 0, r4     /* stop interrupts */
	st  r4, (r5)

	lda IPND_ADDR, r6    /* (r6) = IPND register */
	ld  _register_set+REG_IPND, r4

	/* Only change bits in IPND that the user changed since we stopped. */
	ld	ipnd_save, r7
	xor	r4, r7, r7	/* bits to change */
	/* Can't use modify with an sf register */
	/* We also can't move ipnd into another register to operate on it; */
	/* we might miss an interrupt.  Note: the processor only sets bits */
	/* in the IPND register; it never clears them. */
	/* modify	r7,r4,sf0 */
	and	    r4, r7, r8	/* bits to set */
	andnot	r4, r7, r7	/* bits to clear */
    ld      (r6), r4
	andnot	r7, r4, r4
	or	    r8, r4, r4
	st      r4, (r6)     /* new IPND */

    ld  _register_set+REG_IMSK,r4
	st  r4, (r5)     /* new IMSK */
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
	lda IMSK_ADDR, g1
	ld  (g1), g2
	setbit	g0, g2, g2
	st  g2, (g1)
	ret

	.globl	_disable_interrupt
_disable_interrupt:
	lda IMSK_ADDR, g1
	ld  (g1), g2
	clrbit	g0, g2, g2
	st  g2, (g1)
	ret
