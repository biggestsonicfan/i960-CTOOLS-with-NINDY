/*cb*/
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
#include "cyc9060.h"
#include "c145_eep.h"
#include "16552.h"

extern int strcpy(), strcat();
extern int dram_size;
extern unsigned char inreg();

volatile PCI_CONFIG_SPACE *PCI_CFG = (PCI_CONFIG_SPACE *)PCI9060_BASE_ADDR;

int is_new_baseboard();
int get_cyclone_revision();
void init_console(int baud);

char board_name[sizeof("Cyclone PCI, Rev. X")];
#if KXSX_CPU
    ADDR unwritable_addr = 0x0;         /* Boot rom locn */
#else
    ADDR unwritable_addr = 0xf0000000;  /* Boot rom locn */
#endif
unsigned int __cpu_speed;
unsigned int __cpu_bus_speed;

/* global which holds the silicon rev. of the PLX 9060 */
int plx_revision = 0;


/*-------------------------------------------------------------
 * Function:      static void init_cio()
 *
 * Action:        Sets up CIO's bit ports, used for board
 *                control.
 *
 *          If the board is Rev. C or later, then the CIO ports
 *          are set up to allow user-programmable LED writes on
 *          Port C and to read the revision code from the upper
 *          nibble of Port B.
 *-------------------------------------------------------------*/
void
init_cio()
{
/***************************************************************
*
* DO NOT use a software reset command on the CIO.  The software
* reset command causes the clock speed selects (FREQ0, FREQ1, and
* FREQ2) to glitch causing unpredictable results in the Clock
* PLL device.
*
*/
    static unsigned char cmds[] = {
        /*    reg#    value                    */
        /*    -----    -----                    */
        /*    Unreset CIO                    */
        0x00,   0x00,    /* Clear reset bit,
                   disable interrupts        */

        /*    Disable ports A, B, and C            */
        0x01,    0x00,    /* Mast Conf Ctrl -disable ports*/

        /*    Configure CIO port A - Processor Module options    */
        0x20,    0x00,    /* Mode spec - bit port        */
        0x21,    0x00,    /* Handshake spec - not used    */
        0x22,    0x00,    /* Polarity - all bits non-inverting*/
        0x23,    0xff,    /* Direction - all bits inputs    */
        0x24,    0x00,    /* Spec i/o ctrl - normal inputs*/
        0x25,    0x00,    /* Pattern matching registers    */
        0x26,    0x00,    /* Pattern matching not used    */
        0x27,    0x00,

        /*    Configure CIO port B - Serial EEPROM's clk/data    */
        0x28,    0x00,    /* Mode spec - bit port        */
        0x29,    0x00,    /* Handshake spec - not used    */
        0x2a,    0x00,    /* Polarity - no inverting    */
        0x2b,    0xc0,    /* Dir - unused bits inputs    */
        0x2c,    0x2a,    /* Data lines open drain    */
        0x2d,    0x00,    /* Pattern matching registers    */
        0x2e,    0x00,    /* Pattern matching not used    */
        0x2f,    0x00,
        0x0e,    0xea,    /* Data reg - SCL's low, let    */
                /* data lines float high.    */

        /*    Configure CIO port C - Misc control bits    */
        0x06,    0x07,    /* Data dir - bit3 out, 0-2 in    */
        0x0f,    0x00,    /* Data reg - pull RECONFIG low    */

        /*    Enable ports A, B, and C            */
        0x01,    0x94    /* Mast Conf Ctrl - Enable ports*/
    };

    int i;
    volatile unsigned char junk;

    if (is_new_baseboard() == FALSE)
    {
        /* Reset CIO state machine */
        junk = CIO->ctrl;    /* Put CIO in reset state or state 0 */
        CIO->ctrl = 0;    /* Put CIO state 0 or 1 */
        junk = CIO->ctrl;    /* Put CIO in state 0 */

        for ( i = 0; i < sizeof(cmds); i++ ){
        CIO->ctrl = cmds[i];
        }
    }
    else
    {
        /* do nothing, the CIO is intialized in c145_asm.s */
    }
    return;
}

