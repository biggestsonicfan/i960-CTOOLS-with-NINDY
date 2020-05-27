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
 * $Header: /ffs/p1/dev/src/hdil/common/RCS/aplink.c,v 1.7 1995/09/21 18:39:15 cmorgan Exp $
 *
 * MODULE
 *     aplink.c
 *
 * PURPOSE
 *     Routines designed primarily to support the ApLink target.
 */

/* C include files */
#include <stdio.h>

/* HDI include files */
#include "hdil.h"
#include "private.h"
#include "dbg_mon.h"

/* -------------------------------------------------------------------- */

/*
 * FUNCTION 
 *   hdi_set_mcon(unsigned long region, unsigned long value)
 *
 *   region - memory region, valid values are 0-15.  For the Jx 
 *            processor, this value will be divided by two prior to 
 *            being written in the processor's control tables.
 *
 *   value  - value to write into processor's memory control tables.
 *
 * RETURNS:
 *   OK if no errors are detected, else ERR, plus an appropriate HDI error 
 *   code via hdi_cmd_stat.
 *
 * DESCRIPTION:
 *   Writes a value to a specified memory control register and then,
 *   in the case of the Cx, causes the processor to reload that register.
 *   This function is primarily of value to users of the ApLink target,
 *   since ApLink cannot know in advance how the memory of the target under
 *   test will be configured.
 */

int 
hdi_set_mcon(region, value)
unsigned long region, value;
{
    if (_hdi_arch != ARCH_CA && _hdi_arch != ARCH_JX && _hdi_arch != ARCH_HX)
    {
        hdi_cmd_stat = E_ARCH;
        return(ERR);
    }
    com_init_msg();
    if (com_put_byte(SET_PROC_MCON) != OK ||
                              com_put_byte(~OK) != OK ||
                                     com_put_long(region) != OK || 
                                                com_put_long(value) != OK)
    {
        hdi_cmd_stat = com_get_stat();
        return(ERR);
    }

    if (_hdi_send(0) == NULL)
        return (ERR);

    return (OK);
}



/*
 * FUNCTION 
 *   hdi_set_lmadr(unsigned long lmreg, unsigned long value)
 *
 *   lmreg - Jx or Hx logical memory address register number.
 *
 *   value - value to write into lmreg.
 *
 * RETURNS:
 *   OK if no errors are detected, else ERR, plus an appropriate HDI error 
 *   code via hdi_cmd_stat.
 *
 * DESCRIPTION:
 *   Simple routine to place a value in a Jx/Hx LMADR (logical memory
 *   addres register).  This routine exists in HDI rather than client
 *   code so as to (hopefully) shield clients from the necessity
 *   of hardcoding processor constants into application code.
 */

int 
hdi_set_lmadr(lmreg, value)
unsigned long lmreg, value;
{
    REG dummy;

    if (_hdi_arch != ARCH_JX && _hdi_arch != ARCH_HX)
    {
        hdi_cmd_stat = E_ARCH;
        return(ERR);
    }
    if (lmreg > 1)
    {
        hdi_cmd_stat = E_ARG;
        return(ERR);
    }
    return (hdi_set_mmr_reg(0x8108 + lmreg * 8, value, -1, &dummy));
}



/*
 * FUNCTION 
 *   hdi_set_lmmr(unsigned long lmreg, unsigned long value)
 *
 *   lmreg - Jx or Hx logical memory mask register number.
 *
 *   value - value to write into lmreg.
 *
 * RETURNS:
 *   OK if no errors are detected, else ERR, plus an appropriate HDI error 
 *   code via hdi_cmd_stat.
 *
 * DESCRIPTION:
 *   Simple routine to place a value in a Jx/Hx LMRR (logical memory
 *   mask register).  This routine exists in HDI rather than client
 *   code so as to (hopefully) shield clients from the necessity
 *   of hardcoding processor constants into application code.
 */

