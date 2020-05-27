/*(cb*/
/**************************************************************************
 *
 *     Copyright (c) 1992, 1995 Intel Corporation.  All rights reserved.
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


#ifndef C145_H
#define C145_H

/*************************************************************************** 
*    Written:    23 May 1994, Scott Coulter - changed for c145/c146 CX
*       Revised:
*
***************************************************************************/

#ifndef STD_DEFS_H
#   include "std_defs.h"
#endif

/******************************************************************************
 *
 *    Common hardware definitions for the Cyclone C145 base board.
 *
 ******************************************************************************/


/*******************  16C552 UART Definitions  ******************************/
 
#define DUART        0xb0000000    /* base address */
#define DUART_DELTA  4        /* register address boundaries */
#define DFLTPORT     0          /* in 16552 chan 2 = offset 0 works for 16550 */
#define XTAL         1843200
#define ACCESS_DELAY 0


/*******************  Memory Map Definitions   *******************/
#define MEMBASE_DRAM        0xa0000000


/*******************  Flash ROM Definitions **********************/
#define NUM_FLASH_BANKS 2        /* number of flash banks */
#define FLASH_WIDTH        1        /* width of flash in bytes */
#define FLASH_ADDR        0xe0000000    /* base address of flash socket 0 */
#define FLASH_ADDR_INCR    0x00040000    /* address offset of each flash bank */
#define FLASH_TIME_ADJUST 1        /* delay adjusmet factor for delay times */

/* The timing delays in flash.c can be calibrated using the processor clock
 * or using a Z8536 timer.  If PROC_FREQ is defined, it is used; otherwise
 * timer 3 of the Z8536 is used.  (Using the timer allows a board to be
 * upgraded to a faster processor without changing the monitor.)
 * If the timer is used, it is required only during board initialization.
 * After that it is available to the application. */
#undef  PROC_FREQ            /* use the timer not CPU delays */


/************ Z85C36 CIO Counter Timer and parallel ports  ************/
typedef struct cio
{
    volatile unsigned char cdata;
    volatile unsigned char pad1[3];    /* longword boundary padding */
    volatile unsigned char bdata;
    volatile unsigned char pad2[3];    /* longword boundary padding */
    volatile unsigned char adata;
    volatile unsigned char pad3[3];    /* longword boundary padding */
    volatile unsigned char ctrl;
}
CIO_DEV;

#define CIO          ((volatile CIO_DEV *) 0xb0040000)
#define CIO_CLK      4000000
#define ROLL_32_BITS 0xffffffff

/* Define user leds for debugging */
#define LED_1_ADDR  0xb0040000 
#define LEDS_MASK   0x0f
#define LEDS_SIZE   4
#define BLINK_PAUSE (0x4500 * __cpu_speed)
#define LEDS_ON_IS_0


/* defines for the uses of the CIO's i/o ports */
/* Port A: Processor Module options
    Bit 0 -  Processor Type Bit 0
    Bit 1 -  Processor Type Bit 1
    Bit 2 -  Processor Type Bit 2
    Bit 3 -  Clock Frequency Bit 0
    Bit 4 -  Clock Frequency Bit 1 
    Bit 5 -  Clock Frequency Bit 2
    Bit 6 -  DRAM Speed Selection (60 vs 70 ns)
    Bit 7 -  N/C
*/

#define GET_CPU_TYPE()      (((CIO->adata) & 0x07) >> 0)
#define GET_CLK_FREQ()      (((CIO->adata) & 0x38) >> 3)
#define GET_DRAM_SPEED()    (((CIO->adata) & 0x40) >> 6)

/* Processor module types */
#define CPU_TYPE_SA_SB     0
#define CPU_TYPE_KA_KB     1
#define CPU_TYPE_CA_CF     2
#define CPU_TYPE_HX        3
#define CPU_TYPE_JX        4

/* Processor speeds */
#define FREQ_4_MHZ         0
#define FREQ_8_MHZ         1
#define FREQ_16_MHZ        2
#define FREQ_20_MHZ        3
#define FREQ_25_MHZ        4
#define FREQ_33_MHZ        5    /* actually 33.33 MHz */
#define FREQ_40_MHZ        6
#define FREQ_50_MHZ        7

/* Port B: Serial data and clocks for on-board and squall module EEPROM
    Bit 0 -  On board Serial EEPROM clock
    Bit 1 -  On board Serial EEPROM data
    Bit 2 -  Squall Module Serial EEPROM clock
    Bit 3 -  Squall Module Serial EEPROM data
    Bit 4 -  Reserved
    Bit 5 -  Reserved
    Bit 6 -  Reserved
    Bit 7 -  Reserved
*/

