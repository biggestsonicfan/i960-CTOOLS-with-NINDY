
/*(c**************************************************************************** *
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


/*********************************************************************
 *
 * ghist960: histogram of 80960 program execution.
 *
 * This program constructs statistical profile information when
 * provided an application executable file and an associated binary raw
 * profile data file. It matches bucket hits to the executable file's
 * symbolic debug line number information. See the ghist960 manual
 * for a complete description of the functionality offered by this
 * program.
 *
 *********************************************************************/


	/************************
	 *			*
	 *    INCLUDE FILES	*
	 *			*
	 ************************/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#ifdef DOS
#	include <fcntl.h>
#else
#	include <sys/file.h>
#endif
#include "getopt.h"
#include "bfd.h"
#include "libbfd.h"
#include "elf.h"
#include "libdwarf.h"
#ifdef USG
#   if !defined(__HIGHC__) && !defined(WIN95) /* Microsoft C 9.00 (Windows 95) or Metaware C */
#      include <unistd.h>
#   endif 
#endif
#ifdef DOS
#	include "gnudos.h"
#endif

	/************************
	 *			*
	 *  FORWARD REFERENCES	*
	 *			*
	 ************************/

extern void	data_file();
extern void	next_entry();
extern void	print_profile();
extern void     read_int();
extern void	dump_internal_table();
extern void	print_bar();
extern void	byteswap();

	/************************
	 *			*
	 *   STRING CONSTANTS  	*
	 *			*
	 ************************/

static char	*Version	= "V3.0";
static char	*default_data	= "ghist.dat";
static char	bell		= '\007';

	/************************
	 *			*
	 *   GLOBAL VARIABLES  	*
	 *			*
	 ************************/

FILE		*data_desc;
char		*app_file_name;
bfd             *app_bfd;
char		*data_file_name;
char		*program_name;
int		bucket_size;
int		done;
int		total_hits;
int		total_line_numbers;
int		dump_symbols;
int		all_symbols;
int		print_hits;
int		print_header;
int		tally_functions;
bfd_ghist_info	*sym_table;
unsigned int    sym_table_length;



/*********************************************************************
 *
 * NAME
 *	main standard unix entry point.
 *
 * DESCRIPTION
 *	Set the necessary global variables. Process the program's
 *	arguments, then initiate the program execution by slurping
 *	the applications symbols using bfd.
 *
 * PARAMETERS
 *	argc	- unix standard argument count.
 *	argv	- unix standard argument list.
 *
 * RETURNS
 *	Nothing.
 *
 *********************************************************************/

char *utext[] = {
        "",
        "Ghist960: print runtime profile results",
        "",
        "   -a:  print all buckets with one or more hits",
        "   -d:  print the internal symbol table",
        "   -f:  print the number of hits for each function",
        "   -h:  display this help message",
	"         [warning: this generates large amounts of data]",
        "   -n:  print the number of hits for each bucket",
        "   -s:  suppress printing of miscellaneous header/footer/format info",
        "   -V:  print version information and continue",
        "-v960:  print version information and exit",
        "",
	"See your user's guide for a complete command-line description",
        "",
        NULL
};

static put_ghist_help()
{
    int i;

	fprintf(stdout,"\nUsage: %s [-ahdfnsV | v960] application [data file]\n",program_name);
	paginator( utext );
} /* put_ghist_help */


static usage()
{
    fprintf(stderr,"\nusage: %s [-ahdfnsV | v960] application [data file]\n",
	    program_name);
    fprintf(stderr,"use -h option to get help\n\n");
    exit(1);
}

static
void
build_libdwarf_section_list (abfd, sectp, sect_list)
     bfd *abfd;
     asection *sectp;
     PTR sect_list;
{
    Dwarf_Section sections = (Dwarf_Section) sect_list;
    sections[sectp->secnum - 1].name = (char *) bfd_section_name(abfd, sectp);
    sections[sectp->secnum - 1].size = bfd_section_size(abfd, sectp);
    sections[sectp->secnum - 1].file_offset = sectp->filepos;
    sections[sectp->secnum - 1].big_endian = bfd_get_section_flags(abfd, sectp) & SHF_960_MSB;
}


