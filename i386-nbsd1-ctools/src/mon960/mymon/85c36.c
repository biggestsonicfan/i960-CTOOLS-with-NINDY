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


extern int c145_isr_0, c145_isr_1, c145_isr_2;

static int bentime_init_count;

#if KXSX_CPU
static unsigned int caller_priority;
#endif


/************************************************/
/* Timer_critical_region                    */
/* This routine stop timer interrupts from */
/* occuring while timer registers are being set.*/
/* This is necessary because of setting register and then the value*/
/* is a two step operation that may no happen in parallel. */
/* Also diables dcahe during timer register operations */
/*    on = True turn off interrupts, Falsse turn them back on */
/*   irq = Timers irq     */
/************************************************/
static void 
timer_critical_region(int on, int irq)
{
#if CXHXJX_CPU
    if (on == TRUE)
        {
        /* disable bit IRQ = 0 */
        set_mask(get_mask() & (0xffffffff ^ (1 << irq)));
        disable_dcache();
        }
    else
        {
        set_mask(get_mask() | (1 << irq)); 
        enable_dcache();
        }
#endif /* KXSX */
#if KXSX_CPU
    if (on == TRUE)
        caller_priority = change_priority(31);
    else
        change_priority(caller_priority);
#endif /* KXSX */
}
    

/************************************************/
/* INIT_CIO                    */
/* This routine initializes the 85c36 */
/* cio registers and initializes the cio*/
/************************************************/
static void
start_timer(int counter, unsigned int init_time, int is_16_bit)
{
    TIMER_BASE->ctrl = 0x0a+counter;    /* 0x0a = Timer  Cmd & Status */
    TIMER_BASE->ctrl = 0xe0;            /* Clear IE, reset Gate Cmd Bit */

    /*
     * Clear IP, IUS, and keep gate command bit reset.
     * Must reset IP first and then IUS as per 85c36 errata sheet.
     */
    TIMER_BASE->ctrl = 0x0a+counter;    /* 0x0a = Timer  Cmd & Status */
    TIMER_BASE->ctrl = 0xa0;            /* Clear IP */
    TIMER_BASE->ctrl = 0x0a+counter;    /* 0x0a = Timer  Cmd & Status */
    TIMER_BASE->ctrl = 0x60;            /* Clear IUS */

    TIMER_BASE->ctrl = 0x1c+counter;    /* 0x1c = Timer  Mode Specification */

    if (is_16_bit)
    {
        /* 
         * Program 16-bit counter for single cycle, one-shot, no ext
         * trigger, retrigger enabled operations (used in ISR to force
         * reload of timer count -- makes ghist timer interrupts
         * predictable).  Note that single cyle operation is Very Important
         * in level triggered interrupt mode because it ensures that a
         * pre-empted timer interrupt (e.g., pre-empted by a serial break
         * interrupt) does not cause multiple, unserviced timer interrupts
         * (which tend to lock up the monitor).
         */

        TIMER_BASE->ctrl = 0x05;
    }
    else
    {
        /* 
         * Program first half of linked, 32-bit counter such that the first
         * counter drives second counter in continuous cycle, pulse output,
         * no ext trigger, retrigger enabled mode.  Note that pulse output
         * mode is required for proper gating of the second counter.
         *
         * Note that the second half of the linked counter will be
         * programmed to run in single cycle, one-shot, no ext trigger,
         * retrigger enabled mode.  Again, note that single cyle operation
         * is Very Important in level triggered interrupt mode to ensure
         * that a pre-empted timer interrupt does not cause multiple,
         * unserviced timer interrupts.
         */

        TIMER_BASE->ctrl = 0x84;
    }
    TIMER_BASE->ctrl = 0x16+(counter * 2);   /* 0x16 = Timer  TC, MS byte */
    TIMER_BASE->ctrl = ((init_time >> 8) & 0xff); 
    TIMER_BASE->ctrl = 0x17+(counter * 2);   /* 0x17 = Timer  TC, LS byte */
    TIMER_BASE->ctrl = (init_time & 0xff); 
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
unsigned char LSB, MSB;
unsigned int irq, counter, count, current_count;

    /* counter 1, 2 */
    counter = get_timer_offset(timer);
    irq = get_timer_irq(timer);
    if ((counter < 1) || (counter > 2) || (get_timer_inuse(timer) == FALSE))
        return ERR;

    timer_critical_region(TRUE, irq);

    count = _bentime_counter;

    if (counter == 1)
        {
        /* quickly freeze  and read counter values */
        TIMER_BASE->ctrl = 0x0a;
        TIMER_BASE->ctrl = 0x08;
        TIMER_BASE->ctrl = 0x0b;
        TIMER_BASE->ctrl = 0x08;
        TIMER_BASE->ctrl = 0x12;
        MSB = TIMER_BASE->ctrl;        /* read timer value */
        TIMER_BASE->ctrl = 0x13;
        LSB = TIMER_BASE->ctrl;        /* read timer value */
        current_count = (MSB << 8) + LSB;
        
        TIMER_BASE->ctrl = 0x10;
        MSB = TIMER_BASE->ctrl;        /* read timer value */
        TIMER_BASE->ctrl = 0x11;
        LSB = TIMER_BASE->ctrl;        /* read timer value */
        current_count = (current_count << 16) + (MSB << 8) + LSB;
        TIMER_BASE->ctrl = 0x0b;
        TIMER_BASE->ctrl = 0x04;
        TIMER_BASE->ctrl = 0x0a;
        TIMER_BASE->ctrl = 0x04;
        }
	else
		{
        /* quickly freeze  and read counter values */
        TIMER_BASE->ctrl = 0x0c;
        TIMER_BASE->ctrl = 0x08;
        TIMER_BASE->ctrl = 0x14;
        MSB = TIMER_BASE->ctrl;        /* read timer value */
        TIMER_BASE->ctrl = 0x15;
        LSB = TIMER_BASE->ctrl;        /* read timer value */
        current_count = (MSB << 8) + LSB;
        TIMER_BASE->ctrl = 0x0c;
        TIMER_BASE->ctrl = 0x04;
		}

    timer_critical_region(FALSE, irq);

    if ((get_bentime_timer() == timer) && (get_timer_intr(timer) == TRUE))
        return (((count * bentime_init_count) + 
             (bentime_init_count - current_count) ) / CRYSTALTIME);
    else
        return ((bentime_init_count - current_count) / CRYSTALTIME);
}


