/*(c**************************************************************************** *.*
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

/* Utility for examining b.out and coff format files
 * See usage() function for instructions.
 *
 * Most of the code is in this one file.  COFF-specific stuff is in
 * coffdmp.c, bout-specific in boutdmp.c.
 */

static char rcsid[] = "$Id: gdmp960.c,v 1.105 1996/01/08 21:14:56 paulr Exp $";

#if 0
#include <stdio.h>
#ifdef  USG
#	include <fcntl.h>
#if !defined(WIN95) && !defined(__HIGHC__) && !defined(APOLLO_400) /* Microsoft C 9.00 (Windows 95), Metaware C or Apollo 400 systems */
#	include <unistd.h>
#endif
#	define  L_SET   SEEK_SET

#	define bzero(s,n) memset(s,0,n)
#else   /* BSD */
#	include <sys/file.h>
#endif

/* #include "bout.h" */
#include "coff.h"
#include "bfd.h"
#include "gdmp960.h"
#ifdef DOS
#       include "gnudos.h"
#endif

#else

#include <sysdep.h>
#include "bfd.h"
#include "coff.h"
#include "elf.h"
#include "dio.h"
#include "gdmp960.h"

#ifndef L_SET
#define  L_SET   SEEK_SET
#endif
#ifndef L_XTND
#define	L_XTND SEEK_END
#endif

#endif

static asymbol	*get_function_by_name PARAMS ((char *name));
static char	*get_sym_name PARAMS ((long val, long loc));
static char 	*getsection PARAMS ((asection *sec));
static int	(*dmp_data) PARAMS ((char *p, int len, long start_addr));
static int	cobr PARAMS ((unsigned long memaddr, unsigned long word1));
static int	command_line PARAMS ((int argc, char **argv));
static int	ctrl PARAMS ((unsigned long memaddr, unsigned long word1));
static int	disasm PARAMS ((char *x, long len, unsigned long baseaddr, int big_endian_flag));
static int	dmp_hex PARAMS ((char *p, int len, long start_addr));
static int	dmp_oct PARAMS ((char *p, int len, long start_addr));
static int	dstop PARAMS ((int mode, int reg, int fp));
static int	effec_addr PARAMS ((unsigned long memaddr, int mode, char *reg2, char *reg3, unsigned long word1, unsigned long word2, int noprint));
static int	get_reloc_by_address PARAMS ((long loc, asymbol *sp));
static int	get_symbol_by_value PARAMS ((long val, int *sindex));
static int	invalid PARAMS ((int word1));
static int	mem PARAMS ((unsigned long memaddr, unsigned long word1, unsigned long word2, int noprint));
static int	pinsn PARAMS ((unsigned long memaddr, unsigned long word1, unsigned long word2));
static char	*symbolic_address PARAMS ((unsigned long a, unsigned long loc));
static int	regop PARAMS ((int mode, int spec, int reg, int fp));
static int 	build_reloc_table PARAMS ((void));
static int 	build_value_table PARAMS ((void));
static int 	dmp PARAMS ((char *fn));
static int 	put_abs PARAMS ((unsigned long word1, unsigned long word2));
static int 	reg PARAMS ((unsigned long word1));
static int 	usage PARAMS ((char *progname));
static int 	put_gdmp_help PARAMS ((char *progname));
static long	compare_reloc_values PARAMS ((arelent **rp1, arelent **rp2));
static long	compare_symbol_values PARAMS ((asymbol **sp1, asymbol **sp2));
static long	compare_symbol_values_only PARAMS ((asymbol **sp1, asymbol **sp2));
static long	get_function_end PARAMS ((long start_addr));
static long 	getword PARAMS ((char *p, int big_endian_data));
static void	print_label PARAMS ((long loc));
static void	parse_dwarf_options PARAMS ((char **argp));

/* 
 * The value table is used for symbolic disassembly, to make searching 
 * for a symbol, given its value, fast.  It is sorted by value.
 *
 * The reloc table is similar, but is used to search for a symbol via
 * relocation structs.  It is sorted by address.
 *
 * The symbol table is the canonicalized table as received from BFD.
 */

asymbol		**symbol_table;
asymbol		**value_table;
arelent		**reloc_table;

static int get_rid_of_abs = 1;

/* The following 2 items  are here to make the interface to the
 * disassembler the same here as it is in gdb960.
 */
FILE *stream = stdout;
char * reg_names[] = {
	"pfp", "sp",  "rip", "r3",  "r4",  "r5",  "r6",  "r7", 
	"r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
	"g0",  "g1",  "g2",  "g3",  "g4",  "g5",  "g6",  "g7",
	"g8",  "g9",  "g10", "g11", "g12", "g13", "g14", "fp"
};


char host_is_big_endian;	/* TRUE or FALSE */
char file_is_big_endian;	/* TRUE or FALSE */

char 	flagseen [128];	/* Command-line options; 1 = "saw this option" */
char	dwarf_flagseen [128]; /* Suboptions for Dwarf dumper */
int	special_secnum; /* Section number corresponding to special_sect */
long 	numrawsyms;	/* Total number of symbols in symbol table */
long	numsyms;	/* Total number of symbols after filtering for -s */
long	numrels;	/* Total number of relocations after filtering for -s */

char 	*program_name;
extern char gnu960_ver[];

special_type *special_sect = NULL ;  /* User requested that only this section(s) be dumped */
special_type *special_func = NULL ;  /* User requested that only this function(s) be dumped */
special_type *special_sectptr;
special_type *special_funcptr;

/* 
 * printf format strings
 */ 
char		*oct_addr_fmt = "%11o:  "; 	/* Print octal addresses */
char		*hex_addr_fmt = "%8x:  "; 	/* Print hex addresses */
char		*addr_fmt;			/* location counter down left side */
char		*sec_name_fmt = "Section '%s'%s:\n";
char 		*fil_name_fmt = "\n%s:\n";
char		*label_fmt = "%s:\n";		/* labels on left for symbolic disass. */

/* We try to use the BFD library routines to access the object file because it
 * is a more general interface and should allow automatic migration to new
 * object file formats.  But when we get down to the relocation directives
 * and file headers, we want to see all the bits that are really there;  if
 * BFD obscures them too much, we go right in and read them directly, via
 * a separate file descriptor.
 */
bfd *abfd;	/* BFD descriptor of object file */
static FILE *file_handle;		/* Normal file descriptor for object file */

main( argc, argv )
	int argc;
    	char *argv[];
{
	int 		numfiles;	/* number of input files to process */
	int 		i;
	static short 	test_byte_order = 0x1234;
	
	argc = get_response_file(argc,&argv);
	program_name = argv[0];

	/* Set up some defaults; can be overridden with -o */
	addr_fmt = hex_addr_fmt;
	dmp_data = dmp_hex;

	check_v960( argc, argv );
	check_help( argc, argv );
	if ( !strcmp( argv[0], "test") ){
		/* Invoke this program under the name "test" to test the
		 * disassembler:  ascii hex will be read from stdin and
		 * disassembled to stdout.
		 */
		char buf[100];
		long word1, word2;
		while ( gets(buf) != NULL ) {
			word1 = word2 = 0;
			sscanf(buf,"%lx%lx", &word1, &word2);
			pinsn( (long) 0, word1, word2 );
		}
		exit( 0 );
	}

	host_is_big_endian = (*((char *) &test_byte_order) == 0x12);

	numfiles = command_line( argc, argv );

	if ( numfiles == 0 )
	{
		put_gdmp_help( argv[0] );
		exit(0);
	}

	for ( i = 1; i < argc; i++ )
	{
		if ( argv[i][0] != 0 )
		{
			if ( !flagseen['p'] && numfiles > 1 )
			{
				printf( fil_name_fmt, argv[i] );
			}
			if (!dmp( argv[i] )) exit(1);
		}
	}
	exit(0);
} /* main() */


check_help( argc, argv )
    int argc;
    char *argv[];
{
        int i;
        for ( i = 1; i < argc; i++ ){
                if ( !strcmp(argv[i],"-help")
#ifdef DOS
                        || !strcmp(argv[i], "/help")
#endif
              ) {
                        put_gdmp_help(argv[0]);
                        exit (0);
                }
        }
}


/* 
 * command_line()
 *
 * Return the number of input files to process.  If a command-line argument
 * is an option, not a file name, set its first character to 0 after 
 * processing as a flag to main() to ignore it.  Do the same for arguments
 * following certain options (e.g. the "name" argument in -n name)
 */
static
int
command_line ( argc, argv )
    int argc;
    char **argv;
{
    char	*optarg;	/* argument following -n or -F */
    int		ctrl_opt = 0;	/* > 0 if any control option was seen */
    int		numfiles = 0;	/* Number of input files to process */
    int		i; 
    char 	*p;
    
    for ( i = 1; i < argc; i++ )
    {
	if ( argv[i][0] == 0 )
	{
	    /* this was an option-argument; it has already been
	     * processed
	     */
	    continue;
	}
	if ( argv[i][0] != '-' 
#ifdef DOS
	    && argv[i][0] != '/'
#endif 
	    )
	{
	    ++numfiles;
	    continue;
	}

	/* Run through a block of options, like -xyz */
	for ( p = &argv[i][1]; *p; p++ )
	{
	    flagseen[*p] = 1;
	    switch ( *p )
	    {
	    case 'A':
		get_rid_of_abs = 0;
		break;
	    case 'n':
	    case 'F':
		/* These options expect an argument.
		 * Following arg may come in same option block,
		 * or it may be the next argument.
		 */
		if ( *(p + 1) )
		{
		    /* argument follows in same option block */
		    optarg = xmalloc(strlen(p + 1) + 1);
		    strcpy(optarg, p + 1);
		    /* mark this arg as having been processed */
		    argv[i][0] = 0;
		}
		else if ( i < argc-1 && * argv[i+1] != '-' )
		{
		    /* argument follows in next spot on cmd line */
		    optarg = xmalloc(strlen(argv[i+1]) + 1);
		    strcpy(optarg, argv[i+1]);
		    /* mark the option-arg as having been processed */
		    argv[i+1][0] = 0;
		}
		else 
		{
		    error ("Option -%c requires an argument.", *p);
		    usage (argv[0]);
		    exit (1);
		}

		switch ( *p )
		{
		case 'n':
                    /* create a list of sections to dump */
		    if (special_sect == NULL)
		    {
			special_sect = (special_type *)xmalloc(sizeof(special_type));
			special_sect->name = optarg;
			special_sect->id = -1;
			special_sect->link = NULL;
			special_sectptr = special_sect;
		    }
		    else
		    {
			special_sectptr->link = (special_type *)xmalloc(sizeof(special_type));
			special_sectptr = special_sectptr->link;
			special_sectptr->id = -1;
			special_sectptr->link = NULL;
			special_sectptr->name = optarg;
		    }
		    break;
		case 'F':
		    /* create a list of functions to dump */
		    flagseen['d'] = 1;
		    if (special_func == NULL)
		    {
			special_func = (special_type *)xmalloc(sizeof(special_type));
			special_func->name = optarg;
			special_func->link = NULL;
			special_funcptr = special_func;
		    }
		    else
		    {
			special_funcptr->link = (special_type *)xmalloc(sizeof(special_type));
			special_funcptr = special_funcptr->link;
			special_funcptr->link = NULL;
			special_funcptr->name = optarg;
		    }
		    break;

		default:
		    ;
		}
		/* Run to the end of this arg */
		for ( ; *(p+1); ++p )
		    ;
		break;

	    case 'g':
		/* Dwarf dumping.  Suboptions (if any) appear immediately
		   following with no whitespace. */
		ctrl_opt++;
		parse_dwarf_options(&p);
		break;

		/* The following are control options. */
	    case 'c':
	    case 'd':
	    case 'f':	
	    case 'h':
	    case 'i':
	    case 'l':
	    case 'm':
	    case 'q':
	    case 'r':
	    case 't':
		ctrl_opt++;
		break;

		/* The following are formatting options. */
	    case 'o':
		addr_fmt = oct_addr_fmt;
		dmp_data = dmp_oct;
		/* Intentional fallthrough */
	    case 'a':
	    case 'D':
	    case 'T':
	    case 'p':
	    case 's':
	    case 'N':
	    case 'x':
	    case 'z':
		break;
				
	    case 'V':
		/* Print version string and continue. */
		gnu960_put_version();
		break;

	    default: 
		error ("Unknown option: -%c", *p);
		usage(argv[0]); 
		exit(1);
	    } /* end switch (option letter) */

	} /* end while (more options in -xyz) */

	/* Mark -xyz arg as having been processed. */
	argv[i][0] = 0;

    } /* end for each (command line argument) */
    
    if ( ! ctrl_opt )
	/* -d is the default control option. */
	flagseen ['d'] = 1;
    
    return numfiles;
    
} /* command_line() */


