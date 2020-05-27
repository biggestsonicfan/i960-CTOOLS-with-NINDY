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

/*** qtc_fsqrt.c ***/

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
FUNCTION float qtc_fsqrt(float param)
/*
**  Purpose: This is a table-based square root routine.  Tuckerman
**           rounding is used to ensure that the result is correctly rounded.
**
**        Accuracy - correctly-rounded value always returned
**        Valid input range - all positive normalized floating-point numbers
**
**        Special case handling - non-IEEE:
**
**          qtc_fsqrt(-x)   - calls qtc_ferr() with FSQRT_OUT_OF_RANGE,
**                            input value
**          qtc_fsqrt(0)    - returns 0.0
**
**        Special case handling - IEEE:
**
**          qtc_fsqrt(+INF) - calls qtc_ferror() with FSQRT_INFINITY,
**                            input value
**          qtc_fsqrt(-INF) - calls qtc_ferror() with FSQRT_OUT_OF_RANGE,
**                            input value
**          qtc_fsqrt(QNAN) - calls qtc_ferror() with FSQRT_QNAN, input value
**          qtc_fsqrt(SNAN) - calls qtc_ferror() with FSQRT_INVALID_OPERATION,
**                            input value
**          qtc_fsqrt(DEN)  - returns correct result
**          qtc_fsqrt(+-0)  - returns +/-0.0
**          qtc_fsqrt(-x)   - calls qtc_ferror() with FSQRT_OUT_OF_RANGE,
**                            input value
**
**
**  Method:
**
**        This function is based on the formula
**
**         x = 2**e * (x0 + d), where 1.0 <= x0+d < 4.0, e is even, and
**                                    x0 is a point in the table
**         Then,
**             sqrt(x) = 2**(e/2) * sqrt(x0+d)
**                     = 2**(e/2) * sqrt(x0)*sqrt(1+d/x0)
**
**         sqrt(1+d/x0) is approximated by:
**
**             sqrt(1+t) = 1 + t/2 - t**2/2**3 + t**3/2**4 - t**4*5/2**7
**                         t**5*7/2**8 - t**6*21/2**10 + ...
**
**         The index into the table is derived from the
**         first bits of the fractional part of x.  The IEEE single-
**         precision representation of x is
**
**           (s) (exp) (fraction)   where s is 1 bit, exp is 8 bits,
**                                  and fraction is 23 bits, with an
**                                  implied leading 1 if exp != 0.
**
**         When compiled with the -DIEEE switch, this routine properly
**         handles infinities, NaNs, signed zeros, and denormalized
**         numbers.
**
**
**  Input Parameters:
*/
/*    float x;  the value for which the square root */
                  /* is desired                                     */
/*
**  Output Parameters: none
**
**  Return value: returns a pointer to the single-precision square root of the
**                input argument
**
**  Called by:  any
** 
**  History:
**
**        Version 1.0 - 8 July 1988, J F Kauth and L A Westerman
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

    long_float x;
    long_float temp_lf;
    long_float a2;
    long_float oneminushulp;
    long_float sqrtx0_high;
    long_float sqrtx0_low;
#ifdef IEEE
    long_float two_exp_m24;
    long_float two_exp_48;
    long int exp;
    float dummy;
#else
    long_float zero;
#endif
    static float temp_return;

    long int ee;
    int   i, j, k, rm;
    float invx0, mu, pp, t, temp, x0, yp;

/*******************************
**  Global data definitions   **
*******************************/
    extern const unsigned long int flogix[];
    extern const unsigned long int fsqtfh[];
    extern const unsigned long int fsqtflb[];

/*******************************
**  External functions        **
*******************************/
#ifdef IEEE
    /* extern float qtc_ferror(); */
#else
    extern float *qtc_ferr();
#endif
    extern short fp_setround(short); /* Sets the rounding mode */
    extern short  fp_getround(void); /* Returns the current rounding mode */

/*******************************
** Function body              **
*******************************/
    x.f = param;
    a2.i = 0xBF000000L;  /* -0.5    = -1/2  */
    oneminushulp.i = 0x3F7FFFFFL;
#ifdef IEEE
    two_exp_m24.i = 0x33800000L; /* 2**-24 */
    two_exp_48.i  = 0x57800000L; /* 2**48  */
#else
    zero.i = 0x00000000L;
