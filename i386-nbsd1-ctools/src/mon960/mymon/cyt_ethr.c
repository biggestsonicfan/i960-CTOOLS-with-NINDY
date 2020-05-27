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

/* Program to do loopback tests on 82596 Ethernet Chip. */

/******************************************************************************
*
* MODIFICATION HISTORY:
*
* 05mar95	tga - initial authorship
* 31aug94	snc - changed to support Cx, Sx, and Kx
* 12sep94	snc - more fixes
* 09aug95	snc - fixed size setup of Rcv. Frame Descriptor
*		      to include the Ethernet header as well
*		      as the data area.  fixed checkPacket()
*		      to check entire packet and print error
*		      information.
*
*/

#include "this_hw.h"
#include "cyt_ethr.h"
#include "cyt_intr.h"

#define NULL ((void *) 0)

/* Forward declarations */
int etherTest ();
static int i596SelfTest ();
static int initialize ();
static int i596Init ();
int i596Config ();
static int i596AddrSet ();
int i596RUStart ();
static void setUpPacket ();
int txPacket ();
static char *malloc ();
static void bzero ();
int init_ether();

/* Externals */
long hexIn();
void prtf();
void bcopy();

/* Globals needed by both main program and irq handler */
struct SCBtype *pSCB;		/* Pointer to SCB in use */
u_int waitSem;			/* Used to block test until interrupt */
u_short i596Status;		/* Status code from SCB */
static volatile char *mem_pool; /* Ptr to malloc's free memory pool */

/*
 * These globals used to be declared as constants in ether_te.h.
 * In order to simplify porting to the 964, where 596 device addresses
 * and xint pins vary by squall module, we assign these globals in
 * ether_test
 */
volatile unsigned long * ca_addr;   /* Address of CA register */
volatile unsigned long * port_addr; /* Address of PORT register */
int which_irq;			/* Parameter for enable/disable intr */

/* flag to determine if i596 interrupt should be serviced */
char int_enabled;

/* The test */

void ether_test (ca_addr_loc, port_addr_loc, which_irq_loc)
volatile unsigned long * ca_addr_loc;
volatile unsigned long * port_addr_loc;
int which_irq_loc;
{
struct SCPtype	*pSCP;		/* Address of system configuration pointer */
char		*pPacketBuf;

	ca_addr = ca_addr_loc;
	port_addr = port_addr_loc;
	which_irq = which_irq_loc;
	int_enabled = 0;	/* i596 interrupts not serviced yet */

	/* Initialize malloc's memory pool pointer */
	mem_pool = (char *) ETHER_MEM_POOL;

	/* Start off by resetting 596 to put it in a known state */
	resetChip ();

	/* Run the built-in self test through the port register */
	if (i596SelfTest () == ERROR){
		int_enabled = 0;	/* i596 interrupts not serviced yet */
		return;}

	prtf ("Press return to initialize ethernet controller.\n");
	hexIn ();

	/* Initialize data structures */
	if ((initialize (&pSCP,1) == ERROR) ||
	    ((pPacketBuf = malloc(ETHERMTU + sizeof(u_short) + 6)) == NULL)){
		int_enabled = 0;	/* i596 interrupts not serviced yet */
		return;}

	/* Initialize chip and locate SCB in address space */
	if (i596Init (pSCP,1) == ERROR){
		int_enabled = 0;	/* i596 interrupts not serviced yet */
		return;}

	/* Set hardware address */
	if (i596AddrSet () == ERROR){
		int_enabled = 0;	/* i596 interrupts not serviced yet */
		return;}

	prtf ("Press return to perform internal loopback test.\n");
	hexIn ();

	/* Configure for internal loopback */
	if (i596Config (INT_LOOP_BACK) == ERROR){
		int_enabled = 0;	/* i596 interrupts not serviced yet */
		return;}

	/* Initialize receive buffer and enable receiver */
	if (i596RUStart () == ERROR){
		int_enabled = 0;	/* i596 interrupts not serviced yet */
		return;}

	/* Send a packet */
	setUpPacket (pPacketBuf);
	if (txPacket (pPacketBuf) == ERROR){
		int_enabled = 0;	/* i596 interrupts not serviced yet */
		return;}

	prtf ("Press return to perform external loopback through\n");
	prtf ("82c501 serial interface chip.\n");
	hexIn ();

	/* Repeat with external loopback, LPBK* pin asserted */
	if (i596Config (EXT1_LOOP_BACK) == ERROR){
		int_enabled = 0;	/* i596 interrupts not serviced yet */
		return;}

	if (i596RUStart () == ERROR){
		int_enabled = 0;	/* i596 interrupts not serviced yet */
		return;}

	setUpPacket (pPacketBuf);
	if (txPacket (pPacketBuf) == ERROR){
		int_enabled = 0;	/* i596 interrupts not serviced yet */
		return;}

	prtf ("Press return to perform external loopback through\n");
	prtf ("external transceiver.  NOTE: this test will work only\n");
	prtf ("if a properly functioning transceiver and ethernet\n");
	prtf ("cable are attached to the network connector on the\n");
	prtf ("front panel.\n");
	hexIn ();

	/* Repeat with external loopback, LPBK* pin not asserted */
	if (i596Config (EXT0_LOOP_BACK) == ERROR){
		int_enabled = 0;	/* i596 interrupts not serviced yet */
		return;}

	if (i596RUStart () == ERROR){
		int_enabled = 0;	/* i596 interrupts not serviced yet */
		return;}

	setUpPacket (pPacketBuf);
	if (txPacket (pPacketBuf) == ERROR) {
		prtf ("Double-check drop cable, transceiver, and network\n");
		prtf ("coax cable.  Try testing them with another system\n");
		prtf ("(such as a workstation) that is working correctly.\n");
		prtf ("Also verify that the Ethernet type jumper is\n");
		prtf ("set correctly for your network type.\n");
		int_enabled = 0;	/* i596 interrupts not serviced yet */
		return;
	}

	/* It worked! */

	int_enabled = 0;	/* i596 interrupts not serviced yet */

	prtf ("\nEthernet controller passed.  Press return to continue.\n");
	hexIn ();
}

