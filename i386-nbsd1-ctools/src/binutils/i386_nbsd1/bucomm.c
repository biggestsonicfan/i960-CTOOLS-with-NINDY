/******************************************************************************
 * Copyright (C) 1985, 1990 Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 ******************************************************************************/

/* bucomm.c -- Bin Utils COMmon code.
 *	$Id: bucomm.c,v 1.12 1993/11/19 00:36:49 dramos Exp $
 */

#include "sysdep.h"
#include "bfd.h"

#if	defined(__STDC__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif

char *target = NULL;		/* default as late as possible */

/* Yes, this is what atexit is for, but that isn't guaranteed yet.
   And yes, I know this isn't as good, but it does what is needed just fine */
void (*exit_handler) ();

void mode_string ();
static char ftypelet ();
static void rwx ();
static void setst ();

/* make a string describing file modes */


/* filemodestring - fill in string STR with an ls-style ASCII
 * representation of the st_mode field of file stats block STATP.
 * 10 characters are stored in STR; no terminating null is added.
 * The characters stored in STR are:
 *
 * 0	File type.  'd' for directory, 'c' for character
 *	special, 'b' for block special, 'm' for multiplex,
 *	'l' for symbolic link, 's' for socket, 'p' for fifo,
 *	'-' for any other file type
 *
 *  1	'r' if the owner may read, '-' otherwise.
 *
 *  2	'w' if the owner may write, '-' otherwise.
 *
 *  3	'x' if the owner may execute, 's' if the file is
 *	set-user-id, '-' otherwise.
 *	'S' if the file is set-user-id, but the execute
 *	bit isn't set.
 *
 *  4	'r' if group members may read, '-' otherwise.
 *
 *  5	'w' if group members may write, '-' otherwise.
 *
 *  6	'x' if group members may execute, 's' if the file is
 *	set-group-id, '-' otherwise.
 *	'S' if it is set-group-id but not executable.
 *
 *  7	'r' if any user may read, '-' otherwise.
 *
 *  8	'w' if any user may write, '-' otherwise.
 *
 *  9	'x' if any user may execute, 't' if the file is "sticky"
 *	(will be retained in swap space after execution), '-'
 *	otherwise.
 *	'T' if the file is sticky but not executable.
 */
void
filemodestring (statp, str)
    struct stat *statp;
    char *str;
{
	mode_string (statp->st_mode, str);
}

/* Like filemodestring, but only the relevant part of the `struct stat'
 * is given as an argument.
 */
void
mode_string (mode, str)
unsigned short mode;
char *str;
{
	str[0] = ftypelet (mode);
	rwx ((mode & 0700) << 0, &str[1]);
	rwx ((mode & 0070) << 3, &str[4]);
	rwx ((mode & 0007) << 6, &str[7]);
	setst (mode, str);
}

/* Return a character indicating the type of file described by
 * file mode BITS:
 * 'd' for directories
 * 'b' for block special files
 * 'c' for character special files
 * 'm' for multiplexor files
 * 'l' for symbolic links
 * 's' for sockets
 * 'p' for fifos
 * '-' for any other file type.
 */
static char
ftypelet (bits)
    unsigned short bits;
{
	switch (bits & S_IFMT) {
	default:
		return '-';
	case S_IFDIR:
		return 'd';
#ifdef S_IFLNK
	case S_IFLNK:
		return 'l';
#endif
#ifdef S_IFCHR
	case S_IFCHR:
		return 'c';
#endif
#ifdef S_IFBLK
	case S_IFBLK:
		return 'b';
#endif
#ifdef S_IFMPC
	case S_IFMPC:
	case S_IFMPB:
		return 'm';
#endif
#ifdef S_IFSOCK
	case S_IFSOCK:
		return 's';
#endif
#ifdef S_IFIFO
#if S_IFIFO != S_IFSOCK
	case S_IFIFO:
		return 'p';
#endif
#endif
#ifdef S_IFNWK			/* HP-UX */
	case S_IFNWK:
		return 'n';
#endif
	}
}

/* Look at read, write, and execute bits in BITS and set
 * flags in CHARS accordingly.
 */
static void
rwx (bits, chars)
unsigned short bits;
char *chars;
{
	chars[0] = (bits & S_IREAD) ? 'r' : '-';
	chars[1] = (bits & S_IWRITE) ? 'w' : '-';
	chars[2] = (bits & S_IEXEC) ? 'x' : '-';
}

