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

/*** qtc_dsqrt.c ***/

#include "qtc_intr.h"
#include "qtc.h"

/*
******************************************************************************
**
*/
FUNCTION double qtc_dsqrt(param)
/*
**  Purpose: This is a table-based square root routine.  Tuckerman
**           rounding is used to ensure that the result is correctly rounded.
**
**        Accuracy - correctly-rounded value always returned
**        Valid input range - all positive normalized floating-point numbers
**
**        Special case handling - non-IEEE:
**
**          qtc_dsqrt(-x)   - calls qtc_err() with DSQRT_OUT_OF_RANGE,
**                            input value
**          qtc_dsqrt(0)    - returns 0.0
**
**        Special case handling - IEEE:
**
**          qtc_dsqrt(+INF) - calls qtc_error() with DSQRT_INFINITY,
**                            input value
**          qtc_dsqrt(-INF) - calls qtc_error() with DSQRT_OUT_OF_RANGE,
**                            input value
**          qtc_dsqrt(QNAN) - calls qtc_error() with DSQRT_QNAN, input value
**          qtc_dsqrt(SNAN) - calls qtc_error() with DSQRT_INVALID_OPERATION,
**                            input value
**          qtc_dsqrt(DEN)  - returns correct result
**          qtc_dsqrt(+-0)  - returns +/-0.0
**          qtc_dsqrt(-x)   - calls qtc_error() with DSQRT_OUT_OF_RANGE,
**                            input value
**
**
**  Method:
**
**        This function is based on the formula
**
**          if x = 2**e * (x0 + rr), where 1.0 <= x0+rr < 4.0, e is even, and
**                                    x0 is a point in the table
**          then,
**             sqrt(x) = 2**(e/2) * sqrt(x0+rr)
**                     = 2**(e/2) * sqrt(x0)*sqrt(1+rr/x0)
**
**         sqrt(1+rr/x0) is approximated by:
**
**             sqrt(1+t) = 1 + t/2 - t**2/2**3 + t**3/2**4 - t**4*5/2**7
**                         t**5*7/2**8 - t**6*21/2**10 + ...
**
**         The index into the table is derived from the
**         first 8 or 9 bits of the fractional part of x.
**
**  Notes:
**
**         When compiled with the -DIEEE switch, this routine properly
**         handles infinities, NaNs, signed zeros, and denormalized
**         numbers.
**
**         One of the compile line switches -DMSH_FIRST or -DLSH_FIRST should
**         be specified when compiling this source.
**
**  Input Parameters:
*/
    double param; /* input value for which the square root is desired */
