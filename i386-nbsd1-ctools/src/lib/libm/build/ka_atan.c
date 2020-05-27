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

/*** qtc_datan.c ***/

#include "qtc_intr.h"
#include "qtc.h"
#include <stdlib.h>
#include <math.h>

/*
******************************************************************************
**
*/
FUNCTION double qtc_datan(param)
/*
**  Purpose: This is a table-based inverse tangent routine
**
**           Accuracy - less than 1 ULP error
**           Valid input range - all floating-point numbers
**
**           Special case handling - non-IEEE
**             no special cases
**
**           Special case handling - IEEE:
**             qtc_datan(+INF) - returns pi/2
**             qtc_datan(-INF) - returns -pi/2
**             qtc_datan(QNAN) - calls qtc_error() with DATAN_QNAN, input value
**             qtc_datan(SNAN) - calls qtc_error() with DATAN_INVALID_OPERATION,
**                               input value
**             qtc_datan(DEN)  - returns correct value
**             qtc_datan(+-0)  - returns +/-0.0
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
**         One of the compile line switches -DMSH_FIRST or -DLSH_FIRST should
**         be specified when compiling this source.
**
**  Input Parameters:
*/
    double param; /* input value for which the arctangent is desired */
/*
**  Output Parameters: none
**
**  Return value: returns the double-precision inverse tangent of the
**                input argument, in radians
**
**  Called by:  any
** 
**  History:
**
**        Version 1.0 - 8 July 1988, J F Kauth and L A Westerman
**                1.1 - 31 October 1988, L A Westerman
**                1.2 - 2 November 1988, L A Westerman, minor editing
**                                       changes
**                1.3 - 30 November 1988, J F Kauth, declared dtanfh, dtanfl,
**                      and dsinfh "unsigned" for consistency
**                1.4 - 13 February 1989, L A Westerman, corrected problem with
**                      allocation of static storage space being misaligned
**                      for double values, changed handling of small input
**                      values to return x when x is small, denormalized, or
**                      +/- 0.0
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
    extern const unsigned long int dtanfh[][2];
    extern const unsigned long int dtanfl[][2];
    extern const unsigned long int dsinfh[][2];

    volatile double x = param;

/*
** Force double-word alignment
*/
    static const double dummy = 0.0;
/*
** pi/512 = pio512hi + pio512lo
** pio512hi has 8 trailing zeros so that j*pio512hi is exact
** 
** pi/2 = pio2 + pio2lo
*/
#ifdef MSH_FIRST
    static const unsigned long int inv_11[2]= {0x3FB745D1L,0x745D1746L};
        /* 9.0909090909091e-02 */
    static const unsigned long int inv_13[2]= {0x3FB3B13BL,0x13B13B14L};
        /* 7.6923072307692e-02 */
    static const unsigned long int inv_15[2]= {0x3FB11111L,0x11111111L};
        /* 6.6666666666667e-02 */
    static const unsigned long int inv_17[2]= {0x3FAE1E1EL,0x1E1E1E1EL};
        /* 5.8823529411764e-02 */
    static const unsigned long int inv_3[2] = {0x3FD55555L,0x55555555L};
        /* 3.3333333333333e-01 */
    static const unsigned long int inv_5[2] = {0x3FC99999L,0x9999999AL};
        /* 2.0000000000000e-01 */
    static const unsigned long int inv_7[2] = {0x3FC24924L,0x92492492L};
        /* 1.4285714285714e-01 */
    static const unsigned long int inv_9[2] = {0x3FBC71C7L,0x1C71C71CL};
        /* 1.1111111111111e-01 */
    static const unsigned long int pio2[2]  = {0x3FF921FBL,0x54442D18L};
        /* 1.5707963267949e+00 */
    static const unsigned long int pio2lo[2]= {0x3C91A626L,0x33145C07L};
        /* 6.1232339957367e-17 */
    static const unsigned long int pio4[2]  = {0x3FE921FBL,0x54442D18L};
        /* 7.8539816339748e-01 */
    static const unsigned long int pio512hi[2]  = {0x3F7921FBL,0x54442D00L};
        /* 6.1359231515426e-03 */
    static const unsigned long int pio512lo[2]  = {0x3C784698L,0x98CC5170L};
        /* 2.1055870539680e-17 */
    static const unsigned long int teno3[2] = {0x400AAAAAL,0xAAAAAAABL};
        /* 3.3333333333333e+00 */
