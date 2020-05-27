/* Memory-access and commands for remote NINDY process, for GDB.
   Copyright (C) 1990-1991 Free Software Foundation, Inc.
   Contributed by Intel Corporation.  Modified from remote.c by Chris Benenati.

GDB is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY.  No author or distributor accepts responsibility to anyone
for the consequences of using it or for whether it serves any
particular purpose or works at all, unless he says so in writing.
Refer to the GDB General Public License for full details.

Everyone is granted permission to copy, modify and redistribute GDB,
but only under the conditions described in the GDB General Public
License.  A copy of this license is supposed to have been given to you
along with GDB so you can know your rights and responsibilities.  It
should be in a file named COPYING.  Among other things, the copyright
notice and this notice must be preserved on all copies.

In other words, go ahead and share GDB, but don't try to stop
anyone else from sharing it farther.  Help stamp out software hoarding!
*/

/*
Except for the data cache routines, this file bears little resemblence
to remote.c.  A new (although similar) protocol has been specified, and
portions of the code are entirely dependent on having an i80960 with a
NINDY ROM monitor at the other end of the line.
*/

/*****************************************************************************
 *
 * REMOTE COMMUNICATION PROTOCOL BETWEEN GDB960 AND THE NINDY ROM MONITOR.
 *
 *
 * MODES OF OPERATION
 * ----- -- ---------
 *	
 * As far as NINDY is concerned, GDB is always in one of two modes: command
 * mode or passthrough mode.
 *
 * In command mode (the default) pre-defined packets containing requests
 * are sent by GDB to NINDY.  NINDY never talks except in reponse to a request.
 *
 * Once the the user program is started, GDB enters passthrough mode, to give
 * the user program access to the terminal.  GDB remains in this mode until
 * NINDY indicates that the program has stopped.
 *
 *
 * PASSTHROUGH MODE
 * ----------- ----
 *
 * GDB writes all input received from the keyboard directly to NINDY, and writes
 * all characters received from NINDY directly to the monitor.
 *
 * Keyboard input is neither buffered nor echoed to the monitor.
 *
 * GDB remains in passthrough mode until NINDY sends a single ^P character,
 * to indicate that the user process has stopped.
 *
 * Note:
 *	GDB assumes NINDY performs a 'flushreg' when the user program stops.
 *
 *
 * COMMAND MODE
 * ------- ----
 *
 * All info (except for message ack and nak) is transferred between gdb
 * and the remote processor in messages of the following format:
 *
 *		<info>#<checksum>
 *
 * where 
 *	#	is a literal character
 *
 *	<info>	ASCII information;  all numeric information is in the
 *		form of hex digits ('0'-'9' and lowercase 'a'-'f').
 *
 *	<checksum>
 *		is a pair of ASCII hex digits representing an 8-bit
 *		checksum formed by adding together each of the
 *		characters in <info>.
 *
 * The receiver of a message always sends a single character to the sender
 * to indicate that the checksum was good ('+') or bad ('-');  the sender
 * re-transmits the entire message over until a '+' is received.
 *
 * In response to a command NINDY always sends back either data or
 * a result code of the form "Xnn", where "nn" are hex digits and "X00"
 * means no errors.  (Exceptions: the "s" and "c" commands don't respond.)
 *
 * SEE THE HEADER OF THE FILE "gdb.c" IN THE NINDY MONITOR SOURCE CODE FOR A
 * FULL DESCRIPTION OF LEGAL COMMANDS.
 *
 * SEE THE FILE "stop.h" IN THE NINDY MONITOR SOURCE CODE FOR A LIST
 * OF STOP CODES.
 *
 ******************************************************************************/

#ifdef DOS
#	include <malloc.h>
#	include <errno.h>
#	include "gnudos.h"
#define TTY_ATTACH_MSG "\nAttach serial port -- specify comN, or \"quit\" to quit:  "
#else
#define TTY_ATTACH_MSG "\nAttach /dev/ttyNN -- specify NN, or \"quit\" to quit:  "
#endif /*DOS*/
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <setjmp.h>

#include "defs.h"

#include "frame.h"
#include "inferior.h"
#include "target.h"
#include "gdbcore.h"
#include "command.h"
#include "bfd.h"

#ifdef DOS
#	include <fcntl.h>
#else
#	include <sys/file.h>
#endif /*DOS*/

#ifdef IMSTG
#	include "ttycntl.h"
#	include "demux.h"
#	include "stop.h"
#	ifdef M_HP9000
#		include "terminal.h"
#	endif
#else
#	include "nindy-share/ttycntl.h"
#	include "nindy-share/demux.h"
#	include "nindy-share/stop.h"
#endif

extern int unlink();
extern char *getenv();
extern char *mktemp();

extern char *coffstrip();
extern void add_syms_addr_command ();
extern value_ptr call_function_by_hand ();
extern void generic_mourn_inferior ();

extern struct target_ops nindy_ops;
extern FILE *instream;

extern char ninStopWhy ();

