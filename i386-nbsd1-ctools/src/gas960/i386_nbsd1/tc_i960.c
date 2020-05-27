/* i960.c - All the i80960-specific stuff
   Copyright (C) 1989, 1990, 1991 Free Software Foundation, Inc.

This file is part of GAS.

GAS is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GAS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GAS; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* $Id: tc_i960.c,v 1.101 1995/08/07 20:32:40 kevins Exp paulr $ */

/* See comment on md_parse_option for 80960-specific invocation options. */

/******************************************************************************
 * i80960 NOTE!!!:
 *	Header, symbol, and relocation info will be used on the host machine
 *	only -- only executable code is actually downloaded to the i80960.
 *	Therefore, leave all such information in host byte order.
 *
 *	(That's a slight lie -- we DO download some header information, but
 *	the downloader converts the file format and corrects the byte-ordering
 *	of the relevant fields while doing so.)
 *
 ***************************************************************************** */

/* There are 4 different lengths of (potentially) symbol-based displacements
 * in the 80960 instruction set, each of which could require address fix-ups
 * and (in the case of external symbols) emission of relocation directives:
 *
 * 32-bit (MEMB)
 *	This is a standard length for the base assembler and requires no
 *	special action.
 *
 * 13-bit (COBR)
 *	This is a non-standard length, but the base assembler has a hook for
 *	bit field address fixups:  the fixS structure can point to a descriptor
 *	of the field, in which case our md_number_to_field() routine gets called
 *	to process it.
 *
 *	I made the hook a little cleaner by having fix_new() (in the base
 *	assembler) return a pointer to the fixS in question.  And I made it a
 *	little simpler by storing the field size (in this case 13) instead of
 *	of a pointer to another structure:  80960 displacements are ALWAYS
 *	stored in the low-order bits of a 4-byte word.
 *
 *	Since the target of a COBR cannot be external, no relocation directives
 *	for this size displacement have to be generated.  But the base assembler
 *	had to be modified to issue error messages if the symbol did turn out
 *	to be external.
 *
 * 24-bit (CTRL)
 *	Fixups are handled as for the 13-bit case (except that 24 is stored
 *	in the fixS).
 *
 *	The relocation directive generated is the same as that for the 32-bit
 *	displacement, except that it's PC-relative (the 32-bit displacement
 *	never is).   The i80960 version of the linker needs a mod to
 *	distinguish and handle the 24-bit case.
 *
 * 12-bit (MEMA)
 *	MEMA formats are always promoted to MEMB (32-bit) if the displacement
 *	is based on a symbol, because it could be relocated at link time.
 *	The only time we use the 12-bit format is if an absolute value of
 *	less than 4096 is specified, in which case we need neither a fixup nor
 *	a relocation directive.
 */

#include <stdio.h>
#include <ctype.h>

#include "as.h"

#include "obstack.h"

#ifdef GNU960
#	include "i960_opc.h"
#else
#	include "i960-opcode.h"
#endif
#include "cc_info.h"

#ifdef NO_STDARG
#include <varargs.h>	/* For md_set_arch_message */
#else
#include <stdarg.h>
#endif /* NO_STDARG */

extern char *input_line_pointer;
extern struct hash_control *po_hash;
extern unsigned char nbytes_r_length[];
extern char *next_object_file_charP;

/* Global vars related to general segments */
extern segS	*segs;		/* Internal segment array */
extern int  	curr_seg;	/* Active segment (index into segs[]) */
extern int	DEFAULT_TEXT_SEGMENT; 	/* .text */
extern int	DEFAULT_DATA_SEGMENT; 	/* .data */
extern int	DEFAULT_BSS_SEGMENT; 	/* .bss  */

#ifdef OBJ_COFF
int md_reloc_size = sizeof(struct reloc);
#else /* OBJ_COFF */
int md_reloc_size = sizeof(struct relocation_info);
#endif /* OBJ_COFF */

#ifdef	DEBUG
extern int	tot_instr_count;
extern int	mem_instr_count;
extern int	mema_to_memb_count;
extern int	FILE_tot_instr_count;
extern int	FILE_mem_instr_count;
extern int	FILE_mema_to_memb_count;
extern char 	*instr_count_file;
#endif

#if defined(OBJ_BOUT)
#ifdef __STDC__

static void emit_machine_reloc(fixS *fixP, relax_addressT segment_address_in_file);

#else /* __STDC__ */

static void emit_machine_reloc();

#endif /* __STDC__ */

void (*md_emit_relocations)() = emit_machine_reloc;
#endif /* OBJ_BOUT */

	/***************************
	 *  Local i80960 routines  *
	 ************************** */

static void	brcnt_emit();	/* Emit branch-prediction instrumentation code*/
static char *	brlab_next();	/* Return next branch local label */
       void	brtab_emit();	/* Emit br-predict instrumentation table */
static void	cobr_fmt();	/* Generate COBR instruction */
static void	ctrl_fmt();	/* Generate CTRL instruction */
static void	cc_info();	/* Process .ccinfo and .cchead directives */
       long	i960_ccinfo_size(); /* Report total size of ccinfo data */
       void	i960_emit_ccinfo(); /* Append ccinfo data to output file */
static char *	emit();		/* Emit (internally) binary */
static int	get_args();	/* Break arguments out of comma-separated list*/
static void	get_cdisp();	/* Handle COBR or CTRL displacement */
static char *	get_ispec();	/* Find index specification string */
static int	get_regnum();	/* Translate text to register number */
static int	i_scan();	/* Lexical scan of instruction source */
static void	mem_fmt();	/* Generate MEMA or MEMB instruction */
static void	mema_to_memb();	/* Convert MEMA instruction to MEMB format */
static segT	parse_expr();	/* Parse an expression */
static int	parse_ldconst();/* Parse and replace a 'ldconst' pseudo-op */
static void	parse_memop();	/* Parse a memory operand */
static void	parse_po();	/* Parse machine-dependent pseudo-op */
static void	parse_regop();	/* Parse a register operand */
static void	copr_fmt();	/* Gnerate a coprocessor instruction */
static void	reg_fmt();	/* Generate a REG format instruction */
       void	reloc_callj();	/* Relocate a 'callj' instruction */
static void	relax_cobr();	/* "De-optimize" cobr into compare/branch */
static void	s_leafproc();	/* Process '.leafproc' pseudo-op */
static void	s_sysproc();	/* Process '.sysproc' pseudo-op */
static void	s_mark_pixfile(); /* Process '.pic', '.pid', '.link_pix' pseudo-ops */
static int	shift_ok();	/* Will a 'shlo' substiture for a 'ldconst'? */
static void	syntax();	/* Give syntax error */
static int	targ_has_sfr();	/* Target chip supports spec-func register? */
static int	targ_has_instruction();/* Target chip supports instruction? */
static void 	s_extended();   /* Disable .extended for now. */

/* A function pointer to report either errors or warnings for 
   opcode-not-in-architecture, based on setting of -x.
   (defaults to as_error in md_begin, may be set to as_warn 
   in md_set_arch_message) */
#if defined(__STDC__) && ! defined(NO_STDARG)
static void	(*wrong_arch_message)(const char *, ...);
#else
static void	(*wrong_arch_message)();
#endif

/* Characters that always start a comment. */
char comment_chars[] = "#";

/* Characters that only start a comment at the beginning of a line. */
char line_comment_chars[] = "";

/* Also note that C-style comments like this one will always work. */


/* Chars that can be used to separate mant from exp in floating point nums */
char EXP_CHARS[] = "eE";

/* Chars that mean this number is a floating point constant,
 * as in 0f12.456 or 0d1.2345e12
 */
char FLT_CHARS[] = "fFdDtT";

/* Workaround for an ambiguity in "0f-10": is it a FP constant or 
 * is the 0f a local label?
 */
int force_local_label;

/* Table used by base assembler to relax addresses based on varying length
 * instructions.  The fields are:
 *   1) most positive reach of this state,
 *   2) most negative reach of this state,
 *   3) how many bytes this mode will add to the size of the current frag
 *   4) which index into the table to try if we can't fit into this one.
 *
 * For i80960, the only application is the (de-)optimization of cobr
 * instructions into separate compare and branch instructions when a 13-bit
 * displacement won't hack it.
 */
const relax_typeS
md_relax_table[] = {
	{0,         0,        0,0}, /* State 0 => no more relaxation possible */
	{4088,      -4096,    0,2}, /* State 1: conditional branch (cobr) */
	{0x800000-8,-0x800000,4,0}, /* State 2: compare (reg) & branch (ctrl) */
};


/* These are the machine dependent pseudo-ops.
 *
 * This table describes all the machine specific pseudo-ops the assembler
 * has to support.  The fields are:
 *	pseudo-op name without dot
 *	function to call to execute this pseudo-op
 *	integer arg to pass to the function
 */
#define S_LEAFPROC	1
#define S_SYSPROC	2
#define S_PIC		3
#define	S_PID		4
#define S_LINKPIX	5

const pseudo_typeS
md_pseudo_table[] = {
	{ "bss",	s_lcomm,	1	},
	{ "cchead",	cc_info,	1	},
	{ "ccinfo",	cc_info,	0	},
	{ "leafproc",	parse_po,	S_LEAFPROC },
#ifdef ALLOW_BIG_CONS
	{ "quad",	big_cons,	16	},
#endif
	{ "sysproc",	parse_po,	S_SYSPROC },
	{ "word",	cons,		4	},
	{ "pic",	parse_po,	S_PIC },
	{ "pid", 	parse_po,	S_PID },
	{ "link_pix",	parse_po,	S_LINKPIX },
	{ 0,		0,		0	}
};
 
/* Macros to extract info from an 'expressionS' structure 'e' */
#define e_adds(e)	e.X_add_symbol
#define e_subs(e)	e.X_subtract_symbol
#define e_offs(e)	e.X_add_number
#define e_segs(e)	e.X_seg


/* Branch-prediction bits for CTRL/COBR format opcodes */
#define BP_MASK		0x00000002  /* Mask for branch-prediction bit */
#define BP_TAKEN	0x00000000  /* Value to OR in to predict branch */
#define BP_NOT_TAKEN	0x00000002  /* Value to OR in to predict no branch */


/* Some instruction opcodes that we need explicitly */
#define BE	0x12000000
#define BG	0x11000000
#define BGE	0x13000000
#define BL	0x14000000
#define BLE	0x16000000
#define BNE	0x15000000
#define BNO	0x10000000
#define BO	0x17000000
#define CHKBIT	0x5a002700
#define CMPI	0x5a002080
#define CMPO	0x5a002000

#define B	0x08000000
#define BAL	0x0b000000
#define CALL	0x09000000
#define CALLS	0x66003800
#define RET	0x0a000000


/* These masks are used to build up a set of MEMB mode bits. */
#define	A_BIT		0x0400
#define	I_BIT		0x0800
#define MEMB_BIT	0x1000
#define	D_BIT		0x2000


/* Mask for the only mode bit in a MEMA instruction (if set, abase reg is used) */
#define MEMA_ABASE	0x2000

/* Info from which a MEMA or MEMB format instruction can be generated */
typedef struct {
	long opcode;	/* (First) 32 bits of instruction */
	int disp;	/* 0-(none), 12- or, 32-bit displacement needed */
	char *e;	/* The expression in the source instruction from
			 *	which the displacement should be determined
			 */
} memS;


/* The two pieces of info we need to generate a register operand */
struct regop {
	int mode;	/* 0 =>local/global/spec reg; 1=> literal or fp reg */
	int special;	/* 0 =>not a sfr;  1=> is a sfr (not valid w/mode=0) */
	int n;		/* Register number or literal value */
};


/* Number and assembler mnemonic for all registers that can appear in operands */
struct  i960_register
{
	char *reg_name;
	int reg_num;
}; 

static struct i960_register
regnames[] = 
{
	{ "pfp",  0 }, { "sp",   1 }, { "rip",  2 }, { "r3",   3 },
	{ "r4",   4 }, { "r5",   5 }, { "r6",   6 }, { "r7",   7 },
	{ "r8",   8 }, { "r9",   9 }, { "r10", 10 }, { "r11", 11 },
	{ "r12", 12 }, { "r13", 13 }, { "r14", 14 }, { "r15", 15 },
	{ "g0",  16 }, { "g1",  17 }, { "g2",  18 }, { "g3",  19 },
	{ "g4",  20 }, { "g5",  21 }, { "g6",  22 }, { "g7",  23 },
	{ "g8",  24 }, { "g9",  25 }, { "g10", 26 }, { "g11", 27 },
	{ "g12", 28 }, { "g13", 29 }, { "g14", 30 }, { "fp",  31 },

	/* Numbers for special-function registers are for assembler internal
	 * use only: they are scaled back to range [0-31] for binary output.
	 */
#	define SF0	32

	{ "sf0", 32 }, { "sf1", 33 }, { "sf2", 34 }, { "sf3", 35 },
	{ "sf4", 36 }, { "sf5", 37 }, { "sf6", 38 }, { "sf7", 39 },
	{ "sf8", 40 }, { "sf9", 41 }, { "sf10",42 }, { "sf11",43 },
	{ "sf12",44 }, { "sf13",45 }, { "sf14",46 }, { "sf15",47 },
	{ "sf16",48 }, { "sf17",49 }, { "sf18",50 }, { "sf19",51 },
	{ "sf20",52 }, { "sf21",53 }, { "sf22",54 }, { "sf23",55 },
	{ "sf24",56 }, { "sf25",57 }, { "sf26",58 }, { "sf27",59 },
	{ "sf28",60 }, { "sf29",61 }, { "sf30",62 }, { "sf31",63 },

	/* Numbers for floating point registers are for assembler internal use
	 * only: they are scaled back to [0-3] for binary output.
	 */
#	define FP0	64

	{ "fp0", 64 }, { "fp1", 65 }, { "fp2", 66 }, { "fp3", 67 },

	{ NULL,  0 },		/* END OF LIST */
};

#define	IS_RG_REG(n)	((0 <= (n)) && ((n) < SF0))
#define	IS_SF_REG(n)	((SF0 <= (n)) && ((n) < FP0))
#define	IS_FP_REG(n)	((n) >= FP0)

/* Number and assembler mnemonic for all registers that can appear as 'abase'
 * (indirect addressing) registers.
 */
