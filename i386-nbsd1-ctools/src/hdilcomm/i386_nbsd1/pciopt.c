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
 * $Header: /tmp_mnt/ffs/p1/dev/src/hdilcomm/common/RCS/pciopt.c,v 1.4 1995/11/27 20:15:33 cmorgan Exp $
 */

/**************************************************************************
 *
 *  Name:
 *    pciopt
 *
 *  Description:
 *    loads the global _com_config structure with proper pci
 *    communication configuration information on startup.
 *
 ************************************************************************m*/


#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef __STDC__
#include <stdlib.h>
#endif

#include "common.h"
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
com_select_pci_comm()
{
    /* Host requests target connection via pci comm. */

    _com_config.dev_type = HDI_PCI_COMM;
}



void
com_pci_debug(state)
int state;
{
    _com_config.pci_debug = state;
}



void 
com_pciopt_cfg(pci_cfg)
COM_PCI_CFG *pci_cfg;
{
    /* Cache host-specified PCI address. */

    _com_config.pci = *pci_cfg;
}
