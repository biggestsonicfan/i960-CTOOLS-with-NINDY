/*(cb*/
/**************************************************************************
 *
 *     Copyright (c) 1994, 1995 Intel Corporation.  All rights reserved.
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


/***************************************************************************
*       Written:        23 May 1994, Scott Coulter - changed for c145/c146 CX
*       Revised:	19 Dec 1995, Scott Coulter - changed LED_1_BIT to be 1
*
***************************************************************************/

	.file	"c145_asm.s"
#include "std_defs.h"

#if JX_CPU
	.globl	__pci_deadlock_isr
#endif
#if CXHXJX_CPU
    .globl  __pci_lserr_isr
#define XINT_CIO    6
#elif KXSX_CPU
#define XINT_CIO    0
#else
#error  "Must have a processor specified"
#endif

#define MEMBASE_DRAM 	    0xa0000000
#define DRAM_2MEG           0xa0200000
#define DRAM_8MEG           0xa0800000
#define DRAM_32MEG	    0xa2000000
#define DRAM_MON_OFFSET	    0x00008000
#define DUMMY_LOC           0xa0001000
#define UART_SCRATCH        0xb000001c
#define UART_MSR            0xb0000018
#define RI_BIT              6
#define CIO_CONTROL         0xb004000c
#define LED_REG             0xb0040000
#define CIO_REG             0x16
#define NEW_BOARD           0xa5a5a5a5
#define OLD_BOARD           0x5a5a5a5a
#define CIO_COUNT           46          /* table byte count, must match
                                           init table */
#define LED_0              0x01
#define LED_1_BIT          1		/* bit position, counting from 0 */

/**************************************************************************
 * init_mem - Initialize memory pre-init routine.
 *
 * This routine does the following:
 *
 *	- Disable interrupts by clearing IPND and IMSK.
 *	- Size and clear DRAM memory (2, 8 or 32 Mbytes).
 *
 * It is called with a balx from init.s, with the return address in g14.
 * The register g11 holds the processor stepping.  Neither g11 nor g14
 * are used (except for the bx (g14) to return to caller).
 **************************************************************************/

        .text
	.align	4
        .globl  init_mem
init_mem:

/*
# --  Disable interrupts - State of IPND register (sf0) undefined at reset.
 */
#if CXHXJX_CPU
	mov  g14, g13
	mov  0x00, g0
	bal  set_imsk
	mov  g13, g14
#endif /* CXHXJX */


/*
 * clear dram.
 */
zero_dram:
	ldconst	MEMBASE_DRAM, g2	/* Get base of dram */

	/* Do 8 writes to memory... Dram precharge timing crap-ola */
	lda	0x00000000,g1	/* Write 0's */
	st	g1,(g2)		/* 1... */
	st	g1,(g2)		/* 2... */
	st	g1,(g2)		/* 3... */
	st	g1,(g2)		/* 4... */
	st	g1,(g2)		/* 5... */
	st	g1,(g2)		/* 6... */
	st	g1,(g2)		/* 7... */
	st	g1,(g2)		/* and 8! */


/* Perform some simple tests to verify the functionality of the
   hardware (CIO, DRAM, etc), the results will be posted on the
   LEDs (for boards so-equipped) if the tests pass */

	/* walking ones test on the UART */

        ldconst UART_SCRATCH,g2 /* UART scratch register */
        ldconst DUMMY_LOC,g3    /* dummy write location */
        ldconst 0x00000001,g1   /* Initial value to write */
        ldconst 0xffffffff,g9   /* Value to force up data lines */
	ldconst 0,g13		/* clear new board indication */
	ldconst 0xff,g5
 
