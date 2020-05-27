char rcsid[] =
	"$Id: grom960.c,v 1.24 1995/06/23 01:28:49 rdsmithx Exp $";



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
 * Utility to extract text and data sections from b.out files, place them in
 * a specified sequence in a binary image, and generate Intel 32-bit format
 * hex files from the binary image.
 *
 * Each output hex file corresponds to a single ROM device, and can be
 * downloaded directly to a ROM burner.
 *
 * Three parameters of the target ROM can be specified, controlling which bytes
 * in the binary image are written to which hex files, i.e., the way in which
 * the binary image is "split" into ROMs:
 *
 *	ROM length
 *		The number of bytes that will fit into a single ROM.  After a
 *		ROM image is full, a new image (new "row") will be started.
 *
 *	ROM width
 *		The number of bytes that will be written to a single ROM before
 *		output is directed to the next bank ("column") of ROMs.
 *
 *	Number of ROM banks
 *		The number of "columns" of ROMs.
 *
 * E.g., assume:
 *	number of banks = 4
 *	ROM width = 2
 *	ROM length = 64K
 *	image size = 512K
 *	first 16 bytes in image are: 0x00112233445566778899aabbccddeeff
 *
 * then the following hex files would be output, with the first 4 bytes in
 * each bank as indicated:
 *
 *	  BANK 0      BANK 1      BANK 2      BANK 3
 *
 *	 rom00.hex   rom01.hex   rom02.hex   rom03.hex
 *	+--------+  +--------+  +--------+  +--------+
 *	|00118899|  |2233aabb|  |4455ccdd|  |6677eeff|  64K each
 *	+--------+  +--------+  +--------+  +--------+
 *
 *	 rom10.hex   rom11.hex   rom12.hex   rom13.hex
 *	+--------+  +--------+  +--------+  +--------+
 *	|        |  |        |  |        |  |        |  64K each
 *	+--------+  +--------+  +--------+  +--------+
 *
 ******************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#ifdef DOS
#	include <fcntl.h>
#else
#	include <sys/file.h>
#endif
#include <setjmp.h>
#include <ctype.h>


#include "bfd.h"
#include "dio.h"
#ifdef	USG
#	include <fcntl.h>
#if !defined(__HIGHC__) && !defined(WIN95) /* Microsoft C 9.00 (Windows 95) or Metaware C */
#	include <unistd.h>
#endif
#	include "sysv.h"
#else	/* BSD */
#	include "string.h"
#endif

#ifdef DOS
#	include "gnudos.h"
#endif

#ifndef MIN
#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#endif

/*****************************************************************************
 * After a binary single image is created, it is "split" according to the
 * ROM width and the number (N) of banks into N bank images.  If there are
 * more than 1 bank, these images are kept in temporary files.
 *
 * A bank image is tracked with the following structure.
 *****************************************************************************/
typedef struct {
	char *fn;	/* Tmp file name;  NULL the image is in-core (pointed
			 *   at by the variable 'image') rather than in a tmp
			 *   file.
			 */
	FILE *f;	/* Tmp file descriptor (NULL if not opened)	*/
	unsigned len;	/* Number of bytes in image			*/
} BANKIMAGE;

BANKIMAGE *banks;	/* Dynamically allocated array, one entry per bank */


/*****************************************************************************
 * Each input object file is tracked with one of the following:
 *****************************************************************************/
typedef struct infile {
	char *fn;		/* File name			*/
	bfd  *bfdp;		/* Binary file descriptor	*/
	sec_ptr tsec;		/* BFD text section descriptor	*/
	sec_ptr dsec;		/* BFD data section descriptor	*/
	struct infile *next;	/* Next file on linked list	*/
} INFILE;

INFILE *filelist = NULL;	/* Linked list of input files	*/


/*****************************************************************************
 * Each text or data segment to go into the binary image is tracked with one
 * of the following:
 *****************************************************************************/
typedef struct sect {
	int type;		/* S_TEXT, S_DATA, or S_BOTH	*/
	char *fn;		/* File name				*/
	INFILE *file;		/* Pointer to descriptor of file (see above) */
	int addr_valid;		/* TRUE => 'begin' & 'end' are known	*/
	unsigned int begin;	/* Starting address of section in bin image */
	unsigned int end;	/* Ending address of section in binary image */
	struct sect *next;	/* Next section in linked list		*/
	struct sect *prev;	/* Previous section in linked list	*/
} SECT;

#define S_TEXT	0
#define S_DATA	1
#define S_BOTH	2
/* S_BOTH is a temporary place-holder, that will get replaced by S_TEXT
 * and S_DATA sections with the following characteristics:  the order of
 * the text and data sections will be the same as in the input file (i.e.,
 * the one linked at the lower address comes first);  and the gap between
 * the sections will be the same as in the input file.
 */

/* Dummy head of linked list of sections.  (Having a dummy head instead of 
 * a pointer eliminates special-casing when list is empty.)
 */
SECT sechead = { S_TEXT, "DUMMY", NULL, 1, -1, -1, &sechead, &sechead};

/*****************************************************************************
 * Invocation parameters, with default values.
 *****************************************************************************/
char * program_name;	/* Name by which we were invoked		*/

int image_only = 0;	/* TRUE iff "-i" switch (output binary image only) */
char *basename = NULL;	/* non-NULL if "-o" switch (output file base name) */

int addr20 = 0;		/* TRUE iff "-20" switch used (20-bit address format)*/

int verbose = 0;	/* TRUE iff "-v" switch used			*/
int map = 0;		/* TRUE iff "-v" or "-m" switch used		*/
int full = 0;           /* TRUE iff "-f" switch used                    */

