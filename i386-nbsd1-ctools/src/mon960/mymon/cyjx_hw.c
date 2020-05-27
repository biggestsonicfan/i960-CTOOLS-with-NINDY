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
#include "this_hw.h"
#include "cyc9060.h"
#include "hdi_arch.h"

extern void set_lmar0(), set_lmmr0();
extern void cpu_identify(void);
extern void init_cio();
extern void init_squall();
extern void pci_hw_init();
extern void init_eeprom();      /* sets up info about Flash Banks */
extern int test_uart();

extern struct PRCB rom_prcb;

/************  C145/C146 CX BOARD-SPECIFIC CONSTANTS AND DATA  ***************/
/*********************************************************************
 * board identifier
 *********************************************************************/
int arch = ARCH_JX;            /* Processor type */
char arch_name[32] = "JX";     /* add speed and mem size in cpu_identify() */

#if BIG_ENDIAN_CODE
    char target_common_name[] = "cyjxbe";/* Informal target name */
#else
    char target_common_name[] = "cyjx";     /* Informal target name */
#endif

const CONTROL_TABLE boot_control_table = {
    /* -- Group 0 -- Breakpoint Registers (reserved by monitor) */
    {0, 0, 0, 0,

    /* -- Group 1 -- Interrupt Map Registers */
    0, 0, 0,        /* Interrupt Map Regs (set by code as needed) */
    0x4020,         /* ICON - dedicated mode, level detected (except
                     * Deadlock, XINT3), enabled,
                     * mask unchanged, not cached, fast */


    /* -- Groups 2-5 -- Bus Configuration Registers */
    REGION_0_CONFIG, 0,
    REGION_2_CONFIG, 0,
    REGION_4_CONFIG, 0,
    REGION_6_CONFIG, 0,
    REGION_8_CONFIG, 0,
    REGION_A_CONFIG, 0,
    REGION_C_CONFIG, 0,
    REGION_E_CONFIG, 0,

    /* -- Group 6 -- Breakpoint, Trace and Bus Control Registers */
    0,                      /* Reserved */
    0,                      /* BPCON Register (reserved by monitor) */
    0,                      /* Trace Controls  */
    REGION_TABLE_VALID(1)}  /* BCON Register  */
};


/*-------------------------------------------------------------
 * Function:      void init_hardware()
 *
 * Action:        Sets up target control hardware and peripherals.
 *-------------------------------------------------------------*/
void
init_hardware()
{
    unsigned char leds;

    /* set data cache on for A memory region, DRAM */
    set_lmar0(0xa0000002);
    set_lmmr0(0xf0000001);
    enable_dcache();

    init_cio();             /* Initialize the Z8536 CIO */

    /* must init CIO first! */
    cpu_identify();         /* modify CPU string to include clock speed */

    init_eeprom();          /* establish operational parameters for
                               the various banks of Flash ROM */

    init_squall();          /* Get Squall Module data */
        
    if (test_uart() == OK)    /* Run an internal loopback test on the uart */
    {
       leds = get_led_value();
       set_led_value(leds | LED_2);    /* uart test passed */
    }

    set_mask(0);            /* reset all interrupts */  
    set_pending(0);

    pci_hw_init();             /* Set up PCI i960 Bridge */
    register_set[REG_IMSK] |= (1 << XINT_PCI) | (1 << XINT_LSERR) | (1 << XINT_DEADLOCK);

    leds = get_led_value();
    set_led_value(leds | LED_3);     /* hardware init complete */
    pause(); /* Let user look at diag LEDs before erased by mon960 boot code. */
}


/* Set up the interrupt map register to use the appropriate vector. */
void
init_imap_reg(PRCB *prcb)
{
    unsigned int BRK_PRIO, BRKIV=0, BRK_PIN=0;
    
    if (serial_comm() == TRUE)
        {
        BRKIV = VECTOR_UART;
        BRK_PIN = XINT_UART;
        }
    else if (pci_comm() == TRUE)
        {
        BRKIV = VECTOR_PCI;
        BRK_PIN = XINT_PCI;
        }
    else 
        return;

    if (BRK_PIN < 4)
       set_imap(0, (get_imap(0) & ~(0xf << BRK_PIN*4)) | ((BRKIV>>4) << BRK_PIN*4));
    else
       set_imap(1, (get_imap(1) & ~(0xf << (BRK_PIN-4)*4)) | ((BRKIV>>4) << (BRK_PIN-4)*4));

/* allow LSERR interrupts to be serviced */
    BRK_PRIO = ((BRKIV >> 3) - 1);
    if (BRK_PRIO > ((VECTOR_LSERR >> 3) - 1))
        BRK_PRIO = ((VECTOR_LSERR >> 3) - 1);

    if (get_mon_priority() > BRK_PRIO)
        set_mon_priority(BRK_PRIO);

    enable_interrupt(BRK_PIN);
}


/*-------------------------------------------------------------
 * Function:    void board_reset(void)
 *
 * Action:      Software board reset;
 *              Mask interrupts, reload the rom-based prcb, and
 *              reinitialize the CPU.
 *              This function does not return.
 *-------------------------------------------------------------*/
void
board_reset(void)
{
    extern void reinit();

    set_mask(0);         /* Reset all interrupts */
    set_pending(0);
    asm("ld _boot_g0, g11");
    send_sysctl(0x300,(unsigned long) reinit,(unsigned long) &rom_prcb);

    /* SHOULDN'T GET HERE */
    fatal_error(4,(int)"Board_reset",0,0,0);
}


void
board_go_user()
{
    if (serial_comm() == TRUE)
        register_set[REG_IMSK] |= 1 << XINT_UART;
    else if (pci_comm() == TRUE)
        register_set[REG_IMSK] |= 1 << XINT_PCI;
}

void
board_exit_user()
{
}
