/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994, 1995 Intel Corporation
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

#include <string.h>

#include "retarget.h"
#include "mon960.h"
#include "hdi_com.h"


/* 'end' is actually the linker-generated address of the first location *after*
 * BSS.  We store restart information there because there are some things we
 * want to remember across system restarts, but data and bss get reinitialized
 * (clobbered) when the monitor restarts.
 *
 * Meanings of the fields:
 *	r_magic	The structure only contains valid information if this
 *		is set to the value RESTART_ARGS_VALID.  (There is nothing
 *		valid here when the board is first powered up.)
 *
 *	r_baud	Set to the baud rate of the serial port. This baud rate
 *		should be restored after restart.
 *
 *	r_host	Set to TRUE if the monitor was connected to a host.
 */
extern struct restart {
	unsigned long r_magic;
	unsigned long r_baud;
	int	      r_host;
} end;

#define RESTART_ARGS_VALID	0x12345678


extern void init_monitor(int);

extern void (*restart_func)();

static void init_regs();
static void restart();


void
main()
{
	init_regs();
	init_hardware();

	/* Check if a valid restart information (host connection/baud rate)
	 * has been saved. */
	if (end.r_magic == RESTART_ARGS_VALID)
	{
	    host_connection = end.r_host;
	    baud_rate = end.r_baud;
	}

	end.r_magic = 0;	/* restart information is not valid unless
				 * we are reset by the appropriate command. */
	restart_func = restart;

	init_monitor(FALSE);

	monitor(NULL);
	/* NOTREACHED */
}


/****************************************/
/* Initialize Register Set    		*/
/* 					*/ 
/* initialize the stored "register set" */
/* to correct values, so that if called */
/* from a start up routine, the regs    */
/* will be set appropriately 		*/
/****************************************/
static void
init_regs()
{
	extern int user_stack[];
	int i;

#if HXJX_CPU
    unsigned int  brk_resource;
#endif
    

	/* set frame, stack pointer to point at applciation stack */
	register_set[REG_FP]  = (REG)user_stack;
	register_set[REG_PFP] = 0;
	register_set[REG_SP]  = (REG)user_stack + 0x40;

	register_set[REG_IP] = 0;
#if CXHXJX_CPU
	register_set[REG_IPND] = 0;
	register_set[REG_IMSK] = 0;
#if CX_CPU
	register_set[REG_DMAC] = 0;
#endif /*CX*/
#if HX_CPU
	register_set[REG_CCON] = 0;
	register_set[REG_ICON] = 0;
	register_set[REG_GCON] = 0;
#endif /*CX*/
#endif /*CXHXJX*/

	/* initialize user process controls based on current pc value */
	register_set[REG_PC] = (i960_modpc(0, 0) & ~0x001f2103) | 0x001f0003;
	register_set[REG_AC] = 0x3b001000;

	/* initialize the Trace Controls register to a sensible value */
	register_set[REG_TC] = 0x00000080; /* Breakpoint Trace Mode Enable */

	/* initialize floating point register set */
	for (i = 0; i < NUM_FP_REGS; i++)
	    memset(&fp_register_set[i], 0, sizeof(FPREG));

#if HXJX_CPU
	/*
     * Dynamically determine number of HW data and instruction brkpts that
     * are available (i.e., not currently being used by an ICE).
     */
    brk_resource       = get_brkreq();
    instr_brkpts_avail = brk_resource & 0xf;
    data_brkpts_avail  = brk_resource >> 4;
#endif  /* HXJX */
}

/* Setup restart block so parameters are retained after the reset */
static void
restart()
{
	end.r_baud = baud_rate;
	end.r_host = host_connection;
	end.r_magic = RESTART_ARGS_VALID;
}