static struct i960_register
aregs[] = 
{
	{ "(pfp)",  0 }, { "(sp)",   1 }, { "(rip)",  2 }, { "(r3)",   3 },
	{ "(r4)",   4 }, { "(r5)",   5 }, { "(r6)",   6 }, { "(r7)",   7 },
	{ "(r8)",   8 }, { "(r9)",   9 }, { "(r10)", 10 }, { "(r11)", 11 },
	{ "(r12)", 12 }, { "(r13)", 13 }, { "(r14)", 14 }, { "(r15)", 15 },
	{ "(g0)",  16 }, { "(g1)",  17 }, { "(g2)",  18 }, { "(g3)",  19 },
	{ "(g4)",  20 }, { "(g5)",  21 }, { "(g6)",  22 }, { "(g7)",  23 },
	{ "(g8)",  24 }, { "(g9)",  25 }, { "(g10)", 26 }, { "(g11)", 27 },
	{ "(g12)", 28 }, { "(g13)", 29 }, { "(g14)", 30 }, { "(fp)",  31 },

#	define IPREL	32
	/* for assembler internal use only: this number never appears in binary
	 * output.
	 */
	{ "(ip)", IPREL },

	{ NULL,  0 },		/* END OF LIST */
};

/* A few global and local regs are aliased in the i960 instruction set.
 * The convention is that you MUST use the alias, not the "real" name.
 * List their "real" names so that error messages can be more coherent.
 */
static struct i960_register
illegalregs[] = 
{
	{ "r0", 0 },   /* alias pfp */
	{ "r1", 1 },   /* alias sp */
	{ "r2", 2 },   /* alias rip */
	{ "g15", 31 }, /* alias fp */
	{ NULL, 0 }    /* END OF LIST */
};

/* Hash tables */
static struct hash_control *op_hash = NULL;	/* Opcode mnemonics */
static struct hash_control *reg_hash = NULL;	/* Register name hash table */
static struct hash_control *areg_hash = NULL;	/* Abase register hash table */

/* Special hash table for errata detection */
static struct hash_control *errata1_hash = NULL;/* instructions we detect */

/* Architecture for which we are assembling */
#define ARCH_ANY	0	/* No architecture checking */
#define ARCH_KA		1
#define ARCH_KB		2
#define ARCH_CX		3
#define ARCH_JX		4
#define ARCH_JL		5
#define ARCH_HX		6

static int architecture = ARCH_KB;	/* Architecture seen on command line
					 * or with I960ARCH env var
					 */
#ifdef OBJ_ELF
static int elf_arch = EF_960_KB;
#endif

struct tabentry 
{ 
	char *flag; 
	int arch;
#ifdef OBJ_ELF
	int elf_arch;
#endif
};

/* Only the architectures listed in the following table 
 * will assemble without error.
 */
static struct tabentry arch_tab[] = 
{
#ifdef OBJ_ELF
        "ANY", ARCH_ANY,   EF_960_GENERIC,
	"KA", ARCH_KA,     EF_960_KA,
	"KB", ARCH_KB,     EF_960_KB,
	"SA", ARCH_KA,     EF_960_SA, /* Synonym for KA */
	"SB", ARCH_KB,     EF_960_SB, /* Synonym for KB */
	"CA", ARCH_CX,     EF_960_CA,
	"CF", ARCH_CX,     EF_960_CF,
	"CA_DMA", ARCH_CX, EF_960_CA,
	"CF_DMA", ARCH_CX, EF_960_CF,
	"JA", ARCH_JX,     EF_960_JA,
	"JF", ARCH_JX,     EF_960_JF,
	"JD", ARCH_JX,     EF_960_JD,
	"JL", ARCH_JL,     EF_960_JL,
	"RP", ARCH_JX,     EF_960_RP,
	"HA", ARCH_HX,     EF_960_HA,
	"HD", ARCH_HX,     EF_960_HD,
	"HT", ARCH_HX,     EF_960_HT,
	NULL, 0, 0		/* Last entry must be NULL for a sentinel */
#else
        "ANY", ARCH_ANY,
	"KA", ARCH_KA,
	"KB", ARCH_KB,
	"SA", ARCH_KA,  /* Synonym for KA */
	"SB", ARCH_KB,  /* Synonym for KB */
	"CA", ARCH_CX,
	"CF", ARCH_CX,
	"CA_DMA", ARCH_CX,
	"CF_DMA", ARCH_CX,
	"JA", ARCH_JX,
	"JF", ARCH_JX,
	"JD", ARCH_JX,
	"JL", ARCH_JL,
	"RP", ARCH_JX,
	"HA", ARCH_HX,
	"HD", ARCH_HX,
	"HT", ARCH_HX,
	NULL, 0		/* Last entry must be NULL for a sentinel */
#endif
};

int iclasses_seen = 0;		/* OR of instruction classes (I_* constants)
				 *	for which we've actually assembled
				 *	instructions.
				 */
int pic_flag = 0;		/* Position-independent code flag */
int pid_flag = 0;               /* Position-independent data flag */
int link_pix_flag = 0;          /* Linkable with PIC or PID module flag */

/* CA DMA ERRATA1
 * There is an instruction triplet that we must detect, and insert a nop
 * into the instruction stream when it is detected.  The triple is
 * REG MEM CTRL but only certain REG's and CTRL's are candidates.
 * (see i960_opc.h for a complete list.)
 * 
 * We will only do the above if a command-line switch was thrown.
 * The flag "detect_errata1" will be set if the command-line switch was seen.
 *
 * The implementation is a simple state machine.
 *	state 0 = no danger of triplet
 *	state 1 = reg-side instruction was seen
 *	state 2 = mem-side instruction was seen AND state == 1
 *	state 3 = ctrl instruction was seen AND state == 2
 * We will emit a nop (reg) instruction just before the ctrl, only when
 * in state 3.  In all cases, if the state doesn't advance, it goes to 0.
 */
static int	detect_errata1 = 0;
static int 	errata1_state = 0;
static long 	nop_instr = 0x5c801610;  /* mov g0, g0 */

/* BRANCH-PREDICTION INSTRUMENTATION
 *
 *	The following supports generation of branch-prediction instrumentation
 *	(turned on by -b switch).  The instrumentation collects counts
 *	of branches taken/not-taken for later input to a utility that will
 *	set the branch prediction bits of the instructions in accordance with
 *	the behavior observed.  (Note that the KX series does not have
 *	brach-prediction.)
 *
 *	The instrumentation consists of:
 *
 *	(1) before and after each conditional branch, a call to an external
 *	    routine that increments and steps over an inline counter.  The
 *	    counter itself, initialized to 0, immediately follows the call
 *	    instruction.  For each branch, the counter following the branch
 *	    is the number of times the branch was not taken, and the difference
 *	    between the counters is the number of times it was taken.  An
 *	    example of an instrumented conditional branch:
 *
 *				call	BR_CNT_FUNC
 *				.word	0
 *		LBRANCH23:	be	label
 *				call	BR_CNT_FUNC
 *				.word	0
 *
 *	(2) a table of pointers to the instrumented branches, so that an
 *	    external postprocessing routine can locate all of the counters.
 *	    the table begins with a 2-word header: a pointer to the next in
 *	    a linked list of such tables (initialized to 0);  and a count
 *	    of the number of entries in the table (exclusive of the header.
 *
 *	    Note that input source code is expected to already contain calls
 *	    an external routine that will link the branch local table into a
 *	    list of such tables.
 */

static int br_cnt = 0;		/* Number of branches instrumented so far.
				 * Also used to generate unique local labels
				 * for each instrumented branch
				 */

#define BR_LABEL_BASE	"LBRANCH"
				/* Basename of local labels on instrumented
				 * branches, to avoid conflict with compiler-
				 * generated local labels.
				 */

#define BR_CNT_FUNC	"__inc_branch"
				/* Name of the external routine that will
				 * increment (and step over) an inline counter.
				 */

#define BR_TAB_NAME	"__BRANCH_TABLE__"
				/* Name of the table of pointers to branches.
				 * A local (i.e., non-external) symbol.
				 */
 
/*****************************************************************************
 * md_begin:  One-time initialization.
 *
 *	Set up hash tables.
 *	Set default architecture if I960ARCH is set.
 *	Set default architecture-checking error level.
 *
 *****************************************************************************/
void
md_begin()
{
	int i;				/* Loop counter */
	const struct i960_opcode *oP; /* Pointer into opcode table */
	char *retval;			/* Value returned by hash functions */
	char *arch_str;			/* User's I960ARCH setting */

	if ( (op_hash = hash_new()) == 0
	    || (reg_hash = hash_new()) == 0
	    || (areg_hash = hash_new()) == 0
	    || (errata1_hash = hash_new()) == 0 )
	{
		as_fatal("virtual memory exceeded");
	}

	retval = "";	/* For some reason, the base assembler uses an empty
			 * string for "no error message", instead of a NULL
			 * pointer.
			 */

	for (oP=i960_opcodes; oP->name && !*retval; oP++) {
		retval = hash_insert(op_hash, oP->name, oP);
	}

	for (i=0; regnames[i].reg_name && !*retval; i++) {
		retval = hash_insert(reg_hash, regnames[i].reg_name,
				     &regnames[i].reg_num);
	}

	for (i=0; aregs[i].reg_name && !*retval; i++){
		retval = hash_insert(areg_hash, aregs[i].reg_name,
				     &aregs[i].reg_num);
	}

	for ( i = 0; errata1_opcodes[i].name && !*retval; i++ )
	{
	    retval = hash_insert(errata1_hash, errata1_opcodes[i].name, &errata1_opcodes[i]);
	}

	if (*retval) {
		as_fatal("Hashing returned \"%s\".", retval);
	}

	if ( (arch_str = env_var("I960ARCH")) )
	{
		/* Check user's architecture choice against 
		 * valid choices.
		 */
		int	valid = 0;
		for ( i = 0; arch_tab[i].flag; ++i )
		{
			if ( ! strcmp(arch_str, arch_tab[i].flag) )
			{
				architecture = arch_tab[i].arch;
#ifdef OBJ_ELF
				elf_arch = arch_tab[i].elf_arch;
#endif
				valid = 1;
				break;
			}
		}
		if ( ! valid )
			as_warn("Ignoring I960ARCH unsupported architecture: %s", arch_str);
		else if ( ! strcmp(arch_str, "CA_DMA") || ! strcmp(arch_str, "CF_DMA") )
			detect_errata1 = 1;
	}
	/* Default error level for opcode-not-in-architecture is error.
	   (may be set to warning if -x seen on command line) */
	wrong_arch_message = as_bad;
} /* md_begin() */

/*****************************************************************************
 * md_end:  One-time final cleanup
 *
 *	None necessary
 *
 **************************************************************************** */
void
md_end()
{
}

/*****************************************************************************
 * md_arch_hook: 
 *
 * After all preliminary processing is complete, but before the first 
 * instruction is read in, do some futzing based on the architecture.
 * Currently, there is only one thing to do:
 *
 * (1) Relax the syntax definition of sysctl for CA and CX processors
 *     to allow a literal in the 3rd operand.  The relaxed syntax will fault
 *     if decoded on a J-series or H-series processor.
 *
 *****************************************************************************/
md_arch_hook()
{
    if ( architecture == ARCH_CX || architecture == ARCH_ANY )
    {
	struct i960_opcode *op;
	op = (struct i960_opcode *) hash_find(op_hash, "sysctl");
	op->operand[2] |= OP_LIT;	    /* Allow literal in 3rd operand */
    }
}

/*****************************************************************************
 * md_assemble:  Assemble an instruction
 *
 * Assumptions about the passed-in text:
 *	- all comments, labels removed
 *	- text is an instruction
 *	- all white space compressed to single blanks
 *	- all character constants have been replaced with decimal
 *
 **************************************************************************** */
void
md_assemble(textP)
    char *textP;	/* Source text of instruction */
{
    char *args[MAX_OPS+1];	/* Parsed instruction text, containing NO whitespace:
				 *	arg[0]->opcode mnemonic
				 *	arg[1-X]->operands, with char constants
				 *			replaced by decimal numbers
				 */
    int n_ops;			/* Number of instruction operands */

    struct i960_opcode *oP;
    /* Pointer to instruction description */
    struct i960_errata1 *eP;
    /* Pointer to (potentially) part of dangerous
       instruction triplet */
    int branch_predict;
    /* TRUE iff opcode mnemonic included branch-prediction
     *	suffix (".f" or ".t")
     */
    long bp_bits;		/* Setting of branch-prediction bit(s) to be OR'd
				 *	into instruction opcode of CTRL/COBR format
				 *	instructions.
				 */
    int n;			/* Offset of last character in opcode mnemonic */

    static const char bp_error_msg[] = "branch prediction invalid on this opcode";

#ifdef  DEBUG
    /* Bump total instruction counter */
    if ( flagseen ['T'] )
	++tot_instr_count;
#endif

    /* Parse instruction into opcode and operands */
    bzero(args, sizeof(args));
    n_ops = i_scan(textP, args);
    if (n_ops == -1)
	return;

    /* Do "macro substitution" (sort of) on 'ldconst' pseudo-instruction */
    if (!strcmp(args[0],"ldconst"))
    {
	n_ops = parse_ldconst(args);
	if (n_ops == -1)
	    return;
    }

    /* Check for branch-prediction suffix on opcode mnemonic, strip it off */
    n = strlen(args[0]) - 1;
    branch_predict = 0;
    bp_bits = 0;
    if (args[0][n-1] == '.' && (args[0][n] == 't' || args[0][n] == 'f'))
    {
	/* We could check here to see if the target architecture
	 * supports branch prediction, but why bother?  The bit
	 * will just be ignored by processors that don't use it.
	 */
	branch_predict = 1;
	bp_bits = (args[0][n] == 't') ? BP_TAKEN : BP_NOT_TAKEN;
	args[0][n-1] = '\0';	/* Strip suffix from opcode mnemonic */
    }

    /* Look up opcode mnemonic in table and check number of operands.
     * Check that opcode is legal for the target architecture.
     * If all looks good, assemble instruction.
     */
    oP = (struct i960_opcode *) hash_find(op_hash, args[0]);
    if ( ! oP )
    {
	as_bad("Invalid opcode, \"%s\".", args[0]);
	return;
    }

    if ( ! targ_has_instruction(oP) )
    {
	wrong_arch_message("Opcode is not in target architecture: \"%s\".", args[0]);
	if ( ! flagseen['x'] )
	    return;
    } 

    if ( n_ops != oP->num_ops ) 
    {
	as_bad("Improper number of operands.  expecting %d, got %d", oP->num_ops, n_ops);
	return;
    }
	
    switch (oP->format)
    {
    case CTRL:
    case FBRA:
	if ( detect_errata1 )
	{
	    if ( errata1_state == 2 && (eP = (struct i960_errata1 *) hash_find(errata1_hash, args[0])) && eP->format == CTRL )

	    {
		emit(nop_instr);
	    }
	    errata1_state = 0;
	}

	ctrl_fmt(args[1], oP->opcode | bp_bits, oP->num_ops);
	if (oP->format == FBRA)
	{
	    /* Now generate a 'bno' to same arg */
	    ctrl_fmt(args[1], BNO | bp_bits, 1);
	}
	break;

    case COBR:
    case COJ:
	if ( detect_errata1 ) 
	    errata1_state = 0;
	cobr_fmt(args, oP->opcode | bp_bits, oP);
	break;

    case COPR:
        if ( detect_errata1 )
	{
	    if ( (eP = (struct i960_errata1 *) hash_find(errata1_hash, args[0])) && eP->format == REG )
	    {
		errata1_state = 1;
	    }
	    else
	    {
		errata1_state = 0;
	    }
	}
	copr_fmt(args, oP);
	break;

    case REG:
	if ( detect_errata1 )
	{
	    if ( (eP = (struct i960_errata1 *) hash_find(errata1_hash, args[0])) && eP->format == REG )
	    {
		errata1_state = 1;
	    }
	    else
	    {
		errata1_state = 0;
	    }
	}
	if (branch_predict)
	{
	    as_warn(bp_error_msg);
	}
	reg_fmt(args, oP);
	break;
    case MEM1:
    case MEM2:
    case MEM4:
    case MEM8:
    case MEM12:
    case MEM16:
	if ( detect_errata1 )
	{
	    errata1_state = errata1_state == 1 ? 2 : 0;
	}
	if (branch_predict)
	{
	    as_warn(bp_error_msg);
	}
	mem_fmt(args, oP, 0);
	break;

    case CALLJ:
	if ( detect_errata1 )
	    errata1_state = 0;
	if (branch_predict)
	{
	    as_warn(bp_error_msg);
	}
	/* Output opcode & set up "fixup" (relocation);
	 * flag relocation as 'callj' type.
	 */
	know(oP->num_ops == 1);
	get_cdisp(args[1], "CTRL", oP->opcode, 24, 0, 1);
	break;
    case CALLJX:
	if ( detect_errata1 ) errata1_state = 0;
	if (branch_predict)
	{
	    as_warn (bp_error_msg);
	}
	/* Output opcode & set up "fixup" (relocation);
	 * flag relocation as 'calljx' type.
	 */
	know(oP->num_ops == 1);
	mem_fmt(args, oP, 1);
	break;
		
    default:
	BAD_CASE(oP->format);
	break;
    }
} /* md_assemble() */

