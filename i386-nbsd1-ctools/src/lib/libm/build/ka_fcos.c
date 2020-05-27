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

/*** qtc_fcos.c ***/

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
FUNCTION float qtc_fcos(float param)
/*
**  Purpose: This is a table-based cosine routine.
**
**        Accuracy - less than 1 ULP error
**        Valid input range - |x| < 1.3176793e+07 ( (2**22 - 1) * pi )
**    
**        Special case handling - non-IEEE:
**    
**          qtc_fcos(+OOR) - calls qtc_ferr() with FCOS_OUT_OF_RANGE,
**                           input value
**          qtc_fcos(-OOR) - calls qtc_ferr() with FCOS_OUT_OF_RANGE,
**                           input value
**    
**        Special case handling - IEEE:
**    
**          qtc_fcos(+INF) - calls qtc_ferror() with FCOS_INFINITY, input value
**          qtc_fcos(-INF) - calls qtc_ferror() with FCOS_INFINITY, input value
**          qtc_fcos(QNAN) - calls qtc_ferror() with FCOS_QNAN, input value
**          qtc_fcos(SNAN) - calls qtc_ferror() with FCOS_INVALID_OPERATION,
**                           input value
**          qtc_fcos(DEN)  - returns correct value
**          qtc_fcos(+-0)  - returns +/-0.0
**          qtc_fcos(+OOR) - calls qtc_ferror() with FCOS_OUT_OF_RANGE,
**                           input value
**          qtc_fcos(-OOR) - calls qtc_ferror() with FCOS_OUT_OF_RANGE,
**                           input value
**    
**  Method:
**
**         This routine is based on the formula
**
**           cos(x0+rr.f) = cos(x0)*cos(rr.f) - sin(rr.f)*sin(x0)
**                     = cos(x0) * ( 1 - rr**2/2! + rr**4/4! - ... )
**                       - sin(x0) * ( rr - rr**3/3! + rr**5/5! - ... )
**
**  Notes:
**
**         When compiled with the -DIEEE switch, this routine properly
**         handles infinities, NaNs, signed zeros, and denormalized
**         numbers.
**
**
**  Input Parameter:
*/
/*    float x; value for which the cosine is desired */
                  /* The value is in radians                        */
/*
**  Output Parameters: none
**
**  Return value: returns the single-precision cosine of the
**                input argument
**
**  Called by:  any
** 
**  History:
**
**        Version 1.0 - 8 July 1988, J F Kauth and L A Westerman
**                1.1 - 18 November 1988, L A Westerman, cut table size in half
**                1.2 - 6 January 1989, L A Westerman, cleaned up for release
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
    long_float s_lead;
    long_float c_lead;
    long_float c_trail;
    long_float temp_lf;

    long_float halfpi1;
    long_float inv_p;
    long_float inv_pi;
    long_float inv_4fact;
    long_float inv_5fact;
    long_float ninv_2;
    long_float ninv_3fact;
    long_float ninv_6fact;
    long_float ninv_7fact;
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
    long_float pi5;
    long_float spi2_2;
    long_float spi3_2;
    long_float spi4;
    long_float spi4_2;
    long_float threshold_1;
    long_float threshold_2;
    long_float rr;
#ifdef IEEE
    long_float fone;
    long int exp;
    float dummy;
#endif
    static float temp_return;
    long int  an2, m, n, nn, n0, n1, n2;
    float cr, fn, fnn, fn0, fn1, fn2; 
    float pp, r1, r2, rx, r_sav, sr, temp, z;
/*  float x; */

/*******************************
**  Global data definitions   **
*******************************/
    extern unsigned long int fsinfh[];
    extern unsigned long int fsinfl[];

/*******************************
** External functions         **
*******************************/
#ifdef IEEE
    /* extern float qtc_ferror(); */
#else
    extern float *qtc_ferr();
