/* Remote debugging interface for Intel mon960 monitor Version 1.1
   and later.  

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* System include files */

#include <stdio.h>
#include <fcntl.h>    /* for O_RDONLY.. */
#include <ctype.h>    /* for isspace() */
#include <signal.h>   /* for SIGINT */

#include "defs.h"
#include "frame.h"
#include "inferior.h"
#include "bfd.h"
#include "symfile.h"
#include "target.h"
#include "breakpoint.h" /* for enum bptype */
#include "gdbcmd.h"
#include "gdbcore.h"  /* get_exec_file() */
#include "valprint.h" /* for addressprint flag */
#include "dis-asm.h"  /* for struct disassemble_info */
#ifdef DOS
#include <stdlib.h>   /* for getenv */
#include "gnudos.h"
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* Track current target communication channel. */
#define GDB_COMM_NONE   0
#define GDB_COMM_SERIAL 1
#define GDB_COMM_PCI    2

/* HDIL include files. */

/* To avoid confusion, we use the NUM_REGS macro
 * from hdil.h NOT from tm-i960.h
 */
#undef  NUM_REGS

#define HOST
#include "hdil.h"
#include "hdi_com.h"

/* HDI_CALL calls an HDI function and prints an error message if the 
   return value is anything other than OK.  HDI_CALL can be used as 
   an expression wherever "a" could be used. */
#define HDI_CALL(a) ((a) == OK ? OK : \
    (fprintf_filtered(gdb_stderr, "HDIL error (%d), %s\n", hdi_cmd_stat,  hdi_get_message()), ERR))
#define HDI_CONNECTED (mon960_device[0] != (char)NULL)
#define HDI_SANITY() \
	if (!HDI_CONNECTED) { fprintf_filtered(gdb_stderr, "Not connected to any port right now\n"); return; }
#define HDI_SANITY_WITH_RTN(x) \
	if (!HDI_CONNECTED) { fprintf_filtered(gdb_stderr, "Not connected to any port right now\n"); return x; }

#define SAVESTRING(x) savestring((x),strlen(x))

#define PCI_COMM_INDX    0		/* Index to comm_used[] array */
#define PARA_COMM_INDX   1		/* Index to comm_used[] array */

#define PCI_COMM_DFLT    0x1
#define PCI_COMM_BUS     0x2
#define PCI_COMM_VND     0x4
#define PCI_COMM_MAX_VAL (PCI_COMM_VND)
#define PCI_COMM_MIN_VAL (PCI_COMM_DFLT | PCI_COMM_BUS)

#define DEFAULT_BAUD_RATE 38400
#define DEFAULT_PKT_LEN "1024"
#define DEFAULT_RESET_TIME "0"		/* the target resets in 0 millisecsonds */
#define DEFAULT_TINT "-1"		/* N/A */
#define DEFAULT_MON_PRIORITY "-1"	/* N/A */

#define DEFAULT_NO_RESET 0		/* reset upon hdi_init(), if needed */
#define DEFAULT_SLEEP_SECS 1		/* this is a hack for hdilcomm */

/* GLOBAL global VARIABLES */

/* Command-line option args.  These must be global to be seen in main(). */
char 	*target_ttyname;		/* e.g. "/dev/tty01" */
char 	*target_type = "mon960";	/* or "whatever" */
char 	*parallel_device;		/* valid if using parallel download */
int 	target_initial_brk;		/* send break to target on startup */
int	pci_dflt_conn;			/* -pci switch set by user */
int	pci_debug;    			/* -pcid switch set by user */
char	*pci_config_arg;		/* -pcic cmdline arg */
char	*pci_bus_arg[3];		/* -pcib cmdline arg */
char	*pci_vnd_arg[2];		/* -pciv cmdline arg */
int 	bigendian_switch;		/* was -G seen on the command-line? */

/* bytes to insert for software breakpoint, little-endian target */
unsigned char little_endian_break_insn[] = LITTLE_ENDIAN_BREAKPOINT;

/* bytes to insert for software breakpoint, big-endian target */
unsigned char big_endian_break_insn[] = BIG_ENDIAN_BREAKPOINT;

/* address that caused a STOP_BP_DATA trap */
CORE_ADDR mon960_data_bp_address;

/* GLOBAL static VARIABLES */

/* Vars to support parallel and PCI download, as well as PCI comm. */
static DOWNLOAD_CONFIG fast_config_record = { 0, 0, NULL, { COM_PCI_IOSPACE } };
static DOWNLOAD_CONFIG *fast_config       = NULL;
static DOWNLOAD_CONFIG saved_fast_config;   /* Needed when terminating comm
                                             * channel.
                                             */

/* 
 * comm_channel tracks GDB's current communication channel.  Can be one of
 * GDB_COMM_NONE, GDB_COMM_PCI, or GDB_COMM_SERIAL.
 */
static int comm_channel = GDB_COMM_NONE; 

static HDI_CONFIG     hdi_config; /* filled in by hdi_init */

static char mon960_device[32];
static int  arch_type;        /* type of i960: ARCH_KA... etc */
static char *loadedfilename;  /* These two filled in by mon960_load. */

static char *runargs;         /* Initialized by mon960_create_interior,
			       * referenced by hdi_get_cmdline 
			       */

static char **saved_environ;	/* Save and restore of gdb's environment
				 * on entry to mon960_create_inferior
				 */

/* This is the host system's pointer to the user's environment. */
extern char **environ;

/* From infrun.c: nonzero if breakpoints are currently inserted 
   in the inferior.  Used in mon960_resume. */
extern int breakpoints_inserted;

/* This is true if the target is running and stopped at a breakpoint. */
static const STOP_RECORD *run_stop_record; 

/* mon960 target operations. */
static void	mon960_open	PARAMS ((char *, int));
static void	mon960_close 	PARAMS ((int));
static void 	mon960_detach 	PARAMS ((char *, int));
static void	mon960_resume	PARAMS ((int, int, enum target_signal));
static int	mon960_wait	PARAMS ((int, struct target_waitstatus *));
static void	mon960_fetch_registers 	PARAMS ((int));
static void	mon960_store_registers	PARAMS ((int));
static void	mon960_prepare_to_store	PARAMS ((void));
static int	mon960_xfer_memory	PARAMS ((CORE_ADDR, char *, int, int, struct target_ops *));
static void	mon960_files_info	PARAMS ((struct target_ops *));
static int	mon960_insert_breakpoint	PARAMS ((CORE_ADDR, char *, int));
static int	mon960_remove_breakpoint 	PARAMS ((CORE_ADDR, char *));
static void	mon960_kill	PARAMS ((void));
static void	mon960_load	PARAMS ((char *, int));
static void	mon960_create_inferior	PARAMS ((char *, char *, char **));
static void	mon960_mourn_inferior	PARAMS ((void));

/* Other mon960 functions. */
static void	mon960_signal_handler	    PARAMS ((int));
static void	init_i960_architecture	    PARAMS ((int));

/* Interface required by gdb 4.13 HW breakpoint system */
int mon960_check_hardware_resources 	PARAMS ((enum bptype, int));
int mon960_insert_hw_breakpoint	PARAMS ((CORE_ADDR, char *));
int mon960_remove_hw_breakpoint	PARAMS ((CORE_ADDR, char *));
int mon960_insert_watchpoint 	PARAMS ((CORE_ADDR, int, int));
int mon960_remove_watchpoint 	PARAMS ((CORE_ADDR, int, int));
CORE_ADDR mon960_stopped_data_address PARAMS ((void));

/* Interface to disassembly info */
int mon960_dis_asm_read_memory PARAMS ((CORE_ADDR, char *, int, disassemble_info *));

/*
 * target operations:  function jump table for mon960 target.
 */
struct target_ops mon960_ops = 
{
    "mon960",
    "Remote target in i960 MON960-specific protocol", 
#ifdef DOS
    "Remote debugging on the mon960 monitor via a serial line or PCI bus.\n\
Specify the serial device or PCI address it is connected to (e.g. com1\n\
or -pci).  Optional HDI arguments can follow, e.g. -baud baudrate",
#else
    "Remote debugging on the mon960 monitor via a serial line.\n\
Specify the serial device it is connected to (e.g. /dev/ttya).\n\
Optional HDI arguments can follow, e.g. -baud baudrate",
#endif
    mon960_open,
    mon960_close,
    0,		/* void (*to_attach) (); */
    mon960_detach,
    mon960_resume,
    mon960_wait,
    mon960_fetch_registers,
    mon960_store_registers,
    mon960_prepare_to_store,
    mon960_xfer_memory,
    mon960_files_info,
    0,	/* int (*to_insert_breakpoint) (); */
    0,	/* int (*to_remove_breakpoint) (); */
    mon960_insert_hw_breakpoint,
    mon960_remove_hw_breakpoint,
    mon960_insert_watchpoint,
    mon960_remove_watchpoint,
    mon960_stopped_data_address,
    0,	/* void (*to_terminal_init) (); */
    0, 	/* void (*to_terminal_inferior) (); */
    0,	/* void (*to_terminal_ours_for_output) (); */
    0,	/* void (*to_terminal_ours) (); */
    0,	/* void (*to_terminal_info) (); */
    mon960_kill,
    mon960_load,
    0,	/* int (*to_lookup_symbol) (); */
    mon960_create_inferior,
    mon960_mourn_inferior,
    0,	/* int (*to_can_run) PARAMS (); */
    0,	/* void (*to_notice_signals) PARAMS (); */
    process_stratum,
    0,	/* struct target_ops *to_next; */
    1,	/* to_has_all_memory; */
    1,	/* to_has_memory; */
    0,	/* to_has_stack; */
    1,	/* to_has_registers; */
    0,	/* to_has_execution; */
    0,  /* to_hardware_breakpoint_limit; */
    0,  /* to_hardware_watchpoint_limit; */
    0,	/* to_sections; */
    0,	/* to_sections_end; */
    OPS_MAGIC /* to_magic; */
};

/* Routines required by hdil. */
void
hdi_get_cmd_line(cmdline, len)
    char *cmdline;
    int len;
{
    char buff[1024];

    if ( ! loadedfilename )
    {
        char *cp = get_exec_file(1);

        loadedfilename = SAVESTRING(cp);
    }
    sprintf(buff,"%s %s",loadedfilename ? loadedfilename : "",runargs ? runargs : "");
    strncpy(cmdline,buff,len);    
}

