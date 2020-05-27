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

/* library function: Bessel Function - Y0
 * Copyright (c) 1987 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

/* function:  _y0(x)
 *     Calculate Bessel function Y0(X) for any positive X.
 *
 * Note:  Algorithm taken from:
 *     Numerical Recipes, Cambridge University Press; pg. 173.
 */

#include <stdlib.h>
#include <math.h>
#include "lm_math.h"

#define        P1     1.0		/* Polynomial constants */
#define        P2    -1.098628627e-3
#define        P3     2.734510407e-5 
#define        P4    -2.073370639e-6 
#define        P5     2.093887211e-7 

#define        Q1    -1.562499995e-2    /* Polynomial constants */
#define        Q2     1.430488765e-4 
#define        Q3    -6.911147651e-6 
#define        Q4     7.621095161e-7 
#define        Q5    -9.349451520e-8 

#define        R1    -2.957821389e+9    /* Polynomial constants */
#define        R2     7.062834065e+9 
#define        R3    -5.123598036e+8 
#define        R4     1.087988129e+7 
#define        R5    -8.632792757e+4 
#define        R6     2.284622733e+2 

#define        S1     4.0076544269e+10  /* Polynomial constants */
#define        S2     7.452499648e+8
#define        S3     7.189466438e+6
#define        S4     4.744726470e+4
#define        S5     2.261030244e+2
#define        S6     1.0

#define        CONST1 0.785398164
#define        CONST2 0.636619772

double _y0(x)
double x;
{
    double xx, y, z, bessy0;

    if (x < 8) {
        y = x * x;
        z = _j0(x);
        z *= log(x);
        bessy0 = (R1+y*(R2+y*(R3+y*(R4+y*(R5+y*R6))))) /
                 (S1+y*(S2+y*(S3+y*(S4+y*(S5+y*S6))))) + CONST2*z;

    } else {
        z = 8.0 / x;
#ifndef __i960
        y = square(z);
#else
        y = z * z;
#endif
        xx = x - CONST1;
        bessy0 = sqrt(CONST2/x)*(sin(xx)*(P1+y*(P2+y*(P3+y*(P4+y*P5)))) +
                               z*cos(xx)*(Q1+y*(Q2+y*(Q3+y*(Q4+y*Q5)))));

    }
    return bessy0;
}
