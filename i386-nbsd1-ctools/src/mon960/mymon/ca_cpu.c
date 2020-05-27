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
#include "dbg_mon.h"
#include "hdi_errs.h"

extern PRCB *get_prcbptr();
extern void set_prcb(PRCB *new_prcb);
extern com_init_msg(), com_put_byte(), com_put_long(), com_put_msg();
extern send_sysctl();

void get_cpu(const void *cmd_buf);
void send_iac_cmd(const void *cmd_buf);
void sys_ctl_cmd(const void *cmd_buf);
void set_prcb_cmd(const void *cmd_buf);

/*
 * This file contains the CA-specific implementations of architecture-dependent
 * commands from the host.  This includes stubs for commands that are not
 * allowed on the CA (e.g., send_iac_cmd).
 */

#define PUTBUF(msg) com_put_msg((const unsigned char *)&(msg), sizeof(msg))

/*
 * System control instruction message types.
 */
#define POST_INTERRUPT     0
#define PURGE_CACHE        1
#define CONFIGURE_CACHE    2
#define REINIT_PROCESSOR   3
#define MOD_CTL_REGS       4

/*****************************************************************************
 *
 * FUNCTION NAME: get_cpu
 *
 * ACTION:
 *      get_cpudata returns the current cpu information.
 *
 *****************************************************************************/
void
get_cpu(const void *p_cmd)
{
	PRCB *prcb_ptr = get_prcbptr();

	com_init_msg();
	com_put_byte(GET_CPU);
	com_put_byte(OK);
        com_put_long((unsigned long) prcb_ptr);
        com_put_long((unsigned long) prcb_ptr->sys_proc_table_adr);
        com_put_long((unsigned long) prcb_ptr->fault_table_adr);
        com_put_long((unsigned long) prcb_ptr->interrupt_table_adr);
        com_put_long((unsigned long) prcb_ptr->interrupt_stack);
	com_put_long(0UL);	/* sat (Kx only) */
        com_put_long((unsigned long) prcb_ptr->cntrl_table_adr);
	com_put_msg(NULL, 0);
}



/*****************************************************************************
 *
 * FUNCTION NAME: sys_ctl_cmd
 *
 * ACTION:
 *      sys_ctl_cmd executes a system control instruction.
 *      This command is valid only for the CA.
 *
 *****************************************************************************/
void
sys_ctl_cmd(const void *p_cmd)
{
	register const unsigned char *rp = (unsigned char *)p_cmd + 2;
	unsigned long field1, field2, field3;
	int mess_type;
	CMD_TMPL response;

	field1 = get_long(rp);
	field2 = get_long(rp);
	field3 = get_long(rp);

	mess_type = (field1 >> 8) & 0xff;

	response.cmd = SYS_CNTL;

	/* Must use restart to reinitialize */
	if (mess_type == REINIT_PROCESSOR)
		response.stat = E_ARG;
	else
	{
		send_sysctl(field1, field2, field3);
		response.stat = OK;
	}

	PUTBUF(response);
}


/*****************************************************************************
 *
 * FUNCTION NAME: send_iac_cmd
 *
 * ACTION:
 *      send_iac_cmd implements the target side of the IAC command.
 *      This command is not allowed for the CA; return BAD_CMD.
 *
 *****************************************************************************/
void
send_iac_cmd(const void *p_cmd)
{
	CMD_TMPL response;
	response.cmd = ((unsigned char *)p_cmd)[0];
	response.stat = E_BAD_CMD;
	PUTBUF(response);
}


/*****************************************************************************
 *
 * FUNCTION NAME: set_prcb_cmd
 *
 * ACTION:
 *      set_prcb_cmd restarts the processor with a new prcb.
 *
 *****************************************************************************/
void
set_prcb_cmd(const void *p_cmd)
{
	register const unsigned char *rp = (unsigned char *)p_cmd + 2;
	ADDR prcb;
	CMD_TMPL response;

	prcb = get_long(rp);

	set_prcb((PRCB *)prcb);

	/* Passing 0 for the instruction address (second parameter) causes
	 * send_sysctl to return here. */
	send_sysctl(0x300, 0UL, prcb);

	response.cmd = SET_PRCB;
	response.stat = OK;

	PUTBUF(response);
}
