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
/* cyt_intr.c - Test interrupt handling routines for CVME964 */
/*****************************************************************************
 *
 * Revision history:
 *   7/08/94, Geoff Hickey - modified for C145/145 SX/KX processor modules
 *       5/25/94, Scott Coulter - Modified for C145/146
 *      11/23/93, Scott Coulter - changed interrupt handling
 *       5/27/93, Greg Ames - Modified for the CVME964
 *      11/13/92, Brian Bailey - Written for the C120
 *
 *****************************************************************************/

#include "common.h"
#include "this_hw.h"
#include "cyt_intr.h"
#include "i960.h"

/*
 * Table for handling local interrupt sources.  This table contains
 * an entry for each local interrupt source that is signalled on the
 * XINT pins.
 */

typedef struct irq_local_source
{
    int         enabled;
    void        (*handler)();
}
IRQ_LOCAL_SRC;

static volatile IRQ_LOCAL_SRC irq_local_srcs[IRQ_MAX_LOCAL_SRCS];

extern PRCB *get_prcbptr(void); /* test_asm.s */
extern void interrupt_register_write(void *); /* kx.s */
extern void interrupt_register_read(void *);
extern void _uart_wrapper(void);        /* test_asm.s */
extern void _cio_wrapper(void);         /* test_asm.s */
extern void _sq_irq1_wrapper(void);     /* test_asm.s */
extern void _sq_irq0_wrapper(void);     /* test_asm.s */
extern void _parallel_wrapper(void);    /* test_asm.s */
extern void _pci_wrapper(void);         /* test_asm.s */

extern void lserr_isr(void);
extern void deadlock_isr(void);

extern void set_imap();
/*
 * This macros call the user-installed handler if one is present
 */
#define CALL_LOCAL_HANDLER(intr_src) do { \
		if (irq_local_srcs[intr_src].enabled && \
		    irq_local_srcs[intr_src].handler != 0x0) \
			(*irq_local_srcs[intr_src].handler)(); } while (0)

/*
 * _sq_irq1_handler()
 *
 *      Top-level interrupt handler for all squall II, irq #1 interrupts.
 *      Called by _sq_irq1_wrapper, an assembly language routine that's
 *      inserted into the 960's interrupt table.
 *
 *      This routine calls the user handler for the interrupt.  It does
 *      not clear the IPND bit, as we have no way to know if this is an
 *      edge or level triggered interrupt.  The user routine is
 *      responsible for clearing the IPND bit, if necessary.
 */
void _sq_irq1_handler()
{
    CALL_LOCAL_HANDLER(IRQ_SQ_IRQ1);
}


/*
 * _sq_irq0_handler()
 *
 *      Top-level interrupt handler for all squall II, irq #0 interrupts.
 *      Called by _sq_irq0_wrapper, an assembly language routine that's
 *      inserted into the 960's interrupt table.
 *
 *      This routine calls the user handler for the interrupt.  It does
 *      not clear the IPND bit, as we have no way to know if this is an
 *      edge or level triggered interrupt.  The user routine is
 *      responsible for clearing the IPND bit, if necessary.
 */

/* Note that this handler is obsolete since the SQIRQ0 handler is
   hardcoded in cyt_asm.s to be the i596 Ethernet handler as this
   is the only Squall tested under MON960 */

void _sq_irq0_handler()
{
    CALL_LOCAL_HANDLER(IRQ_SQ_IRQ0);
}


/*
 * install_local_handler()
 *
 * This routine inserts the address of the interrupt handler into the
 * irq_local_srcs array.
 *
 */
void install_local_handler (intr_src, handler)
int     intr_src;
void    (*handler)();
{

    irq_local_srcs[intr_src].handler = handler;
}


#if KXSX_CPU
/*
 * _uart_handler()
 *
 *      Top-level interrupt handler for all UART interrupts.  Called by
 *      _uart_wrapper, an assembly language routine that's inserted into
 *      the 960's interrupt table.
 *
 *      This routine calls the user handler for the interrupt and clears
 *      IPND bit.
 */
void _uart_handler()
{
    CALL_LOCAL_HANDLER(IRQ_UART);
}


/*
 * _cio_handler()
 *
 *      Top-level interrupt handler for all CIO interrupts.  Called by
 *      _cio_wrapper, an assembly language routine that's inserted into
 *      the 960's interrupt table.
 *
 *      This routine calls the user handler for the interrupt and clears
 *      IPND bit.
 */
