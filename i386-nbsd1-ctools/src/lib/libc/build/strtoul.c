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

/* strtoul - convert a string to unsigned long
 * Copyright (c) 1984,85,86 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <stddef.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#define NEGATIVE	1
#define	POSITIVE	0


unsigned long int strtoul(const char *buf, char **end, int base)
{
    int		index;
    int		sign;
    int		save_errno;
    const char	*save_buf;
    const char	*pre_convert_buf = NULL;
    unsigned long	el = 0L;


    /*
     * Save the value of the pointer to the buffer's first
     * character, save the current errno value, and then
     * skip over any white space in the buffer:
     */
    save_errno = errno;
    errno = 0;
    save_buf = buf;
    while (isspace(*buf) || *buf == '\t') {
        ++buf;
    }

    /*
     * The buffer may contain an optional plus or minus sign.
     * If it does, then skip over it but remember what it was:
     */
    if (*buf == '-') {
        sign = NEGATIVE;
        ++buf;
    } else if (*buf == '+') {
        ++buf;
        sign = POSITIVE;
    } else {
        sign = POSITIVE;
    }

    /*
     * If the input parameter base is zero, then we need to
     * determine if it is octal, decimal, or hexadecimal:
     */
    if (base == 0) {
        if (*buf == '0') {
            if (tolower(*(buf+1)) == 'x') {
                base = 16;
		buf +=2;	/* Skip over 0[xX] */
            } else {
                base = 8;
            }
        } else {
            base=10;
        }
    } else if (base == 16) {
	/* For hexadecimal, skip over the leading 0[xX], if it is present. */
	if (*buf == '0' && tolower(*(buf+1)) == 'x') {
		buf += 2;
	}
    } else if (base < 2 || base > 36) {
        /*
         * The specified base parameter is not in the domain of
         * this function:
         */
        goto done;
    }


    /*
     * Main loop:  convert the string to an unsigned long:
     */
    pre_convert_buf = buf;
    while (*buf) {
        if (isdigit(*buf)) {
            index = *buf - '0';
        } else {
            index = toupper(*buf);
            if (isupper(index))
                index = index - 'A' + 10;
            else
                goto done;
        }
        if (index >= base)
            goto done;

        /*
         * Check to see if value is out of range:
         */
        if (el > ((ULONG_MAX - (unsigned long)index) / (unsigned long)base)) {
            errno = ERANGE;
            el = 0L;			/* reset */
            break;
        }
        el *= base;
        el += index;
        ++buf;
    }

done:
    /*
     * If appropriate, update the caller's pointer to the next
     * unconverted character in the buffer.  Also, either set 
     * the return value for the ERANGE case or restore errno:
     */
    if (end) {
        /*Did we convert any of the string?*/
        if ( buf == pre_convert_buf || buf == save_buf)
            *end = (char *)save_buf;
        else
            *end = (char *)buf;
    }
    if (errno == ERANGE)
        el = ULONG_MAX;
    else
		errno = save_errno;
    /*
     * If a minus sign was present, then "the conversion is negated":
     */
    if (sign == NEGATIVE) {
        el = (ULONG_MAX - el) + 1;
    }
    return (el);
}
