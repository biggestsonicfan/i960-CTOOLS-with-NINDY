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

/* tmpnam - create a unique temporary file name
 * Copyright (c) 1984,85,86 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stat.h>

static unsigned rand_num;		/* random number */
static char prefix[4];

char *tmpnam(s)
char *s;
{
    char buf[16];
    static char result[L_tmpnam];
    register char *p;
	struct stat stat_buf;

    /* static char prefix[] = "aaa"; */
    /* changed to remove 'prefix' from .data section */
    /* new code puts prefix in .bss, which is set to zeros. */
    if (prefix[0] == (char) 0)
    {
        prefix[0] = 'a';
        prefix[1] = 'a';
        prefix[2] = 'a';
        prefix[3] = '\0';
    }
    if (!s)
        s = result;

    /*
     * Generate a temporary file name and verify that a file by that name
     * does not exist.  If the file does exist, continue generating
     * temporary file names until non-file name is generated (stat fails).
     */
    do {
        rand_num = ((rand_num * 1103515245L + 12345) & 0x7fff);
        strcpy(s,prefix);
        itoa(rand_num, buf, 10);
        strcat(s, buf);

        p = prefix;
        while(*p == 'z')
    	    *p++ = 'a';
        if (*p != '\0')
    	    ++*p;
    } while (stat(s, &stat_buf) == 0);
   
    return s;
}
