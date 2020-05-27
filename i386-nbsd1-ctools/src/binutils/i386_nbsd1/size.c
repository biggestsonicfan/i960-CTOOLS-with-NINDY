/*
 * Copyright (C) 1991 Free Software Foundation, Inc.
 *
 * size.c is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * size.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with size.c.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* $Id: size.c,v 1.30 1995/11/21 21:16:18 paulr Exp $ */

#include "sysdep.h"
#include "bfd.h"
#include "getopt.h"

extern char *program_name;

PROTO(static void, display_file, (char *filename));
PROTO(static void, print_sysv_format, (bfd *file));
PROTO(static unsigned long, get_common_size, (bfd *abfd));

typedef enum number_base { unknown, decimal, octal, hex } base_type;
base_type radix = unknown;

int seen_p_option = 0; /* -p seen (no headers */
int seen_c_option = 0; /* -c print size of COMMON */

/* program options */
/* ELIMINATE THIS COMPLETELY SOMEDAY */
static struct option long_options[] = { { 0,0,0,0 } };

static void
usage ()
{
	printf ("\nUsage: %s [-doxVhpc] [-v960] files ...\n\n", program_name);
	printf ("Tell size requirements of an object file\n");
	puts( "\t-d    -- output in decimal." );
	puts( "\t-o    -- output in octal." );
	puts( "\t-x    -- output in hex." );
	puts( "\t-c    -- display total size of COMMON symbols");
	puts( "\t-p    -- do not display headers.");
	puts( "\t-h    -- this help message" );
	puts( "\t-V    -- display version and perform size calculation." );
	puts( "\t-v960 -- display version and do nothing else." );
        puts( "" );
        puts( "See your user's guide for a complete comand-line description" );
        puts( "" );
}


int
main (argc, argv)
int argc;
char **argv;
{
	int c;				/* sez which option char */
	int i;
	extern int optind;		/* steps thru options */

	/* Check the command line for a resonse file, and handle it if found.*/
	argc = get_response_file(argc,&argv);
	program_name = *argv;
	check_v960( argc, argv );

	while ((c=getopt_long(argc,argv,"Vdoxhpc",long_options,&i)) != EOF){
		switch(c) {
	        case 'V': /* print version but don't exit */
			gnu960_put_version();
			break;
		case 'd': radix = decimal;	break;
		case 'x': radix = hex;		break;
		case 'o': radix = octal;	break;
		case 'p': seen_p_option = 1; break;
		case 'c': seen_c_option = 1; break;
		case 'h': usage();	exit(0);
		default:  usage();	exit(1);
		}
	}

	if (optind == argc) {
		display_file ("a.out");
	} else {
		for (; optind < argc;) {
			display_file(argv[optind++]);
		}
	}
	return 0;
}


static void
display_bfd (abfd)
    bfd *abfd;
{
	if (bfd_check_format(abfd, bfd_archive)){
		; /* do nothing */

	} else if (bfd_check_format(abfd, bfd_object)) {
		print_sysv_format(abfd);
		printf("\n");

	} else {
		printf("Unknown file format: %s\n", bfd_get_filename(abfd));
		exit(1);
	}
}

static void
display_file(filename)
    char *filename;
{
	bfd *file;
	bfd *arfile = (bfd *) 0;

	file = bfd_openr (filename, NULL);
	if (file == NULL) {
		bfd_perror (filename);
		exit(1);
	}

	if (!bfd_check_format(file, bfd_archive)) {
		display_bfd (file);
	} else {
		for(;;) {
			bfd_error = no_error;
			arfile = bfd_openr_next_archived_file (file, arfile);
			if (arfile == NULL) {
				if (bfd_error != no_more_archived_files) {
					bfd_perror (bfd_get_filename (file));
				}
				exit(1);
			}
			display_bfd (arfile);
			/* Don't close the archive elements;
			 * we need them for next_archive
			 */
		}
	}
	bfd_close (file);
}


static void
rprint_number(width, num, radix)
    int width, num;
    base_type radix;
{
	char *format;

	switch (radix){
	case decimal:	format = "  %*u  "; break;
	case octal:	format = " 0%0*o  "; break;
	case hex:	format = "0x%0*x  "; break;
	}
	printf(format, width, num);
}


static int svi_total = 0;

static void
sysv_internal_printer(file, sec)
    bfd *file;
    sec_ptr sec;
{
	int size = bfd_section_size (file, sec);

	svi_total += size;
	printf ("%-20s  ", bfd_section_name(file, sec));
	rprint_number (8, size, radix == unknown ? decimal : radix);
	if (BFD_BOUT_FILE_P(file) && (!strcmp(".bss",bfd_section_name(file, sec)))) 
	   printf("%10s","__Bbss");
     	else		
	   rprint_number (8, bfd_section_pma(file, sec), radix == unknown ? hex :
		       radix);
	printf ("\n");
}

static void
print_sysv_format(file)
    bfd *file;
{
	svi_total = 0;
	if (!seen_p_option) {
		printf ("%s  ", bfd_get_filename (file));
		if (file->my_archive) {
			printf (" (ex %s)", file->my_archive->filename);
		}
		printf(":\n%-20s  %10s  %10s\n\n","Section","Size","Address"); 
	}
	bfd_map_over_sections (file, sysv_internal_printer, NULL);

	if (seen_c_option) {
		unsigned long comm_size = get_common_size(file);
		printf("COMMON                ");
		rprint_number(8, comm_size, radix == unknown ? decimal :radix);
		printf("\n");
		svi_total += comm_size;
	}
		
	if (!seen_p_option) {
		printf("\nTotal                 ");
		rprint_number(8, svi_total, radix == unknown ? decimal : radix);
		printf("\n");  
	}
}

/*
 * Return the biggest of the "common" symbols
 */
static unsigned long get_common_size(abfd)
bfd *abfd;
{
    unsigned int storage;
    asymbol **syms;
    unsigned int symcount = 0;
	unsigned int sym_counter;
	unsigned long common_size = 0;

#define i960_align(this, boundary)  ((( (this) + ((boundary) -1)) & (~((boundary)-1))))

    if ((bfd_get_file_flags (abfd) & HAS_SYMS)) {
		storage = get_symtab_upper_bound (abfd);
		if (storage != 0) 	{
			syms = (asymbol **) xmalloc (storage);
			symcount = bfd_canonicalize_symtab (abfd, syms);

			if (symcount != 0){
				for (sym_counter = 0; sym_counter < symcount ; sym_counter++) {
					flagword flags = (syms[sym_counter])->flags;
					if (flags & BSF_FORT_COMM) {
						unsigned align;
						unsigned value = (syms[sym_counter])->value;
						/* have to follow linker's alignment rules */
						switch (value) {
						case 0:
						case 1:
						case 2:
						case 3:
						case 4:
							align = 4;
							break;
						case 5:
						case 6:
						case 7:
						case 8:
							align = 8;
							break;
						default:
							align = 16;
							break;
						}
						common_size += i960_align(value,align);
					}
				} 
			} 
			free(syms);
		} 
	} 
	return common_size;
}
