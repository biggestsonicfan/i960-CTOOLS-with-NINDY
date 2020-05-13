/*********************************************************************
 *
 * 		Copyright (c) 1989, Intel Corporation
 *
 * Intel hereby grants you permission to copy, modify, and 
 * distribute this software and its documentation.  Intel grants
 * this permission provided that the above copyright notice 
 * appears in all copies and that both the copyright notice and
 * this permission notice appear in supporting documentation.  In
 * addition, Intel grants this permission provided that you
 * prominently mark as not part of the original any modifications
 * made to this software or documentation, and that the name of 
 * Intel Corporation not be used in advertising or publicity 
 * pertaining to distribution of the software or the documentation 
 * without specific, written prior permission.  
 *
 * Intel Corporation does not warrant, guarantee or make any 
 * representations regarding the use of, or the results of the use
 * of, the software and documentation in terms of correctness, 
 * accuracy, reliability, currentness, or otherwise; and you rely
 * on the software, documentation and results solely at your own 
 * risk.
 *
 *********************************************************************/

/*********************************************************************
 *
 * Gcc 80960 Profiler, EVCA-960 Specific File
 *
 * The functions in this file provide board specific runtime support
 * for the Gnu 80960 profiler. These functions provide timer setup
 * and interrupt service functions.
 *
 *********************************************************************/


	/************************
	 *			*
	 *    INCLUDE FILES     *
	 *			*
	 ************************/

#include "t82c54.h"


	/************************
	 *			*
	 * FORWARD DECLARATIONS *
	 *			*
	 ************************/

extern unsigned short set_p_timer();
extern unsigned int   profile_isr();


	/************************
	 *			*
	 *      CONSTANTS       *
	 *			*
	 ************************/

#define TIMER_VECTOR 226
static volatile unsigned char *timer_0  = ((unsigned char *) 0xDFE00000);
static volatile unsigned char *timer_1  = ((unsigned char *) 0xDFE00001);
static volatile unsigned char *timer_2  = ((unsigned char *) 0xDFE00002);
static volatile unsigned char *timer_cw = ((unsigned char *) 0xDFE00003);



/*********************************************************************
 *
 * NAME
 *	init_int_vector - modify interrupt vector table
 *
 * DESCRIPTION
 *	This function writes the address of the profiler's interrupt
 *	service routine to the interrupt vector table. The convoluted
 *	path to the vector table involves obtaining the PRCB, which
 *	is indexed into to get the vector table. See below for more
 *	details.
 *
 * PARAMETERS
 *	None.
 *
 * RETURNS
 *	Void.
 *
 *********************************************************************/

void
init_int_vector()

{
	unsigned int	*prcb;		/* prcb base */
	unsigned int	*int_table;	/* interrupt table base */
	unsigned int	*control_t;	/* control table base */
	unsigned int	mask;		/* current interrupt mask state */

        /* We need to locate the interrupt table in order to
         * enter our new vector into the table.  We will do this
         * by first finding the PRCB (Processor control block) base
         * and then indexing into it to find the interrupt table.
         * Once we have the interrupt table, we can index into it
         * and put the vector into the appropriate spot.
	 * Start by getting a pointer to the PRCB.
	 */
        prcb = (unsigned int *) get_prcb();

        /*
	 * Get the interrupt vector table and add our timer
	 * interrupt service routine address.
	 */
        int_table = (unsigned int *) prcb[4];
        int_table[TIMER_VECTOR + 1] = (unsigned int) profile_isr;

	/*
	 * Get a pointer to the control table. Alter the IMAP
	 * and ICON registers to what the Heurikon manual suggests,
	 * then perform a sysctl command to force the register update.
	 */
        control_t = (unsigned int *) prcb[1];
	control_t[4] = 0xE000;
	control_t[5] = 0x0000;
	control_t[6] = 0x0000;
	control_t[7] = 0x03FC;
	send_sysctl(0x0401, 0, 0);

	/*
	 * Clear away all pending interrupts, modify the interrupt
	 * mask to allow CIO interrupts, and lower the interrupt priority.
	 */
	mask = get_mask();
	set_pending(get_pending() & 0xfffff000);
	set_mask(mask | 0x08); 
	change_priority(0);
}



/*********************************************************************
 *
 * NAME
 *	init_p_timer - initialize the timer hardware
 *
 * DESCRIPTION
 *	This function initializes the timer for operation.
 *	The function actually enabling the timer is called
 *	successively until we receive our first timer interrupt.
 *
 * PARAMETERS
 *	None.
 *
 * RETURNS
 *	Void.
 *
 *********************************************************************/

void
init_p_timer(frequency)

int frequency;

{
	unsigned short intr_freq;

	/*
	 * Call the function which translates the timer frequency
	 * parameter into an actual 82C54 timer count value.
	 */
	intr_freq = set_p_timer(frequency);

	/*
	 * Set the timer's control word for mode #2 operation.
	 * Then set timer #2's count value. LSB First.
	 */
	*timer_cw = 0xB6;
	*timer_2 = (intr_freq & 0x00FF);
	*timer_2 = ((intr_freq >> 8) & 0x00FF);
}



/*********************************************************************
 *
 * NAME
 *	set_p_timer - calculate the timer rate
 *
 * DESCRIPTION
 *	Calculate and return the counter amount necessary for the
 *	desired	interrupt rate. Default is one millisecond.
 *	Note that the EVCA timer clocks at 8 Mhz.
 *
 * PARAMETERS
 *	None.
 *
 * RETURNS
 *	Number of CIO ticks for desired rate.
 *
 *********************************************************************/

static unsigned short
set_p_timer(frequency)

int frequency;

{
	/*
	 * 1: 500 microseconds
	 * 2:   1 millisecond (default)
	 * 3:   2 milliseconds
	 * 4:   5 milliseconds
	 */
	switch(frequency) {
	case 1:
		return(4000);
	case 2:
		return(8000);
	case 3:
		return(16000);
	case 4:
		return(40000);
	default:
		return(8000);
	};
}



/*********************************************************************
 *
 * NAME
 *	term_p_timer - terminate the timer interrupts.
 *
 * DESCRIPTION
 *	This function disables the timer operation so we don't
 *	receive more interrupts.
 *
 * PARAMETERS
 *	None.
 *
 * RETURNS
 *	Void.
 *
 *********************************************************************/

void
term_p_timer()

{
	/*
	 * Set the timer's control word. Leave it hanging
	 * for counter data.
	 */
	*timer_cw = 0x0;
}