void _cio_handler()
{
    CALL_LOCAL_HANDLER(IRQ_CIO);

}


/*
 * _parallel_handler()
 *
 *      Top-level interrupt handler for the Parallel Port interrupts.  Called by
 *      _parallel_wrapper, an assembly language routine that's inserted into
 *      the 960's interrupt table.
 *
 *      This routine calls the user handler for the interrupt and clears
 *      IPND bit.
 */
void _parallel_handler()
{
    CALL_LOCAL_HANDLER(IRQ_PARALLEL);
}


/*
 * _pci_handler()
 *
 *      Top-level interrupt handler for all PCI interrupts.  Called by
 *      _pci_wrapper, an assembly language routine that's inserted into
 *      the 960's interrupt table.
 *
 *      This routine calls the user handler for the interrupt and clears
 *      IPND bit.
 */
void _pci_handler()
{
    CALL_LOCAL_HANDLER(IRQ_PCI);
}



/*
 * init_intr()
 *
 *      Initialize interrupts, program interrupt control registers.
 */
void 
init_intr(void)
{

    int i;                      /* Loop variable */
    PRCB *prcb_ptr;
    INTERRUPT_TABLE *int_table_ptr;
	unsigned int intr_control;                      /* Interrupt control
register value */


    /* Mark all local sources as disabled */
    for (i = 0; i < IRQ_MAX_LOCAL_SRCS; i++)
    {
	irq_local_srcs[i].enabled = FALSE;
	irq_local_srcs[i].handler = NULL;
    }

    /* Get address of PRCB and interrupt table */
    prcb_ptr = get_prcbptr();
    int_table_ptr = prcb_ptr->interrupt_table_adr;

	/* Now set up the interrupt control register with vectors for the 4 */
	/* interrupt sources. */
	intr_control = ((VECTOR_SQ_IRQ0 << (8 * XINT_SQ_IRQ0))  |
					(VECTOR_PCI     << (8 * XINT_PCI))              |
					(VECTOR_UART    << (8 * XINT_UART))             |
					(VECTOR_CIO     << (8 * XINT_CIO)));

	/* And set the interrupt control register with the new value */
	interrupt_register_write ((void *)&intr_control);

    /*
     * Insert assembly wrapper routines into interrupt table.  Use
     * vector-8 is offset, to take into account the fact that the
     * pending_interrupts array is really vectors 0-7, and vector
     * 8 is at entry 0 in the interrupt_proc array.  This really
     * should be handled as a union containing the two fields.
     */
    int_table_ptr->interrupt_proc[VECTOR_UART - 8]       = _uart_wrapper;
    int_table_ptr->interrupt_proc[VECTOR_CIO - 8]        = _cio_wrapper;
    int_table_ptr->interrupt_proc[VECTOR_SQ_IRQ0 - 8]    = _sq_irq0_wrapper;
    int_table_ptr->interrupt_proc[VECTOR_PCI - 8]        = _pci_wrapper;
}


/*
 * enable_intr()
 *
 *      For all interrupts, set the appropriate bit in the handler table.
 */
void enable_intr (intr_src)
int     intr_src;
{

    switch (intr_src)
    {
    /*
     * Enable a local interrupt
     */
    case IRQ_UART:
	/*
	 * Mark the interrupt enabled in the table.  This assumes that a
	 * handler has already been installed.
	 */
	irq_local_srcs[intr_src].enabled = TRUE;
	break;

    case IRQ_CIO:
	irq_local_srcs[intr_src].enabled = TRUE;
	break;

/*
    case IRQ_SQ_IRQ1:
	irq_local_srcs[intr_src].enabled = TRUE;
	break;
*/

    case IRQ_SQ_IRQ0:
	irq_local_srcs[intr_src].enabled = TRUE;
	break;

/*
    case IRQ_PARALLEL:
	irq_local_srcs[intr_src].enabled = TRUE;
	break;
*/

    case IRQ_PCI:
	irq_local_srcs[intr_src].enabled = TRUE;
	break;

    default:
	break;
    } /* switch (intr_src) */


} /* end enable_intr */


/*
 * disable_intr()
 *
 *      Disable an interrupt.
 *      For all interrupts, clear the appropriate bit in the handler table.
 *      For CIO, UART, Parallel, PCI, and Squall ints.
 *  Note:  Since there is no IMSK register on the SX or KX processors, we
 *  can't actually turn off an interrupt without disabling it at its source.
 *  Therefore if an interrupt is disabled here and the source continues to
 *  send interrupts, the CPU will get awfully busy, since the interrupt will
 *  never be cleared.
 */
