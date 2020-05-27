/*(cb*/
/**************************************************************************
 *
 *     Copyright (c) 1992 Intel Corporation.  All rights reserved.
 *
 *
 * Intel hereby grants you permission to copy, modify, and distribute this
 * software and its documentation.  Intel grants this permission provided
 * that the above copyright notice appears in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  In addition, Intel grants this permission provided that
 * you prominently mark as not part of the original any modifications made
 * to this software or documentation, and that the name of Intel
 * Corporation not be used in advertising or publicity pertaining to the
 * software or the documentation without specific, written prior
 * permission.
 *
 * Intel provides this AS IS, WITHOUT ANY WARRANTY, INCLUDING THE WARRANTY
 * OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, and makes no
 * guarantee or representations regarding the use of, or the results of the
 * use of, the software and documentation in terms of correctness,
 * accuracy, reliability, currentness, or otherwise, and you rely on the
 * software, documentation, and results solely at your own risk.
 *
 **************************************************************************/
/*)ce*/

#include "this_hw.h"
#include "c145_eep.h"

#define EEPROM_PAGE_SIZE	16	/* Maximum size of X24C08 page write */
#define SLAVE_ADDR_READ		0xa1	/* Slave address byte for read */
#define SLAVE_ADDR_WRITE	0xa0	/* and writes. */
#define LOWER_8_BITS_MASK	0x0ff	/* Mask for lower 8 bits of addr */
#define UPPER_2_BITS_MASK	0x0300	/* And mask with eeprom addr, shift */
#define UPPER_2_BITS_OFFSET	7	/* right 7 bits, or into slave addr */

/*
 * X24C08 timing information.  All times in u-sec.  Note: These time are
 * all specified in the data sheet as minimum delays.  These numbers are
 * fairly conservative, ie, we are waiting a lot longer than needed in
 * most cases.
 */

/* Serial clock line */
#define SCL_LOW_PERIOD		10	/* Time serial clock is low */
#define SCL_HIGH_PERIOD		10	/* Time serial clock is high */

/* Serial data line */

#define START_SETUP_TIME	10	/* SCL high to SDA low */
#define START_HOLD_TIME		10	/* SDA low to SCL low */
#define STOP_SETUP_TIME		10	/* SCL high to SDA high */
#define DATA_IN_HOLD_TIME	1	/* SCL low to SDA change */
#define DATA_IN_SETUP_TIME	1	/* SDA change to SCL high */

/* Misc timings */
#define BUS_FREE		10	/* Delay between a stop condition */
					/* and another start condition    */
#define POWER_UP_DELAY		10000	/* From Vcc stable to read/write */
#define WRITE_DELAY		15000	/* From stop condition after write */
					/* until next start condition      */

/* Serial clock and data line states (assumes ports are non-inverting) */
#define HIGH			1
#define LOW			0

/* local functions */

static void init_timer3 ();
void eeprom_delay (int usec);
static int eeprom_page_write (int which_eeprom,	/* which eeprom to use */
			      int eeprom_addr,	/* byte offset from start */
			      unsigned char *p_data,	/* data from memory */
			      int nbytes);	/* num of bytes (0<x<=16) */
static int eeprom_send_start (int which_eeprom);/* which eeprom to use */
static int eeprom_send_stop (int which_eeprom);	/* which eeprom to use */
static int eeprom_send_byte (int which_eeprom,	/* which eeprom to use */
			     unsigned char *p_data); /* ptr to data byte */
static int eeprom_get_byte_w_ack (int which_eeprom,  /* which eeprom to use */
				  unsigned char *p_data);/* where byte goes */
static int eeprom_get_byte_no_ack (int which_eeprom, /* which eeprom to use */
				   unsigned char *p_data);/* where byte goes */
static void set_scl_line (int which_eeprom,	/* which eeprom to use */
			  int state);		/* HIGH or LOW */
static void set_sda_line (int which_eeprom,	/* which eeprom to use */
			  int state);		/* HIGH or LOW */
