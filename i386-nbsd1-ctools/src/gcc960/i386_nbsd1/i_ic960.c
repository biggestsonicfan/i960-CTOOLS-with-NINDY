/*******************************************************************************
 * 
 * Copyright (c) 1993 Intel Corporation
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

#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include "i_toolib.h"

#if defined(DOS)

#include <io.h>
#include <process.h>

#else

#include <sys/file.h>
#include <sys/wait.h>

#endif

#if defined(USG) || defined(DOS)
#ifndef R_OK
#define R_OK 4
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef X_OK
#define X_OK 1
#endif
#endif

extern char* getenv();
void remove_temporary_files();

/* Make main's argc and argv global so they can be accessed outside main. */
/* Needed only for passing the driver's command line to the compiler. */
static	int	Argc = 0;
static	char**	Argv = NULL;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define OPT_STRING "aA:b:cCD:Ef:F:g::G:hi:I:j:J:l:L:mMno:O:Pq:QrRsST:u:U:vVw:W:xY:z:Z:"

	/* An option letter succeeded by a single colon indicates
	   that the option requires an argument.

	   An option letter succeeded by two colons indicates that the
	   option takes an "optional" numeric argument.  The argument,
	   if given, must be a non-negative decimal integer.  For example,
	   "-g", "-g1" and "-g 1" are all valid debug_info specifications,
	   while "-gc" is interpreted as "-g -c".  "-g3xyz" is an error.

	   An option letter *not* succeeded by a colon indicates
	   that the option does not take an argument.
	 */

/* Define CTOOLS960 environment variable names. */

static char	IC_ENV_ARCH[] = "I960ARCH";
static char	IC_ENV_AS[] = "I960AS";
static char	IC_ENV_BASE[] = "I960BASE";
static char	IC_ENV_CC1[] = "I960CC1";
static char	IC_ENV_CPP[] = "I960CPP";
static char	IC_ENV_INC[] = "I960INC";
static char	IC_ENV_LD[] = "I960LD";
static char	IC_ENV_PDB[] = "I960PDB";

/* Define file name suffixes */

static char	IC_CPP_OUT_SUFFIX[] = ".i";
static char	IC_CC1_OUT_SUFFIX[] = ".s";
static char	IC_LIST_FILE_SUFFIX[] = ".L";
static char	IC_SINFO_SUFFIX[] = ".sin";
static char	IC_LIST_TMP_SUFFIX[] = ".ltm";
static char	IC_BNAME_TMP_SUFFIX[] = ".btm";

static char	IC_DIR_BIN[] = "bin";
static char	IC_DIR_INC[] = "include";
static char	IC_DIR_LIB[] = "lib";

#if defined(DOS)

static char	IC_EXE_ASM_COFF[] = "asm960.exe";
static char	IC_EXE_ASM_ELF[] = "gas960e.exe";
static char	IC_EXE_LNK[] = "lnk960.exe";
static char	IC_EXE_CC1[] = "cc1.exe";
static char	IC_EXE_CPP[] = "cpp.exe";

#else

static char	IC_EXE_ASM_COFF[] = "asm960";
static char	IC_EXE_ASM_ELF[] = "gas960e";
static char	IC_EXE_LNK[] = "lnk960";
static char	IC_EXE_CC1[] = "cc1.960";
static char	IC_EXE_CPP[] = "cpp.960";

#endif

typedef int	Boolean;

/*
 * Define a string list and supporting functions for general use.
 */

typedef struct sll {
	char*		s;
	struct sll*	next;
} String_list_link;

typedef String_list_link*	String_list;

static char	my_name[] = "ic960";
static char	error_prefix[] = "COMMAND LINE ERROR: ";
static char	warn_prefix[] = "COMMAND LINE WARNING: ";

/*
 * Define characteristics of the various 80960 architectures.
 */

typedef struct {
	char*		ic960_name;
	char*		cc1_name;
	char*		macro_name;
	unsigned int	flags;
#define				SUPPORTS_BIG_ENDIAN	0x1
#define				AVOID_UNALIGNED_ACCESS	0x2

#define				arch_supports_big_endian(a)	\
					((a)->flags & SUPPORTS_BIG_ENDIAN)

#define				arch_dislikes_unaligned_access(a)	\
					((a)->flags & AVOID_UNALIGNED_ACCESS)
} Target_descr;

Target_descr targets[] = {
	{ "KA",		"ka",	"KA",	0 },
	{ "KB",		"kb",	"KB",	0 },
	{ "SA",		"sa",	"SA",	0 },
	{ "SB",		"sb",	"SB",	0 },
	{ "CA",		"ca",	"CA",	SUPPORTS_BIG_ENDIAN
					| AVOID_UNALIGNED_ACCESS },
	{ "CA_DMA",	"ca",	"CA",	SUPPORTS_BIG_ENDIAN
					| AVOID_UNALIGNED_ACCESS },
	{ "CF",		"cf",	"CF",	SUPPORTS_BIG_ENDIAN
					| AVOID_UNALIGNED_ACCESS },
	{ "CF_DMA",	"cf",	"CF",	SUPPORTS_BIG_ENDIAN
					| AVOID_UNALIGNED_ACCESS },
	{ "JA",		"ja",	"JA",	SUPPORTS_BIG_ENDIAN
					| AVOID_UNALIGNED_ACCESS },
	{ "JD",		"jd",	"JD",	SUPPORTS_BIG_ENDIAN
					| AVOID_UNALIGNED_ACCESS },
	{ "JF",		"jf",	"JF",	SUPPORTS_BIG_ENDIAN
					| AVOID_UNALIGNED_ACCESS },
	{ "JL",		"jl",	"JL",	SUPPORTS_BIG_ENDIAN
					| AVOID_UNALIGNED_ACCESS },
	{ "RP",		"rp",	"RP",	SUPPORTS_BIG_ENDIAN
					| AVOID_UNALIGNED_ACCESS },
	{ "HA",		"ha",	"HA",	SUPPORTS_BIG_ENDIAN },
	{ "HD",		"hd",	"HD",	SUPPORTS_BIG_ENDIAN },
	{ "HT",		"ht",	"HT",	SUPPORTS_BIG_ENDIAN },
	{ NULL,		NULL,	NULL,	0 }
};

/* Define a structure to hold the value of one -f option */

typedef struct {
	char	*name;
	char	*value;	/* NULL if -f[no-]name wasn't specified, otherwise
			   this points to "[no-]name" in argv[].
			 */
	int	phases;	/* which subphases should we pass this option to? */
} Little_f_value;

Little_f_value little_f_options[] = {
#define DEF_F_OPTION(name, phases, help_text) { name, NULL, phases, },
#include "i_icopts.def"
	{ NULL, 0, 0 }
};

/* Define a structure to hold the value of one -W diagnostic option */

typedef struct {
	char	*name;
	char	*value;	/* NULL if -W[no-]name wasn't specified, otherwise
			   this points to "[no-]name" in argv[].
			 */
	int	phases;	/* which subphases should we pass this option to? */
} Big_W_value;

Big_W_value big_W_options[] = {
#define DEF_W_OPTION(name, phases, help_text) { name, NULL, phases, },
#include "i_icopts.def"
	{ NULL, 0, 0 }
};

/*
 * Define a structure to hold all values of command line arguments.
 */

typedef struct {
	Boolean		strict_ansi;				/* -a */
	Target_descr*	architecture;				/* -A */
	long		limit_optimizations;			/* -b */
	int		stop_after;				/* -EQPnSc */
#define				STOP_AT_DEPENDENCIES	1	/*   -Q */
#define				STOP_AT_PP_EXPAND	2	/*   -E */
#define				STOP_AT_PP_FILE		3	/*   -P */
#define				STOP_AT_SYNTAX		4	/*   -n */
#define				STOP_AT_ASSEMBLY	5	/*   -S */
#define				STOP_AT_OBJECT		6	/*   -c */
#define				STOP_AT_EXECUTABLE	7
	Boolean		keep_comments;				/* -C */
	String_list	list_of_macro_definitions;		/* -D */
	int		object_module_format;			/* -Fcoff,elf */
#define				OMF_COFF		1
#define				OMF_ELF			2
	unsigned long	fine_tune_enable;			/* -Fopt */
	unsigned long	fine_tune_disable;			/* -Fnoopt */
#define				OPT_NONE			0x0
#define				OPT_LEAF_PROC			0x1
#define				OPT_COLLAPSE_IDENTITIES		0x2
#define				OPT_SPECIAL_INSTRUCTIONS	0x4
#define				OPT_LOCAL_PROMOTION		0x8
#define				OPT_BRANCH_OPT			0x10
#define				OPT_TAIL_CALL_ELIMINATION	0x20
#define				OPT_SCHEDULING			0x40
#define				OPT_CONSTANT_PROPAGATION	0x80
#define				OPT_CSE				0x100
#define				OPT_DEAD_CODE			0x200
#define				OPT_LOOP_CODE_MOTION		0x400
#define				OPT_CODE_COMPACTION		0x800
#define				OPT_LIVE_TRACK_SEPARATOR	0x1000
#define				OPT_GLOBAL_VAR_MIGRATE		0x2000
#define				OPT_INDUCTION_VARIABLES		0x4000
#define				OPT_ALIAS_ANALYSIS		0x8000
#define				OPT_LOCAL_CSE			0x10000
#define				OPT_LOCAL_CONSTANT_FOLD		0x20000
#define				OPT_BLOCK_REARRANGEMENT		0x40000
#define				OPT_PRAGMA_INLINING		0x80000
#define				OPT_AUTO_INLINING		0x100000
#define				OPT_GLOBAL_INLINING		0x200000
#define				OPT_PROFILE_BRANCH_PREDICT	0x400000
#define				OPT_DEAD_FUNCTION_ELIM		0x800000
#define				OPT_SUPER_BLOCKS		0x1000000
#define				OPT_FUNCTION_PLACEMENT		0x2000000
#define				OPT_COMPARE_AND_BRANCH		0x4000000
#define				OPT_CODE_ALIGN			0x8000000
#define				OPT_STRICT_ALIGN		0x10000000

	int		flag_fbuild_db;				/* -fbuild-db*/
	int		flag_fdb;				/* -fdb */
	int		flag_fprof;				/* -fprof */
#define				EXPLICITLY_ENABLED	1
#define				EXPLICITLY_DISABLED	(-1)
#define				UNSPECIFIED		0
		/* We need to single these out of the generic -f option
		   list because they affect the default optimization level.
		   In particular, a minimum optimization level of 1 is
		   required if these are specified.
		  */
#define force_O1()	(cmd_line.flag_fbuild_db == EXPLICITLY_ENABLED \
			 || cmd_line.flag_fdb == EXPLICITLY_ENABLED \
			 || cmd_line.flag_fprof == EXPLICITLY_ENABLED)

	int		debug_info;				/* -g[0123] */
#define				DEBUG_INFO_OFF		0
#define				DEBUG_INFO_TERSE	1
#define				DEBUG_INFO_NORMAL	2
#define				DEBUG_INFO_VERBOSE	3
	long		structure_alignment;			/* -Gac */
	Boolean		backward_compatible;			/* -Gbc */
	Boolean		generate_big_endian;			/* -Gbe */
	Boolean		generate_cave;				/* -Gcave */
	Boolean		char_unsigned;				/* -Gcs,cu */
	Boolean		strict_refdef;				/* -Gdc,ds */
	Boolean		generate_pic;				/* -Gpc */
	Boolean		generate_pid;				/* -Gpd */
	Boolean		preserve_g12;				/* -Gpr */
	Boolean		generate_abi;				/* -Gabi */
	Boolean		generate_cpmulo;			/* -Gcpmul */
	long		spill_register_address;			/* -Gsa */
	long		spill_register_length;			/* -Gsl=num */
	char*		spill_register_symbol;			/* -Gsl=sym */
	Boolean		generate_extended_calls;		/* -Gxc */
	long		wait_state;				/* -Gwait=n */
	String_list	preinclude_files;			/* -i */
	String_list	search_include_directories;		/* -I */
        int		errata_number;				/* -jnum */
	Boolean		gcc_style_diagnostics;			/* -Jgd */
	Boolean		compiler_quiet;				/* -J[no]cq */
	String_list	linker_library_options;			/* -l */
	String_list	linker_options;			/* -LmrsTux and -Wl */
	Boolean		interleave_source;			/* -M */
	char*		output_file_name;			/* -o */
	int		optimization_level;			/* -O */
	char*		profile_library;			/* -ql */
	int		profile_level;				/* -qp[12] */
#define				PROFILE_OFF		0
#define				PROFILE_INSTRUMENTATION	1
#define				PROFILE_RECOMPILATION	2
			/* profile_level records -qp1 and -qp2.  It is not
			   affected by -gcdm nor -f.
			 */
	Boolean		report_peak_memory_usage;		/* -R */
	String_list	list_of_macro_undefines;		/* -U */
	Boolean		verbose;				/* -v */
	Boolean		version_requested;			/* -V */
	int		diagnostic_level;			/* -w[012] */
	String_list	assembler_pass_options;			/* -Wa */
	String_list	compiler_pass_options;			/* -Wc and -f */
	String_list	cpp_pass_options;			/* -Wp */
	char*		profile_database;		/* -Yd or IC_ENV_PDB */
	unsigned int	listing_options;			/* -z */
#define				LISTING_DISABLED		0x0
#define				LISTING_PRIMARY_SOURCE		0x1
#define				LISTING_INCLUDED_SOURCE		0x2
#define				LISTING_ASSEMBLY		0x4
#define				LISTING_MACRO_EXPANSION		0x8
#define				LISTING_CONDITIONAL_REJECT	0x10
#define is_listing_enabled()	(cmd_line.listing_options != LISTING_DISABLED)
	char*		listing_file_name;			/* -Z */
	String_list	list_of_input_files;
	char*		product_base_dir;		/* IC_ENV_BASE */
} Cmdline_args;

