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

/* frexp - return exponent and fraction */

#include <math.h>

double (frexp)(double val, int *ip)
{
#if defined(__i960_BIG_ENDIAN__)
  typedef struct {
    unsigned long _frac_low;
    unsigned _sign     : 1;
    unsigned _exponent : 11;
    unsigned _frac_hi  : 20;
  } __DOUBLE;
#else
  typedef struct {
    unsigned long _frac_low;
    unsigned _frac_hi  : 20;
    unsigned _exponent : 11;
    unsigned _sign     : 1; } __DOUBLE;
#endif /* __i960_BIG_ENDIAN__ */
  union {
    double _d;
    __DOUBLE _D; } un;

  if (val == 0.0)
  { *ip = 0;
    return 0.0;
  }
  un._d = val;
  *ip = un._D._exponent - 1022;	/* extract _exponent,subtract bias */
  un._D._exponent = 1022;	/* _fractional part in range 0.5 .. <1.0 */
  return un._d;
}