static int get_sda_line (int which_eeprom);	/* which eeprom to use */


/* global variables */
int powerup_wait_done = 0;		/* set true after power-up wait done */


/*-------------------------------------------------------------
 * Function:	int eeprom_read ()
 *
 * Action:	Read data from the eeprom, place it at p_data
 *
 * Returns:	OK if read worked, EEPROM_NOT_RESPONDING if
 *		read fails.
 *-------------------------------------------------------------*/
int eeprom_read (int which_eeprom,	/* one of the define's above */
		 int eeprom_addr,	/* byte offset from start of eeprom */
		 unsigned char *p_data,		/* where to put data in memory */
		 int nbytes		/* number of bytes to read */
		 )
{
    int status;				/* result code from page writes */
    unsigned char slave_addr_read;	/* slave address bytes */
    unsigned char slave_addr_write;
    unsigned char eeprom_high_addr;	/* Bits 8 and 9 of eeprom address */
    unsigned char eeprom_low_addr;	/* Bits 0-7 of eeprom address */
    int i;				/* loop variable */

    /* Do one-time initialization */
    if (!powerup_wait_done) {
	init_timer3 ();
	eeprom_delay (POWER_UP_DELAY);
	powerup_wait_done = 1;
    }

    /* Ignore trivial reads of < 0 bytes */
    if (nbytes <= 0)
	return (OK);

    /*
     * Make sure caller isn't requesting a read beyond the end of the 
     * eeprom.  In this case, the X24C08 will wrap around to the
     * beginning of the eeprom, which is most likely not what the user
     * wants.
     */
    if ((eeprom_addr + nbytes) > EEPROM_SIZE)
	return (EEPROM_TO_SMALL);

    /*
     * Break up eeprom address - bits 8 and 9 in slave address byte,
     * bits 0-7 in next byte
     */
    eeprom_high_addr = (eeprom_addr & UPPER_2_BITS_MASK)>>UPPER_2_BITS_OFFSET;
    eeprom_low_addr = eeprom_addr & LOWER_8_BITS_MASK;

    slave_addr_read = SLAVE_ADDR_READ | eeprom_high_addr;
    slave_addr_write = SLAVE_ADDR_WRITE | eeprom_high_addr;

    /*
     * Start read with a "dummy write" to set internal address pointer
     * to desired internal address.  See X24C08 desc of "random reads".
     */
    if (((status = eeprom_send_start (which_eeprom)) != OK) ||
	((status = eeprom_send_byte (which_eeprom, &slave_addr_write))!= OK) ||
	((status = eeprom_send_byte (which_eeprom, &eeprom_low_addr)) != OK) ||

    /* Send another start to begin the read */
	((status = eeprom_send_start (which_eeprom)) != OK) ||
	((status = eeprom_send_byte (which_eeprom, &slave_addr_read)) != OK))
	return (status);

    /* Read in desired number of bytes minus one, sending ACK after each */
    for (i = 0; i < (nbytes - 1); i++)
	if ((status = eeprom_get_byte_w_ack (which_eeprom, p_data++)) != OK)
	    return (status);

    /* Get last byte without issuing ACK */
    if ((status = eeprom_get_byte_no_ack (which_eeprom, p_data++)) != OK)
	return (status);

    /* Send stop condition to terminate read */
    if ((status = eeprom_send_stop (which_eeprom)) != OK)
	return (status);
    else
	return (OK);
}

/*-------------------------------------------------------------
 * Function:	int eeprom_write ()
 *
 * Action:	Write the given data to the eeprom.
 *
 * Returns:	OK if write worked, EEPROM_NOT_RESPONDING if
 *		write fails.
 *-------------------------------------------------------------*/
