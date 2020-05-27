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

#include "common.h"

extern prtf();

extern int cmd_stat;

const char * const mon_errlist[] = {
			 0,
    /* E_ALIGN */		"Address not properly aligned",
    /* E_ARCH */		"Processor architecture does not support specified operation",
    /* E_ARG */			"Invalid argument",
    /* E_BAD_CMD */		0,
    /* E_BPNOTSET */	"Attempt to delete a breakpoint from address at which none is set",
    /* E_BPSET */		"Breakpoint already exists at specified address",
    /* E_BPUNAVAIL */	"All breakpoints of the specified type are already in use",
    /* E_BUFOVFLH */	0,
    /* E_BUFOVFLT */	"Buffer overflow in target",
    /* E_COMM_ERR */	"Communication failure",
    /* E_COMM_TIMO */	0,
    /* E_EEPROM_ADDR */	"Invalid EEPROM address or length",
    /* E_EEPROM_PROG */	"EEPROM is not erased",
    /* E_EEPROM_FAIL */	"Attempt to erase or program EEPROM failed",
    /* E_FILE_ERR */	"Error reading COFF file",
    /* E_INTR */		"Keyboard interrupt",
    /* E_NOMEM */		0,
    /* E_NOTRUNNING */	0,
    /* E_RUNNING */		0,
    /* E_VERSION */		"Not supported in this version",
    /* E_READ_ERR */	"Error reading target memory",
    /* E_WRITE_ERR */	"Error writing to target memory",
    /* E_VERIFY_ERR */	"Write verification error",
    /* E_BAD_CONFIG */	0,
    /* E_SYS_ERR */		0,
    /* E_RESET */		0,
    /* E_BAD_MAGIC */	0,
    /* E_SWBP_ERR */	"Unable to write sw breakpoint (write verification error)",
    /* E_NO_FLASH */	"FLASH_ADDR in <board>.h is not flash memory",
    /* E_OLD_MON */     0,
    /* E_TARGET_RESET*/	0
};

#define TBL_SIZE(a) (sizeof(a) / sizeof((a)[0]))

void
perror(char *fmt, char *arg)
{
    if (fmt != NULL)
    {
	prtf(fmt, arg);

	if (cmd_stat != 0)
	    prtf(": ");
    }

    if (cmd_stat > 0 && cmd_stat < TBL_SIZE(mon_errlist))
	prtf(mon_errlist[cmd_stat]);
    else if (fmt == NULL || cmd_stat > 0)
        prtf("Unknown error");

    prtf("\n");
}
