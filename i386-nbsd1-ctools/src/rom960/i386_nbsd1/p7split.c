
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

/*
 * p7split.c -- munges up memory images prior to actual downloading.
 */
#include <stdio.h>
#include <ctype.h>
#include <setjmp.h>
#include "err_msg.h"
#include "rom960.h"
#include "portable.h"

#if defined(__STDC__) || defined(WIN95)
#include <errno.h>	/* needed to work around MWC system() bug */
#include <stdlib.h>
#include <string.h>
#endif

#if !defined(MSDOS) || defined(MWC) || defined(CBLD)
#define	myalloc(a)	malloc(a)
#define	MAXDRW		0x7FFFFFFF

# else
#include <malloc.h>
#define	free(a)		hfree(a)
#define	myalloc(a)	halloc(a,1)

#define	MAXDRW		32767
# endif

#define DEFAULT_I960LIB "/lib"
#define DEFAULT_I960BASE "/usr/local/i960"

#define STRSIZ 256
#define MAXBITS 64
#define P7ADDRSIZ 32

/* These are poorly defined, need more parentheses.  (mlb) */
#if !defined(min)
#define min(x,y)	(x<y ? x : y)
#endif

#if !defined(max)
#define max(x,y)	(x>y ? x : y)
#endif


# if !defined(MSDOS) || defined(MWC) || defined(CBLD)
typedef char * char_p;
# else
typedef char huge * char_p;
# endif


char *p7base;
char *libdir;
char path_tmp[STRSIZ];
char configfile[STRSIZ];
static char options[] = "Vh"; 

jmp_buf parse_err;	/* environment used to long jump */
			/* in case of a parsing error */

char **g_argv;
int g_argc;		/* global versions of main's args */

int line;		/* line currently processing */
int interactive;	/* set to 1 if in interactive mode */

extern char * stralloc();
extern usage();

#if !defined(MSDOS) && !defined(__STDC__) && !defined(POSIX) && !defined(WIN95)
extern char * strcpy();
extern char * strcat();
extern char * malloc();
char_p fill_image();
int write_image();
# else
char_p fill_image(FILE *,long,long *);
int write_image(char *,FILE *,long);
# endif
extern char * getenv();
extern FILE * get_bin_file_w();
extern FILE * get_bin_file_r();
extern FILE * get_bin_file_rname();
extern FILE * get_bin_file_rw();
extern char * get_file_name();
extern long get_int();


int debug=0;

/* MSDOS, int to long */
long memlen;
int memwidth;
long romlen;
int romwidth,romcount;
unsigned long bytes_in_rom;
char    flagseen [128]; /* Command-line options; 1 = "saw this option" */

char *
stralloc(n)
        int n;
{
        register char *s;

        s = NULL;
        s = malloc(n+1);
        if (s == NULL) {
		/* the following line was commented out and the ones following
		added during update to common error handling 9/29/89 --
		robertat
                error("out of space", (char *) NULL, (char *) NULL);
		*/
		{
		error_out(ROMNAME,ALLO_MEM,0);
		longjmp(parse_err,1);
		}
        }
        return(s);
}


char *
copy(s)
        register char *s;
{
        register char *ns;

        ns = stralloc(strlen(s));
        return(strcpy(ns, s));
}

/* The next two routines should never be called after update to common
error handling 
error(s, x, y)
        char *s, *x, *y;
{
        fprintf(stderr  , s, x, y);
        putc('\n',  stderr);
	exit(EXIT_FAIL);
}

p_error(s, x, y)
        char *s, *x, *y;
{
	fprintf(stderr , "at line %d:\n",line);
        fprintf(stderr  , s, x, y);
        putc('\n',  stderr);
	longjmp(parse_err,1);
}
*/

char *
xmalloc( len )
    int len;
{
        char *p;
#if !defined( DOS ) && !defined( RS6000_SYS )
        extern char * malloc();
#endif
 
        p = malloc( len );
        if ( p == NULL )
                printf( "xmalloc: Unsuccessful memory allocation\n" );
 
        return p;
}

