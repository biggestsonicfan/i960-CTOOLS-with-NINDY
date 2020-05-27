static char rcsid[] =
	"$Id: gmung960.c,v 1.15 1995/08/29 21:16:49 paulr Exp $";


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

/******************************************************************************
 *
 * Utility to change the text/data load addresses in an executable object file
 * (b.out or coff or elf).
 *
 * This is necessary for a situation such as building a version of the NINDY
 * monitor for Flash memory on the QT960 board:
 *
 *	When NINDY comes up, it will have to copy its data out of flash memory
 *	and into RAM at the addresses where it was linked.  Normally, the
 *	ROMer (grom960) is used to place the data at an address different from
 *	that at which it was linked.  The problem is that the output of grom960
 *	is either hex or raw binary, neither of which are downloadable to NINDY.
 *	So gmung960 is used to directly modify the object file, which *can* be
 *	downloaded to Flash.
 *
 * See the manual page for full details on user invocation.
 *
 * CAUTION:
 *	Once an object file has been "munged", it may not be good for anything
 *	else other than downloading (some GNU/960 tools may not work correctly
 *	on it).
 *
 ******************************************************************************/

#include "sysdep.h"
#include "bfd.h"

#define S_TEXT	0
#define S_DATA	1

static char * progname;	/* Name by which we were invoked	*/

/* The user can specify the text and data sections to be loaded at specific
 * addresses, or at default addresses.  The rules for "default addresses" are:
 *
 *	o if this is the 1st section specified, it goes at address 0.
 *
 *	o if this is the 2nd section specified, it immediately follows the
 *		first section.
 *
 * The following array tracks the specification the user made.  0, 1, or 2
 * entries in the array may be in use, as indicated by the 'sec_cnt' variable.
 */
static struct {
	int type;		/* S_TEXT or S_DATA		*/
	unsigned long begin;	/* Starting address of section	*/
	int addr_valid;		/* False if this is a 'default address' whose
				 *	actual value is yet to be determined
				 *	('begin' is meaningless).
				 */
} sect[2];

static int sec_cnt = 0;

static unsigned long taddr, daddr;
static unsigned long tsize, dsize;


/* Routines in this file
 */
static	void	dump();
static	void	err();
static	sec_ptr	filesecGet();
static	unsigned int numParse();
static	void	perr();
static	char *	optGet();
static	void	overlapChk();
static	void	overflowChk();
static	void	secAdd();
static	void	usage();

