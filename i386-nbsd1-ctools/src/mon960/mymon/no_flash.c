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
/********************************************************/
/* Stubs for boards with no EEPROM support 	  	*/
/********************************************************/

#include <string.h>

#include "retarget.h"
#include "hdi_errs.h"

unsigned long eeprom_size;
ADDR eeprom_prog_first, eeprom_prog_last;

int
erase_eeprom(ADDR addr, unsigned long length)
{
	cmd_stat = E_NO_FLASH;
	return ERR;
}

int
check_eeprom(ADDR addr, unsigned long length)
{
	cmd_stat = E_NO_FLASH;
	return ERR;
}

/*
 * Since check_eeprom always returns ERROR, this will never be called */
int
write_eeprom(ADDR addr, const void *data, int data_size)
{
	cmd_stat = E_NO_FLASH;
	return ERR;
}

/*
 * This stub is not needed for boards that do not have eeprom, because
 * it is never called.  However, it is required to link a monitor that
 * does not support eeprom for a board that has eeprom.
 */
void
init_eeprom()
{
}

/********************************************************/
/* IS EEPROM  	  	 			*/
/* Check if memory is Flash */
/*alwats return FALSE						*/
/*	                           			*/
/********************************************************/
int
is_eeprom(ADDR addr, unsigned long length)
{
    return FALSE;
}

int
flash_supported(int max_banks, ADDR bank_addr[], unsigned long bank_size[])
{
    memset(bank_addr, 0, sizeof(ADDR) * max_banks);
    memset(bank_size, 0, sizeof(unsigned long) * max_banks);
    return (FALSE);
}
