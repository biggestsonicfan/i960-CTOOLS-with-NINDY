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

/*** qtc_dexpm1.c ***/

#include "qtc_intr.h"
#include "qtc.h"

/*
******************************************************************************
**
*/
FUNCTION double qtc_dexpm1(param)
/*
**  Purpose: This is a table-based natural exponential minus one routine
**
**           Accuracy - less than 1 ULP
**           Valid input range - -708.3964 < x < 709.7827
**
**           Special case handling - non-IEEE:
**
**             qtc_dexpm1(+OOR) - calls qtc_err() with DEXP_OUT_OF_RANGE,
**                                input value
**             qtc_dexpm1(-OOR) - calls qtc_err() with DEXP_OUT_OF_RANGE,
**                                input value
**
**           Special case handling - IEEE:
**
**             qtc_dexpm1(+INF) - calls qtc_error() with DEXP_INFINITY,
**                                input value
**             qtc_dexpm1(-INF) - returns 0.0
**             qtc_dexpm1(QNAN) - calls qtc_error() with DEXP_QNAN, input value
**             qtc_dexpm1(SNAN) - calls qtc_error() with DEXP_INVALID_OPERATION,
**                                input value
**             qtc_dexpm1(DEN)  - returns 1.0
**             qtc_dexpm1(+-0)  - returns 1.0
**             qtc_dexpm1(+OOR) - calls qtc_error() with DEXP_OUT_OF_RANGE,
**                                input value
**             qtc_dexpm1(-OOR) - calls qtc_error() with DEXP_OUT_OF_RANGE,
**                                input value
**
**  Method:
**
**        This function is based on the formula
**
**          if x = (32*m + j)*log2/32 + rr,
**
**          exp(x) = exp((32*m+j)*log2/32+rr) - 1
**                 = exp((32*m+j)*log2/32) * exp(rr) -1 
**                 = 2**m * 2**(j/32) * (1 + rr + rr**2/2! + rr**3/3! + .. ) - 1
**
**  Notes:
**         When compiled with the -DIEEE switch, this routine properly
**         handles infinities, NaNs, signed zeros, and denormalized
**         numbers.
**
**         One of the compile line switches -DMSH_FIRST or -DLSH_FIRST should
**         be specified when compiling this source.
**
**  Input Parameters:
*/
    double param; /* input value for which the exponential is desired */
/*
**  Output Parameters: none
**
**  Return value: returns the double-precision natural exponential minus one
**                of the input argument
**
**  Called by:  any
** 
**  History:
**
**        Version 1.0 - 25 October 1988, K A Wildermuth
** 
**  Copyright: 
**      (c) 1988 Quantitative Technology Corporation.
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
#ifdef MSH_FIRST
    static const unsigned long int threshold_1[2] =
        {0x40862E42L, 0xFEFA39EFL};  /* 7.0978271289338e+02 */
    static const unsigned long int threshold_2[2] =
        {0xC086232BL, 0xDD7ABCD2L};  /* -7.0839641853226e-02 */
    static const unsigned long int inv_l[2] = {0x40471547L, 0x652B82FEL};
        /* 4.6166241308447e+01 */
    static const unsigned long int l1[2]    = {0x3F962E42L, 0xFEF00000L};
        /* 2.1660849390173e-02 */
    static const unsigned long int l2[2]    = {0x3D8473DEL, 0x6AF278EDL};
        /* 2.3251928468789e-12 */
    static const unsigned long int a1[2]    = {0x3FE00000L, 0x00000000L};
    static const unsigned long int a2[2]    = {0x3FC55555L, 0x55555555L};
    static const unsigned long int a3[2]    = {0x3FA55555L, 0x55555555L, };
    static const unsigned long int a4[2]    = {0x3F811111L, 0x11111111L, };
    static const unsigned long int a5[2]    = {0x3F56C16CL, 0x16C16C17L, };
    static const unsigned long int a6[2]    = {0x3F2a01a0L, 0x1a01a01aL, };
    static const unsigned long int a7[2]    = {0x3EFa01a0L, 0x1a01a01aL, };
    static const unsigned long int a8[2]    = {0x3ec71de3L, 0xa556c734L, };
#else
    static const unsigned long int a1[2]    = {0x00000000L, 0x3FE00000L};
    static const unsigned long int a2[2]    = {0x55555555L, 0x3FC55555L};
    static const unsigned long int a3[2]    = {0x55555555L, 0x3FA55555L};
    static const unsigned long int a4[2]    = {0x11111111L, 0x3F811111L};
    static const unsigned long int a5[2]    = {0x16C16C17L, 0x3F56C16CL};
    static const unsigned long int a6[2]    = {0x1a01a01aL, 0x3F2a01a0L};
    static const unsigned long int a7[2]    = {0x1a01a01aL, 0x3EFa01a0L};
    static const unsigned long int a8[2]    = {0xa556c734L, 0x3ec71de3L};
    static const unsigned long int inv_l[2] = {0x652B82FEL, 0x40471547L};
    static const unsigned long int l1[2]    = {0xFEF00000L, 0x3F962E42L};
    static const unsigned long int l2[2]    = {0x6AF278EDL, 0x3D8473DEL};
    static const unsigned long int threshold_1[2] = {0xFEFA39EFL, 0x40862E42L};
    static const unsigned long int threshold_2[2] = {0xDD7ABCD2L, 0xC086232BL};