static
void
parse_dwarf_options ( argp )
    char **argp;
{
    int count = 0;

    /* Advance the arg pointer one character to the right to start. */
    for ( ++(*argp); **argp; ++(*argp), ++count )
    {
	dwarf_flagseen[**argp] = 1;
	switch (**argp)
	{
	case 'i':   /* dump .debug_info */
	case 'l':   /* dump .debug_line */
	case 'f':   /* dump .debug_frame */
	case 'p':   /* dump .debug_pubnames */
	case 'a':   /* dump .debug_aranges */
	case 'm':   /* dump .debug_macinfo */
	    break;
	case 'A':   /* dump all .debug_* sections */
	    dwarf_flagseen['i'] = 1;
	    dwarf_flagseen['l'] = 1;
	    dwarf_flagseen['f'] = 1;
	    dwarf_flagseen['p'] = 1;
	    dwarf_flagseen['a'] = 1;
	    dwarf_flagseen['m'] = 1;
	    break;
	default:
	    error("Unrecognized 'g' suboption: '%c'", **argp);
	    break;
	}
    }

    /* The default is .debug_info if no options were given */
    if ( count == 0 )
	dwarf_flagseen['i'] = 1;

    /* Restore argv to caller's expected state */
    --(*argp);
    return;
}

/* 
 * Error:   Print a message but don't exit (caller will do that)
 * Fatal:   Print a message and exit immediately with non-zero status
 */

#ifdef SOLARIS
/* FIXME: Workaround for broken vfprintf() on sol-sun4 */
char    sol_tmpbuf[1024];
#endif

#ifdef __STDC__
void 	error(const char* Format, ...)
{
	va_list args;
	va_start(args, Format);
	vfprintf(stderr, Format, args);
	va_end(args);
	(void) putc('\n', stderr);
} /* error */

void 	fatal(const char* Format, ...)
{
	va_list args;
	va_start(args, Format);
	vfprintf(stderr, Format, args);
	va_end(args);
	(void) putc('\n', stderr);
	exit (42);
} /* fatal */

#else	/* non-ANSI C */
#ifdef  SOLARIS
/* FIXME: Workaround for broken vfprintf() on sol-sun4 */
void	error(va_alist)
va_dcl
{
	char *Format;
	va_list args;
	va_start(args);
	Format = va_arg(args, char *);
	vsprintf (sol_tmpbuf, Format, args);
	fprintf(stderr, "%s\n", sol_tmpbuf);
	va_end(args);
} /* error */

void	fatal(va_alist)
va_dcl
{
	char *Format;
	va_list args;
	va_start(args);
	Format = va_arg(args, char *);
	vsprintf (sol_tmpbuf, Format, args);
	fprintf(stderr, "%s\n", sol_tmpbuf);
	va_end(args);
	exit (42);
} /* error */

#else	/* NOT Solaris */
void	error(Format,va_alist)
	char *Format;
	va_dcl
{
	va_list args;
	va_start(args);
	vfprintf(stderr, Format, args);
	va_end(args);
	(void) putc('\n', stderr);
} /* error */


void	fatal(Format,va_alist)
	char *Format;
	va_dcl
{
	va_list args;
	va_start(args);
	vfprintf(stderr, Format, args);
	va_end(args);
	(void) putc('\n', stderr);
	exit (42);
} /* fatal */
#endif  /* if SOLARIS */
#endif 	/* __STDC__ */


static
char *utext[] =
{
        "",
        "Dumps information about the specified object files:",
        "",
	"     -A: Include absolute symbols with symbolic disassembly",
        "     -a: disassemble / dump all sections in the object file",
        "     -c: string table",
        "     -d: disassembly (contents of all sections) (this is the default)",
        "     -f: file header",
	"     -g[suboptions]: dump Dwarf2 sections (Elf file only)",
	"         [valid suboption are \"afilmpA\" (default is 'i')]",
        "     -h: section headers",
        "     -i: optional header (COFF file only)",
        "     -l: line number table (COFF file only)",
        "     -r: relocation information",
        "     -s: symbolic disassembly (print symbolic addresses where possible)",
        "     -t: symbol table",
        "     -o: print addresses in octal instead of hex",
        "     -p: suppress headers",
        "     -q: query the file and display its type",
        "     -x: suppress symbolic translation of symbol types and storage classes",
        "          (COFF file only)",
        "          [applies only to the -t option]",
        "     -z: suppress zeros when displaying TEXT-type section contents",
        "          [applies only to the -d option]",
        "     -D: dump section contents as if it were a DATA-type section",
        "          [applies only to the -d option]",
        "     -T: dump section contents as if it were a TEXT-type section",
        "          [applies only to the -d option]",
        "     -V: print version information and continue",
        "  -help: display this help message",
        "  -v960: print version information and exit",
        "  -n  section: dump only the given section",
        "  -F function: disassemble only the given function (COFF file only)",
        "",
        "See your user's guide for a complete command-line description",
        "",
	NULL
};

static
usage( progname )
    char *progname;
{
	fprintf( stderr, "\nUsage: %s [options] filename ...\n", progname );
	fprintf( stderr, "use -help to get help\n\n" );
}

static 
put_gdmp_help( progname )
char *progname;
{
	fprintf( stdout, "\nUsage: %s [options] filename ...\n", progname );
	paginator( utext );
}

/* byteswap:    Reverse the order of a string of bytes.
 */
byteswap( p, n )
     char *p;    /* Pointer to source/destination string */
	 int n;      /* Number of bytes in string            */
{
	int i;  /* Index into string                    */
	char c; /* Temp holding place during 2-byte swap*/

	for ( n--, i = 0; i < n; i++, n-- ){
		c = p[i];
		p[i] = p[n];
		p[n] = c;
	}
}


void
xread( bufp, n )
    char *bufp;
    int n;
{
	if ( fread( bufp, 1, n, file_handle ) != n )
		fatal ( "xread: Input file read failed" );
}

int
xread_rs( bufp, n )
    char *bufp;
    int n;
{
    return fread( bufp, 1, n, file_handle );
}


long
xseek( offset )
    long offset;
{
	long	pos = fseek( file_handle, offset, SEEK_SET );
	
     	if ( pos == -1 )
		fatal ( "xseek: Input file seek failed\n" );

	return ftell(file_handle);
}

long
xsize()
{
    struct stat buf;

    if (fstat(fileno(file_handle),&buf) == 0)
	    return buf.st_size;
    else
	    fatal("xsize: Can not fstat()\n");
    
}

long
xseekend()
{
	long	pos = xseek(xsize());

     	if ( pos == -1 )
		fatal ( "xseekend: Input file seek failed\n" );

	return ftell(file_handle);
}


char *
xmalloc( len )
    int len;
{
	char *p = (char *) malloc( len );
	if ( p == NULL )
		fatal ( "xmalloc: Unsuccessful memory allocation\n" );
	return p;
}

char *
xrealloc( oldbuf, len )
	char *oldbuf;
	int len;
{
	char *newbuf = (char *) realloc( oldbuf, len );
	if ( newbuf == NULL )
		fatal ( "xrealloc: Unsuccessful memory reallocation\n" );
	return newbuf;
}

static
long
getword( p_arg, big_endian_data)
    char *p_arg;
    int big_endian_data;
{
	int i;
	unsigned long val;
	unsigned char *p = (unsigned char *)p_arg;

	if (big_endian_data) {
	   /* return big-endian word */
	   val = *p++;
	   for ( i=0; i<3; i++ ){
		   val = (val << 8) | *p++;
	   }
        } else {
	   /* return little-endian word */
	   val =0;
	   for ( i=3; i>=0; i-- ){
		   val <<= 8;
		   val |= p[i] & 0x0ff;
	   }
        }
	return val;
}

/* Disassemble a TEXT-type section.  */
static
int disasm( x, len, baseaddr, big_endian_flag )
    char x[];
    long len;
    unsigned long baseaddr;
    int big_endian_flag;
{
	long 	word1, word2;
	long 	addr = 0;
	
	special_funcptr = special_func;
	do
	{
		if ( flagseen['F'] )
		{
			/* Disassemble only a named function */
			asymbol 		*sym = get_function_by_name(special_funcptr->name);
			more_symbol_info	msi;

			if ( sym == NULL )
			{
				/* OK, so this is a bit hokey: If symbol is not found
				 * try again with a prepended underscore
				 */
				char	*c = xmalloc(strlen(special_funcptr->name) + 2);
				strcpy(c, "_");
				strcat(c, special_funcptr->name);
				sym = get_function_by_name(c);
				free(c);
				if ( sym == NULL )
				{
					error ("Requested function not found: %s", special_funcptr->name);
					return 0;
				}
			}
			addr = sym->value;
			if ( bfd_more_symbol_info(abfd, sym, &msi, bfd_function_size) )
				len = addr + msi.function_size;
			else
			{
				error ("Internal error: function size not available for %s", sym->name);
				return 0;
			}
		}

		for ( ; addr < len; )
		{
			if ( (flagseen['s'] && !flagseen['N']) || flagseen['F'] )
			{
				/* Symbolic disassembly; insert label name
				 * if there is one for this address.
				 */
				print_label(baseaddr + addr);
			}
			word1 = getword( &x[addr], big_endian_flag );
			if ( (word1 == 0) && flagseen['z'] )
			{
				/* "Don't display zeroes" flag is in effect. */
				putchar( '\n' );
				do 
				{
					addr += 4;
					if ( addr >= len )
						return 1;
					word1 = getword( &x[addr], big_endian_flag );
				} while ( word1 == 0 );
			}
			word2 = getword( &x[addr+4], big_endian_flag );
			printf( addr_fmt, addr + baseaddr );
			addr += pinsn( addr + baseaddr, word1, word2 );
			putchar( '\n' );
		}
		putchar( '\n' );
		if ((flagseen['F']) && special_funcptr)
			special_funcptr = special_funcptr->link;
	} while (special_funcptr);

	return 1;
}

