/*
 * Copyright (C) 1991 Free Software Foundation, Inc.
 *
 * nm.c is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * nm.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with nm.c.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Describe symbol table of an object file.
 * $Id: nm.c,v 1.31 1995/10/05 17:08:12 paulr Exp $
 */

#include "sysdep.h"
#include "bfd.h"
#include "coff.h"
#include "getopt.h"

PROTO(static boolean, display_file, (char *filename));
PROTO(static void, do_one_rel_file, (bfd *file));
PROTO(static unsigned int, filter_symbols, (bfd *file, asymbol **syms,
					 unsigned long symcount));
PROTO(static void, print_symbols, (bfd *file, asymbol **syms,
				     unsigned long symcount));
PROTO(static void, print_symdef_entry, (bfd * abfd));
PROTO(int, (*sorters[2][2]), (char *x, char *y));

extern char *program_name;
extern char *target;

/* Command options.  */

static int external_only = 0;		/* print external symbols only	   */
static int file_on_each_line = 0;	/* print file name on each line	   */
static int do_sort = 0;			/* print syms in order found	   */
static int print_debug_syms = 0;	/* print debugger-only symbols too */
static int print_armap = 0;		/* print symbol maps in archives   */
static int reverse_sort = 0;		/* sort in reverse order	   */
static int sort_numerically = 0;	/* sort in numeric order	   */
static int undefined_only = 0;		/* print undefined symbols only	   */
static int print_each_filename = 0;	/* used in archives		   */
static int parseable_format = 0;        /* print in parseable format       */
static int print_full = 0;              /* include .text, .data, .bss in listing */
static int no_header = 0;               /* don't include title in display  */
static int trunc = 0;                /* truncate filename to within     */
                                        /*    column widths.               */
static int show_names = 0;

/* option flags are used to communicate to icoffdmp.c of
 * what format it has to display symbols
 */
static char option_flags = 0;

#define F_HEXADECIMAL   0x00  /* all numbers displayed in hexadecimal */
#define F_DECIMAL       0x01   /* all numbers displayed in decimal */
#define F_OCTAL         0x02  /* all numbers displayed in octal */
#define F_EXTERNAL      0x04  /* external flag only */
#define F_UNDEFINED     0X08  /* undefined symbols only */
#define F_HEADER        0x10  /* don't display header */
#define F_FULL          0x20  /* display .text, .data, & .bss */
#define F_PREPEND       0x40  /* prepend filename to each display line */
#define F_TRUNCATE      0x80  /* truncate names to within column widths */

/* ELIMINATE THIS COMPLETELY SOME SAD */
struct option long_options[] = {
	{0, 0, 0, 0}
};

static enum Radix_type { decimal, hex, octal } radix = hex;


/* Display Title */
static char     *fmt_ar_title[2] = {
                        "\n\nSymbols from %s[%s]:\n\n",
                        "\n\nUndefined symbols from %s[%.16s]:\n\n"
			};

static char     *fmt_title[2] = {
                        "\n\nSymbols from %s:\n\n",
                        "\n\nUndefined symbols from %s:\n\n"
			};

/* for numeric display of values */
static	char *display_form[3] = {
		"%10lu ", "0x%08lx ", "%.11lo "};  /* decimal, hex, and octal */

static	char *display_undefined[3] = { /* for undefined symbols only */
		"%10s ", "%10s ", "%11s " /* decimal, hex, and octal */
		};



/* some error reporting functions */

static
char *utext[] = {
        "",
        "Symbol dumper for binary files",
        "",
        "Options:",
        "    -a    print the debug symbols",
        "    -e    print only global (external) symbols",
        "    -f    produce full output",
        "    	    [including .text, .data, and .bss symbols, which are",
        "    	     normally ignored]",
        "    -g     print only global (external) symbols (equivilant to -e)",
        "    -u     print undefined symbols only",
        "    -d     display numbers in decimal",
        "    -x     display numbers in hexidecimal (this is the default)",
        "    -o     display numbers in octal",
        "    -n     sort symbols alphabetically",
        "    	     [version 2.0 of gnm960 the -n option sorted numerically]",
        "    -v     sort by numerical value",
        "    -R     sort symbols in reverse (downward alphabetic or numeric) order",
        "    	     [in version 2.0 of gnm960 this was the -r option]",
        "    -p     print symbol value, type code, and name in three-column format",
        "    	     [in version 2.0 of gnm960 this was the default]",
        "    -r     prepend the file or library element name to each output line",
        "    	     [in version 2.0 of gnm960 this was -o option]",
        "    -h     remove header information",
        "    -s     describe the archive symbol map data in any input libraries",
        "    -T     truncate names to keep symbol names within column widths",
        "    -V     print version information and continue",
        "    -help  print this help message",
        "    -v960  print version information and exit",
        "",
        "See your user's guide for a complete command-line description.",
        "",
        NULL
};