/*
**  Output Parameters: none
**
**  Return value: returns the double-precision square root of the
**                input argument
**
**  Called by:  any
** 
**  History:
**
**        Version 1.0 -  8 July 1988, J F Kauth and L A Westerman
**                1.1 - 15 July 1988, L A Westerman
**                      corrected error in qtc_err() call and cleaned up
**                      variable definitions
**                1.2 - 13 Oct 1988,  J F Kauth
**                      biased result by -1/2 ulp to ensure that it is low
**                      for Tuckerman rounding; modified Tuckerman rounding
**                      to check result and result + 1 ulp for correctness.
**                1.3 - 31 October 1988, L A Westerman, cleaned up documentation
**                      for release version
**                1.4 - 20 February 1989, L A Westerman, added extra static
**                      double to force double-word alignment
** 
**  Copyright: 
**      (c) 1988
**      (c) 1989 Quantitative Technology Corporation.
**               8700 SW Creekside Place Suite D
**               Beaverton OR 97005 USA
**               503-626-3081
**      Intel usage with permission under contract #89B0090
** 
******************************************************************************
*/
{
/*******************************
** Internal Data Requirements **
*******************************/
/*
** Force double word alignment
*/
    static const double dummy = 0.0;
    volatile double x = param;

#ifdef MSH_FIRST
    static const unsigned long int a2[2] = {0xBFE00000L, 0x00000000L};
        /* -0.5    = -1/2  */
    static const unsigned long int a3[2] = {0x3FE00000L, 0x00000000L};
        /*  0.5    =  1/2  */
    static const unsigned long int a4[2] = {0xBFE40000L, 0x00000000L};
        /* -0.625  = -5/8  */
    static const unsigned long int a5[2] = {0x3FEC0000L, 0x00000000L};
        /*  0.875  =  7/8  */
    static const unsigned long int oneminushulp[2] = {0x3FEFFFFFL, 0xFFFFFFFFL};
        /*  1.0 - 1/2 ulp  */
#ifdef IEEE
    static const unsigned long int two_exp_128[2] = {0x47F00000L, 0x00000000L};
        /* 2**128 */
    static const unsigned long int two_exp_m64[2] = {0x3BF00000L, 0x00000000L};
        /* 2**-64 */
#endif
#else
    static const unsigned long int a2[2] = {0x00000000L, 0xBFE00000L};
        /* -0.5    = -1/2  */
    static const unsigned long int a3[2] = {0x00000000L, 0x3FE00000L};
        /*  0.5    =  1/2  */
    static const unsigned long int a4[2] = {0x00000000L, 0xBFE40000L};
        /* -0.625  = -5/8  */
    static const unsigned long int a5[2] = {0x00000000L, 0x3FEC0000L};
        /*  0.875  =  7/8  */
    static const unsigned long int oneminushulp[2] = {0xFFFFFFFFL, 0x3FEFFFFFL};
        /*  1.0 - 1/2 ulp  */
#ifdef IEEE
    static const unsigned long int two_exp_128[2] = {0x00000000L, 0x47F00000L};
        /* 2**128 */
    static const unsigned long int two_exp_m64[2] = {0x00000000L, 0x3BF00000L};
        /* 2**-64 */
#endif
#endif
#ifdef IEEE
    long int exp;
#endif
    int   ee, i, j, k, rm;
    volatile double invx0, pp, sqrtx0_high, sqrtx0_low, t, x0, yp;

/*******************************
**  Global data definitions   **
*******************************/
    extern const unsigned long int dlogix[][2];
    extern const unsigned long int dsqtfh[][2];
    extern const unsigned long int dsqtflb[][2];

/*******************************
**  External functions        **
*******************************/
#ifdef IEEE
    extern double qtc_error();
#else
    extern double qtc_err();
#endif
    extern short fp_setround(short); /* Sets the rounding mode */
    extern short  fp_getround(void); /* Returns the current rounding mode */

/*******************************
** Function body              **
*******************************/
#ifdef IEEE
    /*
    ** Extract the exponent
    */
    exp = MSH(x) & 0x7FF00000L;
    /*
    ** If the exponent is 0x7FF, the value is either an infinity or NaN
    */
    if ( exp == 0x7FF00000L ) {
      /*
      ** If the fraction bits are all zero, this is an infinity
      */
      if ( ( ( MSH(x) & 0x000FFFFFL ) | LSH(x) ) == 0L ) {
        if ( MSH(x) >= (unsigned long int) 0x80000000L ) {
          /*
          ** For negative infinity, signal out of range
          */
            return (qtc_error(DSQRT_OUT_OF_RANGE,x));
        }
        /*
        ** For positive infinity, signal infinity
        */
        return (qtc_error(DSQRT_INFINITY,x));
      }
      /*
      ** If some fraction bits are non-zero, this is a NaN
      */
      else {
        /*
        ** For signaling NaNs, call the error code with the invalid
        ** operation signal
        */
        if ( is_sNaN(x) )
          return (qtc_error(DSQRT_INVALID_OPERATION,x));
        /*
        ** For quiet NaNs, call the error code with the qNaN signal
        */
        else
          return (qtc_error(DSQRT_QNAN,x));
      }
    }
    /*
    ** For +/- 0.0, return x
    */
    if ( ( (MSH(x) & 0x7FFFFFFFL) | LSH(x) ) == 0 )
      return (x);
    /*
    ** Check the sign of the value
    */
    if ( MSH(x) >= (unsigned long int) 0x80000000L ) {
      /*
      ** For non-zero negative values, signal out of range
      */
        return (qtc_error(DSQRT_OUT_OF_RANGE,x));
    }
    /*
    ** Check for a denormalized value.
    ** For denormalized values, return the correct value by biasing x,
    ** calculating the square root, then biasing the result for return
    */
    if ( exp == 0L ) {
        return (DOUBLE(two_exp_m64)*qtc_dsqrt(x*DOUBLE(two_exp_128)));
    }
#else /* end of IEEE special case handling, now for non-IEEE case */

    /*
    ** Exceptional cases 
    */
    if ( MSH(x) >= (unsigned long int) 0x80000000L )  /* If x < 0.0 */
        return (qtc_err(DSQRT_OUT_OF_RANGE,x));
    if ( ( MSH(x) | LSH(x) ) == 0x00000000L )
        return (0.0);
#endif /* end of non-IEEE special case handling */

    /*
    ** Reduce x to: x = 2**ee * xn,
    **       where ee is even and xn is in [1.0,4.0)
    ** also find j, the index into the sqrt(x0) table,
    **           i, the index into the 1/x0 table,
    **           x0, the table point nearest to xn
    **           1/x0 (for x0 in [2,4), the table value 1/(x0/2)
    **                 is used and divided by 2)
    **
    ** If ee is even, xn is in [1,2) and intervals 0-127 which lie in
    ** this range look like this (all intervals are shifted to the
    ** right by 1/512 to allow sharing of tables: interval -1 is
    ** handled as a special case with x0 = 1.0 ):
    **
    **  1   1+1/512             1+5/512             1+9/512    : x
    **
    **  |_-1__|_________0_________|_________1_________|___ ... : j
    **      0    1    2    3    4    5    6    7    8          : k
    **        0         1         2         3         4        : i
    **
    ** If ee is odd, R is in [2,4) and intervals 128-255 for this
    ** range look like this:
    **
    **  2  2+2/512             2+10/512            2+18/512    : x
    **
    **  |_-1__|________128________|________129________|___ ... : j
    **      0    1    2    3    4    5    6    7    8          : k
    **        0         1         2         3         4        : i
    **
    ** Note that all intervals in the range [1,2) are of length 4/512
    ** except interval -1 (length 1/512) and interval 127 (length 3/512)
    **
    ** All intervals in the range [2,4) are of length 8/512
    ** except interval -1 (length 2/512) and interval 255 (length 6/512)
    ** (interval -1 in the range [2,4) is appended to interval 127.)
    */
    ee = ((0x7FF00000L & MSH(x)) >> 20) - 0x3FF;  /* ee = exponent of x */
    k = (0x000FF800L & MSH(x)) >> 11;             /* 1st 9 fraction bits */
    if ( ( ee & 1 ) == 0) {    /* if ee even, 1.0 <= xn < 2.0 */
        /*
        ** k, the first 9 fraction bits of x, divides [1,2) into
        ** 512 intervals: 4 of these intervals are in each j interval
        */
        if ( k != 0 ) {
            j = (k - 1) >> 2;   /* j = (k-1)/4 */
            i = (j << 1) + 1;           /* i = 2*j + 1 */
            LSH(x0) = 0;                /* x0 = (513+2*i) * 1/512 */
            MSH(x0) = 0x3FF00800L + ( i << 12 );
            LSH(invx0) = dlogix[i][0];
            MSH(invx0) = dlogix[i][1] - 0x00100000L;  /* 1/(2*x0) */
        }
        else {
            /*
            ** Handle the j = -1 interval separately, with an
            ** expansion about 1.0
            */
            MSH(x) -= (ee << 20);          /* xn = x * 2**(-ee), overwrites x */
            LSH(t) = LSH(x);
            MSH(t) = MSH(x) - 0x00100000L; /* xn/2  */
            t -= 0.5;                      /* t = (xn - 1.0)/2 */
            pp = (((t*DOUBLE(a5) + DOUBLE(a4))*t + DOUBLE(a3))*t +
                   DOUBLE(a2))*t*t + t;
            pp += DOUBLE(oneminushulp);    /* Bias downward by 1/2 ulp */
                                           /* for Tuckerman rounding   */
            goto tuckerman_rounding;
        }
    }
    else {               /* if ee odd, 2.0 <= xn < 4.0 */
        /*
        ** i, the first 8 fraction bits of x, divides [2,4) into
        ** 256 intervals
        */
        if ( k != 0 ) {
            j = (k - 1) >> 2;   /* j = (k-1)/4 */
            i = (j << 1) + 1;   /* i = 2*j + 1 */
            j += 128;
            LSH(x0) = 0;                /* x0 = (513+2*i) * 1/512 */
            MSH(x0) = 0x40000800L + ( i << 12 );
            LSH(invx0) = dlogix[i][0];
            MSH(invx0) = dlogix[i][1] - 0x00200000L; /* 1/4 of table value */
        }
        else {
            /*
            ** Handle the k = 0 interval separately, as part of the
            ** j = 127 interval
            */
            j = 127;
            i = 255;                    /* i = 4*j + 2 */
            LSH(x0) = 0;                /* x0 = (513+2*i) * 1/512 */
            MSH(x0) = 0x3FFFF800L;
            LSH(invx0) = dlogix[i][0];
            MSH(invx0) = dlogix[i][1] - 0x00100000L;  /* 1/(2*x0) */
        }
        ee--;
    }
    /*
    ** Get sqrt(x0) = sqrtx0_high + sqrtx0_low from table
    ** (Sqrtx0_low is biased by -1/2 ulp of sqrtx0_high to ensure that
    ** the result is low, as required for Tuckerman rounding.)
    */
    LSH(sqrtx0_high) = dsqtfh[j][0];
    MSH(sqrtx0_high) = dsqtfh[j][1];
    LSH(sqrtx0_low) = dsqtflb[j][0];
    MSH(sqrtx0_low) = dsqtflb[j][1];
    /*
    ** Determine t = (xn-x0)/(2*x0)
    */
    MSH(x) -= (ee << 20);          /* xn = x * 2**(-ee), overwrites x */
    t = (x - x0) * invx0;
    /*
    ** Polynomial approximation of sqrt(1 + t) - 1:
    **  pp =  t + t*(t*(DOUBLE(a2) + t*(DOUBLE(a3) +
    **             t*(DOUBLE(a4) + t*(DOUBLE(a5))))))
    ** (where t = rr/(2*x0))
    ** Reconstruction of sqrt(x):
    **           sqrt(x) = 2**(ee/2) * (sqrt(x0) + sqrt(x0)*pp)
    ** (sqrt(x0) = sqrtx0_high + sqrtx0_low)
    */
    pp = ((((t*DOUBLE(a5) + DOUBLE(a4))*t + DOUBLE(a3))*t +
                   DOUBLE(a2))*t*t + t)*sqrtx0_high + sqrtx0_low;
    pp += sqrtx0_high;
    /*
    ** Tuckerman rounding
    **     pp is either the correct sqrt(x) or too small,
    **     since it has been biased by -1/2 ulp.  Therefore
    **     the correct result is either pp or pp + 1 ulp.
    **   yp = pp + 1 ulp
    */
tuckerman_rounding:
    if ( LSH(pp) == (unsigned long int) 0xFFFFFFFFL ) {
        LSH(yp) = 0x0L;
        MSH(yp) = MSH(pp) + 1;
    }
    else {
        LSH(yp) = LSH(pp) + 1;
        MSH(yp) = MSH(pp);
    }
    rm = fp_getround();
    fp_setround(RZ);
    /*
    ** Compare xn to the truncated product and correct if necessary
    */
    if ( x > pp*yp ) {
        MSH(pp) = MSH(yp);
        LSH(pp) = LSH(yp);
    }
    fp_setround(rm);
    MSH(pp) += ee << 19;

    return (pp);

}   /* end of qtc_dsqrt() routine */

/*** end of qtc_dsqrt.c ***/