/* Dump a DATA-type section with hex bytes */
static
dmp_hex( p, len, start_addr )
    char *p;
    int len;
    long start_addr;
{
    int i,printed_last_line = 0;
    int this_quad_word_is_all_zeroes = 0;

/* We'll dump this line of hex .data ...

   if there is no -z on the command line 

   OR

   if there is a  -z on the command line AND this quad word is NOT all zeroes. */

#define DUMP_THIS_LINE (!flagseen['z'] || (flagseen['z'] && this_quad_word_is_all_zeroes == 0))

    for ( i = 0; i < len; i++ )	{
	if ( (i & 0x0f) == 0 ) {
	    /* 16-byte boundary; start a new line */

	    if (flagseen['z']) {  /* Look ahead at next 4 words to see if they are all zero. */
		int j;

		this_quad_word_is_all_zeroes = 1;  /* Assume they are all zero. */
		for (j=i;j < len && j < (i+4*4);j++)
			if (p[j]) {
			    /* Oh no Batman!  A byte is non-zero.  Quick Robin,
			       set the flag indicating 'this_quad_word_is_all_zeroes' to
			       zero and bug out. */
			    this_quad_word_is_all_zeroes = 0;
			    break;
			}
	    }
	    if ( printed_last_line ) {
		/* If we just printed something out, then put out a newline. */
		putchar ('\n');
		printed_last_line = 0;
	    }
	    if ( DUMP_THIS_LINE ) {
		printf( addr_fmt, i+start_addr );
		printed_last_line = 1;
	    }
	}
	else if ( DUMP_THIS_LINE && ((i & 0x03) == 0) )
		/* 4-byte boundary; leave a space */
		putchar (' ');
	if ( DUMP_THIS_LINE )
		printf( "%02x", p[i] & 0xff );
    }
    putchar( '\n' );
    putchar( '\n' );
}


/* Dump a DATA-type section in octal (group the image in groups of 4).
 * Build up a number with groups of 4 bytes at a time.  (This is for 
 * consistency with dis960, and also with the default, dmp_hex()).
 *
 * Don't print the number until you have all 4 bytes; then print it
 * in octal notation.
 */
static
dmp_oct( p, len, start_addr )
    char *p;
    int len;
    long start_addr;
{
	int 		i;
	int		multiplier = 24;
	int		number_in_progress = 0;
	unsigned int	number = 0;

	for ( i = 0; i < len; i++ )
	{
		if ( (i & 0x0f) == 0 )
		{
			/* 16-byte boundary; drop off a number and
			 * start a new line.
			 */
			if ( i > 0 )
			{
				printf( "%011o\n", number );
				number = number_in_progress = 0;
				multiplier = 24;
			}
			printf( addr_fmt, i+start_addr );
		}
		else if ( (i & 0x03) == 0 )
		{
			/* 4-byte boundary; drop off a number and 
			 * leave a space 
			 */
			printf( "%011o ", number );
			number = number_in_progress = 0;
			multiplier = 24;
		}

		/* Add in a new byte's worth to the number */
		number += (p[i] & 0xff) << multiplier;
		multiplier -= 8;
		number_in_progress = 1;
	}
	if ( number_in_progress )
	{
		/* There is one more word to print; go ahead and put it out;
		 * any extra bytes will have been set to 0 already.
		 */
		printf( "%011o\n", number ); 
	}
	putchar( '\n' );
} /* dmp_oct() */

static
char *
getsection(sec)
    asection *sec;
{
	char *fn;
	char *buf;
	int size;

	fn =  bfd_get_filename(abfd);

	size = bfd_section_size(abfd,sec);
	if ( size == 0 ){
		return NULL;
	}
	/* Always malloc an even number of words: and zero */
	buf = xmalloc( size = 4*((size/4)+1) );
	bzero(buf,size);
	if ( !bfd_get_section_contents(abfd,sec,buf,0,bfd_section_size(abfd,sec)) ){
		bfd_perror( fn );
		if (buf) {
			free( buf );
		}
		buf = NULL;
	}
	return buf;
}

static
dmp( fn )
    char *fn;
{
    char *buf;
    
    file_handle = fopen( fn, FRDBIN );
    if ( ! file_handle ){
	error ( "Can't open input file '%s'\n", fn );
	return 0;
    }
    
    abfd = bfd_openr(fn,(char *)NULL);
    
    if ( abfd == NULL )
    {
	bfd_perror(fn);
	fclose(file_handle);
	return 0;
    }
    
    if ( flagseen['q'] )
    {
	char *omf = "unknown",*hostendian="unknown",*targetendian="unknown";

	if ( bfd_check_format( abfd, bfd_object ) ) 
	{
	    omf = BFD_COFF_FILE_P(abfd) ? "coff" :
		BFD_BOUT_FILE_P(abfd) ? "bout" :
		    "elf";
	    hostendian = BFD_BIG_ENDIAN_FILE_P(abfd) ? "big" : "little";
	    if (BFD_ELF_FILE_P(abfd))
	    {
		asection *s = abfd->sections;
		int bigendian = -1;

		/* For Elf, if all text and data sections have the same 
		   target byte order, we'll report what it is. Otherwise 
		   print something to indicate that byte order differs */
		for ( ; s != NULL; s = s->next )
		{
		    if (s->flags & SEC_ALLOC)
		    {
			if (bigendian == -1)
			    bigendian = (s->flags & SEC_IS_BIG_ENDIAN);
			else
			    if (bigendian != (s->flags & SEC_IS_BIG_ENDIAN))
			    {
				bigendian = -1;
				break;
			    }
		    }
		}
		
		if (bigendian == -1)
		    targetendian = "varies";
		else
		    targetendian = bigendian ? "big" : "little";
	    }
	    else
		targetendian = BFD_BIG_ENDIAN_TARG_P(abfd) ? "big" : "little";
	}
	else if ( bfd_check_format ( abfd, bfd_archive ) )
	    omf = "archive";
	if ( flagseen['p'] )
	    printf("%s %s %s %s\n",fn,omf,hostendian,targetendian);
	else 
	{
	    printf("%-18s %s\n","File:",fn);
	    printf("%-18s %s\n","OMF:",omf);
	    printf("%-18s %s\n","Host Byte Order:",hostendian);
	    printf("%-18s %s\n","Target Byte Order:",targetendian);
	}
    }
    
    if ( ! bfd_check_format ( abfd, bfd_object ) ) 
    {
	if ( ! flagseen['q'] )
	    bfd_perror(fn);
	fclose(file_handle);
	return flagseen['q'];  
    }
    
    /* Now we know it's an object (not an archive) file */
    file_is_big_endian  = BFD_BIG_ENDIAN_FILE_P(abfd);
    
    /* for objmap */
    if ( flagseen['m'] && abfd && bfd_check_format(abfd,bfd_object) == true )
    {
	file_is_big_endian =
	    (abfd->xvec->header_byteorder_big_p == true);
	if ( BFD_COFF_FILE_P(abfd) )
	    map_coff(abfd);
	else if ( BFD_BOUT_FILE_P(abfd) )
	    map_bout(abfd);
	else if ( BFD_ELF_FILE_P(abfd) )
	    map_elf();
    }
    
    if ( flagseen['n'] )
    {
	/* Dump only the given section */
	asection	*s = abfd->sections;

	for ( ; s != NULL; s = s->next )
	{
	    for (special_sectptr = special_sect; special_sectptr ; 
		 special_sectptr = special_sectptr->link)
	    {
		if ( ! strcmp(s->name, special_sectptr->name) )
		{
		    /* Found a match; the actual section number
		     * as seen by coff_dmp_symtab is one higher
		     * than the number found here
		     */
		    special_sectptr->id = s->secnum + 1;
		    break;
		}
	    }
	}

	for (special_sectptr = special_sect; special_sectptr ; 
	     special_sectptr = special_sectptr->link)
	{
	    if ( special_sectptr->id < 0)
	    {
		error ("No such section: %s", special_sect->name);
		return 0;
	    }
	}
    }
    
    if ( flagseen['F'] && BFD_BOUT_FILE_P(abfd) )
    {
	error("Option -F is implemented for COFF and ELF files only");
	flagseen['F'] = 0;
	return 0;
    }
    
    if ( flagseen['s'] || flagseen['F'] )
    {
	/* Symbolic disassembly */
	if ( build_value_table() )
	{
	    build_reloc_table();
	}
	else
	{
	    /* build_value_table found no symbols */
	    if ( flagseen['s'] )
		error ("No symbols found.  Can't do symbolic disassembly.");
	    else 
		error ("No symbols found.  Can't disassemble by function name.");
	    flagseen['s'] = flagseen['F'] = 0;
	    return 0;
	}
    }
    
    if ( flagseen['f'] ) 
    {
	/* Dump the file header */
	if ( BFD_COFF_FILE_P(abfd) ) 
	{
	    if (!dmp_coff_file_hdr())
		exit(1);
	}
	else if ( BFD_BOUT_FILE_P(abfd) ) 
	{
	    if (!dmp_bout_hdr())
		exit(1);
	}
	else if ( BFD_ELF_FILE_P(abfd) ) 
	{
	    if (!dmp_elf_hdr())
		exit(1);
	}
    }
    
    if ( flagseen['g'] )
    {
	/* Dump Dwarf sections */
	if ( BFD_ELF_FILE_P(abfd) )
	{
	    /* Open a libdwarf file handle.  The return value, if non-NULL,
	       can be used by all subsequent Dwarf-dumping functions. */
	    Dwarf_Debug dbg;
	    
	    Dwarf_Section sections = (Dwarf_Section)
		xmalloc(sizeof(struct Dwarf_Section) * 
			bfd_count_sections(abfd));

	    bfd_map_over_sections(abfd, 
				  build_libdwarf_section_list, 
				  (PTR) sections);
 
            /* Set up basic libdwarf error handling. */
            dwarf_seterrhand(dwarf2_errhand);
            dwarf_seterrarg("Libdwarf error in dwarf_init");

	    dbg = dwarf_init((FILE *) abfd->iostream,
			     DLC_READ, 
			     sections, 
			     bfd_count_sections(abfd));
            dwarf_seterrarg(NULL);
	    
	    if (dbg == NULL)
		fatal("Can't read Dwarf sections from file: %s", 
		      bfd_get_filename(abfd));
	    
	    if (dwarf_flagseen['i'] && ! dmp_debug_info(dbg))
		exit(1);
	    if (dwarf_flagseen['l'] && ! dmp_debug_line(dbg))
		exit(1);
	    if (dwarf_flagseen['f'] && ! dmp_debug_frame(dbg))
		exit(1);
	    if (dwarf_flagseen['p'] && ! dmp_debug_pubnames(dbg))
		exit(1);
	    if (dwarf_flagseen['a'] && ! dmp_debug_aranges(dbg))
		exit(1);
	    if (dwarf_flagseen['m'] && ! dmp_debug_macinfo(dbg))
		exit(1);

	    /* Free memory allocated by libdwarf */
	    dwarf_finish(dbg);
	}
	else
	{
	    error("Can only dump Dwarf sections from an Elf object");
	}
    }

    if ( flagseen['h'] )
    {
	/* Dump the section headers */
	if ( BFD_COFF_FILE_P(abfd) )
	{
	    if(!dmp_coff_section_hdrs()) exit(1);
	}
	else if ( BFD_BOUT_FILE_P(abfd) ) 
	{
	    if (!dmp_bout_hdr()) exit(1);
	}
	else if ( BFD_ELF_FILE_P(abfd) ) 
	{
	    if (!dmp_elf_section_hdrs()) exit(1);
	}
    }
    
    if ( flagseen['i'] )
    {
	/* Dump the COFF optional header */
	if ( BFD_COFF_FILE_P(abfd) )
	    if (!dmp_coff_optional_hdr()) exit(1);
    }
    
    if ( flagseen['d'] )
    {
	/* Disassemble.  Format of dump depends on other flags. */
	asection *s;
	char *sname;
		
	putchar ('\n');
	for ( s = abfd->sections; s != NULL; s = s->next )
	{
	    if ( flagseen['n'] )
	    {
		int secfound = 0;
		special_sectptr = special_sect;
		for ( special_sectptr = special_sect; 
		     special_sectptr; 
		     special_sectptr = special_sectptr->link)
		{
		    if (!strcmp(s->name, special_sectptr->name))
			secfound = 1;
		}
		if (!secfound) 
		    continue;
	    }
	    else
		/* We don't want to dump a BSS-type section
		 * unless it was expressly named with -n, or -a is in effect.
		 */
		if ( !flagseen['a'] &&
		    ! (bfd_get_section_flags(abfd, s) & SEC_LOAD) )
		    continue;

	    printf( sec_name_fmt, (char *) s->name, (s->flags & SEC_IS_BIG_ENDIAN) ?
		   " (Big Endian) " : "");
	    buf = getsection(s);
	    if ( flagseen['T'] )
	    {
		/* Disassemble it like a text section */
		if (!disasm( buf, bfd_section_size(abfd,s), 
			    bfd_section_vma(abfd,s),
			    s->flags & SEC_IS_BIG_ENDIAN ))
		    exit(1);
		if (flagseen['F'])
		    /* list of function has been disassembled, quit */
		    break;	
	    }
	    else
		if ( flagseen['D'] )
		    /* Dump it like a data section */
		    dmp_data( buf, bfd_section_size(abfd,s), 
			     bfd_section_vma(abfd,s) );
		else
		    /* Use whichever makes sense for the section type */
		    if ( bfd_get_section_flags(abfd,s) & SEC_CODE ) 
		    {
			if (!disasm( buf, bfd_section_size(abfd,s),
				    bfd_section_vma(abfd,s),
				    s->flags & SEC_IS_BIG_ENDIAN ) )
			    exit(1);
			if (flagseen['F'])
			    /* list of functions has been disassembled, quit */
			    break; 
		    }
		    else
			dmp_data( buf, bfd_section_size(abfd,s), bfd_section_vma(abfd,s) );
	    if (buf) 
		free( buf );
	}
    }
    
    if ( flagseen['r'] )
    {
	/* Dump relocation information */
	if ( BFD_COFF_FILE_P(abfd) )
	    dmp_coff_rel(abfd);
	else if ( BFD_ELF_FILE_P(abfd) )
	    dmp_elf_rel(abfd);
	else if ( BFD_BOUT_FILE_P(abfd) )
	    dmp_bout_rel();
    }
    
    if ( flagseen['l'] )
    {
	/* Dump line number information */
	if ( ! bfd_dmp_linenos(abfd, flagseen['p']) )
	    error ("Can't read symbol table for %s", abfd->filename );
    }
    
    if ( flagseen['t'] )
    {
	if (flagseen['n'])
	{
	    for ( special_sectptr = special_sect; 
		 special_sectptr ; 
		 special_sectptr = special_sectptr->link)
		if ( ! bfd_dmp_symtab(abfd, flagseen['p'], flagseen['x'], 
				      special_sectptr->id) )
		    error ("Can't read symbol table for %s", abfd->filename );
	}
	else
	    /* Dump the symbol table */
	    if ( ! bfd_dmp_symtab(abfd, flagseen['p'], flagseen['x'], -1) )
		error ("Can't read symbol table for %s", abfd->filename );
    }
    
    if ( flagseen['c'] )
    {
	/* Dump the string table */
	if ( BFD_COFF_FILE_P(abfd) )
	{
	    if (!dmp_coff_stringtab()) exit(1);
	}
	else if( BFD_BOUT_FILE_P(abfd) ) 
	{
	    if (!dmp_bout_stringtab()) exit(1);
	}
	else if( BFD_ELF_FILE_P(abfd) ) 
	{
	    if (!dmp_elf_stringtab()) exit(1);
	}
    }
    
    bfd_close(abfd);
    fclose(file_handle);
    return 1;
} /* dmp */


