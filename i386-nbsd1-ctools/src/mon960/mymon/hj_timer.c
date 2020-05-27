/*******************************************************************************
 * 
 * Copyright (c) 1993, 1995 Intel Corporation
 * 
 * Intel hereby grants you permission to copy, modify, and distribute this
 * software and its documentation.  Intel grants this permission provided
 * that the above copyright notice appears in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  In addition, Intel grants this permission provided that
 * you prominently mark as not part of the original" any modifications
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

#define TIMER_VECTOR ((counter==0)?0x12:0x22)

static unsigned long ext_timer_speed;

static unsigned int timer_init_count;

static int ghist_counter = -1;   /* Ghist optimization hack. */

/************************************************/
/* INIT_INTERRUPTS                */
/************************************************/
static void
init_interrupts(int counter)
{
	int mask, irq = (counter==0)?16:20;
    /*
     * Only change the bits we need in IMAP 2.
     */

    mask = (0xffffffff ^ (0xf << irq));
	set_imap(2, (get_imap(2) & mask) | ((TIMER_VECTOR >> 4) << irq));

    mask = (0xffffffff ^ (1 << (counter + 12)));
    set_pending(get_pending() & mask);
    set_mask(get_mask() | (1 << (counter + 12))); 
}

/************************************************/
/* start_TIMER                    */
/************************************************/
static void
start_timer(int counter, unsigned int init_time)
{
    if ((counter != 0) && (counter != 1))
	   return;

    set_tmr(counter, 0);
    set_tcr(counter, 0);
    set_trr(counter, 0);
    set_trr(counter, init_time);
    set_tmr(counter, 0x0e);
    return;
}


/************************************************/
/* READ_timer                    */
/*                         */
/* This routine reads the counter values and    */
/* returns the uS value.            */
/************************************************/
unsigned int 
timer_read(int timer)
{
    int counter, count, count_value;
    unsigned long tmp, tmp1000;
    struct { unsigned long rem, quo; } result;

    counter = get_timer_offset(timer)-1;
    if ((counter != 0) && (counter != 1))
        return ERR;

flush:
    count = _bentime_counter;

    count_value = get_tcr(counter);

    if (count != _bentime_counter)
        goto flush;

    /*
     * Compute elapsed microseconds, accounting for the fact that 
     * ext_timer_speed is scaled in 1000's of MHz.
     */
    tmp = timer_init_count - count_value;
    if (get_bentime_timer() == timer && get_timer_intr(timer) == TRUE)
        tmp += count * timer_init_count;

    tmp1000 = 1000;
    {
        asm volatile("emul %1, %2, %0"  
                     : "=t" (tmp) 
                     : "d" (tmp), "d" (tmp1000));
        asm volatile("ediv %1, %2, %0"  
                     : "=t" (result) 
                     : "d" (ext_timer_speed), "t" (tmp));
    }
    return (result.quo);
}


/************************************************/
/* Function:   void term_timer(int timer)          */
/*                                              */
/* Passed:     void.                            */
/*                                              */
/* Returns:    void.                            */
/*                                              */
/* Action:     terminates counter values        */
/************************************************/
void
timer_term(int timer)
{
int counter, mask, irq;

    counter = get_timer_offset(timer)-1;
	irq = (counter==0)?16:20;
    if ((counter != 0) && (counter != 1))
	   return;

    set_tmr(counter, 0);   /* Kill CPU counter                      */
    set_tcr(counter, 0);   /* Effectively zero CPU counter register */
    set_trr(counter, 0);   /* Effectively zero CPU counter register */
    mask = (0xffffffff ^ (1 << (counter + 12)));
    set_mask(get_mask() & mask);  /* disable bit IRQ = 0 */
    set_pending(get_pending() & mask);
    set_timer_not_inuse(timer);
    ghist_counter = -1;
}


