/*
 * cyt_dma.c - this file performs an PCI-9060 dma test
 */

/*******************************************************************************
*
* MODIFICATION HISTORY:
*
* 17aug95	snc - no PCI Direct Master reads for PLX chips with a 
*		      revision < 3
* 27sep93	snc - initial authorship
* 09aug95       snc - added to "po" test suite
* 31aug95	snc - fixed a compiler warning for SetMem() calls (ptr addition)
*
*/

#include "dmatest.h"
#include "cyt_intr.h"
#include "this_hw.h"

/* flags for DMA interrupts - in cytpci.c */
extern volatile int pci_dma0;
extern volatile int pci_dma1;

extern void dumpMem();
extern void hex32out();

extern prtf();

extern void write_long();
extern unsigned long read_long();

/* the silicon revision of the PLX chip - only valid after a hard reset */
extern int plx_revision;

/* boolean indicating if the PLX has been screened for Direct Master Reads */
extern int plx_screened;

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
	SetMem((long *) ((char *) remote + LOCAL_TO_PCI_OFFSET), local, nbytes);

	start = (long)local;	/* look in Local DRAM for data */
    }

    else if (dir == WRITE)
    {
	/* clear PCI memory */
	ClearMem(remote + LOCAL_TO_PCI_OFFSET, nbytes);

	/* stuff local memory with addresses */
	SetMem(local, (long *) ((char *)remote + LOCAL_TO_PCI_OFFSET), nbytes);

	/* look at Slave Memory for data */
	start = (long)(remote + LOCAL_TO_PCI_OFFSET);
    }

    InitDMA(unit, local, remote, nbytes, dir);

    if (CheckTurkey(unit) == OK)
    {
      /* Direct Master Read errata fixed in Rev. 3
         However,some parts are screened to be Direct Master Read functional */
      if ((plx_revision >= 3) || ((plx_revision < 3) && (plx_screened == TRUE)))
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
      else    /* with revision <3, can't use Direct Master Read to look
		 at the data that was written across the PCI Bus, so
		 set up the DMA to copy the data back and then verify
		 the data in local memory */
      {
	/* clear local memory before DMA read */
	ClearMem ((long *)local, nbytes);

    	InitDMA(unit, local, remote, nbytes, READ);
    	if (CheckTurkey(unit) == OK)
	{
	unsigned long pciAddr;

	/* validate that data is correct */
	for (currentAddr = (unsigned long)local, pciAddr = start;
     	    (currentAddr < ((unsigned long)local + DMA_BLK)) && (!fail);
     	     currentAddr += 4, pciAddr += 4)
	{
	  if ((*(long *)currentAddr) != pciAddr)
		fail = 1;
	  if (fail)
	  {
            *badAddr = currentAddr - 4;
            nodump = FALSE;     /* data miscompare */
            return FAILED;
          }
          else
          {
            return PASSED;
          }
	}

	}
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

    if (plx_revision >= 3) /* Rev. 2 of the PLX chip requires that DMA Channel 0
         		      runs at 1 wait state to local DRAM, Cyclone memory
	                      is 0 wait states for reads... */
    {
 	prtf("Testing DMA Controller 0\n");
	if (dmaTest(0,(long *)PstartAddr, lptr, (long)DMA_BLK) == 0)
	{
	    stats++;
	}
    }

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
