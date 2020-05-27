#ifndef __LIMITS_H__
#define __LIMITS_H__

#include <__macros.h>

/*
   All constants have types consistent with integral promotion
   rules.  This means that some definitions are sensitive
   to -traditional for gcc960.

   CHAR_BIT   == Number of bits in a char
   SCHAR_MIN  == minimum value a signed char can hold
   SCHAR_MAX  == maximum value a signed char can hold
   UCHAR_MAX  == maximum value a unsigned char can hold
   CHAR_MIN   == minimum value a char can hold
   CHAR_MAX   == maximum value a char can hold
   MB_LEN_MAX == max multibyte characters supported
   SHRT_MIN   == minimum value a short int can hold
   SHRT_MAX   == maximum value a short int can hold
   USHRT_MAX  == maximum value an unsigned short int can hold
   INT_MIN    == minimum value an int can hold
   INT_MAX    == maximum value an int can hold
   UINT_MIN   == minimum value an unsigned int can hold
   UINT_MAX   == maximum value an unsigned int can hold
   LONG_MIN   == minimum value a  long can hold
   LONG_MAX   == maximum value a  long can hold
   ULONG_MAX  == maximum value an unsigned long can hold
*/

#define MB_LEN_MAX  1
#define CHAR_BIT    8
#define SCHAR_MAX   127
#define SCHAR_MIN   (-SCHAR_MAX - 1)

#define SHRT_MAX    32767
#define SHRT_MIN    (-SHRT_MAX - 1)

#define LONG_MAX    2147483647L
#define LONG_MIN    (-LONG_MAX - 1)

#define INT_MAX     LONG_MAX
#define INT_MIN     LONG_MIN
#define UINT_MAX    4294967295U

/* Remaining defs are sensitive to integral promotions, and
   presence/absence of 'ul' in constants. */

#ifdef __STDC__

#define UCHAR_MAX 255

#if defined(__CHAR_UNSIGNED__) && !defined(__SIGNED_CHARS__)
#define CHAR_MIN 0
#define CHAR_MAX 255
#else
#define CHAR_MIN -128
#define CHAR_MAX  127
#endif

#define USHRT_MAX 65535
#define ULONG_MAX 4294967295UL

#else

#define UCHAR_MAX 255U

#if defined(__CHAR_UNSIGNED__) && !defined(__SIGNED_CHARS__)
#define CHAR_MIN 0U
#define CHAR_MAX 255U
#else
#define CHAR_MIN -128
#define CHAR_MAX  127
#endif

#define USHRT_MAX 65535U
#define ULONG_MAX 4294967295U
#endif
#endif
