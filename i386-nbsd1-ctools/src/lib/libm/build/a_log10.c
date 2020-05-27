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

/* log10 - log base 10
 * Copyright (c) 1984,85,86,87 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <math.h>
#include <errno.h>
#include "lm_math.h"

#define M_LOG10E	0.43429448190325182765

double (log10)(double val)
{
    double temp;
    register _temp;
    int save_errno;
    struct _exception exc;

    if (val > 0.0) { 			/* argument is in the domain */
        save_errno = errno;		/* save original errno */
        errno = 0;
	temp = log(val) * M_LOG10E;
        errno = save_errno;		/* restore orginal errno */
    }
    else {
        if (val == 0.0)
            errno = ERANGE;
        else
            errno = EDOM;
        temp = -HUGE_VAL;
        exc.type = DOMAIN;
        goto L10;
    }
    if ((_temp = 0) & 0x80) {
        exc.type = _temp;
L10:
        exc.name = "log10";
        exc.arg1 = val;
        exc.arg2 = 0;
        exc.retval = temp;
        if (_Lmatherr(&exc))
            temp = exc.retval;
    }
    return temp;
}
