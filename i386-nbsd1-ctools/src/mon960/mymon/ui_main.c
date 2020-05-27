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

#define _UI_DEFINES
#include "ui.h"
/* ui.h includes common.h needed by i960.h */
#include "i960.h"
#include "retarget.h"
#include "hdi_stop.h"
#include "hdi_errs.h"
#include "hdi_arch.h"

extern int break_vector;
extern const char base_version[];
extern const char build_date[];
extern char *step_string;

extern int load_mem(ADDR, int mem_size, void *buf, int buf_size);
extern void set_mon_priority(unsigned int priority);
extern unsigned int get_mon_priority();
extern int set_proc_mcon(unsigned int, unsigned long);

extern prtf(), strlen(), strcpy(), readline(), get_regnum(), stoh();
extern ci(), get_words();
extern atod(), strncmp(), perror(), exit_mon(), prepare_go_user(), atoh();

int ui_cmd(char *cmd, int terminal);

void breakpt(), dasm(), databreak(), delete(), disp_float();
void display(), display_char(), display_regs(), download();
void fill(), he_cmd(), modify(), modify_d(), reset(), trace();

static void disp_prcb(); 
static void mcon_cmd(), lmadr_cmd(), lmmr_cmd(); 
static void pci_pc(),pci_test(), ef_cmd(), go(), help(), post_test(), banner(), cf_cmd();
static void disp_82c54(), cio_disp(), stop_message(const STOP_RECORD *stop_reason);

#define HELP
#ifdef HELP
static const char dot_help[] = ".\n  Repeat previous command.\n";

static const char bd_help[] =
	"bd [<addr>]\n"
    "  Set data breakpoint at addr or list all data breakpoints.\n";

static const char br_help[] =
	"br[eak] <addr>\n"
    "  Set instruction breakpoint at addr or list all instruction breakpoints.\n";

static const char cf_help[] =
	"cf\n"
    "  Check if EEPROM is blank.  If not blank, prints the EEPROM size.\n";

static const char da_help[] =
	"da[sm] [<addr>][#<words>]\n"
    "  Disassemble an instruction beginning at addr or the current IP.\n";

static const char db_help[] =
	"db <addr>[#<bytes>]\n"
    "  Display one or more bytes in hex and ACSII.\n";

static const char dc_help[] =
	"dc <addr>[#<bytes>]\n"
    "  Display one or more bytes in ACSII (.=non-ASCII).\n";

static const char dd_help[] =
	"dd <addr>[#<times>]\n"
    "  Display one or more sets of double words.\n";

static const char de_help[] =
	"de[lete] <addr>\n"
    "  Delete the specified breakpoint.\n";

static const char di_help[] =
	"di[splay] <register> or <addr>[#<words>]\n"
    "  Display any register, or one or more words.\n";

static const char do_help[] =
	"do[ownload] [offset]\n"
    "  Serial download using Xmodem. Add offset to each addr in this COFF file.\n";

static const char dq_help[] =
	"dq addr[#times]\n"
    "  Display one or more sets of quad words.\n";

static const char ds_help[] = 
	"ds <addr>[#<shorts>]\n"
    "  Display one or more short words.\n";

static const char dt_help[] =
	"dt <addr>[#<times>]\n"
    "  Display one or more sets of triple words.\n";

static const char ef_help[] = "ef\n  Erase EEPROM.\n";

static const char fi_help[] =
	"fi[ll] <addr1>#<addr2>#<value>\n"
    "  Fill memory from addr1 to addr2 inclusive with word(s) of value.\n";

static const char fl_help[] =
	"fl[oat] <addr>[#<times>]\n"
    "  Display one or more long real (64 bit) numbers.\n";

static const char fr_help[] =
	"fr <addr>[#<times>]\n"
    "  Display one or more real (32 bit) numbers.\n";

static const char fx_help[] =
	"fx <addr>[#<times>]\n"
    "  Display one or more extended real (80 bit) numbers.\n";