/*-----------------------------------------------------------------
 * Function:      static void init_squall()
 *
 * Action:        Checks to see if the squall module is installed
 *                and sets up MCON and interrupt info if so.
 *
 * Note:      init_cio() must be called before this routine, so
 *          that data port B may be used to access the eeprom!
 *-----------------------------------------------------------------*/
void
init_squall()
{
    SQUALL_EEPROM_DATA squall_data;

#if KXSX_CPU   
    if (eeprom_read (SQUALL_EEPROM, GENERIC_DATA_OFFSET,
             (void *) &squall_data, GENERIC_DATA_SIZE) != OK)
     {
     /* nothing to do for KX/SX chips at present */ /* self test ???*/
     }
#endif


#if JX_CPU
    unsigned long icon;
        unsigned long cx_mcon;
        unsigned long squall_pmcon = BUS_WIDTH(32);

    /* read current value of the ICON */
    icon = *(unsigned long *)ICON_ADDR;


    /*
     * Try reading configuration info from squall eeprom.
     * If read fails, no squall module installed, just return
     */
    if (eeprom_read (SQUALL_EEPROM, GENERIC_DATA_OFFSET,
             (void *) &squall_data, GENERIC_DATA_SIZE) != OK)
        /* don't do anything, just return */
        return;
    else
        {
            /* Read Cx-style MCON word from EEPROM
             * Jx processor only requires the regions bus width */
            cx_mcon =  squall_data.mcon_byte_0 |
                      (squall_data.mcon_byte_1 << 8) |
                      (squall_data.mcon_byte_2 << 16) |
                      (squall_data.mcon_byte_3 << 24);
 
            switch (cx_mcon & CX_BUS_WIDTH)
            {
                case CX_BUS_8:
                    squall_pmcon = BUS_WIDTH(8);
                    break;
 
                case CX_BUS_16:
                    squall_pmcon = BUS_WIDTH(16);
                    break;
 
                case CX_BUS_32:
                default:
                    squall_pmcon = BUS_WIDTH(32);
                    break;
            }
            /* set value of the PMCON12 (for region C) */
            *(unsigned long *)PMCON12_ADDR = squall_pmcon;


        /* Clear detection mode bits in ICON */
        icon &= ~(EDGE_DETECT(XINT_SQ_IRQ0,1) |
            EDGE_DETECT(XINT_SQ_IRQ1,1));

        /* Set ICON bits according to value in eeprom */
        icon |=
          EDGE_DETECT(XINT_SQ_IRQ0,(squall_data.intr_det_mode & IRQ_0_MASK)
== IRQ_0_FALLING_EDGE);

        icon |=
          EDGE_DETECT(XINT_SQ_IRQ1,(squall_data.intr_det_mode & IRQ_1_MASK)
== IRQ_1_FALLING_EDGE);
        }

    /* Store the new ICON */
    *(unsigned long *)ICON_ADDR = icon;

    return;
#endif

#if CXHX_CPU
    CONTROL_TABLE *control_tab = prcb_ptr->cntrl_table_adr;
    CX_MCON cx_mcon;

#if HX_CPU
    HX_MCON hx_mcon;

    /* clear all bits in both unions */
    cx_mcon.mcon = 0;
    hx_mcon.mcon = 0;
#endif

    /*
     * Try reading configuration info from squall eeprom.
     * If read fails, no squall module installed, set MCON
     * word to NO_SQUALL.
     */
    if (eeprom_read (SQUALL_EEPROM, GENERIC_DATA_OFFSET,
             (void *) &squall_data, GENERIC_DATA_SIZE) != OK)
    	{
#if HX_CPU
    	hx_mcon.bits.bus_width = 0x2;	/* 32 bit bus */
    	hx_mcon.bits.ready_enab = 1;	/* Ready enabled */

        /* set value of the PMCON12 (for region C) */
        *(unsigned long *)PMCON12_ADDR = hx_mcon.mcon;

        control_tab->control_reg[MCON12] = hx_mcon.mcon;
#else
        control_tab->control_reg[MCON12] = NO_SQUALL;
#endif
    	}
    else
        {
        cx_mcon.mcon     = squall_data.mcon_byte_0 |
                          (squall_data.mcon_byte_1 << 8) |
                          (squall_data.mcon_byte_2 << 16) |
                          (squall_data.mcon_byte_3 << 24);
#if HX_CPU
    	hx_mcon.bits.nrad = cx_mcon.bits.nrad;
    	hx_mcon.bits.nrdd = cx_mcon.bits.nrdd;
    	hx_mcon.bits.nwad = cx_mcon.bits.nwad;
    	hx_mcon.bits.nwdd = cx_mcon.bits.nwdd;
    	hx_mcon.bits.nxda = cx_mcon.bits.nxda;	/* not a direct cross Hx has
                        						   more bits */
    	hx_mcon.bits.bus_width = cx_mcon.bits.bus_width;
    	hx_mcon.bits.pipeline_enab = cx_mcon.bits.pipeline_enab;
    	hx_mcon.bits.burst_enab = cx_mcon.bits.burst_enab;
    	hx_mcon.bits.ready_enab = cx_mcon.bits.ready_enab;

        /* Set MCON word in control table */
        control_tab->control_reg[MCON12] = hx_mcon.mcon;

        /* set value of the PMCON12 (for region C) */
        *(unsigned long *)PMCON12_ADDR = hx_mcon.mcon;
#else /*CX_CPU*/
        /* Set MCON word in control table */
        control_tab->control_reg[MCON12] = cx_mcon.mcon;
#endif

        /* Clear detection mode bits in ICON */
        control_tab->control_reg[ICON] &=
            ~(EDGE_DETECT(XINT_SQ_IRQ0,1) | EDGE_DETECT(XINT_SQ_IRQ1,1));

        /* Set ICON bits according to value in eeprom */
        control_tab->control_reg[ICON] |=
          EDGE_DETECT(XINT_SQ_IRQ0,(squall_data.intr_det_mode & IRQ_0_MASK) == IRQ_0_FALLING_EDGE);
        control_tab->control_reg[ICON] |=
          EDGE_DETECT(XINT_SQ_IRQ1,(squall_data.intr_det_mode & IRQ_1_MASK) == IRQ_1_FALLING_EDGE);

#if HX_CPU
    	/* Store the new ICON */
    	*(unsigned long *)ICON_ADDR = control_tab->control_reg[ICON];
#endif
        }

#if CX_CPU
    /* Re-load the modified sections of the control table into the CPU */
    send_sysctl(0x401, 0, 0);    /* Reload ICON */
    send_sysctl(0x405, 0, 0);    /* Reload MCON 12-15 */
#endif
    return;
#endif
}

