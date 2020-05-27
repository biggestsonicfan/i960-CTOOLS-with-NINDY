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

/*** qtc_dtan.c ***/

#include "qtc_intr.h"
#include "qtc.h"

/*
******************************************************************************
**
*/
FUNCTION double qtc_dtan(param)
/*
**  Purpose: This is a table-based tangent routine, which uses special
**           tables to extend the range of values over which the table
**           expansion is done.
**
**      Accuracy - less than 1 ULP error
**      Valid input range - |x| < 1.3176793e+07 ( (2**22 - 1) * pi )
**
**      Special case handling - non-IEEE:
**
**        qtc_dtan(+OOR) - calls qtc_err() with DTAN_OUT_OF_RANGE, input value
**        qtc_dtan(-OOR) - calls qtc_err() with DTAN_OUT_OF_RANGE, input value
**
**      Special case handling - IEEE:
**
**        qtc_dtan(+INF) - calls qtc_error() with DTAN_INFINITY, input value
**        qtc_dtan(-INF) - calls qtc_error() with DTAN_INFINITY, input value
**        qtc_dtan(QNAN) - calls qtc_error() with DTAN_QNAN, input value
**        qtc_dtan(SNAN) - calls qtc_error() with DTAN_INVALID_OPERATION,
**                         input value
**        qtc_dtan(DEN)  - returns correct value
**        qtc_dtan(+-0)  - returns +/-0.0
**        qtc_dtan(+OOR) - calls qtc_error() with DTAN_OUT_OF_RANGE, input value
**        qtc_dtan(-OOR) - calls qtc_error() with DTAN_OUT_OF_RANGE, input value
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
**         One of the compile line switches -DMSH_FIRST or -DLSH_FIRST should
**         be specified when compiling this source.
**
**  Input Parameters:
*/
    double param; /* input value for which the tangent is desired, in radians */
/*
**  Output Parameters: none
**
**  Return value: returns the double-precision tangent of the
**                input argument
**
**  Called by:  any
** 
**  History:
**
**        Version 1.0 - 8 July 1988, J F Kauth and L A Westerman
**                1.1 - 18 July 1988, L A Westerman, corrected errors
**                      in variable definitions
**                1.2 - 27 July 1988, J F Kauth, eliminated p4 term
**                      in argument reduction near multiples of pi
**                1.3 - 31 October 1988, L A Westerman, restored p4 term
**                      and added code for values extremely close to
**                      multiples of pi
**                1.4 - 30 November 1988, J F Kauth, changed floating point
**                      "1" to "1.0"
**                1.5 - 20 February 1989, L A Westerman, added extra static
**                      double to force double-word alignment
**                1.6 - 1 March 1989, L A Westerman, changed formula slightly
**                      to eliminate intermediate product, which was generating
**                      inaccurate results in the Sun installation
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
    static const unsigned long int a11[2]     = {0x3F8226E3L,0x55E6C23DL};
        /* 8.8632355299022e-03 */
    static const unsigned long int a13[2]     = {0x3F6D6D3DL,0x0E157DE0L};
        /* 3.5921280365725w-03 */
    static const unsigned long int a15[2]     = {0x3F57DA36L,0x452B75E3L};
        /* 1.4558343870513e-03 */
    static const unsigned long int a17[2]     = {0x3F435582L,0x48036744L};
        /* 5.9002744094559e-04 */
    static const unsigned long int a3[2]      = {0x3FD55555L,0x55555555L};
        /* 3.3333333333333e-01 */
    static const unsigned long int a5[2]      = {0x3FC11111L,0x11111111L};
        /* 1.3333333333333e-01 */
    static const unsigned long int a7[2]      = {0x3FABA1BAL,0x1BA1BA1CL};
        /* 5.3968253968254e-02 */
    static const unsigned long int a9[2]      = {0x3F9664F4L,0x882C10FAL};
        /* 2.1869488536155e-02 */
    static const unsigned long int b3[2]      = {0x3F96C16CL,0x16C16C17L};
        /* 2.2222222222222e-02 */
    static const unsigned long int b5[2]      = {0x3F61566AL,0xBC011567L};
        /* 2.1164021164021e-03 */
    static const unsigned long int halfpi1[2] = {0x3FF921FBL,0x54400000L};
        /* 1.5707963267341e+00 */
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
    static const unsigned long int p3_2[2]    = {0xBB5D9CCEL,0xBA800000L};
        /*-9.7979640922063e-23 */
    static const unsigned long int p4[2]      = {0x39701B83L,0x9A25204AL};
        /* 4.9634995156796e-32 */
    static const unsigned long int threshold_1[2] = { 0x416921FBL, 0x2219586AL};
        /* 1.3176793065593719e+07 */