int 
hdi_set_lmmr(lmreg, value)
unsigned long lmreg, value;
{
    REG dummy;

    if (_hdi_arch != ARCH_JX && _hdi_arch != ARCH_HX)
    {
        hdi_cmd_stat = E_ARCH;
        return(ERR);
    }
    if (lmreg > 1)
    {
        hdi_cmd_stat = E_ARG;
        return(ERR);
    }
    return (hdi_set_mmr_reg(0x810c + lmreg * 8, value, -1, &dummy));
}



/*
 * FUNCTION 
 *   hdi_aplink_switch(unsigned long region, unsigned long mode)
 *
 *   region - memory region, range is 0 - 0x1f.  Nyuk, Nyuk, Nyuk.
 *
 *   mode   - switch to one of ApLink's supported modes, range is 0-4.
 *
 * RETURNS:
 *   OK if no errors are detected, else ERR, plus an appropriate HDI error 
 *   code via hdi_cmd_stat.
 *
 * DESCRIPTION:
 *   Simple routine to modify the ApLink mode register.  Refer to the 
 *   ApLink User's guide for more info.
 */

int 
hdi_aplink_switch(region, mode)
unsigned long region, mode;
{
    com_init_msg();
    if (com_put_byte(APLINK_SWITCH) != OK ||
                              com_put_byte(~OK) != OK ||
                                     com_put_long(region) != OK || 
                                                com_put_long(mode) != OK)
    {
        hdi_cmd_stat = com_get_stat();
        return(ERR);
    }

    if (_hdi_send(0) == NULL)
        return (ERR);

    return (OK);
}



/*
 * FUNCTION 
 *   hdi_aplink_enable(unsigned long bit, unsigned long value)
 *
 *   bit   - mode register bit, range is 2-4.
 *
 *   value - new mode bit value, range is 0-1.
 *
 * RETURNS:
 *   OK if no errors are detected, else ERR, plus an appropriate HDI error 
 *   code via hdi_cmd_stat.
 *
 * DESCRIPTION:
 *   Simple routine to modify the ApLink mode register.  Refer to the 
 *   ApLink User's guide for information regarding bits 2-4 of the ApLink
 *   mode register.
 */

int 
hdi_aplink_enable(bit, value)
unsigned long bit, value;
{
    com_init_msg();
    if (com_put_byte(APLINK_ENABLE) != OK ||
                              com_put_byte(~OK) != OK ||
                                     com_put_long(bit) != OK || 
                                                com_put_long(value) != OK)
    {
        hdi_cmd_stat = com_get_stat();
        return(ERR);
    }

    if (_hdi_send(0) == NULL)
        return (ERR);

    return (OK);
}



/*
 * FUNCTION 
 *   hdi_aplink_sync(int sync_type)
 *
 *   sync_type - target synchronization type.  May be either
 *               HDI_APLINK_RESET or HDI_APLINK_WAIT.
 *
 * RETURNS:
 *   OK if no errors are detected, else ERR, plus an appropriate HDI error 
 *   code via hdi_cmd_stat.
 *
 * DESCRIPTION:
 *   This command is used after ApLink has been switched into mode 2 and
 *   a new IMI has been downloaded into the appropriate memory region.
 *   At this point, the HDI_APLINK_WAIT command may be used to cause the
 *   monitor to configure itself so that it believes it is in user mode
 *   and then wait for a HW reset.
 *
 *   The HDI_APLINK_RESET command can be used to force the monitor to
 *   configure itself as if it were in user mode and then reset the
 *   target from the new IMI.
 */

int 
hdi_aplink_sync(sync_type)
int sync_type;
{
    char sync;
    const unsigned char *rp;
    int sz;
    
    sync = (sync_type == HDI_APLINK_RESET) ? APLINK_RESET : APLINK_WAIT;

    com_init_msg();
    if (com_put_byte(sync) != OK || com_put_byte(~OK) != OK)
    {
        hdi_cmd_stat = com_get_stat();
        return(ERR);
    }

    if (_hdi_send(0) == NULL)
        return (ERR);

    /* wait for mon960 message to sync that the reset is complete. */
    rp = com_get_msg(&sz, COM_WAIT_FOREVER);
    
    if ((rp == NULL) || (sz != 2) || (rp[0] != APLINK_RESET) || (rp[1] != OK))
        return ERR;

    return (OK);
}