/*****************************************************************************
 * md_number_to_chars:  convert a number to target byte order
 *
 **************************************************************************** */
void
md_number_to_chars(buf, value, nbytes, big_endian_flag)
    char 	*buf;		/* Put output here */
    long 	value;		/* The integer to be converted */
    int 	nbytes;		/* Number of bytes to output */
    int big_endian_flag;
{
	register long	val = value;
	register int	n = nbytes;

	if (big_endian_flag)
	{       /* put in big-endian order */
		char *cp = buf + n - 1;
		while (n--){
			*cp-- = val;
			val >>= 8;
		}
	} else {
		/* put in little-endian order */
		while (n--){
			*buf++ = val;
			val >>= 8;
		}
	}

	if (val != 0 && val != -1)
	{
		as_bad("The number %d is too large to fit into %d bits.", value, nbytes * 8);
	}
} /* md_number_to_chars() */

/*****************************************************************************
 * md_chars_to_number:  convert from target byte order to host byte order.
 *
 **************************************************************************** */
int
md_chars_to_number(val, n, big_endian_flag)
    unsigned char *val;	/* Value in target byte order */
    int n;		/* Number of bytes in the input */
    int big_endian_flag;
{
	int retval;

	if (big_endian_flag)
	{       /* put in big-endian order */
		for (retval = 0; n--;){
			retval <<= 8;
			retval |= *val++;
		}
	} else {
		/* put in little-endian order */
		for (retval=0; n--;){
			retval <<= 8;
			retval |= val[n];
		}
	}
	return retval;
}


#define MAX_LITTLENUMS	6
#define LNUM_SIZE	sizeof(LITTLENUM_TYPE)

/*****************************************************************************
 * md_atof:	convert ascii to floating point
 *
 * Turn a string at input_line_pointer into a floating point constant of type
 * 'type', and store the appropriate bytes at *litP.  The number of LITTLENUMS
 * emitted is returned at 'sizeP'.  An error message is returned, or a pointer
 * to an empty message if OK.
 *
 * Note we call the i386 floating point routine, rather than complicating
 * things with more files or symbolic links.
 *
 **************************************************************************** */
char * md_atof(type, litP, sizeP)
int type;
char *litP;
int *sizeP;
{
	LITTLENUM_TYPE 	words[MAX_LITTLENUMS];
	LITTLENUM_TYPE 	*wordP;
	int 		prec, i;
	char 		*t;
	char 		*atof_ieee();

	switch(type) {
	case 'f':
	case 'F':
		prec = 2;
		break;

	case 'd':
	case 'D':
		prec = 4;
		break;

	case 't':
	case 'T':
		prec = 5;
		type = 'x';	/* That's what atof_ieee() understands */
		break;

	default:
		*sizeP=0;
		return "Bad call to md_atof()";
	}

	t = atof_ieee(input_line_pointer, type, words);
	if (t){
		input_line_pointer = t;
	}

	*sizeP = prec * LNUM_SIZE;

    if (SEG_IS_BIG_ENDIAN(curr_seg)) {
	/* Output the WORDS with big-endian byte order (words remain
	 * in relative little-endian order)
	 * NOTE: since only the Cx supports big-endian, there should be
	 * no prec == 5 cases. If this occurs, only the first 8 bytes are used.
	 */
	int wordcnt = prec/sizeof(LITTLENUM_TYPE); /* 1 or 2 */
	LITTLENUM_TYPE *sp = &words[(wordcnt-1) * 2];

	if (prec == 5)
	{
		as_fatal("IEEE extended precision not supported in big-endian memory");
		/* no return */
	}

	while (wordcnt--) {
	    md_number_to_chars(litP, (((*sp) << 16) | *(sp+1)), 4, 1);
	    litP += 4;
	    sp -= 2;
	}
    } else {
	/* Output the LITTLENUMs in REVERSE order in accord with i80960
	 * word-order.  (Dunno why atof_ieee doesn't do it in the right
	 * order in the first place -- probably because it's a hack of
	 * atof_m68k.)
	 */

	/* FIXME-SOMEDAY This for loop may be unneeded for big-endian
	 * output. As it stands it should work corectly for either byte
	 * ordering; the only reason to change it would be for performance.
	 */

	for ( wordP = words + prec - 1; wordP >= words; --wordP )
	{
		md_number_to_chars(litP, (long) *wordP, LNUM_SIZE, 0);
		litP += sizeof(LITTLENUM_TYPE);
	}
    }

	/* An asm960 convention we will adopt: pad 80-bit extendeds 
	 * with 16 bits of zero.  This way extendeds can always be
	 * treated as triples.
	 */
	if ( prec == 5 )
	{
		md_number_to_chars(litP, 0L, LNUM_SIZE, 1);
		*sizeP += LNUM_SIZE;
	}

	return "";	/* Someone should teach Dean about null pointers */
}


/*****************************************************************************
 * md_number_to_imm
 *
 **************************************************************************** */
void
md_number_to_imm(buf, val, n, big_endian_flag)
    char *buf;
    long val;
    int n;
    int big_endian_flag;
{
	md_number_to_chars(buf, val, n, big_endian_flag);
}


/*****************************************************************************
 * md_number_to_disp
 *
 **************************************************************************** */
void
md_number_to_disp(buf, val, n)
    char *buf;
    long val;
    int n;
{
	md_number_to_chars(buf, val, n, SEG_IS_BIG_ENDIAN(curr_seg));
}

/*****************************************************************************
 * md_number_to_field:
 *
 *	Stick a value (an address fixup) into a bit field of
 *	previously-generated instruction.
 *
 **************************************************************************** */
static void
md_number_to_field(instrP, val, bfixP, labelname, big_endian_flag)
    char *instrP;	  /* Pointer to instruction to be fixed */
    long val;		  /* Address fixup value */
    bit_fixS *bfixP;	  /* Description of bit field to be fixed up */
    char *labelname;      /* Carried along just for error messages. */
    int  big_endian_flag; /* The data in instrP is big endian. */
{
	int numbits;	/* Length of bit field to be fixed */
	long instr;	/* 32-bit instruction to be fixed-up */
	long sign;	/* 0 or -1, according to sign bit of 'val' */

	/* Convert instruction back to host byte order
	 */
	instr = md_chars_to_number(instrP, 4, big_endian_flag);

	/* Surprise! -- we stored the number of bits
	 * to be modified rather than a pointer to a structure.
	 */
	numbits = (int)bfixP;
	switch ( numbits )
	{
	case 13:
	case 24:
		/* Normal case COBR or CTRL:
		 * Propagate sign bit of 'val' for the given number of bits.
		 * Result should be all 0 or all 1
		 */
		sign = val >> (numbits - 1);
		if ( (val < 0 && sign != -1) || (val > 0 && sign != 0) )
		{
			as_bad("Offset of %d (label name: `%s') too large for field width of %d.", val, labelname, numbits);
			break;
		}
		
		/* Test clobbering the lowest 2 bits */
		if ( val & 0x1 )
			as_warn ("Branch instruction is non-zero in bit 0 (label name: `%s')", labelname);
		if ( val & 0x2 )
			as_warn ("Branch instruction is non-zero in bit 1 (label name: `%s')", labelname);
		break;
	case 1:
		/* This is a no-op, stuck here by reloc_callj() */
		return;
	case 12:
		/* This is a MEMA CTRL:  val is always unsigned.
		 * Just check that val will fit into 12 bits.
		 */
		if ( (val & ~(-1 << numbits)) != val )
			as_bad("Offset of %d (label name: `%s') too large for field width of %d.", val,
			       labelname, numbits);
		break;
	default:
		as_bad("Internal error: Invalid field width to md_number_to_field.");
	}

	/* Put bit field into instruction and write back in target byte order. */
	val &= ~(-1 << (int)numbits);
	instr |= val;
	md_number_to_chars(instrP, instr, 4, big_endian_flag);

} /* md_number_to_field() */


/* Check user's -AXY option.  */
void
md_parse_arch(arg)
char	*arg;
{
	struct tabentry	*tp = arch_tab;

	for ( ; tp->flag != NULL; tp++ )
	{
	    if ( ! strcmp(arg,tp->flag) )
		break;
	}

	if ( tp->flag == NULL )
	{
	    as_bad("Unsupported architecture: %s", arg);
	}
	else
	{
	    architecture = tp->arch;
#ifdef OBJ_ELF
	    elf_arch = tp->elf_arch;
#endif
	    if ( ! strcmp(arg, "CA_DMA") || ! strcmp(arg, "CF_DMA") )
		detect_errata1 = 1;
	}
} 


/* Position-independence switches coming from the command line; 
 * this applies to COFF only but #ifdef'ing them is just too painful.
 * The flags the are set below will be ignored for bout.
 */
void
md_parse_pix(arg)
char	*arg;
{
	if ( strlen(arg) != 1 )
	{
		as_bad ("Unknown pic/pid flag: %s", arg);
		return;
	}
	switch ( *arg )
	{
	case 'c':
		pic_flag = 1;
		break;
	case 'd':
		pid_flag = 1;
		break;
	case 'b':
		pic_flag = 1;
		pid_flag = 1;
		break;
	default:
		as_bad ("Unknown pic/pid flag: %s", arg);
		return;
	}
}
 
/* Errata workaround switches coming from the command line.
 * So far, -j1 is the only one implemented: it means generate nop 
 * instructions to workaround the CA C-Step DMA errata. 
 */
void
md_parse_errata(arg)
    char *arg;
{
    if ( strlen(arg) != 1 )
    {
	as_bad ("Unknown errata flag: %s", arg);
	return;
    }
    switch ( *arg )
    {
    case '1':
	detect_errata1 = 1;
	break;
    default:
	as_bad ("Unknown errata flag: %s", arg);
	return;
    }
}
 


/*****************************************************************************
 * md_convert_frag:
 *	Called by base assembler after address relaxation is finished:  modify
 *	variable fragments according to how much relaxation was done.
 *
 *	If the fragment substate is still 1, a 13-bit displacement was enough
 *	to reach the symbol in question.  Set up an address fixup, but otherwise
 *	leave the cobr instruction alone.
 *
 *	If the fragment substate is 2, a 13-bit displacement was not enough.
 *	Replace the cobr with a two instructions (a compare and a branch).
 *
 **************************************************************************** */
void
md_convert_frag(fragP)
    fragS * fragP;
{
	fixS 	*fixP;	/* Structure describing needed address fix */

	switch (fragP->fr_subtype)
	{
	case 1:
		/* LEAVE SINGLE COBR INSTRUCTION */
		fixP = fix_new(fragP,
			       fragP->fr_opcode-fragP->fr_literal,
			       4,
			       fragP->fr_symbol,
			       0,
			       fragP->fr_offset,
			       1,
			       0);

		fixP->fx_bit_fixP = (bit_fixS *) 13;	/* size of bit field */
		frag_wane(fragP);
		break;
	case 2:
		/* REPLACE COBR WITH COMPARE/BRANCH INSTRUCTIONS */
		relax_cobr(fragP);
		frag_wane(fragP);
		break;
	default:
		BAD_CASE(fragP->fr_subtype);
		break;
	}
}

/*****************************************************************************
 * md_estimate_size_before_relax:  How much does it look like *fragP will grow?
 *
 *	Called by base assembler just before address relaxation.
 *	Return the amount by which the fragment will grow.
 * 	We are here because this frag has type rs_machine_dependent.
 *
 *	Any symbol that is now undefined will not become defined; cobr's
 *	based on undefined symbols will have to be replaced with a compare
 *	instruction and a branch instruction, and the code fragment will grow
 *	by 4 bytes.
 *
 **************************************************************************** */
int
md_estimate_size_before_relax(fragP, segment)
     register fragS *fragP;
     register int   segment;
{
	if ( fragP->fr_subtype > 0 )
	{
		/* This is a COBR-format; potentially a "real" relax.
		 * If symbol is undefined in this segment, go to relaxed state
		 * (compare and branch instructions instead of cobr) right now.
		 */
		if (S_GET_SEGMENT(fragP->fr_symbol) != segment) 
		{
			relax_cobr(fragP);
			return 4;
		}
		else
			return 0;
	}
	else
	{
		/* This is a MEM-format; size will not change. */
		return 0;
	}
} /* md_estimate_size_before_relax() */


#ifdef OBJ_BOUT
/*****************************************************************************
 * md_ri_to_chars:
 *	This routine exists in order to overcome machine byte-order problems
 *	when dealing with bit-field entries in the relocation_info struct.
 *
 *	But relocation info will be used on the host machine only (only
 *	executable code is actually downloaded to the i80960).  Therefore,
 *	we leave it in host byte order.
 *
 *****************************************************************************/
void md_ri_to_chars(ri)
struct reloc_info_generic *ri;
{
	struct relocation_info br;

	(void) bzero(&br, sizeof(br));

