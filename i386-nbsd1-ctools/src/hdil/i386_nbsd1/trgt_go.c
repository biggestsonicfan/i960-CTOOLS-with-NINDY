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
 * SERVER: libc support during execution
 */
/* $Header: /ffs/p1/dev/src/hdil/common/RCS/trgt_go.c,v 1.3 1995/01/11 19:51:14 gorman Exp $$Locker:  $ */

#include "private.h"
#include "hdi_com.h"
#include "dbg_mon.h"

int _hdi_running = FALSE;

/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: target_go						     */
/*                                                                           */
/* ACTION:                                                                   */
/*	target_go controls exiting the target monitor to execute user code.  */
/*	It adjusts the trace controls, sets hardware and software	     */
/*	breakpoints,  writes the registers to the target, and then issues    */
/*	the target `exit to user' command.  It then waits for the target     */
/*	code to re-enter the target monitor.  When the target monitor is     */
/*	re-entered, this function determines why and what action should be   */
/*	taken.								     */
/*                                                                           */
/*****************************************************************************/
const STOP_RECORD *
hdi_targ_go(mode)
int mode;
{
	int bp_flag;
	unsigned long bp_instr;
	const STOP_RECORD *r;
	REG ip;

	if (_hdi_running)
	{
	    hdi_cmd_stat = E_RUNNING;
	    return((STOP_RECORD *)NULL);
	}

	if (_hdi_put_regs() != OK
	    || _hdi_flush_cache() != OK
	    || hdi_reg_get(REG_IP, &ip) != OK)
	{
	    return((STOP_RECORD *)NULL);
	}

	bp_flag = _hdi_bp_instr(ip, &bp_instr);

	com_init_msg();
	if (com_put_byte(EXEC_USR) != OK
	    || com_put_byte(~OK) != OK
	    || com_put_byte(mode & GO_MASK) != OK
	    || com_put_byte(bp_flag) != OK
	    || com_put_long(bp_instr) != OK)
	{
	    hdi_cmd_stat = com_get_stat();
	    return((STOP_RECORD *)NULL);
	}

	if (_hdi_send(0) == NULL)
	    return((STOP_RECORD *)NULL);

	_hdi_running = TRUE;
	_hdi_invalidate_registers();
	_hdi_serve_init();

	if ((mode & GO_BACKGROUND) != 0)
	{
	    static const STOP_RECORD s = { STOP_RUNNING };
	    return(&s);
	}

	/* FALSE argument to serve means wait forever for response */
	r = _hdi_serve(FALSE);

	return(r);
}


/*
 * Interrupt the target via the mechanism provided for in this implementation's
 * host-target communication scheme.  If the target was executing it should 
 * be pre-empted and enter the monitor.  If we do not receive a stop record
 * the target is corrupted.
 */
const STOP_RECORD *
hdi_targ_intr()
{
	const STOP_RECORD *r;

	if (!_hdi_running)
	{
		hdi_cmd_stat = E_NOTRUNNING;
		return((STOP_RECORD *)NULL);
	}

	if (_hdi_break_causes_reset)
	{
		/* Hdi_reset will send the break.  If it does not
		 * return OK, hdi_cmd_stat is already set.  */
		if (hdi_reset() == OK)
		    hdi_cmd_stat = E_RESET;
		return((STOP_RECORD *)NULL);
	}

	if (com_intr_trgt() != OK)
	{
		hdi_cmd_stat = com_get_stat();
		return((STOP_RECORD *)NULL);
	}

	/* TRUE => target has been interrupted; expect response
	 * within timeout */
	r = _hdi_serve(TRUE);
	_hdi_fast_download.download_active = FALSE;

	return(r);
}
