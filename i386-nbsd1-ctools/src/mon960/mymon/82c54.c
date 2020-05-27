/*****************************************************************************
 * Copyright (c) 1993, 1994, 1995 Intel Corporation
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
#include "82c54.h"

/* ------------------------------ Warning ----------------------------- * 
 *
 * The code in this module assumes that that 82c54 counter 0's output is 
 * cascaded into the clock of 82c54 counter 1 (forming a 32-bit counter).
 * If such is not the case, any use of counter 1 (e.g., bentime) on your
 * target is most likely not going to work.  A quick workaround might be
 * to modify your hardware.h file and map TIMER0_OFFSET to counter 2 and
 * then test bentime to see how well it behaves (theoretically, bentime()
 * should function correctly with a 16-bit counter, so long as your 
 * applications don't run for a long, long time).
 *
 * ---------------------------- End Warning --------------------------- */

static suspended_time;

static unsigned int timer_init_count;

static int ghist_ccounter = -1;   /* Ghist optimization hack. */


static void
stabilize_timer()
{
    /*
     * Wait for the @$@#$@#$ @$@#$@#$ @#$@#$@!#$(*&) 82c54 timers to 
     * stabilize before using their outputs.  All @$!@#$ magic
     * numbers used below are empirically derived.
     */

    pause();
#ifdef SLOW_TIMER_HW
    eat_time(__cpu_speed * 2000 * SLOW_TIMER_HW);
#else
    eat_time(__cpu_speed * 2000);
#endif
    pause();
}



static void
start_timer_2(unsigned int init_time)
{
    /* 
     * LSB,MSB,16-bit binary, MODE3.
     *
     * We use mode 3 (square wave countdown) because on a target with a 10
     * MHz timer clock (aka ApLink or EPCX), the rapid high-low-high
     * transition of counter 2 OUT may be too fast for detection (in
     * edge-triggered mode) by the i960 CPU's interrupt controller.
     */

    /* 
     * Due to use of MODE3, which decrements the timer count by 2 for every
     * clock in, we must double the client's requested clock count to
     * satisfy the client's timing interval.
     */
    init_time *= 2;

    store_byte ((CTRL_COUNTER2 | RW_BOTH | MODE3), CONTROL_REG);  

    /* LSB initial count */
    store_byte ((init_time & 0xff), COUNTER_2); 

    /* MSB initial count */
    store_byte (((init_time >> 8) & 0xff), COUNTER_2); 
}


static void
start_timer_0_and_1(unsigned int init_0_time, unsigned int init_1_time)
{
    /*
     * Must init the control registers of the cascaded timers first
     * before setting their counts.
     */

    /* LSB,MSB,16-bit binary */
    store_byte ((CTRL_COUNTER0 | RW_BOTH | MODE2), CONTROL_REG);  
    store_byte ((CTRL_COUNTER1 | RW_BOTH | MODE2), CONTROL_REG);  

    /* LSB initial count */
    store_byte ((init_1_time & 0xff), COUNTER_1); 

    /* MSB initial count */
    store_byte (((init_1_time >> 8) & 0xff), COUNTER_1); 

    /* LSB initial count */
    store_byte ((init_0_time & 0xff), COUNTER_0); 

    /* MSB initial count */
    store_byte (((init_0_time >> 8) & 0xff), COUNTER_0); 
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
    unsigned int counter, current_count, LSB=0, MSB=0, count=0;

    counter = get_timer_offset(timer);
	if (counter > 2)
		return ERR;

    /* quickly freeze counter values */
flush:
    count = _bentime_counter;

    /* counter 0, 1, 2 */
    if (counter == 2)
        {
        store_quick_byte(0xd0 | 0x8, CONTROL_REG);  /* Use read-back latch */
        LSB = load_byte (COUNTER_2);                /* read timer value */
        MSB = load_byte (COUNTER_2);
		current_count = (MSB << 8) + LSB;
        }
    else if (counter == 1)
        {
        store_quick_byte(0xd0 | 0x6, CONTROL_REG);  /* Use read-back latch */
        LSB = load_byte (COUNTER_1);                /* read timer value */
        MSB = load_byte (COUNTER_1);
		current_count = (MSB << 8) + LSB;
        LSB = load_byte (COUNTER_0);                /* read timer value */
        MSB = load_byte (COUNTER_0);
		current_count = (current_count << 16) + (MSB << 8) + LSB;
        }
    else if (counter == 0)
        {
        store_quick_byte(0xd0 | 0x2, CONTROL_REG);  /* Use read-back latch */
        LSB = load_byte (COUNTER_0);                /* read timer value */
        MSB = load_byte (COUNTER_0);
		current_count = (MSB << 8) + LSB;
        }
    else
        return (ERR);

    if (count != _bentime_counter)
        goto flush;

    if (get_timer_intr(timer) == TRUE)
        return (((count * timer_init_count) + (timer_init_count - 
		    current_count) - suspended_time) / CRYSTALTIME);
    else
        return ((timer_init_count - current_count) / CRYSTALTIME);
}