int eeprom_write (int which_eeprom,	/* which one to write-see eeprom.h */
		  int eeprom_addr,	/* byte offset from start of eeprom */
		  unsigned char *p_data,		/* where to get data from memory */
		  int nbytes		/* number of bytes to write */
		  )
{
    int status;				/* result code from page writes */
    int next_page_boundary;		/* Start addr of next page */
    int bytes_to_next_page;		/* # of bytes to get to next page */

    /* Do one-time initialization */
    if (!powerup_wait_done) {
	init_timer3 ();
	eeprom_delay (POWER_UP_DELAY);
	powerup_wait_done = 1;
    }

    /* Ignore trivial write of < 0 bytes */
    if (nbytes <= 0)
	return (OK);

    /*
     * Make sure caller isn't requesting a write beyond the end of the 
     * eeprom.  In this case, the X24C08 will wrap around to the
     * beginning of the eeprom, which is most likely not what the user
     * wants.
     */
    if ((eeprom_addr + nbytes) > EEPROM_SIZE)
	return (EEPROM_TO_SMALL);

    /*
     * Write to eeprom.  Use page writes to send as much data as
     * possible to the eeprom at once.  First use a page write of
     * <= EEPROM_PAGE_SIZE to get to the next page boundary in
     * the eeprom, then do full page writes of EEPROM_PAGE_SIZE,
     * and finish with a smaller write at the end.
     */

    next_page_boundary = eeprom_addr / EEPROM_PAGE_SIZE; /* Page # for addr */
    next_page_boundary += 1;				 /* Go to next page */
    next_page_boundary *= EEPROM_PAGE_SIZE;		 /* Page start addr */

    bytes_to_next_page = next_page_boundary - eeprom_addr;

    /*
     * If the user's write size isn't enough to cross over to next page
     * in the eeprom (and therefore, nbytes <= EEPROM_PAGE_SIZE), just
     * do the nbytes page write and return.
     */
    if (nbytes <= bytes_to_next_page)
	return (eeprom_page_write (which_eeprom, eeprom_addr, p_data, nbytes));

    /*
     * Otherwise, do a page write of enough data to bring us to a
     * page boundary in the eeprom, then do full pages until less
     * then a whole page is left.
     */

    if ((status = eeprom_page_write (which_eeprom, eeprom_addr, p_data,
				     bytes_to_next_page)) != OK)
	return (status);
    eeprom_addr  += bytes_to_next_page;		/* Update pointers & count */
    p_data       += bytes_to_next_page;
    nbytes       -= bytes_to_next_page;

    while (nbytes > EEPROM_PAGE_SIZE) {
	if ((status = eeprom_page_write (which_eeprom, eeprom_addr, p_data,
					 EEPROM_PAGE_SIZE)) != OK)
	    return (status);
	eeprom_addr  += EEPROM_PAGE_SIZE;	/* Update pointers & count */
	p_data       += EEPROM_PAGE_SIZE;
	nbytes       -= EEPROM_PAGE_SIZE;
    }

    /* Write the last chunk */
    if ((status = eeprom_page_write (which_eeprom, eeprom_addr, p_data,
				     nbytes)) != OK)
	return (status);
    else
	return (OK);
}

/*-------------------------------------------------------------
 * Function:	int eeprom_installed ()
 *
 * Action:	Checks to see if requested eeprom is installed.
 *
 * Returns:	0 if eeprom doesn't respond, non-0 if it does.
 *-------------------------------------------------------------*/
int eeprom_installed (int which_eeprom)
{
    int junk;				/* dummy address */

    if (eeprom_read (which_eeprom, 0, (void *) &junk, 1) == OK)
	return (1);
    else
	return (0);
}

/*-------------------------------------------------------------
 * Private routines, used only within this file
 *-------------------------------------------------------------*/

/*-------------------------------------------------------------
 * Function:	void init_timer3 ()
 *
 * Action:	Sets up CIO timer 3 to be used for eeprom delays.
 *
 * Returns:	N/A.
 *
 * Note:	This routine must be called before using the
 *		eeprom.
 *-------------------------------------------------------------*/