	br.r_address = ri->r_address;
	br.r_symbolnum = ri->r_index;
	br.r_pcrel = ri->r_pcrel;
	br.r_length = ri->r_length;
	br.r_extern = ri->r_extern;
	br.r_bsr = ri->r_bsr;
	br.r_disp = ri->r_disp;
	br.r_callj = ri->r_callj;
	br.r_calljx = ri->r_calljx;

	output_file_append((char *) &br, sizeof (struct relocation_info), out_file_name);
} /* md_ri_to_chars() */
#endif

	/*************************************************************
	 *                                                           *
	 *  FOLLOWING ARE THE LOCAL ROUTINES, IN ALPHABETICAL ORDER  *
	 *                                                           *
	 ************************************************************ */



/*****************************************************************************
 * brcnt_emit:	Emit code to increment inline branch counter.
 *
 *	See the comments above the declaration of 'br_cnt' for details on
 *	branch-prediction instrumentation.
 **************************************************************************** */
static void
brcnt_emit()
{
	ctrl_fmt(BR_CNT_FUNC,CALL,1);/* Emit call to "increment" routine */
	emit(0);		/* Emit inline counter to be incremented */
}

/*****************************************************************************
 * brlab_next:	generate the next branch local label
 *
 *	See the comments above the declaration of 'br_cnt' for details on
 *	branch-prediction instrumentation.
 **************************************************************************** */
static char *
brlab_next()
{
	static char buf[20];

	sprintf(buf, "%s%d", BR_LABEL_BASE, br_cnt++);
	return buf;
}

#ifdef INSTRUMENT_BRANCHES

/*****************************************************************************
 * brtab_emit:	generate the fetch-prediction branch table.
 *
 *	See the comments above the declaration of 'br_cnt' for details on
 *	branch-prediction instrumentation.
 *
 *	The code emitted here would be functionally equivalent to the following
 *	example assembler source.
 *
 *			.data
 *			.align	2
 *	   BR_TAB_NAME:
 *			.word	0		# link to next table
 *			.word	3		# length of table
 *			.word	LBRANCH0	# 1st entry in table proper
 *			.word	LBRANCH1
 *			.word	LBRANCH2
 ***************************************************************************** */
void
brtab_emit()
{
	int i;
	char buf[20];
	char *p;		/* Where the binary was output to */
	fixS *fixP;		/*->description of deferred address fixup */

	if ( ! flagseen['b'] )
	{
		return;
	}

	frag_wane (curr_frag);
	seg_change (DEFAULT_DATA_SEGMENT);	/*	.data */
	frag_align(2,0);		/* 	.align 2 */
	record_alignment(curr_seg,2);
	colon(BR_TAB_NAME);		/* BR_TAB_NAME: */
	emit(0);			/* 	.word 0	#link to next table */
	emit(br_cnt);			/*	.word n #length of table */

	for (i=0; i<br_cnt; i++){
		sprintf(buf, "%s%d", BR_LABEL_BASE, i);
		p = emit(0);
		fixP = fix_new(curr_frag,
			       p - curr_frag->fr_literal,
			       4,
			       symbol_find(buf),
			       0,
			       0,
			       0,
			       0);
		fixP->fx_im_disp = 2;		/* 32-bit displacement fix */
	}
}
#endif

/******************************************************************************
 *
 * Support for cc_info data in output file (data used by compiler in 2-pass
 * optimization model).
 *
 ******************************************************************************/

#define CHUNK_SIZE ((32*1024)-32)

struct cc_info_rec {
	struct cc_info_rec * cc_next;
	unsigned short cc_num_bytes;
	unsigned char cc_data[CHUNK_SIZE];
};

static struct cc_info_rec * cc_info_head;
static struct cc_info_rec * cc_info_next;
static long cc_info_sect_size;
static int ignore_cc_info;


/*****************************************************************************
 * cc_info:	Process .ccinfo and .cchead directives
 *
 * ASSUMES:
 * input_line_pointer is pointing at first character after directive.
 * Directive consists of at most a single white-space, followed by ASCII data
 * characters, terminated by new line.  Anything else should give error message.
 * Data characters are either hex digits or other alphas; the latter are either
 * used to represent compressed hex digits (2-digit values) or symbol names.
 *
 **************************************************************************** */
static
void
cc_info( header )
    int header;   /* if zero, this is a ".ccinfo" directive;  else ".cchead" */
{
	int c;
	int c2;
	int i;

	if (cc_info_head == 0){
		/* do initialization */
		cc_info_next = cc_info_head =
		      (struct cc_info_rec *)xmalloc(sizeof(struct cc_info_rec));
		cc_info_next->cc_next = 0;
		cc_info_sect_size = 
			cc_info_next->cc_num_bytes = CI_HEAD_REC_SIZE;
	}

	/* skip the white space */
	if ((c= *input_line_pointer) == '\t' || c == ' ' || c =='\f'){
		input_line_pointer ++;
	}

	i = 0;
	do {
		switch (c){

		case 'G': case 'H': case 'I': case 'J': case 'K':
		case 'L': case 'M': case 'N': case 'O': case 'P':
			/* translated into 0x00 - 0x09 */
			c -= 'G';
			break;

		case 'Q': case 'R': case 'S': case 'T': case 'U':
		case 'V': case 'W': case 'X': case 'Y': case 'Z':
			/* translated into 0xF6 - 0xFF */
			c = (c - 'Q') + 0xF6;
			break;

		case 'a': case 'b': case 'c': case 'd': case 'e':
		case 'f': case 'g': case 'h': case 'i': case 'j':
		case 'k': case 'l': case 'm': case 'n': case 'o':
		case 'p': case 'q': case 'r': case 's': case 't':
		case 'u': case 'v': case 'w': case 'x': case 'y':
		case 'z': case '_': case '.':
			/* These don't have any translation */
			break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			/*
			 * Mod c so falling thru to next case will
			 * create a valid hex value
			 */
			c = (c - '0') + 'A' - 10;
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
			/* get c's hex value */
			c = (c - 'A') + 10;

			/* two bytes of HEX */
			c <<= 4;
			input_line_pointer ++;
			c2 = *input_line_pointer;
			if (c2 >= '0' && c2 <= '9'){
				c |= c2 - '0';
				break;
			} else if (c2 >= 'A' && c2 <= 'F'){
				c |= (c2 - 'A') + 10;
				break;
			}
			/* fall thru to give error */

		default:
			/* reset input line pointer to bad char */
			input_line_pointer --;
			as_warn("bad .%s directive, untranslatable char '%c'.",
			    header ? "cchead" : "ccinfo", *input_line_pointer);
			as_warn ("All .ccinfo and .cchead directives ignored.");
			ignore_cc_info = 1;
			ignore_rest_of_line();
			return;
		}

		if ( header ){
			cc_info_head->cc_data[i++] = c;
		} else {
			if ((unsigned long)cc_info_next->cc_num_bytes >= CHUNK_SIZE){
				cc_info_next->cc_next =
					(struct cc_info_rec *)
					xmalloc(sizeof(struct cc_info_rec));
				cc_info_next = cc_info_next->cc_next;
				cc_info_next->cc_num_bytes = 0;
				cc_info_next->cc_next = 0;
			}
			cc_info_sect_size += 1;
			cc_info_next->cc_data[cc_info_next->cc_num_bytes] = c;
			cc_info_next->cc_num_bytes++; 
		}

		c = *++input_line_pointer;

	} while (c != ';' && c != '\n');

	if ( header && (i != CI_HEAD_REC_SIZE) ){
		as_warn("bad .cchead directive, too many bytes of header.");
		as_warn ("All .ccinfo and .cchead directives ignored.");
		ignore_cc_info = 1;
		ignore_rest_of_line();
		return;
	}

	demand_empty_rest_of_line();
}


long
i960_ccinfo_size()
{
	long retval = ignore_cc_info ? 0 : cc_info_sect_size;

#ifdef OBJ_COFF
	if ( retval > 0 ){
		/* +4 bytes for delimiter: see comment in i960_emit_ccinfo() */
		retval += 4;
	}
#endif
	return retval;
}


void
i960_emit_ccinfo ()
{
	struct cc_info_rec *list_p;
	
	if ( !i960_ccinfo_size() ){
		return;
	}

#ifdef OBJ_COFF
	/* COFF also needs a delimiter word of 0xffffffff between the symbol
	 * table and the cc_info data, so the reader of the file will be able
	 * to tell if there's a string table present without examining the
	 * entire symbol table.
	 */
	{
	static long delimiter = -1;
	output_file_append((char *)&delimiter, 4, out_file_name);
	}
#endif

#ifdef ASM_SET_CCINFO_TIME
	if (cc_info_head)
		set_ccinfo_offset ();
#endif
	for (list_p = cc_info_head; list_p != 0; list_p = list_p->cc_next){
		output_file_append((char *)list_p->cc_data, list_p->cc_num_bytes, out_file_name);
	}
}
 
/*****************************************************************************
 * cobr_fmt:	generate a COBR-format instruction
 *
 **************************************************************************** */
static
void
cobr_fmt(arg, opcode, oP)
    char *arg[];	/* arg[0]->opcode mnemonic, arg[1-3]->operands (ascii) */
    long opcode;	/* Opcode, with branch-prediction bits already set
			 *	if necessary.
			 */
    struct i960_opcode *oP;
			/*->description of instruction */
{
	long instr;		/* 32-bit instruction */
	struct regop regop;	/* Description of register operand */
	int n;			/* Number of operands */
	char *outP;		/* where in the frag the binary code is */
	int var_frag;		/* 1 if varying length code fragment should
				 *	be emitted;  0 if an address fix
				 *	should be emitted.
				 */

	instr = opcode;
	n = oP->num_ops;

	if (n >= 1) {
		/* First operand (if any) of a COBR is always a register
		 * operand.  Parse it.
		 */
		parse_regop(&regop, arg[1], oP->operand[0]);
		instr |= (regop.n << 19) | (regop.mode << 13);
	}
	if (n >= 2) {
		/* Second operand (if any) of a COBR is always a register
		 * operand.  Parse it.
		 */
		parse_regop(&regop, arg[2], oP->operand[1]);
		instr |= (regop.n << 14) | regop.special;
	}


	if (n < 3){
		outP = emit(instr);
		if ( listing_now ) listing_info ( complete, 4, outP, curr_frag, LIST_BIGENDIAN );
	} else {
		if ( flagseen['b'] )
		{
			brcnt_emit();
			colon(brlab_next());
		}

		/* A third operand to a COBR is always a displacement.
		 * Parse it; if it's relaxable (a cobr "j" directive, or any
		 * cobr other than bbs/bbc when the -n "no relax" option is 
		 * not in use) set up a variable code fragment;  otherwise 
		 * set up an address fix.
		 */
		var_frag = (! flagseen['n']) || (oP->format == COJ); /* TRUE or FALSE */
		get_cdisp(arg[3], "COBR", instr, 13, var_frag, 0);

		if ( flagseen['b'] )
		{
			brcnt_emit();
		}
	}
} /* cobr_fmt() */


/*****************************************************************************
 * ctrl_fmt:	generate a CTRL-format instruction
 *
 **************************************************************************** */
static
void
ctrl_fmt(targP, opcode, num_ops)
    char *targP;	/* Pointer to text of lone operand (if any) */
    long opcode;	/* Template of instruction */
    int num_ops;	/* Number of operands */
{
	int instrument;	/* TRUE iff we should add instrumentation to track
			 * how often the branch is taken
			 */
	char *outP;	/* where is the binary code in the frag? */

	if (num_ops == 0){
		outP = emit(opcode);		/* Output opcode */
		if ( listing_now ) listing_info ( complete, 4, outP, curr_frag, LIST_BIGENDIAN );
	} else {

		instrument = flagseen['b'] && (opcode!=CALL)
			&& (opcode!=B) && (opcode!=RET) && (opcode!=BAL);

		if (instrument){
			brcnt_emit();
			colon(brlab_next());
		}

		/* The operand MUST be an ip-relative displacment. Parse it
		 * and set up address fix for the instruction we just output.
		 */
		get_cdisp(targP, "CTRL", opcode, 24, 0, 0);

		if (instrument){
			brcnt_emit();
		}
	}

}


/*****************************************************************************
 * emit:	output instruction binary
 *
 *	Output instruction binary, in target byte order, 4 bytes at a time.
 *	Return pointer to where it was placed.
 *
 **************************************************************************** */
static
char *
emit(instr)
    long instr;		/* Word to be output, host byte order */
{
	char *toP;	/* Where to output it */

	toP = frag_more(4);			/* Allocate storage */
	md_number_to_chars(toP, instr, 4,       /* Convert to target byte order */
			   SEG_IS_BIG_ENDIAN(curr_seg));
	return toP;
}


/*****************************************************************************
 * get_args:	break individual arguments out of comma-separated list
 *
 * Input assumptions:
 *	- all comments and labels have been removed
 *	- all strings of whitespace have been collapsed to a single blank.
 *	- all character constants ('x') have been replaced with decimal
 *
 * Output:
 *	args[0] is untouched. args[1] points to first operand, etc. All args:
 *	- are NULL-terminated
 *	- contain no whitespace
 *
 * Return value:
 *	Number of operands (0,1,2, or 3) or -1 on error.
 *
 **************************************************************************** */
static int get_args(p, args)
    register char *p;	/* Pointer to comma-separated operands; MUCKED BY US */
    char *args[];	/* Output arg: pointers to operands placed in args[1-3].
			 * MUST ACCOMMODATE 4 ENTRIES (args[0-3]).
			 */
{
	register int n;		/* Number of operands */
	register char *to;

	/* Skip lead white space */
	while (*p == ' '){
		p++;
	}

	if (*p == '\0'){
		return 0;
	}

	n = 1;
	args[1] = p;

	/* Squeze blanks out by moving non-blanks toward start of string.
	 * Isolate operands, whenever comma is found.
	 */
	to = p;
	while (*p != '\0'){

		if (*p == ' '){
			p++;

		} else if (*p == ','){

			/* Start of operand */
			if (n == MAX_OPS){
				as_bad("too many operands");
				return -1;
			}
			*to++ = '\0';	/* Terminate argument */
			args[++n] = to;	/* Start next argument */
			p++;

		} else {
			*to++ = *p++;
		}
	}
	*to = '\0';
	return n;
}


/*****************************************************************************
 * get_cdisp:	handle displacement for a COBR or CTRL instruction.
 *
 *	Parse displacement for a COBR or CTRL instruction.
 *
 *	If successful, output the instruction opcode and set up for it,
 *	depending on the arg 'var_frag', either:
 *	    o an address fixup to be done when all symbol values are known, or
 *	    o a varying length code fragment, with address fixup info.  This
 *		will be done for cobr instructions that may have to be relaxed
 *		in to compare/branch instructions (8 bytes) if the final address
 *		displacement is greater than 13 bits.
 *
 **************************************************************************** */
