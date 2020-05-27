/******************************************************************/
/* 		Copyright (c) 1991, Intel Corporation

   Intel hereby grants you permission to copy, modify, and 
   distribute this software and its documentation.  Intel grants
   this permission provided that the above copyright notice 
   appears in all copies and that both the copyright notice and
   this permission notice appear in supporting documentation.  In
   addition, Intel grants this permission provided that you
   prominently mark as not part of the original any modifications
   made to this software or documentation, and that the name of 
   Intel Corporation not be used in advertising or publicity 
   pertaining to distribution of the software or the documentation 
   without specific, written prior permission.  

   Intel Corporation does not warrant, guarantee or make any 
   representations regarding the use of, or the results of the use
   of, the software and documentation in terms of correctness, 
   accuracy, reliability, currentness, or otherwise; and you rely
   on the software, documentation and results solely at your own 
   risk.							  */
/******************************************************************/

#include "this_hw.h"
#include "cyt_intr.h"
#include "cyt_pci.h"
#include "cyc9060.h"

void clear_pending();

extern void hex32out();

extern void enable_interrupt();
extern void sgets (char *);

typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef unsigned char UCHAR;

/*
 * Internal routines
 */
static void		disable_pci_ints(void);
static void		enable_pci_ints(void);
void 			print_test_results(void);

/* exportable */
extern void		pci_isr(void);
extern void		deadlock_isr(void);
extern void		lserr_isr(void);

void prtf();
long hexIn();
int sysBusTas();
int AtomicModify();

extern int memTest();
extern int testDMA();
extern int ShortAddr();
extern int ShortBar();
extern int ByteAddr();
extern int ByteBar();
extern void dumpMem();

extern PCI_CONFIG_SPACE *PCI_CFG;

extern int plx_revision;
int plx_screened = FALSE;       /* boolean, if TRUE indicates that the PLX
                                   has been screened to ensure that the
                                   PCI Direct Master Read function works,
                                   if FALSE indicates that PCI Direct Master
                                   Reads should not be attempted */

/*
 * Boolean flags to indicate a pci interrupts
 */
volatile int pci_bist;
volatile int pci_doorbell;
volatile int pci_dma0;
volatile int pci_dma1;
volatile int pci_deadlock;

/* Semaphore and Count variable for TAS Test */
unsigned long count_sem;
unsigned long tas_count;
unsigned long in_the_house[2];

/* The PCI Test Summary Structure */
PCI_TEST_RESULTS test_results;

extern quadtest();

int Rendezvous_with_RAMA (void)
{
    /* tell the PC to run its tests */
    write_long (LTOP_MBOX7, PC_CAN_CONTINUE);
 
    /* wait for the host PC to do its part */
    while ((read_long (PTOL_MBOX0) != PCI_TEST_CONTINUE) &&
	   (read_long (PTOL_MBOX0) != PCI_HAS_ABORTED))
        ;

    if (read_long (PTOL_MBOX0) == PCI_TEST_CONTINUE)
    {
    	write_long (PTOL_MBOX0, 0);	/* clear our message box */
	return OK;
    }
    if (read_long (PTOL_MBOX0) == PCI_HAS_ABORTED)
    {
    	write_long (PTOL_MBOX0, 0);	/* clear our message box */
	return ERROR;
    }
}

/*******************************************************************************
*
* mbox_test - perform a walking ones test on the given mailbox
*
*/
static
int
mbox_test (unsigned long *mbox)
{
    unsigned long       out, in,errors;
    int                 bitpos;
    volatile int        junk;
 
    junk = (int) &junk; /* Don't let compiler optimize or "registerize" */
 
    errors = 0;
    prtf("PCI-9060 PCI Mailbox Test on MBOX at 0x%X...\n",mbox);
    for (bitpos = 0; bitpos < 32; bitpos++)
    {
        out = 1 << bitpos;
 
        write_long (mbox, out);      /* Write data */
        junk = ~0;                      /* Force data lines high */
        in = read_long (mbox);
 
        /* make sure it's what we wrote */
        if (in != out)
        {
            prtf("PCI Mailbox Test Error: Expected 0x%08X Actual 0x%08X\n",
                        out,in);
            errors++;
        }
    }
    if (errors)
        return (0);
    else
        return (1);
}


/************************************************/
/* DISABLE_PCI_INTS				*/
/* This routine disables pci interrupts		*/
/************************************************/
static
void
disable_pci_ints ()
{
    disable_intr (IRQ_PCI);		/* Shut off interrupt at CPU */

    /* Shut off interrupts at PCI-9060 */
    write_long (PCI_INT_CSTAT, read_long(PCI_INT_CSTAT) &
		~(LOCAL_INT_ENABLE | DMA_CH0_ENABLE | DMA_CH1_ENABLE |
                  LOCAL_DOORBELL_ENABLE));

}

/************************************************/
/* ENABLE_PCI_INTS				*/
/* This routine enables pci interrupts		*/
/* 	- Local Interrrupts			*/
/* 	- Doorbell Interrupts			*/
/* 	- DMA Channel 0 Interrupts		*/
/* 	- DMA Channel 1 Interrupts		*/
/*	- Deadlock Interrupts (Jx only)		*/
/************************************************/
static
void
enable_pci_ints ()
{
    unsigned long temp;

    disable_pci_ints();
    install_local_handler (IRQ_PCI, pci_isr);

    /* clear any junk hanging around */
    CLR_DMA_CH0();
    CLR_DMA_CH1();
    CLR_BIST_INT();
    temp = read_long (PTOL_DOORBELL);  	/* read the register */
    if (temp != 0)          	/* any bits set */
    	write_long (PTOL_DOORBELL, temp);
	/* clear doorbell bit to clear int. */

    /* turn on local interrupts incl. doorbell and DMA */
    write_long (PCI_INT_CSTAT, read_long(PCI_INT_CSTAT) |
			(LOCAL_INT_ENABLE | LSERR_INT_ENABLE | PCI_INT_ENABLE |
		     	DMA_CH0_INT_ENABLE | DMA_CH1_INT_ENABLE |
		     	LOCAL_DOORBELL_ENABLE | PCI_DOORBELL_ENABLE));

    enable_intr(IRQ_PCI);

#if JX_CPU
    enable_interrupt(XINT_DEADLOCK);
#endif /*JX*/
}