#endif

    x.f = param;

    halfpi1.i      = 0x3FC91000L;  /* 1.57080078e+00 */
    inv_p.i        = 0x42A2F983L;  /* 8.14873276e+01 */
    inv_pi.i       = 0x3EA2F983L;  /* 3.18309873e-01 */
    inv_4fact.i    = 0x3D2AAAABL;  /* 4.16666667e-02 */
    inv_5fact.i    = 0x3C088889L;  /* 8.33333333e-03 */
    ninv_2.i       = 0xBF000000L;  /*-5.00000000e-01 */
    ninv_3fact.i   = 0xBE2AAAABL;  /*-1.66666667e-01 */
    ninv_6fact.i   = 0xBAB60B61L;  /*-1.38888889e-03 */
    ninv_7fact.i   = 0xB9500D01L;  /*-1.98412698e-04 */
    p1.i           = 0x3C491000L;  /* 1.22718811e-02 */
    p2.i           = 0xB315777AL;  /*-3.48004292e-08 */
    p2_2.i         = 0xB3158000L;  /*-3.48081812e-08 */
    p3.i           = 0x2D085A31L;  /* 7.75073148e-12 */
    p3_2.i         = 0x2D088000L;  /* 7.75912667e-12 */
    p4.i           = 0xA8173DCBL;  /*-8.39558305e-15 */
    pi1.i          = 0x40491000L;  /* 3.14160156e+00 */
    pi2.i          = 0xB715777AL;  /*-8.90890988e-06 */
    pi2_2.i        = 0xB7158000L;  /*-8.91089439e-06 */
    pi3_2.i        = 0x31088000L;  /* 1.98633643e-09 */
    pi4.i          = 0xAC173DCBL;  /*-2.14926926e-12 */
    pi5.i          = 0xA7772CEDL;  /*-3.43024902e-15 */
    spi2_2.i       = 0xB7150000L;  /*-8.88109207e-06 */
    spi3_2.i       = 0xB2EF0000L;  /*-2.78232619e-08 */
    spi4.i         = 0x2CB4611AL;  /* 5.12668813e-12 */
    spi4_2.i       = 0x2CB48000L;  /* 5.13011855e-12 */
    threshold_1.i  = 0x4B490FDAL;  /* 1.31767940e+07 */
    threshold_2.i  = 0x47490FDBL;  /* 5.01471855e+04 */

    fone.i         = 0x3F800000L;  /* 1.00000000e-00 */

/*******************************
** Function body              **
*******************************/
/*    x.i = *x_ptr.i; */
/*
** Argument reduction for this function proceeds by subtracting
** a multiple of pi/256 from the input argument.  The size of the
** multiple is determined by multiplying x by inv_p, which is 256/pi.
**
** This value is reduced to an integer, then that multiple of
** pi/256 is subtracted from the input argument.  This subtraction
** proceeds in several steps, using two or three values which
** separately add up to a highly-precise representation of pi/256.
**
** pi/256 = p1 + p2 ( + error )
**        = p1 + p2_2 + p3 ( + error )
**
** The values p1 and p2_2 have at least 11 trailing zero bits, 
** since the argument reduction involves multiplying these values by
** (potentially large) integers, and subtracting from x.  For these
** multiplications to be exact, the size of the integer and the
** number of trailing zeros must be matched.  For large values of x,
** the argument reduction utilizes the extended p1,p2_2,p3
** representation of pi/256.
**
** The error in argument reduction can be expressed as:
**
**   error = n * abs(p1 + p2_2 + ... + pm - pi/256)
**          + n * abs(pm) * 2**-24
**
** since the size of the last bit in pm is 2**-23 times pm.
**
** abs(p2) < 2**(-25)   abs(p1+p2-pi/256)                < 2**(-50)
** abs(p3) < 2**(-37)   abs(p1+p2_2+p3-pi/256)           < 2**(-62)
** abs(p4) < 2**(-47)   abs(p1+p2_2+p3_2+p4-pi/256)      < 2**(-73)
** abs(p5) < 2**(-57)   abs(p1+p2_2+p3_2+p4_2+p5-pi/256) < 2**(-84)
**
** If x <= threshold_1, n will fit in a 32 bit integer.
** threshold_1 is (2**22)*pi - 1 ulp or 4194304 * pi.
**
** If x <= threshold_2, the integer portion and the first fraction bit
** of x*256/pi is representable as a single precision number.
** threshold_2 is 2**14 * pi or 16384 * pi.
**
** pi = pi1 + pi2
**    = pi1 + pi2_2 + pi3
** pi1 = 256*p1; pi2 = 256*p2; pi2_2 = 256*p2_2; pi3 = 256*p3
**
** Spi2_2, spi3_2, have one more trailing zero than pi2_2 and pi3_2, 
** respectively.
** They are used for the case of x near a large odd multiple of pi/2
** to ensure that (n1+0.5)*spi2_2 and (n1+0.5)*spi3_2 are exact:
**
**   pi = pi1 + spi2_2 + spi3_2 + spi4
**   pi = pi1 + spi2_2 + spi3_2 + spi4_2 + pi5
*****/

