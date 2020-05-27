/* listing.c - produce an assembly listing
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
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* $Id: listing.c,v 1.17 1995/06/07 22:29:49 paulr Exp paulr $ */

#include "as.h"

#define		FORMFEED_CHAR	12
#define		SRCBUF_SIZE 	1024
#define 	next_info_line()	los.info_full = read_listinfo()

extern int 	DEFAULT_TEXT_SEGMENT;
extern char	*physical_input_file;
extern segS	*segs;

/* protected extensions are defined in out_file.c */
#ifdef	DOS
extern char *protected_extensions[];
#else
extern char *protected_extensions[];
#endif

/* Listing file */
static FILE	*listing_fp;		/* permanent listing file */

/* Temp files */
static FILE	*listing_pass1_src;
static FILE	*listing_pass1_info;
static char 	*listing_pass1_src_name;
static char 	*listing_pass1_info_name;


/* Input processing */
static char	*listing_buffer_start;
static char	*listing_line_pointer;
static char 	*advance_guard;
static int	listing_buffer_size;

/* A structure to hold listing input buffer info across .include's */
struct saved_listing
{
	char	*listing_buffer_start;
	char	*listing_line_pointer;
	char 	*advance_guard;
	int	listing_buffer_size;
	int	listing_now;
	int	listing_include;
};

/* Misc variables, local to this file */
static int	listing_lineid;		
static char	*listing_title;
static char	*listing_command_line;
static char	*listing_default_title;
static char	listing_error_buf[256];		/* FIXME: why a fixed size? */
static char	hex_digits [16] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
static char 	*complete_line = "%5d  %06x  %8s %8s  %s";
static char 	*redundant_fill_line = "%5d  %06x  < repeats ... >\n";
static char 	*source_change_line = "\nSource File: %s\n";
static char	*title_format = "%s\n";
static char	*default_title_format = "ASSEMBLER LISTING OF: %s\n";
static char	*version_format = "ASSEMBLER VERSION: %s\n";
static char	*timedate_format = "TIME OF ASSEMBLY: %s";
static char	*cmdline_format = "COMMAND LINE: %s\n\n";
static char	*error_rpt_format = "Number of errors: %4d\n";
static char	*warning_rpt_format = "Number of warnings: %2d\n";
static char	*blanks = "                ";  /* 16 */

struct listing_output_status
{
	long	curr_line;		/* in listing file */
	long	srcline;		/* in current source file */
	long	dot;			/* program address of start of current line */
	long	naddr;			/* program address of next byte to be output */
	char	codestr[16];		/* stringized code buf; not null-terminated */
	int	codeindx;		/* indexes code_str */
	char	*srcbuf;		/* user's source code buf */
	int	info_full;		/* 1 = listinfo is valid */
	int	src_full;		/* 1 = srcbuf is valid */
	int	partial;		/* 1 = there is at least one byte ready to be dumped */
} los;


/* 
 * Here's a VERY rough Table of Contents for this file as of 7/7/93, 
 * by approximate line number:
 *
 * 	Initialization utilities	90
 *	PASS1 (input)		       389
 * 	PASS2 (output)		       713
 *	Error message handling	      1196
 *	Debug routines		      1229
 */

/*
 * Initialization utilities
 *
 */

/*
 * listing_begin
 *
 * Open 2 temporary listing files, one for the input source lines,
 * (ascii text file) and one for the machine bytes generated. (binary)
 * Open the permanent listing file, to make sure it CAN be opened,
 * although we won't use it until after the second pass.  
 *
 * Set the global var "listing_now".  We start off assuming that a .list
 * has been seen.  (Of course, we would not be here unless a command-
 * line option had been given.)
 */
listing_begin()
{
#ifdef	DOS
	/* open source temp file in text mode */
	listing_pass1_src = create_temp_file ( &listing_pass1_src_name, "wt" );

	/* open info temp file in binary mode */
	listing_pass1_info = create_temp_file ( &listing_pass1_info_name, "wb" );

	/* open permanent listing file in text mode */
	listing_fp = listing_file_name ? fopen(listing_file_name, "wt") : stdout;
#else	
	listing_pass1_src = create_temp_file ( &listing_pass1_src_name, "w" );
	listing_pass1_info = create_temp_file ( &listing_pass1_info_name, "w" );
	listing_fp = listing_file_name ? fopen(listing_file_name, "w") : stdout;
#endif

	if ( listing_fp == NULL )
	{
		/* DON'T call as_fatal or you might wipe an existing file */
		fprintf (stderr, "Can't open listing file: %s\n", listing_file_name ? listing_file_name : "{standard output}");
		listing_end(0);
		exit(42);
	}

	/* Set the input scrubbers to be the modified ones */
	scrub_from_file = scrub_from_file_listing;
	scrub_to_file = scrub_to_file_listing;

	/* Set the default pass1 listing info struct */
	clear_listinfo();

	listing_input_start();
	listing_now = 1;
}