put_gnm_help()
{
	int i;
	
	fprintf(stdout,"\nUsage: %s [-adefghnoprRsTuvVx | -help | -v960] filename...\n", program_name);
	paginator(utext);
}

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
                        put_gnm_help();
                        exit (0);
                }
        }
}

void
usage ()
{
	fprintf(stderr, "\nUsage: %s [-adefghnoprRsTuvVx | -help | -v960] filename...\n", program_name);
	fprintf(stderr, "Use -help option to get help\n\n");
}

int
main (argc, argv)
    int argc;
    char **argv;
{
	int c; /* sez which option char */
	int ind = 0; /* used by getopt and ignored by us */
	extern int optind; /* steps thru options */
	int retval;

	/* Check the command line for a resonse file, and handle it if found.*/
	argc = get_response_file(argc,&argv);
	program_name = *argv;
	if (argc == 1) {
		put_gnm_help();
		exit(0);
	}
	check_v960( argc, argv );	/* never returns */
	check_help( argc, argv );	/* never returns */

	while ((c = getopt_long(argc, argv, "adefghnoprRsTuvVx", long_options, &ind)) != EOF) {
		switch (c) {
		case 'a': print_debug_syms = 1;	break;

		case 'd': 
			radix = decimal;      
			option_flags |= F_DECIMAL;
			break;

		case 'e':
		case 'g': 
			external_only = 1;	
			option_flags |= F_EXTERNAL;
			break;

		case 'f': 
			print_full = 1; 
			option_flags |= F_FULL;
			break;

		case 'h': 
			no_header = 1;
			option_flags |= F_HEADER;
			break;

		case 'n': 
			sort_numerically = 0;
			do_sort = 1;
			break;

		case 'o':
			radix = octal;
			option_flags = F_OCTAL;
			break;
		case 'r': 
			option_flags |= F_PREPEND;
			file_on_each_line = 1;
			break;
		case 'p': parseable_format = 1;	break;
		case 'R': 
			reverse_sort = 1;
			do_sort = 1;
			break;
		case 's': print_armap = 1;	break;
		case 'T': 
			trunc = 1;
			option_flags |= F_TRUNCATE;
			break;

		case 'u': 
			undefined_only = 1;	
			option_flags |= F_UNDEFINED;
			break;

		case 'v':
			sort_numerically = 1;	
			do_sort = 1;
			break;

		case 'V': /* -V means print version but don't exit */
			gnu960_put_version();
			break;

		case 'x': 
			radix = hex;
			option_flags |= F_HEXADECIMAL;
			break;

		default:  usage ();		exit(1);
		}
	}

	
	/* OK, all options now parsed.  If no filename specified, do a.out. */
	if (optind == argc){
		return !display_file ("a.out");
	}

	/* We were given one or more filenames to do */
	retval = 0;
	show_names = (argc - optind) > 1;
	while (optind < argc){
		if (!display_file (argv[optind++])) {
			retval++;
		}
	}
	return retval;
}

/** Display a file's stats */

