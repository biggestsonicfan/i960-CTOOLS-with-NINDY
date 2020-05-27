/* output-file.c -  Deal with the output file
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

/* static const char rcsid[] = "$Id: out_file.c,v 1.22 1995/06/22 22:55:47 rdsmithx Exp $"; */

/*
 * Confines all details of emitting object bytes to this module.
 * All O/S specific crocks should live here.
 * What we lose in "efficiency" we gain in modularity.
 * Note we don't need to #include the "as.h" file. No common coupling!
 */

 /* note that we do need config info.  xoxorich. */

/* #include "style.h" */
#include <stdio.h>

#include "as.h"

/* For SEEK_SET, not in stdio.h on sun4 */
#if !defined(WIN95) && !defined(__HIGHC__) && !defined(M_AP400) && !defined(M_GCC960) /* Microsoft C 9.00 (Windows 95), Metaware C, Apollo 400 and GCC960. */
#include <unistd.h>
#endif

#include "obstack.h"

#ifdef GNU960
#	include "out_file.h"
#	include "dio.h"
#else
#	include "output-file.h"
#endif

static FILE *stdoutput;
extern char *physical_input_file;
extern char *physical_input_filename;

#ifdef	DOS
char *protected_extensions[] = { ".s", ".as", ".asm", ".S", ".AS", ".ASM", NULL };
#else
char *protected_extensions[] = { ".s", ".as", ".asm", NULL };
#endif
char *asm_extensions[] = { ".s", ".as", NULL };

/* 	output_file_setname()
 *
 *	Name the output file based on the name of the input file.
 *	Rules:  The user's -o option overrides everything else.  You will 
 *	know that this has been done if out_file_name is non-NULL (it would
 *	have been set in main() in as.c.)  If so, return.
 *
 * 	Algorithm for Unix hosts:
 *      If the input file name ends in ".s" or ".as" replace 
 *      the extension with ".o".  Otherwise append ".o" to whatever the
 *	input name is.
 *
 *	Algorithm for DOS:
 *	If the input file has any extension, replace it with ".o".  Otherwise 
 *	append ".o" to the input file name.
 *
 *      Variable physical_input_filename is assigned in input_scrub_new_file().
 */

void output_file_setname()
{
	if ( flagseen['L'] )
		/* We're listing; set up the default title */
		set_listing_default_title (physical_input_file, physical_input_filename);
		
	if ( out_file_name )
		return;
	
	if ( ! strcmp (physical_input_filename, STDIN_FILENAME) )
	{
		/*  No physical input file; use "a.out" for COFF files, 
		 *  "b.out" for BOUT files 
		 */   
		out_file_name = OBJ_DEFAULT_OUTPUT_FILE_NAME;
	}
	else
	{
		char *c;
		int  i;
		out_file_name = (char *) obstack_alloc (&notes, strlen (physical_input_filename) + 3);
		strcpy (out_file_name, physical_input_filename);
		if ( c = (char *) strrchr(out_file_name, '.') )
		{
#ifdef  DOS
			*c = 0;
#else
			for ( i = 0; asm_extensions[i]; ++i ) 
				if ( ! strcmp(c, asm_extensions[i]) )
				{
					*c = 0;
					break;
				}
#endif
		}
		strcat (out_file_name, ".o");
	}
} /* output_file_setname() */	
	

/* 	output_file_checkname()
 *
 *	Called from command_line() when the -o option is given. 
 * 	This is to be compatible with past CTOOLS assemblers that did
 *	not allow you to blow away an existing file when the existing
 *	file ends in .s, .as, or .asm.
 */
void
output_file_checkname()
{
	int	i = 0;
	char	*c = (char *) strrchr(out_file_name, '.'); 
	if ( c )
	{
		for ( ; protected_extensions[i]; ++i ) 
			if ( ! strcmp(c, protected_extensions[i]) )
			{
				FILE	*fp = fopen(out_file_name, "r"); 
				if ( fp )
				{
					fclose(fp);
					as_fatal("Output file will overwrite existing protected file.");
				}
				break;
			}
	}
} /* output_file_checkname() */


void output_file_create(name)
char *name;
{
	if ( name[0]=='-' && name[1]=='\0' )
		stdoutput = stdout;
	else
	{
#ifdef ASM_SET_CCINFO_TIME
		remember_old_output (name);
#endif
		stdoutput = fopen( name, FWRTBIN );
		if (stdoutput == 0)
			as_fatal ("Can't create output file: %s", name);
	}
} /* output_file_create() */



void output_file_close(filename)
	char *filename;
{
	if ( EOF == fclose( stdoutput ) )
		as_fatal ("Can't close %s", filename);

	stdoutput = NULL;		/* Trust nobody! */
	
	/* Check for errors in the back end */
	if ( had_errors() && ! flagseen['Z'] )
		unlink(filename);
} /* output_file_close() */


#ifdef ASM_SET_CCINFO_TIME
/* Remember the offset in the output file of the start of the ccinfo */
extern long ccinfo_offset;
void set_ccinfo_offset ()
{
	ccinfo_offset = ftell (stdoutput);
}
#endif

void output_file_append(buf, length, filename)
char *buf;
long length;
char *filename;
{
	for (; length; length--,buf++)
	{
		putc(*buf, stdoutput);
		if ( ferror(stdoutput) )
			as_fatal("Output write failed for object file: %s", filename);
	}
} /* output_file_append() */


/* 
 * Seek from the BEGINNING of the output file.
 * Leave file pointer at desired offset.
 */
void output_file_seek(offset, filename)
long offset;
char *filename;
{
	if ( fseek(stdoutput, offset, SEEK_SET) == -1 )
		as_fatal("Fseek failed for output file: %s", filename);
} /* output_file_seek() */

/* end: output-file.c */



