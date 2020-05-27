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

#include <stddef.h>
#include <fcntl.h>
#include <types.h>
#include <stat.h>
#include <errno.h>
#include "common.h"
#include "hdi_errs.h"
#include "hdi_com.h"
#include "dbg_mon.h"
#include "cop.h"

#define EIO 5

extern int break_vector;
extern void * memcpy(void *, const void *, size_t);
extern size_t strlen(const char *);
extern void enter_runtime(), exit_runtime(int flag);
extern void force_interrupt(int);

static int save_cmd;
static int intr_flag;

static void rt_init_msg(int cmd)
{
	intr_flag = FALSE;
	save_cmd = cmd;

	com_init_msg();
	com_put_byte(cmd);
	com_put_byte(0xff);
}

int break_pending;

static const unsigned char *
rt_send_cmd(int wait, int *statp, int *sizep)
{
	const unsigned char *rp=NULL;
	int size = 0;

	if (wait == COM_WAIT_FOREVER)
	    {
	    if (com_put_msg(NULL, 0) == OK)
	    	while ((rp = com_get_msg(&size, COM_POLL)) == NULL &&
	    		         com_get_stat() == E_COMM_TIMO)
	        	{
		        /* If an interrupt was received from the HOST,  */
		        /* clear the pending interrupt and handle it.   */
		        if (break_pending == break_vector)
		            {
			        break_pending = 0;
			        force_interrupt(break_vector);
		            }
		        }
	    }
	else
	    rp = com_send_cmd(NULL, &size, wait);
	
	if (rp == NULL || size < 2 || rp[0] != save_cmd)
	    *statp = EIO;
	else
	    {
	    if (rp[1] & 0x80)
	        intr_flag = TRUE;
	    *statp = rp[1] & ~0x80;
	    }    

	if (sizep)
	    *sizep = size - 2;

	return rp + 2;
}


int
host_arg_init(char *argbuf, int len)
{
	const char *rp;
	int stat, size;

	enter_runtime();

	rt_init_msg(DSTDARG);
	rp = (const char *)rt_send_cmd(COM_WAIT_FOREVER, &stat, &size);

	if (stat != OK)
    	{
	    exit_runtime(intr_flag);
	    return -1;
    	}
	if (size > len - 2)
    	{
	    size = len - 2;
	    argbuf[len-2] = argbuf[len-1] = '\0';
    	}

	memcpy(argbuf, rp, size);

	exit_runtime(intr_flag);
	return OK;
}


int
host_close(fd)
int	fd;
{
	int stat;

	enter_runtime();

	rt_init_msg(DCLOSE);
	com_put_byte(fd);
	(void)rt_send_cmd(COM_WAIT, &stat, NULL);

	exit_runtime(intr_flag);
	return stat;
}


int
host_isatty(int fd, int *result)
{
	const unsigned char *rp;
	int stat;

	enter_runtime();

	rt_init_msg(DISATTY);
	com_put_byte(fd);
	rp = rt_send_cmd(COM_WAIT, &stat, NULL);
	*result = get_byte(rp);

	exit_runtime(intr_flag);
	return stat;
}


long
host_lseek(int fd, long offset, int whence, long *new_offset)
{
	const unsigned char *rp;
	int stat;

	enter_runtime();

	rt_init_msg(DSEEK);
	com_put_byte(fd);
	com_put_long(offset);
	com_put_byte(whence);
	rp = rt_send_cmd(COM_WAIT, &stat, NULL);
	*new_offset = get_long(rp);

	exit_runtime(intr_flag);
	return stat;
}


int
host_open(char *fn, int mode, int cmode, int *fd)
{
	const unsigned char *rp;
	int stat;

	enter_runtime();

	rt_init_msg(DOPEN);
	com_put_short(mode);
	com_put_short(cmode);
	if (com_put_data((const unsigned char *)fn, strlen(fn)+1) != OK)
    	{
	    exit_runtime(intr_flag);
	    return EIO;
    	}
	rp = rt_send_cmd(COM_WAIT_FOREVER, &stat, NULL);
	*fd = get_byte(rp);

	exit_runtime(intr_flag);
	return stat;
}


int
host_read(int fd, char *cp, int sz, int *nread)
{
	const unsigned char *rp;
	int stat, part_sz;

	enter_runtime();

	*nread = 0;
	while ((part_sz = sz-*nread) > 0)
	    {
		/* Cut the read into packet-sized parts, if necessary. */
		if (part_sz > MAX_MSG_SIZE - 4)
		    part_sz = MAX_MSG_SIZE - 4;

		rt_init_msg(DREAD);
		com_put_byte(fd);
		com_put_short(part_sz);
		rp = rt_send_cmd(COM_WAIT_FOREVER, &stat, NULL);

		if (stat == OK)
	    	{
		    int actual = get_short(rp);
		    if (actual > 0)
	    	    {
				if (actual > (sz - *nread)) /* don't overflow buffer */
					actual = sz - *nread;

		        memcpy(cp, (const char *)rp, actual);
				cp += actual;
		        *nread += actual;
	    	    }
		    /*
		     * Take care of the cases in which the requested sz is
		     * larger than what actually turns out to be available.
		     * This includes input from an interactive "file", such
		     * as stdin.  There is one hole here: if the interactive
		     * read has requested >MAX_MSG_SIZE-4 and the actual
		     * size returned is exactly ==MAX_MSG_SIZE-4, then we
		     * will loop one more time.  This forces the interactive
		     * user to enter something more...even just a carriage-
		     * return...before we will break out of this loop.
		     */
		    if (actual < part_sz)
	 		/*We've read all there is...return with a "short read"*/
			break;
	    	}
		else
		    /*Read failed*/
		    break;
	    }

	exit_runtime(intr_flag);
	return stat;
}