static const char go_help[] =
	"go [<addr>]\n"
    "  Begins execution of program at addr given or\n"
    "  the current IP (eg download IP or last breakpoint IP).\n";

static const char he_help[] =
	"he[lp] [<command>] or ? [<command>]\n"
    "  Gives help for 'command' or list all available commands.\n";

static const char la_help[] =
	"la <regnum> <value>\n"
    "  Write <value> to logical memory address register <regnum>.\n";

static const char lm_help[] =
	"lm <regnum> <value>\n"
    "  Write <value> to logical memory mask register <regnum>.\n";

static const char mb_help[] =
	"mb <addr>\n"
    "  Modify a byte in memory.\n";

static const char mc_help[] =
	"mc <region> <value>\n"
    "  Write <value> to <region>'s memory control register.\n";

static const char md_help[] = 
    "md[dify] <register>#<hex-value> or <addr>#<hex-value>\n"
    "  Modify a register (g1,r4 [not floating point regs]) or a word in memory.\n"
    "  USE care modifying fp, pfp, sp, pc, ac, or tc.\n";

static const char mo_help[] =
	"mo[dify] <register> or <addr>[#<words>]\n"
    "  Modify a register (g1,r4 [not floating point regs]) or words in memory.\n"
    "  USE care modifying fp, pfp, sp, pc, ac, or tc.\n";

static const char ps_help[] =
	"ps[xt] [<addr>]\n"
    "  Single step through program.  Step over and break on return from\n"
    "  call, callx, calls, bal, or balx instructions.\n";

static const char qu_help[] =
	"qu[it] or rb\n"
    "  Resets the board and autobauds, target becomes ready to reconnect.\n";

static const char re_help[] =
	"re[gisters]\n"
    "  Dump contents of all registers.\n";

static const char rs_help[] =
	"rs\n"
    "  Resets the board but keeps the current connection.\n";

static const char st_help[] =
	"st[ep] [<addr>]\n"
    "  Single step through program.\n";

static const char tr_help[] =
	"tr[ace] [br|ca|re|su [on|off]]\n"
    "  Turn on or off one of the following trace options: br[anch],\n"
    "  ca[ll], re[turn] or su[pervisor].  If on/off is not given, the\n"
    "  status of that option is displayed.\n";

static const char ve_help[] = "ve[rsion]\n  Print the version header.\n";

static const char po_help[] = "po\n  Call post test routine.\n";
static const char pc_help[] = "pc\n  Call PCI test routine.\n";

static const char all_help[] =
	"See Chapter 3 in the MON960 User's Guide, Available comands are:\n"
    "  bd br cf da db dc dd de di do dq ds dt ef fi fl fr fx\n"
	"  go he la lm mb mc md mo pc po ps qu re rs rb st tr ve ?.\n";
#else

#define dot_help 0
#define bd_help 0
#define br_help 0
#define cf_help 0
#define da_help 0
#define db_help 0
#define dc_help 0
#define dd_help 0
#define de_help 0
#define di_help 0
#define do_help 0
#define dq_help 0
#define ds_help 0
#define dt_help 0
#define ef_help 0
#define fi_help 0
#define fl_help 0
#define fr_help 0
#define fx_help 0
#define go_help 0
#define he_help 0
#define la_help 0
#define lm_help 0
#define mb_help 0
#define mc_help 0
#define md_help 0
#define mo_help 0
#define ps_help 0
#define qu_help 0
#define re_help 0
#define rs_help 0
#define st_help 0
#define tr_help 0
#define ve_help 0
#define po_help 0
#define pc_help 0
#endif