static boolean
display_file (filename)
    char *filename;
{
	bfd *file;
	bfd *arfile = NULL;
	boolean retval = true;

	file = bfd_openr(filename, target);
	if (file == NULL) {
		fprintf (stderr, "\n%s: can't open '%s'.\n",
						program_name, filename);
		return false;
	}

	if (bfd_check_format(file, bfd_object)) {
		if (show_names) {
			printf ("\n%s:\n",filename);
		}
		if (!no_header && !parseable_format) {
			printf(fmt_title[undefined_only], filename);
		}
		do_one_rel_file (file);

	} else if (bfd_check_format (file, bfd_archive)) {
		printf("\n%s:\n", filename);
		if (print_armap){
			print_symdef_entry (file);
		}
		for (;;) {
			arfile = bfd_openr_next_archived_file (file, arfile);
			if (arfile == NULL) {
				if (bfd_error != no_more_archived_files){
					bfd_fatal (filename);
					/* NO RETURN */
				}
				break;
			}
			if (!no_header && !parseable_format) {
				printf(fmt_ar_title[undefined_only],
				       filename, arfile->filename);
			}
			if (!bfd_check_format(arfile, bfd_object)) {
				printf("%s: not an object file\n",
							arfile->filename);
			} else {
				printf ("\n%s:\n", arfile->filename);
				do_one_rel_file (arfile);
			}
		}

	} else {

		fprintf (stderr, "\n%s:  %s: unknown format.\n",
						program_name, filename);
		retval = false;
	}

	if ( !bfd_close(file) ) {
		bfd_fatal (filename);
	}
	return retval;
}

static void
do_one_rel_file (abfd)
    bfd *abfd;
{
	unsigned int storage;
	asymbol **syms;
	unsigned int symcount = 0;

	if (!(bfd_get_file_flags (abfd) & HAS_SYMS)) {
		printf ("No symbols in \"%s\".\n", bfd_get_filename (abfd));
		return;
	}

	storage = get_symtab_upper_bound (abfd);
	if (storage == 0) {
		fprintf (stderr, "%s: Symflags set but there are none?\n",
		    bfd_get_filename (abfd));
		exit (1);
	}

	syms = (asymbol **) xmalloc (storage);

	symcount = bfd_canonicalize_symtab (abfd, syms);

	if (symcount == 0){
		fprintf (stderr, "%s: Symflags set but there are none?\n",
		    bfd_get_filename (abfd));
		exit (1);
	}

	/* Discard the symbols we don't want to print.
	 * It's OK to do this in place; we'll free the storage anyway
	 * (after printing)
	 */

	symcount = filter_symbols (abfd, syms, symcount);

	if (do_sort) {
		qsort((char *) syms, symcount, sizeof (asymbol *),
				sorters[sort_numerically][reverse_sort]);
	}

	if (print_each_filename && !file_on_each_line) {
		printf("\n%s:\n", bfd_get_filename(abfd));
	}

	/* support full formatted output for coff file only */
	if (parseable_format || !BFD_COFF_FILE_P(abfd))
		print_symbols(abfd, syms, symcount);
	else
		coff_dmp_full_fmt(abfd, syms, symcount, option_flags);

	free (syms);
}

static int
my_strcmp(l,r)
    char *l,*r;
{
    if (l && r)
	    return strcmp(l,r);
    else if (!l && !r)
	    return 0;
    else if (!l && r)
	    return -1;
    else return 1;
}

/* Symbol-sorting predicates */
#define valueof(x)  ((x)->section ? (x)->section->vma + (x)->value : (x)->value)
int
numeric_forward (x, y)
    char *x;
    char *y;
{
    unsigned long x_v;
    unsigned long y_v;

    x_v = valueof(*(asymbol **)x);
    y_v = valueof(*(asymbol **)y);

    if (x_v > y_v)
      return 1;
    else if (x_v < y_v)
      return -1;
    else
      return my_strcmp((*(asymbol **)x)->name,(*(asymbol **)y)->name);
}

int
numeric_reverse (x, y)
    char *x;
    char *y;
{
    unsigned long x_v;
    unsigned long y_v;

    x_v = valueof(*(asymbol **)x);
    y_v = valueof(*(asymbol **)y);

    if (y_v > x_v)
      return 1;
    else if (y_v < x_v)
      return -1;
    else
      return my_strcmp((*(asymbol **)y)->name,(*(asymbol **)x)->name);
}

int
non_numeric_forward (x, y)
    char *x;
    char *y;
{
	CONST char *xn = (*(asymbol **) x)->name;
	CONST char *yn = (*(asymbol **) y)->name;

	return xn ? (yn ? strcmp(xn, yn) : 1) : (yn ? -1 : 0);
}

int
non_numeric_reverse (x, y)
    char *x;
    char *y;
{
	return -(non_numeric_forward (x, y));
}

int (*sorters[2][2])() = {
	{non_numeric_forward, non_numeric_reverse},
	{numeric_forward, numeric_reverse},
};

