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

#include <stdio.h>
#include <locale.h>
#include <limits.h>

static struct lconv _locale_stub;
static char C_str[2] = "C";

char *
setlocale(int category, const char *locale)
{
  if (locale != 0 &&
      locale[0] != '\0' &&
      locale[0] != 'C' && locale[1] != '\0')
    return(NULL);
  return(C_str);
}

struct lconv *localeconv (void)
{
  _locale_stub.decimal_point= ".";
  _locale_stub.thousands_sep= "";
  _locale_stub.grouping= "";
  _locale_stub.int_curr_symbol = "";
  _locale_stub.currency_symbol = "";
  _locale_stub.mon_decimal_point = "";
  _locale_stub.mon_thousands_sep = "";
  _locale_stub.mon_grouping = "";
  _locale_stub.positive_sign = "";
  _locale_stub.negative_sign = "";
  _locale_stub.int_frac_digits = CHAR_MAX;
  _locale_stub.frac_digits = CHAR_MAX;
  _locale_stub.p_cs_precedes = CHAR_MAX;
  _locale_stub.p_sep_by_space = CHAR_MAX;
  _locale_stub.n_cs_precedes = CHAR_MAX;
  _locale_stub.n_sep_by_space = CHAR_MAX;
  _locale_stub.p_sign_posn = CHAR_MAX;
  _locale_stub.n_sign_posn = CHAR_MAX;

  return &_locale_stub;
}
