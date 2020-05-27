/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994 Intel Corporation
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
/*)ce*/

#include "ui.h"
#include <errno.h>

extern co(), ci();

#define STDIN  0
#define STDOUT 1
#define STDERR 2

#define ECHO   1
#define CRLF   2
#define CBREAK 4

static int flags = ECHO|CRLF;

int
term_ctl(int new_flags)
{
	int old_flags = flags;
	flags = new_flags;
	return old_flags;
}


int
term_write(int fd, char *buf, int count, int *nwritten)
{
	if (fd != STDOUT && fd != STDERR)
	    return EBADF;

	*nwritten = count;
	while (count-- > 0)
	{
	    if (*buf == '\n' && (flags & CRLF))
		co('\r');
	    co(*buf++);
	}

	return 0;
}

int
term_read(int fd, char *buf, int count, int *nread)
{
	int received = 0;
	char c;
	int dummy;

	if (fd != STDIN)
	    return EBADF;

	while (received < count)
	{
	    c = ci();

	    if ((c == '\177' || c == '\b') && !(flags & CBREAK))
	    {
	        if (received > 0)
		{
	            buf--;
		    received--;
		    if (flags & ECHO)
			term_write(STDOUT, "\b \b", 3, &dummy);
		}
	    }
	    else
	    {
		if (c == '\r' && (flags & CRLF))
		    c = '\n';

		if (flags & ECHO)
		    term_write(STDOUT, &c, 1, &dummy);

		*buf++ = c;
		received++;
	    }

	    if (flags & CBREAK)
	        break;

	    if (c == '\n')
	        break;
	}

	*nread = received;
	return 0;
}