uartloop:
        stob    g1,(g2)         /* write the test value */
        st      g9,(g3)         /* force data lines high */
        ldob    (g2),g4         /* Load the value back from memory */
        cmpobne g1,g4,skip1     /* Compare written and read values */
 
        shlo 0x01,g1,g1         /* Walk the one */
        and  g5,g1,g1         	/* Mask the upper 24 bits */
        cmpobne 0x0,g1,uartloop /* Loop until g1 == 0 */

	/* Now look at Ring Indicator in UART's Modem Status Register
	   If the RI bit is set, the board is a new rev board
	   Else the board is an old rev board; In either case, store
	   the result in g13 for safe keeping */
	ldconst UART_MSR,g1
readloop:
	ldob	(g1),g2
	bbc	RI_BIT,g2,old_rev  /* see if Ring Indicator is clear */
	ldconst NEW_BOARD,g13	
	b	ciotest
old_rev:
	ldconst OLD_BOARD,g13	
	b	skip1
	
	/* walking ones test on the CIO */
ciotest:
        ldconst CIO_CONTROL,g2  /* CIO control register */
        ldconst DUMMY_LOC,g3    /* dummy write location */
        ldconst 0x00000001,g1   /* Initial value to write */
	ldconst 0xff,g6

	/* first reset the CIO state machine */
	ldob	(g2),g4		/* CIO remains in reset state */
	ldconst	0,g5
	stob	g5,(g2)		/* clear Reset bit */
	ldob	(g2),g4		/* leave CIO is state 0 */

cioloop:
	ldconst	CIO_REG,g5
	stob	g5,(g2)		/* point to correct register */
        stob    g1,(g2)         /* write the test value */
        st      g9,(g3)         /* force data lines high */
	stob	g5,(g2)		/* point to correct register */
        ldob    (g2),g4         /* Load the value back from memory */
        cmpobne g1,g4,skip1     /* Compare written and read values */
 
        shlo 0x01,g1,g1         /* Walk the one */
        and  g6,g1,g1         /* Mask the upper 24 bits */
        cmpobne 0x0,g1,cioloop  /* Loop until g1 == 0 */

	/* CIO walking ones test passed, so initialize the CIO */

cio_init:
	/* get start of config. table by emulating the macro */
	/* ROM_ADDR(x)     ((x) - __Bdata + rom_data) */

	lda	CIO_DATA, g1
	lda	__Bdata, g2
	lda	rom_data, g3
	subo	g2,g1,g1	/* get offset into data segment */
	addo	g1,g3,g1	/* get absolute rom address */

        ldconst CIO_CONTROL,g2  /* CIO control register */
        ldconst CIO_COUNT, g3  	/* table byte count */

	/* first reset the CIO state machine */
	ldob	(g2),g4
	ldob	(g2),g4		/* leave CIO in state 0 */

        ldconst 0,g4            /* initial byte count */
init_cio:
        ldob    (g1), g5         /* get byte from table */
        stob    g5, (g2)         /* write byte to CIO */
        addo    1, g1, g1        /* increment pointer */
        addo    1, g4, g4        /* increment byte count */
        cmpobl 	g4, g3, init_cio /* check to see if all data is sent */
 
	/* CIO is now initialized so turn on LED 0 */
	ldconst LED_REG,g2
	ldconst LED_0,g3
	stob	g3,(g2)
	
skip1:
	/* Now do a walking ones test on a memory location. */
	ldconst MEMBASE_DRAM,g2	/* memory location */
	ldconst DUMMY_LOC,g3	/* dummy write location */
	ldconst 0x00000001,g1	/* Initial value to write */
	ldconst 0xffffffff,g9	/* Value to force up data lines */
	ldconst 0x00000000,g4	/* bit counter */
	ldconst 32,g5		/* number of bits to test */

onesloop:
	st	g1,(g2)		/* write the test value */
	st	g9,(g3)		/* force data lines high */
	ld	(g2),g4		/* Load the value back from memory */
	cmpobne g1,g4,skip2	/* Compare written and read values */

	shlo 0x01,g1,g1		/* Walk the one */
	addi 1,g4,g4		/* increment counter */
	cmpobl g4,g5,onesloop	/* Loop until all bits are tested */