#define build_list_option(buf) { \
	(buf)[0] = '\0'; \
	if (cmd_line.listing_options & LISTING_PRIMARY_SOURCE) \
		(void) strcat((buf), "s"); \
	if (cmd_line.listing_options & LISTING_INCLUDED_SOURCE) \
		(void) strcat((buf), "i"); \
	if (cmd_line.listing_options & LISTING_ASSEMBLY) \
		(void) strcat((buf), "o"); \
	if (cmd_line.listing_options & LISTING_MACRO_EXPANSION) \
		(void) strcat((buf), "m"); \
	if (cmd_line.listing_options & LISTING_CONDITIONAL_REJECT) \
		(void) strcat((buf), "c");}

/*
 * Initialize default values of all command line options.
 */

Cmdline_args cmd_line = {
	FALSE,		/* strict_ansi */
	NULL,		/* architecture */
	-1,		/* limit_optimizations */
	STOP_AT_EXECUTABLE, /* stop_after */
	FALSE,		/* keep_comments */
	NULL,		/* list_of_macro_definitions */
	OMF_COFF,	/* object_module_format */
	OPT_NONE,	/* fine_tune_enable */
	OPT_NONE,	/* fine_tune_disable */
	UNSPECIFIED,	/* flag_fbuild_db */
	UNSPECIFIED,	/* flag_fdb */
	UNSPECIFIED,	/* flag_fprof */
	DEBUG_INFO_OFF,	/* debug_info */
	-1,		/* structure_alignment */
	FALSE,		/* backward_compatible */
	FALSE,		/* generate_big_endian */
	FALSE,		/* generate_cave */
	FALSE,		/* char_unsigned */
	FALSE,		/* strict_refdef */
	FALSE,		/* generate_pic */
	FALSE,		/* generate_pid */
	FALSE,		/* preserve_g12 */
	FALSE,		/* generate_abi */
        FALSE,		/* generate_cpmulo */
	-1,		/* spill_register_address */
	-1,		/* spill_register_length */
	NULL,		/* spill_register_symbol */
	FALSE,		/* generate_extended_calls */
	-1,		/* wait_state */
	NULL,		/* preinclude_files */
	NULL,		/* search_include_directories */
        -1,		/* no errata specified */
	FALSE,		/* gcc_style_diagnostics */
	TRUE,		/* compiler_quiet */
	NULL,		/* linker_library_options */
	NULL,		/* linker_options */
	FALSE,		/* interleave_source */
	NULL,		/* output_file_name */
	-1,		/* optimization_level */
	NULL,		/* profile_library */
	PROFILE_OFF,	/* profile_level */
	FALSE,		/* report_peak_memory_usage */
	NULL,		/* list_of_macro_undefines */
	FALSE,		/* verbose */
	FALSE,		/* version_requested */
	1,		/* diagnostic_level */
	NULL,		/* assembler_pass_options */
	NULL,		/* compiler_pass_options */
	NULL,		/* cpp_pass_options */
	NULL,		/* profile_database */
	LISTING_DISABLED,	/* listing_options */
	NULL,		/* listing_file_name */
	NULL,		/* list_of_input_files */
	NULL		/* product_base_dir */
};

/*
 * Define a table used to print information for the -h option.
 */

static char *help_table[] =
{
"  -A arch           Select the 80960 architecture.  Valid arch values",
"                    are:  KA,SA,KB,SB,CA,CF,JA,JD,JF,HA,HD,HT,RP.",
"                    Default is KB, unless I960ARCH is set.",
"  -a                Emit diagnostics for non-ANSI constructs.",
"  -b num            Suppress expensive optimizations for functions having",
"                    more than 'num' intermediate-language instructions.",
"  -C                Keep comments in preprocessor output.",
"  -c                Stop at object file creation, suppress linking.",
"  -D symbol[=value] Define a preprocessor symbol, with an optional value.",
"  -E                Stop after preprocessing, sending the output to stdout.",
"  -F arg[,arg...]   Select OMF or fine-tune optimization.  arg is one of:",
"     coff           Select COFF as the object module format (default).",
"     elf            Select Elf/DWARF as the object module format.",
"     [no]ai         Enable or disable automatic function inlining.",
"     [no]ca         Enable or disable code alignment.",
"     [no]cb         Enable or disable use of compare and branch instructions.",
"     [no]lp         Enable or disable leaf procedures.",
"     [no]pf         Enable or disable function placement.",
"     [no]sa         Enable or disable strict alignment assumptions.",
"     [no]sb         Enable or disable superblock formation.",
"     [no]tce        Enable or disable tail call elimination.",
"  -f [no-]arg       Enable or disable an optimization.  arg is one of:",

#define DEF_F_OPTION(name, phases, help_text) help_text,
#include "i_icopts.def"

"  -G arg[,arg...]   Select a code generation option.  arg is one of:",
"     abi            Generate code that conforms to the 80960 ABI.",
"     ac=n           Align struct types on byte boundary n [1, 2, 4, 8 or 16].",
"     bc             Generate code backward compatible with Release 2.0.",
"     be             Generate big endian code.",
"     cave           Generate compression assisted virtual execution code.",
"     cs             Treat type 'char' as signed.  This is the default.",
"     cu             Treat type 'char' as unsigned.",
"     dc             Use the relaxed ref-def linkage model.  Default.",
"     ds             Use the strict ref-def linkage model.",
"     pc             Generate position-independent code.",
"     pd             Generate position-independent data.",
"     pr             Do not use register g12.",
"     wait=n         Specify wait state for memory accesses.  0 <= n <= 32.",
#if 0
/* Obsolete options */
"     sa={address|name} Create a spill-register area at the given address.",
"     sl=length      Create a spill-register area with the given length.",
#endif
"     xc             Generate extended call opcodes.",
"  -gcdm,arg[,arg...] Select program-wide build options.",
"                     See the user's manual for details.",
"  -g [level]        Place debug information in object file.  Level is one of:",
"     0              Disable debug information (default if -g is not specified).",
"     1              Minimal debug information (only valid for Elf/DWARF).",
"     2              Normal debug information (default if level not specified).",
"     3              Verbose debug information (only valid for Elf/DWARF).",
"  -h                Print this summary of compiler options.",
"  -I dir            Select directory to search for #include files.",
"  -i file           Logically prepend 'file' to compiled C files.",
"  -J arg[,arg...]   Selects miscellaneous controls.  arg is one of:",
#if 0
/* Unadvertised options */
"     cq             Pass -quiet to compiler.  This is the default.",
"     nocq           Let compiler emit timing statistics.",
#endif
"     gd             Issue gcc960-style diagnostics.",
"     nogd           Issue ic960-style diagnostics (default).",
"  -j num            Specify processor errata.  num is one of:",
"     1              Generate code to work around the Cx DMA errata.",
"  -L dir            Select library search directory for linker.",
"  -l archive        Specify a library as input to the linker.",
"  -M                Mix C source with assembly language output.",
"  -m                Produce a linker memory map.",
"  -n                Check C syntax only.",
"  -O level          Select code optimization level.  level is one of:",
"     0              No optimization.  This is the default if -g is specified.",
"     1              Local optimizations (default).",
"     2              Global optimizations.",
"     3              Program-wide optimizations.",
"  -o file           Place compilation output in the given file.",
"  -P                Stop after preprocessing, placing output in file.i.",
"  -Q                Print #include file dependencies to stdout.",
"  -q arg[,arg...]   Specify obsolete profiling options.  arg is one of:",
"     p1             Perform profile instrumentation (implies -O1 or higher).",
"     p2             Perform profiling optimizations (implies -O3).",
"     l,library      Specifies an alternate profiling library.",
"  -R                Report peak memory usage.",
"  -r                Cause linker to create a relocatable object file.",
"  -S                Stop after assembly, placing output in file.s.",
"  -s                Cause linker to strip debug information from object file.",
"  -T file           Select a linker configuration file.",
"  -U symbol         Undefine a preprocessor symbol.",
"  -u symbol         Place unresolved 'symbol' in the linker's symbol table.",
"  -V                Display version information.",
"  -v                Display compilation phase invocations.",
"  -v960             Display version information and exit.",
"  -W phase,arg[,arg...]  Pass arguments to a specific phase.  Phase is one of:",
"     a              Pass args to the assembler.",
"     c              Pass args to the compiler.",
"     l              Pass args to the linker.",
"     p              Pass args to the preprocessor.",

"  -W [no-]arg       Enable or disable a warning.  arg is one of:",

#define DEF_W_OPTION(name, phases, help_text) help_text,
#include "i_icopts.def"

"  -w level          Control diagnostic messages.  level is one of:",
"     0              Enable remarks, warnings, and errors.",
"     1              Enable warnings and errors only (default).",
"     2              Enable only errors.",
"  -x                Omit local symbols from linker's output.",
"  -Y d,dir          Specify the program database directory.",
"  -Z file           Specify a single listing file (implies -zs).",
"  -z list           Select listing file contents.  'list' is one or more of:",
"     c              Output conditionally excluded lines.",
"     i              Output #included files.",
"     m              Output macro expansions.",
"     o              Output assembly code.",
"     s              Output primary source.",
NULL
};

static void print_help_list()
{
	int	i;

	(void) printf("\n%s is a C compiler for the Intel i960(R) processor family.\n",my_name);
	(void) printf("\nUsage:  PATHNAME/%s [options] file ...\n", my_name);
	(void) printf("\nOptions:\n\n");


	for (i = 0; help_table[i] != NULL; i++)
		(void) printf("%s\n", help_table[i]);
}

static char* xmalloc(size)
int size;
{
	char* value = (char*) malloc((unsigned)size);
	if (value == NULL) {
		(void) fprintf(stderr,
				"%s FATAL ERROR: Out of memory.\n", my_name);
		exit(FATAL_EXIT_CODE);
	}
	return value;
}

void cmd_line_error_str(msg, str_parm)
char* msg;		/* msg may contain at most one %s printf format */
char* str_parm;		/* Parameter for %s, or NULL */
{
	char	stack_buf[512];
	char*	buf;
	int	length;

	length = strlen(my_name) + 1 + strlen(error_prefix) + strlen(msg) + 2;
	if (length > 512)
		buf = xmalloc(length);
	else
		buf = stack_buf;
	(void) sprintf(buf, "%s %s%s\n", my_name, error_prefix, msg);
	(void) fprintf(stderr, buf, str_parm ? str_parm : "");

	if (length > 512)
		free(buf);
	remove_temporary_files();
	exit(FATAL_EXIT_CODE);
}

void cmd_line_warn_str(msg, str_parm)
char* msg;		/* msg may contain at most one %s printf format */
char* str_parm;		/* Parameter for %s, or NULL */
{
	char	stack_buf[512];
	char*	buf;
	int	length;

	length = strlen(my_name) + 1 + strlen(warn_prefix) + strlen(msg) + 2;
	if (length > 512)
		buf = xmalloc(length);
	else
		buf = stack_buf;
	(void) sprintf(buf, "%s %s%s\n", my_name, warn_prefix, msg);
	(void) fprintf(stderr, buf, str_parm ? str_parm : "");

	if (length > 512)
		free(buf);
}

static void
print_a_command_line(argc, argv)
int	argc;
char	*argv[];
{
	int i, has_space;
	(void) fprintf(stderr, "%s", argv[0]);
	for (i=1; i < argc; i++)
	{
		has_space = strchr(argv[i],' ') || strchr(argv[i],'\t');
		if (has_space)
			(void) fprintf(stderr, " \"%s\"", argv[i]);
		else
			(void) fprintf(stderr, " %s", argv[i]);
	}
	(void) fprintf(stderr, "\n");
}

/* Deallocate a possibly allocated char* pointer, reallocate
 * it and copy in the given new string.
 */
static void replace_string(old_str, new_str)
char**		old_str;
char*		new_str;
{
	if (*old_str) free(*old_str);
	*old_str = xmalloc(strlen(new_str) + 1);
	(void) strcpy(*old_str, new_str);
}

static void string_list_delete(list)
String_list*	list;
{
	String_list	p,t;

	if (!list) return;

	t = *list; p = NULL;

	while (t) {
		p = t; t = t->next;
		free(p->s);
		free(p);
	}

	*list = NULL;
}

/* Allocate space for a new string, and append it to the given list. */

static void string_list_append(list, str)
String_list*	list;
char*		str;
{
	String_list new_string, p, t;

	if (!str) return;

	new_string = (String_list) xmalloc(sizeof(String_list_link));
	new_string->s = xmalloc(strlen(str) + 1);
	(void) strcpy(new_string->s, str);
	new_string->next = NULL;

	p = NULL; t = *list;	while (t) { p = t; t = t->next; }
	if (p)
		p->next = new_string;
	else
		*list = new_string;
}

/* Make a copy of a string list, and append the copy to another string list. */

static void string_list_append_list(dst, src)
String_list*	dst;
String_list	src;
{
	for(; src; src = src->next)
		string_list_append(dst, src->s);
}

/* Given "option", allocate a single string of the form "-option"
 * ("/option" on DOS) and append this string to the given list.
 */
static void string_list_append_string_option(list, option)
String_list*	list;
char*		option;
{
	String_list new_string, p, t;

	new_string = (String_list) xmalloc(sizeof(String_list_link));
	new_string->next = NULL;
	new_string->s = xmalloc(1 + strlen(option) + 1);
#if defined(DOS)
	new_string->s[0] = '/';
#else
	new_string->s[0] = '-';
#endif
	(void) strcpy(new_string->s+1, option);

	p = NULL; t = *list;	while (t) { p = t; t = t->next; }
	if (p)
		p->next = new_string;
	else
		*list = new_string;
}

/* Given 'X' and "argument"(possibly NULL), allocate a single string of the
 * form "-Xargument" (/Xargument on DOS), and append this string to the
 * given list.
 */
static void string_list_append_opt_and_arg(list, option, argument)
String_list*	list;
char		option;
char*		argument;
{
	String_list new_string, p, t;

	new_string = (String_list) xmalloc(sizeof(String_list_link));
	new_string->s = xmalloc(3 + (argument ? strlen(argument) : 0));
#if defined(DOS)
	new_string->s[0] = '/';
#else
	new_string->s[0] = '-';
#endif
	new_string->s[1] = option;
	if (argument)
		(void) strcpy(new_string->s+2, argument);
	else
		new_string->s[2] = '\0';
	new_string->next = NULL;

	p = NULL; t = *list;	while (t) { p = t; t = t->next; }
	if (p)
		p->next = new_string;
	else
		*list = new_string;
}