static bfd_ghist_info *
dwarf_fetch_ghist_info(abfd, p, nelements, nlinenumbers)
    bfd *abfd;
    bfd_ghist_info *p;
    unsigned int *nelements,*nlinenumbers;
{
    bfd_ghist_info	*p2, *q, *qprev;
    Dwarf_Debug		dbg;
    Dwarf_Section	sections, sectp;
    int			numsects = 0, has_dwarf_info = 0;
    int 		p_max, i, j, n;

    *nlinenumbers = 0;

    /* p is a bfd_ghist_info list built from Elf info.  It contains function entries 
       with addresses, but has no file names or line numbers. This list is sorted by 
       address.  Next step: crack open the dwarf sections to get file names and line 
       numbers. */

    p_max = *nelements;

    numsects = bfd_count_sections(abfd);
    sections = (Dwarf_Section)bfd_alloc(abfd, sizeof(struct Dwarf_Section) * numsects);

    bfd_map_over_sections(abfd, build_libdwarf_section_list, (PTR)sections);

    /* Check for Dwarf sections. */
    for (i = 0, sectp = sections; i < numsects; i++, sectp++)
    {
	has_dwarf_info = strcmp(".debug_info", sectp->name) == 0;
	if (has_dwarf_info)
	    break;
    }

    if (!has_dwarf_info)
	return p;
 
    /* Absorb Dwarf info */
    dbg = dwarf_init((FILE *) abfd->iostream, DLC_READ, sections, numsects);

    if (dbg == NULL)
	return p;

    /* Embellish ghist info w/file names, line #s, addresses.  We do this by 
       creating a new ghist info list using Dwarf info, and then releasing the initial
       Elf-based ghist info. */

    for (i = 0, n = *nelements; i < n; i++)
    {
	Dwarf_Attribute attr;
	Dwarf_Die cu_die = NULL, func_die = NULL;
	Dwarf_Addr lowpc, highpc;
	Dwarf_Line linebuf, lp;
	int num_lines = 0;

	/* q points to the ith element in p. q is set by assignment because
	   p may be reallocated during _bfd_add_bfd_ghist_info(). */
	q = p + i;

	/* Get the Dwarf compilation-unit and function dies for each function
	   in the table built from the Elf file. The compilation unit die is needed to
	   get the file name in which function is defined.  The function die is needed
	   to get function's (line #, address) pairs. */

	if (cu_die = dwarf_cu_pubdie(dbg, (char *)q->func_name))
	{
	    /*  This is a function w/external linkage. Get function die from
		.debug_pubnames. */

	    func_die = dwarf_pubdie(dbg, (char *)q->func_name);
	}
	else
	{
	    /* This is a static function.  Find "owning" compilation unit, then
	       function die. */

	    /* The following trick gives first cu entry. */
	    cu_die = dwarf_child(dbg, NULL);

	    /* Now, iterate over cu's until owner is found. */
            while (cu_die)
	    {
		lowpc = dwarf_lowpc(cu_die);
		highpc = dwarf_highpc(cu_die);
		if ((Dwarf_Addr)q->address >= lowpc && (Dwarf_Addr)q->address <= highpc)
		{
		    break;
		}
		cu_die = dwarf_siblingof(dbg, cu_die);
	    }

	    if (cu_die == NULL)
		continue;

	    /* Next, look in cu for function die. */
	    func_die = dwarf_child(dbg, cu_die);
	    while (func_die)
	    {
		if (dwarf_tag(func_die) == DW_TAG_subprogram)
		{
		    char *func_name = NULL;

	            attr = dwarf_attr(func_die, DW_AT_name);
		    if (attr)
		        func_name = dwarf_formstring(attr);
		    if (func_name && strcmp(q->func_name, func_name) == 0)	
			break;
	        }
	    	func_die = dwarf_siblingof(dbg, func_die);
	    }
	}

	if (func_die == NULL)
  	    continue;

	/* Get file name which defines function. */
	attr = dwarf_attr(cu_die, DW_AT_name);
	if (attr)
	    q->file_name = dwarf_formstring(attr);

        num_lines = dwarf_srclines(dbg, cu_die, &linebuf);

	lowpc = dwarf_lowpc(func_die);
        highpc = dwarf_highpc(func_die);

	if (num_lines == DLV_NOCOUNT || 
	    lowpc == DLV_BADADDR || highpc == DLV_BADADDR)
	    continue;

	/* Add line info for this function to ghist info */
	for (j = 0, lp = linebuf; j < num_lines; j++, lp++)
	{
	    if ((Dwarf_Addr)lp->address < lowpc)
		continue;
	    if ((Dwarf_Addr)lp->address > highpc)
		break;
            _bfd_add_bfd_ghist_info(&p, nelements, &p_max, lp->address, 
		q->func_name, q->file_name, lp->line);
	}
    }

    qsort(p, *nelements, sizeof(bfd_ghist_info), _bfd_cmp_bfd_ghist_info);

    /* Eliminate duplicate entries in the ghist info.  Do this by creating
       another list w/only unique entries.  */

    /* First, mark duplicate entries */
    qprev = p;
    q = p + 1;
    n = *nelements;

    for (i = 1; i < n; q++, qprev++, i++)
    {
	if (qprev->address == q->address &&
	    strcmp(qprev->func_name, q->func_name) == 0)
	{
	    /* Mark duplicate */
	    qprev->address = 0xffffffff;
	}
    }

    /* Next, write unique entries to new list. */
    q = p; 
    n = p_max = *nelements;
    *nelements = 0;
    p2 = (bfd_ghist_info *)bfd_alloc(abfd, p_max*sizeof(bfd_ghist_info));

    for (i = 0; i < n; q++, i++)
    {
	/* Detect & skip duplicate */
	if (q->address == 0xffffffff)
	    continue;
	(*nlinenumbers)++;
        _bfd_add_bfd_ghist_info(&p2, nelements, &p_max, q->address, 
	    q->func_name, q->file_name, q->line_number);
    }

    bfd_release(abfd, p);
    return p2;
}