/************************************************/
/* timer_init                                   */
/*                                              */
/* This routine initializes the timer interrupt */
/* and 8254  registers and initializes the timer*/
/* count. It returns the overhead for the       */
/* read_timer(counter) call.                    */
/************************************************/
unsigned int 
timer_init(int          timer, 
           unsigned int init_time, 
           void         *timer_isr, 
           unsigned int *prev_prio)
{
    unsigned int overhead, this_time, local_prev_prio;
    int          i, counter, is_bentime_timer;

    is_bentime_timer = (timer == get_bentime_timer());

    if (get_timer_inuse(timer) == TRUE)
       timer_term(timer);

    /* counter 1, 2 */
    counter = get_timer_offset(timer);
    if ((counter < 1) | (counter > 2))
        return ERR;

    if (is_bentime_timer)
    {
        if ((init_time != 0) && ((init_time >> 16) == 0))
            init_time = (1 << 16) + init_time;

        timer_init_count = init_time;
        suspended_time   = 0;
    }

    if (counter == 1)
        start_timer_0_and_1(init_time & 0xffff, init_time >> 16);
    else
        start_timer_2(init_time & 0xffff);

    if (is_bentime_timer)
        stabilize_timer(timer);

    if (timer_isr == NULL)
    {
        /* non interrupt just return */

        set_timer_inuse(timer);
        return(OK);
    }

    set_timer_interrupt(timer, timer_isr);
    set_timer_inuse(timer);

    /* Timer interrupt handler installed, let monitor field interrupts. */
    local_prev_prio = (change_priority(0) & 0x001f0000) >> 16;
    if (prev_prio)
        *prev_prio = local_prev_prio;

    if (! is_bentime_timer)
        return (OK);

    /* begin calculation of bentime overhead */
    overhead = 0;
    for (i = 1; i <= 100 ; i++)
    {
        _bentime_counter  = 0;
        this_time         = timer_read(counter);
        overhead         += timer_read(counter) - this_time;
    }

    /* average of 100 calls should be about right. */
    overhead = (overhead + 99) / 100;

    /* end new calculation of bentime overhead */
    /* reset so counter does not give an extra intr */
    _bentime_counter = 0;
    return(overhead);
}


/************************************************/
/* Function:   void timer_term(int timer)       */
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
    unsigned int timer_mask=0, timer_irq;
    int          ccounter=0, counter;

	counter   = get_timer_offset(timer);
	timer_irq = get_timer_irq(timer);
    if ((counter < 0) | (counter > 2))
        return;

    if (counter == 2)
       {
       ccounter = CTRL_COUNTER2;
       counter = COUNTER_2;
       }
    else if (counter == 1)
       {
       ccounter = CTRL_COUNTER1;
       counter = COUNTER_1;
       }
    else if (counter == 0)
       {
       ccounter = CTRL_COUNTER0;
       counter = COUNTER_0;
       }

#if CXHXJX_CPU
    if (get_timer_intr(timer) == TRUE)
		{
        timer_mask = (0xffffffff ^ (1 << (timer_irq)));
        set_mask(get_mask() & timer_mask);  /* disable bit IRQ = 0 */
        set_pending(get_pending() & timer_mask);
		}
#endif  /* CX */

#if KXSX_CPU
    timer_mask = interrupt_register_read() & (~(0xff << (timer_irq * 8)));
    interrupt_register_write(&timer_mask);
