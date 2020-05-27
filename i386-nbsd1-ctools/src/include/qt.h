
/*(cb*/

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

/*)ce*/

/* Definitions for Intel QT960 board. */ 


/* Definitions used by i510.c */
/* Base address and spacing of 82510 registers */
#define I510BASE	0x20000000
#define I510DELTA	4
/* Frequency of 82510 baud rate generation crystal */
#define XTAL		9830400
#define ACCESS_DELAY	2


/* Definitions used by flash.c */
#define	FLASH_ADDR 0x10000000	/* base address of flash */
#define MAXFLASH   0x100000     /* Used by NINDY */
#define	FLASH_WIDTH 4		/* width of flash in bytes */

/* The timing delays in flash.c can be calibrated using the processor clock
 * or using an 8254 timer.  If PROC_FREQ is defined, it is used; otherwise
 * timer 0 of the 8254 is used.  (Using the timer allows a board to be
 * upgraded to a faster processor without changing the monitor.)  If
 * PROC_FREQ is defined, the constants TIMER_CNTL, TIMER_0, and TIMER_XTAL
 * do not need to be defined.  If the timer is used, it is required only
 * during board initialization.  After that it is available to the
 * application. */

#if 0
#define PROC_FREQ   20		/* processor clock frequency in MHz */
#endif

/* The flash programming code uses timer 0 of the 8254-compatible timers in
 * the 82380.  The addresses of these registers is defined below. */
#define TIMER_CNTL CWR1
#define TIMER_0    CR0
#define	TIMER_XTAL 9830400

/* This value is used by the benchmark timer. */
#define CRYSTALTIME (1000000.0 / TIMER_XTAL)

#define CSR0    ((volatile unsigned char *)0x28000000)
#define CSR1    ((volatile unsigned char *)0x28000004)
#define CSR2    ((volatile unsigned char *)0x28000008)
#define CSR3    ((volatile unsigned char *)0x2800000c)
#define CSR4    ((volatile unsigned char *)0x28000010)
#define CSR5    ((volatile unsigned char *)0x28000014)
#define CSR6    ((volatile unsigned char *)0x28000018)

#define LED0    0x01
#define LED1    0x02
#define LED2    0x01
#define LED3    0x02

#define BASE_ADDR0	((volatile unsigned char *)0x18000000)
#define BASE_ADDR1	((volatile unsigned char *)0x18010000)

/* DMA */
/* channel 0 */
#define TA0_0	(BASE_ADDR0 + 0x00)  /* Target Address bits 0-7,8-15 */
#define TA0_1	(BASE_ADDR1 + 0x85)  /*  		        bits 15-23 */
#define TA0_2	(BASE_ADDR0 + 0x10)  /* 		        bits 24-31 */
#define BC0_0	(BASE_ADDR0 + 0x01)  /* Byte Count bits 0-7,8-15 */
#define BC0_1	(BASE_ADDR0 + 0x11)  /* 		    bits 16-23 */
#define RA0_0	(BASE_ADDR0 + 0x90)  /* Requester Address bits 0-7,8-15 */
#define RA0_1	(BASE_ADDR0 + 0x91)  /*                bits 16-23,24-31 */

/* channel 1 */
#define TA1_0	(BASE_ADDR1 + 0x00)  
#define TA1_1	(BASE_ADDR1 + 0x81)
#define TA1_2	(BASE_ADDR1 + 0x10)
#define BC1_0	(BASE_ADDR1 + 0x01)
#define BC1_1	(BASE_ADDR1 + 0x11)
#define RA1_0	(BASE_ADDR1 + 0x90)
#define RA1_1	(BASE_ADDR1 + 0x91)

/* channel 2 */
#define TA2_0	(BASE_ADDR0 + 0x04)
#define TA2_1	(BASE_ADDR0 + 0x81)
#define TA2_2	(BASE_ADDR0 + 0x14)
#define BC2_0	(BASE_ADDR0 + 0x05)
#define BC2_1	(BASE_ADDR0 + 0x15)
#define RA2_0	(BASE_ADDR0 + 0x94)
#define RA2_1	(BASE_ADDR0 + 0x95)

/* channel 3 */
#define TA3_0	(BASE_ADDR1 + 0x04)
#define TA3_1	(BASE_ADDR1 + 0x80)
#define TA3_2	(BASE_ADDR1 + 0x14)
#define BC3_0	(BASE_ADDR1 + 0x05)
#define BC3_1	(BASE_ADDR1 + 0x15)
#define RA3_0	(BASE_ADDR1 + 0x94)
#define RA3_1	(BASE_ADDR1 + 0x95)