/* From infrun.c: nonzero if breakpoints are currently inserted 
   in the inferior.  Used in mon960_resume. */
extern int breakpoints_inserted;

/* Special breakpoint used only for next-ing over calls (see nindy_resume) */
static struct breakpoint *nindy_next_breakpoint;

extern int target_initial_brk;	/* nonzero to send an initial BREAK to nindy */

#define DEFAULT_BAUD_RATE	9600

#define DLE	'\020'	/* Character NINDY sends to indicate user program has
			 * halted.  */
#define TRUE	1
#define FALSE	0

int nindy_fd = 0;	/* Descriptor for I/O to NINDY  */
static int have_regs = 0;	/* 1 iff regs read since i960 last halted */
static int regs_changed = 0;	/* 1 iff regs were modified since last read */

extern char *exists();
static void dcache_flush (), dcache_poke (), dcache_init(), reset_command();
static int dcache_fetch ();
static void nindy_fetch_registers PARAMS ((int));
static void nindy_store_registers PARAMS ((int));
static void nindy_get_target_byte_order PARAMS ((void));


/* FIXME, we can probably use the normal terminal_inferior stuff here.
   We have to do terminal_inferior and then set up the passthrough
   settings initially.  Thereafter, terminal_ours and terminal_inferior
   will automatically swap the settings around for us.  */

/* Restore TTY to normal operation */

static TTY_STRUCT orig_tty;	/* TTY attributes before entering passthrough */

static void
restore_tty()
{
	TTY_SET(0,orig_tty);
}


/* Recover from ^Z or ^C while remote process is running */

static void (*old_ctrlc) PARAMS ((int));    /* Signal handlers before entering passthrough */

#ifdef SIGTSTP
static void (*old_ctrlz) PARAMS ((int));
#endif

#if defined(DOS) && defined(__HIGHC__)
/* We normally have pragma On(Check_stack) for runtime stack overflow checking.
   But with Metaware HIGH-C, you can't BOTH check for stack overflow AND also
   handle a hardware-caused signal.  So we'll temporarily turn off stack
   checking while defining signal handlers. */
#pragma Off(Check_stack)
#endif

static
#ifdef USG
void
#endif
cleanup(dummy)
int dummy;
{
	restore_tty();
	signal(SIGINT, old_ctrlc);
#ifdef SIGTSTP
	signal(SIGTSTP, old_ctrlz);
#endif
	error("\n\nYou may need to reset the 80960 and/or reload your program.\n");
}

#if defined(DOS) && defined(__HIGHC__)
#pragma Pop(Check_stack)	/* Restore previous value of Check_stack */
#endif  /* Workaround for HIGHC signal-handler/Check_stack bug. */


/* Clean up anything that needs cleaning when losing control.  */

static char *savename;

static void
nindy_close (quitting)
     int quitting;
{
#ifdef	DOS
	close_port (nindy_fd);
#else
	if (nindy_fd)
		close (nindy_fd);
#endif
  nindy_fd = 0;

  if (savename)
    free(savename);
  savename = 0;
}

unsigned long *mconftab = NULL;
			/* Points to memory configuration table
			 * (bus configuration register values for 960CA
			 * memory regions 0-E), if any.  NULL otherwise.
                         */

/* Open a connection to a remote debugger.   
   FIXME, there should be a way to specify the various options that are
   now specified with gdb command-line options.  (baud_rate, 
   mconftab, and initial_brk)  */
void
nindy_open (name, from_tty)
    char *name;		/* "/dev/ttyXX", "ttyXX", or "XX": tty to be opened */
    int from_tty;
{

    if (!name)
	    error_no_arg ("serial port device name");

    nindy_close (0);
    
    have_regs = regs_changed = 0;
    dcache_init();

    /* Allow user to interrupt the following -- we could hang if
     * there's no NINDY at the other end of the remote tty.
     */
    immediate_quit++;
    nindy_fd = ninConnect(name, 
			  baud_rate ? baud_rate : NINDY_DEFAULT_BAUD_RATE,
			  target_initial_brk, 
			  ! from_tty, 
			  0);
    immediate_quit--;

    if ( nindy_fd < 0 ){
	nindy_fd = 0;
	error( "Can't open tty '%s'", name );
    }

    savename = savestring (name, strlen (name));
    push_target (&nindy_ops);
    target_fetch_registers(-1);

    nindy_get_target_byte_order();

    if ( mconftab ){
	ninConfRegs(mconftab);
    }

    /* Nindy-specific (as opposed to mon960) form of the reset command. */
    add_com ("reset", class_obscure, reset_command,
	   "Send a 'break' to the remote target system.\n\
Only useful if the target has been equipped with a circuit\n\
to perform a hard reset when a break is detected.");

}