/* Initialization of the ethernet chip so it would turn off interrupt */

int init_ether()
{
struct SCPtype	*pSCP;		/* Address of system configuration pointer */
char		*pPacketBuf;

	/* Initialize malloc's memory pool pointer */
	mem_pool = (char *) ETHER_MEM_POOL;

	/* Start off by resetting 596 to put it in a known state */
	resetChip ();

	/* Initialize data structures */
	if ((initialize (&pSCP,0) == ERROR) ||
	    ((pPacketBuf = malloc(ETHERMTU + sizeof(u_short) + 6)) == NULL))
		return(ERROR);

	/* Initialize chip and locate SCB in address space */
	if (i596Init (pSCP,0) == ERROR)
		return(ERROR);

	disable_intr (which_irq);	/* Disable 596 interrupt */
	return(OK);
}

/* Perform internal self test - returns OK if sucessful, ERROR if not. */

static int i596SelfTest ()
{
	struct selfTest *pSelfTestMem;
	u_long		oldWord2;
	long		delay;
	int		rtnVal;

	/* Allocate some memory for the self test */
	pSelfTestMem = (struct selfTest *) malloc (sizeof(struct selfTest));

	if (pSelfTestMem == NULL) {
		prtf ("Couldn't get memory for self test.\n");
		return (ERROR);
	}

	prtf ("Sending PORT* self-test command...\n");
	prtf ("Dump address = 0x%X\n", pSelfTestMem);

	/* Set all bits in  second word, wait until it changes or a timeout */
	pSelfTestMem->u.word2 = ~0;
	oldWord2 = pSelfTestMem->u.word2;

	portWrite ((u_long) pSelfTestMem + PORT_SELF_TEST);

	/* Wait for test completion or for timeout */
	for (delay = 0;
	     (delay < MAX_DELAY) && (pSelfTestMem->u.word2 == oldWord2);
	     delay++)
		;			/* Wait... */

	/* Print results */
	prtf ("Self test result: %s\n", (pSelfTestMem->u.bits.selfTest)
					  ? "Fail" : "Pass");
	prtf ("ROM content test: %s\n", (pSelfTestMem->u.bits.romTest)
					  ? "Fail" : "Pass");
	prtf ("Register test:    %s\n", (pSelfTestMem->u.bits.regTest)
					  ? "Fail" : "Pass");
	prtf ("Bus timer test:   %s\n", (pSelfTestMem->u.bits.busTimerTest)
					  ? "Fail" : "Pass");
	prtf ("Diagnose test:    %s\n", (pSelfTestMem->u.bits.diagnTest)
					  ? "Fail" : "Pass");
	prtf ("ROM signature:    0x%X\n", pSelfTestMem->romSig);

	rtnVal = pSelfTestMem->u.bits.selfTest ? ERROR : OK;

	return (rtnVal);
}