/*-------------------------------------------------------------
 * Function:    void cpu_identify(void)
 *
 * Action:      Modify the arch_name string to include the actual 
 *              clock frequency the CPU is running at.
 *        Note that the macros which obtain the processor type
 *        and clock frequency rely on the cio being already
 *        initialized
 *-------------------------------------------------------------*/
void
cpu_identify(void)
{
    int speed;
    unsigned char buffer[20];
    int rev;
    char rev_char;

    speed = GET_CLK_FREQ();

    switch (speed)
    {
    case FREQ_4_MHZ:
        strcat(arch_name, " at 4 MHz");
        __cpu_speed = 4;
        break;
    case FREQ_8_MHZ:
        strcat(arch_name, " at 8 MHz");
        __cpu_speed = 8;
        break;
    case FREQ_16_MHZ:
        strcat(arch_name, " at 16 MHz");
        __cpu_speed = 16;
        break;
    case FREQ_20_MHZ:
        strcat(arch_name, " at 20 MHz");
        __cpu_speed = 20;
        break;
    case FREQ_25_MHZ:
        strcat(arch_name, " at 25 MHz");
        __cpu_speed = 25;
        break;
    case FREQ_33_MHZ:
        strcat(arch_name, " at 33 MHz");
        __cpu_speed = 33;
        break;
    case FREQ_40_MHZ:
        strcat(arch_name, " at 40 MHz");
        __cpu_speed = 40;
        break;
    case FREQ_50_MHZ:
        strcat(arch_name, " at 50 MHz");
        __cpu_speed = 50;
        break;
    default:
        strcat(arch_name, " at ?? MHz");
        __cpu_speed = 33;   /* best guess */
        break;
    }

    __cpu_bus_speed = __cpu_speed;

    sprtf(buffer, 20," with%dMB DRAM", (char)(dram_size >> 20),0,0,0);
    strcat(arch_name, buffer);

    rev = get_cyclone_revision();
    rev_char = 'B' + rev;        /* Rev. 0 == Rev. B, ... */

    if (PCI_INSTALLED() == TRUE)
    {
        sprtf(board_name, 30, "Cyclone PCI, Rev. %c", rev_char,0,0,0);
    }
    else
    {
        sprtf(board_name, 30, "Cyclone  EP, Rev. %c", rev_char,0,0,0);
    }
     
    return;
}


