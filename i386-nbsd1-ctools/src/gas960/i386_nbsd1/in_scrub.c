/* input_scrub.c - Break up input buffers into whole numbers of lines.
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

/* static const char rcsid[] = "$Id: in_scrub.c,v 1.17 1994/03/12 00:25:46 peters Exp $"; */

#include <errno.h>	/* Need this to make errno declaration right */
#include "as.h"
#ifdef GNU960
#	include "in_file.h"
#else
#	include "input-file.h"
#endif

#include "obstack.h"

/*
 * O/S independent module to supply buffers of sanitised source code
 * to rest of assembler. We get sanitized input data of arbitrary length.
 * We break these buffers on line boundaries, recombine pieces that
 * were broken across buffers, and return a buffer of full lines to
 * the caller.
 * The last partial line begins the next buffer we build and return to caller.
 * The buffer returned to caller is preceeded by BEFORE_STRING and followed
 * by AFTER_STRING, as sentinels. The last character before AFTER_STRING
 * is a newline.
 * Also looks after line numbers, for e.g. error messages.
 */

/*
 * We don't care how filthy our buffers are, but our callers assume
 * that the following sanitation has already been done.
 *
 * No comments, reduce a comment to a space.
 * Reduce a tab to a space unless it is 1st char of line.
 * All multiple tabs and spaces collapsed into 1 char. Tab only
 *   legal if 1st char of line.
 * # line file statements converted to .line x;.file y; statements.
 * Escaped newlines at end of line: remove them but add as many newlines
 *   to end of statement as you removed in the middle, to synch line numbers.
 */
 
#define BEFORE_STRING ("\n")
#define AFTER_STRING ("\0")	/* bcopy of 0 chars might choke. */
#define BEFORE_SIZE (1)
#define AFTER_SIZE  (1)

#ifdef DOS
#define INCLUDE_STACK_LIMIT 20  /* number of nested includes without a return */
#else
#define INCLUDE_STACK_LIMIT 50
#endif

static char *	buffer_start;	/*->1st char of full buffer area. */
static char *	partial_where;	/*->after last full line in buffer. */
static int partial_size;	/* >=0. Number of chars in partial line in buffer. */
static char save_source [AFTER_SIZE];
				/* Because we need AFTER_STRING just after last */
				/* full line, it clobbers 1st part of partial */
				/* line. So we preserve 1st part of partial */
				/* line here. */
static int buffer_length;	/* What is the largest size buffer that */
				/* input_file_give_next_buffer() could */
				/* return to us? */

/* Saved information about the file that .include'd this one.  When we
   hit EOF, we automatically pop to that file. */

static char *next_saved_file;

/* To track include file nesting */

static int include_depth;

/*
We can have more than one source file open at once, though the info for
all but the latest one are saved off in a struct input_save.  These
files remain open, so we are limited by the number of open files allowed
by the underlying OS.
We may also sequentially read more than one source file in an assembly.
 */


/*
We must track the physical file and line number for error messages.
We also track a "logical" file and line number corresponding to (C?)
compiler source line numbers.  NOTE: As of CTOOLS 4.0 we no longer track 
the logical line number but I am leaving the code so it can be easily done 
if desired.

Whenever we open a file we must fill in physical_input_file. So if it is NULL
we have not opened any files yet.
*/

char 		*physical_input_file;
char		*physical_input_filename;
static char 	*logical_input_file;

typedef unsigned int line_numberT;	/* 1-origin line number in a source file. */
                                        /* A line ends in '\n' or eof. */

static
line_numberT	physical_input_line,
		logical_input_line;

/* Struct used to save the state of the input handler during include files */
struct input_save {
	char *buffer_start;
	char *partial_where;
	int partial_size;
	char save_source [AFTER_SIZE];
	int buffer_length;
	char *physical_input_file;	/* full pathname */
	char *physical_input_filename;  /* just the basename */
	char *logical_input_file;
	line_numberT	physical_input_line;
	line_numberT	logical_input_line;
	char *next_saved_file;	/* Chain of input_saves */
	char *input_file_save;	/* Saved state of input routines */
	char *listing_save;	/* Saved state of partial listing buffer */
	char *saved_position;	/* Caller's saved position in buf */
};

