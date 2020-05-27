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

/*
 *      16552 Device Driver
 */

#include "common.h"
#include "i960.h"
#include "retarget.h"
#include "16552.h"
#include "this_hw.h"

/*
 * Enable receive and transmit FIFOs.
 *
 * FCR<7:6>     00      trigger level = 1 byte
 * FCR<5:4>     00      reserved
 * FCR<3>       0       mode 1 - interrupt on fifo threshold
 * FCR<2>       1       clear xmit fifo
 * FCR<1>       1       clear recv fifo
 * FCR<0>       1       turn on fifo mode
 */
#define FIFO_ENABLE 0x07
#define INT_ENABLE      (IER_RLS)   /* default interrupt mask */

/*
 * REMEMBER TO SET curoffset=0 FOR BOARD TEST CODE BEFORE MONITOR INITIALIZATION
 *    or this code will fail because memory initializations will not work
 *    This is required for test code only.
 */
unsigned int curoffset = DFLTPORT;

/*
 * forward references
 */
int inreg(int reg);
void outreg(int reg, unsigned char val);


/* Read a received character if one is available.  Return -1 otherwise. */
int
serial_getc()
{
    if (inreg(LSR) & LSR_DR)
    {
         return inreg(RBR);
    }
    return -1;
}

/* Transmit a character. */
void
serial_putc(int c)
{
    while ((inreg(LSR) & LSR_THRE) == 0)
        ;
    outreg(THR, c);
}


/*
 * Initialize the device driver.
 */
void
serial_init(void)
{
    /*
     * Configure active port, (curoffset already set.)
     *
     * Set 8 bits, 1 stop bit, no parity.
     *
     * LCR<7>       0       divisor latch access bit
     * LCR<6>       0       break control (1=send break)
     * LCR<5>       0       stick parity (0=space, 1=mark)
     * LCR<4>       0       parity even (0=odd, 1=even)
     * LCR<3>       0       parity enable (1=enabled)
     * LCR<2>       0       # stop bits (0=1, 1=1.5)
     * LCR<1:0>     11      bits per character(00=5, 01=6, 10=7, 11=8)
     */
    outreg(LCR, 0x3);

    outreg(FCR, FIFO_ENABLE);	/* Enable the FIFO */

	outreg(IER, INT_ENABLE);	/* Enable appropriate interrupts */
}

/*
 * Set the baud rate.
 */
void
serial_set(unsigned long baud)
{
        unsigned char sav_lcr;

        /*
         * Enable access to the divisor latches by setting DLAB in LCR.
         *
         */
         sav_lcr = inreg(LCR);
         outreg(LCR, LCR_DLAB | sav_lcr);

        /*
         * Set divisor latches.
         */
        outreg(DLL, XTAL/(16*baud));
        outreg(DLM, (XTAL/(16*baud)) >> 8);

        /*
         * Restore line control register
         */
        outreg(LCR, sav_lcr);
}


/*
 * Serial_intr is called by clear_break_condition (in <board>_hw.c) when
 * the monitor is entered because of an interrupt from the serial port.
 * This routine will loop until the BREAK indication has been cleared and
 * the associated character in the FIFO has been read.
 */
int
serial_intr(void)
{
	unsigned char	ssr;
	unsigned char	c;

	ssr = inreg(LSR);

	if (!(ssr & LSR_BI)) {
	    /*
	     * Interrupt not caused by break, must be overrun, or framing error.
	     * Reading the Line Status Register (done on entry) clears the
	     * status, return FALSE.
	     */
	    return(FALSE);
	}

	while (ssr & LSR_BI) {
		ssr = inreg(LSR);
		if (ssr & LSR_DR) {
			c = inreg(RBR);
		}
	}

	return(TRUE);
}


/*
 * This routine is used by calc_looperms to put the UART in loopback mode.
 * If your UART doesn't have a loopback mode you will need to change
 * calc_looperms in the file serial.c.
 */

void
serial_loopback(int flag)
{
    if (flag)
    {
	outreg(MCR, inreg(MCR) | MCR_LOOP);	/* enable loop back mode */
    }
    else
    {
	outreg(MCR, inreg(MCR) & ~MCR_LOOP);	/* disable loop back mode */
    }
}

/*
 * These routines are used to read and write to the registers of the
 * 16552.  The delay routine guarantees the required recovery time between
 * cycles to the 16552.
 * DUART is the base address of the 16552.
 * DUART_DELTA gives the spacing between adjacent registers of the 16552.
 * For example, if A0,A1,A2 of the 16552 are connected to A2,A3,A4 of
 * the processor, DUART_DELTA must be 4.
 */
static void
delay()
{
    static volatile unsigned char dummy;
    int i;
    for (i = 0; i < ACCESS_DELAY; i++)
	dummy = 0;
}

int
inreg(int reg)
{
    int val;
    val = *((volatile unsigned char *)DUART+(curoffset+reg)*DUART_DELTA);
    delay();
    return val;
}

void
outreg(int reg, unsigned char val)
{
    *((volatile unsigned char *)DUART+(curoffset+reg)*DUART_DELTA) = val;
    delay();
}