static
void
get_cdisp(dispP, ifmtP, instr, numbits, var_frag, callj)
    char *dispP;	/*->displacement as specified in source instruction */
    char *ifmtP;	/*->"COBR" or "CTRL" (for use in error message) */
    long instr;		/* Instruction needing the displacement */
    int numbits;	/* # bits of displacement (13 for COBR, 24 for CTRL) */
    int var_frag;	/* 1 if varying length code fragment should be emitted;
			 *	0 if an address fix should be emitted.
			 */
    int callj;		/* 1 == callj relocation should be done; else 0 */
{
	expressionS e;	/* Parsed expression */
	fixS *fixP;	/* Structure describing needed address fix */
	char *outP;	/* Where instruction binary is output to */

	fixP = NULL;

	switch ( parse_expr(dispP,&e) )
	{
	case SEG_GOOF:
		as_bad("expression syntax error");
		break;

	case SEG_TEXT:
	case SEG_DATA:
	case SEG_BSS:
	case SEG_UNKNOWN:
		if (var_frag) 
		{
			outP = frag_more(8);	/* Allocate worst-case storage */
			md_number_to_chars(outP, instr, 4, SEG_IS_BIG_ENDIAN(curr_seg));
			frag_variant(rs_machine_dependent, 4, 4, 1,
						e_adds(e), e_offs(e), outP, 0, 0);
			if ( listing_now ) listing_info ( varying, 8, outP, curr_frag->fr_prev, LIST_BIGENDIAN );
		} else 
		{
			/* Set up a new fix structure, so address can be updated
			 * when all symbol values are known.
			 */
			outP = emit(instr);
			fixP = fix_new(curr_frag,
				       outP - curr_frag->fr_literal,
				       4,
				       e_adds(e),
				       0,
				       e_offs(e),
				       1,
				       0);

			/* Set up callj relocation if requested, but prevent 
			   callj relocation for "callj foo+12" */
			if ( callj && e_offs(e) == 0 )
			{
			    fixP->fx_callj = 1;
			}
			
			/* We want to modify a bit field when the address is
			 * known.  But we don't need all the garbage in the
			 * bit_fix structure.  So we're going to lie and store
			 * the number of bits affected instead of a pointer.
			 */
			fixP->fx_bit_fixP = (bit_fixS *) numbits;
			if ( listing_now ) listing_info ( complete, 4, outP, curr_frag, LIST_BIGENDIAN );
		}
		break;

	default:
		as_bad("target of %s instruction must be a label", ifmtP);
		break;
	}
}


/*****************************************************************************
 * get_ispec:	parse a memory operand for an index specification
 *
 *	Here, an "index specification" is taken to be anything surrounded
 *	by square brackets and NOT followed by anything else.
 *
 *	If it's found, detach it from the input string, remove the surrounding
 *	square brackets, and return a pointer to it.  Otherwise, return NULL.
 *
 **************************************************************************** */
static
char *
get_ispec(textP)
    char *textP; /*->memory operand from source instruction, no white space */
{
	char *start;	/*->start of index specification */
	char *end;	/*->end of index specification */

	/* Find opening square bracket, if any
	 */
	start = index(textP, '[');

	if (start != NULL){

		/* Eliminate '[', detach from rest of operand */
		*start++ = '\0';

		end = index(start, ']');

		if (end == NULL){
			as_bad("unmatched '['");

		} else {
			/* Eliminate ']' and make sure it was the last thing
			 * in the string.
			 */
			*end = '\0';
			if (*(end+1) != '\0'){
				as_bad("garbage after index spec");
			}
		}
	}
	return start;
}

/*****************************************************************************
 * get_regnum:
 *
 *	Look up a (suspected) register name in the register table and return
 *	the associated register number (or -1 if not found).  
 *	When parameter "check_aliases" is 1, then on a bad match,
 *	try again with one of the aliased register names (r0 - r2, g15) so 
 *	that we can give the user some coherent information about why the
 *	register name was not found.  If an alias is found, call as_bad to
 *	set an error status, but return the register's alias number so that
 *	processing can continue.
 *
 *****************************************************************************/
static
int
get_regnum(regname, check_aliases)
    char *regname;	/* Suspected register name */
    int check_aliases;  /* Try again for r0, r1, r2, g15 */
{
	int *rP;
	struct i960_register *reg;

	if ( (rP = (int *) hash_find(reg_hash, regname)) == NULL )
	{
		/* Not a valid register name */
		if ( check_aliases )
			/* try the list of registers with aliases */
			for ( reg = illegalregs; reg->reg_name; ++reg )
				if ( ! strcmp(regname, reg->reg_name) )
				{
					as_bad("Illegal register name: %s.  Use %s instead.", regname, regnames[reg->reg_num].reg_name);
					return reg->reg_num;
				}
	}

	return (rP == NULL) ? -1 : *rP;
}


/*****************************************************************************
 * i_scan:	perform lexical scan of ascii assembler instruction.
 *
 * Input assumptions:
 *	- input string is an i80960 instruction (not a pseudo-op)
 *	- all comments and labels have been removed
 *	- all strings of whitespace have been collapsed to a single blank.
 *
 * Output:
 *	args[0] points to opcode, other entries point to operands. All strings:
 *	- are NULL-terminated
 *	- contain no whitespace
 *	- have character constants ('x') replaced with a decimal number
 *
 * Return value:
 *	Number of operands (0,1,2, or 3) or -1 on error.
 *
 **************************************************************************** */
static int i_scan(iP, args)
    register char *iP;	/* Pointer to ascii instruction;  MUCKED BY US. */
    char *args[];	/* Output arg: pointers to opcode and operands placed
			 *	here.  MUST ACCOMMODATE 4 ENTRIES.
			 */
{

	/* Isolate opcode.  Skip lead space, if any */
	if ( *iP == ' ' ) 
		iP++;

	args[0] = iP;
	for ( ; *iP && *iP != ' ' && *iP != '('; iP++ ) 
		;

	switch ( *iP )
	{
	case ' ':
		/* Normal case; there are one or more arguments */
		*iP++ = '\0';	/* Terminate opcode */
		return(get_args(iP, args));

	case '\0':
		/* There are no operands */
		if (args[0] == iP) 
		{
			/* We never moved: there was no opcode either! */
			as_bad("missing opcode");
			return -1;
		}
		return 0;

	case '(':
		/* Special case; no space after opcode; register indirect operand */
	{
		char	*opcode = xmalloc(iP - args[0] + 1);
		strncpy(opcode, args[0], iP - args[0]);
		opcode[iP - args[0]] = '\0';
		args[0] = opcode;
		return(get_args(iP, args));
	}
		
	}
} /* i_scan() */


/*****************************************************************************
 * mem_fmt:	generate a MEMA- or MEMB-format instruction
 *
 **************************************************************************** */
static void mem_fmt(args, oP, calljx)
    char *args[];	/* args[0]->opcode mnemonic, args[1-3]->operands */
    struct i960_opcode *oP; /* Pointer to description of instruction */
    int calljx;		/* flag, 1 = special calljx pseudo-instruction */
{
	int i;			/* Loop counter */
	struct regop regop;	/* Description of register operand */
	char opdesc;		/* Operand descriptor byte */
	memS instr;		/* Description of binary to be output */
	char *out1;		/* Where the binary was output to: word1 */
	char *out2;		/* Where the binary was output to: word2 */
	expressionS expr;	/* Parsed expression */
	fixS *fixP;		/*->description of deferred address fixup */
	segT	seg;		/* Returned by expression() */

#ifdef  DEBUG
	/* Bump count for any mem-type instruction */
	if ( flagseen ['T'] )
		++mem_instr_count;
#endif

	bzero(&instr, sizeof(memS));
	instr.opcode = oP->opcode;

	/* Process operands. */
	for (i = 1; i <= oP->num_ops; i++)
	{
		opdesc = oP->operand[i-1];
		if (MEMOP(opdesc))
		{
			if ( calljx )
				/* Undo the CALLJX opcode format hack */
				parse_memop(&instr, args[i], MEM1);
			else
				parse_memop(&instr, args[i], oP->format);
		} 
		else 
		{
			parse_regop(&regop, args[i], opdesc);
			instr.opcode |= regop.n << 19;
		}
	}

	if (instr.disp == 0)
	{
	    	out1 = emit(instr.opcode);
		if ( listing_now ) listing_info ( complete, 4, out1, curr_frag, LIST_BIGENDIAN );
		return;
	}

	/* Parse and process the displacement */
	seg = parse_expr(instr.e,&expr);

	switch ( seg )
	{
	case SEG_GOOF:
		as_bad("expression syntax error");
		break;

	case SEG_ABSENT:
		/* Treat an empty expression (displacement) as an absolute 0 */
		e_offs(expr) = 0;
		e_segs(expr) = SEG_ABSOLUTE;

		/* Intentional fallthrough */

	case SEG_ABSOLUTE:
		if (instr.disp == 32)
		{
			out1 = emit(instr.opcode);
			out2 = emit(e_offs(expr));	/* Output displacement */
			if ( listing_now ) listing_info ( complete, 8, out1, curr_frag, LIST_BIGENDIAN );
		}
		else 
		{
			/* 12-bit displacement */
			if (e_offs(expr) & ~0xfff)
			{
				/* Won't fit in 12 bits: output a MEMA then
				 * convert to MEMB (FIXME-SOMEDAY: this
				 * 2-step method is no longer required.)
				 */
				out1 = emit(instr.opcode);
				mema_to_memb(out1,curr_frag->fr_big_endian);
				out2 = emit(e_offs(expr));
				if ( listing_now ) listing_info ( complete, 8, out1, curr_frag, LIST_BIGENDIAN );
			} 
			else 
			{
				/* WILL fit in 12 bits:  OR offset into the 
				 * opcode.
				 */
				instr.opcode |= e_offs(expr);
				out1 = emit(instr.opcode);
				if ( listing_now ) listing_info ( complete, 4, out1, curr_frag, LIST_BIGENDIAN );
			}
		}
		break;

	case SEG_TEXT:
	case SEG_DATA:
	case SEG_BSS:
	case SEG_UNKNOWN:
		if ( instr.disp == 12 && ! calljx && ! flagseen ['M'] )
		{
		    /* Displacement is dependent on a symbol.  Choose MEMA
		       if the symbol was previously declared lomem, else
		       fall through into SEG_DIFFERENCE code and use MEMB. */
		    if (e_adds(expr) && SF_GET_LOMEM(e_adds(expr)))
		    {
			/* Lomem symbol.  Generate MEMA-format instruction
			   with RELSHORT relocation */
			out1 = emit(instr.opcode);
			fixP = fix_new(curr_frag,
				       out1 - curr_frag->fr_literal,
				       0,  /* 0 == RELSHORT */
				       e_adds(expr),
				       NULL,
				       e_offs(expr),
				       0,
				       0);
		
			/* Store size of bit field */
			fixP->fx_bit_fixP = (bit_fixS *) 12;
		
			if ( listing_now ) 
			    listing_info ( complete, 4, out1, curr_frag, LIST_BIGENDIAN );
			return;
		    }
		}

		/* Intentional fallthrough; works if EITHER calljx or -M is 
		   true; and in any case we want to forego mema optimization
		   for SEG_DIFFERENCE.
		 */
	case SEG_DIFFERENCE:
		if ( instr.disp == 12 )
		{
			/* Option 'M' was given, OR calljx was given OR this is a
			 * a SEG_DIFFERENCE: suppress MEMA optimization 
			 * (FIXME-SOMEDAY: the 2-step method is no longer required.)
			 */
			out1 = emit(instr.opcode);
			mema_to_memb(out1,curr_frag->fr_big_endian);
			out2 = emit(0L);
		}
		else
		{
			out1 = emit (instr.opcode);
			out2 = emit(0L);
		}

		fixP = fix_new(curr_frag,
			       out2 - curr_frag->fr_literal,
			       4,
			       e_adds(expr),
			       e_subs(expr),
			       e_offs(expr),
			       0,
			       0);
		fixP->fx_im_disp = 2;		/* 32-bit displacement fix */
		
		/* Set up calljx relocation if requested, but prevent 
		   calljx relocation for "calljx foo+12" */
		if ( calljx && e_offs(expr) == 0 )
		{
		    fixP->fx_calljx = 1;
		}

		if ( listing_now ) listing_info ( complete, 8, out1, curr_frag, LIST_BIGENDIAN );
		break;

	default:
		as_bad ("Illegal expression: %s", instr.e);
		break;
	}
} /* mem_fmt() */


/*****************************************************************************
 * mema_to_memb:	convert a MEMA-format opcode to a MEMB-format opcode.
 *
 * There are 2 possible MEMA formats:
 *	- displacement only
 *	- displacement + abase
 *
 * They are distinguished by the setting of the MEMA_ABASE bit.
 *
 **************************************************************************** */
static void mema_to_memb(opcodeP,big_endian_flag)
    char *opcodeP;	/* Where to find the opcode, in target byte order */
    int big_endian_flag;
{
	long opcode;	/* Opcode in host byte order */
	long mode;	/* Mode bits for MEMB instruction */

#ifdef  DEBUG
	/* Bump count for mema to memb conversion */
	if ( flagseen ['T'] )
		++mema_to_memb_count;
#endif

	opcode = md_chars_to_number(opcodeP, 4, big_endian_flag);
	know(!(opcode & MEMB_BIT));

	mode = MEMB_BIT | D_BIT;
	if (opcode & MEMA_ABASE){
		mode |= A_BIT;
	}

	opcode &= 0xffffc000;	/* Clear MEMA offset and mode bits */
	opcode |= mode;		/* Set MEMB mode bits */

	md_number_to_chars(opcodeP, opcode, 4, big_endian_flag);
} /* mema_to_memb() */


/*****************************************************************************
 * parse_expr:		parse an expression
 *
 *	Use base assembler's expression parser to parse an expression.
 *	It, unfortunately, runs off a global which we have to save/restore
 *	in order to make it work for us.
 *
 *	Return "segment" to which the expression evaluates.
 *	Return SEG_GOOF regardless of expression evaluation if entire input
 *	string is not consumed in the evaluation -- tolerate no dangling junk!
 *
 **************************************************************************** */
static
segT
parse_expr(textP, expP)
    char *textP;	/* Text of expression to be parsed */
    expressionS *expP;	/* Where to put the results of parsing */
{
	char *save_in;	/* Save global here */
	segT seg;	/* Segment to which expression evaluates */
	symbolS *symP;

	know(textP);

	save_in = input_line_pointer;	/* Save global */
	input_line_pointer = textP;	/* Make parser work for us */
	/* Workaround for local labels getting confused with
	 * floating-point constants; but FP constants are illegal 
	 * in MEM-format instructions.
	 */
	force_local_label = 1;
	seg = expression(expP);
	force_local_label = 0;
	
	if (input_line_pointer - textP != strlen(textP)) {
		/* Did not consume all of the input */
		seg = SEG_GOOF;
	}
	symP = expP->X_add_symbol;
	if (symP && (hash_find(reg_hash, S_GET_NAME(symP)))) {
		/* Register name in an expression */
		seg = SEG_GOOF;
	}
	
	input_line_pointer = save_in;	/* Restore global */
	return seg;
}


