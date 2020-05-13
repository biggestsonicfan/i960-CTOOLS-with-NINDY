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
#include "defines.h"
#include "globals.h"
#include "block_io.h"

extern unsigned prcb_ram;

unsigned char ci();

#define LINESIZE	80

static unsigned char input_buffer[LINESIZE];
static int in = 0;	/* Offset in buffer at which next input char goes */
static int out = 0;	/* Offset in buffer from which next output char taken */

/* Table of legal baudrates, terminated with illegal rate "0" */
int baudtable[] = { 1200, 2400, 4800, 9600, 19200, 38400, 0 };


/************************************************/
/*  Output					*/
/* 						*/
/* Memory mapped I/O routine, writes 8 bit value*/
/* to port 					*/
/************************************************/
output(port, value)
unsigned char *port, value;
{
	*port = value;
}

/************************************************/
/*  Input					*/
/* 						*/
/*  Memory mapped input routine, returns 8 bit  */
/* value from port				*/
/************************************************/
unsigned char input(port)
unsigned char *port;
{
	return(*port);
}

/************************************************/
/* read byte (timeout)                          */
/*                                              */
/* This routine waits until a character is      */
/* available at the console input.  If the char */
/* is available before the timeout on the serial*/
/* channel, it returns the character. Otherwise,*/
/* return TIMEOUT error.  			*/
/************************************************/

extern int readbyte_ticks_per_second;	/* Board-specific constant */

readbyte(seconds)
int 	seconds;
{
char c;
int ticks;

	for (ticks = seconds*readbyte_ticks_per_second; ticks > 0; ticks--){
		if (csts() != 0){
			return((int)ci());
		}
	}
	
	/* if timeout error, return TIMEOUT */
	return(TIMEOUT);
}

/************************************************/
/* console io                                   */
/*                                              */
/* Provide system services console io.		*/
/* This routine will be entered from "calls 0"  */
/* in the supervisior table, thus allowing an   */
/* application to execute code and do I/O to    */
/* the serial device of this monitor through    */
/* run-time binding 				*/
/************************************************/
console_io(type, chr)
int type, chr;
{
	switch (type) {
	case CI:
		*(unsigned char *)chr = ci();
		break;
	case CO:
		co(chr);
		break;
	default:
		return (ERROR);
	}

	return 0;
}


/************************************************/
/* Output Hex Number         			*/
/*                           			*/
/* output a 32 bit value in hex to the console  */
/* leading determines whether or not to print   */
/* leading zeros				*/
/************************************************/
static
out_hex(value, bits, leading)
unsigned int value;
int bits;	/* Number of low-order bits in 'value' to be output */
int leading;	/* leading 0 flag */
{
	static char tohex[] = "0123456789abcdef";
	unsigned char out;

	for (bits -= 4; bits >= 0; bits -= 4) {
		out = ((value >> bits) & 0xf); /* capture digit */
		if ( out == 0 ){
			if ( leading || (bits == 0) ){
				co('0');
			}
		} else {
			co( tohex[out] );
			leading = TRUE;
		}
	}
}

/************************************************/
/* buffer_char:					*/
/*						*/
/* local routine to add character to input	*/
/* buffer.					*/
/************************************************/
static
buffer_char( c )
    char c;
{
	input_buffer[in] = c;
	if ( ++in >= LINESIZE ){
		in = 0;
	}
}


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
co(c)
    unsigned char c;
{
	unsigned char tmp;

	if ( !downld ){
		/* we need to check for flow control before transmitting */
		while ( csts() ){
			/* there is a character at the input queue, read it */
			tmp = board_ci();
			if ( tmp == XOFF ){
				while ( (tmp=board_ci()) != XON ){
					buffer_char(tmp);
				}
			} else {
				buffer_char(tmp);
			}
		}
	}

	/* transmit character */
	board_co(c);
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

	if ( out != in ){
		/* Input buffer not empty: get character from there. */
		c = input_buffer[out];
		if ( ++out >= LINESIZE ){
			out = 0;
		}

	} else {
		/* get character from keyboard */
		c = board_ci();
	}
	return c;
}

/************************************************/
/* laser io                                     */
/*                                              */
/* Provide laser printer direct lpt I/O.	*/
/* This routine will be entered from "calls 2"  */
/* in the supervisior table, thus allowing an   */
/* application to execute code and do I/O to    */
/* the serial device of this monitor through    */
/* run-time binding 				*/
/************************************************/
lpt_io(type, chr)
int type, chr;
{

	switch (type) {
	case CI:
		*(unsigned char *)chr = ci();
		break;
	case CO:
		co(chr);
		break;
	default:
		return (ERROR);
	}

	return 0;
}