#define SQUALL_DATA_REG        (CIO->bdata)
#define SQUALL_DATA_DIR_REG    0x2b
#define SQUALL_DATA_OFFSET     3
#define SQUALL_CLOCK_REG       (CIO->bdata)
#define SQUALL_CLOCK_OFFSET    2

#define ON_BOARD_DATA_REG      (CIO->bdata)
#define ON_BOARD_DATA_DIR_REG  0x2b
#define ON_BOARD_DATA_OFFSET   1
#define ON_BOARD_CLOCK_REG     (CIO->bdata)
#define ON_BOARD_CLOCK_OFFSET  0

#define NO_SQUALL        0x00100002

/************  Parallel Port Register Definitions  *********************/
#define SECOND_CHECK_PP_READ_STATUS     /* Need to read status a secon time
                                           or the check fails occasionally */
#define PP_DATA_ADDR        0xb0080004
#define PP_STAT_ADDR        0xb0080000
#define PP_CTRL_ADDR        0xb0080000
#define PP_READ_STATUS      0x32        /* BUSY, ACK, and PSTROBE */
#define PP_ERR_BIT          0x01        /* ERROR bit to end parallel transfer */
                                        /* negiative logic 1 = no error */
#define PP_SEL_BIT          0x02        /* select for initial connect and transfers*/
#define PP_POUT_BIT         0x04        /* paper out bit for inital connect*/
#define PP_INIT_BITS        ((PP_ERR_BIT | PP_SEL_BIT) & ~PP_POUT_BIT)


/************  Ethernet Squall II Module definitions  ******************/
#define SQUALL_BASE_ADDR    0xc0000000
#define SQ_01_PORT_OFFSET   0x00
#define SQ_01_CA_OFFSET     0x10

/*
 *  Interuupt definitions
 */

#if KXSX_CPU
/* XINT pins going into CPU */
#define XINT_CIO        0    /* 85C36 CIO   ............ XINT0 */
#define XINT_UART       1    /* 16C550 UART ............ XINT1 */
#define XINT_SQ_IRQ1    1    /* Squall II, irq pin 1 ... XINT1 */
#define XINT_PARALLEL   2    /* Parallel Port .......... XINT2 */
#define XINT_PCI        2    /* PLX 9060 PCI Interface . XINT2 */
#define XINT_SQ_IRQ0    3    /* Squall II, irq pin 0 ... XINT3 */

/* Vectors for IMAP registers */
#define VECTOR_UART       0xf2    /* leave at 0xf2 for compat w mon960 */
#define VECTOR_CIO        0xe2
#define VECTOR_SQ_IRQ1    0xf2
#define VECTOR_SQ_IRQ0    0xc2
#define VECTOR_PARALLEL   0xd2
#define VECTOR_PCI        0xd2
#endif /* KXSX */

#if CXHXJX_CPU
/* XINT pins going into CPU */
#define XINT_UART       7    /* 16C550 UART ............ XINT7 */
#define XINT_CIO        6    /* 85C36 CIO   ............ XINT6 */
#define XINT_SQ_IRQ1    5    /* Squall II, irq pin 1 ... XINT5 */
#define XINT_SQ_IRQ0    4    /* Squall II, irq pin 0 ... XINT4 */
#define XINT_DEADLOCK   3    /* PCI/Local deadlock ..... XINT3 */
#define XINT_LSERR      2    /* PCI LSERR .............. XINT2 */
#define XINT_PARALLEL   1    /* Parallel Port .......... XINT1 */
#define XINT_PCI        0    /* PLX 9060 PCI Interface . XINT0 */

/* Vectors for IMAP registers */
#define VECTOR_UART       0xf2    /* leave at 0xf2 for compat w mon960 */
#define VECTOR_CIO        0xe2
#define VECTOR_SQ_IRQ1    0x62
#define VECTOR_SQ_IRQ0    0x52
#define VECTOR_DEADLOCK   0xb2
#define VECTOR_PARALLEL   0xc2
#define VECTOR_PCI        0xd2
#define VECTOR_LSERR      0xa2
#endif /* CXJX */

/******************************************************************************
 *	Timer hardware definitions for the Cyclone C145 CX
 ******************************************************************************/
#define TIMER_BASE       CIO
#define CRYSTALTIME      2
#define TIMER0_VECTOR    VECTOR_CIO
#define TIMER0_IRQ       XINT_CIO
#define TIMER0_OFFSET    1
#define TIMER1_VECTOR    VECTOR_CIO
#define TIMER1_IRQ       XINT_CIO
#define TIMER1_OFFSET    2

/* Leds */
#define LED_0              0x01
#define LED_1              0x02
#define LED_2              0x04
#define LED_3              0x08


/* -------------------- Cyclone Specific Prototypes -------------------- */

extern unsigned char   get_led_value(void);
extern void            set_led_value(unsigned char);


#endif /* ! defined C145_H */