void init_timer3 ()
{
	static unsigned char cmds[] = {
	    /*	reg#	value						*/
	    /*	-----	-----						*/
	    /*	Configure CIO timer 3					*/
		0x0c,	0xe4,	/* Cmd/Stat - clr IE, gate on		*/
		0x0c,	0x24,	/* Cmd/Stat - clr IP/IUS, gate on	*/
		0x1e,	0x00,	/* Sing cyc, no ext pins/retrig, pulse	*/
		0x04,	0x00	/* Somthing for int vector (not used)	*/
	};
	int i;
	volatile unsigned char junk;

	/* Reset CIO state machine */
	junk = CIO->ctrl;	/* Put CIO in reset state or state 0 */
	CIO->ctrl = 0;		/* Put CIO state 0 or 1 */
	junk = CIO->ctrl;	/* Put CIO in state 0 */

	for ( i = 0; i < sizeof(cmds); i++ ){
		CIO->ctrl = cmds[i];
	}
}

/*-------------------------------------------------------------
 * Function:	void eeprom_delay ()
 *
 * Action:	Uses CIO timer 3 to implement eeprom delays.
 *
 * Returns:	N/A.
 *
 * Note:	This routine polls the CIO for completion; no
 *		interrupts are used.
 *-------------------------------------------------------------*/
void eeprom_delay (int usec)
{
    /*
     * Calculate timer's time constant as follows:
     *
     * "CIO_CLK" PCLK ticks   1 timer tick      1 sec       X usec
     * -------------------- x ------------ x ------------ x ------
     *        1 sec           2 PCLK ticks   1000000 usec
     *
     * Result is number of timer ticks to get the desired number of
     * microseconds.  This number is calculated as a long, but the
     * CIO uses a 16-bit time constant.  If the result is greater
     * than 65535, we will call eeprom_delay twice recursively,
     * with half the delay time.  A time constant of 65535 is
     * almost 32 milliseconds, which, in practice, as a longer time
     * than we will ever need.  CIO_CLK is #define'ed in cvme964.h.
     *
     * (Note: Timers are clocked at a rate 1/2 of PCLK. See CIO Manual)
     */
    unsigned long num_ticks;			/* number of ticks needed */
    volatile unsigned char cmd_stat_reg;/* for reading/writing cmd/stat reg */

    num_ticks = (unsigned long) (((unsigned long)CIO_CLK / (unsigned long)2000000) * usec);

    /* If the count is greater than 16 bits, wait 2X, 1/2 each time */
    if (num_ticks > 0x0ffff) {
	eeprom_delay (usec / 2);
	eeprom_delay (usec - (usec / 2)); /* In case usec is odd */
	return;
    }

    /* Load time constant into Conter/timer 3's TC registers */
    CIO->ctrl = 0x1a;		/* Timer 3 time constant MSB */
    CIO->ctrl = (num_ticks >> 8) & 0x0ff;
    CIO->ctrl = 0x1b;		/* Timer 3 time constant LSB */
    CIO->ctrl = num_ticks & 0x0ff;

    /* Load time constant into Conter/timer 3's TC registers */
    CIO->ctrl = 0x0c;		/* Timer 3 command and status */
    cmd_stat_reg = CIO->ctrl;	/* Get contents */

    cmd_stat_reg = cmd_stat_reg | 0x02;	/* Set trigger bit */

    /* Start timer */
    CIO->ctrl = 0x0c;		/* Timer 3 command and status */
    CIO->ctrl = cmd_stat_reg;	/* Start counting */

    /* Wait for count done (count-in-progress bit cleared) */
    CIO->ctrl = 0x0c;		/* Timer 3 command and status */
    cmd_stat_reg = CIO->ctrl;	/* Get contents */

    while (cmd_stat_reg & 0x01) {
	CIO->ctrl = 0x0c;	/* re-read timer 3 cmnd/stat */
	cmd_stat_reg = CIO->ctrl;
    }
}

/*-------------------------------------------------------------
 * Function:	int eeprom_page_write ()
 *
 * Action:	Issues a page write to the eeprom (16 bytes max)
 *
 * Returns:	Result of write.
 *-------------------------------------------------------------*/
