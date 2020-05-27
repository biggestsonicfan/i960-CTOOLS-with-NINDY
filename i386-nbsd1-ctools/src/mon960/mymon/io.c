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

#include "ui.h"
#include "i960.h"

extern int serial_read(int port, unsigned char *buf, int len, int timo);
extern int serial_write(int port, const unsigned char *buf, int len);
extern hi_co(), host_read(), host_write();

void co(int);
void prtf();

int hi_ui_cmd = FALSE;

#define LINESIZE	80
static unsigned char input_buffer[LINESIZE];
static int in = 0;	/* Offset in buffer at which next input char goes */
static int out = 0;	/* Offset in buffer from which next output char taken */


/************************************************/
/* read byte (timeout)                          */
/*                                              */
/* This routine waits until a character is      */
/* available at the console input.  If the char */
/* is available before the timeout on the serial*/
/* channel, it returns the character. Otherwise,*/
/* return TIMEOUT error.  			*/
/************************************************/

int
readbyte(seconds)
int 	seconds;
{
	unsigned char c;

	if (serial_read(0, &c, 1, seconds*1000) == 1)
	    return(c&0xff);
	else /* if timeout error, return TIMEOUT */
	    return(TIMEOUT);
}


/************************************************/
/* buffer_char:					*/
/*						*/
/* local routine to add character to input	*/
/* buffer.					*/
/************************************************/
static void
buffer_char( c )
    char c;
{
	input_buffer[in] = c;
	if (++in >= LINESIZE)
		in = 0;
}


void
init_sio_buf()
{
	in = out = 0;
}

/************************************************/
/* co:						*/
/*						*/
/* Send character to console, checking for	*/
/* XON/XOFF (don't check for XON/XOFF if an	*/
/* xmodem binary download is in progress	*/
/************************************************/
void
co(c)
int c;
{
	unsigned char tmp;

	if (host_connection)
		{
		if (hi_ui_cmd == FALSE)
		    {
	        tmp = c;
	        host_write(1, &tmp, 0, &c);
		    }
	    else
	    	hi_co(c);
        }
	else
	    {
	    if (!downld )
			{
		    /* we need to check for flow control before transmitting */
	    	if (serial_read(0, &tmp, 1, 1) == 1)
				{
	    	    /* there is a character at the input queue, read it */
	    	    if (tmp == XOFF)
	    		    while (serial_read(0, &tmp, 1, 0) == 1 && tmp != XON)
	        			    buffer_char(tmp);
				else
			        buffer_char(tmp);
		        }
	        }

	    /* transmit character */
	    tmp = c;
	    serial_write(1, &tmp, 1);
	    }
}


/************************************************/
/* ci:						*/
/*						*/
/* If characters are buffered, return next one.	*/
/* Otherwise read a character from the console.	*/
/************************************************/
unsigned char
ci()
{
	unsigned char c;

	if ( out != in )
		{
		/* Input buffer not empty: get character from there. */
		c = input_buffer[out];
		if ( ++out >= LINESIZE )
			out = 0;
	    }
	else
		{
	    if (host_connection)
			{
	        host_read(0, input_buffer, LINESIZE, &in);
			out = 0;
			c = input_buffer[out++];
			}
	    else
	    	/* get character from keyboard */
	    	serial_read(0, &c, 1, 0);
	    }
	return(c);
}



extern int sprtf();

/************************************************/
/* prtf						*/
/*   (1) provides a simple-minded equivalent	*/
/*       of the printf function.		*/
/*   (2) translates '\n' to \n\r'.		*/
/*						*/
/* In addition to the format string, up to 4	*/
/* arguments can be passed.			*/
/*						*/
/* Only the following specifications are	*/
/* recognized in the format string:		*/
/*						*/
/*	%B	output a single hex Byte, as 2	*/
/*		  hex digits with lead 0s.	*/
/*	%b	output a single hex Byte, up to	*/
/*		   2 digits without lead 0s.	*/
/*	%c	output a single character	*/
/*	%H	output a hex Half (short) word,	*/
/*		  as 4 hex digits with lead 0s.	*/
/*	%h	output a hex Half (short) word,	*/
/*		  up to 4 digits w/o lead 0s.	*/
/*	%s	output a character string.	*/
/*	%X	output a hex word, as 8 hex	*/
/*		  digits with lead 0s.		*/
/*	%x	output a hex word: up to 8 hex	*/
/*		  digits without lead 0s.	*/
/*	%%	output the character '%'.	*/
/************************************************/
void
prtf( fmt, arg0, arg1, arg2, arg3 )
char *fmt;
int arg0;
int arg1;
int arg2;
int arg3;
{
    unsigned char buffer[256+40];  /* protect against buffer overflow */
	int i, outptr = 0;

    if ((outptr = sprtf(buffer, 256, fmt, arg0, arg1, arg2, arg3)) == ERR)
		{
        prtf("PRTF Format Error\n");
		return;
		}

	if (host_connection && hi_ui_cmd == FALSE)
	    host_write(1, buffer, outptr, &i);
	else
	    for  (i=0; i<outptr; i++)
		    co(buffer[i]);
}