#endif
#ifdef IEEE
/*****
** Extract the exponent
*****/
   exp = x.i & 0x7F800000L;
/*****
** If the exponent is 0x7F8, the value is either an infinity or NaN
*****/
   if ( exp == 0x7F800000L ) {
   /*
   ** If the fraction bits are all zero, this is an infinity
   */
      if ( (x.i & 0x007FFFFFL ) == 0L ) {
         if ( (x.i & 0x80000000L ) != 0L )
         /*
         ** For negative infinity, signal out of range
         */
            return (qtc_ferror(FSQRT_OUT_OF_RANGE,x.f,dummy));
         else
         /*
         ** For positive infinity, signal infinity
         */
            return (qtc_ferror(FSQRT_INFINITY,x.f,dummy));
      }
   /*
   ** If some fraction bits are non-zero, this is a NaN
   */
      else {
      /*
      ** For signaling NaNs, call the error code with the invalid
      ** operation signal
      */
         if ( is_fsNaN(x) )
            return (qtc_ferror(FSQRT_INVALID_OPERATION,x.f,dummy));
         /*
         ** For quiet NaNs, call the error code with the qNaN signal
         */
         else
            return (qtc_ferror(FSQRT_QNAN,x.f,dummy));
      }
   }
/*****
** For +/- 0.0, return x
*****/
   if ( (x.i & 0x7FFFFFFFL) == 0 )
      return (x.f);
/*****
** Check the sign of the value
*****/
   if ( (x.i & 0x80000000L ) != 0L ) {
   /*
   ** For non-zero negative values, signal out of range
   */
      return (qtc_ferror(FSQRT_OUT_OF_RANGE,x.f,dummy));
   }
/*****
** Check for a denormalized value.
** For denormalized values, return the correct value by biasing x,
** calculating the square root, then biasing the result for return
*****/
   if ( exp == 0L ) {
      temp = x.f*two_exp_48.f;
      temp = qtc_fsqrt(temp);
      temp_return = temp * two_exp_m24.f;
      return (temp_return);
   }
#else /* end of IEEE special case handling, now for non-IEEE case */

/*****
** Exceptional cases 
*****/
   if ( (x.i & 0x80000000L ) != 0L )            /* If x < 0.0 */
      return(qtc_ferr(FSQRT_OUT_OF_RANGE,x));
   if ( x.i == 0L )
      return(zero.f);
