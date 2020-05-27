/*
 * Include file for etherTest.c.
 * Greg Ames, 3/5/91
 */

/*
#include "cvme964.h"
*/

/*
 * To get declaration of ETHERMTU and other network constants.
 * Will also pull in in.h and types.h giving us u_int, u_long,
 * u_short, u_char as well.
 */


typedef volatile unsigned long u_long;
typedef volatile unsigned int u_int;
typedef volatile unsigned short u_short;
typedef volatile unsigned char u_char;

#define ETHERMTU	1500
#define OK		0
#define ERROR		-1

/* Starting location for ether_test private malloc pool */
#define ETHER_MEM_POOL	0xa001e000	/* above top of diags. BE CAREFUL */

/* Length of interrupt time-out loops. */
#define MAX_DELAY		2000000

/* PORT* commands (lower 4 bits) */
#define PORT_RESET	((u_long) 0x0)
#define PORT_SELF_TEST	((u_long) 0x1)
#define ALT_SCP_ADDR	((u_long) 0x2)
#define PORT_DUMP	((u_long) 0x3)

/* Values for the sysbus byte of the SCP */

#define	CSW		0x40	/* Bit #6 must be 1 (see erata) */
#define	INT_ACTIVE_HIGH	0x00	/* Polarity of interrupt pin */
#define	INT_ACTIVE_LOW	0x20
#define	LOCK_ENABLED	0x00	/* Use LOCK* for atomic access? */
#define	LOCK_DISABLED	0x10
#define INT_TRIG_BUS_TH	0x00	/* Int/ext triggering of bus throttle timers */
#define EXT_TRIG_BUS_TH	0x08
#define MODE_82596	0x00	/* Memory addressing modes */
#define MODE_32_BIT_SEG	0x02
#define MODE_32_BIT_LIN	0x04

#define SYSBUS_BYTE	(CSW | INT_ACTIVE_LOW | LOCK_ENABLED | \
			 INT_TRIG_BUS_TH | MODE_32_BIT_LIN)

/* Information on i596 interrupt timeouts */

#define TIMEOUT_LENGTH	(sysClkRateGet() * 3)	/* 3 second timeout */
#define TIMEOUT_STATUS	0xffff

/* Command codes for the command fields of command descriptor blocks */

#define NOP		0
#define IA_SETUP	1
#define CONFIGURE	2
#define MC_SETUP	3
#define TRANSMIT	4
#define TDR		5
#define DUMP		6
#define DIAGNOSE	7

/* Commands for command unit and receive unit in command word of SCB */
#define CU_NOP		0
#define CU_START	1
#define CU_RESUME	2
#define CU_SUSPEND	3
#define CU_ABORT	4
#define CU_LOAD_TIME	5
#define CU_LD_REST_TIME	6

#define RU_NOP		0
#define RU_START	1
#define RU_RESUME	2
#define RU_SUSPEND	3
#define RU_ABORT	4

/* Misc. defines */
#define END_OF_LIST	1
#define BUSY		1
#define BUS_T_ON_VALUE	0xffff
#define BUS_T_OFF_VALUE	0xffff

#define RU_READY	0x4

/*
 * Structure allignments.  All addresses passed to the 596 must be
 * even (bit 0 = 0), EXCEPT for addresses passed by the PORT*
 * function (self-test address, dump address, and alternate SCP
 * address), which must be 16-byte aligned.
 */

#define SELF_TEST_ALIGN	16
#define DUMP_ALIGN	16
#define SCP_ALIGN	16
#define DEF_ALIGN	4

/*
 * Bit definitions for the configure command.  NOTE:  Byte offsets are
 * offsets from the start of the structure (8 and up) to correspond
 * with the offsets in the 82596 manual.
 */

/* Byte #8 */
#define BYTE_COUNT	0x0e
#define PREFETCH_DISABLE 0
#define BYTE_8		(BYTE_COUNT | PREFETCH_DISABLE)

