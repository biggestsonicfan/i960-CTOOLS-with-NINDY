/*******************************************************************************
 * 
 * Copyright (c) 1993 Intel Corporation
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
 ******************************************************************************/

/* strtol - convert a string to a long
 * Copyright (c) 1984,85,86 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <stddef.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>

long int strtol(const char *buf, char **end, int base)
{
    int	save_errno;
    long val;
    unsigned long	uval;
    const char *save_buf, *pre_strtoul_buf;


    save_errno = errno;
    errno = 0;				/* reset */
    save_buf = buf;

    /* skip leading white space */
    while (isspace(*buf) || *buf == '\t')
        ++buf;

    if (*buf == '-')
    {
	/* Check for bad syntax that strtoul will not catch: "--", "-+", "- " */
	char	ch = *(buf+1);

	if (ch == '-' || ch == '+' || ch == '\t' || isspace(ch))
	{
		if (end)
			*end = (char*) save_buf;
		errno = save_errno;
		return 0;
	}

	/* Handle negative numbers */

	pre_strtoul_buf = buf+1;
	uval = strtoul(buf + 1, end, base);
	val = (long) -uval;
	if (errno == ERANGE || uval > (unsigned long) 0x80000000)
	{
	    errno = ERANGE;
	    val = LONG_MIN;
	}
    }
    else
    {
	/* Handle positive numbers */

	pre_strtoul_buf = buf;
	uval = strtoul(buf, end, base);
	val = (long) uval;

	if (errno == ERANGE || uval > (unsigned long) 0x7fffffff)
	{
		errno = ERANGE;
		val = LONG_MAX;
	}
    }

    if (errno == 0)
	errno = save_errno;

    if (end && *end == pre_strtoul_buf) /* strtoul didn't convert anything */
	*end = (char *)save_buf;

    return val;
}
