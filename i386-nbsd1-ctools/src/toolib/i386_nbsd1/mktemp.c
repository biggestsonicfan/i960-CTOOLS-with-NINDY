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

#include <sysdep.h>

#if defined(DOS) && defined(__HIGHC__)

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* As of High C version 3.31, we need to include Metaware's
   pragma's and prototype that affect 'mktemp'.  Otherwise,
   our version of mktemp won't be linked in.  This is why
   <io.h> is included.
 */
#include <io.h>
#endif /* DOS && __HIGHC__ */

#if (defined(DOS) && defined(__HIGHC__)) || defined(HOST_IS_GCC960) 
/*
 * "mktemp" -- replaces Metaware High C/C++'s implementation.
 *
 * Metaware High C/C++'s mktemp differs so significantly from
 * that on most Unix systems that it wreaks havoc with our tools.
 *
 * We also need to provide this for gcc960 self-host.
 */

/* MKTEMP
 *
 * Create a unique file name given a template of the
 * form  "baseXXXXXX".  See the description of mktemp
 * in the Microsoft C library for more information.
 * Return a pointer to the template name if successful,
 * else NULL.
 *
 * NOTE: The template's XXXXXX is overwritten.  An attempt
 * is made to see if an object with the new name
 * already exists.  If it exists, NULL is returned.
 *
 */

#define DOS_MAX_NAMELEN	8
	/* Don't use file extension because mpp960 expects */
	/* mktemp(XXXXXX) to generate an integer, not a float. */

#define MIN_NUM_OF_X	3
	/* Must be at least MIN_NUM_OF_X X's left to overwrite if */
	/* template is truncated to DOS_MAX_NAMELEN characters. */


char *mktemp(char * template)
{
static unsigned long counter = 0;
    FILE	*fp;
    char	*x_ptr, *file_name;
    int		i, save_errno;
    int		num_tries;
    unsigned int total_length;
    int		file_length;
    int		xxx_length = 6;	/* number of trailing X's in template */

    /*
     * create a 3 digit random number from the time of day.
     */
    if (!counter) {
	srand(time(NULL));
	counter = (unsigned long)(rand() % 1000);
    }
    else {
	counter++;
    }

    if ((template == NULL) || (total_length = strlen(template)) < xxx_length)
	return NULL;
    x_ptr = template + total_length;
    for (i=0; i < xxx_length; i++)
	if (*--x_ptr != 'X')
	    return NULL;

    file_name = x_ptr - 1;
    while ((file_name >= template) &&
	   (*file_name != '/') && (*file_name != '\\') && (*file_name != ':'))
	file_name--;

    file_name++;
    file_length = (x_ptr - file_name) + xxx_length;  /* = strlen(file_name) */

    /* Truncate file name to DOS_MAX_NAMELEN characters. */
    /* Must be atleast MIN_NUM_OF_X X's left to overwrite. */

    if (file_length > DOS_MAX_NAMELEN) {
	xxx_length -= file_length - DOS_MAX_NAMELEN;
	if (xxx_length < MIN_NUM_OF_X)
	    return NULL;
	file_length = DOS_MAX_NAMELEN;
	file_name[file_length] = '\0';
    }

    save_errno = errno;

    for (num_tries = 10; num_tries != 0; --num_tries) {

	(void) sprintf(x_ptr, "%0*d", xxx_length, counter);

	if (++counter > 999)
	    counter = 0;

	errno = 0;
	if ((fp = fopen(template, "r")) != NULL)
	{
	    (void) fclose(fp);
	    continue;	/* try again */
	}
	else if (errno != ENOENT)	/* Error_file_not_found */
	{
	    /* non-file object exists, or bad path, try again */
	    continue;	/* try again */
	}
	/* make sure template is a legal file name */
	else if ((fp = fopen(template, "w")) != NULL)
	{
	    (void) fclose(fp);
	    (void) remove(template);
	    errno = save_errno;
	    return template;
	}
	/* else continue */
    }

    errno = save_errno;
    return NULL;
}
#endif /* (DOS && __HIGHC__) || HOST_IS_GCC960 */