#if CXHXJX_CPU
/********************************************************/
/* LSERR_ISR                        */
/* This routine responds to pci LSERR interrupts    */
/********************************************************/
void
_pci_lserr_isr ()
{
    if (read_long(PCI_STATUS) & RCVD_MASTER_ABORT)
        {
        CLEAR_MASTER_ABORT();
        }
    if (read_long(PCI_STATUS) & RCVD_TARGET_ABORT)
        {
        CLEAR_TARGET_ABORT();
        }
    if (read_long(PCI_STATUS) & PCI_DATA_PARITY_ERR)
        {
        CLEAR_DATA_PARITY_ERROR();
        }
    if (read_long(PCI_STATUS) & PCI_BUS_PARITY_ERR)
        {
        CLEAR_BUS_PARITY_ERROR();
        }

    clear_pending(XINT_LSERR);
    blink(15);
}
#endif /*CXHXJX*/



#if JX_CPU
/********************************************************
 * DEADLOCK_ISR                      
 * This routine responds to pci deadlock interrupts
 * reception of this interrupt indicates that the CPU
 * has been preempted from the local bus due to a 
 * deadlock condition between the PCI bridge device 
 * and the local processor.  On the Cx and Hx processor,
 * the processor's backoff pin is used. On the Jx,
 * an artificial READY is returned to the processor
 * and the interrupt is asserted indicating the non-
 * recoverable system error.  The deadlock isr will delay
 * an application for a noticable period of time and if
 * the target's LEDs are accessible, give visual feedback
 * as well.  The delay and visual feedback are important
 * because the application has just experienced a bad
 * event, as the following e-mail from Cyclone explains.
 *
 *   "Here's the lowdown on the Deadlock resolution on
 *   the Jx processor module.  The Jx processor, unlike
 *   the Cx processor, does not support Backoff
 *   capability wherein the processor can be forced to
 *   suspend an ongoing bus cycle so that another bus
 *   master (the PLX in this case) can gain local bus
 *   ownership.  Therefore, to prevent a lockup on both
 *   the Jx processor and the PCI bus master, a PCI
 *   deadlock condition on the PCI80960-Jx is 'resolved'
 *   by having external logic generate a READY to the
 *   processor along with a Deadlock Interrupt.  Since
 *   the READY causes the processor to complete its
 *   ongoing bus transaction, the Deadlock is 'broken'. 
 *   However, since the READY is generated without valid
 *   data associated with it, the processor Read or Write
 *   which is in progress completes with no valid data
 *   transferred (on a Read, the data latched by the
 *   processor would be garbage, and on a Write, no real
 *   data is transferred to the target location).  For
 *   these reasons, a PCI deadlock interrupt signals a
 *   non-recoverable condition since the user would have
 *   no idea that the bus transaction was, in fact,
 *   bogus."
 *
 ********************************************************/

void
_pci_deadlock_isr ()
{
    int i;

    for (i=15; i>0; i--)    blink(i);
}
#endif

/********************************************************/
/* PCI_ISR                        */
/* This routine responds to pci interrupts        */
/* - the only interrupts which are expected are Doorbell*/
/*   and DMA interrupts.                */
/********************************************************/
static void
_pci_isr ()
{
    unsigned long stat, temp;

    stat = read_long (PCI_INT_CSTAT);

    /* check for Local Doorbell Interrupts */
    if (stat & LOCAL_DOORBELL_INT)
    {
        temp = read_long (PTOL_DOORBELL);   /* read the register */
        if (temp != 0)                       /* not a doorbell interrupt ?? */
        {
           /* clear doorbell bit to clear int. */
           write_long (PTOL_DOORBELL, temp);
        }
    }
#if CXHXJX_CPU
    clear_pending(XINT_PCI);
#endif
}