/* Byte #9 */
#define FIFO_LIMIT	0x08
#define MONITOR_DISABLE	0xc0
#define BYTE_9		(FIFO_LIMIT | MONITOR_DISABLE)

/* Byte #10 */
#define NO_SV_BAD_FRAME	0
#define BYTE_10		(0x40 | NO_SV_BAD_FRAME)

/* Byte #11 */
#define ADDRESS_LENGTH	6
#define INSERT_SRC_ADDR	0
#define PREAMBLE_LENGTH	0x20
#define NO_LOOP_BACK	0
#define INT_LOOP_BACK	0x40
#define EXT0_LOOP_BACK	0x80
#define EXT1_LOOP_BACK	0xc0
#define BYTE_11		(ADDRESS_LENGTH | INSERT_SRC_ADDR | PREAMBLE_LENGTH)

/* Byte #12 */
#define BYTE_12		0

/* Byte #13 */
#define BYTE_13		0x60

/* Byte #14 */
#define BYTE_14		0

/* Byte #15 */
#define SLOT_TIME_HIGH	0x02
#define MAX_RETRIES	0xf0
#define BYTE_15		(MAX_RETRIES | SLOT_TIME_HIGH)

/* Byte #16 */
#define BYTE_16		0

/* Byte #17 */
#define BYTE_17		0

/* Byte #18 - minimum frame length */
#define BYTE_18		0x40

/* Byte #19 */
#define BYTE_19		0xdb

/* Byte #20 */
#define BYTE_20		0

/* Byte #21 */
#define BYTE_21		0x3f

/* Ethernet address for i82596 */
static u_char addr_596[] = { 0x00, 0x80, 0x4d, 0x03, 0x04, 0x05 };

/*
 * 82596 structures.  NOTE: the 596 is used in 32-bit linear addressing
 * mode.  See alignment restrictions above.
 */

/* Result of PORT* self-test command - MUST be 16 byte aligned! */
struct selfTest {
	u_long	romSig;			/* signature of rom */
	union {				/* Flag bits - as u_long or field */
		struct {
			u_int	rsrv1 : 2;
			u_int	romTest : 1;
			u_int	regTest : 1;
			u_int	busTimerTest : 1;
			u_int	diagnTest : 1;
			u_int	rsrv2 : 9;
			u_int	selfTest : 1;
			u_short	rsrv3;
		} bits;
		u_long	word2;
	} u;
};

/* System configuration pointer - MUST be 16 byte aligned! */
struct SCPtype {
	u_char		rsrv1[2];	/* reserved bytes */
	u_char		sysBus;		/* sysbus byte */
	u_char		rsrv2[5];	/* more reserved bytes */
	struct ISCPtype *ISCPAddr;	/* address of the ISCP */
};

/* Intermediate system configuration pointer */
struct ISCPtype {
	u_char		busy;		/* set to 1, 82596 clrs when inited */
	u_char		rsrv1[3];	/* reserved bytes */
	struct SCBtype *SCBAddr;	/* address of the SCB */
};