/*
 * listing_file_checkname
 *
 * Called from set_listing_filename() when the -Lf option is given. 
 * This is to be compatible with past CTOOLS assemblers that did
 * not allow you to blow away an existing file when the existing
 * file ends in .s, .as, or .asm.
 */
void
listing_file_checkname()
{
	int	i = 0;
	char	*c = (char *) strrchr(listing_file_name, '.'); 
	if ( c == NULL )
		return;
	for ( ; protected_extensions[i]; ++i ) 
		if ( ! strcmp(c, protected_extensions[i]) )
		{
			FILE	*fp = fopen(listing_file_name, "r"); 
			if ( fp )
			{
				fclose(fp);
				listing_file_name = NULL;
				as_fatal("Listing file will overwrite existing protected file.");
			}
			break;
		}
} 

extern char *get_960_tools_temp_dir();

/*
 * create_temp_file
 *
 * Create a unique temporary file.
 * Query standard environment variables and use the user's choice
 * of temporary directory if specified.  Note that the algorithm 
 * is different enough on DOS that it made sense to make it a
 * separate function.
 */
FILE *
create_temp_file ( name, mode )
char **name;
char *mode;
{
    extern char * get_960_tools_temp_file();
    FILE *fp = fopen(*name=get_960_tools_temp_file("ASXXXXXX",xmalloc),mode);

    if ( !fp )
	    as_fatal ("Can't create temp file: %s", (*name) ? (*name) : "(NULL)");

    return fp;
}

/*
 * Assign the listing title. 
 * 
 * Do this only once.  Return silently if the title has already
 * been assigned.
 */
set_listing_title( str )
char	*str;
{
	if ( listing_title )
		return;
	listing_title = (char *) xmalloc (strlen(str) + 1);
	strcpy ( listing_title, str );
}

/*
 * Assign the listing (output) file name.
 */
set_listing_filename( str )
char *str;
{
	listing_file_name = (char *) xmalloc (strlen(str) + 1);
	strcpy (listing_file_name, str);
	listing_file_checkname();
}

#ifdef M_GCC960
#define getcwd(a,b) 0
#endif
/*
 * Assign the listing default source file name.
 */
set_listing_default_title( fullpath, base )
char 	*fullpath;
char	*base;
{
	char *result;

	if ( listing_default_title )
		/* Been here once already */
		return;

	if ( ! strcmp (fullpath, base) )
	{
		/* file name is just a basename; get dirname, then append
		 * basename to dirname.
		 */
		if ( ! strcmp(base, STDIN_FILENAME) )
		{
			/* filename is not a file, but the special
			 * <stdin> string.
			 */
			result = (char *) xmalloc(strlen(base) + 1);
			strcpy(result, base);
		}
		else
		{
			result = (char *) xmalloc(128);
			if ( getcwd ( result, 128 ) )
			{
				/* Append the base name */
				int dirlen = strlen(result);
				if ( dirlen + strlen(base) + 2 > 128 )
					result = (char *) xrealloc ( result, dirlen + strlen(base) + 2);
				if ( result[dirlen - 1] != PATHNAME_DELIM )
				{
					result[dirlen] = PATHNAME_DELIM;
					result[dirlen + 1] = '\0';
				}
				strcat ( result, base );
			}
			else
				/* Oh, well; nice try */
				strcpy (result, base);
		}
	}
	else
	{
		/* file name is already a path */
		result = (char *) xmalloc (strlen(fullpath) + 1);
		strcpy ( result, fullpath );
	}

	listing_default_title = result;
}

/*
 * set_listing_command_line
 *
 * Make a copy of the user's command line, to include in the header.
 */
set_listing_command_line( argc, argv )
int argc;
char *argv[];
{
	register int i = 0;
	int len = 0;

	for ( ; i < argc; ++i )
		len += (strlen(argv[i]) + 1);

	listing_command_line = (char *) xmalloc (len + 1);
	*listing_command_line = '\0';

	for ( i = 0; i < argc; ++i )
	{
		strcat ( listing_command_line, argv[i] );
		strcat ( listing_command_line, " " );
	}
}

/*
 * We have a -Lxyz type option.  "optstr" parameter points at "xyz".
 * Find out what 'x' is, and do the appropriate thing for it.
 * Keep going through 'y' and 'z', if present, and if there is
 * supposed to be an argument following 'z', you'll find it in "arg".
 *
 * This imitates the base assembler's flagseen[] array.
 *
 * Return 1 if the second arg was used up, otherwise return 0.
 */
