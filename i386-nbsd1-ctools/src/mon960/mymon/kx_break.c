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
#include "hdi_stop.h"

extern	void send_iac(int, void *);

/* iac structure */
typedef struct {
	unsigned short 	field2;
	unsigned char 	field1;
	unsigned char	message_type;
	unsigned int	field3;
	unsigned int	field4;
	unsigned int	field5;
} iac_struct;

/* Instruction breakpoints only in the 960KX architecture.
 */

struct bpt bptable[] = {
	{ BRK_HW, FALSE, 0, 0L },
	{ BRK_HW, FALSE, 0, 0L },
	{ BRK_NONE }
};

const int have_data_bpts = FALSE;

/**********************************************************************
 * Program Breakpoint Hardware 
 * This version, for the 960KX, uses an IAC to reload the appropriate
 * registers.  This routine reprograms all breakpoints to the state
 * currently found in 'bptable'.
 **********************************************************************/
void
pgm_bp_hw()
{
	iac_struct iac;

	if (bptable[0].active)
	    iac.field3 = bptable[0].addr & ~3;
	else
	    iac.field3 = 2;

	if (bptable[1].active)
	    iac.field4 = bptable[1].addr & ~3;
	else
	    iac.field4 = 2;

	iac.message_type = 0x8f;
	send_iac(0, &iac);
}

void
check_brk_pt(ADDR ip, STOP_RECORD *stop_reason)
{
	if ((register_set[REG_TC] & (1L << 23)) != 0
	    && ((bptable[0].active && bptable[0].addr == ip)
		|| (bptable[1].active && bptable[1].addr == ip)))
	{
	    stop_reason->reason |= STOP_BP_HW;
	    stop_reason->info.hw_bp_addr = ip;
	}
}