/************************************************/
/* ENABLE_PCI_INTS                */
/* This routine enables pci interrupts        */
/*     - Local Interrrupts            */
/*     - Doorbell Interrupts            */
/*     - DMA Channel 0 Interrupts        */
/*     - DMA Channel 1 Interrupts        */
/*    - Deadlock Interrupts (Jx only)        */
/************************************************/
static
void
pci_enable_ints ()
{
    unsigned long temp;
    PRCB *prcb_ptr;
    INTERRUPT_TABLE *int_table_ptr;
#if CXHXJX_CPU
    extern void lserr_wrapper();
    unsigned long imap0;
#endif
#if JX_CPU
    extern void deadlock_wrapper();
#endif
    
    temp = read_long (PTOL_DOORBELL);      /* read the register */
    if (temp != 0)              /* any bits set */
        write_long (PTOL_DOORBELL, temp);
    /* clear doorbell bit to clear int. */

    /* turn on local interrupts incl. doorbell and DMA */
    write_long (PCI_INT_CSTAT, read_long(PCI_INT_CSTAT) |
            (LOCAL_INT_ENABLE |  PCI_INT_ENABLE |
                 LOCAL_DOORBELL_ENABLE | PCI_DOORBELL_ENABLE));

    /* Get address of PRCB and interrupt table */
    prcb_ptr = get_prcbptr();
    int_table_ptr = prcb_ptr->interrupt_table_adr;
    /*
     * Insert assembly wrapper routines into interrupt table.  Use
     * vector-8 is offset, to take into account the fact that the
     * pending_interrupts array is really vectors 0-7, and vector
     * 8 is at entry 0 in the interrupt_proc array.  This really
     * should be handled as a union containing the two fields.
     */
    int_table_ptr->interrupt_proc[VECTOR_PCI - 8] = _pci_isr;

#if CXHXJX_CPU
    /* Set up IMAP0 - XINT's 0-3 */
    imap0 = IMAP_XINT(XINT_LSERR,VECTOR_LSERR) |
            IMAP_XINT(XINT_PCI,VECTOR_PCI);
#if JX_CPU
    imap0 |= IMAP_XINT(XINT_DEADLOCK,VECTOR_DEADLOCK);
    int_table_ptr->interrupt_proc[VECTOR_DEADLOCK - 8]   = deadlock_wrapper;
#endif

    set_imap (0,imap0);        /* set IMAP0 */

    int_table_ptr->interrupt_proc[VECTOR_LSERR - 8]      = lserr_wrapper;
	/* turn on LSERR interrupt for PCI Aborts */
	write_long (PCI_INT_CSTAT, read_long(PCI_INT_CSTAT) | (LSERR_INT_ENABLE));

#if JX_CPU
    enable_interrupt(XINT_DEADLOCK);
#endif
    enable_interrupt(XINT_LSERR);
    enable_interrupt(XINT_PCI);
#endif
}