/*
 * Define a list of files to be removed during abnormal program termination.
 *
 * The recommended procedure is to add a file to this list before the
 * file is processed, and then to delete the file from this list either
 * after removing it yourself, or after it is determined the file is good
 * and should not be removed.
 *
 * It is the caller's responsibility to ensure the file name pointer
 * remains valid while it is on this list.
 */

#define MAX_FILE_REMOVALS	8

static char*	file_removal_list[MAX_FILE_REMOVALS] = { NULL };
#define				cpp_removal_index	0
#define				cc1_removal_index	1
#define				asm_removal_index	2
#define				lnk_removal_index	3
#define				sinfo_removal_index	4
	/* sinfo is the temporary file passed to cpp and cc1 via -sinfo */
#define				list_tmp_removal_index	5
	/* list_tmp is the temporary file passed to cc1 via -z */
#define				bname_tmp_removal_index	6
	/* bname_tmp is the temporary file used by cc1 for
	 * name disambiguation during profiling.
	 */
#define 			x960_removal_index	7
	/* a file possibly created by linker for use by db_x960 */

static char*	sinfo_file_name = NULL;
static char*	list_tmp_file_name = NULL;
static char*	bname_tmp_file_name = NULL;

#define is_sinfo_needed()	(!cmd_line.gcc_style_diagnostics || \
				  cmd_line.interleave_source || \
				  (cmd_line.object_module_format == OMF_ELF \
				   && cmd_line.debug_info > DEBUG_INFO_OFF) ||\
				  (is_listing_enabled()))

static void
record_file_for_removal(idx, name)
int	idx;
char	*name;	/* Use NULL to take a file out of this list. */
{
	assert(idx >= 0 && idx < MAX_FILE_REMOVALS);
	file_removal_list[idx] = name;
}


void
remove_temporary_files()
{
	int	i;

	for (i=0; i < MAX_FILE_REMOVALS; i++)
	{
		if (file_removal_list[i]) {
			(void) unlink(file_removal_list[i]);
			file_removal_list[i] = NULL;
		}
	}
}

/* Provide for temporary file names.  The caller must provide
 * a suffix ("." plus at most 3 characters because of DOS).
 * A buffer, residing in the heap, holding the returned value is returned
 * for each call.  Note that for a given suffix, this function
 * will always return a unique name.
 */

static char* get_temporary_file_name(suffix)
char	*suffix;
{
#define	MAX_SUFFIX_LENGTH 4	/* "." plus 3 characters */
#define GET_960_TOOLS_TEMPLATE "icXXXXXX"
    char template[sizeof(GET_960_TOOLS_TEMPLATE) + MAX_SUFFIX_LENGTH + 1];

    assert (suffix && *suffix == '.'
	    && (int) strlen(suffix) <= MAX_SUFFIX_LENGTH);

    (void) sprintf(template,"%s%s",GET_960_TOOLS_TEMPLATE,suffix);

    return get_960_tools_temp_file(template,xmalloc);
}

/* Determine the type of an input file, and extract its base name. */

#define		C_SOURCE_FILE			1	/* Order is important */
#define		PREPROCESSED_C_SOURCE_FILE	2
#define		ASSEMBLY_SOURCE_FILE		3
#define		OTHER_FILE			4

static void classify_input_file(filename, type, base_name)
char*	filename;
int*	type;
char**	base_name;
{
	char	*base, *suffix, *p;
	*base_name = NULL;
	*type = OTHER_FILE;

	base = base_name_ptr(filename);
	suffix = NULL;
	for (p = base; *p; p++)
		if (*p == '.')
			suffix = p;
	if (suffix) {
		*base_name = xmalloc((suffix - base) + 1);
		(void) strncpy(*base_name, base, suffix - base);
		(*base_name)[suffix - base] = '\0';
	} else {
		replace_string(base_name, base);
	}

	if (suffix && strlen(suffix) == 2)
	{
		char	ch = suffix[1];
#if defined(DOS)
		ch = tolower(ch);
#endif
		if (ch == 'c')		*type = C_SOURCE_FILE;
		else if (ch == 'i')	*type = PREPROCESSED_C_SOURCE_FILE;
		else if (ch == 's')	*type = ASSEMBLY_SOURCE_FILE;
	}
}

static void check_file_name_conflicts()
{
	/* Do a cursory check that the user-specified output file name
	 * does not match any input or listing file names.
	 * Things we check here include:
	 *
	 * - explicit output file must differ from any listing files
	 * - explicit output file must differ from any input files
	 * - explicit listing file must differ from any input files
	 *
	 * Things checked elsewhere include:
	 * (grep for 'is_same_file' to find all checks)
	 *
	 * - if linking, explicit output file must differ from
	 *   any ".o" files we will produce
	 * - explicit listing file must differ from any ".i", ".s"
	 *   and ".o" files we will produce.
	 *
	 * This check compares files by name.  We will not detect
	 * aliases to the same file.
	 */

	String_list	ifile;
	Boolean		out_has_listing_file_suffix = FALSE;
	Boolean		output_file_will_be_used =
			(cmd_line.output_file_name != NULL
			&& (cmd_line.stop_after == STOP_AT_PP_FILE
				|| cmd_line.stop_after > STOP_AT_SYNTAX));

	/* Report an error if the output file name will be written to,
	 * and is the same as the listing file name, if given.
	 */

	if (output_file_will_be_used && is_listing_enabled()
	    && cmd_line.listing_file_name != NULL
	    && is_same_file_by_name(cmd_line.listing_file_name,
				cmd_line.output_file_name))
	{
		cmd_line_error_str("output file and listing file are identical: %s", cmd_line.output_file_name);
	}

	if (!output_file_will_be_used && !is_listing_enabled())
		return;

	/* Classify the output file name's suffix */
	if (cmd_line.output_file_name != NULL)
	{
		int out_length = strlen(cmd_line.output_file_name);
		if (out_length >= 2
		    && cmd_line.output_file_name[out_length-2] == '.')
		{
			char	ch = cmd_line.output_file_name[out_length-1];
#if defined(DOS)
			if (tolower(ch) == 'l')
#else
			if (ch == 'L')
#endif
				out_has_listing_file_suffix = TRUE;
		}
	}

	for (ifile = cmd_line.list_of_input_files; ifile; ifile = ifile->next)
	{
		int	file_type;
		char*	base_name = NULL;

		classify_input_file(ifile->s, &file_type, &base_name);

		if (output_file_will_be_used)
		{
			/* Error if output file matches input file */
			if (is_same_file_by_name(ifile->s,
					cmd_line.output_file_name))
				cmd_line_error_str("input file and output file are identical: %s", ifile->s);

			/* Error if output file matches default list file */
			if (out_has_listing_file_suffix
			    && is_listing_enabled()
			    && cmd_line.listing_file_name == NULL
			    && file_type <= PREPROCESSED_C_SOURCE_FILE
			    )
			{
				char* lname = xmalloc(strlen(base_name)+3);
				(void)strcpy(lname, base_name);
				(void)strcat(lname, ".L");
				if (is_same_file_by_name(lname,
					cmd_line.output_file_name))
				{
					cmd_line_error_str("output file and listing file are identical: %s", lname);
				}
				free(lname);
			}
		}

		if (is_listing_enabled() && cmd_line.listing_file_name)
		{
			/* Error if listing file matches input file name. */
			if (is_same_file_by_name(ifile->s,
						cmd_line.listing_file_name))
				cmd_line_error_str("input file and listing file are identical: %s", ifile->s);

		}

		if (base_name)
			free(base_name);
	}
}

static void resolve_command_line_conflicts()
{
#if 0
	if (cmd_line.generate_extended_calls && cmd_line.generate_pic)
		cmd_line_error_str("illegal combination of code generation options: -Gxc and -Gpc", (char*)NULL);
	/* ic960 did not support this combination, but gcc does. */
#endif
        if (cmd_line.generate_abi && cmd_line.char_unsigned)
		cmd_line_error_str("illegal combination of code generation options: -Gabi and -Gcu", (char*)NULL);

	if (cmd_line.generate_abi && cmd_line.backward_compatible)
		cmd_line_error_str("illegal combination of code generation options: -Gabi and -Gbc", (char*)NULL);

	if (cmd_line.generate_abi && cmd_line.structure_alignment != -1)
		cmd_line_error_str("illegal combination of code generation options: -Gabi and -Gac", (char*)NULL);

	if (cmd_line.generate_big_endian &&
			!arch_supports_big_endian(cmd_line.architecture))
		cmd_line_error_str("The big endian option (-Gbe) is not supported with the %s architecture", cmd_line.architecture->ic960_name);

	/* If -C is given without -E or -P, assume -C is not given. */
	if (cmd_line.keep_comments && (cmd_line.stop_after < STOP_AT_PP_EXPAND
				   ||  cmd_line.stop_after > STOP_AT_PP_FILE))
		cmd_line.keep_comments = FALSE;

	/* If -M is given without -S, assume -M is not given. */
	if (cmd_line.stop_after != STOP_AT_ASSEMBLY)
		cmd_line.interleave_source = FALSE;

	/* If -Z is given without -z, assume -zs is given. */
	if (cmd_line.listing_file_name != NULL && !is_listing_enabled())
		cmd_line.listing_options = LISTING_PRIMARY_SOURCE;

	/* If -a is given, -w must be 0 or 1 */
	if (cmd_line.strict_ansi && cmd_line.diagnostic_level > 1)
		cmd_line.diagnostic_level = 1;

	/* Only Elf/DWARF supports multiple -g levels.
	   For other OMF's, treat all levels as DEBUG_INFO_NORMAL.
	 */
	if (cmd_line.object_module_format != OMF_ELF
	    && cmd_line.debug_info > DEBUG_INFO_OFF)
		cmd_line.debug_info = DEBUG_INFO_NORMAL;

	/* Resolve -O[0123], -g, -q{p1|p2} and 2pass -f options. */

	if (cmd_line.profile_level == PROFILE_RECOMPILATION)
	{
		/* Warn if user specified something other than -O3 */
		if (cmd_line.optimization_level >= 0 &&
				cmd_line.optimization_level != 3)
			cmd_line_warn_str("profile recompilation requires optimization level 3, changing to -O3",(char*)NULL);
		cmd_line.optimization_level = 3;
	}
	else if (cmd_line.profile_level == PROFILE_INSTRUMENTATION)
	{
		/* We use gcc optimization -O4_1 for profile instrumentation,
		 * so that ic960's -O level is irrelevant.
		 *
		 * For compatibility with ic960 R4.0, warn if user
		 * specified something other than -O1.
		 */
		if (cmd_line.optimization_level >= 0 &&
				cmd_line.optimization_level != 1)
			cmd_line_warn_str("profile instrumentation requires optimization level 1, changing to -O1",(char*)NULL);

		/* Setting opt level to 1 is NOT meaningless here.
		 * Other parts of the driver may assume that opt level == 1
		 * when profile instrumentation is enabled.
		 */
		cmd_line.optimization_level = 1;
	}
	else if (cmd_line.optimization_level == 3)
		cmd_line.profile_level = PROFILE_RECOMPILATION;
	else if (force_O1())
	{
		/* Optimization level must be at least -O1.
		   Silently make this so.
		   Note that the interaction of these -f options
		   with -q and -O3 is not very well defined.
		 */
		if (cmd_line.optimization_level < 1)
			cmd_line.optimization_level = 1;
	}
	else if (cmd_line.optimization_level < 0
		 && cmd_line.debug_info > DEBUG_INFO_OFF)
		cmd_line.optimization_level = 0;
	else if (cmd_line.optimization_level < 0)
		cmd_line.optimization_level = 1;

	/* Check for a program database. */
	if (cmd_line.profile_database == NULL)
	{
		char* pdb = getenv(IC_ENV_PDB);
		if (pdb && *pdb != '\0')
			replace_string(&cmd_line.profile_database, pdb);
	}

	if (cmd_line.spill_register_address >= 0 ||
			cmd_line.spill_register_symbol != NULL ||
			cmd_line.spill_register_length > 0)
	{
		cmd_line_warn_str("The spill register area options (-Gsa and -Gsl) are not supported in this release", (char*)NULL);
		cmd_line.spill_register_address = -1;
		cmd_line.spill_register_length = -1;
		if (cmd_line.spill_register_symbol) {
			free(cmd_line.spill_register_symbol);
			cmd_line.spill_register_symbol = NULL;
		}
	}

	/* If no input files are specified and we are not linking,
	 * issue an error.  If linking, the object files may all be
	 * listed in a linker directive file, or passed with -Wl.
	 * Let the linker determine whether there is a problem there.
	 */

	if (cmd_line.stop_after < STOP_AT_EXECUTABLE &&
			cmd_line.list_of_input_files == NULL)
		cmd_line_error_str("missing source file name", (char*)NULL);

	/* Error if the -o option is used with more than one input file. */
	if (cmd_line.stop_after < STOP_AT_EXECUTABLE &&
			cmd_line.output_file_name != NULL &&
			(cmd_line.list_of_input_files == NULL ||
			cmd_line.list_of_input_files->next != NULL))
		cmd_line_error_str("cannot use output option (-o) with multiple input files, except when linking", (char*)NULL);

	check_file_name_conflicts();
}

/* Parse the -G command line option. */

