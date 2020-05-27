
/*(c**************************************************************************** *
 * Copyright (c) 1990, 1991, 1992, 1993 Intel Corporation
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
 ***************************************************************************c)*/

/******************************************************************************
 *
 *	 		NINDY INTERFACE ROUTINES
 *
 * The routines in this file define and implement an interface between code
 * (such as a high-level debugger) running on a remote host and the NINDY
 * ROM monitor on an i960 board.  These routines are to be linked with 
 * and called by the host code.
 *
 * These routines handle both the formatting/transferring of commands to NINDY
 * and the receipt/formatting of data returned in response to them.  The
 * actual transfer protocol is hidden from the host programmer within them.
 * For a full description of the lowest level NINDY/host transfer protocol,
 * see the block header of the file gdb.c, in the NINDY source code.
 *
 * The caller of these routines should be aware that:
 *
 * (1) ninConnect() should be called to open communications with the
 *     remote NINDY board before any of the other routines are invoked.
 *
 * (2) almost all interactions are driven by the host: nindy sends information
 *     in response to host commands.
 *
 * (3) the lone exception to (2) is the single character DLE (^P, 0x10).
 *     Receipt of a DLE from NINDY indicates that the application program
 *     running under NINDY has stopped execution and that NINDY is now
 *     available to talk to the host (all other communication received after
 *     the application has been started should be presumed to come from the
 *     application and should be passed on by the host to stdout).
 *
 * (4) the reason the application program stopped can be determined with the
 *     ninStopWhy() function.  There are three classes of stop reasons:
 *
 *	(a) the application has terminated execution.
 *	    The host should take appropriate action.
 *
 *	(b) the application had a fault or trace event.
 *	    The host should take appropriate action.
 *
 *	(c) the application wishes to make a service request (srq) of the host;
 *	    e.g., to open/close a file, read/write a file, etc.  The ninSrq()
 *	    function should be called to determine the nature of the request
 *	    and process it.
 *
 * WARNING: Changes made here should be tested in both gdb960 and comm960.
 *
 ******************************************************************************/

#include <stdio.h>
#include <sys/types.h>	/* Needed by file.h on Sys V */
#include <signal.h>
#include <fcntl.h>
#include "demux.h" /* Pulls in sys/select.h on posix systems, fcntl.h on Sys V*/
#include "ttycntl.h"

#ifdef DOS
#	include "gnudos.h"
#	define EXEC_SX	exec_sx
#else
#	include <sys/file.h>
#	ifndef tower_68k
#		include <sys/ioctl.h>
#	endif /* tower_68k */
#	define EXEC_SX	execlp
	extern char *malloc();
	extern void free();
#endif /*DOS*/

#include "block_io.h"
#include "wait.h"

#ifdef USG
#if !defined(__HIGHC__) && !defined(WIN95)
#	include <unistd.h>
#endif
#	include "sysv.h"
#else	/* BSD */
#	include "string.h"
#	include <sys/time.h>
#endif

#ifdef __STDC__
#include <stdarg.h>
static void say( char *fmt, ... );
#else
#include <varargs.h>
static void say();
#endif

#ifndef ERROR
#define ERROR	-1
#endif

#define DLE	0x10	/* ^P */
#define XON	0x11	/* ^Q */
#define XOFF	0x13	/* ^S */
#define ESC	0x1b

#define REGISTER_BYTES	((36*4) + (4*8))

#define TIMEOUT		-1

static int nindy_fd;	/* File descriptor of tty connected to 960/NINDY board*/
static int quiet = 0;	/* 1 => stifle unnecessary messages		*/
static ninStrGet();
char *exists();		/* In cstrip.c */

/*******
 * The following array contains the values of MCON0-MCON14 registers, i.e.,
 * the bus configuration for memory regions 0-E.  These values have to
 * be downloaded at runtime for some NINDY boards (e.g., CADIC, which
 * piggy-backs onto a target board).  NINDY is assumed to be running out
 * of Region F, which never gets downloaded.
 *
 * This array gets loaded by the application when it calls ninConfRegs().
 * Its contents are downloaded when the array is first loaded, and again
 * every time NINDY is reset via a ninReset() call.
 *
 * The array does not contain valid information if the flag 'have_cx_conf_regs'
 * is not set.
 *
 * (NOTE:  this array is *always* meaningless if NINDY is running on a
 * KX-architecture 80960).
 *******/
static unsigned long cx_conf_regs[15];
static char have_cx_conf_regs = 0;

		/****************************
		 *                          *
		 *  MISCELLANEOUS UTILTIES  *
		 *                          *
		 ****************************/


/******************************************************************************
 * get_int:
 *	Copy the little-endian integer pointed at by 'p'  and return it in
 *	the host byte order.  'p' may be an unaligned address, so do the copy
 *	a byte at a time.
 ******************************************************************************/
int
get_int( p )
    unsigned char *p;
{
	int n;
	int i;

	n = 0;
	p += sizeof(int) - 1;
	for ( i = 0; i < sizeof(n); i++ ){
		n <<= 8;
		n |= *p--;
	}

	return n;
}