/********************************************************/
/* LSERR_ISR						*/
/* This routine responds to pci LSERR interrupts	*/
/********************************************************/
void
lserr_isr ()
{
    prtf("\n\nPCI LSERR Interrupt Received\n");
    if (*PCI_STATUS & RCVD_MASTER_ABORT)
    {
	prtf("PCI Master Abort Received!\n");
    	CLEAR_MASTER_ABORT();
    }
    if (*PCI_STATUS & RCVD_TARGET_ABORT)
    {
	prtf("PCI Target Abort Received!\n");
    	CLEAR_TARGET_ABORT();
    }
    if (*PCI_STATUS & PCI_DATA_PARITY_ERR)
    {
	prtf("PCI Data Parity Error Received!\n");
    	CLEAR_DATA_PARITY_ERROR();
    }
    if (*PCI_STATUS & PCI_BUS_PARITY_ERR)
    {
	prtf("PCI Bus Parity Error Received!\n");
    	CLEAR_BUS_PARITY_ERROR();
    }
}

/********************************************************/
/* DEADLOCK_ISR						*/
/* This routine responds to pci deadlock interrupts	*/
/* reception of this interrupt indicates that the CPU	*/
/* has been preempted from the local bus due to a	*/
/* deadlock condition between the PCI bridge device	*/
/* and the local processor.  On the Cx and Hx processor,*/
/* the processor's backoff pin is used. On the Jx,	*/
/* an artificial READY is returned to the processor	*/
/* and the interrupt is asserted indicating the non-	*/
/* recoverable system error.				*/
/********************************************************/
void
deadlock_isr ()
{
    prtf("\nPCI Deadlock Interrupt Received\n");
    pci_deadlock++;
}

/********************************************************/
/* PCI_ISR						*/
/* This routine responds to pci interrupts		*/
/* - the only interrupts which are expected are Doorbell*/
/*   and DMA interrupts.				*/
/********************************************************/
void
pci_isr ()
{
    unsigned long stat, temp;

    stat = read_long (PCI_INT_CSTAT);

    /* check for Local Doorbell Interrupts */
    if (stat & LOCAL_DOORBELL_INT)
    {
        temp = read_long (PTOL_DOORBELL);   /* read the register */
    	if (temp != 0)		               /* not a doorbell interrupt ?? */
	{
	    /* clear doorbell bit to clear int. */
    	    write_long (PTOL_DOORBELL, temp);
	    pci_doorbell = TRUE;
	}
    }
    /* check for DMA Channel 0 Interrupts */
    if (stat & DMA_CH0_INT)
    {
	temp = read_long (DMA_CMD_STAT);
	if (temp & DMA0_DONE)
	{
	    CLR_DMA_CH0();
	    pci_dma0 = TRUE;
	}
    }
    /* check for DMA Channel 1 Interrupts */
    if (stat & DMA_CH1_INT)
    {
	temp = read_long (DMA_CMD_STAT);
	if (temp & DMA1_DONE)
	{
	    CLR_DMA_CH1();
	    pci_dma1 = TRUE;
	}
    }
    if (stat & BIST_INT)
    {
	CLR_BIST_INT();
	pci_bist = TRUE;
    }
}

/******************************************************************************
*
* sysBusTas - C callable interface to byte test-and-set.
*
* This routine provides a C callable interface to the 80960
* atmod (atomic modify) instruction.  It does a 680x0-style
* "test-and-set" operation on the byte at the specified
* address.
*
* RETURNS: TRUE if the value was not set (but now is),
*          or FALSE if it was already set.
*/
int sysBusTas (adrs)
    char *adrs;                         /* Address to probe */
{
    unsigned long mask;                 /* Mask of bits to tas */
    unsigned long value;                /* New bit value */
    int byteOffset;                     /* offset in word of byte to tas */
 
    mask = 0x000000ff;                  /* Start at LSB */
    value = 0xffffffff;                 /* Change all unmasked bits */
    byteOffset = (unsigned long) adrs & 0x3;    /* Get byte offset in word */
 
    mask <<= 8 * byteOffset;            /* shift mask to byte to change */
 
    /* since AtomicModify returns the value stored in the memory location
       obtained during the Read portion of the R-M-W cycle, if the the value
       is clear, then the lock will succeed, otherwise the location was
       already locked */
    if ((AtomicModify (value, mask, adrs) & mask) == 0)
        return (TRUE);
    else
        return (FALSE);
}

/*******************************************************************************
*
* TasTest - test-and-set test
*
* This routine, in conjunction with another TasTest() task (running on the PC),
* tests the test-and-set mechanism.  Both tasks 
* spin on the semaphore pointed to by <semaphore> using Atomic cycles for
* mutual exclusion.  When this routine gets the semaphore, it increments
* the <count> if it is ODD.  It then enters a busy-wait loop
* <delay> times, giving the other task a chance to take the semaphore.
* The PC (other task) increments the counter when the count value is EVEN.
*
* This test should be run twice; once with the semaphore in EP80960 DRAM and
* once with the semaphore in PC System Memory.
*
* RETURNS: when count reaches terminal value (OK) or if the count
*	   stalls at some value or if can't get semaphore (ERROR)
*/
 