/************************************************/
/* timer_init                    */
/*                         */
/* This routine initializes the processor interrupt */
/* and 85c36 registers and initializes the timer */
/* count. It returns the overhead for the     */
/* read_timer(counter) call.                */
/************************************************/
unsigned int 
timer_init(int          timer, 
           unsigned int init_time, 
           void         *timer_isr, 
           unsigned int *prev_prio)
{
    unsigned int counter, overhead, this_time, local_prev_prio;
    int i, irq, timer_vector;
    unsigned char intr_start;
    volatile unsigned char reg;

	if (get_timer_inuse(timer) == TRUE)
       timer_term(timer);

    /* counter 1, 2 */
    counter = get_timer_offset(timer);
    if ((counter < 1) || (counter > 2))
        return ERR;

    irq = get_timer_irq(timer);
    timer_vector = get_timer_vector(timer);

    if (timer == get_bentime_timer())
    {
        if ((init_time != 0) && ((init_time >> 16) == 0))
            init_time = (1 << 16) + init_time;

        bentime_init_count = init_time;
    }
    else if (timer_isr != NULL)
    {
        /* 
         * Assume this is a request for ghist services.  In such case, the
         * timer_isr passed to this routine is not much use to us (cause
         * the 85c36 needs an ISR epilogue) and so we'll use our own.  But
         * do fetch the client's ghist callback routine and invoke that
         * when the 85c36 interrupt is taken.
         */

        timer_isr = get_ghist_callback();
    }


    if (timer_isr == NULL)
		/* leave interrupts off for timer */
		intr_start = 0x06;
	else
		/* turn on interrupts for timer */
		intr_start = 0xc6;

    timer_critical_region(TRUE, irq);
    /* Reset CIO state machine */
    reg = TIMER_BASE->ctrl;    /* Put CIO in reset state or state 0 */
    TIMER_BASE->ctrl = 0;        /* Put CIO state 0 or 1 */
    reg = TIMER_BASE->ctrl;    /* Put CIO in state 0 */

    if (counter == 1) /* use 32 bit timer */
        {
        TIMER_BASE->ctrl = 0x01;            /* 0x00 = Master Config Control */
        reg = TIMER_BASE->ctrl;
        TIMER_BASE->ctrl = 0x01;            /* 0x00 = Master Config Control */
        TIMER_BASE->ctrl = (reg & 0x9c);
        TIMER_BASE->ctrl = 0x01;            /* 0x00 = Master Config Control */
        TIMER_BASE->ctrl = (reg | 0x03);
        TIMER_BASE->ctrl = 0x0a;            /* 0x0a = Timer  Cmd & Status */
        TIMER_BASE->ctrl = 0x00;            /* clear */
        TIMER_BASE->ctrl = 0x0b;            /* 0x0a = Timer  Cmd & Status */
        TIMER_BASE->ctrl = 0x00;            /* clear */
        /* Turn off timer  interrupts at CIO */
        start_timer(1, init_time >> 16, TRUE);
        start_timer(0, init_time & 0xffff, FALSE);

        TIMER_BASE->ctrl = 0x01;            /* 0x00 = Master Config Control */
        reg = TIMER_BASE->ctrl;
        TIMER_BASE->ctrl = 0x01;            /* 0x00 = Master Config Control */
        TIMER_BASE->ctrl = (reg | 0x63);
        /* Clear Timer 1,2 enable bit link counters */
        TIMER_BASE->ctrl = 0x0b;            /* 0x0a = Timer  Cmd & Status */
        TIMER_BASE->ctrl = intr_start;            /* Set Gate Cmd Bit */
        TIMER_BASE->ctrl = 0x0a;            /* 0x0a = Timer  Cmd & Status */
        TIMER_BASE->ctrl = 0x06;            /* Set Gate Cmd Bit */

		/* timer not reliable for reads until turns over first time */
		/* 50000 loops should let time turn over */
        eat_time(50000);
        }
    else
        {
        /* Assume this is ghist... */

        TIMER_BASE->ctrl = 0x0c;       /* 0x0c = Timer  Cmd & Status        */
        TIMER_BASE->ctrl = 0x00;       /* Clear Gate Cmd Bit, suspend timer */
        start_timer(2, init_time & 0xffff, TRUE);
        TIMER_BASE->ctrl = 0x0c;       /* 0x0c = Timer  Cmd & Status */
        TIMER_BASE->ctrl = intr_start; /* Set Gate Cmd Bit */
        }

    /* non interrupt just return */
    if (timer_isr == NULL)
		{
        timer_critical_region(FALSE, irq);
        set_timer_inuse(timer);
        return(OK);
        }

	switch (counter)
		{
		case 0:
	    	{
			c145_isr_0 = (unsigned int)timer_isr;
			break;
		    }
		case 1:
	    	{
			c145_isr_1 = (unsigned int)timer_isr;
			break;
		    }
		case 2:
	    	{
			c145_isr_2 = (unsigned int)timer_isr;
			break;
		    }
		default:
			return ERR;
		}

    TIMER_BASE->ctrl = 0x00;            /* 0x00 = Master Int Control */
    TIMER_BASE->ctrl = 0x80;      /* Set master int enable bit */
    timer_critical_region(FALSE, irq);

    set_timer_interrupt(timer, get_timer_default_isr()); 
    set_timer_inuse(timer);

    /* Timer interrupt handler installed, let monitor field interrupts. */
    local_prev_prio = (change_priority(0) & 0x001f0000) >> 16;
    if (prev_prio)
        *prev_prio = local_prev_prio;

    if (timer != get_bentime_timer())
        return OK;

/* begin new calculation of bentime overhead */
    overhead = 0;

    /* use the old way */
    for (i=1; i<=100 ; i++)
        {
        this_time = timer_read(timer);
        _bentime_counter = 0;
        overhead += timer_read(timer) - this_time;
        }
    /* average of 100 calls should be about right. */
    overhead = (overhead + 99) / 100;

    /* reset so counter dos not give an extra intr */
    _bentime_counter = 0;

    return(overhead);
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
    unsigned int counter, irq, timer_mask, reg;

    /* counter 1, 2 */
    counter = get_timer_offset(timer);
    irq = get_timer_irq(timer);
    if ((counter < 1) || (counter > 2) || (get_timer_inuse(timer) == FALSE))
        return;

    timer_critical_region(TRUE, irq);
    /* Reset CIO state machine */
    reg = TIMER_BASE->ctrl;    /* Put CIO in reset state or state 0 */
    TIMER_BASE->ctrl = 0;        /* Put CIO state 0 or 1 */
    reg = TIMER_BASE->ctrl;    /* Put CIO in state 0 */

    /* Turn off timer 1 interrupts at CIO */
    if (counter == 1)
        {
        TIMER_BASE->ctrl = 0x0a;            /* 0x00 = Master Config Control */
        TIMER_BASE->ctrl = 0x00;                    /* Enable Timer enable bit */
        TIMER_BASE->ctrl = 0x0b;            /* 0x00 = Master Config Control */
        TIMER_BASE->ctrl = 0x00;                    /* Enable Timer enable bit */

        TIMER_BASE->ctrl = 0x01;            /* Master Config Control */
        reg = TIMER_BASE->ctrl;
        TIMER_BASE->ctrl = 0x01;            /* Master Config Control */
        TIMER_BASE->ctrl = reg & ~(0x63);   /* Shut down ports A & B */
        }
    else if (counter == 2)
        {
        TIMER_BASE->ctrl = 0x0c;            /* 0x00 = Master Config Control */
        TIMER_BASE->ctrl = 0x00;            /* Enable Timer enable bit */

        /* Note that port C cannot be shutdown -- User LED's live there */
		}

    timer_critical_region(FALSE, irq);

    if (get_timer_intr(timer) == TRUE)
        {
#if CXHXJX_CPU
        timer_mask = (0xffffffff ^ (1 << (irq)));
        set_mask(get_mask() & timer_mask);  /* disable bit IRQ = 0 */

        {
        int count, foo;

        count = 10000;
        for(;count--;)
            foo = count;
        }
        set_pending(get_pending() & timer_mask);
#endif /* CXJX */

#if KXSX_CPU
        timer_mask = interrupt_register_read() & (0xffffffff ^ (0xff << (irq * 8)));
        interrupt_register_write(&timer_mask);
#endif /* KXSX */
        }
    set_timer_not_inuse(timer);
}
    
