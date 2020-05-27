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

typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;

#define shift(index, dtype)     (index % (sizeof(dtype) * 8))
#define num_dtypes(sz, dtype)   (sz / sizeof(dtype))

#define     DRAM_SCAN 19     /* i< 19 is 18 bits of 4 byte words or 20 bits */
#define     DRAM_OFFSET         0x00080000
#define     DRAM_START          (void *) 0xC0080000
#define		DRAM_SCAN_RESULT 	*(unsigned long *)0x40UL

#define  SW_8   0x80
#define  SW_7   0x40
#define  SW_6   0x20
#define  SW_5   0x10
#define  SW_4   0x08
#define  SW_3   0x04
#define  SW_2   0x02
#define  SW_1   0x01

#define DRAM_BYTE_TEST       1
#define DRAM_SHORT_TEST      2
#define DRAM_LONG_TEST       3
#define DRAM_TRIP_TEST       4
#define DRAM_QUAD_TEST       5
#define DRAM_WORD_TEST       6
#define TIMER_TEST           7
#define BUS_IDLE_TEST        8
#define SERIAL_INTFC_TEST    9
#define SERIAL_LOOP_TEST     0xa
