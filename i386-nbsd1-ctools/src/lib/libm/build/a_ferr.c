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

/*** qtc_ferror.c ***/

#include "qtc_intr.h"
#include "qtc.h"
#include <errno.h>

/*
** The following macros should be defined to provide the appropriate
** system signals as desired
*/
#define SIGNAL_DIVBYZERO /* 1.0/zero */
#define SIGNAL_INVALID   /* zero/zero */

typedef volatile union long_float_type
{
    unsigned long int i;
    float f;
} long_float;

    
/*
******************************************************************************
**
*/
FUNCTION float qtc_ferror(int code,float x,float y)
/*
**  Purpose: Performs error handling for the QTC Intrinsics, 
**           IEEE case, single-precision.  
**           
**  Input Parameters:
*/
/*    int code;  error code indicating calling function and error          */
/*    float x;  1st input argument of the calling function                */
/*    float y;  2nd input argument of the calling function, if applicable */
/*
**  Output Parameters: none
**
**  Return value:  an appropriate value to be returned by
**                 the calling function
**
**  Called by:  the QTC Intrinsic functions, single-precision version
** 
**  History:  
**        Version 1.0 - 5 July 1988, by JF Kauth
**                1.1 - 6 January 1989, L A Westerman, cleaned up for release
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
    long_float infinity;
    long_float minus_infinity;
    long_float negative_zero;
    long_float qnan;
    long_float zero;

    long_float return_x;
    long_float return_y;

    long_float temp_return;

    int y_odd_integer;
    long int iy;
    unsigned long int error;

/*******************************
** Function body              **
*******************************/
    infinity.i       = 0x7F800000L;
    minus_infinity.i = 0xFF800000L;
    negative_zero.i  = 0x80000000L;
    qnan.i           = 0x7FC00000L;
    zero.i           = 0x00000000L;

    temp_return.f = x;
    return_x.i = temp_return.i;
    error = error_mask & code;
    switch (fcn_mask & code) {
        case ATAN:
            if ( error == QNAN ) {
                return (x);
            }
            else if ( error == INVALID_OPERATION ) {
                /*
                ** Change x from snan to qnan
                */
                return_x.i |= 0x7FC00000L;
                SIGNAL_INVALID;
                return (return_x.f);
            }
            break;
        case COS:
        case SIN:
        case TAN:
            if ( error == OUT_OF_RANGE ) {
                errno = EDOM;
                return (zero.f);
            }
            else if ( error == QNAN ) {
                return (x);
            }
            else if ( (error == INVALID_OPERATION) || (error == INFINITY) ) {
                /*
                ** Change x from snan or infinity to qnan
                */
                return_x.i |= 0x7FC00000L;
                SIGNAL_INVALID;
                return (return_x.f);
            }
            break;
        case EXP:
            if ( (error_mask & code) == OUT_OF_RANGE ) {
                if (return_x.f < 0.0) {               /* if x < 0.0 */
                    return (zero.f);
                }
                else {                       /* else, x > 0.0 */
                    return (infinity.f);
                }
            }
            else if ( error == QNAN ) {
                return (return_x.f);
            }
            else if ( error == INVALID_OPERATION ) {
                /*
                ** Change x from snan to qnan
                */
                return_x.i |= 0x7FC00000L;
                SIGNAL_INVALID;
                return (return_x.f);
            }
            else if ( error == INFINITY ) {
                return (x);
            }
            break;
        case LOG:
            if ( error == OUT_OF_RANGE ) {
                /*
                ** x <= 0.0
                */
                if (return_x.f == 0.0) {
                    SIGNAL_DIVBYZERO;
                    return (minus_infinity.f);
                }
                else {
                    SIGNAL_INVALID;
                    return (qnan.f);
                }
            }
            else if ( error == QNAN ) {
                return (x);
            }
            else if ( error == INVALID_OPERATION ) {
                /*
                ** Change x from snan to qnan
                */
                return_x.i |= 0x7FC00000L;
                SIGNAL_INVALID;
                return (return_x.f);
            }
            else if ( error == INFINITY ) {
                return (x);
            }
            break;
        case SQRT:
            if ( error == OUT_OF_RANGE ) {
                /*
                ** x < 0.0
                */
                SIGNAL_INVALID;
                return (qnan.f);
            }
            else if ( error == QNAN ) {
                return (x);
            }
            else if ( error == INVALID_OPERATION ) {
                /*
                ** Change x from snan to qnan
                */
                return_x.i |= 0x7FC00000L;
                SIGNAL_INVALID;
                return (return_x.f);
            }
            else if ( error == INFINITY ) {
                return (x);
            }
            break;
        case POW:
            return_y.f = y;
            if ( error == OUT_OF_RANGE ) {
                /*
                ** exp(y*log(x)) will over or underflow
                */
                if ( (return_y.f < 0.0 && return_x.f > 1.0) ||
                     (return_y.f > 0.0 && return_x.f < 1.0) ) {
                    /*
                    ** x,y out of range and y*log(x) < 0.0
                    */
                    return (zero.f);
                }
                else {
                    /*
                    ** x,y out of range and y*log(x) > 0.0
                    */
                    return (infinity.f);
                }
            }
            else if ( error == QNAN ) {
                /*
                ** x = QNAN or y = QNAN
                */
                if ( ( return_x.i & 0x7FC00000L ) == 0x7FC00000L ) {
                   return (x);
                }
                else {
                   return (y);
                }
            }
            else if ( error == INVALID_OPERATION ) {
                /*
                ** x = SNAN or
                ** y = SNAN or
                ** x = +/- 1.0, y = +/- infinity or
                ** x < 0.0 and y is not an integer
                */
                if ( ((return_x.i & 0x7FC00000L) == 0x7F800000L) &&
                     ((return_x.i & 0x003FFFFFL) != 0L ) ) {
                   /*
                   ** x = SNAN
                   */
                    return_x.i |= 0x7FC00000L;
                    SIGNAL_INVALID;
                    return (return_x.f);
                }
                else if ((return_y.i & 0x7F800000L) == 0x7F800000L) {
                    /*
                    ** y is SNAN or +/-infinity
                    */
                    return_y.i |= 0x7FC00000L;
                    SIGNAL_INVALID;
                    return (return_y.f);
                }
                else {
                    SIGNAL_INVALID;
                    return (qnan.f);
                }
            }
            else if ( error == INFINITY ) {
                /*
                ** |x| > 1,   y == +inf,  or
                ** |x| < 1,   y == -inf,  or
                ** x == +inf, y > 0,      or
                ** x == -inf, y == any
                */
                if ( (return_y.i & 0x7FFFFFFFL) == 0x7F800000L ) {
                    /*
                    ** y == +-inf
                    ** |x| > 1,   y == +inf,  or
                    ** |x| < 1,   y == -inf
                    */
                    return (infinity.f);
                }
                else if (return_x.i == 0x7F800000L) {
                    /*
                    ** x == +inf, y > 0
                    */
                    return (infinity.f);
                }
                else {
                    /*
                    ** x == -inf, y == any
                    ** if y a negative odd integer, return -0.0
                    ** if y a positive odd integer, return -infinity and 
                    **                              signal divide by zero
                    ** if y > 0, return +infinity and signal divide by zero
                    ** if y < 0, return +0.0
                    */ 
                    /*
                    ** Check for y an odd integer.
                    ** (Note that if y >= 2**24, y is an even integer.)
                    */
                    y_odd_integer = 0;
                    if ( (return_y.i & 0x7F800000L) < 0x4B800000L ) {
                        iy = (int) return_y.f;
                        if ( ((float)iy == return_y.f) && ((iy & 1L) == 1L) )
                            y_odd_integer = 1;
                    }
                    if ( y_odd_integer ) {
                        if ( ( return_y.i & 0x80000000L ) == 0x0L ) {
                            /*
                            **    y > 0.0
                            */
                            SIGNAL_DIVBYZERO;
                            return (minus_infinity.f);
                        }
                        else { /* y < 0.0 */
                            return (negative_zero.f);
                        }
                    }
                    else if ( ( return_y.i & 0x80000000L ) == 0x0L ) {
                        /*
                        ** y is not an odd integer and y > 0.0
                        */
                        SIGNAL_DIVBYZERO;
                        return (infinity.f);
                    }
                    else {
                        /*
                        ** y is not an odd integer and y < 0.0
                        */
                        return (zero.f);
                    }
                }
            }
            else if ( error == DIV_BY_ZERO ) {
                /*
                ** x = +-0.0; y < 0.0
                ** if x == +0.0, signal divide by zero and return +infinity
                ** if x == -0.0 and y is not an odd integer,
                **              signal divide by zero and return +infinity
                ** if x == -0.0 and y is an odd integer,
                **              signal divide by zero and return -infinity
                */
                if ( return_x.i == negative_zero.i ) {
                    /*
                    ** If x == -0.0, check for odd y
                    ** (Note that if y >= 2**24, y is an even integer.)
                    */
                    if ( (return_y.i & 0x7F800000L) < 0x4B800000L ) {
                        iy = (int) return_y.f;
                        if ( ((float)iy == return_y.f) && ((iy & 1L) == 1L) ) {
                            /*
                            ** y is an odd integer
                            */
                            SIGNAL_DIVBYZERO;
                            return (minus_infinity.f);
                        }
                    }
                }
                SIGNAL_DIVBYZERO;
                return (infinity.f);
        }
        break;
    }
    /*
    ** If fall through to here, received an unexpected error code
    */
    return (zero.f);
}
/*** end of qtc_ferror.c ***/
