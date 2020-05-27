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
/*****************************************************************************/
/*                                                                           */
/* MODULE NAME: set_prcb.c                                                   */
/*                                                                           */
/*****************************************************************************/

#include "mon960.h"
#include "retarget.h"
#include "this_hw.h"

static void init_sys_procs(SYS_PROC_TABLE *sptbl);

/*===========================================================================*/
/* GLOBAL VARIABLES                                                          */
/*===========================================================================*/

int prcb_changed = FALSE;
PRCB *prcb_ptr = NULL;


/* Set up monitor-reserved fields of process control block */
void
set_prcb(PRCB *prcb)
{
#if KXSX_CPU
	static const FAULT_TABLE_ENTRY trace_fault_entry =
		{ FAULT_SYS_CALL(0), TRACE_MAGIC_NUMBER };
#endif /* KXSX */

#if CXHXJX_CPU
	static const FAULT_TABLE_ENTRY trace_fault_entry =
		{ FAULT_SYS_CALL(255), FAULT_MAGIC_NUMBER };
#if CX_CPU
	PRCB *old_prcb = prcb_ptr;
#endif /* CX */
#endif /* CXHXJX */

	prcb_changed = TRUE;
	prcb_ptr = prcb;

	set_break_vector(-1, prcb);
	prcb->fault_table_adr->ft_entry[TRACE_FAULT] = trace_fault_entry;

#if CX_CPU
	/* Initialize the reserved entries in the system procedure table.
	 * This is only done for the CA because the pointer to the system
	 * procedure table in the Kx is not in the PRCB. */
	init_sys_procs(prcb->sys_proc_table_adr);

	if (old_prcb != NULL
	    && prcb->cntrl_table_adr != old_prcb->cntrl_table_adr)
	{
	    CONTROL_TABLE *new_ctl_tab = prcb->cntrl_table_adr;
	    CONTROL_TABLE *old_ctl_tab = old_prcb->cntrl_table_adr;
	    new_ctl_tab->control_reg[IPB0] = old_ctl_tab->control_reg[IPB0];
	    new_ctl_tab->control_reg[IPB1] = old_ctl_tab->control_reg[IPB1];
	    new_ctl_tab->control_reg[DAB0] = old_ctl_tab->control_reg[DAB0];
	    new_ctl_tab->control_reg[DAB1] = old_ctl_tab->control_reg[DAB1];
	    new_ctl_tab->control_reg[BPCON] = old_ctl_tab->control_reg[BPCON];
	}
#endif /* CX */

#if HXJX_CPU
	init_sys_procs(prcb->sys_proc_table_adr);
#endif /* HXJX */
}


#if KXSX_CPU
struct system_base
get_system_base()
{
	struct system_base system_base;
	unsigned long system_base_iac[4];

	system_base_iac[0] = 0x80000000;
	system_base_iac[1] = (unsigned long)&system_base;

	send_iac(0, system_base_iac);	/* Get the current prcb ptr and sat ptr */

	return system_base;
}
#endif /* KXSX */


struct systbl_entries {
	int number;
	void (*proc)();
};

static void init_sys_proc_entries(SYS_PROC_TABLE *sptbl, int min, int max,
	       const struct systbl_entries *systbl_entries);

/*
 * Init_sys_proc_tbl is called by init to initialize the monitor-defined
 * system procedure table.  It is not called by set_prcb, because it
 * changes the entire table, including entries which may have already
 * been initialized by the user.
 * It initializes the entire table to unknown, except for no reserved
 */
void
init_sys_proc_tbl(SYS_PROC_TABLE *sptbl)
{
	extern int trap_stack[];

	sptbl->super_stack = trap_stack;
	init_sys_procs(sptbl);
}


/*
 * This routine initializes the monitor reserved entries in the system
 * procedure table.  Since these are reserved, they are initialized any
 * time set_prcb is called.  (For the CA only.)
 */
