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

/*** qtc_ftan.c ***/

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
FUNCTION float qtc_ftan(float param)
/*
**  Purpose: This is a table-based tangent routine.
**
**      Accuracy - less than 1 ULP error
**      Valid input range - |x| < 1.3176793e+07 ( (2**22 - 1) * pi )
**
**      Special case handling - non-IEEE:
**
**        qtc_ftan(+OOR) - calls qtc_ferr() with FTAN_OUT_OF_RANGE, input value
**        qtc_ftan(-OOR) - calls qtc_ferr() with FTAN_OUT_OF_RANGE, input value
**
**      Special case handling - IEEE:
**
**        qtc_ftan(+INF) - calls qtc_ferror() with FTAN_INFINITY, input value
**        qtc_ftan(-INF) - calls qtc_ferror() with FTAN_INFINITY, input value
**        qtc_ftan(QNAN) - calls qtc_ferror() with FTAN_QNAN, input value
**        qtc_ftan(SNAN) - calls qtc_ferror() with FTAN_INVALID_OPERATION,
**                         input value
**        qtc_ftan(DEN)  - returns correct value
**        qtc_ftan(+-0)  - returns +/-0.0
**        qtc_ftan(+OOR) - calls qtc_ferror() with FTAN_OUT_OF_RANGE, input value
**        qtc_ftan(-OOR) - calls qtc_ferror() with FTAN_OUT_OF_RANGE, input value
**
**  Method:
**
**        This function is based on the formula
**
**          tan(x) = tan(x0+rr)
**                 = tan(x0) + tan(rr)
**                   + ( tan(x0) + tan(rr) ) *
**                     ( tan(x0)*tan(rr) + (tan(x0)*tan(rr))**2 + ... )
**
**          where tan(rr) = rr + rr**3/3 + 2*rr**5/15 + ...
**
**  Notes:
**
**         When compiled with the -DIEEE switch, this routine properly
**         handles infinities, NaNs, signed zeros, and denormalized
**         numbers.
**
**  Input Parameters:
*/
/*    float x;  value for which the tangent is desired */
                  /* The value is in radians                           */
/*
**  Output Parameters: none
**
**  Return value: returns the single-precision tangent of the
**                input argument
**
**  Called by:  any
** 
**  History:
**
**        Version 1.0 - 7 November 1988, J F Kauth and L A Westerman
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

    long_float a3;
    long_float a5;
    long_float a7;
    long_float a9;
    long_float a11;
    long_float b3;
    long_float b5;
    long_float halfpi1;
    long_float inv_p;
    long_float inv_pi;
    long_float p1;
    long_float p2;
    long_float p2_2;
    long_float p3;
    long_float p3_2;
    long_float p4;
    long_float pi1;
    long_float pi2;
    long_float pi2_2;
    long_float pi3_2;
    long_float pi4;
    long_float pi4_2;
    long_float pi5;
    long_float spi2_2;
    long_float spi3_2;
    long_float spi4;
    long_float spi4_2;
    long_float threshold_1;
    long_float threshold_2;
    long_float tanx0_high, tanx0_low;


static const unsigned short int correction[32] = {
      0x3100, 0x31C0, 0x3220, 0x3260, 0x3290, 0x32B0, 0x32D0, 0x32F0,
      0x3308, 0x3318, 0x3328, 0x3338, 0x3348, 0x3358, 0x3368, 0x3378,
      0xB378, 0xB368, 0xB358, 0xB348, 0xB338, 0xB328, 0xB318, 0xB308,
      0xB2F0, 0xB2D0, 0xB2B0, 0xB290, 0xB260, 0xB220, 0xB1C0, 0xB100
    };
#ifdef IEEE
    long int exp;
    float dummy;
#endif
    static float temp_return;

    long int an2, er, e1, e2, m, n, nn, n0, n1, n2;
    unsigned long int ic, irr, it1;
    float    fn, fnn, fn0, fn1, fn2, pp, rr, rrp, rx, r_sav;
    volatile float r1, r2;
    float    tanrr, temp, tt, t0, t1, z;

/*******************************
**  Global variables          **
*******************************/
    extern const unsigned long int ftanfh[];
    extern const unsigned long int ftanfl[];

