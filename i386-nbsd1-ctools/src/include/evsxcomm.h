

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
 * Magic addresses, etc., for NINDY when running on Intel EV960SX board.
 * Used by NINDY monitor and EV960SX library (80960) code.
 *********************************************************************/

/* CONSTANTS USED FOR 82C54 TIMER PROGRAMMING */
#define TIMER_BASE      0x24000000
#define COUNTER_0       TIMER_BASE+0x0
#define COUNTER_1       TIMER_BASE+0x2
#define COUNTER_2       TIMER_BASE+0x4
#define CONTROL_REG     TIMER_BASE+0x6

#define TIMER_VECTOR    252
#define SERIAL_VECTOR    242

/* CRYSTAL DIVISOR */
#define CRYSTALTIME     8   /* for the 8 MHz crystal */

/* step down for timer 0 to generate 1us pulse.  */
#define STEP_DOWN0      CRYSTALTIME*1
#define STEP_DOWN1      5000     /* reduction for timer 1 */

/* CONSTANTS USED FOR 8259A INTERRUPT CONTROLLER PROGRAMMING */
#define I8259_ADDR  0x28000000
#define ICW1 (I8259_ADDR)
#define ICW2 (I8259_ADDR +2)
#define ICW4 (I8259_ADDR +2)
#define OCW1 (I8259_ADDR +2)
#define OCW2 (I8259_ADDR)
#define OCW3 (I8259_ADDR)


/* ICW1 */
#define BIT4  0x10	/* must be 1 */
#define LTIM  0x08	/* level triggered interrupt mode */
#define ADI4  0x04	/* 80/85 call address interval =4 (0 means 8) */
#define SNGL  0x02	/* Single 8259a, (0 means 2 8259a's cascaded */
#define NEED4 0x01	/* 8259a needs ICW4 value to initialize mode */

/* ICW4 */
#define SFNM  0x80	/* special fully nested mode */
#define NBUF  0x00	/* non-buffered mode */
#define BUFM  0x08	/* buffered mode (0 is non-buffered, master ignored) */
#define MSTR  0x04	/* master mode (bit only meaningful in buffered mode */
#define AEOI  0x02	/* automatic EOI */
#define uP86  0x01	/* 8086/8088 uprocessor mode (0 is MCS-80/85 */

/* 8259 operation command word bits */
/* OCW1, Port 1 becomes interrupt mask register, both read and write */
#define LVL2MSK(level) (1<<level)     /* 1 means disabled, 0 enabled */

/* OCW2, OCW3, Port 0 controls EOI commands, and read requests */
#define SPEC_EOI 0x60   /* ORed with int level for EOI in 3 LSB's */
#define NSPC_EOI 0x20	/* non-specific EOI, level bits not used */
#define READ_ISR 0x0A	/* want to read IS register */ 
#define READ_IRR 0x0B	/* want to read IR register */

#define EVSX_VEC_BASE_8259  0x50 /* for non-dedicated interrupts */
#define EVSX_VEC_PARALLEL   ((unsigned char) EVSX_VEC_BASE_8259+0)
#define EVSX_VEC_XINTA      ((unsigned char) EVSX_VEC_BASE_8259+1)
#define EVSX_VEC_SERIAL_P0  ((unsigned char) EVSX_VEC_BASE_8259+2)
#define EVSX_VEC_CIO        ((unsigned char) EVSX_VEC_BASE_8259+3)
#define EVSX_VEC_TIMER_16   ((unsigned char) EVSX_VEC_BASE_8259+4)
#define EVSX_VEC_XINTB      ((unsigned char) EVSX_VEC_BASE_8259+5)
#define EVSX_VEC_ETHERNET   ((unsigned char) EVSX_VEC_BASE_8259+6)
#define EVSX_VEC_TIMER_32A  ((unsigned char) EVSX_VEC_BASE_8259+7)


/* CONSTANTS USED FOR FLASH PROGRAMMING */
#define	FLASH_ADDR	0x10000000
#define	MAXFLASH	0x40000

/* CONSTANTS USED FOR NATIONAL ns16c552 DUART PROGRAMMING */
#define UART_BASE	0x25000000
#define UART1(reg)      (UART_BASE+0x10+(reg*2))
#define UART2(reg)      (UART_BASE+(reg*2))