void
hdi_put_line(p)
const char *p;
{
	printf_filtered("%s", p);
	gdb_flush(gdb_stdout);
}
void
hdi_user_put_line(cp, len)
const char *cp;
int len;
{
	write(1, cp, len);
}

int
hdi_user_get_line(buf, size)
char *buf;
int size;
{
	return read(0, buf, size);
}



static void
dump_pci_addr(connect_verb)
char *connect_verb;
{
    printf_filtered(
            "%s to PCI target at bus# %#x, dev# %#x, func# %#x in %s mode.\n",
                    connect_verb,
                    fast_config->init_pci.bus,
                    fast_config->init_pci.dev,
                    fast_config->init_pci.func,
            (fast_config->init_pci.comm_mode == COM_PCI_MMAP) ? "mmap" : "io"
                    );
}



static void
mon960_reset(args,from_tty)
    char *args;
    int from_tty;
{
    HDI_SANITY();
    if ( query("Really reset the target system?",0,0) )
    {
	HDI_CALL(hdi_reset());
	init_gmu_cmd_cache();
	if (runargs)
	    free(runargs);
	runargs = 0;
	if (loadedfilename)
	    free(loadedfilename);
	loadedfilename = 0;
	run_stop_record = 0;
    }
    else 
	puts_unfiltered("Target system not reset\n");
}

/* This array maps a 'regno' in param.h to an hdi_regname. */

static int regno_to_hdi_regname_map[TOTAL_POSSIBLE_REGS];

static void init_hdi_reg_map()
{
    regno_to_hdi_regname_map[R0_REGNUM]    = REG_R0;
    regno_to_hdi_regname_map[R0_REGNUM+1]  = REG_R1;
    regno_to_hdi_regname_map[R0_REGNUM+2]  = REG_R2;
    regno_to_hdi_regname_map[R0_REGNUM+3]  = REG_R3;
    regno_to_hdi_regname_map[R0_REGNUM+4]  = REG_R4;
    regno_to_hdi_regname_map[R0_REGNUM+5]  = REG_R5;
    regno_to_hdi_regname_map[R0_REGNUM+6]  = REG_R6;
    regno_to_hdi_regname_map[R0_REGNUM+7]  = REG_R7;
    regno_to_hdi_regname_map[R0_REGNUM+8]  = REG_R8;
    regno_to_hdi_regname_map[R0_REGNUM+9]  = REG_R9;
    regno_to_hdi_regname_map[R0_REGNUM+10] = REG_R10;
    regno_to_hdi_regname_map[R0_REGNUM+11] = REG_R11;
    regno_to_hdi_regname_map[R0_REGNUM+12] = REG_R12;
    regno_to_hdi_regname_map[R0_REGNUM+13] = REG_R13;
    regno_to_hdi_regname_map[R0_REGNUM+14] = REG_R14;
    regno_to_hdi_regname_map[R0_REGNUM+15] = REG_R15;
    regno_to_hdi_regname_map[G0_REGNUM]    = REG_G0;
    regno_to_hdi_regname_map[G0_REGNUM+1]  = REG_G1;
    regno_to_hdi_regname_map[G0_REGNUM+2]  = REG_G2;
    regno_to_hdi_regname_map[G0_REGNUM+3]  = REG_G3;
    regno_to_hdi_regname_map[G0_REGNUM+4]  = REG_G4;
    regno_to_hdi_regname_map[G0_REGNUM+5]  = REG_G5;
    regno_to_hdi_regname_map[G0_REGNUM+6]  = REG_G6;
    regno_to_hdi_regname_map[G0_REGNUM+7]  = REG_G7;
    regno_to_hdi_regname_map[G0_REGNUM+8]  = REG_G8;
    regno_to_hdi_regname_map[G0_REGNUM+9]  = REG_G9;
    regno_to_hdi_regname_map[G0_REGNUM+10] = REG_G10;
    regno_to_hdi_regname_map[G0_REGNUM+11] = REG_G11;
    regno_to_hdi_regname_map[G0_REGNUM+12] = REG_G12;
    regno_to_hdi_regname_map[G13_REGNUM]   = REG_G13;
    regno_to_hdi_regname_map[G14_REGNUM]   = REG_G14;
    regno_to_hdi_regname_map[FP_REGNUM]    = REG_G15;
    regno_to_hdi_regname_map[PCW_REGNUM]   = REG_PC;
    regno_to_hdi_regname_map[ACW_REGNUM]   = REG_AC;
    regno_to_hdi_regname_map[IP_REGNUM]    = REG_IP;
    regno_to_hdi_regname_map[TCW_REGNUM]   = REG_TC;
    regno_to_hdi_regname_map[SF0_REGNUM]   = REG_SF0;
    regno_to_hdi_regname_map[SF0_REGNUM+1] = REG_SF1;
    regno_to_hdi_regname_map[SF0_REGNUM+2] = REG_SF2;
    regno_to_hdi_regname_map[SF0_REGNUM+3] = REG_SF3;
    regno_to_hdi_regname_map[SF0_REGNUM+4] = REG_SF4;
}

enum regno_enum 
{ 
    all_regs, 
    thirty_two_bit_regs, 
    eighty_bit_regs, 
    out_of_range
};

static enum regno_enum
which_regno(regno)
    int regno;
{
    if (regno == -1)
	return all_regs;
    else if (regno >= R0_REGNUM && regno < FP0_REGNUM)
	return thirty_two_bit_regs;
    else if (regno >= FP0_REGNUM && regno <= (FP0_REGNUM+3))
	return eighty_bit_regs;
    else
	return out_of_range;
}

#define MAXDEVCHARS 19		/* in device passed to target command */
#define MAX_GDB_OPTS 10

static unsigned char gdb_opt_set[MAX_GDB_OPTS];

static const COM_INVOPT gdb_com_opts[] = {
        {"reset_time",   &gdb_opt_set[0], TRUE,  NULL},
        {"no_reset",     &gdb_opt_set[1], FALSE, NULL},
        {"tint",         &gdb_opt_set[2], TRUE,  NULL},
        {"mon_priority", &gdb_opt_set[3], TRUE,  NULL}
};
#define NUM_GDB_OPTS (sizeof(gdb_com_opts)/sizeof(COM_INVOPT))

static int
set_mon_priority (arg)
const char * arg;
{
	int	prio = atoi(arg);
	if ( prio >= -1 && prio <= 0x1f )
	{
		hdi_config.mon_priority = prio;
		return 0;
	}
	else
		return -1;
}

static int
set_reset_time (arg)
const char * arg;
{
	int	reset_time;

	reset_time = atoi(arg);
	if ( reset_time >= -1 )
	{
		hdi_config.reset_time = reset_time;
		return 0;
	}
	else
		return -1;
}

static int
set_no_reset (dummy)
const char * dummy;
{
	hdi_config.no_reset = 1;
	return 0;
}

static int
set_tint (arg)
const char * arg;
{
	int	tint;
	if ( ! strcmp(arg, "nmi") || ! strcmp(arg, "NMI") )
	{
		hdi_config.tint = 8;
		return 0;
	}
	tint = atoi(arg);
	if ( tint >= -1 && tint <= 7 )
	{
		hdi_config.tint = tint;
		return 0;
	}
	else
		return -1;
}

static const opt_handler_t gdb_opt_handler[] =
{
        set_reset_time,
        set_no_reset,
        set_tint,
        set_mon_priority
};

static char getnextarg_return[100];

char *
getnextarg(app)
char **app;
{
   /* return next white-space delimited argument in **app; set
    * app to the start of the next one after the returned one,
    * or NULL if no more exist.
    */

    char *ap;
    char *rp = getnextarg_return;

    if (!(*app) || !(**app))
        return (char *)NULL;

    ap = *app;

    while (isspace(*ap)) ++ap;

    while (*ap && !isspace(*ap))
    {
        *rp++ = *ap++;
    }

    *app = ap;

    if (rp == getnextarg_return) /* trailing space */
        return (char *) NULL;

    *rp = '\0';

    return (getnextarg_return);
}

/*
 * match serial device prefix with input.
 * return real device name, or NULL if we can't match.
 */
char *
matchDevice(suffix)
char *suffix;
{
    static char *prefix[] = { "", "/dev/", "/dev/tty", NULL };
    static char devbuf[100];  /* cannot exceed 19 chars */
    int i;

#ifdef	DOS
    /* Don't try to access something called "COM1".  com port must be 
       exactly 4 chars long and the first 3 must be "COM"; let HDI catch it
       if it's "COM8" or other nonsense */
    if ( strlen(suffix) != 4 )
	return NULL;
    strcpy (devbuf, suffix);
    for ( i = 0; i < 3; ++i )
	devbuf[i] = toupper(devbuf[i]);
    if ( strncmp(devbuf, "COM", 3) )
	return NULL;
    else
	return devbuf;
#else
    /* Unix */
    for ( i = 0; prefix[i] != NULL; i++ ) {
        strcpy(devbuf, prefix[i]);
        strcat(devbuf, suffix);
        /* if we can write to it, assume it exists; 
	 * The 2 comes from #define W_OK 2 in unistd.h; but unistd.h
	 * is not found on all systems -- hence the name :->
	 */
        if (access(devbuf, 2) == 0) {
      		return(devbuf); 
        }
    }
    return(NULL);
#endif
}

/*
 * match parallel device prefix with input.
 * For now, this doesn't do much.  It just initializes a global
 * data structure with the "device" argument.
 * In future maybe we could do some name-checking on the parallel device.
 * For now, name-checking is done in hdi_download.
 */
char *
matchParallelDevice(device)
char *device;
{
    static char devbuf[56];

    if ( ! *device )
        parallel_device = NULL;
    else
    {
        fast_config_record.download_selector = FAST_PARALLEL_DOWNLOAD;
        fast_config_record.fast_port         = parallel_device = devbuf;
        fast_config                          = &fast_config_record;
#if DOS
        strncpy (devbuf, device, 4);
        devbuf[4] = 0;        /* Handles case of "LPT1:" also */
#else
        /* Unix -- copy user-specified device to static buffer. */
        {
            int i = 0;

            while (*device && i < (sizeof(devbuf) - 1) && ! isspace(*device))
                devbuf[i++] = *device++;
            devbuf[i] = '\0';
        }
#endif
    }
    return parallel_device;
}

