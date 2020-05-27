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

/*** qtc_fsin.c ***/

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
FUNCTION float qtc_fsin(float param)
/*
**  Purpose: This is a table-based sine routine.
**
**        Accuracy - less than 1 ULP error
**        Valid input range - |x| < 1.3176793e+07 ( (2**22 - 1) * pi )
**    
**        Special case handling - non-IEEE:
**    
**          qtc_fsin(+OOR) - calls qtc_ferr() with FSIN_OUT_OF_RANGE,
**                           input value
**          qtc_fsin(-OOR) - calls qtc_ferr() with FSIN_OUT_OF_RANGE,
**                           input value
**    
**        Special case handling - IEEE:
**    
**          qtc_fsin(+INF) - calls qtc_ferror() with FSIN_INFINITY, input value
**          qtc_fsin(-INF) - calls qtc_ferror() with FSIN_INFINITY, input value
**          qtc_fsin(QNAN) - calls qtc_ferror() with FSIN_QNAN, input value
**          qtc_fsin(SNAN) - calls qtc_ferror() with FSIN_INVALID_OPERATION,
**                           input value
**          qtc_fsin(DEN)  - returns correct value
**          qtc_fsin(+-0)  - returns +/-0.0
**          qtc_fsin(+OOR) - calls qtc_ferror() with FSIN_OUT_OF_RANGE,
**                           input value
**          qtc_fsin(-OOR) - calls qtc_ferror() with FSIN_OUT_OF_RANGE,
**                           input value
**    
**  Method:
**
**         This routine is based on the formula
**
**           sin(x0+rr) = sin(x0)*cos(rr) + sin(rr)*cos(x0)
**                      = sin(x0) * ( 1 - rr**2/2! + rr**4/4! - ... )
**                       + cos(x0) * ( rr - rr**3/3! + rr**5/5! - ... )
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
/*    float x; pointer to value for which the sine is desired */
                  /* The value is in radians                        */
/*
**  Output Parameters: none
**
**  Return value: returns the single-precision sine of the
**                input argument
**
**  Called by:  any
** 
**  History:
**
**        Version 1.0 - 8 July 1988, J F Kauth and L A Westerman
**                1.1 - 7 November 1988, L A Westerman
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
    long_float temp_lf2;

    long_float inv_120;
    long_float inv_p;
    long_float inv_pi;
    long_float ninv_5040;
    long_float ninv_6;
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
    long_float threshold_1;
    long_float threshold_2;

#ifdef IEEE
    long int exp;
    float dummy;
#endif
    static float temp_return;

    long int  an2, m, n, nn, n0, n1, n2;
    float cr, fn, fnn, fn0, fn1, fn2, pp, r1, r2, rr;
    float rx, r_sav, sr, temp, z;

    long_float s_lead;
    long_float c_lead;
    long_float s_trail;
    /* float x;*/

/*******************************
**  Global data definitions   **
*******************************/
    extern unsigned long int fsinfh[];
    extern unsigned long int fsinfl[];

    long_float x;

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
    x.f = param;

    inv_120.i      = 0x3C088889L;  /* 8.33333333e-03 */
    inv_p.i        = 0x42A2F983L;  /* 8.14873276e+01 */
    inv_pi.i       = 0x3EA2F983L;  /* 3.18309873e-01 */
    ninv_5040.i    = 0xB9500D01L;  /*-1.98412698e-04 */
    ninv_6.i       = 0xBE2AAAABL;  /*-1.66666667e-01 */
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
    pi4_2.i        = 0xAC170000L;  /*-2.14503906e-12 */
    pi5.i          = 0xA7772CEDL;  /*-3.43024902e-15 */
    threshold_1.i  = 0x4B490FDAL;  /* 1.31767940e+07 */
    threshold_2.i  = 0x47490FDBL;  /* 5.01471855e+04 */

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
         return (qtc_ferror(FSIN_INFINITY,x.f,dummy));
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
            return (qtc_ferror(FSIN_INVALID_OPERATION,x.f,dummy));
         /*****
         ** For quiet NaNs, call the error code with the qNaN signal
         *****/
         else
            return (qtc_ferror(FSIN_QNAN,x.f,dummy));
      }
   }
   else {
   /*****
   ** Check for a denormalized value.  At this point, any exponent
   ** of zero signals either a zero or a denormalized value.
   ** In either case, return the input value.
   *****/
      if ( exp == 0L )
         return (x.f);
   }