/************************************************/
/* INIT_PCI                    */
/* This routine initializes pci registers.    */
/* This routine assumes no initial state of the */
/* PCI device and therefore, no settings are    */
/* preserved across this function call.        */
/*                        */
/* the routines write_long and read_long    */
/* are used when reading and writing 32 bit    */
/* quantities for compatibility with Sx code    */
/************************************************/
void
pci_hw_init ()
{
    unsigned short temp;
    unsigned long mem_size;

    if (PCI_INSTALLED() == FALSE)
    return;

    /* figure out how much system memory is available for PCI Bus access */
    mem_size = dram_size + MEMBASE_DRAM;

    /* set up slave information for PCI->local accesses */
    switch (mem_size)
    {
    case DRAM_32MEG:
        write_long (RANGE_PTOL_0, RANGE_DRAM_32);
        write_long (LOCAL_BASE_PTOL_0, BASE_DRAM);
        break;

    case DRAM_8MEG:
        write_long (RANGE_PTOL_0, RANGE_DRAM_8);
        write_long (LOCAL_BASE_PTOL_0, BASE_DRAM);
        break;

    case DRAM_2MEG:
    default:
        write_long (RANGE_PTOL_0, RANGE_DRAM_2);
        write_long (LOCAL_BASE_PTOL_0, BASE_DRAM);
        break;
    }

    /* use the Local Expansion Flash ROM as PCI Expansion ROM
       0xe0000000 - 0xefffffff
       Note: used in conjunction with the PCI Expansion ROM Base Register */
    write_long (RANGE_PTOL_ROM, 0x00000000);    /* NO FLASH ! */

/*   BREQo must be enabled to detect the deadlock condition */
    write_long (BREQO_CONTROL, (ROM_REMAP_ADDR | DEADLOCK_TIMEOUT |
                BREQO_ENABLE));

    /* set up the Region 0 Descriptor for PCI->local accesses */
    write_long (BUS_REGION_DESC_PTOL, (MEM_BUS_32BIT | MEM_USE_RDY_INPUT |
                ROM_BUS_8BIT | ROM_USE_RDY_INPUT |
                MEM_BURST_ENABLE | ROM_BURST_DISABLE |
                NO_TRDY_WHEN_TXFULL | RETRY_TIMEOUT));

    /* set up master information for local->PCI accesses
          - the PCI Bus will be accessible from local address
      0x40000000 - 0x7fffffff for a range of 1 GByte */

    /* set up the Local Range for Direct Master to PCI */
    write_long (RANGE_MTOP, 0xc0000000);    /* decode A30 & A31 */

    /* set up the Local Base Address for Direct Master to PCI Memory */
    write_long (LOCAL_BASE_MTOPM, 0x40000000);

    /* set up the Local Base Address for Direct Master to PCI I/O CFG */
    write_long (LOCAL_BASE_MTOPI, NO_IO_SPACE);

    /* set up PCI Base Address (Re-map) for Direct Master to PCI */
    write_long (PCI_BASE_MTOP, (PCI_MEM_MASTER_ENAB | PCI_IO_MASTER_DISAB |
             PCI_LOCK_ENAB | PCI_PREFETCH_DISAB |
             PCI_RELEASE_FIFO_FULL | PCI_REMAP_ADDR));

    /* set up PCI Configuration Address for Direct Master to PCI I/O CFG */
    write_long (PCI_CONFIG_ADDR_REG, PCI_CONFIG_DISAB);


    /* set up the Command Codes used for PCI access */
    write_long (PCI_EEPROM_CTL, (DMA_READ_CODE | DMA_WRITE_CODE |
              MASTER_READ_CODE | MASTER_WRITE_CODE));

    write_long (PCI_INT_CSTAT, MAX_RETRIES_256);

    /* set up the necessary information in Config. Space Registers */

    /* set up Board Identifiers in Config. Space */
#if BIG_ENDIAN_CODE
    temp = VENDOR_CYCLONE;
    swap_data_byte_order(2, &temp, 2);
    PCI_CFG->vendor_id = temp;
    temp = PCI80960;
    swap_data_byte_order(2, &temp, 2);
    PCI_CFG->device_id = temp;
#else   
    PCI_CFG->vendor_id = VENDOR_CYCLONE;
    PCI_CFG->device_id = PCI80960;
#endif   
    /* in the Revision Register, store the board Revision in the
       upper nibble and leave the PLX silicon Revision in the lower
       nibble - this gives us access to both values */
    plx_revision = (PCI_CFG->revision_id & 0x0f); /* save silicon rev. */
    PCI_CFG->revision_id |=
        (unsigned char)((get_cyclone_revision() << 4) & 0xf0);
    PCI_CFG->base_class = BASE_CLASS;
    PCI_CFG->sub_class = SUB_CLASS;

    /* Device does not support Built-In-Self-Test (BIST) */
    PCI_CFG->bist = BIST_NOT_SUPPORTED;

    /* Set Enable/Disable bit for Expansion ROM address decode */
    /* Note: change to ENABLE to allow the use of local Expansion
       Flash ROM as PCI Expansion ROM */
    write_long (&PCI_CFG->pcibase_exp_rom, ROM_DECODE_DISABLE);

    /* indicate to the PCI world that the local initialization is complete
       and that RETRY responses to PCI accesses can be stopped */
    write_long (PCI_EEPROM_CTL, (read_long (PCI_EEPROM_CTL) | (1 << 31)));

    /* wait for host init;  done when Master Enable bit is set in the
       PCI Command register in Config space */
    temp = 0;
    while (!(temp & PCI_CMD_MASTER_ENAB))
        {
        temp = PCI_CFG->command;
#if BIG_ENDIAN_CODE 
        swap_data_byte_order(2, &temp, 2);
#endif
        }

    pci_enable_ints();
    return;
}