/*
 * mon960_open
 *
 * called by target command to setup initial
 * connection to "target".
 *
 * params:
 *	other_args: typically something like "/dev/ttya 9600"
 *	comes from target command as optional params.
 */
static int abortClose = 0;

static void
mon960_open (other_args, from_tty)
    char *other_args;	/* "/dev/ttyXX", "ttyXX", or "XX": tty to be opened
			 followed optionally by hdil options. */
    int from_tty;
{
    int i;
    int rv;
    char *serial_dev;
    struct cleanup *cleanup_chain;
#ifdef DOS
    static char *syntax = "{serial port device name | pci address} [-baud bbbb] [hdil arguments]";
#else
    static char *syntax = "serial port device name [-baud bbbb] [hdil arguments]";
#endif
    COM_INVOPT *opt_list;
    const COM_INVOPT *com_opt_list;
    opt_handler_t *opt_handler;
    const opt_handler_t *com_opt_handler;
    int opt_count;
    void (*ctrlc_handler)();
#ifdef WIN95
    void (*ctrlbrk_handler)();
#endif
    char *ap = other_args; /* from the "target" command line */
    char *args; /* copy of "target mon960 args" plus extra hdilcomm-specific */
    char *tmp;  /* all-purpose char pointer */
    int  wants_pci_comm;   /* T -> Cmdline opts indicate PCI comm desired. */
    char comm_used[2];     /* Track number of parallel/pci comm devices
                            * specified by the user. More than one
                            * selection is a semantic error.
                            */

    if (!other_args)
        error_no_arg (syntax);

    wants_pci_comm = FALSE;   /* An assumption. */
    comm_used[0]   = comm_used[1] = 0;
    fast_config    = NO_DOWNLOAD_CONFIG;

    /* Check if target is open already (note this was
     * formerly done in target_command)
     */
    target_preopen(0);

    tmp = getnextarg(&ap);

#ifdef DOS
    /*
     * Running under DOS.  DOS supports both serial and PCI comm, which
     * means that the first argument passed to this routine may be one of:
     *
     *   a serial port device name (e.g., com[1-4])
     *   -pci
     *   -pcib
     *   -pciv
     */
     if (*tmp == '-' || *tmp == '/')
     {
        /* DOS option char... */

        char *cp = tmp + 1;

        if (strcmp(cp, "pci") == 0 || 
                       strcmp(cp, "pcib") == 0 ||
                                       strcmp(cp, "pciv") == 0)
        {
            wants_pci_comm = TRUE;
        }
    }
#endif
    if (! wants_pci_comm)
    {
        /* First command argument must be a serial port name. */

        serial_dev = matchDevice(tmp);
        if ( serial_dev == NULL )
            error ("Can't find serial device: %s\n", tmp);
    }

    /* 
     * Construct a new args list based on user communication channel
     * preference (serial or PCI) and parse the rest of the list.
     * Serial comm's command list will look like this:
     *     "-com parsed_device [command-line options] [hdilcomm args]"
     * PCI comm's command list will look like this:
     *     "pci_addr [command-line options] [hdilcomm args]"
     */
    args = xmalloc(strlen(ap) + 256);
    cleanup_chain = make_cleanup (free, args);
    if (! wants_pci_comm)
    {
        strcpy(args, "-com ");
        strcat(args, serial_dev);
    }
    else
        strcpy(args, tmp);  /* tmp holds name of user-specified PCI option. */

    strcat(args, " ");
    strcat(args, ap);
    if (tmp = strstr(args, "-b "))
    {
        /*
         * Expand -b (an alias) and its argument to a nonaliased switch.
         * This expansion is necessary because hdil will parse the argument
         * list and doesn't know about "-b".
         */

        char *b_ap, *b_aep, baud_arg[32], *c;
        
        if (c = strchr(tmp, ' '))
        { 
            while (*c && isspace(*c)) 
                c++;
        }
        if (! c || 
#ifdef DOS
                    *c == '/' || 
#endif
                                *c == '-')
        {
            error("Expected baud rate after -b.");
        }
        *tmp++ = ' ';            /* Blank out the -b alias */
        *tmp++ = ' ';
        while (*tmp && isspace(*tmp))
            tmp++;               /* Skip -b delimiters     */
        b_ap  = baud_arg;
        b_aep = baud_arg + sizeof(baud_arg) - 1;
        while (*tmp && ! isspace(*tmp) && b_ap < b_aep)
        {
            *b_ap++ = *tmp;
            *tmp++  = ' ';       /* blank out argument to -b */
        }
        *b_ap = '\0';
        strcat(args, " -baud "); /* Append nonaliased switch to end of list */
        strcat(args, baud_arg);
    }

    /* If -baud is on command line, set gdb's global variable while
       you're at it. */
    if ( tmp = strstr(args, "-baud ") )
    {
        baud_rate = atoi(strchr(tmp, ' '));
    }
    else
    {
	/* No -baud; use mon960 default. */
	char minibuf[32];
	if ( baud_rate == -1 )
	    baud_rate = DEFAULT_BAUD_RATE;
	sprintf(minibuf, " -baud %d", baud_rate);
        strcat(args, minibuf);
    }
    
    if (strstr(args, "-max_pkt_len ") == (char *) NULL)
    {
        strcat(args, " -max_pkt_len ");
        strcat(args, DEFAULT_PKT_LEN);
    }

    /* 
     * Parallel download.
     *
     * Make sure to REMOVE the -parallel option (and its aliases) and arg 
     * from the args string, as they are unknown to hdi_init.
     */
    if ((tmp = strstr(args, "-parallel")) || 
                          (tmp = strstr(args, "-D")) ||
                                          (tmp = strstr(args, "-download")))
    {
        char *c = strchr(tmp, ' ');

        if (c)
        { 
            while ( *c && isspace(*c) ) 
                c++;
        }
	
        if (! c || 
                *c == '-' || 
#ifdef DOS
                        *c == '/' || 
#endif
                                matchParallelDevice(c) == NULL)
        {
            error("Expected device name after parallel selection switch.");
        }
        else
            comm_used[PARA_COMM_INDX] = 1;
        if ( c = strchr(c, ' ') )
        {
            /* Remove -parallel / -D option from command line */
            char *d = xmalloc(strlen(c) + 1);
            strcpy(d, c);
            strcpy(tmp, d);
            free(d);
        }
        else
        {
            /* -parallel <port> was last on the command line */

            *tmp = 0;
        }
    }

#ifdef DOS      /* PCI not functional for Unix (will core dump). */
    /*
     * Handle the myriad PCI options, once again being sure to remove
     * these switches from the command line since they are unknown to
     * HDI.
     */
    if (tmp = strstr(args, "-pcic")) 
    {
#define IO_LEN   (sizeof("io") - 1)
#define MMAP_LEN (sizeof("mmap") - 1)

        char *c = strchr(tmp, ' ');

        if (c)
        {
            while (*c && isspace(*c)) 
                ++c;
        }
        if (! c || 
                    ! *c ||
#ifdef DOS
                            *c == '/' || 
#endif
                                        *c == '-')
        {
            error("Expected value after -pcic");
        }
        if (strncmp(c, "io", IO_LEN) == 0 &&
                                    (! c[IO_LEN] || isspace(c[IO_LEN])))
        {
            fast_config_record.init_pci.comm_mode = COM_PCI_IOSPACE;
        }
        else if (strncmp(c, "mmap", MMAP_LEN) == 0 &&
                                    (! c[MMAP_LEN] || isspace(c[MMAP_LEN])))
        {
            fast_config_record.init_pci.comm_mode = COM_PCI_MMAP;
        }
        else
            fprintf_filtered(gdb_stderr, "invalid -pcic option\n");
        if (c = strchr(c, ' '))
        {
            /* Remove -pcic option from command line */

            char *d = xmalloc(strlen(c) + 1);
            strcpy(d, c);
            strcpy(tmp, d);
            free(d);
        }
        else
            *tmp = 0;  /* Last option on cmd line. */
#undef IO_LEN
#undef MMAP_LEN
    }
    if (tmp = strstr(args, "-pciv"))
    {
        unsigned long vendor_id, device_id;
        char          *c, *b, *bbegin, *bend, *prefix = "-pciv";

        if (c = strchr(tmp, ' '))
        {
            while (*c && isspace(*c)) 
                ++c;
        }
        if (! c || 
                    ! *c ||
#ifdef DOS
                            *c == '/' || 
#endif
                                        *c == '-')
        {
            error("Expected 2 values after -pciv");
        }
        if (b = strchr(c, ' '))
        {
            bbegin = b;
            while (*b && isspace(*b)) 
                ++b;
        }
        if (! b || 
                    ! *b || 
#ifdef DOS
                            *b == '/' || 
#endif
                                        *b == '-')
        {
            error("Expected 2 values after -pciv");
        }
        *bbegin = '\0';     /* Terminate first arg (vendor ID)  */
        if (bend = strchr(b, ' '))
        {
            /* 
             * Temporarily clip off trailing remainder of command line to
             * enable parser in hdi_convert_number() to properly parse the
             * PCI device_id. 
             */

            *bend = '\0';
        }
        if ((hdi_convert_number(c,
                                (long *) &vendor_id, 
                                HDI_CVT_UNSIGNED,
                                16,
                                prefix) == OK)
                      &&
            (hdi_convert_number(b,
                                (long *) &device_id, 
                                HDI_CVT_UNSIGNED,
                                16,
                                prefix) == OK))
        {
            fast_config_record.download_selector  = FAST_PCI_DOWNLOAD;
            fast_config_record.init_pci.bus       = COM_PCI_NO_BUS_ADDR;
            fast_config_record.init_pci.func      = COM_PCI_DFLT_FUNC;
            fast_config_record.init_pci.vendor_id = (int) vendor_id;
            fast_config_record.init_pci.device_id = (int) device_id;
            fast_config_record.fast_port          = PCI_UNUSED_FAST_PORT;
            fast_config                           = &fast_config_record;
            comm_used[PCI_COMM_INDX]             |= PCI_COMM_VND;
        }
        if (bend)
        {
            char *d;

            /* Remove -pciv option from command line */

            *bend = ' ';
            d     = xmalloc(strlen(bend) + 1);
            strcpy(d, bend);
            strcpy(tmp, d);
            free(d);
        }
        else
            *tmp = 0;  /* Last option on cmd line. */
    }
    if (tmp = strstr(args, "-pcib"))
    {
        unsigned long dev_no, bus_no, func_no;
        char          *prefix = "-pcib", *c, *b, *bbegin, *a, *abegin, *aend;

        if (c = strchr(tmp, ' '))
        {
            while (*c && isspace(*c)) 
                ++c;
        }
        if (! c || 
                    ! *c ||
#ifdef DOS
                            *c == '/' || 
#endif
                                        *c == '-')
        {
            error("Expected 3 values after -pcib");
        }
        if (b = strchr(c, ' '))
        {
            bbegin = b;
            while (*b && isspace(*b)) 
                ++b;
        }
        if (! b || 
                    ! *b ||
#ifdef DOS
                            *b == '/' || 
#endif
                                        *b == '-')
        {
            error("Expected 3 values after -pcib");
        }
        if (a = strchr(b, ' '))
        {
            abegin = a;
            while (*a && isspace(*a)) 
                ++a;
        }
        if (! a || 
                    ! *a ||
#ifdef DOS
                            *a == '/' || 
#endif
                                        *a == '-')
        {
            error("Expected 3 values after -pcib");
        }
        if (aend = strchr(a, ' '))
        {
            /* 
             * Temporarily clip off trailing remainder of command line to
             * enable parser in hdi_convert_number() to work properly.
             */

            *aend = '\0';
        }
        *abegin = *bbegin = '\0';
        if ((hdi_convert_number(c,
                                (long *) &bus_no, 
                                HDI_CVT_UNSIGNED,
                                16,
                                prefix) == OK)
                      &&
            (hdi_convert_number(b,
                                (long *) &dev_no, 
                                HDI_CVT_UNSIGNED,
                                16,
                                prefix) == OK)
                      &&
            (hdi_convert_number(a,
                                (long *) &func_no, 
                                HDI_CVT_UNSIGNED,
                                16,
                                prefix) == OK))
        {
            fast_config_record.download_selector  = FAST_PCI_DOWNLOAD;
            fast_config_record.init_pci.bus       = (int) bus_no;
            fast_config_record.init_pci.dev       = (int) dev_no;
            fast_config_record.init_pci.func      = (int) func_no;
            fast_config_record.init_pci.vendor_id = COM_PCI_DFLT_VENDOR;
            fast_config_record.fast_port          = PCI_UNUSED_FAST_PORT;
            fast_config                           = &fast_config_record;
            comm_used[PCI_COMM_INDX]             |= PCI_COMM_BUS;
        }
        if (aend)
        {
            char *d;

            /* Remove -pcib option from command line */

            *aend = ' ';
            d     = xmalloc(strlen(aend) + 1);
            strcpy(d, aend);
            strcpy(tmp, d);
            free(d);
        }
        else
            *tmp = 0;  /* Last option on cmd line. */
    }

    if (tmp = strstr(args, "-pcid"))
    {
        com_pci_debug(TRUE);
        if (tmp[(sizeof("-pcid") - 1)] != '\0')
        {
            /* Remove -pcid option from command line */

            char *c, *d;

            c = tmp + sizeof("-pcid") - 1;
            d = xmalloc(strlen(c) + 1);
            strcpy(d, c);
            strcpy(tmp, d);
            free(d);
        }
        else
            *tmp = 0;  /* Last option on cmd line. */
    }

    /*
     * NB:  -pci must be parsed _after_ all other options containing
     * "-pci" as a prefix (e.g., after -pcic).
     */
    if (tmp = strstr(args, "-pci"))
    {
        char *c;

        fast_config_record.download_selector  = FAST_PCI_DOWNLOAD;

        /* Establish some default PCI configuration values. */
        fast_config_record.init_pci.bus       = COM_PCI_NO_BUS_ADDR;
        fast_config_record.init_pci.func      = COM_PCI_DFLT_FUNC;
        fast_config_record.init_pci.vendor_id = COM_PCI_DFLT_VENDOR;
        fast_config_record.fast_port          = PCI_UNUSED_FAST_PORT;
        fast_config                           = &fast_config_record;
        comm_used[PCI_COMM_INDX]             |= PCI_COMM_DFLT;

        if (c = strchr(tmp, ' '))
        {
            /* Remove -pci option from command line */

            char *d = xmalloc(strlen(c) + 1);
            strcpy(d, c);
            strcpy(tmp, d);
            free(d);
        }
        else
            *tmp = 0;  /* Last option on cmd line. */
    }
    
    /* Did the user supply more comm options than can be processed? */
    if ((comm_used[PARA_COMM_INDX] && comm_used[PCI_COMM_INDX])  ||
         (comm_used[PCI_COMM_INDX] > PCI_COMM_MAX_VAL)           ||
         (comm_used[PCI_COMM_INDX] == PCI_COMM_MIN_VAL))
    {
        error(
"-pci, -pciv, -pcib, and -parallel are mutually exclusive--aborting connection"
             );
    }

#endif    /* DOS ---> filter out PCI option parsing */

    ap = args;

    /* get hdilcomm's argument list */
    opt_count = com_get_opt_list(&com_opt_list, &com_opt_handler);

    /* make one list, with gdb960's args first */

    opt_handler = (opt_handler_t *) xmalloc(sizeof(gdb_opt_handler) + (opt_count * sizeof(opt_handler_t)));
    memcpy(opt_handler, gdb_opt_handler, sizeof(gdb_opt_handler));
    memcpy(&opt_handler[NUM_GDB_OPTS], com_opt_handler, (opt_count * sizeof(opt_handler_t)));

    opt_list = (COM_INVOPT *) xmalloc(sizeof(gdb_com_opts) + (opt_count * sizeof(COM_INVOPT)));
    memcpy(opt_list, gdb_com_opts, sizeof(gdb_com_opts));
    memcpy(&opt_list[NUM_GDB_OPTS], com_opt_list, (opt_count * sizeof(COM_INVOPT)));

    opt_count += NUM_GDB_OPTS;

    /* OK. opt_count is the total number of args known about; opt_list
     * is the COM_INVOPT array, with gdb960's args listed first, and
     * opt_handler is the option handling routine.
     */

    /* set up defaults for reset_time, tint and mon_priority */
    set_reset_time(DEFAULT_RESET_TIME);
    set_tint(DEFAULT_TINT);
    set_mon_priority(DEFAULT_MON_PRIORITY);
    hdi_config.no_reset = DEFAULT_NO_RESET;

    /* set up initial break if -brk seen on command line */
    if ( target_initial_brk )
    {
	    hdi_config.intr_trgt = 1;
	    hdi_config.no_reset = 1;
    }

    while (ap && *ap)
    {
        char *arg = getnextarg(&ap);
        if (arg)
        {
            if (*arg == '-')
            {
                arg++;
                for (i = 0; i < opt_count; ++i)
                {
                    if (strcmp(arg, opt_list[i].name) == 0)
                    {
                       if (opt_list[i].needs_arg)
                           (opt_handler[i]) (getnextarg(&ap));
                       else
                           (opt_handler[i]) ((char *) NULL);
                       break; /* out of for loop */
                    }
                }
                if (i >= opt_count) /* no match found */
                   fprintf_filtered(gdb_stderr, "unknown switch -%s ignored\n", arg);
            } else {
                fprintf_filtered(gdb_stderr, "unknown argument %s ignored\n", arg);
            }
        }
    }

    /* OK - all arguments processed */

    /* if connected, close. See push_target below for minor hassle.
     * we need to close here BEFORE we do hdi_init or it
     * get unhappy.
     */

    if (HDI_CONNECTED) {
        mon960_close(0);
    }

    /*
     * NB: Must register comm protocol with HDI following the call to
     * mon960_close(), which calls hdi_term().  If we don't follow this
     * restriction, hdi_term() will erase comm protocol selection and
     * hdi_init() will later barf with an E_COMM_PROTOCOL error.
     */
    if (wants_pci_comm)
    {
        /* Register intentions with HDI. */

        com_select_pci_comm();
        com_pciopt_cfg(&fast_config->init_pci);
    }
    else
    {
    	/* User wants serial comm, let HDI know. */

        com_select_serial_comm();
        if (strlen(serial_dev) > MAXDEVCHARS)
            error_no_arg("Device name can not exceed 19 characters"); /* assumes MAXDEVCHARS = 19 */
    }

    strcpy(mon960_device, (wants_pci_comm) ? "pci" : serial_dev);

    /* fireup HDIL
     * args:
     *      mon960 structure passed IN: mon_priority, tint, reset_time, no_reset
     *      arch_type: RETURNED, i960 subarch type, KA, CA, etc.
     */
    sleep(DEFAULT_SLEEP_SECS);
    ctrlc_handler = signal (SIGINT, mon960_signal_handler);
#ifdef WIN95
    ctrlbrk_handler = signal (SIGBREAK, mon960_signal_handler);
#endif

    HDI_CALL(rv = hdi_init(&hdi_config, &arch_type));

    signal (SIGINT, ctrlc_handler);
#ifdef WIN95
    signal (SIGBREAK, ctrlbrk_handler);
#endif

    if (rv == OK) 
    {
        /* push_target will do a close if already open. Therefore
         * since we already did a close above, we order close
         * to ignore push_target.
         */
        abortClose = 1;
        push_target (&mon960_ops);
        abortClose = 0;

        if (wants_pci_comm)
        {
            com_pci_get_cfg(&fast_config->init_pci);
            comm_channel      = GDB_COMM_PCI;
            saved_fast_config = fast_config_record;
        }
        else
        	comm_channel = GDB_COMM_SERIAL;

        if (!quiet)
        {
            if (comm_channel == GDB_COMM_SERIAL)
            {
                printf_filtered("Connected to %s at %d bps.\n", 
                                mon960_device, baud_rate);
            }
            else
                dump_pci_addr("Connected");
        }

        /* Set target byte order, NUM_REGS, reg_names, etc. */
        init_i960_architecture(arch_type);

	/* Set G12 if needed for debugging PID code.  This will also 
	   get done each time the "run" command is given in case the 
	   user changes the pidoffset via the "file" command. */
	if (pidoffset != 0)
	    write_register(G12_REGNUM, pidoffset);
	
        /* Mon960-specific reset help */
        add_com ("reset", class_obscure, mon960_reset,
	"Resets the remote target system.\n\
	Completely re-initializes the target.  Deletes HW breakpoints.\n\
	Discards cache and register contents.\n\
	Performs a hardware reset if possible, otherwise does a cold start\n\
	from the monitor.");

        /* 
         * If PCI download was requested (as opposed to PCI comm), init as
         * necessary. 
         */
        if (comm_channel == GDB_COMM_SERIAL &&
                                        fast_config && 
                            fast_config->download_selector == FAST_PCI_DOWNLOAD)
        {
            com_get_pci_controlling_port(
                                    fast_config_record.init_pci.control_port
                                        );
        }
    }
    else 
    {     
        /* Failed to connect */

        comm_channel = GDB_COMM_NONE;

        /*
         * force hdil to close the device.  It is (was) possible for hdil
         * to return an error above but leave the device open so that
         * subsequent hdi_init() calls would fail due to opens in exclusive
         * use mode.  1 here is magic that means close the device, but
         * don't try and do a sw reset.
         */
        hdi_term(1);
        sleep(DEFAULT_SLEEP_SECS);
        mon960_device[0] = (char)NULL;
    }
    do_cleanups(cleanup_chain);
}

