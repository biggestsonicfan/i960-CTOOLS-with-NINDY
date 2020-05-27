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

/* $Header: /ffs/p1/dev/src/hdil/common/RCS/hdi_mcfg.h,v 1.12 1995/10/04 18:46:41 cmorgan Exp $ */

/***************************************************************************
 *
 * NAME:         hdi_mcfg.h
 *
 * DESCRIPTION:  Declare the data structure used to transfer
 *               monitor info between host and target.
 *
 ***************************************************************************/

#ifndef HDI_MCFG_H
#define HDI_MCFG_H

#ifndef HDI_COMMON_H
#   include "common.h"
#endif

#define HDI_MONITOR_RELOC       0x01  /* Monitor is relocatable (pic/pid). */
#define HDI_MONITOR_BENDIAN     0x02  /* Monitor is big endian.  If this bit
                                       * is not set, monitor is little endian.
                                       */
            /*
             * ---------------------------------------------------
             * Warning -- The next 2 bits are mutually exclusive *
             *
             * Also, if neither HDI_MONITOR_TRGT_HW_TMR nor
             * HDI_MONITOR_CPU_HW_TMR are defined, the host may 
             * infer that the target either has no timer HW or
             * else was not ported to Mon960's timer API.
             * ---------------------------------------------------
             */
#define HDI_MONITOR_TRGT_HW_TMR 0x04  /* Target implements timer opns (e.g.,
                                       * bentime, ghist, etc.) via dedicated
                                       * target HW.
                                       */
#define HDI_MONITOR_CPU_HW_TMR  0x08  /* Target implements timer opns via
                                       * CPU HW (e.g., Jx/Hx timers).
                                       */


#define HDI_CFG_PARA_COMM       0x01  /* Bidirectional parallel comm */
#define HDI_CFG_PARA_DNLD       0x02  /* Parallel download only      */
#define HDI_CFG_PCI_CAPABLE     0x04  /* PCI comm                    */
#define HDI_CFG_I2C_CAPABLE     0x08  /* I2C comm                    */
#define HDI_CFG_JTAG_CAPABLE    0x10  /* JTAG comm                   */
#define HDI_CFG_SERIAL_CAPABLE  0x20  /* Serial comm                 */

#define HDI_MAX_CFG_UNUSED         4
#define HDI_MAX_FLSH_BNKS          4
#define HDI_MAX_TRGT_NAME         11

typedef struct
{
    ADDR           flash_addr[HDI_MAX_FLSH_BNKS];    
                                     /* Starting address of target flash bank
                                      * 0-(HDI_MAX_FLSH_BNKS - 1).  An array
                                      * element is N/A for this target if
                                      * the correponding flash_size[]
                                      * element is zero.  N/A elements
                                      * are usually initialized to zero,
                                      * but not always (see the "Example
                                      * scenarios" comment in the description
                                      * of flash_size below).
                                      */
    unsigned long  flash_size[HDI_MAX_FLSH_BNKS];
                                     /* Size, in bytes of each element of
                                      * the flash_addr[] array.  Zero means
                                      * no writable flash in this bank. 
                                      *
                                      * Example scenarios:
                                      * 
                                      * 1) a flash part is socketed, but
                                      *    the proper Vpp is not present and
                                      *    so the returned size info is 0.
                                      * 2) when an eprom is placed in a
                                      *    flash socket bank its returned
                                      *    size info will be 0.
                                      */
    ADDR           ram_start_addr;   /* DRAM starting address.            */
    unsigned long  ram_size;         /* DRAM size, 0 if info not available
                                      * (e.g., epcx target).
                                      */
    ADDR           unwritable_addr;  /* An address to which the processor
                                      * cannot write, -1 if N/A for this
                                      * target.
                                      */
    unsigned long  step_info;        /* CPU stepping information (g0 at
                                      * reset).  Will be -1 for Kx and Sx
                                      * processors.
                                      */
    short          arch;             /* Processor architecture, possible
                                      * values defined hdi_arch.h 
                                      */
    short          bus_speed;        /* Processor bus speed (MHz). */
    short          cpu_speed;        /* Processor cpu speed (MHz). */
    short          monitor;          /* Bit mask describing selected target 
                                      * features.  See HDI_MONITOR macro 
                                      * defns above for more details.
                                      */
    short          inst_brk_points;  /* # i960 HW instruct bp's.       */
    short          data_brk_points;  /* # i960 HW data bp's.           */
    short          fp_regs;          /* # i960 FP registers.           */
    short          sf_regs;          /* # i960 special func registers. */
    short          comm_cfg;         /* Bit mask describing monitor's
                                      * communication capabilities.  See
                                      * HDI_CFG macro defns above for more
                                      * details.
                                      */
    char           trgt_common_name[HDI_MAX_TRGT_NAME + 1];
                                     /* Null-terminated string indicating
                                      * target's commonly known name which
                                      * is usually not the same as its
                                      * product name (e.g., EP80960CX). 
                                      * Names returned by Intel targets:
                                      *
                                      *   cy[chjks]x         Cyclone  LE
                                      *   cy[chj]xbe         Cyclone  BE
                                      *   epcx
                                      *   epcxbe             BE (big endian)
                                      *   ap[cjs]x           ApLink   LE
                                      *   ap[cj]xbe          ApLink   BE
                                      */
    short          config_version;   /* Version of HDI_MON_CONFIG message
                                      * supported by monitor.  Bump this
                                      * field whenever an "unused" field is
                                      * claimed by HDI and the monitor for
                                      * a new purpose.  Range of this field
                                      * is -1 to HDI_MAX_CFG_UNUSED - 1.
                                      * When this field is -1, all unused 
                                      * elements are available.
                                      */
    unsigned long  unused[HDI_MAX_CFG_UNUSED];
                                    /* Unused elements initialized to 0. */
} HDI_MON_CONFIG;

#endif /* ! HDI_MCFG_H */