int TasTest
    (
    char *semaphore,	/* semaphore to spin on */
    ULONG *count,	/* counter address */
    ULONG delay		/* delay so other task gets time */
    )
{
    ULONG	busyWait;
    int		semFailures, countFailures;
    ULONG	*PC_in_the_house;
    ULONG	*Cyclone_in_the_house;
 
    PC_in_the_house = (ULONG *)((ULONG)count + 4);
    Cyclone_in_the_house = (ULONG *)((ULONG)count + 8);

    /* clear count, gives us a way to terminate the test when count
       reaches some terminal value */
    *count = 0;
    semFailures = 0;
    countFailures = 0;

#if 0
    /* wait for everything to get started... */
    for (busyWait = 0; busyWait < 0xffffff; busyWait++)
	;
#endif
 
    while (*count < MAX_COUNT)
    {
        if (sysBusTas (semaphore))
        {
	    /* before doing anything, check to see if the PC task
	       also thinks he has the semaphore */
	    if (*PC_in_the_house)
	    {
		prtf("Failed.\n");
		prtf("Mutual Exclusion Not Enforced!\n");
		prtf("Count Value at 0x%x\n\n", *count);
		return(ERROR);
	    }

	    *Cyclone_in_the_house = 0xffffffff;
    	    semFailures = 0;		/* clear semaphore error counter */
            /* check if count is ODD and bump counter if so */
            if ((*count) & 1)
	    {
                (*count) ++;
		countFailures = 0;	/* clear counter error counter */
	    }
	    else
	    {
	    	if (++countFailures > COUNT_STUCK_LIMIT)
	    	{
		    prtf("Failed\n");
		    prtf("\nTasTest: count is stalled at 0x%x!\n",*count);
            	    *semaphore = 0;
		    return(ERROR);
	    	}
	    }
	    /* before giving back semaphore, check to see if the PC task
	       also thinks he has the semaphore */
	    if (*PC_in_the_house)
	    {
		prtf("Failed.\n");
		prtf("Mutual Exclusion Not Enforced!\n");
		prtf("Count Value at 0x%x\n\n", *count);
		return(ERROR);
	    }
	    *Cyclone_in_the_house = 0;
            *semaphore = 0;
 
            /* enter busy-wait loop to give other task a chance */
            for (busyWait = 0; busyWait < delay; busyWait++)
                ;                               /* wait */
        }
	else
	{
	    if (++semFailures > SEM_LOCKED_LIMIT)
	    {
		prtf("Failed\n");
		prtf("\nTasTest: can't get semaphore!\n");
		return(ERROR);
	    }
	}
    }
    prtf("Passed\n");
    return(OK);
}

/******************************************************************************
*
* plx_pci_test - Test the functionality of the PLX PCI-9060 Bridge chip.
*
* Note:	full functionality of the PCI interface is verified using both
*	sofware running locally on the i960 and remotely on the host PC.
*
* PCI-9060 DIAGNOSTIC TEST SUITE
*
* - Perform a Bus Test on the PCI device (walking one's through mailbox registers)
*
* - Test PCI interrupts by interrupting ourselves using the doorbell register
*
* - Remote PC Software will then test the following:
*	Read the PCI-9060 Configuration header information
*	Perform a Bus Test on a PCI device register
*	Send the type of CPU to the PC in mailbox because the Sx and Kx
*		processors only run a subset of the Cx/Jx tests
*	Write a "Test Continue" message to a PCI-9060 mailbox to indicate
*	to the local diagnostics that they may continue testing.
*
* - Perform a Memory Test on the PCI-accessible memory of the PC (including
*   a walking one's test, address test, read-modify-write test, etc.)
*
* - Test the PCI-9060 DMA in both directions and verify that DMA interrupts
*   function properly.
*
* - Prepare to receive a doorbell interrupt from the PC software.
*
* - Remote PC Software will then test the following:
*	Perform a Memory Test on the PCI-accessible memory of the Cyclone card
*	(including a walking one's test, address test, read-modify-write test)
*	Generate an interrupt on the Cyclone card using the doorbell register
*
* - On the Cx and Jx, PC Host to Cyclone deadlock conditions are simulated by
*   having the Cyclone card access the PC Host memory at the same time that
*   the PC Host is accessing memory on the Cyclone card.  In the Jx processor configuration
*   an interrupt is generated during deadlock conditions.  In the Cx processor configuration
*   the Cx processor is backed off the local bus using the BOFF pin.
*
*   In the Cx and Jx processor configurations, a Test-and-Set test is performed.
*   Both the PC and the Cyclone card run a semaphore test to verify
*   the functionality of R-M-W cycles.  The Cyclone board will be designated
*   as the ODD board (increment the semaphore-protected count variable only
*   when its value is odd).  The test will be performed with the semaphore
*   and count value being located in both PC memory and Cyclone memory.
*
*
*   PCI Mailbox Usage:
*
*	PCI->Local Mailbox 0 : used for passing sync. messages from PC
*	PCI->Local Mailbox 1 : used for obtaining sem. pointer from PC
*	PCI->Local Mailbox 2 : used for obtaining count pointer from PC
*
*	Local->PCI Mailbox 7 : used for passing sync. messages to PC
*	Local->PCI Mailbox 5 : used for passing sem. pointer to PC
*	Local->PCI Mailbox 6 : used for passing count pointer to PC
*
*   At each synchronization point:
*	Local Mailbox 0 is cleared
*	The PC is sent a PC_CAN_CONTINUE message in Mailbox 7
*	The PC responds with a PCI_TEST_CONTINUE message in Mailbox 0
*	Extra data is passed using the aforementioned registers.
*	A PC_CAN_CONTINUE message indicates to the PC that valid data is
*	   present in the extra Mailboxes (if necessary)
*	A PCI_HAS_ABORTED message indicates to the PC that the Cyclone test
*	   has been aborted
*	A PCI_TEST_CONTINUE message indicates to the EP80960 that valid data is
*	   present in the extra Mailboxes (if necessary)
*	A PC_HAS_ABORTED message indicates to the Cyclone board that the PC test
*	   has been aborted
*	
*/

