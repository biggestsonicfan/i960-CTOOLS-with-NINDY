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

union  fp_overlay  {
    float               fp_item;
    long unsigned int   uint;
};


float  _AFP_NaN_S(float xx, float yy)
{
    union  fp_overlay   x, y;

    errno = EDOM;

    x.fp_item = xx;
    y.fp_item = yy;

    x.uint |= 0x80400000;
    y.uint |= 0x80400000;

    if  (x.uint >= y.uint)  {
        return(x.fp_item);
    }  else  {
        return(y.fp_item);
    }
}


float  _AFP_INF_S(int  neg_sign)
{
    union  fp_overlay   x;


    errno = ERANGE;

    x.uint = 0x7F800000;

    if  (neg_sign != 0)  {
        x.uint |= 0x80000000;
    }

    return(x.fp_item);
}


float   _AFP_QNaN_S( void )
{
    union  fp_overlay   x;


    errno = EDOM;

    x.uint = 0xFFC00000;

    return(x.fp_item);
}


float   _AFP_mZERO_S( void )
{
    union  fp_overlay   x;


    x.uint = 0x80000000;

    return(x.fp_item);
}
