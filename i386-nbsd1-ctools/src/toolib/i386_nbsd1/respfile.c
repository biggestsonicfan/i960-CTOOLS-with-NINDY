/******************************************************************/
/*       Copyright (c) 1994 Intel Corporation

   Intel hereby grants you permission to copy, modify, and
   distribute this software and its documentation.  Intel grants
   this permission provided that the above copyright notice
   appears in all copies and that both the copyright notice and
   this permission notice appear in supporting documentation.  In
   addition, Intel grants this permission provided that you
   prominently mark as not part of the original any modifications
   made to this software or documentation, and that the name of
   Intel Corporation not be used in advertising or publicity
   pertaining to distribution of the software or the documentation
   without specific, written prior permission.

   Intel Corporation provides this AS IS, without any warranty,
   including the warranty of merchantability or fitness for a
   particular purpose, and makes no guarantee or representations
   regarding the use of, or the results of the use
   of, the software and documentation in terms of correctness,
   accuracy, reliability, currentness, or otherwise; and you rely
   on the software, documentation and results solely at your own
   risk.                                                          */
/******************************************************************/

/* This module parses response files for Unix and DOS hosts.
 * Command line options of the form '@filename' are replaced
 * with the options found in 'filename'
 *
 * For DOS, wild card expansion is also performed, and stderr is
 * redirected to stdout if I960ERR is not set.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "i_toolib.h"

#if defined(DOS)
#include <dos.h>
#if defined(WIN95)
#include <io.h>
#endif
#endif


/* Define the final expanded argv vector.  */

static char	**newargv = NULL;
static int	newargvlength = 0;
static int	newargvmax = 0;

typedef struct {
	char	*file_name;
	FILE	*fp;
	int	line_number;	/* Current line number in file */
	char	*buf;		/* Resizable buffer containing current line */
	int	buf_size;
	char	*pos;		/* Current position in buf */
} Response_file;

#define LINE_SIZE 256

#define is_response_file(f)	((f) && *(f) == '@' && *((f)+1) != '\0')

static void parse_response_file();	/* Forward decl, indirect recursion. */

/*************************************************************************
 * xmalloc()
 *	malloc with error checking.
 ***************************************************************************/
static char *
xmalloc( n )
int n;
{
	char *p;

	p = (char*) malloc(n);
	if (p == NULL){
		(void) fprintf(stderr,
			"ERROR -- Out of memory in get_response_file()\n");
		exit(1);
	}
	return p;
}

/*
 * Append a string to an argv-like vector, expanding argv if necessary.
 * The string is used where it lies, it is not copied.
 */
static void
putinlist(name, list, length, max)
char	*name;
char	***list;
int	*length;
int	*max;
{
	if ((*length)+1 > *max)
	{
		/* Some unix hosts choke on realloc(NULL,...); */

		*max = (*max) + (*max >> 2) + 1; /* 25% growth */
		*list = (*list)
			? (char **) realloc(*list,(*max)*sizeof(char *))
			: (char **) xmalloc((int) ((*max)*sizeof(char *)));
		if (*list == NULL) {
			(void) fprintf(stderr,
			    "ERROR -- realloc failed in get_response_file()\n");
			exit(1);
		}
	}

	(*list)[(*length)] = name;
	*length = (*length) + 1;
}

#if defined(DOS)

static char*
copy_string(s)
char	*s;
{
	char	*t;

	if (s == NULL)
		return NULL;
	t = (char*) xmalloc(strlen(s) + 1);
	(void) strcpy(t, s);
	return t;
}

/* This is the function that qsort() uses to sort its list (see reference
 * in add_file_spec()).
 */
static int
strptrcmp(const void *l, const void *r)
{
    return strcmp(*(char**)l,*(char**)r);
}

/*
 * This function performs some cursory checks to determine whether this
 * string might possibly be a file spec, either wildcarded or not.
 */
static int
pos_file_spec_p(s)
char *s;
{
  char *t;

  /*
   * If the string starts with an option specifier character we
   * are going to say it can't be a filename.
   */
  if (IS_OPTION_CHAR(*s))
    return 0;

  /* We are not going to allow any file-specs to contain white space. */
  t = strchr(s, ' ');
  if (t != 0)
    return 0;

  return 1;
}

/* Does pattern matching on dos.  Expands *.* into a.exe b.exe etc.
 * Adds the sorted lowercase list to newargv.
 * 'arg' itself is never put in the list, so the caller can free arg
 * if not needed.
 */
