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

/* bank 0 registers (8250 mode) */
#define TXD   0
#define RXD   0
#define BAL   0
#define BAH   1
#define GER   1
#define GIR   2
#define BANK  2
#define LCR   3
#define MCR   4
#define LSR   5
#define MSR   6
#define ACR0  7

/* bank 1 registers */
#define RXF   1
#define TXF   1
#define TMST  3
#define TMCR  3
#define FLR   4
#define RST   5
#define RCM   5
#define TCM   6
#define GSR   7
#define ICM   7

/* bank 2 registers */
#define FMD    1
#define TMD    3
#define IMD    4
#define ACR1   5
#define RIE    6
#define RMD    7

/* bank 3 registers */
#define CLCF   0
#define BBL    0
#define BACF   1
#define BBH    1
#define BBCF   3
#define PMD    4
#define MIE    5
#define TMIE   6

/* bank change commands */
#define NAS0    0
#define WORK1   0x20
#define GEN2    0x40
#define MODM3   0x60

/* other commands */
#define OUT2_MCR     0x08
#define DTR_MCR      0x01
#define DSR_MSR      0x20
#define CLRSTAT_ICM  0x04
#define MCR_LOOP     0x10

/* common bit definitions */
/* LSR */
#define TXEMT   0x40      /* nothing in out shift */
#define TXRDY   0x20

/* RST */
#define RXRDY   0x01
#define OE      0x02
#define PE      0x04
#define FE      0x08
#define BR      0x10
#define BRK_TERM 0x20

/* GSR */
#define RFIR	0x01
#define TFIR	0x02