#else
    static const unsigned long int inv_11[2]= {0x745D1746L,0x3FB745D1L};
        /* 9.0909090909091e-02 */
    static const unsigned long int inv_13[2]= {0x13B13B14L,0x3FB3B13BL};
        /* 7.6923072307692e-02 */
    static const unsigned long int inv_15[2]= {0x11111111L,0x3FB11111L};
        /* 6.6666666666667e-02 */
    static const unsigned long int inv_17[2]= {0x1E1E1E1EL,0x3FAE1E1EL};
        /* 5.8823529411764e-02 */
    static const unsigned long int inv_3[2] = {0x55555555L,0x3FD55555L};
        /* 3.3333333333333e-01 */
    static const unsigned long int inv_5[2] = {0x9999999AL,0x3FC99999L};
        /* 2.0000000000000e-01 */
    static const unsigned long int inv_7[2] = {0x92492492L,0x3FC24924L};
        /* 1.4285714285714e-01 */
    static const unsigned long int inv_9[2] = {0x1C71C71CL,0x3FBC71C7L};
        /* 1.1111111111111e-01 */
    static const unsigned long int pio2[2]  = {0x54442D18L,0x3FF921FBL};
        /* 1.5707963267949e+00 */
    static const unsigned long int pio2lo[2]= {0x33145C07L,0x3C91A626L};
        /* 6.1232339957367e-17 */
    static const unsigned long int pio4[2]  = {0x54442D18L,0x3FE921FBL};
        /* 7.8539816339748e-01 */
    static const unsigned long int pio512hi[2]  = {0x54442D00L,0x3F7921FBL};
        /* 6.1359231515426e-03 */
    static const unsigned long int pio512lo[2]  = {0x98CC5170L,0x3C784698L};
        /* 2.1055870539680e-17 */
    static const unsigned long int teno3[2] = {0xAAAAAAABL,0x400AAAAAL};
        /* 3.3333333333333e+00 */
