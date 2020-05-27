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

#include "sdm.h" 

/*********************************************************************
 *
 * NAME
 *    _set_p_timer(timer_client  client, 
 *                 logical_timer timer, 
 *                 void          (* callback)(int IP))
 *
 *    client   - should be initialized to ghist_timer_client (see sdm.h).
 *
 *    timer    - target timer to use (see sdm.h).  Generally, selecting
 *               DEFAULT_TIMER as this parameter's value will cause the
 *               monitor to select an appropriate timer.
 *
 *    callback - routine to record profile data, called by a Mon960 ISR
 *               that's hooked to the user-specified timer.  The callback
 *               routine receives, as its single parameter, the application
 *               address that was active when a timer interrupt fired.
 *
 * DESCRIPTION
 *    This function initializes Mon960's timer data structures,
 *    but does not start the timer.
 *
 * RETURNS
 *    None
 *
 *********************************************************************/

void
_set_p_timer(timer_client  client, 
             logical_timer timer, 
             void          (* callback)(unsigned int IP))
{
    mon_set_timer(client, timer, callback);
}



/*********************************************************************
 *
 * NAME
 *    _init_p_timer(int frequency)
 *
 *    frequency - selects profiling sampling frequency.  Valid values are:
 *
 *         1  -> 500 microseconds
 *         2  ->   1 millisecond (default)
 *         3  ->   2 milliseconds
 *         4  ->   5 milliseconds
 *
 * DESCRIPTION
 *    This function enables processor interrupts and then kicks the timer 
 *    hardware to enable profiling.
 *
 * RETURNS
 *    None
 *
 *********************************************************************/

void
_init_p_timer(int frequency)
{
    mon_init_p_timer(frequency);
}



/*********************************************************************
 *
 * NAME
 *    _term_p_timer - terminate the timer interrupts.
 *
 * DESCRIPTION
 *    This function disables the hw timer to prevent further interrupts.
 *
 * RETURNS
 *    None
 *
 *********************************************************************/

void
_term_p_timer(void)
{
    mon_term_p_timer();
}



/* dummy call for backward compatibility */
void
_init_int_vector(void)
{
}