/* Set up data structures, semaphore, watchdog timer, and interrupts. */

static int initialize (pSCP,msg)
struct SCPtype	**pSCP;		/* Address of system configuration pointer */
int	msg;
{
	struct ISCPtype	*pISCP;
	struct rfd	*pRfd;
	union cmdBlock	*pCmdBlock;

	/* Get memory for all the structures */
	if (((pRfd = (struct rfd *) malloc (sizeof(struct rfd))) == NULL) ||
	((pCmdBlock = (union cmdBlock *) malloc (sizeof(union cmdBlock))) ==
	 NULL) ||
	((pSCB = (struct SCBtype *) malloc (sizeof(struct SCBtype))) ==
	 NULL) ||
	((pISCP = (struct ISCPtype *) malloc (sizeof(struct ISCPtype))) ==
	 NULL) ||
	((*pSCP = (struct SCPtype *) malloc (sizeof(struct SCPtype))) ==
	 NULL)) {
		prtf ("Memory allocation failed.\n");
		return (ERROR);
	}

	/* Set up links between buffers */
	(*pSCP)->sysBus = SYSBUS_BYTE;
	(*pSCP)->ISCPAddr = pISCP;

	pISCP->busy = BUSY;
	pISCP->SCBAddr = pSCB;

	pSCB->pCmdBlock = pCmdBlock;
	pSCB->pRfd = pRfd;

	/* Set bus throttle timers to some value */
	pSCB->tOnTimer = BUS_T_ON_VALUE;
	pSCB->tOffTimer = BUS_T_OFF_VALUE;

	/* Set EL bits in command block and rfd so we don't fall of the end */
	pCmdBlock->nop.el = END_OF_LIST;
	pRfd->el = END_OF_LIST;
	if(msg)
		prtf ("SCP at 0x%X\n", *pSCP);
	return (OK);
}

/* Initialize the 82596. */

static int i596Init (pSCP,msg)
struct SCPtype	*pSCP;		/* Address of system configuration pointer */
int 	msg;
{
	/* Reset chip and initialize */
	if (msg)
		prtf ("Initializing... ");

	resetChip ();
	portWrite ((u_long) pSCP + ALT_SCP_ADDR);
	sendCA ();

	/*
	 * Initialize interrupts
	 */
	int_enabled = 0xff;
	enable_intr (IRQ_SQ_IRQ0);

	/* Wait for test completion or for 1-second timeout */

	if (waitForInt() == ERROR) {
		prtf ("failed.  pISCP->busy still 0x01.\n");
		return (ERROR);
	}
	if (msg)
		prtf ("done.  SCB address = 0x%X\n", pSCB);

	return (OK);
}

/* Set hardware address of the 82596. */

static int i596AddrSet ()
{
	prtf ("Setting hardware ethernet address to ");
	prtf ("%B:%B:%B:", addr_596[0], addr_596[1],
		addr_596[2]);
	prtf ("%B:%B:%B... ", addr_596[3], addr_596[4],
		addr_596[5]);

	/* Zero out commmand block and set up CU and RU commands in SCB */
	bzero ((char *) pSCB->pCmdBlock, sizeof(union cmdBlock));
	pSCB->cmdStat.bits.cuc = CU_START;
	pSCB->cmdStat.bits.ruc = RU_NOP;

	/* Set up iaSetup command block and execute */
	pSCB->pCmdBlock->iaSetup.code = IA_SETUP;
	pSCB->pCmdBlock->iaSetup.el = END_OF_LIST;
	bcopy (addr_596, pSCB->pCmdBlock->iaSetup.enetAddr, sizeof(addr_596));

	sendCA ();

	if ((waitForInt() == ERROR) || (pSCB->pCmdBlock->iaSetup.ok != 1)) {
		prtf ("failed.  Status: 0x%H.\n",
			pSCB->cmdStat.words.status);
		return (ERROR);
	}
	prtf ("done.\n");
	return (OK);
}

/* Configure the 82596. */