/***************************************************************************
 *  The is the same disassembler as used in gdb960.                        *
 ***************************************************************************/

struct tabent {
	char	*name;
	char	numops;
};

static
int
pinsn( memaddr, word1, word2 )
    unsigned long memaddr;
    unsigned long word1, word2;
{
	int instr_len;

	instr_len = 4;
	put_abs( word1, word2 );

	/* Divide instruction set into classes based on high 4 bits of opcode*/
	switch ( (word1 >> 28) & 0xf ){
	case 0x0:
	case 0x1:
		ctrl( memaddr, word1 );
		break;
	case 0x2:
	case 0x3:
		cobr( memaddr, word1 );
		break;
	case 0x5:
	case 0x6:
	case 0x7:
		reg( word1 );
		break;
	case 0x8:
	case 0x9:
	case 0xa:
	case 0xb:
	case 0xc:
	case 0xd:
		if (instr_len = mem( memaddr, word1, word2, 1 ))
			instr_len = mem( memaddr, word1, word2, 0 );
		else {
		    invalid(word1);
		    instr_len = 4;
		}
		break;
	default:
		/* invalid instruction, print as data word */ 
		invalid( word1 );
		break;
	}
	return instr_len;
}

/****************************************/
/* CTRL format				*/
/****************************************/
static
ctrl( memaddr, word1 )
    unsigned long memaddr;
    unsigned long word1;
{
	int i;
	static struct tabent ctrl_tab[] = {
		NULL,		0,	/* 0x00 */
		"syscall",	0,	/* 0x01 */	/* CA simulator only */
		NULL,		0,	/* 0x02 */
		NULL,		0,	/* 0x03 */
		NULL,		0,	/* 0x04 */
		NULL,		0,	/* 0x05 */
		NULL,		0,	/* 0x06 */
		NULL,		0,	/* 0x07 */
		"b",		1,	/* 0x08 */
		"call",		1,	/* 0x09 */
		"ret",		0,	/* 0x0a */
		"bal",		1,	/* 0x0b */
		NULL,		0,	/* 0x0c */
		NULL,		0,	/* 0x0d */
		NULL,		0,	/* 0x0e */
		NULL,		0,	/* 0x0f */
		"bno",		1,	/* 0x10 */
		"bg",		1,	/* 0x11 */
		"be",		1,	/* 0x12 */
		"bge",		1,	/* 0x13 */
		"bl",		1,	/* 0x14 */
		"bne",		1,	/* 0x15 */
		"ble",		1,	/* 0x16 */
		"bo",		1,	/* 0x17 */
		"faultno",	0,	/* 0x18 */
		"faultg",	0,	/* 0x19 */
		"faulte",	0,	/* 0x1a */
		"faultge",	0,	/* 0x1b */
		"faultl",	0,	/* 0x1c */
		"faultne",	0,	/* 0x1d */
		"faultle",	0,	/* 0x1e */
		"faulto",	0,	/* 0x1f */
	};

	i = (word1 >> 24) & 0xff;
	if ( (ctrl_tab[i].name == NULL) || ((word1 & 1) != 0) ){
		invalid( word1 );
		return;
	}

	fputs( ctrl_tab[i].name, stream );
	if ( word1 & 2 ){		/* Predicts branch not taken */
		fputs( ".f", stream );
		word1 &= ~2;
	}

	if ( ctrl_tab[i].numops == 1 ){
		/* EXTRACT DISPLACEMENT AND CONVERT TO ADDRESS */
		word1 &= 0x00ffffff;
		if ( word1 & 0x00800000 ){		/* Sign bit is set */
			word1 |= (-1 & ~0xffffff);	/* Sign extend */
		}
		putc( '\t', stream );
		fprintf(stream, "%s", symbolic_address(word1 + memaddr, memaddr));
	}
} /* ctrl */

/****************************************/
/* COBR format				*/
/****************************************/
static
cobr( memaddr, word1 )
    unsigned long memaddr;
    unsigned long word1;
{
	int src1;
	int src2;
	int i;

	static struct tabent cobr_tab[] = {
		"testno",	1,	/* 0x20 */
		"testg",	1,	/* 0x21 */
		"teste",	1,	/* 0x22 */
		"testge",	1,	/* 0x23 */
		"testl",	1,	/* 0x24 */
		"testne",	1,	/* 0x25 */
		"testle",	1,	/* 0x26 */
		"testo",	1,	/* 0x27 */
		NULL,		0,	/* 0x28 */
		NULL,		0,	/* 0x29 */
		NULL,		0,	/* 0x2a */
		NULL,		0,	/* 0x2b */
		NULL,		0,	/* 0x2c */
		NULL,		0,	/* 0x2d */
		NULL,		0,	/* 0x2e */
		NULL,		0,	/* 0x2f */
		"bbc",		3,	/* 0x30 */
		"cmpobg",	3,	/* 0x31 */
		"cmpobe",	3,	/* 0x32 */
		"cmpobge",	3,	/* 0x33 */
		"cmpobl",	3,	/* 0x34 */
		"cmpobne",	3,	/* 0x35 */
		"cmpoble",	3,	/* 0x36 */
		"bbs",		3,	/* 0x37 */
		"cmpibno",	3,	/* 0x38 */
		"cmpibg",	3,	/* 0x39 */
		"cmpibe",	3,	/* 0x3a */
		"cmpibge",	3,	/* 0x3b */
		"cmpibl",	3,	/* 0x3c */
		"cmpibne",	3,	/* 0x3d */
		"cmpible",	3,	/* 0x3e */
		"cmpibo",	3,	/* 0x3f */
	};

	i = ((word1 >> 24) & 0xff) - 0x20;
	if ( cobr_tab[i].name == NULL ){
		invalid( word1 );
		return;
	}

	fputs( cobr_tab[i].name, stream );
	if ( word1 & 2 ){		/* Predicts branch not taken */
		fputs( ".f", stream );
		word1 &= ~2;
	}
	putc( '\t', stream );

	src1 = (word1 >> 19) & 0x1f;
	src2 = (word1 >> 14) & 0x1f;

	if ( word1 & 0x02000 ){		/* M1 is 1 */
		fprintf( stream, "%d", src1 );
	} else {			/* M1 is 0 */
		fputs( reg_names[src1], stream );
	}

	if ( cobr_tab[i].numops > 1 ){
		if ( word1 & 1 ){		/* S2 is 1 */
			fprintf( stream, ",sf%d,", src2 );
		} else {			/* S1 is 0 */
			fprintf( stream, ",%s,", reg_names[src2] );
		}

		/* Extract displacement and convert to address
		 */
		word1 &= 0x00001ffc;
		if ( word1 & 0x00001000 ){	/* Negative displacement */
			word1 |= (-1 & ~0x1fff);	/* Sign extend */
		}
		fprintf(stream, "%s", symbolic_address(memaddr + word1, memaddr));
	}
} /* cobr */

