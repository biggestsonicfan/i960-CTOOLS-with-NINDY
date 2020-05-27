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

/*** qtc_fatan.c ***/

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
FUNCTION float qtc_fatan(float param)
/*
**  Purpose: This is a table-based inverse tangent routine.
**
**           Accuracy - less than 1 ULP error
**           Valid input range - all floating-point numbers
**
**           Special case handling - non-IEEE
**             no special cases
**
**           Special case handling - IEEE:
**             qtc_fatan(+INF) - returns pi/2
**             qtc_fatan(-INF) - returns -pi/2
**             qtc_fatan(QNAN) - calls qtc_error() with FATAN_QNAN, input value
**             qtc_fatan(SNAN) - calls qtc_error() with FATAN_INVALID_OPERATION,
**                               input value
**             qtc_fatan(DEN)  - returns correct value
**             qtc_fatan(+-0)  - returns +/-0.0
**
**  Method:
**
**        This function is based on the formula
**
**          atan(x) = atan(x0 + rr)
**                  = atan(x0)
**                    + rr/(1+x0*x0)
**                    - x0 * (rr/(1+x0*x0))**2
**                    + (x0**2 - 1/3) * (rr/(1+x0*x0))**3
**                    - (x0**3 - x0) * (rr/(1+x0*x0))**4
**                    + (x0**4 - 2*x0**2 + 1/5) * (rr/(1+x0*x0))**5
**                    - (x0**5 - 10/3*x0**3 + x0) * (rr/(1+x0*x0)**6
**                    + ...
**
**  Notes:
**         When compiled with the -DIEEE switch, this routine properly
**         handles infinities, NaNs, signed zeros, and denormalized
**         numbers.
**
**  Input Parameters:
*/
/*   float x;  value for which the arctangent is desired */
/*
**  Output Parameters: none
**
**  Return value: returns the single-precision inverse
**                tangent of the input argument, in radians
**
**  Called by:  any
** 
**  History:
**
**        Version 1.0 - 2 November 1988, J F Kauth and L A Westerman
**                1.1 - 6 January 1989, L A Westerman, cleaned up for release
**                1.2 - 13 February 1989, L A Westerman, changed processing for
**                      small input arguments, to avoid doing extra calculations
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
    extern const long unsigned int fsinfh[];
    extern const long unsigned int ftanfh[];
    extern const long unsigned int ftanfl[];
/*
** where if x >= threshold1.f, atan(x) = pio2
*/
    static const unsigned long int threshold1  = 0x4C700518L;  /* 6.2919776e07 */