main( argc, argv )
    int argc;
    char **argv;
{
	int i;		/* Loop counter					*/
	char *p;	/* Pointer into an argument on the invocation line */
	char *fn = NULL;/* Name of object file to be munged		*/
	unsigned long next; /* Next address available in load space	*/
	sec_ptr tsec;
	sec_ptr dsec;
	bfd *abfd;      /* BFD descriptor of object file */
	int data_explicitly_specified = 0; /* Flag indicating if 'D' was
					      specified explicitly on the
					      command line.  */

	argc = get_response_file(argc,&argv);

	check_v960( argc, argv );
	progname = argv[0];	/* Save name by which we were invoked */

	if ( argc == 1 ){
		usage();
		exit(0);
	}

	/* Guarantee an arg list ending with NULL, by shifting all args left
	 * (overwriting program name) and setting last arg to NULL.
	 */
	for ( i = 0; i < argc; i++ ){
		argv[i] = argv[i+1];
	}
	argv[i] = NULL;


	/* Process arguments
	 */
	for ( ; *argv != NULL; argv++ ){

		if ( (*argv)[0] == '-' 
#ifdef DOS
		     || (*argv)[0] == '/'
#endif
		     ){

			switch ( (*argv)[1] ){

			case 'D':		/* Data section specification */
			        data_explicitly_specified = 1;
				p = optGet(&argv);
				secAdd( S_DATA, p );
				break;
			case 'V':		/* Help request */
				gnu960_put_version();
				break;
			case 'h':		/* Help request */
				usage();
				exit(0);
			case 'T':		/* Text section specification */
				p = optGet(&argv);
				secAdd( S_TEXT, p );
				break;

			default:
				err("Unknown switch: %s",*argv); /* No return*/
			}

		} else {			/* object file to be munged */
			if ( fn != NULL ){
				err( "Input/output file respecified");
			} else {
				fn = *argv;
			}
		}
	}


	/* Open and verify object file
	 */
	if ( fn == NULL ){
		err( "No input file specified" );	/* No return */
	}
	abfd = bfd_openrw(fn,NULL);
	if ( (abfd == NULL) || !bfd_check_format(abfd,bfd_object) ){
		bfd_perror( fn ); /* No return */
		exit(1);
	}

	/* Get info about text and data sections
	 */
	tsec = filesecGet(abfd,".text");
	dsec = filesecGet(abfd,".data");

	/* Can we modify the .text or .data section's load address? */

	if (tsec->paddr_file_offset == BAD_PADDR_FILE_OFFSET)
		err("Can not modify the %s section of %s.\n",".text",fn);

	if (data_explicitly_specified && dsec->paddr_file_offset == BAD_PADDR_FILE_OFFSET)
		err("Can not modify the %s section of %s.\n",".data",fn);

	taddr = bfd_section_vma(abfd,tsec);
	tsize = bfd_section_size(abfd,tsec);
	daddr = bfd_section_vma(abfd,dsec);
	dsize = bfd_section_size(abfd,dsec);

	/* Assign addresses to sections specified with "default" addresses
	 */
	next = 0;
	for ( i=0; i < sec_cnt; i++ ){
		if ( sect[i].addr_valid ){
			next = sect[i].begin;
		} else {
			sect[i].begin = next;
		}

		if ( sect[i].type == S_TEXT ){
			taddr = sect[i].begin;
			next += tsize;
		} else {
			daddr = sect[i].begin;
			next += dsize;
		}
	}

	/* Check for section overflow and overlap
	 */
	overflowChk( "text", taddr, tsize );
	overflowChk( "data", daddr, dsize );
	overlapChk( "text", taddr, tsize, "data", daddr, dsize );
	overlapChk( "data", daddr, dsize, "text", taddr, tsize );

	/* Update file header:
	 */
	if (1) {
	    unsigned long swapped_text_load_address,swapped_data_load_address;

	    /* Place text and data load addresses into abfd's byte order: */
	    bfd_h_put_32(abfd,taddr,&swapped_text_load_address);
	    bfd_h_put_32(abfd,daddr,&swapped_data_load_address);

	    /* Let's write the text load address first. */
	    if (bfd_seek(abfd,tsec->paddr_file_offset,SEEK_SET))
		    err("Can not seek to physical address of .text section?");
	    if (bfd_write(&swapped_text_load_address,1,sizeof(swapped_text_load_address),abfd) !=
		sizeof(swapped_text_load_address))
		    err("File write error writing text load address.");

	    /* Let's write the data load address now. */
	    if (bfd_seek(abfd,dsec->paddr_file_offset,SEEK_SET)) {
		if (data_explicitly_specified)
			err("Can not seek to physical address of .data section?");
	    }
	    else {
		if (bfd_write(&swapped_data_load_address,1,sizeof(swapped_data_load_address),abfd) !=
		    sizeof(swapped_data_load_address))
			err("File write error writing data load address.");
	    }
	}

	bfd_close(abfd);
	exit(0);
}


/******************************************************************************
 * dump:
 *	Print the relevant portions of the object file header(s).
 *
 ******************************************************************************/
static
void
dump()
{
        printf( "text addr = 0x%x\n", taddr );
        printf( "text size = 0x%x\t(%d)\n", tsize, tsize );
        printf( "data addr = 0x%x\n", daddr );
        printf( "data size = 0x%x\t(%d)\n", dsize, dsize );
	printf( "\n" );
}


/******************************************************************************
 * err:
 *	Print an error message, including the name of this program.
 *	Exit with failure code.
 *
 *	NO RETURN TO CALLER!
 *****************************************************************************/
static
void
err( msg, arg1, arg2 )
    char *msg;		/* Text of message, trailing '\n' unnecessary	*/
    char *arg1;		/* Optional arg if "msg" is a printf format string */
    char *arg2;		/* Optional arg if "msg" is a printf format string */
{
	fprintf( stderr, "%s: ", progname );
	fprintf( stderr, msg, arg1, arg2 );
	fprintf( stderr, "\nInput file unchanged\n" );
	usage();
	exit(1);
	/* NO RETURN */
}

/******************************************************************************
 * filesecGet:
 *	Retrieve pointer to BFD section structure.
 *****************************************************************************/
static
sec_ptr
filesecGet(abfd,secname)
    bfd *abfd;		/* Binary file descriptor			*/
    char *secname;	/* Name (".text" or ".data") of desired section	*/
{
	sec_ptr s;

	s = bfd_get_section_by_name(abfd,secname);
	if ( s == NULL ){
		err( "File does not contain %s section!", secname );
	}
	return s;
}


/*****************************************************************************
 * numParse:
 *	Convert ASCII string to a binary number.  Assume number is decimal,
 *	unless it begins with "0x" (hex).
 *****************************************************************************/
static
unsigned int
numParse( p )
    char *p;	/* Pointer to number in ASCII */
{
	char *origp;	/* Save original string pointer here for error msgs */
	int radix;	/* 10 or 16		*/
	unsigned int n;	/* Build up value here	*/

	origp = p;

	if ( (p[0] == '0') && (p[1] == 'x') ){
		radix = 16;
		p += 2;
	} else {
		radix = 10;
	}

	for ( n = 0; *p != '\0'; p++ ){
		n *= radix;
		if ( isdigit(*p) ){
			n += *p - '0';
		} else if ( isxdigit(*p) && (radix==16) ){
			*p = toupper(*p);
			n += *p - 'A' + 10;
		} else {
			err( "Invalid number: '%s'", origp );
			exit(1);
		}
	}
	return n;
}


