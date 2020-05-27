#ifndef __LOCALE_H__
#define __LOCALE_H__

/*
 * <locale.h> : internationalization routines.
 */

#include <__macros.h>

#define LC_ALL      0
#define LC_COLLATE  1
#define LC_CTYPE    2
#define LC_MONETARY 3
#define LC_NUMERIC  4
#define LC_TIME     5
#define LC_MAX      6	/* above and beyond ansi requirement */

#ifndef NULL
#define NULL ((void *)0)
#endif

#pragma i960_align lconv = 16
struct lconv {
  char *decimal_point;
  char *thousands_sep;
  char *grouping;
  char *int_curr_symbol;
  char *currency_symbol;
  char *mon_decimal_point;
  char *mon_thousands_sep;
  char *mon_grouping;
  char *positive_sign;
  char *negative_sign;
  char  int_frac_digits;
  char  frac_digits;
  char  p_cs_precedes;
  char  p_sep_by_space;
  char  n_cs_precedes;
  char  n_sep_by_space;
  char  p_sign_posn;
  char  n_sign_posn;
};

__EXTERN
char* (setlocale)(int, __CONST char*);
__EXTERN
struct lconv * (localeconv)(__NO_PARMS);

#endif