int i596Config (loopBackMode)
u_char	loopBackMode;		/* None, int, or ext 1, 2 (see etherTest.h) */
{
	prtf ("\nConfiguring for ");
	switch (loopBackMode) {
	case NO_LOOP_BACK:
		prtf ("normal transmission (no loopback)... ");
		break;
	case INT_LOOP_BACK:
		prtf ("internal loopback... ");
		break;
	case EXT0_LOOP_BACK:
		prtf ("external loopback, LPBK* not active... ");
		break;
	case EXT1_LOOP_BACK:
		prtf ("external loopback, LPBK* active... ");
		break;
	default:
		prtf ("Unknown loopback mode, exiting...\n");
		return (ERROR);
	}

	/* Zero out commmand block and set up CU and RU commands in SCB */
	bzero ((char *) pSCB->pCmdBlock, sizeof(union cmdBlock));
	pSCB->cmdStat.bits.cuc = CU_START;
	pSCB->cmdStat.bits.ruc = RU_NOP;

	/* Set up configure command block and execute */
	pSCB->pCmdBlock->configure.code = CONFIGURE;
	pSCB->pCmdBlock->configure.el = END_OF_LIST;
	pSCB->pCmdBlock->configure.configData[0] = BYTE_8;
	pSCB->pCmdBlock->configure.configData[1] = BYTE_9;
	pSCB->pCmdBlock->configure.configData[2] = BYTE_10;
	pSCB->pCmdBlock->configure.configData[3] = BYTE_11 | loopBackMode;
	pSCB->pCmdBlock->configure.configData[4] = BYTE_12;
	pSCB->pCmdBlock->configure.configData[5] = BYTE_13;
	pSCB->pCmdBlock->configure.configData[6] = BYTE_14;
	pSCB->pCmdBlock->configure.configData[7] = BYTE_15;
	pSCB->pCmdBlock->configure.configData[8] = BYTE_16;
	pSCB->pCmdBlock->configure.configData[9] = BYTE_17;
	pSCB->pCmdBlock->configure.configData[10] = BYTE_18;
	pSCB->pCmdBlock->configure.configData[11] = BYTE_19;
	pSCB->pCmdBlock->configure.configData[12] = BYTE_20;
	pSCB->pCmdBlock->configure.configData[13] = BYTE_21;

	sendCA ();

	if ((waitForInt() == ERROR) || (pSCB->pCmdBlock->configure.ok != 1)) {
		prtf ("failed.  Status: 0x%H.\n",
			pSCB->cmdStat.words.status);
		return (ERROR);
	}
	prtf ("done.\n");

	return (OK);
}

/* Set hardware address of the 82596. */

int i596RUStart ()
{
	long delay;
	prtf ("Enabling receiver... ");

	/* Zero out commmand block and set up CU and RU commands in SCB */
	bzero ((char *) pSCB->pRfd, sizeof(struct rfd));
	pSCB->cmdStat.bits.cuc = CU_NOP;
	pSCB->cmdStat.bits.ruc = RU_START;

	/* Set end-of-list bit in the rfd so we don't fall off the end */
	pSCB->pRfd->el = END_OF_LIST;
	pSCB->pRfd->s = 1;
	pSCB->pRfd->sf = 0;		/* Simplified mode */
	pSCB->pRfd->rbdAddr = (u_char *) 0xffffffff; /* No RBD */
	pSCB->pRfd->size = sizeof (pSCB->pRfd->rxData)+
                           sizeof (pSCB->pRfd->destAddr)+
                           sizeof (pSCB->pRfd->sourceAddr)+
                           sizeof (pSCB->pRfd->length); /* buffer size */

	sendCA ();

	/*
	 * Use watchdog timer to prevent us from waiting forever - we
	 * can't use waitForInt (), as this step doesn't generate interrupts.
	 */

	i596Status = 0;

	/* Wait for timeout (i596Status changes) or RU_STAT is RU_READY */
	for (delay = 0;
	     (delay < MAX_DELAY) && (pSCB->cmdStat.bits.rus != RU_READY);
	     delay++)
		;			/* Wait... */

	if (pSCB->cmdStat.bits.rus != RU_READY) {
		prtf ("failed.  Status: 0x%H.\n",
			pSCB->cmdStat.words.status);
		return (ERROR);
	}

	prtf ("done.  Status: 0x%H.\n", pSCB->cmdStat.words.status);
	return (OK);
}

/*
 * Get packet ready to send out over the network.  Buffer should be
 * ETHERMTU + sizeof(enet_addr) + sizeof(u_short)
 */