/******************************************************************************
*
* is_new_baseboard - return TRUE if baseboard is Rev. C or later
*
* The Ring Indicator pin (observable through the UART) is pulled up on Rev.
* A or B boards and is grounded on Rev. C or later boards.
*/
int is_new_baseboard (void)
{
    unsigned char msr;
    /* check to see if the Ring Indicator pin is low or high.  The RI
       pin is an active low signal so that the status bit will be set ony
       when the pin is low */
    msr = inreg(MSR);
    if (msr & MSR_RI)
    {
    return TRUE;    /* Rev. C or later Baseboard */
    }
    else
    return FALSE;    /* a Rev. A or B Baseboard */
}

/******************************************************************************
*
* get_cyclone_revision - get the revision of the Cyclone Baseboard
*
* The possible return values are:
*
*    0         - indicating a Rev. A or B board
*     non-zero     - indicating a newer version (version = return val)
*
* The Ring Indicator pin (observable through the UART) is pulled up on Rev.
* A or B boards and is grounded on Rev. C or later boards.  On Rev. C or
* later boards, the actual revision number can be read through port B of the
* CIO. The revision numbers start at one.
*/
int get_cyclone_revision (void)
{
    unsigned char revision;

    if (is_new_baseboard() == FALSE)
    return (0);
    else
    {
    revision = ((CIO->bdata & 0xf0) >> 4);
    return (revision);
    }
}

/******************************************************************************
*
* PCI_INSTALLED - returns TRUE if the baseboard is equipped with a PCI bridge
*
* On Rev. B boards, the pci_install indication is on CIO port C
* On Rev. C or later boards,
*
* Note: this routine should not be called until after the CIO has been
*       initialized.
*
* Also Note: this routine is capitalized like a #define'd macro because
*         in previous versions it was a #define'd function!
*/
int PCI_INSTALLED (void)
{
    if (is_new_baseboard() == FALSE)
    {
    if (CIO->cdata & 0x04)
        return TRUE;
    else
        return FALSE;
    }
    else
    {
    if (CIO->adata & 0x80)
        return TRUE;
    else
        return FALSE;
    }
}

/******************************************************************************
*
* write_long     - write a 32bit longword
* read_long     - read a 32bit longword
*
* These routines are used to provide a common interface to the PCI bridge
* device and to PCI memory.  They are required because an Sx processor must read and write 
* 32 bit quantities using two bus cycles while the Jx, Kx, and Cx processors
* require only one bus cycle.  On an Sx a 32 bit bus transaction would actually require
* a burst transfer of two 16 bit shortwords.
*
*/

#if SX_CPU 
void write_long (
        volatile unsigned long *rptr,
        unsigned long data
        )
{
    unsigned short upper, lower;
    volatile unsigned short *uptr, *lptr;

    upper = (unsigned short)((data & 0xffff0000) >> 16);
    lower = (unsigned short)(data & 0x0000ffff);

    uptr = (volatile unsigned short *)((unsigned long)rptr + 2);
    lptr = (volatile unsigned short *)rptr;

    /* write the data; least sig. half word first, most sig. half word next */
    *lptr = lower;
    *uptr = upper;
}

unsigned long read_long (volatile unsigned long *rptr)
{
    unsigned short upper, lower;
    volatile unsigned short *uptr, *lptr;
    unsigned long result;
 
    uptr = (volatile unsigned short *)((unsigned long)rptr + 2);
    lptr = (volatile unsigned short *)rptr;
 
    /* read the data; least sig. half word first, most sig. half word next */
    lower = *lptr;
    upper = *uptr;
 
    result = (unsigned long)((upper << 16) | (lower));
    return (result);
}

