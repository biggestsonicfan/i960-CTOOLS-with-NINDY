/******************************************************************/
/* 		Copyright (c) 1989,1990,1991 Intel Corporation

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

   Intel Corporation does not warrant, guarantee or make any
   representations regarding the use of, or the results of the use
   of, the software and documentation in terms of correctness,
   accuracy, reliability, currentness, or otherwise; and you rely
   on the software, documentation and results solely at your own
   risk.							  */
/******************************************************************/

/********************************************************
 * This comment attempts to explain the structure used
 * for system calls.
 *
 *  - Serial I/O calls (read and write to STDIN, STDIO,
 * STDERR) are made through a 960 system call to console().
 *
 *  - File I/O calls (and all other system calls) are made through
 * a 960 system call to files().  All the arguments from the
 * calling procedure are passed directly through, with the name
 * of the calling procedure inserted as the first argument.
 *
 *  - Laser I/O calls (read from LPT1, no write error messages
 * are made through a 960 system call to laser().
 *
 * These calls go through the System Procedure Table as
 * calls 0, calls 1 and calls 2, and are then routed to the
 * routines on the NINDY side, console_io(), file_io(), and
 * lpt_io() respectively.  The arguments passed along
 * from the original calling procedure are held in the
 * global variables until they reach their final destination,
 * the procedure on the NINDY side which actually does the
 * work.
 *
 * This indirection was chosen so that only the hardware
 * dependent pieces of NINDY would have to be changed for
 * I/O to different devices.  This also allows NINDY to
 * handle any errors which may occur.
********************************************************/
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>

#include "block_io.h"

/*
 * Next bit of stuff for timing.
 */
unsigned long __elapsed_time = 0;
static unsigned long clk_start_time;
static int time_on = 0;

_start_time()
{
  if (time_on == 0)
  {
    init_bentime(25);
    eat_time(4000);
    clk_start_time = bentime();
    time_on = 1;
  }
}

_stop_time()
{
  if (time_on != 0)
  {
    __elapsed_time += bentime() - clk_start_time;
    time_on = 0;
  }
}

_print_time()
{
  unsigned long t_sec;
  unsigned long t_tenth;

  _stop_time();
  t_tenth = __elapsed_time / 100000;
  t_sec   = t_tenth / 10;
  t_tenth = t_tenth % 10;
  fprintf (stderr, "\t%d.%d real\t%d.%d user\t0.0 sys\n", 
           t_sec, t_tenth, t_sec, t_tenth);
}

#define STDIN 	0
#define STDOUT 	1
#define STDERR 	2
#define FALSE 	0
#define TRUE 	1
#define DEL	0x7f

struct {		/* IOCTL structure */
	short lpt;	/* read from lpt port */
	short crlf;	/* turn on CR/LF translation */
	short g960_echo;   /* Echo chars read on stdin to stdout. */
	short g960_cbreak; /* Do not buffer (wait for newline) chars. */
} iostruct = {0,1,1,0};

/***************************************/
/* Access                              */
/*                                     */
/* determines accessibility of a file  */
/***************************************/
access(path, mode)
char *path;
int mode;
{
	int ret;
	_stop_time();
	ret = files(BS_ACCESS, path, mode);
        _start_time();
	return ret;
}

/***************************************/
/* Close                               */
/*                                     */
/* closes a file descriptor            */
/***************************************/
close(fd)
int fd;
{
	int ret;
	if ( fd > 2 ){
 		_stop_time();
		ret = files(BS_CLOSE,fd);
		_start_time();
		return ret;
	}
}

/***************************************/
/* Creat                               */
/*                                     */
/***************************************/
creat(path, mode)
char *path;
int mode;
{
	int ret;
	_stop_time();
	ret = files(BS_CREAT, path, mode);
	_start_time();
	return ret;
}

/***************************************/
/* Lseek                               */
/*                                     */
/* moves the file pointer w/in the file*/
/***************************************/
lseek(fd, offset, how)
int fd;
long offset;
int how;
{
	int ret;
	_stop_time();
	ret = files(BS_SEEK, fd, offset, how);
	_start_time();
	return ret;
}

/***************************************/
/* tell                                */
/*                                     */
/* tell where the file pointer is      */
/***************************************/
tell(fd)
int fd;
{
	return lseek(fd, (long)0, SEEK_CUR);
}

/***************************************/
/* Open                                */
/*                                     */
/* opens a file for read/write         */
/***************************************/
open(path, flag, mode)
char *path;
unsigned int flag;
int mode;
{
	int ret;
	_stop_time();
	ret = files(BS_OPEN, path, flag, mode);
	_start_time();
	return ret;
}

/***************************************/
/* Spawnd                              */
/*                                     */
/***************************************/
spawnd(path, argv)
char *path;
char *argv[];
{
	return ERROR;
}

/***************************************/
/* Stat                                */
/*                                     */
/* get status on file		       */
/***************************************/
stat (path, buff)
char *path;
struct st *buff;
{
	int ret;
	_stop_time();
	ret = files(BS_STAT, path, buff);
	_start_time();
	return ret;
}
/***************************************/
/* System                              */
/*                                     */
/***************************************/
systemd(string)
char *string;
{
	int ret;
	_stop_time();
	ret = files(BS_SYSTEMD, string);
	_start_time();
	return ret;
}