void plx_pci_test(void)
{
    volatile int	loop;
    int			looplim;
    unsigned long	*start_p,*end_p,*phys_pci_p;
    int			stats = 0;
    char		answer[20];

#if CXHXJX_CPU
#if 0
    char		*sem_p;
    unsigned long	*count_p;
#endif

#else	/* Sx or Kx */
    extern int onesTest(), LWAddr(), LWBar();

    unsigned long	badAddr;
#endif /*CXHXJX*/
     
    if (PCI_INSTALLED() == FALSE)
    {
	prtf("\n\nCannot Diagnose PCI Interface\n");
	prtf("PLX 9060 Bridge Device Not Installed\n");
	return;
    }

    looplim = 2000000;

    prtf("PLX 9060 PCI Chip Test...\n");
    prtf("PLX Silicon Revison = 0x%B\n", plx_revision);
 
    if (plx_revision < 3)
    {
        prtf("\n\nHas the PLX-9060 been screened? (y/n): ");
        sgets(answer);
        prtf("\n");
        if ((answer[0] == 'y') || (answer[0] == 'Y'))
            plx_screened = TRUE;
        else
            plx_screened = FALSE;
    }


    disable_pci_ints();
    
    if (!mbox_test ((unsigned long *)PTOL_MBOX0))
    {
	prtf ("\nERROR:  mbox_test 0 for PCI9060 failed\n");
	test_results.mbox0_test = TEST_FAILED;
    }
    else
    {
    	prtf ("\nmbox_test 0 for PCI9060 passed\n\n");
	test_results.mbox0_test = TEST_PASSED;
    }

    if (!mbox_test ((unsigned long *)PTOL_MBOX1))
    {
	prtf ("\nERROR:  mbox_test 1 for PCI9060 failed\n");
	test_results.mbox1_test = TEST_FAILED;
    }
    else
    {
    	prtf ("\nmbox_test 1 for PCI9060 passed\n\n");
	test_results.mbox1_test = TEST_PASSED;
    }

    if (!mbox_test ((unsigned long *)PTOL_MBOX2))
    {
	prtf ("\nERROR:  mbox_test 2 for PCI9060 failed\n");
	test_results.mbox2_test = TEST_FAILED;
    }
    else
    {
    	prtf ("\nmbox_test 2 for PCI9060 passed\n\n");
	test_results.mbox2_test = TEST_PASSED;
    }

    if (!mbox_test ((unsigned long *)PTOL_MBOX3))
    {
	prtf ("\nERROR:  mbox_test 3 for PCI9060 failed\n");
	test_results.mbox3_test = TEST_FAILED;
    }
    else
    {
    	prtf ("\nmbox_test 3 for PCI9060 passed\n\n");
	test_results.mbox3_test = TEST_PASSED;
    }

    if (!mbox_test ((unsigned long *)LTOP_MBOX4))
    {
	prtf ("\nERROR:  mbox_test 4 for PCI9060 failed\n");
	test_results.mbox4_test = TEST_FAILED;
    }
    else
    {
    	prtf ("\nmbox_test 4 for PCI9060 passed\n\n");
	test_results.mbox4_test = TEST_PASSED;
    }

    if (!mbox_test ((unsigned long *)LTOP_MBOX5))
    {
	prtf ("\nERROR:  mbox_test 5 for PCI9060 failed\n");
	test_results.mbox5_test = TEST_FAILED;
    }
    else
    {
    	prtf ("\nmbox_test 5 for PCI9060 passed\n\n");
	test_results.mbox5_test = TEST_PASSED;
    }

    if (!mbox_test ((unsigned long *)LTOP_MBOX6))
    {
	prtf ("\nERROR:  mbox_test 6 for PCI9060 failed\n");
	test_results.mbox6_test = TEST_FAILED;
    }
    else
    {
    	prtf ("\nmbox_test 6 for PCI9060 passed\n\n");
	test_results.mbox6_test = TEST_PASSED;
    }

    if (!mbox_test ((unsigned long *)LTOP_MBOX7))
    {
	prtf ("\nERROR:  mbox_test 7 for PCI9060 failed\n");
	test_results.mbox7_test = TEST_FAILED;
    }
    else
    {
    	prtf ("\nmbox_test 7 for PCI9060 passed\n\n");
	test_results.mbox7_test = TEST_PASSED;
    }

    /* test local PCI interrupts by using our BIST interrupt */
    pci_bist = FALSE;
    loop = 0;

    prtf ("Testing PCI BIST Interrupt...");
    enable_pci_ints();

    PCI_CFG->bist |= PCI_BIST_INTERRUPT;	/* should generate an int. */

    while ((pci_bist == FALSE) && (loop++ < looplim))
    	;

    if (pci_bist == FALSE)
    {
    	prtf (" Test failed\n");
	test_results.bist_test = TEST_FAILED;
    }
    else
    {
    	prtf (" Test passed\n");
	test_results.bist_test = TEST_PASSED;
    }

    write_long (PTOL_MBOX1, 0);	/* clear start addr. reg. */
    write_long (PTOL_MBOX2, 0);	/* clear end addr. reg. */

#if CXHXJX_CPU
/*  move comments to do Test-and-Set testing 
    write_long (LTOP_MBOX5, CX_JX_HX_PROCESSOR);
*/
    write_long (LTOP_MBOX5, CX_JX_HX_NOTAS);
#else	/* Sx or Kx */
    write_long (LTOP_MBOX5, SX_KX_PROCESSOR);
#endif /*CXHXJX*/

    prtf("Waiting for PC Host to run a bus test on Cyclone board\n");
    if (Rendezvous_with_RAMA() == ERROR)
    {
	prtf("\nPC Testing Has Been Aborted!\n");
	return;
    }


    start_p = (unsigned long *)read_long(PTOL_MBOX1);
    end_p = (unsigned long *)read_long(PTOL_MBOX2);

    /* some PCI addresses are with respect to local addresses where an
       offset of LOCAL_TO_PCI_OFFSET is required.  The DMA needs the 
       actual physical PCI address of Host memory, hence the difference */
    phys_pci_p = (unsigned long *)read_long(PTOL_MBOX3);

    prtf("Memtest start pointer = 0x%x\n", start_p);
    prtf("Memtest end pointer = 0x%x\n", end_p);
    prtf("PCI Physical Addr = 0x%x\n", phys_pci_p);

  /* Direct Master Read errata fixed in Rev. 3
     However,some parts are screened to be Direct Master Read functional */
  if ((plx_revision >= 3) || ((plx_revision < 3) && (plx_screened == TRUE)))
  {
    prtf("Data at Base of PC Host Memory is 0x%x\n", read_long(start_p));

#if CXHXJX_CPU
    prtf("Starting Memory Tests on PC Host Memory...\n\n");

    /* perform a Memory test on the PC's memory */
    if (memTest(start_p, end_p, FALSE) == 0)
    {
	stats++;
    }
    else
	test_results.pci_mem_tests = TEST_PASSED;

    prtf("\n");
 
    prtf("\nRepeating test using read-modify-write cycles...\n");
 
    if (memTest(start_p, end_p, TRUE) == 0)
    {
	stats++;
    }
    else
	test_results.pci_mem_tests = TEST_PASSED;

#else 		/* Sx or Kx */
    prtf("Starting Limited Memory Tests on PC Host Memory...\n\n");

    /* perform a Memory test on the PC's memory, ShortAddr, ShortBar, ByteAddr
       and ByteBar only */

    prtf("\nShort address test: ");
    if (ShortAddr(start_p, end_p, &badAddr) == 1) {
    	prtf("failed");
    	dumpMem(badAddr);
	stats++;
    }
    else
    {
	prtf("passed");
	test_results.pci_mem_tests = TEST_PASSED;
    }
 
    prtf("\nShort address bar test: ");
    if (ShortBar(start_p, end_p, &badAddr) == 1) {
    	prtf("failed");
    	dumpMem(badAddr);
	stats++;
    }
    else
    {
	prtf("passed");
	test_results.pci_mem_tests = TEST_PASSED;
    }

    prtf("\nByte address test: ");
    if (ByteAddr(start_p, end_p, &badAddr) == 1) {
    	prtf("failed");
    	dumpMem(badAddr);
	stats++;
    }
    else
    {
	prtf("passed");
	test_results.pci_mem_tests = TEST_PASSED;
    }
 
    prtf("\nByte address bar test: ");
    if (ByteBar(start_p, end_p, &badAddr) == 1) {
    	prtf("failed");
    	dumpMem(badAddr);
	stats++;
    }
    else
    {
	prtf("passed");
	test_results.pci_mem_tests = TEST_PASSED;
    }

#if KX_CPU
    /* the Kx can do longwords */
    prtf("\n");
    if (onesTest(start_p) == 1)
    {
        prtf("\nWalking 1's test: failed");
	stats++;
    }
    else
    {
    	prtf("\nWalking 1's test: passed");
	test_results.pci_mem_tests = TEST_PASSED;
    }
 
 
    prtf("\nLong word address test: ");
    if (LWAddr(start_p, end_p, &badAddr, 0) == 1) {
        prtf("failed");
        dumpMem(badAddr);
 	stats++;
    }
    else
    {
    	prtf("passed");
    	test_results.pci_mem_tests = TEST_PASSED;
    }
 
    prtf("\nLong word address bar test: ");
    if (LWBar(start_p, end_p, &badAddr, 0) == 1) {
        prtf("failed");
        dumpMem(badAddr);
    	stats++;
    }
    else
    {
    	prtf("passed");
    	test_results.pci_mem_tests = TEST_PASSED;
    }
#endif		/* Kx processor */
#endif		/* Kx/Sx Memory Tests */

    if (stats)
    	test_results.pci_mem_tests = TEST_FAILED;

    prtf ("\n\nPCI to PC Memory test done.\n");

  }     /* PLX revision >= 3 to fix Direct Master Read  */
  else  /* don't run, could hang */
  {
    prtf ("\n\nBypassing PCI Direct Master Memory Tests...\n");
    test_results.pci_mem_tests = TEST_NOTRUN;
  }

    prtf("\nTesting PCI-9060 DMA Channels...\n");
    if (testDMA(phys_pci_p) == 1)
	test_results.pci_dma_tests = TEST_FAILED;    
    else
	test_results.pci_dma_tests = TEST_PASSED;    
    prtf("\n\n");
    
    /* test PCI interrupts by using our doorbell */
    pci_doorbell = FALSE;
    loop = 0;

    /* synchronization point with PC, waiting for 
       Doorbell Test */

    prtf("Waiting for PC Host to run a memory test on Cyclone board\n");

    if (Rendezvous_with_RAMA() == ERROR)
    {
        prtf("\nPC Testing Has Been Aborted!\n");
        return;
    }

    prtf ("Testing PCI DOORBELL INTERRUPT...");

    while ((pci_doorbell == FALSE) && (loop++ < looplim))
    	;

    if (pci_doorbell == FALSE)
    {
    	prtf (" Test failed\n");
	test_results.doorbell_test = TEST_FAILED;
    }
    else
    {
    	prtf (" Test passed\n");
	test_results.doorbell_test = TEST_PASSED;
    }

    if (Rendezvous_with_RAMA() == ERROR)
    {
        prtf("\nPC Testing Has Been Aborted!\n");
        return;
    }

    /* Ring all of the host's doorbells... */
    prtf("Ringing Host's Doorbells...\n");
    write_long (LTOP_DOORBELL, 0xffffffff);

    test_results.local_tas_test = TEST_NOTRUN;
    test_results.remote_tas_test = TEST_NOTRUN;
    if (Rendezvous_with_RAMA() == ERROR)
    {
        prtf("\nPC Testing Has Been Aborted!\n");
        return;
    }

#if 0		/* don't run TAS tests - PC HW doesn't implement */

    /* synchronization point with PC, waiting for 
       Test-and-Set Test
	- test first with semaphore in Cyclone Memory
	- test with semaphore in PC System Memory */

    /* clear all data associated with the TasTest */
    tas_count = 0;
    count_sem = 0;
    in_the_house[0] = 0;
    in_the_house[1] = 0;
    
    /* The PC will need to convert the following addresses to PCI bus 
       addresses */

    write_long (LTOP_MBOX5, (ULONG)&count_sem);
    /* tell the PC where the semaphore is */

    write_long (LTOP_MBOX6, (ULONG)&tas_count);
    /* tell the PC where the count variable is */

    prtf("Waiting for Local TAS Test to start...\n");
    if (Rendezvous_with_RAMA() == ERROR)
    {
        prtf("\nPC Testing Has Been Aborted!\n");
        return;
    }

    prtf ("Semaphore at 0x%X\n", (ULONG)&count_sem);
    prtf ("Count at 0x%X\n", (ULONG)&tas_count);

    prtf ("Testing Semaphore Locking Mechanism (Semaphore is Local)...");
    if (TasTest((char *)&count_sem, (ULONG *)&tas_count, TAS_DELAY) == ERROR)
    {
	prtf ("Local TasTest Failed\n");
	test_results.local_tas_test = TEST_FAILED;
    }
    else
    {
	test_results.local_tas_test = TEST_PASSED;
    }

    prtf("\n\n");

    write_long (PTOL_MBOX1, 0);	/* clear semaphore pointer */
    write_long (PTOL_MBOX2, 0);	/* clear counter pointer */

    /* wait for the host PC to do its part
	- specify the address of the semaphore
	- specify the address of the counter */
    prtf("Waiting for Remote TAS Test to start...\n");
    if (Rendezvous_with_RAMA() == ERROR)
    {
        prtf("\nPC Testing Has Been Aborted!\n");
        return;
    }

    sem_p = (char *)read_long(PTOL_MBOX1);
    count_p = (ULONG *)read_long(PTOL_MBOX2);

    prtf ("Semaphore at 0x%X\n", (ULONG)sem_p);
    prtf ("Count at 0x%X\n", (ULONG)count_p);
    
    prtf ("Testing Semaphore Locking Mechanism (Semaphore is Remote)...");
    if (TasTest(sem_p, count_p, TAS_DELAY) == ERROR)
    {
	prtf ("Remote TasTest Failed\n");
	test_results.remote_tas_test = TEST_FAILED;
    }
    else
    {
	test_results.local_tas_test = TEST_PASSED;
    }

#endif	    /* no TAS testing */

    test_results.local_tas_test = TEST_NOTRUN;
    test_results.remote_tas_test = TEST_NOTRUN;

    disable_pci_ints ();
    prtf ("PCI-9060 tests done.\n");
    print_test_results();
    prtf ("Press return to continue.\n");
    (void) hexIn();
}