#endif
    /*
    ** threshold1 = MSH(x1),
    ** where if x >= x1, atan(x) = pio2
    */
    static const int threshold1  = 0x43349FF2L;
    /*
    ** atani[] is the index table into the tangent table for arguments
    ** in the range [0,1)
    **
    ** j = (atani[i-64] + 1) is the index into the tangent table for interval 
    ** [i/512,(i+1)/512).  It gives the minimum (x-x0)/(1+x0**2),
    ** where x0 = tan(j*pi/512), 1/(1+x0**2) = (cos(j*pi/512))**2
    ** There is no index for intervals 0 through 63, since expansion about
    ** zero is used in this range.
    ** The index is stored with a bias of -1 so that it will fit in 1 byte
    ** (j <= 128).
    */
    static const char atani[448] = {
          19,20,20,20,21,21,21,22,22,22,23,23,
          23,23,24,24,24,25,25,25,26,26,26,27,27,27,28,28,28,28,29,29,
          29,30,30,30,31,31,31,32,32,32,32,33,33,33,34,34,34,35,35,35,
          35,36,36,36,37,37,37,38,38,38,38,39,39,39,40,40,40,41,
          41,41,41,42,42,42,43,43,43,44,44,44,44,45,45,45,46,46,46,46,47,47,
          47,48,48,48,49,49,49,49,50,50,50,51,51,51,51,52,52,52,53,53,53,53,
          54,54,54,55,55,55,55,56,56,56,56,57,57,57,58,58,58,58,59,
          59,59,60,60,60,60,61,61,61,61,62,62,62,63,63,63,63,64,64,64,64,
          65,65,65,66,66,66,66,67,67,67,67,68,68,68,68,69,69,69,
          70,70,70,70,71,71,71,71,72,72,72,72,73,73,73,73,74,74,74,74,
          75,75,75,75,76,76,76,76,77,77,77,77,78,78,78,78,79,79,79,79,
          80,80,80,80,81,81,81,81,82,82,82,82,83,83,83,83,84,84,84,84,
          85,85,85,85,85,86,86,86,86,87,87,87,87,88,88,88,88,89,89,89,
          89,89,90,90,90,90,91,91,91,91,92,92,92,92,92,93,93,93,93,
          94,94,94,94,94,95,95,95,95,96,96,96,96,96,97,97,97,97,97,98,98,
          98,98,99,99,99,99,99,100,100,100,100,100,101,101,101,101,
          102,102,102,102,102,103,103,103,103,103,104,104,104,104,104,105,
          105,105,105,105,106,106,106,106,106,107,107,107,107,107,108,108,
          108,108,108,109,109,109,109,109,110,110,110,110,110,110,
          111,111,111,111,111,112,112,112,112,112,113,113,113,113,113,114,
          114,114,114,114,114,115,115,115,115,115,116,116,116,116,116,116,
          117,117,117,117,117,117,118,118,118,118,118,119,119,119,119,119,
          119,120,120,120,120,120,120,121,121,121,121,121,121,122,122,122,
          122,122,123,123,123,123,123,123,124,124,124,124,124,124,124,125,
          125,125,125,125,125,126,126,126,126,126,126,127,127,127
    };
    /*
    ** atani2[] is the index table into the tangent table for arguments
    ** in the range [1,2)
    **
    ** j = (atani2[i] + 128) is the index into the tangent table for interval 
    ** [1+i/128,1+(i+1)/128).  It gives the minimum (x-x0)/(1+x0**2),
    ** where x0 = tan(j*pi/512), 1/(1+x0**2) = (cos(j*pi/512))**2
    ** The index is stored with a bias of -128 so that it will fit in 1 byte
    */
    static const char atani2[128] = {
          0, 1, 2, 2, 3, 3, 4, 5, 5, 6, 6, 7, 8, 8, 9, 9,10,10,11,12,
         12,13,13,14,14,15,15,16,16,17,17,18,18,19,19,20,20,21,21,22,
         22,23,23,24,24,24,25,25,26,26,27,27,27,28,28,29,29,30,30,30,
         31,31,32,32,32,33,33,34,34,34,35,35,35,36,36,36,37,37,38,38,
         38,39,39,39,40,40,40,41,41,41,42,42,42,43,43,43,44,44,44,44,
         45,45,45,46,46,46,47,47,47,47,48,48,48,49,49,49,49,50,50,50,
         50,51,51,51,52,52,52,52
    };
    int    i, j, k, sign_flag, inv_flag;
    long int exp;
    double  dj, t2, x02, x03;
    volatile double pp, x0lo, c, t, x0;

/*******************************
**  External functions        **
*******************************/
#ifdef IEEE
    extern double qtc_error();
#endif

/*******************************
** Function body              **
*******************************/

    /*
    ** Extract the exponent
    */
    exp = MSH(x) & 0x7FF00000L;
#ifdef IEEE
    /*
    ** If the exponent is 0x7FF, the value is either an infinity or NaN
    */
    if ( exp == 0x7FF00000L ) {
      /*
      ** If the fraction bits are all zero, this is an infinity
      */
      if ( ( ( MSH(x) & 0x000FFFFFL ) | LSH(x) ) == 0L ) {
        /*
        ** For negative infinity, return -pi/2
        */
        if ( MSH(x) & 0x80000000L )
          return (-DOUBLE(pio2));
        /*
        ** For positive infinity, return pi/2
        */
        else
          return (DOUBLE(pio2));
      }
      /*
      ** If some fraction bits are non-zero, this is a NaN
      */
      else {
        if ( is_sNaN(x) ) {
          /*
          ** For signaling NaNs, call the error code with the invalid
          ** operation signal
          */
          return (qtc_error(DATAN_INVALID_OPERATION,x));
        }
        else {
          /*
          ** For quiet NaNs, call the error code with the qNaN signal
          */
          return (qtc_error(DATAN_QNAN,x));
        }
      }
    }
