/* messages.c - error reporter - */

/*   MODIFIED BY CHRIS BENENATI, FOR INTEL CORPORATION, 4/89 */
/* messages.c - error reporter -
   Copyright (C) 1987 Free Software Foundation, Inc.

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

/* $Id: messages.c,v 1.16 1994/03/12 00:26:46 peters Exp $ */

#include <stdio.h>		/* define stderr */

#include "as.h"

#if	defined(DOS) || defined(__HIGHC__)
#   include <errno.h>
#endif

#ifdef NO_STDARG
#include <varargs.h>
#else
#include <stdarg.h>
#endif /* NO_STDARG */

/* PS: Definition of the #defined constants, by important hosts.
   A constant not listed here is NOT defined.

   sun4		i386vr4		rs6000		DOS
   NO_STDARG	NO_STDARG	 none		__STDC__
                    ?
*/
/*
 * as_fatal() is used when gas is quite confused and
 * continuing the assembly is pointless.  In this case we
 * exit immediately with error status.
 *
 * as_bad() is used to mark errors that result in what we
 * presume to be a useless object file.  Say, we ignored
 * something that might have been vital.  If we see any of
 * these, assembly will continue to the end of the source,
 * no object file will be produced, and we will terminate
 * with error status.  The new option, -Z, tells us to
 * produce an object file anyway but we still exit with
 * error status.  The assumption here is that you don't want
 * this object file but we could be wrong.
 *
 * as_warn() is used when we have an error from which we
 * have a plausible error recovery.  eg, masking the top
 * bits of a constant that is longer than will fit in the
 * destination.  In this case we will continue to assemble
 * the source, although we may have made a bad assumption,
 * and we will produce an object file and return normal exit
 * status (ie, no error).  The new option -X tells us to
 * treat all as_warn() errors as as_bad() errors.  That is,
 * no object file will be produced and we will exit with
 * error status.  The idea here is that we don't kill an
 * entire make because of an error that we knew how to
 * correct.  On the other hand, sometimes you might want to
 * stop the make at these points.
 */

#ifdef	SOLARIS
char	sol_tmpbuf[1024];	/* FIXME: Workaround for broken vfprintf() */
#endif

static int warning_count = 0; /* Count of number of warnings issued */

int had_warnings() {
	return(warning_count);
} /* had_err() */

/* Nonzero if we've hit a 'bad error', and should not write an obj file,
   and exit with a nonzero error code */

static int error_count = 0;

int had_errors() {
	return(error_count);
} /* had_errors() */


/*
 *			a s _ p e r r o r
 *
 * Like perror(3), but with more info.
 */
void as_perror(gripe, filename)
char *gripe;		/* Unpunctuated error theme. */
char *filename;
{
#ifndef errno
#ifndef __HIGHC__
	extern int errno; /* See perror(3) for details. */
#endif
#endif
#ifndef M_GCC960
	extern int sys_nerr;
#ifdef I386_NBSD1
	extern const char *const sys_errlist[];
#else       
	extern char *sys_errlist[];
#endif
#endif

	as_where();
	fprintf(stderr,gripe,filename);

#ifndef M_GCC960
	if (errno > sys_nerr)
	    fprintf(stderr, "\nUnknown error #%d.\n", errno);
	else
	    fprintf(stderr, "\n%s.\n", sys_errlist[errno]);
#else
	perror("gas960");
#endif
	errno = 0; /* After reporting, clear it. */
	++error_count;
} /* as_perror() */


/*
 *			a s _ w a r n ()
 *
 * Send to stderr a string (with bell) (JF: Bell is obnoxious!) as a warning, and locate warning
 * in input file(s).
 * Please only use this for when we have some recovery action.
 * Please explain in string (which may have '\n's) what recovery was done.
 */

#ifdef NO_STDARG
/* Old-style varargs implementation */

#ifdef SOLARIS
/* FIXME! Workaround for broken vfprintf() function */
void as_warn(va_alist)
va_dcl
{
	va_list args;
	char	*Format;

	if(!flagseen['W']) 
	{
		va_start(args);
		Format = va_arg(args, char *);
		vsprintf(sol_tmpbuf, Format, args);

		++warning_count;
		as_where();
		fprintf (stderr, "Warning: ");
		fprintf (stderr, "%s\n", sol_tmpbuf);

		if ( listing_now && ! listing_pass2 ) 
		{
			listing_append_error_buf ("Warning: ");
			strcpy (listing_error_tmpbuf, sol_tmpbuf);
		}
		va_end(args);
	}
}
#else /* Not Solaris */
void as_warn(Format,va_alist)
char *Format;
va_dcl
{
	va_list args;
	
	if(!flagseen['W']) 
	{
		++warning_count;
		as_where();
		fprintf (stderr, "Warning: ");
		va_start(args);
		vfprintf(stderr, Format, args);
		(void) putc('\n', stderr);
		if ( listing_now && ! listing_pass2 ) 
		{
			listing_append_error_buf ("Warning: ");
			vsprintf (listing_error_tmpbuf, Format, args);
		}
		va_end(args);
	}
}
#endif  /* if ( Solaris ) */

