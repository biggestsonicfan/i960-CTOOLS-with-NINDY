/* as.c - GAS main program.
   Copyright (C) 1987, 1990, 1991 Free Software Foundation, Inc.

This file is part of GAS, the GNU Assembler.

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
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. */

/* static const char rcsid[] = "$Id: as.c,v 1.68 1995/11/17 17:08:03 peters Exp $"; */

/*
 * Main program for AS; a 32-bit assembler of GNU.
 * Understands command arguments.
 * Has a few routines that don't fit in other modules because they
 * are shared.
 *
 *
 *			bugs
 *
 * : initializers
 *	Since no-one else says they will support them in future: I
 * don't support them now.
 *
 */

#include <stdio.h>
#include <string.h>

#define COMMON

#include "as.h"
#ifdef DOS
#	include "gnudos.h"
#if defined(WIN95)
#include <fcntl.h>
#endif
#endif /*DOS*/

#ifndef SIGTY
#define SIGTY int
#endif

#ifdef __STDC__
static char *stralloc(char *str);
static void perform_an_assembly_pass(int argc, char **argv);
static void command_line(int argc, char **argv);
static void get_environment(void);
static void usage(void);
static void put_asm_help(void);
#else /* __STDC__ */
static char *stralloc();
static void perform_an_assembly_pass();
static void command_line();
static void get_environment();
static void usage();
static void put_asm_help();
#endif /* __STDC__ */

/* These are the only environment variables recognized by the assembler. */
static env_varS environment [] =
{
	{ "I960ARCH", NULL },
	{ "I960IDENT", NULL },
	{ "I960INC", NULL },
	{ NULL, NULL }		/* The last name must be NULL as a sentinel */
};

char *myname;		/* argv[0] */

#ifdef GNU960
extern char gnu960_ver[];
#else
extern char version_string[];
#endif


#ifdef	DEBUG
int	tot_instr_count;
int	mem_instr_count;
int	mema_to_memb_count;
int	FILE_run_count;
int	FILE_tot_instr_count;
int	FILE_mem_instr_count;
int	FILE_mema_to_memb_count;
char	*instr_count_file = "gas960.trace";
#endif

int main(argc,argv)
	int argc;
	char **argv;
{
	
#ifdef GNU960
#ifdef DOS
#ifdef _INTELC32_
	/********************************************************
	 **  Initialize the internal Code Builder run time libraries
	 **  to accept the maximum number of files open at a time
	 **  that it can.  With this set high, the number of files
	 **  that can be open at a time is completely dependent on
	 **  DOS config.sys FILES= command.
	 *********************************************************/
	_init_handle_count(MAX_CB_FILES);
#endif
#endif
	argc = get_response_file(argc,&argv);
        argc = get_cmd_file(argc, argv);
	check_v960( argc, argv );
#endif
	
	myname = argv[0];

	if (argc == 1) {
		put_asm_help();
		exit(0);
	}
	/* Initializers.  Note: there are order dependencies here. */

	get_environment();	/* O/S environment vars */
	symbol_begin();		/* symbols.c */
	cond_begin();		/* cond.c */
	read_begin();		/* read.c */
	md_begin();		/* MACHINE.c */
	segs_begin();		/* segs.c */
	input_scrub_begin();	/* input_scrub.c */
	command_line(argc, argv);		/* process args */
	md_arch_hook();		/* revisit opcode table with known arch. */
	if ( had_errors() ) {
	        /* Command-line errors are fatal */
		usage();
		exit (1);
	}
	if ( flagseen['L'] )
		/* Listing requested */
		listing_begin();
	perform_an_assembly_pass(argc,argv); /* do input processing */
	if ( flagseen['L'] )
		listing_pass2_begin();

#ifdef INSTRUMENT_BRANCHES
	brtab_emit();
#endif

	if ( ! seen_at_least_1_file() )
	{
	    as_bad ("Must specify either -i switch or input file name");
	    usage();
	    exit(1);	/* really looking for help? */
	}

	if ( ! had_errors() || flagseen['Z'] )
	{
	        extern void md_check_leafproc_list();

		/* fix up addresses, relocations, etc, then write the output */
		write_object_file();
#ifdef ASM_SET_CCINFO_TIME
		set_output_time ();
#endif
		if ( flagseen['L'] )
			write_listing_file();
	}

	input_scrub_end();
	listing_end(0);
	md_end();
	exit ( had_errors() );

} /* main() */


