#ifndef __TIME_H__
#define __TIME_H__

/*
 * <time.h> : time zone stuff
 */

#include <__macros.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef _size_t
#define _size_t
typedef unsigned size_t;                /* result of sizeof operator */
#endif

#ifndef _time_t
#define _time_t
typedef unsigned long time_t;
#endif

typedef unsigned long clock_t;

#define CLOCKS_PER_SEC 100

#pragma i960_align tm = 16
struct	tm {	
  int tm_sec;		/* seconds (0-59) */
  int tm_min;		/* minutes (0-59) */
  int tm_hour;		/* hours (0-23) */
  int tm_mday;		/* days (1-31) */
  int tm_mon;		/* months (0-11) */
  int tm_year;		/* year -1900 */
  int tm_wday;		/* day of week (sun = 0) */
  int tm_yday;		/* day of year (0 - 365) */
  int tm_isdst;		/* non-zero if DST */
};

__EXTERN
clock_t (clock)(__NO_PARMS);
__EXTERN
double (difftime)(time_t,time_t);
__EXTERN
time_t (mktime)(struct tm*);
__EXTERN
time_t (time)(time_t*);
__EXTERN
char * (asctime)(__CONST struct tm*);
__EXTERN
char * (ctime)(__CONST time_t*);
__EXTERN
struct tm * (gmtime)(__CONST time_t*);
__EXTERN
struct tm * (localtime)(__CONST time_t*);
__EXTERN
size_t  (strftime)(char*s,size_t,__CONST char*,__CONST struct tm*);

#if !defined(__STRICT_ANSI) && !defined(__STRICT_ANSI__)

__EXTERN
void tzset(void);

#include <reent.h>

/* This is non-ansi.  Use CLOCKS_PER_SEC instead. */
#define CLK_TCK CLOCKS_PER_SEC

#define daylight        _tzset_ptr()->_daylight
#define timezone        _tzset_ptr()->_timezone
#define tzname          _tzset_ptr()->_tzname
#endif /* strict ANSI */

#endif