parse_listing_option( optstr, arg, myname )
char *optstr;
char *arg;
char *myname;
{
	int opt;

	while ( opt = *optstr++ )
	{
		listflagseen[opt] = 1;
		switch ( opt )
		{
		case 'a':	/* List all, ignoring .nolists; second arg is meaningless */
		case 'n':	/* Don't list include files; second arg is meaningless */
		case 'e':	/* List strict target endian, whether little- or big- */
		case 'z':	/* Don't print the listing header */
			break;
		case 'f':
			/* Output file name.  Use second arg. */
			if ( *optstr )
			{
				/* There was more left in "xyz" */
				set_listing_filename(optstr);
				return 0;
			}
			else if ( arg == NULL )
			{
				as_bad ("%s: Expected file name after -Lf", myname);
				return 0;
			}
			else
			{
				set_listing_filename(arg);
				return 1;
			}
		case 't':
			/* Title.  Use second arg. */
			if ( *optstr )
			{
				/* There was more left in "xyz" */
				set_listing_title(optstr);
				return 0;
			}
			else if ( arg == NULL )
			{
				as_bad ("%s: Expected listing title after -Lt", myname);
				return 0;
			}
			else
			{
				set_listing_title(arg);
				return 1;
			}
		default:
			as_bad ("%s: Unknown listing option: -L%c", myname, opt);
			return 0;
		}
	}
	return 0;
}



/*
 * "PASS 1" (input)
 *
 * Build the temp files from information passed in from the base assembler.
 *
 */

/*
 * listing_info()
 *
 * Report listing info from main assembler.
 * Inputs: 	status: choose from enum list
 *		size: total size of code buffer, beginning at "codebuf"
 *		codebuf: start of machine bytes within frag
 *		frag: points to start of frag
 *		byteswap: 
 *			for "complete" and "varying" lines:
 *				1 = swap the endianness of the codebuf 
 *				(mainly for printing instructions bigendian)
 *			for "info" lines: 
 *				a code to tell you what	type of info line it is.
 *		
 * Output:	a filled-in listinfo struct; writes same to pass1 info file
 */
listing_info (status, size, codebuf, frag, byteswap)
enum lst_status status;
int		size;
char		*codebuf;
fragS		*frag;
int	        byteswap;
{
	listinfo.status = status;
	listinfo.lineid = report_listing_lineid();
	listinfo.srcline = report_line_number();
	listinfo.size = size;
	listinfo.byteswap = byteswap;
	listinfo.frag = (char *) frag;
	listinfo.fragptr = codebuf;

	if ( status == info && byteswap == source_change )
	{
		/* Tells the backend to print an extra line (the source
		 * file name), so the lineid and source line will be
		 * off by one. 
		 */
		++listinfo.lineid;
		++listinfo.srcline;
	}

	write_listinfo();	/* also clears listinfo for next time */
}
	
/*
 * read_listinfo
 * 
 * read from the pass1 temp info file into the global listinfo struct.
 *
 * Return 1 if successful, 0 if not.
 */
read_listinfo()
{
	if ( fread ( &listinfo, sizeof listinfo, 1, listing_pass1_info ) != 1 )
	{
		if ( feof ( listing_pass1_info ) )
			return 0;
		else
			as_fatal ("Internal: error reading from listing temp file");
	}
	else
		return 1;
}

/*
 * write_listinfo
 *
 * Write from:  the global info structure.
 * Write to: 	the pass1 temp file (for listing info)
 */
write_listinfo()
{
	register int i;
	if ( fwrite ( &listinfo, sizeof listinfo, 1, listing_pass1_info ) != 1)
		as_fatal ("Internal: error writing to listing temp file");
	clear_listinfo();
}

/*
 * echo_char_to_listing
 *
 * Write a single input char to the listing input buffer.
 * The "advance guard" pointer will always be poised at the
 * next available cell in the buffer.  (unless it is beyond
 * the end of the buffer, in which case we'll deal with it.)
 *
 * Because of somewhat ... uh ... convoluted input logic
 * we have to check for EOF character here.
 *
 * If you are at the end of the buffer: this is not pretty.
 * FIXME: Make this a true circular buffer.  For now, just start
 * up a new buffer and copy remaining chars into the new buffer.
 * Don't realloc this buffer in the normal case.
 *
 * Note that it is possible, although rare, for this buffer to 
 * fill up before ANY chars are written to the pass1 temp file.
 * In that case, realloc the current buffer and don't copy 
 * anything.
 */
echo_char_to_listing(ch)
int ch;
{
	if ( ch != EOF )
	{
		if ( advance_guard == listing_buffer_start + listing_buffer_size )
		{
			/* Full buffer; Somebody, please ... FIXME! This should
			 * work like a true circular buffer with no copying ...
			 */
			if ( listing_buffer_start == listing_line_pointer )
			{
				/* No chars have been copied out of this buffer.
				 * Just make the buffer bigger; sooner or later
				 * the chars will be flushed out to the tmp file.
				 */
				listing_buffer_start = listing_line_pointer = 
					(char *) xrealloc ( listing_buffer_start, listing_buffer_size + INPUT_BUFFER_SIZE );
				advance_guard = listing_buffer_start + listing_buffer_size;
				listing_buffer_size += INPUT_BUFFER_SIZE;
			}
			else
			{
				/* Normal case; at least SOME chars have been flushed
				 * to the temp file.  Copy remaining chars to the start 
				 * of the buffer and don't increase the size.
				 */
				register char *c;
				for ( c = listing_buffer_start; listing_line_pointer < advance_guard; ++listing_line_pointer, ++c )
					*c = *listing_line_pointer;
				listing_line_pointer = listing_buffer_start;
				advance_guard = c;
			}
		}
		/* OK, buffer is ready for new chars. */
		*advance_guard++ = ch;
	}
}

