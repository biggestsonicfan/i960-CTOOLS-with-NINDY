/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994, 1995 Intel Corporation
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
/*
 * SERVER: libc support during execution
 */
/* $Header: /ffs/p1/dev/src/hdil/common/RCS/serve.c,v 1.6 1995/08/22 00:55:23 cmorgan Exp $$Locker:  $ */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "private.h"
#include "hdi_com.h"
#include "dbg_mon.h"
#include "cop.h"


#define STDIN  0
#define STDOUT 1
#define STDERR 2

#ifdef __STDC__
#include <stdlib.h>

#ifdef _MSC_VER
typedef struct _stat struct_stat;
#define O_RDONLY _O_RDONLY
#define O_WRONLY _O_WRONLY
#define O_RDWR   _O_RDWR
#define O_TEXT   _O_TEXT
#define O_BINARY _O_BINARY
#define O_EXCL   _O_EXCL
#define O_CREAT  _O_CREAT
#define O_TRUNC  _O_TRUNC
#define O_APPEND _O_APPEND
#define S_IWRITE _S_IWRITE
#define S_IREAD  _S_IREAD
#else /* _MSC_VER */
typedef struct stat struct_stat;
#endif /* _MSC_VER */

#ifdef MSDOS
#include <conio.h>
#include <process.h>
#include <io.h>

#if !defined(__HIGHC__) /* Not Metaware */
extern int open(const char *, int, ...);
extern int read(int, char *, int);
extern int write(int, const char *, int);
extern long lseek(int, long, int);
extern int close(int);
extern int unlink(const char *);
extern int fstat(int, struct_stat *);
extern int stat(const char *, struct_stat *);
extern int isatty(int);

#endif /* not HIGHC */
#endif /* MSDOS */

static const STOP_RECORD *serve();
static const STOP_RECORD *handle_target_request(const unsigned char *, int sz);
const STOP_RECORD *handle_stop_msg(const unsigned char *);
static int send_user_input();
static int new_fd(int fd);
static int map_fd(int fd);
static void do_redirection(char *cmdline);
static void put_env(int max_size);
static void put_statbuf(struct_stat *sb);
static int xlate_flags(int flags);

#else /* __STDC__ */
typedef struct stat struct_stat;

extern int system();
extern int unlink();
extern int fstat();
extern int stat();
extern int isatty();

static const STOP_RECORD *serve();
static const STOP_RECORD *handle_target_request();
const STOP_RECORD *handle_stop_msg();
static int send_user_input();
static int new_fd();
static int map_fd();
static void do_redirection();
static void put_env();
static void put_statbuf();
static int xlate_flags();
#endif /* __STDC__ */

#ifndef O_TEXT
#define O_TEXT 0
#endif


static int isext;
static int interrupted;
static int fd_map[20];
static const STOP_RECORD running = { STOP_RUNNING };
static int use_async_input = FALSE;
static int waiting_for_user_input = FALSE;
static char user_input_buf[MAX_MSG_SIZE - 4];
static int user_input_end = 0;
static int user_input_start = 0;
static int read_count;  /* requested read count */


/* Called by trgt_go after starting execution.  This call will be followed
 * by a call to _hdi_serve or by multiple calls to hdi_poll.
 */
void
_hdi_serve_init()
{
    isext = hdi_cmdext(HDI_EINIT, (unsigned char *)NULL, 0);
    interrupted = _hdi_signalled = FALSE;
}


/*
 * Become a server for the executing target, until an SDM event (ie, 
 * breakpoint), program exit, or a CNTL-C is struck at the keyboard.
 */
