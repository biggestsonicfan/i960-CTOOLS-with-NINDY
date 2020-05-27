/* input_file.c - Deal with Input Files -
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

/* static const char rcsid[] = "$Id: in_file.c,v 1.11 1994/08/11 19:19:59 peters Exp $"; */

/*
 * Confines all details of reading source bytes to this module.
 * All O/S specific crocks should live here.
 * What we lose in "efficiency" we gain in modularity.
 * Note we don't need to #include the "as.h" file. No common coupling!
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "as.h"
#ifdef GNU960
#	include "in_file.h"
#else
#	include "input-file.h"
#endif

/*
 * This code opens a file, then delivers INPUT_BUFFER_SIZE character
 * chunks of the file on demand.  (INPUT_BUFFER_SIZE defined in as.h)
 * INPUT_BUFFER_SIZE is supposed to be a number chosen for speed.
 * The caller only asks once what INPUT_BUFFER_SIZE is, and asks before
 * the nature of the input files (if any) is known.
 */

/*
 * We use static data: the data area is not sharable.
 */

FILE *f_in;
/* static JF remove static so app.c can use file_name */
char *	file_name;

/* Struct for saving the state of this module for file includes.  */
struct saved_file {
  FILE *f_in;
  char *file_name;
  char *app_save;
};

/* These hooks accomodate most operating systems. */

void input_file_begin() {
  f_in = (FILE *)0;
}

void input_file_end () { }

 /* Return INPUT_BUFFER_SIZE. */
int input_file_buffer_size() {
  return (INPUT_BUFFER_SIZE);
}

int input_file_is_open() {
  return f_in!=(FILE *)0;
}

/* Push the state of our input, returning a pointer to saved info that
   can be restored with input_file_pop ().  */
char *input_file_push () {
  register struct saved_file *saved;

  saved = (struct saved_file *)xmalloc (sizeof *saved);

  saved->f_in		= f_in;
  saved->file_name	= file_name;
  saved->app_save	= app_push ();

  input_file_begin ();	/* Initialize for new file */

  return (char *)saved;
}

void
input_file_pop (arg)
     char *arg;
{
  register struct saved_file *saved = (struct saved_file *)arg;

  input_file_end ();	/* Close out old file */

  f_in			= saved->f_in;
  file_name		= saved->file_name;
  app_pop(saved->app_save);

  free(arg);
}

void
input_file_open (filename)
     char *	filename;	/* "" means use stdin. Must not be 0. */
{
	int	c;
	char	buf[80];

	assert( filename != 0 );
	if (filename [0]) 
	{
#ifdef  DOS
		/* Open in binary mode, so that CTRL-Z's can be stripped.
		 * See also app.c:scrub_from_file()
		 */
		f_in = fopen(filename, "rb");
#else
		f_in = fopen(filename, "r");
#endif
		file_name=filename;
	} 
	else 
	{ /* use stdin for the input file. */
		f_in = stdin;
		file_name = STDIN_FILENAME;
	}
	if ( f_in == NULL )
		as_fatal ("Can't open %s for reading", file_name);

	/* Ask stdio to buffer our input at INPUT_BUFFER_SIZE, with a dynamically
	   allocated buffer.  */
	setvbuf(f_in, (char *)NULL, _IOFBF, INPUT_BUFFER_SIZE);

}


/* Close input file.
 * NOTE:  At this writing, Metaware has a bug in that you must not
 *        try to fclose one of the standard streams.  So check for this.
 */
void
input_file_close()
{
#if	defined(__HIGHC__)
  if ( f_in && f_in != stdin )
#else
  if ( f_in )
#endif
  {
    fclose (f_in);
    f_in = (FILE *)0;
  }
}

char *
input_file_give_next_buffer (where)
	char *		where;	/* Where to place 1st character of new buffer. */
{
	char *	return_value;	/* -> Last char of what we read, + 1. */
	register int	size;
	char *p;
	int n;
	int ch;
	extern FILE *scrub_file;
	
	if (f_in == (FILE *)0)
		return 0;
	/*
	 * fflush (stdin); could be done here if you want to synchronise
	 * stdin and stdout, for the case where our input file is stdin.
	 * Since the assembler shouldn't do any output to stdout, we
	 * don't bother to synch output and input.
	 */
	
	scrub_file=f_in;
	for (p = where, n = INPUT_BUFFER_SIZE; n; --n) 
	{
		ch = do_scrub_next_char( scrub_from_file, scrub_to_file );
		if (ch == EOF)
			break;
		*p++=ch;
	}
	size=INPUT_BUFFER_SIZE-n;
	if (size < 0)
		as_fatal ("Can't read from %s", file_name);

	if (size)
		return_value = where + size;
	else
	{
		input_file_close();
		return_value = 0;
	}
	return (return_value);
}