quadtesting:
	/* Now do a quadword write/read test */
	ldconst MEMBASE_DRAM,g8	/* destination address */

	ldconst 0,g0
	ldconst 0,g1
	ldconst 0,g2
	ldconst 0,g3
	stq	g0,(g8)		/* clear first quadword */

	ldconst 0xa5a5a5a5,g0	/* source data */
	ldconst 0xb5b5b5b5,g1
	ldconst 0xc5c5c5c5,g2
	ldconst 0xd5d5d5d5,g3
	
quadwrite:	/* write quad, read back with longs */
	stq	g0,(g8)
	ldconst DUMMY_LOC,g5
	st g9,(g5)    		/* force data lines high */

	ld (g8),g4		/* read data back using longs */
	cmpobne g0,g4,skip2
	addi 4,g8,g8
	ld (g8),g4
	cmpobne g1,g4,skip2
	addi 4,g8,g8
	ld (g8),g4
	cmpobne g2,g4,skip2
	addi 4,g8,g8
	ld (g8),g4
	cmpobne g3,g4,skip2

quadread:	/* write longs, read back with quad */

	ldconst MEMBASE_DRAM,g8	/* destination address */
	st g0,(g8)
	addi 4,g8,g8
	st g1,(g8)
	addi 4,g8,g8
	st g2,(g8)
	addi 4,g8,g8
	st g3,(g8)

	st g9,(g5)    		/* force data lines high */

	ldconst MEMBASE_DRAM,g8	/* destination address */
	ldq (g8),g4		/* reload the data for compare */
	cmpobne g0,g4,skip2	/* Compare the reloaded data */
	cmpobne g1,g5,skip2
	cmpobne g2,g6,skip2
	cmpobne g3,g7,skip2

	/* Memory Test passed, turn on LED 1 (if new board) */
	ldconst NEW_BOARD,g10
	cmpobne g10,g13,skip2
	ldconst LED_REG,g2
	ldob	(g2),g3
	and	0x0f,g3,g3
	setbit	LED_1_BIT,g3,g3
	stob	g3,(g2)

skip2:

	ldconst	MEMBASE_DRAM, g2	/* Get base of dram */
	ldconst	MEMBASE_DRAM+DRAM_MON_OFFSET, g0

	/* First erase all locations we will play with, so we don't */
	/* run into parity problems, and so we know what is there */
	/* to start off the test.  Then determine memory size - 2, 8, */
	/* or 32 meg. */

	lda	0x00000000,g1	/* 0's to erase memory */
	mov	g0,g2		/* First location */
	st	g1,(g2)		/* Erase it */
	ldconst	DRAM_2MEG+DRAM_MON_OFFSET, g2	/* above 2nd Mb */
	st	g1,(g2)
	addo	4,g2,g2
	st	g1,(g2)
	ldconst	DRAM_8MEG+DRAM_MON_OFFSET,g2	/* above 8th Mb */
	st	g1,(g2)
	addo	4,g2,g2
	st	g1,(g2)
 
        /* write to the base of DRAM.  If the test data is repeated
           at addresses above (i.e. above 2nd or 8th Mbyte) then the memory map
           has repeated.  If not then the board is equipped with either
           8 or 32 Mbyte
        */
 
	lda	0xA5A5A5A5,g3	/* Pattern to write to memory */
	mov	g0, g2	/* Store it in the first bank */
	st	g3,(g2)
	lda	0xFFFFFFFF,g4	/* Write all F's to next word to */
	addo	4,g2,g2		/* force all data lines high to */
	st	g4,(g2)		/* prevent interference */

	lda	DRAM_2MEG+DRAM_MON_OFFSET, g2	/* Read from above 2nd meg. */
	ld	(g2),g1		/* if the two words are equal then memory */
				/* has folded and only 2 megs. are present */
	cmpibne	g1,g3,dram8	/* if the two words are not equal, then */
				/* there are either 8 or 32 megs. */
dram2:
	lda	0x00200000,g0	/* We have 2Mb */
    b  erase_dram