/************************************************************************
 * COMMAND TABLE
 *
 * The following table describes the keyboard commands.
 * The fields in each entry have the following meanings:
 *
 * cmd_name
 *    Name of command as a '\0'-terminated string.  Only the number of
 *    leading characters required for a match with user-entered input
 *    appear here,
 *
 * cmd_action
 *    Function that actually performs the command.  It should be
 *    invoked with the following arguments, in order:
 *        - the cmd_internal_arg from the command entry.
 *        - the number of following arguments that are valid on
 *            this call (some arguments may be optional).
 *        - one or more user-entered arguments, already parsed
 *            according to the corresponding cmd_argtypes.
 *
 * cmd_internal_arg
 *    First argument to be passed to the command action function.
 *
 * cmd_argtypes
 *    A '\0'-terminated list of characters describing the legal
 *    user-entered arguments to the command.  The ascii version
 *    of each argument should be translated into the corresponding
 *    type before it is passed to the action function.  Valid types are:
 *
 *    D  The text may be either a decimal constant (it should be
 *       converted to binary before being passed) or a register name
 *       (the register's contents should be passed).
 *
 *    H  The text may be either a hexadecimal constant (it should be
 *       converted to binary before being passed) or a register name
 *       (the register's contents should be passed).
 *
 *    S  The ASCII string should be passed unchanged to the action function.
 *
 *    Upper case letters indicate required arguments, lower case letters
 *    indicated optional arguments.  It is assumed that all required
 *    arguments always precede all optional ones.
 *
 * cmd_help
 *    Text of the 'help' message for the command.
 *
 ************************************************************************/


struct cmd {
    char cmd_name[3];
    void (*cmd_action)();
    int cmd_internal_arg;
    char cmd_argtypes[3];
    const char * cmd_help;
};

static struct cmd *lookup_cmd(char * cmd);

static const struct cmd
cmd_table[] = {
    { ".",  0,            0,        "",      dot_help},
    { "?",  help,         0,        "s",     he_help },
    { "bd", databreak,    0,        "h",     bd_help },
    { "br", breakpt,      0,        "h",     br_help },
    { "cf", cf_cmd,       0,        "",      cf_help },
    { "da", dasm,         0,        "hd",    da_help },
    { "db", display,      BYTE,     "Sd",    db_help },
    { "dc", display_char, FALSE,    "Sd",    dc_help },
    { "dd", display,      LONG,     "Sd",    dd_help },
    { "de", delete,       0,        "H",     de_help },
    { "di", display,      WORD,     "Sd",    di_help },
    { "do", download,     0,        "h",     do_help },
    { "dq", display,      QUAD,     "Sd",    dq_help },
    { "ds", display,      SHORT,    "Sd",    ds_help },
    { "dt", display,      TRIPLE,   "Sd",    dt_help },
    { "ef", ef_cmd,       0,        "",      ef_help },
    { "fi", fill,         0,        "HHH",   fi_help },
    { "fl", disp_float,   LONG,     "Hd",    fl_help },
    { "fr", disp_float,   WORD,     "Hd",    fr_help },
    { "fx", disp_float,   EXTENDED, "Hd",    fx_help },
    { "go", go,           GO_RUN,   "h",     go_help },
    { "he", help,         0,        "s",     he_help },
    { "la", lmadr_cmd,    0,        "HH",    la_help },
    { "lm", lmmr_cmd,     0,        "HH",    lm_help },
    { "mb", modify,       BYTE,     "Sd",    mb_help },
    { "mc", mcon_cmd,     0,        "HH",    mc_help },
    { "md", modify_d,     WORD,     "SH",    md_help },
    { "mo", modify,       WORD,     "Sd",    mo_help },
    { "ps", go,           GO_NEXT,  "h",     ps_help },
    { "qu", reset,        1,        "",      qu_help },
    { "rb", reset,        1,        "",      qu_help },
    { "re", display_regs, 0,        "",      re_help },
    { "rs", reset,        0,        "",      rs_help },
    { "st", go,           GO_STEP,  "h",     st_help },
    { "tr", trace,        0,        "ss",    tr_help },
    { "ve", banner,       0,        "",      ve_help },
    { "po", post_test,    0,        "",      po_help },
    { "pp", pci_test,     0,        "hh",    pc_help },
    { "yy", pci_pc,       0,        "h",     po_help },
    { "zz", disp_prcb,    0,        "",      he_help },
    { "\0", 0, 0, "", 0 }
};

ADDR start_addr = NO_ADDR;
int user_is_running = FALSE;
int downld = FALSE;