/* Determine the target's byte ordering automatically.  
   NOTES: 
   (1) you don't have to do this if user set -G on command line.
   (2) you don't have to do this for Kx architectures (always little-endian)
   (3) for CA, the first checksum word in the IBR is -2.  Since -2 is stored
       into a fixed address, but in TARGET BYTE ORDER, we can determine
       the byte order.
   (4) global "target_byte_order" is set to LITTLE_ENDIAN on entry.
*/
static void
nindy_get_target_byte_order()
{
    CORE_ADDR magic_address;	/* address containing -2 in the IBR */
    char magic_number[4];	/* 0xfffffffe (bigendian) or
				   0xfeffffff (littleendian) */
    char verbuf[10];		/* Nindy's version info response, in the form
				   x.xx,aa, where x.xx is version number
				   aa is architecture: "KA", "KB", "CA" */

    if ( bigendian_switch )
	/* done on command-line */
	return;
    
    ninVersion(verbuf);

    if ( ! strncmp(verbuf + 5, "CA", 2) )
    {
	/* Read a signed word; if it is -2, then target_byte_order is OK.
	   If not, try swapping the byte order of the word.  If that results
	   in a -2, then target_byte_order is backwards.  If if still doesn't
	   look like a -2 to the host, then something else has been written
	   over the IBR and we have more problems than just byte order. 
	   In that case, bail and accept the default. */
	magic_address = 0xffffff18;
	target_read_memory(magic_address, magic_number, 4);
	if ( extract_signed_integer(magic_number, 4) != -2 )
	{
	    /* swap bytes and try again */
	    byteswap_within_word(magic_number);
	    if ( extract_signed_integer(magic_number, 4) == -2 )
		/* target order is backwards */
		target_byte_order = TARGET_LITTLE_ENDIAN ? BIG_ENDIAN : LITTLE_ENDIAN;
	}
	/* target order is now correct; big-endian targets need to swap
	   the BREAKPOINT (fmark) instruction */
	if ( TARGET_BIG_ENDIAN )
	    memory_swap_break_insn();
    }
    /* All other architectures default to little-endian */
}

/* User-initiated quit of nindy operations.  */

static void
nindy_detach (name, from_tty)
     char *name;
     int from_tty;
{
  dont_repeat ();
  if (name)
    error ("Too many arguments");
  pop_target ();
}

static void
nindy_files_info (dummy)
     struct target_ops *dummy; /* not used */
{
  printf("\tAttached to %s at %d bps %s initial break.\n", savename, baud_rate, target_initial_brk ? " with": " without");
}


/* Pass the args the way catch_errors wants them.  */
static int
nindy_open_stub (arg)
     char *arg;
{
  nindy_open (arg, 1);
  return 1;
}

static int
load_stub (arg)
     char *arg;
{
  target_load (arg, 1);
  return 1;
}

#ifdef IMSTG
	/* Non-zero iff a download has been performed since the last time an
	 * exec was done.
	 */
	static int nindy_file_loaded = 0;

	nindy_file_not_loaded()
	{
	  nindy_file_loaded = 0;
	}
#endif

void
nindy_load( filename, from_tty )
    char *filename;
    int from_tty;
{
  char *tmpfile;
  struct cleanup *old_chain;
  char *scratch_pathname;
  int scratch_chan;

  if (!filename)
    filename = get_exec_file (1);

  filename = tilde_expand (filename);
  make_cleanup (free, filename);

  scratch_chan = openp ((char *)getenv ("PATH"), 1, filename, O_RDONLY, 0,
			&scratch_pathname);
  if (scratch_chan < 0)
    perror_with_name (filename);
  close (scratch_chan);		/* Slightly wasteful FIXME */

  have_regs = regs_changed = 0;
  mark_breakpoints_out();
  inferior_pid = 0;
  dcache_flush();

#ifdef __HIGHC__
     /* coffstrip which we are about to call will spawn
      * objcopy. In Windows, leaving the timer and serial handlers installed 
      * during the spawn will crash the system. So we suspend them here.
      */
      close_com_port();
#endif

#if !defined(NO_BIG_ENDIAN_MODS)
  tmpfile = coffstrip(scratch_pathname,-1); /* make host big-endian same as target */
#else
  tmpfile = coffstrip(scratch_pathname,0);
#endif /* NO_BIG_ENDIAN_MODS */

#ifdef __HIGHC__
  /* restore timer/serial handler */
      open_com_port();
#endif

  if ( tmpfile ){
	  old_chain = make_cleanup(unlink,tmpfile);
	  immediate_quit++;
	  ninDownload( tmpfile, (!from_tty || quiet) );
/* FIXME, don't we want this merged in here? */
	  immediate_quit--;
	  do_cleanups (old_chain);
  }

#ifdef IMSTG
  nindy_file_loaded = 1;
#endif
}

/* Return the number of characters in the buffer before the first DLE character.
 */

static
int
non_dle( buf, n )
    char *buf;		/* Character buffer; NOT '\0'-terminated */
    int n;		/* Number of characters in buffer */
{
	int i;

	for ( i = 0; i < n; i++ ){
		if ( buf[i] == DLE ){
			break;
		}
	}
	return i;
}

/* Tell the remote machine to resume.  */

