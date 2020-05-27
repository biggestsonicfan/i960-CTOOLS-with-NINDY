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

extern void set_prcb(PRCB *new_prcb);
extern com_put_byte(), com_init_msg(), com_put_msg(), com_put_long(), send_iac();

void get_cpu(const void *cmd_buf);
void send_iac_cmd(const void *cmd_buf);
void sys_ctl_cmd(const void *cmd_buf);
void set_prcb_cmd(const void *cmd_buf);

/*
 * This file contains the KX-specific implementations of architecture-dependent
 * commands from the host.  This includes stubs for commands that are not
 * allowed on the KX (e.g., sys_ctl).
 */

#define PUTBUF(msg) com_put_msg((const unsigned char *)&(msg), sizeof(msg))

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
	struct system_base system_base;

	system_base = get_system_base();   /* Get the current prcb ptr and sat ptr */

	com_init_msg();
	com_put_byte(GET_CPU);
	com_put_byte(OK);
	com_put_long((unsigned long)system_base.prcb_ptr);
	com_put_long((unsigned long)
	    system_base.sat_ptr[system_base.prcb_ptr->magic_number_2>>6].address);
	com_put_long((unsigned long)system_base.prcb_ptr->fault_table_adr);
	com_put_long((unsigned long)system_base.prcb_ptr->interrupt_table_adr);
        com_put_long((unsigned long)system_base.prcb_ptr->interrupt_stack);
	com_put_long((unsigned long)system_base.sat_ptr);
	com_put_long(0UL);	/* ctl tbl (CA only) */
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
	CMD_TMPL response;
	response.cmd = ((unsigned char *)p_cmd)[0];
	response.stat = E_BAD_CMD;
	PUTBUF(response);
}


/*****************************************************************************
 *
 * FUNCTION NAME: send_iac_cmd
 *
 * ACTION:
 *      send_iac_cmd implements the target side of the IAC command.
 *
 *****************************************************************************/
void
send_iac_cmd(const void *p_cmd)
{
	register const unsigned char *rp = (unsigned char *)p_cmd + 2;
	unsigned long dest_addr;
	unsigned long iac[4];
        CMD_TMPL response;

	dest_addr = get_long(rp);
	iac[0] = get_long(rp);
	iac[1] = get_long(rp);
	iac[2] = get_long(rp);
	iac[3] = get_long(rp);

        send_iac(dest_addr, iac);

        response.cmd = SEND_IAC;
        response.stat = OK;

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
	unsigned long restart_iac[4];
	CMD_TMPL response;

	prcb = get_long(rp);

	set_prcb((PRCB *)prcb);

	restart_iac[0] = 0x93000000;
	restart_iac[1] = (unsigned long)get_system_base().sat_ptr;
	restart_iac[2] = prcb;
	restart_iac[3] = 0;		/* send_iac will fill in the proper value */

	send_iac(0, restart_iac);

	response.cmd = SET_PRCB;
	response.stat = OK;

	PUTBUF(response);
}
