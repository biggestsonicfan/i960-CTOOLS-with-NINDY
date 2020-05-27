/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1995 Intel Corporation
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
 * $Header: /ffs/p1/dev/src/hdilcomm/common/RCS/seropt.c,v 1.5 1995/08/31 19:28:45 cmorgan Exp $
 */

 
/**************************************************************************
 *
 *  Name:
 *    seropt
 *
 *  Description:
 *    loads the global _com_config structure with proper serial configuration
 *    information on startup
 *
 ************************************************************************m*/


#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef __STDC__
#include <stdlib.h>
#endif

#include "hdil.h"
#include "com.h"



/* 
 * In general, config "handlers" to do the following:
 *    (1) verify the validity of a user option
 *    (2) set the correct field within the global config struct. 
 *
 * Preconditions (when handler is passed argument):
 *    Callee has verified user-specified arg is not NULL or unspecified
 *    (e.g., arg is not another command line switch).
 *
 * Handlers, when applicable, return OK if successful, ERR otherwise.
 */


void
com_select_serial_comm()
{
    /* Host requests target connection via serial comm. */

    _com_config.dev_type = RS232;
}



int
com_seropt_baud(arg, err_prefix)
const char *arg;
const char *err_prefix;
{
    unsigned long baud;
    int           ec;

    if ((ec = hdi_convert_number(arg,
                                 (long *) &baud,
                                 HDI_CVT_UNSIGNED,
                                 10,
                                 err_prefix)) != OK)
    {
        return (ec);
    }

    /* 
     * Baud is the only serial parameter HDI currently validates when
     * a serial connection is established.  Nothing else to do.
     */
    _com_config.baud = baud;
    return (OK);
}



int
com_seropt_port(arg, err_prefix)
const char *arg;
const char *err_prefix;
{
#ifdef MSDOS
    char tmp[32], *cp;
#endif

#ifdef MSDOS
    /* list of DOS COM ports is known -- validate now */

    strncpy(tmp, arg, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    cp = tmp;
    while (*cp)
    {
        if (islower(*cp))
            *cp = toupper(*cp);
        cp++;
    }
    cp = &tmp[3];
    if (! (strncmp(tmp, "COM", 3) == 0 && 
           (*cp == '1' || *cp == '2' || *cp == '3' || *cp == '4')))
    {
        return (hdi_invalid_arg(err_prefix));
    }

    strcpy(_com_config.device, tmp);
#else
    strcpy(_com_config.device, arg);
#endif
    return (OK);
}



#ifdef MSDOS

int 
com_seropt_freq(arg, err_prefix)
const char *arg;
const char *err_prefix;
{
    int  ec;
    long freq;

    if ((ec = hdi_convert_number(arg,
                                 &freq,
                                 HDI_CVT_UNSIGNED,
                                 10,
                                 err_prefix)) != OK)
    {
        return (ec);
    }

    if (freq < 0)
        return (hdi_invalid_arg(err_prefix));

    _com_config.freq = freq;
    return (OK);
}

#endif  /* MSDOS */