/* Choose which symbol entries to print;
 * compact them downward to get rid of the rest.
 * Return the number of symbols to be printed.
 */
static unsigned int
filter_symbols (abfd, syms, symcount)
    bfd *abfd;
    asymbol **syms;
    unsigned long symcount;
{
	asymbol **from, **to;
	unsigned int dst_count = 0;
	unsigned int src_count;

	from = to = syms;
	for ( src_count = 0; src_count <symcount; src_count++) {
		int keep = 0;
		flagword flags = (from[src_count])->flags;

		if (undefined_only) {
			keep = flags & BSF_UNDEFINED;
		} else if (external_only) {
			keep = flags & (BSF_LOCAL|BSF_GLOBAL|BSF_UNDEFINED|BSF_FORT_COMM);
		} else {
			keep = 1;
		}
		if (flags & BSF_DEBUGGING) {
			keep = print_debug_syms;
		}
		if (keep) {
			to[dst_count++] = from[src_count];
		}
	}
	return dst_count;
}

/* Return a character corresponding to the symbol class of sym
 */
char
decode_symclass (abfd, sym)
    bfd *abfd;	
    asymbol *sym;
{
	flagword flags = sym->flags;

	if (flags & BSF_FORT_COMM) return 'C';
	if (flags & BSF_UNDEFINED) return 'U';
	if (flags & BSF_ABSOLUTE)  return 'a';

        if (flags & BSF_DEBUGGING) {
		more_symbol_info m;

		if (bfd_more_symbol_info( abfd, sym, &m, bfd_storage_class)) {
			if ( m.sym_class == C_FILE)  {
				return 'f';
			}
			else {
				return '?';
			}
	        } 
		return '?';
	}

	if ( (flags & BSF_GLOBAL) || (flags & BSF_LOCAL) ){
	        if (sym->section != NULL && sym->section->name != NULL) {
		       if ( !strcmp(sym->section->name, ".text") ){
			    return 't';
		       } else if ( !strcmp(sym->section->name, ".data") ){
			    return 'd';
		       } else if ( !strcmp(sym->section->name, ".bss") ){
			    return 'b';
		       } else {
			    return 'o';
		       }
		} else {
                    return 'o';
		}
	}

	return '?';
}


static void
print_symbols (abfd, syms, symcount)
    bfd *abfd;
    asymbol **syms;
    unsigned long symcount;
{
	asymbol **sym = syms, **end = syms + symcount;
	char class;

	for (; sym < end; ++sym) {
		/* only display .text, .data, .bss, if '-f' option */
                if ( (*sym != NULL) && ((*sym)->name != NULL) ) {
		        if (!print_full && (!strcmp((*sym)->name, ".text") ||
		            !strcmp((*sym)->name, ".data") ||
		            !strcmp((*sym)->name, ".bss"))) {
			        continue;
		        }
		}
		if (file_on_each_line) {
			printf("%s:", bfd_get_filename(abfd));
		}

		if (undefined_only) {
			if ((*sym)->flags & BSF_UNDEFINED) {
				puts ((*sym)->name);
			}
		} else {
			asymbol *p = *sym;
			if (p) {
				class = decode_symclass (abfd, p);
				if (p->flags & BSF_GLOBAL) {
					class = toupper (class);
				}

				if (p->value || !(p->flags & BSF_UNDEFINED)) {
					printf (display_form[(int)radix], p->section
						? p->value + p->section->vma
						: p->value);
				} else {
					printf(display_undefined[(int)radix], " ");
				}
				printf ("%c %s\n", class, p->name == NULL ? "" : p->name);
			}
		}
	}
}

static void
print_symdef_entry (abfd)
    bfd * abfd;
{
	symindex idx = BFD_NO_MORE_SYMBOLS;
	carsym *thesym;
	boolean everprinted = false;
	bfd *elt;

	for (idx = bfd_get_next_mapent (abfd, idx, &thesym);
	    idx != BFD_NO_MORE_SYMBOLS;
	    idx = bfd_get_next_mapent (abfd, idx, &thesym)) {
		if (!everprinted) {
			printf ("\nArchive index:\n");
			everprinted = true;
		}
		elt = bfd_get_elt_at_index (abfd, idx);
		if (thesym->name) {
			printf ("%s in %s\n",
					thesym->name, bfd_get_filename (elt));
		}
	}
}



