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

/*** qtc_dlog.c ***/

#include "qtc_intr.h"
#include <qtc.h>

/*
******************************************************************************
**
*/
FUNCTION double qtc_dlog(param)
/*
**  Purpose: This is a table-based natural logarithm routine
**
**           Accuracy - less than 1 ULP error
**           Valid input range - all positive non-zero floating-point numbers
**
**           Special case handling - non-IEEE:
**       
**             qtc_dlog(0)    - calls qtc_error() with DLOG_OUT_OF_RANGE,
**                              input value
**             qtc_dlog(-x)   - calls qtc_error() with DLOG_OUT_OF_RANGE,
**                              input value
**       
**           Special case handling - IEEE:
**       
**             qtc_dlog(+INF) - calls qtc_error() with DLOG_INFINITY,
**                              input value
**             qtc_dlog(-INF) - calls qtc_error() with DLOG_OUT_OF_RANGE,
**                              input value
**             qtc_dlog(QNAN) - calls qtc_error() with DLOG_QNAN, input value
**             qtc_dlog(SNAN) - calls qtc_error() with DLOG_INVALID_OPERATION,
**                              input value
**             qtc_dlog(DEN)  - returns correct result
**             qtc_dlog(+-0)  - calls qtc_error() with DLOG_OUT_OF_RANGE,
**                              input value
**             qtc_dlog(-x)   - calls qtc_error() with DLOG_OUT_OF_RANGE,
**                              input value
**       
**  Method:
**
**        This function is based on the formula
**
**         if x = 2**n * (x0 + rr), where 1.0 <= x0+rr < 2.0, and
**                                    x0 is a point in the table
**         then,
**             log(x) = n*log(2) + log(x0+rr)
**                    = n*log(2) + log(x0) + log(1+rr/x0)
**
**         log(1+rr/x0) is approximated by:
**
**             log(1+t) = 1 + t - t**2/2 + t**3/3 - t**4/4 + ...
**
**         The index into the table is derived from the
**         first bits of the fractional part of x.  The IEEE double-
**         precision representation of x is
**
**           (s) (exp) (fraction)   where s is 1 bit, exp is 11 bits,
**                                  and fraction is 52 bits, with an
**                                  implied leading 1 if exp != 0.
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
    double param; /* input value for which the logarithm is desired */
/*
**  Output Parameters: none
**
**  Return value: returns the double-precision natural logarithm of the
**                input argument
**
**  Called by:  any
** 
**  History:
**
**        Version 1.0 - 8 July 1988, J F Kauth and L A Westerman
**                1.1 - 15 July 1988, L A Westerman
**                      corrected code for n=0,j=0 case with 7 terms,
**                      cleaned up variable definitions
**                1.2 - 26 October 1988, L A Westerman
**                1.3 - 29 November 1988, J F Kauth
**                      added variable "temp" to ensure proper operation order
**                1.4 - 20 February 1989, L A Westerman, added extra static
**                      double to force double-word alignment
**                1.5 - 1 May 1989, L A Westerman, corrected the code for
**                      dealing with denormalized input arguments
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

    /*
    ** log(2) = ln2hi + ln2lo
    ** LN2HI has at least 10 trailing zeros, so N*LN2HI is exact for n < 1024
    ** (The maximum n in the algorithm is 1023.)
    */
#ifdef MSH_FIRST
    static const unsigned long int a2[2] = {0xBFE00000L, 0x00000000L}; /* -1/2 */
    static const unsigned long int a3[2] = {0x3FD55555L, 0x55555555L}; /* +1/3 */
    static const unsigned long int a4[2] = {0xBFD00000L, 0x00000000L}; /* -1/4 */
    static const unsigned long int a5[2] = {0x3FC99999L, 0x9999999AL}; /* +1/5 */
    static const unsigned long int a6[2] = {0xBFC55555L, 0x55555555L}; /* -1/6 */
    static const unsigned long int a7[2] = {0x3FC24924L, 0x92492492L}; /* +1/7 */
    static const unsigned long int a8[2] = {0xBFC00000L, 0x00000000L}; /* -1/8 */
    static const unsigned long int ln2hi[2] = {0x3FE62E42L, 0xFEFA3800L};
        /* 6.931471805599e-01 */
    static const unsigned long int ln2lo[2] = {0x3D2EF357L, 0x93C76730L};
        /* 5.497923018708e-14 */