void disable_intr (intr_src)
int     intr_src;
{

    switch (intr_src)
    {
    /*
     * Disable a local interrupt
     */
    case IRQ_UART:
	irq_local_srcs[intr_src].enabled = FALSE;
	break;

    case IRQ_CIO:
	irq_local_srcs[intr_src].enabled = FALSE;
	break;

/*
    case IRQ_SQ_IRQ1:
	irq_local_srcs[intr_src].enabled = FALSE;
	break;
*/

    case IRQ_SQ_IRQ0:
	irq_local_srcs[intr_src].enabled = FALSE;
	break;

/*
    case IRQ_PARALLEL:
	irq_local_srcs[intr_src].enabled = FALSE;
	break;
*/

    case IRQ_PCI:
	irq_local_srcs[intr_src].enabled = FALSE;
	break;

    default:
	break;
    } /* switch (intr_src) */

} /* end disable_intr */
#endif /* KXSX */

#if CXHXJX_CPU
extern void clear_pending(), send_sysctl(), enable_interrupt(), disable_interrupt();
/*
 * _uart_handler()
 *
 *      Top-level interrupt handler for all UART interrupts.  Called by
 *      _uart_wrapper, an assembly language routine that's inserted into
 *      the 960's interrupt table.
 *
 *      This routine calls the user handler for the interrupt and clears
 *      IPND bit.
 */
void _uart_handler()
{
    CALL_LOCAL_HANDLER(IRQ_UART);

    clear_pending (XINT_UART);
}


/*
 * _cio_handler()
 *
 *      Top-level interrupt handler for all CIO interrupts.  Called by
 *      _cio_wrapper, an assembly language routine that's inserted into
 *      the 960's interrupt table.
 *
 *      This routine calls the user handler for the interrupt and clears
 *      IPND bit.
 */
void _cio_handler()
{
    CALL_LOCAL_HANDLER(IRQ_CIO);

    clear_pending (XINT_CIO);
}


/*
 * _parallel_handler()
 *
 *      Top-level interrupt handler for the Parallel Port interrupts.  Called by
 *      _parallel_wrapper, an assembly language routine that's inserted into
 *      the 960's interrupt table.
 *
 *      This routine calls the user handler for the interrupt and clears
 *      IPND bit.
 */
void _parallel_handler()
{
    CALL_LOCAL_HANDLER(IRQ_PARALLEL);

    clear_pending (XINT_PARALLEL);
}


/*
 * _pci_handler()
 *
 *      Top-level interrupt handler for all PCI interrupts.  Called by
 *      _pci_wrapper, an assembly language routine that's inserted into
 *      the 960's interrupt table.
 *
 *      This routine calls the user handler for the interrupt and clears
 *      IPND bit.
 */
void _pci_handler()
{
    CALL_LOCAL_HANDLER(IRQ_PCI);

    clear_pending (XINT_PCI);
}


/*
 * init_intr()
 *
 *      Initialize interrupts at the SCV64.  Program the IPL's for the SCV64
 *      local interrupt sources.
 */