#endif /* end of non-IEEE special case handling */
/*****
** Reduce x to: x = 2**ee * rr,
**       where ee is even and rr is in [1.0,4.0)
** also find j, the index into the sqrt(x0) table,
**           i, the index into the 1/x0 table,
**           x0, the table point nearest to rr
**           1/x0 (for x0 in [2,4), the table value 1/(x0/2)
**                 is used and divided by 2)
**
** If ee is even, rr is in [1,2) and intervals 0-63 which lie in
** this range look like this (all intervals are shifted to the
** right by 1/512 to allow sharing of tables: interval -1 is
** handled as a special case with x0 = 1.0 ):
**
**  |_-1__|___________________0___________________|___ ... :j
**  1   1+1/512                                1+9/512
**      0    1    2    3    4    5    6    7    8          :k
**        0         1         2         3         4        :i
**
** If ee is odd, R is in [2,4) and intervals 64,127 for this
** range look like this:
**
**  |_-1__|___________________64__________________|___ ... :j
**  2   2+2/512                                 2+18/512
**      0    1    2    3    4    5    6    7    8          :k
**        0         1         2         3         4        :i
**
** Note that all intervals in the range [1,2) are of length 8/512
** except interval -1 (length 1/512) and interval 63 (length 7/512)
**
** All intervals in the range [2,4) are of length 16/512
** except interval -1 (length 2/512) and interval 127 (length 14/512)
** (interval -1 in the range [2,4) is appended to interval 63.)
*****/
   ee = (0x7F800000L & x.i) - 0x3F800000L; /* ee = shifted exponent of x */
   k = (0x007FC000L & x.i) >> 14;          /* 1st 9 fraction bits */
   if ( ( ee & 0x00800000L ) == 0) {    /* if exponent even, 1.0 <= rr < 2.0 */
   /*
   ** k, the first 9 fraction bits of x, divides [1,2) into
   ** 512 intervals: 8 of these intervals are in each j interval
   */
      if ( k != 0 ) {
         j = (k - 1) >> 3;   /* j = (k-1)/8 */
         i = (j << 2) + 2;   /* i = 4*j + 2 */
         temp_lf.i = 0x3F804000L + ( i << 15 );   /* x0 = (513+2*i) / 512 */
         x0 = temp_lf.f;

         temp_lf.i = flogix[i] - 0x00800000L;  /* 1/(2*x0) */
         invx0 = temp_lf.f;

      }
      else {
      /*
      ** Handle the j = -1 interval separately, with an
      ** expansion about 1.0
      */
         x.i -= ee;                    /* x*2**(-ee) */
         temp_lf.i = x.i - 0x00800000L;  /* t = (x*2**(-ee))/2 */
         t = temp_lf.f;
         mu = t - 0.5;                     /* delta/2 */
         pp = a2.f*mu;
         pp *= mu;
         pp += mu;
         pp += oneminushulp.f;        /* Bias downward by 1/2 ulp */
                                           /* for Tuckerman rounding   */
         goto tuckerman_rounding;
      }
   }
   else {               /* if ee odd, 2.0 <= rr < 4.0 */
   /*
   ** k, the first 9 fraction bits of x, divides [2,4) into
   ** 512 intervals: 8 of these intervals are in each j interval
   */
      if ( k != 0 ) {
         j = (k - 1) >> 3;   /* j = (k-1)/8 */
         i = (j << 2) + 2;   /* i = 4*j + 2 */
         j += 64;
         temp_lf.i = 0x40004000L + ( i << 15 );   /* x0 = (513+2*i) / 256 */
         x0 = temp_lf.f;
          
         temp_lf.i = flogix[i] - 0x01000000L;  /* 1/4 of table value */
         invx0 = temp_lf.f;
      }
      else {
      /*
      ** Handle the k = 0 interval separately, as part of the
      ** j = 63 interval
      */
         j = 63;
         i = 254;                  /* i = 4*j + 2 */
         temp_lf.i = 0x3FFF4000L;   /* x0 = (513+2*i) / 512 */
         x0 = temp_lf.f;
         temp_lf.i = flogix[i] - 0x00800000L;  /* 1/(2*x0) */
         invx0 = temp_lf.f;
      }
      ee -= 0x00800000L;
   }
/*****
** Get sqrt(x0) = sqrtx0_high + sqrtx0_low from table
** (Sqrtx0_low is biased by -1/2 ulp of sqrtx0_high to ensure that
** the result is low, as required for Tuckerman rounding.)
*****/
   sqrtx0_high.i = fsqtfh[j];
   sqrtx0_low.i = fsqtflb[j];
/*****
** Determine mu = (rr-x0)/(2*x0)
*****/
   x.i -= ee;   /* rr = x * 2**(-ee) */
   mu = x.f - x0;
   mu *= invx0;
/*****
** Polynomial approximation of sqrt(1 + mu) - 1:
**  pp =  mu + mu*(mu*(a2 + mu*a3))
** (where mu = delta/(2*x0))
** Reconstruction of sqrt(x):
**           sqrt(x) = 2**(ee/2) * (sqrt(x0) + sqrt(x0)*pp)
** (sqrt(x0) = sqrtx0_high + sqrtx0_low)
*****/
   pp = a2.f*mu;
   pp *= mu;
   pp += mu;
   pp *= sqrtx0_high.f;
   pp += sqrtx0_low.f;
   pp += sqrtx0_high.f;
/*****
** Tuckerman rounding
**     pp is either the correct sqrt(x) or too small,
**     since it has been biased by -1/2 ulp.  Therefore
**     the correct result is either pp or pp + 1 ulp.
**   yp = pp + 1 ulp
*****/
tuckerman_rounding:
   rm = fp_getround();
   fp_setround(RZ);

   temp_lf.f = pp;
   temp_lf.i += 1;
   yp  = temp_lf.f;

   temp = pp*yp;
   fp_setround(rm);
   if (x.f <= temp) {    /* Compare rr to the truncated product */
      temp_lf.f = pp;
      temp_lf.i += (ee >> 1);
      temp_return = temp_lf.f;
   }
   else {
      temp_lf.f = yp;
      temp_lf.i += (ee >> 1);
      temp_return = temp_lf.f;
   }
   return (temp_return);
}
/*** end of qtc_fsqrt.c ***/