/************************************************/
/* Monitor Shell                         */
/************************************************/
void
ui_main(const STOP_RECORD *stop_reason)
{
#define LINELEN         40      /* length of input line */
    char linebuff[LINELEN]; /* command line buffer */
    static char histbuff[LINELEN] = "";    /* command history buffer */
    char *p;
    int req_pri;
    static int first_time = TRUE;

    /* Enable break interrupt, so user can interrupt monitor output.
     * This will not enable the interrupt if an application is running
     * which has raised the processor priority above req_pri. */
    req_pri = (break_vector >> 3) - 1;
    if (get_mon_priority() > req_pri)
        set_mon_priority(req_pri);

    if (first_time)
    {
        banner();
        first_time = FALSE;
    }

    if (break_flag)
        /* workaround for Windows terminal emulation losing about 100
         * charaters after a break. We put in an artifical delay to make
         * it work */
        {
        int i,j;
        for (i=0; i<5000000; i++) j=i;       
        }

    if (stop_reason != NULL)
        stop_message(stop_reason);

    while (TRUE){
        do {
            break_flag = FALSE;
            prtf("=>");
            readline(linebuff,sizeof(linebuff));
            prtf("\n");
        } while (break_flag || linebuff[0] == '\0');

        for ( p = linebuff; *p; p++ ){
		/* convert upper case to lower case */
            if ( ((*p) >= 65) && ((*p) <= 90) ){
                *p = (*p) + 32;
            }
        }

        /* Check if first non-blank is repeat command */
        for ( p = linebuff; *p == ' ' || *p == '\t'; p++ ){
            ;
        }
        if ( *p == '.' ){
            strcpy(linebuff,histbuff);
            prtf("%s\n",histbuff);
        } else {
            /* not repeat command, add to history buffer */
            strcpy( histbuff, linebuff );
        }

        ui_cmd(linebuff, TRUE);

        if (break_flag)
            prtf("Interrupted\n");
    }
}

int
ui_cmd(char *linebuff, int terminal)
{
    struct cmd *cp;
    long pargs[4];
    char *args[MAXARGS];    /* Pointers to the individual words in linebuff.
                 * Each word is '\0'-terminated.  */
    int nargs;        /* Number of meaningful entries in 'args' */
    char *p;
    int i;
    int r;

    /* Break individual arguments out of command
     */
    nargs = get_words( linebuff, args, MAXARGS );

    if (!terminal && 
        ((strncmp(args[0],"do",2) == 0)  ||
         (strncmp(args[0],"mo",2) == 0)  ||
         (strncmp(args[0],"mb",2) == 0) )) 
        {
        prtf("Invalid command for non terminal host.\n");
        return OK;
        }

    /* Check for valid command and number of arguments */
    if ((cp=lookup_cmd(args[0])) == NULL || cp->cmd_action == NULL)
    {
        prtf("Invalid command\n");
        return OK;
    }
    if ( nargs > strlen(cp->cmd_argtypes)+1 ){
        /* +1 since nargs includes command name */
        prtf("Too many arguments\n");
        return OK;
    }


    /* Parse arguments according to types stored in command table
     */
    p = cp->cmd_argtypes;
    for ( i = 1; i < nargs; i++, p++ ){
        switch ( *p ){
        case 'H':
        case 'h':
			if (args[i][0]==0)
				{
				pargs[i] = NO_ADDR;
				break;
				}
            r = get_regnum(args[i]);
            if ( (r != ERR) && (r < NUM_REGS) ){
                pargs[i] = register_set[r];
            } else if ( !atoh(args[i],&pargs[i]) ){
                prtf("Invalid hex number: %s\n",args[i]);
                return OK;
            }
            break;
        case 'D':
        case 'd':
            r = get_regnum(args[i]);
            if ( (r != ERR) && (r < NUM_REGS) ){
                pargs[i] = register_set[r];
            } else if ( !atod(args[i],&pargs[i]) ){
                prtf("Invalid decimal number: %s\n",args[i]);
                return OK;
            }
            break;
        case 'S':
        case 's':
            pargs[i] = (long) args[i];
            break;
        }
    }

    /* Make sure all required arguments were present.
     * If so (upper case letter), call action function.
     */
    if ( ((*p) >= 65) && ((*p) <= 90) ){
        prtf("Missing argument(s)\n");
        return OK;
    }

    (*cp->cmd_action)(cp->cmd_internal_arg,i-1,pargs[1],pargs[2],pargs[3]);

    return OK;
}


