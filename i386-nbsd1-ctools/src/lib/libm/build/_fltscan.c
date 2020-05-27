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

/* _Lfltscan - floating point support for _Ldoscan()
 * Copyright (c) 1987 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>


#define	isdigit(c)	( (c) >= '0' && (c) <= '9' )


extern  long double  _Lstrtoe(const char *,char **);
extern  float        _Lstrtof(const char *,char **);


#if (__PIC)
#pragma optimize(read_char="notce") /* Needed for PIX960 */
#endif


static int
read_char(FILE *str, unsigned *width, int (*fp)())
{
    (*width)--;
    return (*fp)(str);
}


static void
unread_char(int c, FILE *str, unsigned *width, int (*ufp)())
{
    (*width)++;
    if (c != EOF)
        (*ufp)(c, str);
}


int _Lfltscan(stream, width, longflg, ap, suppress, fp, ufp)
FILE *stream;
unsigned width;
unsigned longflg;
va_list *ap;
unsigned suppress;
int (*fp)();
int (*ufp)();
{
    int          c;
    char *       bptr;
    char         buffer[350];
    float        f;
    double       d;
    long double  e;
    int          local_errno;


    bptr = buffer;                          /*  Temp buffer pointer  */
    c    = read_char(stream, &width, fp);   /*  Read-ahead character  */

    width++;


    /*  Leading sign?  */

    if  ( c == '-'  ||  c == '+' )  {
        *bptr++ = c;
        c       = read_char(stream, &width, fp);
    }


    /*  Digits left of decimal point  */

    while  (isdigit(c) && width)  {
        *bptr++ = c;
        c       = read_char(stream, &width, fp);
    }


    /*  Decimal point plus digits to the right of the decimal point  */

    if  (c == '.' && width)  {
        *bptr++ = '.';
        c       = read_char(stream, &width, fp);

        while  (isdigit(c) && width)  {
            *bptr++ = c;
            c       = read_char(stream, &width, fp);
        } 
    }


    /*  Trailing exponent field  */

    if  ((c == 'e' || c == 'E') && width)  {
        *bptr++ = c;
        c       = read_char(stream, &width, fp);

        /*  Copy the exponent sign as req'd  */

        if  ((c == '-'  ||  c == '+') && width) {
            *bptr++ = c;
            c       = read_char(stream, &width, fp);
        }

        /*  Require at least one exponent digit  */

        if  (!isdigit(c) || !width) {
            unread_char(c, stream, &width, ufp);
            return 0;
        }  else  {
            while (isdigit(c) && width) {
                *bptr++ = c;
                c       = read_char(stream, &width, fp);
            }
        }
    }


    unread_char(c, stream, &width, ufp); /* Release the read-ahead character */

    if  (bptr == buffer)            /*  If no characters -> return  */
        return 0;

    *bptr = 0;                      /*  Add a null byte terminator  */


    if  (!suppress)  {                      /*  Cnvt val if not "suppressed"  */

        bptr        = va_arg(*ap, char *);  /*  Point to destination  */
        local_errno = errno;
        errno       = 0;

        switch  (longflg)  {
        case 2:
                e = _Lstrtoe(buffer,NULL);
                if  (errno != EDOM)  {
                    *((long double *)bptr) = e;
                }
                break;

        case 1:
                d = strtod(buffer,NULL);
                if  (errno != EDOM)  {
                    *((double      *)bptr) = d;
                }
                break;

        case 0:
                f = _Lstrtof(buffer,NULL);
                if  (errno != EDOM)  {
                    *((float       *)bptr) = f;
                }
                break;
        }

        /*  If ill-formed number, return 0 width (dest left unchanged)  */

        if  (errno == EDOM)  {
            errno = local_errno;        /*  Restore errno value  */
            return 0;
        }  else  {
            errno = local_errno;        /*  Restore errno value  */
        }

    }

    return width;
}