static void parse_generate_options(list)
char*	list;
{
	char	*option, *opt_arg;
	Boolean	opt_arg_required;
	char	*buf = xmalloc(strlen(list) + 1);
	(void) strcpy(buf, list);

	for(option = strtok(buf,","); option; option = strtok((char*)NULL,","))
	{
		if (strcmp(option, "clean-linkage") == 0 ||
			strcmp(option, "noclean-linkage") == 0)
		{
			cmd_line_warn_str("obsolete code generation option: %s",
						option);
			continue;
		}

		/* get the optional argument, if any. */
		if ((opt_arg = strchr(option, '=')) != NULL)
			*opt_arg++ = '\0';

		opt_arg_required = FALSE;

		if (strcmp(option,"xc") == 0)
			cmd_line.generate_extended_calls = TRUE;
		else if (strcmp(option,"pc") == 0)
			cmd_line.generate_pic = TRUE;
		else if (strcmp(option,"pd") == 0)
			cmd_line.generate_pid = TRUE;
		else if (strcmp(option,"pr") == 0)
			cmd_line.preserve_g12 = TRUE;
		else if (strcmp(option,"bc") == 0)
			cmd_line.backward_compatible = TRUE;
		else if (strcmp(option,"be") == 0)
			cmd_line.generate_big_endian = TRUE;
		else if (strcmp(option,"cave") == 0)
			cmd_line.generate_cave = TRUE;
		else if (strcmp(option,"cu") == 0)
			cmd_line.char_unsigned = TRUE;
		else if (strcmp(option,"cs") == 0)
			cmd_line.char_unsigned = FALSE;
		else if (strcmp(option,"dc") == 0)
			cmd_line.strict_refdef = FALSE;
		else if (strcmp(option,"ds") == 0)
			cmd_line.strict_refdef = TRUE;
		else if (strcmp(option,"abi") == 0)
			cmd_line.generate_abi = TRUE;
		else if (strcmp(option,"cpmul") == 0)
			cmd_line.generate_cpmulo = TRUE;
		else if (strcmp(option,"ac") == 0)
		{
			opt_arg_required = TRUE;
			if (opt_arg)
			{
				char	*end_ptr;
				long	struct_align =
						strtol(opt_arg, &end_ptr, 0);
				if (*end_ptr || (struct_align != 1 &&
				    struct_align != 2 && struct_align != 4 &&
				    struct_align != 8 && struct_align != 16))
				{
					cmd_line_error_str(
					"invalid struct alignment: %s",opt_arg);
				}

				cmd_line.structure_alignment = struct_align;
			}
		}
		else if (strcmp(option,"sa") == 0)
		{
			opt_arg_required = TRUE;
			if (opt_arg)
			{
				char	*end_ptr;
				long	base = strtol(opt_arg, &end_ptr, 0);
				if (*end_ptr)
					replace_string(
						&cmd_line.spill_register_symbol,
						    opt_arg);
				else
					cmd_line.spill_register_address = base;
			}
		}
		else if (strcmp(option,"sl") == 0)
		{
			opt_arg_required = TRUE;
			if (opt_arg)
			{
				char	*end_ptr;
				long	length = strtol(opt_arg, &end_ptr, 0);
				if (*end_ptr || length <= 0)
				{
					cmd_line_error_str(
					"invalid spill area length: %s",
							opt_arg);
				}
				cmd_line.spill_register_length = length;
			}
		}
		else if (strcmp(option,"wait") == 0)
		{
			opt_arg_required = TRUE;
			if (opt_arg)
			{
				char	*end_ptr;
				long	val = strtol(opt_arg, &end_ptr, 0);
				if (*end_ptr || val < 0 || val > 32)
				{
					cmd_line_error_str(
					"invalid wait state: %s",opt_arg);
				}

				cmd_line.wait_state = val;
			}
		}
		else
			cmd_line_error_str("invalid code generation option: %s",
								option);

		if (opt_arg && !opt_arg_required)
			cmd_line_error_str("invalid optional argument: %s",
								opt_arg);
		if (!opt_arg && opt_arg_required)
			cmd_line_error_str("option %s requires an argument",
								option);
	}

	free(buf);
}

/* Parse the -J command line option. */

static void parse_style_options(list)
char*	list;
{
	char	*option;
	char	*buf = xmalloc(strlen(list) + 1);
	(void) strcpy(buf, list);

	for(option = strtok(buf,","); option; option = strtok((char*)NULL,","))
	{
		if (strcmp(option,"gd") == 0)
			cmd_line.gcc_style_diagnostics = TRUE;
		else if (strcmp(option,"nogd") == 0)
			cmd_line.gcc_style_diagnostics = FALSE;
		else if (strcmp(option,"cq") == 0)
			cmd_line.compiler_quiet = TRUE;
		else if (strcmp(option,"nocq") == 0)
			cmd_line.compiler_quiet = FALSE;
		else
			cmd_line_error_str("invalid style option: %s", option);
	}

	free(buf);
}

/* Parse the -W diagnostic command line option. */

static void parse_big_W_diagnostic_option(option)
char*	option;
{
	Big_W_value	*w;
	char		*end_ptr, *name = option;
	int		status = EXPLICITLY_ENABLED;

	if (strncmp(option, "no-", 3) == 0)
	{
		name += 3;
		status = EXPLICITLY_DISABLED;
	}

	/* Handle -Wid-clash-len specially */
	if (strncmp(name, "id-clash-",9) == 0)
	{
		if (status != EXPLICITLY_DISABLED
		    && strtol(name+9, &end_ptr, 0) > 0
		    && *end_ptr == '\0')
		{
			for (w = &big_W_options[0]; w->name; w++)
			{
				if (strncmp(w->name,"id-clash-",9) == 0)
				{
					w->value = option;
					return;
				}
			}
		}
		/* Else fall thru and report an error. */
	}
	else
	{
		for (w = &big_W_options[0]; w->name; w++)
		{
			if (strcmp(name, w->name) == 0)
			{
				/* -Wall does not have a no- form */
				if (status == EXPLICITLY_DISABLED
				    && strcmp(name,"all") == 0)
					break;	/* Report an error */

				w->value = option;
				return;
			}
		}
	}

	cmd_line_error_str("invalid diagnostic control: -W%s", option);
}

/* Parse the -W "pass" command line option. */

static void parse_pass_options(list)
char*	list;
{
	char	*option;
	char	phase = *list;
	char	*buf;

	/* list must begin with "[aclp][, ]" */

	if ((list[1] != ',' && list[1] != ' ') ||
	    (phase != 'a' && phase != 'c' && phase != 'l' && phase != 'p'))
	{
		/* Must be a diagnostic option. */
		parse_big_W_diagnostic_option(list);
		return;
	}

	buf = xmalloc(strlen(list) + 1);

	(void) strcpy(buf, list+2);
	if ((option = strtok(buf, ",")) == NULL)
		cmd_line_error_str("invalid pass option: %s", list);

	do {
		if (phase == 'a')
			string_list_append(&cmd_line.assembler_pass_options,
								option);
		else if (phase == 'c')
			string_list_append(&cmd_line.compiler_pass_options,
								option);
		else if (phase == 'l')
			string_list_append(&cmd_line.linker_options, option);
		else /* if (phase == 'p') */
			string_list_append(&cmd_line.cpp_pass_options, option);
	} while (option = strtok((char*)NULL, ","));

	free(buf);
}

/* Parse the -q command line option. */

static void parse_profile_options(list)
char*	list;
{
	char	*option;
	char	*buf = xmalloc(strlen(list) + 1);

	(void) strcpy(buf, list);
	if ((option = strtok(buf, ",")) == NULL)
		cmd_line_error_str("invalid profile option: %s", list);

	do {
		if (!strcmp(option,"p1"))
			cmd_line.profile_level = PROFILE_INSTRUMENTATION;
		else if (!strcmp(option,"p2"))
			cmd_line.profile_level = PROFILE_RECOMPILATION;
		else if (!strcmp(option,"l"))
		{
			char* library = strtok((char*)NULL, ",");
			if (!library || *library == '\0')
				cmd_line_error_str("option requires a library name: l,<library>", (char*)NULL);
#if defined(DOS)
			replace_string(&cmd_line.profile_library,
				normalize_file_name(library));
#else
			replace_string(&cmd_line.profile_library, library);
#endif
		}
		else
			cmd_line_error_str("invalid profile option: %s",option);
	} while (option = strtok((char*)NULL, ","));

	free(buf);
}

/* Parse the -f command line option. */

static void parse_little_f_option(option)
char*	option;
{
	Little_f_value	*f;
	char		*name = option;
	int		status = EXPLICITLY_ENABLED;

	if (strncmp(option, "no-", 3) == 0)
	{
		name += 3;
		status = EXPLICITLY_DISABLED;
	}

	for (f = &little_f_options[0]; f->name; f++)
	{
		if (strcmp(name, f->name) == 0)
		{
			f->value = option;

			if (strcmp(name, "build-db") == 0)
				cmd_line.flag_fbuild_db = status;
			else if (strcmp(name, "db") == 0)
				cmd_line.flag_fdb = status;
			else if (strcmp(name, "prof") == 0)
				cmd_line.flag_fprof = status;
			return;
		}
	}

	cmd_line_error_str("invalid option: -f%s", option);
}

/* Parse the -F command line option. */

static void parse_fine_tune_options(list)
char*	list;
{
	char	*option;
	char	*buf = xmalloc(strlen(list) + 1);

/* We only support a handful of the fine tune optimizations
 * that ic960 R4.0 supported.  Use -f or  -Wc,f... to
 * enable/disable the other optimizations.
 */
static	unsigned long	supported_fine_tune_opts =
		OPT_AUTO_INLINING |
                OPT_LEAF_PROC |
                OPT_FUNCTION_PLACEMENT |
		OPT_SUPER_BLOCKS |
                OPT_TAIL_CALL_ELIMINATION |
		OPT_COMPARE_AND_BRANCH |
		OPT_CODE_ALIGN |
		OPT_STRICT_ALIGN;

	(void) strcpy(buf, list);
	if ((option = strtok(buf, ",")) == NULL)
		cmd_line_error_str("invalid fine tune or OMF option: %s", list);

	do {
		char*	opt = option;
		Boolean	enable = TRUE;
		unsigned long	mask = OPT_NONE;

		/* As of ic960 R5.0, -F is overloaded to accept
		 * an object module format.
		 */

		if (strcmp(option,"coff") == 0) {
			cmd_line.object_module_format = OMF_COFF;
			continue;
		} else if (strcmp(option,"elf") == 0) {
			cmd_line.object_module_format = OMF_ELF;
			continue;
		} else if (strcmp(option,"bout") == 0) {
			cmd_line_error_str("Unsupported OMF option: %s",option);
			continue;	/* NOT REACHED */
		}

		if (strncmp(option,"no",2) == 0) {
			opt = option+2;
			enable = FALSE;
		}

		if (!strcmp(opt, "lp")) mask = OPT_LEAF_PROC;
		else if (!strcmp(opt, "ca"))	mask = OPT_CODE_ALIGN;
		else if (!strcmp(opt, "cb"))	mask = OPT_COMPARE_AND_BRANCH;
		else if (!strcmp(opt, "sa"))	mask = OPT_STRICT_ALIGN;
		else if (!strcmp(opt, "ci"))	mask = OPT_COLLAPSE_IDENTITIES;
		else if (!strcmp(opt, "sis"))	mask = OPT_SPECIAL_INSTRUCTIONS;
		else if (!strcmp(opt, "lvp"))	mask = OPT_LOCAL_PROMOTION;
		else if (!strcmp(opt, "bo"))	mask = OPT_BRANCH_OPT;
		else if (!strcmp(opt, "tce"))	mask =OPT_TAIL_CALL_ELIMINATION;
		else if (!strcmp(opt, "is"))	mask = OPT_SCHEDULING;
		else if (!strcmp(opt, "cp"))	mask = OPT_CONSTANT_PROPAGATION;
		else if (!strcmp(opt, "cse"))	mask = OPT_CSE;
		else if (!strcmp(opt, "pi"))	mask = OPT_PRAGMA_INLINING;
		else if (!strcmp(opt, "lts"))	mask = OPT_LIVE_TRACK_SEPARATOR;
		else if (!strcmp(opt, "dce"))	mask = OPT_DEAD_CODE;
		else if (!strcmp(opt, "dfe"))	mask = OPT_DEAD_FUNCTION_ELIM;
		else if (!strcmp(opt, "licm"))	mask = OPT_LOOP_CODE_MOTION;
		else if (!strcmp(opt, "cc"))	mask = OPT_CODE_COMPACTION;
		else if (!strcmp(opt, "gvm"))	mask = OPT_GLOBAL_VAR_MIGRATE;
		else if (!strcmp(opt, "ive"))	mask = OPT_INDUCTION_VARIABLES;
		else if (!strcmp(opt, "alias"))	mask = OPT_ALIAS_ANALYSIS;
		else if (!strcmp(opt, "lcse"))	mask = OPT_LOCAL_CSE;
		else if (!strcmp(opt, "ai"))	mask = OPT_AUTO_INLINING;
		else if (!strcmp(opt, "bbr"))	mask = OPT_BLOCK_REARRANGEMENT;
		else if (!strcmp(opt, "gpi"))	mask = OPT_GLOBAL_INLINING;
		else if (!strcmp(opt, "pbp"))	mask=OPT_PROFILE_BRANCH_PREDICT;
		else if (!strcmp(opt, "sb"))	mask = OPT_SUPER_BLOCKS;
		else if (!strcmp(opt, "pf"))	mask = OPT_FUNCTION_PLACEMENT;
		else
			cmd_line_error_str("invalid fine tune option: %s",
								option);

		if ((supported_fine_tune_opts & mask) == 0)
		{
			cmd_line_warn_str("fine tune option %s is not supported, use corresponding -Wc,[fm] option", option);
			mask = OPT_NONE;
		}

		if (enable) {
			cmd_line.fine_tune_enable |= mask;
			cmd_line.fine_tune_disable &= ~mask;
		} else {
			cmd_line.fine_tune_enable &= ~mask;
			cmd_line.fine_tune_disable |= mask;
		}
	} while (option = strtok((char*)NULL, ","));
	free(buf);
}

/* Parse the -A command line option or the value of IC_ENV_ARCH. */

static void parse_architecture(arch)
char*	arch;
{
	Target_descr *t;

	for (t = &targets[0]; t->ic960_name; t++) {
		if (strcmp(arch, t->ic960_name) == 0) {
			cmd_line.architecture = t;
			break;
		}
	}

	if (!t->ic960_name)
		cmd_line_error_str("unknown architecture: %s", arch);
}