void
nindy_resume (pid, step, siggnal)
int pid, step;
enum target_signal siggnal;
{
    if (siggnal != TARGET_SIGNAL_0 && siggnal != TARGET_SIGNAL_STOP)
	error ("Can't send signals to remote NINDY targets.");
    
    dcache_flush();
    if ( regs_changed ){
	nindy_store_registers (0);
	regs_changed = 0;
    }
    have_regs = 0;
    if ( step && step_over_calls > 0 )
    {
	unsigned long rtn_addr;
	
	/* Handle stepping over calls.  In some cases the normal 
	   step-and-check method doesn't work (e.g. if we step into an
	   optimized function with no debug information we can't find the
	   return address.)  So ... insert a momentary breakpoint IF the
	   current PC is one of the 4 call instructions.  Then we'll resume
	   the target for RUN, not STEP.  */
	if ( rtn_addr = current_pc_is_call(read_pc()) )
	{
	    struct symtab_and_line sal;
	    struct breakpoint *bp = NULL;
	    
	    if ( breakpoints_inserted )
	    {
		remove_breakpoints();
		breakpoints_inserted = 0;
	    }
	    sal.pc = rtn_addr;
	    sal.symtab = NULL;
	    sal.line = 0;
	    bp = set_momentary_breakpoint (sal, NULL, bp_step_resume);
	    target_insert_breakpoint(bp->address, bp->shadow_contents);
	    bp->inserted = 1;

	    /* Make sure nindy_wait knows this is a special breakpoint */
	    nindy_next_breakpoint = bp;

	    /* Now resume target for RUN, not step */
	    ninGo(0);
	    return;
	}
    }
    ninGo( step );
}

/* Wait until the remote machine stops. While waiting, operate in passthrough
 * mode; i.e., pass everything NINDY sends to stdout, and everything from
 * stdin to NINDY.
 *
 * Return to caller, storing status in 'status' just as `wait' would.
 */

static int
nindy_wait( pid, status )
    int pid;
    struct target_waitstatus *status;
{
    DEMUX_DECL;	/* OS-dependent data needed by DEMUX... macros */
    char buf[500];	/* FIXME, what is "500" here? */
    int i, n;
    unsigned char stop_exit; /* 1 == target process has exited */
    unsigned char stop_code; /* nindy secret code explaining stop */
    enum target_signal stop_signal; /* gdb secret code explaining stop */

    TTY_STRUCT tty;
    long ip_value, fp_value, sp_value;	/* Reg values from stop */

    /* OPERATE IN PASSTHROUGH MODE UNTIL NINDY SENDS A DLE CHARACTER */

    /* Save current tty attributes, set up signals to restore them.
     */
    TTY_GET(0,orig_tty);

#if !(defined(DOS) && defined(__HIGHC__))
    /* Can't longjmp back up to top level from within an interrupt 
       handler under Phar Lap.  See comment in xm-dos.h */
    old_ctrlc = signal( SIGINT, cleanup );
#ifdef SIGTSTP
    old_ctrlz = signal( SIGTSTP, cleanup );
#endif
#endif
    /* Pass input from keyboard to NINDY as it arrives.
     * NINDY will interpret <CR> and perform echo.
     */
    tty = orig_tty;
    TTY_NINDYTERM( tty );
    TTY_SET(0,tty);

    while ( 1 ){
	/* Go to sleep until there's something for us on either
	 * the remote port or stdin.
	 */

#if defined(DOS) && defined(__HIGHC__)
	/* Must poll the CTRL-C interrupt.  See comment in xm-dos.h */
	QUIT;
#endif

	DEMUX_WAIT( nindy_fd );

	/* Pass input through to correct place */

	n = DEMUX_READ( 0, buf, sizeof(buf) );
	if ( n ){				/* Input on stdin */
	    WRITE_TTY( nindy_fd, buf, n );
	}

	n = DEMUX_READ( nindy_fd, buf, sizeof(buf) );
	if ( n ){				/* Input on remote */
	    /* Write out any characters in buffer preceding DLE */
	    i = non_dle( buf, n );
	    if ( i > 0 ){
		write( 1, buf, i );
	    }

	    if ( i != n ){
		/* There *was* a DLE in the buffer */
		stop_exit = ninStopWhy( &stop_code,
				       &ip_value, &fp_value, &sp_value);
		if ( !stop_exit && (stop_code==STOP_SRQ) ){
		    immediate_quit++;
		    ninSrq();
		    immediate_quit--;
		} else {
		    /* Get out of loop */
		    supply_register (IP_REGNUM, (char *) &ip_value);
		    supply_register (FP_REGNUM, (char *) &fp_value);
		    supply_register (SP_REGNUM, (char *) &sp_value);
		    break;
		}
	    }
	}
    }

#if !(defined(DOS) && defined(__HIGHC__))
    signal( SIGINT, old_ctrlc );
#ifdef SIGTSTP
    signal( SIGTSTP, old_ctrlz );
#endif
#endif
    restore_tty();

