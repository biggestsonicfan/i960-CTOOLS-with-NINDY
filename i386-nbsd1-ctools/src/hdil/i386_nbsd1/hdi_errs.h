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
/* $Header: /ffs/p1/dev/src/hdil/common/RCS/hdi_errs.h,v 1.26 1995/11/05 07:32:04 cmorgan Exp $$Locker:  $ */

#ifndef HDI_ERRS_H
#define HDI_ERRS_H

/* Error code Constants */

#define	E_ALIGN		1
#define	E_ARCH		2
#define	E_ARG		3
#define E_BAD_CMD	4
#define	E_BPNOTSET	5
#define	E_BPSET		6
#define	E_BPUNAVAIL	7
#define	E_BUFOVFLH	8
#define	E_BUFOVFLT	9
#ifdef HOST
#define	E_BUFOVFL	E_BUFOVFLH
#else /* HOST */
#define E_BUFOVFL	E_BUFOVFLT
#endif /* HOST */
#define	E_COMM_ERR	10
#define	E_COMM_TIMO	11
#define	E_EEPROM_ADDR	12
#define	E_EEPROM_PROG	13
#define	E_EEPROM_FAIL	14
#define	E_FILE_ERR	15
#define	E_INTR		16
#define	E_NOMEM		17
#define	E_NOTRUNNING	18
#define	E_RUNNING	19
#define	E_VERSION	20
#define	E_READ_ERR	21
#define	E_WRITE_ERR	22
#define	E_VERIFY_ERR	23
#define	E_BAD_CONFIG	24
#define E_SYS_ERR	25
#define E_RESET		26
#define E_BAD_MAGIC	27
#define E_SWBP_ERR	28
#define E_NO_FLASH	29
#define E_OLD_MON	30
#define E_TARGET_RESET	31
#define E_PARALLEL_DOWNLOAD_NOT_SUPPORTED 32
#define E_FAST_DOWNLOAD_BAD_FORMAT 33
#define E_FAST_DOWNLOAD_BAD_DATA_CHECKSUM 34
#define E_PARA_DNLOAD_TIMO	35
#define E_PCI_COMM_NOT_SUPPORTED 36
#define E_NUM_CONVERT	37
#define E_NO_PCIBIOS	38
#define E_PCI_ADDRESS	39
#define E_PCI_MULTIFUNC	40
#define E_PCI_CFGREAD	41
#define E_PCI_CFGWRITE	42
#define E_PCI_HOST_PORT	43
#define E_PCI_NODVC		44
#define E_PCI_SRCH_FAIL	45
#define E_PHYS_MEM_FREE	46
#define E_PHYS_MEM_MAP	47
#define E_PHYS_MEM_ALLOC	48
#define E_CONTROLLING_PORT	49
#define E_PORT_SEARCH	50
#define E_PCI_COMM_TIMO	51
#define E_FAST_DNLOAD_ERR	52
#define E_PARA_NOCOMM	53
#define E_PARA_SYS_ERR	54
#define E_PARA_WRITE	55
#define E_ARG_EXPECTED	56
#define E_ELF_CORRUPT 	57
#define E_APLINK_SWITCH2 	58
#define E_APLINK_REGION 59
#define E_GMU_BADREG	60
#define E_COMM_PROTOCOL	61

#define HDI_N_ERRS 61

/*
 * Define a macro that can be used to test an HDI error code to see if some
 * version of the monitor (potentially quite old) rejected an HDI request
 * as "unimplemented".  I.E., the monitor thinks the request is bogus
 * simply because the monitor does not implement that particular service. 
 * Error codes returned by various released monitors for this situation are:
 *
 *     E_ARCH     ver >= 2.1.63 && ver <= 2.2.5Alpha
 *     E_BAD_CMD  ver < 2.1.63
 *     E_VERSION  ver > 2.2.5Alpha
 *
 * Use this macro with great discretion!  I.E., use this macro only when
 * you suspect that an HDI service request might fail on an older monitor.
 * Unfortunately, the monitor and HDI will return E_ARCH and E_BAD_CMD for 
 * reasons other than "command unimplemented."
 */
#define HDI_SERVICE_UNIMP(x) ((x) == E_ARCH ||  \
                                       (x) == E_BAD_CMD || \
                                                      (x) == E_VERSION)

extern char *hdi_err_param1, *hdi_err_param2, *hdi_err_param3;

#endif /* HDI_ERRS_H */
