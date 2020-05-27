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

/*** qtc_flog.c ***/

#include "qtc_intr.h"
#include "qtc.h"


typedef volatile union long_float_type
{
    unsigned long int i;
    float f;
} long_float;

/*
******************************************************************************
**
*/
FUNCTION float qtc_flog(float param)
/*
**  Purpose: This is a table-based single-precision natural logarithm routine
**
**           Accuracy - less than 1 ULP error
**           Valid input range - all positive non-zero floating-point numbers
**
**           Special case handling - non-IEEE:
**       
**             qtc_flog(0)    - calls qtc_ferror() with FLOG_OUT_OF_RANGE, input
**                          value
**             qtc_flog(-x)   - calls qtc_ferror() with FLOG_OUT_OF_RANGE, input
**                          value
**       
**           Special case handling - IEEE:
**       
**             qtc_flog(+INF) - calls qtc_ferror() with FLOG_INFINITY,
**                              input value
**             qtc_flog(-INF) - calls qtc_ferror() with FLOG_OUT_OF_RANGE,
**                              input value
**             qtc_flog(QNAN) - calls qtc_ferror() with FLOG_QNAN, input value
**             qtc_flog(SNAN) - calls qtc_ferror() with FLOG_INVALID_OPERATION,
**                              input value
**             qtc_flog(DEN)  - returns correct result
**             qtc_flog(+-0)  - calls qtc_ferror() with FLOG_OUT_OF_RANGE,
**                              input value
**             qtc_flog(-x)   - calls qtc_ferror() with FLOG_OUT_OF_RANGE,
**                              input value
**       
**  Method:
**
**        This function is based on the formula
**
**         if x = 2**n * (x0 + d), where 1.0 <= x0+d < 2.0, and
**                                    x0 is a point in the table
**         then,
**             log(x) = n*log(2) + log(x0+d)
**                    = n*log(2) + log(x0) + log(1+d/x0)
**
**         log(1+d/x0) is approximated by:
**
**             log(1+t) = 1 + t - t**2/2 + t**3/3 - t**4/4 + ...
**
**         The index into the table is derived from the
**         first bits of the fractional part of x.  The IEEE double-
**         precision representation of x is
**
**           (s) (exp) (fraction)   where s is 1 bit, exp is 8 bits,
**                                  and fraction is 23 bits, with an
**                                  implied leading 1 if exp != 0.
**
**  Notes:
**
**         When compiled with the -DIEEE switch, this routine properly
**         handles infinities, NaNs, signed zeros, and denormalized
**         numbers.
**
**  Input Parameters:
*/
/*    float x; input value for which the logarithm is desired */
/*
**  Output Parameters: none
**
**  Return value: returns the single-precision natural
**                logarithm of the input argument
**
**  Called by:  any
** 
**  History:
**
**        Version 1.0 - 26 July 1988, J F Kauth and L A Westerman
**                1.1 - 6 January 1989, L A Westerman, cleaned up for release
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
    ** log(2) = ln2hi + ln2lo
    ** LN2HI has at least 7 trailing zeros, so n*ln2hi is exact for n < 126
    ** (The maximum n in the algorithm is 125.)
    */
    long_float temp_lf;
    long_float invx0;
    long_float logx0_high;
    long_float logx0_low;

    long_float x;
    long_float a2;
    long_float a3;
    long_float a4;
    long_float a5;
    long_float ln2hi;
    long_float ln2lo;
#ifdef IEEE
    unsigned long int exp;
    float dummy;
#endif
    static float temp_return;
    long int e;
    int abs_n, j, n;
    float dn, pp, rr, t, temp, x0;

/*******************************
** Global data definitions    **
*******************************/
    extern unsigned long int flogfh[];
    extern unsigned long int flogfl[];
    extern unsigned long int flogix[];

/*******************************
** External functions         **
*******************************/
#ifdef IEEE
    /* extern float qtc_ferror(); */