static void copy_statbuf(struct stat *, const unsigned char *);

int
host_fstat(int fd, struct stat *bp)
{
	const unsigned char *rp;
	int stat;

	enter_runtime();

	rt_init_msg(DFSTAT);
	com_put_byte(fd);
	rp = rt_send_cmd(COM_WAIT, &stat, NULL);

	if (stat == OK)
	    copy_statbuf(bp, rp);

	exit_runtime(intr_flag);
	return stat;
}

int
host_stat(char *path, struct stat *bp)
{
	const unsigned char *rp;
	int stat;

	enter_runtime();

	rt_init_msg(DSTAT);
	if (com_put_data((const unsigned char *)path, strlen(path)+1) != OK)
	    {
	    exit_runtime(intr_flag);
	    return EIO;
	    }
	rp = rt_send_cmd(COM_WAIT, &stat, NULL);

	if (stat == OK)
	    copy_statbuf(bp, rp);

	exit_runtime(intr_flag);
	return stat;
}

static void
copy_statbuf(struct stat *sb, const unsigned char *rp)
{
    sb->st_dev = get_short(rp);
	sb->st_ino = get_short(rp);
	sb->st_mode = get_short(rp);
	sb->st_nlink = get_short(rp);
	sb->st_uid = get_short(rp);
	sb->st_gid = get_short(rp);
	sb->st_rdev = get_short(rp);
	sb->st_size = get_long(rp);
	sb->st_atime = get_long(rp);
	sb->st_mtime = get_long(rp);
	sb->st_ctime = get_long(rp);
}


int
host_system(char *cp, int *result)
{
	const unsigned char *rp;
	int stat;

	enter_runtime();

	rt_init_msg(DSYSTEM);
	if (cp)
    	{
	    if (com_put_data((const unsigned char *)cp, strlen(cp)+1) != OK)
    	    {
    		exit_runtime(intr_flag);
    		return EIO;
    	    }
    	}
	rp = rt_send_cmd(COM_WAIT_FOREVER, &stat, NULL);
	*result = get_short(rp);

	exit_runtime(intr_flag);
	return stat;
}


int
host_time(long *time)
{
	const unsigned char *rp;
	int stat;

	enter_runtime();

	rt_init_msg(DTIME);
	rp = rt_send_cmd(COM_WAIT, &stat, NULL);
	*time = get_long(rp);

	exit_runtime(intr_flag);
	return stat;
}


int
host_unlink(char *path)
{
	int stat;

	enter_runtime();

	rt_init_msg(DUNLINK);
	if (com_put_data((const unsigned char *)path, strlen(path)+1) != OK)
	    {
	    exit_runtime(intr_flag);
	    return EIO;
	    }
	(void)rt_send_cmd(COM_WAIT, &stat, NULL);

	exit_runtime(intr_flag);
	return stat;
}


int
host_rename(char *old, char *new)
{
	int stat;

	enter_runtime();

	rt_init_msg(DRENAME);
	if (com_put_data((const unsigned char *)old, strlen(old)+1) != OK ||
	    com_put_data((const unsigned char *)new, strlen(new)+1) != OK)
	    {
		exit_runtime(intr_flag);
		return EIO;
	    }

	(void)rt_send_cmd(COM_WAIT, &stat, NULL);

	exit_runtime(intr_flag);
	return stat;
}


int
host_write(int fd, char *cp, int sz, int *nwritten)
{
	enter_runtime();

	*nwritten = 0;
	while (sz > 0)
	    {
		int want = ((sz < (MAX_MSG_SIZE - 5))?sz:(MAX_MSG_SIZE-5));
		const unsigned char *rp;
		int stat, actual;

		rt_init_msg(DWRITE);
		com_put_byte(fd);
		com_put_short(want);
		if (com_put_data((const unsigned char *)cp, want) != OK)
		    {
		    exit_runtime(intr_flag);
		    return EIO;
		    }

		rp = rt_send_cmd(COM_WAIT, &stat, NULL);
		if (stat != OK)
		    {
		    exit_runtime(intr_flag);
		    return stat;
		    }

		actual = get_short(rp);

		*nwritten += actual;
		cp += actual;
		sz -= actual;

		if (actual != want)
			break;
    	}

	exit_runtime(intr_flag);
	return OK;
}
