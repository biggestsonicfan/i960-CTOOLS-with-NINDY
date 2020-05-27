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

#include <stdio.h>

#include "common.h"
#include "hdi_errs.h"

char *hdi_err_param1, *hdi_err_param2, *hdi_err_param3;

static const char * errlist[] = {
			"Unknown HDI error",
    /* E_ALIGN */	"Address not properly aligned",
    /* E_ARCH */	"Processor architecture does not support specified operation",
    /* E_ARG */		"Invalid argument",
    /* E_BAD_CMD */	"Unsupported command",
    /* E_BPNOTSET */	"No breakpoint at that address",
    /* E_BPSET */	"Breakpoint exists at that address",
    /* E_BPUNAVAIL */	"All breakpoints of the specified type are already in use",
    /* E_BUFOVFLH */	"Buffer overflow in host",
    /* E_BUFOVFLT */	"Buffer overflow in target",
    /* E_COMM_ERR */	"Communication failure",
    /* E_COMM_TIMO */	"Communication timed out",
    /* E_EEPROM_ADDR */	"Invalid EEPROM address or length",
    /* E_EEPROM_PROG */	"EEPROM is not erased",
    /* E_EEPROM_FAIL */	"Attempt to erase or program EEPROM failed",
    /* E_FILE_ERR */	"%s: Error reading file %s",
    /* E_INTR */	"Function terminated by keyboard interrupt",
    /* E_NOMEM */	"Unable to allocate memory on the host",
    /* E_NOTRUNNING */	"Target is not running",
    /* E_RUNNING */	"Target is running",
    /* E_VERSION */	"Not supported in this version",
    /* E_READ_ERR */	"Error reading target memory",
    /* E_WRITE_ERR */	"Error writing to target memory",
    /* E_VERIFY_ERR */	"Write verification error",
    /* E_BAD_CONFIG */	"Unknown device or illegal configuration",
    /* E_SYS_ERR */	"%s: System error %s",
    /* E_RESET */	"Target was reset",
    /* E_BAD_MAGIC */	"File %s is not an i960 executable (bad magic number)",
    /* E_SWBP_ERR */	"Unable to write sw breakpoint (write verification error)",
    /* E_NO_FLASH */	"Target flash region is not functional flash memory",
    /* E_OLD_MON */	"Not supported in old monitor",
    /* E_TARGET_RESET */        "Target was in reset state, board was reset",
    /* E_PARALLEL_DOWNLOAD_NOT_SUPPORTED */ "Target does not support parallel downloads",
    /* E_FAST_DOWNLOAD_BAD_FORMAT */   "Non download message at fast download port",
    /* E_FAST_DOWNLOAD_BAD_DATA_CHECKSUM */   "Data CRC does not match data at download port",
    /* E_PARA_DNLOAD_TIMO */	"Parallel download timeout",
    /* E_PCI_COMM_NOT_SUPPORTED */ "Target does not support PCI communication",
    /* E_NUM_CONVERT */	"Invalid numeric value",
    /* E_NO_PCIBIOS  */	"PCI BIOS unavailable",
    /* E_PCI_ADDRESS */	"Invalid PCI address component specified",
    /* E_PCI_MULTIFUNC */   "PCI device not multi-function",
    /* E_PCI_CFGREAD */	"Error reading PCI configuration space",
    /* E_PCI_CFGWRITE */	"Error writing PCI configuration space",
    /* E_PCI_HOST_PORT  */	"PCI comm unsupported for host OS on specified target",
    /* E_PCI_NODVC   */	"No PCI device at specified address",
    /* E_PCI_SRCH_FAIL */	"Search for specified PCI device failed",
    /* E_PHYS_MEM_FREE */	"Unable to free mapped, physical memory",
    /* E_PHYS_MEM_MAP */	"Physical memory mapping unavailable",
    /* E_PHYS_MEM_ALLOC */	"Unable to alloc mapped, physical memory",
    /* E_CONTROLLING_PORT */	
                        "Specified target not controlled by communication port",
    /* E_PORT_SEARCH */ "Cannot locate controlling communication port",
    /* E_PCI_COMM_TIMO */	"PCI communication timeout",
    /* E_FAST_DNLOAD_ERR */	"FAST download error bit set by MON960",
    /* E_PARA_NOCOMM  */	"Parallel communication unsupported for host OS and/or host parallel HW",
    /* E_PARA_SYS_ERR */	"%s: Parallel comm system error %s",
    /* E_PARA_WRITE */	"Parallel communication I/O error: write mismatch",
    /* E_ARG_EXPECTED */	"Argument expected",
    /* E_ELF_CORRUPT  */	"ELF file %s corrupt: %s",
    /* E_APLINK_SWITCH2 */	"Only one switch command allowed following reset",
    /* E_APLINK_REGION */   "Region conflicts with dedicated processor resources",
    /* E_GMU_BADREG */      "The specified GMU register does not exist",
    /* E_COMM_PROTOCOL */   "Communication protocol unspecified or unsupported",
};

#define N_ERRS 61

#if N_ERRS != HDI_N_ERRS /* from errs.h */
error "errlist out of date";
#endif

extern int hdi_cmd_stat;
extern int _hdi_save_errno;
#ifndef I386_NBSD1
extern char *sys_errlist[];
#else
extern const char *const sys_errlist[];
#endif

const char *
hdi_get_message()
{
    static char buf[256];

    if (hdi_cmd_stat < 0 || hdi_cmd_stat > N_ERRS)
         return (errlist[0]);

    if (hdi_cmd_stat == E_SYS_ERR || 
                hdi_cmd_stat == E_FILE_ERR || 
                           hdi_cmd_stat == E_PARA_SYS_ERR)
    {
        sprintf(buf,
                errlist[hdi_cmd_stat],
                sys_errlist[_hdi_save_errno],
                hdi_err_param1,
                hdi_err_param2,
                hdi_err_param3);
    }
    else
    {
        sprintf(buf, 
                errlist[hdi_cmd_stat], 
                hdi_err_param1,
                hdi_err_param2,
                hdi_err_param3);
    }

    return (buf);
}
