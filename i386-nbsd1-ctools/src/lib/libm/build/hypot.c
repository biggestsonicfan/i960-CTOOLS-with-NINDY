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

/*** qtc_dhypot.c ***/

#include "qtc_intr.h"
#include "qtc.h"
#include "qtc_cons.h"

/*
************************************************************************
**
*/
FUNCTION double qtc_dhypot(param1, param2)
/*
**  Purpose  This is the hypotenuse function, sqrt(x*x+y*y)
**
**           Accuracy - less than 1.5 ulp error
**
**           Valid input range - all floating point numbers, but may
**                               overflow for some input arguments
**
**           Special case handling - IEEE:
**
**             qtc_dhypot(sNaN,any)         - calls qtc_error() with 
**                                          DHYPOT_INVALID_OPERATION, input value.
**             qtc_dhypot(!sNaN,sNaN)       - calls qtc_error() with 
**                                          DHYPOT_INVALID_OPERATION, input value.
**             qtc_dhypot(qNaN,!sNaN)       - calls qtc_error() with DHYPOT_QNAN,
**                                          input value.
**             qtc_dhypot(!NaN,qNaN)        - calls qtc_error() with DHYPOT_QNAN,
**                                          input value.
**             qtc_dhypot(+-INF,!NaN)       - calls qtc_error() with 
**                                          DHYPOT_INFINITY, input value.
**             qtc_dhypot(!NaN,+-INF)       - calls qtc_error() with 
**                                          DHYPOT_INFINITY, input value.
**             qtc_dhypot(DEN,!(NaN||INF))  - correct value
**             qtc_dhypot(!(NaN||INF),DEN)  - correct value
**             qtc_dhypot(+-0,!(NaN||INF))  - correct value
**             qtc_dhypot(!(NaN||INF),+-0)  - correct value
**             qtc_dhypot(finite,finite)    - correct value
**             qtc_dhypot(+-OOR,+-OOR)      - calls qtc_error() with
**                                          DHYPOT_OUT_OF_RANGE, input value.
**
**           Special case handling - non-IEEE:
**
**             qtc_dhypot(+OOR)             - calls qtc_err() with
**                                          DHYPOT_OUT_OF_RANGE, input value.
**
**  Method: 
**
**        This function handles the IEEE special cases and then
**        computes the square root of x*s+y*y.  To preserve
**        accuracy when the input argument is large or small,
**        scaling is used according to the formula:
**           dhypot(x) = ( x**2 + y**2 ) ** 1/2
**                   = 2**-k * 2**k * ( x**2 + y**2 ) ** 1/2
**                   = 2**-k * ( x**2 * 2**(2k) + y**2 * 2**(2k) ) ** 1/2
**                   = 2**-k * ( (x*(2**k))**2 + (y*(2**k))**2 ) ** 1/2
**        To reduce the number of floating point calculations,
**        multiplying by 2**-k and 2**k is accomplished by adding
**        the offset k to the exponent.
**
**  Notes:
**         When compiled with the -DIEEE switch, this routine properly
**         handles NaNs and infinities.
**
**         One of the compile line switches -DMSH_FIRST or -DLSH_FIRST should
**         be specified when compiling this source.
**
**  Input Parameters:
*/
    double param1; /* first input value */
    double param2; /* second input value */
/*
**  Output Parameters: none
**
**  Return value: returns the square root of the sum of the squares of the
**                two input arguments
**
**  Calls:      qtc_util_dhypot()
**              qtc_error()
**              qtc_err()
**
**  Called by:  any
** 
**  History:
**
**        Version 1.0 -  7 September 1989, Larry Westerman
** 
**  Copyright: 
**      (c) 1989 Quantitative Technology Corporation.
**               8700 SW Creekside Place Suite D
**               Beaverton OR 97005 USA
**               503-626-3081
**      Intel usage with permission under contract #89B0090
** 
************************************************************************
*/
{
/*******************************
** Internal Data Requirements **
*******************************/
    double result;
    int overflow_flag;

#ifdef IEEE
    unsigned long int x_exp, y_exp;
    unsigned long int x_frac, y_frac;
#endif

/*******************************
**  External functions        **
*******************************/
#ifdef IEEE
    extern double qtc_error();
#else
    extern double qtc_err();
#endif
    extern double qtc_util_dhypot();

/*******************************
** Function body              **
*******************************/

volatile double x = param1;
volatile double y = param2;

#ifdef IEEE
    /*
    ** Extract the exponent and fractional part of both x and y
    */
    x_exp  =  MSH(x) & EXP_MASK;
    y_exp  =  MSH(y) & EXP_MASK;
    x_frac =  MSH(x) & FRAC_MASK | LSH(x);
    y_frac =  MSH(y) & FRAC_MASK | LSH(y);

    /*
    **  check the special case list in order
    */
    if ( (x_exp == EXP_MASK) && (x_frac != 0L) && (is_sNaN(x)) ) {
       /*
       **  1.  qtc_dhypot (sNaN, any) => INVALID_OPERATION.
       */
       return (qtc_error(DHYPOT_INVALID_OPERATION,x,y));
    }
    else if ( (y_exp == EXP_MASK) && (y_frac != 0L) && 
              (is_sNaN(y)) ) {
       /*
       **  2.  qtc_dhypot (!sNaN, sNaN) => INVALID_OPERATION.
       */
       return (qtc_error(DHYPOT_INVALID_OPERATION,x,y));
    }
    else if ( (x_exp == EXP_MASK) && (x_frac != 0L) ) {
       /*
       **  3.  qtc_dhypot (qNaN, !sNaN) => QNAN.
       */
       return (qtc_error(DHYPOT_QNAN,x,y));
    }
    else if ( (y_exp == EXP_MASK) && (y_frac != 0L) ) {
       /*
       **  4.  qtc_dhypot (!NaN, qNaN) => QNAN.
       */
       return (qtc_error(DHYPOT_QNAN,x,y));
    }
    else if (x_exp == EXP_MASK) {
       /*
       **  5.  qtc_dhypot (+-INF, !NaN) => INFINITY.
       */
       return (qtc_error(DHYPOT_INFINITY,x,y));
    }
    else if (y_exp == EXP_MASK) {
       /*
       **  6.  qtc_dhypot (!NaN, +-INF) => INFINITY.
       */
       return (qtc_error(DHYPOT_INFINITY,x,y));
    }

#endif  /* end of IEEE special case handling */
    
    /*
    ** Call utility to get the absolute value, and
    ** check overflow status upon returning.
    */
    result = qtc_util_dhypot(x,y,&overflow_flag);
    if (overflow_flag == TRUE) {
#ifdef IEEE
        return (qtc_error(DHYPOT_OUT_OF_RANGE,x,y));
#else
        return (qtc_err(DHYPOT_OUT_OF_RANGE,x,y));
#endif
    }

    return(result);

}   /* end of qtc_dhypot() routine */

/*** end of qtc_dhypot.c ***/