/******************************************************************************
 * put_int:
 *	Copy the integer 'n' (which is in host byte order) to the location
 *	pointed at by 'p', leaving it in little-endian byte order.
 *	'p' may be an unaligned address, so do the move a byte at a time.
 ******************************************************************************/
void
put_int( p, n )
    unsigned char *p;
    int n;
{
	int i;

	for ( i = 0; i < sizeof(n); i++ ){
		*p++ = n;
		n >>= 8;
	}
}


/******************************************************************************
 * say:
 *	This is a printf that takes at most two arguments (in addition to the
 *	format string) and that outputs nothing if verbose output has been
 *	suppressed.
 ******************************************************************************/
#ifdef __STDC__
/* ANSI-style stdarg system */

static void
say( char *fmt, ... )
{
	va_list args;
	va_start (args, fmt);
	if ( !quiet ){
		vprintf( fmt, args );
		fflush( stdout );
	}
	va_end (args);
}

#else /* Not __STDC__ */
/* K&R-style varargs system */

static void
say( va_alist )
	va_dcl
{
	va_list args;
	char *fmt;
	va_start (args);
	fmt = va_arg(args, char *);
	if ( !quiet ){
		vprintf( fmt, args );
		fflush( stdout );
	}
	va_end (args);
}

#endif  /* __STDC__ */

		/*****************************
		 *                           *
		 *  LOW-LEVEL COMMUNICATION  *
		 *                           *
		 *****************************/

/******************************************************************************
 * timed_read:
 *	Read up to 'n' characters (less if fewer are available) from the NINDY
 *	tty. Wait up to 'timeout' seconds for something to arrive.  Return
 *	the number of characters read, 0 on timeout.
 ******************************************************************************/
#ifdef USG

static int saw_alarm;

static void
alarm_handler(dummy)
int dummy;
{
	saw_alarm = 1;
}


static
int
timed_read(buf,n,timeout)
    unsigned char * buf;	/* Where to put the read characters	*/
    int n;			/* Max number of characters to read	*/
    int timeout;		/* Timeout, in seconds			*/
{
        void (*old_alarm)();    /* Save alarm signal handler here on entry */
	int cnt;


#ifdef DOS
	/***********************************************************
	**  If this is DOS, call the serial port timed routine.
	**  If for some reason - like the nindy_fd is not the serial
	**  port - go thru the existing timed read.
	************************************************************/
	cnt = timed_read_port(nindy_fd, buf, n, timeout);
	if (cnt != -2) return(cnt);
#endif /*DOS*/

        old_alarm = signal( SIGALRM,alarm_handler );
        saw_alarm = 0;
        alarm(timeout);
        do {
		cnt = n;
                TTY_NBREAD(nindy_fd,cnt,buf);
        } while ( (cnt <= 0) && !saw_alarm );

        alarm(0);
        signal( SIGALRM,old_alarm );

	return saw_alarm ? 0 : cnt;
}

#else		/* BSD */

static
int
timed_read(buf,n,timeout)
    unsigned char * buf;	/* Where to put the read characters	*/
    int n;			/* Max number of characters to read	*/
    int timeout;		/* Timeout, in seconds			*/
{
	struct timeval t;
	fd_set f;

	t.tv_sec = (long) timeout;
	t.tv_usec= 0;

	FD_ZERO( &f );
	FD_SET( nindy_fd, &f );
	if ( select(nindy_fd+1,&f,0,0,&t) ){
		return READ_TTY( nindy_fd, buf, n );
	} else {
		return 0;
	}
}
#endif

/******************************************************************************
 * rdnin:
 *	Read *exactly* 'n' characters from the NINDY tty.  Translate escape
 *	sequences into single characters, counting each such sequence as 
 *	1 character.
 *
 *	An escape sequence consists of ESC and a following character.  The
 *	ESC is discarded and the other character gets bit 0x40 cleared --
 *	thus ESC P == ^P, ESC S == ^S, ESC [ == ESC, etc.
 *
 *	Return 1 if successful, 0 if more than 'timeout' seconds pass without
 *	any input.
 ******************************************************************************/
static
int
rdnin(buf,n,timeout)
    unsigned char * buf;	/* Where to place characters read	*/
    int n;			/* Number of characters to read		*/
    int timeout;		/* Timeout, in seconds			*/
{
	static unsigned char *mybuf = NULL;
				/* Dynamically allocated local buffer */
	static int mybuflen = 0;
				/* Current size of local buffer	*/
	int escape_seen;	/* 1 => last character of a read was an ESC */
	int nread;		/* Number of chars returned by timed_read() */
	unsigned char c;
	int i;

	/* Make sure local buffer is big enough
	 */
	if ( n > mybuflen ){
		if ( mybuf ){
			free( mybuf );
		}
		mybuf = (unsigned char *) malloc( mybuflen=n );
	}


	/* More than one loop will be necessary if there are any
	 * escape sequences in the input
	 */
	escape_seen = 0;
	while ( n ){
		nread = timed_read(mybuf,n,timeout);
		if ( nread <= 0 ){
			return 0;	/* TIMED OUT */
		}

		/* Copy characters from local buffer to caller's buffer,
		 * converting escape sequences as they're encountered.
		 */
		for ( i = 0; i < nread; i++ ){
			c = mybuf[i];
			if ( escape_seen ){
				escape_seen = 0;
				c &= ~0x40;
			} else if ( c == ESC ){
				if ( ++i >= nread ){
					/* Need to refill local buffer */
					escape_seen = 1;
					break;
				}
				c = mybuf[i] & ~0x40;
			}
			*buf++ = c;
			n--;
		}
	}
	return 1;
}


