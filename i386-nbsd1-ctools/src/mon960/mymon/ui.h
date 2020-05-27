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
 
#include "common.h"

/* ASCII characters
 */
#define	NUL		000	/* null */
#define	SOH  		001 	/* start of header */
#define	EOT		004	/* end of transmission */
#define	ACK  		006	/* acnowledge */
#define	BEL  		007	/* bell */
#define	TAB		011	/* tab */
#define	NAK  		025	/* no acnowledge */
#define	CAN		030	/* cancel */
#define	ESC		033	/* Escape */
#define	DEL		177	/* delete */
#define	CTRL_P		020	/* Control P */
#define	XON		021	/* flow control on */
#define	XOFF		023	/* flow control off */

#define	MAXDIGITS 	10	/* max num of digits in int */
#define TIMEOUT -1
#define	MAXARGS	5

/* Flags that can get passed to parse_arg() routine.
 * Must be separate bits (OR-able together).
 */
#define ARG_CNT  1	/* If set, count portion of argument is required */
#define ARG_ADDR 2	/* If set, address portion of argument is required */

extern char *get_regname();

#ifndef _UI_DEFINES
extern int downld;	/* If TRUE, xmodem download is in progress over the
			 * serial port. The download must be cancelled before
			 * error messages can be printed.  Also, since binary
			 * is downloaded, XON/XOFF checking must be suspended
			 * until the download is over.
			 */

extern int user_is_running;	/* TRUE if user's application program has
				 * been started, and monitor is running as
				 * result of a fault/breakpoint. FALSE otherwise
				 */
#endif