const STOP_RECORD *
_hdi_serve(intr_flag)
int intr_flag;
{
	const unsigned char *request;
	int sz;
	const STOP_RECORD *r;

	if (!_hdi_running)
	{
	    hdi_cmd_stat = E_NOTRUNNING;
	    return(NULL);
	}

	interrupted = intr_flag;

	do {
	    /* If r is not set to non-NULL in the body of the loop,
	     * we will exit with an error. */
	    r = NULL;

	    request = com_get_msg(&sz, interrupted ? COM_WAIT : COM_WAIT_FOREVER);

		/* check if target was reset after last poll */
		if (com_get_target_reset())
		{
			do_reset(FALSE);
	   		hdi_cmd_stat = E_TARGET_RESET;
	   		return(NULL);
		}
	    
	    /* If comm system is interrupted, but manages to
	       successfully complete the packet, no interrupt will
	       be sent to the target.  The signal bit will be set
	       in the response status.
	       EXCEPTION: If break_causes_reset flag is TRUE, always
	       send the break--this gives consistent behavior.
	       */
	    if (_hdi_signalled && _hdi_break_causes_reset)
	    {
			/* Hdi_reset will send the break.  If it does not
			 * return OK, hdi_cmd_stat is already set.  */
			if (hdi_reset() == OK)
		    	hdi_cmd_stat = E_RESET;
	    }
	    else if (request == NULL)
	    {
			hdi_cmd_stat = com_get_stat();
			if (hdi_cmd_stat == E_INTR && !interrupted)
			{
		   		if (com_intr_trgt() != OK) /* Send an interrupt */
		    	{
					hdi_cmd_stat = com_get_stat();
					return(NULL);
		    	}
		    	interrupted = TRUE;
		   		r = &running;	/* Continue looping to wait for
					 * a response to the interrupt. */
			}
	    }
	    else
			r = handle_target_request(request, sz);

	} while (r != NULL && r->reason == STOP_RUNNING);

	return(r);
}

const STOP_RECORD *
hdi_poll()
{
	const unsigned char *request;
	int sz;
	const STOP_RECORD *r;

	/* check if target was reset after last poll */
	if (com_get_target_reset())
	{
		do_reset(FALSE);
	    hdi_cmd_stat = E_TARGET_RESET;
	    return(NULL);
	}

	if (!_hdi_running)
	{
	    hdi_cmd_stat = E_NOTRUNNING;
	    return(NULL);
	}

	/* If break_causes_reset flag is TRUE, send a break to reset the
	 * target before checking for a packet--this gives consistent behavior
	 * (at least, I think it will).  -prl
	 */

	if (_hdi_signalled && _hdi_break_causes_reset)
	{
	    /* Hdi_reset will send the break.  If it does not
	     * return OK, hdi_cmd_stat is already set.  */
	    if (hdi_reset() == OK)
		hdi_cmd_stat = E_RESET;

	    _hdi_signalled = FALSE;

	    return(NULL);
	}

	if (send_user_input() != OK)
	    return(NULL);

	request = com_get_msg(&sz, COM_POLL);

	if (request == NULL)
	{
	    hdi_cmd_stat = com_get_stat();
	    if (hdi_cmd_stat != E_COMM_TIMO && hdi_cmd_stat != E_INTR)
			return(NULL);
	}
	else
	{
	    r = handle_target_request(request, sz);
	    if (r == NULL || r->reason != STOP_RUNNING)
		return(r);
	}

	if (_hdi_signalled)
	{
	    if (com_intr_trgt() != OK)
	    {
		hdi_cmd_stat = com_get_stat();
		return(NULL);
	    }

	    interrupted = TRUE;
	    _hdi_signalled = FALSE;
	}

	return &running;
}


