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

#define BASE_510	0x20000000

#define TXD		BASE_510+0
#define RXD		BASE_510+0
#define BAL		BASE_510+0
#define BAH		BASE_510+4
#define GER		BASE_510+4
#define GIR		BASE_510+8
#define BANK		BASE_510+8
#define LCR		BASE_510+12
#define MCR		BASE_510+16
#define LSR		BASE_510+20
#define MSR		BASE_510+24
#define ACR0		BASE_510+28

#define RXF		BASE_510+4
#define TXF		BASE_510+4
#define TMST		BASE_510+12
#define TMCR		BASE_510+12
#define FLR		BASE_510+16
#define RST		BASE_510+20
#define RCM		BASE_510+20
#define TCM		BASE_510+24
#define GSR		BASE_510+28
#define ICM		BASE_510+28

#define FMD		BASE_510+4
#define TMD		BASE_510+12
#define IMD		BASE_510+16
#define ACR1		BASE_510+20
#define RIE		BASE_510+24
#define RMD		BASE_510+28

#define CLCF		BASE_510+0
#define BBL		BASE_510+0
#define BACF		BASE_510+4
#define BBH		BASE_510+4
#define BBCF		BASE_510+12
#define PMD		BASE_510+16
#define MIE		BASE_510+20
#define TMIE		BASE_510+24

/* set baud rate for the 9.8304 MHZ Clock */

#define	BAUD_38400	16
#define BAUD_19200	32
#define BAUD_9600	64
#define BAUD_4800	128
#define BAUD_2400	256
#define BAUD_1200	512

#define XTAL		9830400
