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

/* ecvt - float to ASCII
 * Copyright (c) 1986,87 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static char buf[315];

static int
cp_c(int c, char **f)
{
  return (*(*f)++ = c);
}

_Lflt_interface(int prec,
               int format,
               int width,
               int alt,
               int csign,
               int leftadj,
               int padchar,
               char **buf_p,
               int longflg,
               ...)
{
  va_list args;
  va_start(args, longflg);
  _Lfltprnt(prec, &args, format, width, alt, csign, leftadj, padchar,
            (FILE *)buf_p, cp_c, longflg);
  va_end(args);
}

char *ecvt(double value, int digits, int *point, int *sign)
{
    char buffer[315], *p0, *p1;
    int i;

    for (i = 0; i < 315; i++)
      buf[i] = 0;

    if (digits <= 0) {
        buf[0] = 0;
        *point = 0;
    }
    else {
        char *str_ptr = buffer;
        _Lflt_interface(digits-1, 'e', 0, 0, '+', 0, ' ', &str_ptr, 0, value);

        *sign = (buffer[0] != '+');
        p0 = buf;
        *p0++ = buffer[1];
        p1 = &buffer[3];
        while (*p1 != 'e') {
            *p0++ = *p1++;
            --digits;
        }
        for (i = 0; i < digits - 1; i++)
          p0[i] = '0';
        p1++;

        {
          int t = 0;
          int is_neg = 0;
          if (*p1 == '-' || *p1 == '+')
          {
            is_neg = *p1 == '-';
            p1 ++;
          }
            
          while (*p1 >= '0' && *p1 <= '9')
            t = t*10 + (*p1++ - '0');

          if (is_neg)
            t = -t;

          *point = t + 1;
        }
    }
    return buf;
}