static void parse_command_line_arguments(argc, argv)
int argc;
char *argv[];
{
	int	argv_index;	/* current index into argv[] */
	char*	cur_opt;	/* walks through options following '-' */
	char	opt_char;	/* option pointed to by cur_opt */
	char*	opt_found;	/* Is opt_char a valid option? */
	char*	opt_arg;	/* argument following cur_opt, or NULL */
	int	opt_int;	/* optional non-negative integer argument
				   for some options, -1 means not specified */

	for (argv_index = 1; argv_index < argc; argv_index++)
	{
		if (argv[argv_index][0] != '-'
#if defined(DOS)
			&& argv[argv_index][0] != '/' /* Allow - or / on DOS */
#endif
		   )
		{
#if defined(DOS)
			(void) normalize_file_name(argv[argv_index]);
#endif
			string_list_append(&cmd_line.list_of_input_files,
							argv[argv_index]);
			continue;
		}
		if (argv[argv_index][1] == '\0')
			cmd_line_error_str("no option following -",(char*)NULL);

		/* Handle -gcdm option specially.  It and -v960 are the
		   only ic960 options that consist of more than one letter.
		 */
		if (strncmp(argv[argv_index] + 1, "gcdm", 4) == 0)
		{
			string_list_append_string_option(
					&cmd_line.linker_options,
					argv[argv_index] + 1);
			continue;
		}

		/* Parse all options following the hyphen. */
		cur_opt = argv[argv_index] + 1;
		for (; *cur_opt; cur_opt++)
		{
			opt_char = *cur_opt;
			if ((opt_found = strchr(OPT_STRING, opt_char)) == NULL
			     || opt_char == ':')
			{
				char buf[2];
				buf[0] = opt_char;
				buf[1] = '\0';
				cmd_line_error_str("Unrecognized option -%s",
									buf);
			}

			/* Get the option's argument, if mandated */

			opt_arg = NULL;
			opt_int = -1;

			if (opt_found[1] == ':' && opt_found[2] != ':')
			{
				/* An argument must be specified. */

				if (cur_opt[1] != '\0')
					opt_arg = cur_opt + 1;
				else if (argv_index+1 < argc)
					opt_arg = argv[++argv_index];
				else
				{
					char buf[2];
					buf[0] = opt_char;
					buf[1] = '\0';
					cmd_line_error_str("Option %s requires an argument", buf);
				}
			}
			else if (opt_found[1] == ':' && opt_found[2] == ':')
			{
				/* A non-negative decimal argument *may*
				   be specified.
				 */

				if (cur_opt[1] != '\0')
				{
					if (isdigit(cur_opt[1]))
						opt_arg = cur_opt + 1;
				}
				else if (argv_index+1 < argc
					 && isdigit(argv[argv_index+1][0]))
					opt_arg = argv[++argv_index];

				if (opt_arg)
				{
					char	*end_ptr;
					opt_int = (int)
						strtol(opt_arg, &end_ptr, 0);
					if (*end_ptr)
						cmd_line_error_str("ill-formed decimal integer: %s", opt_arg);
				}
			}

			if (opt_arg)
			{
				/* Advance cur_opt to opt_arg's last character
				   so the for loop will see '\0' next time.
				 */
				cur_opt = opt_arg + strlen(opt_arg) - 1;
			}

			switch (opt_char)
			{
			case 'a': cmd_line.strict_ansi = TRUE; break;
			case 'A': parse_architecture(opt_arg); break;
			case 'b':
				{
				char* end_ptr;
				long size = strtol(opt_arg, &end_ptr, 0);
				if (*end_ptr || size < 0)
					cmd_line_error_str("invalid optimization size limit: %s", opt_arg);
				cmd_line.limit_optimizations = size;
				}
				break;
			case 'c': cmd_line.stop_after = STOP_AT_OBJECT; break;
			case 'C': cmd_line.keep_comments = TRUE; break;
			case 'D': string_list_append_opt_and_arg(
					&cmd_line.list_of_macro_definitions,
					opt_char, opt_arg);
				break;
			case 'E': cmd_line.stop_after =STOP_AT_PP_EXPAND; break;
			case 'f': parse_little_f_option(opt_arg); break;
			case 'F': parse_fine_tune_options(opt_arg); break;
			case 'g':
				if (opt_int < 0)
					cmd_line.debug_info = DEBUG_INFO_NORMAL;
				else if (opt_int <= DEBUG_INFO_VERBOSE)
					cmd_line.debug_info = opt_int;
				else
					cmd_line_error_str("invalid debug information level: %s", opt_arg);
				break;
			case 'G': parse_generate_options(opt_arg); break;
			case 'h': print_help_list();
				exit(SUCCESS_EXIT_CODE); break;
			case 'i': string_list_append_string_option(
					&cmd_line.preinclude_files, "include");
#if defined(DOS)
				(void) normalize_file_name(opt_arg);
#endif
				string_list_append(&cmd_line.preinclude_files,
								opt_arg);
				break;
			case 'I':
#if defined(DOS)
				(void) normalize_file_name(opt_arg);
#endif
				string_list_append_opt_and_arg(
					&cmd_line.search_include_directories,
					opt_char, opt_arg);
				break;
			case 'j':
				{
				char* end_ptr;
				long size = strtol(opt_arg, &end_ptr, 0);
				if (*end_ptr || size != 1)
					cmd_line_error_str("invalid errata specified: %s", opt_arg);
				cmd_line.errata_number = 1;
				}
				break;
			case 'J': parse_style_options(opt_arg); break;
			case 'l':
#if defined(DOS)
				(void) normalize_file_name(opt_arg);
#endif
				string_list_append_opt_and_arg(
					&cmd_line.linker_library_options,
					opt_char, opt_arg);
				break;
			case 'L': case 'T':
#if defined(DOS)
				(void) normalize_file_name(opt_arg);
#endif
				string_list_append_opt_and_arg(
					&cmd_line.linker_options,
					opt_char, opt_arg);
				break;
			case 'm': case 'r': case 's':
			case 'u': case 'x':
				string_list_append_opt_and_arg(
					&cmd_line.linker_options,
					opt_char, opt_arg);
				break;
			case 'M': cmd_line.interleave_source = TRUE; break;
			case 'n': cmd_line.stop_after = STOP_AT_SYNTAX; break;
			case 'o':
#if defined(DOS)
				(void) normalize_file_name(opt_arg);
#endif
				replace_string(&cmd_line.output_file_name,
							opt_arg);
				break;
			case 'O':
				if (opt_arg[1] ||
					(*opt_arg != '0' && *opt_arg != '1' &&
					 *opt_arg != '2' && *opt_arg != '3'))
				{
					cmd_line_error_str("invalid optimization level: %s", opt_arg);
				}
				cmd_line.optimization_level = *opt_arg - '0';
				break;
			case 'P': cmd_line.stop_after = STOP_AT_PP_FILE; break;
			case 'q': parse_profile_options(opt_arg); break;
			case 'Q': cmd_line.stop_after = STOP_AT_DEPENDENCIES;
				break;
			case 'R': cmd_line.report_peak_memory_usage=TRUE; break;
			case 'S': cmd_line.stop_after = STOP_AT_ASSEMBLY; break;
			case 'U': string_list_append_opt_and_arg(
					&cmd_line.list_of_macro_undefines,
					opt_char, opt_arg);
				break;
			case 'v': cmd_line.verbose = TRUE; break;
			case 'V':
				if (!cmd_line.version_requested) {
					extern char ic960_ver[];
					(void) fprintf(stderr,
						"%s\n", ic960_ver);
					cmd_line.version_requested = TRUE;
				}
				break;
			case 'w':
				if (opt_arg[1] || (*opt_arg != '0' &&
					*opt_arg != '1' && *opt_arg != '2'))
				{
					cmd_line_error_str("invalid diagnostic level: %s", opt_arg);
				}
				cmd_line.diagnostic_level = *opt_arg - '0';
				break;
			case 'W': parse_pass_options(opt_arg); break;
			case 'Y':
				if (*opt_arg != 'd' || (opt_arg[1] != ' ' &&
				     opt_arg[1] != ',') || opt_arg[2] == '\0')
				{
					cmd_line_error_str("invalid program database: %s", opt_arg);
				}
#if defined(DOS)
				(void) normalize_file_name(opt_arg+2);
#endif
				replace_string(&cmd_line.profile_database,
							opt_arg+2);
				break;
			case 'z':
			    {
				unsigned int	mask = LISTING_DISABLED;
				char		*cp = opt_arg;
				for(; *cp; cp++) {
				  switch (*cp) {
				  case 'c': mask = LISTING_CONDITIONAL_REJECT;
					break;
				  case 'i': mask = LISTING_INCLUDED_SOURCE;
					break;
				  case 'm': mask = LISTING_MACRO_EXPANSION;
					break;
				  case 'o': mask = LISTING_ASSEMBLY;
					break;
				  case 's': mask = LISTING_PRIMARY_SOURCE;
					break;
				  default:
					cmd_line_error_str(
					"invalid listing option string: %s",
							opt_arg);
				  }
				  cmd_line.listing_options |= mask;
				}
			    }
				break;
			case 'Z':
#if defined(DOS)
				(void) normalize_file_name(opt_arg);
#endif
				replace_string(&cmd_line.listing_file_name,
							opt_arg);
				break;

			default:
				cmd_line_error_str("Internal command line processing error.", (char*)NULL);
			}
		}
	}

	{
		char* base = getenv(IC_ENV_BASE);
		if (!base || *base == '\0')
			cmd_line_error_str("environment variable undefined: %s",
							IC_ENV_BASE);
		replace_string(&cmd_line.product_base_dir, base);
#if defined(DOS)
		(void) normalize_file_name(cmd_line.product_base_dir);
#endif
	}

	if (cmd_line.architecture == NULL)
	{
		char* arch_env = getenv(IC_ENV_ARCH);
		if (!arch_env || *arch_env == '\0')
			parse_architecture("KB");
		else
			parse_architecture(arch_env);
	}

	resolve_command_line_conflicts();

	if (cmd_line.verbose)
		print_a_command_line(argc, argv);
}

/* Invoke the given command and return its exit status.
 * A status > 0 indicates an error.
 * A status == 0 generally indicates success.
 * A status < 0 generally indicates a catastrophic error.
 */


int invoke_command(cmd)
String_list cmd;
{
	int	status;
	String_list	sl;
	char	**argv;
	int	argc = 0;
	static char	*c960_cmd_name = 0;

	if (!cmd) return -1;

        if (c960_cmd_name == 0)
		c960_cmd_name = get_temporary_file_name(".it1");

	/* Create an argv array for cmd */

	for (sl = cmd; sl; sl = sl->next)
		argc++;
	argv = (char**)xmalloc((3+argc) * sizeof(char*));
	for (argc = 0, sl = cmd; sl; sl = sl->next)
		argv[argc++] = sl->s;
        argv[argc++] = "-c960";
        argv[argc++] = c960_cmd_name;
	argv[argc] = NULL;

	if (cmd_line.verbose)
		print_a_command_line(argc, argv);

	/* Flush standard output streams */

	(void) fflush(stdout);
	(void) fflush(stderr);

#if defined(DOS)
	{
		char	**argv2;

		argv2 = check_dos_args(argv);
		status = spawnvp(P_WAIT, argv2[0], argv2);
		if (status < 0)
			perror(argv2[0]);
		delete_response_file();
	}
#else
	{
		int	pid = fork();
		status = 0;

		if (pid == -1)		/* fork failed */
		{
			status = -1;
			perror(my_name);
		}
		else if (pid == 0)	/* In child */
		{
			(void) execvp(argv[0], &argv[0]);
			perror(argv[0]);
			exit(-1);	/* exec failed */
		}
		else			/* In parent */
		{
			int	wait_status = 0;

			/* Wait for our child to terminate */
			while (wait(&wait_status) != pid)
				;

			if (WIFEXITED(wait_status))
			{
				/* Return the child's exit status.
				 * On some hosts, only the low 8-bits
				 * of the exit status are returned.
				 * By convention, we'll assume any exit
				 * status exceeding 0x7f is negative.
				 */
				status = WEXITSTATUS(wait_status);
				if (status > 0x7f)
					status = -1;
			} else {
				/* Child exited abnormally, not via its
				 * own call to exit or _exit.
				 */
				(void) fprintf(stderr,"%s FATAL ERROR: program %s got fatal signal %d\n", my_name, argv[0], WTERMSIG(wait_status));
				(void) unlink(c960_cmd_name);
				remove_temporary_files();
				exit(FATAL_EXIT_CODE);
				/* NOT REACHED */
				/* status = -1;	*/
			}
		}
	}
#endif
	free(argv);

	if (status == 0)
	{
		char *argv[6];
		int argc = 0;

		argv[argc++] = "cc_x960";
		argv[argc++] = c960_cmd_name;
		if (cmd_line.verbose)
			argv[argc++] = "-v";
		argv[argc] = 0;

		status = db_x960(argc, argv);
	}
	(void) unlink(c960_cmd_name);

	return status;
}


/* Create a string list containing all preprocessor invocation options
 * that are independent of the file being processed.
 */

