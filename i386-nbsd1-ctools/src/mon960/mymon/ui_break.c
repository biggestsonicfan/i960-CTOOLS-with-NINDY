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
#include "hdi_brk.h"
#include "hdi_errs.h"

extern const int have_data_bpts; /* TRUE if there are one or more data
				  * breakpoint entries in the table */
extern int set_bp(int type, int mode, ADDR addr);
extern int clr_bp(ADDR);
extern const struct bpt *bptable_ptr();
extern prtf();

static const char no_dbpts[] = "No Data breakpoints in this architecture\n";
static const char bp_set_err[] = "Error setting breakpoint at %X\n";

/************************************************/
/* Display Breakpoints              	 	*/
/************************************************/
static void
display_bpt( type )
char type;	/* BRK_HW or BRK_DATA */
{
	int count;
	const struct bpt *bp;

	count = 0;
	prtf("%s breakpoints set: ",type==BRK_HW ? "Instruction":"Data");
	for ( bp = bptable_ptr(); bp->type != BRK_NONE; bp++ ){
		if (bp->active && bp->type == type){
			count++;
			prtf (" %X", bp->addr);
		}
	}
	if (count == 0) {
		prtf (" none");
	}
	prtf("\n");
}

/************************************************/
/* Set a Breakpoint              	 	*/
/*                           			*/
/* 'breakpt' and 'databreak' set (or display)	*/
/* instruction and data breakpoints, respectivly*/
/************************************************/
void
breakpt( dummy, nargs, addr )
int dummy;	/* Ignored */
int nargs;	/* Number of following arguments that are valid (0 or 1) */
int addr;	/* Optional breakpoint address				*/
{
	if (nargs == 0)
	    display_bpt(BRK_HW);
	else if (set_bp(BRK_HW, 0, addr) != OK)
	{
	    switch (cmd_stat)
	    {
		case E_BPSET:
		    prtf("Instruction breakpoint already set at %X\n", addr);
		    break;
		case E_BPUNAVAIL:
		    prtf("No Instruction breakpoints left\n");
		    break;
		case E_ALIGN:
		    prtf("Instruction breakpoint must be word aligned\n");
		    break;
		default:
		    prtf(bp_set_err, addr);
		    break;
	    }
	}
}


void
databreak( dummy, nargs, addr )
int dummy;	/* Ignored */
int nargs;	/* Number of following arguments that are valid (0 or 1) */
int addr;	/* Optional breakpoint address			*/
{
	if (!have_data_bpts)
		prtf(no_dbpts);
	else if (nargs == 0)
	    display_bpt(BRK_DATA);
	else if (set_bp(BRK_DATA, DBP_ANY, addr) != OK)
	{
	    if (cmd_stat == E_BPSET)
		prtf("Data breakpoint already set at %X\n", addr);
	    else if (cmd_stat == E_BPUNAVAIL)
		prtf("No more data breakpoints available\n");
	    else
		prtf(bp_set_err, addr);
	}
}

/************************************************/
/* Delete a Breakpoint              	 	*/
/*                           			*/
/* 'delete' removes instruction or data brkpts	*/
/************************************************/
void
delete( dummy, nargs, addr )
int dummy;	/* Ignored */
int nargs;	/* Number of following arguments that are valid (0 or 1) */
int addr;	/* Optional breakpoint address	*/
{
	if (clr_bp(addr) != OK)
	    if (cmd_stat == E_BPNOTSET)
		prtf("No breakpoint set at %X\n", addr);
	    else
		prtf("Error deleting breakpoint at %X\n", addr);
}