/******************************************************************************
 * getpkt:
 *	Read a packet from a remote NINDY, with error checking, into the
 *	indicated buffer.
 *
 *	Return packet status byte on success, TIMEOUT on failure.
 ******************************************************************************/
static
int
getpkt(buf)
     unsigned char *buf;
{
	int i;
	unsigned char hdr[3];	/* Packet header:
				 *	hdr[0] = low byte of message length
				 *	hdr[1] = high byte of message length
				 *	hdr[2] = message status
				 */
	int cnt;		/* Message length (status byte + data)	*/
	unsigned char cs_calc;	/* Checksum calculated			*/
	unsigned char cs_recv;	/* Checksum received			*/
	static char errfmt[] =
			"Bad checksum (recv=0x%02x; calc=0x%02x); retrying\n";

	while (1){
		if ( !rdnin(hdr,3,5) ){
			return TIMEOUT;
		}
		cnt = (hdr[1]<<8) + hdr[0] - 1;
					/* -1 for status byte (already read) */

		/* Caller's buffer may only be big enough for message body,
		 * without status byte and checksum, so make sure to read
		 * checksum into a separate buffer.
		 */
		if ( !rdnin(buf,cnt,5) || !rdnin(&cs_recv,1,5) ){
			return TIMEOUT;
		}

		/* Calculate checksum
		 */
		cs_calc = hdr[0] + hdr[1] + hdr[2];
		for ( i = 0; i < cnt; i++ ){
			cs_calc += buf[i];
		}
		if ( cs_calc == cs_recv ){
			WRITE_TTY (nindy_fd, "+", 1);
			return hdr[2];
		}
	
		/* Bad checksum: report, send NAK, and re-receive
		 */
		fprintf(stderr, errfmt, cs_recv, cs_calc );
		WRITE_TTY (nindy_fd, "-", 1);
	}
}


/******************************************************************************
 * putpkt:
 *	Send a packet to NINDY, checksumming it and converting special
 *	characters to escape sequences.
 ******************************************************************************/

/* This macro puts the character 'c' into the buffer pointed at by 'p',
 * and increments the pointer.  If 'c' is one of the 4 special characters
 * in the transmission protocol, it is converted into a 2-character
 * escape sequence.
 */
#define PUTBUF(c,p)						\
	if ( c == DLE || c == ESC || c == XON || c == XOFF ){	\
		*p++ = ESC;					\
		*p++ = c | 0x40;				\
	} else {						\
		*p++ = c;					\
	}

static
putpkt( msg, len )
    unsigned char *msg;	/* Command to be sent, without lead ^P or checksum */
    int len;	/* Number of bytes in message			*/
{
	static char *buf = NULL;/* Local buffer -- build packet here	*/
	static int maxbuf = 0;	/* Current length of buffer		*/
	unsigned char ack;	/* Response received from NINDY		*/
	unsigned char checksum;	/* Packet checksum			*/
	char *p;		/* Pointer into buffer			*/
	int lenhi, lenlo; 	/* High and low bytes of message length	*/
	int i;


	/* Make sure local buffer is big enough.  Must include space for
	 * packet length, message body, and checksum.  And in the worst
	 * case, each character would expand into a 2-character escape
	 * sequence.
	 */
	if ( maxbuf < ((2*len)+10) ){
		if ( buf ){
			free( buf );
		}
		buf = malloc( maxbuf=((2*len)+10) );
	}

	/* Attention, NINDY!
	 */
	WRITE_TTY( nindy_fd, "\020", 1 );


	lenlo = len & 0xff;
	lenhi = (len>>8) & 0xff;
	checksum = lenlo + lenhi;
	p = buf;

	PUTBUF( lenlo, p );
	PUTBUF( lenhi, p );

	for ( i=0; i<len; i++ ){
		PUTBUF( msg[i], p );
		checksum += msg[i];
	}

	PUTBUF( checksum, p );

	/* Send checksummed message over and over until we get a positive ack
	 */
	WRITE_TTY( nindy_fd, buf, p-buf );
	while (1){
		if ( !rdnin(&ack,1,5) ){
			/* timed out */
			fprintf(stderr,"ACK timed out; resending\n");
			WRITE_TTY(nindy_fd,"\020",1);/* Attention, NINDY! */
			WRITE_TTY( nindy_fd, buf, p-buf );
		} else if ( ack == '+' ){
			return;
		} else if ( ack == '-' ){
			fprintf( stderr, "Remote NAK; resending\n" );
			WRITE_TTY( nindy_fd, buf, p-buf );
		} else {
			fprintf( stderr, "Bad ACK, ignored: <%c>\n", ack );
		}
	}
}



