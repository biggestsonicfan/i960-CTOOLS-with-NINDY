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
#include "version.h"
#include "break.h"
#include "memmap.h"
#include "setjmp.h"
#include "block_io.h"
#include "stop.h"

extern char *strchr();

static jmp_buf j;

/**************************************************************************
 * This module implements the commands that can be sent by a debugger
 * running on a remote host.  Since the first such debugger was GDB,
 * these are referred to as "GDB commands".
 *
 * The communication protocol assumes that NINDY is essentially passive:
 * it waits for the host to send commands, to which it responds with status and 
 * (if appropriate) requested data.  The only exceptions are:
 *
 *	o when the application program terminates execution.
 *	o when the application program halts due to trace or fault exception.
 *	o when the application program requires service (such as file I/O)
 *		from the remote host.
 *
 * In these cases, NINDY sends the single character DLE (^P, or 0x10) to the
 * host.  It is then the host's responsibility to query as to the reason for
 * the interruption.
 *
 *
 * PACKET FORMAT
 * ------ ------
 * All GDB commands and NINDY responses to them are transferred in packets
 * of the following format:
 *
 *	  byte 0 byte 1                   byte len+2
 *	 +------+------+---    ...    ---+-------+
 *	 | len  | len  |     message     | check |
 *	 | low    high |(len bytes long) | sum   |
 *	 +------+------+---    ...    ---+-------+
 *
 * where 
 *	len low
 *		is the low-order byte of a 16-bit message length.
 *	len high
 *		is the high-order byte of a 16-bit message length.
 *
 *	checksum
 *		is an 8-bit checksum formed by adding together
 *		<len low>, <len high>, and each of the message bytes.
 *
 * The receiver of a message always sends a single character to the
 * sender to indicate that the checksum was good ('+') or bad ('-');
 * the sender re-transmits the entire message over whenever it
 * receives a '-'.
 *
 *
 * SPECIAL CHARACTERS
 * ------- ----------
 * The characters ^S (0x13), ^Q (0x11), and ^P (0x10) have special meaning.
 *
 * ^S and ^Q are used for flow control between NINDY and the host: if either
 * side is in danger of a input buffer overflow, it sends ^S to halt input;
 * ^Q is sent to resume.  [At this writing, NINDY will respects control
 * flow characters from the host, but NINDY's input is polled and unbuffered,
 * so it never generates these characters.
 *
 * ^P is used as a prefix to GDB commands sent from the host to NINDY.
 * Whenever NINDY receives a ^P, it goes into GDB mode and attempts to
 * read a new GDB command (it drops any other command that it will in
 * the process of reading -- this can help the host to sync up with NINDY
 * after a timeout).
 *
 * To send the actual binary values 0x10m 0x11, and 0x13 requires an escape
 * mechanism.  The ESC (0x1b) character is reserved for this purpose, and must
 * only appear in the data stream in one of the following 2-character
 * combinations:
 * 		ESC P	represents	^P (0x10)
 * 		ESC Q	represents	^Q (0x11)
 * 		ESC S	represents	^S (0x13)
 * 		ESC ESC	represents	ESC (0x1b)
 *
 * NOTE THAT EACH OF THESE SEQUENCES IS TREATED AS A SINGLE CHARACTER (THE
 * ONE SHOWN IN THE RIGHT COLUMN) WHEN GENERATING CHECKSUMS OR MESSAGE LENGTHS.
 * I.e., when either side sends a message it first generated the lengths and
 * checksums, and then converts any instances of the special characters into
 * two-character escape sequences.
 *
 *
 * GDB COMMANDS
 * --- --------
 * The first byte of a GDB command's "message" unambiguously identifies the
 * command, and the other message bytes (if any) are command arguments.
 *
 *
 * NINDY RESPONSES
 * ----- ---------
 * The first "message" byte of a NINDY response contains one of the following
 * binary values, indicating the execution status of the preceding command:
 *
 *	0 - No errors
 *	1 - Buffer overflow
 *	2 - Unknown command
 *	3 - Wrong amount of data to load register(s)
 *	4 - Missing command argument(s)
 *	5 - Odd number of digits sent to load memory
 *	6 - Unknown register name
 *	7 - No such memory segment (bad 'num' passed to 'S' command).
 *	8 - Attempt to set breakpoint of a type that is not available on the
 *		processor under test, or all breakpoints of specified type
 *		already in use.
 *	9 - Can't set requested baud rate.
 *
 * If the command causes NINDY to return any data, the data begins in the
 * byte following the status byte. (In actual practice, there is no data
 * if the status is not 0.)
 *
 *
 * ARGUMENTS
 * ---------
 * In the command descriptions below, "<>" brackets indicate a required
 * argument, "[]" an optional one.  The formats of the arguments are as follows:
 *
 *	addr	4 binary bytes, low-order byte first.
 *
 *	baud	any legal NINDY baud rate, as a \0-terminated string of ascii
 *		decimal digits (e.g., "9600\0").
 *
 *	c	a single ascii character
 *
 *	data	a series of binary data bytes in 960 memory order;  in the
 *		case of integer values, this implies low-order byte first.
 *
 *	len	2 binary bytes forming an unsigned 16-bit number, low-order
 *		byte first.
 *
 *	n	a single binary byte.
 *
 *	regname	a 2- or 3- character 80960 register name, from the following
 *		list:
 *			pfp  sp   rip  r3   r4   r5   r6   r7 
 *			r8   r9   r10  r11  r12  r13  r14  r15 
 *			g0   g1   g2   g3   g4   g5   g6   g7 
 *			g8   g9   g10  g11  g12  g13  g14  fp 
 *			pc   ac   ip   tc   fp0  fp1  fp2  fp3
 *
 *	regs	the concatenated values of all 80960 registers, in the order
 *		shown above for 'regname'.  Each individual register comprises
 *		4 binary bytes (8 for fp0-fp3), low-order byte first.
 *
 *
 * LEGAL COMMANDS:
 * ----- --------
 *	B<c><addr>
 * 	b<c>[addr]
 *		Set ('B') or remove ('b') a hardware breakpoint of type <c> at
 *		address <addr>.  The legal types are:
 *			'i' = instruction breakpoint
 *			'd' = data breakpoint
 *
 *		If <addr> is omitted from the 'b' command, *all* breakpoints
 *		of the specified type are removed.
 *
 *		Attempts to set already-set breakpoints or to delete
 *		non-existent breakpoints are ignored, but cause no error.
 *
 *	c[addr]	Resume user program execution at address <addr>.
 *		If addr is omitted, resumes at address in the ip.
 *
 *	D	Download a COFF executable file.  Prepares NINDY to accept
 *		an X-modem file transfer.
 *
 *	e<n>	Return the value <n> to the user application program,
 *		indicating that processing of a service request by the
 *		remote host has completed with that return value.
 *
 *	E	Exit GDB mode, output a keyboard prompt.
 *
 *	m<addr><len>
 *		Send (to host) contents of <len> bytes of memory starting at 
 *		memory location <addr>.
 *
 *	M<addr><data>
 *		update memory, starting at memory location <addr>, 
 *		with the binary data <data>.
 *
 *	r	Send (to host) the values of all user registers, in the same
 *		format as is accepted by the 'R' command.
 *
 *	R<regs>	Load the values <regs> into the user registers.
 *
 *	s[addr]	Single-step user program at address <addr> (at the address in
 *		the ip, if addr is omitted).
 *
 *	Sn	Return the entry for memory segment #n from the memory map for
 *		the board in question, in the following binary format:
 *
 *				nbbbbbbbbeeeeeeee
 *
 *		     n: type of segment: 0=RAM 1=ROM 2=FLASH 3=H/W
 *		     b: address of begining of memory segment, in <addr> format
 *		     e: address of end of memory segment, in <addr> format
 *
 *		If n is 0xff, returns the number of memory segments in the
 *		memory map.
 *
 *		Notes on memory map:
 *			Memory segments are numbered consecutively from 0,
 *			in order of increasing beginning address.
 *
 *			Discontinuities in the memory map indicate address
 *			ranges that are not decoded into anything meaningful.
 *
 *	u<regname>
 *		Return the current value of user register <regname>, as 4
 *		binary bytes in little-endian order.
 *
 *	U<regname>,<data>
 *		Update the register whose name is <regname> with the little-
 *		endian binary value <data>.  <data> must be 8 bytes for
 *		floating point registers, 4 for others.
 *
 *	v	Send version info, as a \0-terminated ascii string in the form
 *		x.xx,aa, where
 *
 *		    x.xx is version number
 *		      aa is processor architecture: "KA", "KB", "MC", or "CA"
 *
 *	X	Reset the processor, suppressing powerup greeting message.
 *
 *	z<baud>
 *		Set baud rate to <baud>.
 *
 *	"<addr>	Send a string of data bytes starting at address <addr>
 *		up to and including the first 0x00 byte.
 *
 *	!	Send application service request (srq) to host in the following
 *		format:
 *			n<arg1><arg2><arg3>
 *		where
 *		        n    srq number -- see block_io.h
 *			arg  each argument is 4 binary bytes, low-order first.
 *
 *	?	Send the status with which user program halted, in the
 *		following format (all binary bytes):
 *			xyiiiiffffssss
 *		where
 *		       x  non-zero if user program exited, 00 otherwise
 *		       y  return code, if program exited; otherwise
 *				    fault code (see fault.c for legal values).
 *		    iiii  contents of register ip (little-endian order)
 *		    ssss  contents of register sp (little-endian order)
 *		    ffff  contents of register fp (little-endian order)
 *
 **************************************************************************/


