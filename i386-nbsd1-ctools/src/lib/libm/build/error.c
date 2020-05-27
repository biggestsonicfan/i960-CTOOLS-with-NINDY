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

/*** qtc_error.c ***/

#include "qtc_intr.h"
#include "qtc.h"
#include <math.h>
#include <errno.h>
#include <fpsl.h>

/*
** The following macros should be defined to provide the appropriate
** system signals as desired
*/
#define SIGNAL_DIVBYZERO fp_setflags(FPX_INVOP) /* 1.0/zero */
#define SIGNAL_INVALID   fp_setflags(FPX_ZDIV) /* zero/zero */

#define is_qNaN(x) (((MSH(x)&0x7FF80000L) == 0x7FF80000L)?1:0)
#define qNaN(x) (MSH(x) |= 0x7FF80000L)

#define QNAN_MASK 0x7FF80000L
#define REST_MASK 0x0007FFFFL

/*
******************************************************************************
**
*/
FUNCTION double qtc_error(code,param1,param2)
/*
**  Purpose: Performs error handling for the QTC Intrinsics, 
**           IEEE case, double precision.  
**           
**         One of the compile line switches -DMSH_FIRST or -DLSH_FIRST should
**         be specified when compiling this source.
**
**  Input Parameters:
*/
    int code;  /* error code indicating calling function and error          */
    double param1;  /* 1st input argument of the calling function                */
    double param2;  /* 2nd input argument of the calling function, if applicable */
