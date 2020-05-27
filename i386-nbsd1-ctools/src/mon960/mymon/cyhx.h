/*(cb*/
/**************************************************************************
 *
 *     Copyright (c) 1992 Intel Corporation.  All rights reserved.
 *
 *
 * Intel hereby grants you permission to copy, modify, and distribute this
 * software and its documentation.  Intel grants this permission provided
 * that the above copyright notice appears in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  In addition, Intel grants this permission provided that
 * you prominently mark as not part of the original any modifications made
 * to this software or documentation, and that the name of Intel
 * Corporation not be used in advertising or publicity pertaining to the
 * software or the documentation without specific, written prior
 * permission.
 *
 * Intel provides this AS IS, WITHOUT ANY WARRANTY, INCLUDING THE WARRANTY
 * OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, and makes no
 * guarantee or representations regarding the use of, or the results of the
 * use of, the software and documentation in terms of correctness,
 * accuracy, reliability, currentness, or otherwise, and you rely on the
 * software, documentation, and results solely at your own risk.
 *
 **************************************************************************/
/*)ce*/

#include "c145.h"

/***************************************************************************
*     Written:     23 May 1994, Scott Coulter - changed for c145/c146 CX
*       Revised:
*
***************************************************************************/

/* Conversion unions for Cx-to-Hx bus configs - needed for Squall EEPROM*/

typedef union {
    struct {
     unsigned long   burst_enab : 1;
     unsigned long   ready_enab : 1;
     unsigned long   pipeline_enab : 1;
     unsigned long   nrad : 5;
     unsigned long   nrdd : 2;
     unsigned long   nxda : 2;
     unsigned long   nwad : 5;
     unsigned long   nwdd : 2;
     unsigned long   bus_width : 2;
     unsigned long   res1 : 1;
     unsigned long   byte_order : 1;
     unsigned long   dcache_enab : 1;
     unsigned long   res2 : 8;
    } bits;
    unsigned long  mcon;
} CX_MCON;

typedef union {
    struct {
     unsigned long   nrad : 5;
     unsigned long   res1 : 1;
     unsigned long   nrdd : 2;
     unsigned long   nwad : 5;
     unsigned long   res2 : 1;
     unsigned long   nwdd : 2;
     unsigned long   nxda : 4;
     unsigned long   parity_enab : 1;
     unsigned long   parity_odd : 1;
     unsigned long   bus_width : 2;
     unsigned long   pipeline_enab : 1;
     unsigned long   res3 : 3;
     unsigned long   burst_enab : 1;
     unsigned long   ready_enab : 1;
     unsigned long   res4 : 2;
    } bits;
    unsigned long  mcon;
} HX_MCON;


/* Bus configuration */
/*
 * Perform a bit-wise OR of the desired parameters to specify a region.
 *
 *
 *  bus_region_1_config =
 *      BURST_ENABLE | BUS_WIDTH(32) | NRAD(3) | NRDD(1) |
 *      NXDA(1) | NWAD(2) | NWDD(2);
 */


/* For ibr region F description
 */
#define BYTE_0(x)     ( (x)        & 0xff)
#define BYTE_1(x)     (((x) >>  8) & 0xff)
#define BYTE_2(x)     (((x) >> 16) & 0xff)
#define BYTE_3(x)     (((x) >> 24) & 0xff)

#if BIG_ENDIAN_CODE
#define BYTE_ORDER BIG_ENDIAN(1)
#else
#define BYTE_ORDER BIG_ENDIAN(0)
#endif /* __i960_BIG_ENDIAN__ */


#define PCI_BUS          (BURST(1) | READY(1) | PIPELINE(0) | \
                    NRAD(0) | NRDD(0) | NXDA(0) | NWAD(0) | \
                    NWDD(0) | BUS_WIDTH(32) | BYTE_ORDER) 
#define DRAM          (BURST(1) | READY(1) | PIPELINE(0) | \
                    NRAD(0) | NRDD(0) | NXDA(0) | NWAD(0) | \
                    NWDD(0) | BUS_WIDTH(32) | BYTE_ORDER)
#define UART_CIO_PARA     (BURST(0) | READY(1) | PIPELINE(0) | \
                    NRAD(0) | NRDD(0) | NXDA(0) | NWAD(0) | \
                    NWDD(0) | BUS_WIDTH(8))
#define SQUALL_1     (BURST(0) | READY(1) | PIPELINE(0) | \
                    NRAD(0) | NRDD(0) | NXDA(0) | NWAD(0) | \
                    NWDD(0) | BUS_WIDTH(8))
#define RESERVED_AREA     (BURST(0) | READY(1) | PIPELINE(0) | \
                    NRAD(0) | NRDD(0) | NXDA(0) | NWAD(0) | \
                    NWDD(0) | BUS_WIDTH(8))
#define FLASH_ROM     (BURST(0) | READY(1) | PIPELINE(0) | \
                    NRAD(0) | NRDD(0) | NXDA(0) | NWAD(0) | \
                    NWDD(0) | BUS_WIDTH(8))

/* workaround because processor doesn't figure out that the region
   isn't Ready enabled until it reads the 4th byte of the IBR, so set up
   the NRAD value to equal the value which would be set by using Ready */
#define BOOT_FLASH_ROM     (BURST(0) | READY(1) | PIPELINE(0) | \
                    NRAD(8) | NRDD(0) | NXDA(3) | NWAD(0) | \
                    NWDD(0) | BUS_WIDTH(8))

#define  REGION_0_CONFIG   PCI_BUS
#define  REGION_1_CONFIG   PCI_BUS
#define  REGION_2_CONFIG   PCI_BUS
#define  REGION_3_CONFIG   PCI_BUS
#define  REGION_4_CONFIG   PCI_BUS
#define  REGION_5_CONFIG   PCI_BUS
#define  REGION_6_CONFIG   PCI_BUS
#define  REGION_7_CONFIG   PCI_BUS
#define  REGION_8_CONFIG   PCI_BUS
#define  REGION_9_CONFIG   PCI_BUS

#define  REGION_A_CONFIG   DRAM
#define  REGION_B_CONFIG   UART_CIO_PARA
#define  REGION_C_CONFIG   SQUALL_1     /* Fix in initialization routines */
#define  REGION_D_CONFIG   RESERVED_AREA
#define  REGION_E_CONFIG   FLASH_ROM
#define  REGION_F_CONFIG   FLASH_ROM
#define  REGION_BOOT_CONFIG   BOOT_FLASH_ROM | BYTE_ORDER