/* WARNING:
 *	If these change or are added to, update the toolib source
 *	file "nindy.c".
 */
#define X_OK		0	/* No errors				*/
#define X_BUFFER	1	/* Buffer overflow			*/
#define X_CMD		2	/* Unknown command			*/
#define X_DATLEN	3	/* Wrong amount of data to load register(s) */
#define X_ARG		4	/* Missing command argument(s)		*/
#define X_MEMLD		5	/* Odd number of digits sent to load memory */
#define X_REGNAME	6	/* Unknown register name		*/
#define X_MEMSEG	7	/* No such memory segment		*/
#define X_NOBPT		8	/* No breakpoint available		*/
#define X_BAUD		9	/* Can't set requested baud rate	*/

#define REGISTER_BYTES	(36*REG_SIZE + 4*FP_REG_SIZE)

/* Send status byte (only) back to host
 */
#define ERR(status)	putpkt(status,NULL,0);

/* Send data back to host (status implied to be ok)
 */
#define SEND(data,len)	putpkt( X_OK,data,len);

/* Access as an integer the four bytes beginning at &x
 */
#define INTEGER(x)      (*(int*)(&x))


/* Macros to translate printing characters to control characters and
 * vice versa, for encoding/decoding escape sequences.
 */
#define TO_CNTL(c)	( c & ~0x40)
#define FROM_CNTL(c)	( c | 0x40)