/*
 * unecho_char_from_listing
 *
 * In rare cases, the input logic of the assembler pushes a char
 * back onto the input stream.  But we've already echoed it to
 * the listing buffer.  So back up over that char.
 */
unecho_char_from_listing()
{
	if ( advance_guard <= listing_line_pointer )
		as_fatal ("Internal: listing input buffer inconsistent");
	--advance_guard;
}


/*
 * listing_input_start
 *
 * Initialize the input buffer parameters (static to this file.)
 */
listing_input_start()
{
	listing_buffer_start = listing_line_pointer = advance_guard =
		(char *) xmalloc ( LISTING_BUFFER_SIZE );
	listing_buffer_size = LISTING_BUFFER_SIZE;
}

/*
 * listing_input_end
 *
 * We are returning from an include push.  Do listing_input_start
 * in reverse. 
 */
listing_input_end ()
{
	if ( listing_buffer_start )
	{
		free ( listing_buffer_start );
		listing_buffer_start = NULL;
	}
}


/*
 * listing_push, listing_pop
 *
 * Save the state of our input buffer across .include's.
 * The struct "saved" will be stored with in_scrub's saved 
 * state; it can be restored with a call to listing_pop().
 */
char *
listing_push()
{
	struct saved_listing *saved = 
		(struct saved_listing *) xmalloc (sizeof *saved);

	saved->listing_buffer_start	= listing_buffer_start;
	saved->listing_line_pointer	= listing_line_pointer;
	saved->advance_guard		= advance_guard;
	saved->listing_buffer_size	= listing_buffer_size;
	saved->listing_now		= listing_now;
	saved->listing_include		= listing_include;

	/* If -Ln was seen, turn off listing; previous state will be 
	 * restored when we pop.
	 */
	listing_now = listflagseen['n'] ? 0 : listing_now;
	listing_include = 1;

	listing_input_start();	/* Re-initialize */
	return (char *)saved;
}

void
listing_pop (dummy)
     char *dummy;
{
	struct saved_listing *saved = (struct saved_listing *) dummy;

	listing_input_end();	/* free current buffers */

	listing_buffer_start	= saved->listing_buffer_start;
	listing_line_pointer	= saved->listing_line_pointer;
	advance_guard		= saved->advance_guard;
	listing_buffer_size	= saved->listing_buffer_size;
	listing_now		= saved->listing_now;
	listing_include		= saved->listing_include;
	free(dummy);
}

/*
 * bump_listing_lineid
 *
 * Increment the line number ID: i.e. the LISTING line number as
 * opposed to the source code line number (physical_input_line)
 */
bump_listing_lineid()
{
	++listing_lineid;
}

report_listing_lineid()
{
	return listing_lineid;
}

/*
 * write_listing_pass1_source_line
 *
 * Write from:  the current input buffer location to and inluding the 
 * 		first newline encountered.
 * Write to: 	the pass1 temp file (for user source code)
 * Exception:   if "string" arg is non-NULL, just write the string to
 *		the temp file and ignore the input buffer.
 */
write_listing_pass1_source_line(string)
char *string;
{
	char 	*c = listing_line_pointer;

	if ( string )
	{
		fputs ( string, listing_pass1_src );
		bump_listing_lineid();
	}
	else
	{
		if ( c == NULL )
			as_fatal ("Internal: listing input buffer NULL");
		
		for ( ; *listing_line_pointer && listing_line_pointer < advance_guard; ++listing_line_pointer )
		{
			/* FIXME: TEMP: the test against 0 should not be needed.
			 * Convince yourself that it is not needed and remove it.
			 */
			putc ( *listing_line_pointer, listing_pass1_src );
			if ( *listing_line_pointer == '\n' )
			{
				++listing_line_pointer;
				bump_listing_lineid();
				return;
			}
		}
		if ( *listing_line_pointer == 0 )
			as_fatal ("Internal: listing input buffer inconsistent");
	}
}

/*
 * clear_listing_pass1_source_line
 *
 * Similar to the write_listing_... version, only we don't actually 
 * write to the file.  This is needed because we must keep echoing 
 * chars into the listing buffer even when .nolist is in effect.  
 *
 */