main(argc, argv)
    int argc;
    char *argv[];
{
        int	c;

	argc = get_response_file(argc,&argv);

	check_v960( argc, argv );

	program_name = argv[0];

	if (argc < 2) {
		put_ghist_help();
		exit(0);
	}

	/*
	 * Initialize the global switch tracking variables.
	 */
	all_symbols = 0;
	dump_symbols = 0;
	print_hits = 0;
	print_header = 1;
	tally_functions = 0;

	/*
	 * Process arguments
	 */
	while ((c = getopt (argc, argv, "ahdfnsV")) != EOF) {
		switch (c) {

		case 'a':
			all_symbols = 1;
			break;
		case 'd':
			dump_symbols = 1;
			break;
		case 'f':
			tally_functions = 1;
			break;
		case 'n':
			print_hits = 1;
			break;
		case 'h':
			put_ghist_help();
			exit(0);
		case 's':
			print_header = 0;
			break;
                case 'V':
                        /* Print version string and continue. */
                        gnu960_put_version();
                        break;
		default:
			usage();
			break;
		}
	}

	data_file_name = default_data;

	/*
	 * Process the remaining arguments.
	 */
	switch (argc - optind) {

	/*
	 * The form is: ghist960 [-ahdfnsV | v960] application
	 */
	case 1:
 		app_file_name = argv[optind];
		break;

	/*
	 * The form is: ghist960 [-ahdfnsV | v960] application data file,
	 */
	case 2:
		app_file_name = argv[optind];
		data_file_name = argv[optind + 1];
		break;

	/*
	 * Incorrect invocation.
	 */
	default:
		usage();
		break;
	}

#ifdef DOS
#define FOPEN_READ "rb"
#else
#define FOPEN_READ "r"
#endif

	if ((data_desc = fopen(data_file_name, FOPEN_READ)) == NULL) {  
	    fprintf(stderr,"%s: could not open data file '%s'%c\n",
		    program_name, data_file_name, bell);
	    exit(1);
	}

	/*
	 * Print a hello, initialize more globals, then
	 * slurp the application's symbol table. */
	if (print_header) {
		printf("\nGcc 80960 History Profiler %s\n\n", Version);
	}
	if (!(app_bfd = bfd_openr(app_file_name,NULL))) {
	    perror(app_file_name);
	    exit(1);
	}
	if (!(bfd_check_format(app_bfd,bfd_object))) {
	    fprintf(stderr,"Bad file format?: %s\n",app_file_name);
	    exit(1);
	}
	if (!(sym_table=bfd_fetch_ghist_info(app_bfd,
					     &sym_table_length,
					     &total_line_numbers))) {
	    fprintf(stderr,"%s: can not read symtab from application file: %s\n",
		    program_name,app_file_name);
	    fprintf(stderr,"Is it stripped?\n");
	    exit (1);
	}
	if (BFD_ELF_FILE_P(app_bfd) && 
	    !(sym_table=dwarf_fetch_ghist_info(app_bfd, sym_table,
					     &sym_table_length,
					     &total_line_numbers))) {
	    fprintf(stderr,"%s: can not read Dwarf info from application file: %s\n",
		    program_name,app_file_name);
	    fprintf(stderr,"Is it stripped?\n");
	    exit (1);
	}
            
	/* Now, call data_file() to parse the data file
	 * and store the hit-address information in the sym_table.
	 */
	data_file();
	/*
	 * If the "-d" flag was specified the user wants to see the
	 * contents of our internal symbol table. Print the execution
	 * profile results to standard-out via print_profile().
	 */
	if (dump_symbols)
		dump_internal_table();
	print_profile();
	exit(0);
}