/* Information about remote host service request (SRQ) issued by application.
 */
static char srq_num;			/* SRQ number (type)	*/
static int  srq_args[MAX_SRQ_ARGS];	/* SRQ arguments, if any*/
static int  srq_ret;			/* Host return value	*/


/********************************************************
 * remote_srq()
 *	Process an application program's host service
 *	request (srq).
 *
 *	This involves storing the srq number and arguments
 *	in globals, notifying the host by sending a DLE,
 *	and then entering a loop to process host GDB commands
 *	until gdb_cmd() returns TRUE (which means that the
 *	host sent a completion code, which can be found
 *	in the global srq_ret).
 *
 ********************************************************/
remote_srq(n, arg1, arg2, arg3)
    int n;			/* SRQ number		*/
    int arg1, arg2, arg3;	/* SRQ arguments	*/
{
	int gdb_flag;	/* Remember whether or not we were in gdb mode
			 * on entry.  We want to leave in the same
			 * state.
			 */

	switch ( n ){
        case BS_CLOSE:
		if ( arg1 < 3 ){
			/* Never really close host's stdin, stdout, stderr */
			return 0;
		}

		/* DROP INTO NEXT CASE */

        case BS_CREAT:
        case BS_SEEK:
        case BS_OPEN:
        case BS_READ:
        case BS_WRITE:
		srq_num  = n;
		srq_args[0] = arg1;
		srq_args[1] = arg2;
		srq_args[2] = arg3;

		stop_exit = FALSE;
		stop_code = STOP_SRQ;

		co( DLE );

		gdb_flag = gdb;
		while ( !gdb_cmd() ){
			;
		}
		gdb = gdb_flag;

		return srq_ret;
                break;

        case BS_ACCESS:
        case BS_STAT:
        case BS_SYSTEMD:
        case BS_TIME:
        case BS_UNLINK:
        case BS_UNMASK:
        default:
		/* currently unimplemented */
		return ERROR;
	}
}