    if ( stop_exit )
    {
	/* User program exited */
	status->kind = TARGET_WAITKIND_EXITED;
	status->value.integer = stop_code;
    } 
    else 
    {
	/* Fault or trace */
	switch (stop_code)
	{
	case STOP_GDB_BPT:
	    /* Check for nexting-over-calls patch */
	    if ( nindy_next_breakpoint )
	    {
		delete_breakpoint( nindy_next_breakpoint );
		nindy_next_breakpoint = NULL;
		if ( breakpoints_inserted )
		    insert_breakpoints();
	    }
	    /* Deliberate fallthrough */
	case TRACE_STEP:
	    /* Make it look like a VAX trace trap */
	    stop_signal = TARGET_SIGNAL_TRAP;
	    break;
	default:
	    /* The target is not running Unix, and its
	       faults/traces do not map nicely into Unix signals.
	       Make sure they do not get confused with Unix signals
	       by numbering them with values higher than the highest
	       legal Unix signal.  code in i960_print_fault(),
	       called via PRINT_RANDOM_SIGNAL, will interpret the
	       value.  */
	    /* FIXME: this method no longer works with gdb's new
	       target_waitstatus system. 
	    stop_code += NSIG; */
	    stop_signal = TARGET_SIGNAL_UNKNOWN;
	    break;
	}
	status->kind = TARGET_WAITKIND_STOPPED;
	status->value.sig = stop_signal;
    }
    return inferior_pid;
}

/* Read the remote registers into the block REGS.  */

/* This is the block that ninRegsGet and ninRegsPut handles.  */
struct nindy_regs {
  char	local_regs[16 * 4];
  char	global_regs[16 * 4];
  char	pcw_acw[2 * 4];
  char	ip[4];
  char	tcw[4];
  char	fp_as_double[4 * 8];
};

static void
nindy_fetch_registers(dummy)
     int dummy; /* not used */
{
  struct nindy_regs nindy_regs;
  char fake_extended[12];

  immediate_quit++;
  ninRegsGet( (char *) &nindy_regs );
  immediate_quit--;

  bcopy (nindy_regs.local_regs, &registers[REGISTER_BYTE (R0_REGNUM)], 16*4);
  bcopy (nindy_regs.global_regs, &registers[REGISTER_BYTE (G0_REGNUM)], 16*4);
  bcopy (nindy_regs.pcw_acw, &registers[REGISTER_BYTE (PCW_REGNUM)], 2*4);
  bcopy (nindy_regs.ip, &registers[REGISTER_BYTE (IP_REGNUM)], 1*4);
  bcopy (nindy_regs.tcw, &registers[REGISTER_BYTE (TCW_REGNUM)], 1*4);
  /* AY-yi-yi.  nindy stores fp regs as doubles.  Period.  We are not
     going to rebuild nindy, but we want gdb to see fp regs 
     consistently.  So we'll do an extra, superflous conversion to
     IEEE-extended now. */
  double_to_extended (&nindy_regs.fp_as_double[0], fake_extended);
  bcopy (fake_extended, &registers[REGISTER_BYTE (FP0_REGNUM)], 10);
  double_to_extended (&nindy_regs.fp_as_double[8], fake_extended);
  bcopy (fake_extended, &registers[REGISTER_BYTE (FP0_REGNUM + 1)], 10);
  double_to_extended (&nindy_regs.fp_as_double[16], fake_extended);
  bcopy (fake_extended, &registers[REGISTER_BYTE (FP0_REGNUM + 2)], 10);
  double_to_extended (&nindy_regs.fp_as_double[24], fake_extended);
  bcopy (fake_extended, &registers[REGISTER_BYTE (FP0_REGNUM + 3)], 10);
  registers_fetched ();
}

static void
nindy_prepare_to_store()
{
  nindy_fetch_registers(-1);
}

static void
nindy_store_registers(dummy)
     int dummy; /* not used */
{
  struct nindy_regs nindy_regs;
  char fake_extended[12];

  bcopy (&registers[REGISTER_BYTE (R0_REGNUM)], nindy_regs.local_regs,  16*4);
  bcopy (&registers[REGISTER_BYTE (G0_REGNUM)], nindy_regs.global_regs, 16*4);
  bcopy (&registers[REGISTER_BYTE (PCW_REGNUM)], nindy_regs.pcw_acw,     2*4);
  bcopy (&registers[REGISTER_BYTE (IP_REGNUM)], nindy_regs.ip,           1*4);
  bcopy (&registers[REGISTER_BYTE (TCW_REGNUM)], nindy_regs.tcw,         1*4);
  /* AY-yi-yi.  nindy stores fp regs as doubles.  Period.  We are not
     going to rebuild nindy, but we want gdb to see fp regs 
     consisently.  So we'll do an extra, superflous conversion from
     IEEE-extended now. */
  bcopy (&registers[REGISTER_BYTE (FP0_REGNUM)], fake_extended, 10);
  extended_to_double (fake_extended, &nindy_regs.fp_as_double[0]);
  bcopy (&registers[REGISTER_BYTE (FP0_REGNUM + 1)], fake_extended, 10);
  extended_to_double (fake_extended, &nindy_regs.fp_as_double[8]);
  bcopy (&registers[REGISTER_BYTE (FP0_REGNUM + 2)], fake_extended, 10);
  extended_to_double (fake_extended, &nindy_regs.fp_as_double[16]);
  bcopy (&registers[REGISTER_BYTE (FP0_REGNUM + 3)], fake_extended, 10);
  extended_to_double (fake_extended, &nindy_regs.fp_as_double[24]);

  immediate_quit++;
  ninRegsPut( (char *) &nindy_regs );
  immediate_quit--;
}

