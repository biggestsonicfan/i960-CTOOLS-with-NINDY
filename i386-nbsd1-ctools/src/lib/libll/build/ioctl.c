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

#include "errno.h"
#include "sdm.h"
#define	ENOTTY	25	/* Not a typewriter */

int
ioctl(int fd, int cmd, int arg)
{
	/*
	 * We currently don't support any configuration of tty ports;
	 * however, the HLL calls this to determine whether or not fd
	 * refers to a tty.  So, if fd refers to a tty, return 0.
	 * Otherwise return -1. */
	/* NOTE: This must not call isatty - there is an isatty in the
	 * HLL as well as the one in the LLL.  Calling the one in the
	 * LLL would be fine, but the one in the HLL calls this routine.
	 * We can't tell which one was linked.
	 */
	int result;
	int r = sdm_isatty(fd, &result);
	if (r != 0)
	{
	    errno = r;
	    return -1;
	}
	if (!result)
	{
	    errno = ENOTTY;
	    return -1;
	}
	return 0;
}