/*****************************************************************************
 * parse_ldcont:
 *	Parse and replace a 'ldconst' pseudo-instruction with an appropriate
 *	i80960 instruction.
 *
 *	Assumes the input consists of:
 *		arg[0]	opcode mnemonic ('ldconst')
 *		arg[1]  first operand (constant)
 *		arg[2]	name of register to be loaded
 *
 *	Replaces opcode and/or operands as appropriate.
 *
 *	Returns the new number of arguments, or -1 on failure.
 *
 **************************************************************************** */
static
int
parse_ldconst(arg)
    char *arg[];	/* See above */
{
	int n;			/* Constant to be loaded */
	int shift;		/* Shift count for "shlo" instruction */
	static char buf[5];	/* Literal for first operand */
	static char buf2[5];	/* Literal for second operand */
	expressionS e;		/* Parsed expression */


	if (!arg[2]) {
	    as_bad("ldconst requires destination register.");
	    return -1;
	}

	arg[3] = NULL;	/* So we can tell at the end if it got used or not */

	switch(parse_expr(arg[1],&e)){

	case SEG_TEXT:
	case SEG_DATA:
	case SEG_BSS:
	case SEG_UNKNOWN:
	case SEG_DIFFERENCE:
		/* We're dependent on one or more symbols -- use "lda" */
		arg[0] = "lda";
		break;

	case SEG_ABSOLUTE:
		/* Try the following mappings:
		 *	ldconst 0,<reg>  ->mov  0,<reg>
		 * 	ldconst 31,<reg> ->mov  31,<reg>
		 * 	ldconst 32,<reg> ->addo 1,31,<reg>
		 * 	ldconst 62,<reg> ->addo 31,31,<reg>
  		 *	ldconst 64,<reg> ->shlo 8,3,<reg>
		 * 	ldconst -1,<reg> ->subo 1,0,<reg>
		 * 	ldconst -31,<reg>->subo 31,0,<reg>
		 *
		 * anthing else becomes:
		 * 	lda xxx,<reg>
		 */
		n = e_offs(e);
		if ((0 <= n) && (n <= 31)){
			arg[0] = "mov";

		} else if ((-31 <= n) && (n <= -1)){
			arg[0] = "subo";
			arg[3] = arg[2];
			sprintf(buf, "%d", -n);
			arg[1] = buf;
			arg[2] = "0";

		} else if ((32 <= n) && (n <= 62)){
			arg[0] = "addo";
			arg[3] = arg[2];
			arg[1] = "31";
			sprintf(buf, "%d", n-31);
			arg[2] = buf;

		} else if ((shift = shift_ok(n)) != 0){
			arg[0] = "shlo";
			arg[3] = arg[2];
			sprintf(buf, "%d", shift);
			arg[1] = buf;
			sprintf(buf2, "%d", n >> shift);
			arg[2] = buf2;

		} else {
			arg[0] = "lda";
		}
		break;

	default:
		as_bad("invalid constant");
		return -1;
		break;
	}
	return (arg[3] == 0) ? 2: 3;
}

/*****************************************************************************
 * parse_memop:	parse a memory operand
 *
 *	This routine is based on the observation that the 4 mode bits of the
 *	MEMB format, taken individually, have fairly consistent meaning:
 *
 *		 M3 (bit 13): 1 if displacement is present (D_BIT)
 *		 M2 (bit 12): 1 for MEMB instructions (MEMB_BIT)
 *		 M1 (bit 11): 1 if index is present (I_BIT)
 *		 M0 (bit 10): 1 if abase is present (A_BIT)
 *
 *	So we parse the memory operand and set bits in the mode as we find
 *	things.  Then at the end, if we go to MEMB format, we need only set
 *	the MEMB bit (M2) and our mode is built for us.
 *
 *	Unfortunately, I said "fairly consistent".  The exceptions:
 *
 *		 DBIA
 *		 0100	Would seem illegal, but means "abase-only".
 *
 *		 0101	Would seem to mean "abase-only" -- it means IP-relative.
 *			Must be converted to 0100.
 *
 *		 0110	Would seem to mean "index-only", but is reserved.
 *			We turn on the D bit and provide a 0 displacement.
 *
 *	The other thing to observe is that we parse from the right, peeling
 *	things * off as we go:  first any index spec, then any abase, then
 *	the displacement.
 *
 **************************************************************************** */
static
void
parse_memop(memP, argP, optype)
    memS *memP;	/* Where to put the results */
    char *argP;	/* Text of the operand to be parsed */
    int optype;	/* MEM1, MEM2, MEM4, MEM8, MEM12, or MEM16 */
{
	char *indexP;	/* Pointer to index specification with "[]" removed */
	char *p;	/* Temp char pointer */
	char iprel_flag;/* True if this is an IP-relative operand */
	int regnum;	/* Register number */
	int scale;	/* Scale factor: 1,2,4,8, or 16.  Later converted
			 *	to internal format (0,1,2,3,4 respectively).
			 */
	int mode; 	/* MEMB mode bits */
	int *intP;	/* Pointer to register number */

	/* The following table contains the default scale factors for each
	 * type of memory instruction.  It is accessed using (optype-MEM1)
	 * as an index -- thus it assumes the 'optype' constants are assigned
	 * consecutive values, in the order they appear in this table
	 */
	static int def_scale[] = {
		1,	/* MEM1 */
		2,	/* MEM2 */
		4, 	/* MEM4 */
		8,	/* MEM8 */
		-1,	/* MEM12 -- no valid default */
		16 	/* MEM16 */
	};


	iprel_flag = mode = 0;

	/* Any index present? */
	indexP = get_ispec(argP);
	if (indexP) {
		p = index(indexP, '*');
		if (p == NULL) {
			/* No explicit scale -- use default for this
			 *instruction type.
			 */
			scale = def_scale[ optype - MEM1 ];
		} else {
			*p++ = '\0';	/* Eliminate '*' */

			/* Now indexP->a '\0'-terminated register name,
			 * and p->a scale factor.
			 */

			if (!strcmp(p,"16")){
				scale = 16;
			} else if (index("1248",*p) && (p[1] == '\0')){
				scale = *p - '0';
			} else {
				scale = -1;
			}
		}

		regnum = get_regnum(indexP, 1);		/* Get index reg. # */
		if (!IS_RG_REG(regnum)){
			as_bad("invalid index register");
			return;
		}

		/* Convert scale to its binary encoding */
		switch (scale){
		case  1: scale = 0 << 7; break;
		case  2: scale = 1 << 7; break;
		case  4: scale = 2 << 7; break;
		case  8: scale = 3 << 7; break;
		case 16: scale = 4 << 7; break;
		default: as_bad("invalid scale factor"); return;
		};

		memP->opcode |= scale | regnum;	 /* Set index bits in opcode */
		mode |= I_BIT;			/* Found a valid index spec */
	}

	/* Any abase (Register Indirect) specification present? */
	if ((p = rindex(argP,'(')) != NULL) {
		/* "(" is there -- does it start a legal abase spec?
		 * (If not it could be part of a displacement expression.)
		 */
		intP = (int *) hash_find(areg_hash, p);
		if (intP != NULL){
			/* Got an abase here */
			regnum = *intP;
			*p = '\0';	/* discard register spec */
			if (regnum == IPREL){
				/* We have to specialcase ip-rel mode */
				iprel_flag = 1;
			} else {
				memP->opcode |= regnum << 14;
				mode |= A_BIT;
			}
		}
	}

	/* Any expression present? */
	memP->e = argP;
	if (*argP != '\0'){
		mode |= D_BIT;
	}

	/* Special-case ip-relative addressing */
	if (iprel_flag){
		if (mode & I_BIT){
			syntax();
		} else {
			memP->opcode |= 5 << 10;	/* IP-relative mode */
			memP->disp = 32;
		}
		return;
	}

	/* Handle all other modes */
	switch (mode){
	case D_BIT | A_BIT:
		/* Go with MEMA instruction format for now (grow to MEMB later
		 *	if 12 bits is not enough for the displacement).
		 * MEMA format has a single mode bit: set it to indicate
		 *	that abase is present.
		 */
		memP->opcode |= MEMA_ABASE;
		memP->disp = 12;
		break;

	case D_BIT:
		/* Go with MEMA instruction format for now (grow to MEMB later
		 *	if 12 bits is not enough for the displacement).
		 */
		memP->disp = 12;
		break;

	case A_BIT:
		/* For some reason, the bit string for this mode is not
		 * consistent:  it should be 0 (exclusive of the MEMB bit),
		 * so we set it "by hand" here.
		 */
		memP->opcode |= MEMB_BIT;
		break;

	case A_BIT | I_BIT:
		/* set MEMB bit in mode, and OR in mode bits */
		memP->opcode |= mode | MEMB_BIT;
		break;

	case I_BIT:
		/* Treat missing displacement as displacement of 0 */
		mode |= D_BIT;
		/***********************
		 * Fall into next case *
		 ********************** */
	case D_BIT | A_BIT | I_BIT:
	case D_BIT | I_BIT:
		/* set MEMB bit in mode, and OR in mode bits */
		memP->opcode |= mode | MEMB_BIT;
		memP->disp = 32;
		break;

	default:
		syntax();
		break;
	}
}

/*****************************************************************************
 * parse_po:	parse machine-dependent pseudo-op
 *
 *	This is a top-level routine for machine-dependent pseudo-ops.  It slurps
 *	up the rest of the input line, breaks out the individual arguments,
 *	and dispatches them to the correct handler.
 **************************************************************************** */
static
void
parse_po(po_num)
    int po_num;	 /* Pseudo-op number:  currently S_LEAFPROC or S_SYSPROC */
{
	char *args[4];	/* Pointers operands, with no embedded whitespace.
			 *	arg[0] unused.
			 *	arg[1-3]->operands
			 */
	int n_ops;	/* Number of operands */
	char *p;	/* Pointer to beginning of unparsed argument string */
	char eol;	/* Character that indicated end of line */

	extern char is_end_of_line[];

	/* Advance input pointer to end of line. */
	p = input_line_pointer;
	while (!is_end_of_line[ *input_line_pointer ]){
		input_line_pointer++;
	}
	eol = *input_line_pointer;	/* Save end-of-line char */
	*input_line_pointer = '\0';  	/* Terminate argument list */

	/* Parse out operands */
	n_ops = get_args(p, args);
	if (n_ops == -1){
		return;
	}

	/* Dispatch to correct handler */
	switch(po_num){
	case S_SYSPROC:		
		s_sysproc(n_ops, args);
		break;
	case S_LEAFPROC:	
		s_leafproc(n_ops, args);
		break;
	case S_PIC:
	case S_PID:
	case S_LINKPIX:
		s_mark_pixfile(po_num);
		break;
	default:
		BAD_CASE(po_num);
		break;
	}

	/* Restore eol, so line numbers get updated correctly.  Base assembler
	 * assumes we leave input pointer pointing at char following the eol.
	 */
	*input_line_pointer++ = eol;
}

/*****************************************************************************
 * parse_regop: parse a register operand.
 *
 *	In case of illegal operand, issue a message and return some valid
 *	information so instruction processing can continue.
 **************************************************************************** */
static
void
parse_regop(regopP, optext, opdesc)
    struct regop *regopP; /* Where to put description of register operand */
    char *optext;	/* Text of operand */
    char opdesc;	/* Descriptor byte:  what's legal for this operand */
{
	int n;		/* Register number */
	expressionS e;	/* Parsed expression */

	/* See if operand is a register */
	if ( (n = get_regnum(optext, 0)) >= 0 )
	{
		if (IS_RG_REG(n) && REG_OK(opdesc)){
			/* global or local register */
			if (!REG_ALIGN(opdesc,n)){
				as_warn("unaligned register");
			}
			regopP->n = n;
			regopP->mode = 0;
			regopP->special = 0;
			return;
		} else if (IS_FP_REG(n) && FP_OK(opdesc)){
			/* Floating point register, and it's allowed */
			regopP->n = n - FP0;
			regopP->mode = 1;
			regopP->special = 0;
			return;
		} else if (IS_SF_REG(n) && SFR_OK(opdesc)){
			/* Special-function register, and it's allowed */
			if (!REG_ALIGN(opdesc,n - SF0)){
				as_warn("unaligned register");
			}
			regopP->n = n - SF0;
			regopP->mode = 0;
			regopP->special = 1;
			if (!targ_has_sfr(regopP->n)){
				wrong_arch_message("no such sfr in this architecture");
			}
			return;
		}
	} 
	else if (LIT_OK(opdesc))
	{
		/* How about a literal? */
		regopP->mode = 1;
		regopP->special = 0;
		if (FP_OK(opdesc))
		{ 	/* floating point literal acceptable */
                        /* Skip over 0f, 0d, or 0e prefix */
                        if ( (optext[0] == '0')
                             && (optext[1] >= 'd')
                             && (optext[1] <= 'f') ){
                                optext += 2;
                        }

                        if (!strcmp(optext,"0.0") || !strcmp(optext,"0") ){
                                regopP->n = 0x10;
                                return;
                        }
                        if (!strcmp(optext,"1.0") || !strcmp(optext,"1") ){
                                regopP->n = 0x16;
                                return;
                        }

		} 
		else 
		{
			/* fixed point literal acceptable */
			if ( parse_expr(optext,&e) == SEG_ABSOLUTE 
			    && e_offs(e) >= 0 
			    && e_offs(e) <= 31 )
			{
				regopP->n = e_offs(e);
				return;
			}
		}
	}
	else if (LIT12_OK(opdesc))
	{
		/* How about an 8 bit literal */
		regopP->mode = 1;
		regopP->special = 0;
		if ( parse_expr(optext,&e) == SEG_ABSOLUTE
			&& e_offs(e) >= 0
			&& e_offs(e) < 0x1000)
		{
			regopP->n = e_offs(e);
			return;
		}
	}

	/* Fall through and try one more time for a register alias */
	n = get_regnum(optext, 1);
	if (IS_RG_REG(n) && REG_OK(opdesc))
	{
		/* global or local register */
		if (!REG_ALIGN(opdesc,n))
			as_warn("unaligned register");
		regopP->n = n;
		regopP->mode = 0;
		regopP->special = 0;
		return;
	}	

	/* Nothing worked */
	syntax();
	regopP->mode = 0;	/* Register r0 is always a good one */
	regopP->n = 0;
	regopP->special = 0;
} /* parse_regop() */

/*****************************************************************************
 * copr_fmt:	generate a coprocessor format instruction
 *
 * this is essentially a reg format instruction with some special fields.
 *
 *****************************************************************************/