/* set baud rate for the 18.432 MHZ Clock */
#define BAUD_38400      30
#define BAUD_19200      60
#define BAUD_9600       120
#define BAUD_4800       240
#define BAUD_2400       480
#define BAUD_1200       920
#define XTAL            18432000

/* CONSTANTS USED FOR LED PROGRAMMING */
#define TEST_POINT 0x21000000

#define ZEROLED 0xc0
#define ONELED 0xcf
#define TWOLED 0xa4
#define THREELED 0xb0
#define FOURLED 0x99
#define FIVELED 0x92
#define SIXLED 0x82
#define SEVENLED 0xd8
#define EIGHTLED 0x80
#define NINELED 0x90
#define ALED 0x88
#define BLED 0x83
#define CLED 0xc6
#define ELED 0x86
#define FLED 0x8e
#define DOTLED 0x7f
#define ALL_ON 0x00
#define ALL_OFF 0xff
#define WHO_KNOWS       ALL_ON

#define SEVEN_SEG       (*(volatile unsigned char *)0x22000000)
#define BAR_LEDS        (*(volatile unsigned char *)0x22000020)
#define light_led(led)  (SEVEN_SEG = (led))    /* 7 Segment Display */
#define B_LEDS(led)     (BAR_LEDS = (led))     /* 10 Bar LED */


/* CONSTANTS USED FOR 82596SX LAN PROGRAMMING */
#define PORT_596        0x23000020   /* 596 port */
#define CA_596          0x21000020   /* channel attention */
#define INTS            0            /* interrupts active high */

struct SCP_struct {
   unsigned int         zeros:17;
   unsigned int         mode:2;         /* mode */
   unsigned int         trigger:1;      /* int/ext trigger */
   unsigned int         lock:1;         /* lock enable/disable */
   unsigned int         int_polarity:1; /* interrupt polarity */
   unsigned int         reserved:10;    /* set to 0x1 */
   unsigned int         zeros2;
   unsigned int         ISCP_Addr;
};

struct ISCP_struct {
	unsigned int	Busy:8;
	unsigned int	zeros:24;
   	unsigned int 	SCB_Addr;
};

struct SCB_struct { 
	unsigned int	reserved1:3;	/* zeros 		*/
	unsigned int	t:1;		/* bus throttle timers	*/
	unsigned int 	rus:4;		/* receive unit status  */
	unsigned int	cus:3;		/* command unit status	*/
	unsigned int	zero:1;		/* zero			*/
	unsigned int	status:4;	/* status 		*/
	unsigned int	reserved2:4;  	/* zeros 		*/
	unsigned int	ruc:3;		/* receive unit command */
	unsigned int	r:1;		/* reset		*/
	unsigned int	cuc:3;		/* command unit command */
	unsigned int	zero2:1;	/* zero			*/
	unsigned int	ack:4;		/* acknowledges action	*/
	/*   */
	unsigned int	CBL_Addr;    	/* CBL Address 		*/ 
	unsigned int	RFA_Addr;   	/* RFA Address 		*/
	unsigned int	CRCErrs; 	/* CRC Errors 		*/ 
	unsigned int	AlinErrs; 	/* Alignment Errors 	*/ 
	unsigned int	RescErrs;  	/* Resource Errors 	*/ 
	unsigned int	OvernErrs;  	/* Overrun Errors 	*/ 
	unsigned int	RcvcdtErrs; 	/* RCVCDT Errors 	*/ 
	unsigned int	ShortFrameErrs; /* Short Frame Errors 	*/ 
	unsigned short	TOffTimer; 	/* T-ON TIMER		*/
	unsigned short	TOnTimer; 	/* T-OFF TIMER 		*/ 
};