static int eeprom_page_write (int which_eeprom,	/* one of the define's above */
			      int eeprom_addr,	/* byte offset from start */
			      unsigned char *p_data,	/* data from memory */
			      int nbytes)	/* num of bytes (0<x<=16) */
{
    int status;				/* result code from page writes */
    unsigned char slave_addr;		/* slave address byte for write */
    unsigned char eeprom_high_addr;	/* Bits 8 and 9 of eeprom address */
    unsigned char eeprom_low_addr;	/* Bits 0-7 of eeprom address */
    int i;				/* loop variable */

    /* Ignore trivial writes of < 0 bytes */
    if (nbytes <= 0)
	return (OK);

    /*
     * Make sure caller isn't requesting a write beyond the end of the 
     * eeprom.  In this case, the X24C08 will wrap around to the
     * beginning of the eeprom, which is most likely not what the user
     * wants.
     */
    if (nbytes > EEPROM_PAGE_SIZE)
	return (EEPROM_ERROR);

    /*
     * Break up eeprom address - bits 8 and 9 in slave address byte,
     * bits 0-7 in next byte
     */
    eeprom_high_addr = (eeprom_addr & UPPER_2_BITS_MASK)>>UPPER_2_BITS_OFFSET;
    eeprom_low_addr = eeprom_addr & LOWER_8_BITS_MASK;

    slave_addr = SLAVE_ADDR_WRITE | eeprom_high_addr;

    /* Send start condition to begin the write */
    if (((status = eeprom_send_start (which_eeprom)) != OK) ||
	((status = eeprom_send_byte (which_eeprom, &slave_addr)) != OK) ||
	((status = eeprom_send_byte (which_eeprom, &eeprom_low_addr)) != OK))
	return (status);

    /* Send the desired number of bytes */
    for (i = 0; i < nbytes; i++)
	if ((status = eeprom_send_byte (which_eeprom, p_data++)) != OK)
	    return (status);

    /* Send stop condition to terminate write */
    if ((status = eeprom_send_stop (which_eeprom)) != OK)
	return (status);

    /* Now wait for eeprom's internal write cycle to complete */
    eeprom_delay (WRITE_DELAY);

    return (OK);
}

/*-------------------------------------------------------------
 * Function:	int eeprom_send_start ()
 *
 * Action:	Issues a start condition to the eeprom.
 *
 * Returns:	OK.
 *-------------------------------------------------------------*/
static int eeprom_send_start (int which_eeprom)
{
    /*
     * Start condition is defined as SDA going low when SCL is high.
     * First we have to be sure SDA is high, so it can go low.  Before
     * driving SDA high, we set SCL low, so we don't accidentally cause
     * a stop condition if SCL was high when we drove SDA high.
     */
    set_scl_line (which_eeprom, LOW);
    eeprom_delay (DATA_IN_HOLD_TIME);
    set_sda_line (which_eeprom, HIGH);
    eeprom_delay (DATA_IN_SETUP_TIME);
    eeprom_delay (SCL_LOW_PERIOD);

    /*
     * Now that the two lines are in the proper states, cause the start
     * condition.  Pay attention to setup and hold times.
     */
    set_scl_line (which_eeprom, HIGH);
    eeprom_delay (START_SETUP_TIME);
    set_sda_line (which_eeprom, LOW);
    eeprom_delay (START_HOLD_TIME);

    /* Set clock line low for next operation, let data float back high. */
    set_scl_line (which_eeprom, LOW);
    eeprom_delay (DATA_IN_HOLD_TIME);
    set_sda_line (which_eeprom, HIGH);
    eeprom_delay (DATA_IN_SETUP_TIME);
    eeprom_delay (SCL_LOW_PERIOD);

    return (OK);
}

/*-------------------------------------------------------------
 * Function:	int eeprom_send_stop ()
 *
 * Action:	Issues a stop condition to the eeprom.
 *
 * Returns:	OK.
 *-------------------------------------------------------------*/
