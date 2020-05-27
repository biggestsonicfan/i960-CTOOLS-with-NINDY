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

#include <errno.h>
#include <fcntl.h>

#include  "common.h"

#include "sdm.h"	/*This is needed only when the monitor is linked with
			 *an application.  In that case, linker warnings will
			 *be generated unless sdm.h is included here.  (The
			 *linker will think that the runtime routines are
			 *defined twice, once here and once as a pragma system
			 *call.  Actually, the pragma system ultimately
			 *calls into here, but the linker won't know that.)
			 *Note that it doesn't hurt anything to include sdm.h
			 *even when not linking into the application.
			 */

extern int host_close(int);
extern int host_arg_init(char *, int);
extern int host_isatty(int, int *);
extern long host_lseek(int, long, int, long *);
extern int host_open(const char *, int, int, int *);
extern int host_read(int, char *, int, int *);
extern int host_fstat(int, void *);
extern int host_stat(const char *, void *);
extern int host_system(const char *, int *);
extern int host_time(long *);
extern int host_rename(const char *, const char *);
extern int host_unlink(const char *);
extern int host_write(int, const char *, int, int *);
extern int term_read(int, char *, int, int *);
extern int term_write(int, const char *, int, int *);

extern int host_connection;


int
sdm_arg_init(char *buf, int len)
{
	if (host_connection)
	    return host_arg_init(buf, len);
	else
	    return -1;
}

int
sdm_close(int fd)
{
	if (host_connection)
	    return host_close(fd);
	else if (fd >= 0 && fd <= 2)
	    return 0;
	else
	    return EBADF;
}

int
sdm_isatty(int fd, int *result)
{
	if (host_connection)
	    return host_isatty(fd, result);
	else if (fd >= 0 && fd <= 2)
	{
	    *result = TRUE;
	    return 0;
	}
	else
	    return EBADF;
}

int
sdm_lseek(int fd, long offset, int whence, long *new_offset)
{
	if (host_connection)
	    return host_lseek(fd, offset, whence, new_offset);
	else
	    return EBADF;
}

int
sdm_open(const char *fn, int mode, int cmode, int *fd)
{
	if (host_connection)
	    return host_open(fn, mode, cmode, fd);
	else
	    return ENOENT;
}

int
sdm_read(int fd, char *cp, int sz, int *nread)
{
	if (host_connection)
	    return host_read(fd, cp, sz, nread);
	else
	    return term_read(fd, cp, sz, nread);
}

int
sdm_fstat(int fd, void *bp)		/* bp is actually a (struct stat *) */
{
	if (host_connection)
	    return host_fstat(fd, bp);
	else
	    return EBADF;
}

int
sdm_stat(const char *path, void *bp)	/* bp is actually a (struct stat *) */
{
	if (host_connection)
	    return host_stat(path, bp);
	else
	    return ENOENT;
}

int
sdm_system(const char *cp, int *result)
{
	if (host_connection)
	    return host_system(cp, result);
	else
	    return -1;
}

int
sdm_time(long *time)
{
	if (host_connection)
	    return host_time(time);
	else
	    return -1;
}

int
sdm_rename(const char *old, const char *new)
{
	if (host_connection)
	    return host_rename(old, new);
	else
	    return ENOENT;
}

int
sdm_unlink(const char *path)
{
	if (host_connection)
	    return host_unlink(path);
	else
	    return ENOENT;
}

int
sdm_write(int fd, const char *cp, int sz, int *nwritten)
{
	if (host_connection)
	    return host_write(fd, cp, sz, nwritten);
	else
	    return term_write(fd, cp, sz, nwritten);
}