static void copr_fmt(args, oP)
    char *args[];
    struct i960_opcode *oP;
{
	long 		instr;	/* Binary to be output */
	struct regop 	regop;	/* Description of register operand */
	int 		n_ops;	/* Number of operands */
	char 		*buf;	/* Where within the frag the instr lives */
	unsigned	cp_func;

	instr = oP->opcode;
	n_ops = oP->num_ops;

	assert(n_ops == 4);
        parse_regop(&regop, args[1], oP->operand[0]);

        cp_func = regop.n;
        /* check for valid ranges of coprocessor function code.
         * 0x000 - 0x4ff   unallowed
         * 0x500 - 0x57f   allowed
         * 0x580 - 0x70f   unallowed
         * 0x710 - 0x73f   allowed
         * 0x740 - 0x74f   unallowed
         * 0x750 - 0x77f   allowed
         * 0x780 - 0xfff   unallowed
         */
        if (!((cp_func >= 0x500 && cp_func <= 0x57f) ||
              (cp_func >= 0x710 && cp_func <= 0x73f) ||
              (cp_func >= 0x750 && cp_func <= 0x77f)))
          as_bad("invalid coprocessor function field");
        instr |= (cp_func & 0xff0) << 20;
	instr |= (cp_func & 0xf)  << 7;

        parse_regop(&regop, args[2], oP->operand[1]);
	instr = instr | regop.n | (regop.mode << 11);

        parse_regop(&regop, args[3], oP->operand[2]);
	instr = instr | (regop.n << 14) | (regop.mode << 12);

        parse_regop(&regop, args[4], oP->operand[3]);
        instr = instr | (regop.n << 19) | (regop.mode << 13);

	buf = emit(instr);
	if ( listing_now ) listing_info ( complete, 4, buf, curr_frag, LIST_BIGENDIAN );
}

/*****************************************************************************
 * reg_fmt:	generate a REG-format instruction
 *
 *****************************************************************************/
static void reg_fmt(args, oP)
    char *args[];	/* args[0]->opcode mnemonic, args[1-3]->operands */
    struct i960_opcode *oP; /* Pointer to description of instruction */
{
	long 		instr;	/* Binary to be output */
	struct regop 	regop;	/* Description of register operand */
	int 		n_ops;	/* Number of operands */
	char 		*buf;	/* Where within the frag the instr lives */

	instr = oP->opcode;
	n_ops = oP->num_ops;

	if (n_ops >= 1)
	{
		parse_regop(&regop, args[1], oP->operand[0]);

		if ((n_ops == 1) && !(instr & M3))
		{
			/* 1-operand instruction in which the dst field should
			 * be used (instead of src1).
			 */
			regop.n       <<= 19;
			if (regop.special)
			{
				regop.mode = regop.special;
			}
			regop.mode    <<= 13;
			regop.special = 0;
		}
		else 
		{
			/* regop.n goes in bit 0, needs no shifting */
			regop.mode    <<= 11;
			regop.special <<= 5;
		}
		instr |= regop.n | regop.mode | regop.special;
	}

	if (n_ops >= 2) {
		parse_regop(&regop, args[2], oP->operand[1]);

		if ((n_ops == 2) && !(instr & M3)){
			/* 2-operand instruction in which the dst field should
			 * be used instead of src2).
			 */
			regop.n       <<= 19;
			if (regop.special){
				regop.mode = regop.special;
			}
			regop.mode    <<= 13;
			regop.special = 0;
		} else {
			regop.n       <<= 14;
			regop.mode    <<= 12;
			regop.special <<= 6;
		}
		instr |= regop.n | regop.mode | regop.special;
	}
	if (n_ops == 3){
		parse_regop(&regop, args[3], oP->operand[2]);
		if (regop.special){
			regop.mode = regop.special;
		}
		instr |= (regop.n <<= 19) | (regop.mode <<= 13);
	}
	buf = emit(instr);
	if ( listing_now ) listing_info ( complete, 4, buf, curr_frag, LIST_BIGENDIAN );
}


/*****************************************************************************
 * relax_cobr:
 *	Replace cobr instruction in a code fragment with equivalent branch and
 *	compare instructions, so it can reach beyond a 13-bit displacement.
 *	Set up an address fix/relocation for the new branch instruction.
 *
 **************************************************************************** */

/* This "conditional jump" table maps cobr instructions into equivalent
 * compare and branch opcodes.
 */
static
struct {
	long compare;
	long branch;
} coj[] = {		/* COBR OPCODE: */
	CHKBIT,	BNO,	/*	0x30 - bbc */
	CMPO,	BG,	/*	0x31 - cmpobg */
	CMPO,	BE,	/*	0x32 - cmpobe */
	CMPO,	BGE,	/*	0x33 - cmpobge */
	CMPO,	BL,	/*	0x34 - cmpobl */
	CMPO,	BNE,	/*	0x35 - cmpobne */
	CMPO,	BLE,	/*	0x36 - cmpoble */
	CHKBIT,	BO,	/*	0x37 - bbs */
	CMPI,	BNO,	/*	0x38 - cmpibno */
	CMPI,	BG,	/*	0x39 - cmpibg */
	CMPI,	BE,	/*	0x3a - cmpibe */
	CMPI,	BGE,	/*	0x3b - cmpibge */
	CMPI,	BL,	/*	0x3c - cmpibl */
	CMPI,	BNE,	/*	0x3d - cmpibne */
	CMPI,	BLE,	/*	0x3e - cmpible */
	CMPI,	BO,	/*	0x3f - cmpibo */
};



static
void
relax_cobr(fragP)
    register fragS *fragP;	/* fragP->fr_opcode is assumed to point to
				 * the cobr instruction, which comes at the
				 * end of the code fragment.
				 */
{
	int opcode, src1, src2, m1, s2;
			/* Bit fields from cobr instruction */
	long bp_bits;	/* Branch prediction bits from cobr instruction */
	long instr;	/* A single i960 instruction */
	char *iP;	/*->instruction to be replaced */
	fixS *fixP;	/* Relocation that can be done at assembly time */

	/* PICK UP & PARSE COBR INSTRUCTION */
	iP = fragP->fr_opcode;
	instr  = md_chars_to_number(iP, 4, fragP->fr_big_endian);
	opcode = ((instr >> 24) & 0xff) - 0x30;	/* "-0x30" for table index */
	src1   = (instr >> 19) & 0x1f;
	m1     = (instr >> 13) & 1;
	s2     = instr & 1;
	src2   = (instr >> 14) & 0x1f;
	bp_bits= instr & BP_MASK;

	/* GENERATE AND OUTPUT COMPARE INSTRUCTION */
	instr = coj[opcode].compare
			| src1 | (m1 << 11) | (s2 << 6) | (src2 << 14);
	md_number_to_chars(iP, instr, 4, fragP->fr_big_endian);

	/* OUTPUT BRANCH INSTRUCTION */
	md_number_to_chars(iP+4, coj[opcode].branch | bp_bits, 4, fragP->fr_big_endian);

	/* SET UP ADDRESS FIXUP/RELOCATION */
	fixP = fix_new(fragP,
		       iP+4 - fragP->fr_literal,
		       4,
		       fragP->fr_symbol,
		       0,
		       fragP->fr_offset,
		       1,
		       0);

	fixP->fx_bit_fixP = (bit_fixS *) 24;	/* Store size of bit field */
	fragP->fr_fix += 4;

	/* For the listing: Set the subtype to 2 so that the listing backend
	 * will ignore this frag; i.e. the size is already correct.
	 */
	fragP->fr_subtype = 2;

	/* Frag_wane sets the type to rs_fill, so this frag will be ignored
	 * by further relax-ing routines.
	 */
	frag_wane(fragP);

}


/*****************************************************************************
 * reloc_callj:	Relocate a 'callj' instruction
 *
 *	This is a "non-(GNU)-standard" machine-dependent hook.  The base
 *	assembler calls it when it decides it can relocate an address at
 *	assembly time instead of emitting a relocation directive.
 *
 *	Check to see if the relocation involves a 'callj' instruction to a:
 *	    sysproc:	Replace the default 'call' instruction with a 'calls'
 *	    leafproc:	Replace the default 'call' instruction with a 'bal'.
 *	    other proc:	Do nothing.
 *
 *	See b.out.h for details on the 'n_other' field in a symbol structure.
 *
 **************************************************************************** */
void reloc_callj(fixP)
fixS *fixP;		/* Relocation that can be done at assembly time */
{
    char *where;	/*->the binary for the instruction being relocated */
    symbolS *bp; 	/* Bal entry point symbol */
    
    if ( ! fixP->fx_callj || ! fixP->fx_addsy )
    {
	return;
    } /* This wasn't a callj instruction in the first place */
    
    where = fixP->fx_frag->fr_literal + fixP->fx_where;
    
    if (TC_S_IS_SYSPROC(fixP->fx_addsy)) 
    {
	/* Symbol is a .sysproc: replace 'call' with 'calls'.
	 * System procedure number is (other-1).
	 */
	int   idx = TC_S_GET_SYSINDEX(fixP->fx_addsy);
	if ( idx == -1 )
	{
	    /* Special "don't know what the index is" index */
	    /* FIXME: this is not right; caller assumes that everything
	       is fixed up, and doesn't make a relocation */
	    return;
	}
	if ( idx < 0 || idx > 31 )
	{
	    as_bad("Sysproc index %d too large for callj optimization", idx);
	    return;
	}
	else
	{
	    md_number_to_chars(where, CALLS|idx, 4, fixP->fx_frag->fr_big_endian);
	}
	
	/* Nothing else needs to be done for this instruction.
	 * Make sure 'md_number_to_field()' will perform a no-op.
	 */
	fixP->fx_bit_fixP = (bit_fixS *) 1;
	
    } 
    else if ( TC_S_IS_LEAFPROC(fixP->fx_addsy) ) 
    {
	/* Replace the "add" symbol in this fixup with the bal entry 
	 * point symbol for this leafproc.  Calling code will
	 * complete relocation as if nothing happened here.
	 */
	bp = symbol_find(TC_S_GET_BALNAME(fixP->fx_addsy));
	if ( S_IS_DEFINED(bp) ) 
	{
	    fixP->fx_addsy = bp;
	}
	else
	{
	    as_bad("No 'bal' entry point for leafproc %s", S_GET_NAME(fixP->fx_addsy));
	    return;
	}
	
	/* Replace 'call' opcode with 'bal' */
	md_number_to_chars(where, BAL, 4, fixP->fx_frag->fr_big_endian);
    }
    else if ( TC_S_IS_BALNAME(fixP->fx_addsy) ) 
    {
	/* We have a "callj balname", a weird but valid case.  The add
	   symbol is already correct; just change the 'call' to 'bal'. */
	md_number_to_chars(where, BAL, 4, fixP->fx_frag->fr_big_endian);
    }
	
    /* else Symbol is neither a sysproc nor a leafproc */
    
} /* reloc_callj() */


/* Returns 1 iffi the two symbols belong to the same segment. */
static int same_segment(p,q)
    struct symbol *p,*q;
{
    if (S_GET_SEGTYPE(p) != S_GET_SEGTYPE(q))
 	    return 0;
    return p->sy_segment == q->sy_segment;
}

/* Make sure all leafprocs are defined and belong to the same segment. */
void md_check_leafproc_list()
{
    struct symbol *p;

    for (p=symbol_rootP;p;p=p->sy_next)
 	    if (TC_S_IS_LEAFPROC(p)) {
 		if (!same_segment(p,p->bal_entry_symbol))
 			as_bad(".leafproc symbol: %s belongs to a different segment than: %s.",S_GET_NAME(p),
 			       S_GET_NAME(p->bal_entry_symbol));
 		if (S_GET_SEGTYPE(p) == SEG_UNKNOWN)
 			as_bad(".leafproc symbol: %s is undefined.", S_GET_NAME(p));
 	    }
}

/*****************************************************************************
 * s_leafproc:	process .leafproc pseudo-op
 *
 * .leafproc takes one or two arguments:
 *	if two (normal case):
 *		arg[1] is name of 'call' entry point to leaf procedure
 *		arg[2] is name of 'bal' entry point to leaf procedure
 *	if one:
 *		arg[1] is name of both 'call' and 'bal' entry points
 *
 * For a.out or b.out:
 *	If there are 2 distinct arguments, we must make sure that the 'bal'
 *	entry point immediately follows the 'call' entry point in the linked
 *	list of symbols.
 *
 * For elf:
 *	We store the name of the bal entry point into the symbol structure.
 *	Then in obj_emit_symbols we create a dummy symbol that stores the
 * 	bal entry point.  The bal entry point symbol is completely separate
 *	and acts just like a {static|global} label.
 **************************************************************************** */

static void s_leafproc(n_ops, args)
int n_ops;		/* Number of operands */
char *args[];	/* args[1]->1st operand, args[2]->2nd operand */
{
	symbolS *callP;	/* Pointer to leafproc 'call' entry point symbol */

	if ((n_ops != 1) && (n_ops != 2)) 
	{
		as_bad(".leafproc requires 1 or 2 operands");
		return;
	} /* Check number of arguments */

	/* Find or create symbol for 'call' entry point. */
	callP = symbol_find_or_make(args[1]);

	if (TC_S_IS_SYSPROC(callP))
	{
		/* Previously-defined sysproc */
		as_bad("Symbol already defined as sysproc: %s.\nYou can't redefine it as leafproc.", S_GET_NAME(callP));
		return;
	}

#ifdef	OBJ_COFF
	/* FIXME: do this in obj_emit_symbols */
	S_SET_NUMBER_AUXILIARY(callP,2);
#endif

	/* If there was only one argument, use it as both the 'call' and 
	 * the 'bal' entry point.  Otherwise, mark this symbol as LEAFPROC
	 * and store the name of the 'bal' entry point. 
	 */
	if ( n_ops == 1 ) {
 	    TC_S_SET_BALSYM(callP, callP);
	    TC_S_SET_BALNAME(callP, S_GET_NAME(callP));
	}
	else
	{
	    symbolS *balP;
	    char *balname = (char *) xmalloc(strlen(args[2]) + 1);

	    strcpy(balname, args[2]);

	    /* Find or create symbol for 'bal' entry point. */
	    balP = symbol_find_or_make(balname);
 	    TC_S_SET_BALSYM(callP, balP);
 	    TC_S_SET_BALNAME(callP, balname);
	    TC_S_IS_BALNAME(balP) = 1; /* Used in reloc_callj */
	}
	TC_S_IS_LEAFPROC(callP) = 1;
} /* s_leafproc() */


/*
 * s_sysproc:	process .sysproc pseudo-op
 *
 *	.sysproc takes two arguments:
 *		arg[1]: name of entry point to system procedure
 *		arg[2]: 'entry_num' (index) of system procedure in the range
 *			[0,31] inclusive.
 *
 *      If arg[2] is not present, set the entry num to -1 as per 
 *	CTOOLS R3.5 convention.
 *
 *	For [ab].out, we store the 'entrynum' in the 'n_other' field of
 *	the symbol.  Since that entry is normally 0, we bias 'entrynum'
 *	by adding 1 to it.  It must be unbiased before it is used.
 */