/******************************************************************************
 * send:
 *	Send a message to a remote NINDY.  Check message status byte
 *	for error responses.  If no error, return NINDY reponse (if any).
 ******************************************************************************/
static
send( out, len, in )
    char *out;	/* Message to be sent to NINDY			*/
    int len;	/* Number of meaningful bytes in out buffer	*/
    char *in;	/* Where to put response received from NINDY	*/
{
	char *fmt;
	int status;
	static char *errmsg[] = {
		"",						/*  0 */
		"Buffer overflow",				/*  1 */
		"Unknown command",				/*  2 */
		"Wrong amount of data to load register(s)",	/*  3 */
		"Missing command argument(s)",			/*  4 */
		"Odd number of digits sent to load memory",	/*  5 */
		"Unknown register name",			/*  6 */
		"No such memory segment",			/*  7 */
		"No breakpoint available",			/*  8 */
		"Can't set requested baud rate",		/*  9 */
		"Command can't be excuted on this architecture",/* 10 */
	};
#	define NUMERRS	( sizeof(errmsg) / sizeof(errmsg[0]) )

	static char err1[] = "Unknown error response from NINDY: #%d\n";
	static char err2[] = "Error response #%d from NINDY: %s\n";

	while (1){
		putpkt(out,len);
		status = getpkt(in);
		if ( status == TIMEOUT ){
			fprintf( stderr, "Response timed out; resending\n" );
		} else {
			break;
		}
	}

	if ( status ){
		fmt =  status > NUMERRS ? err1 : err2;
		fprintf( stderr, fmt, status, errmsg[status] );
		abort();
	}
}

		/************************
		 *                      *
		 *  BAUD RATE ROUTINES  *
		 *                      *
		 ************************/

/* Table of baudrates known to be acceptable to NINDY.  Each baud rate
 * appears both as character string and as a Unix baud rate constant.
 */
struct baudrate {
	int standard;
	char *string;
	int rate;
};

static struct baudrate baudtab[] = {
    {1, "9600", B9600},

#ifdef B19200
    {1, "19200", B19200},
#endif

#ifdef B38400
    {1, "38400", B38400},
#endif

#ifdef B57600
    {1, "57600",B57600},
#else
#ifdef B50
    {0, "57600",B50},
#endif
#endif

#ifdef B76800
    {1, "76800",B76800},
#else
#ifdef B75
    {0, "76800",B75},
#endif
#endif

#ifdef B115200
    {1, "115200",B115200},
#else
#ifdef B110
    {0, "115200",B110},
#endif
#endif

    {1, "4800", B4800},
    {1, "2400", B2400},
    {1, "1200", B1200},
    {1,	NULL,    0}		/* End of table */
};
 

/******************************************************************************
 * parse_baudrate:
 *	Look up the passed baud rate in the baudrate table.  If found, change
 *	our internal record of the current baud rate, but don't do anything
 *	about the tty just now.
 *
 *	Return pointer to baudrate structure on success, NULL on failure.
 ******************************************************************************/
static
struct baudrate *
parse_baudrate(s)
    char *s;	/* Desired baud rate, as an ASCII (decimal) string */
{
	int i;

	for ( i=0; baudtab[i].string != NULL; i++ ){
		if ( !strcmp(baudtab[i].string,s) ){
			return &baudtab[i];
		}
	}
	return NULL;
}

/******************************************************************************
 * try_baudrate:
 *	Try speaking to NINDY via the specified file descriptor at the
 *	specified baudrate.  Assume success if we can send an empty command
 *	with a bogus checksum and receive a NAK (response of '-') back within
 *	one second.
 *
 *	Return 1 on success, 0 on failure.
 ******************************************************************************/

static int
try_baudrate( fd, brp )
    int fd;
    struct baudrate *brp;
{
	TTY_STRUCT tty;
	unsigned char c;
	
	/* Set specified baud rate and flush all pending input */
	TTY_GET(fd,tty);
	TTY_REMOTE(tty,brp->rate);
	TTY_SET(fd,tty);
	tty_flush( fd );

	/* Send empty command with bad checksum, hope for NAK ('-') response */
	WRITE_TTY( fd, "\020\0\0\001", 4 );
	if ( !rdnin(&c,1,1) ){
		/* timed out */
		return 0;
	} else {
		return (c == '-');
	}
}

/******************************************************************************
 * autobaud:
 *	Get NINDY talking over the specified file descriptor at the specified
 *	baud rate.  First see if NINDY's already talking at 'baudrate'.  If
 *	not, run through all the legal baudrates in 'baudtab' until one works,
 *	and then tell NINDY to talk at 'baudrate' instead.
 ******************************************************************************/
static
autobaud( fd, brp )
    int fd;
    struct baudrate *brp;
{
	int i;
	int failures;
	TTY_STRUCT tty;


