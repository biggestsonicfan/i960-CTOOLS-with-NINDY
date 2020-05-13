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
#include "regs.h"

static int parser_state;		/* reentry state for COFF parser */

static struct {				/* FILE HEADER STRUCTURE	*/
	unsigned short	file_type;	/*	file type		*/
	unsigned short	num_secs;	/*	number of sections	*/
	long		time_date;	/*	time and date stamp	*/
	long		symtbl_ptr; 	/*	symbol table ptr	*/
	long		num_syms; 	/*	num entries in symtab	*/
	unsigned short	opt_hdr;	/*	size of optional header	*/
	unsigned short	flags;		/*	flags			*/
} filebuf;

/* The size of the above structures NOT INCLUDING any padding
 * added by the compiler for alignment of entries within arrays.
 */
#define FHDR_UNPADDED_SIZE (sizeof(filebuf.file_type) \
				+ sizeof(filebuf.num_secs) \
				+ sizeof(filebuf.time_date) \
				+ sizeof(filebuf.symtbl_ptr) \
				+ sizeof(filebuf.num_syms) \
				+ sizeof(filebuf.opt_hdr) \
				+ sizeof(filebuf.flags))

/* COFF parser states
 */
#define	P_INIT 		0	/* parser initialize */
#define	P_SECT 		1	/* parse sections */
#define	P_RAWDATA 	2	/* parse raw data */
#define	P_DONE 		3	/* parser finished */

/* XMODEM Constants
 */
#define ERRORMAX	15	/* max errors tolerated */
#define	XMODSIZE	128	/* size of xmodem buffer */


/************************************************/
/* Download                  			*/
/*                           			*/
/************************************************/
download(eflag)
int eflag;
{
	noerase = eflag;

	if (eflag && (fltype == 0) ){
		prtf ("\n Flash not implemented on this board");

	} else if (user_is_running) {
		prtf ("\n Already running program, new download");
		prtf ("\n not allowed. Reset board or allow program");
		prtf ("\n to complete to download new program.");

	} else {
		prtf ("\n Downloading");
		if (noerase){
			prtf (" to Flash");
		}

		parser_state = P_INIT;
		prtf ("\n Receiving file\n");

		downld = TRUE;
		if (receive_xmodem() == ERROR){
			prtf ("\n\r Download failed");
			while (readbyte(2) != TIMEOUT){
				;	/* wait for line to settle */
			}
		} else {
			eat_time(2);
			prtf("\n -- Download complete --\n");
			prtf("\n    Start address is : %X\n",aoutbuf.start_addr);
		}
		downld = FALSE;
	}
}

/****************************************/
/* COFF Parser 				*/
/* 					*/
/* The coff file is parsed as each      */
/* packet is received.  The state of    */
/* parsing is saved between each packet */
/* so that the parser can be reentered  */
/****************************************/
static
parse_coff( buff )
unsigned char *buff;
{
int i, j, n;
int size, a, b;
unsigned char *b_ptr;
unsigned char *p;
static unsigned char *secbuffptr;	/* pointer to section info */
static int buffindex;			/* place holder for xmodem buffer */
static int current_section;		/* section number		*/
static int current_byte;		/* byte within section		*/


switch (parser_state) {

case P_INIT: /* transfer buffer to file header info */

	for ( p=(unsigned char*)&filebuf, i=0; i<FHDR_UNPADDED_SIZE; p++, i++ ){
		*p = buff[i];
	}

	buffindex = FHDR_UNPADDED_SIZE;

	/* transfer buffer to optional info 
	   only if AOUT is present as OHDR */

	if (filebuf.opt_hdr >= AOUT_UNPADDED_SIZE){
		p = (unsigned char*)&aoutbuf;
		for (  i=0; i<AOUT_UNPADDED_SIZE; p++, i++ ){
			*p = buff[buffindex + i];
		}
	}
	
	buffindex += filebuf.opt_hdr;

	/* set up start section and start word */
	current_section = 0;
	current_byte = 0;

	if (buffindex > XMODSIZE)
		/* sorry, this condition shouldn't happen */
		return(ERROR);
	else if (buffindex == XMODSIZE)	{
		parser_state = P_SECT;
		return(0);
	}
	
	/* most likely case is some section stuff in first buffer */
	
case P_SECT:
	
	/* if code is from above, then don't reset buffer */
	if (parser_state == P_SECT) buffindex = 0;
	/* and away we go......... */

	for (i = current_section; i < filebuf.num_secs; i++) {
		p = (unsigned char*) &sectbuf[i];
		for (j=current_byte; j < SHDR_UNPADDED_SIZE; j++) {
			if (buffindex == XMODSIZE) {
				/* done with packet, save state */
				current_byte = j;
				current_section = i;
				parser_state = P_SECT;
				return (0);
			} else {
				/* need to process byte */
				p[j]= buff[buffindex];
				buffindex++;
			}
		}
		current_byte = 0;
	}
	/* done with headers, so continue on raw data */
	current_section = 0;
	current_byte = 0;
	/* check type of download */
	if (check_download(filebuf.num_secs) == ERROR)
		return (ERROR);

case P_RAWDATA:
	
	/* if code is from above, then don't reset buffer */
	if (parser_state == P_RAWDATA) buffindex = 0;

	/* transfer raw data to respective sections */
	for (i=current_section; i<filebuf.num_secs; i++) {
		if (current_byte == 0){
			secbuffptr = (unsigned char *)sectbuf[i].p_addr;
		}

		if (sectbuf[i].data_ptr == 0) {
			continue;	/* skip NOLOAD sections */
		}

		/* finish the section */
		for (j=current_byte; j<sectbuf[i].sec_size; j++) {

			/* done with packet, save state */
			if (buffindex == XMODSIZE) {
				current_byte = j;
				current_section = i;
				parser_state = P_RAWDATA;
				return (0);
			}

			/* process packet */
			b_ptr = (unsigned char *)(buff+buffindex);

			/* set size to smaller of bytes left in packet
			 * or bytes left in section
			 */
			a = XMODSIZE - buffindex;
			b = sectbuf[i].sec_size - current_byte;
			size = a < b ? a : b;

			if (downtype[i] == FLASH) {
				if (download_flash(secbuffptr,b_ptr,size)==ERROR)
					return (ERROR);
				secbuffptr += size;
			} else {
				int cnt;
				for ( cnt=0; cnt < size; cnt++ ){
					*secbuffptr++ = *b_ptr++;
				}
			}
			buffindex += size;
			j += (size - 1);
		}
		current_byte = 0;
	}
	parser_state = P_DONE;


case P_DONE:
	/* YEA!  we're done! */
		return (0);
}
}