/*
**  Output Parameters: none
**
**  Return value:  an appropriate value to be returned by
**                 the calling function
**
**  Called by:  the QTC Intrinsic functions
** 
**  History:
**      Version 1.0 - 5 July 1988, J F Kauth
**              1.1 - 26 October 1988, L A Westerman
**              1.2 - 29 November 1988, J F Kauth, changed the declaration
**                    of input argument "code" from unsigned long int to int
**                    to be consistent with how this routine is called.
**                    Also, added code to avoid integer overflow when checking
**                    y for an odd integer.
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
#ifdef MSH_FIRST
    static const unsigned long int qnan[2]          = {0x7FF80000L, 0x00000000L};
    static const unsigned long int infinity[2]      = {0x7FF00000L, 0x00000000L};
    static const unsigned long int minus_infinity[2]= {0xFFF00000L, 0x00000000L};
    static const unsigned long int negative_zero[2] = {0x80000000L, 0x00000000L};
#else
    static const unsigned long int qnan[2]          = {0x00000000L, 0x7FF80000L};
    static const unsigned long int infinity[2]      = {0x00000000L, 0x7FF00000L};
    static const unsigned long int minus_infinity[2]= {0x00000000L, 0xFFF00000L};
    static const unsigned long int negative_zero[2] = {0x00000000L, 0x80000000L};
#endif
    static const double zero = 0.0;

    long int iy;
    unsigned long int error;
    int exp_y, y_odd_integer;

/*******************************
** Function body              **
*******************************/
    volatile double x = param1;
    volatile double y = param2;
    error = error_mask & code;
    switch (fcn_mask & code) {
        case DATAN:
            if ( error == QNAN ) {
                return(x);
            }
            else if ( error == INVALID_OPERATION ) {
                /*
                ** Change x from snan to qnan
                */
                qNaN(x);
                SIGNAL_INVALID;
                return(x);
            }
            break;
        case DATAN2:
            if ( error == OUT_OF_RANGE ) {
		errno = EDOM;
                SIGNAL_INVALID;
                return(0.0);
            }
	    break;
        case DEXP:
            if ( (error_mask & code) == OUT_OF_RANGE ) {
		errno = ERANGE;
                if (x < 0.0) {               /* if x < 0.0 */
                    return(0.0);
                }
                else {                       /* else, x > 0.0 */
                    return (HUGE_VAL);
                }
            }
            else if ( error == QNAN ) {
                return(x);
            }
            else if ( error == INVALID_OPERATION ) {
                /*
                ** Change x from snan to qnan
                */
                qNaN(x);
                SIGNAL_INVALID;
                return(x);
            }
            else if ( error == INFINITY ) {
                return(x);
            }
            break;
        case DHYPOT:
            if ( error == OUT_OF_RANGE ) {
                return(DOUBLE(infinity));
            }
            else if ( error == QNAN ) {
                if ( is_qNaN(x) )
                   return(x);
                else
                   return(y);
            }
            else if ( error == INFINITY ) {
                return(DOUBLE(infinity));
            }
            else if ( error == INVALID_OPERATION ) {
                if ( ((MSH(x) & 0x7FF80000L) == 0x7FF00000L) &&
                     (((MSH(x) & 0x0007FFFFL) | LSH(x) ) != 0L ) ) {
                   /*
                   ** x = SNAN
                   */
                    qNaN(x);
                    SIGNAL_INVALID;
                    return(x);
                }
                else {
                    /*
                    ** y is SNAN
                    */
                    qNaN(y);
                    SIGNAL_INVALID;
                    return(y);
                }
            }
            break;
        case DACOS:
        case DASIN:
            if ( ( error == OUT_OF_RANGE ) || ( error == INFINITY) ) {
                /*
                **  Change x into a qNaN and signal invalid 
                */
		errno = EDOM;
                return(0.0);
            }
            else if ( error == QNAN ) {
                return(x);
            }
            else if ( (error == INVALID_OPERATION) ) {
                /*
                ** Change x from snan or infinity to qnan
                */
                qNaN(x);
                SIGNAL_INVALID;
                return(x);
            }
            break;
        case DCOS:
        case DSIN:
        case DTAN:
            if ( error == OUT_OF_RANGE ) {
                return(0.0);
            }
            else if ( error == QNAN ) {
                return(x);
            }
            else if ( (error == INVALID_OPERATION) || (error == INFINITY) ) {
                /*
                ** Change x from snan or infinity to qnan
                */
                qNaN(x);
                SIGNAL_INVALID;
                return(x);
            }
            break;
        case DCOSH:
            if ( (error_mask & code) == OUT_OF_RANGE ) {
		    errno = ERANGE;
                    return (HUGE_VAL);
            }
            else if ( error == QNAN ) {
                return(x);
            }
            else if ( error == INVALID_OPERATION ) {
                /*
                ** Change x from snan to qnan
                */
                qNaN(x);
                SIGNAL_INVALID;
                return(x);
            }
            else if ( error == INFINITY ) {
		errno = ERANGE;
                return (DOUBLE(infinity));
            }
            break;
        case DTANH:
            if ( error == QNAN ) {
                return(x);
            }
            else if ( error == INVALID_OPERATION ) {
                /*
                ** Change x from snan to qnan
                */
                qNaN(x);
                SIGNAL_INVALID;
                return(x);
            }
            break;
        case DLOG:
            if ( error == OUT_OF_RANGE ) {
                /*
                ** x <= 0.0
                */
                if (x == 0.0) {
		    errno = ERANGE;
                    SIGNAL_DIVBYZERO;
                    return(-DOUBLE(infinity));
                }
                else {
		    errno = EDOM;
                    SIGNAL_INVALID;
                    return(DOUBLE(qnan));
                }
            }
            else if ( error == QNAN ) {
                return(x);
            }
            else if ( error == INVALID_OPERATION ) {
                /*
                ** Change x from snan to qnan
                */
                qNaN(x);
                SIGNAL_INVALID;
                return(x);
            }
            else if ( error == INFINITY ) {
                return(x);
            }
            break;
        case DPOW:
            if ( error == OUT_OF_RANGE ) {
                /*
                ** exp(y*log(x)) will over or underflow
                */
                if ( (y < 0.0 && x > 1.0) || (y > 0.0 && x < 1.0) ) {
                    /*
                    ** x,y out of range and y*log(x) < 0.0
                    */
		    errno = ERANGE;
                    return(0.0);
                }
                else {
                    /*
                    ** x,y out of range and y*log(x) > 0.0
                    */
		    errno = ERANGE;
                    return (HUGE_VAL);
                }
            }
            else if ( error == QNAN ) {
                /*
                ** x = QNAN or y = QNAN
                */
                if (is_qNaN(x)) {
                   return(x);
                }
                else {
                   return(y);
                }
            }
            else if ( error == INVALID_OPERATION ) {
                /*
                ** x = SNAN or
                ** y = SNAN or
                ** x = +/- 1.0, y = +/- infinity or
                ** x < 0.0 and y is not an integer
                */
                if ( ((MSH(x) & 0x7FF80000L) == 0x7FF00000L) &&
                     (((MSH(x) & 0x0007FFFFL) | LSH(x) ) != 0L ) ) {
                   /*
                   ** x = SNAN
                   */
                    qNaN(x);
                    SIGNAL_INVALID;
                    return(x);
                }
                else if ((MSH(y) & 0x7FF00000L) == 0x7FF00000L) {
                    /*
                    ** y is SNAN or +/-infinity
                    */
                    qNaN(y);
                    SIGNAL_INVALID;
                    return(y);
                }
                else {
		    errno = EDOM;
                    SIGNAL_INVALID;
                    return(DOUBLE(qnan));
                }
            }
            else if ( error == INFINITY ) {
                /*
                ** |x| > 1,   y == +inf,  or
                ** |x| < 1,   y == -inf,  or
                ** x == +inf, y > 0,      or
                ** x == -inf, y == any
                */
                if ( (MSH(y) & 0x7FFFFFFFL) == 0x7FF00000L ) {
                    /*
                    ** y == +-inf
                    ** |x| > 1,   y == +inf,  or
                    ** |x| < 1,   y == -inf
                    */
                    return(DOUBLE(infinity));
                }
                else if (MSH(x) == 0x7FF00000L) {
                    /*
                    ** x == +inf, y > 0
                    */
                    return(DOUBLE(infinity));
                }
                else {
                    /*
                    ** x == -inf, y == any
                    ** if y a negative odd integer, return -0.0
                    ** if y a positive odd integer, return -infinity and 
                    **                              signal divide by zero
                    ** if y > 0, return +infinity and signal divide by zero
                    ** if y < 0, return +0.0
                    ** 
                    ** Check for odd y, but avoid integer overflow
                    ** First, check for y less than the maximum integer
                    ** (Note that if y >= 2**53, y is an even integer.)
                    */
                    y_odd_integer = 0;
                    exp_y = ( MSH(y) & 0x7FF00000L ) >> 20 ;
                    if ( exp_y < 0x41E ) {
                      iy = (int)y;
                      if ( ((double)iy == y) && ((iy & 1L) == 1L) )
                        y_odd_integer = 1;
                    }
                    /*
                    ** Above the integer limit, check the trailing bits of the
                    ** mantissa, including the unity bit and the fraction bits
                    */
                    else if ( exp_y < 0x434 ) {
                      if ( (LSH(y)<<(10 + (exp_y - 0x41E)))
                              == (unsigned long int) 0x80000000L )
                        y_odd_integer = 1;
                    }
                    if ( y_odd_integer ) {
                        if ( ( MSH(y) & 0x80000000L ) == 0x0L ) {
                            /*
                            ** if y > 0.0
                            */
                            SIGNAL_DIVBYZERO;
                            return(-DOUBLE(infinity));
                        }
                        else {                                /* else y < 0.0 */
                            return(DOUBLE(negative_zero));
                        }
                    }
                    else if ( ( MSH(y) & 0x80000000L ) == 0x0L ) {
                        /*
                        ** y is not an odd integer and y > 0.0
                        */
                        SIGNAL_DIVBYZERO;
                        return(DOUBLE(infinity));
                    }
                    else {
                        /*
                        ** y is not an odd integer and y < 0.0
                        */
                        return(0.0);
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
                if ( MSH(x) == (unsigned long int) 0x80000000L ) {
                  /*
                  ** If x == -0.0, check for odd y, but avoid integer overflow
                  ** First, check for y less than the maximum integer
                  */
                  exp_y = ( MSH(y) & 0x7FF00000L ) >> 20 ;
                  if ( exp_y < 0x41E ) {
                    iy = (int)y;
                    if ( ((double)iy == y) && ((iy & 1L) == 1L) ) {
                      /*
                      ** y is an odd integer
                      */
                        SIGNAL_DIVBYZERO;
                        return(-DOUBLE(infinity));
                    }
                  }
                  else
                  /*
                  ** Above the integer limit, check the trailing bits of the
                  ** mantissa, including the unity bit and the fraction bits
                  */
                  if ( exp_y < 0x434 ) {
                    if ( (LSH(y)<<(10 + (exp_y - 0x41E)))
                            == (unsigned long int) 0x80000000L ) {
                      /*
                      ** y is an odd integer
                      */
                        SIGNAL_DIVBYZERO;
                        return(-DOUBLE(infinity));
                    }
                  }
                }
		errno = EDOM;
                SIGNAL_DIVBYZERO;
                return(DOUBLE(infinity));
            }
            break;
        case DSINH:
            if ( (error_mask & code) == OUT_OF_RANGE ) {
		errno = ERANGE;
                if (x < 0.0) {               /* if x < 0.0 */
                    return (-HUGE_VAL);
                }
                else {                       /* else, x > 0.0 */
                    return (HUGE_VAL);
                }
            }
            else if ( error == QNAN ) {
                return(x);
            }
            else if ( error == INVALID_OPERATION ) {
                /*
                ** Change x from snan to qnan
                */
                qNaN(x);
                SIGNAL_INVALID;
                return(x);
            }
            else if ( error == INFINITY ) {
                errno = ERANGE;
                return(x);
            }
            break;
        case DSQRT:
            if ( error == OUT_OF_RANGE ) {
                /*
                ** x < 0.0
                */
		errno = EDOM;
                SIGNAL_INVALID;
                return(DOUBLE(qnan));
            }
            else if ( error == QNAN ) {
                return(x);
            }
            else if ( error == INVALID_OPERATION ) {
                /*
                ** Change x from snan to qnan
                */
                qNaN(x);
                SIGNAL_INVALID;
                return(x);
            }
            else if ( error == INFINITY ) {
                return(x);
            }
            break;
    }
    /*
    ** If fall through to here, received an unexpected error code
    */
    return (0.0);
}
/*** end of qtc_error.c ***/
