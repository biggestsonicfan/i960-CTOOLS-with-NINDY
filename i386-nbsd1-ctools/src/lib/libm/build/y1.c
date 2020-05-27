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

/* library function: Bessel Function - Y1
 * Copyright (c) 1987 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include "lm_math.h"

double y1(arg)
double arg;
{
    double temp;
    register _temp;
    struct _exception exc;

    if (arg <= 0.0) {
        errno = EDOM;
        temp = -HUGE_VAL;
        exc.type = DOMAIN;
        goto SQ1;
    }
    temp = _y1(arg);
    if ((_temp = 0) & 0x80) {
        exc.type = _temp;
SQ1:
        exc.name = "y1";
        exc.arg1 = arg;
        exc.arg2 = 0;
        exc.retval = temp;
        if (_Lmatherr(&exc))
            temp = exc.retval;
    }
    return temp;
}