static String_list	create_cpp_cmd_line_prefix()
{
	String_list	options = NULL;
	char*		s;
	Little_f_value	*f;
	Big_W_value	*w;
	char		buf[128];	/* scratch space */

	/* Get the preprocessor name */
	if ((s = getenv(IC_ENV_CPP)) == NULL || *s == '\0')
	{
		s = xmalloc(strlen(cmd_line.product_base_dir) +
				strlen(IC_DIR_LIB) + strlen(IC_EXE_CPP) + 3);
		(void) strcpy(s, cmd_line.product_base_dir);
		path_concat(s, s, IC_DIR_LIB);
		path_concat(s, s, IC_EXE_CPP);
	}

	string_list_append(&options, s);
	string_list_append_string_option(&options, "ic960");
	if (!cmd_line.gcc_style_diagnostics)
		string_list_append_opt_and_arg(&options, 'f', "fancy-errors");

	if (is_sinfo_needed())
	{
		string_list_append_string_option(&options, "sinfo");
		string_list_append(&options, sinfo_file_name);
	}

	string_list_append_string_option(&options, "trigraphs");
	string_list_append_string_option(&options, "undef");
	if (cmd_line.version_requested)
		string_list_append_string_option(&options, "version");

	if (cmd_line.debug_info == DEBUG_INFO_VERBOSE
	    && cmd_line.stop_after > STOP_AT_DEPENDENCIES)
	{
		(void) sprintf(buf, "%d", cmd_line.debug_info);
		string_list_append_opt_and_arg(&options, 'g', buf);
	}

	if (cmd_line.keep_comments)
		string_list_append_opt_and_arg(&options, 'C', (char*)NULL);

	if (cmd_line.stop_after == STOP_AT_DEPENDENCIES)
		string_list_append_opt_and_arg(&options, 'M', (char*)NULL);
	else if (cmd_line.stop_after == STOP_AT_PP_FILE)
		string_list_append_opt_and_arg(&options, 'P', (char*)NULL);
		/* ic960 suppresses #line directives under -P, but not -E */

	if (cmd_line.diagnostic_level == 2)	/* Disable all warnings */
		string_list_append_string_option(&options, "w");
	else if (cmd_line.diagnostic_level == 1)
		string_list_append_opt_and_arg(&options, 'w', "1");
	else
		string_list_append_opt_and_arg(&options, 'w', "0");

	/* But, allow -Wcontrol to override -w */

	for (w = &big_W_options[0]; w->name; w++)
	{
		if (w->value && (w->phases & TO_CPP))
			string_list_append_opt_and_arg(&options, 'W',w->value);
	}

	if (is_listing_enabled())
	{
		/* Add the option  -z ctrls */
		char	zoptions[32];
		build_list_option(zoptions);
		string_list_append_string_option(&options, "clist");
		string_list_append(&options, zoptions);
	}

	/* Add compiler predefined macros */
	string_list_append_opt_and_arg(&options, 'D', "__IC960");
	string_list_append_opt_and_arg(&options, 'D', "__i960");
	string_list_append_opt_and_arg(&options, 'D', "i960");

        {
		extern char ic960_ver_mac[];
		(void) sprintf(buf, "__IC960_VER=%s", ic960_ver_mac);
		string_list_append_opt_and_arg(&options, 'D', buf);
	}

	{
		char	arch_mac[6 + 9 + 1];
				/* __i960 + generous_length_for(arch) + \0 */
		assert((int)strlen(cmd_line.architecture->macro_name) <= 9);
		(void) strcpy(arch_mac, "__i960");
		(void) strcat(arch_mac, cmd_line.architecture->macro_name);

		/* add single underscore version for compatibility */
		string_list_append_opt_and_arg(&options,'D',arch_mac);
		string_list_append_opt_and_arg(&options,'D',arch_mac+1);
	}

	if (cmd_line.generate_big_endian)
		string_list_append_opt_and_arg(&options, 'D', "__i960_BIG_ENDIAN__");

	if (cmd_line.generate_pic) {
		string_list_append_opt_and_arg(&options, 'D', "__PIC");
		string_list_append_opt_and_arg(&options, 'D', "__PIC__");
	}
	if (cmd_line.generate_pid) {
		string_list_append_opt_and_arg(&options, 'D', "__PID");
		string_list_append_opt_and_arg(&options, 'D', "__PID__");
	}
	if (cmd_line.strict_ansi) {
		string_list_append_opt_and_arg(&options, 'D',"__STRICT_ANSI");
		string_list_append_opt_and_arg(&options, 'D',"__STRICT_ANSI__");
	}
	if (cmd_line.char_unsigned)
		string_list_append_opt_and_arg(&options, 'D', "__CHAR_UNSIGNED__");
	else
		string_list_append_opt_and_arg(&options, 'D', "__SIGNED_CHARS__");
	if (cmd_line.optimization_level == 0)
		string_list_append_opt_and_arg(&options, 'D', "__NO_INLINE");

	string_list_append_list(&options, cmd_line.list_of_macro_definitions);
	string_list_append_list(&options, cmd_line.list_of_macro_undefines);

	/* To be compatible with ic960 R4.0, we must specify the
	 * following order for -I:
	 *
	 * -nostdinc user_specified_-Is {IC_ENV_INC | IC_ENV_BASE/include}
	 *
	 * IC_ENV_BASE/include is used iff IC_ENV_INC is not set.
	 *
	 * The GCC preprocessor must also recognize the -ic960 option,
	 * and search for '#include "file"` in the primary C source
	 * file's directory before searching the -I directories.
	 * For '#include <file>`, only the -I directories are searched.
	 */
	string_list_append_string_option(&options, "nostdinc");
	string_list_append_list(&options, cmd_line.search_include_directories);

	if ((s = getenv(IC_ENV_INC)) && *s)
		string_list_append_opt_and_arg(&options, 'I', s);
	else
	{
		char* ibase = xmalloc(strlen(cmd_line.product_base_dir)
					+ strlen(IC_DIR_INC) + 2);
		(void) strcpy(ibase, cmd_line.product_base_dir);
		path_concat(ibase, ibase, IC_DIR_INC);
		string_list_append_opt_and_arg(&options, 'I', ibase);
		free(ibase);
	}

	string_list_append_list(&options, cmd_line.preinclude_files);

	for (f = &little_f_options[0]; f->name; f++)
	{
		if (f->value && (f->phases & TO_CPP))
			string_list_append_opt_and_arg(&options, 'f',f->value);
	}

	string_list_append_list(&options, cmd_line.cpp_pass_options);

	return options;
}

/* Append input-file-specific options to the cpp command line,
 * invoke the preprocessor, and return its exit status.
 */

static int preprocess_a_file(input_file_name, input_basename,
					cpp_cmd_prefix, output_file_name)
char		*input_file_name;
char		*input_basename;
String_list	cpp_cmd_prefix;
char		**output_file_name;
{
	String_list	cmd = NULL;
	int		status;

	/* Determine a name for the output file */
	/* Use NULL if the only output is stdout, like with -Q and -E */

	if (cmd_line.stop_after == STOP_AT_PP_FILE) {
		if (cmd_line.output_file_name)
			replace_string(output_file_name,
					cmd_line.output_file_name);
		else {
			*output_file_name = xmalloc(strlen(input_basename) + 3);
			(void) strcpy(*output_file_name, input_basename);
			(void) strcat(*output_file_name, ".i");
		}
	}
	else if (cmd_line.stop_after > STOP_AT_PP_FILE) {
		char* t = get_temporary_file_name(IC_CPP_OUT_SUFFIX);
		replace_string(output_file_name, t);
	}
	else
		assert(*output_file_name == NULL);

	if (*output_file_name)
		record_file_for_removal(cpp_removal_index, *output_file_name);

	/* Create a command line and invoke the preprocessor */

	string_list_append_list(&cmd, cpp_cmd_prefix);

	/* Add listing options, if any. */
	if (is_listing_enabled())
	{
		/* Add the option  -outz list_file  if the compiler
		 * will not be invoked.
		 */
		char	*lname = NULL;
		if (cmd_line.stop_after <= STOP_AT_PP_FILE)
		{
			if ((lname = cmd_line.listing_file_name) == NULL)
			{
				lname = (char*)xmalloc(strlen(input_basename)
					+ strlen(IC_LIST_FILE_SUFFIX) + 1);
				(void) strcpy(lname, input_basename);
				(void) strcat(lname, IC_LIST_FILE_SUFFIX);
			}
		}

		if (lname)
		{
			string_list_append_string_option(&cmd, "outz");
			string_list_append(&cmd, lname);
			if ((*output_file_name &&
				is_same_file_by_name(lname, *output_file_name))
			   || (cmd_line.output_file_name &&
				is_same_file_by_name(lname,
						cmd_line.output_file_name)))
			{
				cmd_line_error_str("output file and listing file are identical: %s", lname);
			}
			if (cmd_line.listing_file_name == NULL)
			{
				/* The preprocessor and compiler will open the
				 * listing file in append mode to support -Z.
				 * In the absence of -Z, we must remove the
				 * listing file so that cpp doesn't append to
				 * a possibly pre-existing file.
				 */
				(void) unlink(lname);
				free(lname);
			}
		}
	}

	string_list_append(&cmd, input_file_name);
	if (*output_file_name)
		string_list_append(&cmd, *output_file_name);

	status = invoke_command(cmd);

	/* Make sure a negative status is returned if the compiler is
	 * to be invoked, and cpp did not create its output file.
	 */

	if (*output_file_name && status >= 0
		&& cmd_line.stop_after >= STOP_AT_SYNTAX
		&& (access(*output_file_name, R_OK) != 0))
	{
		status = -1;
	}

	/* Delete the output file if a catastrophic error occurred,
	 * or if a regular error occurred and we will not be invoking
	 * the compiler.
	 */

	if (*output_file_name &&
		(status < 0 ||
		(status > 0 && cmd_line.stop_after <= STOP_AT_PP_FILE)))
	{
		(void) unlink(*output_file_name);
		free(*output_file_name);
		*output_file_name = NULL;
		record_file_for_removal(cpp_removal_index, (char*)NULL);
	}

	string_list_delete(&cmd);
	return status;
}

static void append_fine_tune_options_to_cc1_command(cmd)
String_list*	cmd;
{

typedef struct {
	unsigned long	opt_mask;
	char*		cc1_option_string;
	char		cc1_option_letter;
} Fine_tune_option_map;

static Fine_tune_option_map mapping[] = {
	{ OPT_AUTO_INLINING,		"no-inline-functions",	'f'},
	{ OPT_LEAF_PROC,		"no-leaf-procedures",	'm'},
#if 0
	{ OPT_FUNCTION_PLACEMENT,	"no-place-functions",	'f'},
#endif
	{ OPT_SUPER_BLOCKS,		"no-sblock",		'f'},
	{ OPT_TAIL_CALL_ELIMINATION,	"no-tail-call",		'm'},
#if 0
	/* The following options are not directly supported, but if they
	 * were here's a good stab at the corresponding -Wc,f options.
	 */
	{ OPT_BRANCH_OPT,		"no-thread-jumps",	'f'},
	{ OPT_SCHEDULING,		"no-schedule-insns",	'f'},
		/* Scheduling also requires -f[no-]schedule-insns2 */
	{ OPT_CONSTANT_PROPAGATION,	"no-constprop",		'f'},
	{ OPT_GLOBAL_VAR_MIGRATE,	"no-shadow-globals",	'f'},
	{ OPT_INDUCTION_VARIABLES,	"no-strength-reduce",	'f'},
	{ OPT_PRAGMA_INLINING,		"no-inline-functions",	'f'},
	{ OPT_GLOBAL_INLINING,		"no-glob-inline",	'f'},
	{ OPT_DEAD_FUNCTION_ELIM,	"no-keep-inline-functions",	'f'},
	{ OPT_COLLAPSE_IDENTITIES,	NULL,			'\0'},
	{ OPT_SPECIAL_INSTRUCTIONS,	NULL,			'\0'},
	{ OPT_LOCAL_PROMOTION,		NULL,			'\0'},
	{ OPT_CSE,			NULL,			'\0'},
	{ OPT_DEAD_CODE,		NULL,			'\0'},
	{ OPT_LOOP_CODE_MOTION,		NULL,			'\0'},
	{ OPT_CODE_COMPACTION,		NULL,			'\0'},
	{ OPT_LIVE_TRACK_SEPARATOR,	NULL,			'\0'},
	{ OPT_ALIAS_ANALYSIS,		NULL,			'\0'},
	{ OPT_LOCAL_CSE,		NULL,			'\0'},
	{ OPT_LOCAL_CONSTANT_FOLD,	NULL,			'\0'},
	{ OPT_BLOCK_REARRANGEMENT,	NULL,			'\0'},
	{ OPT_PROFILE_BRANCH_PREDICT,	NULL,			'\0'},
#endif
	{ OPT_COMPARE_AND_BRANCH,	"no-cmpbr",		'm'},
	{ OPT_CODE_ALIGN,		"no-code-align",	'm'},
	{ OPT_STRICT_ALIGN,		"no-strict-align",	'm'},
	{ OPT_NONE,			NULL,			'\0'}
};


	/* Determine the optimizations that need to be enabled or disabled */
	Fine_tune_option_map	*mp;

	/* Enabled and Disabled must be disjoint */
	assert((cmd_line.fine_tune_enable & cmd_line.fine_tune_disable) == 0);

	if (cmd_line.fine_tune_enable == 0 && cmd_line.fine_tune_disable == 0)
		return;

	for (mp = &mapping[0]; mp->opt_mask != OPT_NONE; mp++)
	{
		if ((mp->opt_mask & cmd_line.fine_tune_enable)
		     && mp->cc1_option_string)
		{
			string_list_append_opt_and_arg(cmd,
					mp->cc1_option_letter,
					mp->cc1_option_string + 3);
#if 0
			if (mp->opt_mask == OPT_SCHEDULING)
				string_list_append_opt_and_arg(cmd,
							mp->cc1_option_letter,
							"schedule-insns2");
#endif
		}

		if ((mp->opt_mask & cmd_line.fine_tune_disable)
		     && mp->cc1_option_string)
		{
			string_list_append_opt_and_arg(cmd,
						mp->cc1_option_letter,
						mp->cc1_option_string);
#if 0
			if (mp->opt_mask == OPT_SCHEDULING)
				string_list_append_opt_and_arg(cmd,
							mp->cc1_option_letter,
							"no-schedule-insns2");
#endif
		}
	}
}

/* Create a string list containing all compiler invocation options
 * that are independent of the file being processed.
 */