/* 
 * If we are here, gdb has successfully connected to mon960 via HDI.
 * Set up some global target parameters based on the particular flavor of
 * 80960 processor.
 *
 * If the monitor version is 2.1 or newer, you can get alot of info
 * in one HDI call.  Else use the given 80960 architecture to make some 
 * educated guesses.
 */
static void
init_i960_architecture(arch_type)
    int arch_type;
{
    HDI_MON_CONFIG mon_config;

#ifdef MON_CONFIG_HAS_SF_REGS	/* FIXME ! */
    /* Don't even bother with this chunk of code until it can do everything */
    /* Don't use HDI_CALL here cause we want to silently default to the
       older method if hdi_get_monitor_config not available. */
    if ( (rv = hdi_get_monitor_config(&mon_config)) == OK )
    {
	if ( ! bigendian_switch )
	    target_byte_order = 
		(mon_config.monitor & HDI_MONITOR_BENDIAN) ? 
		    BIG_ENDIAN : LITTLE_ENDIAN;
	mon960_ops.to_hardware_breakpoint_limit = 
	    mon_config.inst_brk_points;
	mon960_ops.to_hardware_watchpoint_limit = 
	    mon_config.data_brk_points;
	num_fp_regs = mon_config.fp_regs;
	num_sf_regs = mon_config.sf_regs;
    }
    else
#endif
    {
	/* Probably a pre-2.1 mon960 */
	if ( bigendian_switch )
	{
	    /* If -G was seen on command-line, force BIG_ENDIAN */
	    target_byte_order = BIG_ENDIAN;
	}
	else
	{
	    CORE_ADDR magic_address;	/* address containing -2 in the IBR */
	    char magic_number[4];	/* 0xfffffffe (bigendian) or
					   0xfeffffff (littleendian) */
	    switch ( arch_type )
	    {
	    case ARCH_KA:
	    case ARCH_KB:
		/* can only be little-endian */
		target_byte_order = LITTLE_ENDIAN;
		magic_address = 0;
		break;
	    case ARCH_CA:
		magic_address = 0xffffff18;
		break;
	    case ARCH_JX:
	    case ARCH_HX:
		magic_address = 0xfeffff48;
		break;
	    default:
		/* attn maintainers; add new architectures above as needed */
		/* Default to little-endian */
		target_byte_order = LITTLE_ENDIAN;
		magic_address = 0;
	    }
	    if (magic_address)
	    {
		/* Read a signed word; if it is -2, then target_byte_order is OK.
		   If not, try swapping the byte order of the word.  If that 
		   results in a -2, then target_byte_order is backwards.  If it
		   still doesn't look like a -2 to the host, then something else 
		   has been written over the IBR and we have more problems than 
		   just byte order.  In that case, bail and accept the default. */
		target_read_memory(magic_address, magic_number, 4);
		if ( extract_signed_integer(magic_number, 4) != -2 )
		{
		    /* swap bytes and try again */
		    byteswap_within_word(magic_number);
		    if ( extract_signed_integer(magic_number, 4) == -2 )
			/* target order is backwards */
			target_byte_order = TARGET_LITTLE_ENDIAN ? 
			    BIG_ENDIAN : LITTLE_ENDIAN;
		}
	    }
	}

	/* target byte order is now correct; set the appropriate 
	   software breakpoint instruction */
	if ( TARGET_BIG_ENDIAN )
	    set_break_insn(big_endian_break_insn);
	else
	    set_break_insn(little_endian_break_insn);
	
	/* Set some other architecture-dependent variables */
	switch (arch_type)
	{
	case ARCH_KA:
	    num_fp_regs = 0;
	    num_sf_regs = 0;
	    mon960_ops.to_hardware_breakpoint_limit = 2;
	    mon960_ops.to_hardware_watchpoint_limit = 0;
	    break;
	case ARCH_KB:
	    num_fp_regs = 4;
	    num_sf_regs = 0;
	    mon960_ops.to_hardware_breakpoint_limit = 2;
	    mon960_ops.to_hardware_watchpoint_limit = 0;
	    break;
	case ARCH_CA:
	    num_fp_regs = 0;
	    num_sf_regs = 3;
	    mon960_ops.to_hardware_breakpoint_limit = 2;
	    mon960_ops.to_hardware_watchpoint_limit = 2;
	    break;
	case ARCH_JX:
	    num_fp_regs = 0;
	    num_sf_regs = 0;
	    mon960_ops.to_hardware_breakpoint_limit = 2;
	    mon960_ops.to_hardware_watchpoint_limit = 2;
	    break;
	case ARCH_HX:
	    num_fp_regs = 0;
	    num_sf_regs = 5;
	    init_gmu_cmd_cache(); /* In i960-tdep.c */
	    mon960_ops.to_hardware_breakpoint_limit = 6;
	    mon960_ops.to_hardware_watchpoint_limit = 6;
	    break;
	default:
	    /* attn maintainers; add new architectures above as needed */
	    num_fp_regs = 0;
	    num_sf_regs = 0;
	    mon960_ops.to_hardware_breakpoint_limit = 0;
	    mon960_ops.to_hardware_watchpoint_limit = 0;
	    break;
	}
    }
    num_regs = 36 + num_fp_regs + num_sf_regs;
    init_reg_names();
    init_hdi_reg_map();
    registers_changed();
}