/************************************************/
/* Function:   void suspend_timer()       */
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
    int counter, irq, bentime_timer;

    /* counter 1, 2 */
	bentime_timer = get_bentime_timer();
    counter = get_timer_offset(bentime_timer);
    irq = get_timer_irq(bentime_timer);
    if ((counter < 1) || (counter > 2) || (get_timer_inuse(bentime_timer) == FALSE))
        return;

    if (get_onboard_only() == TRUE)
        {
        timer_critical_region(TRUE, irq);
        if (counter == 0)
            {
            TIMER_BASE->ctrl = 0x0a;            /* 0x00 = Master Config Control */
            TIMER_BASE->ctrl = 0x00;                    /* Enable Timer enable bit */
            TIMER_BASE->ctrl = 0x0b;            /* 0x00 = Master Config Control */
            TIMER_BASE->ctrl = 0x00;                    /* Enable Timer enable bit */
            }
        else
            {
            TIMER_BASE->ctrl = 0x0c;            /* 0x00 = Master Config Control */
            TIMER_BASE->ctrl = 0x00;                    /* Enable Timer enable bit */
            }
        timer_critical_region(FALSE, irq);
        }
}


/************************************************/
/* Function:   void resume_timer()       */
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
    int irq, counter, bentime_timer;

    /* counter 1, 2 */
	bentime_timer = get_bentime_timer();
    counter = get_timer_offset(bentime_timer);
    irq = get_timer_irq(bentime_timer);
    if ((counter < 1) || (counter > 2) || (get_timer_inuse(bentime_timer) == FALSE))
        return;

    if (get_onboard_only() == TRUE)
        {
        timer_critical_region(TRUE, irq);
        if (counter == 0)
            {
            TIMER_BASE->ctrl = 0x0b;            /* 0x00 = Master Config Control */
            TIMER_BASE->ctrl = 0x04;                    /* Enable Timer enable bit */
            TIMER_BASE->ctrl = 0x0a;            /* 0x00 = Master Config Control */
            TIMER_BASE->ctrl = 0x04;                    /* Enable Timer enable bit */
            }
        else
            {
            TIMER_BASE->ctrl = 0x0c;            /* 0x00 = Master Config Control */
            TIMER_BASE->ctrl = 0x04;                    /* Enable Timer enable bit */
            }
        timer_critical_region(FALSE, irq);
        }
}


int
get_cio_crtl(int addr)
{
    int reg_val;
        timer_critical_region(TRUE, 6);
            TIMER_BASE->ctrl = addr & 0x1f;         /* 0x00 = Master Config Control */
            reg_val = TIMER_BASE->ctrl;                  /* Enable Timer enable bit */
        timer_critical_region(FALSE, 6);
        return reg_val;
}


int
timer_supported(void)
{
    return (TIMER_API_VIA_TRGT_HW);
}



void 
ghist_reload_timer(unsigned int count)
{
    /* This is a stub -- its functionality is hand coded in c145_asm.s */
}



void
ghist_suspend_timer(void)
{
    /* This is a stub -- its functionality is hand coded in c145_asm.s */
}