	say("NINDY at wrong baud rate? Trying to autobaud...\n");
	failures = i = 0;
	while ( 1 ){
	    if(baudtab[i].standard && (strcmp(baudtab[i].string, brp->string) || (failures))){
		say( "\r%s...   ", baudtab[i].string );
		if ( try_baudrate(fd, &baudtab[i]) ){
			break;
		}
		if ( baudtab[++i].string == NULL ){
			/* End of table -- wraparound */
			i = 0;
			if ( failures++ ){
				say("\nAutobaud failed again.  Giving up.\n");
#ifdef	DOS
				exitdos(1);
#else
				exit(1);
#endif
			} else {
				say("\nAutobaud failed. Trying again...\n");
			}
		}
	    }
	    else
	      i++;
	}

	/* Found NINDY's current baud rate;  now change it.
	 */
	say("\rChanging NINDY baudrate to %s\n", brp->string);
	ninBaud( brp->string );

	/* Change our baud rate back to rate to which we just set NINDY.
	 */
	TTY_GET(fd,tty);
	TTY_REMOTE(tty,brp->rate);
	TTY_SET(fd,tty);
}

		/**********************************
		 *				  *
		 *   NINDY INTERFACE ROUTINES	  *
		 *                            	  *
		 * ninConnect *MUST* be the first *
		 * one of these routines called.  *
		 **********************************/

/******************************************************************************
 * ninBaud:
 *	Ask NINDY to change the baud rate on its serial port.
 *	Assumes we know the baud rate at which NINDY's currently talking.
 ******************************************************************************/
ninBaud( baudrate )
    char *baudrate;	/* Desired baud rate, as a string of ASCII decimal
			 * digits.
			 */
{
	char msg[100];

	tty_flush( nindy_fd );

	/* Can't use "send" because NINDY reply will be unreadable after
	 * baud rate change.
	 */
	sprintf( msg, "z%s", baudrate );
	putpkt( msg, strlen(msg)+1 );	/* "+1" to send terminator too */
}


/******************************************************************************
 * ninBptDel:
 *	Ask NINDY to delete the specified type of *hardware* breakpoint at
 *	the specified address.  If the 'addr' is -1, all breakpoints of
 *	the specified type are deleted.
 ******************************************************************************/
ninBptDel( addr, type )
    long addr;	/* Address in 960 memory	*/
    char type;	/* 'd' => data bkpt, 'i' => instruction breakpoint */
{
	char buf[10];

	buf[0] = 'b';
	buf[1] = type;

	if ( addr == -1 ){
		send( buf, 2, NULL );
	} else {
		put_int( &buf[2], addr );
		send( buf, 6, NULL );
	}
}


/******************************************************************************
 * ninBptSet:
 *	Ask NINDY to set the specified type of *hardware* breakpoint at
 *	the specified address.
 ******************************************************************************/
ninBptSet( addr, type )
    long addr;	/* Address in 960 memory	*/
    char type;	/* 'd' => data bkpt, 'i' => instruction breakpoint */
{
	char buf[10];

	buf[0] = 'B';
	buf[1] = type;
	put_int( &buf[2], addr );
	send( buf, 6, NULL );
}


/******************************************************************************
 * ninConfRegs:
 *	Send values for the MCON0-MCON14 registers (bus configuration for
 *	memory regions 0-E) to NINDY (which will load them into the processor).
 *	Save a copy of the registers to be downloaded every time NINDY is
 *	reset.
 ******************************************************************************/
ninConfRegs( regs )
    unsigned long regs[];	/* Values for MCON0-MCON14 (15 entries) */
{
	char buf[1+(15*4)];	/* Build NINDY command here	*/
	char *p;		/* Pointer into 'buf'		*/
	int i;			/* Loop counter			*/


	buf[0] = 'a';
	for ( i=0, p = &buf[1]; i < 15; i++, p +=4 ){
		cx_conf_regs[i] = regs[i];
		put_int( p, cx_conf_regs[i] );
	}
	have_cx_conf_regs = 1;
	send( buf, sizeof(buf), NULL );
}


/******************************************************************************
 * ninConnect:
 *	Open the specified tty.  Get communications working at the specified
 *	baud rate.  Flush any pending I/O on the tty.
 *
 *	Return the file descriptor, or -1 on failure.
 ******************************************************************************/
