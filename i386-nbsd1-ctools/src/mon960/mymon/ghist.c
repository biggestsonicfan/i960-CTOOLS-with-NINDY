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

/*********************************************************************
 * 80960 Profiler, 
 *
 * The functions in this file provide runtime support
 * for the 80960 profiler.  These functions provide timer setup
 * and interrupt service functions.
 *********************************************************************/

#include "mon960.h"
#include "this_hw.h"
#include "retarget.h"

static unsigned int initial_ghist_priority;

unsigned int ghist_count;

static GHIST_FNPTR ghist_callback;


/*********************************************************************
 *
 * NAME
 *	_generic_ghist_isr
 *
 * DESCRIPTION
 *	Simple isr to capture the IP of the current application and pass it
 *  to a callback routine that can record profile bucket data.
 *  Note that for certain timer HW (e.g., 85c36) this ISR is not 
 *  sufficient and must be overridden in the routine timer_init().
 *
 * PARAMETERS
 *	None.
 *
 * RETURNS
 *	Void.
 *
 *********************************************************************/
#pragma isr( _generic_ghist_isr )

static void 
_generic_ghist_isr(void)
{
    int running_rip;

#if KXSX_CPU
    /*
     * The Kx/Sx family apparently does not store an interrupted
     * application's current IP in the RIP.  Goodness knows why.  So
     * go fetch it from the stack and store it in g0.
     */
    asm volatile("flushreg");
    asm volatile("notand pfp, 0xf, r3");
    asm volatile("ld 8(r3), %0"  : "=d" (running_rip));
#else
    asm volatile("mov rip, %0"  : "=d" (running_rip));
#endif

    ghist_suspend_timer();
    ghist_callback(running_rip);
    ghist_reload_timer(ghist_count);

#if ! defined(HJ_TIMER)
    clear_pending(get_timer_irq(get_ghist_timer()));
#endif   /* Hx, Jx CPU timers are always edge-triggered. */
}



/*********************************************************************
 *
 * NAME
 *	_init_p_timer - initialize the timer hardware
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
_init_p_timer(int frequency)
{
	unsigned int intr_freq;

	intr_freq = 1000;

	if ((ghist_callback = get_ghist_callback()) == NULL)
	   return;

	/*
	 * 1: 500 microseconds
	 * 2:   1 millisecond (default)
	 * 3:   2 milliseconds
	 * 4:   5 milliseconds
	 */
	switch(frequency)
    	{
	    case 1:
	    	intr_freq = 500;
            break;
    	case 2:
    		intr_freq = 1000;
            break;
    	case 3:
    		intr_freq = 2000;
            break;
    	case 4:
    		intr_freq = 5000;
            break;
    	default:
    		intr_freq = 1000;
            break;
    	};

    ghist_count = intr_freq * 
#if defined(HJ_TIMER)
#   if HXJX_CPU
                                __cpu_bus_speed;
#   else
#       error "On chip timers REQUIRE a HX or JX Processor - Check your Makefile"
#   endif
#else
                                CRYSTALTIME;
#endif


    (void) timer_init(get_ghist_timer(), 
                      ghist_count,
                      _generic_ghist_isr,
                      &initial_ghist_priority);
}


/*********************************************************************
 *
 * NAME
 *	_term_p_timer - terminate the timer interrupts.
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
_term_p_timer(void)
{
    /* Set the timer's control word. Leave it hanging for counter data. */
    (void) change_priority(initial_ghist_priority);
    timer_term(get_ghist_timer());
}