void print_test_results(void)
{
    prtf("\n\n");
    prtf("Local PCI Test Results:\n");
    prtf("---------------------------------------------\n\n");
    prtf("PLX Mailbox 0 Test       :    %s", test_results.mbox0_test);
    prtf("PLX Mailbox 1 Test       :    %s", test_results.mbox1_test);
    prtf("PLX Mailbox 2 Test       :    %s", test_results.mbox2_test);
    prtf("PLX Mailbox 3 Test       :    %s", test_results.mbox3_test);
    prtf("PLX Mailbox 4 Test       :    %s", test_results.mbox4_test);
    prtf("PLX Mailbox 5 Test       :    %s", test_results.mbox5_test);
    prtf("PLX Mailbox 6 Test       :    %s", test_results.mbox6_test);
    prtf("PLX Mailbox 7 Test       :    %s", test_results.mbox7_test);
    prtf("PLX BIST Interrupt Test  :    %s", test_results.bist_test);
    prtf("PCI Memory Tests         :    %s", test_results.pci_mem_tests);
    prtf("PCI DMA Tests            :    %s", test_results.pci_dma_tests);
    prtf("PCI Doorbell Test        :    %s", test_results.doorbell_test);
    prtf("Local Test-and-Set Test  :    %s", test_results.local_tas_test);
    prtf("Remote Test-and-Set Test :    %s", test_results.remote_tas_test);
    prtf("\n\n");
}

