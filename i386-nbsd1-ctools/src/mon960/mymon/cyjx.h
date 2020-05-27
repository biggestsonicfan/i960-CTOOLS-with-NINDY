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

/*************************************************************************** 
*	Written:	23 May 1994, Scott Coulter - changed for c145/c146 CX
*       Revised:
*
***************************************************************************/

#include "c145.h"

/* Conversion macros for Cx-to-Jx bus configs - needed for Squall EEPROM*/
#define CX_BUS_WIDTH    0x00180000
#define CX_BUS_8        0x00000000
#define CX_BUS_16       0x00080000
#define CX_BUS_32       0x00100000

/* Bus configuration */
/* For ibr region F description */
#define BYTE_0(x)	( (x)        & 0xff)
#define BYTE_1(x)	(((x) >>  8) & 0xff)
#define BYTE_2(x)	(((x) >> 16) & 0xff)
#define BYTE_3(x)	(((x) >> 24) & 0xff)

#if BIG_ENDIAN_CODE
#define PCI_BUS	        (BUS_WIDTH(32) | BIG_ENDIAN(1))
#define DRAM	        (BUS_WIDTH(32) | BIG_ENDIAN(1))
#define FLASH_ROM       (BUS_WIDTH(8) | BIG_ENDIAN(1))
#else /*LE*/
#define PCI_BUS	        BUS_WIDTH(32) 
#define DRAM	        BUS_WIDTH(32) 
#define FLASH_ROM	    BUS_WIDTH(8)
#endif /* __i960_BIG_ENDIAN__ */

#define UART_CIO_PARA   BUS_WIDTH(8)
#define SQUALL_1	    BUS_WIDTH(32)

#define  REGION_0_CONFIG   PCI_BUS 
#define  REGION_2_CONFIG   PCI_BUS
#define  REGION_4_CONFIG   PCI_BUS
#define  REGION_6_CONFIG   PCI_BUS
#define  REGION_8_CONFIG   PCI_BUS
#define  REGION_A_CONFIG   DRAM     /* also conatins uart, cio and parallel */
#define  REGION_C_CONFIG   SQUALL_1	/* Fix in initialization routines */
#define  REGION_E_CONFIG   FLASH_ROM