#endif /* end of IEEE special case handling */
    /*
    ** Check for a small value of x, which may be a denormalized
    ** value or zero.  Any exponent of less than or equal to 0x3e2
    ** (2**-29) will result in the input value being returned.
    */
    if ( exp <= 0x3E200000L )
        return (x);

    /*
    ** Reduce x to the range [0,2).
    **       atan(x) = -atan(-x)
    **       atan(x) = pi/2 - atan(1/x)
    */
    if ( MSH(x) < (unsigned long int) 0x80000000L )         /* if x >= 0.0 */
        sign_flag = 0;
    else {                           /* else       */
        MSH(x) &= 0x7FFFFFFFL;       /*    x = -x  */
        sign_flag = 1;
    }
    if ( MSH(x) < 0x40000000L )
        /*
        ** x in [0.0,2.0)
        */
        inv_flag = 0;
    else if ( MSH(x) < threshold1 ) {
        /*
        ** x in [2.0,threshold1):  atan(x) = pi/2 - atan(1/x)
        */
        x = 1.0/x;
        inv_flag = 1;
    }
    else {
        /*
        ** x in [threshold1,inf):  atan(x) = pi/2
        */
        if (!sign_flag)
            return (DOUBLE(pio2));
        else
            return (-DOUBLE(pio2));
    }
    if ( MSH(x) >= 0x3FC00000L ) {
        /*
        ** atan(x0+rr) = atan(x0) + pp
        ** pp = t + a2*t**2 + a3*t**3 + a4*t**4 + a5*t**5 + a6*t**6 ...
        **  where
        **       x = x0 + rr
        **       t = rr/(1+x0**2)
        **       a2 = -x0
        **       a3 = x0**2 - 1/3
        **       a4 = -x0**3 + x0
        **       a5 = x0**4 - 2*x0**2 + 1/5
        **       a6 = -x0**5 + 10/3*x0**3 - x0
        **       a7 = x0**6 - 5*x0**4 + 3*x0**2 - 1/7
        **
        ** For x in [0,1):
        **    Determine i such that x is in [i/512, (i+1)/512)
        **    by extracting the first 9 fraction bits of (1.0+x).
        **    Then the index into the tan tables is j = atani[i-64] + 1
        **    (for this range the a7*t**7 term is not needed)
        ** For x in [1,2):
        **    Determine i such that x is in [1+i/128, 1+(i+1)/128)
        **    by extracting the first 7 fraction bits of x.
        **    Then the index into the tan tables is j = atani2[i] + 128
        **    (for this range the -1/7 term of a7 is not needed)
        **
        ** x0 = tan(j*pi/512) = dtanfh[j] + dtanfl[j]
        ** atan(x0) = j*pi/512
        ** 1/(1+x0**2) = (cos(j*pi/512))**2 = (dsinfh[256-j])**2
        */
        if ( MSH(x) < 0x3FF00000L ) {    /* x in [0,1) */
            t = 1.0 + x;
            /*
            **  if t = 2.0, then x = 1.0 - 1 ulp: atan(x) = pi/4
            */
            if ( MSH(t) == 0x40000000L ) { /* if t == 2 */
                if (!sign_flag)
                    return (DOUBLE(pio4));
                else
                    return (-DOUBLE(pio4));
            }
            i = ( MSH(t) & 0x000FF800L ) >> 11;
            j = atani[i-64] + 1;
        }
        else {                        /* x in [1,2) */
            i = ( MSH(x) & 0x000FE000L ) >> 13;
            j = atani2[i] + 128;
        }
        LSH(x0) = dtanfh[j][0];
        MSH(x0) = dtanfh[j][1];
        LSH(x0lo) = dtanfl[j][0];
        MSH(x0lo) = dtanfl[j][1];         /* x0 = x0+x0lo = tan(j*pi/512) */
        k = 256 - j;
        LSH(c) = dsinfh[k][0];
        MSH(c) = dsinfh[k][1];            /* c = cos(j*pi/512) */
        t = (x - x0);
        t = (t - x0lo)*c*c;               /* t = rr/(1+x0**2) */
        x02 = x0*x0;
        x03 = x02*x0;
        if ( MSH(x) < 0x3FF00000L ) {
            pp = ((((((DOUBLE(teno3) - x02)*x03 - x0)*t +
                     ((x02 - 2.0)*x02 + DOUBLE(inv_5)))*t +
                     (x0 - x03))*t +
                     (x02 - DOUBLE(inv_3)))*t -
                      x0)*t*t +
                      t;
        }
        else {
            pp = ((((((((x02-5.0)*x02 + 3.0)*x02)*t +
                     (DOUBLE(teno3) - x02)*x03 - x0)*t +
                     ((x02 - 2.0)*x02 + DOUBLE(inv_5)))*t +
                     (x0 - x03))*t +
                     (x02 - DOUBLE(inv_3)))*t -
                      x0)*t*t +
                      t;
        }
        /*
        ** Add atan(x0) = j*pi/512
        **
        */
        if (!inv_flag) {
            dj = (double)j;
            pp += dj * DOUBLE(pio512lo);
            pp += dj * DOUBLE(pio512hi);
        }
        /*
        ** atan(x) = pi/2 - atan(1/x)   for x >= 2.0
        */
        else {
            dj = (double) k;
            pp = dj * DOUBLE(pio512lo) - pp;
            pp += dj * DOUBLE(pio512hi);
        }
    }
    else {
        /*
        ** For x < 2**(-3), use Taylor's expansion about 0:
        **   atan(x) = x**17/17 - x**15/15 + x**13/13 - ... - x**3/3 + x
        **        (for x < 2**(-4) omit x**17, x**15 terms)
        **
        **   atan(x) = pi/2 - atan(1/x)   for x >= 1.0
        */
        t2 = x*x;
        if ( MSH(x) >= 0x3FB00000L ) {  /* If 2**(-4) <= x < 2**(-3) */
            if (!inv_flag)
                pp = (((((((DOUBLE(inv_17)*t2 - DOUBLE(inv_15))*t2 +
                     DOUBLE(inv_13))*t2 - DOUBLE(inv_11))*t2 +
                     DOUBLE(inv_9))*t2 - DOUBLE(inv_7))*t2 +
                     DOUBLE(inv_5))*t2 - DOUBLE(inv_3))*t2*x + x;
            else {
                pp = DOUBLE(pio2lo) - (((((((DOUBLE(inv_17)*t2 -
                     DOUBLE(inv_15))*t2 + DOUBLE(inv_13))*t2 -
                     DOUBLE(inv_11))*t2 + DOUBLE(inv_9))*t2 -
                     DOUBLE(inv_7))*t2 + DOUBLE(inv_5))*t2 - 
                     DOUBLE(inv_3))*t2*x;
                pp -= x;
                pp += DOUBLE(pio2);
            }
        }
        else {                        /* If x < 2**(-4) */
            if (!inv_flag)
                pp = (((((DOUBLE(inv_13)*t2 - DOUBLE(inv_11))*t2 +
                     DOUBLE(inv_9))*t2 - DOUBLE(inv_7))*t2 + 
                     DOUBLE(inv_5))*t2 - DOUBLE(inv_3))*t2*x + x;
            else {
                pp = DOUBLE(pio2lo) - (((((DOUBLE(inv_13)*t2 -
                     DOUBLE(inv_11))*t2 + DOUBLE(inv_9))*t2 - 
                     DOUBLE(inv_7))*t2 + DOUBLE(inv_5))*t2 -
                     DOUBLE(inv_3))*t2*x;
                pp -= x;
                pp += DOUBLE(pio2);
            }
        }
    }
    /*
    ** Reconstruction of atan(x):
    **           atan(x) = -atan(-x)          for x < 0.0
    */
    if (sign_flag)
        MSH(pp) ^= 0x80000000L;
    return (pp);

}   /* end of qtc_datan() routine */

double
atan2(double y, double x)
{
/*******************************
** Internal Data Requirements **
*******************************/
    extern const unsigned long int pi[];
    extern const unsigned long int _LPI_over_2[];
/*******************************
**  External functions        **
*******************************/
#ifdef IEEE
    extern double qtc_error();
#endif

    register int neg_y = 0;
    double at;

    if (!fabs(x) && !fabs(y))
        return (qtc_error(DATAN2_OUT_OF_RANGE,y,x));
    if (x > 0.0)
        return (atan(y/x));
    if (y < 0.0)
	neg_y++;
    if (x < 0.0)
	return ((neg_y ? -1 : 1) * (DOUBLE(pi) - atan(fabs(y/x))));
    if (x == 0.0)
	return ((neg_y ? -1 : 1) * (DOUBLE(_LPI_over_2)));
}

/*** end of qtc_datan.c ***/