/*
 * cyt_dma - this  performs an PCI-9060 dma test
 *
 * Written 12 Nov. 1993 by Scott Coulter for the SCV64 VME DMA
 * Modified 27 Sept. 1994 by Scott Coulter for the PCI-9060 PCI DMA
 *
 * Version: @(#)
 */

/*
 * must use write_long() and read_long() since 16 bit burst transfers
 * will not work properly across the PCI bus.
 */

void ClearMem (
	long 	*start,			/* Start of mem to be cleared */
	long	size			/* how much */
	)
{
    register int i;

    for (i=0; i<(size/4); i++)
    {
	write_long (start, 0);		/* clear the block of memory */
	start++;
    }
}
		
void SetMem (
	long 	*start,			/* Start of mem to be set */
	long	*sval,			/* Starting value */
	long	size			/* how much */
	)
{
    register int i;

    for (i=0; i<(size/4); i++)
    {
	write_long (start, (long)sval);
	start++;
	sval++;
    }				/* set the block of memory
				   to give each memory location an
				   address as its data */
}
		
/*
 * Routine to initialize and kick off a PCI-9060 DMA Transfer.
 *
 * Note: this routine starts the DMA so the data must be set up
 *	 prior to call.
 */

void InitDMA	(
	int	unit,		/* DMA 0 or 1 */
	void	*local,		/* pointer to Local DRAM */
	void	*remote,	/* pointer to PCI memory (somewhere??) */
	int	nbytes,		/* number of bytes in the block */
	int	dir		/* READ or WRITE */
	)
{
    /* before starting the DMA prepare for possible interrrupts
	- abort and disable DMAs
	- clear DMA interrupts */

    if (unit)
    {
    	CLR_DMA_CH1();
	pci_dma1 = FALSE;
    }
    else
    {
    	CLR_DMA_CH0();
	pci_dma0 = FALSE;
    }

    write_long (DMA_ARB_REG0, DMA_ARB0_VALUE);
    write_long (DMA_ARB_REG1, DMA_ARB1_VALUE);

    if (unit)
    {
	write_long (DMA_CH1_PADDR, (unsigned long)remote);
	write_long (DMA_CH1_LADDR, (unsigned long)local);
	write_long (DMA_CH1_BCOUNT, nbytes);
	write_long (DMA_CH1_MODE, (LOCAL_BUS_32BIT | USE_RDY_INPUT |
			DMA_BURSTING_ENABLE |
			DMA_CHAINING_DISABLED | DMA_INT_ENDOFTRANSFER));

	if (dir == WRITE)
	    DMA_CH1_WRITE();
	else
	    DMA_CH1_READ();

	ENABLE_DMA_CH1();
	START_DMA_CH1();		/* kick off DMA Transfer */
    }
    else
    {
	write_long (DMA_CH0_PADDR, (unsigned long)remote);
	write_long (DMA_CH0_LADDR, (unsigned long)local);
	write_long (DMA_CH0_BCOUNT, nbytes);
	write_long (DMA_CH0_MODE, (LOCAL_BUS_32BIT | USE_RDY_INPUT |
			DMA_BURSTING_ENABLE |
			DMA_CHAINING_DISABLED | DMA_INT_ENDOFTRANSFER));
	if (dir == WRITE)
	    DMA_CH0_WRITE();
	else
	    DMA_CH0_READ();

	ENABLE_DMA_CH0();
	START_DMA_CH0();		/* kick off DMA Transfer */
    }

    /* Must now wait for the DMA interrupt (or a timeout) */
}