/****************************************/
/* MEM format				*/
/****************************************/
static
int				/* returns instruction length: 0 (not an instruction)
				   4 or 8 */
mem( memaddr, word1, word2, noprint )
    unsigned long memaddr;
    unsigned long word1, word2;
    int noprint;		/* If TRUE, return instruction length, but
				 * don't output any text.
				 */
{
    int i, j;
    int len;
    int mode;
    int offset;
    char *reg1, *reg2, *reg3;
    
    /* This lookup table is too sparse to make it worth typing in, but not
     * so large as to make a sparse array necessary.  We allocate the
     * table at runtime, initialize all entries to empty, and copy the
     * real ones in from an initialization table.
     *
     * NOTE: In this table, the meaning of 'numops' is:
     *	 1: single operand
     *	 2: 2 operands, load instruction
     *	-2: 2 operands, store instruction
     */
    static struct tabent *mem_tab = NULL;
    static struct { int opcode; char *name; char numops; } mem_init[] = {
#define MEM_MIN	0x80
	0x80,	"ldob",	 2,
	0x82,	"stob",	-2,
	0x84,	"bx",	 1,
	0x85,	"balx",	 2,
	0x86,	"callx", 1,
	0x88,	"ldos",	 2,
	0x8a,	"stos",	-2,
	0x8c,	"lda",	 2,
	0x90,	"ld",	 2,
	0x92,	"st",	-2,
	0x98,	"ldl",	 2,
	0x9a,	"stl",	-2,
	0x9d,	"dchint",1,
	0xa0,	"ldt",	 2,
	0xa2,	"stt",	-2,
	0xad,	"dcinva",1,
	0xb0,	"ldq",	 2,
	0xb2,	"stq",	-2,
	0xbd,	"dcflusha",1,
	0xc0,	"ldib",	 2,
	0xc2,	"stib",	-2,
	0xc8,	"ldis",	 2,
	0xca,	"stis",	-2,
	0xd0,   "icemark",2,
#define MEM_MAX	0xd0
#define MEM_NEL (MEM_MAX-MEM_MIN+1) 
#define MEM_SIZ	(MEM_NEL * sizeof(struct tabent))
	0,	NULL,	0
    };
    
    if ( mem_tab == NULL ){
	mem_tab = (struct tabent *) xmalloc( MEM_SIZ );
	bzero( mem_tab, MEM_SIZ );
	for ( i = 0; mem_init[i].opcode != 0; i++ ){
	    j = mem_init[i].opcode - MEM_MIN;
	    mem_tab[j].name = mem_init[i].name;
	    mem_tab[j].numops = mem_init[i].numops;
	}
    }
    
    i = ((word1 >> 24) & 0xff) - MEM_MIN;
    
    if (i >= MEM_NEL)
	{
	    if (!noprint)
		    invalid (word1);
	    return 0;
	}
    
    mode = (word1 >> 10) & 0xf;
    
    if (i >= 0 /* && i < MEM_NEL (not needed due to above check) */
	&& mem_tab[i].name != NULL		/* Valid instruction */
	&& ((mode == 5) || (mode >=12))){	/* With 32-bit displacement */
	len = 8;
    } else {
	len = 4;
    }
    
    if ( (mem_tab[i].name == NULL) || (mode == 6) ){
	if (!noprint)
		invalid( word1 );
	return 0;
    }
    
    if (!noprint)
	    fprintf( stream, "%s\t", mem_tab[i].name );
    
    reg1 = reg_names[ (word1 >> 19) & 0x1f ];	/* MEMB only */
    reg2 = reg_names[ (word1 >> 14) & 0x1f ];
    reg3 = reg_names[ word1 & 0x1f ];		/* MEMB only */
    offset = word1 & 0xfff;				/* MEMA only  */
    
    switch ( mem_tab[i].numops )
	{
	    
    case 2: /* LOAD INSTRUCTION */
	    if ( mode & 4 )
		{			/* MEMB FORMAT */
		    if (!effec_addr( memaddr, mode, reg2, reg3, word1, word2, noprint ))
			    return 0;
		    if (!noprint)
			    fprintf( stream, ",%s", reg1 );
		} 
	    else 
		{			/* MEMA FORMAT */
		    if (!noprint) {
			fprintf(stream, "%s", symbolic_address (offset, memaddr));
			if (mode & 8)
				fprintf( stream, "(%s)", reg2 );
			fprintf( stream, ",%s", reg1 );
		    }
		}
	    break;
	    
    case -2: /* STORE INSTRUCTION */
	    if ( mode & 4 )
		{			/* MEMB FORMAT */
		    if (!noprint)
			    fprintf( stream, "%s,", reg1 );
		    if (!effec_addr( memaddr, mode, reg2, reg3, word1, word2, noprint ))
			    return 0;
		} 
	    else 
		{			/* MEMA FORMAT */
		    if (! noprint) {
			fprintf( stream, "%s,", reg1 );
			fprintf(stream, "%s", symbolic_address(offset, memaddr));
		    }
		    if (mode & 8) {
			if (! noprint)
				fprintf( stream, "(%s)", reg2 );
		    }
		}
	    break;
	    
    case 1: /* BX/CALLX INSTRUCTION */
	    if ( mode & 4 )
		{			/* MEMB FORMAT */
		    if (!effec_addr( memaddr, mode, reg2, reg3, word1, word2, noprint ))
			    return 0;
		} 
	    else 
		{			/* MEMA FORMAT */
		    if (! noprint) {
			fprintf(stream, "%s", symbolic_address(offset, memaddr));
			if (mode & 8)
				fprintf( stream, "(%s)", reg2 );
		    }
		}
	    break;
	}
    
    return len;
} /* mem */

