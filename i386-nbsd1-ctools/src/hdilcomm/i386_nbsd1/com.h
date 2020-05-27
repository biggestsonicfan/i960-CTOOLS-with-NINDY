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
/*
 * Internal to communication system
 * $Header: /tmp_mnt/ffs/p1/dev/src/hdilcomm/common/RCS/com.h,v 1.9 1995/11/27 20:15:17 cmorgan Exp $$Locker:  $
 */

#ifndef HDILCOMM_COM_H
#define HDILCOMM_COM_H

/*
 * Device type definition:
 */
#define  NO_COMM      0
#define  RS232        1
#define  HDI_PARALLEL     2
#define  HDI_PCI_COMM     3
#define  HDI_I2C          4
#define  HDI_DEV_COUNT    4

#define  RS232_BAUD   9600L

/* ---------------------- Host-Related Init Structures ----------------- */
#ifdef HOST

#ifndef HDI_COM_H
#   include "hdi_com.h"
#endif

typedef struct 
{
    int             dev_type;   /* Selects COMM channel used by HDI.
                                 * Should be either RS232 or HDI_PCI_COMM. 
                                 */
    unsigned int    iobase;     /* I/O base value.  RS232 only. */
    unsigned long   baud;       /* RS232 only. */
    unsigned long   freq;       /* UART input frequency in Hz. RS232 only. */
    unsigned short  host_pkt_timo;
    unsigned short  target_pkt_timo;
    unsigned short  ack_timo;
    unsigned short  max_len;
    COM_PCI_CFG     pci;        /* PCI comm only. */
    int             pci_debug;  /* Boolean, T -> PCI debug is enabled. */
    unsigned char   max_try;
    unsigned char   irq;        /* Host interrupt vector.  RS232 only. */
    char            device[20]; /* RS232 port name. RS232 only. */
} COM_CONFIG;

/* defintion for the _com_config_set.structure */
#define UNSET          0
#define CMD_LINE       1
#define INI_FILE       2
#define ERR_CMD_LINE  -1
#define ERR_INI_FILE  -2

typedef struct 
{
   short device,
         iobase,
         irq,
         dev_type,
         baud,
         freq,
         host_pkt_timo,
         target_pkt_timo,
         ack_timo,
         max_len,
         max_try;
} COM_CONFIG_SET;


extern COM_CONFIG_SET _com_config_set;
extern COM_CONFIG     _com_config;

/* Macro to determine if the HOST's pci debug flag is enabled. */
#define PCI_DEBUG()   (_com_config.pci_debug)

#endif  /* defined HOST */


extern int com_stat;


#endif  /* ! defined HDILCOMM_COM_H */