static String_list	create_cc1_cmd_line_prefix()
{
	String_list	options = NULL;
	char*		s;
	Little_f_value	*f;
	Big_W_value	*w;
	char		buf[128];	/* scratch space */

	/* Get the compiler's name */
	if ((s = getenv(IC_ENV_CC1)) == NULL || *s == '\0')
	{
		s = xmalloc(strlen(cmd_line.product_base_dir) +
				strlen(IC_DIR_LIB) + strlen(IC_EXE_CC1) + 3);
		(void) strcpy(s, cmd_line.product_base_dir);
		path_concat(s, s, IC_DIR_LIB);
		path_concat(s, s, IC_EXE_CC1);
	}

	string_list_append(&options, s);
	string_list_append_string_option(&options, "ic960");
	if (!cmd_line.gcc_style_diagnostics)
		string_list_append_opt_and_arg(&options, 'f', "fancy-errors");

	if (is_sinfo_needed())
	{
		string_list_append_string_option(&options, "sinfo");
		string_list_append(&options, sinfo_file_name);
	}

	if (cmd_line.stop_after == STOP_AT_SYNTAX)
		string_list_append_opt_and_arg(&options, 'f', "syntax-only");

	string_list_append_opt_and_arg(&options, 'f', "no-builtin");
	if (cmd_line.compiler_quiet)
		string_list_append_string_option(&options, "quiet");

	if (cmd_line.object_module_format == OMF_COFF)
		string_list_append_opt_and_arg(&options, 'F', "coff");
	else if (cmd_line.object_module_format == OMF_ELF)
		string_list_append_opt_and_arg(&options, 'F', "elf");
	else
		assert(0);

	string_list_append_opt_and_arg(&options, 'm',
				cmd_line.architecture->cc1_name);
	if (cmd_line.backward_compatible)
		string_list_append_string_option(&options, "mic2.0-compat");
	else
		string_list_append_string_option(&options, "mic3.0-compat");
	if (cmd_line.char_unsigned)
		string_list_append_opt_and_arg(&options, 'f', "unsigned-char");
	else
		string_list_append_opt_and_arg(&options, 'f', "signed-char");

	if (!cmd_line.backward_compatible &&
	    (cmd_line.fine_tune_disable & OPT_STRICT_ALIGN) == 0 &&
	    (cmd_line.fine_tune_enable & OPT_STRICT_ALIGN) == 0 &&
	    (cmd_line.structure_alignment == -1
	    || cmd_line.structure_alignment > 4)
	    && arch_dislikes_unaligned_access(cmd_line.architecture))
		string_list_append_opt_and_arg(&options, 'm', "strict-align");

	if (cmd_line.version_requested)
		string_list_append_string_option(&options, "version");
	if (cmd_line.strict_ansi) {
		string_list_append_string_option(&options, "ansi");
		string_list_append_string_option(&options, "pedantic");
	}
	if (cmd_line.diagnostic_level == 2)	/* Disable all warnings */
		string_list_append_string_option(&options, "w");
	else if (cmd_line.diagnostic_level == 1)
		string_list_append_opt_and_arg(&options, 'w', "1");
	else
		string_list_append_opt_and_arg(&options, 'w', "0");

	/* But, allow -Wcontrol to override -w */

	for (w = &big_W_options[0]; w->name; w++)
	{
		if (w->value && (w->phases & TO_CC1))
			string_list_append_opt_and_arg(&options, 'W',w->value);
	}

	if (cmd_line.limit_optimizations >= 0)
	{
		(void)sprintf(buf,"%d",cmd_line.limit_optimizations);
		string_list_append_string_option(&options, "bsize");
		string_list_append(&options, buf);
	}

	if (cmd_line.interleave_source)
		string_list_append_opt_and_arg(&options, 'f', "mix-asm");
	if (cmd_line.generate_pic)
		string_list_append_opt_and_arg(&options, 'm', "pic");
	if (cmd_line.generate_pid)
		string_list_append_opt_and_arg(&options, 'm', "pid");
	if (cmd_line.preserve_g12)
		string_list_append_opt_and_arg(&options, 'm', "pid-safe");
	if (cmd_line.generate_extended_calls)
		string_list_append_opt_and_arg(&options, 'm', "long-calls");
	if (cmd_line.generate_big_endian)
		string_list_append_opt_and_arg(&options, 'G', (char*)NULL);
	if (cmd_line.generate_cave)
		string_list_append_opt_and_arg(&options, 'm', "cave");
	if (cmd_line.strict_refdef)
		string_list_append_opt_and_arg(&options, 'm', "strict-ref-def");
	if (cmd_line.wait_state >= 0)
	{
		(void)sprintf(buf,"wait=%d",cmd_line.wait_state);
		string_list_append_opt_and_arg(&options, 'm', buf);
	}

	if (cmd_line.generate_abi)
		string_list_append_opt_and_arg(&options, 'm', "abi");

	if (cmd_line.generate_cpmulo)
		string_list_append_opt_and_arg(&options, 'm', "cpmul");

	if (cmd_line.structure_alignment >= 1 &&
	    cmd_line.structure_alignment <= 16)
	{
		(void)sprintf(buf,"i960_align=%d",cmd_line.structure_alignment);
		string_list_append_opt_and_arg(&options, 'm', buf);
	}

	/* Always pass thru the profile database
	   and the bname temporary file.
	 */
	if (cmd_line.profile_database)
	{
		string_list_append_opt_and_arg(&options, 'Z', (char*)NULL);
		string_list_append(&options, cmd_line.profile_database);
	}

	if (bname_tmp_file_name)
	{
		string_list_append_string_option(&options, "bname_tmp");
		string_list_append(&options, bname_tmp_file_name);
	}

	if (cmd_line.profile_level == PROFILE_INSTRUMENTATION)
		string_list_append_opt_and_arg(&options, 'O', "4_1");
	else if (cmd_line.profile_level == PROFILE_RECOMPILATION ||
		 cmd_line.optimization_level == 3)
		string_list_append_opt_and_arg(&options, 'O', "4_2");
	else if (cmd_line.optimization_level == 2)
		string_list_append_opt_and_arg(&options, 'O', "4");
	else if (cmd_line.optimization_level == 1)
	{
		string_list_append_opt_and_arg(&options, 'O', "1");
		/* Disable inlining at -O1, unless user enabled it with -F */
		if ((cmd_line.fine_tune_enable & OPT_AUTO_INLINING) == 0 &&
		    (cmd_line.fine_tune_enable & OPT_PRAGMA_INLINING) == 0)
		{
			string_list_append_opt_and_arg(&options, 'f',
						"no-inline-functions");
		}
	}
	else /* (cmd_line.optimization_level == 0) */
		string_list_append_opt_and_arg(&options, 'O', "0");

	if (cmd_line.debug_info > DEBUG_INFO_OFF)
	{
		/* For cleanliness, just pass -g if level is 2, since
		   -g == -g2 and non-Elf/DWARF users likely use just -g.
		 */
		if (cmd_line.debug_info != DEBUG_INFO_NORMAL)
		{
			(void) sprintf(buf, "%d", cmd_line.debug_info);
			string_list_append_opt_and_arg(&options, 'g', buf);
		}
		else
			string_list_append_opt_and_arg(&options, 'g', (char*)NULL);
	}

	append_fine_tune_options_to_cc1_command(&options);

	if (is_listing_enabled())
	{
		/* Add the option  -z ctrls */
		char	zoptions[32];
		build_list_option(zoptions);
		string_list_append_string_option(&options, "clist");
		string_list_append(&options, zoptions);
	}

	for (f = &little_f_options[0]; f->name; f++)
	{
		if (f->value && (f->phases & TO_CC1))
			string_list_append_opt_and_arg(&options, 'f',f->value);
	}

	string_list_append_list(&options, cmd_line.compiler_pass_options);

	/* Pass the driver's command line to cc1 so that it can be
	 * echoed as a comment in the assembly file.
	 */
	if (Argc > 0)
	{
		char*	list;
		int i, length=0;
		for (i = 0; i < Argc; i++)
			length += strlen(Argv[i]);
		length += Argc - 1;
		list = (char*)xmalloc(length + 1);
		(void) strcpy(list, Argv[0]);
		for (i = 1; i < Argc; i++)
		{
			(void) strcat(list, " ");
			(void) strcat(list, Argv[i]);
		}
		string_list_append_string_option(&options, "dcmd");
		string_list_append(&options, list);
	}

	return options;
}

static int compile_a_file(input_file_name, input_basename,
				cc1_cmd_prefix, output_file_name)
char		*input_file_name;
char		*input_basename;
String_list	cc1_cmd_prefix;
char		**output_file_name;
{
	String_list	cmd = NULL;
	int		status;

	/* Determine a name for the output file */

	if (cmd_line.stop_after == STOP_AT_ASSEMBLY) {
		if (cmd_line.output_file_name)
			replace_string(output_file_name,
					cmd_line.output_file_name);
		else {
			*output_file_name = xmalloc(strlen(input_basename) + 3);
			(void) strcpy(*output_file_name, input_basename);
			(void) strcat(*output_file_name, ".s");
		}
	}
	else
	{
		char* t = get_temporary_file_name(IC_CC1_OUT_SUFFIX);
		replace_string(output_file_name, t);
		assert(cmd_line.stop_after > STOP_AT_ASSEMBLY ||
				cmd_line.stop_after == STOP_AT_SYNTAX);
	}

	record_file_for_removal(cc1_removal_index, *output_file_name);

	/* Create a command line and invoke the compiler */

	string_list_append_list(&cmd, cc1_cmd_prefix);

	/* Add the dumpbase option for debugging purposes */

	string_list_append_string_option(&cmd, "dumpbase");
	string_list_append(&cmd, input_basename);

	/* Add listing options, if any. */

	if (is_listing_enabled())
	{
		/* Create the options  -outz list_file -tmpz list_tmp_file */
		char	*lname;
		if ((lname = cmd_line.listing_file_name) == NULL)
		{
			lname = (char*)xmalloc(strlen(input_basename)
				+ strlen(IC_LIST_FILE_SUFFIX) + 1);
			(void) strcpy(lname, input_basename);
			(void) strcat(lname, IC_LIST_FILE_SUFFIX);
		}

		string_list_append_string_option(&cmd, "outz");
		string_list_append(&cmd, lname);
		string_list_append_string_option(&cmd, "tmpz");
		string_list_append(&cmd, list_tmp_file_name);

		if (is_same_file_by_name(lname, *output_file_name) ||
			(cmd_line.output_file_name &&
			is_same_file_by_name(lname, cmd_line.output_file_name)))
		{
			cmd_line_error_str("output file and listing file are identical: %s", lname);
		}
		if (cmd_line.listing_file_name == NULL)
		{
			/* The preprocessor and compiler will open the
			 * listing file in append mode to support -Z.
			 * In the absence of -Z, we must remove the listing
			 * file so that cc1 doesn't append to garbage.
			 */
			(void) unlink(lname);
			free(lname);
		}
	}

	string_list_append(&cmd, input_file_name);
	string_list_append_opt_and_arg(&cmd, 'o', (char*)NULL);
	string_list_append(&cmd, *output_file_name);

	status = invoke_command(cmd);

	/* Delete the output file if an error occurred,
	 * or if only doing syntax checking.
	 */
	if (status || cmd_line.stop_after == STOP_AT_SYNTAX)
	{
		(void) unlink(*output_file_name);
		free(*output_file_name);
		*output_file_name = NULL;
		record_file_for_removal(cc1_removal_index, (char*)NULL);
	}

	string_list_delete(&cmd);
	return status;
}

/* Create a string list containing all assembler invocation options
 * that are independent of the file being processed.
 */

static String_list	create_asm_cmd_line_prefix()
{
	String_list	options = NULL;
	char*		s;

	/* Get the assembler's name */
	if ((s = getenv(IC_ENV_AS)) == NULL || *s == '\0')
	{
		s = xmalloc(strlen(cmd_line.product_base_dir)
			+ strlen(IC_DIR_BIN)
			+ strlen((cmd_line.object_module_format == OMF_COFF)
					? IC_EXE_ASM_COFF
					: IC_EXE_ASM_ELF)
			+ 3);	/* for slashes and nul */
		(void) strcpy(s, cmd_line.product_base_dir);
		path_concat(s, s, IC_DIR_BIN);
		path_concat(s, s, (cmd_line.object_module_format == OMF_COFF)
					? IC_EXE_ASM_COFF
					: IC_EXE_ASM_ELF);
	}

	string_list_append(&options, s);
	if (cmd_line.version_requested)
		string_list_append_opt_and_arg(&options, 'V', (char*)NULL);
	string_list_append_opt_and_arg(&options, 'A',
				cmd_line.architecture->ic960_name);
	if (cmd_line.errata_number != -1)
	{
		char buf[20];
		(void)sprintf(buf, "%d", cmd_line.errata_number);
		string_list_append_opt_and_arg(&options, 'j', buf);
	}

	if (cmd_line.generate_big_endian)
		string_list_append_opt_and_arg(&options, 'G', (char*)NULL);
	if (cmd_line.generate_pic && cmd_line.generate_pid)
		string_list_append_string_option(&options, "pb");
	else
	{
		if (cmd_line.generate_pic)
			string_list_append_string_option(&options, "pc");
		if (cmd_line.generate_pid)
			string_list_append_string_option(&options, "pd");
	}

	string_list_append_list(&options, cmd_line.assembler_pass_options);

	return options;
}

static int assemble_a_file(input_file_name, input_basename,
					asm_cmd_prefix, output_file_name)
char		*input_file_name;
char		*input_basename;
String_list	asm_cmd_prefix;
char		**output_file_name;
{
	String_list	cmd = NULL;
	int		status;

	/* Determine a name for the output file */

	if (cmd_line.stop_after == STOP_AT_OBJECT &&
	    cmd_line.output_file_name != NULL)
			replace_string(output_file_name,
					cmd_line.output_file_name);
	else {
		*output_file_name = xmalloc(strlen(input_basename) + 3);
		(void) strcpy(*output_file_name, input_basename);
		(void) strcat(*output_file_name, ".o");

		if (cmd_line.stop_after >= STOP_AT_EXECUTABLE
		    && cmd_line.output_file_name
		    && is_same_file_by_name(cmd_line.output_file_name,
							*output_file_name))
		{
			cmd_line_error_str("object file and output file are identical: %s", *output_file_name);
		}

	}

	if (is_listing_enabled() && cmd_line.listing_file_name
	    && is_same_file_by_name(cmd_line.listing_file_name,
						*output_file_name))
	{
		cmd_line_error_str("object file and listing file are identical: %s", *output_file_name);
	}

	record_file_for_removal(asm_removal_index, *output_file_name);

	/* Create a command line and invoke the assembler */

	string_list_append_list(&cmd, asm_cmd_prefix);
	string_list_append_opt_and_arg(&cmd, 'o', (char*)NULL);
	string_list_append(&cmd, *output_file_name);
	string_list_append(&cmd, input_file_name);

	status = invoke_command(cmd);

	/* Delete the output file if an error occurred. */
	if (status)
	{
		(void) unlink(*output_file_name);
		free(*output_file_name);
		*output_file_name = NULL;
	}

	/* The assembler's output is final at this point. */
	record_file_for_removal(asm_removal_index, (char*)NULL);

	string_list_delete(&cmd);
	return status;
}

