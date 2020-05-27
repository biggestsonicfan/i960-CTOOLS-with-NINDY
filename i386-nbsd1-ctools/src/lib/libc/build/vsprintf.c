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

/* vsprintf - print to memory
 * Copyright (c) 1984,85,86,87 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>


int vsprintf(char *s, const char *format, va_list args)
{
    int     i;
    FILE    file;

    /*
     * Initialize the file structure for use by _Ldoprnt():
     */
    file._ptr = (unsigned char *)s;
    file._base = (unsigned char *)s;
    file._cnt = 0x7FFF;			/* reset the length */
    file._flag = _IOWRT;
    file._fd = FOPEN_MAX;
    file._size = 2;			/* >1 */
    file._sem = NULL;			/* no semaphore functions required
					   since only one thread of execution
					   can use this file structure */

    i = _Ldoprnt(format, &args, &file, _putch);

    /*
     * Terminate the buffer:
     */
    s[i] = _NUL;				/* only one 'L' !!! */
    return i;
}
