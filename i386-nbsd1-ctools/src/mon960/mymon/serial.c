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

#include "retarget.h"
#include "mon960.h"
#include "hdi_com.h"
#include "hdi_errs.h"
#include "com.h"

/*
 * Looperms is used for software timing loop while reading from the serial
 * device.  Serial_read is called with a timeout value in milliseconds.  This
 * is multiplied by looperms to obtain the number of times to loop before
 * timing out.  The value of looperms is calculated by calc_looperms.
 * The value DFLTLOOPERMS is used only if calc_looperms fails.
 */
#define DFLTLOOPERMS 500
static int looperms;
static int calc_looperms(void);


static serial_already_open = FALSE;
/*
 * open the port and prepare for IO
 * Return	0 for success
 *		-1 for fail
 */
int
serial_open(void)
{
  if (serial_already_open == FALSE)
    {
	/* Calculate the time constant for timeouts on serial_read. */
	if ((looperms = calc_looperms()) <= 0)
	    looperms = DFLTLOOPERMS;

	/* Initialize the serial port and set the baud rate.
	 * The baud rate is set here for sanity only; the autobaud
	 * mechanism will change it as required when the host connects.
	 */
	serial_init();
	serial_set(baud_rate?baud_rate:9600L);
	serial_write(0, (const unsigned char *)"\r\nMon960\r\n", 10);

    serial_already_open=TRUE;
    }

	return(0);
}


/*
 * Try to read as many as N bytes from the port.
 * Return immediately when any data is available.
 * Return after timo milliseconds if no data is available.
 * If timo is 0, wait forever.
 * Return the number of bytes read.
 */
int
serial_read(int port, unsigned char *buf, int len, int timo)
{
	int c, count=0, actual=0;

	leds(1, 1);

	/* If timo is zero, loop forever; if timo is -1, check for input once
	 * and return if there is none; otherwise loop timo*looperms times.
	 */

	switch (timo)
	    {
	    case COM_POLL:
	        if ((c = serial_getc()) < 0)
	        	{
	    	    com_stat = E_COMM_TIMO;
	 	    	return(0);
		        }
		    break;
	    
	    case COM_WAIT_FOREVER:
		    while ((c = serial_getc()) < 0)
		        ;
		    break;

	    default:
		/* NOTE: the timing of this loop, the one below, and the
		 * one in calc_looperms() must all match.
		 */
	        count = timo*looperms;
		    do  {
		        c = serial_getc();
		        } while (c < 0 && count-- > 0);

	    	if (c < 0)
	    	    {
	    	    com_stat = E_COMM_TIMO;
	     		return(0);
		        }
	    	break;
	    }


	/* If a break was received just as serial_getc was called, it
	 * will read a garbage character.  Checking break_flag here
	 * prevents that character from being returned. */
	if (break_flag)
	    {
	    com_stat = E_INTR;
	 	return(-1);
	    }
			
	/* Once we have received a byte, wait up to 1/3 ms for each
	 * subsequent byte to be received.  This is long enough for
	 * one byte at 38400 baud.  In this way, if data is received
	 * continuously at 38400 baud or above, we will not return
	 * until we have read all bytes requested.  At less than 38400
	 * baud, there is enough time between bytes to return and be
	 * called again without losing data.  We don't want to wait
	 * any longer because it would mess up the timeout (if no
	 * more data arrives).
	 */
	do  {
	    *buf++ = c;
	    if (++actual == len)
		break;

	    /* Wait up to 1/3 ms--long enough for one byte at 38400 baud.
	     * If there is more data in the FIFO, this will not wait at all.
	     * NOTE: the timing of this loop, the one above (in the
	     * default case of the switch), and the one in calc_looperms()
	     * must all match.
	     */
	    count = (looperms + 2) / 3;
	    do  {
	    	c = serial_getc();
	        } while (c < 0 && count-- > 0);
	    } while (c >= 0);

	leds(0, 1);
	return (actual);
}

/*
 * Write N bytes to the port.
 * Return number of bytes sent, or -1 for error.
 */
int
serial_write(int port, const unsigned char *buf, int len)
{
	int	actual;
	leds(2, 2);

	for (actual = 0; actual < len; ++actual)
	    serial_putc(*buf++);

	leds(0, 2);
	return(actual);
}

/*
 * This routine sets the baud rate of the device to that specified by arg.
 */
int
serial_baud(int port, unsigned long baud)
{
	/* For an RS-422 port, this test must be removed. */
	if (baud > 115200)
	    return(ERR);
	serial_set(baud);
	return(OK);
}



/* Establish the loop/time constant to be used in the timing loop in
 * serial_read.  This is done by putting the UART into loopback mode.
 * After transmitting a character at 300 baud, we wait for the character
 * to be received.  Then divide the number of loops waited by the number
 * of milliseconds it takes to transmit 10 bits at 300 baud.
 * If your transmitter doesn't have a loopback mode, this value can be
 * calculated using a timer or some other facility, or an approximate
 * constant can be used.
 */

#define TESTBAUD 300L
#define NBYTES	 10
#define BITS_PER_BYTE 10	/* 1 start bit, 8 data bits, 1 stop bit */
#define TOTAL_MS (NBYTES*BITS_PER_BYTE*1000/TESTBAUD)

static int
calc_looperms(void)
{
	int	i, count, c;
	int	totalloops = 0;

	serial_init();
	serial_set(TESTBAUD);		/* set 300 baud */
	serial_loopback(1);		/* enable loop back mode */

	for (i=0; i < NBYTES; i++)
		{
   		count = 1;
		serial_putc(0xaa);	/* xmit character */

		/*
		 * The timing loop is the same as the loops in serial_read.
		 * Any changes to the loops in serial_read should be reflected
		 * here.
		 */
		do  {
		    c = serial_getc();
		    } while (c < 0 && count++ > 0);

		totalloops += count;
	    }

	serial_loopback(0);
	return(totalloops/TOTAL_MS);
}



int
serial_supported(void)
{
    return (TRUE);
}
