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
/*
 *	8250 definitions for AT serial communications
 */
/* $Header: /ffs/p1/dev/src/hdilcomm/common/RCS/8250.h,v 1.1 1994/04/05 15:55:22 gorman Exp $$Locker:  $ */

/* register addresses */
#define ST_REG		5

#define	LCNT_REG	0
#define	HCNT_REG	1

#define	IER_REG		1
#define	ISR_REG		2
#define	LCR_REG		3
#define	MCR_REG		4
#define	LSR_REG		5
#define	MSR_REG		6


/* register bit maps */
	/* status register */
#define	GETRDYBIT	0x01
#define	PUTRDYBIT	0x20
#define	ERRBITS		0x0E

	/* interrupt register */
#define	ENMODST 	0x08
#define	ENRECST 	0x04
#define	ENTXINT 	0x02
#define	ENRXINT		0x01
	/* interrupt status register */
#define	INTID1	 	0x04
#define	INTID0 		0x02
#define	NOTVALID	0x01
	/* line control register */
#define	ENCNTREG	0x80
#define	SETBREAK	0x40
#define	STICKPARITY 	0x20
#define	EVENPARITY	0x10
#define	ENPARITY 	0x08
#define	EN2STOP 	0x04
#define	CHLEN1 		0x02
#define	CHLEN0		0x01
	/* modem control register */
#define	ENLOOP	 	0x10
#define	OUT2 		0x08
#define	OUT1 		0x04
#define	RTS 		0x02		/* inverted */	
#define	DTR		0x01		/* inverted */
	/* line status register */
#define	NONN	 	0x80
#define	TSREADY		0x40
#define	TXREADY 	0x20
#define	BREAKINT 	0x10		/* break occured (full char size all 0) */
#define	FRAMINGERR 	0x08
#define	PARITYERR 	0x04
#define	OVERRUNERR 	0x02
#define	RXREADY		0x01
	/* modem status register */
#define	RLSD 		0x80
#define	RI 		0x40
#define	DSR 		0x20
#define	CTS 		0x10
#define	D_R 		0x08		/* all D_ are delta */
#define	TER 		0x04
#define	D_DSR 		0x02
#define	D_CTS		0x01