#endif /* end of IEEE special case handling */

/*****
** Use absolute value of x.
** Since sin(x) = -sin(-x), if x negative, set m = 1 for proper sign
*****/
   if ( ( x.i & 0x80000000L ) != 0 ) {       /* if x < 0.0 */
        x.i &= 0x7FFFFFFFL;                /*    x = -x  */
      m = 1;
   }
   else
      m = 0;
/*****
** Exceptional cases 
*****/
   if ( x.i > threshold_1.i ) /* Above this n, n1, or n2 won't fit in int */
#ifdef IEEE
      return (qtc_ferror(FSIN_OUT_OF_RANGE,x.f,dummy));
#else
      return (qtc_ferr(FSIN_OUT_OF_RANGE,x));
#endif
/*****
** Argument reduction: |x| = (n1 + n2)*pi/256 + rr, which n1 is a
** multiple of 256.
**
** If x < threshold_2, the integer portion and the first fraction
** bit of x*256/pi is representable in a sp floating point number
*****/
   if ( x.i <= threshold_2.i ) {
      temp = x.f*inv_p.f;
      temp += 0.5;
      n = (int)temp;     /* n = intrnd(x*inv_p.f) */
      n2 = n & 0xFF;     /* n2 has 8 bits max */
      if (n2 > 128)
         n2 -= 256;      /* -128 < n2 <= 128 */
      n1 = n - n2;
      m += n1 >> 8;
      an2 = ABS(n2);
      if (an2 <= 25) {
      /***** 
      ** Expansion about zero:
      ** Use polynomial expansion about zero for small x:
      **  pp = rr - rr**3/3! + rr**5/5! - rr**7/7! + ...
      ** where rr is the distance from 0.0
      ** 
      ** For expansion about zero, rr is the dominant term in the
      ** result.  Therefore, sufficient terms in the representation
      ** of pi/256 must be used to ensure that the reduced
      ** argument is precise to at least it's last bit.  In
      ** the following table, a precision of three bits beyond
      ** the last bit in rr is used, to reduce round-off errors.
      **
      ** The code combines regions in the table for simplicity.
      **
      **  n2    min rr   ok error     p2 term (for larger n, use p3)
      **  --    ------   --------     -------
      **   0    2**-9     2**-36     n < 2**13
      **   1    2**-7     2**-34     n < 2**15
      **   2    2**-6     2**-33     n < 2**16
      **  3-5   2**-5     2**-32     n < 2**17
      **  6-10  2**-4     2**-31     n < 2**18
      ** 11-20  2**-3     2**-30     n < 2**19
      ** 21-25  2**-2     2**-29     n < 2**20
      **
      ** For n2 = 0, the minimum rr is SP(n*pi/256)-n*pi/256, where
      ** SP(a) is the single precision number nearest to a.
      ** Min rr >= 2**(N-7)*2**-(24-z), where n = 2**N and z is the
      ** number of zeros in n*pi/256 after the SP truncation.  An
      ** acceptable error in rr is 2**(-27) times min rr, or 2**(N-58-z).
      ** Equating this acceptable error to the error in argument reduction
      ** gives the number of terms in pi/256 needed for a given z:
      **
      ** # of terms in pi/256      error         # of zeros
      **           2             2**(-N-49)          --
      **           3             2**(-N-61)           3
      **           4             2**(-N-71)          13
      **           5             2**(-N-81)          23
      ** For x < threshold_2, the largest z is 12; for x < threshold_1,
      ** the largest z is 21.
      **
      ** Note that if n > 2**20, n1 will have more than 11 bits.
      ** In this case n1 is divided into n0 and n1.
      *****/
         if ( an2 == 0 ) {                /* n2 = 0 */
            if (n < 0x80000L) {
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
               temp_lf2.f = rr;
               if ( ( 0x7FFFFFFFL & temp_lf2.i ) < 0x38000000L ) {
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
               temp_lf2.f = rr;
               if ( ( temp_lf2.i & 0x7FFFFFFFL ) < 0x39800000L ) {
                  temp = fn0*p3_2.f;
                  r1 -= temp;
                  temp = fn1*p3_2.f;
                  r1 -= temp;
                  r2 = fnn*p4.f;
                  rr = r1 - r2;
               }
            }
         }
         else {                         /* n2 = 1,25 */
            if (n < 0x4000L) {
               fn = (float)n1;
               temp = fn*p1.f;
               r1 = x.f - temp;
               r2 = fn*p2.f;
               rr = r1 - r2;
            }
            else
            if (n < 0x80000L) {
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
         if (an2 > 8 ) {             /* |n2| = 9,25 */
            pp = ninv_5040.f*z;
            pp += inv_120.f;
            pp *= z;
            pp += ninv_6.f;
            pp *= z;
         }
         else
         if (an2 > 1) {         /* |n2| = 2,8 */
            pp = inv_120.f*z;
            pp += ninv_6.f;
            pp *= z;
         }
         else {                      /* |n2| = 0,1  */
            pp = ninv_6.f*z;
         }
         pp *= rr;
         pp -= r2;
         pp += r1;
      /*****
      ** Reconstruction of sin(x):
      **           sin(x) = (-1)**m * pp
      *****/
         if ( (m & 1) == 1 ) {
            long_float temp_lf;
            temp_lf.f = pp;
            temp_lf.i ^= 0x80000000L;
            pp = temp_lf.f;
         }
         temp_return = pp;
         return (temp_return);
      }           
      else {
      /*****
      ** Expansion about a table value:
      ** The dominant term in the result is sin(x0), where
      ** x0 = n2*pi/256.  The minimum sin(x0) is sin(26*pi/256)
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
            rr = x.f - temp;
            temp = fn*p2.f;
            rr -= temp;
         }
      /*****
      ** Else if n < 2**20, n1*p1 and n2*p1 are 
      ** exactly representable and rr = ((x - n1*p1) - n2*p1) - n*p2
      *****/
         else
         if (n < 0x80000L) {
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
      /*****
      ** Else n < 2**23 (since x < threshold_2),
      ** n0*p1, n1*p1, n2*p1, n0*p2_2,
      ** n1*p2_2, and n2*p2_2 are exactly representable and 
      ** rr = ((((((x - n0*p1) - n1*p1) - n0*p2_2) - n2*p1)
      **             - n1*p2_2) - n*p3) - n2*p2_2
      *****/
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
      }
   }
   else {
/*****
** The integer part and first fraction bit of x*256/pi is not
** representable in a single-precision floating point number.  Therefore,
** determine n in two steps:  n0,n1 (each 11 bits or less) is
** the integer part of x*1/pi; n2 (8 bits or less) is the integer
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
      temp_lf2.f = temp;
      if ( temp_lf2.i >= (unsigned long int) 0x80000000L ) {
         temp -= 0.5;
      }
      else {
         temp += 0.5;
      }
      n2 = (int)temp;      /* n2 = intrnd(rx*inv_p.f) */
   /*****
   ** Generally n2 will be in [-128,128].  However, because of 
   ** roundoff error and the error in the s.p. representation of 
   ** inv_pi, nn may be too small or too large by 1, which will
   ** cause n2 to be too large or too small.  The following
   ** logic corrects for these cases.
   *****/
      if ( n2 > 128 ) {
         n2 -= 256;
         nn++;
         if (n1 < 0x7FFL) {
            n1++;
            fn1 = (float)n1;
         }
         else {
            n1 = 0;
            n0 += 0x800L;
            fn1 = 0.0;
            fn0 = (float)n0;
         }
         fnn = (float)nn;
         r_sav -= pi1.f;
      }
      if ( n2 < -128 ) {
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
      m += n1;
      an2 = ABS(n2);
      temp = fn0*pi2_2.f;
      rr = r_sav - temp;     /* r_sav =  x - 256*(n0+n1)*p1 */
      if ( an2 <= 19 ) {
      /***** 
      ** Use polynomial expansion about zero for small x:
      **  pp = rr - rr**3/3! + rr**5/5! - rr**7/7! + ...
      ** where rr is the distance from 0.0
      *****/
         temp = fn1*pi2_2.f;
         r1 = rr - temp;
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
         temp_lf2.f = rr;
         if ( ( an2 == 0 ) && ( ( temp_lf2.i & 0x7FFFFFFFL ) < 0x38800000L ) ) {
            r1 -= temp;
            temp = fn0*pi4_2.f;
            r1 -= temp;
            r2 = fnn*pi5.f;
            temp = fn1*pi4_2.f;
            r2 += temp;
            rr = r1 - r2;
         }
         z = rr*rr;
         if ( an2 > 7 ) {            /* |n2| = 8,19 */
            pp = ninv_5040.f*z;
            pp += inv_120.f;
            pp *= z;
            pp += ninv_6.f;
            pp *= z;
         }
         else
         if ( an2 > 1 ) {            /* |n2| = 2,7 */
            pp = inv_120.f*z;
            pp += ninv_6.f;
            pp *= z;
         }
         else {                      /* |n2| = 0,1  */
            pp = ninv_6.f*z;
         }
         pp *= rr;
         pp -= r2;
         pp += r1;
      /*****
      ** Reconstruction of sin(x):
      **           sin(x) = (-1)**m * pp
      *****/
         if ( (m & 1) == 1 ) {
            long_float temp_lf;
            temp_lf.f = pp;
            temp_lf.i ^= 0x80000000L;
            pp = temp_lf.f;
         }
         temp_return = pp;
         return (temp_return);
      }           
      else {
      /*****
      ** rr = ((((((x - n0*pi1) - n1*pi1) - n0*pi2_2) - n2*p1)
      **             - n1*pi2_2) - n*p3) - n2*p2_2
      *****/
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
      }
   }
/*****
** If j < 0, fold into quadrant I (j > 0) for table lookup
** and increment m to change the sign of the result
*****/
   if ( n2 < 0 ) {
      long_float temp_lf;
      temp_lf.f = rr;
      temp_lf.i ^= 0x80000000L;
      rr = temp_lf.f;		/* rr = -rr */
      n2 = -n2;
      m++;
   }
/*****
** Get table values for sine and cosine
*****/
   s_lead.i = fsinfh[n2];
   s_trail.i = fsinfl[n2];
   n2 = 128 - n2;
   c_lead.i = fsinfh[n2];

/*****
** Polynomial approximation of sin(x0+rr) 
** sin(x0+rr) = sin(x0)*cos(rr) + cos(x0)*sin(rr)
**           = sin(x0)*(1 - rr**2/2!) +
**             cos(x0)*(rr - rr**3/3!)
** where x0 = j*pi/256
*****/
   z = rr*rr;
   cr = -0.5*z;
   pp = s_lead.f*cr;
   pp += s_trail.f;
   sr = z * ninv_6.f;
   sr *= rr;
   sr += rr;
   temp = c_lead.f*sr;
   pp += temp;
   pp += s_lead.f;

/*****
** Reconstruction of sin(x):
**           sin(x) = (-1)**m * pp
*****/
   if ( (m & 1) == 1 ) {
      long_float temp_lf;
      temp_lf.f = pp;
      temp_lf.i ^= 0x80000000L;
      pp = temp_lf.f;
   }

   temp_return = pp;
   return (temp_return);

}   /* end of qtc_fsin() routine */

/*** end of qtc_fsin.c ***/
