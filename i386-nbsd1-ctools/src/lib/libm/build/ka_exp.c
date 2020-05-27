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

/*** qtc_dexp.c ***/

#include "qtc_intr.h"
#include <qtc.h>
#include <math.h>

/*
******************************************************************************
**
*/
FUNCTION double qtc_dexp(param)
/*
**  Purpose: This is a table-based natural exponential routine
**
**           Accuracy - less than 1 ULP error
**           Valid input range - -708.3964 < x < 709.7827
**
**           Special case handling - non-IEEE:
**
**             qtc_dexp(+OOR) - calls qtc_err() with DEXP_OUT_OF_RANGE,
**                              input value
**             qtc_dexp(-OOR) - calls qtc_err() with DEXP_OUT_OF_RANGE,
**                              input value
**
**           Special case handling - IEEE:
**
**             qtc_dexp(+INF) - calls qtc_error() with DEXP_INFINITY,
**                              input value
**             qtc_dexp(-INF) - returns 0.0
**             qtc_dexp(QNAN) - calls qtc_error() with DEXP_QNAN, input value
**             qtc_dexp(SNAN) - calls qtc_error() with DEXP_INVALID_OPERATION,
**                              input value
**             qtc_dexp(DEN)  - returns 1.0 + x
**             qtc_dexp(+-0)  - returns 1.0
**             qtc_dexp(+OOR) - calls qtc_error() with DEXP_OUT_OF_RANGE,
**                              input value
**             qtc_dexp(-OOR) - calls qtc_error() with DEXP_OUT_OF_RANGE,
**                              input value
**
**  Method:
**
**        This function is based on the formula
**
**          if x = (32*m + j)*log2/32 + rr,
**
**          exp(x) = exp((32*m+j)*log2/32+rr)
**                 = exp((32*m+j)*log2/32) * exp(rr)
**                 = 2**m * 2**(j/32) * ( 1 + rr + rr**2/2! + rr**3/3! + ... )
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
**  Return value: returns the double-precision natural exponential of the
**                input argument
**
**  Called by:  any
** 
**  History:
**
**        Version 1.0 - 8 July 1988, J F Kauth and L A Westerman
**                1.1 - 18 July 1988, L A Westerman, cleaned up
**                      variable definitions
**                1.2 - 31 October 1988, L A Westerman
**                1.3 -  7 December 1988, J F Kauth, modified to
**                       use tables with trailing zeros on dexpfh,
**                       so that exp and expm1 can share tables
**                1.4 - 13 February 1989, L A Westerman, corrected
**                      minor error in documentation
**                1.5 - 20 February 1989, L A Westerman, added
**                      extra static double to force double-word
**                      alignment
**                1.6 - 26 April 1989, L A Westerman, deleted one term
**                      from the expansion of pp
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
**  Force double-word alignment
*/
    static const double dummy = 0.0;
    volatile double x = param;

#ifdef MSH_FIRST
    static const unsigned long int a1[2]    = {0x3FE00000L, 0x00000000L};
        /* 5.0000000000000e-01 */
    static const unsigned long int a2[2]    = {0x3FC55555L, 0x55555555L};
        /* 1.6666666666526e-01 */
    static const unsigned long int a3[2]    = {0x3FA55555L, 0x55555555L};
        /* 4.1666666666226e-02 */
    static const unsigned long int a4[2]    = {0x3F811111L, 0x11111111L};
        /* 8.3333679843422e-03 */
    static const unsigned long int a5[2]    = {0x3F56C16CL, 0x16C16C17L};
        /* 1.3888949086378e-03 */
    static const unsigned long int inv_l[2] = {0x40471547L, 0x652B82FEL};
        /* 4.6166241308447e+01 */
    static const unsigned long int l1[2]    = {0x3F962E42L, 0xFEF00000L};
        /* 2.1660849390173e-02 */
    static const unsigned long int l2[2]    = {0x3D8473DEL, 0x6AF278EDL};
        /* 2.3251928468789e-12 */
    static const unsigned long int threshold_1[2] =
        {0x40862E42L, 0xFEFA39EFL};  /* 7.0978271289338e+02 */
    static const unsigned long int threshold_2[2] =
        {0xC086232BL, 0xDD7ABCD2L};  /* -7.0839641853226e-02 */