static void
add_file_spec(arg)
char	*arg;
{
#if defined(WIN95)
    struct _finddata_t	c_file;
    long fh;
#else
    struct find_t	c_file;
#endif
    char		*filespec;

    filespec = copy_string(arg);

#if defined(WIN95)
    if ((fh = _findfirst(filespec, &c_file)) != -1)
#else
    if (_dos_findfirst(filespec, _A_NORMAL, &c_file) == 0)
#endif
    {
	char	buff[_MAX_DRIVE + _MAX_DIR + _MAX_FNAME + _MAX_EXT + 10];
	char	**list = (char **) 0;
	int	length = 0, max = 0;
	char	drive[_MAX_DRIVE];   /* Dummy for _splitpath.         */
	char	dir[_MAX_DIR];       /* Dummy for _splitpath.         */
	char	fname[_MAX_FNAME];   /* Dummy for _splitpath.         */
	char	ext[_MAX_EXT];       /* Buffer for extention of file. */

	_splitpath(filespec, drive, dir, fname, ext);
	sprintf(buff,"%s%s%s",drive,dir,c_file.name);
	putinlist(copy_string(buff), &list, &length, &max);

#if defined(WIN95)
	while (_findnext(fh, &c_file) == 0)
#else
	while (_dos_findnext(&c_file) == 0)
#endif
	{
	    sprintf(buff,"%s%s%s", drive, dir, c_file.name);
	    putinlist(copy_string(buff), &list, &length, &max);
	}

	/* If we found more than one match, or if we found a single match
	 * which differs in a way other than upper/lower case, then we can
	 * assume we have a legitimate file name and can treat it as such.
	 */

	if (!is_same_file_by_name(filespec,list[0]) || length > 1)
	{
		int	i;
		for(i=0; i < length; i++)
			(void) normalize_file_name(list[i]);
		qsort((void *)list, length, sizeof(char *), strptrcmp);
		for(i=0; i < length; i++)
		{
		    putinlist(list[i], &newargv, &newargvlength, &newargvmax);
		}
		free (filespec);
	}
	else
	{
		/* Only one file was found, filespec contained no wild card
		 * characters, and we don't want to normalize it.
		 * Use filespec instead of list[0], so that 'AbCd' is
		 * not inadvertantly "expanded" into 'abcd'.
		 */
		putinlist(filespec, &newargv, &newargvlength, &newargvmax);
		free(list[0]);
	}

	free(list);
    }
    else
    {
	    putinlist(filespec, &newargv, &newargvlength, &newargvmax);
    }
}

#endif /* DOS */

static void
response_file_error(rf, msg, arg)
Response_file	*rf;
char	*msg;
char	*arg;
{
	(void) fprintf(stderr, "COMMAND OPTION FILE ERROR: %s: ",rf->file_name);
	if (rf->line_number > 0)
		(void) fprintf(stderr, "line %d -- ", rf->line_number);
	(void) fprintf(stderr, "%s%s\n", msg, (arg ? arg : ""));
	if (rf->fp != NULL)
		(void) fclose(rf->fp);
	exit(1);
}

/* Read a line from the given response file and place it in the given buffer.
 * Resize the buffer if needed.
 * Return 1 if at EOF, otherwise 0.
 */
static int
fill_buffer(rf)
Response_file	*rf;
{
	int	length;

	if (fgets(rf->buf, rf->buf_size, rf->fp) == NULL) {
		if (!feof(rf->fp)) {
			response_file_error(rf,
				"I/O error during fgets", (char*) NULL);
		}
		rf->buf[0] = '\0';
		rf->pos = rf->buf;
		return 1;
	}

	/* Handle the case of buffer overflow.  We have overflowed if the
	/* last character read is not a new line, and we are not at EOF.
	 */

	length = strlen(rf->buf);

	while (rf->buf[length-1] != '\n')
	{
		char*	new_buf;

		/* Extend the buffer. */

		rf->buf_size += LINE_SIZE;

		new_buf = (char*) xmalloc(rf->buf_size);
		(void) strcpy(new_buf, rf->buf);
		free(rf->buf);
		rf->buf = new_buf;

		/* Read some more of the current line. */
		/* fgets will return NULL if we're at EOF. */

		if (fgets(rf->buf + length,		/* Start at last '\0' */
			LINE_SIZE + 1, rf->fp) == NULL)	/* Overwrite '\0'. */
		{
			if (!feof(rf->fp)) {
				response_file_error(rf,
					"I/O error during fgets", (char*) NULL);
			}
			rf->buf[length] = '\0';
			rf->pos = rf->buf;
			return 0;
		}

		length = strlen(rf->buf);
	}

	rf->buf[length-1] = '\0';	/* Zero new line */
	rf->pos = rf->buf;
	rf->line_number++;
	return 0;
}

/* Allocate space for and return the next command line option in
 * the given response file.  Return NULL when out of arguments.
 */

