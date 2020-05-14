#ifndef __FLOAT_H__
#define __FLOAT_H__ 

/*
 * float.h - 960 floating point characteristics
 */

#include <__macros.h>

#define __FLT_EPSILON	1.19209290e-7F
#define __FLT_MAX	3.40282347e+38F
#define __FLT_MIN       1.17549435e-38F

#define __DBL_EPSILON	2.2204460492503131e-16
#define __DBL_MAX	1.7976931348623157e+308
#define __DBL_MIN	2.2250738585072016e-308

#define __LDBL_EPSILON  1.08420217248550443401e-19L
#define __LDBL_MAX      1.18973149535723176502e+4932L
#define __LDBL_MIN	3.36210314311209350626e-4932L

#define FLT_RADIX        2

__EXTERN unsigned int (_getac)(__NO_PARMS);
#define FLT_ROUNDS       (0x2D >> ((_getac() >> 29) & 0x6))

#define FLT_DIG          6
#define FLT_EPSILON      __FLT_EPSILON
#define FLT_MANT_DIG     24
#define FLT_MAX          __FLT_MAX
#define FLT_MAX_EXP      128
#define FLT_MAX_10_EXP	 38
#define FLT_MIN          __FLT_MIN
#define FLT_MIN_EXP	 (-125)
#define FLT_MIN_10_EXP   (-37)

#define DBL_DIG          15
#define DBL_EPSILON      __DBL_EPSILON
#define DBL_MANT_DIG     53
#define DBL_MAX          __DBL_MAX
#define DBL_MAX_EXP      1024
#define DBL_MAX_10_EXP	 308
#define DBL_MIN          __DBL_MIN
#define DBL_MIN_EXP	 (-1021)
#define DBL_MIN_10_EXP   (-307)

#define LDBL_DIG         18  /* max decimal digits of long double precisn */
#define LDBL_EPSILON     __LDBL_EPSILON
#define LDBL_MANT_DIG    64
#define LDBL_MAX         __LDBL_MAX
#define LDBL_MAX_EXP     16384
#define LDBL_MAX_10_EXP  4932
#define LDBL_MIN         __LDBL_MIN
#define LDBL_MIN_EXP     (-16381)
#define LDBL_MIN_10_EXP  (-4931)

#endif