static int eeprom_send_stop (int which_eeprom)
{
    /*
     * Stop condition is defined as SDA going high when SCL is high.
     * First we have to be sure SDA is low, so it can go high.  Before
     * driving SDA low, we set SCL low, so we don't accidentally cause
     * a start condition if SCL was high when we drove SDA low.
     */
    set_scl_line (which_eeprom, LOW);
    eeprom_delay (DATA_IN_HOLD_TIME);
    set_sda_line (which_eeprom, LOW);
    eeprom_delay (DATA_IN_SETUP_TIME);
    eeprom_delay (SCL_LOW_PERIOD);

    /*
     * Now that the two lines are in the proper states, cause the stop
     * condition.  Pay attention to setup and hold times.
     */
    set_scl_line (which_eeprom, HIGH);
    eeprom_delay (STOP_SETUP_TIME);
    set_sda_line (which_eeprom, HIGH);
    eeprom_delay (BUS_FREE);

    /* Set clock line low for next operation, leave data floating high. */
    set_scl_line (which_eeprom, LOW);
    eeprom_delay (SCL_LOW_PERIOD);

    return (OK);
}

/*-------------------------------------------------------------
 * Function:	int eeprom_send_byte ()
 *
 * Action:	Sends a byte to the eeprom.  Bit 7 (MSB) is sent
 *		first, bit 0 (LSB) is sent last.
 *
 * Returns:	OK, or EEPROM_NOT_RESPONDING if eeprom doesn't
 *		send back an ACK bit after byte.
 *-------------------------------------------------------------*/
static int eeprom_send_byte (int which_eeprom,
			     unsigned char *p_data)
{
    int i;				/* loop variable */
    int status;				/* get_sda_line return value */

    set_scl_line (which_eeprom, LOW);	/* SCL low during SDA change */
    eeprom_delay (DATA_IN_HOLD_TIME);	/* Hold time before data changes */

    /* Do each bit, MSB => LSB */
    for (i = 7; i >= 0; i--) {

	/* If this bit is a 1, set SDA high.  If 0, set it low */
	if (*p_data & (1 << i))
	    set_sda_line (which_eeprom, HIGH);
	else
	    set_sda_line (which_eeprom, LOW);

	eeprom_delay (DATA_IN_SETUP_TIME); /* Data setup before raising SCL */
	eeprom_delay (SCL_LOW_PERIOD);     /* Wait minimum SCL low period */

	set_scl_line (which_eeprom, HIGH); /* Clock in this data bit */

	eeprom_delay (SCL_HIGH_PERIOD);    /* Wait minimum SCL high period */

	set_scl_line (which_eeprom, LOW);  /* Prepare for next bit */

	eeprom_delay (DATA_IN_HOLD_TIME);  /* Wait before changing SDA */
    }

    /* Now look for ACK from eeprom */
    eeprom_delay (SCL_LOW_PERIOD);	/* Wait for eeprom to put out ACK */
    set_sda_line (which_eeprom, HIGH);	/* Let open drain line to float high */
    eeprom_delay (SCL_LOW_PERIOD);	/* Wait for line to go up */

    status = get_sda_line (which_eeprom); /* Read in ACK bit */

    set_scl_line (which_eeprom, HIGH);	/* Finish this cycle */
    eeprom_delay (SCL_HIGH_PERIOD);	/* Wait minimum SCL high period */
    set_scl_line (which_eeprom, LOW);	/* Let eeprom release bus */

    if (status == LOW)
	return (OK);
    else
	return (EEPROM_NOT_RESPONDING);
}

/*-------------------------------------------------------------
 * Function:	int eeprom_get_byte_w_ack ()
 *
 * Action:	Receives a byte from the eeprom, sends an ACK
 *		bit after byte is received.
 *
 * Returns:	OK.
 *-------------------------------------------------------------*/
