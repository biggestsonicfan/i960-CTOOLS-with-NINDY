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

/*** qtc_dtanh.c ***/

#include "qtc_intr.h"
#include "qtc.h"

#define SMALL_THRESHOLD 0x3E400000L
#define LARGE_THRESHOLD 0x40331000L
#define FORM2_THRESHOLD 0x3FE80000L

/*
******************************************************************************
**
*/
FUNCTION double qtc_dtanh(param)
/*
**  Purpose: This is a hyperbolic tangent (tanh) function.
**
**           Accuracy - less than 2 ULPs error
**
**           Valid input range - All floating-point numbers
**
**           Special case handling - IEEE:
**
**             qtc_dtanh(sNaN)   - calls qtc_error() with
**                                 DTANH_INVALID_OPERATION, input value 
**             qtc_dtanh(qNaN)   - calls qtc_error() with
**                                 DTANH_QNAN, input value 
**             qtc_dtanh(+/-INF) - returns +-1.0
**             qtc_dtanh(+/-DEN) - returns +-DEN
**             qtc_dtanh(+/-0.0) - returns +-0.0
**
**           Special case handling - non-IEEE:
**
**             None
**
**  Method:
**
**         This function is based on the formula
**
**           tanh(x) = ( exp(x) - exp(-x) ) / ( exp(x) + exp(-x) )
**
**         Multiplying this formula by exp(x) / exp(x) gives
**
**           tanh(x) = ( exp(2x) - 1 ) / ( exp(2x) + 1 )
**
**         Using the expm1 function to avoid the case where
**         exp(2x) is near 1, this formula becomes
**
**           tanh(x) = expm1(2x) / ( expm1(2x) + 2 )
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
    double param; /* input value for which the hyperbolic tangent is desired */
/*
**  Output Parameters: none
**
**  Return value: returns the double-precision hyperbolic tangent of the
**                input argument
**
**  Calls:      qtc_dexpm1()
**              qtc_error()
** 
**  Called by:  any
** 
**  History:
**
**        Version 1.0 - 10 May 1989, 	Brent Leback
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
#ifdef IEEE
    long int x_exp;
#endif
    double twox, expx, tanh_result;

/*******************************
**  Global variables          **
*******************************/
   extern const unsigned long int _Ldpone[];
   extern const unsigned long int _Ldptwo[];

/*******************************
**  External functions        **
*******************************/
#ifdef IEEE
    extern double qtc_error();
#endif
    extern double qtc_dexpm1();

/*******************************
** Function body              **
*******************************/

    volatile double x = param;

#ifdef IEEE
    /*
    ** Extract the exponent
    */
    x_exp = MSH(x) & EXP_MASK;
    /*
    ** If the exponent is 0x7FF, the value is either an infinity or NaN
    */
    if ( x_exp == EXP_MASK ) {
      /*
      ** If the fraction bits are all zero, this is an infinity
      */
      if ( ( ( MSH(x) & FRAC_MASK ) | LSH(x) ) == 0x0L ) {
        /*
        ** For positive infinity, return 1.0
        */
        if ( (MSH(x) & SIGN_MASK) == 0x0L ) 
          return (1.0);
        /*
        ** For negative infinity, return -1.0
        */
        else
          return (-1.0);
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
          return (qtc_error(DTANH_INVALID_OPERATION,x));
        /*
        ** For quiet NaNs, call the error code with the qNaN signal
        */
        else
          return (qtc_error(DTANH_QNAN,x));
      }
    }
  
#endif /* end of IEEE special case handling */

    /*
    **  See if the magnitude of x is so small that exp(x) 
    **  and exp(-x) near 1.0, expm1(2x) nears 2x, and
    **  expm1(2x)/(expm1(2x)+2) = 2x / 2 = x
    */
    if ( (MSH(x) & ABS_MASK) <= SMALL_THRESHOLD)
        return(x);

    /*
    **  See if the magnitude is so large that exp(-x) is 
    **  insignificant in comparison with exp(x).  For positive x,
    **  expm1(2x)+2 = expm1(2x).  For negative x, expm1(2x) = -1.
    */
    if ( (MSH(x) & ABS_MASK) > LARGE_THRESHOLD) {
        if ( (MSH(x) & SIGN_MASK) == 0x0L ) 
            return(1.0);
        else
            return(-1.0);
    }

    /*
    **  Basic strategy is:
    **    For better accuracy, call expm1 with positive input 
    **      values.  
    **    For larger magnitudes of x, the formula
    **        tanh(x) = 1 - 2 / ( expm1(2x) + 2 )
    **      yields better results.
    */
    else {
        if ( (MSH(x) & ABS_MASK) < FORM2_THRESHOLD) {
            if ( (MSH(x) & SIGN_MASK) == 0x0L ) { 
                /*
                ** Use:  expm1(2x) / ( expm1(2x) + 2 )
                */
                twox = DOUBLE(_Ldptwo) * x;
                expx = qtc_dexpm1(twox);
                tanh_result = expx / (expx + DOUBLE(_Ldptwo));
            }
            else {
                /*
                ** Use:  - ( expm1(-2x) / ( expm1(-2x) + 2 ) )
                */
                MSH(x) = MSH(x) ^ SIGN_MASK;
                twox = DOUBLE(_Ldptwo) * x;
                expx = qtc_dexpm1(twox);
                tanh_result = expx / (expx + DOUBLE(_Ldptwo));
                MSH(tanh_result) = MSH(tanh_result) | SIGN_MASK;
            }
        }
        else {
            if ( (MSH(x) & SIGN_MASK) == 0x0L ) { 
                /*
                ** Use:  1 - ( 2 / ( expm1(2x) + 2 ) )
                */
                twox = DOUBLE(_Ldptwo) * x;
                expx = qtc_dexpm1(twox);
                tanh_result = DOUBLE(_Ldpone) -
                              ( DOUBLE(_Ldptwo) / (expx + DOUBLE(_Ldptwo)) );
            }
            else {
                /*
                ** Use:  ( 2 / ( expm1(-2x) + 2 ) ) - 1
                */
                MSH(x) = MSH(x) ^ SIGN_MASK;
                twox = DOUBLE(_Ldptwo) * x;
                expx = qtc_dexpm1(twox);
                tanh_result = ( DOUBLE(_Ldptwo) / (expx + DOUBLE(_Ldptwo)) )
                                                      - DOUBLE(_Ldpone);
            }
        }
        return(tanh_result);
    }

}   /* end of qtc_dtanh() routine */

/*** end of qtc_dtanh.c ***/