clear_listing_pass1_source_line()
{
	char * c = listing_line_pointer;
	if ( c == NULL )
		as_fatal ("Internal: listing input buffer NULL");
	
	for ( ; *listing_line_pointer && listing_line_pointer < advance_guard; ++listing_line_pointer )
	{
		/* FIXME: TEMP: the test against 0 should not be needed.
		 * Convince yourself that it is not needed and remove it.
		 */
		if ( *listing_line_pointer == '\n' )
		{
			++listing_line_pointer;
			return;
		}
	}
	if ( *listing_line_pointer == 0 )
		as_fatal ("Internal: listing input buffer inconsistent");
}


/*
 * "PASS 2" (output)
 *
 * Re-open the temp files for input, write the listing file.
 *
 */

/*
 * listing_pass2_begin
 * 
 * Input processing is done.  No fatal errors have occurred.  Re-open 
 * the temp files for update, set listing_now permanently to "yes".
 * Set a "pass2 active" flag for the error handlers.  Initialize the
 * los struct internals.
 */
listing_pass2_begin()
{
	fclose ( listing_pass1_src );
	fclose ( listing_pass1_info );
#ifdef	DOS
	/* open for update in text mode */
	if ( (listing_pass1_src = fopen(listing_pass1_src_name, "r+t")) == NULL )
		as_fatal ("Internal: Pass 2: can't open listing temp file 1: %s", listing_pass1_src_name);
	/* open for update in binary mode */
	if ( (listing_pass1_info = fopen(listing_pass1_info_name, "r+b")) == NULL )
		as_fatal ("Internal: Pass 2: can't open listing temp file 2: %s", listing_pass1_info_name);
#else
	if ( (listing_pass1_src = fopen(listing_pass1_src_name, "r+")) == NULL )
		as_fatal ("Internal: Pass 2: can't open listing temp file 1: %s", listing_pass1_src_name);
	if ( (listing_pass1_info = fopen(listing_pass1_info_name, "r+")) == NULL )
		as_fatal ("Internal: Pass 2: can't open listing temp file 2: %s", listing_pass1_info_name);
#endif	
	listing_now = 1;
	listing_pass2 = 1;
	los.srcbuf = (char *) xmalloc (SRCBUF_SIZE);
	/* IMPORTANT: the special char at the end of srcbuf is used
	 * to tell in pass2 if the srcbuf is filled up.  Set it to 
	 * anything other than 0.
	 */
	los.srcbuf[SRCBUF_SIZE - 1] = 0x42;
}

/*
 * write_listing_file
 *
 * Input: pass1 source temp file
 *        pass1 info temp file
 * Output: the permanent listing file
 *
 * This is the main worker for the backend.  Course through the two 
 * input files, merging them appropriately into the output listing file.
 *
 * Note: next_info_line is a macro, defined at the top of the file.
 *       next_source_line is a function, defined elsewhere in this file.
 */