struct Conf_struct {
	unsigned int	zeros:12; 	/* zeros */
	unsigned int	a:1;		/* command aborted */
	unsigned int	ok:1;		/* command completed-no errors */
	unsigned int	b:1;		/* executing NOP */
	unsigned int	c:1;		/* cmd completion status */
	unsigned int	cmd:13;		/* configure command 0x2 */
	unsigned int	i:1;		/* interrupt on completion */
	unsigned int	s:1;		/* suspend CU on completion */
	unsigned int	el:1;		/* last block on the CBL */
	/*  */
	unsigned int	lnk_addr;	/* addr of next command block */
	/*  */
	unsigned int	byte:7;		/* byte count */
	unsigned int	prefetch:1;	/* prefetch bit */
	unsigned int	fifo:6;		/* fifo limit */
	unsigned int	mon_cfg:2;	/* monitor mode cfg bits */
	unsigned int	reserved:7;	/* set to 0x40 */
	unsigned int	bf:1;		/* save bad frames */
	unsigned int	addr_len:3;	/* address length */
	unsigned int	src_addr_ins:1;	/* no source addr insertion */
	unsigned int	preamble:2;	/* preamble length */
	unsigned int	loopback:2;	/* loopback mode */
	/*  */
	unsigned int	lin_prior:4;	/* linear priority */
	unsigned int	exp_proir:3;	/* exponential priority */
	unsigned int	backoff:1;	/* backoff method */
	unsigned int	spacing:8;	/* interframe spacing */
	unsigned int	low_slot:8;	/* slot time low byte */
	unsigned int	hi_slot:4;	/* upper 3 bits slot time */
	unsigned int	retry:4;	/* maximum retry number */
	/*  */
	unsigned int	prm:1;		/* promiscuous mode */
	unsigned int	bc_dis:1;	/* broadcast disable */
	unsigned int	man_nrz:1;	/* manchester/nrz */
	unsigned int	tx_no_crs:1;	/* xmit on no Carrier Sense */
	unsigned int	crc_ins:1;	/* no CRC insertion */
	unsigned int	crc16_32:1;	/* CRC 16/CRC 32 */
	unsigned int	bit_stuff:1;	/* bit stuffing */
	unsigned int	padding:1;	/* padding method */
	unsigned int	cs_filter:3;	/* Carrier Sense filter */
	unsigned int	cs_src:1;	/* Carrier Sense source */
	unsigned int	cdt_filter:3;	/* collision detect filter */
	unsigned int 	cdt_src:1;	/* collision detect source */
	unsigned int	min_frame:8;	/* minimum frame length */
	unsigned int	precrs:1;	/* preamble til Carrier Sense */
	unsigned int	lngfld:1;	/* length field */
	unsigned int	crcinm:1;	/* CRC in memory */
	unsigned int	autotx:1;	/* auto retransmit */
	unsigned int	cdbsac:1;	/* collision detect by
					   src addr comparison */
	unsigned int	mc_all:1;	/* multicast all */
	unsigned int	monitor:2;	/* monitor */
	/*  */
	unsigned int	dcr_addr:6;	/* DCR slot number */
	unsigned int	fdx:1;		/* full duplex operation */
	unsigned int	dcr:1;		/* DCR */
	unsigned int	dcr_sta:6;	/* DCR number of stations */
	unsigned int	mult_ia:1;	/* multiple individual addr */
	unsigned int	dis_bof:1;	/* disable backoff */
};

struct IndAddr_struct {
	unsigned int	zeros:12; 	/* zeros */
	unsigned int	a:1;		/* command aborted */
	unsigned int	ok:1;		/* command completed-no errors */
	unsigned int	b:1;		/* executing NOP */
	unsigned int	c:1;		/* cmd completion status */
	unsigned int	cmd:13;		/* set ind addr command 0x1 */
	unsigned int	i:1;		/* interrupt on completion */
	unsigned int	s:1;		/* suspend CU on completion */
	unsigned int	el:1;		/* last block on the CBL */
	/*  */
	unsigned int	lnk_addr;	/* addr of next command block */
	/*  */
	unsigned int	byte_1:8;	/* Individual Addr byte 1 */
	unsigned int	byte_2:8;	/* Individual Addr byte 2 */
	unsigned int	byte_3:8;	/* Individual Addr byte 3 */
	unsigned int	byte_4:8;	/* Individual Addr byte 4 */
	/*  */
	unsigned int	byte_5:8;	/* Individual Addr byte 5 */
	unsigned int	byte_6:8;	/* Individual Addr byte 6 */
};

/* LAN Globals */
struct  SCP_struct      scp;
struct  ISCP_struct     iscp;
struct  SCB_struct      scb;
struct  Conf_struct     config;
struct  IndAddr_struct  ia_struct;
