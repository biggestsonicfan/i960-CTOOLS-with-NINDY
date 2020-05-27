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

/*** qtc_fexp.c ***/

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
FUNCTION float qtc_fexp(float param)
/*
**  Purpose: This is a table-based natural exponential routine
**
**           Accuracy - less than 1 ULP error
**           Valid input range - -87.3365 < x < 88.7228
**
**           Special case handling - non-IEEE:
**
**             qtc_fexp(+OOR) - calls qtc_ferr() with FEXP_OUT_OF_RANGE,
**                              input value
**             qtc_fexp(-OOR) - calls qtc_ferr() with FEXP_OUT_OF_RANGE,
**                              input value
**
**           Special case handling - IEEE:
**
**             qtc_fexp(+INF) - calls qtc_ferror() with FEXP_INFINITY,
**                              input value
**             qtc_fexp(-INF) - returns 0.0
**             qtc_fexp(QNAN) - calls qtc_ferror() with FEXP_QNAN, input value
**             qtc_fexp(SNAN) - calls qtc_ferror() with FEXP_INVALID_OPERATION,
**                              input value
**             qtc_fexp(DEN)  - returns 1.0 + x
**             qtc_fexp(+-0)  - returns 1.0
**             qtc_fexp(+OOR) - calls qtc_ferror() with FEXP_OUT_OF_RANGE,
**                              input value
**             qtc_fexp(-OOR) - calls qtc_ferror() with FEXP_OUT_OF_RANGE,
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
**
**  Input Parameters:
*/
/*    float x;  value for which the exponential is desired */
/*
**  Output Parameters: none
**
**  Return value: returns the single-precision natural
**                exponential of the input argument
**
**  Called by:  any
** 
**  History:
**
**        Version 1.0 - 8 July 1988, J F Kauth and L A Westerman
**                1.1 - 6 January 1989, L A Westerman, cleaned up for release
**                1.2 - 13 February 1989, L A Westerman, corrected minor error
**                      in code for small values of x
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
    long_float temp_lf;

    long_float a1;
    long_float a2;
    long_float inv_l;
    long_float l1;
    long_float l2;
    long_float threshold_1;
    long_float threshold_2;
    long_float s_lead;
    long_float s_trail;

#ifdef IEEE
    long_float fone;
    long_float fzero;
    long int x_exp;
    float dummy;
#endif

    long_float x;

    static float temp_return;
    int    j, n;
    float dn, pp, rr, r1, r2, temp;
    /* float x; */

/*******************************
**  Global variables          **
*******************************/
    extern unsigned long int fexpfh[];
    extern unsigned long int fexpfl[];

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
    a1.i    = 0x3F000000L;
    a2.i    = 0x3E2AAAABL;
    inv_l.i = 0x4238AA3BL;
    l1.i    = 0x3CB17200L;
    l2.i    = 0x333FBE8EL;
    threshold_1.i = 0x42B17218L;  /* 8.8722839052e+01 */
    threshold_2.i = 0xC2AEAC50L;  /*-8.7336544751e+01 */

#ifdef IEEE
    fone.i   = 0x3F800000L;
    fzero.i  = 0x00000000L;
#endif

    /* x.i = *(unsigned long int*)x_ptr; */
#ifdef IEEE
    /*
    ** Extract the exponent
    */
    x_exp = x.i & 0x7F800000L;
    /*
    ** If the exponent is 0x7F8, the value is either an infinity or NaN
    */
    if ( x_exp == 0x7F800000L ) {
      /*
      ** If the fraction bits are all zero, this is an infinity
      */
      if ( ( x.i & 0x007FFFFFL ) == 0L ) {
        /*
        ** For negative infinity, return zero
        */
        if ( x.i & 0x80000000L )
          return (fzero.f);
        /*
        ** For positive infinity, signal infinity
        */
        else
          return (qtc_ferror(FEXP_INFINITY,x.f,dummy));
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
          return (qtc_ferror(FEXP_INVALID_OPERATION,x.f,dummy));
        /*
        ** For quiet NaNs, call the error code with the qNaN signal
        */
        else
          return (qtc_ferror(FEXP_QNAN,x.f,dummy));
      }
    }
    else
      /*
      ** Check for a denormalized value.  For this case, any exponent
      ** of zero (which signals either +/- 0 or a denormalized value)
      ** will result in a return value of 1.0
      */
      if ( x_exp == 0L )
        return (fone.f);
  
#endif /* end of IEEE special case handling */

    /*
    ** Exceptional cases (here handle under, overflow cases)
    */
    if ( ( x.f > threshold_1.f ) || ( x.f < threshold_2.f ) )
#ifdef IEEE
      return (qtc_ferror(FEXP_OUT_OF_RANGE,x.f,dummy));
#else
      return (qtc_ferr(FEXP_OUT_OF_RANGE,x.f));
#endif
    temp_lf.i = x.i & 0x7FFFFFFFL; /* temp_return = fabs(x) */
    temp_return = temp_lf.f;
    if ( temp_lf.i < 0x38000000L ) {    /* threshold_3 = 2**(-15) */
        temp_return = 1.0 + x.f;
        return (temp_return);                 /*             = 3.05e-5  */
    }
    /*
    ** Argument reduction: x = (32*m + j)*log2/32 + (r1 + r2)
    ** (inv_l = 32/log2)
    ** (n = intrnd(x*inv_l))
    */
    temp = x.f * inv_l.f;
    if ( (x.i & 0x80000000L ) == 0) {
        temp += 0.5;
    }
    else {
        temp -= 0.5;
    }
    n = (int) temp;
    j = n & 0x1F;
    /*
    ** Get table values for 2**(j/32) = s_lead.f + s_trail.f
    */
    s_lead.i = fexpfh[j];
    s_trail.i = fexpfl[j];
    /*
    ** Polynomial approximation of exp(rr) - 1:
    **  pp =  r1 + (r2 + rr*rr*(a1 + rr*a2))
    ** Reconstruction of exp(x):
    **           exp(x) = 2**m * (2**(j/32) + 2**(j/32)*pp)
    */
    dn = (float)n;
    r2 = dn*l2.f;
    if (ABS(n) < 512) {
        temp = dn*l1.f;
        r1 = x.f - temp;
    }
    else {
        dn = (float)(n-j);
        temp = dn*l1.f;
        r1 = x.f - temp;
        dn = (float)j;
        temp = dn*l1.f;
        r1 -= temp;
    }
    rr = r1 - r2;
    pp = a2.f*rr;
    pp += a1.f;
    pp *= rr;
    pp *= rr;
    pp -= r2;
    pp += r1;
    temp = s_lead.f + s_trail.f;
    pp *= temp;
    pp += s_trail.f;
    pp += s_lead.f;
    temp_lf.f = pp;
    temp_lf.i += ((n - j) & 0xFFFFFFE0L) << 18; /* pp = pp*2**m, m=(n-j)/32 */
    pp += temp_lf.f;
    temp_return = pp;
    return (temp_return);
}
/*** end of qtc_fexp.c ***/
