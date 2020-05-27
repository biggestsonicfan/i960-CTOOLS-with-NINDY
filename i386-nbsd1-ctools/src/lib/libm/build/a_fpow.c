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

#include <errno.h>

#include "afp_cls.h"

#define	class_vector(a,b)	((((a) & 7) << 3) | ((b) & 7))

extern int     __clssfsi(float x);
extern float   _AFP_int_powf(float x, float y);
extern float   _AFP_NaN_S(float x, float y);
extern float   _AFP_INF_S(int neg_sign);
extern float   _AFP_QNaN_S( void );
extern float   _AFP_mZERO_S( void );
extern float   fabsf(float x);

float powf(float x, float y)
{
  int     x_class, y_class;
  long    y_int;
  float   y_cycle;

  x_class = __clssfsi(x);
  y_class = __clssfsi(y);


  switch (class_vector(x_class,y_class)) {

    case  class_vector(CLASS_zero,     CLASS_zero):     /* 0      ^ 0 */
    case  class_vector(CLASS_normal,   CLASS_zero):	/* Finite ^ 0 */
    case  class_vector(CLASS_infinity, CLASS_zero):	/* INF    ^ 0 */
    case  class_vector(CLASS_denormal, CLASS_zero):     /* Finite ^ 0 */
    case  class_vector(CLASS_QNaN,     CLASS_zero):     /* QNaN   ^ 0 */
    case  class_vector(CLASS_SNaN,     CLASS_zero):     /* SNaN   ^ 0 */
      return(1.0f);  /* all IEEE representations to the 0 power produce 1.0 */


    default:                                            /* One or more NaN's */
      return(_AFP_NaN_S(x,y));


    case  class_vector(CLASS_zero,     CLASS_normal):
    case  class_vector(CLASS_zero,     CLASS_infinity):
    case  class_vector(CLASS_zero,     CLASS_denormal):
      if  ((y_class & CLASS_sign_bit) == 0)  {
        return(x);
      }  else  {
        return(_AFP_INF_S(x_class & CLASS_sign_bit));  /* +/- INF */
      };


    case  class_vector(CLASS_normal,   CLASS_infinity):
    case  class_vector(CLASS_infinity, CLASS_infinity):
    case  class_vector(CLASS_denormal, CLASS_infinity):
      if  (fabsf(x) == 1.0f)  {
        return(_AFP_QNaN_S());
      }  else if  ((fabsf(x) > 1.0f) ^ ((y_class & CLASS_sign_bit) != 0))  {
        return(_AFP_INF_S(0));
      }  else  {
        return(0.0f);
      }


    case  class_vector(CLASS_infinity, CLASS_normal):
      if  (y == 1.0f)  {
        return(x);
      }
    case  class_vector(CLASS_infinity, CLASS_denormal):
      if  ((x_class & CLASS_sign_bit) == 0)  {
        if  ((y_class & CLASS_sign_bit) == 0)  {
          errno = ERANGE;
          return(x);
        }  else  {
          return(0.0f);
        }
      }  else if  ((y_class & CLASS_sign_bit) != 0)  {
        return(_AFP_mZERO_S());
      }  else  {
        return(_AFP_INF_S(0));
      }


    case  class_vector(CLASS_normal,   CLASS_normal):
    case  class_vector(CLASS_normal,   CLASS_denormal):
    case  class_vector(CLASS_denormal, CLASS_normal):
    case  class_vector(CLASS_denormal, CLASS_denormal):

      if  ((x_class & CLASS_sign_bit) != 0)  {
        y_int   = y;
        y_cycle = y_int;
        if  (y != y_cycle)  {             /* Neg val to non-integral ...    */
          return(_AFP_QNaN_S());          /* ... power -> invalid arguments */
        };

        if  ((y_int & 1) != 0)  {
          return(_AFP_int_powf( x,y));    /* Neg val to odd power */
        } else {
          return(_AFP_int_powf(-x,y));    /* Neg val to even power */
        };
      } else {
        return(_AFP_int_powf(x,y));
      };

  }
}