dram8:
	st	g3,(g2)
	addo	4,g2,g2		/* force all data lines high to */
	st	g4,(g2)		/* prevent interference */

	lda	DRAM_8MEG+DRAM_MON_OFFSET,g2	/* Read from above 8th meg. */
	ld	(g2),g1		/* if the two words are equal then memory */
				/* has folded and only 8 megs. are present */
	cmpibne	g1,g3,dram32	/* if the two words are not equal, then */
				/* there are 32 megs. present */
	lda	0x00800000,g0	/* We have 8Mb */
    b  erase_dram

dram32:
	lda	0x02000000,g0	/* We have 32Mb */

erase_dram:
	mov g0, g1
	ldconst	MEMBASE_DRAM, g2	/* Get base of dram */
	addo	g0,g2,g0	        /* g2 = start address, 
								   g0 = memory size + start_addr */
	ldconst 0,g4
	ldconst 0,g5
	ldconst 0,g6
	ldconst 0,g7

loop_dram:
	stq   	g4,(g2)           /* zero 4 words				*/
	addi	16,g2,g2	      /* increment pointer by 16 bytes	*/
	cmpibl	g2,g0,loop_dram   /* loop until done			*/

	mov     g1, g0            /* dram size */
	bx	    (g14)	          /* Return to caller			*/

/* Offsets into control tables */
#define IMAP0REG	0x10
#define IMAP1REG	0x14
#define ICONREG		0x1c

#define REG0WORD	0x20
#define REGCWORD	0x50
#define REGDWORD	0x54
#define REGFWORD	0x5c

/**************************************************************************
 * fix_ctrl_table - Fix control table post-init routine.
 *
 * This routine does the following:
 *
 *	- Nothing currently...
 *	  may eventually be used to remap memory regions (post_init)
 *
 * It is called with a balx from init.s, with the return address in g14.
 * The register g11 holds the processor stepping.  Neither g11 nor g14
 * are used (except for the bx (g14) to return to caller).
 **************************************************************************/

	.globl	fix_ctrl_table
	.align	4
fix_ctrl_table:
	mov	g14, r4		/* Save return address */
	ldconst	0, g14		/* Compiler expects g14=0 */
	call	_get_prcbptr	/* Address of ram_prcb returned in g0 */
	mov	r4, g14		/* Restore return address */

	ld	4(g0), g0	/* g0 <- ram control table address */

    bx	(g14)		/* Return to caller */


#if CXHXJX_CPU
/*************************************************************************
 *  Data cache management routines for 960CF
 *************************************************************************/

	.globl	_purge_icache
	.align	4
_purge_icache:
    ldconst 0x100, g0
	sysctl	g0,g0,g0
	ret

#if CX_CPU
	.globl	_disable_dcache
	.globl	_enable_dcache
	.globl	_check_dcache
	.globl	_real_check_dcache
	.globl	_restore_dcache
	.globl	_suspend_dcache
	.globl	_invalidate_dcache

/* the following routines are examples of possible implementations of data
 * cache management.  better performance may be available by subsuming these
 * functions into the code requiring them.  if that is done, the 2 clock
 * latency may need to be taken into account explicitly.  it is hidden here
 * by the ret. return true if data cache is enabled, false if disabled
 */
	.align	4
_check_dcache:
	chkbit		30, sf2
	alterbit	0, 0, g0
	notbit		0, g0, g0
	ret

/* attempt to generate stale data in a possible data cache.  return result.
   returns 1 if stale data generated, 0 otherwise. */
	.align	4
_real_check_dcache:
	call		_suspend_dcache
	call		_enable_dcache
	ldconst		0xdeaddead, r4
	st		r4, scribble		# cache = memory
	call		_disable_dcache
	ldconst		0xfeedface, r4
	st		r4, scribble		# memory only
	call		_enable_dcache
	ld		scribble, r5		# from cache
	cmpo		r4, r5
	st		r4, scribble		# cache = memory
	alterbit	0, 0, r5		# indicate 1 if "cache" == mem
	call		_restore_dcache
	notbit		0, r5, g0
	ret

