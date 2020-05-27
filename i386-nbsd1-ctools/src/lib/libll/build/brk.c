/*******************************************************************************
 * 
 * Copyright (c) 1993 Intel Corporation
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

/*
 * Simulation of brk(2), sbrk(2) system calls.
 */
#include "errno.h"

extern char heap_base[];
extern char heap_end[];

static char *curbrk;


/*
 * This routine initializes the curbrk pointer.  This must be done
 * at run-time, not at link-time, because the value of heap_base
 * may change if Position-Independent Data (PID) is used.
 *
 * heap_base is defined in the linker-directive files (*.ld).  As
 * shipped, the .ld files define heap_base relative to the end of
 * the data section.  If you want to have an absolute area for the
 * heap, you can simply change the definition of heap_base in the .ld
 * file.  If you want an absolute heap plus PID, you must also change
 * the references to heap_base and heap_end in this module so that
 * they are not biased by g12 (g12 is the PID base register).
 */
void
ll_brk_init()
{
	curbrk = heap_base;
}


char *
sbrk(incr)
unsigned incr;
{
	register char *ret, *end;

	/* Increment the end of the data area by incr bytes.  Return
	 * a pointer to the new memory allocated (i.e., the previous
	 * value of the end of the data area.  Return (char *)-1 if
	 * not enough memory is available. */

	if (incr == 0)
	    return curbrk;

	ret = (char *)((unsigned long)(curbrk + 0xf) & (~0xf));
	end = ret + (incr - 1);

	if (ret < curbrk || end < ret		/* check for wrap-around */
	    || end > heap_end)			/* check for not enough mem */
	{
	    errno = ENOMEM;
	    return((char *) -1);
	}

	curbrk = end + 1;
	if (curbrk < end)	     /* special case for heap_end=0xffffffff */
		curbrk = heap_end;

	return(ret);
}

int brk(endds)
char *endds;
{
	/* Set the end of the data area to endds.  Return 0 if endds is
	 * valid; -1 otherwise.  */

	if( endds < heap_base || endds - 1 > heap_end ) {
		errno = ENOMEM;
		return(-1);
	}
	curbrk  = endds;
	return(0);
}