static const STOP_RECORD *
handle_target_request(request, sz)
const unsigned char *request;
int sz;
{
	int cmd;
	unsigned short	mode;
	unsigned short	perm;
	const char	*name;
	int		ret;
	unsigned short	fd;
	unsigned short	count;
	struct_stat	sb;

	if (sz < 2 || request[1] != 0xff)
	{
	    hdi_cmd_stat = E_COMM_ERR;
	    return(NULL);
	}
	     
	/* Let debugger have first crack at it if it asked for it */
	if (isext)
	    if (hdi_cmdext(HDI_EPOLL, request, sz) == 1)
		return &running;

	cmd = get_byte(request);
	request++;			/* Skip status field */

	com_init_msg();
	com_put_byte(cmd);

/* Bit 7 of the return status tells the target to stop */
#define put_status(status) (interrupted |= _hdi_signalled, \
					    _hdi_signalled = FALSE, \
			 		    com_put_byte((status) | (interrupted ? 0x80 : 0)))

	switch (cmd) {
	default:
		if (hdi_cmdext(HDI_EDATA, request-2, sz) == 1)
			return &running;

		interrupted = TRUE;
		put_status(E_ARG);
		break;

	case STOP:
		return handle_stop_msg(request);

	case DOPEN:
		mode = get_short(request);
		perm = get_short(request);
		name = (const char *)request;

                mode = xlate_flags(mode);

		ret = new_fd(open((char *)name, mode, perm));

		if (ret < 0)
		    put_status(errno);
		else
		{
		    put_status(OK);
		    com_put_byte(ret);
		}
		break;

	case DREAD:
		{
		    char read_buf[MAX_MSG_SIZE - 4];
		    int count;

		    fd = get_byte(request);
		    count = get_short(request);

		    fd = map_fd(fd);
		    if (count > sizeof(read_buf))
				count = sizeof(read_buf);   /* Max size of 996 per request */
				/* monitor handles case of > 996 bytes by sending 
				   requests in 996 byte pieces */

		    if (fd == STDIN)
		    {
				ret = hdi_user_get_line(read_buf, count);
				if (use_async_input)
				{
				    waiting_for_user_input = TRUE;

			    /* Don't make a response to the target; that
			     * will be done when input is available.  If
			     * input is already available (typeahead), it
			     * will be sent the next time hdi_poll is
			     * called.  */
					read_count = count; /* save count for async read */
				    return &running;
				}
		    }
		    else
				ret = read(fd, read_buf, count);

		    if (ret < 0)
				put_status(errno);
		    else
		    {
			put_status(OK);
			com_put_short(ret);
			if (ret > 0)
			    com_put_data((unsigned char *)read_buf,
					 ret);
		    }
		}
		break;
	case DWRITE:
		fd = get_byte(request);
		count = get_short(request);
		fd = map_fd(fd);

		if ((fd == STDOUT) || (fd == STDERR)) {
		    hdi_user_put_line((const char *)request, count);
		    ret = count;
		}
		else 
		    ret = write(fd, (const char *)request, count);

		if (ret < 0)
		    put_status(errno);
		else
		{
		    put_status(OK);
		    com_put_short(ret);
		}
		break;

	case DSEEK:
		{
		    long offset;
		    int whence;

		    fd = get_byte(request);
		    offset = get_long(request);
		    whence = get_byte(request);

		    offset = lseek(map_fd(fd), offset, whence);

		    if (offset == -1)
			put_status(errno);
		    else
		    {
			put_status(OK);
			com_put_long(offset);
		    }
		}
		break;

	case DCLOSE:
		{
		    int errcode = OK;

		    fd = get_byte(request);

		    if (map_fd(fd) < 0)
			errcode = EBADF;
		    else
		    {
			/* If the fd is mapped to one of our standard
			 * files, just unmap it; don't close our file*/
			if (fd_map[fd] > STDERR)
			{
			    if (close(fd_map[fd]) < 0)
				errcode = errno;
			}
			fd_map[fd] = -1;
		    }
		    put_status(errcode);
		}
		break;

	case DTIME:
		put_status(OK);
		com_put_long(time((time_t *)NULL));
		break;
	
	case DSYSTEM:
#if defined(WINDOWS)
		/* System not supported */
		ret = FALSE;
#else
		/* If system was passed a NULL pointer, sz = 2;
		 * in this case we must return TRUE, indicating
		 * we can execute a system command.  (On a host
		 * where system is not implemented, return FALSE.)
		 * If the argument to system is non-NULL, sz > 2,
		 * and we return whatever system returns.
		 */
		if (sz == 2)
		    ret = TRUE;
		else
		    ret = system((const char *)request);
#endif /* WINDOWS */
		put_status(OK);
		com_put_short(ret);
		break;

	case DRENAME:
		{
		const char *old = (const char *)request;
		const char *new = old + strlen(old) + 1;

#ifdef __STDC__
		if (rename(old, new) < 0)
		    put_status(errno);
		else
		    put_status(OK);
#else /* __STDC__ */
#ifdef USG
		if(unlink(new) && errno != ENOENT)
		    put_status(errno);
		else if(link(old, new))
		    put_status(errno);
		else if(unlink(old))
		    put_status(errno);
#else /* assume BSD functionality */
		if (rename(old, new) < 0)
		    put_status(errno);
		else
		    put_status(OK);
#endif /* USG */
#endif /* __STDC__ */
		}
		break;

	case DUNLINK:		/* delete the path */
		if (unlink((const char *)request) < 0)
		    put_status(errno);
		else
		    put_status(OK);
		break;

	case DFSTAT:		/* get stat of handle */
		fd = get_byte(request);
		if (fstat(map_fd(fd), &sb) < 0)
		    put_status(errno);
		else
		{
		    put_status(OK);
		    put_statbuf(&sb);
		}
		break;

	case DSTAT:		/* get stat of filepath */
		if (stat((char *)request, &sb) < 0)
		    put_status(errno);
		else
		{
		    put_status(OK);
		    put_statbuf(&sb);
		}
		break;

	case DISATTY:
		fd = get_byte(request);
		fd = map_fd(fd);
		if ((fd == STDIN) || (fd == STDOUT) || (fd == STDERR))
		    ret = TRUE;
		else
		    ret = isatty(fd);
		put_status(OK);
		com_put_byte(ret);
		break;

	case DSTDARG:		/* get environmental issues */
		{
		    char cmdline[MAX_MSG_SIZE-2];
		    int size;

		    _hdi_io_init();
		    hdi_get_cmd_line(cmdline, sizeof(cmdline));
		    do_redirection(cmdline);
		    size = strlen(cmdline) + 1;
		    put_status(OK);
		    com_put_data((unsigned char *)cmdline, size);
		    put_env(MAX_MSG_SIZE - 2 - size);
		}
		break;
	}

	if (com_put_msg(NULL, 0) != OK)
	{
		hdi_cmd_stat = com_get_stat();
		return NULL;
	}

	return &running;
}