/*******************************
**  External functions        **
*******************************/
#ifdef IEEE
    /* extern float qtc_ferror(); */
#else
    extern float *qtc_ferr();
#endif

/*******************************
** Function body              **
*******************************/
    x.f = param;

    a3.i           = 0x3EAAAAABL;  /* 3.33333343e-01 */
    a5.i           = 0x3E088889L;  /* 1.33333340e-01 */
    a7.i           = 0x3D5D0DD1L;  /* 5.39682545e-02 */
    a9.i           = 0x3CB327A4L;  /* 2.18694881e-02 */
    a11.i          = 0x3C11371BL;  /* 8.86323582e-03 */
    b3.i           = 0x3CB60B61L;  /* 2.22222228e-02 */
    b5.i           = 0x3B0AB356L;  /* 2.11640215e-03 */
    halfpi1.i      = 0x3FC91000L;  /* 1.57080078e+00 */
    inv_p.i        = 0x42A2F983L;  /* 8.14873276e+01 */
    inv_pi.i       = 0x3EA2F983L;  /* 3.18309873e-01 */
    p1.i           = 0x3C491000L;  /* 1.22718811e-02 */
    p2.i           = 0xB315777AL;  /*-3.48004292e-08 */
    p2_2.i         = 0xB3158000L;  /*-3.48081812e-08 */
    p3.i           = 0x2D085A31L;  /* 7.75073148e-12 */
    p3_2.i         = 0x2D088000L;  /* 7.75912667e-12 */
    p4.i           = 0xA8173DCBL;  /*-8.42680807e-15 */
    pi1.i          = 0x40491000L;  /* 3.14160156e+00 */
    pi2.i          = 0xB715777AL;  /*-8.90890988e-06 */
    pi2_2.i        = 0xB7158000L;  /*-8.91089439e-06 */
    pi3_2.i        = 0x31088000L;  /* 1.98633643e-09 */
    pi4.i          = 0xAC173DCBL;  /*-2.14926926e-12 */
    pi4_2.i        = 0xAC170000L;  /*-2.14503906e-12 */
    pi5.i          = 0xA7772CEDL;  /*-3.43024902e-15 */
    spi2_2.i       = 0xB7150000L;  /*-8.88109207e-06 */
    spi3_2.i       = 0xB2EF0000L;  /*-2.78232619e-08 */
    spi4.i         = 0x2CB4611AL;  /* 5.12668813e-12 */
    spi4_2.i       = 0x2CB48000L;  /* 5.13011855e-12 */
    threshold_1.i  = 0x4B490FDAL;  /* 1.31767940e+07 */
    threshold_2.i  = 0x47490FDBL;  /* 5.14718555e+04 */

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
      if ( ( x.i & 0x007FFFFFL ) == 0) {
        /*
        ** For infinity, call the error handler with the infinity signal
        */
        return (qtc_ferror(FTAN_INFINITY,x.f,dummy));
      }
      else {
        /*
        ** If some fraction bits are non-zero, this is a NaN
        */
        if ( is_fsNaN(x) ) {
          /*
          ** For signaling NaNs, call the error code with the invalid
          ** operation signal
          */
          return (qtc_ferror(FTAN_INVALID_OPERATION,x.f,dummy));
        }
        else {
          /*
          ** For quiet NaNs, call the error code with the qNaN signal
          */
          return (qtc_ferror(FTAN_QNAN,x.f,dummy));
        }
      }
    }
    else {
      /*
      ** Check for a denormalized value.  At this point, any exponent
      ** of zero signals either a zero or a denormalized value.
      ** In either case, return the input value.
      */
      if ( exp == 0L )
        return (x.f);
    }
#endif /* end of IEEE special case handling */

    /*
    ** Use absolute value of x.
    ** Since tan(x) = -tan(-x), if x negative, set m = 1 for proper sign
    */
    if ( ( x.i & 0x80000000L ) != 0x0L ) {       /* if x < 0.0 */
        x.i &= 0x7FFFFFFFL;                      /*    x = -x  */
        m = 1;
    }
    else
        m = 0;
    /*
    ** Exceptional cases 
    */
    if ( x.i > threshold_1.i )
#ifdef IEEE
        return (qtc_ferror(FTAN_OUT_OF_RANGE,x.f,dummy));