#endif

#ifdef IEEE
    long int x_exp;
#endif
    int j;
    long int n, ax, m;
    double dn, pp1, pp2, rr, r1, r2, s, temp;
    volatile double s_lead, s_trail, nm, pp;

/*******************************
**  Global variables          **
*******************************/
    extern const unsigned long int _Ldexpfh[][2];
    extern const unsigned long int _Ldexpfl[][2];

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

    volatile double x = param;

#ifdef IEEE
    /*
    ** Extract the exponent
    */
    x_exp = MSH(x) & 0x7FF00000L;
    /*
    ** If the exponent is 0x7FF, the value is either an infinity or NaN
    */
    if ( x_exp == 0x7FF00000L ) {
      /*
      ** If the fraction bits are all zero, this is an infinity
      */
      if ( ( ( MSH(x) & 0x000FFFFFL ) | LSH(x) ) == 0L ) {
        /*
        ** For negative infinity, return zero
        */
        if ( ( MSH(x) & 0x80000000L ) != 0x0L )
          return(0.0);
        /*
        ** For positive infinity, signal infinity
        */
        else
          return(qtc_error(DEXP_INFINITY,x));
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
          return(qtc_error(DEXP_INVALID_OPERATION,x));
        /*
        ** For quiet NaNs, call the error code with the qNaN signal
        */
        else
          return(qtc_error(DEXP_QNAN,x));
      }
    }
    else
      /*
      ** Check for a denormalized value.  For this case, any exponent
      ** of zero (which signals either +/- 0 or a denormalized value)
      ** will result in a return value of 0.0
      */
      if ( x_exp == 0L )
        return(0.0);
  
#endif /* end of IEEE special case handling */

    /*
    ** Exceptional cases (here handle under, overflow cases)
    */
    if ( ( x > DOUBLE(threshold_1) ) || ( x < DOUBLE(threshold_2) ) )
#ifdef IEEE
      return(qtc_error(DEXP_OUT_OF_RANGE,x));
#else
      return(qtc_err(DEXP_OUT_OF_RANGE,x));
#endif
    ax = MSH(x) & 0x7FFFFFFFL;      /* ax = fabs(x) */
    if ( ax < 0x3C900000L)           /* threshold_3 = 2**(-54) */
        return (x);                  /*             = 5.55e-17 */
    /*
    ** Argument reduction: x = (32*m + j)*log2/32 + (r1 + r2)
    ** (inv_l = 32/log2)
    ** (n = intrnd(x*inv_l))
    */
    if ( ( MSH(x) & 0x80000000L) != 0x0L )
        n = (int)(x*DOUBLE(inv_l) - 0.5);
    else
        n = (int)(x*DOUBLE(inv_l) + 0.5);
    if ( ( n > -4 ) & ( n < 4 ) ) n = 0;
    j = n & 0x1F;
    /*
    ** Get table values for 2**(j/32) = s_lead + s_trail
    */
    LSH(s_lead) = _Ldexpfh[j][0];
    MSH(s_lead) = _Ldexpfh[j][1];
    LSH(s_trail) = _Ldexpfl[j][0];
    MSH(s_trail) = _Ldexpfl[j][1];
    /*
    ** Polynomial approximation of exp(rr) - 1:
    **  pp =  r1 + (r2 + rr*rr*(a1 + rr*(a2 + rr*(a3 + rr*(a4 + rr*a5)))))
    ** Reconstruction of exp(x):
    **           exp(x)-1 = 2**m * (2**(j/32) + 2**(j/32)*pp) - 1
    **                    = 2**m * 2**(j/32) + 2**m*2(j/32)*pp - 2**m/2**m
    */
    m = ((n - j) & 0xFFFFFFE0L) << 15; /* m = 2**m (unbaised), m=(n-j)/32 */
    LSH(nm) = 0;
    MSH(nm) = 0x3FF00000L - m;  /* nm = 2**-m, =bias-m, bias=1023 */

    dn = (double)n;
    r1 = x - dn*DOUBLE(l1);
    r2 = dn*DOUBLE(l2);
    rr = r1 - r2;
    pp=(((((((DOUBLE(a8)*rr+DOUBLE(a7))*rr+DOUBLE(a6))*rr+DOUBLE(a5))*rr+
              DOUBLE(a4))*rr+DOUBLE(a3))*rr+DOUBLE(a2))*rr+DOUBLE(a1))*rr*rr;
    pp = pp - r2;
    pp = pp + r1;
/*
    pp1 = pp * s_trail + s_trail;
    pp = (pp * s_lead) + pp1;
*/
    pp = pp * (s_lead + s_trail) + s_trail;
    if ( n < 0 ) {
        if ( n < -5*32 ) {
            pp += s_lead;
            MSH(pp) += m;
            pp -= 1.0;
        }
        else {
            MSH(s_lead) += m;
            s_lead -= 1.0;
            MSH(pp) += m;
            pp += s_lead;
        }
    }
    else {
        if ( n > 50*32 ) {
            pp -= nm;
            pp += s_lead;
        }
        else {
            s_lead -= nm;
            pp += s_lead;
        }
        MSH(pp) += m;
    }
/*
    pp1 = s_lead - nm;
    pp2 = pp1 + nm;
    pp2 -= s_lead;
    pp += pp2;
    pp += pp1;
*/
    return (pp);
}
/*** end of qtc_dexpm1.c ***/
