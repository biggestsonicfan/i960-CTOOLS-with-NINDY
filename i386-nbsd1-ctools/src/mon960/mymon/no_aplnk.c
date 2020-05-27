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

#include "hdi_com.h"
#include "hdi_errs.h"
#include "dbg_mon.h"

/*
 * $Header: /ffs/p1/dev/src/mon960/common/RCS/no_aplnk.c,v 1.1 1995/08/01 20:59:19 cmorgan Exp $
 *
 * MODULE
 *     no_aplnk.c
 *
 * PURPOSE
 *     Stubs for those non-ApLink targets.
 */

/* -------------------------------------------------------------------- */

static void
hi_e_version(int cmd)
{
    CMD_TMPL response;

    response.cmd  = cmd;
    response.stat = E_VERSION;
    com_put_msg((const unsigned char *) &response, sizeof(response));
}



void 
aplink_switch_cmd(const void *cmd)
{
    hi_e_version(APLINK_SWITCH);
}



void 
aplink_reset_cmd(const void *cmd)
{
    hi_e_version(APLINK_RESET);
}



void 
aplink_enable_cmd(const void *cmd)
{
    hi_e_version(APLINK_ENABLE);
}



void 
aplink_wait_cmd(const void *cmd)
{
    hi_e_version(APLINK_WAIT);
}
