
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

/******************************************************************************
 *
 *	Some hardware definitions for the Heurikon HK80/V960E.
 *
 ******************************************************************************/


/*******************  Z85C30 Serial ports A-D  *******************/
 
struct SCCport {
	volatile unsigned char control;
	unsigned char dummy[15];
	volatile unsigned char data;
};

#define SCC_PORTA	((volatile struct SCCport *) 0x02200008)
#define SCC_PORTB	((volatile struct SCCport *) 0x02200000)
#define SCC_PORTC	((volatile struct SCCport *) 0x02300008)
#define SCC_PORTD	((volatile struct SCCport *) 0x02300000)

#define CONSOLE		SCC_PORTA


/************ Z85C36 CIO Counter Timer and parallel ports  ************/

struct cio {
	volatile unsigned char cdata; unsigned char dummy[7];
	volatile unsigned char bdata; unsigned char dummy2[7];
	volatile unsigned char adata; unsigned char dummy3[7];
	volatile unsigned char ctrl;
};

#define CIO ((volatile struct cio *) 0x02e00000)



/******************  WD33C93 SCSI interface  ******************/

struct SCSIChip {
	unsigned char SC_AddrPtr;
	unsigned char SC_Dummy[7];
	unsigned char SC_Register;
};

#define SCSI ((struct SCSIChip *) 0x02400000)


#define SCWriteReg(Reg,Val)	SCSI->SC_AddrPtr = Reg; SCSI->SC_Register = Val
#define SCReadReg(Reg,Val)	SCSI->SC_AddrPtr = Reg; Val = SCSI->SC_Register

#define SREG_STAT	0x17
#define SREG_CMD	0x18



/******************  USER LED DEFINITIONS ******************************/

#define LED1	((unsigned char *) 0x02000020)
#define LED2	((unsigned char *) 0x02000028)
#define LED3	((unsigned char *) 0x02000030)
#define LED4	((unsigned char *) 0x02000038)

#define LED_ON	0
#define LED_OFF	1


/******************  VIC Interface definitions  ************************/

	/* Registers are 8 bits each, with 3 dummy bytes
	 * between each one.
	 */

#define VIC_REG(n)	((unsigned char *) (0x02a00000 + (4*(n))) )

	/* Interprocessor communication registers #6 and #7
	 * needed to deassert SYSFAIL
	 */

#define VIC_ICR6	(VIC_REG(30))
#define VIC_ICR7	(VIC_REG(31))

	/* Slave select registers needed to allow external VxWorks system
	 * on same VME bus to load VxWorks (via Ethernet) into our memory.
	 */
#define VIC_SLV_SEL_1_0	(VIC_REG(50))
#define VIC_SLV_SEL_1_1	(VIC_REG(51))

	/* System control latch for enabling VME bus Slave extended space
	 * (see HK80/V960E manual, section 6.8)
	 */
#define VME_SLV_EXTENDED	((unsigned char *)0x02000100)