/* Set the 's' and 't' flags in file attributes string CHARS,
 * according to the file mode BITS.
 */
static void
setst (bits, chars)
    unsigned short bits;
    char *chars;
{
#ifdef S_ISUID
	if (bits & S_ISUID) {
		if (chars[3] != 'x') {
			/* Set-uid, but not executable by owner. */
			chars[3] = 'S';
		} else {
			chars[3] = 's';
		}
	}
#endif
#ifdef S_ISGID
	if (bits & S_ISGID) {
		if (chars[6] != 'x') {
			/* Set-gid, but not executable by group. */
			chars[6] = 'S';
		} else {
			chars[6] = 's';
		}
	}
#endif
#ifdef S_ISVTX
	if (bits & S_ISVTX) {
		if (chars[9] != 'x') {
			/* Sticky, but not executable by others. */
			chars[9] = 'T';
		} else {
			chars[9] = 't';
		}
	}
#endif
}

/* Error reporting */

char *program_name;

void
bfd_fatal (string)
    char *string;
{
	char *errmsg =  bfd_errmsg (bfd_error);

	if (string) {
		fprintf (stderr, "%s: %s: %s\n", program_name, string, errmsg);
	} else {
		fprintf (stderr, "%s: %s\n", program_name, errmsg);
	}

	if (NULL != exit_handler){
		(*exit_handler) ();
	}
	exit (1);
}

/* 
 * FIXME: Workaround for broken vfprintf() on sol-sun4 
 */
#ifdef	SOLARIS
char 	sol_tmpbuf[1024];
#endif

#if	defined(__STDC__)
void fatal (char *the_fmt, ...)
#else
void fatal (va_alist)
    va_dcl
#endif
{
	char *Format;
	va_list args;
#if	defined(__STDC__)
	va_start (args, the_fmt);
	Format = the_fmt;
#else
	va_start (args);
	Format = va_arg(args, char *);
#endif

#ifdef	SOLARIS
	/* FIXME: Workaround for broken vfprintf() on sol-sun4 */
	vsprintf(sol_tmpbuf, Format, args);
	fprintf(stderr, sol_tmpbuf);
#else
	vfprintf (stderr, Format, args);
#endif
	va_end (args);
	(void) putc ('\n', stderr);
	if (NULL != exit_handler){
		(*exit_handler) ();
	}
	exit (1);
} /* fatal() */


/** Display the archive header for an element as if it were an ls -l listing */

/* Mode       User\tGroup\tSize\tDate               Name */

void
print_arelt_descr (abfd, verbose, suppress_time)
    bfd *abfd;
    boolean verbose;
	boolean suppress_time;
{
	struct stat buf;
	char modebuf[11];
	char timebuf[40];
	long when;
	long current_time = time ((long *) 0);

	if (verbose) {
		if (bfd_stat_arch_elt (abfd, &buf) == 0) { /* if not, huh? */
			mode_string (buf.st_mode, modebuf);
			modebuf[10] = '\0';
			fputs (modebuf, stdout);
			if (suppress_time)
				when = 0;
			else
				when = buf.st_mtime;
			strcpy (timebuf, (CONST char *)ctime (&when));

			/* This code comes from gnu ls */
			if ((current_time - when > 6 * 30 * 24 * 60 * 60)
			|| (current_time - when < 0)) {
				/* The file is fairly old or in the future.
				 * POSIX says the cutoff is 6 months old;
				 * approximate this by 6*30 days.
				 * Show the year instead of the time of day.
				 */
				strcpy (timebuf + 11, timebuf + 19);
			}
			timebuf[16] = 0;
			printf (" %d\t%d\t%ld\t%s ",
				buf.st_uid, buf.st_gid, buf.st_size, timebuf);
		}
	}
	puts (abfd->filename);
}

/* Like malloc but get fatal error if memory is exhausted.  */
char *
xmalloc (size)
    unsigned size;
{
	register char *result = (PTR) malloc (size);
	if (result == (char *)NULL && size != 0) {
		fatal ("virtual memory exhausted");
	}
	return result;
}

/* Like realloc but get fatal error if memory is exhausted.  */
char *
xrealloc (ptr, size)
    char *ptr;
    unsigned size;
{
	register char *result = (PTR) realloc (ptr, size);

	if (result == 0 && size != 0) {
		fatal ("virtual memory exhausted");
	}
	return result;
}
