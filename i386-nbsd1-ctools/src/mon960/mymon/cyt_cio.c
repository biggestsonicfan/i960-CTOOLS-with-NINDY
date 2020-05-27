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

/******************************************************************************
*
* MODIFICATION HISTORY:
*
* 05mar95       tga - initial authorship
* 31aug94       snc - changed to support Cx, Sx, and Kx
* 12sep94       snc - more fixes
* 09aug95       snc - fixed timer init/interrupt test so that the timer
*		      is setup to run in continuous cycle mode so that
*		      more than one interrupt is generated.  fixed routine
*		      which checks number of interrupts received.
*
*/

#include "this_hw.h"
#include "cyt_intr.h"

/*
 * Internal routines
 */
static int	bus_test(void);
static void	init_cio(void);
static void	cio_isr(void);

void prtf();
long hexIn();

/*
 * Flag to indicate a cio interrupt
 */
static volatile int cio_int;

/************************************************/
/* BUS_TEST					*/
/* This routine performs a walking ones test	*/
/* on the given cio chip to test it's bus	*/
/* interface.  It writes to the counter 0 mode	*/
/* control word, then reads it back.  During	*/
/* this test all 8 data lines from the chip	*/
/* get written with both 1 and 0.		*/
/************************************************/
static
int
bus_test ()
{
    unsigned char	out, in,errors;
    int			bitpos;
    volatile int 	junk;

    junk = (int) &junk;	/* Don't let compiler optimize or "registerize" */

    /*
     * We don't want to mess with the CIO to much, since the ports are
     * used for c145/146 control bits.  Resetting it is definitely out.
     * We'll assume it is in "state 0" (see CIO manual).
     */

    errors = 0;
    for (bitpos = 0; bitpos < 8; bitpos++)
    {
	out = 1 << bitpos;

	CIO->ctrl = 0x16;		/* WR16 = Timer 1 TC, MS byte */
	CIO->ctrl = out;		/* Write data */

	CIO->ctrl = 0x16;		/* WR16 = Timer 1 TC, MS byte */
	junk = ~0;			/* Force data lines high */
	in = CIO->ctrl;			/* Read data */

	prtf ("%B ", in);

	/* make sure it's what we wrote */
	if (in != out)
	{
	    prtf("CIO Bus Test Error: Expected 0x%B Actual 0x%B\n",out,in);
	    errors++;
	}
    }
    prtf ("\n");
    if (errors)	
        return (0);
    else	
    	return (1);
}

/************************************************/
/* DISABLE_CIO_INTS				*/
/* This routine disables cio interrupts		*/
/************************************************/
static
void
disable_cio_ints ()
{
    unsigned char reg;

    disable_intr (IRQ_CIO);		/* Shut off interrupt at CPU */

    /* Turn off timer 1 interrupts at CIO */
    CIO->ctrl = 0x00;			/* 0x00 = Master Interrupt Control */
    reg = CIO->ctrl;
    CIO->ctrl = 0x00;			/* 0x00 = Master Interrupt Control */
    CIO->ctrl = reg & 0x7f;		/* Clear Master Int Enable bit */

    CIO->ctrl = 0x01;			/* 0x00 = Master Config Control */
    reg = CIO->ctrl;
    CIO->ctrl = 0x01;			/* 0x00 = Master Config Control */
    CIO->ctrl = reg & 0xbf;		/* Clear Timer 1 enable bit */
}

/************************************************/
/* CIO_ISR					*/
/* This routine responds to cio interrupts	*/
/************************************************/
static
void
cio_isr ()
{
    unsigned char junk;

    junk = CIO->ctrl;           /* reset the CIO state machine */
    CIO->ctrl = 0x0a;
    junk = CIO->ctrl;           /* get Counter/Timer1 status */
 
    if (junk & 0x20)
    {
        cio_int += 1;
        CIO->ctrl = 0x0a;
        /* reset IUS and IP, don't gate */
        CIO->ctrl = 0x24;
    }
}

/************************************************/
/* INIT_CIO					*/
/* This routine initializes the 82C54 interrupt */
/* and cio registers and initializes the cio*/
/* count.					*/
/************************************************/
static
void
init_cio ()
{
    unsigned char reg;

    disable_cio_ints ();

    CIO->ctrl = 0x1c;			/* 0x1c = Timer 1 Mode Specification */
    CIO->ctrl = 0x80;			/* continuous cyc, pulse, no ext */

    CIO->ctrl = 0x0a;			/* 0x1c = Timer 1 Cmd & Status */
    CIO->ctrl = 0x24;			/* Clear IP/IUS, set Gate Cmd Bit */

    CIO->ctrl = 0x0a;			/* 0x1c = Timer 1 Cmd & Status */
    CIO->ctrl = 0xc4;			/* Set IE, set Gate Cmd Bit */

    CIO->ctrl = 0x16;			/* 0x16 = Timer 1 TC, MS byte */
    CIO->ctrl = 0x4e;
    CIO->ctrl = 0x17;			/* 0x17 = Timer 1 TC, LS byte */
    CIO->ctrl = 0x20;			/* 20000 counts */

    CIO->ctrl = 0x01;			/* 0x00 = Master Config Control */
    reg = CIO->ctrl;
    reg &= 0xfc;			/* Clear timer link control bits */
    reg |= 0x40;			/* Set timer 1 enable bit */
    CIO->ctrl = 0x01;			/* 0x00 = Master Config Control */
    CIO->ctrl = reg;

    CIO->ctrl = 0x00;			/* 0x00 = Master Int Control */
    reg = CIO->ctrl;
    CIO->ctrl = 0x00;			/* 0x00 = Master Int Control */
    CIO->ctrl = reg | 0x80;		/* Set master int enable bit */

    CIO->ctrl = 0x0a;			/* 0x1c = Timer 1 Cmd & Status */
    CIO->ctrl = 0x06;			/* Null code, set Gate Cmd, Trigger */

    /*
     * Initialize interrupts
     */

    enable_intr (IRQ_CIO);
}

/****************************************/
/* CIO DIAGNOSTIC TEST		*/
/****************************************/
void
cio_test ()
{
    volatile int	loop;
    int			looplim;

    looplim = 2000000;

    disable_intr (IRQ_CIO);
    install_local_handler (IRQ_CIO, cio_isr);
    
    if (!bus_test ())
	prtf ("\nERROR:  bus_test for CIO failed\n");
    else
    {
	prtf ("\nbus_test for CIO passed\n");

	cio_int = 0;
	init_cio ();
	
	loop = 0;
	while ((cio_int < 4) && (loop < looplim))
	    loop++;

	if (cio_int < 4)
	    prtf ("CIO INTERRUPT test failed\n") ;
	else
	    prtf ("CIO INTERRUPT test passed\n");
    }

    disable_cio_ints ();
    prtf ("CIO tests done.\n");
    prtf ("Press return to continue.\n");
    (void) hexIn();
}
