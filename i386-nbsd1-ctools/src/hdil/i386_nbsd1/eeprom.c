/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994 Intel Corporation
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
 * Host side of logical communication layer.  These commands are used
 * by sdm to implement user commands.
 */
/* $Header: /ffs/p1/dev/src/hdil/common/RCS/eeprom.c,v 1.2 1994/08/16 22:37:12 gorman Exp $$Locker:  $ */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "private.h"
#include "hdi_com.h"
#include "dbg_mon.h"

int
hdi_eeprom_check(addr, length, eeprom_size, prog)
ADDR addr;
unsigned long length;
unsigned long *eeprom_size;
ADDR prog[2];
{
	const unsigned char *rp;
	int msg_sz;

        com_init_msg();
        if (com_put_byte(CHECK_EEPROM) != OK
            || com_put_byte(~OK) != OK
            || com_put_long(addr) != OK
            || com_put_long(length) != OK
	    || com_put_msg(NULL, 0) != OK
	    || (rp = com_get_msg(&msg_sz, COM_WAIT)) == NULL)
        {
            hdi_cmd_stat = com_get_stat();
            return(ERR);
        }

	if (msg_sz < 2 || rp[0] != CHECK_EEPROM)
	{
	    hdi_cmd_stat = E_COMM_ERR;
            return(ERR);
	}

	hdi_cmd_stat = rp[1];

	/* The earliest monitors did not return additional information. */
	if (msg_sz > 2)
	{
	    rp += 2;		/* skip message header */

	    if (eeprom_size != NULL)
		*eeprom_size = get_long(rp);
	    else
		rp += 4;

	    if (hdi_cmd_stat == E_EEPROM_PROG && prog != NULL)
	    {
		prog[0] = get_long(rp);
		prog[1] = get_long(rp);
	    }
	}
	else
	{
	    if (eeprom_size != NULL)
		*eeprom_size = 0;
	    if (prog != NULL)
		prog[0] = NO_ADDR;
	}

	return(hdi_cmd_stat == OK ? OK : ERR);
}


int
hdi_eeprom_erase(addr, length)
ADDR addr;
unsigned long length;
{
    com_init_msg();
    if (com_put_byte(ERASE_EEPROM) != OK
       || com_put_byte(~OK) != OK
       || com_put_long(addr) != OK
       || com_put_long(length) != OK)
        {
        hdi_cmd_stat = com_get_stat();
        return(ERR);
        }

    if (_hdi_send(0) == NULL)
        return(ERR);

	return(OK);
}
