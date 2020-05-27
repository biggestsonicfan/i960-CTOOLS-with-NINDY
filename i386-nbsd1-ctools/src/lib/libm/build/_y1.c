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

/* function:  _y1(x)
 *     Calculate Bessel function Y1(X) for any positive X.
 *
 * Note:  Algorithm taken from:
 *     Numerical Recipes, Cambridge University Press; pg. 174.
 */

#include <stdlib.h>
#include <math.h>

#define        P1     1.0        	/* Polynomial constants */
#define        P2     1.831050000e-3
#define        P3    -3.516396496e-5
#define        P4     2.457520174e-6
#define        P5    -2.403370190e-7

#define        Q1     4.687499995e-2	/* Polynomial constants */
#define        Q2    -2.002690873e-4
#define        Q3     8.449199096e-6
#define        Q4    -8.822898700e-7
#define        Q5     1.057874120e-7

#define        R1    -4.900604943e+12	/* Polynomial constants */
#define        R2     1.275274390e+12
#define        R3    -5.153438139e+10
#define        R4     7.349264551e+8
#define        R5    -4.237922726e+6
#define        R6     8.511937935e+3

#define        S1     2.499580570e+13	/* Polynomial constants */
#define        S2     4.244419664e+11
#define        S3     3.733650367e+9
#define        S4     2.245904002e+7
#define        S5     1.020426050e+5
#define        S6     3.549632885e+2
#define        S7     1.0

#define        CONST1 2.356194491
#define        CONST2 0.636619772

double _y1(x)
double x;
{
    double xx, y, z, bessy1;

    if (x < 8) {
        y = x * x;
        z = _j1(x) * log(x) - 1.0 / x;
        bessy1 = x*(R1+y*(R2+y*(R3+y*(R4+y*(R5+y*R6))))) /
                   (S1+y*(S2+y*(S3+y*(S4+y*(S5+y*(S6+y*S7)))))) +
                   CONST2*z;
        
    } else {
        z = 8.0 / x;
#ifndef __i960
	y = square(z);
#else
        y = z * z;
#endif
        xx = x - CONST1;
        bessy1 = sqrt(CONST2/x)*(sin(xx)*(P1+y*(P2+y*(P3+y*(P4+y*P5)))) +
                               z*cos(xx)*(Q1+y*(Q2+y*(Q3+y*(Q4+y*Q5)))));
        
    }
    return bessy1;
}