/****************************************/
/* REG format				*/
/****************************************/
static
reg( word1 )
    unsigned long word1;
{
	int i, j;
    	int opcode;
	int fp;
	int m1, m2, m3;
	int s1, s2;
	int src, src2, dst;
	int numops;
	char *mnemp;

	/* This lookup table is too sparse to make it worth typing in, but not
	 * so large as to make a sparse array necessary.  We allocate the
	 * table at runtime, initialize all entries to empty, and copy the
	 * real ones in from an initialization table.
	 *
	 * NOTE: In this table, the meaning of 'numops' is:
	 *	 1: single operand, which is NOT a destination.
	 *	-1: single operand, which IS a destination.
	 *	 2: 2 operands, the 2nd of which is NOT a destination.
	 *	-2: 2 operands, the 2nd of which IS a destination.
	 *	 3: 3 operands
	 *      -3: 3 operands, the 3rd of which is NOT a destination.
	 *
	 *	If an opcode mnemonic begins with "F", it is a floating-point
	 *	opcode (the "F" is not printed).
	 */

	static struct tabent *reg_tab = NULL;
	static struct { int opcode; char *name; char numops; } reg_init[] = {
#define REG_MIN	0x580
		0x580,	"notbit",	3,
		0x581,	"and",		3,
		0x582,	"andnot",	3,
		0x583,	"setbit",	3,
		0x584,	"notand",	3,
		0x586,	"xor",		3,
		0x587,	"or",		3,
		0x588,	"nor",		3,
		0x589,	"xnor",		3,
		0x58a,	"not",		-2,
		0x58b,	"ornot",	3,
		0x58c,	"clrbit",	3,
		0x58d,	"notor",	3,
		0x58e,	"nand",		3,
		0x58f,	"alterbit",	3,
		0x590, 	"addo",		3,
		0x591, 	"addi",		3,
		0x592, 	"subo",		3,
		0x593, 	"subi",		3,
		0x594,	"cmpob",	2,	/* JX */
		0x595,	"cmpib",	2,	/* JX */
		0x596,	"cmpos",	2,	/* JX */
		0x597,	"cmpis",	2,	/* JX */
		0x598, 	"shro",		3,
		0x59a, 	"shrdi",	3,
		0x59b, 	"shri",		3,
		0x59c, 	"shlo",		3,
		0x59d, 	"rotate",	3,
		0x59e, 	"shli",		3,
		0x5a0, 	"cmpo",		2,
		0x5a1, 	"cmpi",		2,
		0x5a2, 	"concmpo",	2,
		0x5a3, 	"concmpi",	2,
		0x5a4, 	"cmpinco",	3,
		0x5a5, 	"cmpinci",	3,
		0x5a6, 	"cmpdeco",	3,
		0x5a7, 	"cmpdeci",	3,
		0x5ac, 	"scanbyte",	2,
		0x5ad,	"bswap",	-2,	/* JX */
		0x5ae, 	"chkbit",	2,
		0x5b0, 	"addc",		3,
		0x5b2, 	"subc",		3,
		0x5b4,	"intdis",	0,	/* JX */
		0x5b5,	"inten",	0,	/* JX */
		0x5cc,	"mov",		-2,
		0x5d8,	"eshro",	3,
		0x5dc,	"movl",		-2,
		0x5ec,	"movt",		-2,
		0x5fc,	"movq",		-2,
		0x600,	"synmov",	2,
		0x601,	"synmovl",	2,
		0x602,	"synmovq",	2,
		0x610,	"atmod",	3,
		0x612,	"atadd",	3,
		0x615,	"synld",	-2,
		0x630,	"sdma",		3,
		0x631,	"udma",		0,
		0x640,	"spanbit",	-2,
		0x641,	"scanbit",	-2,
		0x642,	"daddc",	3,
		0x643,	"dsubc",	3,
		0x644,	"dmovt",	-2,
		0x645,	"modac",	3,
		0x650,	"modify",	3,
		0x651,	"extract",	3,
		0x654,	"modtc",	3,
		0x655,	"modpc",	3,
		0x658,	"intctl",	-2,      /* JX */
		0x659,	"sysctl",	3,
		0x65b,	"icctl",	3,	/* JX */
		0x65c,	"dcctl",	3,	/* JX */
		0x65d,	"halt",		1,	/* JX */
		0x660,	"calls",	1,
		0x66b,	"mark",		0,
		0x66c,	"fmark",	0,
		0x66d,	"flushreg",	0,
		0x66f,	"syncf",	0,
		0x670,	"emul",		3,
		0x671,	"ediv",		3,
 		0x672,	"cvtadr",	-2,
		0x674,	"Fcvtir",	-2,
		0x675,	"Fcvtilr",	-2,
		0x676,	"Fscalerl",	3,
		0x677,	"Fscaler",	3,
		0x680,	"Fatanr",	3,
		0x681,	"Flogepr",	3,
		0x682,	"Flogr",	3,
		0x683,	"Fremr",	3,
		0x684,	"Fcmpor",	2,
		0x685,	"Fcmpr",	2,
		0x688,	"Fsqrtr",	-2,
		0x689,	"Fexpr",	-2,
		0x68a,	"Flogbnr",	-2,
		0x68b,	"Froundr",	-2,
		0x68c,	"Fsinr",	-2,
		0x68d,	"Fcosr",	-2,
		0x68e,	"Ftanr",	-2,
		0x68f,	"Fclassr",	1,
		0x690,	"Fatanrl",	3,
		0x691,	"Flogeprl",	3,
		0x692,	"Flogrl",	3,
		0x693,	"Fremrl",	3,
		0x694,	"Fcmporl",	2,
		0x695,	"Fcmprl",	2,
		0x698,	"Fsqrtrl",	-2,
		0x699,	"Fexprl",	-2,
		0x69a,	"Flogbnrl",	-2,
		0x69b,	"Froundrl",	-2,
		0x69c,	"Fsinrl",	-2,
		0x69d,	"Fcosrl",	-2,
		0x69e,	"Ftanrl",	-2,
		0x69f,	"Fclassrl",	1,
		0x6c0,	"Fcvtri",	-2,
		0x6c1,	"Fcvtril",	-2,
		0x6c2,	"Fcvtzri",	-2,
		0x6c3,	"Fcvtzril",	-2,
		0x6c9,	"Fmovr",	-2,
		0x6d9,	"Fmovrl",	-2,
		0x6e1, 	"Fmovre",	-2,
		0x6e2, 	"Fcpysre",	3,
		0x6e3, 	"Fcpyrsre",	3,
		0x701,	"mulo",		3,
		0x703,	"cpmulo",	3,
		0x708,	"remo",		3,
		0x70b,	"divo",		3,
		0x741,	"muli",		3,
		0x748,	"remi",		3,
		0x749,	"modi",		3,
		0x74b,	"divi",		3,
		0x780,	"addono",	3,	/* JX */
		0x781,	"addino",	3,	/* JX */
		0x782,	"subono",	3,	/* JX */
		0x783,	"subino",	3,	/* JX */
		0x784,	"selno",	3,	/* JX */
		0x78b,	"Fdivr",	3,
		0x78c,	"Fmulr",	3,
		0x78d,	"Fsubr",	3,
		0x78f,	"Faddr",	3,
		0x790,	"addog",	3,	/* JX */
		0x791,	"addig",	3,	/* JX */
		0x792,	"subog",	3,	/* JX */
		0x794,	"selg",		3,	/* JX */
		0x793,	"subig",	3,	/* JX */
		0x79b,	"Fdivrl",	3,
		0x79c,	"Fmulrl",	3,
		0x79d,	"Fsubrl",	3,
		0x79f,	"Faddrl",	3,
		0x7a0,	"addoe",	3,	/* JX */
		0x7a1,	"addie",	3,	/* JX */
		0x7a2,	"suboe",	3,	/* JX */
		0x7a3,	"subie",	3,	/* JX */
		0x7a4,	"sele",		3,	/* JX */
		0x7b0,	"addoge",	3,	/* JX */
		0x7b1,	"addige",	3,	/* JX */
		0x7b2,	"suboge",	3,	/* JX */
		0x7b3,	"subige",	3,	/* JX */
		0x7b4,	"selge",	3,	/* JX */
		0x7c0,	"addol",	3,	/* JX */
		0x7c1,	"addil",	3,	/* JX */
		0x7c2,	"subol",	3,	/* JX */
		0x7c3,	"subil",	3,	/* JX */
		0x7c4,	"sell",		3,	/* JX */
		0x7d0,	"addone",	3,	/* JX */
		0x7d1,	"addine",	3,	/* JX */
		0x7d2,	"subone",	3,	/* JX */
		0x7d3,	"subine",	3,	/* JX */
		0x7d4,	"selne",	3,	/* JX */
		0x7e0,	"addole",	3,	/* JX */
		0x7e1,	"addile",	3,	/* JX */
		0x7e2,	"subole",	3,	/* JX */
		0x7e3,	"subile",	3,	/* JX */
		0x7e4,	"selle",	3,	/* JX */
		0x7f0,	"addoo",	3,	/* JX */
		0x7f1,	"addio",	3,	/* JX */
		0x7f2,	"suboo",	3,	/* JX */
		0x7f3,	"subio",	3,	/* JX */
		0x7f4,	"selo",		3,	/* JX */

#define REG_MAX	0x7f4
#define REG_SIZ	((REG_MAX-REG_MIN+1) * sizeof(struct tabent))
		0,	NULL,	0
	};

	if ( reg_tab == NULL ){
		reg_tab = (struct tabent *) xmalloc( REG_SIZ );
		bzero( reg_tab, REG_SIZ );
		for ( i = 0; reg_init[i].opcode != 0; i++ ){
			j = reg_init[i].opcode - REG_MIN;
			reg_tab[j].name = reg_init[i].name;
			reg_tab[j].numops = reg_init[i].numops;
		}
	}

	opcode = ((word1 >> 20) & 0xff0) | ((word1 >> 7) & 0xf);
	i = opcode - REG_MIN;

	s1   = (word1 >> 5)  & 1;
	s2   = (word1 >> 6)  & 1;
	m1   = (word1 >> 11) & 1;
	m2   = (word1 >> 12) & 1;
	m3   = (word1 >> 13) & 1;
	src  =  word1        & 0x1f;
	src2 = (word1 >> 14) & 0x1f;
	dst  = (word1 >> 19) & 0x1f;

	/* coprocessor instructions need special support, check for them
         * here, and do whats necessary. */
        if ((opcode >= 0x500 && opcode <= 0x57f) ||
            (opcode >= 0x710 && opcode <= 0x73f) ||
            (opcode >= 0x750 && opcode <= 0x77f))
        {
		if (m3)
			fprintf (stream, "cpdcp\t0x%x,", opcode);
		else
			fprintf (stream, "cpd960\t0x%x,", opcode);
        	numops = -3;
		fp = 0;
        }
	else
	{
	    if ((opcode<REG_MIN) || (opcode>REG_MAX) ||
		(reg_tab[i].name==NULL)) {
		invalid( word1 );
		return;
	    }
	    mnemp = reg_tab[i].name;
	    if ( mnemp && *mnemp == 'F' ) {
		fp = 1;
		mnemp++;
	    }
	    else
		    fp = 0;
	    numops = reg_tab[i].numops;


	    if (((numops > 0 || numops <= -2) && (s1 && (src > 4)))  ||
		((numops > 1 || numops < -2) && (s2 && (src2 > 4))) ||
		((numops > 2 || numops < 0) && (!fp && m3 && (dst > 4)))) {
		invalid( word1 );
		return;
	    }

	    fputs( mnemp, stream );
	    
	    if (numops != 0)
		    putc( '\t', stream );
        }


	if  ( numops != 0 )
	{
		switch ( numops )
		{
		case -3:
			regop( m1, s1, src, fp );
			putc( ',', stream );
			regop( m2, s2, src2, fp );
			putc( ',', stream );
			regop( m3, 0,  dst, fp );
			break;
		case -2:
			regop( m1, s1, src, fp );
			putc( ',', stream );
			dstop( m3, dst, fp );
			break;
		case -1:
			dstop( m3, dst, fp );
			break;
		case 1:
			regop( m1, s1, src, fp );
			break;
		case 2:
			regop( m1, s1, src, fp );
			putc( ',', stream );
			regop( m2, s2, src2, fp );
			break;
		case 3:
			regop( m1, s1, src, fp );
			putc( ',', stream );
			regop( m2, s2, src2, fp );
			putc( ',', stream );
			dstop( m3, dst, fp );
			break;
		}
	}
} /* reg */


/*
 * Print out effective address for memb instructions.
 * Returns 0 if it detects that the insn is not a valid insn.
 * Else, returns 1.
 */
static int
effec_addr( memaddr, mode, reg2, reg3, word1, word2, noprint )
    unsigned long memaddr;
    int mode;
    char *reg2, *reg3;
    unsigned long word1, word2;
    int noprint;
{
    int scale;
    int relocaddr = memaddr + 4;  /* used when -s is in effect. */
    static int scale_tab[] = { 1, 2, 4, 8, 16 };
    
    scale = (word1 >> 7) & 0x07;
    if ( (scale > 4) || ((word1 >> 5) & 0x03 != 0) ){
	if (!noprint)
		invalid( word1 );
	return 0;
    }
    scale = scale_tab[scale];
    
    switch (mode) {
 case 4:	 					/* (reg) */
	if (!noprint)
		fprintf( stream, "(%s)", reg2 );
	break;
 case 5:						/* displ+8(ip) */
	if (!noprint)
		fprintf(stream, "%s(ip)", symbolic_address(word2, relocaddr)); 
	break;
 case 7:						/* (reg)[index*scale] */
	if (scale == 1) {
	    if (!noprint)
		    fprintf( stream, "(%s)[%s]", reg2, reg3 );
	} else {
	    if (!noprint)
		    fprintf( stream, "(%s)[%s*%d]",reg2,reg3,scale);
	}
	break;
 case 12:					/* displacement */
	if (! noprint)
		fprintf(stream, "%s", symbolic_address( word2, relocaddr ));
	break;
 case 13:					/* displ(reg) */
	if (!noprint) {
	    fprintf(stream, "%s", symbolic_address( word2, relocaddr ));
	    fprintf( stream, "(%s)", reg2 );
	}
	break;
 case 14:					/* displ[index*scale] */
	if (! noprint) {
	    fprintf(stream, "%s", symbolic_address( word2, relocaddr ));
	    if (scale == 1) {
		fprintf( stream, "[%s]", reg3 );
	    } else {
		fprintf( stream, "[%s*%d]", reg3, scale );
	    }
	}
	break;
 case 15:				/* displ(reg)[index*scale] */
	if (!noprint) {
	    fprintf(stream, "%s", symbolic_address( word2, relocaddr ));
	    if (scale == 1) {
		fprintf( stream, "(%s)[%s]", reg2, reg3 );
	    } else {
		fprintf( stream, "(%s)[%s*%d]",reg2,reg3,scale );
	    }
	}
	break;
 default:
	if (! noprint)
		invalid( word1 );
	return 0;
    }
    return 1;
} /* effec_addr */