int numbanks = 1;	/* Number of banks of ROM	*/
int romwidth = 1;	/* Width of ROM, in bytes	*/
int romlength = 0x10000;/* Length of ROM, in bytes	*/
int check_gen = 0;      /*  16 if gen 16-bit check, 32 if gen 32-bit check */
int check_start = 0;    /* start address of checksum calculation           */
int check_end = 0xffff; /* end address of checksum calculation             */
int check_addr = 0x1000;/* address of checksum store                       */
 
/*****************************************************************************
 * Misc. garbage
 *****************************************************************************/
char *image=NULL;	/* Points to complete binary image	*/

/* Types of Intel hex records */
#define IHEX_DATA	0	/* Data record			*/
#define IHEX_END	1	/* End of file record		*/
#define IHEX_ADDRH20	2	/* Extended address record, 20-bit address  */
#define IHEX_ADDRH	4	/* Extended address record (32-bit address) */
#define IHEX_START	5	/* Start address record		*/

/*****************************************************************************
 * Routines in this utility
 *****************************************************************************/
void	binihex();
void	cvt();
void	err();
INFILE *fileGet();
void	hexAddh();
void	imageSplit();
char *	mycopy();
char *	optGet();
unsigned int	numParse();
void	perr();
void	secAdd();
SECT *	secInsert();
void	secLink();
void	secMap();
char *	tohex();
void	usage();
void	put_grom_help();
FILE *	xfopen();
void	xfwrite();
char *	xmalloc();
int	xopen();
void	xread();
int	xseek();
void	xwrite();
unsigned short crc16();
unsigned long crc32();