/* channel 5 */
#define TA5_0	(BASE_ADDR1 + 0xc0)
#define TA5_1	(BASE_ADDR1 + 0x89)
#define TA5_2	(BASE_ADDR1 + 0xd0)
#define BC5_0	(BASE_ADDR1 + 0xc1)
#define BC5_1	(BASE_ADDR1 + 0xd1)
#define RA5_0	(BASE_ADDR1 + 0x98)
#define RA5_1	(BASE_ADDR1 + 0x99)

/* channel 6 */
#define TA6_0	(BASE_ADDR0 + 0xc4)
#define TA6_1	(BASE_ADDR0 + 0x89)
#define TA6_2	(BASE_ADDR0 + 0xd4)
#define BC6_0	(BASE_ADDR0 + 0xc5)
#define BC6_1	(BASE_ADDR0 + 0xd5)
#define RA6_0	(BASE_ADDR0 + 0x9c)
#define RA6_1	(BASE_ADDR0 + 0x9d)

/* channel 7 */
#define TA7_0	(BASE_ADDR1 + 0xc4)
#define TA7_1	(BASE_ADDR1 + 0x88)
#define TA7_2	(BASE_ADDR1 + 0xd4)
#define BC7_0	(BASE_ADDR1 + 0xc5)
#define BC7_1	(BASE_ADDR1 + 0xd5)
#define RA7_0	(BASE_ADDR1 + 0x9c)
#define RA7_1	(BASE_ADDR1 + 0x9d)

/* init registers */
#define CR1_0	(BASE_ADDR0 + 0x08)  /* Command Regiser I  ch 0-3 */
#define CR1_1	(BASE_ADDR0 + 0xc8)  /*                    ch 4-7 */
#define CR2_0	(BASE_ADDR1 + 0x18)  /* Command Register II ch 0-3 */
#define CR2_1	(BASE_ADDR1 + 0xd8)  /*                     ch 4-7 */
#define MR1_0	(BASE_ADDR1 + 0x09)  /* Mode Register I  ch 0-3 */
#define MR1_1	(BASE_ADDR1 + 0xc9)  /*   	       ch 4-7 */
#define MR2_0	(BASE_ADDR1 + 0x19)  /* Mode Register II  ch 0-3 */
#define MR2_1	(BASE_ADDR1 + 0xd9)  /*	 	        ch 4-7 */
#define SRR_0	(BASE_ADDR0 + 0x09)  /* Software Request Register 0-3 */
#define SRR_1	(BASE_ADDR0 + 0xc9)  /*				4-7 */
#define MSRR_0	(BASE_ADDR1 + 0x08)  /* Mask Set/Reset Register 0-3 */
#define MSRR_1	(BASE_ADDR1 + 0xc8)  /*  			      4-7 */
#define MRWR_0	(BASE_ADDR1 + 0x0d)  /* Mask Read/Write Register 0-3 */
#define MRWR_1	(BASE_ADDR1 + 0xcd)  /*			       4-7 */
#define SR_0	(BASE_ADDR0 + 0x08)  /* Status Register ch 0-3 */
#define SR_1	(BASE_ADDR0 + 0xc8)  /*                 ch 4-7 */
#define BSR_0	(BASE_ADDR0 + 0x18)  /* Bus Size Register 0-3 */
#define BSR_1	(BASE_ADDR0 + 0xd8)  /*                   4-7 */
#define CHR_0	(BASE_ADDR0 + 0x19)  /* Chaining Register 0-3 */
#define CHR_1	(BASE_ADDR0 + 0xd9)  /*                   4-7 */
#define CBPFF	(BASE_ADDR0 + 0x0c)  /* Clear Byte Pointer Flip Flop */
#define CTCIR	(BASE_ADDR1 + 0x1c)  /* Clear TC Interrupt Register */

#define MCLR 	(BASE_ADDR0 + 0x0d)  /* Clear Master DMA Register */

/* INTERRUPT CONTROLLER */

/* bank A */
#define ICW1_A	(BASE_ADDR0 + 0x30)  /* Initialization Command Word 1 */
#define ICW2_A	(BASE_ADDR0 + 0x31)  /* Initialization Command Word 2 */
#define ICW3_A  ICW2_A             /* "                         " 3 */
#define ICW4_A	ICW2_A             /* "                         " 4 */
#define ICW2R_A	(BASE_ADDR1 + 0x30)  /* Read addr of ICW2_A */
#define ICWR_A  ICW2R_A		   /* 2nd definition for above     */

