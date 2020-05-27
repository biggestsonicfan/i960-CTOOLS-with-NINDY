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
 * $Header: /ffs/p1/dev/src/hdilcomm/common/RCS/commopt.c,v 1.3 1995/03/21 17:24:02 cmorgan Exp $
 */

 
/**************************************************************************
 *
 *  Name:
 *    commopt
 *
 *  Description:
 *    loads the global _com_config structure with proper packet 
 *    configuration information on startup
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
 * Config "handlers" to do the following:
 *    (1) verify the validity of a user option
 *    (2) set the correct field within the global config struct. 
 *
 * Preconditions:
 *    Callee has verified user-specified arg is not NULL or unspecified
 *    (e.g., arg is not another command line switch).
 *
 * handlers all return OK if successful, ERR otherwise.
 */



int 
com_commopt_max_pktlen(arg, err_prefix)
const char *arg;
const char *err_prefix;
{
    long pktlen;
    int  ec;

    if ((ec = hdi_convert_number(arg,
                                 &pktlen,
                                 HDI_CVT_UNSIGNED,
                                 10,
                                 err_prefix)) != OK)
    {
        return (ec);
    }

    if (pktlen < 2 || pktlen > 4095)
        return (hdi_invalid_arg(err_prefix));

    _com_config.max_len = pktlen;
    return (OK);
}



int 
com_commopt_max_retry(arg, err_prefix)
const char *arg;
const char *err_prefix;
{
    int  ec;
    long max_retry;

    if ((ec = hdi_convert_number(arg,
                                 &max_retry,
                                 HDI_CVT_UNSIGNED,
                                 10,
                                 err_prefix)) != OK)
    {
        return (ec);
    }

    if (max_retry < 1 || max_retry > 255)
        return (hdi_invalid_arg(err_prefix));

    _com_config.max_try = max_retry;
    return (OK);
}



int 
com_commopt_host_timo(arg, err_prefix)
const char *arg;
const char *err_prefix;
{
    int  ec;
    long host_timo;

    if ((ec = hdi_convert_number(arg,
                                 &host_timo,
                                 HDI_CVT_UNSIGNED,
                                 10,
                                 err_prefix)) != OK)
    {
        return (ec);
    }

    if (host_timo < 1 || host_timo > 65535)
        return (hdi_invalid_arg(err_prefix));

    _com_config.host_pkt_timo = host_timo;
    return (OK);
}



int 
com_commopt_ack_timo(arg, err_prefix)
const char *arg;
const char *err_prefix;
{
    int  ec;
    long ack_timo;

    if ((ec = hdi_convert_number(arg,
                                 &ack_timo,
                                 HDI_CVT_UNSIGNED,
                                 10,
                                 err_prefix)) != OK)
    {
        return (ec);
    }

    if (ack_timo < 1 || ack_timo > 65535)
        return (hdi_invalid_arg(err_prefix));

    _com_config.ack_timo = ack_timo;
    return (OK);
}



int 
com_commopt_target_timo(arg, err_prefix)
const char *arg;
const char *err_prefix;
{
    int  ec;
    long target_timo;

    if ((ec = hdi_convert_number(arg,
                                 &target_timo,
                                 HDI_CVT_UNSIGNED,
                                 10,
                                 err_prefix)) != OK)
    {
        return (ec);
    }

    if (target_timo < 1 || target_timo > 65535)
        return (hdi_invalid_arg(err_prefix));

    _com_config.target_pkt_timo = target_timo;
    return (OK);
}