static void setUpPacket (pBuf)
char *pBuf;			/* Where to put it */
{

	bcopy (addr_596, pBuf, sizeof(addr_596));
	pBuf += sizeof(addr_596);

	*((u_short *) pBuf) = 0;
	pBuf += sizeof(u_short);

	makePacket (pBuf, ETHERMTU);

}

/* Send and verify a packet using the current loopback mode. */

int txPacket (pBuf)
char *pBuf;			/* Dest addr, ethertype, buffer */
{
	prtf ("Sending packet... ");

	/* Zero out commmand block and set up CU and RU commands in SCB */
	bzero ((char *) pSCB->pCmdBlock, sizeof(union cmdBlock));
	pSCB->cmdStat.bits.cuc = CU_START;
	pSCB->cmdStat.bits.ruc = RU_NOP;

	/* Set up transmit command block and execute */
	pSCB->pCmdBlock->transmit.code = TRANSMIT;
	pSCB->pCmdBlock->transmit.el = END_OF_LIST;
	pSCB->pCmdBlock->transmit.sf = 0;		/* Simplified mode */
	pSCB->pCmdBlock->transmit.tbdAddr = (u_char *) 0xffffffff; /* No TBD */
	pSCB->pCmdBlock->transmit.eof = 1;	/* Entire frame is here */
	pSCB->pCmdBlock->transmit.tcbCount =	/* # bytes to tx */
			sizeof (pSCB->pCmdBlock->transmit.destAddr) +
			sizeof (pSCB->pCmdBlock->transmit.length) +
			sizeof (pSCB->pCmdBlock->transmit.txData);
	bcopy (pBuf, pSCB->pCmdBlock->transmit.destAddr,
		     sizeof(addr_596) + sizeof(u_short) + ETHERMTU);

	sendCA ();

	if ((waitForInt() == ERROR) || (pSCB->pCmdBlock->transmit.ok != 1)) {
		prtf ("tx failed.  Status: 0x%H.\n",
			pSCB->cmdStat.words.status);
		return (ERROR);
	}

	/* If RU still ready, hang for receive interrupt */
	if ((pSCB->cmdStat.bits.rus == RU_READY) &&
	   ((waitForInt() == ERROR) || (pSCB->pRfd->ok != 1))) {
		prtf ("rx failed.  Status: 0x%H.\n",
			pSCB->cmdStat.words.status);
		return (ERROR);
	}


	prtf ("done.\n");

	prtf ("Verifying packet... ");
	if (checkPacket (pSCB->pCmdBlock->transmit.txData,
			 pSCB->pRfd->rxData, ETHERMTU) == ERROR) {
		prtf ("data verify error.\n");
		return (ERROR);
	}

	prtf ("data OK.\n");
	return (OK);
}

/*
 * "Poor Man's Malloc" - return a pointer to a block of memory at least
 * The block returned will have been zeroed.
 */

static char *malloc (numBytes)
int	numBytes;		/* number of bytes needed */
{
	volatile char *rtnPtr;	/* ptr to return to caller */
	long	new_mem_pool;	/* For figuring new pool base address */

	rtnPtr = mem_pool;	/* Return pointer to start of free pool */

	/* Now calculate new base of free memory pool (round to >= 16 bytes) */
	new_mem_pool = (unsigned long) mem_pool;
	new_mem_pool = ((new_mem_pool + numBytes + 0x10) &
			(~((unsigned long) 0x0f)));
	mem_pool = (volatile char *) new_mem_pool;

	bzero (rtnPtr, numBytes);

	return ((char *) rtnPtr);
}

/* "Poor Man's bzero" - zero's a block of memory. */

static void bzero (ptr, num_bytes)
char *	ptr;			/* Start of memory to zero */
long	num_bytes;		/* number of bytes to clear */
{
	long	i;		/* loop counter */

	/* zero out space */
	for (i = 0; i < num_bytes; *ptr++ = 0, i++)
		;
}

/*
 * Write "value" to port register of 82596.  To properly write the port,
 * two 32-bit writes of the same 32-bit data must be written.
 *
 * To make the code identical between Cx, Sx, and Kx processors,
 * do the PORT Access as follows:
 *
 * write 16 bit short to port_addr + 0 (least sig. short, D00 - D15)
 * write 16 bit short to port_addr + 2 ( most sig. short, D16 - D31)
 */