#define OCW1_A	(BASE_ADDR0 + 0x31)  /* Operation Control Word 1 */
#define OCW2_A	(BASE_ADDR0 + 0x30)  /* Operation Control Word 2 */
#define OCW3_A  OCW2_A

#define READ_A  (BASE_ADDR0 + 0x30)
#define MASK_A  (BASE_ADDR0 + 0x31)  /* READ ADDRESS FOR INTERRUPT MASK  */

#define VR0	(BASE_ADDR0 + 0x38)  /* Vector Register 0 */
#define VR1	(BASE_ADDR0 + 0x39)  /* Vector Register 1 */
#define VR1_5	(BASE_ADDR1 + 0x38)  /* Vector Register 1.5 */
#define VR3	(BASE_ADDR1 + 0x39)  /* Vector Register 3 */
#define VR4	(BASE_ADDR0 + 0x3c)  /* Vector Register 4 */
#define VR7	(BASE_ADDR1 + 0x3d)  /* Vector Register 7 */

/* bank B */
#define ICW1_B	(BASE_ADDR0 + 0x20)
#define ICW2_B	(BASE_ADDR0 + 0x21)
#define ICW3_B  ICW2_B
#define ICW4_B	ICW2_B

#define ICW2R_B	(BASE_ADDR1 + 0x20)
#define ICWR_B  ICW2R_B		     /* 2nd def for ICW2R_B */

#define READ_B  (BASE_ADDR0 + 0x20)
#define MASK_B  (BASE_ADDR0 + 0x21)  /* READ ADDRESS FOR INTERRUPT MASK  */

#define OCW1_B	(BASE_ADDR0 + 0x21)
#define OCW2_B	(BASE_ADDR0 + 0x20)
#define OCW3_B  OCW2_B

#define VR8	(BASE_ADDR0 + 0x28)
#define VR9	(BASE_ADDR0 + 0x29)
#define VR11	(BASE_ADDR1 + 0x29)
#define VR12	(BASE_ADDR0 + 0x2c)
#define VR13	(BASE_ADDR0 + 0x2d)
#define VR14	(BASE_ADDR1 + 0x2c)
#define VR15	(BASE_ADDR1 + 0x2d)

/* bank C */
#define ICW1_C	(BASE_ADDR0 + 0xa0)
#define ICW2_C	(BASE_ADDR0 + 0xa1)
#define ICW3_C  ICW2_C
#define ICW4_C	ICW2_C

#define ICW2R_C	(BASE_ADDR1 + 0xa0)
#define ICWR_C  ICW2R_C		    /* 2nd def for ICW2R_C  */

#define OCW1_C	(BASE_ADDR0 + 0xa1)
#define OCW2_C	(BASE_ADDR0 + 0xa0)
#define OCW3_C  OCW2_C

#define READ_C  (BASE_ADDR0 + 0xa0)
#define MASK_C  (BASE_ADDR0 + 0xa1)  /* READ ADDRESS FOR INTERRUPT MASK  */

#define VR16	(BASE_ADDR0 + 0xa8)
#define VR17	(BASE_ADDR0 + 0xa9)
#define VR18	(BASE_ADDR1 + 0xa8)
#define VR19	(BASE_ADDR1 + 0xa9)
#define VR20	(BASE_ADDR0 + 0xac)
#define VR21	(BASE_ADDR0 + 0xad)
#define VR22	(BASE_ADDR1 + 0xac)
#define VR23	(BASE_ADDR1 + 0xad)

/* TIMER */
#define CWR1	(BASE_ADDR1 + 0x41)  /* Control Word Register I */
#define CWR2	(BASE_ADDR1 + 0x45)  /* Control Word Register II */
#define CR0	(BASE_ADDR0 + 0x40)  /* Counter Register 0 */
#define CR1	(BASE_ADDR0 + 0x41)  /* Counter Register 1 */
#define CR2	(BASE_ADDR1 + 0x40)  /* Counter Register 2 */
#define CR3	(BASE_ADDR0 + 0x44)  /* Counter Register 3 */

/* PROGRAMMABLE WAIT STATE GENERATOR */
#define WSR0	(BASE_ADDR1 + 0x70)
#define WSR1	(BASE_ADDR1 + 0x71)
#define WSR2	(BASE_ADDR0 + 0x74)
#define RWSR	(BASE_ADDR0 + 0x75)

/* PROGRAMMABLE CONTROL REGISTERS */
#define CNTL1	(BASE_ADDR0 + 0x61)

/* RESET PORT ON 380 TO RESET 960 */
#define		RESET_ADDR	0x18000064
#define		RESET_DATA	0xf0