/* This is a function to give the target a chance to do some activities
   whenever the symfile changes.  Very handy for the Hx GMU.  Not used
   for anything else currently. */
void
mon960_clear_symtab_users()
{
    if (! HDI_CONNECTED)
	return;
    if (arch_type == ARCH_HX)
	reprogram_gmu();
}

int
mon960_arch()
{
    if (HDI_CONNECTED)
	return arch_type;
    else
	return -1;
}

int
mon960_connected()
{
    return HDI_CONNECTED;
}

static void
mon960_close (quitFlag)
    int quitFlag;
{
    if (abortClose)
        return;
    
    if ( saved_environ )
    {
        /* Restore GDB's original environment */
        environ = saved_environ;
    }

    if (HDI_CONNECTED) 
    {
        if (! quiet)
        {
            if (comm_channel == GDB_COMM_SERIAL)
            {
                printf_filtered("Terminating old session with %s\n",
                                 mon960_device);
                gdb_flush(gdb_stdout);
            }
            else if (comm_channel == GDB_COMM_PCI)
            {
                printf_filtered("Terminating old session at bus# %#x, dev# %#x, func# %#x\n",
                                 saved_fast_config.init_pci.bus,
                                 saved_fast_config.init_pci.dev,
                                 saved_fast_config.init_pci.func);
                gdb_flush(gdb_stdout);
            }
        }
        HDI_CALL(hdi_term(quitFlag));
        sleep(DEFAULT_SLEEP_SECS);
        mon960_device[0] = (char)NULL;
        if (runargs)
                free(runargs);
        runargs = 0;
        if (loadedfilename)
                free(loadedfilename);
        loadedfilename = 0;
        run_stop_record = 0;
    }
}

static void
print_out_stop_record()
{
    static struct {
	char *name;
	int value;
    } map_stop[] = 
    { 
    { "Mon960 was left running", STOP_RUNNING },
    { "Program exit", STOP_EXIT },
    { "Software breakpoint", STOP_BP_SW },
    { "Hardware breakpoint", STOP_BP_HW },
    { "Hardware watchpoint", STOP_BP_DATA0 },
    { "Hardware watchpoint", STOP_BP_DATA1 },
    { "Trace fault", STOP_TRACE },
    { "Unclaimed fault", STOP_FAULT },
    { "Unclaimed interrupt", STOP_INTR },
    { "Debugger interrupt", STOP_CTRLC },
    { "Internal error", STOP_UNK_SYS },
    { "", STOP_UNK_BP },
    { "Program called mon_entry", STOP_MON_ENTRY },
    { NULL, 0 } 
    };

    int i, matches, r = run_stop_record->reason;
    char buff[1024];

    for ( i = 0, buff[0] = 0, matches = 0; map_stop[i].name; i++ )
    {
	if ( (map_stop[i].value & r) && map_stop[i].name[0] ) 
	{
	    if ( matches > 0 )
		strcat(buff, ", ");
	    strcat(buff, map_stop[i].name);
	    ++matches;
	}
    }
    printf_filtered("Program stop reason: %s\n", buff);
    if (r & STOP_EXIT)
	printf_filtered("\tExit code: %d\n",
			run_stop_record->info.exit_code);
    if (r & STOP_BP_SW)
	printf_filtered("\tProgram stopped at 0x%x\n",
			run_stop_record->info.sw_bp_addr);
    if (r & STOP_BP_HW)
	printf_filtered("\tProgram stopped at 0x%x\n",
			run_stop_record->info.hw_bp_addr);
    if (r & STOP_BP_DATA0)
	printf_filtered("\tAddress of data 0 breakpoint: 0x%x\n",
			run_stop_record->info.da0_bp_addr);
    if (r & STOP_BP_DATA1)
	printf_filtered("\tAddress of data 1 breakpoint: 0x%x\n",
			run_stop_record->info.da1_bp_addr);
    if (r & STOP_TRACE)
	printf_filtered("\tTrace type 0x%x, address 0x%x\n",
			run_stop_record->info.trace.type,
			run_stop_record->info.trace.ip);
    if (r & STOP_FAULT)
    {
	print_out_fault_type(run_stop_record);
    }
    if (r & STOP_INTR)
	printf_filtered("\tInterrupt vector 0x%0x\n",
			run_stop_record->info.intr_vector);
}

