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

/* memicmp - case-insentitive buffer comparison
 * Copyright (c) 1986 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <ctype.h>
#include <string.h>

#define mtoupper(x) (islower(x) ? _toupper(x) : x)

int memicmp(const void *buf1, const void *buf2, unsigned count)
{
    char *str1, *str2;
    char c1,c2;

    str1 = (char *)buf1;
    str2 = (char *)buf2;

    while (count--) {
        if (mtoupper(*str1) != mtoupper(*str2)) break;
        str1++;
        str2++;
    }

    c1 = *str1;
    c2 = *str2;

    if (!isalpha(c1) || !isalpha(c2))
        return c1 - c2;
    return mtoupper(c1) - mtoupper(c2);
}