/******************************************************************************
 * perr:
 *	Print an error message, including the name of this program.
 *	Append the most recent system error.  Exit with failure.
 *
 *	NO RETURN TO CALLER!
 *****************************************************************************/
static
void
perr( msg, arg1, arg2 )
    char *msg;		/* Text of message, trailing '\n' unnecessary	*/
    char *arg1;		/* Optional arg if "msg" is a printf format string */
    char *arg2;		/* Optional arg if "msg" is a printf format string */
{
	fprintf( stderr, "%s: ", progname );
	fprintf( stderr, msg, arg1, arg2 );
	fprintf( stderr, ": " );
	perror( "" );
	exit(1);
	/* NO RETURN */
}


/*****************************************************************************
 * optGet:
 *	Return a pointer to the string following a single-character switch.
 *	The string may be appended onto the character switch ("-Xabc") or it
 *	may be the next argument ("-X abc").  Return NULL if there is no
 *	next arg, or if it's another switch.
 *
 *****************************************************************************/
static
char *
optGet( argvp )
    char ***argvp;	/* Pointer to argv from main */
{
	char *p;	/* String following switch */

	p = **argvp + 2;
	if ( *p == '\0' ){
		(*argvp)++;
		if ( (**argvp == NULL) || !isdigit(**argvp[0]) ){
			(*argvp)--;
			p = NULL;
		} else {
			p = **argvp;
		}
	}
	return p;
}


/*****************************************************************************
 * overflowChk:
 *	Make sure that a section's starting address leaves enough room for the
 *	section contents in a 32-bit address space.
 *****************************************************************************/
static
void
overflowChk( name, start, size )
    char *name;			/* Section name ("text" or "data" )	*/
    unsigned long start;	/* Section starting address		*/
    unsigned long size;		/* Section size	(# bytes)		*/
{
	if ( (size != 0) && (start + size <= start) ){
		dump();
		err( "%s section overflows address space", name ); /*NO RETURN*/
	}
}


/*****************************************************************************
 * overlapChk:
 *	Make sure that a section's starting address will not cause it to
 *	overlap the following section.
 *****************************************************************************/
static
void
overlapChk( name1, start1, size1, name2, start2, size2 )
    char *name1;	  /* Name of section being checked ("text" or "data") */
    unsigned long start1; /* Start address of section			*/
    unsigned long size1;  /* Size (# bytes) section			*/
    char *name2;	  /* Name of "the other" section ("data" or "text") */
    unsigned long start2; /* Start address of "the other" section	*/
    unsigned long size2;  /* Size (# bytes) section			*/
{
	if ( size1 && size2 && (start1 <= start2) && (start1 + size1 > start2) ){
		dump();
		err( "%s section overlaps %s section", name1, name2 );
								/*NO RETURN*/
	}
}

/*****************************************************************************
 * secAdd:
 *	Parse section description seen on invocation line, place information
 *	in next entry in the global section description array.
 *****************************************************************************/
static
void
secAdd( type, p )
    int type;		/* Type of section:  S_DATA or S_TEXT */
    char *p;		/* Pointer to ASCII address		*/
{
	if ((sec_cnt==2)
	||  ((sec_cnt==1) && (sect[0].type==type)) ){
		err( "Can't specify %s section twice",
					type == S_DATA ? "data" : "text" );
	}

	sect[sec_cnt].type = type;
	if ( p ){
		sect[sec_cnt].begin = numParse(p);
		sect[sec_cnt].addr_valid = 1;
	} else {
		sect[sec_cnt].addr_valid = 0;
	}
	sec_cnt++;
}


/*****************************************************************************
 * usage:
 *	Print out usage message.
 *****************************************************************************/

char *utext[] = {
	"", 
	"Modify text load and/or data load address(es) in an object file",
	"", 
	"  -D [addr]:  place .data section of specified file at 'addr'",
	"         -h:  display this help message",
	"         -V:  print version information and continue",
	"      -v960:  print version information and exit",
	"   -T[addr]:  place .text section of specified file at 'addr'",
	"",
	"   The order of -D and -T switches is significant: if not", 
	"   specified, 'addr' defaults to next available address in",
	"   binary image (0 for the first section).",
	"",
	"   'addr' is decimal unless preceded with '0x' (hex).",
	"",
	"See your user's guide for a complete command-line description",
	"",
	NULL
};

static
void
usage()
{
	int i;

	printf( "\nUsage:  %s -{D|h|T|V|v960} object-file\n", progname ); 
	paginator(utext);
}