void portWrite (value)
u_long	value;
{
	int	delay;		/* delay betwen port accesses */
	unsigned short *first;
	unsigned short *second;

	first = (unsigned short *)port_addr;
	second = (unsigned short *)((unsigned long)port_addr + 2);

	*first = (unsigned short)(value & 0xffff);

	for (delay = 0; delay < 5000; delay++)
		;		/* wait a bit */

	*second = (unsigned short)((value>>16) & 0xffff) ;
}

/*
 * Send a channel attention signal to the 82596.
 */

void sendCA ()
{
	waitSem = 0;				/* Clear flag */

	/* use a 16 bit access to generate the CA to ensure Cx, Sx, and Kx
	   code compatibility */
	*(short *)ca_addr = (u_short) 0x0;	/* any access will do... */
}

/*
 * Do a port reset on 82596.
 */

void resetChip ()
{
	long	delay;

	portWrite (PORT_RESET);	/* bits 4-31 not used for reset */

	/* Wait a bit for chip to stabilize */
	for (delay = 0; delay < 100000; delay++)
		;			/* Wait... */
}

/*
 * Setup contents of a packet.
 */

void makePacket (pPacket, length)
u_char	*pPacket;		/* Where to put transmit data */
int	length;			/* How many bytes to put there */
{
	int	byteNum;	/* Current byte number */

	for (byteNum = 0; byteNum < length; byteNum++)
		*pPacket++ = byteNum + ' ';
}

/*
 * Verify contents of a received packet to what was transmitted.
 * Returns OK if they match, ERROR if not.
 */

int checkPacket (pTxBuffer, pRxBuffer, length)
u_char	*pTxBuffer;		/* Pointer data that was transmitted */
u_char	*pRxBuffer;		/* Pointer data that was received */
int	length;			/* How many bytes to check */
{
	int	byteNum;	/* Current byte number */

        prtf ("\n");
        prtf ("Transmit Buffer at 0x%X\n", pTxBuffer);
        prtf ("Received Buffer at 0x%X\n", pRxBuffer);
        prtf ("Data Length = 0x%H\n", length);
 
        for (byteNum = 0; byteNum < length; byteNum++)
            if (*pTxBuffer++ != *pRxBuffer++)
            {
                prtf ("Data Error at byte 0x%H\n", byteNum);
                prtf ("Expected 0x%B got 0x%B\n",
                        *(pTxBuffer - 1), *(pRxBuffer - 1));
                return (ERROR);
            }
	return (OK);
}

/*
 * Interrupt handler for i82596.  It acknowledges the interrupt
 * by setting the ACK bits in the command word and issuing a
 * channel attention.  It then updates the global status variable
 * and gives the semaphore to wake up the main routine.
 */

void i596IntHandler ()
{
	if (int_enabled == 0)		/* ints not enabled yet so return immediately */
	    return;

	/* Wait for command word to clear - indicates no pending commands */
	while (pSCB->cmdStat.words.command)
		;

	/* Update global status variable */
	i596Status = pSCB->cmdStat.words.status;

	/* Acknowledge interrupt by setting ack bits and issuing CA */
	pSCB->cmdStat.bits.ack_cx = pSCB->cmdStat.bits.cx; /* cmd exec done */
	pSCB->cmdStat.bits.ack_fr = pSCB->cmdStat.bits.fr; /* Frame rx done */
	pSCB->cmdStat.bits.ack_cna = pSCB->cmdStat.bits.cna; /* CU not ready */
	pSCB->cmdStat.bits.ack_rnr = pSCB->cmdStat.bits.rnr; /* RU not ready */

	/* Ordinarily the function call sendCA () would be made, but to conserve
	   space on the interrupt stack, the function is performed inline */

        waitSem = 0;                            /* Clear flag */

        /* use a 16 bit access to generate the CA to ensure Cx, Sx, and Kx
           code compatibility */

        *(short *)ca_addr = (u_short) 0x0;      /* any access will do... */

	/* Wait for command word to clear - indicates IACK accepted */
	while (pSCB->cmdStat.words.command)
		;

	/* Update global status variable and unblock task */
	waitSem = 1;

}

/*
 * Take the semaphore and block until i596 interrupt or timeout.
 * Returns OK if an interrupt occured, ERROR if a timeout.
 */

int waitForInt()
{
	volatile long	delay;


	/* Wait for timeout or interrupt (waitSem becomes 1) */
	for (delay = 0; (delay < MAX_DELAY) && !waitSem; delay++)
		;			/* Wait... */

	if (!waitSem)
		return (ERROR);
	else
		return (OK);
}