#else
    static const unsigned long int a2[2] = {0x00000000L, 0xBFE00000L}; /* -1/2 */
    static const unsigned long int a3[2] = {0x55555555L, 0x3FD55555L}; /* +1/3 */
    static const unsigned long int a4[2] = {0x00000000L, 0xBFD00000L}; /* -1/4 */
    static const unsigned long int a5[2] = {0x9999999AL, 0x3FC99999L}; /* +1/5 */
    static const unsigned long int a6[2] = {0x55555555L, 0xBFC55555L}; /* -1/6 */
    static const unsigned long int a7[2] = {0x92492492L, 0x3FC24924L}; /* +1/7 */
    static const unsigned long int a8[2] = {0x00000000L, 0xBFC00000L}; /* -1/8 */
    static const unsigned long int ln2hi[2] = {0xFEFA3800L, 0x3FE62E42L};
        /* 6.931471805599e-01 */
    static const unsigned long int ln2lo[2] = {0x93C76730L, 0x3D2EF357L};
        /* 5.497923018708e-14 */
#endif
#ifdef IEEE
    long int exp;
#endif
    int j, n;
    long int e;
    unsigned long int frac_x;
    double dn, pp, t, temp;
    volatile double invx0, logx0_high, logx0_low, x0, xn;

/*******************************
** Global data definitions    **
*******************************/
    extern unsigned long int dlogix[][2];
    extern unsigned long int dlogfh[][2];
    extern unsigned long int dlogfl[][2];

/*******************************
** External functions         **
*******************************/
#ifdef IEEE
    extern double qtc_error();
#else
    extern double qtc_err();
#endif 

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
            return (qtc_error(DLOG_OUT_OF_RANGE,x));
        }
        /*
        ** For positive infinity, signal infinity
        */
        return (qtc_error(DLOG_INFINITY,x));
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
          return (qtc_error(DLOG_INVALID_OPERATION,x));
        /*
        ** For quiet NaNs, call the error code with the qNaN signal
        */
        else
          return (qtc_error(DLOG_QNAN,x));
      }
    }
    /*
    ** Check the sign of the value
    */
    if ( MSH(x) >= (unsigned long int) 0x80000000L ) {
      /*
      ** For negative values, signal out of range
      */
        return (qtc_error(DLOG_OUT_OF_RANGE,x));
    }
    /*
    ** For zero argument, signal out of range
    */
    if ( ( MSH(x) | LSH(x) ) == 0x0L ) 
      return (qtc_error(DLOG_OUT_OF_RANGE,x));
  
#else /* end of IEEE special case handling, now for non-IEEE case */
    /*
    ** Exceptional cases 
    */
    if ( ( MSH(x) >= (unsigned long int) 0x80000000L ) || /* If x < 0.0  */
         ( ( MSH(x) | LSH(x) ) == 0x0 ) )                 /* or x == 0.0 */
        return (qtc_err(DLOG_OUT_OF_RANGE,x));

#endif /* end of non-IEEE special case handling */

    /*
    ** Calculate the table index, j = first 8 bits of the fraction of x
    **
    ** Determine n such that x = rr * 2**n and 1.0 <= rr < 2.
    */
    e = (0x7FF00000L & MSH(x)) - 0x3FF00000L;
    n = e >> 20;

#ifdef IEEE
    /*
    ** Check for a denormalized value, and handle separately
    */
    if ( n == -0x3FF ) {
        n++;
        /*
        ** Shift the fraction to the left until the explicit MSB is set
        ** Count the number of shifts and correct the exponent as necessary
        */
        frac_x = MSH(x) & 0x000FFFFFL;
        if ( frac_x != 0x0L ) {
            do {
                frac_x = ( frac_x << 1 ) |
                          ( ( LSH(x) & 0x80000000L ) ? 0x1L : 0x0L );
                LSH(x) <<= 1;
                n--;
            } while ( frac_x < (unsigned long int) 0x00100000L );
        }
        else {
            frac_x = LSH(x);
            LSH(x) = 0x0L;
            n -= 32;
            while ( frac_x >= 0x00200000L ) {
                LSH(x) = ( LSH(x) >> 1 ) |
                          ( ( frac_x & 0x1L ) ? 0x80000000L : 0x0L );
                frac_x >>= 1;
                n++;
            }
            while ( frac_x < 0x00100000L ) {
                frac_x <<= 1;
                n--;
            }
        }
    }
    else