int
hdi_check_for_debug_msg()
{
	const unsigned char *request;
	int sz, cmd, ret;
	unsigned short	fd;
	unsigned short	count;

	request = com_get_msg(&sz, COM_POLL);

	if (request == NULL)
		return OK;

	if (sz < 2 || request[1] != 0xff)
	{
	    hdi_cmd_stat = E_COMM_ERR;
	    return ERR;
	}
	if (request[0] != DWRITE)
	{
	    hdi_cmd_stat = E_NOTRUNNING;
	    return ERR;
    }

	cmd = get_byte(request);
	request++;			/* Skip status field */

	com_init_msg();
	com_put_byte(cmd);
	fd = get_byte(request);
	count = get_short(request);
	fd = map_fd(fd);

	if ((fd == STDOUT) || (fd == STDERR)) {
	    hdi_user_put_line((const char *)request, count);
	    ret = count;
	}
	else 
	{
	    hdi_cmd_stat = E_NOTRUNNING;
	    return ERR;
    }

	if (ret < 0)
	    put_status(errno);
	else
	{
        put_status(OK);
	    com_put_short(ret);
	}
   
	if (com_put_msg(NULL, 0) != OK)
	{
		hdi_cmd_stat = com_get_stat();
		return ERR;
	}
	return OK;
}


/*
 * This function is called during initialization if the debugger wishes
 * to use hdi_inputline to provide user input to the application.  If
 * this function is not called, hdi_user_get_line is expected to return
 * the user input.  Note: hdi_user_get_line is called in any case; its
 * return value is ignored if hdi_async_input has been called.
 */
void
hdi_async_input()
{
    use_async_input = TRUE;
}

void
hdi_flush_user_input()
{
    user_input_end = 0;
    user_input_start = 0;
}



/*
 * FUNCTION 
 *   hdi_inputline(char *buffer, int length)
 *
 *   buffer - buffer containing data to be sent to an application's stdin 
 *            stream.
 *
 *   length - number of data bytes in buffer.  Maximum input buffer is
 *            MAX_MSG_SIZE bytes.
 *
 * DESCRIPTION:
 *   The host debugger has elected to support asynchronous input (i.e.,
 *   typeahead) from its user interface (say, an I/O window in a GUI). 
 *   In such case, hdi_inputline() is used by the debugger to pass
 *   asynchronous input to HDI, which buffers it and passes it to the 
 *   application's stdin stream when requested by the target.  If the target
 *   has already requested input, the buffered data is sent the next
 *   time hdi_poll() is called.  Note that HDI could send the data directly
 *   to the target from this routine, but that might confuse error reporting.
 *
 *   To configure HDI to support asynchronous input, these steps are 
 *   required:
 *
 *   1) After debugger and HDI initialization, but before an
 *      application is executed, the debugger calls hdi_async_input() to 
 *      signal its request for asynchronous input.
 *
 *   2) Each time an application is downloaded or restarted, the
 *      debugger calls hdi_flush_user_input() to discard data buffered
 *      in hdi_inputline().
 *
 *   3) Finally, the debugger must start applications in the background
 *      (i.e., pass GO_BACKGROUND requests to hdi_targ_go()).
 *
 *   Note 1:  This particular feature was added to support DB960's I/O
 *            Window.
 *
 *   Note 2:  Even when configured to use async input, HDI still calls
 *            hdi_user_get_line().  However, the info returned by this
 *            call is ignored.  In this situation, a debugger may want
 *            to take advantage of this "dummy" call to manage its
 *            async input queue.
 *
 * RETURNS: 
 *   None.
 */

