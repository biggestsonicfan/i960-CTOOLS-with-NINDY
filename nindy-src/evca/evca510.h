/******************************************************************/
/* 		Copyright (c) 1989, Intel Corporation

   Intel hereby grants you permission to copy, modify, and 
   distribute this software and its documentation.  Intel grants
   this permission provided that the above copyright notice 
   appears in all copies and that both the copyright notice and
   this permission notice appear in supporting documentation.  In
   addition, Intel grants this permission provided that you
   prominently mark as not part of the original any modifications
   made to this software or documentation, and that the name of 
   Intel Corporation not be used in advertising or publicity 
   pertaining to distribution of the software or the documentation 
   without specific, written prior permission.  

   Intel Corporation does not warrant, guarantee or make any 
   representations regarding the use of, or the results of the use
   of, the software and documentation in terms of correctness, 
   accuracy, reliability, currentness, or otherwise; and you rely
   on the software, documentation and results solely at your own 
   risk.							  */
/******************************************************************/

#define NAS0		0
#define WORK1		0x20
#define GEN2		0x40
#define MODM3		0x60


#define BASE_510	0xDFC00000

#define TXD		BASE_510+0
#define RXD		BASE_510+0
#define BAL		BASE_510+0
#define BAH		BASE_510+1
#define GER		BASE_510+1
#define GIR		BASE_510+2
#define BANK		BASE_510+2
#define LCR		BASE_510+3
#define MCR		BASE_510+4
#define LSR		BASE_510+5
#define MSR		BASE_510+6
#define ACR0		BASE_510+7

#define RXF		BASE_510+1
#define TXF		BASE_510+1
#define TMST		BASE_510+3
#define TMCR		BASE_510+3
#define FLR		BASE_510+4
#define RST		BASE_510+5
#define RCM		BASE_510+5
#define TCM		BASE_510+6
#define GSR		BASE_510+7
#define ICM		BASE_510+7

#define FMD		BASE_510+1
#define TMD		BASE_510+3
#define IMD		BASE_510+4
#define ACR1		BASE_510+5
#define RIE		BASE_510+6
#define RMD		BASE_510+7

#define CLCF		BASE_510+0
#define BBL		BASE_510+0
#define BACF		BASE_510+1
#define BBH		BASE_510+1
#define BBCF		BASE_510+3
#define PMD		BASE_510+4
#define MIE		BASE_510+5
#define TMIE		BASE_510+6

/* set baud rate for the 18.432 Mhz Clock */

#define	BAUD_38400	16
#define BAUD_19200	32
#define BAUD_9600	64
#define BAUD_4800	128
#define BAUD_2400	256
#define BAUD_1200	512

#define STATUS	0xDFC00005
#define XTAL		18432000