/************************************************/
/* Output the Intro message            */
/************************************************/
static void
banner()
{
    prtf("\nMon960 User Interface: Version %s %s", base_version, build_date);
    prtf("\n%s; for i960 %s", board_name, arch_name);
    if (step_string != NULL) prtf("; %s", step_string);
#if BIG_ENDIAN_CODE
    prtf(" (Big-Endian)");
#endif /* __i960_BIG_ENDIAN__ */
    prtf("\nCopyright 1995 Intel Corporation\n");
}

static void
stop_message(const STOP_RECORD *stop_reason)
{
    static const char fault_names[][16] = {
    "Parallel",        /* Type 0 */
    "Trace",        /* Type 1 */
    "Operation",        /* Type 2 */
    "Arithmetic",        /* Type 3 */
    "Floating Point",    /* Type 4 */
    "Constraint",        /* Type 5 */
    "Virtual Memory",    /* Type 6 */
    "Protection",        /* Type 7 */
    "Machine",        /* Type 8 */
    "Structural",        /* Type 9 */
    "Type",            /* Type a */
    "Reserved (0xb)",    /* Type b */
    "Process",        /* Type c */
    "Descriptor",        /* Type d */
    "Event",        /* Type e */
    "Reserved (0xf)"    /* Type f */
    };

    int reason = stop_reason->reason;

    if (reason & STOP_EXIT)
        {
        user_is_running = FALSE;
        prtf("Program Exit: %x\n", stop_reason->info.exit_code);
        }

    if (reason & STOP_MON_ENTRY)
        prtf("Program called monitor\n");

    if (reason & STOP_CTRLC)
        prtf("User Interrupt\n");

    if (reason & STOP_BP_HW)
        prtf("Breakpoint at %X\n", stop_reason->info.hw_bp_addr);

    if (reason & STOP_BP_SW)
        prtf("Software breakpoint at %X\n", stop_reason->info.sw_bp_addr);

    if (reason & STOP_BP_DATA0)
        prtf("Data breakpoint at %X\n", stop_reason->info.da0_bp_addr);

    if (reason & STOP_BP_DATA1)
        prtf("Data breakpoint at %X\n", stop_reason->info.da1_bp_addr);

    if (reason & STOP_TRACE)
        {
        int type = stop_reason->info.trace.type;

        if (type != 0x02 || reason != STOP_TRACE)
            {
            prtf("Trace at %X:", stop_reason->info.trace.ip);
            if (type & 0x02) prtf(" Single-step");
            if (type & 0x04) prtf(" Branch");
            if (type & 0x08) prtf(" Call");
            if (type & 0x10) prtf(" Return");
            if (type & 0x20) prtf(" Pre-return");
            if (type & 0x40) prtf(" Supervisor Call");
            prtf("\n");
            }
        }

    if (reason & STOP_INTR)
        prtf("Unclaimed interrupt %d\n", stop_reason->info.intr_vector);

    if (reason & STOP_FAULT)
        {
        FAULT_RECORD *fault_ptr=(FAULT_RECORD *)stop_reason->info.fault.record;
        int type = stop_reason->info.fault.type;

        if (type < sizeof(fault_names)/sizeof(fault_names[0]))
            {
#if KXSX_CPU
            /*Special case when type==0.  The name of this fault
             *is not the same between CA & MC chips.  Since this
             *isn't the CA and we don't support the MC, report
             *it as reserved.  */
            if (type != 0)
                prtf("%s fault at ", fault_names[type] );
            else
                prtf("Reserved (0x0) at ");
#else /*CXHXJX*/
            prtf("%s fault at ", fault_names[type] );
#endif /*KXSX*/
            }
        else
            prtf("Unknown fault of type %B at ", type );

        prtf("%X, subtype %B\n",stop_reason->info.fault.ip,
             stop_reason->info.fault.subtype);
        prtf("Fault record is saved at %X\n", fault_ptr);
        }

    if (reason & STOP_UNK_SYS)
        prtf("Unknown system call or obsolete runtime request\n");

    if (reason & ~(STOP_EXIT|STOP_CTRLC|STOP_TRACE|STOP_FAULT
                  |STOP_INTR|STOP_BP_SW|STOP_UNK_BP|STOP_BP_HW
                  |STOP_BP_DATA0|STOP_BP_DATA1|STOP_UNK_SYS|STOP_MON_ENTRY))
        {
        int unkreason = reason & ~(STOP_EXIT|STOP_CTRLC|STOP_TRACE|STOP_FAULT
                                  |STOP_INTR|STOP_BP_SW|STOP_UNK_BP|STOP_BP_HW
                                  |STOP_BP_DATA0|STOP_BP_DATA1|STOP_UNK_SYS
                  |STOP_MON_ENTRY);
        prtf("Unrecognized stop reason %x\n", unkreason);
        }

    if (stop_reason->reason != STOP_EXIT)
        dasm(0, 0, 0, 0 );
}