/********************************************************
 * gdb_cmd()  -- process a GDB command from a remote host
 *
 *	Assume the ^P prefix of a GDB command has been
 *	read.  Receive the rest of the command from the
 *	host and process it.
 *
 *	Return TRUE if control should be returned to the
 *	user application, FALSE otherwise.
 *
 ********************************************************/
int
gdb_cmd()
{
	char cmd[ BUFSIZE + 20 ];
		/* Message, stripped of ^P/length/checksum, goes here.
		 * I.e., cmd[0] is command name, cmd[1] is first
		 * character of command arguments.
		 */
	int i;
	int addr;
	int len;


	setjmp(j);	/* Return to this point whenever a ^P is read */


	len = getpkt(cmd,sizeof(cmd));		/* Read command */
	if ( len <= 0 ){
		return FALSE;
	}

	switch ( cmd[0] ){
	case 'b':
	case 'B':
		gdb_bpt( cmd, len );
		break;
	case 'c':
	case 's':
		/* Command name may be followed by address at which
		 * user program should resume.
		 */
		if ( len == 1 ){
			/* No address specified, use value in ip */
			addr = register_set[REG_IP];
		} else { 
			addr = INTEGER(cmd[1]);
		}
		go( cmd[0]=='s', 1, addr );
		return user_is_running;
	case 'D':
		download( FALSE );
		break;
	case 'e':
		srq_ret = INTEGER(cmd[1]);
		ERR( X_OK );
		return TRUE;
	case 'E':
		gdb = FALSE;
		break;
	case 'm':
	case 'M':
		mem( cmd, len );
		break;
	case 'r':
		cpy( &cmd[0], register_set, 36*REG_SIZE );
		cpy( &cmd[36*REG_SIZE], fp_register_set, 4*FP_REG_SIZE );
		SEND( cmd, REGISTER_BYTES );
		break;
	case 'R':
		if ( len != REGISTER_BYTES + 1 ){
			ERR( X_DATLEN ); /* Wrong amount of data in buffer */
		}
		cpy( register_set, cmd+1, 36*REG_SIZE );
		cpy( fp_register_set, &cmd[1+36*REG_SIZE], 4*FP_REG_SIZE );
		ERR( X_OK );
		break;
	case 'S':
		map( cmd[1] );
		break;
	case 'u':
	case 'U':
		update_reg( cmd );
		break;
	case 'v':
		strcpy( cmd, VERSION );
		strcat( cmd, "," );
		strcat( cmd, architecture );
		SEND( cmd, strlen(cmd)+1 );
		break;
	case 'X':
		/* Reset -- set a magic flag so we come up again in GDB
		 * mode (see declaration of 'end' in globals.h).
		 * Kill time to allow the last ack to get out of the
		 * UART: if we reset too soon it gets lost and the
		 * host gets out of sync.
		 */
		end.r_gdb = TRUE;
		eat_time(1000);
		reset();	/* No return */
		break;
	case 'z':
		/* Don't change baud rate until packet ack has had time
		 * to make it out.
		 */
		eat_time(1000);
		if ( baud(0,0,&cmd[1]) ){
			ERR( X_OK );
		} else {
			ERR( X_BAUD );
		}
		break;
	case '"':
		if ( len != 5 ){
			ERR( X_ARG ); /* Missing address */
		} else {
			addr = INTEGER(cmd[1]);
			SEND( addr, strlen(addr)+1 );
		}
		break;
	case '?':
		/* Send reason why user program last stopped, as well
		 * as values of ip, fp, sp with which it stopped.
		 */
		cmd[0] = stop_exit;
		cmd[1] = stop_code;
		INTEGER(cmd[2])  = register_set[REG_IP];
		INTEGER(cmd[6])  = register_set[REG_FP];
		INTEGER(cmd[10]) = register_set[REG_SP];
		SEND( cmd, 14 );
		break;
	case '!':
		/* Send srq information
		 */
		cmd[0] = srq_num;
		INTEGER(cmd[1]) = srq_args[0];
		INTEGER(cmd[5]) = srq_args[1];
		INTEGER(cmd[9]) = srq_args[2];
		SEND( cmd, 13 );
		break;
	default:
		ERR( X_CMD );
		break;
	}
	return FALSE;
}