void
init_intr(void)
{

    int i;                      /* Loop variable */
    PRCB *prcb_ptr;
    INTERRUPT_TABLE *int_table_ptr;
    unsigned long imap0, imap1;


    /* Mark all local sources as disabled */
    for (i = 0; i < IRQ_MAX_LOCAL_SRCS; i++)
    {
	irq_local_srcs[i].enabled = FALSE;
	irq_local_srcs[i].handler = NULL;
    }

    /* Get address of PRCB, interrupt table, and control table */
    prcb_ptr = get_prcbptr();
    int_table_ptr = prcb_ptr->interrupt_table_adr;

    /* Set up IMAP1 - XINT's 4-7 */
    imap1 =	IMAP_XINT(XINT_UART,VECTOR_UART) |
	 	IMAP_XINT(XINT_CIO,VECTOR_CIO) |
	 	IMAP_XINT(XINT_SQ_IRQ1,VECTOR_SQ_IRQ1) |
	 	IMAP_XINT(XINT_SQ_IRQ0,VECTOR_SQ_IRQ0);

    /* Set up IMAP0 - XINT's 0-3 */
    imap0 = 	IMAP_XINT(XINT_PARALLEL,VECTOR_PARALLEL) |
		IMAP_XINT(XINT_DEADLOCK,VECTOR_DEADLOCK) |
		IMAP_XINT(XINT_LSERR,VECTOR_LSERR) |
	 	IMAP_XINT(XINT_PCI,VECTOR_PCI);

    set_imap (0,imap0);		/* set IMAP0 */
    set_imap (1,imap1);		/* set IMAP1 */

    /*
     * Insert assembly wrapper routines into interrupt table.  Use
     * vector-8 is offset, to take into account the fact that the
     * pending_interrupts array is really vectors 0-7, and vector
     * 8 is at entry 0 in the interrupt_proc array.  This really
     * should be handled as a union containing the two fields.
     */
    int_table_ptr->interrupt_proc[VECTOR_UART - 8]       = _uart_wrapper;
    int_table_ptr->interrupt_proc[VECTOR_CIO - 8]        = _cio_wrapper;
    int_table_ptr->interrupt_proc[VECTOR_SQ_IRQ1 - 8]    = _sq_irq1_wrapper;
    int_table_ptr->interrupt_proc[VECTOR_SQ_IRQ0 - 8]    = _sq_irq0_wrapper;
    int_table_ptr->interrupt_proc[VECTOR_PARALLEL - 8]   = _parallel_wrapper;
    int_table_ptr->interrupt_proc[VECTOR_PCI - 8]        = _pci_wrapper;
    int_table_ptr->interrupt_proc[VECTOR_DEADLOCK - 8]   = deadlock_isr;
    int_table_ptr->interrupt_proc[VECTOR_LSERR - 8]      = lserr_isr;
}


/*
 * enable_intr()
 *
 *      For all interrupts, set the appropriate bit in the handler table.
 */
void enable_intr (intr_src)
int     intr_src;
{

    switch (intr_src)
    {
    /*
     * Enable a local interrupt
     */
    case IRQ_UART:
	/*
	 * Mark the interrupt enabled in the table.  This assumes that a
	 * handler has already been installed.
	 */
	irq_local_srcs[intr_src].enabled = TRUE;
	enable_interrupt (XINT_UART);
	break;

    case IRQ_CIO:
	irq_local_srcs[intr_src].enabled = TRUE;
	enable_interrupt (XINT_CIO);
	break;

    case IRQ_SQ_IRQ1:
	irq_local_srcs[intr_src].enabled = TRUE;
	enable_interrupt (XINT_SQ_IRQ1);
	break;

    case IRQ_SQ_IRQ0:
	irq_local_srcs[intr_src].enabled = TRUE;
	enable_interrupt (XINT_SQ_IRQ0);
	break;

    case IRQ_PARALLEL:
	irq_local_srcs[intr_src].enabled = TRUE;
	enable_interrupt (XINT_PARALLEL);
	break;

    case IRQ_PCI:
	irq_local_srcs[intr_src].enabled = TRUE;
	enable_interrupt (XINT_PCI);
	break;

    default:
	break;
    } /* switch (intr_src) */


} /* end enable_intr */


/*
 * disable_intr()
 *
 *      Disable an interrupt.
 *      For all interrupts, clear the appropriate bit in the handler table.
 *      For CIO, UART, Parallel, PCI, and Squall ints, clear the mask bit in
 *      the CPU's IMSK register.
 */
void disable_intr (intr_src)
int     intr_src;
{

    switch (intr_src)
    {
    /*
     * Disable a local interrupt
     */
    case IRQ_UART:
	disable_interrupt (XINT_UART);
	irq_local_srcs[intr_src].enabled = FALSE;
	break;

    case IRQ_CIO:
	disable_interrupt (XINT_CIO);
	irq_local_srcs[intr_src].enabled = FALSE;
	break;

    case IRQ_SQ_IRQ1:
	disable_interrupt (XINT_SQ_IRQ1);
	irq_local_srcs[intr_src].enabled = FALSE;
	break;

    case IRQ_SQ_IRQ0:
	disable_interrupt (XINT_SQ_IRQ0);
	irq_local_srcs[intr_src].enabled = FALSE;
	break;

    case IRQ_PARALLEL:
	disable_interrupt (XINT_PARALLEL);
	irq_local_srcs[intr_src].enabled = FALSE;
	break;

    case IRQ_PCI:
	disable_interrupt (XINT_PCI);
	irq_local_srcs[intr_src].enabled = FALSE;
	break;

    default:
	break;
    } /* switch (intr_src) */

} /* end disable_intr */
#endif /* CX/HX/JX */