/*
** pi/256 = pio256hi + pio256lo
** pio256hi has 7 trailing zeros so that j*pio256hi is exact
** 
** pi/2 = pio2 + pio2lo
*/
    long_float temp_lf;

    long_float inv_11;
    long_float inv_13;
    long_float inv_3;
    long_float inv_5;
    long_float inv_7;
    long_float inv_9;
    long_float npio2;
    long_float npio4mulp;
    long_float pio2;
    long_float pio2lo;
    long_float pio4mulp;
    long_float pio256hi;
    long_float pio256lo;

    /*
    ** atani[] is the index table into the tangent table for arguments
    ** in the range [0,1)
    **
    ** j = atani[i] is the index into the tangent table for interval 
    ** [i/256,(i+1)/256).  It gives the minimum (x-x0)/(1+x0**2),
    ** where x0 = tan(j*pi/256), 1/(1+x0**2) = (cos(j*pi/256))**2
    */
    static const char atani[256] = {
           0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6,
           7, 7, 7, 7, 8, 8, 8, 9, 9, 9,10,10,10,11,11,11,12,12,12,12,
          13,13,13,14,14,14,15,15,15,16,16,16,16,17,17,17,18,18,18,19,
          19,19,20,20,20,20,21,21,21,22,22,22,22,23,23,23,24,24,24,25,
          25,25,25,26,26,26,27,27,27,27,28,28,28,29,29,29,29,30,30,30,
          30,31,31,31,32,32,32,32,33,33,33,33,34,34,34,35,35,35,35,36,
          36,36,36,37,37,37,37,38,38,38,38,39,39,39,39,40,40,40,40,41,
          41,41,41,42,42,42,42,43,43,43,43,44,44,44,44,44,45,45,45,45,
          46,46,46,46,47,47,47,47,47,48,48,48,48,49,49,49,49,49,50,50,
          50,50,50,51,51,51,51,52,52,52,52,52,53,53,53,53,53,54,54,54,
          54,54,55,55,55,55,55,56,56,56,56,56,56,57,57,57,57,57,58,58,
          58,58,58,58,59,59,59,59,59,60,60,60,60,60,60,61,61,61,61,61,
          61,62,62,62,62,62,62,63,63,63,63,63,63,64,64,64
    };
    /*
    ** atani2[] is the index table into the tangent table for arguments
    ** in the range [1,8)
    **
    ** j = atani2[i] is the index into the tangent table for 
    ** the following intervals:
    **
    ** [1,2):  32 table entries i= 0,...,31 for sub-intervals
    **         [ 1+i/32,1+(i+1)/32 )
    **
    ** [2,4):  32 table entries i= 32,...,63 for sub-intervals
    **         [ 2+k/16,2+(k+1)/16 )  ( k = i-32 )
    **
    ** [4,8):  32 table entries i= 64,...,95 for sub-intervals
    **         [ 4+i/8 ,4+(i+1)/8 )   ( k = i-64 )
    **         
    ** Atan2i gives the minimum (x-x0)/(1+x0**2), where
    ** x0 = tan(j*pi/256), 1/(1+x0**2) = (cos(j*pi/256))**2
    **         
    ** Note: the interval [1,2) needs only 16 indices but uses 32
    ** so as to be consistent with intervals [2,4) and [4,8) which
    ** need 32 indices.  The 16 indices which give equally good
    ** results for interval [1,2) are: 
    **    65,68,70,72,74,76,78,79,81,82,84,85,86,88,89,90
    */
    static const char atani2[96] = {
          65, 66, 67, 68, 69, 70, 72, 73, 74, 74, 75, 76, 77, 78, 79, 80,
          80, 81, 82, 83, 83, 84, 85, 85, 86, 87, 87, 88, 88, 89, 89, 90,

          91, 92, 93, 94, 94, 95, 96, 97, 97, 98, 99, 99,100,100,101,102,
         102,103,103,103,104,104,105,105,106,106,106,107,107,107,108,108,

         108,109,109,110,110,111,111,112,112,113,113,113,114,114,114,114,
         115,115,115,115,116,116,116,116,117,117,117,117,117,117,118,118
    };
    static float temp_return;
    int   i, inv_flag, j, k, sign_flag;
    long int exp;
    float a3, a4, c, fj, pp, t, temp, t2, x0, x0lo, x02;
    float dummy;
    /* float x; */

    long_float x;

    inv_11.i    = 0x3DBA2E8CL; /* 9.0909093618e-02 */
    inv_13.i    = 0x3D9D89D9L; /* 7.6923079789e-02 */
    inv_3.i     = 0x3EAAAAABL; /* 3.3333334327e-01 */
    inv_5.i     = 0x3E4CCCCDL; /* 2.0000000298e-01 */
    inv_7.i     = 0x3E124925L; /* 1.4285714924e-01 */
    inv_9.i     = 0x3DE38E39L; /* 1.1111111194e-01 */
    npio2.i     = 0xBFC90FDBL; /*-1.5707963705e+00 */
    npio4mulp.i = 0xBF490FDAL; /*-7.8539812565e-01 */
    pio2.i      = 0x3FC90FDBL; /* 1.5707963705e+00 */
    pio2lo.i    = 0xB33BBD2EL; /*-4.3711388286e-08 */
    pio4mulp.i  = 0x3F490FDAL; /* 7.8539812565e-01 */
    pio256hi.i  = 0x3C491000L; /* 1.2271881103e-02 */
    pio256lo.i  = 0xB315777AL; /*-3.4800429205e-08 */

/*******************************
**  External functions        **
*******************************/
#ifdef IEEE
    /* extern float qtc_ferror(); */
#endif