int
ninConnect( name, baudrate, brk, silent )
    char *name;		/* "/dev/ttyXX" to be opened			*/
    char *baudrate;     /* baud rate: a string, eg "38400"; NULL => use default */
    int brk;		/* 1 => send break to tty first thing after opening it*/
    int silent;		/* 1 => stifle unnecessary messages when talking to 
			 *	this tty.
			 */
{
	int i;
	char *p;
	struct baudrate *brp;

	/* We will try each of the following paths when trying to open the tty
	 */
	static char *prefix[] = { "", "/dev/", "/dev/tty", NULL };

	quiet = silent;		/* Make global to this file */

	for ( i=0; prefix[i] != NULL; i++ ){
		p = malloc(strlen(prefix[i]) + strlen(name) + 1 );
		strcpy( p, prefix[i] );
		strcat( p, name );
#ifdef DOS
                baudrate = set_dos_baud(baudrate);
#else
		if ( baudrate == NULL )
			baudrate = GBAUD_DFLT;
#endif


#ifdef USG
		nindy_fd = OPEN_TTY(p,(O_RDWR | O_EXCL | O_NDELAY));
		{
		  int n;
		  unsigned char buf[32];
		  TTY_STRUCT nindytty;

		  n = 32;
		  TTY_GET(nindy_fd,nindytty);
		  TTY_REMOTE(nindytty, B9600); /* use this to get CLOCAL */
		  TTY_SET(nindy_fd, nindytty);
		  TTY_NBREAD(nindy_fd, n, buf);/* use this to clear O_NDELAY */
		}
#else
		nindy_fd = OPEN_TTY(p,O_RDWR | O_NDELAY);
#endif
		if ( nindy_fd >= 0 ){
#ifndef USG
#ifdef TIOCEXCL
			/* Exclusive use mode (hp9000 does not support it) */
		        ioctl(nindy_fd,TIOCEXCL,NULL);
#endif
#endif
			if ( brk ){
				send_break( nindy_fd );
			}
   
                        brp = parse_baudrate( baudrate );
	                if ( brp == NULL ){
		           say("Illegal baudrate %s ignored; using %s\n", 
                                                         baudrate, GBAUD_DFLT);
		           brp = parse_baudrate( GBAUD_DFLT );
	                }

			if ( !try_baudrate(nindy_fd,brp) && !try_baudrate(nindy_fd,brp) ){
				autobaud(nindy_fd,brp);
			}

			tty_flush( nindy_fd );
			say( "Connected to %s at %s bps.\n", p, brp->string );
			free(p);
			break;
		}
		free(p);
	}
	return nindy_fd;
}



/******************************************************************************
 * ninDownload:
 *	Ask NINDY to start up it's COFF downloader. Invoke 'sx' to perform
 *	the XMODEM download from the host end.
 *
 *	Return 1 on success, 0 on failure.
 ******************************************************************************/

#ifdef DOS
#       define XMODEM   "sx.exe" /* Name of dos xmodem transfer utility */
#else
#       define XMODEM   "sx"    /* Name of xmodem transfer utility      */
#endif /*DOS*/
 
char _nin_noresets = 0;	/* if set, dont reset at download */

int
ninDownload( fn, quiet )
    char *fn;		/* Stripped copy of object file			*/
    int quiet;
{
        char *p = XMODEM; /* Pointer to sx utility			*/
	int success;	/* Return value					*/
	int pid;	/* Process ID of xmodem transfer utility	*/
	WAITTYPE w;	/* xmodem transfer completion status		*/
	int wret;	/* Value returned by wait			*/
	int exec_status; /* Status returned by child process            */
	char buf[200];


	/* Make sure the xmodem utility is findable.  This must be done before
	 * we start up the NINDY end of the download (NINDY will hang if we
	 * don't complete the download).
	 */
	if ( (p = exists("G960BIN",XMODEM,NULL,NULL,1)) == NULL
	    && (p = exists("G960BASE","bin",XMODEM, NULL,1)) == NULL )
	{
	    p = XMODEM;
	}

	if ( !quiet ){
		printf( "Downloading %s\n", fn );
	}

	/* Reset NINDY,  wait until "reset-complete" ack */
	if(_nin_noresets == 0)
		ninReset();

	/* Start up the NINDY end of the download. */
	putpkt((unsigned char *) "D", 1 );

	/* Invoke x-modem transfer, a separate process.  DON'T
	 * use system() to do this -- under system V Unix, the
	 * redirection of stdin/stdout causes the nindy tty to
	 * lose all the transmission parameters we've set up.
	 */
	success = 0;

#if defined(USG) && !defined(HAVE_VFORK)
	pid = fork ();
#else
	pid = vfork ();
#endif

	if ( pid == -1 ){
		perror( "Can't fork process:" );

	} else if ( pid == 0 ){		/* CHILD */
               	DUP2_TTY( nindy_fd, 0 );        /* Redirect stdin */
                DUP2_TTY( nindy_fd, 1 );        /* Redirect stout */
                if ( quiet ){
                        exec_status = EXEC_SX( p, p, "-q", fn, (char*)0 );
                } else {
                        exec_status = EXEC_SX(p, p, fn, (char*)0, (char*)0);
                }
                /* if we're here with bad status EXEC_SX failed */
                if (exec_status == -1) {
                        sprintf( buf, "Can't exec %s", p );
                        perror( buf );
			fprintf(stderr, "Check env variables G960BIN and/or G960BASE\n");
                }
                else return(1);
	} else {			/* PARENT */
		do {
			wret = wait(&w);
		} while ( wret != pid && wret != -1 );

		if ( wret == -1 ){
			perror( "Wait failed" );
		} else if (WIFEXITED(w) && (WEXITSTATUS(w) == 0)){
			success = 1;
		}
	}
	return success;
}


/******************************************************************************
 * ninGdbExit:
 *	Ask NINDY to leave GDB mode and print a NINDY prompt.
 ******************************************************************************/