/* System command block */
struct SCBtype {
	union {
		struct {
			u_int	rsrv1 : 3;	/* Reserved */
			u_int	t : 1;		/* Bus thr. timers loaded */
			u_int	rus : 4;	/* Receive unit status */
			u_int	cus : 3;	/* Command unit status */
			u_int	rsrv2 : 1;	/* Reserved */
			u_int	rnr : 1;	/* RU not ready */
			u_int	cna : 1;	/* CU not active */
			u_int	fr : 1;		/* Frame reception done */
			u_int	cx : 1;		/* Cmd exec completed */
			u_int	rsrv3 : 4;	/* Reserved */
			u_int	ruc : 3;	/* Receive unit command */
			u_int	r : 1;		/* Reset chip */
			u_int	cuc : 3;	/* Command unit command */
			u_int	rsrv4 : 1;	/* Reserved */
			u_int	ack_rnr : 1;	/* ACK RU not ready */
			u_int	ack_cna : 1;	/* ACK CU not active */
			u_int	ack_fr : 1;	/* ACK frame reception done */
			u_int	ack_cx : 1;	/* ACK cmd exec completed */
		} bits;
		struct {
			u_short	status;
			u_short	command;
		} words;
	} cmdStat;
	union cmdBlock	*pCmdBlock;	/* Ptr to first command block */
	struct rfd	*pRfd;		/* Ptr to first receive frame disc. */
	u_long		crcErrs;	/* # frames w/ CRC errors */
	u_long		alignErrs;	/* # frames w/ alignment errors */
	u_long		rsrcErrs;	/* # lost (no resources available) */
	u_long		overrunErrs;	/* # lost (couldn't get local bus) */
	u_long		rcvcdtErrs;	/* # collis. during frame reception */
	u_long		shrtFrmErrs;	/* # frames w/ len < min frame len */
	u_short		tOnTimer;	/* Bus throttle timers */
	u_short		tOffTimer;
};