/* get_environment()
 *
 * Get user's environment.
 * Depends on the last entry in the environment[] array being NULL.
 * This is the ONLY place that the O/S environment query (getenv
 * or equivalent) should be called.
 * 
 * Note we must make COPIES of the environment strings because of
 * the unfortunate Metaware/Thompson Toolkit combination that 
 * appears to overwrite the result of getenv() when called more 
 * than once.
 */
static void
get_environment()
{
	env_varS	*eP = environment;
	char		*c;

	for ( ; eP->name != NULL; ++eP )
	{
		c = (char *) getenv(eP->name);
		if ( c )
		{
			eP->value = xmalloc(strlen(c) + 1);
			strcpy (eP->value, c);
		}
	}
}


/* env_var()
 *
 * Get the setting of a user environment var.
 * Depends on the last entry in the environment[] array being NULL.
 * 
 */
char *
env_var(name)
char	*name;
{
	env_varS	*eP = environment;

	for ( ; eP->name != NULL; ++eP )
		if ( ! strcmp(name, eP->name) )
			return eP->value;
	return NULL;
}


/* command_line()
 *
 * Parse arguments, but we are only interested in flags for now.
 *
 * When we find a flag, we process it then make its argv[] NULL.
 * This helps any future argv[] scanners avoid what we processed.
 * Since it is easy to do here we interpret the special arg "-"
 * to mean "use stdin" and we set that argv[] pointing to "".
 * After we have munged argv[], the only things left are source file
 * name(s) and ""(s) denoting stdin. These file names are used
 * (perhaps more than once) later.
 *
 * For making a listing: make a copy of the command line, before
 * it is known whether a listing has been requested, because the
 * processing below trashes the arguments as it goes along.
 */