/* Create a string list containing all linker invocation options except the
 * input file names, and libraries specified with -l.
 */

static String_list	create_link_cmd_line_prefix()
{
	String_list	options = NULL;
	char*		s;

	/* Get the linker's name */
	if ((s = getenv(IC_ENV_LD)) == NULL || *s == '\0')
	{
		s = xmalloc(strlen(cmd_line.product_base_dir) +
				strlen(IC_DIR_BIN) + strlen(IC_EXE_LNK) + 3);
		(void) strcpy(s, cmd_line.product_base_dir);
		path_concat(s, s, IC_DIR_BIN);
		path_concat(s, s, IC_EXE_LNK);
	}

	string_list_append(&options, s);

	if (cmd_line.version_requested)
		string_list_append_opt_and_arg(&options, 'V', (char*)NULL);
	if (cmd_line.verbose)
		string_list_append_opt_and_arg(&options, 'v', (char*)NULL);
#if 0
	if (cmd_line.fine_tune_enable & OPT_FUNCTION_PLACEMENT)
		string_list_append_opt_and_arg(&options, 'b', (char*)NULL);
#endif
	string_list_append_opt_and_arg(&options, 'A',
				cmd_line.architecture->ic960_name);

	if (cmd_line.object_module_format == OMF_ELF)
		string_list_append_opt_and_arg(&options, 'F', "elf");

	if (cmd_line.generate_big_endian)
		string_list_append_opt_and_arg(&options, 'G', (char*)NULL);

	if (cmd_line.generate_pic && cmd_line.generate_pid)
		string_list_append_string_option(&options, "pb");
	else
	{
		if (cmd_line.generate_pic)
			string_list_append_string_option(&options, "pc");
		if (cmd_line.generate_pid)
			string_list_append_string_option(&options, "pd");
	}

	if (cmd_line.profile_database)
	{
		string_list_append_opt_and_arg(&options, 'Z', (char*)NULL);
		string_list_append(&options, cmd_line.profile_database);
	}

	string_list_append_list(&options, cmd_line.linker_options);

	if (cmd_line.output_file_name) {
		string_list_append_opt_and_arg(&options, 'o', (char*)NULL);
		string_list_append(&options, cmd_line.output_file_name);
	}

	return options;
}

static int process_input_files()
{
	String_list	cur_input_file = cmd_line.list_of_input_files;
	Boolean		error_occurred = FALSE;
	Boolean		more_than_one_input_file = FALSE;

	String_list	cpp_options;
	String_list	cc1_options;
	String_list	asm_options;

	String_list	linker_input_files = NULL;

	/* Create temporary files used by cpp and cc1 to facilitate
	 * ic960-style diagnostics, listings, -SM, and profiling.
	 */

	if (is_sinfo_needed())
	{
		replace_string(&sinfo_file_name,
				get_temporary_file_name(IC_SINFO_SUFFIX));
		record_file_for_removal(sinfo_removal_index, sinfo_file_name);
	}
	if (is_listing_enabled())
	{
		replace_string(&list_tmp_file_name,
			get_temporary_file_name(IC_LIST_TMP_SUFFIX));
		record_file_for_removal(list_tmp_removal_index,
							list_tmp_file_name);
	}
	if (cmd_line.stop_after >= STOP_AT_SYNTAX)
	{
		replace_string(&bname_tmp_file_name,
				get_temporary_file_name(IC_BNAME_TMP_SUFFIX));
		record_file_for_removal(bname_tmp_removal_index,
					bname_tmp_file_name);
	}

	cpp_options = create_cpp_cmd_line_prefix();
	cc1_options = create_cc1_cmd_line_prefix();
	asm_options = create_asm_cmd_line_prefix();

	/* If a concatenated listing is requested (-Z filename), remove
	 * that file before invoking cpp or cc1, since they open the
	 * file in append mode for ic960.
	 */

	if (is_listing_enabled() && cmd_line.listing_file_name != NULL)
		(void) unlink(cmd_line.listing_file_name);

	if (cur_input_file && cur_input_file->next)
		more_than_one_input_file = TRUE;

	for (; cur_input_file; cur_input_file = cur_input_file->next)
	{
		int	cpp_status = 0;
		int	cc1_status = 0;
		int	asm_status = 0;
		char*	base_name = NULL;
		int	file_type;
		char*	previous_output_file_name = NULL;
			/* Name of file created by the last phase run will
			 * be the name of the input file for the next phase.
			 */

		if (more_than_one_input_file)
			(void) fprintf(stderr, "%s:\n", cur_input_file->s);

		classify_input_file(cur_input_file->s, &file_type, &base_name);

		/*
		 * Run the preprocessor, if needed.
		 */

		if (file_type == C_SOURCE_FILE)
		{
			cpp_status = preprocess_a_file(cur_input_file->s,
					base_name, cpp_options,
					&previous_output_file_name);

			if (cpp_status == 0 &&
				cmd_line.stop_after <= STOP_AT_PP_FILE)
			{
				/* The output file is final, don't remove it. */
				record_file_for_removal(cpp_removal_index,
								(char*)NULL);
			}
		}

		/*
		 * Run the compiler, if needed.
		 * Note that we invoke the compiler even in the case
		 * where cpp exited with a >0 status, so that we can
		 * check for more syntax errors.
		 */

		if (cpp_status >= 0 && cmd_line.stop_after >= STOP_AT_SYNTAX &&
			file_type <= PREPROCESSED_C_SOURCE_FILE)
		{
			/* Determine the compiler's input source file name.
			 * Make a copy, since previous_output_file_name
			 * will be overwritten by compile_a_file().
			 */

			char*	input_file = previous_output_file_name;
			previous_output_file_name = NULL;
			if (!input_file) {
				/* cpp must not have been run */
				assert(file_type == PREPROCESSED_C_SOURCE_FILE);
				replace_string(&input_file, cur_input_file->s);
			}

			/* If compiling a .i file, inform cc1 that it must
			 * create the source info file.
			 */
			if (file_type == PREPROCESSED_C_SOURCE_FILE &&
							is_sinfo_needed())
				(void) unlink(sinfo_file_name);

			cc1_status = compile_a_file(input_file, base_name,
						cc1_options,
						&previous_output_file_name);

			/* If the compiler's input file was a temporary
			 * created by the preprocessor, then delete it.
			 */
			if (file_type != PREPROCESSED_C_SOURCE_FILE)
			{
				assert(input_file);
				(void) unlink(input_file);
				/* Don't remove it twice. */
				record_file_for_removal(cpp_removal_index,
								(char*)NULL);
			}
			free(input_file);

			if (cc1_status == 0 && cpp_status > 0 &&
				cmd_line.stop_after != STOP_AT_SYNTAX &&
				previous_output_file_name)
			{
				/* cc1 succeeded even though cpp failed.
				 * Delete cc1's output file in this case.
				 * If STOP_AT_SYNTAX, then compile_a_file()
				 * has already removed the output file.
				 */
				(void) unlink(previous_output_file_name);
				free(previous_output_file_name);
				previous_output_file_name = NULL;
			}

			if (cpp_status == 0 && cc1_status == 0 &&
				cmd_line.stop_after == STOP_AT_ASSEMBLY)
			{
				/* The output file is final, don't remove it. */
				record_file_for_removal(cc1_removal_index,
								(char*)NULL);
			}
		}

		/*
		 * Run the assembler, if needed.
		 */

		if (cpp_status == 0 && cc1_status == 0 &&
			cmd_line.stop_after > STOP_AT_ASSEMBLY &&
			file_type <= ASSEMBLY_SOURCE_FILE)
		{
			/* Determine the assembler's input source file. */
			char*	input_file = previous_output_file_name;
			previous_output_file_name = NULL;
			if (!input_file) {
				assert(file_type == ASSEMBLY_SOURCE_FILE);
				replace_string(&input_file, cur_input_file->s);
			}

			asm_status = assemble_a_file(input_file, base_name,
						asm_options,
						&previous_output_file_name);

			/* If the assembler's input file was a temporary
			 * created by the compiler, then delete it.
			 */
			if (file_type != ASSEMBLY_SOURCE_FILE)
			{
				assert(input_file);
				(void) unlink(input_file);
				/* Don't remove it twice. */
				record_file_for_removal(cc1_removal_index,
								(char*)NULL);
			}
			free(input_file);
		}

		if (cpp_status || cc1_status || asm_status)
			error_occurred = TRUE;

		if (cmd_line.stop_after >=STOP_AT_EXECUTABLE && !error_occurred)
		{
			/* Add previous output file or input file
			 * to list of files to be linked.
			 */
			char*	linker_file = previous_output_file_name;
			if (!linker_file) {
				assert(file_type == OTHER_FILE);
				linker_file = cur_input_file->s;
			}
			string_list_append(&linker_input_files, linker_file);
			if (previous_output_file_name)
				free(previous_output_file_name);
		}

		if (base_name)
			free(base_name);
	}

	string_list_delete(&cpp_options);
	string_list_delete(&cc1_options);
	string_list_delete(&asm_options);

	/* Temporary files for listing support are no longer needed. */
	if (is_sinfo_needed())
	{
		(void) unlink(sinfo_file_name);
		record_file_for_removal(sinfo_removal_index, (char*)NULL);
		free(sinfo_file_name); sinfo_file_name = NULL;
	}

	if (is_listing_enabled())
	{
		(void) unlink(list_tmp_file_name);
		record_file_for_removal(list_tmp_removal_index, (char*)NULL);
		free(list_tmp_file_name); list_tmp_file_name = NULL;
	}

	if (bname_tmp_file_name)
	{
		(void) unlink(bname_tmp_file_name);
		record_file_for_removal(bname_tmp_removal_index, (char*)NULL);
		free(bname_tmp_file_name); bname_tmp_file_name = NULL;
	}

	if (cmd_line.report_peak_memory_usage)
	{
		(void)fprintf(stderr,"\nPeak memory allocation:  No information available\n");
	}

	/*
	 * Run the linker, if needed.
	 */

	if (cmd_line.stop_after >= STOP_AT_EXECUTABLE && !error_occurred)
	{
		char		*output_file_name = cmd_line.output_file_name;
		int		linker_status;
		String_list	link_cmd = create_link_cmd_line_prefix();

		string_list_append_list(&link_cmd, linker_input_files);

		/* Add libraries to be linked. */
		string_list_append_list(&link_cmd,
					cmd_line.linker_library_options);
		/* Add the profiling library.  Ideally, we'd like to do
		   this only for program-wide optimizations.  But that
		   would entail interpretation of several options (-q,
		   -gcdm, -f, etc).  Rather than hard-wiring the logic here,
		   we always pass the profiling library to the linker.
		   One not-so-nice side effect of this is that we can't
		   give a warning about passing the wrong profile library.
		   The warning would be inappropriate if the user isn't
		   doing program-wide optimizations.
		 */
		{
			if (cmd_line.profile_library != NULL)
				string_list_append_opt_and_arg(&link_cmd,
					'l', cmd_line.profile_library);
			else
			{
				string_list_append_opt_and_arg(&link_cmd,
					'l', "qf");
			}
		}

		if (!output_file_name)
		{
			if (cmd_line.object_module_format == OMF_COFF)
				output_file_name = "a.out";
			else if (cmd_line.object_module_format == OMF_ELF)
				output_file_name = "e.out";
			else
				assert(0);
		}
		record_file_for_removal(lnk_removal_index, output_file_name);

		linker_status = invoke_command(link_cmd);
		if (linker_status)
		{
			error_occurred = TRUE;
			(void) unlink(output_file_name);
		}
		record_file_for_removal(lnk_removal_index, (char*)NULL);

		string_list_delete(&linker_input_files);
		string_list_delete(&link_cmd);
	}

	/* Make sure we haven't left any temporary files around */
	remove_temporary_files();

	return error_occurred ? -1 : 0;
}

void
sig_handler(signum)
int	signum;
{
	(void) signal(signum, SIG_DFL);	/* This should happen automatically */
	remove_temporary_files();
	(void) fprintf(stderr,
		"%s FATAL ERROR: caught signal %d.\n", my_name, signum);
	_exit(FATAL_EXIT_CODE);
}

static void
set_signal_handlers()
{
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void) signal(SIGINT, sig_handler);
	if (signal(SIGABRT, SIG_IGN) != SIG_IGN)
		(void) signal(SIGABRT, sig_handler);
#if defined(SIGHUP)
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		(void) signal(SIGHUP, sig_handler);
#endif
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		(void) signal(SIGTERM, sig_handler);
#if defined(SIGSEGV)
	if (signal(SIGSEGV, SIG_IGN) != SIG_IGN)
		(void) signal(SIGSEGV, sig_handler);
#endif
#if defined(SIGIOT)
	if (signal(SIGIOT, SIG_IGN) != SIG_IGN)
		(void) signal(SIGIOT, sig_handler);
#endif
#if defined(SIGILL)
	if (signal(SIGILL, SIG_IGN) != SIG_IGN)
		(void) signal(SIGILL, sig_handler);
#endif
#if defined(SIGBUS)
	if (signal(SIGBUS, SIG_IGN) != SIG_IGN)
		(void) signal(SIGBUS, sig_handler);
#endif
}

#define gnu960_ver ic960_ver
extern char gnu960_ver[];
#include "ver960.h"	/* Support check_v960 */

main(argc, argv)
int	argc;
char	*argv[];
{
	int	status;

	argc = get_response_file(argc,&argv);
	check_v960(argc, argv);

	Argc = argc; Argv = argv;	/* Make these global */
	parse_command_line_arguments(argc, argv);

	set_signal_handlers();

	status = process_input_files();

	if (status != 0)
		exit(FATAL_EXIT_CODE);
	exit(SUCCESS_EXIT_CODE);
	return 0;
}
