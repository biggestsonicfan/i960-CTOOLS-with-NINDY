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

/* gmtime - return Greenwich Mean Time
 * Copyright (c) 1985,86,87 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <time.h>
#include <reent.h>

struct tm *gmtime(const time_t *timer)
{
    struct _thread *p = _thread_ptr();
#define t	(*p->_gmtime_buffer)
    int month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    register int i;
    time_t tmr = *timer;
	time_t temp;
	short tmr_screwed_with;  /* Needed to deal properly with time_t as unsigned */

    t.tm_sec = tmr % 60;		/* get seconds */
    tmr /= 60;
    t.tm_min = tmr % 60;		/* get minutes */
    tmr /= 60;
    t.tm_hour = tmr % 24;		/* get hours */
    tmr /= 24;
	temp = (tmr < 3) ? (7 + tmr) : tmr; /* Prevent unsigned wrap */
    t.tm_wday = (temp - 3) % 7;		/* day on Jan 1, 1970 was Thursday = 4 */
	tmr_screwed_with = (tmr < 1) ? 1 : 0;
	temp = (tmr < 1) ? 1 : tmr; /* Prevent unsigned wrap */
    t.tm_year = 70 + (((temp - 1) / (4 * 365 + 1)) * 4);

    for (tmr = ((temp - 1) % (4 * 365 + 1)) + 1; 1; ++t.tm_year) { /* perverse code */
        if (t.tm_year % 4 || !(t.tm_year % 100) && t.tm_year % 400) { /* not leap year */
            if (tmr > 365)
                tmr -= 365;
            else
                break;
        }
        else {
            if (tmr > 366)
                tmr -= 366;
            else
                break;
        }
    }

	/* Undo the tmr year kludge (if applied) for month and day. */
	if (tmr_screwed_with == 1)
		tmr--;

    if (!(t.tm_year % 4 || !(t.tm_year % 100) && t.tm_year % 400))
        month[1] = 29;
    t.tm_yday = tmr;
    t.tm_mon = 0;
    i = 0;

    while (tmr >= month[i] && i < 12) {
        ++t.tm_mon;
        tmr -= month[i];
        ++i;
    }

    t.tm_mday = tmr + 1;
    t.tm_isdst = 0;			/* are you kidding? */
    return (&t);
}
