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

char gdb = FALSE;   /* Set to TRUE after the first GDB command is
		     * seen.  Thereafter, NINDY operates in "gdb mode"
		     * exclusively.  Otherwise, NINDY assumes it is
		     * interacting directly with a humanoid.
		     */

struct cmd * lookup_cmd();

	void baud();
	void breakpt();
static	void cf_cmd();
	void dasm();
	void databreak();
	void delete();
	void delete_data();
	void disp_float();
	void display();
	void display_regs();
	void download();
static	void ef_cmd();
	void fill();
	void go();
	void he_cmd();
static	void help();
	void modify();
	void out_message();
	void reset();
	void trace();

static const char
help_all1[] =
	"he [cmd]         - help info for optional cmd\n"
	"? [cmd]          - help info for optional cmd\n"
	"ve               - print out version header\n"
	"cf               - check if Flash is blank\n"
	"df               - download to Flash using Xmodem\n"
	"ef               - erase Flash\n"
	"rs               - reset board\n"
	"do               - download using Xmodem\n"
	"st [address]     - single step through program\n"
	"go [address]     - go from start, or continue from breakpoint\n"
	"fr address#times - display one or more real (32 bit) floating\n"
	"                   point numbers\n"
	"fl address#times - display one or more long real (64 bit)\n"
	"                   floating point numbers\n"
	"fx address#times - display one or more extended real (80 bit)\n"
	"                   floating point numbers\n"
	"da address#times - disassemble one or more instructions\n"
	"db address#bytes - display one or more bytes\n"
	"ds address#shrts - display one or more shorts\n"
	"di address#words - display one or more words\n"
	"dd address#times - display one or more sets of double words\n";

static const char help_all2[] =
	"dt address#times - display one or more sets of triple words\n"
	"dq address#times - display one or more sets of quad words\n"
	"re               - dump contents of registers\n"
	"di reg           - display the contents of the register 'reg'\n"
	"mo reg           - modify a register. Reg can NOT be fp0-fp3,\n"
	"mo address#words - modify one or more words in memory\n"
	"mb address       - modify a byte in memory (doesn't read byte first)\n"
	"fi address address data - fill memory with data value\n"
	"ba rate          - change current baud rate\n"
        "                   Choices are: 1200 2400 9600 19200 38400\n"
	"tr option on/off - turn on or off one of the following trace options\n"
        "                   branch, call, return or supervisor call\n"
	"br [address]     - set instruction break. If no address is given,\n"
        "                   all current breakpoints are displayed\n"
	"de address       - delete the specified instruction breakpoint\n"
	"bd [address]     - set data breakpoint. If no address is given,\n"
        "                   all current breakpoints are displayed\n"
	"ed address       - delete the specified data breakpoint\n"
	".                - repeat previous command\n";

static const char
dot_help[] =
	"\n.\n"
	"  Repeat previous command.\n";

static const char
ba_help[] =
	"\nba[ud] <rate>\n\n"
	"  Change the current baud rate to the one given.\n"
	"  Choices are:  1200 2400 4800 9600 19200 38400\n";

static const char
bd_help[] =
	"\nbd [<address>]\n\n"
	"  Set data breakpoint at the address given.  This only applies\n"
	"  to 80960Cx versions which have hardware data breakpoints.  If\n"
	"  no address is given all of the current data breakpoints are\n"
	"  displayed.  To delete a data breakpoint use the 'ed' command.\n";

static const char
br_help[] =
	"\nbr[eak] <address>\n\n"
	"  Set instruction break. If no address is given, all of the\n"
	"  current instruction breakpoints are displayed.  To delete an\n"
	"  instruction breakpoint, use the 'de' command.\n";

static const char
cf_help[] =
	"\ncf\n\n"
	"  Check if Flash is blank.  If not blank, prints out a message\n"
	"  stating the first and last addresses at which it is\n"
	"  programmed and the total size of the Flash EPROMs.\n";

static const char
da_help[] =
	"\nda[sm] [<address>][#<words>]\n\n"
	"  Disassemble an instruction beginning at the address given.\n"
	"  If no address is given, the current instruction pointer is used.\n";

static const char
db_help[] =
	"\ndb <address>[#<bytes>]\n\n"
	"  Display one or more bytes beginning at the address given.\n"
	"  Also displays bytes as ASCII characters if printable.\n";

static const char
dd_help[] =
	"\ndd <address>[#<times>]\n\n"
	"  Display one or more sets of double words at the address\n"
	"  given. Note that this command invokes a two-word burst fetch\n"
	"  on the 80960 burst bus.\n";

static const char
de_help[] =
	"\nde[lete] <address>\n\n"
	"  Delete the specified instruction breakpoint.\n";

