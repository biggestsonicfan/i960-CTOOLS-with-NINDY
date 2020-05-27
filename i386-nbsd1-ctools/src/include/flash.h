
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

#define		TIMEOUT_6	6 	/* 6 uS timeout for FLASH */
#define		TIMEOUT_10	15 	/* 10 uS timeout for FLASH */
#define		TIMEOUT_100	100 	/* 100 uS timeout for FLASH */
#define		TIMEOUT_10000	10000	/* 10 mS timeout for FLASH */

/* GLOBALS USED FOR FLASH PROGRAMMING */
int numloops6;	 /* number of loops needed for 6uS FLASH timeout */
int numloops10;	 /* number of loops needed for 10uS FLASH timeout */
int numloops100; /* number of loops needed for 100uS FLASH timeout */
int bigloops;	 /* number of loops needed for 10mS+ FLASH timeout */
int fltype; 	 /* Flash type used for Flash download */
int flsize;	 /* Flash size used for Flash download */
int fladdr;	 /* Flash address used for Flash erase */


/* ASSEMBLY INLINE */

#	define GASM	__asm__ __volatile__

	__inline__ static void
	time( loops )
	{
		int cnt;

		GASM( "mov %1,%0" : "=d"(cnt) : "d"(loops) );
	  	GASM( "0: cmpdeco 0,%1,%0; bl 0b" : "=d"(cnt) : "0"(cnt) );
	}