/*save state and disable */
	.align	4
_suspend_dcache:
	mov		sf2, r4
	st		r4, suspended_state
	setbit		30, sf2, sf2
	ret

/* restore saved state */
	.align	4
_restore_dcache:
	ld		suspended_state, r4
	chkbit		30, r4
	alterbit	30, sf2, sf2
	ret

/* invalidate */
	.align	4
_invalidate_dcache:
	setbit	31, sf2, sf2
	ret

	.bss	suspended_state, 4, 2		# state save area
	.bss	scribble, 4, 2			# state save area
#endif /* CX */
#endif /* CXHXJX */

#if JX_CPU
        .text
        .align  4
        .globl  _deadlock_wrapper
_deadlock_wrapper:
        lda     __pci_deadlock_isr, r6
        b       1f
#endif
#if CXHXJX_CPU
        .text
        .align  4
        .globl  _lserr_wrapper
_lserr_wrapper:
        lda     __pci_lserr_isr, r6
        b       1f
#endif 

	.align	4
    .globl __pci_intr_handler
__pci_intr_handler:
	lda	_pci_isr, r6
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

/* TIMER INTERRUPT SERVICE ROUTINES */
c145_isr:
	/*
	 * Store global registers to avoid corruption.
	 */
    mov    sp,       r11
    lda    0x40(sp), sp
    stq    g0,       0(r11)
    stq    g4,       16(r11)
    stq    g8,       32(r11)
    stt    g12,      48(r11)

#if KXSX_CPU
    /*
     * The Kx/Sx family apparently does not store an interrupted
     * application's current IP in the RIP.  Goodness knows why.  So
     * go fetch it from the stack and store it in g0.
     */
    flushreg
    notand pfp,   0xf, r3
    ld     8(r3), g0
#else
	/* Store IP of interrupted application in g0 */
	mov	   rip,   g0
#endif

    /*
     * Zero g14 in preparation for the 'C' function call.
     * It must be zero unless we have an extra long argument
     * list, and we don't know what it was prior to interrupt.
     */
    mov    0, g14

    /*
     * R14 is a flag that signals whether or not a timer interrupt
     * was detected at the CIO.
     */
    mov    1, r14

    lda    0xb004000c, r4

c145_1:
    ldconst 0x0b, r5
    stob    r5, (r4)
    ldob    (r4), r6
    bbc     6, r6, c145_2
    bbs     5, r6, c145_timer_1

c145_2:
    ldconst 0x0c, r5
    stob    r5, (r4)
    ldob    (r4), r6
    bbc     6, r6, c145_0
    bbs     5, r6, c145_timer_2

c145_0:
    ldconst 0x0a, r5
    stob    r5, (r4)
    ldob    (r4), r6
    bbc     6, r6, c145_no_timer_int
    bbs     5, r6, c145_timer_0

c145_no_timer_int:

    /* no cio interrupt on timers ??? */
    mov     0, r14
    b       c145_timer_epilog

c145_timer_0:

    stob    r5,   (r4)  /* r5 == affected 85c36 counter */
    ldconst 0xa0, r9
    stob    r9,   (r4)  /* clear gate command bit (GCB) & IP */

	ld     _c145_isr_0, r7
    callx  (r7)
    b      c145_timer_epilog

c145_timer_1:

    stob    r5,   (r4)  /* r5 == affected 85c36 counter */
    ldconst 0xa0, r9
    stob    r9,   (r4)  /* clear gate command bit (GCB) & IP */

	ld     _c145_isr_1, r7
    callx  (r7)
    b      c145_timer_epilog

c145_timer_2:

    stob    r5,   (r4)  /* r5 == affected 85c36 counter */
    ldconst 0xa0, r9
    stob    r9,   (r4)  /* clear gate command bit (GCB) & IP */

    ld     _c145_isr_2, r7
    callx  (r7)

    /* Fall through to c145_timer_epilog */