/************************************************/
/* Function:   void timer_suspend()       */
/*                                              */
/* Passed:     void.                            */
/*                                              */
/* Returns:    void.                            */
/*                                              */
/* Action:     suspends counter incrementing    */
/************************************************/
void
timer_suspend()
{
	int counter;

    if (get_onboard_only() == TRUE)
        {
        counter = get_timer_offset(get_bentime_timer())-1;
        if ((counter == 0) || (counter == 1))
            set_tmr(counter, 0xc);
        }
}


/************************************************/
/* Function:   void timer_resume()       */
/*                                              */
/* Passed:     void.                            */
/*                                              */
/* Returns:    void.                            */
/*                                              */
/* Action:     restart counter incrementing    */
/************************************************/
void
timer_resume()
{
    int counter;

    if (get_onboard_only() == TRUE)
        {
        counter = get_timer_offset(get_bentime_timer())-1;
        if ((counter == 0) || (counter == 1))
            set_tmr(counter, 0xe);
        }
}


/************************************************/
/* timer_init                    */
/*                         */
/* This routine initializes the JX    interrupt */
/* and JX timer  registers and initializes the timer*/
/* count. It returns the overhead for the     */
/* read_timer(counter) call.                */
/************************************************/
unsigned int
timer_init(int          timer, 
           unsigned int init_time, 
           void         *timer_isr, 
           unsigned int *prev_prio)
{
    unsigned int *int_table, *prcb;
    unsigned int overhead, this_time, local_prev_prio;
    int          counter, i, is_bentime_timer;

    is_bentime_timer = (timer == get_bentime_timer());

	if (get_timer_inuse(timer) == TRUE)
       timer_term(timer);

    counter = get_timer_offset(timer)-1;
    if ((counter != 0) && (counter != 1))
	    return ERR;

    if (is_bentime_timer)
    {
        timer_init_count = init_time;
        ext_timer_speed  = timer_extended_speed();
    }

    if (timer_isr == NULL)
        /* non interrupt just return */
		{
        start_timer(counter, init_time);
		set_timer_inuse(timer);
        return(OK);
		}

    /* We need to locate the interrupt table in order to 
    enter our new vector into the table.  We will do this
    by first finding the PRCB (Processor control block) base
    and then indexing into it to find the interrupt table.
    Once we have the interrupt table, we can index into it
    and put the vector into the appropriate spot.  */

    /* move PRCB to prcb pointer */
    prcb = (unsigned int *)get_prcbptr();    

    /* Interrupt table is here */
    int_table = (unsigned int *)prcb[4]; 

    /* set our timer vector */
    int_table[TIMER_VECTOR+1] = (unsigned int) timer_isr;

    init_interrupts(counter);
    set_timer_inuse(timer);

    /* Timer interrupt handler installed, let monitor field interrupts. */
    local_prev_prio = (change_priority(0) & 0x001f0000) >> 16;
    if (prev_prio)
        *prev_prio = local_prev_prio;

    start_timer(counter, init_time);
    if (! is_bentime_timer)
        return (OK);

    /* begin new calculation of bentime overhead */
    overhead = 0;

    /* use the old way */
    for (i=1; i<=100 ; i++)
        {
        this_time         = timer_read(timer);
        _bentime_counter  = 0;
        overhead         += timer_read(timer) - this_time;
        }
    /* average of 100 calls should be about right. */
    overhead = (overhead + 99) / 100;

    /* end new calculation of bentime overhead */
    /* reset so counter dos not give an extra intr */
    _bentime_counter = 0;

    return(overhead);
}



int
timer_supported(void)
{
    return (TIMER_API_VIA_CPU);
}



void
ghist_suspend_timer(void)
{
    if (ghist_counter == -1)
       ghist_counter = get_timer_offset(get_ghist_timer() - 1);

    set_tmr(ghist_counter, 0);   /* Kill the CPU counter      */
    set_tcr(ghist_counter, 0);   /* Effectively zero CPU counter register */
    set_trr(ghist_counter, 0);   /* Effectively zero CPU counter register */
}



void
ghist_reload_timer(unsigned int init_count)
{
    /* Assumes ghist counter is disabled and CPU counter is zeroed. */

    set_trr(ghist_counter, init_count);
    set_tmr(ghist_counter, 0x0e);
}
