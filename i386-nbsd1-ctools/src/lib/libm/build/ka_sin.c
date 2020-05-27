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

/*** qtc_dsin.c ***/

#include "qtc_intr.h"
#include "qtc.h"

/*
******************************************************************************
**
*/
FUNCTION double qtc_dsin(param)
/*
**  Purpose: This is a table-based sine routine.
**
**        Accuracy - less than 1 ULP
**        Valid input range - |x| < 1.3176793e+07 ( (2**22 - 1) * pi )
**    
**        Special case handling - non-IEEE:
**    
**          qtc_dsin(+OOR) - calls qtc_err() with DSIN_OUT_OF_RANGE, input value
**          qtc_dsin(-OOR) - calls qtc_err() with DSIN_OUT_OF_RANGE, input value
**    
**        Special case handling - IEEE:
**    
**          qtc_dsin(+INF) - calls qtc_error() with DSIN_INFINITY, input value
**          qtc_dsin(-INF) - calls qtc_error() with DSIN_INFINITY, input value
**          qtc_dsin(QNAN) - calls qtc_error() with DSIN_QNAN, input value
**          qtc_dsin(SNAN) - calls qtc_error() with DSIN_INVALID_OPERATION,
**                           input value
**          qtc_dsin(DEN)  - returns correct value
**          qtc_dsin(+-0)  - returns +/-0.0
**          qtc_dsin(+OOR) - calls qtc_error() with DSIN_OUT_OF_RANGE,
**                           input value
**          qtc_dsin(-OOR) - calls qtc_error() with DSIN_OUT_OF_RANGE,
**                           input value
**    
**  Method:
**
**         This routine is based on the formula
**
**           sin(x0+rr) = sin(x0)*cos(rr) + sin(rr)*cos(x0)
**                      = sin(x0) * ( 1 - rr**2/2! + rr**4/4! - ... )
**                        + cos(x0) * ( rr - rr**3/3! + rr**5/5! - ... )
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
    double param; /* input value for which the sine is desired, in radians */
/*
**  Output Parameters: none
**
**  Return value: returns the double-precision sine of the
**                input argument
**
**  Called by:  any
** 
**  History:
**
**        Version 1.0 - 8 July 1988, J F Kauth and L A Westerman
**                1.1 - 18 July 1988, L A Westerman, cleaned up variable
**                      definitions
**                1.2 - 27 July 1988, J F Kauth, eliminated p4 term
**                      in argument reduction near multiples of pi
**                1.3 - 31 October 1988, L A Westerman, restored p4 term to
**                      take care of certain pathological points extremely
**                      close to even multiples of pi
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
    static const unsigned long int inv_120[2] = {0x3F811111L,0x11111111L};
        /* 8.3333333333333e-03 */
    static const unsigned long int inv_24[2]  = {0x3FA55555L,0x55555555L};
        /* 4.1666666666667e-02 */
    static const unsigned long int inv_5040[2]= {0x3F2A01A0L,0x1A01A01AL};
        /* 1.9841269841270e-04 */
    static const unsigned long int inv_6[2]   = {0x3FC55555L,0x55555555L};
        /* 1.6666666666667e-01 */
    static const unsigned long int inv_p[2]   = {0x40645F30L,0x6DC9C883L};
        /* 1.6297466172610e+02 */
    static const unsigned long int p1[2]      = {0x3F7921FBL,0x54400000L};
        /* 6.1359231513052e-03 */
    static const unsigned long int p2[2]      = {0x3D50B461L,0x1A626331L};
        /* 2.3738673853540e-13 */
    static const unsigned long int p2_2[2]    = {0x3D50B461L,0x1A800000L};
        /* 2.3738673863338e-13 */
    static const unsigned long int p3[2]      = {0xBB5D9CCEL,0xBA3F91F2L};
        /*-9.7979640872428e-23 */
    static const unsigned long int p3_2[2]    = {0xBB5D9CCEL,0xBA400000L};
        /*-9.7979640872760e-23 */
    static const unsigned long int p4[2]      = {0x38FB839AL,0x252049C1L};
        /* 3.3118858048316e-34 */
    static const unsigned long int threshold_1[2] = { 0x416921FB, 0x2219586A};
        /* 1.3176793065593719e+07 */
