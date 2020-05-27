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

/* ultoa - convert unsigned long to string
 * Copyright (c) 1986 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <stdlib.h>
#include <string.h>

struct ediv_result {
	long rem;
	long quot;
};

/* divide x by y */
static
__inline__ struct ediv_result ediv(unsigned long x, unsigned long y)
{
  struct ediv_result ret;
  struct long_long
  {
        unsigned long lo;
        unsigned long hi;
  } x_ext;

  x_ext.lo = x;
  x_ext.hi = 0;
  __asm__ ("ediv	%2,%1,%0": "=t"(ret) : "tI"(x_ext),"dI"(y));
  return ret;
}

char *ultoa(unsigned long value, char *buf, int base)
{
    static const unsigned char cbuf[36] = "0123456789abcdefghijklmnopqrstuvwxyz";
    unsigned char rbuf[34];
    register int i = 33;
    struct ediv_result result;

    if (base < 2 || base > 36)
        *buf = 0;
    else {
        rbuf[i--] = 0;
        if (value == 0L) {		/* special case for value equal zero */
            buf[0] = '0';
            buf[1] = 0;  
        }
        else {				/* for value greater than zero */
            while (value) {
		result = ediv(value, base);
                rbuf[i--] = cbuf[result.rem];
                value = result.quot;
            }
            strcpy(buf, (const char *)&rbuf[i + 1]);
        }
    }
    return buf;
}