static void
init_sys_procs(SYS_PROC_TABLE *sptbl)
{
	extern void unknown(), fault_entry(), mon_entry(), _post_test();
	extern void sdm_open(), sdm_read(), sdm_write(), sdm_lseek(),
	    sdm_close(), sdm_isatty(), sdm_stat(), sdm_fstat(), sdm_rename(),
	    sdm_unlink(), sdm_time(), sdm_system(), sdm_arg_init(), sdm_exit(),
	    app_exit_user(), app_go_user(), bentime_onboard_only();
    extern void init_bentime(), bentime(), term_bentime(), init_bentime_noint(),
        mon_set_timer(), bentime_noint(), init_eeprom(); 

	static const struct systbl_entries systbl_entries[] = {
	    { 220, init_bentime },
	    { 221, bentime },
	    { 222, term_bentime },
	    { 223, init_bentime_noint },
	    { 224, bentime_noint },
	    { 225, init_eeprom },
	    { 226, (void (*)()) is_eeprom },
	    { 227, (void (*)()) check_eeprom },
	    { 228, (void (*)()) erase_eeprom },
	    { 229, (void (*)()) write_eeprom },
	    { 230, sdm_open },
	    { 231, sdm_read },
	    { 232, sdm_write },
	    { 233, sdm_lseek },
	    { 234, sdm_close },
	    { 235, sdm_isatty },
	    { 236, sdm_stat },
	    { 237, sdm_fstat },
	    { 238, sdm_rename },
	    { 239, sdm_unlink },
	    { 240, sdm_time },
	    { 241, sdm_system },
	    { 242, sdm_arg_init },
	    { 243, _init_p_timer },
	    { 244, set_prcb },
	    { 245, (void (*)())get_prcbptr },
	    { 246, _term_p_timer },		/* obsolete cmdbf entry */
	    { 247, set_mon_priority },
	    { 248, (void (*)())get_mon_priority },
		{ 249, mon_set_timer },
        /*250  reserved for CAVE function */
	    { 252, bentime_onboard_only },
	    { 253, _post_test },
	    { 254, mon_entry },
	    { 255, fault_entry },
	    { 256, fault_entry },
	    { 257, sdm_exit },		/* exit application */
	    { 258, app_exit_user },
	    { 259, app_go_user },
	    { -1 }
	};

	init_sys_proc_entries(sptbl, 220, 259, systbl_entries);
}
	
static void
init_sys_proc_entries(SYS_PROC_TABLE *sptbl, int min, int max,
	       const struct systbl_entries *systbl_entries)
{
	extern void unknown();
	int i;
	const struct systbl_entries *p;

	/* First initialize the reserved entries to unknown. */
	for (i = min; i <= max; i++)
	    sptbl->sysp_entry[i] = SUPER_CALL(unknown);

	/* Then initialize the entries that are defined. */
	for (p = systbl_entries; p->number >= 0; p++)
	    sptbl->sysp_entry[p->number] = SUPER_CALL(p->proc);
}

void
set_break_vector(int new_vector, PRCB *prcb)
{
	if (prcb == NULL)
	    prcb = prcb_ptr;

	if (new_vector != -1)
	    break_vector = new_vector;

	if (break_vector > 7)
	{
	    prcb->interrupt_table_adr->interrupt_proc[break_vector - 8] = intr_entry;

#if CXHXJX_CPU
#define cache_table ((void (**)())0)
	    if (break_vector == 248) /* NMI */
		cache_table[0] = intr_entry;
	    else
	    {
		init_imap_reg(prcb);

		if ((prcb->cntrl_table_adr->control_reg[ICON]
			& VECTOR_CACHE_ENABLE(1))
		    && (break_vector & 0x0f) == 2)
		cache_table[break_vector >> 4] = intr_entry;
	    }
#endif /*CXHXJX*/
	}
}

PRCB *
get_prcbptr()
{
	return prcb_ptr;
}

int
allow_unaligned()
{
#if KXSX_CPU
    return TRUE;
#endif /*KXSX*/

#if CX_CPU
    /* CA allows unaligned accesses only if the fault is masked. */
    return (prcb_ptr->fault_config & (1 << 30)) != 0;
#endif /*CX_CPU*/
}