/************************************************/
/* Register Instruction Operand  		*/
/************************************************/
static
regop( mode, spec, reg, fp )
    int mode, spec, reg, fp;
{
	if ( fp ){				/* FLOATING POINT INSTRUCTION */
		if ( mode == 1 ){			/* FP operand */
			switch ( reg ){
			case 0:  fputs( "fp0", stream );	break;
			case 1:  fputs( "fp1", stream );	break;
			case 2:  fputs( "fp2", stream );	break;
			case 3:  fputs( "fp3", stream );	break;
			case 16: fputs( "0f0.0", stream );	break;
			case 22: fputs( "0f1.0", stream );	break;
			default: putc( '?', stream );		break;
			}
		} else {				/* Non-FP register */
			fputs( reg_names[reg], stream );
		}
	} else {				/* NOT FLOATING POINT */
		if ( mode == 1 ){			/* Literal */
			fprintf( stream, "%d", reg );
		} else {				/* Register */
			if ( spec == 0 ){
				fputs( reg_names[reg], stream );
			} else {
				fprintf( stream, "sf%d", reg );
			}
		}
	}
}

/************************************************/
/* Register Instruction Destination Operand	*/
/************************************************/
static
dstop( mode, reg, fp )
    int mode, reg, fp;
{
	/* 'dst' operand can't be a literal. On non-FP instructions,  register
	 * mode is assumed and "m3" acts as if were "s3";  on FP-instructions,
	 * sf registers are not allowed so m3 acts normally.
	 */
	 if ( fp ){
		regop( mode, 0, reg, fp );
	 } else {
		regop( 0, mode, reg, fp );
	 }
}


static
invalid( word1 )
    int word1;
{
	fprintf( stream, flagseen['o'] ? ".word\t%011o" : ".word\t0x%08x", word1 );
}	

static
char *
symbolic_address(a, loc)
	unsigned long a;   /* address to print */
	unsigned long loc; /* address of this word of this instruction */
{
	static char buf[16];	/* When symbolic disass. fails */
	char	*c;

	if ( flagseen['s'] && (c = get_sym_name (a, loc)) )
	{
		/* Symbolic disassembly is in effect; 
		 * and a symbol was found for this value 
		 */
		return c;
	}
	else
	{
		sprintf (buf,
			 flagseen['o'] ? (a > 0 ? "0%o" : "0") : "0x%x", 
			 a);
		return buf;
	}
}

static
put_abs( word1, word2 )
    unsigned long word1, word2;
{
#ifdef IN_GDB
	return;
#else
	int len;

	switch ( (word1 >> 28) & 0xf ){
	case 0x8:
	case 0x9:
	case 0xa:
	case 0xb:
	case 0xc:
	case 0xd:
		/* MEM format instruction */
		len = mem( 0, word1, word2, 1 );
		break;
	default:
		len = 4;
		break;
	}

	if ( len == 8 )
	{
		fprintf(stream, 
			flagseen['o'] ? "%011o %011o\t" : "%08x %08x\t", 
			word1, 
			word2);
	} 
	else 
	{
		fprintf(stream, 
			flagseen['o'] ? "%011o            \t" : "%08x         \t", 
			word1 );
	}

#endif
} /* put_abs */


/* 
 * get_sym_name()
 * 
 * For symbolic disassembly:
 * Find the first reasonable symbol name given a symbol value
 * and the current relocation address.  If there is only one
 * symbol with the given value, then use it; otherwise, choose
 * the name that matches a relocation for the current relocation
 * address.  If there are no matches, use the first name found.
 */
static
char	
*get_sym_name ( val, loc )
	long val;  /* value of symbol to find */
	long loc;  /* location of reloc if more than 1 match */
{
    char 	*result;
    int	sindex;		/* index into the value table */
    int	hits;		/* number of matches */
    asymbol	**sp;		/* all-purpose pointer into the value table */
    asymbol *best_sp;	/* all-purpose pointer to an asymbol */

    int	rtab_index;	/* index into the reloc table */
    arelent *first_rp;	/* all-purpose pointer to an arelent */

    /* First try a direct lookup on the value */
    hits = get_symbol_by_value (val, &sindex);

    switch ( hits ) {
 case 0:
	/* Nothing; probably an absolute number */
	return NULL;
 case 1:
	/* Nice shot */
	return (char *) value_table[sindex]->name;
 default:
	/* More than one symbol answering to this value;
	 * try to find a relocation targetting the given address,
	 * where the relocation's symbol has the same name as the
	 * found symbol, and in the same section
	 */
	best_sp = value_table[sindex];
	sp = & value_table[sindex];
	if ( numrels ) {
	    for ( ; hits; --hits, ++sp ) {
		if ( get_reloc_by_address (loc, *sp) )
			return (char *) (*sp)->name;
		else if (0 == ((*sp)->flags & (BSF_UNDEFINED | BSF_FORT_COMM)))
			best_sp = *sp;
	    }
	}
    }
    /* We return the first name we found which is, by construction,
       the best one we can find (see compare_symbol_values()). */
    return (char *) best_sp->name;
} /* get_sym_name */


/*
 * is_special_sym()
 *
 *   Return 1 if symbol is a special-compiler-generated
 *   symbol, else return 0.
 */
static int is_special_sym( name )
char *name;
{
	int j;

	/* 
	 * Symbols that should be filtered, since they only clutter
	 * the symbolic disassembly
	 */
	static char *sym_special [] =
	{
		"gcc_compiled.",
		"gcc2_compiled.",
		"___gnu_compiled_c",
		"ic_name_rules.",
		"",                  /* ELF leafproc/sysproc and first zero symbol. */
		NULL
	};

	for (j = 0; sym_special[j] != NULL; j++)
	{
		if (strcmp(name, sym_special[j]) == 0)
		{
			return 1;
		}
	}
	return 0;
}


/* 
 * build_value_table()
 *
 * For symbolic disassembly:  We need to be able to quickly search the
 * symbol table when all we know is a symbol's value.  To set this up,
 * we'll read in the canonicalized symbol table, and then make another
 * table, with symbols we know we won't ever need filtered out, sorted
 * by value.  That will allow a binary search later.  We need to keep 
 * the original table around for building the reloc table.
 *
 * "While you're at it", set the global variable numsyms to the total 
 * number of symbols in the value table.
 * 
 * This uses only high-level BFD calls.  This has the advantage that
 * it will work for both B.OUT and COFF, but it has the disadvantage
 * that an extra call to more_symbol_info() is required for COFF symbols.
 */
static
int
build_value_table()
{
	asymbol 		*src, **dest;
	more_symbol_info	msi;
	int			i, j;
	unsigned char *keep_symbol; /* an array for tagging symbols to keep */
	unsigned int storage;

	if ( symbol_table )
		/* Left over from a previous file */
		free(symbol_table);
	if ( value_table )
		/* Ditto */
		free(value_table);

	storage = get_symtab_upper_bound(abfd);
	if (storage <= 0 )
		return 0;
 	symbol_table = (asymbol **) xmalloc(storage);
	numrawsyms = bfd_canonicalize_symtab(abfd, symbol_table);
	numsyms = 0;
	if ( numrawsyms == 0 )
	{
		if (symbol_table) {
			free (symbol_table);
		}
		return 0;
	}
	
	/* First rip through the raw symbol table to determine how many 
	 * value table elements there will be.  Comments for these decisions
	 * can be found below in the copying loop. Then tag the symbols to keep.
	 */
	keep_symbol = (unsigned char *) xmalloc(numrawsyms);
	for ( i = 0; i < numrawsyms; ++i )
	{
		keep_symbol[i] = 1;
		src = symbol_table[i]; 
		/* We remove debug, commons and absoutes from considertation. */
		if (src->flags & BSF_FORT_COMM)
			src->value = 0;
		if (src->flags & BSF_DEBUGGING ||
		    (src->flags & (get_rid_of_abs * BSF_ABSOLUTE))) {
		    keep_symbol[i] = 0;
		    continue;
		}
		else if ( src->section && ! strcmp ( src->name, src->section->name ) )
		{
			keep_symbol[i] = 0;
			continue;
		}
		else if ( BFD_COFF_FILE_P(abfd) && bfd_more_symbol_info(abfd, src, &msi, bfd_storage_class) )
		{
			switch ( msi.sym_class )
			{
			case C_BLOCK:
			case C_FCN:
			case C_EOS:
			case C_EFCN:
			case C_MOS:
			case C_STRTAG:
			case C_MOU:
			case C_UNTAG:
			case C_TPDEF:
			case C_ENTAG:
			case C_MOE:
			case C_FILE:
			case C_LINE:
				keep_symbol[i] = 0;
				continue;

			case C_LABEL:
			case C_ULABEL: /* filter out special symbols */
				if (is_special_sym(src->name))
				{
					keep_symbol[i] = 0;
					continue;
				}
			default:
			        ;
			}
		}
		else if (is_special_sym(src->name))
		{
			keep_symbol[i] = 0;
			continue;
		}
		++numsyms;
	}

	if (numsyms <= 0)
		return 1;

	value_table = (asymbol **) xmalloc( numsyms * sizeof(asymbol *) );

	/* Now do the actual copying into the new value_table */
	for ( i = 0, dest = & value_table[0]; i < numrawsyms; ++i )
	{
		if (!keep_symbol[i]) continue;

		src = symbol_table[i];
		/* OK, make a new symbol and add to the value table */
		*dest = (asymbol *) xmalloc (sizeof(asymbol));
		**dest = *src;
		if ( src->section )
		{
			/* This is a defined external symbol.  Bfd's convention
			 * subtracts off the 
			 * section's start address of asymbols.
			 * So we now need to add the section's start address 
			 * back in.
			 */
			(*dest)->value += src->section->vma;
		}
		++dest;
	}
	free(keep_symbol);

	/* Now sort the resulting table by value.  */
	qsort((char *) value_table, numsyms, sizeof(asymbol *), compare_symbol_values);
	return 1;
} /* build_value_table */


/* 
 * get_symbol_by_value()
 *
 * For symbolic disassembly:
 * Return the number of times this value was found in the value table.
 * Fill a passed-in pointer with the index of the first match found.
 *
 * The call to bsearch is strange because the value table is an array of
 * pointers to asymbols, but bsearch expects pointers to the elements 
 * of the array, i.e. pointers to pointers to asymbols.
 */
static
int	
get_symbol_by_value (val, sindex)
	long 	val;
	int	*sindex;	/* The index into value_table where first match was found. */
{
	asymbol key;		/* trickery for bsearch() */
	asymbol *kp;		/* ditto; need pointer-to-pointer */
	asymbol **sp = NULL; 	/* all-purpose temp pointer */
	asymbol	**limit;	/* pointer to just past the last asymbol in value table */
	int	hits = 0;	/* number of matches found */

	key.value = val;
	key.name = NULL;
	kp = &key;
	
	if ( numsyms > 0 )
  	        sp = (asymbol **) bsearch( (char *) &kp, (char *) value_table, numsyms, sizeof(asymbol *), compare_symbol_values_only);

	if (sp)
	{
		/* Find first one */
		for ( ; sp >= value_table && (*sp)->value == val; --sp )
			;
		++sp;

		*sindex = ((char *) sp - (char *) value_table) / sizeof(asymbol *);

		/* Count the hits */
		limit = value_table + numsyms;
		for ( ; sp < limit && (*sp)->value == val; ++hits, ++sp )
			;
	}
	return hits;
} /* get_symbol_by_value */


