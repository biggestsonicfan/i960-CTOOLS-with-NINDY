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
#include "evca510.h"
#include "block_io.h"

#define SLOW_OUTPUT_LOOPS 175

unsigned char input_buffer[80];
unsigned char *input_ptr;

/****************************************/
/* init_console  			*/
/* 					*/
/* this routine initializes the 82510   */
/* for operation in the polled mode,    */
/* interrupts disabled.  It accepts an  */
/* integer designating the baud rate,   */
/* which is computed from the input     */
/* frequency				*/
/****************************************/
init_console(baud)
int baud;
{
unsigned char *port510;
unsigned char data;

	/* initialize the 82510 */

	output(BANK, WORK1);
	eat_time(1);
	output(ICM, 0x10);	/* software reset */
	eat_time(1);
	output(LCR, 0x03);	/* 8 bits, no parity */
	eat_time(1);
	output(MCR, 0x01);	/* enable DTR */
	eat_time(1);

	output(LCR, (input(LCR) | 0x80));	/* enable BAUD */
	output(BAL, ((XTAL/(32*baud)) & 0xff ));	/* baud low */
	output(BAH, (((XTAL/(32*baud)) & 0xff00) >> 8)); /* baud high */
	output(LCR, (input(LCR) & 0x7f));	/* disable BAUD */

	/* configure the silly BRGB port */

	output(BANK, MODM3);
	eat_time(1);
	output(CLCF, 0x50); /* source is BRGA */
	eat_time(1);
	output(BBCF, 0x00); /* tell BRGB to be a TIMER */
	eat_time(1);

	output(BANK, NAS0);	/* return to bank 0 for operation */
	eat_time(1);
	output(GER, 0x00);	/* no interrupts */
	eat_time(1);

	/* now initialize the character input buffer */
	init_sio_buf();
}

/****************************************/
/* Change Baud Rate                     */
/* 		           	        */
/*  for the serial device 		*/
/****************************************/
change_baud(baud)
int baud;
{
unsigned char data;

	output(LCR, (input(LCR) | 0x80));	/* enable BAUD */
	output(BAL, ((XTAL/(32*baud)) & 0xff ));	/* baud low */
	output(BAH, (((XTAL/(32*baud)) & 0xff00) >> 8)); /* baud high */
	output(LCR, (input(LCR) & 0x7f));	/* disable BAUD */

	/* configure the silly BRGB port */

	output(BANK, MODM3);
	output(CLCF, 0x50); /* source is BRGA */
	output(BBCF, 0x00); /* tell BRGB to be a TIMER */

	output(BANK, NAS0);	/* return to bank 0 for operation */

}
/************************************************/
/*  Csts - Character status			*/
/* 						*/
/* returns 0 if no character available at the  	*/
/* device, nonzero if a character is available	*/
/************************************************/
csts()
{
	return (input(LSR) & 0x01); 	/* return status */
}

/****************************************/
/* board_ci:				*/
/*					*/
/* wait for character to appear at UART.*/
/* Return it to caller.			*/
/****************************************/
unsigned char
board_ci()
{
	while ( !csts() ){
		;		/* wait for char */
	}
	return(input(RXD));  	/* read and return char */
}


/****************************************/
/* board_co:				*/
/*					*/
/* wait for UART to become available,	*/
/* then xmit the character.		*/
/****************************************/
board_co(c)
{
	/* transmit character */
	while ( !(input(LSR) & 0x20) ){
		;		/* wait for XMIT buffer to clear */
	}
	output(TXD,c);		/* output char */
}

/****************************************/
/* file_io:				*/
/*					*/
/* Request service from remote host	*/
/****************************************/
file_io(op, arg1, arg2, arg3)
int op, arg1, arg2, arg3;
{
	return remote_srq(op, arg1, arg2, arg3);
}