/* Read a word from remote address ADDR and return it.
 * This goes through the data cache.
 */
int
nindy_fetch_word (addr)
     CORE_ADDR addr;
{
	return dcache_fetch (addr);
}

/* Write a word WORD into remote address ADDR.
   This goes through the data cache.  */

void
nindy_store_word (addr, word)
     CORE_ADDR addr;
     int word;
{
	dcache_poke (addr, word);
}

/* Copy LEN bytes to or from inferior's memory starting at MEMADDR
   to debugger memory starting at MYADDR.   Copy to inferior if
   WRITE is nonzero.  Returns the length copied.

   This is stolen almost directly from infptrace.c's child_xfer_memory,
   which also deals with a word-oriented memory interface.  Sometime,
   FIXME, rewrite this to not use the word-oriented routines.  */

int
nindy_xfer_inferior_memory(memaddr, myaddr, len, write, dummy)
     CORE_ADDR memaddr;
     char *myaddr;
     int len;
     int write;
     struct target_ops *dummy;  /* not used */
{
  register int i;
  /* Round starting address down to longword boundary.  */
  register CORE_ADDR addr = memaddr & - sizeof (int);
  /* Round ending address up; get number of longwords that makes.  */
  register int count
    = (((memaddr + len) - addr) + sizeof (int) - 1) / sizeof (int);
  /* Allocate buffer of that many longwords.  */
  register int *buffer = (int *) alloca (count * sizeof (int));
#ifndef DOS
  extern int errno;
#endif /*NOT DOS*/

  if (write)
    {
      /* Fill start and end extra bytes of buffer with existing memory data.  */

      if (addr != memaddr || len < (int)sizeof (int)) {
	/* Need part of initial word -- fetch it.  */
        buffer[0] = nindy_fetch_word (addr);
      }

      if (count > 1)		/* FIXME, avoid if even boundary */
	{
	  buffer[count - 1]
	    = nindy_fetch_word (addr + (count - 1) * sizeof (int));
	}

      /* Copy data to be written over corresponding part of buffer */

      bcopy (myaddr, (char *) buffer + (memaddr & (sizeof (int) - 1)), len);

      /* Write the entire buffer.  */

      for (i = 0; i < count; i++, addr += sizeof (int))
	{
	  errno = 0;
	  nindy_store_word (addr, buffer[i]);
	  if (errno)
	    return 0;
	}
    }
  else
    {
      /* Read all the longwords */
      for (i = 0; i < count; i++, addr += sizeof (int))
	{
	  errno = 0;
	  buffer[i] = nindy_fetch_word (addr);
	  if (errno)
	    return 0;
	  QUIT;
	}

      /* Copy appropriate bytes out of the buffer.  */
      bcopy ((char *) buffer + (memaddr & (sizeof (int) - 1)), myaddr, len);
    }
  return len;
}

/* The data cache records all the data read from the remote machine
   since the last time it stopped.

   Each cache block holds 16 bytes of data
   starting at a multiple-of-16 address.  */

#define DCACHE_SIZE 64		/* Number of cache blocks */

struct dcache_block {
	struct dcache_block *next, *last;
	unsigned int addr;	/* Address for which data is recorded.  */
	int data[4];
};

struct dcache_block dcache_free, dcache_valid;

/* Free all the data cache blocks, thus discarding all cached data.  */ 
static
void
dcache_flush ()
{
  register struct dcache_block *db;

  while ((db = dcache_valid.next) != &dcache_valid)
    {
      remque (db);
      insque (db, &dcache_free);
    }
}

/*
 * If addr is present in the dcache, return the address of the block
 * containing it.
 */
static
struct dcache_block *
dcache_hit (addr)
     unsigned int addr;
{
  register struct dcache_block *db;

  if (addr & 3)
    abort ();

  /* Search all cache blocks for one that is at this address.  */
  db = dcache_valid.next;
  while (db != &dcache_valid)
    {
      if ((addr & 0xfffffff0) == db->addr)
	return db;
      db = db->next;
    }
  return NULL;
}

/*  Return the int data at address ADDR in dcache block DC.  */
static
int
dcache_value (db, addr)
     struct dcache_block *db;
     unsigned int addr;
{
  if (addr & 3)
    abort ();
  return (db->data[(addr>>2)&3]);
}