static void
command_line(argc, argv)
int argc;
char **argv;
{
	int 	work_argc = argc - 1;	/* variable copy of argc */
	char 	**work_argv = argv + 1; /* variable copy of argv */
	char 	*arg;			/* an arg to program */
	char 	a;			/* an arg flag (after -) */
	char 	little_i = 0;  		/* -i needs an extra flag */

	set_listing_command_line(argc, argv);

	for ( ; work_argc--; work_argv++ ) 
	{
		arg = * work_argv;	/* next argument */
#ifdef DOS
		if ( *arg != '-' && *arg != '/' )
			continue;
#else
		if ( *arg != '-' )	/* Filename. We need it later. */
			continue;
#endif
		if ( arg[1] == '-' && arg[2] == 0 ) 
		{
			/* "--" as an argument means read STDIN */
			* work_argv = "";	/* Code that means 'use stdin'. */
			continue;
		}

		arg ++;		/* one past '-' */

		while ( (a = * arg) != '\0' )  
		{
			/* scan all the 1-char flags */
			arg ++;
			a &= 0x7F;	/* ascii only please */
			flagseen[a] = 1;
			switch (a) 
			{
			case 'G':
				/* target processor is big-endian */
#ifdef OBJ_BOUT
				as_fatal("-G only available with COFF output files.");
#endif /* OBJ_BOUT */
				segs_assert_msb();
				break;

			case 'A':
				/* Architecture */
				if ( *arg )
					md_parse_arch(arg);
				else if ( work_argc ) 
				{	/* Arch. flag is in the next arg */
					*work_argv = NULL;
					++work_argv;
					--work_argc;
					md_parse_arch(*work_argv);
				} 
				else
					as_bad("%s: Expected architecture after -A", myname);
				arg = "";	/* Finished with this arg. */
				break;
					
			case 'p':
				/* PIC/PID */
				if ( *arg )
					/* pic/pid flag is adjacent to the -p */
					md_parse_pix(arg);
				else if ( work_argc ) 
				{	/* pic/pid flag is in the next arg */
					*work_argv = NULL;
					++work_argv;
					--work_argc;
					md_parse_pix(*work_argv);
				} 
				else
					as_bad("%s: Expected pic/pid flag after -p", myname);
				segs_assert_pix();
				arg = "";	/* Finished with this arg. */
				break;
					
			case 'j':
				/* Hardware errata workaround */
				if ( *arg )
				    /* errata flag is adjacent to the -j */
				    md_parse_errata(arg);
				else if ( work_argc ) 
				{	
				    /* errata flag is in the next arg */
				    *work_argv = NULL;
				    ++work_argv;
				    --work_argc;
				    md_parse_errata(*work_argv);
				} 
				else
				    as_bad("%s: Expected errata number after -j", myname);
				arg = "";	/* Finished with this arg. */
				break;
					
			case 'L':
				/* Listing */
				if ( *arg )
				{
					/* More listing info follows adjacent to the -L. */
					if ( parse_listing_option(arg, work_argc ? *(work_argv + 1) : NULL, myname) )
					{
						/* There was an argument following the -Lx, and
						 * the following arg was eaten for listing purposes
						 */
						*work_argv = NULL;
						++work_argv;
						--work_argc;
					}
				}
				/* When listing, always try to generate an object file,
				 * even when errors occur.
				 */
				flagseen['Z'] = 1;
				arg = "";
				break;

			case 'D':
				/* defsym: following arg is expected to be of 
				 * the form foo=x
				 */
				if ( *arg )
					parse_defsym_option(arg, myname);
				else if ( work_argc ) 
				{	/* defsym is in the next arg */
					*work_argv = NULL;
					++work_argv;
					--work_argc;
					parse_defsym_option(*work_argv, myname);
				}
				else
					as_bad("%s: Expected defsym expression after -D", myname);
				arg = "";	/* Finished with this arg. */
				break;

			case 'I': 
			{ 
				/* Include file directory */
				char *temp;
				if (*arg)
				    temp = stralloc (arg);
				else if (work_argc) 
				{
					* work_argv = NULL;
					work_argc--;
					temp = * ++ work_argv;
				} else
				    as_bad("%s: Expected a filename after -I", myname);
				add_include_dir (temp);
				arg = "";	/* Finished with this arg. */
				break;
			}


			case 'd': 
				/* -d means keep L* (debug) symbols */
				break;

#ifdef INSTRUMENT_BRANCHES
			case 'b':
				/* Add code to collect information about branches taken, 
				 * for later optimization of branch prediction bits 
				 * by a separate tool.  See comment near br_cnt() in
				 * tc_i960.c for details.
				 */
				break;
#endif
			case 'm':
				/* -m means call mpp960 preprocessor first */
				break;
			case 'n':
				/* -n means "no relax" (formerly -norelax)
				 * Conditional branch instructions that require 
				 * displacements greater than 13 bits (or that 
				 * have external targets) should generate errors.  
				 * The default is to silently replace each such 
				 * instruction with the corresponding compare and
				 * branch instructions.
				 */
				break;

			case 'o':
				if (*arg)	/* Rest of argument is object file-name. */
					out_file_name = stralloc (arg);
				else if (work_argc) 
				{	/* Next arg is the file name. */
					* work_argv = NULL;
					++work_argv;
					--work_argc;
					out_file_name = *work_argv;
				} 
				else
				{
					as_bad("%s: Expected a filename after -o", myname);
					arg = "";	/* Finished with this arg. */
					break;
				}
				output_file_checkname();
				arg = "";	/* Finished with this arg. */
				break;

			case 'R':
				/* -R is a vestige from previous releases, for "make rom-able code" */
				as_warn ("-R option no longer supported. (option ignored)");
				break;
				
			case 'r':
				/* -r is a vestige from previous releases, for "delete source file" */
				as_warn ("-r option no longer supported. (option ignored)");
				break;

			case 'v':
				/* Intentional fallthrough: V and v are synonyms */
			case 'V':
				/* -V means print version info but don't exit */
				gnu960_put_version();
				break;
				
			case 'h':
				/* -h means produce help screen */
				put_asm_help();
				exit(0);  		
		
			case 'i':
				/* -i means interactive mode: take input from 
				   stdin instead of a file */
				little_i = 1;
				break;
			case 'a':
				/* -a means force MEMB -> MEMA optimization */
				flagseen['M'] = 0;
				break;
			case 'M':
				/* -M means turn off MEMB -> MEMA optimization */
			case 'P':
				/* All-purpose flag to use while developing */
#ifdef	DEBUG
			case 'T':
				/* -T means trace (count) instructions */
			case 'Q':
				/* -Q means trace (count) instructions quietly */
#endif
			case 'W':
				/* -W means don't warn about things */
				break;
			case 'x':
				/* -x means architecture errors are warnings */
				md_set_arch_message(as_warn);
				break;
			case 'X':
				/* -X means don't delete temporary files */
			case 'Z':
				/* -Z means attempt to generate object file even after errors. */
			case 'z':
				/* -z means suppress writing a timestamp into the file header. */
				break;

			default:
				as_bad ("%s: Unrecognized option: '%c'.", myname, a);
				if ( arg && *arg )
					++arg;
				break;
			}
		} /* for each (letter within a "-xyz" command-line arg) */

		/*
		 * We have just processed a "-..." arg, which was not a
		 * file-name. Smash it so the
		 * things that look for filenames won't ever see it.
		 *
		 * Whatever work_argv points to, it has already been used
		 * as part of a flag, so DON'T re-use it as a filename, 
		 * except in the special case of interactive mode (-i)
		 */
		if ( little_i )
		{
			little_i = 0;
			*work_argv = "";   /* "" means "use STDIN"  */
		}
		else
			*work_argv = NULL; /* NULL means 'not a file-name' */
	} /* for each (command-line arg) */

	/* only 80960CA supports big-endian memory */
	if (flagseen['G'])
		md_check_arch();

#ifdef OBJ_BOUT
	/* Suppress memb-to-mema optimization for b.out */
	flagseen ['M'] = 1;
#endif

} /* command_line() */


