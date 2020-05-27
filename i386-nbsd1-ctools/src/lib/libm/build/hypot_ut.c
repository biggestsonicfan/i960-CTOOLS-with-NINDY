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

/*** qtc_util_dhypot.c ***/

#include "qtc_intr.h"
#include "qtc.h"
#include "qtc_cons.h"

#define HBIT_MASK  0x80000000L

/*
************************************************************************
**
*/
FUNCTION double qtc_util_dhypot(param1, param2, overflow_flag)
/*
**  Purpose:  This is the utility hypotenuse routine
**
**  Valid input range:  All floating point numbers
**
**  Method: 
**        This utility computes the square root of x*x+y*y.
**        To preserve accuracy when the input argument is large or 
**        small, scaling is used according to the formula:
**           dhypot(x) = ( x**2 + y**2 ) ** 1/2
**                   = 2**-k * 2**k * ( x**2 + y**2 ) ** 1/2
**                   = 2**-k * ( x**2 * 2**(2k) + y**2 * 2**(2k) ) ** 1/2
**                   = 2**-k * ( (x*(2**k))**2 + (y*(2**k))**2 ) ** 1/2
**        To reduce the number of floating point calculations,
**        multiplying by 2**-k and 2**k is accomplished by adding
**        the offset k to the exponent.
**
**  Input Parameters:
*/
    double param1; /* first input value */
    double param2; /* second input value */
/*
**  Output Parameters:
*/
    int *overflow_flag; /* Pointer to overflow status flag */
/*
**  Return value: returns the positive square root of x*x + y*y
**
**  Calls:      qtc_dsqrt()
**
**  Called by:  qtc_cabs()
**              qtc_csqrt()
**              qtc_clog()
**              qtc_cpow()
**              qtc_dhypot()
** 
**  History:
**
**        Version 1.0 -  20 June 1989, 	Brent Leback
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

    double x_sq, y_sq, xsum;
    volatile double result, scaled_x, scaled_y, scaled_sqrt;

    long int x_exp, y_exp;
    long int x_frac, y_frac;
    long int expoffset, sqrt_exp;
    unsigned long int mfrac_x, lfrac_x;
    unsigned long int mfrac_y, lfrac_y;
    unsigned long int mfrac_rr, lfrac_rr;

/*******************************
**  Global variables          **
*******************************/
#ifdef IEEE
    extern const unsigned long int _Linfinity[];
#else
    extern const unsigned long int _Lxmax[];
#endif

/*******************************
**  External functions        **
*******************************/
    extern double qtc_dsqrt();

/*******************************
** Function body              **
*******************************/

    volatile double x = param1;
    volatile double y = param2;
    /*
    ** Initialize overflow flag
    */
    *overflow_flag = FALSE;

    /*
    ** Extract the exponent and fractional parts of x and y
    */
    x_exp  =  MSH(x) & EXP_MASK;
    y_exp  =  MSH(y) & EXP_MASK;
    x_frac =  MSH(x) & FRAC_MASK | LSH(x);
    y_frac =  MSH(y) & FRAC_MASK | LSH(y);

    /*
    ** Check each input for value for zero.
    ** If zero, return correct value.
    */
    if ( (x_exp | x_frac) == 0L) {
        MSH(result) = MSH(y) & ABS_MASK;
        LSH(result) = LSH(y);
        return(result);
    }
    if ( (y_exp | y_frac) == 0L) {
        MSH(result) = MSH(x) & ABS_MASK;
        LSH(result) = LSH(x);
        return(result);
    }

    /*
    ** Compute the value of each exponent and fraction
    */
    x_exp >>= MSH_FRACTION_WIDTH_DP;
    y_exp >>= MSH_FRACTION_WIDTH_DP;

    mfrac_x = MSH(x) & FRAC_MASK;
    lfrac_x = LSH(x);
    mfrac_y = MSH(y) & FRAC_MASK;
    lfrac_y = LSH(y);