void
hdi_inputline(buffer, length)
char *buffer;
int length;
{
	int copy_length = length;
	int i;

	if (user_input_end + copy_length > sizeof(user_input_buf))
		{
        /* reset buffer to start at 0 byte */
		for (i=0; i<user_input_end-user_input_start; i++)
			{
			user_input_buf[i] = user_input_buf[i + user_input_start];
			}
		user_input_end -= user_input_start;
		user_input_start = 0;
		}
	
    if (user_input_end + copy_length > sizeof(user_input_buf))
	    copy_length = sizeof(user_input_buf) - user_input_end;

	if (copy_length > 0)
	{
	    memcpy(user_input_buf + user_input_end, buffer, length);
	    user_input_end += copy_length;
	}
}


/*
 * Check whether the target is waiting for input from the user and
 * whether input has been received.  If so, send the input to the target.
 */
static int
send_user_input()
{
	int chars_available = user_input_end - user_input_start;
	int i, count;

	if (waiting_for_user_input && chars_available > 0)
	{
	    if (chars_available <= read_count)
		    count = read_count;
	    else
		    count = chars_available;

		for (i=user_input_start; i<user_input_start + count; i++)
		{
			if (user_input_buf[i] == '\n')
			{
				read_count = i - user_input_start + 1;
				break;
			}
		}
 
	    com_init_msg();
	    com_put_byte(DREAD);
	    com_put_byte(0);
		if (chars_available <= read_count)
		{
	    	com_put_short(chars_available);
	    	com_put_data((unsigned char *)user_input_buf, chars_available);
	    	user_input_end = 0;
	    	user_input_start = 0;
		}
		else
		{
	    	com_put_short(read_count);
	    	com_put_data((unsigned char *)user_input_buf, read_count);
	    	user_input_start += read_count;
		}

	    waiting_for_user_input = FALSE;

	    if (com_put_msg(NULL, 0) != OK)
	    {
		hdi_cmd_stat = com_get_stat();
		return ERR;
	    }
	}

	return OK;
}


const STOP_RECORD *
handle_stop_msg(sp)
register const unsigned char *sp;
{
    static STOP_RECORD status;
    unsigned long reason;
    REG ip, fp;

    _hdi_running = FALSE;

    reason = get_long(sp);

    ip = get_long(sp);
    fp = get_long(sp);
    _hdi_set_ip_fp(ip, fp);

    if (reason & STOP_EXIT)
        status.info.exit_code = get_long(sp);

    if (reason & STOP_BP_SW)
    {
	status.info.sw_bp_addr = get_long(sp);

        /* If this is a breakpoint we know about, back up to address of
	 * breakpoint. */
	if (hdi_bp_type(status.info.sw_bp_addr) == BRK_SW)
	    hdi_reg_put(REG_IP, status.info.sw_bp_addr);
	else
	    reason |= STOP_UNK_BP;
    }

    if (reason & STOP_BP_HW)
        status.info.hw_bp_addr = get_long(sp);

    if (reason & STOP_BP_DATA0)
	status.info.da0_bp_addr = get_long(sp);

    if (reason & STOP_BP_DATA1)
	status.info.da1_bp_addr = get_long(sp);

    if (reason & STOP_TRACE)
    {
	status.info.trace.type = get_byte(sp);
	status.info.trace.ip = get_long(sp);
    }

    if (reason & STOP_FAULT)
    {
	status.info.fault.type = get_byte(sp);
	status.info.fault.subtype = get_byte(sp);
	status.info.fault.ip = get_long(sp);
	status.info.fault.record = get_long(sp);
    }

    if (reason & STOP_INTR)
	status.info.intr_vector = get_byte(sp);

    status.reason = reason;

    hdi_cmdext(HDI_EEXIT, (unsigned char *)&status, 0);

    return &status;
}

static int
new_fd(fd)
int fd;
{
    register int i;

    if (fd < 0)
	return -1;

    for (i = 0; i < sizeof(fd_map)/sizeof(fd_map[0]); i++)
	if (fd_map[i] == -1)
	{
	    fd_map[i] = fd;
	    return i;
	}

    return -1;
}


static int
map_fd(fd)
int fd;
{
    if (fd < 0 || fd >= sizeof(fd_map)/sizeof(fd_map[0]))
	return -1;

    return fd_map[fd];
}


/*
 * Transfer the hosts environment to a buffer, using the standard 
 * environ format.
 */