write_listing_file()
{
	register int	i;		/* all-purpose */
	register char	*raw;		/* all-purpose */
	fragS	*fragP;		/* for fill-type frags */
	int	total;		/* for fill-type frags */
	int	savcodeindx, prechars;
	long	savdot, savnaddr;

	rewind (listing_pass1_src);
	rewind (listing_pass1_info);

	write_listing_header();
	clear_listinfo();
	next_info_line();

	while ( next_source_line() )
	{
		if ( ! los.info_full || listinfo.lineid > los.curr_line )
		{
			/* This is a source-only line: there is no info struct for it. */
			++los.srcline;
			dump_listing_line();
		}
		else
		{
			/* We have some info for this line; in general, keep
			 * looping and reading more info lines while the info
			 * continues to refer to the same source line.
			 */
			los.srcline = listinfo.srcline;

			while ( los.info_full && listinfo.lineid == los.curr_line )
			{
				fragP = (fragS *) listinfo.frag;

				switch ( listinfo.status )
				{
				case varying:
					if ( fragP->fr_subtype == 1 )
						/* This is a cobr instruction 
						 * that did not relax; hence the size
						 * field is wrong.  Otherwise treat 
						 * just like a "complete"
						 */
						listinfo.size = 4;
					/* Deliberate fallthrough */
				case complete:
					raw = listinfo.fragptr;
					for ( i = 0; i < listinfo.size; ++i )
						write_machine_byte ( raw, i, listinfo.byteswap );
					next_info_line();
					break;
				case fill:
					raw = fragP->fr_literal + fragP->fr_fix;
					if ( fragP->fr_var < 0 )
					{
						/* If fr_var is negative, it is a flag from 
						 * s_alignfill() that we have the special form
						 * of an rs_align here.  (.alignfill)  Get the 
						 * total number of bytes to fill from the frag
						 * addresses.
						 */
						total = fragP->fr_next->fr_address - los.naddr;
					}
					else
					{
						/* The normal case.  fr_offset is the repeat 
						 * factor for a memory chunk fr_var bytes long.
						 */
						total = fragP->fr_offset * fragP->fr_var;
					}

					/* Note: the listinfo size will typically be
					 * much smaller than the total size.  If the total
					 * size is > 16, don't print a s***load of zeroes
					 * into the listing; Print the first 16 bytes, then
					 * use a shorthand to indicate the rest.
					 */
					savdot = los.dot;
					savnaddr = los.naddr;
					savcodeindx = los.codeindx;

					/* calculate the number of bytes to allow before we
					 * go to the shorthand.  End the preamble on a double
					 * word boundary just for looks.
					 */
					prechars = 8 + ( 16 - savcodeindx ) / 2;
					if ( total > prechars + 8 )
					{
						for ( i = 0; i < prechars; ++i )
							write_machine_byte ( raw, i % listinfo.size, listinfo.byteswap );
						fprintf ( listing_fp, redundant_fill_line,
							 los.srcline, los.dot );
						los.dot = savdot + total;
						los.naddr = savnaddr + total;
						los.codeindx = (savcodeindx + ( total * 2 )) % 16;
						sync_listing_line();
					}
					else
					{
						/* Not big enough to mess with the shorthand;
						 * just print 'em all 
						 */
						for ( i = 0; i < total; ++i )
							write_machine_byte ( raw, i % listinfo.size, listinfo.byteswap );
					}
					next_info_line();
					break;
				case info:
					/* byteswap is a flag, to tell you what to do. */
					switch ( (enum lst_info_type) listinfo.byteswap )
					{
					case segment_change:
						update_dot();
						/* Do the following, in case the code stream didn't
						   end on a word boundary */
						los.codeindx = (los.dot * 2) % 8;
						break;
					case source_change:
						/* The frag pointer has been usurped (hacked)
						 * to point to the file name of the new source
						 * file.  Print it in its own format.
						 */
						fprintf ( listing_fp, source_change_line, listinfo.frag );
						break;
					case include_start:
						/* If you are here, then there was an include file
						 * we did NOT list because of a -Ln option.  Just
						 * sync up dot, and dump this line.
						 */
						update_dot();
						dump_listing_line();
						break;
					case bss:
						/* The frag pointer points to a symbol.  Print its
						 * value in the "dot" field.  Just push-and-pop the
						 * current dot field.
						 */
#ifdef OBJ_ELF
#define GET_LISTING_SYMBOL_VALUE(SYM) (S_GET_VALUE(SYM) + SYM->sy_frag->fr_address)
#else
#define GET_LISTING_SYMBOL_VALUE(SYM) (S_GET_VALUE(SYM))
#endif
						savdot = los.dot;
						los.dot = GET_LISTING_SYMBOL_VALUE(((symbolS *) listinfo.frag));
						dump_listing_line();
						los.dot = savdot;
						break;
					case eject:
						/* Put a formfeed into the listing */
						dump_listing_line();
						fprintf ( listing_fp, "%c\n", FORMFEED_CHAR );
						break;
					case error:
						/* An error message was forced into the source 
						 * file, masquerading as a source line.  Print
						 * it on its own line.  Note that the source 
						 * line is now off by one so adjust it.
						 */
						fprintf ( listing_fp, "%s", los.srcbuf );
						los.src_full = 0;
						--los.srcline;
						break;
					default:
						as_fatal ("Internal: unknown listing info line");
					}
					next_info_line();
					break;
				default:
					as_fatal ("Internal: listing info temp file corrupted");
				}
			}
			if ( los.src_full || los.partial )
			{
				dump_listing_line();
				sync_listing_line();  /* move blocks of data from the middle
						       * column over to the far left 
						       */
			}
		}
	}
}

/* 
 * clear_listinfo
 *
 * Set global listinfo struct to the default state
 */
clear_listinfo()
{
	listinfo.status = 0;
	listinfo.srcline = listinfo.lineid = listinfo.size = listinfo.byteswap = 0;
	listinfo.fragptr = listinfo.frag = NULL;
}

/*
 * clear_codestr
 *
 * Fill the output code buffer with blanks.
 */
static
clear_codestr()
{
	memcpy ( los.codestr, blanks, 16 );
}

/*
 * update_dot
 * 
 * Sync up the listing output struct field "dot" with reality.
 * (i.e. with the current code frag.)
 */
update_dot()
{
	fragS 	*fragP = (fragS *) listinfo.frag;
	long	newdot;

	if ( fragP )
		newdot = fragP->fr_address + (listinfo.fragptr ? listinfo.fragptr - fragP->fr_literal : 0);
	else
		newdot = 0;
	los.dot = los.naddr = newdot;
	if ( newdot == (newdot & ~ 0x3) )
	{
		/* newdot is word-aligned; save yourself some frustration
		 * later by aligning the output buffer index now.
		 */
		if ( los.partial == 0 )
		{
			clear_codestr();
			los.codeindx = 0;
		}
	}
	return  newdot;
}

