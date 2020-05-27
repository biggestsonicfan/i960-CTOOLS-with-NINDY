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

#include <errno.h>

union  dp_overlay  {
    double  dp_item;
    struct  {
        long unsigned int   lo_word, hi_word;
    }  uints;
};


double  _AFP_NaN_D(double xx, double yy)
{
    union  dp_overlay   x, y;

    errno = EDOM;

    x.dp_item = xx;
    y.dp_item = yy;

    x.uints.hi_word |= 0x80080000;
    y.uints.hi_word |= 0x80080000;

    if  (    x.uints.hi_word > y.uints.hi_word
         ||  (    x.uints.hi_word == y.uints.hi_word
              &&  x.uints.lo_word >= y.uints.lo_word ) )  {
        return(x.dp_item);
    }  else  {
        return(y.dp_item);
    }
}


double  _AFP_INF_D(int  neg_sign)
{
    union  dp_overlay   x;


    errno = ERANGE;

    x.uints.hi_word = 0x7FF00000;
    x.uints.lo_word = 0x00000000;

    if  (neg_sign != 0)  {
        x.uints.hi_word |= 0x80000000;
    }

    return(x.dp_item);
}


double  _AFP_QNaN_D( void )
{
    union  dp_overlay   x;


    errno = EDOM;

    x.uints.hi_word = 0xFFF80000;
    x.uints.lo_word = 0x00000000;

    return(x.dp_item);
}