/*********************************************************************
 *
 * NAME
 *	dump_internal_table - dump internal symbol table
 *
 * DESCRIPTION
 *	Formats and prints the entire contents of the internal
 *	symbol table.
 *
 * PARAMETERS
 *	None.
 *
 * RETURNS
 *	Void.
 *
 *********************************************************************/

void
dump_internal_table()

{
    int	i;	/* loop control */

    for (i = 0; i < sym_table_length; i++)
	    printf("%d\t0x%x %14s %-10s line #%d, hits = %d\n",
		   i, sym_table[i].address, 
		   sym_table[i].func_name, 
		   sym_table[i].file_name ? sym_table[i].file_name : "(null)",
		   sym_table[i].line_number,
		   sym_table[i].num_hits);
}


/*********************************************************************
 *
 * NAME
 *	data_file - process the runtime data file
 *
 * DESCRIPTION
 *	Read and process the runtime output script file. This function
 *	cycles through the ascii script file, skips the header garbage
 *	and other unneccessary lines, processing valid profile data
 *	lines.
 *
 * PARAMETERS
 *	None.
 *
 * RETURNS
 *	Void.
 *
 *********************************************************************/

void
data_file()

{
    if (print_header) {
	printf("Reading data from '%s'", data_file_name);
	fflush(stdout);
    }

    /*
     * Process all the data file entries. The hit information
     * is stored directly into the internal symbol table structure
     * for later dump.
     */

    read_int(&bucket_size);
    while (!done)
	    next_entry();
    fclose(data_desc);
}

static void
read_int(ip)
    int *ip;
{
    if (fread(ip, sizeof(int), 1, data_desc) < 1) {
	if (feof(data_desc)) {
	    done = 1;
	    if (print_header) {	    
		printf(" Done.\n\n");
	    }
	    return;
	}
	fprintf(stderr,"File read error!%c\n", bell);
	fclose(data_desc);
	exit(1);
    }
    byteswap(ip, 4);
}