#endif /* KXSX */

    /* timer: mode 0, count down to 0 and shut up */
    if (counter == COUNTER_2)
    {
        store_byte ((ccounter | RW_BOTH | MODE0), CONTROL_REG);
                                            /* LSB,MSB,16-bit binary */
        store_byte (0x02, counter);         /* LSB initial count */
        store_byte (0x00, counter);         /* MSB initial count */
    }
    else
    {
        unsigned int i, timer1_out;

        /* 
         * Terminating cascaded counter 1 -- must force both counter 0 & 1
         * to count down and shut up.  There's a trick here:
         *
         * Since counter 1 is slaved to counter 0's output, you need a
         * reliable clock input to shut down counter 1.  To do this, put
         * counter 0 in mode 2 with the minimum possible count.  After
         * kicking counter 0, start counter 1 in mode 0 and wait for
         * counter 0 to drive counter 1 down to 0.  At that point, counter
         * 1 (in mode 0) shuts off.  Finally, put counter 0 in mode 0 with
         * the minimum possible count and wait for it to count down and
         * shut off.
         *
         * Is all of this effort really necessary?  For targets that OR the
         * timer 1 and timer 2 output lines into the same interrupt, you
         * betcha'.  You can't have both counters active at the same time,
         * nor can you permit one counter to initialize itself in a state
         * such that its output line is always low (e.g., counter 1). 
         * Reason: if any counter has its output line go low due to random
         * initialization, any valid high-low transition by the other
         * counter will be seen by the CPU as a garbled signal and erratic
         * timer interrupts are sure to ensue.
         *
         * Is there a target that OR's both interrupt lines together?  Yes:
         * Aplink.
         */

        store_byte((CTRL_COUNTER0 | RW_BOTH | MODE2), CONTROL_REG);
                                           /* LSB,MSB,16-bit binary */
        store_byte(0x02, COUNTER_0);       /* LSB initial count */
        store_byte(0x00, COUNTER_0);       /* MSB initial count */

        store_byte((CTRL_COUNTER1 | RW_BOTH | MODE0), CONTROL_REG);
        store_byte(0x02, COUNTER_1);
        store_byte(0x00, COUNTER_1);

        eat_time(10);

        /* Wait for counter 1 to count down and then kill counter 0. */
        for (i = 0; i < 800; i++)
        {
            store_byte(0xe4, CONTROL_REG);     /* latch counter 1 status */
            timer1_out = load_byte(COUNTER_1); /* read counter 1 status  */
            if (timer1_out & 0x80)
                break;
        }
        store_byte((CTRL_COUNTER0 | RW_BOTH | MODE0), CONTROL_REG);
        store_byte(0x02, COUNTER_0);
        store_byte(0x00, COUNTER_0); 
    }
    set_timer_not_inuse(timer);
    ghist_ccounter = -1;
}

static  unsigned int stop_time;
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
timer_suspend(void)
{
    if (get_onboard_only() == TRUE)
        stop_time = timer_read(get_bentime_timer());
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
timer_resume(void)
{
    if (get_onboard_only() == TRUE)
       suspended_time += timer_read(get_bentime_timer()) - stop_time;
}


unsigned int
get_82c54_reg(int reg_no)
{
    if (reg_no == 12)
    {
        store_byte(0xe8,CONTROL_REG);
       return  load_byte (COUNTER_2);        /* read timer value */
    }
    if (reg_no == 11)
    {
        store_byte(0xe4,CONTROL_REG);
       return  load_byte (COUNTER_1);        /* read timer value */
    }
    if (reg_no == 10)
    {
        store_byte(0xe2,CONTROL_REG);
       return  load_byte (COUNTER_0);        /* read timer value */
    }
    if (reg_no == 2)
    {
        store_byte(0x80,CONTROL_REG);
       return  (load_byte (COUNTER_2) << 8) + load_byte (COUNTER_2);        /* read timer value */
    }
    if (reg_no == 1)
    {
        store_byte(0x40,CONTROL_REG);
       return  (load_byte (COUNTER_1) << 8) + load_byte (COUNTER_1);        /* read timer value */
    }
    if (reg_no == 0)
    {
        store_byte(0x00,CONTROL_REG);
       return  (load_byte (COUNTER_0) << 8) + load_byte (COUNTER_0);        /* read timer value */
    }
    return ERR;
}


int
timer_supported(void)
{
    return (TIMER_API_VIA_TRGT_HW);
}


void
ghist_suspend_timer(void)
{
    /*
     * This is a very primitive timer -- no way to suspend it without
     * killing its GATE input, which we can't control (at least not for the
     * EPCX or an ApLink target).
     */
}



void
ghist_reload_timer(unsigned int reload_count)
{
    if (ghist_ccounter == -1)
    {
        int counter = get_timer_offset(get_ghist_timer());

        if (counter == 2)
            ghist_ccounter = COUNTER_2;
        else if (counter == 1)
            ghist_ccounter = COUNTER_1;
        else
            ghist_ccounter = COUNTER_0;
    }

    /* 
     * Assume counter is running in mode 3 and bump its input count
     * accordingly to match the behavior of the counter.
     */
    reload_count *= 2;

    store_byte ((reload_count & 0xff), ghist_ccounter); 
    store_byte (((reload_count >> 8) & 0xff), ghist_ccounter); 
}