c145_timer_epilog:

#if CXHXJX_CPU
    /* clear the pending interrupt for level triggered interrupts. */

    ldconst XINT_CIO, g0
    call   _clear_pending
#endif

    /* Did we field an actual timer interrupt? */
    cmpobe  0, r14, c145_timer_iret

    /* Yes! */
    stob    r5, (r4)  /* r5 == affected 85c36 counter */
    ldconst 0x66, r9  /* Clear IUS, set GCB, force counter reload */
    stob    r9, (r4)


    /* Was this an interrupt associated with counter 1? */
    cmpobne 0x0b, r5, c145_timer_iret

    /* 
     * Yes.  Assume counter 1 and counter 0 are linked as a 32-bit counter. 
     * Force counter 0 to reload (to minimize timer overhead in the
     * ISR--counter 0 is still running).
     */
    ldconst 0x0a, r7
    stob    r7,   (r4)
    ldconst 0x06, r9
    stob    r9,   (r4)

c145_timer_iret:

    /*
     * Restore global registers.
     */
    ldq    0(r11),  g0
    ldq    16(r11), g4
    ldq    32(r11), g8
    ldt    48(r11), g12
    ret

no_isr:
    ret

	.globl   _get_timer_default_isr
_get_timer_default_isr:
	 lda     c145_isr, g0
	 ret

	.globl   _c145_isr_0
	.globl   _c145_isr_1
	.globl   _c145_isr_2
	.data
_c145_isr_0: .word   no_isr
_c145_isr_1: .word   no_isr
_c145_isr_2: .word   no_isr

/* z8536 cio initialization table - used for Rev. C and later baseboards

   If this table is modified make sure that the byte count definition
   at the top of this file is correct
   */
 
CIO_DATA:
	/*	 reg#	 value						*/
	/*	 ----	 -----						*/
        .byte    0x00,   0x00	/* clear reset bit, disable interrupts 	*/
        .byte    0x01,   0x00	/* Mast Conf Ctrl, disable ports 	*/

	/* Configure CIO port A - Processor Module Options 		*/
        .byte    0x20,   0x00	/* Mode spec - bit port         	*/
        .byte    0x21,   0x00	/* Handshake spec - not used    	*/
        .byte    0x22,   0x00	/* Polarity - all bits non-inverting 	*/
        .byte    0x23,   0xff	/* Direction - all bits inputs  	*/
        .byte    0x24,   0x00	/* Spec i/o ctrl - normal inputs	*/
        .byte    0x25,   0x00	/* Pattern matching registers   	*/
        .byte    0x26,   0x00	/* Pattern matching not used    	*/
        .byte    0x27,   0x00

	/*  Configure CIO port B - Serial EEPROM's clk/data 		*/
        .byte    0x28,   0x00	/* Mode spec - bit port         	*/
        .byte    0x29,   0x00	/* Handshake spec - not used    	*/
        .byte    0x2a,   0x00	/* Polarity - no inverting      	*/
        .byte    0x2b,   0xf0	/* Data dir - revision = input, 	*/
        .byte    0x2c,   0x0a	/* Data lines open drain        	*/
        .byte    0x2d,   0x00	/* Pattern matching registers   	*/
        .byte    0x2e,   0x00	/* Pattern matching not used    	*/
        .byte    0x2f,   0x00
        .byte    0x0e,   0x0a	/* Data reg - SCL's low, let    	*/
                                /* data lines float high.       	*/

	/*  Configure CIO port C - User LEDs                		*/
        .byte    0x05,   0x0f	/* Polarity - all bits inverting	*/
        .byte    0x06,   0x00	/* Data dir - all bits output   	*/
        .byte    0x0f,   0x00	/* Data reg - clear LEDs        	*/

	/*  Enable ports A, B, and C                        		*/
        .byte    0x01,   0x94	/* Mast Conf Ctrl - Enable ports	*/