static int eeprom_get_byte_w_ack (int which_eeprom,
				  unsigned char *p_data)
{
    int i;				/* loop variable */

    *p_data = 0x00;			/* "prime" data byte for or's */

    set_scl_line (which_eeprom, LOW);	/* SCL low during SDA change */
    eeprom_delay (SCL_LOW_PERIOD);	/* Wait min SCL low, then read SDA */

    /* Do each bit, MSB => LSB */
    for (i = 7; i >= 0; i--) {

	/* If SDA is high, set this bit.  All bits were preset to 0 above. */
	if (get_sda_line (which_eeprom) == HIGH)
	    *p_data |= (1 << i);

	/* Finish this half of the cycle */
	set_scl_line (which_eeprom, HIGH);
	eeprom_delay (SCL_HIGH_PERIOD);

	/* Start next bit */
	set_scl_line (which_eeprom, LOW);
	eeprom_delay (SCL_LOW_PERIOD);
    }

    /*
     * At this point, the eighth bit has been sent to the eeprom and
     * SCL has been low for the required clock period.  Now send the
     * ACK bit.  Set SDA low, wait the data setup time, raise SCL, and
     * clock the bit in.
     */
    eeprom_delay (DATA_IN_HOLD_TIME);	/* hold time before changing SDA */
    set_sda_line (which_eeprom, LOW);	/* Drive SDA low for ACK bit */

    eeprom_delay (DATA_IN_SETUP_TIME);	/* Data setup before raising SCL */

    set_scl_line (which_eeprom, HIGH);	/* Finish this cycle */
    eeprom_delay (SCL_HIGH_PERIOD);	/* Wait minimum SCL high period */

    set_scl_line (which_eeprom, LOW);

    return (OK);
}

/*-------------------------------------------------------------
 * Function:	int eeprom_get_byte_no_ack ()
 *
 * Action:	Receives a byte from the eeprom without sending
 *		an ACK bit after byte is received.
 *
 * Returns:	OK.
 *-------------------------------------------------------------*/
static int eeprom_get_byte_no_ack (int which_eeprom,
				   unsigned char *p_data)
{
    int i;				/* loop variable */

    *p_data = 0x00;			/* "prime" data byte for or's */

    set_scl_line (which_eeprom, LOW);	/* SCL low during SDA change */
    eeprom_delay (SCL_LOW_PERIOD);	/* Wait min SCL low, then read SDA */

    /* Do each bit, MSB => LSB */
    for (i = 7; i >= 0; i--) {

	/* If SDA is high, set this bit.  All bits were preset to 0 above. */
	if (get_sda_line (which_eeprom) == HIGH)
	    *p_data |= (1 << i);

	/* Finish this half of the cycle */
	set_scl_line (which_eeprom, HIGH);
	eeprom_delay (SCL_HIGH_PERIOD);

	/* Start next bit */
	set_scl_line (which_eeprom, LOW);
	eeprom_delay (SCL_LOW_PERIOD);
    }

    /*
     * At this point, the eighth bit has been sent to the eeprom and
     * SCL has been low for the required clock period.  Now send a
     * 1 bit, indicating no ACK.
     */
    eeprom_delay (DATA_IN_HOLD_TIME);	/* hold time before changing SDA */
    set_sda_line (which_eeprom, HIGH);	/* SDA HIGH == no ACK to eeprom */

    eeprom_delay (DATA_IN_SETUP_TIME);	/* Data setup before raising SCL */

    set_scl_line (which_eeprom, HIGH);	/* Finish this cycle */
    eeprom_delay (SCL_HIGH_PERIOD);	/* Wait minimum SCL high period */

    set_scl_line (which_eeprom, LOW);

    return (OK);
}

/*-------------------------------------------------------------
 * Function:	void set_scl_line ()
 *
 * Action:	Sets the value of the eeprom's serial clock line
 *		to the value HIGH or LOW.
 *
 * Returns:	N/A.
 *-------------------------------------------------------------*/