#else
    static const unsigned long int a11[2]     = {0x55E6C23DL,0x3F8226E3L};
        /* 8.8632355299022e-03 */
    static const unsigned long int a13[2]     = {0x0E157DE0L,0x3F6D6D3DL};
        /* 3.5921280365725w-03 */
    static const unsigned long int a15[2]     = {0x452B75E3L,0x3F57DA36L};
        /* 1.4558343870513e-03 */
    static const unsigned long int a17[2]     = {0x48036744L,0x3F435582L};
        /* 5.9002744094559e-04 */
    static const unsigned long int a3[2]      = {0x55555555L,0x3FD55555L};
        /* 3.3333333333333e-01 */
    static const unsigned long int a5[2]      = {0x11111111L,0x3FC11111L};
        /* 1.3333333333333e-01 */
    static const unsigned long int a7[2]      = {0x1BA1BA1CL,0x3FABA1BAL};
        /* 5.3968253968254e-02 */
    static const unsigned long int a9[2]      = {0x882C10FAL,0x3F9664F4L};
        /* 2.1869488536155e-02 */
    static const unsigned long int b3[2]      = {0x16C16C17L,0x3F96C16CL};
        /* 2.2222222222222e-02 */
    static const unsigned long int b5[2]      = {0xBC011567L,0x3F61566AL};
        /* 2.1164021164021e-03 */
    static const unsigned long int halfpi1[2] = {0x54400000L,0x3FF921FBL};
        /* 1.5707963267341e+00 */
    static const unsigned long int inv_p[2]   = {0x6DC9C883L,0x40645F30L};
        /* 1.6297466172610e+02 */
    static const unsigned long int p1[2]      = {0x54400000L,0x3F7921FBL};
        /* 6.1359231513052e-03 */
    static const unsigned long int p2[2]      = {0x1A600000L,0x3D50B461L};
        /* 2.3738673853540e-13 */
    static const unsigned long int p2_2[2]    = {0x1A800000L,0x3D50B461L};
        /* 2.3738673863338e-13 */
    static const unsigned long int p3[2]      = {0x00000000L,0XBB600000L};
        /*-9.7979640872428e-23 */
    static const unsigned long int p3_2[2]    = {0x00000000L,0xBB600000L};
        /*-9.7979640922063e-23 */
    static const unsigned long int p4[2]      = {0x00000000L,0x00000000L};
        /* 4.9634995156796e-32 */
    static const unsigned long int threshold_1[2] = { 0x2219586AL, 0x416921FBL};
        /* 1.3176793065593719e+07 */
#endif
#ifdef IEEE
    long int exp;
#endif
    int      i, i7;
    long int an2, e1, e2, er, m, n, n1, n2;
    unsigned long int ia0, ia1, ia2, ia3;
    unsigned long int ib0, ib1, ib2, ib3;
    unsigned long int ir2, ir3;
    double   dn, ppp, r3, tanrr;
    double   tanx0_high, tanx0_low, temp, z;
    volatile double rr, r12, tanx0_high, tanx0_low, tt, delta, r1, r2,t1,t0, pp;

    static const unsigned short int ulps[] = {
      0x3C50, 0x3C68, 0x3C74, 0x3C7C, 0x3C82, 0x3C86, 0x3C8A, 0x3C8E,
      0x3C91, 0x3C93, 0x3C95, 0x3C97, 0x3C99, 0x3C9B, 0x3C9D, 0x3C9F,
      0xBC9F, 0xBC9D, 0xBC9B, 0xBC99, 0xBC97, 0xBC95, 0xBC93, 0xBC91,
      0xBC8E, 0xBC8A, 0xBC86, 0xBC82, 0xBC7C, 0xBC74, 0xBC68, 0xBC50
    };
    double rrp;

