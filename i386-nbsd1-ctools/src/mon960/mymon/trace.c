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

#include "ui.h"
#include "i960.h"

extern prtf(), strncmp(), badarg();

/* Mask for appropriate bit in register tc for each trace type
 */
#define	BRANCH 	0x04		/* branch trace		*/
#define	CALL 	0x08		/* call trace		*/
#define	RET 	0x10		/* return trace		*/
#define	SUP 	0x40		/* supervisor trace	*/

static void display_trace(unsigned int bitmask);


/************************************************/
/* Trace Flags Set or Cleared        	 	*/
/*                           			*/
/************************************************/
void
trace( dummy, nargs, type, state )
int dummy;	/* Ignored */
int nargs;	/* Number of the following arguments that are valid (0,1,2) */
char *type;	/* Optional trace type: "br", "ca", "re", or "su"	*/
char *state;	/* Desired trace state ("on" or "off")	*/
{
unsigned int bitmask=0;

	if ( nargs > 0 ){

		/* First argument (trace type) */

		if ( !strncmp(type,"br",2) ){
			bitmask = BRANCH;
		} else if ( !strncmp(type,"ca",2) ){
			bitmask = CALL;
		} else if ( !strncmp(type,"re",2) ){
			bitmask = RET;
		} else if ( !strncmp(type,"su",2) ){
			bitmask = SUP;
		} else {
			badarg( type );
			return;
		}

		/* Second argument ("on"/"off") */

		if ( nargs < 2 ) {
			/* no 2nd argument, display trace status */
			display_trace(bitmask);
			return;
		} else if ( !strncmp(state,"of",2) ){
			register_set[REG_TC] &= ~bitmask;
		} else if ( !strncmp(state,"on",2) ){
			register_set[REG_TC] |= bitmask;
		} else {
			badarg( state );
			return;
		}
	}

	display_trace(BRANCH);
	display_trace(CALL);
	display_trace(RET);
	display_trace(SUP);
}

/************************************************/
/* Display Trace Status                    	*/
/*                           			*/
/************************************************/
static void
display_trace(bitmask)
unsigned int bitmask;	/* BRANCH, CALL, RET, or SUP */
{
	unsigned char *p = "";

	switch ( bitmask ){
	case BRANCH: p = "Branch";     break;
	case CALL:   p = "Call";       break;
	case RET:    p = "Return";     break;
	case SUP:    p = "Supervisor"; break;
	}

	prtf("%s trace %s\n",p,register_set[REG_TC] & bitmask ? "on" : "off" );
}