static void 
put_asm_help()
{
	register int i;
        static char *utext[] = {
                "",
                "Cross-assembler for the i960 processor",
                "",
                "   -A: specify an 80960 architecture to enable opcode-checking",
                "        [valid architectures are: KA,SA,KB,SB,CA,CF,JA,JF,JD,",
		"        HA,HD,HT,RP,ANY; default is KB]",
		"        [-AANY disables opcode-checking]",
		"   -D: define symbol",
                "   -d: retain symbol information about local labels beginning with (L) or (.)",
                "   -G: assemble to run in big-endian memory region",
                "        [only used with COFF or Elf invocation]",
                "   -h: display this help screen",
                "   -Ipathname: set search path for include files",
                "   -i: accept input from stdin instead of from a file",
                "   -L: generate assembler listing",
		"        [see user's guide or man page for more information on this]",
                "   -n: no relax",
		"        [see user's guide or man page for more information on this]",
                "   -o: name the output file",
                "   -p[c|d|b]: set flag to mark the output file as containing",
		"        [c: position independent code]",
		"        [d: position independent data]",
		"        [b: both position independent code and data]",
		"        [only used with COFF or Elf invocation, default is no marking]",
                "   -V: print version information and continue",
                "   -v960: print version information and exit",
                "   -W: suppress all warning messages",
		"   -x: downgrade opcode-not-in-architecture errors to warnings",
                "   -z: suppress the time/date stamp",
		"        [only used with COFF invocation]",
                "",
		"See your user's guide for a complete command-line description.",
                "",
                NULL
        };

	fprintf (stdout, "\nUsage: %s [options] filename ...\n", myname);
	for ( i = 0; utext[i]; ++i )
		fprintf(stdout, "%s\n", utext[i]);
} /* put_asm_help() */


static void 
usage()
{
	fprintf (stderr, "\nUsage: %s [options] filename ...\n", myname);
	fprintf (stderr, "Use -h option to get help\n\n");
} /* usage() */


/* perform_an_assembly_pass()
 *
 * Attempt one pass over each input file.
 * We scan argv looking for filenames or exactly "" which is
 * shorthand for stdin. Any argv that is NULL is not a file-name.
 * We set need_pass_2 TRUE if, after this, we still have unresolved
 * expressions of the form (unknown value)+-(unknown value).
 *
 * Note the un*x semantics: there is only 1 logical input file, but it
 * may be a catenation of many 'physical' input files.
 *
 * PS: The starting segment is SEG_TEXT by default.
 */
static void perform_an_assembly_pass(argc, argv)
int argc;
char **argv;
{
	need_pass_2 = 0;
#ifdef	DOS
	/* Standard input must be binary to handle CTRL-Z's in a 
	 * redirected file (see app.c:scrub_from_file)
	 */
#if defined(WIN95)
	_setmode(stdin, _O_BINARY);
#else
	_setmode(stdin, _BINARY);
#endif
#endif
	/* skip argv[0] */
	for (argv++, argc--; argc; argv++, argc--) 
	{
		if (*argv) 
		{	
			/* It's a file-name argument */
			read_a_source_file(*argv);
			
			/* Check the .if/.endif stack for this file */
			if ( ! check_cond_stack() )
				as_bad("Unbalanced .if/.endif blocks");
		}
	}
} /* perform_an_assembly_pass() */


/*
 *			stralloc()
 *
 * Allocate memory for a new copy of a string. Copy the string.
 * Return the address of the new string. Die if there is any error.
 */

static char *
stralloc (str)
char *	str;
{
	register char *	retval;
	register long len;

	len = strlen (str) + 1;
	retval = xmalloc (len);
	(void) strcpy(retval, str);
	return(retval);
}

/*
 * Local Variables:
 * comment-column: 0
 * fill-column: 131
 * End:
 */

/* end: as.c */