main( argc, argv )
    int argc;
    char **argv;
{
	int i;		/* Loop counter					*/
	char *p;	/* Pointer into an argument on the invocation line */
	SECT *sp;	/* Pointer used to traverse section list	*/
	SECT *tmp;	/* Pointer used to manipulate section list	*/
	int size;	/* Section size; later used to track bank image size */
	int image_size;	/* Size (# bytes) of full binary image		*/
	int fd;
	INFILE *fp;	/* Pointer used to traverse file list		*/
	char filename[200]; /* Create name of ROM image (hex) file here	*/
	int seq;	/* Sequence ("row") number of hex file		*/
	FILE *hexfile;	/* Hex output file descriptor			*/

	argc = get_response_file(argc,&argv);
	check_v960( argc, argv );	/* never returns if -v960 */

	program_name = mycopy(argv[0]);	/* Save name we were invoked by */

	if ( argc == 1 ){
		put_grom_help();
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

		if ( !strcmp(*argv,"-20")
#ifdef DOS
		     || !strcmp(*argv, "/20")
#endif
		    ){
			addr20 = 1;

		} else if ( (*argv)[0] == '-' 
#ifdef DOS
		             || (*argv)[0] == '/'
#endif
			   ){

			switch ( (*argv)[1] ){

			case 'B':
				p = optGet(&argv);
				secAdd( S_BOTH, p, *argv );
				break;

			case 'b':
				numbanks = numParse( optGet(&argv) );
				break;

			case 'D':
				/* Add data section to image section list */
				p = optGet(&argv);
				secAdd( S_DATA, p, *argv );
				break;

			case 'h':
				put_grom_help();
				exit(0);

			case 'i':
				if ( (*argv)[2] != '\0' ){
					err("Unknown switch: %s",*argv);
					
							/* No return */
				}
				image_only = 1;
				break;

			case 'l':
				romlength = numParse( optGet(&argv) );
				break;

			case 'm':
				map = 1;
				break;

            case 'f':
                full = 1;
                break;

			case 'o':
				basename = optGet(&argv);
				break;

			case 'T':
				/* Add text section to image section list */
				p = optGet(&argv);
				secAdd( S_TEXT, p, *argv );
				break;

			case 'v':
				if ( (*argv)[2] != '\0' ){
					err("Unknown switch: %s",*argv);
								/* No return */
				}
				map = verbose = 1;
				break;

    		case 'V':
            	gnu960_put_version();
        		break;

			case 'w':
				romwidth = numParse( optGet(&argv) );
				break;

            case 'c': /* crc generation */
                check_gen = numParse( optGet(&argv) );
                if (check_gen != 16 && check_gen != 32){
                                    err("Unsupported checksum requested: %d",
                                        check_gen); /* No return */
								}
                break;

            case 'S': /* CRC start address */
                check_start = numParse( optGet(&argv) );
                break;

            case 'E': /* CRC end address */
                check_end = numParse( optGet(&argv) );
                break;

            case 'A': /* CRC address */
                check_addr = numParse( optGet(&argv) );
                break;
 
			default:
				err("Unknown switch: %s",*argv); /* No return*/
			}

		} else {
			secAdd( S_TEXT, *argv, *argv );
			secAdd( S_DATA, *argv, *argv );
		}
	}

	if ( sechead.next == &sechead ){
		err( "No input files specified" );	/* No return */
	}

	/* Fill in file info and section end addresses for each section.
	 * Expand S_BOTH entries into S_DATA and S_TEXT sections as
	 * described above at the definition of S_BOTH.
	 * Make all section begin addresses "valid": make sections that were
	 * specified without an address follow immediately after whatever
	 * section was specified before them.
	 */
	for ( sp = sechead.next; sp != &sechead; sp = sp->next ){
		INFILE *ifp;

		ifp = sp->file = fileGet( sp->fn );
		if ( !sp->addr_valid ){
			sp->begin = sp->prev->end + 1;
			sp->addr_valid = 1;
		}
		if ( sp->type== S_BOTH ){
			SECT *datasp;
			unsigned long tload;
			unsigned long dload;

			tload = bfd_section_vma( ifp->bfdp, ifp->tsec );
			dload = bfd_section_vma( ifp->bfdp, ifp->dsec );

			datasp = (SECT *) xmalloc( sizeof(SECT) );
			*datasp = *sp;		/* Duplicate entry */
			sp->type = S_TEXT;
			datasp->type = S_DATA;

			if ( tload < dload  ){
				/* Data section comes after text */
				datasp->begin += dload - tload;
				secLink( sp, datasp );
			} else {
				/* Data section comes before text */
				sp->begin += tload - dload;
				secLink( sp->prev, datasp );
				sp = datasp;
			}
		}
		
		if ( sp->type == S_TEXT ){
			size = bfd_section_size( ifp->bfdp, ifp->tsec );
		} else {
			size = bfd_section_size( ifp->bfdp, ifp->dsec );
		}
		sp->end = sp->begin + size - 1;
	}

	/* Now sort section list numerically by section beginning address;
	 * eliminate 0-length sections.  To do the sort, detach the list from
	 * the dummy sechead element and run down it, inserting non-0-length
	 * sections back onto the original list, in numeric order now that
	 * all addresses have been resolved.
	 */
	sp = sechead.next;
	sechead.next = sechead.prev = &sechead;

	while (  sp != &sechead ){
		tmp = sp;
		sp = sp->next;

		if ( tmp->type == S_TEXT ){
			size = bfd_section_size( bfdp, tmp->file->tsec );
		} else {
			size = bfd_section_size( bfdp, tmp->file->dsec );
		}
		if ( size == 0 ){
			free( tmp );
		} else {
			secInsert( tmp );
		}
	}

	/* Check for section overlap
	 */
	for ( sp = sechead.next; sp->next != &sechead; sp = sp->next ){
		if ( sp->end >= sp->next->begin ){
			secMap();
			err( "Sections overlap, no output produced" );
								/* No return */
		}
	}

	if ( verbose ){
		printf( "\noutput file basename = '%s'\n",
					basename == NULL ? "" : basename );
		printf( " number of ROM banks = %d\n", numbanks );
		printf( "  ROM length (bytes) = %d\n", romlength );
		printf( "   ROM width (bytes) = %d\n", romwidth );
	}

	if ( map ){
		secMap();
	}

	/* Allocate storage for and create binary load image.
	 *	Use a background of 0xff bytes, since that's what an erased
	 *	PROM looks like.  Then read each section on the section list
	 *	into the correct location in the image.
	 * If performing CRC, check if the location of the CRC is outside the
	 * file image, if so the allocate at least that size.
	 	 */
	image_size = sechead.prev->end + 1;
	if (check_gen && check_addr>image_size){
		image_size = check_addr + (check_gen==16?2:4);
	}

	image = xmalloc( image_size );
	for ( p = image; p < image + image_size; p++ ){
		*p = 0xff;
	}

	for ( sp = sechead.next; sp != &sechead; sp = sp->next ){
		INFILE *ifp;
		sec_ptr bfd_sec;

		ifp = sp->file;
		bfd_sec = (sp->type == S_TEXT) ? ifp->tsec : ifp->dsec;
		if ( !bfd_get_section_contents( ifp->bfdp,
						bfd_sec,
						image + sp->begin,
						0,
						sp->end - sp->begin + 1) ){
			bfd_perror( ifp->fn );
			exit(1);
		}
	}

	/* Close all input file descriptors */
	for ( fp = filelist; fp; fp = fp->next ){
		if (bfd_close(fp->bfdp) == false) {
			bfd_perror(bfd_get_filename(fp->bfdp));
		}
	}

    /* generate and store checksum, if so requested */

    if (check_gen)
    {
      if (check_gen == 16)
      {
          unsigned short checksum;

		  if (check_end >= image_size)
		  {
			  printf("WARNING: Checksum end-address is out-of-bounds, will use 0x%x\n", image_size - 1);
			  check_end = image_size - 1;
		  }
          checksum = crc16(image+check_start, image+check_end);

/* store as little-endian */

          image[check_addr] = checksum & 0xff;
          image[check_addr+1] = (checksum>>8) & 0xff;
          if (verbose)
          {
          printf("calculated 16-bit CRC from %x to %x, stored checksum = %x -at %x\n", 
				 check_start, check_end, checksum, check_addr);
	       }
      }
      else
      {
          unsigned long checksum;
		  if (check_end >= image_size)
		  {
			  printf("WARNING: Checksum end-address is out-of-bounds, will use 0x%x\n", image_size - 1);
			  check_end = image_size - 1;
		  }
          checksum = crc32(image+check_start, image+check_end);

/* store as little-endian */

          image[check_addr] = checksum & 0xff;
          image[check_addr+1] = (checksum>>8) & 0xff;
          image[check_addr+2] = (checksum>>16) & 0xff;
          image[check_addr+3] = (checksum>>24) & 0xff;

          if (verbose)
          {
          printf("calculated 32-bit CRC from %x to %x, stored checksum = %x -at %x\n", 
				 check_start, check_end, checksum, check_addr);
	      }
	  }
  }
 
	/* Output binary load image, or hex ROM images, as appropriate */

	if ( image_only ){
		if ( basename == NULL ){
			basename = "image";
		}
		fd = xopen( basename, O_BINARY|O_WRONLY|O_CREAT|O_TRUNC );
		xwrite( fd, image, image_size );

	} else {
		if ( basename == NULL ){
			basename = "rom";
		}

		/* Create individual bank images
		 */
		imageSplit( image, image_size );

		/* Convert one bank image at a time into hex files
		 */
		for ( i = 0; i < numbanks; i++ ){

			/* Read bank image from tmp file, if necessary
			 */
			if ( banks[i].fn != NULL ){
				image = xmalloc( banks[i].len );
				fd = xopen( banks[i].fn, O_BINARY|O_RDONLY );
				xread( fd, image, banks[i].len );
				close( fd );
			}

			/* Output as many ROM images as necessary for bank
			 */
			p = image;
			seq = 0;
			for ( size = banks[i].len; size>0; size -= romlength ){
				sprintf( filename, "%s%d%d.hex",
							basename, seq, i );
				hexfile = xfopen( filename, "w+" );
				cvt( hexfile, p, MIN(romlength,size) );
				fclose( hexfile );
				p += romlength;
				seq++;
			}

			/* Eliminate tmp file and free up image memory */
			if ( banks[i].fn != NULL ){
				unlink( banks[i].fn );
				free( image );
			}
		}
	}
	exit(0);
}