ninGdbExit()
{
        putpkt((unsigned char *) "E", 1 );
}


/******************************************************************************
 * ninGo:
 *	Ask NINDY to start or continue execution of an application program
 *	in it's memory at the current ip.
 ******************************************************************************/
ninGo( step_flag )
    int step_flag;	/* 1 => run in single-step mode */
{
	putpkt((unsigned char *) (step_flag ? "s" : "c"), 1 );
}


/******************************************************************************
 * ninMemGet:
 *	Read a string of bytes from NINDY's address space (960 memory).
 ******************************************************************************/
ninMemGet(ninaddr, hostaddr, len)
     long ninaddr;	/* Source address, in the 960 memory space	*/
     unsigned char *hostaddr;	/* Destination address, in our memory space */
     int len;		/* Number of bytes to read			*/
{
	char buf[BUFSIZE+20];
	int cnt;		/* Number of bytes in next transfer	*/

	for ( ; len > 0; len -= BUFSIZE ){
		cnt = len > BUFSIZE ? BUFSIZE : len;

		buf[0] = 'm';
		put_int( &buf[1], ninaddr );
		buf[5] = cnt & 0xff;
		buf[6] = (cnt>>8) & 0xff;

		send( buf, 7, hostaddr );

		ninaddr += cnt;
		hostaddr += cnt;
	}
}


/******************************************************************************
 * ninMemPut:
 *	Write a string of bytes into NINDY's address space (960 memory).
 ******************************************************************************/
ninMemPut( ninaddr, hostaddr, len )
     long ninaddr;	/* Destination address, in NINDY memory space	*/
     unsigned char *hostaddr;	/* Source address, in our memory space	*/
     int len;		/* Number of bytes to write			*/
{
	char buf[BUFSIZE+20];
	int cnt;		/* Number of bytes in next transfer	*/

	for ( ; len > 0; len -= BUFSIZE ){
		cnt = len > BUFSIZE ? BUFSIZE : len;

		buf[0] = 'M';
		put_int( &buf[1], ninaddr );
		bcopy( hostaddr, buf+5, cnt );
		send( buf, cnt+5, NULL );

		ninaddr += cnt;
		hostaddr += cnt;
	}
}

/******************************************************************************
 * ninRegGet:
 *	Retrieve the contents of a 960 register, and return them as a long
 *	in host byte order.
 *
 *	THIS ROUTINE CAN ONLY BE USED TO READ THE LOCAL, GLOBAL, AND
 *	ip/ac/pc/tc REGISTERS.
 *
 ******************************************************************************/
long
ninRegGet( regname )
    char *regname;	/* Register name recognized by NINDY, subject to the
			 * above limitations.
			 */
{
	char outbuf[10];
	char inbuf[20];

	sprintf( outbuf, "u%s:", regname );
	send( outbuf, strlen(outbuf), inbuf );
	return get_int(inbuf);
}

/******************************************************************************
 * ninRegPut:
 *	Set the contents of a 960 register.
 *
 *	THIS ROUTINE CAN ONLY BE USED TO SET THE LOCAL, GLOBAL, AND
 *	ip/ac/pc/tc REGISTERS.
 *
 ******************************************************************************/
ninRegPut( regname, val )
    char *regname;	/* Register name recognized by NINDY, subject to the
			 * above limitations.
			 */
    long val;		/* New contents of register, in host byte-order	*/
{
	char buf[20];
	int len;

	sprintf( buf, "U%s:", regname );
	len = strlen(buf);
	put_int( &buf[len], val );
	send( buf, len+4, NULL );
}

/******************************************************************************
 * ninRegsGet:
 *	Get a dump of the contents of the entire 960 register set.  The
 *	individual registers appear in the dump in the following order:
 *
 *		pfp  sp   rip  r3   r4   r5   r6   r7 
 *		r8   r9   r10  r11  r12  r13  r14  r15 
 *		g0   g1   g2   g3   g4   g5   g6   g7 
 *		g8   g9   g10  g11  g12  g13  g14  fp 
 *		pc   ac   ip   tc   fp0  fp1  fp2  fp3
 *
 *	Each individual register comprises exactly 4 bytes, except for
 *	fp0-fp3, which are 8 bytes.  All register values are in 960
 *	(little-endian) byte order.
 *
 ******************************************************************************/
ninRegsGet( regp )
    char *regp;	/* Where to place the register dump */
{
	send("r", 1, regp );
}

/******************************************************************************
 * ninRegsPut:
 *	Initialize the entire 960 register set to a specified set of values.
 *	The format of the register value data should be the same as that
 *	returned by ninRegsGet.
 *
 * WARNING:
 *	All register values must be in 960 (little-endian) byte order.
 *
 ******************************************************************************/
ninRegsPut( regp )
    char *regp;		/* Pointer to desired values of registers */
{
	char buf[REGISTER_BYTES+10];

	buf[0] = 'R';
	bcopy( regp, buf+1, REGISTER_BYTES );
	send( buf, REGISTER_BYTES+1, NULL );
}