/***********************************************/
/* This routine allows the PRCB to be returned */
/* to the io libraries, in order to set        */
/* interrupts, modify control table, etc.      */
/***********************************************/
unsigned int get_prcb()
{
	return((int)&prcb_ram);
}

/************************************************/
/* Change the Baud rate           	 	*/
/*                           			*/
/************************************************/
baud( dummy, dummy2, baudrate )
int dummy;	/* Ignored */
int dummy2;	/* Ignored */
char *baudrate;
{
int i;
int baud;

	if ( atod(baudrate,&baud) && (i=lookup_baud(baud)) != ERROR ){
		change_baud(baud);
		end.r_baud = i;
		return TRUE;
	} else {
		prtf( "Bad baud rate specification" );
		return FALSE;
	}
}


/************************************************/
/* Return the offset of a baudrate in the table	*/
/* baudtable[], ERROR if not found.		*/
/*                           			*/
/************************************************/
int
lookup_baud(baudrate)
int baudrate;
{
int i;

	for ( i=0; baudtable[i]; i++ ){
		if ( baudtable[i] == baudrate ){
			return i;
		}
	}
	return ERROR;
}


/************************************************/
/* prtf						*/
/*   (1) provides a simple-minded equivalent	*/
/*       of the printf function.		*/
/*   (2) translates '\n' to \n\r'.		*/
/*   (3) does nothing (suppresses output) if we	*/
/*	 are running in gdb mode (i.e.,		*/
/*	 processing a remote debugger commmand).*/
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
/*	%X	output a heX word, as 8 hex	*/
/*		  digits with lead 0s.		*/
/*	%x	output a heX word: up to 8 hex	*/
/*		  digits without lead 0s.	*/
/*	%%	output the character '%'.	*/
/************************************************/
#define MAX_PRTF_ARGS 4

prtf( fmt, arg0, arg1, arg2, arg3 )
char *fmt;
int arg0;
int arg1;
int arg2;
int arg3;
{
int args[MAX_PRTF_ARGS];
int argc;
char *p;
char *q;

	if ( gdb ){
		return;		/* suppress output */
	}

	args[0] = arg0;
	args[1] = arg1;
	args[2] = arg2;
	args[3] = arg3;
	argc = 0;
	for ( p = fmt; *p; p++ ){
		if ( *p != '%' ){
			co( *p );
			if ( *p == '\n' ){
				co('\r');
			}
			continue;
		}

		/* Everything past this point is processing a '%'
		 * format specification.
		 */

		p++;	/* p -> character after the '%'	*/

		if ( *p == '\0' ){
			badfmt( fmt );
			return;
		}
		
		if ( *p == '%' ){
			co( *p );
			continue;
		}

		if ( argc >= MAX_PRTF_ARGS ){
			badfmt( fmt );
			return;
		}
		switch (*p){
		case 'B':
		case 'b':
			out_hex (args[argc], BYTE, (*p=='B') ? TRUE : FALSE);
			break;
		case 'c':
			co( args[argc] );
			if ( args[argc] == '\n' ){
				co('\r');
			}
			break;
		case 'H':
		case 'h':
			out_hex (args[argc], SHORT, (*p=='H') ? TRUE : FALSE);
			break;
		case 's':
			for ( q = (char*)args[argc]; *q; q++ ){
				co(*q);
				if ( *q == '\n' ){
					co('\r');
				}
			}
			break;
		case 'X':
		case 'x':
			out_hex (args[argc], INT, (*p=='X') ? TRUE : FALSE);
			break;
		default:
			badfmt( fmt );
			return;
		}
		argc++;
	}
}

static
badfmt( f )
char *f;
{
	prtf( "\nInternal error: bad prtf format: %s\n", f );
}

/************************************************/
/* gprtf					*/
/*	This is a version of prtf() that does   */
/*      suppress output while in gdb mode, and	*/
/*	so can be used to format replies to a	*/
/*	debugger on a remote host.		*/
/************************************************/
gprtf( fmt, arg0, arg1, arg2, arg3 )
char *fmt;
int arg0;
int arg1;
int arg2;
int arg3;
{
	char old_gdb;

	old_gdb = gdb;
	gdb = FALSE;
	prtf(fmt,arg0,arg1,arg2,arg3);
	gdb = old_gdb;
}