/*
 * Routine to check status after transfer completion.  Called from
 * the routine which has decided that the DMA is done.
 *
 * RETURNS: OK if all is as expected or  TRANSFER_ERROR if there is a problem
 */

int nodump;

int InspectDMA(int unit)
{
    register int status, temp;
    unsigned long done;
    unsigned long abort;

    status = 0;
    temp = read_long (PCI_STATUS);

    if (temp & RCVD_MASTER_ABORT)
    {
	prtf("PCI-9060 Received a PCI Master Abort!\n");
	CLEAR_MASTER_ABORT();
    }

    if (temp & RCVD_TARGET_ABORT)
    {
	prtf("PCI-9060 Received a PCI Target Abort!\n");
	CLEAR_TARGET_ABORT();
    }

    if (temp & SIGNALLED_SYSTEM_ERROR)
    {
	prtf("PCI-9060 Signalled SERR!\n");
	CLEAR_SIG_SERR();
    }

    if (unit)
    {
	done = 1<<12;	/* DMA 1 done bit in status register */
	abort = 1<<26;	/* DMA 1 was bus master during an abort */
    }
    else
    {
	done = 1<<4;	/* DMA 0 done bit in status register */
	abort = 1<<25;	/* DMA 0 was bus master during an abort */
    }
 
    temp = read_long (PCI_INT_CSTAT);
    if (temp & abort)
    {
	prtf("DMA %d Received an Abort!\n",unit);
	status++;
    }
    temp = read_long (DMA_CMD_STAT);
    if (!(temp & done))
    {
	prtf("DMA %d Done Bit not Set!\n",unit);
	status++;
    }

    if (status)
    {
	return(TRANSFER_ERROR);
    }
    else
    {
	return(OK);
    }
}