static void
put_env(max_size)
int max_size;
{
        extern char **environ;
	char **ep = environ;		/* global */
	int size;
        int total_size = 0;

	for (; *ep; ++ep) {
		size = strlen(*ep) + 1;
		if (total_size += size >= max_size)
		    break;		    /* environment space too large */
		com_put_data((unsigned char *)*ep, size);
	}
	com_put_byte('\0');
}

void
_hdi_io_init()
{
	register int i;

	for (i = 0; i < sizeof(fd_map)/sizeof(fd_map[0]); i++)
	{
	    if (fd_map[i] > STDERR)
		close(fd_map[i]);
	    fd_map[i] = -1;
	}

	fd_map[STDIN] = STDIN;
	fd_map[STDOUT] = STDOUT;
	fd_map[STDERR] = STDERR;
}

/*
 * Set up for the target's runtime by building its command line into the 
 * buffer provided.  Implement IO redirection required by analyzing
 * the command line.
 */
static void
do_redirection(cmdline)
char *cmdline;
{
#define	DELS	" \t\n\r"
	char	*ip = NULL, *op = NULL;

	/* should do filename wildcards, too */
	ip = strchr(cmdline, '<');
	op = strchr(cmdline, '>');

	if (ip != NULL) {
		*ip++ = '\0';
		ip = strtok(ip, DELS);
	}

	if (op != NULL) {
		*op++ = '\0';
		op = strtok(op, DELS);
	}

	if (ip != NULL && (fd_map[STDIN] = open(ip, O_RDONLY|O_TEXT, 0)) < 0)
	    hdi_put_line("Cannot open STDIN\n");

	if (op != NULL &&
	    (fd_map[STDOUT] =
	      open(op, O_TRUNC|O_CREAT|O_WRONLY|O_TEXT, S_IWRITE|S_IREAD)) < 0)
	    hdi_put_line("Cannot open STDOUT\n");
}

static void
put_statbuf(sb)
struct_stat *sb;
{
	com_put_short(sb->st_dev);
	com_put_short(sb->st_ino);
	com_put_short(sb->st_mode);
	com_put_short(sb->st_nlink);
	com_put_short(sb->st_uid);
	com_put_short(sb->st_gid);
#ifdef MSDOS
	com_put_short(0);
#else
	com_put_short(sb->st_rdev);
#endif
	com_put_long(sb->st_size);
	com_put_long(sb->st_atime);
	com_put_long(sb->st_mtime);
	com_put_long(sb->st_ctime);
}

/*
 * Translate toolset fcntl values to the host system values.
 * If the values in $G960BASE/include/sys/fcntl.h
 * or $I960BASE/include/fcntl.h
 * change, the TOOL_* values below must change also.
 */

#define  TOOL_O_RDONLY  0x0000  /* read only */
#define  TOOL_O_WRONLY  0x0001  /* write only */
#define  TOOL_O_RDWR    0x0002  /* read/write, update mode */
#define  TOOL_O_APPEND  0x0008  /* append mode */
#define  TOOL_O_CREAT   0x0100  /* create and open file */
#define  TOOL_O_TRUNC   0x0200  /* length is truncated to 0 */
#define  TOOL_O_EXCL    0x0400  /* used with O_CREAT to produce an error if file 
                                   already exists */
#ifdef MSDOS
#define  TOOL_O_TEXT    0x4000  /* ascii mode, <cr><lf> xlated */
#define  TOOL_O_BINARY  0x8000  /* mode is binary (no translation) */
#endif /* MSDOS */

static int
xlate_flags(flags)
int flags;
{
	int new_flags = 0;

	switch(flags & 0x3)
	{
	case TOOL_O_RDONLY:
		new_flags = O_RDONLY;
		break;
	case TOOL_O_WRONLY:
		new_flags = O_WRONLY;
		break;
	case TOOL_O_RDWR:
		new_flags = O_RDWR;
		break;
	default: /* can't ever happen (yeah, sure) */
		new_flags = 0;
		break;
	}

	if (flags & TOOL_O_APPEND)
		new_flags |= O_APPEND;

	if (flags & TOOL_O_CREAT)
		new_flags |= O_CREAT;

	if (flags & TOOL_O_TRUNC)
		new_flags |= O_TRUNC;

	if (flags & TOOL_O_EXCL)
		new_flags |= O_EXCL;

#ifdef MSDOS
	if (flags & TOOL_O_BINARY)
		new_flags |= O_BINARY;
	else
		/* We default to TEXT mode. */
		new_flags |= O_TEXT;
#endif /* MSDOS */

   return new_flags;
}
