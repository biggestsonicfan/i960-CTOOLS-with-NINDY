
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

#define  ROLL_32_BITS         0xFFFFFFFF

#define COUNT0_VAL            0xc350   /* 5 msec at 10 mz clock */
#define COUNT1_VAL            0xbb80   /* 4 minutes at 5msec clock */
#define COUNT2_VAL            0x2      /* Interrupt pulse */
#define COUNT1USECS           5000     /* COUNT0_VAL/T8254CLKMHZ */
#define USECS_PER_INT         ((COUNT1_VAL * COUNT1USECS) + COUNT1USECS)
                                /* Each interrupt is worth the micro
                                   seconds in a full counter 1 PLUS one
                                   more counter 1 output pulse width
                                   (in microseconds).
                                   This represents the additional time
                                   (one counter 1 clock) required to
                                   trigger the one-shot (counter 2)
                                   that generates the interrupt. */

/* READBACK COMMAND FORMATS */
#define READBACK             (3<<6)
#define DONT_LATCH_COUNT     (1<<5)
#define DONT_LATCH_STATUS    (1<<4)

#define RB_COUNTER0          (1<<1)
#define RB_COUNTER1          (1<<2)
#define RB_COUNTER2          (1<<3)

#define DONT_LATCH_CNT_STAT   READBACK | DONT_LATCH_COUNT | DONT_LATCH_STATUS | RB_COUNTER0 | RB_COUNTER1 | RB_COUNTER2

/* Status Bits */
#define OUTPUT_BIT           (1<<7)
#define NULL_COUNT_BIT       (1<<6)
#define RW1_BIT              (1<<5)
#define RW0_BIT              (1<<4)
#define M2_BIT               (1<<3)
#define M1_BIT               (1<<2)
#define M0_BIT               (1<<1)
#define BCD_BIT              (1<<0)


/* CONTROL WORD FORMATS */
#define SC(sc)               ((sc)<<6)
#define RW(rw)               ((rw)<<4)
#define MODE(m)              ((m)<<1)

#define CTRL_COUNTER0        0
#define CTRL_COUNTER1        (1<<6)
#define CTRL_COUNTER2        (1<<7)
#define READ_BACK            ((1<<7) | (1<<6))
#define LATCH_CMD            0

#define RW_LSB               (1<<4)
#define RW_MSB               (1<<5)
#define RW_BOTH              ((1<<5) | (1<<4))

#define MODE0                0
#define MODE1                (1<<1)
#define MODE2                (1<<2)
#define MODE3                ((1<<2) | (1<<1))
#define MODE4                (1<<3)
#define MODE5                ((1<<3) | (1<<1))

#define BINARY               0
#define BCD                  1

#define write_ctrl_reg(data)    (*((volatile u_char *) CONTROL_REG) = (u_char) (data))
#define write_timer(TIM, data)  (*((volatile u_char *) (TIM)) = (u_char) (data))
#define read_timer(TIM)         (*(volatile u_char *) (TIM))