/*
 * next_source_line
 *
 * Read the next line from the input source temp file.
 * Return 1 if OK, 0 if at EOF.
 */
next_source_line()
{
	if ( fgets(los.srcbuf, SRCBUF_SIZE, listing_pass1_src) == NULL )
	{
		if ( feof ( listing_pass1_src ) )
			return 0;
		else 
			as_fatal ("Internal: error reading listing temp file");
	}
	/* Check for the unthinkable: a source line too long 
	 * to fit in the source buffer.
	 */
	if ( los.srcbuf[SRCBUF_SIZE - 1] == '\0' )
	{
		/* Unbelievable: the entire buffer is full.
		 * FIXME-SOMEDAY: we could realloc the buffer and read
		 * new chars into it.  99.99 % of the time it will be
		 * good enough to just throw away chars until the input
		 * file is consistent again.
		 */
		register char ch;
		los.srcbuf[SRCBUF_SIZE - 3] = '\n';
		los.srcbuf[SRCBUF_SIZE - 2] = '\0';
		los.srcbuf[SRCBUF_SIZE - 1] = 0x42;
		while ( (ch = getc (listing_pass1_src)) != EOF )
			if ( ch == '\n' )
				break;
	}
	los.src_full = 1;
	++los.curr_line;
	return 1;
}
		

/*
 * write_machine_byte
 *
 * write one machine byte to the stringized global buffer.
 * Handle byte swapping if necessary.  Update "naddr" (next address)
 *
 * Input: a pointer to a character buffer containing machine bytes.
 *        an integer offset within the input buffer
 *        a flag to tell me whether to byte-swap within a one-word field.
 * 
 * Assumes that there is room in the buffer for the new byte.
 * If the buffer becomes full after adding the new byte, the 
 * entire line will be dumped.
 *
 * Byteswapping alogorithm: very simplistic.  Within a group of 4 bytes,
 * the smallest becomes the largest, the largest becomes the smallest,
 * and the 2 middles swap.  There is NO byte swapping for containers 
 * larger than 4 bytes.
 */
write_machine_byte( raw, offset, byteswap )
char *raw;
int offset;
int byteswap;
{
	raw += offset;
	if ( byteswap )
		raw += (3 - ((offset % 4) * 2));
	
	machine_char_to_hex( &los.codestr[los.codeindx], *raw & 0xff );
	++los.naddr;
	los.partial = 1;
	if ( (los.codeindx += 2) == 16 )
	{
		dump_listing_line();
		los.codeindx = 0;
	}
}

/*
 * machine_char_to_hex
 *
 * Convert a binary byte into a pair of consecutive ascii bytes.
 */
machine_char_to_hex(buf, ch)
char *buf;
int ch;
{
	*buf = hex_digits [ch / 16];
	*(buf + 1) = hex_digits [ch % 16];
}

/*
 * dump_listing_line
 *
 * The 4 fields are guaranteed correct.  Write them out to the 
 * output file.  Reset the source buffer, clear the code buffer,
 * reset the code buffer index.
 */
dump_listing_line()
{
	static char 	word1[10];
	static char	word2[10];
	register int	i;

	for ( i = 0; i < 8; ++i )
		word1[i] = los.codestr[i];
	for ( i = 0; i < 8; ++i )
		word2[i] = los.codestr[i + 8];
	fprintf ( listing_fp, complete_line, los.srcline, los.dot, 
		 word1, word2, los.srcbuf );

	los.srcbuf[0] = '\n';
	los.srcbuf[1] = '\0';
	los.partial = 0;
	los.src_full = 0;
	los.dot = los.naddr;
	clear_codestr();
}

/*
 * sync_listing_line
 *
 * The listing will look nicer if blocks of code or data that 
 * begin on a word-aligned boundary start in the far left column,
 * rather than the middle column.
 */
sync_listing_line()
{
	if ( los.codeindx == 8 && los.partial == 0 )
	{
		clear_codestr();
		los.codeindx = 0;
	}
}

/*
 * The header.
 *
 * If the listing title was not specified, use the name of the primary 
 * source file as the title.
 * 
 * Print the title on a line by itself, followed by a line containing
 * the assembler's banner, and (optionally) a line containing the time 
 * and date of assembly.
 *
 * Don't print the time/date if the -z option was given.
 */
write_listing_header()
{
	long	julian_date;
	if ( listflagseen['z'] )
		return;
	if ( listing_title )
		fprintf ( listing_fp, title_format, listing_title );
	else
		fprintf ( listing_fp, default_title_format, listing_default_title );
	fprintf ( listing_fp, version_format, gnu960_get_version() );
	if ( ! flagseen['z'] )
	{
		time ( &julian_date );
		fprintf ( listing_fp, timedate_format, ctime ( &julian_date ));
		fprintf ( listing_fp, cmdline_format, listing_command_line );
	}
	fprintf ( listing_fp, error_rpt_format, had_errors() );
	fprintf ( listing_fp, warning_rpt_format, had_warnings() );
}