static void
setpaths()
{
        /* Set up location of directories. This routine both
           sets up the location of the directory. and checks validity */

        if ((p7base = getenv("I960BASE")) == NULL)
                p7base = DEFAULT_I960BASE;

        if ((libdir = getenv("I960LIB")) == NULL) {
                strcpy(path_tmp,p7base);
                strcat(path_tmp,DEFAULT_I960LIB);
                libdir = copy(path_tmp);
        }

}

 static char *utext[] = {
                "",
                "ROM image builder",
                "",
                "Options:",
                "   -h: display this help message",
                "   -V: print version information and continue",
                "-v960: print version information and exit",
                "",
                "Commands:",
                "   checksum image <start-addr><end-addr> <16|32> <checksum-addr>",
                "       [computes a 16 or 32 bit crc checksum over the address range",
                "        specified and stores it in the <checksum-addr>]",
                "   ihex <image to translate><output intel hex file>[mode16|mode32]",
                "       [translates a binary ROM image to intel hex format suitable",
                "        for downloading to an intelligent PROM programmer]",
                "   map <object file>",
                "       [produces a listing to stdout of the sections in the COFF/b.out",
                "        file in address order with their address and sizes in hex]",
                "   mkimage <object-input><image-output> [section_1 section_2 ... section_n]",
                "       [translates a COFF executable file into a memory image",
                "        containing an image of the program as it would appear when",
                "        downloaded.  By default, all code and data sections become",
		"        part of the image.  You can override this by explicitly",
		"        specifying the section names: section_1 section_2 ... section_n]",
                "   mkfill <object-input><image-output><fill-character>",
                "       [translates a COFF executable file into a memory image",
                "        containing an image of the program as it would appear when",
                "        downloaded]",
                "   move <object-file>[<mem-sect><phys-address>]",
                "       [changes the physical address of mem-sect in the object file",
                "        to a physical address so that it may be used for a]",
                "        particular EPROM.  The physical address may be specified in",
		"        hex, or in a relative phrase such as 'after .data']",
                "   packhex <hex file to compress>",
                "       [compresses the hex file by re-packing the data records,",
                "        the hex file will be converted in-place, this should be",
                "        done before a split]",
                "   patch <image-to-patch><file-holding-patch> address",
                "       [writes the contents of the file holding the patch into",
                "        the image to patch at offset address]",
                "   permute-a <image-input><new-order><image-output>",
                "       [scrambles the ROM by specifying a permutation of the",
                "        address lines.  New-order is a series of 32 integers",
                "        separated by spaces that specifies, for each address bit",
                "        from left to right, that bit's new position in the output",
                "        ROM image",
                "   permute-d <image-input><new-order><image-output>",
                "       [permutes the bits in each ROM-width hunk of data in the",
                "        rom image.  New-order is a series of integers separated by",
                "        spaces that specifies, for each data bit from least to most",
                "        significant position that bit's new position in the rom-image",
                "   rom <rom-length><rom-width>[rom-count]",
                "       [specifies the length and width of ROM images for subsequent",
                "        permute-d commands; allows the split command to issue",
                "        warnings if its results will not fit the ROM space described]",
                "   sh command",
                "       [spawns a shell command, upon completion of the command,",
                "        control returns to ROM960]",
                "   split image memlen memwidth romlen romwidth outfile_nm",
                "       [splits a memory image of one length in bytes and width",
                "        into ROM images, possibly of another length in bytes and",
                "        width in bits]",
                "",
                "See your user's guide for a complete command-line description",
                "",
                NULL
        };

/* Routine to output help message */
put_rom_help()
{
        fprintf(stdout,"\nUsage: rom960 [-V] [config_file [script_arg[, script_arg ...]]]\n");
        paginator(utext);	/* prints so it can be seen on one page */
} /* put_rom_help */


