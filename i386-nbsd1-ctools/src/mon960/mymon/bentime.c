/*******************************************************************************
 * 
 * Copyright (c) 1993, 1995 Intel Corporation
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

#include "mon960.h"
#include "retarget.h"
#include "this_hw.h"

/*
 * As it turns out, INIT_COUNT has some very fascinating restrictions that
 * must be complied with, or bentime results will be erratic.
 *
 * 1) The low order 16 bits should be the largest practical value to  
 *    ensure proper (i.e., non-erratic) operation of the 85c36 timer.  A
 *    value of 0 or 0xffff is proper, but 0 is not acceptable.
 *
 * 2) For all supported Mon960 timers, a low order 16-bit value of 0 
 *    causes 1/2 of a 32-bit counter to configure itself for the maximum
 *    16-bit count (65,536).  Given #1 above, this sounds perfect, but it's
 *    quite incorrect.  Reason, if/when a bentime interrupt occurs,
 *    all subsequent bentime() reads use computations that rely on knowing
 *    the original bentime INIT_COUNT.  And using 0 in the lower half
 *    of a 32-bit constant to represent 65,536 just doesn't work when
 *    the bentime() routines perform integer arithmetic with this value.
 *    Sooooo, the correct value to use for the lower 16-bits is 0xffff.
 *
 * 3) A "good" value for the upper 16 bits has been empirically determined
 *    to be 50000 decimal.
 */
#define INIT_COUNT  0xc350ffff

#define ROLL_32_BITS 0xffffffff

static initial_bentime_priority;

/************************************************/
/* BENTIME                    */
/*                         */
/* This routine reads the counter values and    */
/* returns the uS value.            */
/************************************************/
unsigned int bentime()
{
    return(timer_read(get_bentime_timer()));
}

/************************************************/
/* INIT_BENTIME                    */
/*                         */
/* This routine initializes the CA    interrupt */
/* and 8254  registers and initializes the timer*/
/* count. It returns the overhead for the     */
/* bentime() call.                */
/************************************************/
unsigned int init_bentime(int x)
           /* "x" not used for this timer, but kept for compatability */
           /* with the EVA init_bentime routine */
           /* no use to time on board time or clock time */
{
    return(timer_init(get_bentime_timer(), 
                      INIT_COUNT, 
                      get_default_bentime_isr(),
                      &initial_bentime_priority));
}
    
/************************************************/
/* TERM_BENTIME                    */
/*                         */
/* This routine turns off all counters        */
/************************************************/
void
term_bentime()
{
    timer_term(get_bentime_timer());
	if (initial_bentime_priority != 0xffffffff)
		(void) change_priority(initial_bentime_priority);
}

/************************************************/
/*                                              */
/* noint_timer.c  benchmark timing functions for*/
/*              the EV80960CA                   */
/*                                              */
/* NOTE:                                        */
/*   counters 0 and 1 are used to form a 32     */
/* bit timer.  The 82C54 counter rolls over     */
/* every seven minutes.  Calls to bentime()     */
/* that are over 7 minutes apart will be        */
/* inaccurate.                                  */
/************************************************/


static unsigned long old_count; /* initialized in init_bentime_noint */
static unsigned long tot_time; /* initialized in init_bentime_noint */

/************************************************/
/* Function:   long bentime_noint(void)         */
/*                                              */
/* Passed:     void.                            */
/*                                              */
/* Returns:    The current timer value, in      */
/*             microseconds, of the free running*/
/*             timer.                           */
/*                                              */
/* Action:     Reads the timer. Converts the    */
/*             time to microseconds.            */
/************************************************/
unsigned long
bentime_noint()
{
    unsigned long new_count;

    new_count = timer_read(get_bentime_timer());   /* read timer 0 value */

    /* take the difference to simulate a count up timer   */
    /* check for a 32 bit roll over                       */
    /* Note:  the division is necessary because tot_time  */
    /* stores microseconds, while old_count and new_count */
    /* store timer ticks ( at CRYSTALTIME).       */

    if(new_count < old_count)
        tot_time += ((ROLL_32_BITS - old_count) + new_count);
    else
        tot_time += (new_count - old_count);

    old_count = new_count;

    return(tot_time);
}


/************************************************/
/* Function:   long init_bentime_noint(int Mhz) */
/*                                              */
/* Passed: UNUSED. int Mhz;                     */
/*         ASSUMED Passed the frequency of the  */
/*         IS      counter's oscillator, in Mhz.*/
/*         CRYSTALTIME                          */
/*                                              */
/* Returns:    The overhead value to call       */
/*             bentime_noint() in a long integer*/
/*                                              */
/* Action:     Initializes free-running timer.  */
/************************************************/
unsigned long
init_bentime_noint(int Mhz)
{
    unsigned long overhd;

    timer_init(get_bentime_timer(), ROLL_32_BITS, NULL, NULL);

    tot_time = 0;
    old_count = 0;

    overhd = bentime_noint(); /*needed delay for cyclone boards to ger timers sync */
    overhd = bentime_noint();
    overhd = bentime_noint() - overhd;
	initial_bentime_priority = 0xffffffff;
    return(overhd);
}


/************************************************/
/* Function:   bentime_onboard_only */
/*                                              */
/* Passed: UNUSED. int Mhz;                     */
/*         ASSUMED Passed the frequency of the  */
/*         IS      counter's oscillator, in Mhz.*/
/*         CRYSTALTIME                          */
/*                                              */
/* Returns:    The overhead value to call       */
/*             bentime_noint() in a long integer*/
/*                                              */
/* Action:     Initializes free-running timer.  */
/************************************************/
void
bentime_onboard_only(int onboard_only)
{
	if (onboard_only == TRUE)
	   timer_for_onboard_only(TRUE);
	else
	   timer_for_onboard_only(FALSE);
}


/************************************************/
/* Function:   bentime_isr */
/*                                              */
/* Passed:     NONE.                      */
/*                                              */
/* Returns:    NONE                                */
/*                                              */
/* Action:     Handles bentime interrupts.  */
/************************************************/
void
_bentime_isr()
{
    _bentime_counter += 1;
    clear_pending(get_timer_irq(get_bentime_timer()));
}
