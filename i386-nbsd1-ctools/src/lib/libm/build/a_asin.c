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

/*** qtc_dasin.c ***/

#include "qtc_intr.h"
#include "qtc.h"

#define INT2_MASK 0xFFFFFF00L

/*
************************************************************************
**
*/
FUNCTION double qtc_dasin(param)
/*
**  Purpose: This is an arcsine routine.
**
**        Accuracy - 2 ulps
**    
**        Valid input range - |x| <= 1.0
**    
**        Special case handling - IEEE:
**    
**          qtc_dasin(sNaN) - call qtc_error() with 
**                            DASIN_INVALID_OPERATION, input value
**          qtc_dasin(qNaN) - call qtc_error() with DASIN_QNAN, input value 
**          qtc_dasin(+INF) - call qtc_error() with DASIN_OUT_OF_RANGE, input 
**                            value
**          qtc_dasin(-INF) - call qtc_error() with DASIN_OUT_OF_RANGE, input 
**                            value
**          qtc_dasin(DEN)  - returns correct value
**          qtc_dasin(+-0)  - returns +/-0.0
**          qtc_dasin(+OOR) - call qtc_error() with DASIN_OUT_OF_RANGE, input 
**                            value
**          qtc_dasin(-OOR) - call qtc_error() with DASIN_OUT_OF_RANGE, input 
**                            value
**    
**        Special case handling - non-IEEE:
**    
**          qtc_dasin(+OOR) - call qtc_err() with DASIN_OUT_OF_RANGE, input 
**                            value
**          qtc_dasin(-OOR) - call qtc_err() with DASIN_OUT_OF_RANGE, input 
**                            value
**    
**  Method:
**
**         The arcsin is computed in terms of the arctan using the 
**         formula:
**
**           arcsine (x) = arctan ( x/sqrt(1-x**2))      
**
**  Notes:
**
**         When compiled with the -DIEEE switch, this routine properly
**         handles infinities, NaNs, signed zeros, and denormalized
**         numbers.
**
**         One of the compile line switches -DMSH_FIRST or -DLSH_FIRST
**         should be specified when compiling this source.
**
**  Input Parameters:
*/
    double param; /* input value for which the arcsine is desired */
/*
**  Output Parameters: none
**
**  Return value: returns the double-precision arcsine of the
**                input argument.  The result is in the range
**                pi/2 to -pi/2 inclusive.
**
**  Called by:  any
** 
**  History:
**
**        Version 1.0 - 30 June 1989        J. Hurst
** 
**  Copyright: 
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
    double x_sqr, x_dif, x2, y2; 


    volatile double y0, y1, ytemp, x0, x1, xtemp, work;

    long int exp;

/*******************************
** External functions         **
*******************************/
#ifdef IEEE
    extern double qtc_error();
#else
    extern double qtc_err();
#endif
    extern const unsigned long int _LPI_over_2[];
    extern double qtc_dsqrt();
    extern double qtc_datan();

    volatile double x = param;

/*******************************
** Function body              **
*******************************/
    /*
    ** Extract the exponent
    */
    exp = MSH(x) & EXP_MASK;
    /*
    ** Check for a denormalized value.  At this point, any exponent
    ** of zero signals either a zero or a denormalized value.
    ** In either case, return the input value.
    */
    if ( exp == 0L )
      return(x);

    /*
    ** Now start special-case handling
    */
#ifdef IEEE
    /*
    ** If the exponent equals EXP_MASK, the value is either 
    ** an infinity or NaN
    */
    if ( exp == EXP_MASK ) {
      /*
      ** If the fraction bits are all zero, this is an infinity
      */
      if ( ( ( MSH(x) & FRAC_MASK) | LSH(x) ) == 0L ) {
        /*
        ** For infinity, call the error handler with the infinity signal
        */
        return(qtc_error(DASIN_INFINITY,x));

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
          return(qtc_error(DASIN_INVALID_OPERATION,x));
        /*
        ** For quiet NaNs, call the error code with the qNaN signal
        */
        else
          return(qtc_error(DASIN_QNAN,x));
      }
    }
#endif /* end of IEEE special case handling */

    /*
    **  Test that the argument is within the range of arcsine
    */
    if ( ( x > 1.0) || (x < -1.0) ) {
#ifdef IEEE
        return(qtc_error(DASIN_OUT_OF_RANGE,x));
#else
        return(qtc_err(DASIN_OUT_OF_RANGE,x));
#endif
    }
    
    /*
    **  Compute the arcsine of x.  If x is less than 0.9, use the simple
    **  formula x_dif = 1.0 - x * x.
    */
    if ((x < 0.9) && (x > -0.9) ) {
       x_sqr  = x*x;
       x_dif  = 1.0 - x_sqr;
    }
    /*
    **  If x is close to 1.0, compute x squared using partial sums to get 
    **  more precision than is available from the double precision multiply.
    */
    else {

        MSH(y0) = MSH(x);
        LSH(y0) = 0;
        ytemp = x - y0;
        MSH(y1) = MSH(ytemp);
        LSH(y1) = 0;
        y2 = ytemp - y1; 

        MSH(x0) = MSH(x) & INT2_MASK;
        LSH(x0) = 0; 
        xtemp = x - x0;
        MSH(x1) = MSH(xtemp) & INT2_MASK;
        LSH(x1) = 0; 
        x2 = xtemp - x1; 
        /*
        **  Now x is represented by two separate sequences, such that
        **  x = x0 + x1 + x2 = y0 + y1 + y2.  Further, the number of bits of
        **  precision in each component is such that the order of the following
        **  sequence of subtraction assures no loss of accuracy.
        */

        x_dif =  1.0 - y0 * x0;
        x_dif -= (y0 * x1);
        x_dif -= (y1 * x0);
        x_dif -= (y0 * x2);
        x_dif -= (y1 * x1);
        x_dif -= (y2 * x0);
        x_dif -= (y1 * x2);
        x_dif -= (y2 * x1);
        x_dif -= (y2 * x2);
    } 
    work   = qtc_dsqrt(x_dif);

    /*
    **  If work is 0.0, it means x = +/-1.0, so just return pi over 2.
    */
    if ( work == 0.0) {
       work = DOUBLE(_LPI_over_2);
       MSH(work) = MSH(work) | (MSH(x) & SIGN_MASK);
       return(work);
    }

    return(qtc_datan(x/work));
    

}   /* end of qtc_dasin() routine */

/*** end of qtc_dasin.c ***/