/*********************************************************************
 *
 * NAME
 *	next_entry - process the next address/hit data entry
 *
 * DESCRIPTION
 *	This function reads the address/hit pair from the data file
 *	and tries to process it into bucket hit information to be
 *	stored in the internal symbol table. The global "done"
 *	is set 1 if the last script line was encountered and
 *	recognized.
 *
 * PARAMETERS
 *	None.
 *
 * RETURNS
 *	Void.
 *
 *********************************************************************/

void
next_entry()

{
    unsigned int addr;	   /* address of hit */
    unsigned int num_hits; /* number of hits */
    int	sym_index;	   /* symbol index number */

    /*
     * Read the next bucket address and count pair. If EOF is
     * encountered then set the global done flag.
     */
    read_int(&addr);
    if (!done)
	    read_int(&num_hits);
    if (!done) {
	
    /*
     * Found a valid entry. Locate the corresponding symbol and
     * increase its hit count, and also increase total hits.
     */
	sym_index = find_symbol_index(addr);
	if (sym_index != -1) {
	    sym_table[sym_index].num_hits += num_hits;
	    total_hits += num_hits;
	}
	else
		fprintf(stderr,
			"Found entry: addr=0x%x, hits=%d that is not in the application's memory space\n",
			addr,num_hits);
    }
}


/*********************************************************************
 *
 * NAME
 *	byteswap - swap bytes within a string
 *
 * DESCRIPTION
 *	This function is called to compensate for little vs. big
 *	endian problems between the i960 and whatever host. If the
 *	host has a different endian than the i960 the passed byte
 *	string is reordered to reflect the host endian order.
 *
 * PARAMETERS
 *	Pointer to the src/dst string.
 *	Number of bytes in the string.
 *
 * RETURNS
 *	Void.
 *
 *********************************************************************/

void
byteswap(p, n)
    char *p;
    int n;
{
    int	i; 		/* index into string */
    char	c;	/* temp holding place during 2-byte swap */
    static short test = 0x0100;

    /* The data in the data file is guaranteed to be little endian.
       Whether it is running on a be or le host, or it it is a be or le
       target. */

    if (*((char *) &test) == 1) {
	/*
	 * Big-endian host, swap the bytes.
	 */
	for ( n--, i = 0; i < n; i++, n-- ){
	    c = p[i];
	    p[i] = p[n];
	    p[n] = c;
	}
    }
}


/*********************************************************************
 *
 * NAME
 *	find_symbol_index - match a script address to symbol table
 *
 * DESCRIPTION
 *	This function is passed an address taken from the script file
 *	and it tries to match the address with an address in the 
 *	internal symbol table. A match provides the symbol name
 *	corresponding to the address.
 *
 * PARAMETERS
 *	The script address to match.
 *
 * RETURNS
 *	The index of the internal symbol table entry with the
 *	matching address.
 *
 *********************************************************************/

int
find_symbol_index(address)

unsigned int	address;

{
    int	high,low,mid;

    /*
     * Make sure that our address is within the range of addresses
     * we calculated when the internal symbol table was constructed.
     */
    if ((address > sym_table[sym_table_length-1].address) ||
	(address < sym_table[0].address))
	    return -1;

    /* 
     * Binary search through the symbol table for the entry
     * with the corresponding address.
     */
    high = sym_table_length-1;
    low = 0;
    while (high-low > 1) {
	mid = (low + high) / 2;
	/*
	 * Too high.
	 */
	if (address < sym_table[mid].address)
		high = mid - 1;
	/*
	 * Too low.
	 */
	else if (address > sym_table[mid].address)
		low = mid;
	/*
	 * Just right.
	 */
	else
		return mid;
    }

    /*
     * We got as close as possible, return the index of
     * the low search counter.
     */
    return (address >= sym_table[high].address) ? high : low;
}


/*********************************************************************
 *
 * NAME
 *	print_profile - format and print hit data
 *
 * DESCRIPTION
 *	This is the function responsible for formatting and
 *	printing the symbol and hit information.
 *
 * PARAMETERS
 *	None.
 *
 * RETURNS
 *	Void.
 *
 *********************************************************************/