#else
        return (qtc_ferr(FTAN_OUT_OF_RANGE,x.f));
#endif
    /*
    ** Argument reduction: |x| = (128*m + n2)*pi/256 + rr
    **
    ** If x < threshold_2, the integer portion and the first fraction
    ** bit of x*256/pi is representable in a sp floating point number
    */
    if ( x.i <= threshold_2.i ) {
        temp = x.f*inv_p.f;
        temp += 0.5;
        n = (int)temp;     /* n = intrnd(x*inv_p.f) */
        n2 = n & 0xFF;     /* n2 has 8 bits max */
        if ( n2 > 141 )
            n2 -= 256;   /* -114 <= n2 <= 141 */
        n1 = n - n2;
        an2 = ABS(n2);
        if ( an2 <= 18 ) {
            /* 
            ** Use polynomial expansion about 0.0 for small x:
            **  pp = rr + a3*rr**3 + a5*rr**5 + a7*rr**7 + ...
            ** where rr is the distance from 0.0
            **
            ** Note that if n > 2**20, n1 will have more than 11 bits.
            ** In this case n1 is divided into n0 and n1.
            */
            if ( an2 == 0 ) {                /* n2 = 0 */
                if ( n < 0x80000L ) {
                    fn = (float)n1;
                    temp = fn*p1.f;
                    r1 = x.f - temp;
                    temp = fn*p2_2.f;
                    r1 -= temp;
                    r2 = fn*p3.f;
                    rr = r1 - r2;
                    /*
                    ** The following logic is to handle cases where 
                    ** x is very close to an integer multiple of pi
                    ** (that is, potential "bad points").
                    **
                    ** The p4 term is used if |rr| < 2**(-15).
                    */
                    temp_lf.f = rr;
                    if ( ( 0x7FFFFFFFL & temp_lf.i ) < 0x38000000L ) {
                        temp = fn*p3_2.f;
                        r1 -= temp;
                        r2 = fn*p4.f;
                        rr = r1 - r2;
                    }
                }
                else {
                    nn = n1;
                    n1 = nn & 0x7FFFFL;
                    n0 = nn - n1;
                    fn0 = (float)n0;
                    fn1 = (float)n1;
                    temp = fn0*p1.f;
                    r1 = x.f - temp;
                    temp = fn1*p1.f;
                    r1 -= temp;
                    temp = fn0*p2_2.f;
                    r1 -= temp;
                    temp = fn1*p2_2.f;
                    r1 -= temp;
                    fnn = (float)nn;
                    r2 = fnn*p3.f;
                    rr = r1 - r2;
                    /*
                    ** The following logic is to handle cases where 
                    ** x is very close to an integer multiple of pi
                    ** (that is, potential "bad points").  For these cases,
                    ** there can be a loss in precision if any bits were lost
                    ** in the preceding subtraction.  If so, use one more
                    ** term in the argument reduction
                    */
                    temp_lf.f = rr;
                    if ( ( temp_lf.i & 0x7FFFFFFFL ) < 0x39800000L ) {
                        temp = fn0*p3_2.f;
                        r1 -= temp;
                        temp = fn1*p3_2.f;
                        r1 -= temp;
                        r2 = fnn*p4.f;
                        rr = r1 - r2;
                    }
                }
            }
            else {                         /* n2 = 1,18 */
                if ( n < 0x4000L ) {
                    fn = (float)n1;
                    temp = fn*p1.f;
                    r1 = x.f - temp;
                    r2 = fn*p2.f;
                    rr = r1 - r2;
                }
                else if ( n < 0x80000L ) {
                    fn = (float)n1;
                    temp = fn*p1.f;
                    r1 = x.f - temp;
                    temp = fn*p2_2.f;
                    r1 -= temp;
                    r2 = fn*p3.f;
                    rr = r1 - r2;
                }
                else {
                    nn = n1;
                    n1 = nn & 0x7FFFFL;
                    n0 = nn - n1;
                    fn0 = (float)n0;
                    fn1 = (float)n1;
                    fnn = (float)nn;
                    temp = fn0*p1.f;
                    r1 = x.f - temp;
                    temp = fn1*p1.f;
                    r1 -= temp;
                    temp = fn0*p2_2.f;
                    r1 -= temp;
                    temp = fn1*p2_2.f;
                    r1 -= temp;
                    r2 = fnn*p3.f;
                    rr = r1 - r2;
                }
            }
            z = rr*rr;
            pp = a11.f*z;
            pp += a9.f;
            pp *= z;
            pp += a7.f;
            pp *= z;
            pp += a5.f;
            pp *= z;
            pp += a3.f;
            pp *= z;
            pp *= rr;
            pp -= r2;
            pp += r1;
            /*
            ** Reconstruction of tan(x):
            **           tan(x) = (-1)**m * pp
            **           tan(x) = 1/tan(pi/2-x), for |x| > pi/4
            */
            if ((m & 1) != 0) 
            {
                temp_lf.f = pp;
                temp_lf.i ^= 0x80000000L;
                pp = temp_lf.f;
            }
            temp_return = pp;
            return (temp_return);
        }
        else if ( an2 <= 114 ) {
            /*
            ** Expansion about a table value
            **
            **   Note: this logic can be expanded to cover n2 <= 115
            **         with the tt**5 term or to n2 <= 120 if the
            **         tt**6 term is added.
            **         However, expansion about pi/2 seems to give
            **         slightly better accuracy for n2 in [115,141].
            **
            ** The dominant term in the result is tan(x0), where
            ** x0 = n2*pi/256.  The minimum tan(x0) is tan(19*pi/256)
            ** which has an exponent of -3.
            ** Therefore an acceptable error in rr is 2**(-2-27)
            ** = 2**-30.  To attain this, use the p2 term in argument
            ** reduction for n < 2**18, and the p3 term for larger n.
            **
            ** If n < 2**12, n*p1 is exactly representable
            ** and rr = (x - n*p1) - n*p2,
            */
            if ( n < 0x800L ) {
                fn = (float)n;
                temp = fn*p1.f;
                rr = x.f - temp;
                temp = fn*p2.f;
                rr -= temp;
            }
            /*
            ** Else if n < 2**18, n1*p1 and n2*p1 are 
            ** exactly representable and rr = ((x - n1*p1) - n2*p1) - n*p2
            */
            else if ( n < 0x60000L ) {
                fn1 = (float)n1;
                temp = fn1*p1.f;
                rr = x.f - temp;
                fn2 = (float)n2;
                temp = fn2*p1.f;
                rr -= temp;
                fn = (float)n;
                temp = fn*p2.f;
                rr -= temp;
            }
            /*
            ** Else n <= 2**22 (since x < threshold_2),
            ** n0*p1, n1*p1, n2*p1, n0*p2_2,
            ** n1*p2_2, and n2*p2_2 are exactly representable and 
            ** rr = ((((((x - n0*p1) - n1*p1) - n0*p2_2) - n2*p1)
            **             - n1*p2_2) - n*p3) - n2*p2_2
            */
            else {
                nn = n1;
                n1 = nn & 0x7FFFFL;
                n0 = nn - n1;
                fn0 = (float)n0;
                fn1 = (float)n1;
                temp = fn0*p1.f;
                rr = x.f - temp;
                temp = fn1*p1.f;
                rr -= temp;
                temp = fn0*p2_2.f;
                rr -= temp;
                fn2 = (float)n2;
                temp = fn2*p1.f;
                rr -= temp;
                temp = fn1*p2_2.f;
                rr -= temp;
                fn = (float)n;
                temp = fn*p3.f;
                rr -= temp;
                temp = fn2*p2_2.f;
                rr -= temp;
            }
            /*
            ** If x0+rr is in [-pi/2,0), fold to (0, pi/2]
            */
            if ( n2 < 0 ) {
                n2 = -n2;
                temp_lf.f = rr;
                temp_lf.i ^= 0x80000000L;
                rr = temp_lf.f;
                m++;
            }
            /*
            ** Get table values for tangent
            */
            tanx0_high.i = ftanfh[n2];
            tanx0_low.i  = ftanfl[n2];
            /*
            ** Polynomial approximation of tan(x0+rr):
            **
            ** tan(x0+rr) = tan(x0) + tan(rr) + (tan(x0)+tan(rr))*
            **              (tt + tt**2 + tt**3 + tt**4 + tt**5 + ...)
            ** where x0 = n2*pi/256
            **       tan(rr) = rr + a3*rr**3 + ...
            **       tt = tan(x0)*tan(rr)
            ** For n2 <= 115, need the tt**5 term.
            */
            z = rr*rr;
            tanrr = a3.f*z;
            tanrr *= rr;
            tanrr += rr;
            tt = tanx0_high.f*tanrr;
            pp = tt + 1.0;
            pp *= tt;
            pp += 1.0;
            pp *= tt;
            pp += 1.0;
            pp *= tt;
            pp *= tt;
            pp += tt;
            temp = tanx0_high.f + tanrr;
            pp *= temp;
            temp = tanrr + tanx0_low.f;
            pp += temp;
            pp += tanx0_high.f;
        }
        else {
            /*
            ** n2 in [115,141]:  Taylors series cotangent expansion about pi/2
            **
            **    (Note:  This same logic can be used for n2 in [109,147].)
            **
            **    tan(x) = cot(pi/2-x) = -cot(x-pi/2) =
            **        -1/rr + rr/3 + rr**3/45 + 2*rr**5/945 + ...
            */
            if ( n < 0x7FF00L ) {
                fn = (float)(n1+128);
                temp = fn*p1.f;
                r1 = x.f - temp;
                temp = fn*p2_2.f;
                r1 -= temp;
                r2 = fn*p3.f;
                rr = r1 - r2;
                /*
                ** The following logic is to handle cases where 
                ** x is very close to an integer multiple of pi/2
                ** (that is, potential "bad points").  For these cases,
                ** there can be a loss in precision if any bits were lost
                ** in the preceding subtraction.  If so, use one more
                ** term in the argument reduction
                */
                temp_lf.f = rr;
                if ( ( an2 == 128 ) && 
                     ( (temp_lf.i & 0x7FFFFFFFL) < 0x38000000L) ) {
                    temp = fn*p3_2.f;
                    r1 -= temp;
                    r2 = fn*p4.f;
                    rr = r1 - r2;
                }
            }
            else {
                nn = n1 + 128;
                n1 = nn & 0x7FFFFL;
                n0 = nn - n1;
                fn0 = (float)n0;
                fn1 = (float)n1;
                temp = fn0*p1.f;
                r1 = x.f - temp;
                temp = fn1*p1.f;
                r1 -= temp;
                temp = fn0*p2_2.f;
                r1 -= temp;
                temp = fn1*p2_2.f;
                r1 -= temp;
                fnn = (float)nn;
                r2 = fnn*p3.f;
                rr = r1 - r2;
                /*
                ** The following logic is to handle cases where 
                ** x is very close to an integer multiple of pi/2
                ** (that is, potential "bad points").  For these cases,
                ** there can be a loss in precision if any bits were lost
                ** in the preceding subtraction.  If so, use one more
                ** term in the argument reduction
                */
                temp_lf.f = rr;
                if ( (an2 == 128) &&
                     ( (temp_lf.i & 0x7FFFFFFFL) < 0x39800000L ) ) {
                    temp = fn0 * p3_2.f;
                    r1 -= temp;
                    temp = fn1 * p3_2.f;
                    r1 -= temp;
                    r2 = fnn * p4.f;
                    rr = r1 - r2;
                }
            }
            /*
            ** The signs and relative magnitudes of r1, r2, and rr
            ** determine the calculation order necessary to
            ** preserve precision in the correction term rrp,
            ** which is the lost precision in the calculation of rr.
            ** The following logic chooses the proper method of
            ** determining the lost precision.
            **
            ** e1, e2, and er are the exponents of r1, r2, and rr
            */
            if ( ( MSH(r1) ^ MSH(r2) ) >= (unsigned long int) 0x80000000L ) {
                                                    /* r1, r2 opposite signs */
                rrp = rr - r1;
                rrp += r2;
            }
            else {                                  /* r1, r2 same sign */
                temp_lf.f = r1;
                e1 = temp_lf.i & 0x7F800000L;

                temp_lf.f = r2;
                e2 = temp_lf.i & 0x7F800000L;

                temp_lf.f = rr;
                er = temp_lf.i & 0x7F800000L;

                rrp = rr - r1;
                rrp += r2;
            }
        
            z = rr*rr;
            pp = b5.f*z;  /* Don't need rr**5 term for 120 <= n2 <= 136 */
            pp += b3.f;   /* Don't need rr**3 term for n2 = 128 */
            pp *= z;
            pp += a3.f;   /* b1 == a3 */
            pp *= rr;

            t1 = 1.0 / rr;
  
            /*
            ** The following logic determines the correction for the
            ** reciprocal operation above, by performing part of the
            ** extended-precision calculation, and determining the
            ** sign and magnitude of the correction to be applied to t1
            */
            temp_lf.f = rr;
            irr = temp_lf.i | 0x00800000L;  /* Insert implicit 1 bits (no  */

            temp_lf.f = t1;
            it1 = temp_lf.i | 0x00800000L;  /* need to mask off exponents) */

            ic = ((irr * it1) & 0x00F80000L) >> 19;

                      /* t0 = correction to 1.0/rr: 1/rr == t1 - t0*t1 */
            temp_lf.i = ( (unsigned long int) correction[ic] ) << 16;
            t0 = temp_lf.f;
            /*
            ** We now have 1/rr = t1 - t0*t1; but we need 1/(rr - rrp).
            ** This is approximately
            **      1/rr + rrp/(rr**2) or
            **      t1 - t0*t1 + rrp*t1**2
            */
            temp = t0*t1;
            pp += temp;
            temp = t1*t1;
            temp *= rrp;
            pp -= temp;
            pp -= t1;
        }
    }         /* end of x <= threshold_2 -if- */
    else {
        /*
        ** The integer part and first fraction bit of x*256/pi is not
        ** representable in a s.p. floating point number.  Therefore,
        ** determine n in two steps:  n0,n1 (each 11 bits or less) is
        ** the integer part of x*1/pi; n2 (9 bits or less) is the integer
        ** part of (x - (n0+n1)*pi)*256/pi.
        ** n is then (n0 + n1)*256 + n2.
        */
        temp = x.f*inv_pi.f;
        temp += 0.5;
        nn = (int)temp;      /* nn = intrnd(x*inv_pi.f) */
                             /* This will put rx in [-pi/2,pi/2]     */
                             /* and, equivalently, n2 in [-128,128]  */
        n1 = nn & 0x7FFL;
        n0 = nn - n1;
        fn0 = (float)n0;
        temp = fn0*pi1.f;
        rx = x.f - temp;
        fn1 = (float)n1;
        temp = fn1*pi1.f;
        rx -= temp;
        r_sav = rx;       /* Save for later use, r_sav = x - 256*(n0+n1)*p1 */
        fnn = (float)nn;
        temp = fnn*pi2.f;
        rx -= temp;
        temp = rx*inv_p.f;
        temp_lf.f = temp;
        if ( (temp_lf.i & 0x80000000L) == 0 ) {
            temp += 0.5;
        }
        else {
            temp -= 0.5;
        }
        n2 = (int)temp;      /* n2 = intrnd(rx*inv_p.f) */
        /*
        ** Generally n2 will be in [-128,128].  It is necessary to
        ** use a range of values from -114 <= n2 <= 141, so the
        ** following logic takes care of values outside that range
        */
        if ( n2 < -114 ) {
            n2 += 256;
            nn--;
            if (n1 != 0) {
                n1--;
                fn1 = (float)n1;
            }
            else {
                n1 = 0x7FFL;
                n0 -= 0x800L;
                fn1 = 2047.0;
                fn0 = (float)n0;
            }
            fnn = (float)nn;
            r_sav += pi1.f;
        }
        an2 = ABS(n2);

        if ( an2 <= 18 ) {
            /* 
            ** Use polynomial expansion about 0.0 for small x:
            **  pp = rr + a3*rr**3 + a5*rr**5 + a7*rr**7 + ...
            ** where rr is the distance from 0.0
            */
            temp = fn0*pi2_2.f;
            r1 = r_sav - temp;     /* r_sav =  x - 256*(n0+n1)*p1 */
            temp = fn1*pi2_2.f;
            r1 -= temp;
            temp = fn0*pi3_2.f;
            r1 -= temp;
            r2 = fnn*pi4.f;
            temp = fn1*pi3_2.f;
            r2 += temp;
            rr = r1 - r2;
            /*
            ** The following logic is to handle cases where 
            ** x is very close to an integer multiple of pi
            ** (that is, potential "bad points").  For these cases,
            ** there can be a loss in precision if any bits were lost
            ** in the preceding subtraction.  If so, use one more
            ** term in the argument reduction
            */
            temp_lf.f = rr;
            if ( (an2 == 0) && ( ( temp_lf.i & 0x7FFFFFFFL ) < 0x38800000L ) ) {
               r1 -= temp;
               temp = fn0*pi4_2.f;
               r1 -= temp;
               r2 = fnn*pi5.f;
               temp = fn1*pi4_2.f;
               r2 += temp;
               rr = r1 - r2;
            }
            z = rr*rr;
            pp = a11.f*z;
            pp += a9.f;
            pp *= z;
            pp += a7.f;
            pp *= z;
            pp += a5.f;
            pp *= z;
            pp += a3.f;
            pp *= z;
            pp *= rr;
            pp -= r2;
            pp += r1;
            /*
            ** Reconstruction of tan(x):
            **           tan(x) = (-1)**m * pp
            **           tan(x) = 1/tan(pi/2-x), for |x| > pi/4
            */
            temp_lf.f = pp;
            if ((m & 1) != 0)
                temp_lf.i ^= 0x80000000L;
            pp = temp_lf.f;
            temp_return = pp;
            return (temp_return);
        }
        else if ( an2 <= 114 ) {
            /*
            ** Expansion about a table value
            **
            **   Note: this logic can be expanded to cover n2 <= 115
            **         with the tt**5 term or to n2 <= 120 if the
            **         tt**6 term is added.
            **         However, expansion about pi/2 seems to give
            **         slightly better accuracy for n2 in [115,141].
            **
            ** rr = ((((((x - n0*pi1) - n1*pi1) - n0*pi2_2) - n2*p1)
            **             - n1*pi2_2) - n*p3) - n2*p2_2
            */
            temp = fn0*pi2_2.f;
            rr = r_sav - temp;     /* r_sav =  x - 256*(n0+n1)*p1 */
            fn2 = (float)n2;
            temp = fn2*p1.f;
            rr -= temp;
            temp = fn1*pi2_2.f;
            rr -= temp;
            fn = (float)((nn << 8) + n2);  /* n = 256*nn + n2 */
            temp = fn*p3.f;
            rr -= temp;
            temp = fn2*p2_2.f;
            rr -= temp;
            /*
            ** If j < 0, fold into quadrant I (j > 0) for table lookup
            ** and increment m to change the sign of the result
            */
            if ( n2 < 0 ) {
                n2 = -n2;
                temp_lf.f = rr;
                temp_lf.i ^= 0x80000000L;
                rr = temp_lf.f;         /* rr = -rr */
                m++;
            }
            /*
            ** Get table values for tangent
            */
            tanx0_high.i = ftanfh[n2];
            tanx0_low.i  = ftanfl[n2];
            /*
            ** Polynomial approximation of tan(x0+rr):
            **
            ** tan(x0+rr) = tan(x0) + tan(rr) + (tan(x0)+tan(rr))*
            **              (tt + tt**2 + tt**3 + tt**4 + tt**5 + ...)
            ** where x0 = n2*pi/256
            **       tan(rr) = rr + a3*rr**3 + ...
            **       tt = tan(x0)*tan(rr)
            ** For n2 <= 115, need the tt**5 term.
            */
            z = rr*rr;
            tanrr = a3.f*z;
            tanrr *= rr;
            tanrr += rr;
            tt = tanx0_high.f*tanrr;
            pp = tt + 1.0;
            pp *= tt;
            pp += 1.0;
            pp *= tt;
            pp += 1.0;
            pp *= tt;
            pp *= tt;
            pp += tt;
            temp = tanx0_high.f + tanrr;
            pp *= temp;
            temp = tanrr + tanx0_low.f;
            pp += temp;
            pp += tanx0_high.f;
        }
        else {
            /*
            ** n2 in [115,141]:  Taylors series cotangent expansion about pi/2
            **
            **    (Note:  This same logic can be used for n2 in [109,147].)
            **
            **    tan(x) = cot(pi/2-x) = -cot(x-pi/2) =
            **        -1/rr + rr/3 + rr**3/45 + 2*rr**5/945 + ...
            */
            temp = fn0*spi2_2.f;
            r1 = r_sav - temp;     /* r_sav =  x - 256*(n0+n1)*p1 */
            temp = halfpi1.f;
            r1 -= temp;
            fn1 += 0.5;   /* Add 1/2 to n1 and nn so that rr is the       */
            fnn += 0.5;   /* distance from pi/2; now n1 has up to 12 bits */
            temp = fn1*spi2_2.f;
            r1 -= temp;
            temp = fn0*spi3_2.f;
            r1 -= temp;
            an2 = ABS(an2-128);
            r2 = fnn * spi4.f;
            temp = fn1 * spi3_2.f;
            r2 += temp;
            rr = r1 - r2;
            /*
            ** The following logic is to handle cases where 
            ** x = SP(k*pi) and SP(k*pi) is very close to k*pi
            ** (that is, potential "bad points").
            **
            ** The p4 term is used if |rr| < 2**(-13).
            */
            temp_lf.f = rr;
            if ( (an2 == 0) && ( (0x7FFFFFFFL & temp_lf.i) < 0x39000000L ) ) {
                r1 -= temp;
                temp = fn0*spi4_2.f;
                r1 -= temp;
                r2 = fnn*pi5.f;
                temp = fn1*spi4_2.f;
                r2 += temp;
                rr = r1 - r2;
            }
            /*
            ** The signs and relative magnitudes of r1, r2, and rr
            ** determine the calculation order necessary to
            ** preserve precision in the correction term rrp,
            ** which is the lost precision in the calculation of rr.
            ** The following logic chooses the proper method of
            ** determining the lost precision.
            **
            ** e1, e2, and er are the exponents of r1, r2, and rr
            */
            if ( ( MSH(r1) ^ MSH(r2) ) >= (unsigned long int) 0x80000000L ) {
                                                    /* r1, r2 opposite signs */
                rrp = rr - r1;
                rrp += r2;
            }
            else {                                  /* r1, r2 same sign */
                temp_lf.f = r1;
                e1 = temp_lf.i & 0x7F800000L;

                temp_lf.f = r2;
                e2 = temp_lf.i & 0x7F800000L;

                temp_lf.f = rr;
                er = temp_lf.i & 0x7F800000L;

                rrp = rr - r1;
                rrp += r2;
            }
        
            z = rr*rr;
            pp = b5.f*z;  /* Don't need rr**5 term for 120 <= n2 <= 136 */
            pp += b3.f;   /* Don't need rr**3 term for n2 = 128 */
            pp *= z;
            pp += a3.f;   /* b1 == a3 */
            pp *= rr;

            t1 = 1.0 / rr;
  
            /*
            ** The following logic determines the correction for the
            ** reciprocal operation above, by performing part of the
            ** extended-precision calculation, and determining the
            ** sign and magnitude of the correction to be applied to t1
            */
            temp_lf.f = rr;
            irr = temp_lf.i | 0x00800000L;  /* Insert implicit 1 bits (no  */

            temp_lf.f = t1;
            it1 = temp_lf.i | 0x00800000L;  /* need to mask off exponents) */

            ic = ((irr * it1) & 0x00F80000L) >> 19;
            temp_lf.i = ( (unsigned long int) correction[ic] ) << 16;
                      /* t0 = correction to 1.0/rr: 1/rr == t1 - t0*t1 */
            t0 = temp_lf.f;

            /*
            ** We now have 1/rr = t1 - t0*t1; but we need 1/(rr - rrp).
            ** This is approximately
            **      1/rr + rrp/(rr**2) or
            **      t1 - t0*t1 + rrp*t1**2
            */
            temp = t0*t1;
            pp += temp;
            temp = t1*t1;
            temp *= rrp;
            pp -= temp;
            pp -= t1;
        }
    }      /* end of threshold_2 < x < threshold_1 -else- */
    /*
    ** Reconstruction of tan(x):
    **           tan(x) = (-1)**m * pp
    **           tan(x) = 1/tan(pi/2-x), for |x| > pi/4
    */
    if ( ( m & 1 ) != 0)
    {
        temp_lf.f = pp;
        temp_lf.i ^= 0x80000000L;
        pp        = temp_lf.f;
    }
    temp_return = pp;
    return (temp_return);
}
/*** end of qtc_ftan.c ***/