#ifdef  DEBUG
dump_input()
{
	printf("buffer_start = 0x%x\n", buffer_start);
	printf("buffer_length  = %d\n", buffer_length);
	printf("physical_input_file = %s\n", physical_input_file);
	printf("logical_input_file = %s\n", logical_input_file ? logical_input_file : "null");
	printf("physical_input_line = %d\n", (int) physical_input_line);
	printf("logical_input_line = %d\n", (int) logical_input_line);
	printf("partial_where = 0x%x\n", partial_where);
	printf("partial_size = %d\n", partial_size);
	printf("next_saved_file = 0x%x\n", next_saved_file);
	printf("save_source = %c\n", save_source[0]);
}
#endif

#ifdef __STDC__
static void as_1_char(unsigned int c, FILE *stream);
#else /* __STDC__ */
static void as_1_char();
#endif /* __STDC__ */

/* Push the state of input reading and scrubbing so that we can #include.
   The return value is a 'void *' (fudged for old compilers) to a save
   area, which can be restored by passing it to input_scrub_pop(). */
char *
input_scrub_push(saved_position)
	char *saved_position;
{
	register struct input_save *saved;
	static int count;
#ifdef DEBUG
	printf ("input_scrub_push: %d\n", ++count);
#endif
	if ( ++include_depth > INCLUDE_STACK_LIMIT )
	    as_fatal (".include recursion too deep");

	saved = (struct input_save *) xmalloc(sizeof *saved);

	saved->saved_position		= saved_position;
	saved->buffer_start		= buffer_start;
	saved->partial_where		= partial_where;
	saved->partial_size		= partial_size;
	saved->buffer_length 		= buffer_length;
	saved->physical_input_file	= physical_input_file;
	saved->physical_input_filename	= physical_input_filename;
	saved->logical_input_file	= logical_input_file;
	saved->physical_input_line	= physical_input_line;
	saved->logical_input_line	= logical_input_line;
	bcopy (save_source, saved->save_source,	sizeof (save_source));
	saved->next_saved_file		= next_saved_file;
	saved->input_file_save		= input_file_push ();
	saved->listing_save		= listing_push ();

	input_scrub_begin ();		/* Reinitialize! */

	return (char *)saved;
}

char *
input_scrub_pop (arg)
	char *arg;
{
	register struct input_save *saved;
	char *saved_position;

#ifdef DEBUG
	printf ("input_scrub_pop\n");
#endif
	if ( --include_depth < 0 )
	    /* Internal error; should never happen */
	    include_depth = 0;

	input_scrub_end ();	/* Finish off old buffer */

	saved = (struct input_save *)arg;

	input_file_pop 		 (saved->input_file_save);
	listing_pop		 (saved->listing_save);
	saved_position		= saved->saved_position;
	buffer_start		= saved->buffer_start;
	buffer_length 		= saved->buffer_length;
	physical_input_file	= saved->physical_input_file;
	physical_input_filename	= saved->physical_input_filename;
	logical_input_file	= saved->logical_input_file;
	physical_input_line	= saved->physical_input_line;
	logical_input_line	= saved->logical_input_line;
	partial_where		= saved->partial_where;
	partial_size		= saved->partial_size;
	next_saved_file		= saved->next_saved_file;
	bcopy (saved->save_source, save_source, sizeof (save_source));

	free(arg);
	return saved_position;
}

 
void
input_scrub_begin ()
{
  know(strlen(BEFORE_STRING) == BEFORE_SIZE);
  know(strlen(AFTER_STRING) ==  AFTER_SIZE
	|| (AFTER_STRING[0] == '\0' && AFTER_SIZE == 1));

  input_file_begin ();

  buffer_length = input_file_buffer_size ();

  buffer_start = xmalloc((long)(BEFORE_SIZE + buffer_length + buffer_length + AFTER_SIZE));
  bcopy (BEFORE_STRING, buffer_start, (int)BEFORE_SIZE);

  /* Line number things. */
  logical_input_line = 0;
  logical_input_file = (char *)NULL;
  physical_input_file = NULL;	/* No file read yet. */
  physical_input_filename = NULL;
  next_saved_file = NULL;	/* At EOF, don't pop to any other file */

  do_scrub_begin();
}