/* 
 * build_reloc_table()
 *
 * For symbolic disassembly:  We need to be able to quickly search the
 * file's relocations when all we know is the address of the relocation.
 * To set this up, we'll read in the relocations, and then sort this 
 * list by address.  That will allow a binary search later.
 *
 * "While you're at it", set the global variable numrels to the total 
 * number of relocations in the reloc table.
 * 
 * This uses only high-level BFD calls.  This has the advantage that
 * it will work for both B.OUT and COFF, but the disadvantage that 
 * some COFF semantics are lost.
 */
static
int
build_reloc_table()
{
	int		i, j, ct;
	struct sec	*sec;	
	arelent 	**rp, **tp;
	int 		relcount;	/* Number relocs in a single section */
	unsigned long 	secaddr;
	int 		reloc_bytes = 0;

	/* Make one big table for all relocs for all sections. */
	for ( sec = abfd->sections; sec; sec = sec->next )
		reloc_bytes += get_reloc_upper_bound (abfd,sec);
	
	if ( reloc_table )
		/* Left over from a previous file */
		free(reloc_table);

	reloc_table = (arelent **) xmalloc( reloc_bytes );
       
	/* Go through each section adding relocs to the table */
	for ( rp = reloc_table, sec = abfd->sections, numrels = 0; sec; rp += relcount, sec = sec->next ) {
	    if ( get_reloc_upper_bound (abfd,sec) )	{
		relcount = bfd_canonicalize_reloc( abfd, sec, rp, symbol_table );
		numrels += relcount;
		secaddr = bfd_section_vma( abfd, sec );
		for ( tp = rp, ct = relcount; ct; ++tp, --ct )
			(*tp)->address += secaddr;
	    }
	}

	/* Now sort the resulting table by address.  */
	qsort((char *) reloc_table, numrels, sizeof(arelent *), compare_reloc_values);

} /* build_reloc_table */



/* 
 * get_reloc_by_address()
 * 
 * For symbolic disassembly:
 * Return non-zero if there a relocation for the given location,
 * for a symbol that matches the given name and section number.
 *
 * The call to bsearch is strange because the reloc table is an array of
 * pointers to arelents, but bsearch expects pointers to the elements 
 * of the array, i.e. pointers to pointers to arelents.
 */
static
int	
get_reloc_by_address (loc, sp)
	long 	loc;		/* address of desired relocation */
        asymbol *sp;            /* symbol pointer with desired name and section */
{
	arelent	key;		/* trickery for bsearch() */
	arelent *kp; 		/* ditto; need pointer-to-pointer */
	arelent *rp;		/* all-purpose temp pointer */
	arelent **rpp = NULL;	/* all-purpose temp double pointer */
	arelent	**limit;	/* pointer to just past the last arelent in reloc table */

	key.address = loc;
	kp = &key;
	if (numrels > 0 )
	        rpp = (arelent **) bsearch( (char *) &kp, (char *) reloc_table, numrels, sizeof(arelent *), compare_reloc_values);
	if (rpp)
	{
		/* Find first one */
		for ( ; rpp >= reloc_table && (*rpp)->address == loc; --rpp )
			;
		++rpp;

		/* Find end of reloc_table */
		limit = reloc_table + numrels;
		for ( ; rpp < limit && (*rpp)->address == loc; ++rpp )
		{
			/* Get a simple pointer */
			rp = *rpp;
			if ( rp->sym_ptr_ptr )
			{
				asymbol	*rpsp = *(rp->sym_ptr_ptr);
				if ( ! strcmp( sp->name, rpsp->name ) )
				{
					/* Found a match on the name; check section number */
					if ( sp->section )
					{
						if ( sp->section->secnum == rpsp->section->secnum )
							/* Eureka */
							return 1;
					}
					else
					{
						/* Some symbols (like this one) have no section 
						 * pointer; that means it is a .comm.  Because
						 * this relocation DOES have a symbol pointer 
						 * then its section pointer is also null.
						 */
						return 1;
					}
				} /* if name matches */
			}
#if 0

This piece of code will never be reached with the new bfd relocation scheme.  ALL
omfs use the sym_ptr_ptr.

			else
			{
				/* This is probably a bout relocation; it has a section
				 * pointer but no symbol pointer.  Just use the section
				 * number to compare.
				 */
				if ( sp->section )
				{
					if ( sp->section->secnum == rp->section->secnum )
						/* Eureka */
						return 1;
				}
				else
				{
					/* Some symbols (like this one) have no section 
					 * pointer; that means it is a .comm.  Because
					 * this relocation DOES have a section pointer 
					 * then this can't possibly be a match.
					 */
					continue;
				}
			} /* if relocation has a symbol pointer */
#endif
		} /* for all address matches */
	} /* if bsearch() returned anything */
	return 0;
} /* get_reloc_by_address */


/*
 * Comparators, one for symbols, one for relocations, suitable for use
 * by either qsort() or bsearch().  Note that value_table and reloc_table
 * are tables of pointers, hence the double indirection in the comparators.
 */

/* This one only considers the symbol values and is used by bsearch(). */
static
long
compare_symbol_values_only(sp1, sp2)
register asymbol **sp1;
register asymbol **sp2;
{
    return (long) (*sp1)->value - (long) (*sp2)->value;
}

/* This one considers both the symbol value, and its name.  
   If two symbols have the same value, it sorts them alphabetically
   with all of symbols containing a '.' occurring last. 
   This sorting is deliberate so in case in get_sym_name() it is unable
   to differentiate which symbol is meant (per relocations e.g.), then 
   it will choose the first one.
   This routine is used by qsort(). */
static
long
compare_symbol_values(sp1, sp2)
register asymbol **sp1;
register asymbol **sp2;
{
    long difference;
    int left_has_dot,right_has_dot;
    int left_is_null,right_is_null;

    if (difference = ((long) (*sp1)->value - (long) (*sp2)->value))
	    return difference;

    left_has_dot = (*sp1)->name && (NULL != strchr((*sp1)->name,'.'));
    right_has_dot = (*sp2)->name && (NULL != strchr((*sp2)->name,'.'));
    left_is_null = (*sp1)->name == NULL || *((*sp1)->name) == 0;
    right_is_null = (*sp2)->name == NULL || *((*sp2)->name) == 0;

    if (left_is_null != right_is_null)
	    return left_is_null ? 1 : -1;   /* If left is null, then order them RL else LR. */
    if (left_has_dot != right_has_dot)
	    return left_has_dot ? 1 : -1;   /* If left has dot, then order them RL else LR. */
    if ((*sp1)->name && (*sp2)->name)
	    return strcmp((*sp1)->name,(*sp2)->name);
    else
	    return 0;
}

static
long
compare_reloc_values(rp1, rp2)
register arelent **rp1;
register arelent **rp2;
{
	return (long) (*rp1)->address - (long) (*rp2)->address;
}


/*
 * get_function_by_name()
 *
 * For -F option:
 * Brute-force search of the symbol table for a symbol with the given name,
 * whose type is FUNCTION.  Return a pointer to its symbol table struct.
 *
 * NOTE: You must search the symbol table (i.e. the one returned by 
 * bfd_canonicalize_symtab) not the value table or else you will lose
 * the native (coff) symbol information.
 */
static
asymbol *
get_function_by_name (name)
	char	*name;
{
	register asymbol	*sym;
	register int		i;

	for ( i = 0; i < numrawsyms; ++i )
	{
		sym = symbol_table[i];
		if ( ! strcmp(sym->name, name) && is_function_type(sym) )
			return sym;
	}
	return  NULL;
}


/*
 * is_function_type()
 *
 * For -F option:
 * Uses more_symbol_info to find out if a COFF/ELF symbol is a function.
 *
 */
static
int
is_function_type( sym )
	asymbol	*sym;
{
    more_symbol_info	msi;

    if (BFD_BOUT_FILE_P(sym->the_bfd))
	return 0;
    if ( bfd_more_symbol_info(abfd, sym, &msi, bfd_symbol_type) ) 
    {
	unsigned long stype = msi.sym_type;
	if (BFD_COFF_FILE_P(sym->the_bfd))
	    return ISFCN(stype);
	else if (BFD_ELF_FILE_P(sym->the_bfd))
	    return ELF32_ST_TYPE(stype) == STT_FUNC;
    }
    return 0;
}


/*
 * print_label()
 *
 * In a text-type section, print a label symbolically, if there is one
 * for this address.  Filter out obviously bad choices, such as an
 * undefined symbol (value 0) or a label not in a text-type section.
 */
static
void
print_label( loc )
	long 	loc;   /* location counter for start of this instruction */
{
	int	vtindex; /* index in value_table of first match */
	asymbol **sp;  /* pointer into value table */
	int	hits = get_symbol_by_value (loc, &vtindex);
	char	*c;

	for ( ; hits; --hits, ++vtindex )
	{
		sp = & value_table [vtindex];
		if ( (*sp)->section && ((*sp)->section->flags & SEC_CODE) && ! ((*sp)->flags & BSF_UNDEFINED) )
		{
			/* Filter out the special coff labels .bb, .eb, .bf, and .ef 
			 * This method is ugly, but the alternative is worse.
			 * (i.e. Not using the asymbol struct at all.)
			 */
			if ( BFD_COFF_FILE_P(abfd) )
			{
				c = (char *) (*sp)->name;
				if ( *c == '.' && strlen(c) == 3 )
				{
					++c; 
					if ( *c == 'b' )
					{
						++c; 
						if ( *c == 'b' || *c == 'f' )
							continue;
					}
					else if ( *c == 'e' )
					{
						++c; 
						if ( *c == 'b' || *c == 'f' )
							continue;
					}
				}
			}
			printf (label_fmt, (*sp)->name);
		}
	}
} /* print_label */

#ifdef DEBUG

dump_value_table()
{
	register int i;
	register asymbol **sp;
	printf ("%10s%10s%10s%10s  %s\n", 
		"value", "flags", "section", "vma", "name");

	for ( i = 0; i < numsyms; ++i )
	{
		sp = & value_table [i];
		printf ("%10u%10x%10d%10d  %s\n", 
			(*sp)->value,
			(*sp)->flags,
			(*sp)->section ? (*sp)->section->secnum : 999,
			(*sp)->section ? (*sp)->section->vma : 999,
			(*sp)->name);
	}
}


dump_reloc_table()
{
	register int i;
	register arelent **rp;
	printf ("%10s%10s  %s\n", 
		"address", "section", "name");

	for ( i = 0; i < numrels; ++i )
	{
		rp = & reloc_table [i];
		printf ("%10x%10d  %s\n", 
			(*rp)->address,
			(*rp)->section ? (*rp)->section->secnum : 999,
			(*rp)->sym_ptr_ptr ? (*((*rp)->sym_ptr_ptr))->name : "null");
	}
}

#endif