/*****
** Start with special-case handling
*****/

#ifdef IEEE

/*****
** Extract the exponent
*****/
   exp = x.i & 0x7F800000L;
/*****
** If the exponent is 0x7F8, the value is either an infinity or NaN
*****/
   if ( exp == 0x7F800000L ) {
   /*****
   ** If the fraction bits are all zero, this is an infinity
   *****/
      if ( ( x.i & 0x007FFFFFL ) == 0L ) {
      /*****
      ** For infinity, call the error handler with the infinity signal
      *****/
         return (qtc_ferror(FCOS_INFINITY,x.f,dummy));
      }

   /*****
   ** If some fraction bits are non-zero, this is a NaN
   *****/
      else {
      /*****
      ** For signaling NaNs, call the error handler with the invalid
      ** operation signal
      *****/
         if ( is_fsNaN(x) )
            return (qtc_ferror(FCOS_INVALID_OPERATION,x.f,dummy));
         /*****
         ** For quiet NaNs, call the error code with the qNaN signal
         *****/
         else
            return (qtc_ferror(FCOS_QNAN,x.f,dummy));
      }
   }
   else {
   /*****
   ** Check for a denormalized value.  At this point, any exponent
   ** of zero signals either a zero or a denormalized value.
   ** In either case, return 1.0.
   *****/
      if ( exp == 0x0L ) {
         return (fone.f);
      }
   }
#endif /* end of IEEE special case handling */

/*****
** Use absolute value of x, since cos(x) = cos(-x)
*****/
   x.i &= 0x7FFFFFFFL;                /*    x = -x  */

/*****
** Exceptional cases 
*****/
   if ( x.i > threshold_1.i ) /* Above this n, n1, or n2 won't fit in int */
#ifdef IEEE
      return (qtc_ferror(FCOS_OUT_OF_RANGE,x.f,dummy));
#else
      return (qtc_ferr(FCOS_OUT_OF_RANGE,x));
#endif