/*
 * listing_end
 *
 * Kill the 2 temporary listing files.
 * NOTE: This must be enterable whether we are making a listing or not.
 * If the fatal_flag is set, then also delete the listing file itself.
 * (typically called by as_fatal())
 */
listing_end( fatal_flag )
int fatal_flag;
{
	if ( flagseen['L'] && ! flagseen['X'] && listing_pass1_src_name )
	{
		if ( listing_pass1_src ) 
			fclose ( listing_pass1_src );
		unlink ( listing_pass1_src_name );
	}
	if ( flagseen['L'] && ! flagseen['X'] && listing_pass1_info_name )
	{
		if ( listing_pass1_info ) 
			fclose ( listing_pass1_info );
		unlink ( listing_pass1_info_name );
	}
	if ( flagseen['L'] && fatal_flag && listing_file_name )
	{
		if ( listing_fp )
			fclose ( listing_fp );
		unlink ( listing_file_name );
	}
}


/*
 * Error-handling (put error and warning messages into the listing)
 *
 */

listing_as_where( fn, line )
char *fn;
long line;
{
	sprintf(listing_error_buf, "%s:%u: ", fn, line);
}

listing_append_error_buf( str )
char *str;
{
	strcat ( listing_error_buf, str );
}

write_listing_error_buf()
{
	if ( *listing_error_buf )
	{
		/* Append the temp buf written by as_bad, as_warn, etc */
		strcat ( listing_error_buf, listing_error_tmpbuf );
		strcat ( listing_error_buf, "\n");
		write_listing_pass1_source_line ( listing_error_buf );
		listing_info ( info, 0, NULL, NULL, error );
		*listing_error_buf = '\0';
		*listing_error_tmpbuf = '\0';
	}
}


#ifdef DEBUG
/*
 * DEBUGGING utilities (not part of the released assembler)
 * For dumping the pass1 temporary files.
 */

/* For dumping the pass1 line info */
char 	*lst_status_str[4] =
{
	"complete",
	"varying",
	"fill",
	"info"
};

/* For dumping the pass1 line info "info" types */
char	*lst_info_type_str[6] =
{
	"section_change",
	"source_change",
	"include_start",
	"bss",
	"eject",
	"error",
};

dump_pass1_listing(arg)
int arg;
{
	char	long_buff[SRCBUF_SIZE];
	int	info_waiting, line_count;
	
	rewind (listing_pass1_src);
	rewind (listing_pass1_info);

	switch ( arg )
	{
	case 1:	/* just the source file */
		while ( fgets(long_buff, SRCBUF_SIZE, listing_pass1_src) )
			fputs(long_buff, stdout);
		break;

	case 2: /* just the info file */
		while ( read_listinfo() )
			dump_listinfo();
		break;

	case 3: /* mix the two */
		info_waiting = 0;
		line_count = 0;
		while ( fgets(long_buff, SRCBUF_SIZE, listing_pass1_src) )
		{
			++line_count;
			fputs(long_buff, stdout);
			if ( ! info_waiting )
			{
				if ( read_listinfo() )
					info_waiting = 1;
				else
					continue;
			}
			if ( listinfo.lineid == line_count )
			{
				while ( listinfo.lineid == line_count )
				{
					dump_listinfo();
					info_waiting = 0;
					if ( read_listinfo() )
						info_waiting = 1;
					else
						break;
				}
			}
			else
				info_waiting = 1;
		}
	}
}


/*
 * dump_listinfo()
 * 
 * dump the local listinfo_str struct in a readable format.
 */
dump_listinfo()
{
	static char word1[10];
	static char word2[10];
	register int i;

	switch ( listinfo.status )
	{
	case info:
		printf ("%d %-10s%d %d %s 0x%x 0x%x\n", 
			listinfo.lineid,
			lst_status_str[listinfo.status],
			listinfo.srcline,
			listinfo.size,
			lst_info_type_str[listinfo.byteswap],
			listinfo.frag,
			listinfo.fragptr);
		break;
	default:
		for ( i = 0; i < 4; ++i )
			write_machine_byte ( listinfo.fragptr, i, word1 );
		for ( ; i < 8; ++i )
			write_machine_byte ( listinfo.fragptr, i, word2 );
		word1[8] = 0;
		word2[8] = 0;
		printf ("%d %d %-10s %s %d %s %s 0x%x 0x%x\n", 
			listinfo.lineid,
			listinfo.srcline,
			lst_status_str[listinfo.status],
			listinfo.byteswap ? "swap" : "noswap",
			listinfo.size,
			word1,
			word2,
			listinfo.frag,
			listinfo.fragptr);
	}
}

#endif  /* DEBUG */
