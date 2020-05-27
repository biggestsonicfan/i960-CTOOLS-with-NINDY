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
#include "i960.h"
#include "hdi_brk.h"
#include "hdi_errs.h"

extern void pgm_bp_hw();
extern struct bpt bptable[];

int instr_brkpts_avail = 2;
#if KXSX_CPU
int data_brkpts_avail = 0;
#else
int data_brkpts_avail = 2;
#endif

/* Set a hardware or data breakpoint, according to `type', at `addr'.
 * This is done by searching the breakpoint table for an unused breakpoint
 * of the given type, and changing it to an active breakpoint. */

int
set_bp(int type, int mode, ADDR addr)
{
	struct bpt *bp;
    int i = 0;

    /* if KX and BRK_DATA this is not available */
#if KXSX_CPU
	if (type == BRK_DATA)
	{
	    cmd_stat = E_ARCH;
	    return ERR;
	}
#endif /*KXSX*/

	/* Hardware breakpoints must be set on an instruction boundary. */
	if (type == BRK_HW && (addr & 3) != 0)
	{
	    cmd_stat = E_ALIGN;
	    return ERR;
	}

	/* Check whether there is an existing breakpoint
	 * of any type at the given address. */
	for (bp = bptable; bp->type != BRK_NONE; bp++)
	{
		if (bp->active && bp->addr == addr)
		{
		    cmd_stat = E_BPSET;
		    return ERR;
		}
	}

	/* Find an unused breakpoint in the table. */
	for (bp = bptable; bp->type != BRK_NONE; bp++)
	{
		if ((bp->type == type && !bp->active) && 
            (( (type == BRK_HW) && (i<instr_brkpts_avail)) ||
             ( (type == BRK_DATA) && ((i-instr_brkpts_avail)<data_brkpts_avail)) ))
		{
			bp->addr = addr;
			bp->active = TRUE;
			bp->mode = mode;
			pgm_bp_hw();
			return OK;
		}
        i++;
	}

	cmd_stat = E_BPUNAVAIL;
	return ERR;
}

/* Clear the breakpoint at the given address. */
int
clr_bp(ADDR addr)
{
	struct bpt *bp;

	for (bp = bptable; bp->type != BRK_NONE; bp++)
	{
		if (bp->active && bp->addr == addr)
		{
		    bp->active = FALSE;
		    pgm_bp_hw();
		    return OK;
		}
	}

	cmd_stat = E_BPNOTSET;
	return ERR;
}

/* Return a pointer to the breakpoint table.
 * This is used by the user interface. */
const struct bpt *
bptable_ptr()
{
    return bptable;
}