/*******************************
** Function body              **
*******************************/
    /* x.i = *x_ptr.i; */
    /*
    ** Extract the exponent
    */
    x.f = param;

    exp = x.i & 0x7F800000L;

#ifdef IEEE
    /*
    ** If the exponent is 0x7F8, the value is either an infinity or NaN
    */
    if ( exp == 0x7F800000L ) {
      /*
      ** If the fraction bits are all zero, this is an infinity
      */
      if ( (x.i & 0x007FFFFFL) == 0x0L ) {
        /*
        ** For negative infinity, return -pi/2
        */
        if ( ( x.i & 0x80000000L ) != 0x0L )
          return ( npio2.f );
        /*
        ** For positive infinity, return pi/2
        */
        else
          return ( pio2.f );
      }
      /*
      ** If some fraction bits are non-zero, this is a NaN
      */
      else {
        if ( is_fsNaN(x) ) {
          /*
          ** For signaling NaNs, call the error code with the invalid
          ** operation signal
          */
          return (qtc_ferror(FATAN_INVALID_OPERATION,x.f,dummy));
        }
        else {
          /*
          ** For quiet NaNs, call the error code with the qNaN signal
          */
          return (qtc_ferror(FATAN_QNAN,x.f,dummy));
        }
      }
    }

#endif /* end of IEEE special case handling */
    /*
    ** Check for a small value of x, including denormalized or zero.
    ** Any of these ranges will result in the input value being returned.
    */
    if ( exp <= 0x38800000L )
      return (x.f);
    /*
    ** Reduce x to the range [0,8).
    **       atan(x) = -atan(-x)
    **       atan(x) = pi/2 - atan(1/x)
    */
    if ( x.i >= (unsigned long int) 0x80000000L ) {   /* if x < 0.0 */
        x.i &= 0x7FFFFFFFL;                           /*    x = -x  */
        sign_flag = 1;
    }
    else {
        sign_flag = 0;
    }
    if ( x.i < 0x41000000L ) {
        /*
        ** x in [0.0,8.0)
        */
        inv_flag = 0;
    }
    else if ( x.i < threshold1 ) {
        /*
        ** x in [2.0,threshold1):  atan(x) = pi/2 - atan(1/x)
        */
        x.f = 1.0/x.f;
        inv_flag = 1;
    }
    else {
        /*
        ** x in [threshold1,inf):  atan(x) = pi/2
        */
        if (!sign_flag)
            return ( pio2.f );
        else
            return ( npio2.f );
    }
    if ( x.i > 0x3E800000L ) {
        /*
        ** atan(x0+rr) = atan(x0) + pp
        ** pp = t + a2*t**2 + a3*t**3 + a4*t**4 + a5*tt**5 + ...
        **  where
        **       x = x0 + rr
        **       t = rr/(1+x0**2)
        **       a2 = -x0
        **       a3 = x0**2 - 1/3
        **       a4 = -x0**3 + x0
        **       a5 = (x0**4 - 2*x0**2 + 1/5)
        **
        ** For x in [0,1):
        **    Determine i such that x is in [i/256, (i+1)/256)
        **    by extracting the first 8 fraction bits of (1.0+x).
        **    Then the index into the tan tables is j = atani[i].
        **    (For this range the a4*t**4  a5*t**5 terms are not needed.)
        **
        ** For x in [1,8):
        **    Determine i such that 
        **        x is in [1+i/32, 1+(i+1)/32), for i =  0,..,31
        **        x is in [2+k/16, 2+(k+1)/16), for i = 32,..,63, k = i-32  
        **        x is in [4+k/8 , 4+(k+1)/8 ), for i = 64,..,95, k = i-64  
        **    by extracting the exponent and first 5 fraction bits of x
        **    and subtracting the exponent bias, 0x3F800000L.
        **    Then the index into the tan tables is j = atani2[i].
        **
        **    (For x in [1,4) the a5*t**5 term is not needed; for
        **     x in [4,8) the (-2x0**2 + 1/5) portion of a5 is not needed.)
        **
        ** x0 = tan(j*pi/256) = ftanfh[j] + ftanfl[j]
        ** atan(x0) = j*pi/256
        ** 1/(1+x0**2) = (cos(j*pi/256))**2 = (dsinfh[128-j])**2
        */
        if ( x.i < 0x3F800000L ) {    /* x in [0,1) */
            t = 1.0 + x.f;
            /*
            **  if t == 2.0, then x = 1.0 - 1 ulp: atan(x) = pi/4 - 1 ulp
            */
            temp_lf.f = t;
            if ( temp_lf.i == 0x40000000L ) { /* if t == 2 */
                if ( !sign_flag )
                    return (pio4mulp.f);
                else
                    return (npio4mulp.f);
            }
            i = (temp_lf.i & 0x007F8000L) >> 15;
            j = atani[i];
        }
        else {                          /* x in [1,8) */
            i = ( (x.i & 0x7FFC0000L) - 0x3F800000L ) >> 18;
            j = atani2[i];
        }
        temp_lf.i = ftanfh[j];
        x0        = temp_lf.f;

        temp_lf.i = ftanfh[j];
        x0lo      = temp_lf.f;

        k = 128 - j;
        temp_lf.i = fsinfh[k];
        c = temp_lf.f;

        t = x.f - x0;
        t -= x0lo;
        t *= c;
        t *= c;                   /* t = rr/(1+x0**2) */
        x02 = x0*x0;
        a3 = x02 - inv_3.f;
        if ( x.i < 0x3F800000L ) {     /* x in [0,1)  */
            pp = a3*t;
        }
        else {                           /* x in [1,8)  */
            a4 = x0*x02; 
            a4 = x0 - a4;   /* a4 = x0 - x0**3  */
            if ( x.i < 0x40800000L ) {      /* x in [1,4)  */
                pp = a4*t;
            }
            else {                            /* x in [4,8)  */
                pp = x02*x02;
                pp *= t;
                pp += a4;
                pp *= t;
            }
            pp += a3;
            pp *= t;
        }
        pp -= x0;
        pp *= t;
        pp *= t;
        pp += t;
        /*
        ** Add atan(x0) = j*pi/256
        */
        if ( !inv_flag ) {
            fj = (float)j;
            temp = fj * pio256lo.f;
            pp += temp;
        }
        /*
        ** atan(x) = pi/2 - atan(1/x)   for x >= 1.0
        */
        else {
            fj = (float)(k);
            temp = fj * pio256lo.f;
            pp = temp - pp;
        }
        temp = fj * pio256hi.f;
        pp += temp;
    }
    else {
        /*
        ** For x <= 0.25, use Taylor's expansion about 0:
        **
        **   atan(x) = x - x**3/3 + x**5/5 - x**7/7 + x**9/9 
        **               - x**11/11 + x**13/13 - ...
        **
        **   For x <= 2**-3, the x**13 and x**11 terms may be dropped.
        **
        ** atan(x) = pi/2 - atan(1/x)   for x >= 1.0
        */
        t2 = x.f*x.f;
        if ( x.i > 0x3E000000L ) {  /* 2**-3 < x <= 2**-2 */
            pp = inv_13.f*t2;
            pp -= inv_11.f;
            pp *= t2;
            pp += inv_9.f;
            pp *= t2;
        }
        else {                        /* x <= 2**-3 */
            pp = inv_9.f*t2;
        }
        pp -= inv_7.f;
        pp *= t2;
        pp += inv_5.f;
        pp *= t2;
        pp -= inv_3.f;
        pp *= t2;
        pp *= x.f;
        if ( !inv_flag ) {
            pp += x.f;
        }
        else {
            pp = pio2lo.f - pp;
            pp -= x.f;
            pp += pio2.f;
        }
    }
    /*
    ** Reconstruction of atan(x):
    **           atan(x) = -atan(-x)          for x < 0.0
    */
    if ( sign_flag ) {
        temp_lf.f = pp;
        temp_lf.i ^= 0x80000000L;
        pp = temp_lf.f;
    }
    temp_return = pp;
    return (temp_return);

}   /* end of qtc_fatan() routine */

/*** end of qtc_fatan.c ***/