/* For more information on the layout of the fault table,
   see i960-tdep.c:init_fault_name_table(). */

print_out_fault_type(sr)
    STOP_RECORD *sr;
{
    int type = run_stop_record->info.fault.type;
    int subtype;
    unsigned long mask;

    switch (type)
    {
    case 1:
    case 4:
    case 7:
	/* These are bitfields. Shift and mask to achieve 0 - 7 numbering. */
	mask = run_stop_record->info.fault.subtype;
	for (subtype = 0, mask &= 0xff; mask > 1; ++subtype, mask >>= 1)
	    ;
	break;
    case 0:
    case 2:
    case 3:
    case 5:
    case 6:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
	/* These are numbers, not bitfields.  They index directly 
	   into the table */
	subtype = run_stop_record->info.fault.subtype;
	break;
    default:
	/* Error case; print "unknown" */
	type = 0;
	subtype = 0;
	break;
    }
    printf_filtered("\tFault type: %s (type 0x%x, subtype 0x%x)\n",
		    fault_name_table[type][subtype] ? 
		    fault_name_table[type][subtype] : "Unknown",
		    run_stop_record->info.fault.type,
		    run_stop_record->info.fault.subtype);
    printf_filtered("\tip 0x%x, fault record @ 0x%x\n",
		    run_stop_record->info.fault.ip,
		    run_stop_record->info.fault.record);
}

static void
mon960_files_info (t)
struct target_ops *t;
{
    HDI_SANITY();
    if (HDI_CONNECTED) 
    {
	char buff[128];
	CPU_STATUS status;
	
	if (comm_channel == GDB_COMM_SERIAL)
		printf_filtered("Attached to %s at %d bps.\n",mon960_device,baud_rate);
	else
		dump_pci_addr("Attached");
	if (HDI_CALL(hdi_version(buff, sizeof(buff))) == OK)
	    printf_filtered("%s\n", buff);
	else
	    printf_filtered("Can't get HDI version\n");
	if (loadedfilename) 
	{
	    printf_filtered("The remote target has the executable file loaded.\n");
	    if (run_stop_record)
		print_out_stop_record();
	}
	else
	    printf_filtered("No executable file has been loaded to the remote.\n");
	if (HDI_CALL(hdi_cpu_stat(&status)) == OK)
	{
	    printf_filtered("Cpu status information:\n");
	    printf_filtered("\tPRCB address                   0x%x\n",status.cpu_prcb);
	    printf_filtered("\tSystem procedure table address 0x%x\n",status.cpu_sptb);
	    printf_filtered("\tFault table address            0x%x\n",status.cpu_ftb);
	    printf_filtered("\tInterrupt table address        0x%x\n",status.cpu_itb);
	    printf_filtered("\tBase of Interrupt stack        0x%x\n",status.cpu_isp);
	    if (arch_type == ARCH_KA || arch_type == ARCH_KB)
		printf_filtered("\tSAT/ST address                 0x%x\n",status.cpu_sat);
	    if (arch_type == ARCH_CA)
		printf_filtered("\tControl table address          0x%x\n",status.cpu_ctb);
	}
	
    }
}

/* Non-zero iff a download has been performed since the last time an
 * exec was done.
 */
static int mon960_file_loaded = 0;

/* Number of lines per page or UINT_MAX if paging is disabled.  */
extern unsigned int lines_per_page;

static void
mon960_load( filename, from_tty )
    char *filename;
    int from_tty;
{
    char 	    *scratch_pathname;
    int		    scratch;
    unsigned long   dummy;  /* to satisfy hdi_download */
    unsigned int    save_lines_per_page;
    void	    (*ctrlc_handler)();
#ifdef WIN95
    void	    (*ctrlbrk_handler)();
#endif
    
    HDI_SANITY();

    if (!filename)
	    filename = get_exec_file (1);

    filename = tilde_expand (filename);

    scratch = openp (getenv ("PATH"), 1, filename, O_RDONLY, 0,
		     &scratch_pathname);
    if (scratch < 0) {
	perror_with_name (filename);
	return;
    }
    close (scratch);		/* Slightly wasteful FIXME */

    mark_breakpoints_out();
    inferior_pid = 0;
    if (loadedfilename)
	    free(loadedfilename);
    loadedfilename = NULL;
    run_stop_record = 0;
    if (runargs)
	    free(runargs);
    runargs = 0;
    registers_changed();	/* invalidate */

    ctrlc_handler = signal (SIGINT, mon960_signal_handler);
#ifdef WIN95
    ctrlbrk_handler = signal (SIGBREAK, mon960_signal_handler);
#endif
    /* Disable GDB's paging mechanism to make sure that the download
       proceeds to the end without interruption. */
    save_lines_per_page = lines_per_page;
    lines_per_page = UINT_MAX;

    HDI_CALL( scratch = hdi_download(scratch_pathname, &dummy,
				     picoffset, pidoffset,
				     0, fast_config, quiet) );
    signal (SIGINT, ctrlc_handler);
#ifdef WIN95
    signal (SIGBREAK, ctrlbrk_handler);
#endif

    /* Restore user's paging preference. */
    lines_per_page = save_lines_per_page;

    if (scratch == OK) 
    {
	loadedfilename = SAVESTRING(filename);
	mon960_file_loaded = 1;
    }
    else
    {
        /* Special case error-checking for fast download */
        if (fast_config && hdi_cmd_stat == E_ARG)
        {
            if (fast_config->download_selector == FAST_PARALLEL_DOWNLOAD)
            {
                fprintf_filtered(gdb_stderr, 
                                 "Invalid parallel device: %s.\n", 
                                 fast_config->fast_port);
            }
            else
                fprintf_filtered(gdb_stderr, "Invalid pci device.\n");
            fprintf_filtered(gdb_stderr, "Can not download %s.\n",filename);
        }
    }
}

static void
mon960_detach (args, from_tty)
     char *args;
     int from_tty;
{
    HDI_SANITY();

    /*  dont_repeat (); */

    mon960_close((args && strstr(args, "-n")));

    pop_target ();
}

volatile static int target_interrupted;

#if defined(DOS) && defined(__HIGHC__)
/* We normally have pragma On(Check_stack) for runtime stack overflow checking.
   But with Metaware HIGH-C, you can't BOTH check for stack overflow AND also
   handle a hardware-caused signal.  So we'll temporarily turn off stack
   checking while defining signal handlers. */
#pragma Off(Check_stack)
#endif

static void
mon960_signal_handler(sig)
int sig;
{
    signal(sig, mon960_signal_handler);
    target_interrupted = 1;
    hdi_signal();
}

#if defined(DOS) && defined(__HIGHC__)
#pragma Pop(Check_stack)	/* Restore previous value of Check_stack */
#endif  /* Workaround for HIGHC signal-handler/Check_stack bug. */

static void
mon960_resume(pid, step, siggnal)
    int pid, step;
    enum target_signal siggnal;
{
    void (*ctrlc_handler)();
#ifdef WIN95
    void (*ctrlbrk_handler)();
#endif
    int mode = step ? GO_STEP : GO_RUN;

    HDI_SANITY();

    if (siggnal != TARGET_SIGNAL_0 && siggnal != TARGET_SIGNAL_STOP)
	    error ("Can't send signals to remote MON960 targets.");

    registers_changed();  /* invalidate regs */

    ctrlc_handler = signal (SIGINT, mon960_signal_handler);
#ifdef WIN95
    ctrlbrk_handler = signal (SIGBREAK, mon960_signal_handler);
#endif

    target_interrupted = 0;

    /* if run_stop_record->reason == STOP_EXIT, the program has already
     * terminated and the user has issued a "continue" command. don't
     * actually execute the target or mon960 will get a software fault.
     * run_stop_record is set to 0 when a run command is executed.
     */
    if (!run_stop_record || run_stop_record->reason != STOP_EXIT)
    {
	run_stop_record = hdi_targ_go(mode);
    }

    signal (SIGINT, ctrlc_handler);
#ifdef WIN95
    signal (SIGBREAK, ctrlbrk_handler);
#endif

    if (run_stop_record == NULL)
	    HDI_CALL(ERR);
}

static void
mon960_create_inferior (execfile, args, env)
    char *execfile;
    char *args;
    char **env;
{
    int pid;
    static char confirm_load[] = "No download since last 'exec'.  Run anyway? ";

    if (execfile == 0 || exec_bfd == 0)
        error ("No executable file specified.  Use \"file\" or \"exec-file\".");

    HDI_SANITY();

    if ( ! mon960_file_loaded && ! batch )
	    if ( query(confirm_load) )
		    mon960_file_loaded = 1;
	    else 
		    error("Not confirmed.");
    
    runargs = (args && *args ) ? SAVESTRING(args) : 0;

/* The "process" (board) is already stopped awaiting our commands, and
   the program is already downloaded.  We just set its PC and go.  */

  inferior_pid = pid = 42;	/* Needed by wait_for_inferior below */

  clear_proceed_status ();

  /* Tell wait_for_inferior that we've started a new process.  */
  init_wait_for_inferior ();

  /* Set up the "saved terminal modes" of the inferior
     based on what modes we are starting it with.  */
  target_terminal_init ();

  /* Install inferior's terminal modes.  */
  target_terminal_inferior ();

    /* Trick HDI into thinking that gdb's homemade environment is the 
       host system's environment.  Make sure to restore this in
       mon960_close. */
    if ( saved_environ == NULL )
	saved_environ = environ;
    environ = env;
       
  /* 
   * Set fp and sp to known, valid initial values.
   * This will allow us to run multiple apps of various sizes without
   * incurring bogus stack faults due to funky stack init code in
   * crt960.s .
   */
  hdi_init_app_stack();

  /* If position-independent data, we must initialize G12 before running. */
  if ( pidoffset != 0 )
	  write_register(G12_REGNUM, pidoffset);

  hdi_restart();
  run_stop_record = (STOP_RECORD *)0;

  mon960_ops.to_has_execution = 1;
  mon960_ops.to_has_stack = 1;

  /* i960_startp is set in exec_file_command, then checked against the 
     downloader's version in mon960_load */
  proceed ((CORE_ADDR)i960_startp, TARGET_SIGNAL_0, 0);
}