/********************************************************
 * update_reg()
 *	Update the value of a single user register.
 *
 *	The command we are passed should look like one of:
 * 		U<regname>:<value>	[receive register value]
 *		u<regname>:		[send register value]
 * 
 ********************************************************/
static
update_reg( cmd )
    char *cmd;
{
	char *value;		/* -> start of <value> in buffer	*/
	unsigned int *regp;	/* -> binary value of the register	*/
	int regnum;		/* # of register whose name is <regname>*/
	int regsize;		/* Size of register, in bytes		*/

	value = strchr( cmd, ':' );
	if ( value == NULL ){
		ERR( X_ARG );	/* Missing argument */
		return;
	} else {
		/* Delimit end of register name,
		 * point to start of register value.
		 */
		*value++ = '\0';
	}

	if ( (regnum = get_regnum(cmd+1)) == ERROR ){
		ERR( X_REGNAME );	/* Bad register name */
		return;
	}
	
	if ( regnum < NUM_REGS ){
		regsize = 4;
		regp = &register_set[regnum];
	} else {
		/* FLOATING POINT REGISTER */
		regsize = 8;
		regp = (unsigned int*) &fp_register_set[regnum-NUM_REGS];
	}

	if ( cmd[0] == 'U' ){
		cpy( regp, value, regsize );
		ERR( X_OK );
	} else {
		SEND( regp, regsize );
	}
}


/********************************************************
 * mem()
 *
 *	This routine processes both the m and M commands
 *	(send memory and receive memory).
 *
 *	The command we are passed should look like one of:
 * 
 *	+---+---+---+---+---+---+---	receive 'data'
 *	| M |    address    | data..	to 'address'.
 *	+---+---+---+---+---+---+---
 *	  0   1   2   3   4   5   6
 *	+---+---+---+---+---+---+---+	send 'length'
 *	| m |    address    |length |	bytes of data
 *	+---+---+---+---+---+---+---+	from 'address'
 *
 ********************************************************/