/*******************************
**  Global variables          **
*******************************/
    extern const unsigned long int dtanfh[][2];
    extern const unsigned long int dtanfl[][2];
    extern const unsigned long int dtnf2h[][2];
    extern const unsigned long int dtnf2l[][2];

/*******************************
**  External functions        **
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
        /*
        ** For infinity, call the error code with the infinity signal
        */
        return (qtc_error(DTAN_INFINITY,x));
      }
      else {
        /*
        ** If some fraction bits are non-zero, this is a NaN
        */
        if ( is_sNaN(x) ) {
          /*
          ** For signaling NaNs, call the error code with the invalid
          ** operation signal
          */
          return (qtc_error(DTAN_INVALID_OPERATION,x));
        }
        else {
          /*
          ** For quiet NaNs, call the error code with the qNaN signal
          */
          return (qtc_error(DTAN_QNAN,x));
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
        return (x);
    }
#endif /* end of IEEE special case handling */

    /*
    ** Use absolute value of x.
    ** Since tan(x) = -tan(-x), if x negative, set m = 1 for proper sign
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
    if (x > DOUBLE(threshold_1))
#ifdef IEEE
        return (qtc_error(DTAN_OUT_OF_RANGE,x));
#else
        return (qtc_err(DTAN_OUT_OF_RANGE,x));
#endif
    /*
    ** Argument reduction: |x| = (n1 + n2)*pi/512 + rr, where n1 is
    ** a multiple of 512
    */
    n = (int)(x*DOUBLE(inv_p) + 0.5); /* n = intrnd(x*DOUBLE(inv_p)) */
    n2 = n & 0x1FF;
    if (n2 > 265)
        n2 -= 512;   /* -246 <= n2 <= 265 */
    n1 = n - n2;
    m += n1 >> 8;    /* m = m + n1/256  (note that n1 is always > 0) */
    an2 = ABS(n2);
    if ( an2 <= 27 ) {
        /* 
        ** Use polynomial expansion about 0.0 for small x:
        **  pp = rr + a3*rr**3 + a5*rr**5 + a7*rr**7 + ...
        ** where rr is the distance from 0.0
        ** (Could expand this region to handle n2 = 28,34 if use rr**19 term.)
        */
        dn = (double)n1;
        r1 = x - dn*DOUBLE(p1);
        r2 = dn*DOUBLE(p2);
        rr = r1 - r2;
        if ( ( an2 != 0 ) || ( (MSH(rr) & 0x7FF00000L) >= 0x3F600000L) ) {
            z = rr*rr;
            /*
            ** The following logic could be collapsed by including extra
            ** (unnecessary) terms in the polynomial expansion
            */
            if (an2 >= 16) {
                if (an2 >= 22) {    /* n2 = 22,27 */
                    pp = (((((((DOUBLE(a17)*z +
                              DOUBLE(a15))*z +
                              DOUBLE(a13))*z +
                              DOUBLE(a11))*z +
                              DOUBLE(a9))*z +
                              DOUBLE(a7))*z +
                              DOUBLE(a5))*z +
                              DOUBLE(a3))*z*rr -
                              r2;
                }
                else {              /* n2 = 16,21 */
                    pp = ((((((DOUBLE(a15)*z +
                              DOUBLE(a13))*z +
                              DOUBLE(a11))*z +
                              DOUBLE(a9))*z +
                              DOUBLE(a7))*z +
                              DOUBLE(a5))*z +
                              DOUBLE(a3))*z*rr -
                              r2;
                }
            }
            else if (an2 >= 5) {
                if (an2 >= 9) {     /* n2 = 9,15  */
                    pp = (((((DOUBLE(a13)*z +
                              DOUBLE(a11))*z +
                              DOUBLE(a9))*z +
                              DOUBLE(a7))*z +
                              DOUBLE(a5))*z +
                              DOUBLE(a3))*z*rr -
                              r2;
                }
                else {              /* n2 = 5,8   */
                    pp =  ((((DOUBLE(a11)*z +
                              DOUBLE(a9))*z +
                              DOUBLE(a7))*z +
                              DOUBLE(a5))*z +
                              DOUBLE(a3))*z*rr -
                              r2;
                }
            }
            else {
                if (an2 >= 2) {     /* n2 = 2,4   */
                    pp =   (((DOUBLE(a9)*z +
                              DOUBLE(a7))*z +
                              DOUBLE(a5))*z +
                              DOUBLE(a3))*z*rr -
                              r2;
                }
                else {              /* n2 = 0,1   */
                    pp = ((DOUBLE(a7)*z +
                           DOUBLE(a5))*z +
                           DOUBLE(a3))*z*rr -
                           r2;
                }
            }
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
            if ( ( MSH(rr) & 0xFFF00000L ) != ( MSH(r12) & 0xFFF00000L ) ) {
                r12 -= dn*DOUBLE(p3_2);
                r3 = dn*DOUBLE(p4);
                rr = r12 - r3;
            }
            z = rr*rr;
            pp = ((DOUBLE(a7)*z +
                       DOUBLE(a5))*z +
                       DOUBLE(a3))*z*rr -
                       r3;
            pp += r12;
        }
    }           
    else if (an2 <= 246) {
        if (n < 0x00400000L) {  /* n < 2**22 (4194304) */
            rr = x - (dn = (double)n)*DOUBLE(p1);
            rr -= dn*DOUBLE(p2);
        }
        else {
            rr = x - ((double)n1)*DOUBLE(p1);
            rr -= ((double)n2)*DOUBLE(p1);
            rr -= ((double)n)*DOUBLE(p2);
        }
        /*
        ** If x0+rr is in [-pi/2,0), fold to (0, pi/2]
        */
        if (n2 < 0) {
            n2 = -n2;
            MSH(rr) ^= 0x80000000L;  /* rr = -rr */
            m++;
        }
        /*
        ** Get table values for tangent
        */
        LSH(tanx0_high) = dtanfh[n2][0];
        MSH(tanx0_high) = dtanfh[n2][1];
        LSH(tanx0_low)  = dtanfl[n2][0];
        MSH(tanx0_low)  = dtanfl[n2][1];
        /*
        ** Polynomial approximation of tan(x0+rr) 
        ** tan(x0+rr) = tan(x0) + tan(rr) + (tan(x0)+tan(rr))*
        **              ((tan(x0)*tan(rr)) + (tan(x0)*tan(rr))**2 + ...)
        ** where x0 = n2*pi/512, and tan(rr) = rr + a3*rr**3 + a5*rr**5 + ...
        */
        z = rr*rr;
        tanrr = (DOUBLE(a5)*z + DOUBLE(a3))*z*rr + rr;
        tt = tanx0_high*tanrr;
        if (n2 <= 139) {
            if (n2 <= 68) {     /* j = 28,68  (could handle 20,68) */
                pp = (tanx0_high+tanrr)*
                     ((((tt + 1.0)*tt + 1.0)*tt + 1.0)*tt*tt + tt) +
                     tanx0_low;
            }
            else {              /* j = 69,139  */
                pp = (tanx0_high+tanrr)*
                     (((((tt + 1.0)*tt + 1.0)*tt + 1.0)*tt + 1.0)*tt*tt + tt) +
                     tanx0_low;
            }
            pp += tanrr;
        }
        else if (n2 <= 216) {
            if (n2 <= 190) {    /* j = 140,190 */
                pp = (tanx0_high+tanrr)*
                     ((((((tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt*tt+tt);
            }
            else {              /* j = 191,216 */
                pp = (tanx0_high+tanrr)*
                     (((((((tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)
                     *tt*tt+tt);
            }
            temp = tanx0_low + tanrr;
            pp += temp;
        }
        else if (n2 <= 237) {
            if (n2 <= 230) {    /* j = 217,230 */
                pp = (tanx0_high+tanrr)*
                     ((((((((tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)
                     *tt+1.0)*tt+1.0)*tt+1.0)*tt*tt+tt);
            }
            else {              /* j = 231,237 */
                pp = (tanx0_high+tanrr)*
                     (((((((((tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)
                     *tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt*tt+tt);
            }
            temp = tanx0_low + tanrr;
            pp += temp;
        }
        else if (n2 <= 245) {
            if (n2 <= 242) {    /* j = 238,242 */
                pp = (tanx0_high+tanrr)*
                     ((((((((((tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)
                     *tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt*tt+tt);
                temp = tanx0_low + tanrr;
                pp += temp;
            }
            else {              /* j = 243,245 */
                /*
                ** For n2 = 243, 244, and 245, need a correction term:
                **       2 * tanx0_high * tanx0_low * tanrr
                **           (this is 2*tanx0_low*tt)
                ** to compensate for loss of precision caused by
                ** approximating tan(x0) by tanx0_high in the polynomial fit.
                */
                ppp = (tanx0_high+tanrr)*
                     (((((((((((tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)
                     *tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt*tt+tt);
                MSH(tt) += 0x00100000L;   /* tt = 2*tt */
                pp = tanx0_low + tanx0_low*tt;
                pp += tanrr;
                pp += ppp;
            }
        }
        else {                  /* j = 246     */
            ppp = (tanx0_high+tanrr)*
                  ((((((((((((tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)
                  *tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt*tt+tt);
            MSH(tt) += 0x00100000L;   /* tt = 2*tt */
            pp = tanx0_low + tanx0_low*tt;
            pp += tanrr;
            pp += ppp;
        }
        pp += tanx0_high;
    }
    else {
        /*
        ** n2 in [247,265]:  special case handling around pi/2
        */
        if (n1 < 0x40000000L) {
            dn = (double)(n1+256);
            r1 = x - dn*DOUBLE(p1);
        }
        else {
            dn = (double)n1;
            r1 = x - dn*DOUBLE(p1);
            r1 -= DOUBLE(halfpi1);
            dn = (double)(n1+256);
        }
        r2 = dn*DOUBLE(p2);
        rr = r1 - r2;
        if ((MSH(rr) & 0x7FF00000L) >= 0x3F700000L) {
            /*
            ** rr >= 2**(-8)  (j = 247,254 and part of 255)
            **
            ** Get table values and generate delta = pi/2 - x0.
            ** The index, i, into the table is the last 4 exponent bits of
            ** rr (minus 7) and the first 3 fraction bits of rr.
            ** delta = pi/2 - x0, is the midpoint of interval i and
            ** is generated from rr (rr's exponent and 1st 3 fraction
            ** bits, a 4th fraction bit of 1, and remaining fraction bits
            ** of zero).
            */
            i = ((MSH(rr) & 0x00FE0000L) - 0x00700000L) >> 17;
            LSH(tanx0_high) = dtnf2h[i][0];
            MSH(tanx0_high) = dtnf2h[i][1];
            LSH(tanx0_low)  = dtnf2l[i][0];
            MSH(tanx0_low)  = dtnf2l[i][1];
            LSH(delta) = 0;
            MSH(delta) = (MSH(rr) & 0x7FFE0000L) | 0x00010000L;
            /*
            ** If x0+rr is in (pi/2,pi/2+d), fold to (pi/2-d,pi/2)
            */
            if (n2 <= 256) {
                rr = r1 + delta;
                rr -= r2;
            }
            else {             /* rr = -rr; change sign of result */
                rr = delta - r1;
                rr += r2;
                m++;
            }
            z = rr*rr;
            tanrr = (DOUBLE(a5)*z + DOUBLE(a3))*z*rr + rr;
            tt = tanx0_high*tanrr;
            if ( (i7 = i & 7) >= 5) {      /* i7 = 5-7 */
                ppp = (tanx0_high+tanrr)*
                     ((((((((((tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)
                     *tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt*tt+tt);
            }
            else if (i7 >= 2) {           /* i7 = 2-4 */
                ppp = (tanx0_high+tanrr)*
                     (((((((((((tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)
                     *tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt*tt+tt);
            }
            else {                        /* i7 = 0-1 */
                ppp = (tanx0_high+tanrr)*
                     ((((((((((((tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)
                     *tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt+1.0)*tt*tt+tt);
            }
            MSH(tt) += 0x00100000L;   /* tt = 2*tt */
            pp = tanx0_low + tanx0_low*tt;
            pp += tanrr;
            pp += ppp;
            pp += tanx0_high;
        }
        else {
            /*
            ** |rr| < 2**(-8)   (j = part of 255,256)
            **
            ** tan(x) = cot(pi/2-x) = -cot(x-pi/2) =
            **      -1/rr + rr/3 + rr**3/45 + 2*rr**5/945 + ...
            **
            ** The signs and relative magnitudes of r1, r2, and rr
            ** determine the calculation order necessary to
            ** preserve precision in the correction term rrp,
            ** which is the lost precision in the calculation of rr.
            ** The following logic chooses the proper method of
            ** determining the lost precision.
            **
            ** e1, e2, and er are the exponents of r1, r2, and rr
            */
            r2 = dn*DOUBLE(p2_2);
            r12 = r1 - r2;
            r3 = dn*DOUBLE(p3);
            rr = r12 - r3;
            /*
            ** Check for lost precision in the calculation of rr
            ** and if necessary, use more terms in the argument reduction
            */
            if ( ( MSH(rr) & 0xFFF00000L ) != ( MSH(r12) & 0xFFF00000L ) ) {
                r1 = r12;
                r2 = dn * DOUBLE(p3_2);
                r12 = r1 - r2;
                r3 = dn*DOUBLE(p4);
                rr = r12 - r3;
            }
            if ( ( MSH(r1) ^ MSH(r2) ) >= (unsigned long int) 0x80000000L ) {
                                                    /* r1, r2 opposite signs */
                rrp = rr - r1;
                rrp += r2;
            }
            else {                                  /* r1, r2 same sign */
                e1 = MSH(r1) & 0x7FF00000L;
                e2 = MSH(r2) & 0x7FF00000L;
                er = MSH(rr) & 0x7FF00000L;
                if ( ( e1 > e2 ) && ( e2 < er ) ) {
                    rrp = rr - r1;
                    rrp += r2;
                }
                else {
                    r1 -= r2;
                    rrp = rr - r1;
                }
            }
            rrp += r3;
        
            z = rr*rr;
            pp = ((DOUBLE(b5)*z + DOUBLE(b3))*z)*rr;
            t1 = 1.0 / rr;
  
            /*
            ** The following logic determines the correction for the
            ** reciprocal operation above, by performing part of the
            ** extended-precision calculation, and determining the
            ** sign and magnitude of the correction to be applied to t1
            */
            ia0 = ( ( MSH(rr) & 0x000FFC00L ) >> 10 ) | 0x400L;
            ia1 = ((MSH(rr)&0x000003FFL) << 4 ) | (( LSH(rr)&0xF0000000L )
                   >> 28 );
            ia2 = ( LSH(rr) & 0x0FFFC000L ) >> 14;
            ia3 = ( LSH(rr) & 0x00003FFFL );

            ib0 = ( ( MSH(t1) & 0x000FFC00L ) >> 10 ) | 0x400L;
            ib1 = ((MSH(t1) & 0x000003FFL) << 4 ) | ((LSH(t1)&0xF0000000L )
                   >> 28 );
            ib2 = ( LSH(t1) & 0x0FFFC000L ) >> 14;
            ib3 = ( LSH(t1) & 0x00003FFFL );
  
            ir2 = ia1 * ib3;
            ir2 += ia2 * ib2;
            ir2 += ia3 * ib1;
            ir3 = ir2 >> 14;
            ir3 += ia0 * ib3;
            ir3 += ia1 * ib2;
            ir3 += ia2 * ib1;
            ir3 += ia3 * ib0;

            ir3 = ( ir3 >> 6 ) & 0x0000001FL;

            LSH(t0) = 0;
            MSH(t0) = 0;
            MSQ(t0) = ulps[ir3];
            t0 -= rrp*t1;
            pp += t0*t1;
            pp += DOUBLE(a3)*rr;
            pp -= t1;
        }
    }
    /*
    ** Reconstruction of tan(x):
    **           tan(x) = (-1)**m * pp
    **           tan(x) = 1/tan(pi/2-x), for |x| > pi/4
    */
    if ( ( m & 1 ) != 0 )
        MSH(pp) ^= 0x80000000L;

    return (pp);

}   /* end of qtc_dtan() routine */

/*** end of qtc_dtan.c ***/