/******************************************************************************
 * ninReset:
 *      Ask NINDY to perform a soft reset; wait for the reset to complete.
 *
 ******************************************************************************/
ninReset()
{
	unsigned char ack;

	while (1){
		putpkt((unsigned char *) "X", 1 );
		while (1){
			if ( !rdnin(&ack,1,5) ){
				/* Timed out */
				break;		/* Resend */
			}
			if ( ack == '+' ){
				if ( have_cx_conf_regs ){
					ninConfRegs( cx_conf_regs );
				}
				return;
			}
		}
	}
}


/******************************************************************************
 * ninSrq:
 *	Assume NINDY has stopped execution of the 960 application program in
 *	order to process a host service request (srq).  Ask NINDY for the
 *	srq arguments, perform the requested service, and send an "srq
 *	complete" message so NINDY will return control to the application.
 *
 ******************************************************************************/
ninSrq()
{
	unsigned char buf[BUFSIZE];
	int retcode;
	unsigned char srqnum;
	int i;
	int offset;
	int arg[MAX_SRQ_ARGS];


	/* Get srq number and arguments
	 */
	send( "!", 1, buf );

	srqnum = buf[0];
	for  ( i=0, offset=1; i < MAX_SRQ_ARGS; i++, offset+=4 ){
		arg[i] = get_int(&buf[offset]);
	}

	/* Process Srq
	 */
	switch( srqnum ){
	case BS_CLOSE:
		/* args: file descriptor */
		if ( arg[0] > 2 ){
			retcode = close( arg[0] );
		} else {
			retcode = 0;
		}
		break;
	case BS_CREAT:
		/* args: filename, mode */
		ninStrGet( arg[0], buf );
		retcode = creat((char *)buf,arg[1]);
		break;
	case BS_OPEN:
		/* args: filename, flags, mode */
		ninStrGet( arg[0], buf );
		retcode = open((char *)buf,arg[1],arg[2]);
		break;
	case BS_READ:
		/* args: file descriptor, buffer, count */
		retcode = read(arg[0],buf,arg[2]);
		if ( retcode > 0 ){
			ninMemPut( arg[1], buf, retcode );
		}
		break;
	case BS_SEEK:
		/* args: file descriptor, offset, whence */
		retcode = lseek(arg[0],arg[1],arg[2]);
		break;
	case BS_WRITE:
		/* args: file descriptor, buffer, count */
		ninMemGet( arg[1], buf, arg[2] );
		retcode = write(arg[0],buf,arg[2]);
		break;
	default:
		retcode = ERROR;
		break;
	}

	/* Send request termination status to NINDY
	 */
	buf[0] = 'e';
	put_int( &buf[1], retcode );
	send( buf, 5, NULL );
}


/******************************************************************************
 * ninStopWhy:
 *	Assume the application program has stopped (i.e., a DLE was received
 *	from NINDY).  Ask NINDY for status information describing the
 *	reason for the halt.
 *
 *	Returns a non-zero value if the user program has exited, 0 otherwise.
 *	Also returns the following information, through passed pointers:
 *           - why: an exit code if program the exited; otherwise the reason
 *			why the program halted (see stop.h for values).
 *	    - contents of register ip (little-endian byte order)
 *	    - contents of register sp (little-endian byte order)
 *	    - contents of register fp (little-endian byte order)
 ******************************************************************************/
char
ninStopWhy( whyp, ipp, fpp, spp )
    unsigned char *whyp;  /* Return the 'why' code through this pointer	*/
    long *ipp;	/* Return contents of register ip through this pointer	*/
    long *fpp;	/* Return contents of register fp through this pointer	*/
    long *spp;	/* Return contents of register sp through this pointer	*/
{
	char buf[30];

	send( "?", 1, buf );

	*whyp = buf[1];
	bcopy( &buf[2],  (char *)ipp, sizeof (*ipp) );
	bcopy( &buf[6],  (char *)fpp, sizeof (*ipp) );
	bcopy( &buf[10], (char *)spp, sizeof (*ipp) );
	return buf[0];
}

/******************************************************************************
 * ninStrGet:
 *	Read a '\0'-terminated string of data out of the 960 memory space.
 *
 ******************************************************************************/
static
ninStrGet( ninaddr, hostaddr )
     unsigned long ninaddr;	/* Address of string in NINDY memory space */
     unsigned char *hostaddr;	/* Address of the buffer to which string should
				 *	be copied.
				 */
{
	char cmd[5];

	cmd[0] = '"';
	put_int( &cmd[1], ninaddr );
	send( cmd, 5, hostaddr );
}

/******************************************************************************
 * ninVersion:
 *	Ask NINDY for version information about itself.
 *	The information is sent as an ascii string in the form "x.xx,<arch>",
 *	where,
 *		x.xx	is the version number
 *		<arch>	is the processor architecture: "KA", "KB", "MC", "CA" *
 *
 ******************************************************************************/
int
ninVersion( p )
     char *p;		/* Where to place version string */
{
	send( "v", 1, p );
	return strlen(p);
}