/******************************************************************************
 * binihex:
 *	Convert binary to intel hex record, buffer to buffer.
 ******************************************************************************/
void
binihex(ibuf, obuf, addr, len, type)
    char *ibuf;		/* Pointer to binary input			*/
    char *obuf;		/* Put hex output here				*/
    unsigned addr;	/* Load address for hex record			*/
    int len;		/* Number of bytes of binary data		*/
    int type;		/* Hex record type (see IHEX_... defines)	*/
{
	unsigned char c, chksum;

	chksum = 0;
	*obuf++ = ':';				/* start of record char */

	obuf = tohex(obuf, len, 2);		/* byte count */
	chksum += len;

	obuf = tohex(obuf, addr, 4);		/* load address */
	chksum += (addr & 0xff) + ((addr >> 8) & 0xff);

	obuf = tohex(obuf, type, 2);		/* record type */
	chksum += type;

	while ( len-- ){			/* data bytes */
		c = (unsigned char) *ibuf++;
		chksum += c;
		obuf = tohex(obuf, c, 2);
	}
	obuf = tohex(obuf, -chksum, 2);		/* append checksum */
	*obuf++ = '\n';
	*obuf = '\0';				/* terminate string */
}

/*****************************************************************************
 * cvt:
 *	Generate Intel hex from specified binary image, and write to specified
 *	output file.  Generate extended address records as needed.  Terminate
 *	with end-of-file record.  Don't both putting out empty data records,
 *	i.e., those with only 0xff bytes.
 *****************************************************************************/



#define RECSIZE	16

void
cvt( f, buf, len )
    FILE *f;		/* File to which to write the hex		*/
    unsigned char *buf;	/* Binary to be converted and written out	*/
    int len;		/* Number of bytes of binary in buf	 	*/
{
	unsigned addr;	/* Load address for data in buf			*/
	unsigned ext_addr; /* Extended address bits (high-order 16) */
	unsigned addrh;
	char hexbuf[200];
	int cnt;

	int first_data_rec;
			/* Many ROM burners seem not to recognize extended
			 * address records until after they have processed
			 * the first data record.  For this reason, always
			 * output the first data record, even if its
			 * all FF bytes.
			 */

	first_data_rec = 1;
	ext_addr = 0;

	for ( addr=0; len>0; len -= RECSIZE, addr += RECSIZE, buf += RECSIZE ){
		addrh = addr & (~0xffff);
		if ( ext_addr != addrh ){
			ext_addr = addrh;
			hexAddh( f, ext_addr );
		}
		cnt = MIN(len,RECSIZE);

/* check to see if a record is all ones, then it doesn't output the file
   if "-f" switch specified do it anyway */

		if ( full ){
            binihex( buf, hexbuf, addr, cnt, IHEX_DATA );
            fputs( hexbuf, f );
            first_data_rec = 0;
        } else	if ( !empty_rec(buf,cnt) || first_data_rec ){
			binihex( buf, hexbuf, addr, cnt, IHEX_DATA );
			fputs( hexbuf, f );
			first_data_rec = 0;
		}
	}
	fputs( ":00000001FF\n", f );	/* end of file */
}

/******************************************************************************
 * empty_rec:
 *	Return non-zero if entire record is empty (contains nothing but
 *	all-1 bytes).
 *****************************************************************************/
empty_rec( rec, len )
    char *rec;
    int len;
{
	while ( len-- ){
		if ( *rec != '\377' ){
			return 0;
		}
		rec++;
	}
	return 1;
}

/******************************************************************************
 * err:
 *	Print an error message, including the name of this program.
 *	Exit with failure code.
 *
 *	NO RETURN TO CALLER!
 *****************************************************************************/
void
err( msg, arg1, arg2 )
    char *msg;		/* Text of message, trailing '\n' unnecessary	*/
    char *arg1;		/* Optional arg if "msg" is a printf format string */
    char *arg2;		/* Optional arg if "msg" is a printf format string */
{
	fprintf( stderr, "%s: ", program_name );
	fprintf( stderr, msg, arg1, arg2 );
	fprintf( stderr, "\n" );
	usage();
	exit(1);
	/* NO RETURN */
}

/*****************************************************************************
 * fileGet:
 *	Return a pointer to the descriptor of the indicated file:  first check
 *	if it already exists on our linked list;  if not: create it, open the
 *	file, add the descriptor to the list and return a pointer to it.
 *****************************************************************************/