/* The registers[] array is assumed to contain target-endian data.
   This routine assures this is obeyed. */

static void swap_regvals(regvals)
    UREG regvals;
{
    int i;

    for (i=0;i < NUM_REGS;i++)
        byteswap_within_word(&regvals[i]);
}

enum regno_read_or_write {regno_read, regno_write};

static void 
mon960_convert_to_virtual(regnum,from,to)
    int regnum;
    char *from;
    char *to;
{
    if (regnum >= FP0_REGNUM)
	    extended_to_double(from,(double*)to);
    else
	    bcopy(from,to,4);
}

static void 
mon960_convert_to_raw(regnum,from,to)
    int regnum;
    char *from;
    char *to;
{
    if (regnum >= FP0_REGNUM)
	    double_to_extended((double *)from,to);
    else
	    bcopy(from,to,4);
}

static void 
mon960_registers(regno,action)
    int regno;
    enum regno_read_or_write action;
{
    enum regno_enum w;

    HDI_SANITY();
    w = which_regno(regno);
    if (w == all_regs) 
    {
	UREG regvals;

	if (action == regno_read) 
	{
	    /* regvals are in HOST byte order */
	    HDI_CALL(hdi_regs_get(regvals));

	    if (HOST_BYTE_ORDER != TARGET_BYTE_ORDER)
		swap_regvals(regvals);
	
	    memcpy(&registers[REGISTER_BYTE (R0_REGNUM)],  
		   &regvals[REG_R0],
		   16*4);	/* R0 - R15 */
	    memcpy(&registers[REGISTER_BYTE (G0_REGNUM)],  
		   &regvals[REG_G0],
		   16*4);	/* G0 - G15 */
	    memcpy(&registers[REGISTER_BYTE (PCW_REGNUM)], 
		   &regvals[REG_PC], 
		   2*4);	/* PC, AC */
	    memcpy(&registers[REGISTER_BYTE (TCW_REGNUM)], 
		   &regvals[REG_TC], 
		   4);		/* TC */
	    memcpy(&registers[REGISTER_BYTE (IP_REGNUM)], 
		   &regvals[REG_IP], 
		   4);		/* IP */
	    memcpy(&registers[REGISTER_BYTE (SF0_REGNUM)],
		   &regvals[REG_SF0], 
		   num_sf_regs*4); /* SF0 - SFn */
	}
	else 
	{
	    /* regno_write */
	    memcpy(&regvals[REG_R0], 
		   &registers[REGISTER_BYTE (R0_REGNUM)],
		   16*4);	/* R0 - R15 */
	    memcpy(&regvals[REG_G0],
		   &registers[REGISTER_BYTE (G0_REGNUM)],  
		   16*4);	/* G0 - G15 */
	    memcpy(&regvals[REG_PC], 
		   &registers[REGISTER_BYTE (PCW_REGNUM)], 
		   2*4);	/* PC, AC */
	    memcpy(&regvals[REG_TC], 
		   &registers[REGISTER_BYTE (TCW_REGNUM)], 
		   4);		/* TC */
	    memcpy(&regvals[REG_IP], 
		   &registers[REGISTER_BYTE (IP_REGNUM)], 
		   4);		/* IP */
	    memcpy(&regvals[REG_SF0], 
		   &registers[REGISTER_BYTE (SF0_REGNUM)],
		   num_sf_regs*4); /* SF0 - SFn */

	    if (HOST_BYTE_ORDER != TARGET_BYTE_ORDER)
		swap_regvals(regvals);

	    HDI_CALL(hdi_regs_put(regvals));
	}

	/* Still doing all_regs ... */
	if (num_fp_regs > 0)
	{
	    int i;

	    for (i=0; i <= 3; i++) 
	    {
		FPREG mon960fpreg;

		if (action == regno_read) 
		{
		    HDI_CALL(hdi_regfp_get(i, FP_80BIT, (FPREG *)&mon960fpreg));
		    memcpy(&registers[REGISTER_BYTE (FP0_REGNUM+i)], 
			   mon960fpreg.fp80,
			   REGISTER_RAW_SIZE(FP0_REGNUM+i));
		}
		else
		{
		    /* regno_write */
		    memcpy(mon960fpreg.fp80,
			   &registers[REGISTER_BYTE (FP0_REGNUM+i)], 
			   REGISTER_RAW_SIZE(FP0_REGNUM+i));
		    HDI_CALL(hdi_regfp_put(i, FP_80BIT, &mon960fpreg));
		}
	    }
	}
	registers_fetched();
    }
    else if (w == thirty_two_bit_regs) 
    {
	REG reg;

	if (action == regno_read) 
	{
	    /* reg is in HOST byte order */
	    HDI_CALL(hdi_reg_get(regno_to_hdi_regname_map[regno],&reg));
	    /* change to target byte order */
	    SWAP_TARGET_AND_HOST(&reg, sizeof(REG));
	    memcpy(&registers[REGISTER_BYTE(regno)], &reg, sizeof(REG));
	    register_valid[regno] = 1;
	}
	else 
	{
	    /* reg is in target byte order */
	    memcpy(&reg, &registers[REGISTER_BYTE(regno)], sizeof(REG));
	    /* change to host byte order */
	    SWAP_TARGET_AND_HOST(&reg, sizeof(REG));
	    HDI_CALL(hdi_reg_put(regno_to_hdi_regname_map[regno], reg));
	    register_valid[regno] = 1;
	}
    }
    else if (w == eighty_bit_regs) 
    {
	if (num_fp_regs > 0)
	{
	    FPREG mon960fpreg;
	    int i = regno - FP0_REGNUM;

	    if (action == regno_read) 
	    {
		HDI_CALL(hdi_regfp_get(i, FP_80BIT, &mon960fpreg));
		memcpy(&registers[REGISTER_BYTE (regno)], 
		       mon960fpreg.fp80,
		       REGISTER_RAW_SIZE(regno));
		register_valid[regno] = 1;
	    }
	    else
	    {
		/* regno_write */
		memcpy(mon960fpreg.fp80,
		       &registers[REGISTER_BYTE (regno)],
		       REGISTER_RAW_SIZE(regno));
		HDI_CALL(hdi_regfp_put(i, FP_80BIT, &mon960fpreg));
		register_valid[regno] = 1;
	    }
	}
	else 
	{
	    warning("Attempted to read/write a non-existent floating point register");
	}
    }
    else 
    {
	warning("Attempted to read/write a non-existent register %d", regno);
    }
}

static void 
mon960_fetch_registers(regno)
    int regno;
{
    mon960_registers(regno,regno_read);
}

static void 
mon960_store_registers(regno)
     int regno;
{
    mon960_registers(regno,regno_write);
}


static void
mon960_prepare_to_store()
{
    return;
}


static int
mon960_xfer_memory(memaddr, myaddr, len, write, t)
     CORE_ADDR memaddr;
     char *myaddr;
     int len;
     int write;
     struct target_ops *t;
{
    ADDR address = memaddr;
    int rv;
    extern int caching_enabled;

    HDI_SANITY_WITH_RTN(-1);

    if ( (arch_type == ARCH_JX || arch_type == ARCH_HX) && (memaddr & 0xff000000) == 0xff000000 )
    {
	/* The memory-mapped registers on the i960 J-series and H-series
	   processors can ONLY be accessed 4 bytes at a time.  Any other
	   type of access causes a fault.
	   Note on byte order: when doing multi-byte accesses, HDI/mon960
	   does its own byte swapping.  Normally we send bytes down
	   to the target with mem_size parameter = 0 to tell mon960 NOT to
	   byte-swap.  But we MUST use mem_size = 4 because of the alignment
	   constraint above.  So we do some unnecessary swapping (sigh) to
	   keep the base gdb/target interface consistent. */
	char dummy_buf[4];
	if ( len != 4 )
	    error("Invalid access to memory-mapped register");

	if (write)
	{
	    memcpy(dummy_buf, myaddr, 4);
	    if ( HOST_BYTE_ORDER != TARGET_BYTE_ORDER )
		byteswap_within_word(dummy_buf);
	    HDI_CALL(rv = hdi_mem_write(address, dummy_buf, 4, 0, 1, 4));
	}
	else
	{
	    HDI_CALL(rv = hdi_mem_read(address, dummy_buf, 4, 1, 4));
	    if ( HOST_BYTE_ORDER != TARGET_BYTE_ORDER )
		byteswap_within_word(dummy_buf);
	    memcpy(myaddr, dummy_buf, 4);
	}
    }
    else
    {
	/* Normal reads and writes */
	if (write)
	    HDI_CALL(rv = hdi_mem_write(address, myaddr, len, 0, !caching_enabled, 0));
	else
	    HDI_CALL(rv = hdi_mem_read(address, myaddr, len, !caching_enabled, 0));
    }
    return rv == OK ? len : 0;
}

static void
mon960_kill()
{
    /* Don't actually kill the target; this would force us to
       reconnect on the next "run" command.  But set inferior_pid to 0
       to trick infrun stuff into thinking we're dead. */
    inferior_pid = 0;
    mon960_ops.to_has_execution = 0;
}

static void
mon960_mourn_inferior ()
{
    remove_breakpoints ();
    if ( saved_environ )
	/* Restore GDB's original environment */
	environ = saved_environ;
    generic_mourn_inferior ();	/* Do all the proper things now */
}

static int
mon960_wait( pid, status )
    int pid;
    struct target_waitstatus *status;
{
    HDI_SANITY_WITH_RTN(-1);