/****************************************/
/* Receive Xmodem Transmission		*/
/* 					*/
/* this function is in charge of the    */
/* handshaking protocol between the     */
/* monitor and Xmodem.			*/
/****************************************/
static
receive_xmodem()
{
int c;			/* 1st byte of packet */
int errors;		/* Running count of errors */
int errorflag;		/* True when error found while reading a packet	*/
int fatalerror;		/* True when fatal error found while reading packet */
unsigned char pktnum;	/* 2nd byte of packet: packet # (mod 128) */
unsigned char pktchk;	/* 3rd byte of packet: should be complement of pktnum */
int pktcnt;		/* total # of packets received so far */
unsigned char buff[XMODSIZE+10];/* buffer for data		*/

	fatalerror = FALSE;
	errors = pktcnt = 0;

	eat_time(4);
	co(NAK);	/* send NAK to start sequence */

	/* MAIN Do-While loop to read packets */
	do {
		errorflag = FALSE;

		/* start by reading first byte in packet */

		switch ( c = readbyte(6) ){

		default:
			continue;

		case ACK:
			if (pktcnt > 0){
				continue;
			}
			break;

		case EOT:
			/* check for REAL EOT */
			if ((c = readbyte(1)) == TIMEOUT) {
				c = EOT;
			} else if (c != EOT) {
				c = TIMEOUT;
				errorflag = TRUE;
			}
			break;

		case TIMEOUT:
			errorflag = TRUE;
			break;

		case CAN: 	/* bailing out? */
			if ((readbyte(3) & 0x7f) != CAN){
				errorflag = TRUE;
			} else {
				fatalerror = TRUE;
			}
			break;

		case SOH:
			/* start reading packet */
			pktnum = readbyte(3);
			pktchk = readbyte(3);

			if (pktnum != (unsigned char)~pktchk) {
				/* MISREAD PACKET # */
				errorflag = TRUE;
				break;
			}

			if (pktnum == (pktcnt & 0xff)){
				/* DUPLICATE PACKET -- DISCARD */
				while (readbyte(3) != TIMEOUT){ ; }
				co(ACK);
				break;
			}

			if ( pktnum != ((pktcnt+1) & 0xff) ){
				/* PHASE ERROR */
				fatalerror = TRUE;
				break;
			}


			/* Read and calculate checksum for a packet of data */

			if ( !readpkt(buff) ){
				errorflag = TRUE;
				break;
			}

			errors = 0;
			pktcnt++;
			if (parse_coff(buff)== ERROR) {
				fatalerror = TRUE;
				break;
			}

			/* ACK the packet */
			co(ACK);
			break;

		}	/* END OF 'readbyte' SWITCH */

		/* check on errors or batch transfers */
		if (errorflag || pktcnt == 0) {
			if (errorflag){
				errors++;
			}


			if ( !fatalerror ){
				/* wait for line to settle */
				while (readbyte(2) != TIMEOUT){
					;
				}
				co(NAK);
			}
		}

	} while ((c != EOT) && (errors < ERRORMAX) && (!fatalerror));

	/* end of MAIN Do-While */

	if ((c == EOT) && (errors < ERRORMAX)) {
		/* normal exit */
		co(ACK);
		return(0);
	} else {
		/* error exit */
		if (pktcnt != 0) {
			co(CAN);
			co(CAN);
		}
		return (ERROR);
	}
}

/************************************************/
/* read buffer                                  */
/*                                              */
/* Get a buffer (XMODSIZE bytes) from uart.	*/
/* Timeout if 3 "seconds" elapse.	 	*/
/*                                              */
/* Return 1 on success, 0 on TIMEOUT or bad     */
/*	checksum.				*/
/************************************************/
static
readpkt(buff)
unsigned char *buff;
{
int count;	 	/* Number of characters read */
int c;			/* Next character read */
unsigned char chksum;	/* accumulate checksum here */

	chksum = 0;

	for (count=0; count<XMODSIZE; count++) {
		buff[count] = c = readbyte(3);
		if ( c == TIMEOUT ){
			return 0;	/* abort operation */
		}
	       	chksum += c;
	}

	/* Read and confirm checksum */
	c = readbyte(3);
	if ( (c == TIMEOUT) || ((c & 0xff) != chksum) ){
		return 0;
	}

	return 1;
}



cancel_download()
{
	if (downld) {
		co (CAN);
		co (CAN);
		eat_time(3);
	}
}