static const char
df_help[] =
	"\ndf\n\n"
	"  Download to Flash EPROMs using Xmodem.  Fatal error messages\n"
	"  will be printed out if the Flash EPROM will no longer program.\n"
	"  The option does NOT erase Flash first before downloading.\n"
	"  This allows multiple programs to be fit into Flash address space.\n";

static const char
di_help[] =
	"di[splay] <address>[#<words>]\n\n"
	"  Display a whole word beginning at the address given.\n"
	"\ndi[splay] <register>\n\n"
	"  Display one register, where register can be any register.\n";

static const char
do_help[] =
	"\ndo[ownload]\n\n"
	"  Download using Xmodem.  If downloading to Flash, this will\n"
	"  automatically erase Flash first.\n";

static const char
dq_help[] =
	"\ndq address[#times]\n\n"
	"  Display one or more sets of quad words beginning at the address\n"
	"  given. Note that this command invokes a four-word burst fetch on\n"
	"  the 80960 burst bus.\n";

static const char
ds_help[] =
	"\nds <address>[#<shorts>]\n\n"
	"  Display one or more shorts beginning at the address given.\n";

static const char
dt_help[] =
	"\ndt <address>[#<times>]\n\n"
	"  Display one or more sets of triple words beginning at the address\n"
	"  given.  Note that this command invokes a three-word burst fetch\n"
	"  on the 80960 burst bus.\n";

static const char
ed_help[] =
	"\ned <address>\n\n"
	"  Erase the specified data breakpoint.";

static const char
ef_help[] =
	"\nef\n\n"
	"  Erase Flash EPROM.  Error messages will be printed\n"
	"  if the Flash will no longer erase.\n";

static const char
fi_help[] =
	"\nfi[ll] <address1> <address2> <data>\n\n"
	"  Fill memory from address1 to address2 inclusive with word of data value.\n"
	"  If address1 = address2 then one word is filled at address1\n";

static const char
fl_help[] =
	"\nfl[oat] <address>[#<times>]\n\n"
	"  Display one or more long real (64 bit) floating point numbers\n"
	"  beginning at the address given.\n";

static const char
fr_help[] =
	"\nfr <address>[#<times>]\n\n"
	"  Display one or more real (32 bit) floating point numbers beginning\n"
	"  at the address given.\n";

static const char
fx_help[] =
	"\nfx <address>[#<times>]\n\n"
	"  Display one or more extended real (80 bit) floating point numbers\n"
	"  beginning at the address given.\n";

static const char
go_help[] =
	"\ngo [<address>]\n\n"
	"  Begins execution of program at address given.  If no address\n"
	"  is given the default is read from information in the downloaded\n"
	"  file or the current IP is used.  Also used as a continue from\n"
	"  breakpoint.\n";

static const char
he_help[] =
	"\nhe[lp] [<command>]\n"
	"? [<command>]\n"
	"  Gives help for optional 'command'.  If no 'command' is specified\n"
	"  or unknown 'command', print short version of all commands.\n";

static const char
mb_help[] =
	"mb <address>\n\n"
	"  Modify a byte in memory (does not read byte first).\n";

static const char
mo_help[] =
	"\nmo[dify] <register>\n\n"
	"  Modify a register (eg. r4). Reg can NOT be floating point register.\n"
	"  NOTE: be careful when modifying the fp, pfp, sp, pc, ac, or tc.\n"
	"  ALSO NOTE: the register set is invalid until after the\n"
	"  application program has begun execution.\n"
	"\nmo[dify] <address>[#<words>]\n\n"
	"  Modify one or more words in memory.\n";

static const char
re_help[] =
	"\nre[gisters]\n\n"
	"  Dump contents of registers.\n";

static const char
rs_help[] =
	"\nrs\n\n"
	"  Resets the board.\n";

static const char
st_help[] =
	"\nst[ep] [<address>]\n\n"
	"  Single step through program.  NOTE: the address of the breakpoint\n"
	"  is the address of the instruction which has JUST FINISHED EXECUTING.\n"
	"  The RIP and IP will contain the instruction to be executed.\n";

static const char
tr_help[] =
	"\ntr[ace] [br|ca|re|su [on|off]]\n\n"
	"  Turn on or off one of the following trace options: br[anch],\n"
	"  ca[ll], re[turn] or su[pervisor].  If on/off is not given, the\n"
	"  status of that option is displayed.\n";


static const char
ve_help[] =
	"\nve[rsion]\n\n"
	"  Print out version header.\n";


