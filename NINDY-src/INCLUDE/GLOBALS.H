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

struct sect { 		/* section header structure */
	char		sec_name[8];   	/* section name */
	long    	p_addr;		/* physical address */
	long    	v_addr;		/* virtual address */
	long    	sec_size;	/* size of sections */
	long    	data_ptr;	/* pointer to data */
	long    	reloc_ptr;      /* relocation pointer */
	long    	line_num_ptr;	/* line number pointer */
	unsigned short	num_reloc;	/* number of reloc entries */
	unsigned short	num_line; 	/* number line num entries */
	long    	flags;		/* flags */
	unsigned long	sec_align; 	/* alignment for sect bndry */
} sectbuf[MAXSECTS];

int downtype[MAXSECTS];	/* type of download (Flash or RAM) for each section */

struct aout {		/* a.out header structure */
	unsigned short 	magic_nmbr;	/* magic number */
	unsigned short 	version;	/* version */
	long		text_size;	/* size of .text section */
	long		data_size;	/* size of .data section */
	long		bss_size;	/* size of .bss section */
	long		start_addr; 	/* starting address */
	long 		text_begin;	/* start of text section */
	long		data_begin; 	/* start of data section */
} aoutbuf;

/* The following are sizes of the above structures NOT INCLUDING any padding
 * added by the compiler for alignment of entries within arrays.
 */
#define SHDR_UNPADDED_SIZE (sizeof(sectbuf[0].sec_name) \
				+ sizeof(sectbuf[0].p_addr) \
				+ sizeof(sectbuf[0].v_addr) \
				+ sizeof(sectbuf[0].sec_size) \
				+ sizeof(sectbuf[0].data_ptr) \
				+ sizeof(sectbuf[0].reloc_ptr) \
				+ sizeof(sectbuf[0].line_num_ptr) \
				+ sizeof(sectbuf[0].num_reloc) \
				+ sizeof(sectbuf[0].num_line) \
				+ sizeof(sectbuf[0].flags) \
				+ sizeof(sectbuf[0].sec_align))

#define AOUT_UNPADDED_SIZE (sizeof(aoutbuf.magic_nmbr) \
				+ sizeof(aoutbuf.version) \
				+ sizeof(aoutbuf.text_size) \
				+ sizeof(aoutbuf.data_size) \
				+ sizeof(aoutbuf.bss_size) \
				+ sizeof(aoutbuf.start_addr) \
				+ sizeof(aoutbuf.text_begin) \
				+ sizeof(aoutbuf.data_begin))


int noerase;	/* should download erase Flash? */

int fltype;	/* Flash type used for Flash download */
int flsize;	/* Flash size used for Flash download */
int downld;	/* If TRUE, xmodem download is in progress over the serial port.
		 * The download must be cancelled before error messages can be
		 * printed.  Also, since binary is downloaded, XON/XOFF checking
		 * must be suspended until the download is over.
		 */


unsigned char linebuff[LINELEN];	/* command line buffer */

int fault_cnt;		/* Number of nested faults */

unsigned long errno;

unsigned int long_data[4];

char user_is_running;		/* TRUE if user's application program has
				 * been started, and monitor is running as
				 * result of a fault/breakpoint. FALSE otherwise
				 */

/* 'end' is actually the linker-generated address of the first location *after*
 * BSS.  We store restart information there because there are some things we
 * want to remember across system restarts, but data and bss get reinitialized
 * (clobbered) when NINDY restarts.
 *
 * Make sure this pseudo-structure contains only chars, so we don't have to
 * worry about alignment-created holes.
 *
 * Meanings of the fields:
 *	r_magic	The structure only contains valid information if this array
 *		is set to the string RESTART_ARGS_VALID.  (There is nothing
 *		valid here when the board is first powered up.
 *
 *	r_gdb	If TRUE, the restart was requested by a remote host: stifle
 *		the normal powerup greeting.
 *
 *	r_baud	Set to the index in 'baudtable[]' of baud rate at which NINDY
 *		was alst talking to the console.  This baud rate should be
 *		restored after restart.
 */
extern struct restart {
	char r_magic[5];
	char r_gdb;
	char r_baud;
} end;

#define RESTART_ARGS_VALID	"\125\252\125\252"	/* 0x55aa55aa */


extern unsigned int *nindy_stack; 	/* stack for nindy use */
extern int baudtable[];
extern char boardname[];	/* Board identifier string		*/
extern char architecture[];	/* Processor architecture identifier string */

extern char *get_regname();


/*
 * THE FOLLOWING ARE HERE TO SUPPORT THE GDB INTERFACE TO NINDY
 *
 * GDB is the GNU symbolic debugger.  It runs on a remote host and talks
 * to NINDY via the serial I/O line.
 *
 * NINDY enters GDB mode as soon as it receives a GDB command.  A GDB
 * command is any command that begins with the character ^P.
 * See gdb.c for a list of legal GDB commands.
 */

extern char gdb;	/* TRUE after the first GDB command is seen,
			 * until a "leave gdb mode" command is received.
			 * If FALSE, NINDY assumes it is
			 * interacting directly with a humanoid.
			 */

extern char stop_exit;
extern char stop_code;
			/* The reason why the user program last stopped.
			 * See fault.c for legal values.
			 */