main(argc,argv)
int argc;               
char **argv;
{
	FILE * fpConfig;
	int foundone = 0;
	static char buf[STRSIZ];	/* buffer from which cmd's interp'd*/
	static char scr_buf[STRSIZ];	/* buffer commands read into from script*/
	int err;
	char prompt[20];		/* Prompt for interactive mode.	*/
	int promptlen;
	int optout;
	int cnt;			/* Loop counter.		*/
	int i;				/* arg count (parameter count)	*/
	int  numfiles;       /* number of input files to process */

#ifdef VMS
	extern 	char	*getfile( );
        extern struct vms_fspec *vtemplate;
	char cliqual;			/* qualifier letter       	*/
	char *tmpptr;			/* temporary pointer 		*/
	char *arg_array[15];		/* argv simulator, max 10 args	*/
        int null_var;			/* null item to point at	*/
#endif

	argc = get_response_file(argc,&argv);

	check_v960(argc,argv);

	/* set these to 0 here so we'll know if there was no invocation
	of the "rom" command */
	romlen = 0;
	romwidth = 0;
	bytes_in_rom = 0;
	/* Need to show version?	*/

#ifndef VMS
 
        while ( (argc > 1) && (optout = getopt(argc, argv, options)) != EOF) {
                switch ((char)optout) {
 
 	           case 'h':
			   put_rom_help();
			   exit(0);
	 
        	   case 'V':
			   gnu960_put_version();
			/* Shift arguments left.        */
			   for (cnt = 2; cnt < argc; cnt++)
				argv[cnt - 1] = argv[cnt];
			   argc--;
                           break;
                   default:
                           usage();
                           exit(1);
      }
}
       
#endif
	/* allow other parts of program to examine command line */
	g_argc = argc;       
	g_argv = argv;

	line = 0;
	interactive = 0;
#ifdef VMS
        cliqual = getopt();
        if (cliqual == 'V') {
	    Showversion(ROMNAME);
            cliqual = getopt();
	}
	else if(cliqual == 'h') {
	    put_rom_help();
	    exit(0);
	}
	else {
	    usage();
	    exit(1);
	}	
        tmpptr = getfile();
	if (tmpptr == NULL) {
#else
	if (argc == 1) {
#endif
		/* we are going into interactive mode; the setjmp keeps
		us from exiting on an interactively entered  command */
		err = setjmp(parse_err);

		sprintf(prompt,"%s>", ROMNAME);
		promptlen = strlen(prompt);
		interactive = 1;
		fwrite(prompt, promptlen,1,stderr);
		while (gets(buf)) {
			line++;
			exec_line(buf);
			fwrite(prompt, promptlen,1,stderr);
		}
		exit (EXIT_OK);
	} else { /* we're getting commands from a configuration file */
		/* make us terminate on error, since later commands in
		a configuration file may depend on the successful completion
		of earlier ones */

		err = setjmp(parse_err);
		if (err)
			exit (err);

		/*   open configuration file   */
#ifdef VMS
        	tmpptr = compose_fname( vtemplate, NULL, NULL, "LD", NULL); 
		strcpy( configfile, tmpptr);
                if (vtemplate->path[0] == 0) {
                    if (!Libsearch( tmpptr, configfile)) 
			/* the following line was commented out and the
			ones after that added during update to common
			error handling 9/29/89 -- robertat 
			error("can't find configuration file %s\n", tmpptr);
			*/
			{
			error_out(ROMNAME,NO_FOUND,0,tmpptr);
			longjmp(parse_err,1);
			}
                }
                                    
                if ((fpConfig = fopen(configfile, FOPEN_RDONLY)) == NULL) {
                    if (fpConfig == NULL)
			/* the following line was commented out and the
			ones after that added during update to common
			error handling 9/29/89 -- robertat 
			error("can't open configuration file %s\n",configfile);
			*/
			{
			error_out(ROMNAME,NOT_OPEN,0,configfile);
			longjmp(parse_err,1);
			}
                }

		/* getfile will return NULL when parameter list exhausted */
                null_var = NULL;
	        for (i=0; i<15; i++) arg_array[i] = &null_var;
                g_argv = arg_array;
		g_argc = 2;  
		while (tmpptr=getfile()) {
                        arg_array[g_argc] = copy(tmpptr);
			g_argc++;
                }    
#else                                  
		/* begin change wrought 10/17/89 by robertat to make the
		ROM builder try to find its script file in the current
		directory before it tries to prepend some default directory
		path to its name */
		strcpy(configfile,argv[1]);
		strcat(configfile,".ld");
		if ((fpConfig = fopen(configfile, FOPEN_RDONLY)) == NULL) {
			/* can't find it here, we'll look in the library
			directories */
#if defined(MSDOS) || defined(MWC) || defined(CBLD)
			/* On DOS, full paths can start with "/", ".",     */
			/* "\" or "d:" where d is a drive specifier. (mlb) */

			if ((strchr("./\\", (int) argv[1][0]) == NULL) &&
			    !(isalpha(argv[1][0]) && argv[1][1] == ':'))
#else
			if (argv[1][0] != '.' && argv[1][0] != '/')
#endif
			{
				setpaths();
				strcpy(configfile,libdir);
				strcat(configfile,"/");
			} else           
				configfile[0]='\0';
       	    
			(void) strcat(configfile,argv[1]);
			(void) strcat(configfile,".ld");
			if ((fpConfig = fopen(configfile, FOPEN_RDONLY)) == NULL)
			{
				/* the following line was commented out and the
				ones after that added during update to common
				error handling 9/29/89 -- robertat 
				error("can't find configuration file %s\n", configfile);
				*/
				/* just can't find the script file */
				error_out(ROMNAME,NO_FOUND,0,configfile);
				longjmp(parse_err,1);
			}
		}
#endif

		/* find all commands for p7rom in configfile */
		/* and perform argment substitution before exec_line*/
		while (fgets(scr_buf,STRSIZ,fpConfig)) {
			line++;
			if (!strncmp(scr_buf,"#*",2)) {
				arg_subst(scr_buf,buf);
				fprintf(stderr,"+ %s",buf);
				exec_line(buf+2);
				foundone++;
			}
		}
	}

	if (!foundone)
		/* the following line was commented out and the ones
		after that added during update to common error handling
		9/29/89 -- robertat 
		error ("no commands for %s in configuration file %s\n",
						ROMNAME, configfile);
		*/
		{
		error_out(ROMNAME,ROM_NO_CMDS,0,configfile);
		longjmp(parse_err,1);
		}
	exit(EXIT_OK);
}

/*
 * arg_subst -- argument substitution on a command
 * read from a target file. $0 -> argv[2], $1 -> argv[3],
 * and so on.
 */

arg_subst(bFrom,bTo)
char *bFrom;
char *bTo;
{
	int arg_num, offset;
	while (*bFrom) {
		while (*bFrom && (*bFrom != '$'))
			*(bTo++) = *(bFrom++);
		if (*bFrom == '$') {
			bFrom++;
			if (index("0123456789",*bFrom)) {
				arg_num = atoi(bFrom);
				while (index("0123456789",*++bFrom));
				if (arg_num < 0 || arg_num >= g_argc-2)
					/* the following line was commented out
					and the ones after that added during
					update to common error handling
					9/29/89 -- robertat 
					p_error("argument number makes no sense\n");
					*/
					{
					error_out(ROMNAME,ROM_SILLY_ARG,0,bFrom);
					longjmp(parse_err,1);
					}
				else {
					offset = 0;
					while (*(g_argv[arg_num+2]+offset)) {
						*bTo= *(g_argv[arg_num+2]+offset);
						bTo++;
						offset++;
					}
				}
			} else
				/* the following line was commented out
				and the ones after that added during
				update to common error handling
				9/29/89 -- robertat 
				p_error ("illegal argument substitution\n");
				*/
				{
				error_out(ROMNAME,ROM_SILLY_ARG,0,bFrom);
				longjmp(parse_err,1);
				}
		}
	}
	*bTo = '\0';
}

/*
 * exec_line -- field command line to routine handling it.
 */
exec_line(s)
char *s;
{
	if (!strncmp("split",s,5)) {
		split(s+5);
	} else if (!strncmp("map",s,3)) {
		map(s+3);
	} else if (!strncmp("rom",s,3)) {
		rom(s+3);
	} else if (!strncmp("mkimage",s,7)) {
		mkimage(s+7);
	} else if (!strncmp("mkfill",s,6)) {
		mkfill(s+6);
	} else if (!strncmp("permute-d",s,9)) {
		permute_d(s+9);
	} else if (!strncmp("permute-a",s,9)) {
		permute_a(s+9);
	} else if (!strncmp("patch",s,5)) {
		patch(s+5);
	} else if (!strncmp("packhex",s,7)) {
		packhex(s+7);
	} else if (!strncmp("checksum",s,8)) {
		checksum(s+8);
	} else if (!strncmp("ihex",s,4)) {
		ihex(s+4);
	} else if (!strncmp("move",s,4)) {
		move(s+4);
	} else if (!strncmp("sh",s,2)) {
#ifdef i960
		/* the following line was commented out and the ones after
		that added during update to common error handling 9/29/89
		--- robertat
		p_error ("unsupported function on the EVA Host");
		*/
		error_out(ROMNAME,ROM_UNSUP_EVA,0);
		longjmp(parse_err,1);
#else
#if defined(MWC)
		errno = 0;	/* needed to work around an MWC system() bug */
		if (system(s+2) != 0) {
			fprintf(stderr, "%s: system(%s) failed.\n", ROMNAME, s+2);
			longjmp(parse_err,1);
		}
#else
		system(s+2);
#endif
#ifdef VMS
                fprintf(stderr,"\n");
#endif
#endif
	} else if ((!strncmp("exit",s,4)) || (!strncmp("quit",s,4))) {
		exit(0);
	} else if (!strncmp("help",s,4)) {
		put_rom_help();
	} else
		/* the following line was commented out and the ones after
		that added during update to common error handling 9/29/89
		--- robertat
		p_error ("unrecognized %s command: %s", ROMNAME, s);
		*/
		{
		error_out(ROMNAME,ROM_SILLY_CMD,0,s);
		longjmp(parse_err,1);
		}
}

/*
 * split - break up an image into rom sized
 * pieces.
 */
split (s)
char *s;
{
	char imagename[256];
	FILE *fpIn = get_bin_file_rname(&s,"binary image input file:", imagename);
	char_p image;
	FILE *fpOut;
	char *out;
	long i;			/* MSDOS int to long */
	int j;
	long k;			/* MSDOS int to long */
	long count;		/* MSDOS int to long */
	char buf[256];		/* In case we can't malloc image.	*/

	memlen = get_int(&s,"memory image length in bytes:",NULL);
	if (memlen <= 0) {
		error_out(ROMNAME,ROM_BAD_MEMLEN,0,memlen);
		longjmp(parse_err,1);
	}
	memwidth = get_int(&s,"memory datapath width in bits:",NULL);
	if (memwidth <= 0) {
		error_out(ROMNAME,ROM_BAD_MEMWIDTH,0,memwidth);
		longjmp(parse_err,1);
	}
	romlen = get_int(&s,"rom size in bytes:",NULL);
	if (romlen <= 0) {
		error_out(ROMNAME,ROM_BAD_ROMLEN,0,romlen);
		longjmp(parse_err,1);
	}
	romwidth = get_int(&s,"rom datapath width in bits:",NULL);
	if (romwidth <= 0) {
		error_out(ROMNAME,ROM_BAD_ROMWIDTH,0,romwidth);
		longjmp(parse_err,1);
	}
	/* memwidth must be greater than romwidth */
	if (memwidth < romwidth) {
		error_out(ROMNAME,ROM_MEMW_LT_ROMW,0,memwidth,romwidth);
		longjmp(parse_err,1);
	}
	/* memwidth must be an even multiple of romwidth */
	if (((memwidth/romwidth) * romwidth) != memwidth) {
		error_out(ROMNAME,ROM_BAD_WIDTH_RATIO,0);
		longjmp(parse_err,1);
	}
	/* can't take ROM widths in other than even byte widths */
	if (((romwidth/8) * 8) != romwidth) {
		error_out(ROMNAME,ROM_ROM_WIDTH,0);
		longjmp(parse_err,1);
	}
	out = copy(get_file_name(&s,"base name for output files:"));

	if (!(image = fill_image(fpIn,memlen,&count)))
		{
		/* the next three lines were commented out and the one
		after that added during update to common error handling
		9/29/89 --- robertat
		fprintf(stderr, 
		"Warning:  Large file (%s) will take a long time to split.\n",
			imagename);
		*/
		warn_out(ROMNAME,ROM_WARN_SPLIT,0,imagename);
		}

	/* i controls the progression vertically through the
	memory image, i.e., each pass through the outermost for loop
	copies one horizontal "stripe" of the memory image into the
	appropriate number of ROMs */
	for (i=0; i<count; i += romlen*(memwidth/romwidth)) {
		/* j controls the progression through each "stripe",
		creating as many ROM images as will represent the
		full width of the memory image one rom-length deep */
		for (j=0; j<memwidth; j += romwidth) {
			sprintf(path_tmp,"%s.%x%x",
			        out,(int)(i/(romlen*(memwidth/romwidth))),j/romwidth);
			if (NULL == (fpOut = fopen(path_tmp, FOPEN_WRONLY_TRUNC)))
				/* the following line was commented out
				and the next two added during the update
				to common error handling 9/29/89 --- robertat 
				error ("couldn't open output file\n");
				*/
				{
				error_out(ROMNAME,NOT_OPEN,0,path_tmp);
				longjmp(parse_err,1);
				}
			/* k skips through the memory image picking out
			the bytes that should go into a specific ROM in
			this "row" of the memory image; for 8-bit-wide
			ROMs taking a 32-bit-wide image, for instance,
			bytes 0,4,8, etc., go into the leftmost ROM,
			bytes 1,5,9, etc., go into the one next to it,
			bytes 2,6,10, etc., into the one after that, etc. */
			for (k=i+(j/8); k<min(i+(romlen*(memwidth/romwidth)),count); k+=memwidth/8)
			{
				if (image)	/* All read in?		*/
				{
					fwrite(&(image[k]),1,romwidth/8,fpOut);
				}
				else		/* The slow way.	*/
					{
					fseek(fpIn, k, 0);
					fread(buf, 1, romwidth/8, fpIn);
					fwrite(buf, 1, romwidth/8, fpOut);
					}
			}
			fclose (fpOut);
		}
	}
	if (image)
		free (image);
	fclose (fpIn);
/*	fclose (fpOut); */
}

/*
 * rom -- set dimensions of rom.
 */
rom (s)
char *s;
{
	romlen = get_int(&s,"rom size in bytes:",NULL);
	if (romlen <= 0) {
		error_out(ROMNAME,ROM_BAD_ROMLEN,0,romlen);
		longjmp(parse_err,1);
	}
	romwidth = get_int(&s,"rom datapath width in bits:",NULL);
	if (romwidth <= 0) {
		error_out(ROMNAME,ROM_BAD_ROMWIDTH,0,romwidth);
		longjmp(parse_err,1);
	}
	/* can't take ROM widths in other than even byte widths */
	if (((romwidth/8) * 8) != romwidth) {
		error_out(ROMNAME,ROM_ROM_WIDTH,0);
		longjmp(parse_err,1);
	}
	/* always ask for rom-count in interactive mode; look for it
	in configuration file only if we haven't hit the end of the
	current command line */
	skip_white(&s);
	romcount = 0;
	if ((interactive) || (*s != '\0'))
	{
/* This is buggy.  If interactive, and input is being redirected from
 * a script, and the romcount argument is absent from the script, we
 * get garbage.  Get_int should be modified to ensure that it reads an
 * integer value, and return an error if it did not. (mlb)
 */
		romcount = get_int(&s,"number of roms to be burned:",NULL);
		if (romcount <= 0) {
			error_out(ROMNAME,ROM_BAD_ROMCOUNT,0,romcount);
			longjmp(parse_err,1);
		}
	}
	/* if we know how many ROMs, figure out how much space total
	will be covered by them */
	if (romcount != 0)
		bytes_in_rom = romlen * romcount;
}

/*
 * mkimage -- obj -> memory image.
 */
mkimage (s)
char *s;
{
    struct name_list *root = 0,**bottom_of_name_list = &root;
    int nsects = 0;
    char *obj_in;
    char *mem_out;

    obj_in = copy(get_file_name(&s,"input file:"));
    mem_out = copy(get_file_name(&s,"memory image output file:"));
    if (*s)
	    skip_white(&s);
    while (*s) {
	struct name_list *p = (*bottom_of_name_list) = (struct name_list *)
		xmalloc(sizeof(struct name_list));
	p->name = copy(get_file_name(&s, NULL));
	p->next_name = 0;
	bottom_of_name_list = &p->next_name;
	nsects++;
	if (*s)
		skip_white(&s);
    }
    obj_to_mem(obj_in,mem_out,0xff,root,nsects);
}


/*
 * mkfill -- obj -> memory image.
 */
mkfill(s)
char *s;
{
    struct name_list *root = 0,**bottom_of_name_list = &root;
    int nsects = 0;
    char *obj_in;
    char *mem_out;
    char fill_char;

    obj_in = copy(get_file_name(&s,"input file:"));
    mem_out = copy(get_file_name(&s,"memory image output file:"));
    fill_char = get_int(&s,"fill character:",NULL);
    if (*s)
	    skip_white(&s);
    while (*s) {
	struct name_list *p = (*bottom_of_name_list) = (struct name_list *)
		xmalloc(sizeof(struct name_list));
	p->name = copy(get_file_name(&s, NULL));
	p->next_name = 0;
	bottom_of_name_list = &p->next_name;
	nsects++;
    }
    obj_to_mem(obj_in,mem_out,fill_char,root,nsects);
}

/*
 *	Move moves a section to a new physical location in a COFF/bout file.
 *	If a section & address aren't given, .data is moved just past
 *	.text.
 */
move (str)
	char *str;			/* ->OBJ file, section, loc.	*/
	{
	char *objname;			/* ->OBJ file.			*/
	char *section,*asectname = NULL;/* Gets section name. and 'after' sectname. */
	long addr;			/* Gets address.		*/
	int issue_hard_error = 1;       /* Flag to tell move section to issue a
					   hard error if the named section does not exist. */

	objname = copy(get_file_name(&str, "object input file:"));
					/* Get section if given.	*/
	if (section = get_file_name(&str, (char *) 0))
		{
		    section = copy(section);
		    addr = get_int(&str, "new physical address:",&asectname);
		    if (asectname)
			    addr = -2;
		}
	else				/* Use .data by default.	*/
		{
		issue_hard_error = 0;
		section = ".data";
		addr = -1;
		}
	move_section(objname, section, addr, issue_hard_error, asectname);
	}

/*
 * permute_d -- permute data bits. on the command line,
 * each successive integer, from bit 0 (LSB) to bit ROMWIDTH-1 (MSB)
 * indicates its location in the corresponding word of the output
 * file.
 */
permute_d (s)
char *s;
{
	long i;			/* MSDOS int to long */
	int j;
	char bit_order[MAXBITS];
	char bits_in[MAXBITS];
	char bits_out[MAXBITS];
	FILE *fpOut;
	FILE *fpIn;
	char_p image;
	char tmp[STRSIZ];
	long count;		/* MSDOS int to long */
	char buf[256];		/* Holds a chunk in case image = 0.	*/
	char *imageptr;		/* ->data in memory.			*/

	if (romlen == 0) {
		error_out(ROMNAME,ROM_BAD_ROMLEN,0,romlen);
		longjmp(parse_err,1);
	}
	if (romwidth == 0) {
		error_out(ROMNAME,ROM_BAD_ROMWIDTH,0,romwidth);
		longjmp(parse_err,1);
	}
	imageptr = buf;
	fpIn = get_bin_file_r(&s, "rom image to permute:");
	for (i=0; i<romwidth; i++) {
		sprintf(tmp,"place bit %d where:",i);
		bit_order[i] = get_int (&s,tmp,NULL);
	}
	image = fill_image(fpIn,(romlen*romcount),&count);
	fpOut = get_bin_file_w(&s, "scrambled rom output file:");

	/* scramble the data bits. */
	for (i=0; i<count; i+=romwidth/8) 
		{
		if (image)		/* Got the whole image?		*/
			imageptr = &(image[i]);
		else
			{		/* Read chunk from file.	*/
			fseek(fpIn, i, 0);
			fread(buf, 1, romwidth/8, fpIn);
			}
		bit_to_byte(imageptr,bits_in,romwidth);
		for (j=0; j<romwidth; j++)
			bits_out[bit_order[j]] = bits_in[j];
		byte_to_bit(bits_out,imageptr,romwidth);
		if (!image)		/* Write out chunk now.		*/
			fwrite(imageptr, 1, romwidth/8, fpOut);
		}
	if (image)
		{
		write_image(image,fpOut,count);
		free (image);
		}
	fclose (fpOut);
	fclose(fpIn);
}

static int host_is_little_endian() {
    int x = 0x12345678;

    if (((char*)&x)[0] == 0x78)
	    return 1;
    else
	    return 0;
}

/* Returns a host endian byte ordered integer as a little endian integer. */

static int he_as_le(n)
    int n;
{
    if (host_is_little_endian())  /* The host is little endian already.  Return n. */
	    return n;
    else { /* Switch byte order. */
	int r;
	char *p = (char *)&n,*q = (char *) &r;
	int i;

	for (i=0;i < 4;i++)
		q[i] = p[3-i];
	return r;
    }
}

/* Translate a little endian integer to host endian byte order. */
static int le_as_he(n)
    int n;
{
    if (host_is_little_endian())  /* The host is little endian already.  Return n. */
	    return n;
    else { /* Switch byte order. */
	int r;
	char *p = (char *)&n,*q = (char *) &r;
	int i;

	for (i=0;i < 4;i++)
		q[i] = p[3-i];
	return r;
    }
}

/*
 * permute_a -- 
 * For each word, take its offset in the file, perform 
 * a bit scrambling similar to permute_d, and use this new
 * offset in storing word in output image.
 */
permute_a (s)
char *s;
{
        unsigned long i,j,i2,out_image_size;
	char bit_order[P7ADDRSIZ];		/*p7 address size*/
	char bits_in[P7ADDRSIZ];		/*assume 32 bit address*/
	char bits_out[P7ADDRSIZ];
	char transfer[MAXBITS/8];	/* Transfer buffer.		*/
	char_p image = 0;
	char_p image_out = 0;
	FILE *fpOut;
	FILE *fpIn;
	char tmp[STRSIZ];
	long count;				/* MSDOS int to long */
	char imagename[256];

	if (romlen == 0) {
		error_out(ROMNAME,ROM_BAD_ROMLEN,0,romlen);
		longjmp(parse_err,1);
	}
	if (romwidth == 0) {
		error_out(ROMNAME,ROM_BAD_ROMWIDTH,0,romwidth);
		longjmp(parse_err,1);
	}
	fpIn = get_bin_file_rname(&s,"rom image for address permutation:",
						imagename);
	for (i=0; i<P7ADDRSIZ; i++) 
		{
		sprintf(tmp,"place bit %d where:",i);
		bit_order[i] = get_int (&s,tmp,NULL);
		}
	fpOut = get_bin_file_w(&s, "scrambled rom output file:");
	if (!(image = fill_image(fpIn,(romlen*romcount),&count)) ||
	    !(image_out = myalloc((romlen*romcount))))
		{
		/* the next three lines were commented out and the one
		after that added during update to common error handling
		9/29/89 --- robertat
		fprintf(stderr, 
	"Warning:  Large file (%s) will take a long time to permute.\n",
			imagename);
		*/
		warn_out(ROMNAME,ROM_WARN_PERMUTE,0,imagename);
		if (image)
			free(image);
		image = 0;
					/* Fill to end if necessary.	*/
		Seekfill(fpOut, (romlen*romcount), '\377');
		}
        else {
                /* If we got to here, we have an image_out array to play
                with and need to initialize it to 0xff before we write
                the real data into it. */
                for (i=0;i<(romlen*romcount);i++ ) {
                        image_out[i]='\377';
                }
                /* And we need to initialize the size of what we write
                out to 0. */
                out_image_size = 0;
        }

	/* scramble address bits */
	for (i=0; i<count; i += romwidth/8) {
	    int temp = he_as_le(i);

	    bit_to_byte(&temp,bits_in,P7ADDRSIZ);
	    for (j=0; j<P7ADDRSIZ; j++)
		    bits_out[bit_order[j]] = bits_in[j];
	    byte_to_bit(bits_out,&temp,P7ADDRSIZ);
	    i2 = le_as_he(temp);
	    /* We test against the full size of the area in ROMs, as
	       described by the ROM command. If we switch around the address
	       bits, we'll probably leave some areas that were populated
	       blank and some that were blank, including some after the
	       end of the current image, populated. */
	    if (i2 + romwidth/8 > (romlen*romcount))
		    /* the next line was commented out and the ones
		       after it added during the update to common error
		       handling 9/29/89 -- robertat
		       p_error("address permutation produces out of bounds address\n");
		       */
		{
		    error_out(ROMNAME,ROM_ADDR_BOUNDS,0);
		    longjmp(parse_err,1);
		}
	    if (i2+(romwidth/8) > out_image_size) {
		out_image_size = i2+(romwidth/8);
	    }
	    if (!image)	/* Slow way?			*/
		{
		    fseek(fpIn, i, 0);
		    fread(transfer, romwidth/8, 1, fpIn);
		    fseek(fpOut, i2, 0);
		    fwrite(transfer, romwidth/8, 1, fpOut);
		}
	    else
		    for (j=0; j<romwidth/8; j++)
			{
			    image_out[i2+j] = image[i+j];
			}
	}
	if (image)
	    {
		free (image);
		write_image(image_out,fpOut,out_image_size);
		free (image_out);
	    }
	fclose (fpOut);
	fclose (fpIn);
}

/*
 * patch -- overlay contents of file over a specified address
 *	of image.
 */
patch (s)
char *s;
{
	FILE *fpIn = get_bin_file_rw(&s, "file to patch:");
	FILE *fpPatch = get_bin_file_r(&s, "file containing patch:");
	unsigned long addr = get_int(&s,"patch target address:",NULL);
	int got = 1;
	char buf[1000];
	fseek(fpIn,addr,0);
	while (got) {
		got = fread(buf,1,1000,fpPatch);
		if (got && !fwrite(buf,1,got,fpIn))
			/* the following line was commented out and the
			ones after that added during update to common
			error handling 9/29/89 --- robertat
			error ("couldn't write patch to image");
			*/
			{
			error_out(ROMNAME,ROM_PATCH_WRITE,0);
			longjmp(parse_err,1);
			}
	}
	fclose (fpIn);
	fclose (fpPatch);
}

/*
 * checksum -- calculate crc16 or crc32 checksum, and store into image.
 */


checksum (s)
char *s;
{
	FILE *fpIn;
	long int fileEnd;
	unsigned long addrStart;
	unsigned long addrEnd;
	unsigned long addrTarget;
	long sum;
	char sum_lower,sum_upper,sum_3, sum_4;
	char *Startaddress = {"Start address"};
	char *Endaddress = {"End address"};
	char *Destinationaddress = {"Destination address"};
	int mode;

	fpIn = get_bin_file_rw(&s, "image in which to insert checksum:");
	addrStart = get_int(&s,"start of range to checksum:",NULL);
	addrEnd = get_int(&s,"end of range to checksum:",NULL);
        mode = 32;
        mode = get_int(&s,"16 or 32?",NULL);
        if ((mode != 16) && (mode != 32)){
                error_out(ROMNAME,ROM_BAD_IHEX_MODE,0);
                return(1);
        }

	if (addrEnd < addrStart) {
		/* start address is greater than end address */
		error_out(ROMNAME,ROM_CKSUM_ADDRS,0);
		longjmp(parse_err,1);
	}
	/* check for addresses beyond the end of the file */
	fseek(fpIn,0,SEEK_END);
	fileEnd = ftell(fpIn);
	fseek(fpIn,0,SEEK_SET);
	
	if (mode == 16) 	
		addrTarget = get_int(&s,"where to place checksum (2 bytes):",NULL);
	else
		addrTarget = get_int(&s,"where to place checksum (4 bytes):",NULL);
	if ((addrTarget >= addrStart) && (addrTarget <= addrEnd)){
		/* checksum will be written into area checksummed,
		rendering checksum invalid */
		error_out(ROMNAME,ROM_CKSUM_TARGET,0);
		longjmp(parse_err,1);
	}
	if (addrStart > fileEnd)
		warn_out(ROMNAME,ROM_CKSUM_STRETCH,0,Startaddress,addrStart);
	if (addrEnd > fileEnd)
		warn_out(ROMNAME,ROM_CKSUM_STRETCH,0,Endaddress,addrEnd);
	if (addrTarget > fileEnd)
		warn_out(ROMNAME,ROM_CKSUM_STRETCH,0,Destinationaddress,addrTarget);

	/* Find starting spot in input file.	*/
	fseek(fpIn, addrStart, 0);

        /* compute crc checksum. */
        if(mode == 16) {
                /* needs to be 16 bits wide */
                sum = crc16(fpIn, (unsigned long) addrEnd - addrStart + 1);
 
                /* patch file in correct place */
                /* for p7, least significant byte in */
                /* lower address */
                sum_lower = sum & 0xff;
                sum_upper = (sum >> 8) &0xff;
        }
        else {          /* mode == 32 */
                /* needs to be 32 bits wide */
                sum = crc32(fpIn, (unsigned long) addrEnd - addrStart + 1);
 
                /* patch file in correct place */
                /* for p7, least significant byte in */
                /* lower address */
                sum_lower = sum & 0xff;
                sum_upper = (sum >> 8) & 0xff;
                sum_3 = (sum >> 16) & 0xff;
                sum_4 = (sum >> 24) & 0xff;
        }
 
	/* go to point in file into which to insert checksum */
	if (addrTarget > fileEnd) {
		Seekfill(fpIn,addrTarget,'\377');  /* pad with 0xFF if need be */
	}
	else {
		fseek(fpIn,addrTarget,0);
	}

        if(mode == 16) {
                fwrite(&sum_lower,1,1,fpIn);
                fwrite(&sum_upper,1,1,fpIn);
	}
        else {
                fwrite(&sum_lower,1,1,fpIn);
                fwrite(&sum_upper,1,1,fpIn);
                fwrite(&sum_3,1,1,fpIn);
                fwrite(&sum_4,1,1,fpIn);
        }

	fclose (fpIn);
}


/* fill up memory array with data from file. */
char_p
fill_image(fp,toget,pcnt)
FILE *fp;
long toget;		/* MSDOS int to long */
long *pcnt;		/* MSDOS int to long */
{
	char_p image;
	char *pc;
	long left;	/* MSDOS int to long */
	int got;
	extern long ftell();

	if (toget == 0 || (!(image = myalloc(toget)))) {
	    fseek(fp, 0, 2);		/* Find end of file.	*/
	    *pcnt = ftell(fp);		/* Return image size.	*/
	    return ((char_p) 0);
	}

	left = toget;
	got = 1;

	/* done via halloc on MSDOS, malloc on MWC */
#if !defined(MSDOS) && !defined(MWC) || defined(CBLD)
	/* in case something screwy happens ..*/
	for (pc = image; pc < image+toget; pc++)
		*pc = 0;
#endif

	while (got) {
		if (left > (long)MAXDRW)
			left -= (got = fread(image+toget-left,1,MAXDRW,fp));
		else
			left -= (got = fread(image+toget-left,1,left,fp));
		if (!left)
			break;
	}

	*pcnt = toget - left;
	if (left == toget)
		/* the following line was commented out and the
		ones after that added during update to common
		error handling 9/29/89 -- robertat 
		error ("couldn't fill image array\n");
		*/
		{
		error_out(ROMNAME,ROM_READ_IMAGE,0);
		longjmp(parse_err,1);
		}
	/* warn user if we didn't get as many bytes as we were asked for */
	if (left > 0) 
		warn_out(ROMNAME,ROM_SHORT_READ,0,toget,*pcnt);

	/* warn user if there is more in the file than we were asked to read */

	/* Feof is not set until after we  try to read past eof. So we will
	try to read one more byte. */
	{
		char cbuf;

		if (fread(&cbuf, sizeof(char), 1, fp) > 0)
		{
			warn_out(ROMNAME, ROM_MORE_TO_GO, 0, toget);
		}
	}

	return image;
}

/* write memory array out to disk again */
write_image(image,fp,cnt)
char_p image;
FILE *fp;
long cnt;			/* MSDOS, int to long */
{
	long left = cnt;	/* MSDOS, int to long */
	int got = 1;

	while (left && got) {
		if (left > (long)MAXDRW)
			left -= (got = fwrite(image+cnt-left,1,MAXDRW,fp));
		else
			left -= (got = fwrite(image+cnt-left,1,left,fp));
		if (!left)
			break;
	}
}

/*
 *	Seekfill fills a file with "fillchar" up to a desired size, if it isn't
 *	possible to seek to it.
 */

Seekfill(fp, size,fillchar)
	FILE *fp;			/* ->file.			*/
	long size;			/* Desired size.		*/
	char fillchar;			/* what to fill with		*/
	{
	char fills[256];
	long end;			/* Current end of file.		*/
	int cnt;			/* # bytes to write.		*/
	int i;
	extern long ftell();
	for (i=0;i<256;i++) fills[i]=fillchar;

	fseek(fp, 0L, 2);	/* Seek to current end.		*/
	end = ftell(fp);
	while (end < size)	/* Fill to desired size.	*/
		{
		cnt = size - end < sizeof(fills) ? size - end : sizeof(fills);
		fwrite(fills, 1, cnt, fp);
		end += cnt;
		}
	}