/* Command blocks - declared as a union; some commands have different fields */
union cmdBlock {
	/* No operation */
	struct {
		u_int	rsrv1 : 13;	/* reserved bits (set to 0) */
		u_int	ok : 1;		/* 1 = command completed, no error */
		u_int	b : 1;		/* 1 = command in progress */
		u_int	c : 1;		/* 1 = command completed */
		u_int	code : 3;	/* command code (0 = NOP) */
		u_int	rsrv2 : 10;	/* reserved bits (set to 0) */
		u_int	i : 1;		/* 1 = interrupt upon completion */
		u_int	s : 1;		/* 1 = suspend CU upon completion */
		u_int	el : 1;		/* 1 = last cmdBlock in list */
		union cmdBlock *link;	/* next block in list */
	} nop;
	/* Individual address setup */
	struct {
		u_int	rsrv1 : 12;	/* reserved bits (set to 0) */
		u_int	a : 1;		/* 1 = command aborted by CU_ABORT */
		u_int	ok : 1;		/* 1 = command completed, no error */
		u_int	b : 1;		/* 1 = command in progress */
		u_int	c : 1;		/* 1 = command completed */
		u_int	code : 3;	/* command code (1 = ia setup) */
		u_int	rsrv2 : 10;	/* reserved bits (set to 0) */
		u_int	i : 1;		/* 1 = interrupt upon completion */
		u_int	s : 1;		/* 1 = suspend CU upon completion */
		u_int	el : 1;		/* 1 = last cmdBlock in list */
		union cmdBlock *link;	/* next block in list */
		u_char	enetAddr[6];	/* hardware ethernet address */
		u_short	rsrv3;		/* padding */
	} iaSetup;
	/* Configure */
	struct {
		u_int	rsrv1 : 12;	/* reserved bits (set to 0) */
		u_int	a : 1;		/* 1 = command aborted by CU_ABORT */
		u_int	ok : 1;		/* 1 = command completed, no error */
		u_int	b : 1;		/* 1 = command in progress */
		u_int	c : 1;		/* 1 = command completed */
		u_int	code : 3;	/* command code (2 = configure) */
		u_int	rsrv2 : 10;	/* reserved bits (set to 0) */
		u_int	i : 1;		/* 1 = interrupt upon completion */
		u_int	s : 1;		/* 1 = suspend CU upon completion */
		u_int	el : 1;		/* 1 = last cmdBlock in list */
		union cmdBlock *link;	/* next block in list */
		u_char	configData[14];	/* configuration data */
		u_short	rsrv3;		/* padding */
	} configure;
	/* Multicast address setup */
	struct {
		u_int	rsrv1 : 12;	/* reserved bits (set to 0) */
		u_int	a : 1;		/* 1 = command aborted by CU_ABORT */
		u_int	ok : 1;		/* 1 = command completed, no error */
		u_int	b : 1;		/* 1 = command in progress */
		u_int	c : 1;		/* 1 = command completed */
		u_int	code : 3;	/* command code (3 = mc setup) */
		u_int	rsrv2 : 10;	/* reserved bits (set to 0) */
		u_int	i : 1;		/* 1 = interrupt upon completion */
		u_int	s : 1;		/* 1 = suspend CU upon completion */
		u_int	el : 1;		/* 1 = last cmdBlock in list */
		union cmdBlock *link;	/* next block in list */
		u_int	mcCount : 14;	/* # of bytes in mcAddrList[] */
		u_int	rsrv3 : 2;	/* Pad to byte boundary */
		u_char	mcAddrList[1];	/* list of multicast addresses */
	} mcSetup;
	/* Transmit */
	struct {
		u_int	maxColl : 4;	/* # collisions for this frame */
		u_int	rsrv1 : 1;	/* unused */
		u_int	toManyColl : 1;	/* tx aborted; to many collisions */
		u_int	heartbeat : 1;	/* heartbeat test */
		u_int	txDeferred : 1;	/* tx not immediate; link busy  */
		u_int	dmaUnderrun : 1; /* tx failed; DMA underrun */
		u_int	ctsLost : 1;	/* tx failed; CTS signal lost */
		u_int	noCarrier : 1;	/* carrier sense lost during tx */
		u_int	lateColl : 1;	/* late collision detected */
		u_int	a : 1;		/* 1 = command aborted by CU_ABORT */
		u_int	ok : 1;		/* 1 = command completed, no error */
		u_int	b : 1;		/* 1 = command in progress */
		u_int	c : 1;		/* 1 = command completed */
		u_int	code : 3;	/* command code (4 = transmit) */
		u_int	rsrv2 : 8;	/* reserved bits (set to 0) */
		u_int	sf : 1;		/* 1 = flexible mode */
		u_int	nc : 1;		/* 1 = no crc insertion enable */
		u_int	i : 1;		/* 1 = interrupt upon completion */
		u_int	s : 1;		/* 1 = suspend CU upon completion */
		u_int	el : 1;		/* 1 = last cmdBlock in list */
		union cmdBlock *link;	/* next block in list */
		u_char	*tbdAddr;	/* tx buf addr; all 1s for simp mode */
		u_int	tcbCount : 14;	/* # bytes to be tx from cmd block */
		u_int	rsrv3 : 1;	/* reserved (set to 0) */
		u_int	eof : 1;	/* 1 = entire frame in cmd block */
		u_short	rsrv4;		/* reserved (set to 0) */
		u_char	destAddr[6];	/* destination hardware address */
		u_short	length;		/* 802.3 packet length (from packet) */
		u_char	txData[ETHERMTU];	/* optional data to tx */
	} transmit;
	/* Time domain reflectometry */
	struct {
		u_int	rsrv1 : 12;	/* reserved bits (set to 0) */
		u_int	a : 1;		/* 1 = command aborted by CU_ABORT */
		u_int	ok : 1;		/* 1 = command completed, no error */
		u_int	b : 1;		/* 1 = command in progress */
		u_int	c : 1;		/* 1 = command completed */
		u_int	code : 3;	/* command code (5 = tdr) */
		u_int	rsrv2 : 10;	/* reserved bits (set to 0) */
		u_int	i : 1;		/* 1 = interrupt upon completion */
		u_int	s : 1;		/* 1 = suspend CU upon completion */
		u_int	el : 1;		/* 1 = last cmdBlock in list */
		union cmdBlock *link;	/* next block in list */
		u_int	time : 11;	/* # tx clocks before echo */
		u_int	rsrv3 : 1;	/* 1 = short on ethernet cable */
		u_int	etSht : 1;	/* 1 = short on ethernet cable */
		u_int	etOpn : 1;	/* 1 = open on ethernet cable */
		u_int	xvrPrb : 1;	/* 1 = problem w/ transceiver */
		u_int	lnkOK : 1;	/* 1 = no problems detected */
		u_short	rsrv4;		/* padding */
	} tdr;
	/* Dump 82596 registers */
	struct {
		u_int	rsrv1 : 13;	/* reserved bits (set to 0) */
		u_int	ok : 1;		/* 1 = command completed, no error */
		u_int	b : 1;		/* 1 = command in progress */
		u_int	c : 1;		/* 1 = command completed */
		u_int	code : 3;	/* command code (6 = dump) */
		u_int	rsrv2 : 10;	/* reserved bits (set to 0) */
		u_int	i : 1;		/* 1 = interrupt upon completion */
		u_int	s : 1;		/* 1 = suspend CU upon completion */
		u_int	el : 1;		/* 1 = last cmdBlock in list */
		union cmdBlock *link;	/* next block in list */
		u_char	*bufAddr;	/* where to dump registers */
	} dump;
	/* Diagnose - perform self test */
	struct {
		u_int	rsrv1 : 11;	/* reserved bits (set to 0) */
		u_int	f : 1;		/* 1 = self test failed */
		u_int	rsrv2 : 1;	/* reserved bits (set to 0) */
		u_int	ok : 1;		/* 1 = command completed, no error */
		u_int	b : 1;		/* 1 = command in progress */
		u_int	c : 1;		/* 1 = command completed */
		u_int	code : 3;	/* command code (7 = diagnose) */
		u_int	rsrv3 : 10;	/* reserved bits (set to 0) */
		u_int	i : 1;		/* 1 = interrupt upon completion */
		u_int	s : 1;		/* 1 = suspend CU upon completion */
		u_int	el : 1;		/* 1 = last cmdBlock in list */
		union cmdBlock *link;	/* next block in list */
	} diagnose;
};