#else
    extern float *qtc_ferr();
#endif

/*******************************
** Function body              **
*******************************/
    x = param;
    a2.i    = 0xBF000000L; /* -1/2 */
    a3.i    = 0x3EAAAAABL; /*  1/3 */
    a4.i    = 0xBE800000L; /* -1/4 */
    a5.i    = 0x3E4CCCCDL; /*  1/5 */
    ln2hi.i = 0x3F317200L; /* 6.93145752e-1 */
    ln2lo.i = 0x35BFBE8EL; /* 1.42860677e-6 */

    /*
    ** Make sure the argument is a float
    */
#ifdef IEEE
    /*
    ** Extract the exponent
    */
    exp = x.i & 0x7F800000L; 
    /*
    ** If the exponent is 0x7F8, the value is either an infinity or NaN
    */
    if ( exp == 0x7F800000L ) {
      /*
      ** If the fraction bits are all zero, this is an infinity
      */
      if ( ( x.i & 0x007FFFFFL ) == 0L ) {
        if ( x.i & 0x80000000L ) { 
          /*
          ** For negative infinity, signal out of range
          */
            return (qtc_ferror(FLOG_OUT_OF_RANGE,x.f,dummy)); 
        }
        /*
        ** For positive infinity, signal infinity
        */
        return (qtc_ferror(FLOG_INFINITY,x.f,dummy)); 
      }
      /*
      ** If some fraction bits are non-zero, this is a NaN
      */
      else {
        /*
        ** For signaling NaNs, call the error code with the invalid
        ** operation signal
        */
        if ( (x.i & 0x00400000L) == 0 )
          return (qtc_ferror(FLOG_INVALID_OPERATION,x.f,dummy));
        /*
        ** For quiet NaNs, call the error code with the qNaN signal
        */
        else
          return (qtc_ferror(FLOG_QNAN,x.f,dummy));
      }
    }
    /*
    ** Check the sign of the value
    */
    if ( x.i & 0x80000000L ) {
      /*
      ** For negative values, signal out of range
      */
        return (qtc_ferror(FLOG_OUT_OF_RANGE,x.f,dummy));
    }
    /*
    ** For zero argument, signal out of range
    */
    if ( x.i == 0x0L ) 
      return (qtc_ferror(FLOG_OUT_OF_RANGE,x.f,dummy));

#else /* end of IEEE special case handling, now for non-IEEE case */

    /*
    ** Exceptional cases 
    */
    if ( ( x.i & 0x80000000L ) ||        /* If x < 0.0 */
         ( x.i == 0x0 ) )               /* or x == 0.0 */
        return (qtc_ferr(FLOG_OUT_OF_RANGE,x));

#endif /* end of non-IEEE special case handling */

    e = ( 0x7F800000L & x.i ) - 0x3F800000L;
    n = e >> 23;
#ifdef IEEE
    /*
    ** For the IEEE case, renormalize the number and correct the
    ** exponent
    */
    if ( n == -0x7F ) {
        n++;
        while ( ( x.i & 0x00800000 ) == 0x0L ) {
            x.i <<= 1;
            n--;
        }
        e = -0x3F000000L;
    }