static void set_scl_line (int which_eeprom,	/* which eeprom to use */
			  int state)		/* HIGH or LOW */
{
    switch (which_eeprom) {
    case SQUALL_EEPROM:
	if (state == HIGH)
	    SQUALL_CLOCK_REG |= (1 << SQUALL_CLOCK_OFFSET);
	else
	    SQUALL_CLOCK_REG &= ~(1 << SQUALL_CLOCK_OFFSET);
	break;

    case ON_BOARD_EEPROM:
	if (state == HIGH)
	    ON_BOARD_CLOCK_REG |= (1 << ON_BOARD_CLOCK_OFFSET);
	else
	    ON_BOARD_CLOCK_REG &= ~(1 << ON_BOARD_CLOCK_OFFSET);
	break;

    default:
	break;
    }
}

/*-------------------------------------------------------------
 * Function:	void set_sda_line ()
 *
 * Action:	Sets the value of the eeprom's serial data line
 *		to the value HIGH or LOW.
 *
 * Returns:	N/A.
 *-------------------------------------------------------------*/
static void set_sda_line (int which_eeprom,	/* which eeprom to use */
			  int state)		/* HIGH or LOW */
{
    /*
     * The serial data lines are configured as open collector
     * outputs, by default.
     */
    switch (which_eeprom) {
    case SQUALL_EEPROM:
	if (state == HIGH)
	    SQUALL_DATA_REG |= (1 << SQUALL_DATA_OFFSET);
	else
	    SQUALL_DATA_REG &= ~(1 << SQUALL_DATA_OFFSET);
	break;

    case ON_BOARD_EEPROM:
	if (state == HIGH)
	    ON_BOARD_DATA_REG |= (1 << ON_BOARD_DATA_OFFSET);
	else
	    ON_BOARD_DATA_REG &= ~(1 << ON_BOARD_DATA_OFFSET);
	break;

    default:
	break;
    }
}

/*-------------------------------------------------------------
 * Function:	int get_sda_line ()
 *
 * Action:	Returns the value of the eeprom's serial data line
 *
 * Returns:	HIGH or LOW.
 *-------------------------------------------------------------*/
static int get_sda_line (int which_eeprom)	/* which eeprom to use */
{
    unsigned char dir_reg;			/* value of the data dir reg */
    int ret_val;				/* result code */

    /*
     * The serial data lines are configured as open collector
     * outputs, by default.
     */
    switch (which_eeprom) {
    case SQUALL_EEPROM:
	CIO->ctrl = SQUALL_DATA_DIR_REG;	/* get current value of reg */
	dir_reg = CIO->ctrl;
	CIO->ctrl = SQUALL_DATA_DIR_REG;	/* set bit == input */
	CIO->ctrl = dir_reg | (1 << SQUALL_DATA_OFFSET);

	/* Clear bit in data reg to clear 1's catcher (spec ctrl bit is set) */
	SQUALL_DATA_REG &= ~(1 << SQUALL_DATA_OFFSET);

	if (SQUALL_DATA_REG & (1 << SQUALL_DATA_OFFSET))
	    ret_val = HIGH;
	else
	    ret_val = LOW;

	CIO->ctrl = SQUALL_DATA_DIR_REG;	/* restore register contents */
	CIO->ctrl = dir_reg;
	break;

    case ON_BOARD_EEPROM:
	CIO->ctrl = ON_BOARD_DATA_DIR_REG;	/* get current value of reg */
	dir_reg = CIO->ctrl;
	CIO->ctrl = ON_BOARD_DATA_DIR_REG;	/* set bit == input */
	CIO->ctrl = dir_reg | (1 << ON_BOARD_DATA_OFFSET);

	/* Clear bit in data reg to clear 1's catcher (spec ctrl bit is set) */
	ON_BOARD_DATA_REG &= ~(1 << ON_BOARD_DATA_OFFSET);

	if (ON_BOARD_DATA_REG & (1 << ON_BOARD_DATA_OFFSET))
	    ret_val = HIGH;
	else
	    ret_val = LOW;

	CIO->ctrl = ON_BOARD_DATA_DIR_REG;	/* restore register contents */
	CIO->ctrl = dir_reg;
	break;

    default:
	ret_val = LOW;		/* actually an error condition */
	break;
    }

    return (ret_val);
}