int CheckTurkey(int unit)
{
    register long timeout;

    timeout = LONGTIME;

    if (unit)
    {
	for (timeout = 0; ((timeout < LONGTIME) && (pci_dma1 == FALSE)); )
	{
	    timeout++;
	}
        /* no interrupt received */
    	if ((timeout == LONGTIME) && (pci_dma1 == FALSE))
    	{
	    prtf("DMA 1 Transfer Timeout!\n");
    	    return(InspectDMA(unit));
    	}
    }
    else
    {
    	while ((timeout--) && (pci_dma0 == FALSE)) ;
        /* no interrupt received */
    	if ((!timeout) && (pci_dma0 == FALSE))
    	{
	    prtf("DMA 0 Transfer Timeout!\n");
    	    return(InspectDMA(unit));
	}
    }
    return(InspectDMA(unit));
}


static int
DMATest (
	int	unit,		/* which DMA Controller */
	void	*local,		/* pointer to Shared DRAM */
	void	*remote,	/* pointer to PCI memory */
	int	nbytes,		/* number of bytes in the block */
	int	dir,		/* READ or WRITE */
	long	*badAddr	/* Failure address */
	)
{
    register long	currentAddr;	/* Current address being tested */
    register long	start;		/* where to look for data */
    char	fail = 0;		/* Test hasn't failed yet */

    start = 0;		/* to quiet the compiler */

    if (dir == READ)
    {
	/* clear local memory */
	ClearMem(local, nbytes);

	/* stuff PCI memory with addresses */
	SetMem((void *) ((char *) remote + LOCAL_TO_PCI_OFFSET), local, nbytes);

	start = (long)local;	/* look in Local DRAM for data */
    }

    else if (dir == WRITE)
    {
	/* clear PCI memory */
	ClearMem((void *) ((char *) remote + LOCAL_TO_PCI_OFFSET), nbytes);

	/* stuff local memory with addresses */
	SetMem(local, (void *) ((char *) remote + LOCAL_TO_PCI_OFFSET), nbytes);

	/* look at Slave Memory for data */
	start = (long)((char *) remote + LOCAL_TO_PCI_OFFSET);
    }

    InitDMA(unit, local, remote, nbytes, dir);

    if (CheckTurkey(unit) == OK)
    {
	/* validate that data is correct */
	for (currentAddr = start;
     	    (currentAddr < (start + DMA_BLK)) && (!fail);
     	     currentAddr += 4)
	{
	    if (read_long((long *)currentAddr) != currentAddr)
		fail = 1;
	}

	if (fail)
	{
	    *badAddr = currentAddr - 4;
	    nodump = FALSE;	/* data miscompare */
	    return FAILED;
	}
	else
	{
	    return PASSED;
	}
    }
    else
    {
	nodump = TRUE;		/* nothing useful to get */
	return FAILED;
    }
}

/*
 * Returns 1 if passed, 0 if failed.
 */

int
dmaTest (
	 int	unit,		/* DMA 0 or DMA 1 */
	 long	*PAddr,		/* PCI Start address of test */
	 long	*LAddr,		/* Local DRAM Start address of test */
	 long	count		/* Byte count for test */
	 )
{
    long	badAddr;		/* Addr test failed at */

    if ((unit != 0) && (unit != 1))
    {
	prtf("Illegal DMA Channel: %d\n", unit);
	return 0;
    }

    prtf("PCI DMA %d Read test: ", unit);
    if (DMATest(unit, LAddr, PAddr, count, READ, &badAddr) == FAILED)
    {
	prtf("failed\n");
	if (nodump == FALSE)
	    dumpMem(badAddr);
	return 0;
    }
    prtf("passed\n");
    prtf("PCI DMA %d Write test: ", unit);
    if (DMATest(unit, LAddr, PAddr, count, WRITE, &badAddr) == FAILED)
    {
	prtf("failed\n\n");
	if (nodump == FALSE)
	    dumpMem(badAddr);
	return 0;
    }
    prtf("passed\n");
    return 1;
}


/*
 * cyt_dmatest.c
 *	written 12 Nov. 1993 by Scott Coulter for VME DMA
 *	modified 27 Sept. 1994 by Scott Coulter for PCI DMA
 *
 * This file contains the routine to test the PCI DMA functions.
 *
 *	PCI transfers will be tested in both directions:
 *
 *		PCI Reads
 *		PCI Writes
 *
 */

int testDMA (unsigned PstartAddr)
{
	long *lptr;
	long	temp;
	int stats = 0;

	lptr = (unsigned long *)ABOVE_EVERYTHING;

	prtf("Testing PCI9060 DMAs on on-board memory from $");
	hex32out((long)lptr);
	prtf(" to $");
	temp = (long)lptr + DMA_BLK;
	hex32out(temp);
	prtf(".\n");
	prtf("Off-board (PC) memory from PCI Bus address $");
	hex32out(PstartAddr);
	prtf(" to $");
	temp = (long)PstartAddr + DMA_BLK;
	hex32out(temp);
	prtf(".\n");
	prtf("Off-board (PC) memory from Local Bus address $");
	hex32out(PstartAddr + LOCAL_TO_PCI_OFFSET);
	prtf(" to $");
	temp = (long)PstartAddr + DMA_BLK + LOCAL_TO_PCI_OFFSET;
	hex32out(temp);
	prtf(".\n\n");

#if 0 /* Rev. 2 of the PLX chip requires that DMA Channel 0
         runs at 1 wait state to local DRAM, Cyclone memory
	 is 0 wait states for reads... */
 	prtf("Testing DMA Controller 0\n");
	if (dmaTest(0,(long *)PstartAddr, lptr, (long)DMA_BLK) == 0)
	{
	    stats++;
	}
#endif
 	prtf("Testing DMA Controller 1\n");
	if (dmaTest(1,(long *)PstartAddr, lptr, (long)DMA_BLK) == 0)
	{
	    stats++;
	}

	if (stats)
	    return FAILED;
	else
	    return PASSED;
}