/***************************************/
/* Time                                */
/*                                     */
/***************************************/
time(t)
long *t;
{
long tmp;

	_stop_time();
	tmp = files(BS_TIME);
	_start_time();
	if (t != 0)
		*t = tmp;
	return tmp;
}

/***************************************/
/* Unmask                              */
/*                                     */
/***************************************/
unmask(mode)
int mode;
{
	int ret;
	_stop_time();
	ret = files(BS_UNMASK, mode);
	_start_time();
	return ret;
}

/***************************************/
/* Unlink                              */
/*                                     */
/***************************************/
unlink(path)
char *path;
{
	int ret;
	_stop_time();
	ret = files(BS_UNLINK, path);
	_start_time();
	return ret;
}

/***************************************/
/* Read                                */
/*                                     */
/***************************************/
read(fd, buff, n)
    int fd;
    unsigned char *buff;
    int n;
{
    /* for now, assume that CR/LF is translated to LF.  this
       should really be controlled by ioctl */

    unsigned char c;
    int i, cnt, recvd;

    if (fd != STDIN) {
	for ( recvd = 0; n > 0; n -= BUFSIZE ){
	    cnt = n > BUFSIZE ? BUFSIZE : n;
	    _stop_time();
	    i = files(BS_READ, fd, buff+recvd, cnt);
	    _start_time();
	    if ( i == ERROR )
		    return ERROR;
	    recvd += i;
	    if ( i != cnt ){
		break;
	    }
	}
    }
    else {	/* STDIN */

	if ( iostruct.lpt ) {		/* STDIN is associated with LASER */
	    for ( recvd=0; recvd < n; recvd++ ) {
		if ( laser(CI,&c) ){
		    break;	/* NO character */
		}
		buff[recvd] = c;
	    }
	}
	else {/* INPUT IS FROM CONSOLE */
	    int terminate = 0;

	    for ( c=recvd=0; ! terminate;) {
		if ( console(CI, &c) == ERROR ){
		    recvd = ERROR;
		    break;
		}
		if (!iostruct.g960_cbreak) {
		    if ( (c == DEL) || (c == '\b') ){
			/* remove previous char, if there */
			/* ignore otherwise */
			if (recvd > 0) {
			    recvd--;
			    console(CO, '\b');
			    console(CO, ' ');
			    console(CO, '\b');
			}
		    }
		    else {
			if ( c == '\r') {
			    if (iostruct.g960_echo)
				    console(CO, '\r');
			    c = '\n';
			}
			if (iostruct.g960_echo)
				console(CO,c);
		    }
		    buff[recvd++] = c;
		    terminate = (recvd >= n) || (c == '\n');
		}
		else { /* G960_CBREAK MODE */
		    if (iostruct.g960_echo)
			    console(CO,c);
		    buff[recvd++] = c;
		    terminate = (recvd >= n);
		}
	    }
	}
    }
    return recvd;
}

/***************************************/
/* Write                               */
/*                                     */
/***************************************/
write(fd, buff, n)
int fd;
unsigned char *buff;
int n;
{
/*   for now (first revision) this routine is assumed to do
	translation of LF to CR/LF sequence (for ttys).  This
	should be made controllable through ioctl() someday.
*/

int sent, cnt, i;

	if ((fd != STDOUT) && (fd != STDERR)) {
		for ( sent = 0; n > 0; n -= BUFSIZE ){
			cnt = n > BUFSIZE ? BUFSIZE : n;
			_stop_time();
			i = files(BS_WRITE, fd, buff+sent, cnt);
			_start_time();
			if ( i == ERROR ){
				return ERROR;
			}
			sent += i;
			if ( i != cnt ){
				break;
			}
		}

	} else {	/* STDOUT or STDERR */

		if ( iostruct.lpt ){	/* TO LASER */

			while (n--){
				if ( laser(CO,*buff++) ){
					break;
				}
			}

		} else {		/* TO CONSOLE */

			for ( sent=0; sent<n && sent != ERROR; buff++ ){
				sent++;
				if ( (*buff == '\n') ){
					if ( (console(CO,'\r') == ERROR) ){
						sent = ERROR;
					}
				}
				if (console(CO,*buff) == ERROR){
					sent = ERROR;
				}
			}
		}
	}
	return sent;
}

/***************************************/
/* ioctl			       */
/*                                     */
/***************************************/
#ifndef __GNUC__
#define _SETERRNO(x)	errno = (x);
#endif

ioctl(fd, request,ptr)
    int fd;
    int request;
    int *ptr;
{
    int rv;

    if ((fd != STDIN) && (fd != STDOUT) && (fd != STDERR)) {
	_SETERRNO(EBADF);
	/* should return ERROR; doesn't for ic960 compatability */
	return EBADF;
    }

#ifdef INCOMPATIBLE	/* left out for ic960 compatability */
    if ((request != LPTON) || (request != LPTOFF)) {
	_SETERRNO(EINVAL);
	return ERROR;
    }
#endif

    iostruct.lpt = (request == LPTON) ? TRUE : FALSE;
    switch (request) {
 case LPTON :
	 break;
 case G960_CBREAK :
	 rv = iostruct.g960_cbreak;
	 iostruct.g960_cbreak = *ptr;
	 break;
 case G960_ECHO :
	 rv = iostruct.g960_echo;
	 iostruct.g960_echo = *ptr;
	 break;
 default :
	 rv = ERROR;
	 break;
     }
    return rv;
}

