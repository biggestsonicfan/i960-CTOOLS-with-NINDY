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
 * Gcc 80960 Profiler, QT Specific File
 *
 * The functions in this file provide board specific runtime support
 * for the Gnu 80960 profiler. These functions provide timer setup
 * and interrupt service functions.
 *
 *********************************************************************/


	/************************
	 *			*
	 *    INCLUDE FILES    	*
	 *			*
	 ************************/

#include "qtcommon.h"


	/************************
	 *			*
	 * FORWARD DECLARATIONS *
	 *			*
	 ************************/

extern	unsigned int   profile_isr();
extern	unsigned short set_p_timer();


	/************************
	 *			*
	 *      CONSTANTS 	*
	 *			*
	 ************************/

#define TIMER_VECTOR 	255


	/************************
	 *			*
	 *     TYPE DEFINES	*
	 *			*
	 ************************/

typedef struct {
	unsigned short 	field2;
	unsigned char 	field1;
	unsigned char	message_type;
	unsigned int	field3;
	unsigned int	field4;
	unsigned int	field5;
} iac_struct;



/*********************************************************************
 *
 * NAME
 *	init_int_vector - modify interrupt vector table
 *
 * DESCRIPTION
 *	This function writes the address of the profiler's interrupt
 *	service routine to the interrupt vector table. The convoluted
 *	path to the vector table involves an IAC to obtain the
 *	PRCB, which is indexed into to get the vector table. See
 *	below for more details.
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
	iac_struct	iac;		/* inter-agent comm. structure */
	unsigned int	*int_table;	/* interrupt vector table */
	unsigned int	*prcb;		/* process control block */
	unsigned int	system_base[2]; /* place to store prcb */

	/*
	 * We need to locate the interrupt table in order to 
	 * enter our new vector into the table.  We will do this
	 * by first finding the PRCB (Processor control block) base
	 * and then indexing into it to find the interrupt table.
	 * Once we have the interrupt table, we can index into it
	 * and put the vector into the appropriate spot.
	 * Set the IAC message type and store the system base
	 * address.
	 */
	iac.message_type = 0x80;
	iac.field3 = (unsigned int) system_base;

	/*
	 * Issue the IAC message.
	 */
	send_iac((int) &iac);

	/*
	 * Move PRCB to prcb pointer to obtain the interrupt
	 * vector table. Then store our own timer interrupt service
	 * routine address in the table.
	 */
	prcb = (unsigned int *) system_base[1];	
	int_table = (unsigned int *) prcb[5]; 
	int_table[TIMER_VECTOR + 1] = (unsigned int) profile_isr;
}
	


/*********************************************************************
 *
 * NAME
 *	init_p_timer - set timer interrupt frequency
 *
 * DESCRIPTION
 *	This function is basically a replica of the timer file
 *	function init_p_timer, with the difference that the
 *	timer interrupt frequency is selectable via parameter.
 *
 * PARAMETERS
 *	Frequency of timer interrupts.
 *
 * RETURNS
 *	Void.
 *
 *********************************************************************/

void
init_p_timer(frequency)

int frequency;

{
	short	t_count;	/* timer count in ticks */

	/*
	 * Initialize counter/timer interrupts. Use the timer
	 * library function.
	 */
	init_interrupts();

	/*
	 * First freeze timers 0, 1, and 2 to prevent them from
	 * ticking.
	 */
	store_byte(0x00, CWR1);
	store_byte(0x40, CWR1);
	store_byte(0x80, CWR1);
	
	/*
	 * Turn on timer 3 for clock, mode 2. Then compute and store
	 * time constant for the clock, in two separate byte store
	 * operations.
	 */
	store_byte (0x34, CWR2);
	t_count = set_p_timer(frequency);
	store_byte(((int)(t_count & 0xff)), CR3);
	store_byte(((int)((t_count & 0xff00) >> 8)), CR3);
	
	/*
	 * Enable the timer to cause interrupt.
	 */
	store_byte(0xfe, OCW1_A);
	store_byte(0xff, OCW1_B);
	store_byte(0x7f, CNTL1);

	/*
	 * Lower interrupt priority.
	 */
	lower_priority(0);
}



/*********************************************************************
 *
 * NAME
 *	set_p_timer - calculate the timer rate
 *
 * DESCRIPTION
 *	Calculate and return the counter amount necessary for the
 *	desired	interrupt rate. Default is one millisecond.
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
		return(4915);
	case 2:
		return(9830);
	case 3:
		return(19660);
	case 4:
		return(49150);
	default:
		return(9830);
	};
}



/*********************************************************************
 *
 * NAME
 *	term_p_timer - terminate timer interrupts
 *
 * DESCRIPTION
 *	This function is just a dummy entry point for the QT
 *	specific term_bentime function which disables timer
 *	interrupts.
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
	term_bentime();
}