#else
    static const unsigned long int a1[2]    = {0x00000000L, 0x3FE00000L};
        /* 5.0000000000000e-01 */
    static const unsigned long int a2[2]    = {0x55555555L, 0x3FC55555L};
        /* 1.6666666666526e-01 */
    static const unsigned long int a3[2]    = {0x55555555L, 0x3FA55555L};
        /* 4.1666666666226e-02 */
    static const unsigned long int a4[2]    = {0x11111111L, 0x3F811111L};
        /* 8.3333679843422e-03 */
    static const unsigned long int a5[2]    = {0x16C16C17L, 0x3F56C16CL};
        /* 1.3888949086378e-03 */
    static const unsigned long int inv_l[2] = {0x652B82FEL, 0x40471547L};
        /* 4.6166241308447e+01 */
    static const unsigned long int l1[2]    = {0xFEF00000L, 0x3F962E42L};
        /* 2.1660849390173e-02 */
    static const unsigned long int l2[2]    = {0x6AF278EDL, 0x3D8473DEL};
        /* 2.3251928468789e-12 */
    static const unsigned long int threshold_1[2] =
        {0xFEFA39EFL, 0x40862E42L};  /* 7.0978271289338e+02 */
    static const unsigned long int threshold_2[2] =
        {0xDD7ABCD2L, 0xC086232BL};  /* -7.0839641853226e-02 */
#endif

#ifdef IEEE
    long int x_exp;
#endif
    int j;
    long int n, ax;
    double dn, rr, r1, r2;
    volatile double s_lead, s_trail, pp;

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
        if ( MSH(x) >= (unsigned long int) 0x80000000L ) 
          return (0.0);
        /*
        ** For positive infinity, signal infinity
        */
        else
          return (qtc_error(DEXP_INFINITY,x));
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
          return (qtc_error(DEXP_INVALID_OPERATION,x));
        /*
        ** For quiet NaNs, call the error code with the qNaN signal
        */
        else
          return (qtc_error(DEXP_QNAN,x));
      }
    }
    else
      /*
      ** Check for a denormalized value.  For this case, any exponent
      ** of zero (which signals either +/- 0 or a denormalized value)
      ** will result in a return value of 1.0
      */
      if ( x_exp == 0L )
        return (1.0);
  
#endif /* end of IEEE special case handling */

    /*
    ** Exceptional cases (overflow and underflow)
    */
    if ( ( x > DOUBLE(threshold_1) ) || ( x < DOUBLE(threshold_2) ) )
#ifdef IEEE
      return (qtc_error(DEXP_OUT_OF_RANGE,x));
#else
      return (qtc_err(DEXP_OUT_OF_RANGE,x));
#endif
    ax = MSH(x) & 0x7FFFFFFFL;      /* ax = fabs(x) */
    if ( ax < 0x3C900000L)          /* threshold_3 = 2**(-54) */
        return (1.0 + x);           /*             = 5.55e-17 */
    /*
    ** Argument reduction: x = (32*m + j)*log2/32 + (r1 + r2)
    ** (inv_l = 32/log2)
    ** (n = intrnd(x*inv_l))
    */
    if ( MSH(x) >= (unsigned long int) 0x80000000L)
        n = (int)(x*DOUBLE(inv_l) - 0.5);   /* x < 0.0 */
    else
        n = (int)(x*DOUBLE(inv_l) + 0.5);   /* x > 0.0 */
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
    **  pp =  r1 + (r2+rr*rr*(1/2!+rr*(1/3!+rr*(1/4!+rr*(1/5!+rr/6!)))))
    ** Reconstruction of exp(x):
    **           exp(x) = 2**m * (2**(j/32) + 2**(j/32)*pp)
    */
    dn = (double)n;
    r1 = x - dn*DOUBLE(l1);
    r2 = dn*DOUBLE(l2);
    rr = r1 - r2;
    pp = ((((DOUBLE(a5)*rr + DOUBLE(a4))*rr + DOUBLE(a3))*rr +
             DOUBLE(a2))*rr + DOUBLE(a1))*rr*rr - r2;
    pp = (pp + r1)*s_lead + s_trail;
    pp += s_lead;
    MSH(pp) += ((n - j) & 0xFFFFFFE0L) << 15; /* pp = pp*2**m, m=(n-j)/32 */
    return (pp);

}   /* end of qtc_dexp() routine */

/*** end of qtc_dexp.c ***/
