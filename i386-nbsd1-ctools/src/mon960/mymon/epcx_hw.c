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
#include "epcx.h"
#include "hdi_arch.h"

extern void set_mon_priority(unsigned int priority);
extern unsigned int get_mon_priority();
extern PRCB *get_prcbptr();
extern enable_interrupt(), send_sysctl(), strcpy(), prtf();
extern int serial_intr(); 
extern void blink(), fatal_error(char id, int a, int b, int c, int d);

extern struct PRCB rom_prcb;

/************  EPCX BOARD-SPECIFIC CONSTANTS AND DATA  *****************/
/*********************************************************************
 * board identifier
 *********************************************************************/
int arch = ARCH_CA;	/* Processor type */
char arch_name[] = "CA";	/* Processor type name */
char board_name[] = "EP80960CX";   /* Formal product name  */

#if BIG_ENDIAN_CODE
    char target_common_name[] = "epcxbe";/* Informal target name */
#else
    char target_common_name[] = "epcx";     /* Informal target name */
#endif

unsigned int __cpu_speed     = 33; /* 33mhz is standard, for paradrvr timeout */
unsigned int __cpu_bus_speed = 33; /* Cx not clock-doubled. */
ADDR unwritable_addr = 0xf0000000;

const CONTROL_TABLE boot_control_table = {
	/* -- Group 0 -- Breakpoint Registers (reserved by monitor) */
	{0, 0, 0, 0,

	/* -- Group 1 -- Interrupt Map Registers */
	0, 0, 0,	/* Interrupt Map Regs (set by code as needed) */
	0xc3fc,		/* ICON - dedicated mode, edge detected, enabled,
			 * mask unchanged, not cached, fast, DMA suspended */

	/* -- Groups 2-5 -- Bus Configuration Registers */
	REGION_0_CONFIG, REGION_1_CONFIG, REGION_2_CONFIG, REGION_3_CONFIG,
	REGION_4_CONFIG, REGION_5_CONFIG, REGION_6_CONFIG, REGION_7_CONFIG,
	REGION_8_CONFIG, REGION_9_CONFIG, REGION_A_CONFIG, REGION_B_CONFIG,
	REGION_C_CONFIG, REGION_D_CONFIG, REGION_E_CONFIG, REGION_F_CONFIG,

	/* -- Group 6 -- Breakpoint, Trace and Bus Control Registers */
	0,			/* Reserved */
	0,			/* BPCON Register (reserved by monitor) */
	0,			/* Trace Controls  */
	REGION_TABLE_VALID(1)}	/* BCON Register  */
};

#define NMI	  /* Use NMI for serial break */
		/* This is controlled by a jumper; thus it may be changed. */
		/* If NMI is not used, the interrupt is connected to int 0. */

/* #define BUS_IDLE_INT  */
#define BUS_IDLE_IV   0xf2  /* interuupt vector for bus idle detector */
#define BRKIV   0xf8        /* board interrupt vector is NMI */
#define IMSK_VAL 1

/* Statics */
/* Used by bus_idle_test() */
CONTROL_TABLE temp_control_table,*pCntrlTbl;
PRCB temp_prcb,*pCurPRCB;
INTERRUPT_TABLE TempIntTbl,*pCurIntTbl;
unsigned long result;


/*-------------------------------------------------------------
 * Function:      void init_hardware()
 *
 * Action:        Sets up target control hardware and peripherals.
 *-------------------------------------------------------------*/
void
init_hardware()
{
	extern void intr_entry();

	blink(1);

#ifdef BUS_IDLE_INT
    PRCB *prcb;
    CONTROL_TABLE *control_tab;
	{
    prcb = get_prcbptr();
    control_tab = prcb->cntrl_table_adr;
    control_tab->control_reg[IMAP0] &= ~(0xf);
    control_tab->control_reg[IMAP0] |= BUS_IDLE_IV >> 4;

    send_sysctl(0x0401,0,0);    /* Read new imap0 contents */
    enable_interrupt(0);        /* Enable the bus idle timer interrupt */
	}
#endif
}

/* Return the break interrupt vector number to the monitor.  */
int
get_int_vector(int dummy)
{
    blink(2);
    return(BRKIV);
}


/* Set up the interrupt map register to use the appropriate vector. */
void
init_imap_reg(PRCB *prcb)
{
#ifndef NMI
    CONTROL_TABLE *control_tab = prcb->cntrl_table_adr;

#if BRK_PIN < 4
    control_tab->control_reg[IMAP0] &= ~(0xf << BRK_PIN*4);
    control_tab->control_reg[IMAP0] |= (BRKIV>>4) << BRK_PIN*4;
#else
    control_tab->control_reg[IMAP1] &= ~(0xf << (BRK_PIN - 4)*4);
    control_tab->control_reg[IMAP1] |= (BRKIV>>4) << (BRK_PIN - 4)*4;
#endif

#define NEW_PRIORITY ((BRKIV >> 3) - 1)
    if (get_mon_priority() > NEW_PRIORITY)
        set_mon_priority(NEW_PRIORITY);

    send_sysctl(0x0401,0,0);    /* Read new imap0 contents */
    enable_interrupt(BRK_PIN);
#endif /* NMI */

}

/*
 * Clear_break_condition is called when the monitor is entered due to a
 * interrupt from the serial port.  It should do any special processing
 * that has to happen in that situation.  For example, the interrupt
 * condition in the interrupt controller or the UART may have to be cleared.
 * Clear_break_condition returns TRUE if the interrupt was caused by
 * a break.  It returns FALSE if the interrupt was caused by some other
 * condition.  The monitor does not care about any conditions other than
 * break, but in some UARTs it is impossible to mask them without masking
 * the break interrupt.
 */
int
clear_break_condition(void)
{
    int flag;
    flag = serial_intr();
    return(flag);
}


/*-------------------------------------------------------------
 * Function:    void board_reset(void)
 *
 * Action:      Software board reset, the EV80960CA has two reset
 *              registers, one for the serial I/O, and one for
 *              the CPU. This function does not return.
 *-------------------------------------------------------------*/
void
board_reset(void)
{
	extern void reinit();

	asm("mov 0, sf1");
	asm("mov 0, sf2");
	asm("ld _boot_g0, g11");
	send_sysctl(0x300, reinit, &rom_prcb);

	/* SHOULDN'T GET HERE */
	fatal_error(4,(int)"Board_reset",0,0,0);
}


void
board_go_user()
{
    register_set[REG_IMSK] |= IMSK_VAL;
}

void
board_exit_user()
{
}