#else         /* Jx, HX, Cx, Kx */
void write_long (
        volatile unsigned long *rptr,
        unsigned long data
        )
{
#if BIG_ENDIAN_CODE 
    unsigned long temp = data;
    
    swap_data_byte_order(4, &temp, 4);
    *rptr = temp;
#else   
    *rptr = data;
#endif
}

unsigned long read_long (volatile unsigned long *rptr)
{
#if BIG_ENDIAN_CODE 
    unsigned long temp;
    
    temp = *rptr;
    swap_data_byte_order(4, &temp, 4);
    return(temp);
#else   
    return (*rptr);
#endif      
}
#endif /*CXHXKXJX*/


/***************************************************************************
*
* test_uart - do an internal loopback test on the UART
*
*/
#define CHAR_TIMEOUT    15000    /* number of rcv checks before
                   declaring that no char. was rcvd. */
int test_uart(void)
{
    char out = 'a';
    char in = 0;
    int i;
    volatile int timeout;

    /* initialize and put into loopback mode */
    serial_init();        /* initialize uart */
    serial_set(9600);        /* set test baud rate */
    serial_loopback(TRUE);    /* enable internal loopback */

    /* transmit and receive the alphabet... */
    for (i = 0; i<26; i++)
    {
    serial_putc(out);
    
    /* wait for receive to happen... */
    timeout = CHAR_TIMEOUT;
    in = serial_getc();
    while ((in == -1) && (timeout > 0))
    {
        in = serial_getc(); /* look for the character */
        timeout--;        /* decrement timeout */
    }

    if (timeout == 0)    /* never got character */
        return (ERROR);
    else            /* compare the rcvd. char */
        if (in != out) return (ERROR);
    out++;
    }
    serial_loopback(FALSE);    /* disable internal loopback */
    serial_init();        /* clear junk */
    return (OK);
}



/***************************************************************************
 *
 * set_led_value - sets up the leds to display the given 4 bit value
 *
 *
 * Note - this routine should only be called after the CIO is intialized
 */
void 
set_led_value(unsigned char val)
{
    /* only the Rev. C or later baseboards have LEDs */
    if (is_new_baseboard() == TRUE)
    	CIO->cdata = val & 0x0f;	/* only 4 leds (bitmapped) */
}



/***************************************************************************
 *
 * get_led_value - gets the led register 4 bit value
 *
 *
 * Note - this routine should only be called after the CIO is intialized
 */
unsigned char 
get_led_value(void)
{
    /* only the Rev. C or later baseboards have LEDs */
    if (is_new_baseboard() == TRUE)
    	return (CIO->cdata & 0x0f);	/* only 4 leds (bitmapped) */
    else
    	return (0);
}


/* Return the break interrupt vector number to the monitor.  */
int
get_int_vector(int dummy)
{
    if (serial_comm() == TRUE)
        return(VECTOR_UART);
    else if (pci_comm() == TRUE)
        return(VECTOR_PCI);
    else
        return ERR;
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
    int flag = FALSE;

    if (serial_comm() == TRUE)
        {
        flag = serial_intr();
        clear_pending (XINT_UART);
        }
    else if (pci_comm() == TRUE)
        {
        flag = pci_intr();
        clear_pending (XINT_PCI);
        }

    return(flag);
}


/*
 * Function to report actual bus freq * 1000.  On a Cyclone baseboard,
 * none of the user-selectable clock frequencies are exact integer
 * multiples in the MHz range.  Normally, this is no big deal...but it
 * matters a lot when using the CPU timers for to implement bentime
 * (the benchmark timer).
 */
unsigned long
timer_extended_speed(void)
{
    unsigned int speed;

    switch (__cpu_bus_speed)
    {
        case 16:
            speed = 16110;
            break;
        case 20:
            speed = 20050;
            break;
        case 25:
            speed = 25060;
            break;
        case 33:
            speed = 33410;
            break;
        case 40:
            speed = 40090;
            break;
        case 50:
            speed = 50110;
            break;
        default:
            speed = __cpu_bus_speed * 1000;
            break;
    }
    return (speed);
}