/************************************************/
/* Look up a command in the command table    */
/************************************************/
static struct cmd *
lookup_cmd( cmd )
char cmd[2];
{
struct cmd *cp;

    for ( cp = (struct cmd *)cmd_table; cp->cmd_name[0] != (char)0; cp++ ){
        if ( !strncmp(cmd,cp->cmd_name,strlen(cp->cmd_name)) ){
            return cp;
        }
    }
    return NULL;
}


/************************************************/
/* Execute erase eeprom command.         */
/************************************************/
static void
ef_cmd()
{
    if (erase_eeprom(NO_ADDR, 0) != OK)
        perror((char *)NULL, 0);
}

/************************************************/
/* Execute check eeprom command.        */
/************************************************/
static void
cf_cmd()
{
    if (check_eeprom(NO_ADDR, 0) == OK)
        prtf("Flash is erased\n");
    else if (cmd_stat == E_EEPROM_PROG)
        prtf("Flash is programmed between 0x%X and 0x%X\n",
                eeprom_prog_first, eeprom_prog_last);
    else
        perror((char *)NULL, 0);

    if (eeprom_size > 0)
        prtf("Total flash size is 0x%x\n", eeprom_size);
}


/************************************************/
/* Parse keyboard command, and start/continue    */
/* execution of application.            */
/************************************************/
static void
go(int mode, int nargs, int addr)
{
    if (nargs == 1)
        register_set[REG_IP] = addr;
    else if (!user_is_running && start_addr != 0xffffffff) {
        /* fresh start, NOT continuing */
        /* Set default run address */
        register_set[REG_IP] = start_addr;
    }

    user_is_running = TRUE;
    if (prepare_go_user(mode, FALSE, 0) == OK)
    {
        exit_mon(mode == GO_SHADOW);
        /* exit_mon does not return */
    }

    perror((char *)NULL, 0);
}

/************************************************/
/* Help                                 */
/*                                       */
/************************************************/
static void
help(int dummy, int nargs, char *cmdname)
{

#ifdef HELP
    struct cmd *cp;

    if ( (nargs > 0) && (cp=lookup_cmd(cmdname)) != NULL )
		{
        prtf(cp->cmd_help);
	    }
    else 
		{
        prtf("Type 'he <command>' for help about a command, Available commands are:\n");
        prtf(all_help);
        prtf("\n");
        }
#else
    prtf("No help\n");
#endif
}

/************************************************/
/* post test for cyclone boards                 */
/*                                              */
/************************************************/
extern void _post_test();
static void
post_test(int dummy, int nargs, char *cmdname)
{
    _post_test();
}