INFILE *
fileGet( fn )
    char *fn;	/* Name of file whose descriptor is desired */
{
	INFILE *fp;	/* descriptor of file */

	for ( fp = filelist; fp; fp = fp->next ){
		if ( !strcmp(fp->fn,fn) ){
			break;
		}
	}

	if ( !fp ){
		fp = (INFILE *) xmalloc( sizeof(INFILE) );
		fp->fn = mycopy(fn);
		fp->bfdp = bfd_openr(fn,(char *)NULL);
		if ( (fp->bfdp == NULL)
		||   (bfd_check_format(fp->bfdp,bfd_object) == false) ){
			bfd_perror(fn);
			exit(1);
		}
		
		fp->tsec = bfd_get_section_by_name( fp->bfdp, ".text" );
		if ( !fp->tsec ){
			err( "File %s has no text section", fp->fn );
		}

		fp->dsec = bfd_get_section_by_name( fp->bfdp, ".data" );
		if ( !fp->dsec ){
			err( "File %s has no data section", fp->fn );
		}

		fp->next = filelist;
		filelist = fp;
	}

	return fp;
}


/*****************************************************************************
 * hexAddh:
 *	Generate an extended address record from bits 31-16 of the passed
 *	address.  Write it to the indicated file.
 *****************************************************************************/
void
hexAddh( f, addr )
    FILE *f;		/* Descriptor for buffered file output	*/
    unsigned addr;	/* Address:  generate record with bits 31-16 */
{
	char buf[2];
	char hexbuf[100];
	int haddr;

	haddr = (addr >> 16) & 0xffff;

	if ( addr20 ){
		/* Output high-order bits of the form 000x as x000 */
		if ( haddr > 0x000f ){
			err("Address overflow: can't use 20-bit address format",
									0, 0 );
		}
		buf[0] = haddr << 4;
		buf[1] = 0;
		binihex( buf, hexbuf, 0, 2, IHEX_ADDRH20 );
	} else {
		buf[0] = haddr >> 8;
		buf[1] = haddr;
		binihex( buf, hexbuf, 0, 2, IHEX_ADDRH );
	}
	fputs( hexbuf, f );
}


/*****************************************************************************
 * imageSplit:
 *	Allocate one BANKIMAGE descriptor for each bank image.
 *
 *	If there's only one bank, leave the full image in-core where it is,
 *	and set up the lone descriptor appropriately.
 *
 *	Otherwise, split the image into separate bank images, writing each
 *	out to a tmp file.
 *****************************************************************************/
void
imageSplit( image, len )
    char *image;	/* Pointer to full binary image		*/
    int len;		/* Number of bytes in full image	*/
{
	int i;		/* Loop counter				*/
	int cnt;	/* Number of bytes to write to tmp file: minimum of
			 *	rom width and number of bytes remaining bytes
			 *	in image.
			 */
	char *p;	/* Pointer into full image		*/

	banks = (BANKIMAGE *) xmalloc( numbanks * sizeof(BANKIMAGE) );
	if ( numbanks == 1 ){
		banks[0].f = NULL;
		banks[0].fn = NULL;
		banks[0].len = len;

	} else {
		/* Open 1 tmp file per bank
		 */
	    for ( i = 0; i < numbanks; i++ ){
		extern char *get_960_tools_temp_file();

		banks[i].fn = get_960_tools_temp_file("GRXXXXXX",xmalloc);
		banks[i].f = xfopen( banks[i].fn, FWRTPBIN );
		banks[i].len = 0;	/* TBD below */
	    }

		/* Circulate through banks, writing the next 'romwidth' bytes
		 * to each one.
		 */
		cnt = romwidth;
		p = image;
		for ( i = 0; len > 0; len -= romwidth ){
			if ( len < romwidth ){
				cnt = len;
			}
			xfwrite( p, cnt, banks[i].f );
			banks[i].len += cnt;
			p += cnt;
			if ( ++i >= numbanks ){
				i = 0;
			}
		}

		/* Close tmp files and free image memory
		 */
		for ( i = 0; i < numbanks; i++ ){
			fclose( banks[i].f );
		}
		free( image );
		image = NULL;
	}
}

/******************************************************************************
 * mycopy:
 *	Allocate space for a copy of a text string and copy it.
 *	Return a pointer to the new copy.
 *
 ******************************************************************************/
char *
mycopy( textP )
    char *textP;	/* Test to be copied	*/
{
	char *copyP;	/* Address of copy	*/

	copyP = xmalloc( strlen(textP) + 1 );
	strcpy( copyP, textP );
	return copyP;
}


