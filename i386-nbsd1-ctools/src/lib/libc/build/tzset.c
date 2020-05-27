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

/* tzset - set the time zone variables from the environment
 * Copyright (c) 1986,87 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <time.h>
#include <reent.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

extern char **environ;

#define HRS 0
#define MIN 1
#define SEC 2
#define TRUE    1
#define FALSE   0

void tzset()
{
    char *p;
    int clock[3];
    int len;
    char *temp_tzname[2];
    long temp_timezone;
    int temp_daylight;
    char cur_char;


    if ((p = getenv("TZ")) == NULL)
        return;


    /************************************************************
    **  Then, we need to look for the "implementation defined"
    **  ':' and decide what to do with it.  At this point, we
    **  will assume the input is of the form:
    **
    **      :stddst
    **
    **  where std is a 3 byte standard time name, dst is a 3 byte
    **  "summer" time name.  It is assumed that there is a 1 hour
    **  difference between the two.
    *************************************************************/
    if (*p == ':') {
        temp_tzname[0] = malloc(4);
        temp_tzname[1] = malloc(4);
        strncpy(temp_tzname[0], ++p, 3);
        *(temp_tzname[0]+3) = '\0';
        p += 3;
        strncpy(temp_tzname[1], p, 3);
        *(temp_tzname[1]+3) = '\0';

        /******************************************************
        **  For this implementation defined mode, we only accept
        **  timezone values that are within the continental US.
        **  These timezones are only defined by PST, MST, CST, 
        **  and EST.  All other forms must use the posix 
        **  defined format.
        *******************************************************/
        cur_char = *(temp_tzname[0]);
        switch (cur_char) {
                    case 'P':
                            temp_timezone = 8*3600;
                            break;

                    case 'C':
                            temp_timezone = 6*3600;
                            break;

                    case 'E':
                            temp_timezone = 5*3600;
                            break;

                    case 'M':
                            temp_timezone = 7*3600;
                            break;

                    default: 
                            return;
        }
        if (strcmp(temp_tzname[0]+1, "ST") != 0) return;
    }
    else {

        /************************************************************
        **  Now we need to process any expanded format forms of the 
        **  time functions.  These are defined as:
        **
        **      stdoffset[dst[offset][,start[/time],end[/time]]]
        **
        **      where std and dst are the timezone identifiers
        **      offset is in the form hh[:mm[:ss]]
        **      and the rest indicates the start and end of "summer" time.
        ****************************************************************/
        len = _Ltzgetname(p);
        temp_tzname[0] = malloc(len+1);
        strncpy(temp_tzname[0], p, len);
        *(temp_tzname[0]+len) = '\0';
        p += len;
    
        /***************************************
        **  Get the offset, if it exists.
        ***************************************/
        len = _Ltzgettime(p, clock);
        temp_timezone = clock[HRS]*3600+clock[MIN]*60+clock[SEC];
        p += len;
    
        /***************************************
        **  Get dst from the string.
        ***************************************/
        len = _Ltzgetname(p);
        temp_tzname[1] = malloc(len+1);
        strncpy(temp_tzname[1], p, len);
        *(temp_tzname[1]+len) = '\0';
        p += len;
    
        /***************************************
        **  Get the offset, if it exists.
        ***************************************/
        len = _Ltzgettime(p, clock);
		if (len > 0)
			temp_timezone = clock[HRS]*3600+clock[MIN]*60+clock[SEC];
        p += len;

        /******************************************
        **  Now we need to get the "rule", if it 
        **  exists.
        *******************************************/
        len = _Ltzgetrule(p);
        p += len;
    
    }
    
    /*****************************************************************
    **  Set daylight savings flag, if there is a "summer" time.
    *****************************************************************/
    if (*temp_tzname[1])
        temp_daylight = 1;
    else
        temp_daylight = 0;

    /***************************************************************
    **  If we got this far, there have been no errors.  Go ahead and
    **  set the real values.
    **
    **  First, free all memory currently being used in the tzname
    **  array.
    ************************************************************/
    free(tzname[0]);
    free(tzname[1]);
    tzname[0] = temp_tzname[0];
    tzname[1] = temp_tzname[1];
    timezone = temp_timezone;
    daylight = temp_daylight;
}

/*************************************************************************/
/*************************************************************************/

/*************************************************************************
**  This internal routine parses through the input string until it finds
**  a terminator.  A terminator for the time name is defined in posix
**  spec, section 8.1.1.
*************************************************************************/
int _Ltzgetname(char *string) {

    int length;
    int index;
    int done;

    /********************************************************
    **  Just keep going thru the input string until we run
    **  out of string, or we hit a terminator.  Then return
    **  the number of characters we found.
    *********************************************************/
    length = strlen(string);
    done = FALSE;
    for (index = 0; index < length; index++) {
        switch (*(string+index)) {
                case '+':
                case '-':
                case ':':
                case ',':
                case '\0':
                        done = TRUE;
                        break;

                default:
                        if (isdigit(*(string+index))) done = TRUE;
        }
        if (done) break;
    }
    return (index);

}

/*************************************************************************/
/*************************************************************************/

/*************************************************************************
**  This internal routine parses through the input string until it finds
**  a terminator.  A terminator for the time offset is defined in posix
**  spec, section 8.1.1, as hh:mm:ss.
*************************************************************************/
int _Ltzgettime(char *string, int *timearr) {

    int length;
    char *start;
    int index;
    int count;
    int timeidx;
    char cur_char;


    *(timearr+HRS) = 0;
    *(timearr+MIN) = 0;
    *(timearr+SEC) = 0;
    count = 0;

    /******************************************************
    **  Look for all 3:  hours, minutes, and seconds.
    ******************************************************/
    for (timeidx = 0; timeidx < 3; timeidx++) {
        start = string;
        length = strlen(string);

        /**************************************************
        **  Loop through the string until we find something
        **  that isn't a numeric digit.
        **************************************************/
        for (index = 0; index < length; index++) {
            cur_char = *(string+index);
            if (!isdigit(cur_char)) {
                if ((cur_char != '+') && (cur_char != '-')) break;
            }
        }
        if (index > 0) *(timearr+timeidx) = atoi(start);
        count += index;
        string += index;

        /***********************************************
        **  If there is no more in the string, we are done.
        ************************************************/
        if (index == length) break;
        if (*string != ':') break;
        else {
            string++;
            count++;
        }
    }

    /***********************************************
    **  Make sure they are all the same sign for
    **  when they are added together.  < 0 means
    **  the time is east of the prime meridian.
    ***********************************************/
    if (*(timearr+HRS) < 0) {
        *(timearr+MIN) *= -1;
        *(timearr+SEC) *= -1;
    }
    return(count);
}

/*************************************************************************/
/*************************************************************************/

/*************************************************************************
**  The "rule" information is not currently supported by any of the time
**  related routines.  Currently, if the information exists, it is ignored.
**************************************************************************/
int _Ltzgetrule(char *string) {
    return 0;
}