/************************************************/
/* PCI test for cyclone boards                 */
/*                                              */
/************************************************/
static void
pci_test(int dummy, int nargs, int addr, int value)
{
    int i, j, pci_reg;

	if (nargs == 0)
		{
	  	for (i=0; i<0x100; i +=0x10)
		    {
	        prtf("\nPCI REG %x = ",i);
		    for (j=0; j<4; j++)
		        {
	            pci_reg =  *(volatile unsigned int *)(0x80000000 + i + j*4);
	            prtf(" %08X",pci_reg);
	    	    }
		    }
		prtf("\n");
		return;
		}

	pci_reg =  *(volatile unsigned int *)(0x80000000 + addr);
	prtf("PCI REG %x = %X",addr,pci_reg);
	if (nargs == 2)
		{
		*(volatile unsigned int *)(0x80000000 + addr) = value;
	    prtf("  now set to %X",value);
        }
	prtf("\n");

}

/************************************************/
/* PCI test for cyclone boards                 */
/*                                              */
/************************************************/
static void
pci_pc(int dummy, int nargs, int addr)
{
    int i, j, pci_reg, pc_addr=addr;

	if (nargs == 0)
		addr =0;

	pc_addr = addr + 0x40000000;

  	for (i=pc_addr; i<pc_addr + 0x100; i +=0x10)
	    {
        prtf("\nPCI PC MEM %x = ",i);
	    for (j=0; j<4; j++)
		        {
	            pci_reg =  *(volatile unsigned int *)(i + j*4);
	            prtf(" %08X",pci_reg);
	    	    }
		}

	prtf("\n");
}



/*****************************************************/
/* Modify a memory control register (ApLink Support) */
/*****************************************************/
static void
mcon_cmd(int dummy, int nargs, unsigned int region, unsigned int value)
{
    if (set_proc_mcon(region, value) != OK)
        perror((char *)NULL, 0);
}



/*************************************************************/
/* Modify a logical memory address register (ApLink Support) */
/*************************************************************/
static void
lmadr_cmd(int dummy, int nargs, unsigned int regno, unsigned int value)
{
#if HXJX_CPU
    if (regno > 1)
    {
        cmd_stat = E_ARG;
        perror((char *)NULL, 0);
    }
    else
    {
        volatile unsigned int *lmadr = (unsigned int *) 0xff008108; 

        lmadr[regno * 2] = value;
    }
#else

    cmd_stat = E_ARCH;
    perror((char *)NULL, 0);
#endif /* HXJX */
}



/**********************************************************/
/* Modify a logical memory mask register (ApLink Support) */
/**********************************************************/
static void
lmmr_cmd(int dummy, int nargs, unsigned int regno, unsigned int value)
{
#if HXJX_CPU
    if (regno > 1)
    {
        cmd_stat = E_ARG;
        perror((char *)NULL, 0);
    }
    else
    {
        volatile unsigned int *lmmr = (unsigned int *) 0xff00810c; 

        lmmr[regno * 2] = value;
    }
#else

    cmd_stat = E_ARCH;
    perror((char *)NULL, 0);
#endif /* HXJX */
}



#if 0
extern int get_82c54_reg();
static void
disp_82c54(int mode, int nargs, int addr)
{
	int reg;

    reg = get_82c54_reg(addr);
	prtf("REG %d = %x\n",addr,reg);
}
#else
static void
disp_82c54(int mode, int nargs, int addr)
{
	prtf("NO_82C54_DISP\n");
}
#endif

#if 0
extern int get_cio_crtl();
static void
cio_disp(int mode, int nargs, int addr)
{
	int reg;

    reg = get_cio_crtl(addr);
	prtf("REG %d = %x\n",addr,reg);
}
#else
static void
cio_disp(int mode, int nargs, int addr)
{
	prtf("NO_CIO_DISP\n");
}
#endif

static void
disp_prcb(int dummy, int nargs)
{
    extern long get_prcbptr();

	prtf("PRCB = %x\n", get_prcbptr());
}
