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
#include "hdi_stop.h"

extern int load_mem(ADDR, int mem_size, void *buf, int buf_size);
extern void check_brk_pt(ADDR, STOP_RECORD *);
extern int break_vector;

#define FMARK 0x66003e00
#define MARK  0x66003d80

static STOP_RECORD stop_reason;

STOP_RECORD *fault_handler(FAULT_RECORD *);
static void trace_fault_handler(FAULT_RECORD *);

/*****************************************/
/* Fault Handler Routine	 	 */
/* 					 */
/* This is the C-level fault handler.	 */
/* It determines whether or not the	 */
/* fault is a trace fault and dispatches */
/* it appropriately			 */
/* The argument is a pointer to the      */
/* fault record.                         */
/*****************************************/
STOP_RECORD *
fault_handler(FAULT_RECORD *fault_ptr)
{
	int type = (fault_ptr->type >> 16) & 0xff;

	if  (type == 1 ){
		trace_fault_handler(fault_ptr);
	} else {
		stop_reason.reason = STOP_FAULT;
		stop_reason.info.fault.type = type;
		stop_reason.info.fault.subtype = fault_ptr->type & 0xff;
		stop_reason.info.fault.ip = (ADDR)fault_ptr->addr;
		stop_reason.info.fault.record = (ADDR)fault_ptr;
	}

	return &stop_reason;
}


static void
trace_fault_handler(FAULT_RECORD *fault_ptr)
{
	ADDR ip = (ADDR)fault_ptr->addr;
	unsigned long instr;

	stop_reason.reason = 0;

	check_brk_pt(ip, &stop_reason);

	if (load_mem(ip, WORD, &instr, sizeof(instr)) == OK
	    && (instr == FMARK || instr == MARK))
	{
	    stop_reason.reason |= STOP_BP_SW;
	    stop_reason.info.sw_bp_addr = ip;
	}
	else if (load_mem(register_set[REG_IP],WORD,&instr,sizeof(instr)) == OK
	    && (instr == FMARK || instr == MARK))
	{
	    stop_reason.reason |= STOP_BP_SW;
	    stop_reason.info.sw_bp_addr = register_set[REG_IP];
	}

	/* Check subtype for causes other than breakpoint */
	if (fault_ptr->type & 0x7e)
	{
	    stop_reason.reason |= STOP_TRACE;
	    stop_reason.info.trace.type = (fault_ptr->type & 0x7e);
	    stop_reason.info.trace.ip = ip;
	}
}

STOP_RECORD *
intr_handler(int vector)
{
	if (vector != break_vector)
	{
	    stop_reason.reason = STOP_INTR;
	    stop_reason.info.intr_vector = vector;
	}
	else
	    stop_reason.reason = STOP_CTRLC;

	return &stop_reason;
}

STOP_RECORD *
exit_handler(int cause)
{
	static const reasons[] = { 0,0,STOP_EXIT,STOP_MON_ENTRY,STOP_UNK_SYS };
	stop_reason.reason = reasons[cause];
	stop_reason.info.exit_code = register_set[REG_G0];
	return &stop_reason;
}