/* Get a free cache block, put or keep it on the valid list,
   and return its address.  The caller should store into the block
   the address and data that it describes, then remque it from the
   free list and insert it into the valid list.  This procedure
   prevents errors from creeping in if a ninMemGet is interrupted
   (which used to put garbage blocks in the valid list...).  */
static
struct dcache_block *
dcache_alloc ()
{
  register struct dcache_block *db;

  if ((db = dcache_free.next) == &dcache_free)
    {
      /* If we can't get one from the free list, take last valid and put
	 it on the free list.  */
      db = dcache_valid.last;
      remque (db);
      insque (db, &dcache_free);
    }

  remque (db);
  insque (db, &dcache_valid);
  return (db);
}

/* Return the contents of the word at address ADDR in the remote machine,
   using the data cache.  */
static
int
dcache_fetch (addr)
     CORE_ADDR addr;
{
  register struct dcache_block *db;
  extern int caching_enabled;

  db = dcache_hit (addr);
  if (db == 0) {
      db = dcache_alloc ();
      immediate_quit++;
      ninMemGet(addr & ~0xf, (unsigned char *)db->data, 16);
      immediate_quit--;
      db->addr = addr & ~0xf;
      remque (db);			/* Off the free list */
      insque (db, &dcache_valid);	/* On the valid list */
  }
  else if (!caching_enabled) {
      immediate_quit++;
      ninMemGet(addr & ~0xf, (unsigned char *)db->data, 16);
      immediate_quit--;
  }
  return (dcache_value (db, addr));
}

/* Write the word at ADDR both in the data cache and in the remote machine.  */
static void
dcache_poke (addr, data)
     CORE_ADDR addr;
     int data;
{
  register struct dcache_block *db;
  extern int caching_enabled;

  /* First make sure the word is IN the cache.  DB is its cache block.  */
  db = dcache_hit (addr);
  if (db == 0)
    {
      db = dcache_alloc ();
      immediate_quit++;
      ninMemGet(addr & ~0xf, (unsigned char *)db->data, 16);
      immediate_quit--;
      db->addr = addr & ~0xf;
      remque (db);			/* Off the free list */
      insque (db, &dcache_valid);	/* On the valid list */
    }
  else if (!caching_enabled) {
      immediate_quit++;
      ninMemGet(addr & ~0xf, (unsigned char *)db->data, 16);
      immediate_quit--;
  }
  /* Modify the word in the cache.  */
  db->data[(addr>>2)&3] = data;

  /* Send the changed word.  */
  immediate_quit++;
  ninMemPut(addr, (unsigned char *)&data, 4);
  immediate_quit--;
}

/* The cache itself. */
struct dcache_block the_cache[DCACHE_SIZE];

/* Initialize the data cache.  */
static void
dcache_init ()
{
  register i;
  register struct dcache_block *db;

  db = the_cache;
  dcache_free.next = dcache_free.last = &dcache_free;
  dcache_valid.next = dcache_valid.last = &dcache_valid;
  for (i=0;i<DCACHE_SIZE;i++,db++)
    insque (db, &dcache_free);
}

/* nindy_frame_chain_valid:
 * Called from frame_chain_valid() in i960-tdep.c when nindy
 * is the current target.
 *
 * 'start_frame' is a variable in the NINDY runtime startup routine
 * that contains the frame pointer of the 'start' routine (the routine
 * that calls 'main').  By reading its contents out of remote memory,
 * we can tell where the frame chain ends:  backtraces should halt before
 * they display this frame.  
 */
int
nindy_frame_chain_valid (chain, curframe)
    unsigned int chain;
    FRAME curframe;
{
	struct symbol *sym;
	struct minimal_symbol *msymbol;

	/* crtnindy.o is an assembler module that is assumed to be linked
	 * first in an i80960 executable.  It contains the true entry point;
	 * it performs startup up initialization and then calls 'main'.
	 *
	 * 'sf' is the name of a variable in crtnindy.o that is set
	 *	during startup to the address of the first frame.
	 *
	 * 'a' is the address of that variable in 80960 memory.
	 */
	static char sf[] = "start_frame";
	CORE_ADDR a;


	chain &= ~0x3f; /* Zero low 6 bits because previous frame pointers
			   contain return status info in them.  */
	if ( chain == 0 ){
		return 0;
	}

	sym = lookup_symbol(sf, 0, VAR_NAMESPACE, (int *)NULL, 
				  (struct symtab **)NULL);
	if ( sym != 0 ){
		a = SYMBOL_VALUE (sym);
	} else {
		msymbol = lookup_minimal_symbol (sf, (struct objfile *) NULL);
		if (msymbol == NULL)
			return 0;
		a = SYMBOL_VALUE_ADDRESS (msymbol);
	}

	return ( chain != read_memory_integer(a,4) );
}