    if (run_stop_record == 0) {
	fprintf_filtered(gdb_stderr,
			 "run_stop_record is null in mon960_wait\n");
	gdb_flush(gdb_stderr);

	status->kind = TARGET_WAITKIND_SIGNALLED;
	status->value.sig = TARGET_SIGNAL_UNKNOWN;
	mon960_ops.to_has_execution = 0;
	mon960_ops.to_has_stack = 0;
	return -1;
    }

    mon960_data_bp_address = 0;
    if (run_stop_record->reason & STOP_EXIT)
    {
	status->kind = TARGET_WAITKIND_EXITED;
	status->value.integer = run_stop_record->info.exit_code;
	mon960_ops.to_has_execution = 0;
	mon960_ops.to_has_stack = 0;
    }
    else if (run_stop_record->reason & STOP_BP_SW 
	     || run_stop_record->reason & STOP_BP_HW
	     || run_stop_record->reason & STOP_TRACE
	     || run_stop_record->reason & STOP_BP_DATA0
	     || run_stop_record->reason & STOP_BP_DATA1) 
    {
	status->kind = TARGET_WAITKIND_STOPPED;
	status->value.sig = TARGET_SIGNAL_TRAP;
        if (run_stop_record->reason & STOP_BP_SW)
	    write_register(IP_REGNUM, run_stop_record->info.sw_bp_addr);
        else if (run_stop_record->reason & STOP_BP_HW)
	    write_register(IP_REGNUM, run_stop_record->info.hw_bp_addr);
	else if (run_stop_record->reason & STOP_BP_DATA0)
	    mon960_data_bp_address = run_stop_record->info.da0_bp_addr;
	else if (run_stop_record->reason & STOP_BP_DATA1)
	    mon960_data_bp_address = run_stop_record->info.da1_bp_addr;
    }
    else if (run_stop_record->reason & STOP_FAULT
	     || run_stop_record->reason & STOP_CTRLC)
    {
	/* This is either a fault that the application did not handle,
	   or it was a user interrupt.  In either case, print out the 
	   stop record, then treat it like a TRAP from GDB's point of view.
	   This makes it easier to continue from the stop and makes GDB's
	   user feedback more intuitive. */
	print_out_stop_record();
	status->kind = TARGET_WAITKIND_STOPPED;
	status->value.sig = TARGET_SIGNAL_TRAP;
    }
    else 
    {
	fprintf_filtered(gdb_stderr, "Target halted for unexpected reason\n");
	gdb_flush(gdb_stderr);
	print_out_stop_record();
	status->kind = TARGET_WAITKIND_STOPPED;
	status->value.sig = TARGET_SIGNAL_UNKNOWN;
    }
    return inferior_pid;
}

static int
mon960_insert_breakpoint(addr,prev_val,hw)
    CORE_ADDR addr;
    char *prev_val;
    int hw;	/* Non-zero == Hardware breakpoint */
{
    int rv;
    mon960_xfer_memory(addr, prev_val, 4, 0, NULL);	/* read current val */
    rv=hdi_bp_set(addr,hw ? BRK_HW : BRK_SW,0);
    if ( rv != OK )
	switch ( hdi_cmd_stat )
	{
	case E_BPUNAVAIL:
	    warning("only %d HW breakpoints can be enabled at a time.",
		    mon960_ops.to_hardware_breakpoint_limit);
	    break;
	case E_ARCH:
	    warning("this architecture does not support HW breakpoints.");
	    break;
	case E_BPSET:
	    if ( addressprint )
		warning("hw breakpoint already set at address 0x%x.", addr);
	    else
		warning("2 or more hw breakpoints set at same address");
	    break;
	default:
	    HDI_CALL(rv);
	}
    return rv == OK ? 0 : -1;
}

static int
mon960_remove_breakpoint(addr,prev_val)
    CORE_ADDR addr;
    char *prev_val;
{
    return HDI_CALL(hdi_bp_del(addr)) == OK ? 0 : -1;
}

/* Interface required by gdb 4.13 HW breakpoint system */
int
mon960_insert_hw_breakpoint(addr, prev_val)
    CORE_ADDR addr;
    char *prev_val;
{
    return mon960_insert_breakpoint(addr, prev_val, 1);
}

int
mon960_remove_hw_breakpoint(addr, prev_val)
    CORE_ADDR addr;
    char *prev_val;
{
    return mon960_remove_breakpoint(addr, prev_val);
}

/* HARDWARE WATCHPOINTS:
   With GDB 4.13, the base code supports HW breakpoints and watchpoints.
   The semantics of watchpoint are slightly different than mon960's 
   notion of a "data breakpoint".  

    	semantics    	mon960	    	    	gdb 4.13
	write only  	store	    	    	watchpoint
	read or write	access	    	    	access
	read only   	n/a (not implemented)	read

   The target must provide a macro:
   TARGET_CAN_USE_HARDWARE_WATCHPOINT(type, count, other)
   The same macro is used by both the HW breakpoint and HW watchpoint system.
	type is enum bptype: hw breakpoint, hw watchpoint, hw access watch ...
	count is the total number of HW *point registers desired by GDB
	other is some Sparclite deal that doesn't apply to us.
   macro should return:
    	0 no HW facility
	> 0 OK
	< 0 Number of HW breakpoints (watchpoints) exceeded
   The macro DOES NOT have to check whether the allowable number of bp's is
   currently being used; it just checks the given count against the limit.
   The function below is our implementation of the macro
*/
int
mon960_check_hardware_resources(type, count)
    enum bptype type;
    int count;
{
    int retval;
    if ( current_target != &mon960_ops )
	return 0;
    switch ( type )
    {
    case bp_hardware_breakpoint:
	retval = (count > mon960_ops.to_hardware_breakpoint_limit
		  || count < 0) ? -1 : 1;
	break;
    case bp_hardware_watchpoint:
	/* what we call a write-only watchpoint */
	/* intentional fallthrough */
    case bp_read_watchpoint:
    case bp_access_watchpoint:
	/* what we call a read/write watchpoint */
	if (mon960_ops.to_hardware_watchpoint_limit > 0)
	    retval = (count > mon960_ops.to_hardware_watchpoint_limit 
		      || count < 0) ? -1 : 1;
	else
	    retval = 0;
	break;
    default:
	retval = 0;
	break;
    }
    return retval;
}

/* Return -1 on error, 0 otherwise */
int
mon960_insert_watchpoint(addr, len, type)
    CORE_ADDR addr;
    int len;
    int type;
{
    int write_only_flag;
    int hdi_rv;
    int rv;

    if ( len > 4 )
	error("Internal error: mon960 supports only 4-byte HW watchpoints");

    switch(type)
    {
	/* The convention for "type" is established in insert_breakpoints. */
    case 0:
	/* bp_hardware_watchpoint: what we call a write-only watchpoint */
	write_only_flag = DBP_S;
	break;
    case 1:
	/* bp_read_watchpoint: we have no equivalent; lump in with access */
	/* intential fallthrough */
    case 2:
	/* bp_access_watchpoint: what we call a read/write watchpoint */
	write_only_flag = DBP_ANY;
	break;
    default:
	/* Should never happen */
	error("Internal error: mon960 does not support this type of HW watchpoint: %d", (int) type);
    }
    
    hdi_rv = hdi_bp_set(addr, BRK_DATA, write_only_flag);

    if ( hdi_rv == OK )
    {
	rv = 0;
    }
    else
    {
	rv = -1;
	switch ( hdi_cmd_stat )
	{
	case E_BPUNAVAIL:
	    warning("only %d HW watchpoints can be enabled at a time.",
		    mon960_ops.to_hardware_watchpoint_limit);
	    break;
	case E_ARCH:
	    /* Should never happen */
	    warning("this architecture does not support HW watchpoints.");
	    break;
	case E_BPSET:
	    /* HW watchpoint already at this address.  Silently ignore. */
	    rv = 0;
	    break;
	default:
	    HDI_CALL(hdi_rv);
	}
    }
    return rv;
}

/* For efficiency of the calling code, this may get called with len > 4,
   or with an addr that does not contain a bp.  Just remove the ones that
   are valid and ignore the rest silently.
   Expected return value: -1 on error, 0 otherwise */
int
mon960_remove_watchpoint(addr, len, type)
    CORE_ADDR addr;
    int len; /* Ignored */
    int type; /* Ignored */
{
    int rv, hdi_rv;

    hdi_rv = hdi_bp_del(addr);

    return 0;
}

CORE_ADDR
mon960_stopped_data_address()
{
    return mon960_data_bp_address;
}

/* 
 * Called from frame_chain_valid() in i960-tdep.c when mon960 
 * is the current target.
 *
 * The lowest frame that is at all interesting belongs to start.
 * The frame just below it is not interesting; in fact, it contains noise.
 * The frame below start's frame has PFP == 0.
 */
int
mon960_frame_chain_valid (chain, curframe)
    unsigned int chain;
    FRAME curframe;
{
    return chain != 0;
}

/* The next function handles the unusual case of disassembling after the 
   target is connected, but before code is downloaded.  The point is,
   only read mon960 target memory if code is downloaded.  Otherwise, read
   it from the exec file. */
extern struct target_ops exec_ops;

int
mon960_dis_asm_read_memory (memaddr, myaddr, len, info)
    CORE_ADDR memaddr;
    char *myaddr;
    int len;
    disassemble_info *info;
{
    if ( mon960_file_loaded )
	return target_read_memory (memaddr, (char *) myaddr, len);
    else
    {
	int res = (*exec_ops.to_xfer_memory) (memaddr, (char *) myaddr, len, 0, &exec_ops);
	return ! (res == len);
    }
}


/* Initialize a stub for "target nindy" so that a helpful message
   will get printed. */
static void
nindy_open(args, from_tty)
    char *args;
    int from_tty;
{
    error("Target nindy is no longer supported.  Use gdb960 release 4.6 or earlier.");
}

extern struct cmd_list_element *targetlist;

void
_initialize_mon960 ()
{
	add_target (&mon960_ops);

	add_cmd ("nindy", no_class, nindy_open, 
"Target nindy is no longer supported.  Use gdb960 release 4.6 or earlier.",
		 &targetlist);

}