static
mem( cmd, len )
    unsigned char cmd[];	/* 'm' or 'M' commmand, as described above */
    int len;			/* Number of valid bytes in cmd[] */
{
	int addr;	/* Beginning location in memory to update or send */
	char *p;	/* Pointer to beginning of second argument in cmd */
	int dlen;	/* # bytes to send or receive			*/

	if ( len < 5 ){
		/* Missing command arguments */
		ERR( X_ARG );
		return;
	}

	addr = INTEGER(cmd[1]);

	if ( cmd[0] == 'm' ){		/* "send" command */
		dlen = (cmd[6] << 8) | cmd[5];
		SEND( addr, dlen );
	} else {			/* "receive" command ('M') */
		dlen = len - 5;
		cpy( addr, &cmd[5], dlen );
		ERR( X_OK );
	}
}

/********************************************************
 * map()
 *
 *	Send the memory map entry for memory segment #n.
 *	If n == 0xff, send the number of memory segments
 *	in the full memory map.
 ********************************************************/
static
map( n )
unsigned char n;
{
	int numsegs;
	struct memseg *sp;
	char buf[30];

	numsegs = sort_map();

	if ( n == 0xff ){
		buf[0] = numsegs;
		SEND(buf,1);
	} else {
		if ( n >= numsegs ){
			ERR(X_MEMSEG);
		} else {
			sp = &memmap[n];
			buf[0] = sp->type;
			INTEGER(buf[1]) = sp->begin;
			INTEGER(buf[5]) = sp->end;
			SEND(buf,9);
		}
	}
}

/********************************************************
 * sort_map()
 *
 *	Sort the memory map into increasing numeric order
 *	of beginning address of each memory segment.
 *
 *	Return the number of memory segments in the map.
 ********************************************************/
static int
sort_map()
{
	int i;
	int j;
	struct memseg tmp;

	for ( i=0; memmap[i].type != SEG_EOM; i++ ){
		for ( j=i+1; memmap[j].type != SEG_EOM; j++ ){
			if ( memmap[j].begin < memmap[i].begin ){
				tmp = memmap[i];
				memmap[i] = memmap[j];
				memmap[j] = tmp;
			}
		}
	}
	return i;
}

/********************************************************
 * gdb_bpt -- set/delete hardware breakpoint
 *
 *	cmd should look like one of:
 * 
 *	+---+---+---+---+---+---+ Set bpt of specified type at
 *	| B |i/d|    address    | specified address.
 *	+---+---+---+---+---+---+
 *	  0   1   2   3   4   5
 *	+---+---+---+---+---+---+ Delete bpt of specified type at
 *	| b |i/d|    address    | given address (all bpts of
 *	+---+---+---+---+---+---+ specified type if address omitted.
 *
 *	Type 'i' is instruction breakpoint.
 *	Type 'd' is data breakpoint.
 ********************************************************/
static
gdb_bpt( cmd, len )
char cmd[];
int len;
{
	struct bpt *bp;	/* Pointer into breakpoint table	*/
	int type;	/* Breakpoint type: BRK_INST, BRK_DATA	*/

	if ( len < 2 ){
		ERR(X_ARG);		/* Missing arg(s) */
	}

	switch ( cmd[1] ){			/* Convert type identifier */
	case 'i':
		type = BRK_INST;
		break;
	case 'd':
		type = BRK_DATA;
		break;
	default:
		ERR(X_NOBPT);
		return;
	}


	if ( cmd[0] == 'B' ){
		if ( !set_bpt(type,INTEGER(cmd[2])) ){	/* Set breakpoint */
			ERR(X_NOBPT);
			return;
		}
	} else {
		if ( len > 2 ){
			/* Address provided;  delete specified bpt */
			del_bpt(type,INTEGER(cmd[2]));
		} else {
			/* No address;  search breakpoint table for all
			 * bpts of specified type, deleting each.
			 */
			for ( bp = &bptable[0]; bp->type != BRK_EOT; bp++ ){
				if ( bp->type == type && bp->active ){
					del_bpt(type,bp->addr);
				}
			}
		}
	}
	ERR(X_OK);
}