#endif
        frac_x = MSH(x); 

    j = ( frac_x & 0x000FF000L ) >> 12;
    /*
    ** Get table values:
    **     invx0
    **     logx0_high
    **     logx0_low
    ** (NOTE: These statements are not needed for n=0,j=0-1 and n=-1,j=252-255)
    */
    LSH(invx0) = dlogix[j][0];
    MSH(invx0) = dlogix[j][1];
    LSH(logx0_high) = dlogfh[j][0];
    MSH(logx0_high) = dlogfh[j][1];
    LSH(logx0_low) = dlogfl[j][0];
    MSH(logx0_low) = dlogfl[j][1];
    /*
    ** Determine x0, rr
    ** (NOTE: These statements are not needed for n=0,j=0-1 and n=-1,j=252-255)
    */
    LSH(x0) = 0;                /* x0 = (513+2*j) * 1/512 */
    MSH(x0) = 0x3FF00800L + ( j << 12 );
    LSH(xn) = LSH(x);
    MSH(xn) = ( frac_x & 0x000FFFFFL ) | 0x3FF00000L; /* xn = x * 2**(-n) */
    /*
    ** Determine t = rr/x0 = (xn-x0)/x0,
    ** Approximate log(1+t) - 1 by
    **         t - t**2/2 + t**3/3 - t**4/4 + ...
    ** Reconstruct log(x) = n*log(2) + log(x0) + log(1+t)
    ** Handle arguments close to 1.0 by an expansion about 1.0
    ** (Note that some of the following branching can be avoided by
    ** using a larger number of terms than needed.)
    */
    if (ABS(n) >= 2) {       /* 5 terms */
        t = (xn - x0)*invx0;
        pp = logx0_low + (dn=(double)n)*DOUBLE(ln2lo);
        pp += (((DOUBLE(a5)*t + DOUBLE(a4))*t + DOUBLE(a3))*t +
               DOUBLE(a2))*t*t + t;
        temp = logx0_high + dn*DOUBLE(ln2hi);
        pp += temp;
    }
    else if (n == 1) {       /* 5 terms */
        t = (xn - x0)*invx0;
        pp = logx0_low + DOUBLE(ln2lo);
        pp += (((DOUBLE(a5)*t + DOUBLE(a4))*t + DOUBLE(a3))*t +
               DOUBLE(a2))*t*t + t;
        temp = logx0_high + DOUBLE(ln2hi);
        pp += temp;
    }
    else if (n == 0) {
        if (j >= 122) {       /* 5 terms */
            t = (xn - x0)*invx0;
            pp = (((DOUBLE(a5)*t + DOUBLE(a4))*t + DOUBLE(a3))*t +
               DOUBLE(a2))*t*t + logx0_low + t;
            pp += logx0_high;
        }
        else if (j >= 2) {   /* 6 terms */
            t = (xn - x0)*invx0;
            pp = ((((DOUBLE(a6)*t + DOUBLE(a5))*t + DOUBLE(a4))*t +
               DOUBLE(a3))*t + DOUBLE(a2))*t*t + logx0_low;
            pp += t;
            pp += logx0_high;
        }
        else if (j == 1) {    /* n == 0; j == 1:  8 terms about 1.0 */
                              /* t >= 2**(-7)                       */
            t = x - 1.0;
            pp = ((((((DOUBLE(a8)*t+DOUBLE(a7))*t+DOUBLE(a6))*t+
                  DOUBLE(a5))*t+DOUBLE(a4))*t+DOUBLE(a3))*t+
                  DOUBLE(a2))*t*t + t;
        }
        else {                /* n == 0; j == 0:  7 terms about 1.0 */
                              /* t >= 2**(-8)                       */
            t = x - 1.0;
            pp = ((((((DOUBLE(a7))*t+DOUBLE(a6))*t+
                  DOUBLE(a5))*t+DOUBLE(a4))*t+DOUBLE(a3))*t+
                  DOUBLE(a2))*t*t + t;
        }
    }
    else {   /* n = -1 */
        if (j <= 194) {       /* 5 terms */
            t = (xn - x0)*invx0;
            pp = logx0_low - DOUBLE(ln2lo);
            pp += (((DOUBLE(a5)*t + DOUBLE(a4))*t + DOUBLE(a3))*t +
                   DOUBLE(a2))*t*t;
            pp += t;
            temp = logx0_high - DOUBLE(ln2hi);
            pp += temp;
        }
        else if (j <= 251) {  /* 6 terms */
            t = (xn - x0)*invx0;
            pp = logx0_low - DOUBLE(ln2lo);
            pp += ((((DOUBLE(a6)*t + DOUBLE(a5))*t + DOUBLE(a4))*t +
                   DOUBLE(a3))*t + DOUBLE(a2))*t*t;
            pp += t;
            temp = logx0_high - DOUBLE(ln2hi);
            pp += temp;
        }
        else if (j <= 253) {  /* n == -1; j == 252,253:  8 terms about 1.0 */
                              /* x in [1-2/256,1), 0 < t < 2**(-7) */
            t = x - 1.0;
            pp = ((((((DOUBLE(a8)*t+DOUBLE(a7))*t+DOUBLE(a6))*t+
                  DOUBLE(a5))*t+DOUBLE(a4))*t+DOUBLE(a3))*t+
                  DOUBLE(a2))*t*t + t;
        }
        else {                /* n == -1; j == 254,255:  7 terms about 1.0 */
                              /* x in [1-1/256,1), 0 < t < 2**(-8) */
            t = x - 1.0;
            pp = (((((DOUBLE(a7)*t+DOUBLE(a6))*t+DOUBLE(a5))*t+
                  DOUBLE(a4))*t+DOUBLE(a3))*t+DOUBLE(a2))*t*t + t;
        }
    }

    return (pp);

}   /* end of qtc_dlog() routine */

/*** end of qtc_dlog.c ***/
