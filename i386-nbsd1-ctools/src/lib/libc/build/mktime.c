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

/* mktime - converts the broken-down time, expressed as local time,
 *          into a calendar time value with the same encoding as that 
 *          of a value returned by time function. 
 * Copyright (c) 1987 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

static const int days_in_month[12] = {31, 28, 31, 30, 31, 30,
                                      31, 31, 30, 31, 30, 31};

static int days(month, date, year)	/* calculate day of the year */
int month, date, year;
{
    int i, yday = 0;

    i = month / 12;
    while (i--) {			/* is no_of_ months > 12 ? */
        yday += 365;
        if (year % 4 == 0) {
            year++;
            yday++;
        }
    }
    month %= 12;
    for (i = 0; i < month; i++)
        yday += days_in_month[i];
    yday += date - 1;			/* complete days before date */
    if ((year % 4 == 0) && (month > 1))  
        yday += 1;
    return yday;
}

time_t mktime(timeptr)
struct tm *timeptr; 
{
    int years, leap_years;
    time_t sec, total_days;
    div_t divmod;


    years = timeptr->tm_year - 70;
    if ((years < (int)0) || (years >= (int)135)) /* out of range */
        return ((time_t)-1);
    leap_years = (years + 1) / 4; 	/* total leap years before current year */
    timeptr->tm_yday = days(timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_year);
    total_days = (years * 365L) + leap_years + timeptr->tm_yday;
    timeptr->tm_wday = (total_days - 3) % 7;
    /* day on Jan 1, 1970 was Thursday = 4 */
    sec = ((((total_days * 24 + timeptr->tm_hour) * 60L) +
            timeptr->tm_min) * 60L) + timeptr->tm_sec;

    /*
     * The value of 'sec' is relative to the local time.  If a timezone
     * is in effect, then adjust it to the value of Greenwich Mean Time:
     */
    if (&tzname) {
        sec += timezone;		/* obtain the timezone difference */
        if (daylight)
            sec -= 3600;		/* and the daylight saving time */
    }
   
    if (timeptr->tm_sec > 59 || timeptr->tm_min > 59 ||    /* modify the struct */
        timeptr->tm_hour > 23 || timeptr->tm_mon > 11 ||   /* if any member is */
        timeptr->tm_mday > days_in_month[timeptr->tm_mon]) /* out of range */
        *timeptr = *localtime(&sec);

    return sec;
}