void
input_scrub_end ()
{
  if (buffer_start)
    {
      free (buffer_start);
      buffer_start = 0;
      input_file_end ();
    }
}


/* 	basename()
 *
 *	A utility to find the file name in a full or relative path name.
 *	Returns a newly-allocated string.  Returns NULL if input path is NULL.
 *      Also works when path is non-NULL but zero-length (should return a
 *	non-NULL zero-length result).
 */
static char *basename (path)
char *path;
{
	register char 	*c;
	char		*result;

	if ( path == NULL ) return NULL;

	/* Work backwards in the string until the first pathname delimiter
	 * is found, or until the beginning of the path is found. 
	 */
#ifdef DOS
	for (c = path + strlen (path) - 1; c >= path; c--)
	{
		char ch = *c;
		if (ch == '\\' || ch == '/' || ch == ':')
		{
			c++;
			break;
		}
	}

	if (c == path - 1)
		c = path;
#else
	for ( c = path + strlen (path) - 1; c > path && *c != PATHNAME_DELIM; --c )
		;
	if ( c >= path && *c == PATHNAME_DELIM )
		++c;
#endif
	result = xmalloc (strlen(c) + 1);
	strcpy (result, *path ? c : path);
	return result;
} /* basename */


/* Start reading input from a new file.
 * Set up the physical file name and reset the physical line number.
 * Also, find the basename portion of the new file name.
 * This is what will be used in error messages.
 */
char *		/* Return start of caller's part of buffer. */
input_scrub_new_file (filename)
     char *	filename;
{
	input_file_open (filename);
	physical_input_file = filename[0] ? filename : STDIN_FILENAME;
	physical_input_filename = filename[0] ? basename (filename) : STDIN_FILENAME;

	physical_input_line = 0;
	/* PS: Don't reset logical_input_line here if you do it line-by-line
	 * by calling new_logical_line().
	 */
	logical_input_line = 0;

	partial_size = 0;

	if ( listing_now )
	{
		/* Make an "info" type info line for the new file, so that
		 * the file name will be reported in the listing.  Usurp 
		 * (kludge) the frag field to hold a pointer to the current
		 * source file name.
		 */
		char *fn = (char *) xmalloc ( strlen(physical_input_file) + 1 );
		strcpy (fn, physical_input_file);
		listing_info ( info, 0, NULL, fn, source_change );
	}

	return (buffer_start + BEFORE_SIZE);
}


/* Include a file from the current file.  Save our state, cause it to
   be restored on EOF, and begin handling a new file.  Same result as
   input_scrub_new_file. */

char *
input_scrub_include_file (filename, position)
     char *filename;
     char *position;
{
   next_saved_file = input_scrub_push (position);
   return input_scrub_new_file (filename);
}

void
input_scrub_close ()
{
  input_file_close ();
}