/* Receive frame descriptors (uses simplified memory structure) */
struct rfd {
	u_int	rxColl : 1;	/* 1 = collision on reception */
	u_int	rsrv1 : 5;	/* reserved bits (set to 0) */
	u_int	noEOP : 1;	/* No EOP flag */
	u_int	iaMatch : 1;	/* Dest addr matched chip's hardware addr */
	u_int	dmaOverrun : 1;	/* DMA overrun (couldn't get local bus) */
	u_int	noRsrc : 1;	/* No resources (out of buffer space) */
	u_int	alignErr : 1;	/* CRC error on misaligned frame */
	u_int	crcErr : 1;	/* CRC error on aligned frame */
	u_int	lenErr : 1;	/* Length error (if checking) */
	u_int	ok : 1;		/* 1 = command completed, no error */
	u_int	b : 1;		/* 1 = command in progress */
	u_int	c : 1;		/* 1 = command completed */
	u_int	rsrv2 : 3;	/* reserved bits (set to 0) */
	u_int	sf : 1;		/* 1 = Flexible mode */
	u_int	rsrv3 : 10;	/* reserved bits (set to 0) */
	u_int	s : 1;		/* 1 = suspend CU upon completion */
	u_int	el : 1;		/* 1 = last cmdBlock in list */
	union cmdBlock *link;	/* next block in list */
	u_char	*rbdAddr;	/* rx buf desc addr; all 1s for simple mode */
	u_int	actCount : 14;	/* # bytes in this buffer (set by 82596) */
	u_int	f : 1;		/* 1 = buffer used */
	u_int	eof : 1;	/* 1 = last buffer for this frame */
	u_int	size : 14;	/* # bytes avail in this buffer (set by CPU) */
	u_int	rsrv4 : 2;	/* reserved bits (set to 0) */
	u_char	destAddr[6];	/* destination address */
	u_char	sourceAddr[6];	/* source address */
	u_short	length;		/* 802.3 packet length (from packet) */
	u_char	rxData[ETHERMTU];	/* optional data (simplified mode) */
};

/* Forward declarations */
char *memget ();
void memfree ();
void portWrite ();
void sendCA ();
void resetChip ();
void makePacket ();
int checkPacket ();
void i596IntHandler ();
void i596WdHandler ();
int waitForInt();

int echoTest ();