#else
/* Newer ANSI-style stdarg implementation */
#ifdef __STDC__
void as_warn(const char* Format, ...)
#else
void as_warn(const char* Format)
#endif	/* __STDC__ */
{
	va_list args;
	
	if(!flagseen['W']) 
	{
		++warning_count;
		as_where();
		fprintf (stderr, "Warning: ");
		va_start(args, Format);
		vfprintf(stderr, Format, args);
		(void) putc('\n', stderr);
		if ( listing_now && ! listing_pass2 ) 
		{
			listing_append_error_buf ("Warning: ");
			vsprintf (listing_error_tmpbuf, Format, args);
		}
		va_end(args);
	}
}
#endif /* NO_STDARG */


/*
 *			a s _ b a d ()
 *
 * Send to stderr a string (with bell) (JF: Bell is obnoxious!) as a warning,
 * and locate warning in input file(s).
 * Please us when there is no recovery, but we want to continue processing
 * but not produce an object file.
 * Please explain in string (which may have '\n's) what recovery was done.
 */

#ifdef NO_STDARG
/* Old-style varargs implementation */

#ifdef	SOLARIS
/* FIXME! Workaround for broken vfprintf() function */
void as_bad(va_alist)
va_dcl
{
	va_list args;
	char	*Format;

	va_start(args);
	Format = va_arg(args, char *);
	vsprintf(sol_tmpbuf, Format, args);

	++error_count;
	as_where();
	fprintf(stderr, "%s\n", sol_tmpbuf);
	if ( listing_now && ! listing_pass2 ) 
		strcpy (listing_error_tmpbuf, sol_tmpbuf);
	va_end(args);
} /* as_bad() */
#else	/* Not Solaris */
void as_bad(Format,va_alist)
char *Format;
va_dcl
{
	va_list args;

	++error_count;
	as_where();
	va_start(args);
	vfprintf(stderr, Format, args);
	(void) putc('\n', stderr);
	if ( listing_now && ! listing_pass2 ) 
		vsprintf (listing_error_tmpbuf, Format, args);
	va_end(args);
} /* as_bad() */
#endif	/* if ( Solaris ) */

#else
/* Newer ANSI-style stdarg implementation */
#ifdef __STDC__
void as_bad(const char *Format, ...)
#else
void as_bad(const char *Format)
#endif	/* __STDC__ */
{
	va_list args;

	++error_count;
	as_where();
	va_start(args, Format);
	vfprintf(stderr, Format, args);
	(void) putc('\n', stderr);
	if ( listing_now && ! listing_pass2 ) 
		vsprintf (listing_error_tmpbuf, Format, args);
	va_end(args);
} /* as_bad() */
#endif /* NO_STDARG */


/*
 *			a s _ f a t a l ()
 *
 * Send to stderr a string (with bell) (JF: Bell is obnoxious!) as a fatal
 * message, and locate stdsource in input file(s).
 * Please only use this for when we DON'T have some recovery action.
 * It exit()s with a nonzero status.
 */

#ifdef NO_STDARG
/* Old-style varargs implementation */

#ifdef	SOLARIS
/* FIXME! Workaround for broken vfprintf() function */
void as_fatal(va_alist)
va_dcl
{
	va_list args;
	char 	*Format;

	va_start(args);
	Format = va_arg(args, char *);
	vsprintf(sol_tmpbuf, Format, args);

	as_where();
	fprintf (stderr, "FATAL: ");
	fprintf(stderr, "%s\n", sol_tmpbuf);
	va_end(args);
	listing_end(1);  /* remove temp files if any */
	exit(42);
} /* as_fatal() */
#else	/* Not Solaris */
void as_fatal(Format,va_alist)
char *Format;
va_dcl
{
	va_list args;

	as_where();
	va_start(args);
	fprintf (stderr, "FATAL: ");
	vfprintf(stderr, Format, args);
	(void) putc('\n', stderr);
	va_end(args);
	listing_end(1);  /* remove temp files if any */
	exit(42);
} /* as_fatal() */
#endif  /* if ( Solaris ) */

#else
/* Newer ANSI-style stdarg implementation */
#ifdef __STDC__
void as_fatal(const char*Format, ...)
#else
void as_fatal(const char*Format)
#endif	/* __STDC__ */
{
	va_list args;

	as_where();
	va_start(args, Format);
	fprintf (stderr, "FATAL: ");
	vfprintf(stderr, Format, args);
	(void) putc('\n', stderr);
	va_end(args);
	listing_end(1);  /* remove temp files if any */
	exit(42);
} /* as_fatal() */
#endif /* NO_STDARG */