#endif
    /*
    ** Calculate the table index, j = first 8 bits of the fraction
    ** of x.
    ** Determine n such that x = rr * 2**n and 1.0 <= rr < 2.
    */
    j = (x.i & 0x007F8000L) >> 15;
    /*
    ** Get table values:
    **     invx0
    **     logx0_high
    **     logx0_low
    ** (NOTE: These statements are not needed for n=0,j=0-1 and n=-1,j=252-255)
    */
    invx0.i      = flogix[j];
    logx0_high.i = flogfh[j];
    logx0_low.i  = flogfl[j];
    /*
    ** Determine x0, rr
    ** (NOTE: These statements are not needed for n=0,j=0-1 and n=-1,j=252-255)
    */
    temp_lf.i = 0x3F804000L + ( j << 15 );
    x0 = temp_lf.f;

    temp_lf.i = x.i - e;       /* rr = x*2**(-n) */
    rr = temp_lf.f;

    /*
    ** Determine t = delta/x0 = (rr-x0)/x0,
    ** Approximate log(1+t) - 1 by
    **         t - t**2/2 + t**3/3 - t**4/4 + ...
    ** Reconstruct log(x) = n*log(2) + log(x0) + log(1+t)
    ** Handle arguments close to 1.0 by an expansion about 1.0
    ** (Note that some of the following branching can be avoided by
    ** using a larger number of terms than needed.)
    **
    ** (Note that the following calculations should be performed in the exact
    ** order shown below.  Other orderings may cause the loss of significant
    ** bits, resulting in less accurate results.  An incorrect ordering of 
    ** these statements may only cause loss of precision in isolated cases.)
    */
    abs_n = ABS(n);
    if ( abs_n >= 2) {       /* 2 terms */
        t = rr - x0;
        t *= invx0.f;
        dn = (float) n;
        pp = dn * ln2lo.f;
        pp += logx0_low.f;
        temp = t * a2.f;
        temp *= t;
        temp += t;
        pp += temp;
        if ( abs_n < 32 ) {
            temp = dn * ln2hi.f;
            temp+= logx0_high.f;
            pp += temp;
        }
        else {
            temp = dn * ln2hi.f;
            pp += logx0_high.f;
            pp += temp;
        }
    }
    else if (n == 1) {       /* 2 terms */
        t = rr - x0;
        t *= invx0.f;
        pp = ln2lo.f + logx0_low.f;
        temp = t * a2.f;
        temp *= t;
        temp += t;
        pp += temp;
        temp = ln2hi.f + logx0_high.f;
        pp += temp;
    }
    else if (n == 0) {
        if (j >= 3) {       /* 3 terms about x0 */
            t = rr - x0;
            t *= invx0.f;
            pp = t * a3.f;
            pp += a2.f;
            pp *= t;
            pp *= t;
            pp += t;
            pp += logx0_low.f;
            pp += logx0_high.f;
        }
        else {                 /* 4 terms about 1.0 */
            t = x.f - 1.0;
            pp = t * a4.f;
            pp += a3.f;
            pp *= t;
            pp += a2.f;
            pp *= t;
            pp *= t;
            pp += t;
        }
    }
    else {   /* n = -1 */
        if (j <= 237) {       /* 3 terms about x0 */
            t = rr - x0;
            t *= invx0.f;
            pp = t * a3.f;
            pp += a2.f;
            pp *= t;
            pp *= t;
            pp -= ln2lo.f;
            pp += t;
            pp += logx0_low.f;
            temp = logx0_high.f - ln2hi.f;
            pp += temp;
        }
        else if (j <= 247) {  /* 4 terms about x0 */
            t = rr - x0;
            t *= invx0.f;
            pp = t * a4.f;
            pp += a3.f;
            pp *= t;
            pp += a2.f;
            pp *= t;
            pp *= t;
            pp -= ln2lo.f;
            pp += t;
            pp += logx0_low.f;
            temp = logx0_high.f - ln2hi.f;
            pp += temp;
        }
        else if ( j <= 253 ) { /* 5 terms about 1.0 */
            t = x.f - 1.0;
            pp = t * a5.f;
            pp += a4.f;
            pp *= t;
            pp += a3.f;
            pp *= t;
            pp += a2.f;
            pp *= t;
            pp *= t;
            pp += t;
        }
        else {                 /* 4 terms about 1.0 */
            t = x.f - 1.0;
            pp = t * a4.f;
            pp += a3.f;
            pp *= t;
            pp += a2.f;
            pp *= t;
            pp *= t;
            pp += t;
        }
    }
    temp_return = pp;
    return (temp_return);
}
/*** end of qtc_flog.c ***/