/********************************************************
 * rdchar -- read a single byte from the host
 *
 *	If we get a DLE, re-sync with host at start of a
 *	new command.
 *
 *	If we get an ESC, discard it; read the next charac-
 *	ter in the input stream; and translate the new
 *	character into a binary value corresponding to a
 *	special character.
 ********************************************************/
static
int
rdchar()
{
	int c;

	switch (c = ci()){
	case DLE:
		longjmp(j,1);
	case ESC:
		c = TO_CNTL( ci() );
		break;
	}

	return c;
}

/********************************************************
 * wrchar -- write a single byte to the host
 *
 *	If the binary value of the byte is equal to one
 *	of the special characters, output an escape
 *	sequence instead (see block header at start of
 *	file).
 ********************************************************/
static
int
wrchar(c)
	unsigned char c;
{
	switch ( c ){
	case DLE:
	case XON:
	case XOFF:
	case ESC:
		co(ESC);
		co( FROM_CNTL(c) );
		break;
	default:
		co(c);
		break;
	}
}


/********************************************************
 * putpkt -- checksum and send message to host
 *
 *	Send message over and over to host until positive
 *	acknowledgment received.
 *
 ********************************************************/
static
putpkt( status, buf, dlen )
    int status;	/* Status byte to be sent	*/
    char *buf;	/* Data to be sent		*/
    int dlen;	/* Number of bytes of data	*/
{
	char csum;	/* Checksum			*/
	char ack;	/* Ack character from host	*/
	int len;	/* Total message length (buffer + status byte) */
	char lenh;
	char lenl;
	int i;


	len = dlen + 1;		/* +1 for status byte */
	lenh = ((len>>8) & 0xff);
	lenl = len & 0xff;

	/* Send message over and over until we get a positive ack */

	while (1){
		csum = lenh + lenl + status;
		wrchar( lenl );		/* Send length, low byte	*/
		wrchar( lenh );		/* Send length, high byte	*/
		wrchar( status );	/* Send status byte		*/

		for ( i = 0; i < dlen; i++ ){
			wrchar(buf[i]); /* Send body of message		*/
			csum += buf[i];
		}

		wrchar( csum );		/* Send checksum		*/

		while ( (ack=rdchar()) != '-' ){
			if ( ack == '+' ){
				return;
			}
		}
	}
}


/********************************************************
 * getpkt -- get message from host, validating and stripping
 *		checksum and packet length.
 *
 *	Return length of message extracted from the input
 *	packet, -1 if buffer overflows.
 *
 *	Assumes the leading DLE of a GDB message has
 *	already been read in.
 ********************************************************/
static
int
getpkt( buf, buflen )
    char *buf;		/* Where to put command			*/
    int buflen;		/* Number of bytes in buffer 'buf'	*/
{
	int length;	/* "Length" field of input packet	*/
	int lenh, lenl;	/* High/low bytes of length		*/
	int c;		/* Next character from host		*/
	int n;		/* Number of characters read from host	*/
	unsigned char csum;	/* Calculated checksum		*/


	while ( 1 ){
		lenl = rdchar();
		lenh = rdchar();
		length = (lenh << 8) | lenl;

		csum = lenh + lenl;

		for ( n=0; n < length; n++ ){
			c = rdchar();
			if ( n < buflen ){
				buf[n] = c;
			}
			csum += c;
		}

		if ( csum == rdchar() ){
			/* Valid checksum */
			co( '+' );
			if ( n >= buflen ){
				ERR( X_BUFFER ); /* Buffer Overflow */
				return -1;
			}
			return n;
		}

		/* Bad checksum: send NAK and re-receive */
		co( '-' );
	}
}


static
cpy( to, from, len )
    char *to;
    char *from;
    int len;
{
	while ( len-- ){
		*to++ = *from++;
	}
}
