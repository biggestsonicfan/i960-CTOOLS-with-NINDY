
/*(c**************************************************************************** *
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
 ***************************************************************************c)*/

#include <stdio.h>
#include <fcntl.h>	/* Needed on Sys V */
#include "ttycntl.h"

/******************************************************************************
 * tty_flush:
 *
 *	This routine puts the specified tty into a quiescent state by flushing
 *	all pending input and output.
 *
 *	The tty is assumed to be connected to an i960 board containing a
 *	a NINDY ROM;  since the 960 may be generating output, we wait until
 *	at least one second goes by without anything new arriving.
 ******************************************************************************/

tty_flush( fd )
    int fd;	/* File descriptor of tty line */
{
	int n;	/* Number of characters of pending input */
	char c;	/* Next character of input (discarded) */

	do {
		TTY_FLUSH( fd );
		sleep(1);
		n = 1;
		TTY_NBREAD( fd, n, &c );	/* Non-blocking read */
	} while ( n > 0 );
}
