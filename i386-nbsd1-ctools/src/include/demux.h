
/*(c****************************************************************************** *
 * Copyright (c) 1990, 1991, 1992, 1993 Intel Corporation
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
 *****************************************************************************c)*/

/******************************************************************************
 * This include file supports demultiplexing of input from two sources:
 * stdin and one external source (normally the NINDY monitor).
 *
 * Its purpose is to promote portability of applications between different
 * flavors (BSD and USG/SysV) of Unix.  As of this writing, it is used by the
 * gdb960 remote communications module and the comm960 utility.
 * 
 * It is assumed that 'USG' is defined on the compiler invocation line if the
 * code should compile and run on a USG/SysV system.  Otherwise, BSD is assumed.
 *
 * The application code must use all three of these macros:
 *
 *	DEMUX_DECL	Declares data structures needed by the other macros.
 *
 *	DEMUX_WAIT(fd)	Waits until input is available on either stdin or
 *			file descriptor 'fd'.
 *
 *	DEMUX_READ(fd,bufp,bufsz)
 *			Reads up to 'bufsz' bytes from file descriptor 'fd'
 *			into buffer pointed at by character pointer 'bufp'.
 *			Returns the number of bytes read, which will be 0
 *			if there was no input pending on 'fd'. 'fd' should be
 *			either 0 (stdin) or the same descriptor that was used
 *			in the invocation of 'DEMUX_WAIT'.
 *
 * WARNINGS ABOUT USG (System V) UNIX!!
 *
 *	The damned 'poll' call can't be used on normal tty's, so DEMUX_WAIT is
 *	also a no-op: DEMUX_READ uses the FIONREAD ioctl if it's available;
 *	otherwise the file descriptor is temporarily set for non-blocking input
 *	and a read is done.
 *
 ******************************************************************************/

#define __DEMUX_SEL_DECL__	fd_set _mux_

#define __DEMUX_SEL_WAIT__(fd)	{				\
		FD_ZERO( &_mux_ );				\
		FD_SET( 0, &_mux_ );				\
		FD_SET( fd, &_mux_ );				\
		/* Check return value of select in case of	\
		 * premature termination due to signal:  clear	\
		 * file descriptors in this case so DEMUX_READ	\
		 * doesn't mistakenly say there's input on them.\
		 */						\
		if (select(fd+1,&_mux_,0,0,0) <= 0){		\
			FD_ZERO(&_mux_);			\
		}						\
}

#define __DEMUX_SEL_READ__(fd,bufp,bufsz) \
			( FD_ISSET(fd,&_mux_) ?	read(fd,bufp,bufsz) : 0 )


#ifdef POSIX

#	ifndef DEC3100
#		include <sys/select.h>
#	endif
#	include <sys/types.h>

#	define DEMUX_DECL		__DEMUX_SEL_DECL__
#	define DEMUX_WAIT(fd)		__DEMUX_SEL_WAIT__(fd)
#	define DEMUX_READ(fd,bufp,bufsz) __DEMUX_SEL_READ__(fd,bufp,bufsz) 

#else
#ifdef DOS

#	include <fcntl.h>
#	define DEMUX_DECL		int __DOS_DUMMY__
#	define DEMUX_WAIT(fd)
#	define DEMUX_READ(fd,bufp,bufsz)	\
	    (fd == 0 ? read_stdin_noecho(bufp,bufsz) : read_port(fd,bufp,bufsz))

#else
#ifdef USG

#	include <fcntl.h>
#	define DEMUX_DECL		int _saveflags_; int _n_
#	define DEMUX_WAIT(fd)

	/* Use non-blocking I/O */
#	define DEMUX_READ(fd,bufp,bufsz) (				\
			_saveflags_ = fcntl( fd, F_GETFL, 0 ),		\
			fcntl( fd, F_SETFL, _saveflags_ | O_NDELAY ),	\
			_n_ = read( fd, bufp, bufsz ),			\
			fcntl( fd, F_SETFL, _saveflags_ ),		\
			_n_ )

#else	/* BSD */

#	include <sys/types.h>
#	include <sys/time.h>
#	include <fcntl.h>		/* for open modes, toolib/nindy.c */

#	define DEMUX_DECL		__DEMUX_SEL_DECL__
#	define DEMUX_WAIT(fd)		__DEMUX_SEL_WAIT__(fd)
#	define DEMUX_READ(fd,bufp,bufsz) __DEMUX_SEL_READ__(fd,bufp,bufsz) 

#endif
#endif
#endif