/************************************************************************
 * COMMAND TABLE
 *
 * The following table describes the keyboard commands.
 * The fields in each entry have the following meanings:
 *
 * cmd_name
 *	Name of command as a '\0'-terminated string.  Only the number of
 *	leading characters required for a match with user-entered input
 *	appear here,
 *
 * cmd_action
 *	Function that actually performs the command.  It should be
 *	invoked with the following arguments, in order:
 *		- the cmd_internal_arg from the command entry.
 *		- the number of following arguments that are valid on
 *			this call (some arguments may be optional).
 *		- one or more user-entered arguments, already parsed
 *			according to the corresponding cmd_argtypes.
 *
 * cmd_internal_arg
 *	First argument to be passed to the command action function.
 *
 * cmd_argtypes
 *	A '\0'-terminated list of characters describing the legal
 *	user-entered arguments to the command.  The ascii version
 *	of each argument should be translated into the corresponding
 *	type before it is passed to the action function.  Valid types are:
 *
 *	D  The text may be either a decimal constant (it should be
 *	   converted to binary before being passed) or a register name
 *	   (the register's contents should be passed).
 *
 *	H  The text may be either a hexadecimal constant (it should be
 *	   converted to binary before being passed) or a register name
 *	   (the register's contents should be passed).
 *
 *	S  The ASCII string should be passed unchanged to the action function.
 *
 *	Upper case letters indicate required arguments, lower case letters
 *	indicated optional arguments.  It is assumed that all required
 *	arguments always precede all optional ones.
 *
 * cmd_help
 *	Text of the 'help' message for the command.
 *
 ************************************************************************/

struct cmd {
	char *cmd_name;
	void (*cmd_action)();
	long cmd_internal_arg;
	unsigned char cmd_argtypes[4];
	const char *cmd_help;
};

static const struct cmd
cmd_table[] = {
	{ ".",  0,		0,		"",	dot_help },
	{ "?",  help,		0,		"s",	he_help },
	{ "ba", baud,		0,		"S",	ba_help },
	{ "bd", databreak,	0,		"h",	bd_help },
	{ "br", breakpt,	0,		"h",	br_help },
	{ "cf", cf_cmd,		0,		"",	cf_help },
	{ "da", dasm,		0,		"hd",	da_help },
	{ "db", display,	BYTE,		"Sd",	db_help },
	{ "dd", display,	LONG,		"Sd",	dd_help },
	{ "de", delete,		0,		"H",	de_help },
	{ "df", download,	TRUE,		"",	df_help },
	{ "di", display,	INT,		"Sd",	di_help },
	{ "do", download,	FALSE,		"",	do_help },
	{ "dq", display,	QUAD,		"Sd",	dq_help },
	{ "ds", display,	SHORT,		"Sd",	ds_help },
	{ "dt", display,	TRIPLE,		"Sd",	dt_help },
	{ "ed", delete_data,	0,		"H",	ed_help },
	{ "ef", ef_cmd,		0,		"",	ef_help },
	{ "fi", fill,		0,		"HHH",	fi_help },
	{ "fl", disp_float,	LONG,		"Hd",	fl_help },
	{ "fr", disp_float,	INT,		"Hd",	fr_help },
	{ "fx", disp_float,	EXTENDED,	"Hd",	fx_help },
	{ "go", go,		FALSE,		"h",	go_help },
	{ "he", help,		0,		"s",	he_help },
	{ "mb", modify,		BYTE,		"Sd",	mb_help },
	{ "mo", modify,		INT,		"Sd",	mo_help },
	{ "re", display_regs,	0,		"",	re_help },
	{ "rs", reset,		0,		"",	rs_help },
	{ "st", go,		TRUE,		"h",	st_help },
	{ "tr", trace,		0,		"ss",	tr_help },
	{ "ve", out_message,	0,		"",	ve_help },
	{ NULL, 0 }
};



/************************************************/
/* Look up a command in the command table	*/
/************************************************/
static
struct cmd *
lookup_cmd( cmd )
char *cmd;
{
struct cmd *cp;

	for ( cp = (struct cmd *)cmd_table; cp->cmd_name != NULL; cp++ ){
		if ( !strncmp(cmd,cp->cmd_name,strlen(cp->cmd_name)) ){
			return cp;
		}
	}
	return NULL;
}