static int cmp_hits(l,r)
    bfd_ghist_info *l,*r;
{
    int x;

    if (x=l->num_hits - r->num_hits)
	    return x;
    else if (x=l->address - r->address)
	    return x;
    else if (x=strcmp(l->file_name,r->file_name))
	    return x;
    else if (x=strcmp(l->func_name,r->func_name))
	    return x;
    else if (x=l->line_number - r->line_number)
	    return x;
    return 0;
    
}

static void
sort_table()
{
    qsort(sym_table,sym_table_length,sizeof(bfd_ghist_info),cmp_hits);
}

void
print_profile()

{
    int	i,j;		/* loop control */
    int	min_hit;	/* minimum hit print count */

    /*
     **  Before sorting, we may wish to tally up the total for
     **  each function.  We are doing this by assuming that each
     **  time the function changes, then the line number for that
     **  symbol is 0
     */
    
    if ( tally_functions ) {
	for ( i = 0; i < sym_table_length; i++ ) {
	    if ( sym_table[i].line_number != 0 )
		    continue;			/* not here */
	    
	    for ( j = i + 1; j < sym_table_length && 
		 sym_table[j].line_number != 0; j++) {
		sym_table[i].num_hits +=
			sym_table[j].num_hits;
		sym_table[j].num_hits = 0;
	    }
	}
    }

    /*
     * The output is ordered by number of hits. If no
     * hits in any buckets then we have a problem.
     */
    sort_table();
    if (total_hits == 0) {
	fprintf(stderr,"Zero total hits found?\n");
	exit(1);
    }

    /*
     * Calculate the minimum number of symbol hits necessary
     * before we'll print. If the user specified -n then print
     * any symbol/line with a hit, else print only those with
     * greater or equal 1.0%.
     */
    if (all_symbols)
	    min_hit = 1;
    else {
	min_hit = (total_hits / 100) - 1;
	if (min_hit < 1)
		min_hit = 1;
    }

    /*
     * If the user so desires print the header information.
     */
    if (print_header) {
	print_bar();
	printf(" Profile of time spent in program '%s'.\n",
	       app_file_name);
	printf(" %d samples taken with bucket size of %d.\n",
	       total_hits, bucket_size);
	printf(" %d line numbers found.\n", total_line_numbers);
	print_bar();
	
	/*
	 * Print the column headers.
	 */
	printf("%% time  ");
	if (print_hits)
		printf("%6s ","# hits");
	printf("%20s ","function name");
	if (total_line_numbers) {
	    printf("%5s ","line");
	}
	printf("%14s  %10s\n","file name","address");
	print_bar();
    }
    
    /*
     * Loop through the internal symbol table printing
     * symbol entries that do have hits.
     */
    for (i = sym_table_length - 1; i >= 0 &&
	 sym_table[i].num_hits >= min_hit; i--) {
	printf(" %5.1f%% ", (double) ((sym_table[i].num_hits * 100.0) / total_hits));
	if (print_hits) {
	    printf("%6d ", sym_table[i].num_hits);
	}
	printf("%20s ", sym_table[i].func_name);
	if (total_line_numbers > 0) {
	    printf("%5d ", sym_table[i].line_number);
	}
	printf("%14s ", sym_table[i].file_name ? sym_table[i].file_name : "?");
	printf(" 0x%08x\n", sym_table[i].address);
    }
    
    if (print_header) {
	print_bar();
    }
}


/*********************************************************************
 *
 * NAME
 *	print_bar - print a header bar
 *
 * DESCRIPTION
 *	Print a line of equal signs of the proper length,
 *	depending on what fields are actually being printed.
 *
 * PARAMETERS
 *	None.
 *
 * RETURNS
 *	Void.
 *
 *********************************************************************/
void
print_bar()
{
    printf("=====================================================");
    if (print_hits)
	    printf("=======");
    if (total_line_numbers)
	    printf("========");
    printf("\n");
}
