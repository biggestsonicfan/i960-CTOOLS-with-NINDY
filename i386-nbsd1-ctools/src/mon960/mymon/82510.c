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
/* i82510 Device Driver */
/* This driver can be used for any board which has a 82510 UART.
 * The include file 'this_hw.h' must contain definitions for the symbols:
 *	I510BASE - base address of the 82510 registers
 *	I510DELTA - spacing between addresses of adjacent 82510 registers
 *	XTAL - frequency of baud rate generation crystal
 *	ACCESS_DELAY - provides delay between accesses to 82510 registers
 */

#include "common.h"
#include "i960.h"
#include "retarget.h"
#include "82510.h"
#include "this_hw.h"

/*
 * largest time that we can loop on a transmitter ready without abandoning
 * the task and reseting the UART
 */
#define	WAITCOUNT	(20 * 1000 * 4)

/*
 * forward references
 */
#pragma inline inreg outreg delay
static int inreg(int reg);
static void outreg(int reg, unsigned char val);
static void delay();

/*
 * I510BASE is the base address of the 82510.
 * I510DELTA gives the spacing between adjacent registers of the 82510.
 * For example, if A0,A1,A2 of the 82510 are connected to A2,A3,A4 of
 * the processor, as on the QT960 board, I510DELTA must be 4.
 */

/*
 * The 82510 is normally in bank 1.  Serial_init sets bank 1; all the
 * other routines (except the interrupt handler) assume that bank 1 is
 * set.  Any routine which needs to use a different bank restores bank 1
 * before returning.  The interrupt handler does not assume anything;
 * before returning it restores the bank that was set when the handler
 * was entered.
 */
#define set_bank(bank)   outreg(BANK, (bank) << 5)

/* Serial_getc and serial_putc read GSR instead of LSR because reading LSR
 * clears the break-received bit in RST, which is used by serial_intr.  If
 * serial_getc is called just as a break is received, the break bit will be
 * cleared, and serial_intr will not treat it as an interrupt caused by a
 * break.  */

/* Read a received character if one is available.  Return -1 otherwise. */
int
serial_getc()
{
    if (inreg(GSR) & RFIR)
    {
         return inreg(RXD);
    }
    return -1;
}

/* Transmit a character. */
void
serial_putc(int c)
{
    while ((inreg(GSR) & TFIR) == 0) ;

	if (break_flag)
		{
        while ((inreg(GSR) & 0x10) == 0 ) ;
    	outreg(TXD, c);
		}
	else
    	outreg(TXD, c);
}

/*
 * Initialize the device.
 */
void
serial_init(void)
{
	/* initialize the 82510 */
	set_bank(1);
	outreg(ICM, 0x10);		/* software reset */

	outreg(LCR, 0x03);		/* 8 bits, no parity */
	outreg(MCR, 0x01);		/* enable DTR */

	/* configure the baud rate generators */
	set_bank(3);
	outreg(CLCF, 0x50); 		/* baud rate source is BRGA */
	outreg(BACF, 0x04); 		/* tell BRGA to be a BRG */
	outreg(BBCF, 0x00); 		/* tell BRGB to be a TIMER */

	set_bank(2);
	outreg(RIE, 0x10);		/* enable break detect interrupt only */
	outreg(IMD, 0x08);		/* enable intr ack and 4-byte receive FIFO */

	set_bank(0);
	outreg(GER, 0x04);		/* enable receive machine interrupts */

	set_bank(1);
	outreg(ICM, 0x04);		/* status clear */
}


/*
 * Set the baud rate of the port.
 */
void
serial_set(unsigned long baud)
{
	int i;

	set_bank(0);

	/* Wait for transmitter to finish */
	for (i = WAITCOUNT; i; --i) {
		if (inreg(LSR) & TXEMT)
			break;
	}

	outreg(LCR, (inreg(LCR) | 0x80));  /* enable write to baud divisor */
	outreg(BAL, XTAL/(32*baud));      /* divisor low byte */
	outreg(BAH, (XTAL/(32*baud)) >> 8); /* divisor high byte */
	outreg(LCR, (inreg(LCR) & 0x7f));  /* disable write to baud divisor */

	set_bank(1);
}



/*
 * Serial_intr is called by clear_break_condition (in <board>_hw.c) when
 * the monitor is entered because of an interrupt from the serial port.
 * It checks whether the interrupt was caused by a BREAK condition; if
 * so, it clears the BREAK condition in the UART.
 */
int
serial_intr(void)
{
	unsigned char save_bank;
	int flag = FALSE;
	int rst;

	/* Since this is called from an interrupt handler, we can't assume
	 * which bank is active. */
	save_bank = inreg(BANK);
	set_bank(1);

	/* If we have received a break, wait until the end of break */
	rst = inreg(RST);
	if (rst & BR)
	{
	    while ((rst & BRK_TERM) == 0)
	    {
			rst = inreg(RST);
	    }
	    
	    /* clear the garbage characters */
		/* part of break sequence and cahr ready */
	    while ((inreg(RXF) & 0x04) && (inreg(GSR) & RFIR))
		{
			inreg(RXD);
		}
			
	    flag = TRUE;
		outreg(ICM, 0x08);   /* intr ack */
	}

	/* Restore original bank.  */
	outreg(BANK, save_bank);

	return(flag);
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
	outreg(MCR, MCR_LOOP);	/* enable loop back mode */
    }
    else
    {
	outreg(MCR, 0);		/* disable loop back mode */
    }
}

/*
 * These routines are used to read and write to the registers of the
 * 82510.  The 82510 cannot accept back-to-back bus cycles, because it
 * has a minimum time between cycles.  These routines ensure that the
 * bus cycle to the 82510 is followed by a dummy cycle to memory.
 */
static void
delay()
{
    static volatile unsigned char dummy;
    int i;
    for (i = 0; i < ACCESS_DELAY; i++)
	dummy = 0;
}

static int
inreg(int reg)
{
    int val;
    val = *((volatile unsigned char *)I510BASE+(reg)*I510DELTA);
    delay();
    return val;
}

static void
outreg(int reg, unsigned char val)
{
    *((volatile unsigned char *)I510BASE+(reg)*I510DELTA) = val;
    delay();
}
