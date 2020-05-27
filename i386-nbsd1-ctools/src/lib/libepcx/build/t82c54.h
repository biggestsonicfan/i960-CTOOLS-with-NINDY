
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

/* CONSTANTS USED FOR 82C54 TIMER PROGRAMMING */
#define TIMER_BASE      0x40000000
#define COUNTER_0       TIMER_BASE+0x0
#define COUNTER_1       TIMER_BASE+0x1
#define COUNTER_2       TIMER_BASE+0x2
#define CONTROL_REG     TIMER_BASE+0x3

#define  ROLL_32_BITS   0xFFFFFFFF
#define TIMER_VECTOR 	226

/* CRYSTAL DIVISOR */
#define CRYSTALTIME     10   /* for the 10 MHz crystal */

#define CLK_SCALER		10000            /* Turns clock rate floating point
										 representation into an integer */
#define T8254CLKMHZ		10.0           /* If > 4 decimal places inc scaler */
#define SCALED_CLK		10000            /* T8254CLKMHZ * CLK_SCALER */
										 /* Integer form of clock */
#define COUNT0_VAL		0xc350           /* 5 msec at 10 mz clock */
#define COUNT1_VAL  	0xbb80           /* 4 minutes at 5msec clock */
#define COUNT2_VAL  	0x2              /* Interrupt pulse */
#define COUNT1USECS     5000             /* COUNT0_VAL/T8254CLKMHZ = micro secs */
#define USECS_PER_INT	((COUNT1_VAL * COUNT1USECS) + COUNT1USECS)
										/* Each interrupt is worth the micro
										seconds in a full counter 1 PLUS one
										more counter 1 output pulse width
										 (in microseconds).
										This represents the	additional time
										(one counter 1 clock) required to
										trigger the one-shot (counter
										2) that generates the interrupt. */

/* Control Word Format */
#define SC(sc)    ((sc)<<6)
#define RW(rw)    ((rw)<<4)
#define MODE(m)   ((m)<<1)
#define BCD       (1)

/* Readback Commands */
#define READBACK     (3<<6)
#define LATCH_COUNT  (1<<4)
#define LATCH_STATUS (1<<5)
#define LATCH_CNT_STAT 0xcf
#define CNT_0        (1<<1)
#define CNT_1        (1<<2)
#define CNT_2        (1<<3)

/* Status Bits */
#define OUTPUT_BIT      (1<<7)
#define NULL_COUNT_BIT  (1<<6)
#define RW1_BIT         (1<<5)
#define RW0_BIT         (1<<4)
#define M2_BIT          (1<<3)
#define M1_BIT          (1<<2)
#define M0_BIT          (1<<1)
#define BCD_BIT         (1<<0)