char *
input_scrub_next_buffer (bufp)
char **bufp;
{
	register char *	limit;	/*->just after last char of buffer. */
	
	*bufp = buffer_start + BEFORE_SIZE;
	
	
	if (partial_size)
	{
		bcopy (partial_where, buffer_start + BEFORE_SIZE, (int)partial_size);
		bcopy (save_source, buffer_start + BEFORE_SIZE, (int)AFTER_SIZE);
	}
	limit = input_file_give_next_buffer (buffer_start + BEFORE_SIZE + partial_size);
	if (limit)
	{
		register char *	p;	/* Find last newline. */
		
		for (p = limit;   * -- p != '\n';)
			;
		++ p;
		if (p <= buffer_start + BEFORE_SIZE)
		{
			as_fatal("Internal error. Input buffer inconsistent.");
		}
		partial_where = p;
		partial_size = limit - p;
		bcopy (partial_where, save_source,  (int)AFTER_SIZE);
		bcopy (AFTER_STRING, partial_where, (int)AFTER_SIZE);
	}
	else
	{
		partial_where = 0;
		if (partial_size > 0)
		{
			as_bad("Partial line at end of file");
		}

		/* If we should pop to another file at EOF, do it.  this
		 * will restore the parent's file name, the parent's line
		 * number, partial_where, etc.
		 */
		if (next_saved_file)
		{
			*bufp = input_scrub_pop (next_saved_file);
			if ( listing_now ) 
			{
				/* We've just returned from the include.  Globals are 
				 * returned to the state of the parent.
				 */
				if ( listflagseen['n'] )
				{
					/* We didn't list the include, so we won't write
					 * the parent's file name now.  But do a segment
					 * change to sync up the source line and address
					 * in the listing back end.
					 */
					listing_info ( info, 0, obstack_next_free(&frags), curr_frag, segment_change );
				}
				else
				{
					/* Put out an info line telling the back end to 
					 * print the parent's file name on a line by itself. 
					 */
					char *fn = (char *) xmalloc ( strlen(physical_input_file) + 1 );
					strcpy (fn, physical_input_file);
					listing_info ( info, 0, NULL, fn, source_change );
				}
			}
		}
	}
	return (partial_where);
} /* input_scrub_next_buffer */


/*
 * The remaining part of this file deals with line numbers, error
 * messages and so on.
 */


int
seen_at_least_1_file ()		/* TRUE if we opened any file. */
{
  return (physical_input_file != NULL);
}

void
bump_line_counters ()
{
  ++ physical_input_line;
  ++ logical_input_line;
}
 
long
report_line_number()
{
	return (long) physical_input_line;
}

/*
 *			new_logical_line()
 *
 * Tells us what the new logical line number and file are.
 * If the line_number is <0, we don't change the current logical line number.
 * If the fname is NULL, we don't change the current logical file name.
 */
void new_logical_line(fname, line_number)
     char *fname;		/* DON'T destroy it! We point to it! */
     int line_number;
{
	if (fname) {
		logical_input_file = fname;
	} /* if we have a file name */
	
	if (line_number >= 0) {
		logical_input_line = line_number;
	} /* if we have a line number */
} /* new_logical_line() */
 
/*
 *			a s _ w h e r e ()
 *
 * Write a line to stderr locating where we are in reading
 * input source files.
 * As a sop to the debugger of AS, pretty-print the offending line.
 */
void
as_where()
{
	char *p;
	line_numberT line;
	
	if (physical_input_file)
	{	/* we tried to read SOME source */
		if (input_file_is_open())
		{	/* we are currently processing input (i.e. in the 
			 * front, not the back end
			 */
			p = logical_input_file ? logical_input_file : physical_input_file;
			line = logical_input_line ? logical_input_line : physical_input_line;
			fprintf(stderr,"%s:%u: ", p, line);
			if ( listing_now && ! listing_pass2 ) 
				listing_as_where ( p, line );
		}
	}
}



 
/*
 *			a s _ h o w m u c h ()
 *
 * Output to given stream how much of line we have scanned so far.
 * Assumes we have scanned up to and including input_line_pointer.
 * No free '\n' at end of line.
 */
void
as_howmuch (stream)
     FILE * stream;		/* Opened for write please. */
{
  register char *	p;	/* Scan input line. */
  /* register char c; JF unused */

  for (p = input_line_pointer - 1;   * p != '\n';   --p)
    {
    }
  ++ p;				/* p->1st char of line. */
  for (;  p <= input_line_pointer;  p++)
    {
      /* Assume ASCII. EBCDIC & other micro-computer char sets ignored. */
      /* c = *p & 0xFF; JF unused */
      as_1_char(*p, stream);
    }
}

static void as_1_char (c,stream)
unsigned int c;
FILE *stream;
{
  if (c > 127)
    {
      (void)putc('%', stream);
      c -= 128;
    }
  if (c < 32)
    {
      (void)putc('^', stream);
      c += '@';
    }
  (void)putc(c, stream);
}