static void
nindy_create_inferior (execfile, args, env)
     char *execfile;
     char *args;
     char **env;
{
  int entry_pt;
  int pid = 42;

  if (args && *args){
    execute_command ("set args", 0);
    error ("Can't pass arguments to remote NINDY process");
  }

  if (execfile == 0 || exec_bfd == 0)
    error ("No exec file specified");

#ifdef IMSTG
  {
    static char confirm_load[] = "No download since last 'exec'.  Run anyway? ";

    if ( !nindy_file_loaded && !batch ){
      if ( query(confirm_load) ){
	nindy_file_loaded = 1;
      } else {
	error("");
      }
    }
  }
#endif

  entry_pt = (int) bfd_get_start_address (exec_bfd);

/* The "process" (board) is already stopped awaiting our commands, and
   the program is already downloaded.  We just set its PC and go.  */

  inferior_pid = pid;		/* Needed for wait_for_inferior below */

  clear_proceed_status ();

  /* Tell wait_for_inferior that we've started a new process.  */
  init_wait_for_inferior ();

  /* Set up the "saved terminal modes" of the inferior
     based on what modes we are starting it with.  */
  target_terminal_init ();

  /* Install inferior's terminal modes.  */
  target_terminal_inferior ();

  /* remote_start(args); */
  /* trap_expected = 0; */
  /* insert_step_breakpoint ();  FIXME, do we need this?  */
  proceed ((CORE_ADDR)entry_pt, -1, 0);		/* Let 'er rip... */
}

static void
reset_command(args, from_tty)
     char *args;
     int from_tty;
{
	if ( !nindy_fd ){
	    error( "No target system to reset -- use 'target nindy' command.");
	}
	if ( query("Really reset the target system?",0,0) ){
		send_break( nindy_fd );
		tty_flush( nindy_fd );
	}
}

void
nindy_kill ()
{
  return;		/* Ignore attempts to kill target system */
}

/* Clean up when a program exits.

   The program actually lives on in the remote processor's RAM, and may be
   run again without a download.  Don't leave it full of breakpoint
   instructions.  */

void
nindy_mourn_inferior ()
{
  remove_breakpoints ();
  generic_mourn_inferior ();	/* Do all the proper things now */
}

/* This routine is run as a hook, just before the main command loop is
   entered.  If gdb is configured for the i960, but has not had its
   nindy target specified yet, this will loop prompting the user to do so.

   Unlike the loop provided by Intel, we actually let the user get out
   of this with a RETURN.  This is useful when e.g. simply examining
   an i960 object file on the host system.  */

nindy_before_main_loop ()
{
  char ttyname[100];
  char *p, *p2;

  while (current_target != &nindy_ops) { /* remote tty not specified yet */
	if ( instream == stdin ){
		printf(TTY_ATTACH_MSG);
		gdb_flush( stdout );
	}
	fgets( ttyname, sizeof(ttyname)-1, stdin );

	/* Strip leading and trailing whitespace */
	for ( p = ttyname; isspace(*p); p++ ){
		;
	}
	if ( *p == '\0' ){
		return;		/* User just hit spaces or return, wants out */
	}
	for ( p2= p; !isspace(*p2) && (*p2 != '\0'); p2++ ){
		;
	}
	*p2= '\0';
	if ( STREQ("quit",p) ){
		exit(1);
	}

	nindy_open( p, 1 );

	if (catch_errors (nindy_open_stub, p, "", RETURN_MASK_ALL))
	  {
	    /* Now that we have a tty open for talking to the remote machine,
	       download the executable file if one was specified.  */
	    if (exec_bfd)
	      {
		catch_errors (load_stub, bfd_get_filename (exec_bfd), "",
			      RETURN_MASK_ALL);
	      }
	  }
  }
}

/* Define the target subroutine names */

struct target_ops nindy_ops = {
	"nindy", "Remote serial target in i960 NINDY-specific protocol",
	"Remote debugging on the NINDY monitor via a serial line.\n\
Specify the name of the device the serial line is connected to.\n\
Use the gdb command line to specify the speed (baud rate), whether\n\
to use the old NINDY protocol, and whether to send a break on startup.",
	nindy_open, nindy_close,
	0,
	nindy_detach,
	nindy_resume,
	nindy_wait,
	nindy_fetch_registers, nindy_store_registers,
	nindy_prepare_to_store,
	nindy_xfer_inferior_memory, nindy_files_info,
	0, 0, /* insert_breakpoint, remove_breakpoint, */
	0, 0, /* insert_hw_breakpoint, remove_hw_breakpoint, */
	0, 0, /* insert_watchpoint, remove_watchpoint, */
	0, /* stopped_data_address */
	0, 0, 0, 0, 0,	/* Terminal crud */
	nindy_kill,
	nindy_load,
	0, /* lookup_symbol */
	nindy_create_inferior,
	nindy_mourn_inferior,
	0,		/* can_run */
	0, /* notice_signals */
	process_stratum, 0, /* next */
	1, 1, 1, 1, 1,	/* all mem, mem, stack, regs, exec */
	0, 0,  /* hardware breakpoint, watchpoint limit */
	0, 0,			/* Section pointers */
	OPS_MAGIC,		/* Always the last thing */
};

void
_initialize_nindy ()
{
  add_target (&nindy_ops);
}
