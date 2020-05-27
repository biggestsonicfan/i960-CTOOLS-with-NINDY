/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994, 1995 Intel Corporation
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
/*)ce*/

/*
 * $Header: /ffs/p1/dev/src/hdil/common/RCS/misc.c,v 1.14 1995/08/22 21:26:23 cmorgan Exp $
 *
 *
 * MODULE
 *     misc.c
 *
 * PURPOSE
 *     A grab bag of miscellaneous routines.  Got a routine and don't know
 *     where to put it?  Use this file.
 */

/* C include files */
#include <stdio.h>
#ifdef __STDC__
#   include <stdlib.h>
#endif
#include <string.h>
#include <ctype.h>
#include <errno.h>

/* HDI include files */
#include "hdil.h"

/* ------------------------------------------------------------------------- */

/*
 * FUNCTION 
 *   hdi_convert_number(const char *num,
 *                      long       *arg, 
 *                      int        arg_type, 
 *                      int        base,
 *                      const char *error_prefix)
 *
 *   num          - ASCII numeric string to be converted to an [unsigned] long.
 *
 *   arg          - converted value of num, returned by reference.
 *
 *   arg_type     - HDI_CVT_UNSIGNED or HDI_CVT_SIGNED.  Specifies the 
 *                  sign of the returned result.  Usually has no effect on 
 *                  the input sign accepted by the conversion routines (all 
 *                  ANSIconversion routines permit an optional leading +/-).
 *
 *   base         - numeric base to be passed to conversion routines.  If 
 *                  the host compiler is K&R, the only acceptable bases are
 *                  8, 10, & 16.  Otherwise, acceptable bases are 2-36.
 *
 *   error_prefix - if !NULL, this function prints out an error message,
 *                  using error_prefix as a leader string, whenever a 
 *                  conversion cannot be performed.  Note that the 
 *                  error_prefix may be set to point at a null character
 *                  string (""), in which case this routine outputs the
 *                  default error message with an empty leader.
 *
 * RETURNS:
 *   OK if no errors are detected, else an appropriate error code.
 *
 * DESCRIPTION:
 *   Convert ASCII numeric string to an [unsigned] long in the specified 
 *   base.  Check that the string contains only legal digits.
 */

int
hdi_convert_number(num, arg, arg_type, base, error_prefix)
const char *num;
long       *arg;
int        arg_type, base;
const char *error_prefix;
{
    int rc = OK;

    while (isspace(*num))  
    {
        /* 
         * Strip leading whitespace to trap empty strings (which is a case
         * that's not handled by MSC's strtoul() implementation).
         */

        num++;
    }
    if (! *num)
        rc = ERR;
    else
    {
#ifdef __STDC__
        char *remainder;

        /* Use a bulletproof scanning and conversion method. */

        errno = 0;
        if (arg_type == HDI_CVT_SIGNED)
            *arg = strtol(num, &remainder, base);
        else
            *(unsigned long *) arg = strtoul(num, &remainder, base);
        if (errno != ERANGE)
        {
            while (isspace(*remainder))
               remainder++;
            if (*remainder)
            {
                /* Argument looks like so:  "dddd[whitespace]<sludge>". */

                rc = ERR;
            }
        }
        else
            rc =  ERR;
#else
        char fmt[24], junk[8], save_char;
        int  modified = FALSE;

        /* Use an imperfect method (does not detect overflow). */

        if (base == 16)
        {
            strcpy(fmt, "%lx");
            if (num[0] == '0' && (num[1] == 'x' || num[1] == 'X'))
                num += 2;   /* K&R sscanf() doesn't like 0x prefix. */
            else if (num[0] == '-' && 
                             num[1] == '0' && 
                                     (num[2] == 'x' || num[2] == 'X'))
            {
                /* 
                 * compensate again for K&R sscanf() by replacing "-0x"
                 * with just "-" .
                 */

                modified  = TRUE;
                save_char = num[2];
                num[2]    = '-';
                num      += 2;
            }
        }
        else if (base == 10)
            strcpy(fmt, (arg_type == HDI_CVT_SIGNED) ? "%ld" : "%lu");
        else     /* Assume octal base. */
            strcpy(fmt, "%lo");
        strcat(fmt, " %1s");
        if (sscanf(num, fmt, arg, junk) != 1)
            rc = ERR;
        if (modified)
            *num = save_char;  /* put overwritten char back */
#endif
    }
    if (rc != OK)
    {
        hdi_cmd_stat = E_NUM_CONVERT;
        if (error_prefix)
        {
            char buf[256];

            sprintf(buf,
                    (*error_prefix) ? "%s: %s\n": "%s%s\n",
                    error_prefix,
                    hdi_get_message());
            hdi_put_line(buf);
        }
    }
    return (rc);
}



/*
 * FUNCTION 
 *   hdi_opt_arg_required(const char *arg, const char *err_prefix)
 *
 *   arg          - command line argument
 *
 *   err_prefix   - Error message leader string to be printed if "arg" is 
 *                  invalid.  Nothing printed if "arg" is NULL.  Note that 
 *                  the error_prefix may be set to point at a null character
 *                  string (""), in which case this routine outputs the
 *                  default error message with an empty leader.
 *
 * DESCRIPTION:
 *   Checks to see that arg is not missing and is not itself a command
 *   line option string.
 *
 * RETURNS:
 *   OK if no errors are detected, else an appropriate error code.
 */

int
hdi_opt_arg_required(arg, err_prefix)
const char *arg, *err_prefix;
{
    if (! arg ||
#ifdef MSDOS
            *arg == '/'  ||
#endif
                        *arg == '-')
    {
        hdi_cmd_stat = E_ARG_EXPECTED;
        if (err_prefix)
        {
            char buf[256];

            sprintf(buf, 
                    "%s%s%s\n", 
                    err_prefix, 
                    *err_prefix ? ": " : "",
                    hdi_get_message());
            hdi_put_line(buf);
        }
        return (ERR);
    }
    return (OK);
}



/*
 * FUNCTION 
 *   hdi_invalid_arg(const char *err_prefix)
 *
 *   err_prefix - Error message leader string to be printed along with a
 *                standard "invalid arg" HDI error message.  Nothing printed
 *                if "arg" is NULL.  Note that the error_prefix may be set 
 *                to point at a null character string (""), in which case 
 *                this routine outputs the default error message with an 
 *                empty leader.
 *
 * DESCRIPTION:
 *   Sets hdi_cmd_stat to indicate a command line or API argument is 
 *   invalid and optionally prints a generic error message describing same.
 *
 * RETURNS:
 *   ERR
 */

int
hdi_invalid_arg(err_prefix)
const char *err_prefix;
{
    hdi_cmd_stat = E_ARG;
    if (err_prefix)
    {
        char buf[256];

        sprintf(buf,
                "%s%s%s\n", 
                err_prefix, 
                *err_prefix ? ": " : "",
                hdi_get_message());
        hdi_put_line(buf);
    }
    return (ERR);
}