static void s_sysproc(n_ops, args)
int n_ops; /* Number of operands */
char *args[]; /* args[1]->1st operand, args[2]->2nd operand */
{
	expressionS 	exp;
	symbolS 	*symP;
	int		entry_num;

	switch ( n_ops )
	{
	case 1:
		entry_num = -1;
		break;
	case 2:
		if ((parse_expr(args[2],&exp) != SEG_ABSOLUTE)
		    || (e_offs(exp) < 0)
		    || (e_offs(exp) > TC_S_MAX_SYSPROC_INDEX)) 
		{
			as_bad("'entry_num' must be absolute number in [0,%d]", TC_S_MAX_SYSPROC_INDEX);
			return;
		}
		entry_num = e_offs(exp);
		break;
	default:
		as_bad(".sysproc should have one or two operands");
		return;
	}

	/* Find/make symbol and stick entry number (biased by +1) into it */
	symP = symbol_find_or_make(args[1]);

	if ( TC_S_IS_SYSPROC(symP) 
	    && TC_S_GET_SYSINDEX(symP) != 0 
	    && TC_S_GET_SYSINDEX(symP) != entry_num )
	{
		as_warn("Redefining entrynum for sysproc %s", S_GET_NAME(symP));
	} /* redefining */

	if ( TC_S_IS_LEAFPROC(symP) )
	{
		/* Previously-defined leafproc */
		as_bad("Symbol already defined as leafproc: %s.\nYou can't redefine it as sysproc.", S_GET_NAME(symP));
		return;
	}

#ifdef	OBJ_COFF
	S_SET_NUMBER_AUXILIARY(symP,2);
	S_SET_STORAGE_CLASS(symP, C_SCALL);
#endif
	TC_S_IS_SYSPROC(symP) = 1;
#ifdef OBJ_BOUT
	if (!IS_SYSPROCIDX(entry_num))
		as_bad("Attempt to define: %s a sysproc: %d which is outside limits of bout omf.",S_GET_NAME(symP),entry_num);
#endif
	TC_S_SET_SYSINDEX(symP, entry_num);
} /* s_sysproc() */


/*
 * s_mark_pixfile:	process .pic, .pid, and .link_pix pseudo-ops
 *
 *	Set flags to show that position-independent code or data directives
 *	were found.  The COFF object file header will be marked as such in 
 *      obj_pre_write_hook().  These same flags will be set in 
 *	md_parse_option() if -pc, -pd, or -pb are encountered on the command 
 *	line.
 *
 *      If not a COFF object, the PIX directives will be recognized but
 *	nothing will be done for them.
 *
 *      There is only one argument, an integer code to determine PIC, PID, 
 *      or both.
 */
static void s_mark_pixfile(po_num)
	int po_num;
{
	switch (po_num)
	{
	case S_PIC:
		pic_flag = 1;
		break;
	case S_PID:
		pid_flag = 1;
		break;
	case S_LINKPIX:
		link_pix_flag = 1;
		break;
	default:
		BAD_CASE(po_num);
		break;
	}
	segs_assert_pix();
} /* s_mark_pixfile() */


/*****************************************************************************
 * shift_ok:
 *	Determine if a "shlo" instruction can be used to implement a "ldconst".
 *	This means that some number X < 32 can be shifted left to produce the
 *	constant of interest.
 *
 *	Return the shift count, or 0 if we can't do it.
 *	Caller calculates X by shifting original constant right 'shift' places.
 *
 **************************************************************************** */
static
int
shift_ok(n)
    int n;		/* The constant of interest */
{
	int shift;	/* The shift count */

	if (n <= 0){
		/* Can't do it for negative numbers */
		return 0;
	}

	/* Shift 'n' right until a 1 is about to be lost */
	for (shift = 0; (n & 1) == 0; shift++){
		n >>= 1;
	}

	if (n >= 32){
		return 0;
	}
	return shift;
}


/*****************************************************************************
 * syntax:	issue syntax error
 *
 **************************************************************************** */
static void syntax() {
	as_bad("syntax error");
} /* syntax() */


/* md_set_arch_message
 * Set architecture mismatch message to error or warning 
 * based on whether -x was seen on command line. 
 * Defaults to as_bad in md_begin.
 */
md_set_arch_message(msg_fcn)
#if defined(__STDC__) && ! defined(NO_STDARG)
void (*msg_fcn)(const char *Format, ...);
#else
void (*msg_fcn)();
#endif
{
    wrong_arch_message = msg_fcn;
}

/*****************************************************************************
 * targ_has_sfr:
 *	Return TRUE iff the target architecture supports the specified
 *	special-function register (sfr).
 *
 **************************************************************************** */
static
int
targ_has_sfr(n)
    int n;	/* Number (0-31) of sfr */
{
    switch (architecture) {
 case ARCH_KA:
 case ARCH_KB:
 case ARCH_JX:
 case ARCH_JL:
	return 0;
 case ARCH_CX:
	return ((0<=n) && (n<=2));
 case ARCH_HX:
	return ((0<=n) && (n<=4));
 case ARCH_ANY:
	return 1;
 default:
	return 0;
    }
}


/*****************************************************************************
 * targ_has_instruction:
 *	Return TRUE iff the target architecture supports the instruction.
 *	As a side effect, record the instruction "iclass" for marking the
 *	object file when we're done assembling.
 **************************************************************************** */
static
int
targ_has_instruction(op)
    struct i960_opcode *op;
{
    iclasses_seen |= op->iclass;
    switch (architecture)
    {
    case ARCH_KA:	
	return op->arch & A_KA;
    case ARCH_KB:	
	return op->arch & A_KB;
    case ARCH_CX:	
	return op->arch & A_CX;
    case ARCH_JX:	
	return op->arch & A_JX;
    case ARCH_JL:
	return op->arch & A_JL;
    case ARCH_HX:	
	return op->arch & A_HX;
    case ARCH_ANY:  
	return 1;
    default:
	return 0;
    }
}


/* Parse an operand that is machine-specific.
   We just return without modifying the expression if we have nothing
   to do. */

/* ARGSUSED */
void
md_operand (expressionP)
     expressionS *expressionP;
{
}

/* We have no need to default values of symbols. */

/* ARGSUSED */
symbolS *md_undefined_symbol(name)
char *name;
{
	return 0;
} /* md_undefined_symbol() */

/* Exactly what point is a PC-relative offset relative TO?
   On the i960, they're relative to the address of the instruction,
   which we have set up as the address of the fixup too. */
long
md_pcrel_from (fixP)
     fixS *fixP;
{
  return fixP->fx_where + fixP->fx_frag->fr_address;
}

void
md_apply_fix(fixP, val, labelname)
    fixS *fixP;
    long val;
    char *labelname;   /* carried along just for error messages. */
{
	char *place;

	/*
	 * Single line, below, split to work around RS6000 compiler bug.
	 * char *place = fixP->fx_where + fixP->fx_frag->fr_literal;
	 */
	place = fixP->fx_frag->fr_literal;
	place += fixP->fx_where;

	if (!fixP->fx_bit_fixP) 
	{

		switch (fixP->fx_im_disp) 
		{
		case 0:
			fixP->fx_addnumber = val;
			md_number_to_imm(place, val, fixP->fx_size, fixP->fx_frag->fr_big_endian);
			break;
		case 1:
			md_number_to_disp(place,
					   fixP->fx_pcrel ? val + fixP->fx_pcrel_adjust : val,
					   fixP->fx_size);
			break;
		case 2: /* fix requested for .long .word etc */
			md_number_to_chars(place, val, fixP->fx_size, fixP->fx_frag->fr_big_endian);
			break;
		default:
			as_fatal("Internal error in md_apply_fix() in file \"%s\"", __FILE__);
		} /* OVE: maybe one ought to put _imm _disp _chars in one md-func */
	} 
	else
		md_number_to_field(place, val, fixP->fx_bit_fixP, labelname, fixP->fx_frag->fr_big_endian);

} /* md_apply_fix() */

#if defined(OBJ_BOUT)
/*
 *		emit_relocations()
 *
 * Crawl along a fixS chain. Emit the segment's relocations.
 */
static void
emit_machine_reloc (fixP, segment_address_in_file)
     register fixS *	fixP;	/* Fixup chain for this segment. */
     relax_addressT	segment_address_in_file;
{
  struct reloc_info_generic	ri;
  register symbolS *		symbolP;

  bzero((char *)&ri,sizeof(ri));
  for (;  fixP;  fixP = fixP->fx_next)
    {
      if ((symbolP = fixP->fx_addsy) != 0)
	{
	  ri . r_callj		= fixP->fx_callj;
	  ri . r_calljx		= fixP->fx_calljx;

	  ri . r_length		= nbytes_r_length [fixP->fx_size];
	  ri . r_pcrel		= fixP->fx_pcrel;
	  ri . r_address	= fixP->fx_frag->fr_address
	    +   fixP->fx_where
	      - segment_address_in_file;
	  if ( (!S_IS_DEFINED(symbolP)) || ri.r_callj || ri.r_calljx )
	    {
	      ri . r_extern	= 1;
	      ri . r_index	= symbolP->sy_number;
	    }
	  else
	    {
	      ri . r_extern	= 0;
	      ri . r_index	= S_GET_TYPE(symbolP);
	    }

	  /* Output the relocation information in machine-dependent form.
	   * Check that the size is big enough to handle a symbol relocation
	   */
	  if ( fixP->fx_size < 4 )
		  as_bad("Can't emit a 4-byte relocation for symbol: %s", S_GET_NAME(symbolP));
	  else
		  md_ri_to_chars( &ri );
	}
    }

} /* emit_machine_reloc() */
#endif /* OBJ_BOUT */


/* Align a segment start address by rounding it up to the specified boundary.
 */
long md_segment_align(addr, exponent)
	relax_addressT 	addr;
	int		exponent; /* Power of 2 to align to */
{
	addr += (1 << exponent) - 1;
	return(addr & (-1 << exponent));
} /* md_segment_align() */


#if defined(OBJ_COFF)
/* Mark the COFF file header flags with an architecture code. */
void tc_coff_headers_hook(headers)
object_headers *headers;
{
	unsigned short arch_flag = 0;

	if ( i960_ccinfo_size() )
		headers->filehdr.f_flags |= F_CCINFO;
	
	/* Was -x seen?  If so, set file header flag to whatever the user
	   set the architecture to on the command line (or let it default.)
	   Else set the file header based on actual instructions processed. */
	if ( flagseen['x'] )
	{
	    switch ( architecture )
	    {
	    case ARCH_ANY:
		headers->filehdr.f_flags |= F_I960CORE;
		break;
	    case ARCH_KA:
		headers->filehdr.f_flags |= F_I960KA;
		break;
	    case ARCH_KB:
		headers->filehdr.f_flags |= F_I960KB;
		break;
	    case ARCH_CX:
		headers->filehdr.f_flags |= F_I960CA;
		break;
	    case ARCH_JX:
	    case ARCH_JL:
		headers->filehdr.f_flags |= F_I960JX;
		break;
	    case ARCH_HX:
		headers->filehdr.f_flags |= F_I960HX;
		break;
	    default:
		/* Should never get here; but what the hell */
		headers->filehdr.f_flags |= F_I960CORE;
		break;
	    }
	}
	else
	{
	    /* Did we do any architecture checking?  If not, set the file hdr
	       flag to CORE1, so that this object will link with any other. 
	       Find the "largest" architecture that was seen during processing.
	       (remember that instructions incompatible with the -A flag were
	       disallowed when first read.)
	       Note that the ORDER in which the tests below are done is
	       critical, since iclasses_seen may contain more than one 
	       architecture bit. */
	    if ( architecture == ARCH_ANY || iclasses_seen == 0 )
		headers->filehdr.f_flags |= F_I960CORE1;
	    
	    else if (iclasses_seen & I_HX)
		headers->filehdr.f_flags |= F_I960HX;
	    
	    else if (iclasses_seen & I_JX)
		headers->filehdr.f_flags |= F_I960JX;
	    
	    else if (iclasses_seen & I_CORE2)
		headers->filehdr.f_flags |= F_I960CORE2;
	    
	    else if ((iclasses_seen & I_CX) || (iclasses_seen & I_CASIM))
		headers->filehdr.f_flags |= F_I960CX;
	    
	    else if (iclasses_seen & (I_DEC|I_FP))
		headers->filehdr.f_flags |= F_I960KB;
	    
	    else if (iclasses_seen & (I_KX))
		headers->filehdr.f_flags |= F_I960KA;
	    
	    else
		headers->filehdr.f_flags |= F_I960CORE1;
	} /* was -x seen? */

	/* set target byte-order flag (COFF only) */
	if (flagseen['G']) {
		headers->filehdr.f_flags |= F_BIG_ENDIAN_TARGET;/* big-endian */
	}
	if (1) {
	    unsigned short short_word = 0x1234;
	    unsigned char *ptr_to_short = (unsigned char *) &short_word;

	    if (ptr_to_short[0] == 0x34)
		    headers->filehdr.f_flags |= F_AR32WR;   /* little-endian */
	}

	/* set magic numbers */
	headers->filehdr.f_magic = I960ROMAGIC;
	headers->aouthdr.magic = NMAGIC;

} /* tc_headers_hook() */
#else
#if defined(OBJ_ELF)
void tc_elf_headers_hook(headers)
object_headers *headers;
{
    unsigned short arch_flag = 0;
    
    headers->elfhdr.e_flags |= elf_arch;

    if (iclasses_seen & I_CORE1)
	    headers->elfhdr.e_flags |= EF_960_CORE1;
    if (iclasses_seen & I_CORE2)
	    headers->elfhdr.e_flags |= EF_960_CORE2;
    if (iclasses_seen & (I_KX|I_DEC))
	    headers->elfhdr.e_flags |= EF_960_K_SERIES;
    if (iclasses_seen & I_CX)
	    headers->elfhdr.e_flags |= EF_960_C_SERIES;
    if (iclasses_seen & I_JX)
	    headers->elfhdr.e_flags |= EF_960_J_SERIES;
    if (iclasses_seen & I_HX)
	    headers->elfhdr.e_flags |= EF_960_H_SERIES;
    if (iclasses_seen & I_FP)
	    headers->elfhdr.e_flags |= EF_960_FP1;


}
#endif
#endif

int
md_check_arch()
{
	/* big-endian memory was specified; make sure architecture
	 * supports it.
	 */
	if ( architecture != ARCH_CX 
	    && architecture != ARCH_JX
	    && architecture != ARCH_JL
	    && architecture != ARCH_HX
	    && architecture != ARCH_ANY )
	{
	    as_fatal("-G or .section with msb attribute specified, for an architecture which does not support big-endian memory.");
	}
}