/************************************************/
/* Monitor Shell             			*/
/************************************************/
void
monitor()
{
struct cmd *cp;
long pargs[4];
static char histbuff[LINELEN] = "";	/* command history buffer */
char *args[MAXARGS];    /* Pointers to the individual words in linebuff.
			 * Each word is '\0'-terminated.
			 */
int nargs;		/* Number of meaningful entries in 'args' */
unsigned char *p;
int i;
int r;


	while (TRUE){
		do {
			prtf( "\n%s", user_is_running ? "->" : "=>" );
			readline(linebuff,sizeof(linebuff),TRUE);
		} while ( linebuff[0] == '\0' );

		if ( linebuff[0] == DLE ){
			/* Start of GDB command seen. */
			if ( gdb_cmd() ){
				return;		/* Return to user application */
			}
			continue;
		}

		for ( p = linebuff; *p; p++ ){
			if ( isupper(*p) ){
				*p = tolower(*p);
			}
		}

		/* Check if first non-blank is repeat command
		 */
		for ( p = linebuff; *p == ' ' || *p == '\t'; p++ ){
			;
		}
		if ( *p == '.' ){
			strcpy(linebuff,histbuff);
			prtf("\n%s",histbuff);
		} else {
			/* not repeat command, add to history buffer */
			strcpy( histbuff, linebuff );
		}
		prtf("\n");


		/* Break individual arguments out of command
		 */
		nargs = get_words( linebuff, args, MAXARGS );


		/* Check for valid command and number of arguments
		 */
		if ( (cp=lookup_cmd(args[0])) == NULL ){
			prtf("Invalid command\n");
			continue;
		}
		if ( nargs > strlen(cp->cmd_argtypes)+1 ){
			/* +1 since nargs includes command name */
			prtf( "Too many arguments\n" );
			continue;
		}


		/* Parse arguments according to types stored in command table
		 */
		p = cp->cmd_argtypes;
		for ( i = 1; i < nargs; i++, p++ ){
			switch ( *p ){
			case 'H':
			case 'h':
				r = get_regnum(args[i]);
				if ( (r != ERROR) && (r < NUM_REGS) ){
					pargs[i] = register_set[r];
				} else if ( !atoh(args[i],&pargs[i]) ){
					prtf("Invalid hex number: %s\n",args[i]);
					return;
				}
				break;
			case 'D':
			case 'd':
				r = get_regnum(args[i]);
				if ( (r != ERROR) && (r < NUM_REGS) ){
					pargs[i] = register_set[r];
				} else if ( !atod(args[i],&pargs[i]) ){
					prtf("Invalid decimal number: %s\n",args[i]);
					return;
				}
				break;
			case 'S':
			case 's':
				pargs[i] = (long) args[i];
				break;
			}
		}

		/* Make sure all required arguments were present.
		 * If so, call action function.
		 */
		if ( isupper(*p) ){
			prtf( "Missing argument(s)\n" );
		} else {
			cp->cmd_action( cp->cmd_internal_arg, i-1,
						pargs[1], pargs[2], pargs[3] );
			if ( cp->cmd_action == go && user_is_running ){
				return;
			}
		}
	}
}


/************************************************/
/* Execute erase flash command. 		*/
/************************************************/
static
void
ef_cmd()
{
	if (erase_flash() == ERROR){
		if (errno == NOFLASH){
			prtf ("No Flash in programmable address space\n");
		} else {
			prtf ("Error erasing Flash\n");
		}
	}
}

/************************************************/
/* Execute check flash command.			*/
/************************************************/
static
void
cf_cmd()
{
int i;

	if (fltype == ERROR) {
		prtf ("Flash not present in programmable address space\n");
	} else {
		check_flash();
	}
}


/************************************************/
/* Parse keyboard command, and start/continue	*/
/* execution of application.			*/
/************************************************/
void
go( step_flag, nargs, addr )
int step_flag;
int nargs;	/* Number of the following arguments that are valid */
int addr;
{
	prtf("\n");

	if ( nargs == 0 ){
		/* Set default run address */

		if ( user_is_running ){	/* continuing from fault */
			addr = register_set[REG_IP];
		} else {		/* fresh start, NOT continuing */
			addr = aoutbuf.start_addr;
		}
	}

	if ( step_flag ){
		set_trace_step();
	}
	register_set[REG_IP] = addr;

	if ( user_is_running ){		/* continuing from fault */
 		/* Return back through the monitor entry point,
		 * the fault handler, and finally the application program.
		 */
		;

	} else {			/* fresh start, NOT continuing */
		begin(addr);
	}
}

/************************************************/
/* Output the Intro message     		*/
/*                           			*/
/************************************************/
void
out_message()
{
	prtf("\n            NINDY monitor for the 80960%s %s\n",
						architecture, boardname );

	prtf("               	    Version %s%s\n\n", XVERSION, VERSION );
 	prtf("          Copyright (c) 1989, 1990, Intel Corporation\n");
}

/************************************************/
/* Help                         		*/
/*                           			*/
/************************************************/
static
void
help( dummy, nargs, cmdname )
int dummy;	/* Ignored */
int nargs;	/* Number of the following arguments that are valid */
char * cmdname;
{
	struct cmd *cp;

	if ( (nargs > 0) && (cp=lookup_cmd(cmdname)) != NULL ){
		prtf( cp->cmd_help );
	} else {
		prtf ("\nAvailable commands are:\n%s", help_all1 );
		prtf (" Hit SPACE to continue (or q to quit)\r");
		if (ci() != 'q'){
			prtf(help_all2);
		}
	}
}