#else
    static const unsigned long int inv_120[2] = {0x11111111L,0x3F811111L};
        /* 8.3333333333333e-03 */
    static const unsigned long int inv_24[2]  = {0x55555555L,0x3FA55555L};
        /* 4.1666666666667e-02 */
    static const unsigned long int inv_5040[2]= {0x1A01A01AL,0x3F2A01A0L};
        /* 1.9841269841270e-04 */
    static const unsigned long int inv_6[2]   = {0x55555555L,0x3FC55555L};
        /* 1.6666666666667e-01 */
    static const unsigned long int inv_p[2]   = {0x6DC9C883L,0x40645F30L};
        /* 1.6297466172610e+02 */
    static const unsigned long int p1[2]      = {0x54400000L,0x3F7921FBL};
        /* 6.1359231513052e-03 */
    static const unsigned long int p2[2]      = {0x1A600000L,0x3D50B461L};
        /* 2.3738673853540e-13 */
    static const unsigned long int p2_2[2]    = {0x1A800000L,0x3D50B461L};
        /* 2.3738673863338e-13 */
    static const unsigned long int p3[2]      = {0x00000000L,0xBB600000L};
        /*-9.7979640872428e-23 */
    static const unsigned long int p3_2[2]    = {0x00000000L,0xBB600000L};
        /*-9.7979640872760e-23 */
    static const unsigned long int p4[2]      = {0x00000000L,0x00000000L};
        /* 3.3118858048316e-34 */
    static const unsigned long int threshold_1[2] = { 0x2219586AL, 0x416921FBL};
        /* 1.3176793065593719e+07 */
#endif
#ifdef IEEE
    long int exp;
#endif
    long int m, n, n1, n2;
    double dn, r3, sr, z;
    volatile double rr, r12, r1, r2, s_lead, s_trail, c_lead, pp;

/*******************************
**  Global data definitions   **
*******************************/
    extern unsigned long int dsinfh[][2];
    extern unsigned long int dsinfl[][2];

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
    /*
    ** Argument reduction for this function proceeds by subtracting
    ** a multiple of pi/512 from the input argument.  The size of the
    ** multiple is determined by multiplying x by inv_p, which is 512/pi.
    **
    ** This value is reduced to an integer, then that multiple of
    ** pi/512 is subtracted from the input argument.  This subtraction
    ** proceeds in several steps, using 2, 3, or 4 values which
    ** separately add up to a highly-precise representation of pi/512.
    **
    ** pi/512 = p1 + p2 ( + error )
    **        = p1 + p2_2 + p3 ( + error )
    **        = p1 + p2_2 + p3_2 + p4  ( + error )
    **
    ** The values p1, p2_2, and p3_2 have at least 22 trailing zero bits,
    ** since the argument reduction involves multiplying these values by
    ** (potentially large) integers, and subtracting from x.  For these
    ** multiplications to be exact, the size of the integer and the
    ** number of trailing zeros must be matched.  For large values of x,
    ** the argument reduction utilizes the extended p1,p2_2,p3_2,p4
    ** representation of pi/512.
    **
    ** The error in argument reduction can be expressed as:
    **
    **   error ~= n * abs(p1 + p2_2 + ... + pm - pi/512)
    **          + n * abs(pm) * 2**-53
    **
    ** since the size of the last bit in pm is 2**-52 times pm.
    **
    ** For the maximum reduced argument to be correct to the last bit,
    ** this error must be less than 2**-62.  This follows from the fact that
    ** the reduced argument rr is less than or equal to pi/1024, which is
    ** approximately 2**-9, so that a unit in the last place of rr is
    ** 2**-9 * 2**-52 = 2**-61.
    **
    ** For values of n, the multiplier of pi/512, in the following ranges,
    ** the appropriate values of pm are
    **
    **     n < 2**32, pm = p2
    **       < 2**64, pm = p3
    **
    ** abs(p2) < 2**-41  abs(p1 + p2 - pi/512)               < 2**-95
    ** abs(p3) < 2**-73  abs(p1 + p2_2 + p3 - pi/512)        < 2**-127
    ** abs(p4) < 2**-101 abs(p1 + p2_2 + p3_2 + p4 - pi/512) < 2**-155
    **
    ** For large values of n, care must be taken that the value of rr is
    ** correct.  Precision can be lost when the result of subtracting
    ** one portion of the representation of n*pi/512 from x is close in
    ** magnitude and opposite in sign from the value of the remaining term
    ** in the reduction.  When this occurs, additional terms must be employed.
    ** This problem is avoided by explicitly checking for lost precision in
    ** the calculation of rr.
    **
    */

    /*
    ** Start with special-case handling
    */
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
        /*
        ** For infinity, call the error handler with the infinity signal
        */
        return (qtc_error(DSIN_INFINITY,x));
      }
      /*
      ** If some fraction bits are non-zero, this is a NaN
      */
      else {
        /*
        ** For signaling NaNs, call the error handler with the invalid
        ** operation signal
        */
        if ( is_sNaN(x) )
          return (qtc_error(DSIN_INVALID_OPERATION,x));
        /*
        ** For quiet NaNs, call the error code with the qNaN signal
        */
        else
          return (qtc_error(DSIN_QNAN,x));
      }
    }
    else {
      /*
      ** Check for a denormalized value.  At this point, any exponent
      ** of zero signals either a zero or a denormalized value.
      ** In either case, return the input value.
      */
      if ( exp == 0L )
        return (x);
    }
