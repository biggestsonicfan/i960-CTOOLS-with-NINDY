
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

#include "std_defs.h"

#define DUART        0x60000000
#define DUART_DELTA  1
#define DFLTPORT     0         /* in 16552 chan 2 = offset 0 works for 16550 */
#define ACCESS_DELAY 5
#define XTAL         7372800   /* #define XTAL        18432000 */

#define PP_DATA_ADDR   0x20000000
#define PP_STAT_ADDR   0x21000000
#define PP_CTRL_ADDR   0x22000000
#define PP_READ_STATUS 0x32
#define PP_ERR_BIT     0x01        /* ERROR bit to end parallel transfer */
                                   /* negative logic 1 = no error */
#define PP_SEL_BIT     0x02        /* select for initial connect and transfers*/
#define PP_POUT_BIT    0x04        /* paper out bit for inital connect*/
#define PP_INIT_BITS   ((PP_ERR_BIT | PP_SEL_BIT) & ~PP_POUT_BIT)

/* CONSTANTS USED FOR 82C54 TIMER PROGRAMMING */
#define TIMER_BASE      0x40000000
#define TIMER0_VECTOR   226
#define TIMER0_IRQ      6
#define TIMER0_OFFSET   1
#define TIMER1_VECTOR   210
#define TIMER1_IRQ      5
#define TIMER1_OFFSET   2
#define CRYSTALTIME     10   /* for the 10 MHz crystal */
#define COUNTER_0       TIMER_BASE+0x0
#define COUNTER_1       TIMER_BASE+0x1
#define COUNTER_2       TIMER_BASE+0x2
#define CONTROL_REG     TIMER_BASE+0x3

#define SWITCH_ADDR      0x23000000
#define LED_8SEG_ADDR    0x24000000
#define BLINK_PAUSE      0x40000

#define DISPLAY0    0x3f    /* codes to display characters on 7-seg LED */
#define DISPLAY1    0x06
#define DISPLAY2    0x5b
#define DISPLAY3    0x4f
#define DISPLAY4    0x66
#define DISPLAY5    0x6d
#define DISPLAY6    0x7d
#define DISPLAY7    0x07
#define DISPLAY8    0x7f
#define DISPLAY9    0x67
#define DISPLAYA    0x77
#define DISPLAYB    0x7c
#define DISPLAYC    0x39
#define DISPLAYD    0x5e
#define DISPLAYE    0x79
#define DISPLAYF    0x71

/*
 * Special codes to Clear the display and display the "dot".
 */
#define CLR_DISP    0x00
#define DOT         0x80

#if BIG_ENDIAN_CODE
#define BYTE_ORDER BIG_ENDIAN(1)
#else
#define BYTE_ORDER BIG_ENDIAN(0)
#endif

/* Bus configuration */
#define EPROM    (BUS_WIDTH(8) | PIPELINE(0) | READY(0) | BURST(0) | \
    DATA_CACHE(1) |BYTE_ORDER | NRAD(8) | NRDD(0) | NXDA(1) | NWAD(8) | NWDD(0))

#define DRAM     (BUS_WIDTH(32) | PIPELINE(0) | READY(1) | BURST(1) | \
    DATA_CACHE(1) |BYTE_ORDER | NRAD(0) | NRDD(0) | NXDA(0) | NWAD(0) | NWDD(0))

#define SRAM     (BUS_WIDTH(32) | PIPELINE(1) | READY(0) | BURST(1) | \
    BYTE_ORDER | NRAD(0) | NRDD(0) | NXDA(0) | NWAD(1) | NWDD(1))

#define I_O     (BUS_WIDTH(8) | PIPELINE(0) | READY(0) | BURST(0) | \
    BYTE_ORDER | NRAD(13) | NRDD(0) | NXDA(3) | NWAD(13) | NWDD(0))

#define UART_DEV (BUS_WIDTH(8) | PIPELINE(0) | READY(0) | BURST(0) | \
    BYTE_ORDER | NRAD(9) | NRDD(3) | NXDA(1) | NWAD(9) | NWDD(3))

#define TIMER     (BUS_WIDTH(8) | PIPELINE(0) | READY(0) | BURST(0) | \
    BYTE_ORDER | NRAD(13) | NRDD(3) | NXDA(2) | NWAD(13) | NWDD(3))

#define XBUS1     (BUS_WIDTH(8) | PIPELINE(0) | READY(0) | BURST(0) | \
    BYTE_ORDER | NRAD(13) | NRDD(0) | NXDA(3) | NWAD(13) | NWDD(0))

#define XBUS2   (BUS_WIDTH(8) | PIPELINE(0) | READY(0) | BURST(0) | \
    BYTE_ORDER | NRAD(13) | NRDD(0) | NXDA(3) | NWAD(13) | NWDD(0))

#define DEFAULT (BUS_WIDTH(8) | PIPELINE(0) | READY(1) | BURST(0) | \
    BYTE_ORDER | NRAD(0) | NRDD(0) | NXDA(0) | NWAD(0) | NWDD(0))

#define  REGION_0_CONFIG   DEFAULT
#define  REGION_1_CONFIG   DEFAULT
#define  REGION_2_CONFIG   I_O
#define  REGION_3_CONFIG   DEFAULT
#define  REGION_4_CONFIG   TIMER
#define  REGION_5_CONFIG   DEFAULT
#define  REGION_6_CONFIG   UART_DEV
#define  REGION_7_CONFIG   DEFAULT
#define  REGION_8_CONFIG   XBUS2
#define  REGION_9_CONFIG   DEFAULT
#define  REGION_A_CONFIG   XBUS1
#define  REGION_B_CONFIG   DEFAULT
#define  REGION_C_CONFIG   DRAM
#define  REGION_D_CONFIG   DRAM
#define  REGION_E_CONFIG   DEFAULT
#define  REGION_F_CONFIG   EPROM

