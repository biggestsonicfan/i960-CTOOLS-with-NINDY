/*(CB*/
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

#include "mon960.h"
#include "retarget.h"
#include "this_hw.h"
#include "hdi_arch.h"

extern void cpu_identify(void);
extern void init_cio();
extern void init_squall();
extern void pci_hw_init();
extern int test_uart();
extern void start_ip();

extern struct PRCB rom_prcb;

/***********  C145/C146 KX/SX BOARD-SPECIFIC CONSTANTS AND DATA  *************/
/*********************************************************************
 * board identifier
 *********************************************************************/
int arch = ARCH_KB;                 /* Processor type */
char arch_name[32] = "KB";         /* add speed and mem size in cpu_identify() */

char target_common_name[] = "cykx";     /* Informal target name */


/*-------------------------------------------------------------
 * Function:      void init_hardware()
 *
 * Action:        Sets up target control hardware and peripherals.
 *-------------------------------------------------------------*/
void
init_hardware()
{
	unsigned char leds;

    /*
     * set interrupt control reg for
     * int0: vector e2 = timer
     * int1: f2 = serial break
     */
    const unsigned long regval =  (VECTOR_PCI << 16) + (VECTOR_UART << 8) + TIMER0_VECTOR;
    interrupt_register_write((int *) &regval);
    set_mon_priority(VECTOR_PCI/8 - 1);

    init_cio();        /* Initialize the Z8536 CIO */

    /* init_cio() MUST be called first */
    cpu_identify();        /* modify CPU string to include clock speed */
    init_squall();        /* Get Squall Module data */

    if (test_uart() == OK)    /* Run an internal loopback test on the uart */
    {
       leds = get_led_value();
       set_led_value(leds | LED_2);    /* uart test passed */
    }

    pci_hw_init();        /* Set Up PCI i960 Bridge */

    leds = get_led_value();
    set_led_value(leds | LED_3);     /* hardware init complete */
    pause(); /* Let user look at diag LEDs before erased by mon960 boot code. */
}


/*-------------------------------------------------------------
 * Function:    void board_reset(void)
 *
 * Action:      Software board reset;
 *        Mask interrupts, reload the rom-based prcb, and 
 *        reinitialize the CPU.
 *              This function does not return.
 *-------------------------------------------------------------*/
void
board_reset(void)
{
    unsigned long restart_iac[4];
    extern SAT    system_address_table;
    extern void reinit();

    /* The processor priority should be 31 here; may need to check and set it */
    restart_iac[0] = 0x93000000;        /* Type 93 IAC - reinit processor */
    restart_iac[1] = (unsigned long)&system_address_table;
    restart_iac[2] = (unsigned long)&rom_prcb;
    restart_iac[3] = (unsigned long)reinit;

    send_iac (0, restart_iac);            /* Reinitialize the processor */

    /* SHOULDN'T GET HERE */
    fatal_error(4,(int)"Board_reset",0,0,0);
}


void
board_go_user()
{
}

void
board_exit_user()
{
}
