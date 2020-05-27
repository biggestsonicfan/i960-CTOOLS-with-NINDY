
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

/* Define how to access the int that the wait system call stores.
   This has been compatible in all Unix systems since time immemorial,
   but various well-meaning people have defined various different
   words for the same old bits in the same old int (sometimes claimed
   to be a struct).  We just know it's an int and we use these macros
   to access the bits.

   POSIX has changed some of these in an upward-compatible way; we
   should eventually do the same.  (FIXME sometime).  */
   
#define WAITTYPE int
#define WIFSTOPPED(w) (((w)&0377) == 0177)
#define WIFSIGNALED(w) (((w)&0377) != 0177 && ((w)&~0377) == 0)
#define WIFEXITED(w) (((w)&0377) == 0)
#define WRETCODE(w) ((w) >> 8)
#define WEXITSTATUS(w) ((w) >> 8)	/* same as WRETCODE */
#define WSTOPSIG(w) ((w) >> 8)
#define WCOREDUMP(w) (((w)&0200) != 0)
#define WTERMSIG(w) ((w) & 0177)
#define WSETEXIT(w, status) ((w) = ((status) << 8))
#define WSETSTOP(w,sig)  ((w) = (0177 | ((sig) << 8)))
