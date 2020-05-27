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

/*** qtc_dsinh.c ***/

#include "qtc_intr.h"
#include "qtc.h"
#include <math.h>

/*
******************************************************************************
**
*/
FUNCTION double (qtc_dsinh)(param)
/*
**  Purpose: This is a hyperbolic sin (sinh) function.
**
**           Accuracy - less than 1 ULP error
**
**           Valid input range - valid range for qtc_dexp()
**
**           Special case handling - IEEE:
**
**             qtc_dsinh(sNaN) - calls qtc_error() with DSINH_INVALID_OPERATION,
**                               input value
**             qtc_dsinh(qNaN) - calls qtc_error() with DSINH_QNAN, input value
**             qtc_dsinh(+INF) - calls qtc_error() with DSINH_INFINITY, input 
**                               value
**             qtc_dsinh(-INF) - calls qtc_error() with DSINH_INFINITY, input
**                               value
**             qtc_dsinh(DEN)  - returns correct value
**             qtc_dsinh(+-0)  - returns correct value
**             qtc_dsinh(+OOR) - calls qtc_error() with DSINH_OUT_OF_RANGE,
**                               input value
**             qtc_dsinh(-OOR) - calls qtc_error() with DSINH_OUT_OF_RANGE,
**                               input value
**
**           Special case handling - non-IEEE:
**
**             qtc_dsinh(+OOR) - calls qtc_err() with DSINH_OUT_OF_RANGE,
**                               input value
**             qtc_dsinh(-OOR) - calls qtc_err() with DSINH_OUT_OF_RANGE,
**                               input value
**
**  Method:
**
**        This function is based on the formula
**
**          sinh(x) = 1/2 ( exp(x) - exp(-x) )
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
    double param; /* input value for which the hyperbolic sin is desired */
/*
**  Output Parameters: none
**
**  Return value: returns the double-precision hyperbolic sin of the
**                input argument
**
**  Called by:  any
** 
**  Calls:	qtc_dexp
**		qtc_dexpm1 
**              qtc_error
**              qtc_err
** 
**  History:
**
**        Version 1.0 - 19 April 1989   J. Hurst
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
    double exp_plus, exp_minus, sinh_result;

/*******************************
**  Global variables          **
*******************************/
    extern const unsigned long int _Ldexp_threshold_1[];
    extern const unsigned long int _Ldexp_threshold_2[];

/*******************************
**  External functions        **
*******************************/
#ifdef IEEE
    extern double (qtc_error)();
#else
    extern double (qtc_err)();
#endif
    extern double (qtc_dexp)();
    extern double (qtc_dexpm1)();

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
      if ( ( ( MSH(x) & FRAC_MASK ) | LSH(x) ) == 0L ) {
        /*
        ** For negative infinity, signal negative infinity
        */
        if ( MSH(x) >= SIGN_MASK ) 
          return (qtc_error(DSINH_INFINITY,x));
        /*
        ** For positive infinity, signal infinity
        */
        else
          return (qtc_error(DSINH_INFINITY,x));
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
          return (qtc_error(DSINH_INVALID_OPERATION,x));
        /*
        ** For quiet NaNs, call the error code with the qNaN signal
        */
        else
          return (qtc_error(DSINH_QNAN,x));
      }
    }
    else
      /*
      ** Check for a denormalized value.  For this case, any exponent
      ** of zero (which signals either +/- 0 or a denormalized value)
      ** will result in a return value of approximately 0.0
      */
      if ( x_exp == 0L )
        return (x);
  
#endif /* end of IEEE special case handling */

    /*
    ** Exceptional cases (overflow and underflow)
    */
    if ( ( x > DOUBLE(_Ldexp_threshold_1) )||( x < DOUBLE(_Ldexp_threshold_2) ) )
#ifdef IEEE
      return (qtc_error(DSINH_OUT_OF_RANGE,x));
#else
      return (qtc_err(DSINH_OUT_OF_RANGE,x));
#endif

    /*
    **  Reaching this point means the input is valid.  Now calculate  
    **  sinh using the formula:
    **     
    **     sinh(x) = 1/2 ( exp(x) - exp(-x) )a
    ** 
    **  However, since exp(x) = 1 + x/1! + x**2/2! + x**3/3! ...,  
    **  this reduces to (1 + x/1! + x**2/2! + x**3/3! ...) - 
    **                  (1 - x/1! - x**2/2! - x**3/3! ...)  
    ** 
    **  This causes cancellation on the high order terms, so when x is
    **  very small, the calculated error is high.  To avoid this, at values
    **  near zero, the formula:
    ** 
    **      sinh(x) = ( (exp(x) - 1) - (exp(x) - 1) )/2 is used so that the
    ** 
    **  cancellation does not occur.  Since the subtracted ones cancel out,
    **  the correct value is calculated.
    **/ 

    /*
    **  See if the number is small enough to cause trouble 
    **/
    if ((x > (-0.5)) && (x < (0.5))) {
       /*
       **  Put exp(x) in exp_plus.
       **/
       exp_plus = qtc_dexpm1(x);
       /*
       **  Put exp(-x) in exp_minus.
       */
       x = -x;
       exp_minus = qtc_dexpm1(x);
    }
    else {
       /*
       **  Put exp(x) in exp_plus.
       **/
       exp_plus = qtc_dexp(x);
   
       /*
       **  Put exp(-x) in exp_minus.
       **/
       x = -x;
       exp_minus = qtc_dexp(x);
    }
    /*
    **  Put sinh = 1/2 ( exp(x) - exp(-x) ) in sinh_result.
    **/
    sinh_result = 0.5 * (exp_plus - exp_minus);
    /*
    **  Return sinh_result.
    **/
    return(sinh_result);

}   /* end of qtc_dsinh() routine */

/*** end of qtc_dsinh.c ***/