#endif /* end of IEEE special case handling */

    /*
    ** Use absolute value of x.
    ** Since sin(x) = -sin(-x), if x negative, set m = 1 for proper sign
    */
    if ( MSH(x) >= (unsigned long int) 0x80000000L ) {  /* if x < 0.0 */
        MSH(x) &= 0x7FFFFFFFL;                          /*    x = -x  */
        m = 1;
    }
    else
        m = 0;
    /*
    ** Exceptional cases 
    */
    if (x > DOUBLE(threshold_1)) /* Above this n, n1, or n2 won't fit in int */
#ifdef IEEE
        return (qtc_error(DSIN_OUT_OF_RANGE,x));
#else
        return (qtc_err(DSIN_OUT_OF_RANGE,x));
#endif
    /*
    ** Argument reduction: |x| = (n1 + n2)*pi/512 + rr, where n1 is a
    ** multiple of 512
    */
    n = (int)(x*DOUBLE(inv_p) + 0.5); /* n = intrnd(x*DOUBLE(inv_p)) */
    n2 = n & 0x1FF;
    if (n2 > 256)
        n2 -= 512;   /* -256 < n2 <= 256 */
    n1 = n - n2;
    m += n1 >> 9;
    if (ABS(n2) <= 5) {
        /* 
        ** Use polynomial expansion about 0.0 for small x:
        **     pp = rr - rr**3/3! + rr**5/5! - rr**7/7!
        ** or: pp = ((((-rr**2/7! + 1/5!)*rr**2 - 1/3!)*rr**2)*rr - r2) + r1
        ** where rr is the distance from 0.0
        */
        dn = (double)n1;
        r1 = x - dn*DOUBLE(p1);
        r2 = dn*DOUBLE(p2);
        rr = r1 - r2;
        if ( ( n2 != 0 ) || ( (MSH(rr) & 0x7FF00000L) >= 0x3F600000L ) ) {
            z = rr*rr;
            pp = ((DOUBLE(inv_120) - DOUBLE(inv_5040)*z)*z -
                    DOUBLE(inv_6))*z*rr - r2;
            pp += r1;
        }
        else {
            /*
            ** Use 3 terms for argument reduction very near zero
            ** (rr < 2**(-9))
            */
            r12 = r1 - dn*DOUBLE(p2_2);
            r3 = dn*DOUBLE(p3);
            rr = r12 - r3;
            /*
            ** Check for lost precision in the calculation of rr
            ** and if necessary, use more terms in the argument reduction
            */
            if ( ( MSH(r12) & 0xFFF00000L ) != ( MSH(rr) & 0xFFF00000L ) ) {
                r3 = dn*DOUBLE(p3_2);
                r12 -= r3;
                r3 = dn*DOUBLE(p4);
                rr = r12 - r3;
            }
            z = rr*rr;
            pp = ((DOUBLE(inv_120) - DOUBLE(inv_5040)*z)*z -
                         DOUBLE(inv_6))*z*rr - r3;
            pp += r12;
        }
    }           
    else {
        if (n < 0x00400000L) {  /* n < 2**22 (4194304) */
            r1 = x - (dn = (double)n)*DOUBLE(p1);
            r2 = dn*DOUBLE(p2);
        }
        else {
            r1 = x - ((double)n1)*DOUBLE(p1);
            r1 -= ((double)n2)*DOUBLE(p1);
            r2 = ((double)n)*DOUBLE(p2);
        }
        /*
        ** If n2 < 0, fold into quadrant I (n2 > 0) for table lookup
        ** and increment m to change the sign of the result
        */
        if (n2 < 0) {
            n2 = -n2;
            MSH(r1) ^= 0x80000000L;  /* r1 = -r1 */
            MSH(r2) ^= 0x80000000L;  /* r2 = -r2 */
            m++;
        }
        /*
        ** Get table values for sine and cosine
        */
        LSH(s_lead) = dsinfh[n2][0];
        MSH(s_lead) = dsinfh[n2][1];
        LSH(s_trail) = dsinfl[n2][0];
        MSH(s_trail) = dsinfl[n2][1];
        n2 = 256 - n2;
        LSH(c_lead) = dsinfh[n2][0];
        MSH(c_lead) = dsinfh[n2][1];
        /*
        ** Polynomial approximation of sin(x0+rr) 
        ** sin(x0+rr) = sin(x0)*cos(rr) + cos(x0)*sin(rr)
        **            = sin(x0)*(1 - rr**2/2! + rr**4/4!) +
        **              cos(x0)*(rr - rr**3/3! + rr**5/5!)
        ** where x0 = n2*pi/512
        */
        rr = r1 - r2;
        z = rr*rr;
        pp = s_trail + (DOUBLE(inv_24)*z - 0.5)*z * s_lead;
        sr = (DOUBLE(inv_120)*z - DOUBLE(inv_6))*z*rr - r2;
        pp += (sr + r1)*c_lead;
        pp += s_lead;
    }
    /*
    ** Reconstruction of sin(x):
    **           sin(x) = (-1)**m * pp
    */
    if ((m & 1) != 0)
        MSH(pp) ^= 0x80000000L;

    return (pp);

}   /* end of qtc_dsin() routine */

/*** end of qtc_dsin.c ***/