/*****
** Argument reduction: |x| = (256*m + n2)*pi/256 + rr
**
** If x < threshold_2, the integer portion and the first fraction
** bit of x*256/pi is representable in a sp floating point number
*****/
   if ( x.i <= threshold_2.i ) {
      temp = x.f * inv_p.f;
      temp += 0.5;
      n = (int) temp;     /* n = intrnd(x*inv_p.f) */
      n2 = n & 0xFFL;     /* n2 has 8 bits max */
      if ( n2 > 147 )
         n2 -= 256;     /* -109 < n2 <= 147 */
      n1 = n - n2;
      m = n1 >> 8;
      an2 = ABS(n2);
   /*****
   ** Check for an expansion about zero
   *****/
      if ( an2 <= 17 ) {

      /*****
      ** Expansion about zero:
      ** Use polynomial expansion about zero for small x:
      **  pp = 1 - rr**2/2! + rr**4/4! - rr**6/6! + ...
      ** where rr is the distance from 0.0
      ** 
      ** Note that if n > 2**20, n1 will have more than 11 bits.
      ** In this case n1 is divided into n0 and n1.
      **
      ** Note that it is important to perform these calculations in the
      ** order shown, since other orderings may cause the loss of 
      ** significant bits.  Theses losses may only show up in isolated 
      ** cases.
      *****/
         if ( n < 0x80000L ) {
            fn = (float)n1;
            temp = fn*p1.f;
            rr.f = x.f - temp;
            temp = fn*p2.f;
            rr.f -= temp;
         }
         else {
            nn = n1;
            n1 = nn & 0x7FFFL;
            n0 = nn - n1;
            fn0 = (float)n0;
            fn1 = (float)n1;
            temp = fn0*p1.f;
            rr.f = x.f - temp;
            temp = fn1*p1.f;
            rr.f -= temp;
            temp = fn0*p2.f;
            rr.f -= temp;
            temp = fn1*p2.f;
            rr.f -= temp;
         }
         z = rr.f*rr.f;
         if ( an2 > 5 ) {         /* |n2| = 6,17  */
            pp = ninv_6fact.f * z;
            pp += inv_4fact.f;
            pp *= z;
            pp += ninv_2.f;
            pp *= z;
         }
         else
         if ( an2 > 0 ) {            /* |n2| = 1,5  */
            pp = inv_4fact.f * z;
            pp += ninv_2.f;
            pp *= z;

         }
         else {                      /* |n2| = 0  */
            pp = ninv_2.f * z;
         }
         pp += 1.0;
      }
   /*****
   ** Check for expansion about a multiple of pi/2
   *****/
      else if ( an2 > 109 ) {
      /*****
      ** For expansion about a multiple of pi/2, the following considerations
      ** are important.
      **
      ** As above, the required smallest term for argument reduction is
      ** the p3 term, since the smallest rr for this case is given by
      ** a value of x which is almost exactly equal to pi/2.  The minimum
      ** value is actually given by the single precision value one ulp
      ** less than pi/2, which yields an rr of ~ 2**-25.
      **
      ** Now, to determine the calculation sequence for the sine series
      ** expansion about pi/2, the following table is useful
      **
      **    j        rr   rr**3/3!   rr**5/r!   rr**7/7!   rr**9/9!
      **  118,138   2**-3  2**-11     2**-21     2**-33     2**-45
      **  108,117   2**-2  2**-8      2**-16     2**-26     2**-36
      **  139,148
      **
      ** Thus, for the range 118 <= j <= 138, use 3 terms
      **                     109 <= j <= 117, use 4 terms
      **                     139 <= j <= 147
      **
      ** The table expansion method uses 9 operations plus 2 memory
      ** references, so that 4 terms in the expansion (which uses 7
      ** operations) is more efficient.
      **
      *****/
         m++;
         an2 = ABS(an2-128);
         if ( n < 0x7FF00L ) {
            fn = (float)(n1+128);
            temp = fn*p1.f;
            r1 = x.f - temp;
            temp = fn*p2_2.f;
            r1 -= temp;
            r2 = fn*p3.f;
            rr.f = r1 - r2;
            /*
            ** The following logic is to handle cases where 
            ** x = SP(k*pi) and SP(k*pi) is very close to k*pi
            ** (that is, potential "bad points").
            **
            ** The p4 term is used if |rr| < 2**(-15).
            */
            if ( (an2 == 0) && ((0x7FFFFFFFL & rr.i) < 0x38000000L) ) {
               temp = fn*p3_2.f;
               r1 -= temp;
               r2 = fn*p4.f;
               rr.f = r1 - r2;
            }
         }
         else {
            nn = n1 + 128;
            n1 = nn & 0x7FFFL;
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
            rr.f = r1 - r2;
            /*
            ** The following logic is to handle cases where 
            ** x = SP(k*pi) and SP(k*pi) is very close to k*pi
            ** (that is, potential "bad points").
            **
            ** The p4 term is used if |rr| < 2**(-12).
            */
            if ( (an2 == 0) && ((0x7FFFFFFFL & rr.i) < 0x39800000L) ) {
               temp = fn0 * p3_2.f;
               r1 -= temp;
               temp = fn1 * p3_2.f;
               r1 -= temp;
               r2 = fnn * p4.f;
               rr.f = r1 - r2;
            }
         }
         z = rr.f*rr.f;
         if ( an2 > 10 ) {      /* 11 to 19, 4 terms */
            pp = ninv_7fact.f*z;
            pp += inv_5fact.f;
            pp *= z;
            pp += ninv_3fact.f;
            pp *= z;
         }
         else {
            pp = inv_5fact.f*z;
            pp += ninv_3fact.f;
            pp *= z;
         }
         pp *= rr.f;
         pp -= r2;
         pp += r1;
      }
      else {
      /*****
      ** Expansion about a table value:
      ** The dominant term in the result is cos(x0), where
      ** x0 = n2*pi/256.  The minimum cos(x0) is cos(174*pi/256)
      ** = 2**-2.
      ** Therefore an acceptable error in rr is 2**(-2-24-3)
      ** = 2**-29.  To attain this, use the p2 term in argument
      ** reduction for n < 2**20, and the p3 term for larger n.
      **
      ** If n < 2**12, n*p1 is exactly representable
      ** and rr = (x - n*p1) - n*p2,
      *****/
         if (n < 0x800L) {
            fn = (float)n;
            temp = fn*p1.f;
            rr.f = x.f - temp;
            temp = fn*p2.f;
            rr.f -= temp;
         }
      /*****
      ** Else if n < 2**20, n1*p1 and n2*p1 are 
      ** exactly representable and rr = ((x - n1*p1) - n2*p1) - n*p2
      *****/
         else
         if ( n < 0x80000L ) {
            fn1 = (float)n1;
            temp = fn1*p1.f;
            rr.f = x.f - temp;
            fn2 = (float)n2;
            temp = fn2*p1.f;
            rr.f -= temp;
            fn = (float)n;
            temp = fn*p2.f;
            rr.f -= temp;
         }
      /*****
      ** Else n < 2**23 (since x < threshold_2),
      ** n0*p1, n1*p1, n2*p1, n0*p2_2,
      ** n1*p2_2, and n2*p2_2 are exactly representable and 
      ** rr.f = ((((((x - n0*p1) - n1*p1) - n0*p2_2) - n2*p1)
      **             - n1*p2_2) - n*p3) - n2*p2_2
      *****/
         else {
            nn = n1;
            n1 = nn & 0x7FFFL;
            n0 = nn - n1;
            fn0 = (float)n0;
            fn1 = (float)n1;
            temp = fn0*p1.f;
            rr.f = x.f - temp;
            temp = fn1*p1.f;
            rr.f -= temp;
            temp = fn0*p2_2.f;
            rr.f -= temp;
            fn2 = (float)n2;
            temp = fn2*p1.f;
            rr.f -= temp;
            temp = fn1*p2_2.f;
            rr.f -= temp;
            fn = (float)n;
            temp = fn*p3.f;
            rr.f -= temp;
            temp = fn2*p2_2.f;
            rr.f -= temp;
         }
      /*****
      ** If j < 0, fold into quadrant I (j > 0) for table lookup
      ** and increment m to change the sign of the result
      *****/
         if (n2 < 0) {
            n2 = -n2;
            rr.i ^= 0x80000000L;    /* rr = -rr */
         }
      /*****
      ** Get table values for sine and cosine
      *****/
         s_lead.i = fsinfh[n2];
         n2 = 128 - n2;
         c_lead.i = fsinfh[n2];
         c_trail.i = fsinfl[n2];
   
      /*****
      ** Polynomial approximation of cos(x0+rr) 
      ** cos(x0+rr) = cos(x0)*cos(rr.f) + sin(x0)*sin(rr.f)
      **           = cos(x0)*(1 - rr**2/2!) -
      **             sin(x0)*(rr - rr**3/3!)
      ** where x0 = j*pi/256
      *****/
         z = rr.f*rr.f;
         cr = -0.5*z;
         pp = c_lead.f*cr;
         pp += c_trail.f;
         sr = z * ninv_3fact.f;
         sr *= rr.f;
         sr += rr.f;
         temp = sr*s_lead.f;
         pp -= temp;
         pp += c_lead.f;
      }
   }         /* end of x <= threshold_2 -if- */

   else {
/*****
** The integer part and first fraction bit of x*256/pi is not
** representable in a single_precision floating point number.  Therefore,
** determine n in two steps:  n0,n1 (each 11 bits or less) is
** the integer part of x*1/pi; n2 (9 bits or less) is the integer
** part of (x - (n0+n1)*pi)*256/pi.
** n is then (n0 + n1)*256 + n2.
*****/
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
   /*****
   ** Generally n2 will be in [-128,128].  It is necessary to
   ** use a range of values from -109 < n2 < 148, so the
   ** following logic takes care of values outside that range
   *****/
      if ( n2 < -108 ) {
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
      m = n1;
      an2 = ABS(n2);

      if ( an2 <= 17 ) {
      /***** 
      ** Use polynomial expansion about zero for small x:
      **  pp = 1 - rr**2/2! + rr**4/4! - rr**6/6! + ...
      ** where rr is the distance from 0.0
      *****/
         temp = fn0*pi2_2.f;
         rr.f = r_sav - temp;     /* r_sav =  x - 256*(n0+n1)*p1 */
         temp = fn1*pi2_2.f;
         r1 = rr.f - temp;
         temp = fn0*pi3_2.f;
         r1 -= temp;
         if ( an2 <= 3 ) {
            temp = fn1*pi3_2.f;
            r1 -= temp;
            r2 = fnn*pi4.f;
         }
         else {
            r2 = fn1*pi3_2.f;
            temp = fnn*pi4.f;
            r2 += temp;
         }
         rr.f = r1 - r2;
         z = rr.f*rr.f;
         if ( an2 > 5 ) {         /* |n2| = 6,17  */
            pp = ninv_6fact.f * z;
            pp += inv_4fact.f;
            pp *= z;
            pp += ninv_2.f;
            pp *= z;
         }
         else
         if ( an2 > 1 ) {            /* |n2| = 2,5  */
            pp = inv_4fact.f * z;
            pp += ninv_2.f;
            pp *= z;

         }
         else {                      /* |n2| = 0,1  */
            pp = ninv_2.f * z;
         }
         pp += 1.0;

      }
   /*****
   ** Check for expansion about a multiple of pi/2
   *****/
      else if ( an2 > 108 ) {
         temp = fn0*spi2_2.f;
         rr.f = r_sav - temp;     /* r_sav =  x - 256*(n0+n1)*p1 */
         m++;
         temp = halfpi1.f;
         r1 = rr.f - temp;
         fn1 += 0.5;   /* Add 1/2 to n1 and nn so that rr is the distance */
         fnn += 0.5;   /* from pi/2; now n1 has up to 12 bits             */
         temp = fn1*spi2_2.f;
         r1 -= temp;
         temp = fn0*spi3_2.f;
         r1 -= temp;
         an2 = ABS(an2-128);
         r2 = fnn * spi4.f;
         temp = fn1 * spi3_2.f;
         r2 += temp;
         rr.f = r1 - r2;
         /*
         ** The following logic is to handle cases where 
         ** x = SP(k*pi) and SP(k*pi) is very close to k*pi
         ** (that is, potential "bad points").
         **
         ** The p4 term is used if |rr.f| < 2**(-13).
         */
         if ( (an2 == 0) && ((0x7FFFFFFFL & rr.i) < 0x39000000L) ) {
            r1 -= temp;
            temp = fn0*spi4_2.f;
            r1 -= temp;
            r2 = fnn*pi5.f;
            temp = fn1*spi4_2.f;
            r2 += temp;
            rr.f = r1 - r2;
         }
         z = rr.f*rr.f;
         if ( an2 > 10 ) {      /* 11 to 19, 4 terms */
            pp = ninv_7fact.f*z;
            pp += inv_5fact.f;
            pp *= z;
            pp += ninv_3fact.f;
            pp *= z;
         }
         else {
            pp = inv_5fact.f*z;
            pp += ninv_3fact.f;
            pp *= z;
         }
         pp *= rr.f;
         pp -= r2;
         pp += r1;
      }
   /*****
   ** Expansion about a table value
   *****/
      else {
      /*****
      ** rr.f = ((((((x - n0*pi1) - n1*pi1) - n0*pi2_2) - n2*p1)
      **             - n1*pi2_2) - n*p3) - n2*p2_2
      *****/
         temp = fn0*pi2_2.f;
         rr.f = r_sav - temp;     /* r_sav =  x - 256*(n0+n1)*p1 */
         fn2 = (float)n2;
         temp = fn2*p1.f;
         rr.f -= temp;
         temp = fn1*pi2_2.f;
         rr.f -= temp;
         fn = (float)((nn << 8) + n2);  /* n = 256*nn + n2 */
         temp = fn*p3.f;
         rr.f -= temp;
         temp = fn2*p2_2.f;
         rr.f -= temp;
      /*****
      ** If j < 0, fold into quadrant I (j > 0) for table lookup
      ** and increment m to change the sign of the result
      *****/
         if ( n2 < 0 ) {
            n2 = -n2;
            rr.i ^= 0x80000000L;    /* rr = -rr */
         }
      /*****
      ** Get table values for sine and cosine
      *****/
         s_lead.i = fsinfh[n2];
         n2 = 128 - n2;
         c_lead.i = fsinfh[n2];
         c_trail.i = fsinfl[n2];
   
      /*****
      ** Polynomial approximation of cos(x0+rr) 
      ** cos(x0+rr) = cos(x0)*cos(rr.f) + sin(x0)*sin(rr.f)
      **           = cos(x0)*(1 - rr**2/2!) -
      **             sin(x0)*(rr - rr**3/3!)
      ** where x0 = j*pi/256
      *****/
         z = rr.f*rr.f;
         cr = -0.5*z;
         pp = c_lead.f*cr;
         pp += c_trail.f;
         sr = z * ninv_3fact.f;
         sr *= rr.f;
         sr += rr.f;
         temp = sr*s_lead.f;
         pp -= temp;
         pp += c_lead.f;
      }
   }      /* end of threshold_2 < x < threshold_1 -else- */
/*****
** Reconstruction of cos(x):
**           cos(x) = (-1)**m * pp
*****/
   if ( (m & 1) == 1 )
   {
      long_float lf;
      lf.f = pp;
      lf.i ^= 0x80000000L;
      pp = lf.f;
   }

   temp_return = pp;
   return (temp_return);

}   /* end of qtc_fcos() routine */

/*** end of qtc_fcos.c ***/
