
/*(c****************************************************************************** *
 * Copyright (c) 1990, 1991, 1992, 1993 Intel Corporation
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
 *****************************************************************************c)*/

/*********************************************************************
 * Magic addresses, etc., for NINDY when running on Intel TomCAt board.
 * Used by NINDY monitor and TomCAt library (80960) code.
 *********************************************************************/

/* CONSTANTS USED FOR TIMER PROGRAMMING */
#define TC_CR0 0xdfe00000
#define TC_CR1 0xdfe00001
#define TC_CR2 0xdfe00002
#define TC_CWR 0xdfe00003

#define TIMER_VECTOR    226
#define LAN_VECTOR    	210

/* CRYSTAL DIVISOR */
#define CRYSTALTIME     98304   /* for the 9.8304 MHz crystal */

/* CONSTANTS USED FOR FLASH PROGRAMMING */
#define		FLASH_ADDR	0x20000000
#define		MAXFLASH	0x400000