/*****************************************************************************
 * numParse:
 *	Convert ASCII string to a binary number.  Assume number is decimal,
 *	unless it begins with "0x" (hex).
 *****************************************************************************/
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
void
perr( msg, arg1, arg2 )
    char *msg;		/* Text of message, trailing '\n' unnecessary	*/
    char *arg1;		/* Optional arg if "msg" is a printf format string */
    char *arg2;		/* Optional arg if "msg" is a printf format string */
{
	fprintf( stderr, "%s: ", program_name );
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
 *	may be the next argument ("-X abc").
 *
 *****************************************************************************/
char *
optGet( argvp )
    char ***argvp;	/* Pointer to argv from main			*/
{
	char s;		/* The switch (the character following "-" ) */
	char *p;	/* String following the switch		*/

	s = (**argvp)[1];
	p = **argvp + 2;
	if ( *p == '\0' ){
		(*argvp)++;
		if ( (**argvp == NULL) || (**argvp[0] == '-')
#ifdef DOS
                                       || (**argvp[0] == '/')
#endif
		         ){
			err( "Missing argument after -%c switch", s );
								/* No return */
		}
		p = **argvp;
	}
	return p;
}


/*****************************************************************************
 * secAdd:
 *	Convert section description, as seen on invocation line, to an internal
 *	section descriptor, and link on to end of section list.
 *****************************************************************************/
void
secAdd( type, spec, arg )
    int type;		/* Type of section:  S_DATA, S_TEXT, or S_BOTH */
    char *spec;		/* Section specification, in form "filename[,addr]" */
    char *arg;		/* Full specification from invocation line, including
			 *     switch (if any); e.g., "-Tfilename[,addr]"
			 */
{
	SECT *newsp;	/* New section descriptor			*/
	char *commap;	/* Pointer to "," in the section specification	*/
	unsigned int addr;	/* Address, from section specification	*/

	if ( *spec == ',' ){
		err("Missing filename in '%s'", arg);	/* No return */
	}

	commap = strchr(spec,',');
	if ( commap != NULL ){
		/* An address should have been specified */
		if ( commap[1] == '\0' ){
			err("Missing address in '%s'", arg);	/* No return */
		}
		*commap = '\0';		/* '\0'-terminate file name */
		addr = numParse( commap+1 );
	}

	newsp = (SECT *) xmalloc( sizeof(SECT) );
	newsp->type = type;
	newsp->fn = spec;
	newsp->addr_valid = (commap != NULL);
	newsp->begin = addr;
	newsp->end = 0;		/* TBD */

	secLink( sechead.prev, newsp );
}


/*****************************************************************************
 * secInsert:
 *	Insert designated section descriptor into the section list
 *	in ascending numeric order by beginning address.
 *****************************************************************************/
SECT *
secInsert( sp )
    SECT *sp;	/* Section descriptor to be inserted */
{
	SECT *listp;

	listp = &sechead;
	while ((listp->next != &sechead) && (listp->next->begin < sp->begin)){
		listp = listp->next;
	}
	secLink( listp, sp );
}


/*****************************************************************************
 * secLink:
 *	Link designated section descriptor into linked list
 *	after designated list element.
 *****************************************************************************/
void
secLink( after, sp )
    SECT *after;	/* Element to link after */
    SECT *sp;		/* New descriptor to be linked in */
{
	sp->prev = after;
	sp->next = after->next;
	if ( after->next ){
		after->next->prev = sp;
	}
	after->next = sp;
}

/*****************************************************************************
 * secMap:
 *	Print map of sections in binary image to stdout.
 *****************************************************************************/
void
secMap()
{
	SECT *sp;
	SECT *next;
	SECT *prev;
	char *type;
	char *overlap;
	unsigned int next_addr;

	next_addr = 0;
	printf( "\nROM map:\n" );
	printf( "\tbegin addr | end addr   | sect  | file   \n" );
	printf( "\t-----------+------------+-------+------------\n" );

	for ( sp = sechead.next; sp != &sechead; sp = sp->next ){
		if ( sp->begin > next_addr + 1 ){
			printf("\t0x%08x | 0x%08x |   -   | -\n",
						next_addr, sp->begin-1 );
		}
		next_addr = sp->end + 1;
		type = (sp->type == S_DATA) ? ".data" : ".text";
		if ( sp->addr_valid ){
			prev = sp->prev;
			next = sp->next;
			if ( ((prev != &sechead) && (sp->begin <= prev->end))
			||   ((next != &sechead) && (sp->end >= next->begin))){
				overlap = "OVERLAP";
			} else {
				overlap = "";
			}
			printf("%s\t0x%08x | 0x%08x",
						overlap, sp->begin, sp->end );
		} else {
			printf("\t  ???????? |   ????????" );
		}
		printf( " | %s | %s\n", type, sp->fn );
	}
}


/******************************************************************************
 * tohex:
 *	Convert binary to ascii hex.
 *
 *	Return updated output buffer pointer (where next output character
 *	should go).
 *****************************************************************************/
char *
tohex(obuf, n, cnt)
    char *obuf;		/* output buffer		*/
    unsigned n;		/* number to be converted	*/
    unsigned cnt;	/* nibble count (convert this many low-order nibbles */
{
	static char hextab[] = "0123456789ABCDEF";
	char  *retval;

	retval = obuf + cnt;
	for (obuf += cnt - 1; cnt > 0; cnt--, obuf-- ){
		*obuf = hextab[ n & 0xf ];
		n >>= 4;
	}
	return retval;
}



/*****************************************************************************
 * put_grom_help:
 *	Print out help message.
 *****************************************************************************/

char *utext[] = {
	"",
	"GNU/960 ROM image generator",
	"",
	"Object-file specification:",
	"  filename[,addr]",
	"           Place .text section of file at <addr>, relative to start",
	"           of load image, with .data section immediately after.",
	"  -B filename[,addr]",
	"           Place both .text and .data section of file at <addr>,",
	"           preserving their order and the padding between them.",
	"  -D filename[,addr]",
	"           Place .data section ONLY of file at <addr>.",
	"  -T filename[,addr]",
	"           Place .text section ONLY of file at <addr>.",
	"",
	"  addr defaults to next available address in binary image (0 for 1st file).",
	"",
	"Legal options:",
	"",
	"  -b N:    Generate N banks (columns) of ROMs [default: 1].",
	"  -h:      Display this help message.",
	"  -i:      Output a single binary image file only [default name: 'image'].",
	"  -l N:    Length of a ROM is N bytes [default: 0x10000 (64K)].",
	"  -m:      Produce a map (on stdout) of the binary image.",
	"  -o name: Set name of binary image file, or basename of ROM hex file(s)",
	"            [Defaults:  binary image 'image',  rom basename 'rom'].",
	"            ROM file(s) are named 'nameXY.hex', where each output file",
	"            corresponds to a ROM device, X is the ROM sequence number",
	"            (row), and Y is the ROM bank number (column)",
	"  -v:      '-m' plus summarize ROM configuration settings.",
	" -v960:    Print the version number and exit.",
	"  -V:      Print the version number and continue.",
	"  -w N:    Width of a ROM is N bytes [default: 1].",
	"  -20:     Generate extended address records (if any are needed) in 20-bit ",
	"           format (e.g., as used by the 8086).  The default is to",
	"           generate 32-bit format records.",
	"  -c N     Generate a 16 bit or 32 bit CRC [N = 16 or 32].",
	"  -S N     set CRC start address to N [default: 0x0].",
	"  -E N     set CRC end address to N [default: 0xffff].",
	"  -A N     set CRC storage address to N [default: 0x1000].",
	"  -f       Dump full image (do not skip all ones records).",
	"",
	"  All numeric arguments (N) are decimal unless preceded with '0x' (hex).",
	"",
	"See your user's guide for a complete command-line description",
	"",
	NULL
	};

void
put_grom_help()
{
	int i;

	printf( "\nUsage:  %s [options ...] object-file ...\n", program_name );
	paginator( utext );
}

/*****************************************************************************
 * usage:
 *	Print out usage message.
 *****************************************************************************/
void
usage()
{
	int i;

	printf( "\nUsage:  %s [options ...] object-file ...\n", program_name );
	printf( "Use -h option for help\n\n");
}

/******************************************************************************
 * xfopen:
 *	Open file for buffered I/O, with error checking.
 *	Return stream pointer.
 ******************************************************************************/
FILE *
xfopen( fn, mode )
    char *fn;	/* Name of file			*/
    char *mode;	/* Any valid "fopen" type	*/
{
	FILE *f;	/* stream pointer	*/

	f = fopen( fn, mode );
	if ( f ==  NULL ){
		perr( "Can't open file %s", fn );	/* NO RETURN */
	}
	return f;
}


/******************************************************************************
 * xfwrite:
 *	Buffered binary write to file, with error checking.
 ******************************************************************************/
void
xfwrite( bufp, n, f )
    char *bufp;	/* Pointer to output buffer	*/
    int n;	/* Size of output buffer	*/
    FILE *f;	/* Stream pointer		*/
{
	if ( fwrite( bufp, 1, n, f ) != n ){
		perr( "File write failed" );	/* NO RETURN */
	}
}


/******************************************************************************
 * xmalloc:
 *	Perform memory allocation with error checking.  On error, issue message
 *	and die.
 *
 *****************************************************************************/
char *
xmalloc( len )
    int len;	/* Allocate this many bytes	*/
{
	char *p;	/* Pointer to allocated memory */
#ifndef DOS
	extern char *malloc();
#endif

	p = malloc( len );
	if ( p == NULL ){
		perr( "malloc failed" );	/* NO RETURN */
	}
	return p;
}

/******************************************************************************
 * xopen:
 *	Open file for I/O, with error checking.
 *	Return file descriptor.
 ******************************************************************************/
int
xopen( fn, mode )
    char *fn;	/* Name of file			*/
    int mode;	/* Mode in which to open it	*/
{
	int fd;	/* File descriptor	*/

	fd = open( fn, mode, 0777 );
	if ( fd < 0 ){
		perr( "Can't open file %s", fn );	/* NO RETURN */
	}
	return fd;
}


/******************************************************************************
 * xread:
 *	Read from file, with error checking.
 ******************************************************************************/
void
xread( fd, bufp, n )
    int fd;	/* File descriptor		*/
    char *bufp;	/* Pointer to input buffer	*/
    int n;	/* Size of input buffer		*/
{
	if ( read( fd, bufp, n ) != n ){
		perr( "File read failed" );	/* NO RETURN */
	}
}


/******************************************************************************
 * xseek:
 *	Seek to location relative to beginning of file, with error checking.
 *	Return position.
 ******************************************************************************/
int
xseek( fd, offset )
    int fd;	/* File descriptor					*/
    int offset;	/* Offset into file at which to position I/O pointer	*/
{
	int pos;

	pos = lseek( fd, offset, L_SET );

 	if ( pos == -1 ){
		perr( "Seek failed" );	/* NO RETURN */
	}
	return pos;
}

/******************************************************************************
 * xwrite:
 *	Write to file, with error checking.
 ******************************************************************************/
void
xwrite( fd, bufp, n )
    int fd;	/* File descriptor		*/
    char *bufp;	/* Pointer to output buffer	*/
    int n;	/* Size of output buffer	*/
{
	if ( write( fd, bufp, n ) != n ){
		perr( "File write failed" );	/* NO RETURN */
	}
}

/****************************************************************************
  crc16:

  This function computes crc16 of a given block of a contiguous
  block of memory. The function needs 2 parameters:
  - Beginning address
  - End address
  Function returns the final 16bit crc
 ****************************************************************************/

unsigned short crc16( begin_address, end_address )
unsigned char *begin_address, *end_address;
{
   static unsigned short crc16_tab[256] =
   {
   0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
   0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
   0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
   0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
   0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
   0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
   0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
   0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
   0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
   0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
   0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
   0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
   0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
   0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
   0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
   0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
   0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
   0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
   0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
   0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
   0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
   0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
   0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
   0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
   0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
   0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
   0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
   0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
   0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
   0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
   0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
   0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040
   };



   register unsigned char* pointer;
   register unsigned int crc;

   crc=0;
   for (pointer = begin_address; pointer<=end_address; pointer++)
   {
       crc = (crc >> 8) ^ crc16_tab[(crc ^ *(pointer))&(unsigned short)0xFF];
   }
   return (crc);
}

unsigned long crc32(begin_address, end_address)
unsigned char *begin_address, *end_address;
{
    static unsigned long xtab[256]=
    {
    0x00000000,0x77073096,0xee0e612c,0x990951ba,
    0x076dc419,0x706af48f,0xe963a535,0x9e6495a3,
    0x0edb8832,0x79dcb8a4,0xe0d5e91e,0x97d2d988,
    0x09b64c2b,0x7eb17cbd,0xe7b82d07,0x90bf1d91,
    0x1db71064,0x6ab020f2,0xf3b97148,0x84be41de,
    0x1adad47d,0x6ddde4eb,0xf4d4b551,0x83d385c7,
    0x136c9856,0x646ba8c0,0xfd62f97a,0x8a65c9ec,
    0x14015c4f,0x63066cd9,0xfa0f3d63,0x8d080df5,
    0x3b6e20c8,0x4c69105e,0xd56041e4,0xa2677172,
    0x3c03e4d1,0x4b04d447,0xd20d85fd,0xa50ab56b,
    0x35b5a8fa,0x42b2986c,0xdbbbc9d6,0xacbcf940,
    0x32d86ce3,0x45df5c75,0xdcd60dcf,0xabd13d59,
    0x26d930ac,0x51de003a,0xc8d75180,0xbfd06116,
    0x21b4f4b5,0x56b3c423,0xcfba9599,0xb8bda50f,
    0x2802b89e,0x5f058808,0xc60cd9b2,0xb10be924,
    0x2f6f7c87,0x58684c11,0xc1611dab,0xb6662d3d,
    0x76dc4190,0x01db7106,0x98d220bc,0xefd5102a,
    0x71b18589,0x06b6b51f,0x9fbfe4a5,0xe8b8d433,
    0x7807c9a2,0x0f00f934,0x9609a88e,0xe10e9818,
    0x7f6a0dbb,0x086d3d2d,0x91646c97,0xe6635c01,
    0x6b6b51f4,0x1c6c6162,0x856530d8,0xf262004e,
    0x6c0695ed,0x1b01a57b,0x8208f4c1,0xf50fc457,
    0x65b0d9c6,0x12b7e950,0x8bbeb8ea,0xfcb9887c,
    0x62dd1ddf,0x15da2d49,0x8cd37cf3,0xfbd44c65,
    0x4db26158,0x3ab551ce,0xa3bc0074,0xd4bb30e2,
    0x4adfa541,0x3dd895d7,0xa4d1c46d,0xd3d6f4fb,
    0x4369e96a,0x346ed9fc,0xad678846,0xda60b8d0,
    0x44042d73,0x33031de5,0xaa0a4c5f,0xdd0d7cc9,
    0x5005713c,0x270241aa,0xbe0b1010,0xc90c2086,
    0x5768b525,0x206f85b3,0xb966d409,0xce61e49f,
    0x5edef90e,0x29d9c998,0xb0d09822,0xc7d7a8b4,
    0x59b33d17,0x2eb40d81,0xb7bd5c3b,0xc0ba6cad,
    0xedb88320,0x9abfb3b6,0x03b6e20c,0x74b1d29a,
    0xead54739,0x9dd277af,0x04db2615,0x73dc1683,
    0xe3630b12,0x94643b84,0x0d6d6a3e,0x7a6a5aa8,
    0xe40ecf0b,0x9309ff9d,0x0a00ae27,0x7d079eb1,
    0xf00f9344,0x8708a3d2,0x1e01f268,0x6906c2fe,
    0xf762575d,0x806567cb,0x196c3671,0x6e6b06e7,
    0xfed41b76,0x89d32be0,0x10da7a5a,0x67dd4acc,
    0xf9b9df6f,0x8ebeeff9,0x17b7be43,0x60b08ed5,
    0xd6d6a3e8,0xa1d1937e,0x38d8c2c4,0x4fdff252,
    0xd1bb67f1,0xa6bc5767,0x3fb506dd,0x48b2364b,
    0xd80d2bda,0xaf0a1b4c,0x36034af6,0x41047a60,
    0xdf60efc3,0xa867df55,0x316e8eef,0x4669be79,
    0xcb61b38c,0xbc66831a,0x256fd2a0,0x5268e236,
    0xcc0c7795,0xbb0b4703,0x220216b9,0x5505262f,
    0xc5ba3bbe,0xb2bd0b28,0x2bb45a92,0x5cb36a04,
    0xc2d7ffa7,0xb5d0cf31,0x2cd99e8b,0x5bdeae1d,
    0x9b64c2b0,0xec63f226,0x756aa39c,0x026d930a,
    0x9c0906a9,0xeb0e363f,0x72076785,0x05005713,
    0x95bf4a82,0xe2b87a14,0x7bb12bae,0x0cb61b38,
    0x92d28e9b,0xe5d5be0d,0x7cdcefb7,0x0bdbdf21,
    0x86d3d2d4,0xf1d4e242,0x68ddb3f8,0x1fda836e,
    0x81be16cd,0xf6b9265b,0x6fb077e1,0x18b74777,
    0x88085ae6,0xff0f6a70,0x66063bca,0x11010b5c,
    0x8f659eff,0xf862ae69,0x616bffd3,0x166ccf45,
    0xa00ae278,0xd70dd2ee,0x4e048354,0x3903b3c2,
    0xa7672661,0xd06016f7,0x4969474d,0x3e6e77db,
    0xaed16a4a,0xd9d65adc,0x40df0b66,0x37d83bf0,
    0xa9bcae53,0xdebb9ec5,0x47b2cf7f,0x30b5ffe9,
    0xbdbdf21c,0xcabac28a,0x53b39330,0x24b4a3a6,
    0xbad03605,0xcdd70693,0x54de5729,0x23d967bf,
    0xb3667a2e,0xc4614ab8,0x5d681b02,0x2a6f2b94,
    0xb40bbe37,0xc30c8ea1,0x5a05df1b,0x2d02ef8d,
    };

    register unsigned char *pointer;
    register unsigned long crc = 0x0;

    for(pointer=begin_address;pointer<=end_address;pointer++)
    {
    crc=(crc>>8)^xtab[(crc^*(pointer))&0xFF];
    }
    return crc;
}