static char *
get_next_response_file_option(rf)
Response_file	*rf;
{
	char	*argument;		/* Argument to be returned */
	int	in_quotes;		/* Inside double quotes? */
	char	*chp;

	/* Locate the next line in the file that contains an option. */

	while (rf->pos == NULL || rf->pos[0] == '\0')
	{
		if (fill_buffer(rf))
			return NULL;
		while (*rf->pos && isspace(*rf->pos))
			rf->pos++;
		/* Ignore comments and empty lines */
		if (*rf->pos == '#' || *rf->pos == '\0')
			rf->pos = NULL;
	}

	if (!rf->pos)
		return NULL;

	/* Allocate space to return the argument. */

	argument = (char*)xmalloc((int) (strlen(rf->pos) + 1));
	argument[0] = '\0';
	in_quotes = 0;
	chp = rf->pos;

	/* NOTE:  Because a literal " in a response file needs to be
	 * escaped with another, as in   "i contain "" a quote"
	 * it is not possible to quote an argument that begins
	 * with a literal quote.  This is a design flaw.
	 */

	while (*chp) {
		if (*chp == '"')
		{
			if (*(chp+1) == '"')
			{
				(void) strcat(argument, "\"");
				chp += 2;
			}
			else if (in_quotes)
			{
				chp++;
				in_quotes = 0;
				break;
			}
			else
			{
				chp++;
				in_quotes = 1;
			}
		}
		else if (in_quotes || !isspace(*chp))
		{
			(void) strncat(argument, chp, 1);
			chp++;
		}
		else	/* white space terminating argument */
		{
			break;
		}
	}

	if (in_quotes)
		response_file_error(rf, "mismatched quotes: ", argument);

	while (*chp && isspace(*chp))
		chp++;
	rf->pos = chp;

	return argument;
}

/* Put the given option into newargv. */

static void
translate_option(option, free_option_if_unused)
char	*option;
int	free_option_if_unused;
	/* call free(option) if 'option' itself is not used. */
{
	if (is_response_file(option))
	{
		parse_response_file(option + 1);
		if (free_option_if_unused)
			free(option);
	}
#if defined(DOS)
	else if (pos_file_spec_p(option))
	{
		add_file_spec(option);
		if (free_option_if_unused)
			free(option);
	}
#endif
	else
		putinlist(option, &newargv, &newargvlength, &newargvmax);
}

/* Read the command line options from the specified file and add them to
 * newargv.  Lines beginning with '#' are treated as comments and ignored.
 * Command line options are separated by white space.  An option must be
 * enclosed in double quotes if it is to include white space.  If an option
 * includes a double quote, specify two double quotes in succession.
 */
static void
parse_response_file(fname)
char	*fname;
{
	Response_file	rf;
	char	*option;

	rf.file_name = fname;
	rf.line_number = 0;

	if ((rf.fp = fopen(fname, "r")) == NULL)
		response_file_error(&rf, "unable to open file", (char*)NULL);

	rf.buf = (char*) xmalloc(LINE_SIZE);
	rf.buf_size = LINE_SIZE;
	rf.buf[0] = '\0';
	rf.pos = rf.buf;

	while ((option = get_next_response_file_option(&rf)) != NULL)
	{
		translate_option(option, 1);
	}

	/* Issue an error if the end of the file has not been reached. */
	if (!feof(rf.fp))
		response_file_error(&rf, "unable to read file", (char*)NULL);

	(void) fclose(rf.fp);
	free(rf.buf);
}

/* Start of get_response_file() code.
 * The usage model is:
 *
 *	main(argc,argv)
 *	int argc; char *argv[];
 *	{
 *		argc = get_response_file(argc,&argv);
 *	...
 *	}
 */

int
get_response_file(argc,argvp)
int argc;
char ***argvp;
{
	int	i;

#if defined(DOS)
	/* Because stderr cannot be independently redirected from the
	 * DOS shell (command.com), make stderr go to stdout by default.
	 * Setting I960ERR overrides this behavior.
	 */

	if (getenv("I960ERR") == NULL)
	{
		if (_dup2(1,2) != 0) /* effectively directs stderr to stdout */
		{
		  (void) fprintf(stderr,
		    "System error:  Failed to redirect stderr to stdout\n");
		}
	}
#endif


#if !defined(DOS)
	/* Unix memory optimization: if no response files avoid allocating
	 * space for newarg.  Can't do this for DOS because we need
	 * to expand wild cards.
	 */

	for (i = 0; i < argc; i++)
	{
		if (is_response_file((*argvp)[i]))
			break;
	}

	if (i >= argc)
		return argc;	/* No response files */
#endif

	/* Allocate newargv to be at least as large as argvp */
	newargvmax = argc + 10;
	newargvlength = 0;
	newargv = (char **) xmalloc((int) (newargvmax * sizeof(char *)));

	for (i = 0; i < argc; i++)
	{
#if defined(DOS)
		if (i == 0)
		{	/* Just normalize argv[0], its the command name. */
			putinlist((*argvp)[i],
				&newargv, &newargvlength, &newargvmax);
			normalize_file_name(newargv[newargvlength-1]);
			continue;
		}
#endif
		translate_option((*argvp)[i], 0);
	}

	putinlist((char*) NULL, &newargv, &newargvlength, &newargvmax);
	newargvlength--;	/* Don't count NULL */

	*argvp = newargv;
	return newargvlength;
}