#ifdef IEEE
    /*
    ** Here is code to correctly handle denormalized numbers
    ** Check real part first
    */
    if (x_exp == 0L) {
        if (mfrac_x != 0) {
            /*
            ** If most significant fraction has bits, shift
            ** up both pieces and decrement exponent.
            */
            do {
                mfrac_x = ( mfrac_x << 1 ) |
                           ( (lfrac_x & HBIT_MASK) ? 0x1L : 0x0L );
                lfrac_x <<= 1;
                x_exp--;
            } while ( mfrac_x < 0x00100000L );
        }
        else {
            /*
            ** Otherwise shift the least significant fraction
            ** bits either up or down and likewise adjust
            ** the exponent.
            */
            mfrac_x = lfrac_x;
            lfrac_x = 0;
            x_exp -= 32;
            while ( mfrac_x >= 0x00200000L ) {
                lfrac_x = ( lfrac_x >> 1 ) |
                           ( (mfrac_x & 0x1L) ? HBIT_MASK : 0x0L );
                mfrac_x >>= 1;
                x_exp++;
            }
            while ( mfrac_x < 0x00100000L ) {
                mfrac_x <<= 1;
                x_exp--;
            }
        }
        /*
        ** Mask out the 1 in the last exponent place and adjust
        ** for non-denormalized exponent.
        */
        mfrac_x &= FRAC_MASK;
        x_exp++;
    }

    /*
    ** Check imaginary part here
    */
    if (y_exp == 0L) {
        if (mfrac_y != 0) {
            /*
            ** If most significant fraction has bits, shift
            ** up both pieces and decrement exponent.
            */
            do {
                mfrac_y = ( mfrac_y << 1 ) |
                           ( (lfrac_y & HBIT_MASK) ? 0x1L : 0x0L );
                lfrac_y <<= 1;
                y_exp--;
            } while ( mfrac_y < 0x00100000L );
        }
        else {
            /*
            ** Otherwise shift the least significant fraction
            ** bits either up or down and likewise adjust
            ** the exponent.
            */
            mfrac_y = lfrac_y;
            lfrac_y = 0;
            y_exp -= 32;
            while ( mfrac_y >= 0x00200000L ) {
                lfrac_y = ( lfrac_y >> 1 ) |
                           ( (mfrac_y & 0x1L) ? HBIT_MASK : 0x0L );
                mfrac_y >>= 1;
                y_exp++;
            }
            while ( mfrac_y < 0x00100000L ) {
                mfrac_y <<= 1;
                y_exp--;
            }
        }
        /*
        ** Mask out the 1 in the last exponent place and adjust
        ** for non-denormalized exponent.
        */
        mfrac_y &= FRAC_MASK;
        y_exp++;
    }

#endif  /* end of IEEE special case handling */

    /*
    ** If one component of the complex number is so much larger
    ** than the other that the square of the smaller is insignificant,
    ** return the correct value.
    */
    if ( x_exp >= y_exp ) {
        if ( (x_exp - y_exp) > HALF_FRACTION_WIDTH ) {
            MSH(result) = MSH(x) & ABS_MASK;
            LSH(result) = LSH(x);
            return(result);
        }
    }
    else {
        if ( (y_exp - x_exp) > HALF_FRACTION_WIDTH ) {
            MSH(result) = MSH(y) & ABS_MASK;
            LSH(result) = LSH(y);
            return(result);
        }
    }

    /*
    ** Now compute the scaled arguments, scaling
    ** such that the numbers will average around 1.0
    ** ( We know the exponents will not differ
    **   by more than HALF_FRACTION_WIDTH )
    */
    expoffset = 0x03FFL - ( (x_exp + y_exp) / 2 );
    MSH(scaled_x) = mfrac_x | 
                        ( (x_exp + expoffset) << MSH_FRACTION_WIDTH_DP );
    LSH(scaled_x) = lfrac_x;
    MSH(scaled_y) = mfrac_y | 
                        ( (y_exp + expoffset) << MSH_FRACTION_WIDTH_DP );
    LSH(scaled_y) = lfrac_y;

    /*
    ** Get the absolute value of the complex number
    */
    x_sq = scaled_x * scaled_x;
    y_sq = scaled_y * scaled_y;
    xsum = x_sq + y_sq;
    scaled_sqrt = qtc_dsqrt(xsum);

    /*
    ** Now revert back to the correct exponent
    */
    sqrt_exp = ( MSH(scaled_sqrt) & EXP_MASK ) >> MSH_FRACTION_WIDTH_DP;
    sqrt_exp -= expoffset;
    if (sqrt_exp > 0x07FEL) {
        /*
        ** Overflow, generated an infinity.
        */
        *overflow_flag = TRUE;
#ifdef IEEE
        return(DOUBLE(_Linfinity));
#else
        return(DOUBLE(_Lxmax));
#endif
    }

#ifdef IEEE
    else if (sqrt_exp <= 0) {
        /*
        ** We have a denormalized number.
        */
        mfrac_rr = ( MSH(scaled_sqrt) & FRAC_MASK ) | 0x00100000L;
        lfrac_rr = LSH(scaled_sqrt);
        while ( sqrt_exp <= 0 ) {
            lfrac_rr = ( lfrac_rr >> 1 ) |
                       ( (mfrac_rr & 0x1L) ? HBIT_MASK : 0x0L );
            mfrac_rr >>= 1;
            sqrt_exp++;
        }
        MSH(result) = mfrac_rr;
        LSH(result) = lfrac_rr;
        return(result);
    }
#endif

    else {
        /*
        ** Here is the usual case, store result.
        */
        MSH(result) = ( MSH(scaled_sqrt) & FRAC_MASK ) |
                        ( sqrt_exp << MSH_FRACTION_WIDTH_DP );
        LSH(result) = LSH(scaled_sqrt);
        return(result);
    }

}   /* end of qtc_util_cabs() routine */

/*** end of qtc_util_cabs.c ***/
