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
#include "this_hw.h"

unsigned volatile int _bentime_counter;

enum logical_timer {DEFAULT_TIMER, t0, t1, t2, t3, t4, t5, NO_TIMER};
enum client {GHIST, BENTIME, OTHER};
#define bentime_default_timer t0
#define ghist_default_timer t1

static unsigned int time_onboard_only = FALSE;

struct timer_entry {
    enum logical_timer   timer;
    int                  timer_offset;
    int                  intr_active;
    int                  irq;
    int                  vector;
    void*                isr;
    int                  inuse;
};

static struct timer_entry timer_list[7] = {
    {NO_TIMER,0,FALSE,0,0,(void *)0,FALSE},
    {bentime_default_timer,TIMER0_OFFSET,FALSE,TIMER0_IRQ,TIMER0_VECTOR,NULL,FALSE},
    {ghist_default_timer,TIMER1_OFFSET,FALSE,TIMER1_IRQ,TIMER1_VECTOR,NULL,FALSE},
    {NO_TIMER,0,FALSE,0,0,(void *)0,FALSE},
    {NO_TIMER,0,FALSE,0,0,(void *)0,FALSE},
    {NO_TIMER,0,FALSE,0,0,(void *)0,FALSE},
    {NO_TIMER,0,FALSE,0,0,(void *)0,FALSE},
    };

static enum logical_timer bentime_timer = bentime_default_timer;
static enum logical_timer ghist_timer = ghist_default_timer;
static GHIST_FNPTR ghist_timer_callback = NULL;


/************************************************/
/* mon_set_timer                                */
/*     ghist_call = true,  set ghist timer      */
/*                  false, set bentime timer    */
/*                  other, reset to defaults    */
/*     timer      = logical timer to set        */
/*     timer_isr  = For bentime, set isr to use */
/*                  for intr. For ghist, set    */
/*                  callback routine called by  */
/*                  intr.                       */
/************************************************/
int
mon_set_timer(enum client user_of_timer, enum logical_timer timer, void *  timer_isr)
{
    if (user_of_timer == GHIST)
        {
		if (timer == DEFAULT_TIMER)
			timer = ghist_default_timer;

        if ((timer_list[timer].timer == NO_TIMER) ||
	    	(timer_list[timer].inuse == TRUE)     ||
            (timer_isr == NULL))
            return ERR;

        ghist_timer = timer;
        ghist_timer_callback = (GHIST_FNPTR) timer_isr;
        timer_list[timer].isr = timer_isr;
        }
    else if (user_of_timer == BENTIME)
        {
		if (timer == DEFAULT_TIMER)
			timer = bentime_default_timer;

        if ((timer_list[timer].timer == NO_TIMER) ||
	    	(timer_list[timer].inuse == TRUE))
            return ERR;

        bentime_timer = timer;
        if (timer_isr == NULL)
            timer_list[timer].isr = get_default_bentime_isr();
        else
            timer_list[timer].isr = timer_isr;
        }
    else
        return ERR;

    return OK;
}


/************************************************/
/* set_timer_interrupt                    */
/*                         */
/* This routine initializes the KX/CX/HX/JX interrupt */
/************************************************/
unsigned int 
set_timer_interrupt(int timer, void * isr)
{
unsigned int *int_table, *prcb;
unsigned int irq, vector;
#if CX_CPU
unsigned int *control_table=NULL;
#endif

    if (timer_list[timer].inuse == TRUE)
        return ERR;

    /* We need to locate the interrupt table in order to 
    enter our new vector into the table.  We will do this
    by first finding the PRCB (Processor control block) base
    and then indexing into it to find the interrupt table.
    Once we have the interrupt table, we can index into it
    and put the vector into the appropriate spot.  */

    /* move PRCB to prcb pointer */
    prcb = (unsigned int *)get_prcbptr();    

#if CX_CPU
    /* Control table is here */
    control_table = (unsigned int *)prcb[1]; 
#endif /* CA */

#if CXHXJX_CPU
    /* Interrupt table is here */
    int_table = (unsigned int *)prcb[4]; 
#endif

#if KXSX_CPU
    /* Interrupt table is here */
    int_table = (unsigned int *)prcb[5]; 
#endif

    if (timer == bentime_timer)
        _bentime_counter = 0;

    /* set our timer vector */
    switch (timer-1){
    case 0:
        {
        irq = TIMER0_IRQ;
        vector = TIMER0_VECTOR;
        timer_list[timer].timer_offset = TIMER0_OFFSET;
        break;
        }
    case 1:
        {
        irq = TIMER1_IRQ;
        vector = TIMER1_VECTOR;
        timer_list[timer].timer_offset = TIMER1_OFFSET;
        break;
        }
#if defined MAX_TIMERS
    case 2:
        {
        irq = TIMER2_IRQ;
        vector = TIMER2_VECTOR;
        timer_list[timer].timer_offset = TIMER2_OFFSET;
        break;
        }
    case 3:
        {
        irq = TIMER3_IRQ;
        vector = TIMER3_VECTOR;
        timer_list[timer].timer_offset = TIMER3_OFFSET;
        break;
        }
    case 4:
        {
        irq = TIMER4_IRQ;
        vector = TIMER4_VECTOR;
        timer_list[timer].timer_offset = TIMER4_OFFSET;
        break;
        }
    case 5:
        {
        irq = TIMER5_IRQ;
        vector = TIMER5_VECTOR;
        timer_list[timer].timer_offset = TIMER5_OFFSET;
        break;
        }
#endif
    default:
        return ERR;
        }

    int_table[vector+1]           = (unsigned int) isr;
    timer_list[timer].irq         = irq;
    timer_list[timer].vector      = vector;
    timer_list[timer].intr_active = TRUE;

    {
#if CX_CPU      
    unsigned int mask, timer_irq, ctrl_tab_word;

    
    /* Configure interrupts, but only change bits for the affected IRQ.  */
    ctrl_tab_word = 4;
    timer_irq     = irq;
    if (irq > 3)
        {
        ctrl_tab_word  = 5;
        irq           -= 4;
        }

    mask = (0xffffffff ^ (0xf << (irq * 4)));
    control_table[ctrl_tab_word] = (control_table[ctrl_tab_word] & mask) | 
        ((vector >> 4) << (irq * 4));

    send_sysctl(0x401, 0, 0);    /* processor reloads ctl table registers */

    mask = (0xffffffff ^ (1 << (timer_irq)));
    set_pending(get_pending() & mask);
    set_mask(get_mask() | (1 << timer_irq)); 
#endif /* CX */

#if HXJX_CPU   
    unsigned int mask, timer_irq, imap_word;

    timer_irq = irq;

#if defined (HX_TIMER) || defined(JX_TIMER)
    irq = irq + 12;
    imap_word = 2;
#else
    imap_word = 0;
    if (irq > 3)
        {
        irq       -= 4;
        imap_word  = 1;
        }
#endif /*HXJX*/

    mask = (0xffffffff ^ (0xf << (irq * 4)));
    set_imap(imap_word, (get_imap(imap_word) & mask) | ((vector >> 4) << (irq * 4)));
    mask = (0xffffffff ^ (1 << (timer_irq)));
    set_pending(get_pending() & mask);
    set_mask(get_mask() | (1 << timer_irq)); 
#endif

#if KXSX_CPU
    unsigned int mask;

    mask = interrupt_register_read() & (~(0xff << (irq * 8)));
    mask |= vector << (irq * 8);
    interrupt_register_write(&mask);
#endif /* KXSX */
    }

    return(OK);
}

/************************************************/
/* get_timer_offset                    */
/*                         */
/************************************************/
unsigned int 
get_timer_offset(int timer)
{
    return timer_list[timer].timer_offset;
}

/************************************************/
/* get_timer_vector                    */
/*                         */
/************************************************/
unsigned int 
get_timer_vector(int timer)
{
    if (timer_list[timer].inuse == TRUE)
        return timer_list[timer].vector;
    else
        return ERR;
}


/************************************************/
/* get_timer_irq                    */
/*                         */
/************************************************/
unsigned int 
get_timer_irq(int timer)
{
    if (timer_list[timer].inuse == TRUE)
        return timer_list[timer].irq;
    else
        return ERR;
}

/************************************************/
/* get_timer_intr                    */
/*                         */
/************************************************/
unsigned int 
get_timer_intr(int timer)
{
    if (timer_list[timer].inuse == TRUE)
        return timer_list[timer].intr_active;
    else
        return FALSE;
}

/************************************************/
/* get_bentime_timer                    */
/*                         */
/************************************************/
int 
get_bentime_timer(void)
{
        return (int)bentime_timer;
}

/************************************************/
/* get_default_bentime_isr                      */
/*                                              */
/************************************************/
void * 
get_default_bentime_isr(void)
{
        return (_bentime_isr);
}

/************************************************/
/* get_ghist_timer                    */
/*                         */
/************************************************/
unsigned int 
get_ghist_timer()
{
        return (int)ghist_timer;
}

/************************************************/
/* get_ghist_callback                           */
/*                                              */
/************************************************/
GHIST_FNPTR
get_ghist_callback()
{
        return ghist_timer_callback;
}

/************************************************/
/* get_timer_inuse                    */
/*                         */
/************************************************/
unsigned int 
get_timer_inuse(int timer)
{
        return timer_list[timer].inuse;
}

/************************************************/
/* set_timer_inuse                    */
/*                         */
/************************************************/
unsigned int 
set_timer_inuse(int timer)
{
        if (timer_list[timer].inuse == TRUE)
            return ERR;

        timer_list[timer].inuse = TRUE;
        return OK;
}

/************************************************/
/* set_timer_not_inuse                    */
/*                         */
/************************************************/
unsigned int 
set_timer_not_inuse(int timer)
{
        if (timer_list[timer].inuse == FALSE)
            return ERR;

        timer_list[timer].inuse = FALSE;
        timer_list[timer].intr_active = FALSE;

        return OK;
}


/************************************************/
/* Function:   void timer_for_onboard_only(int onboard_only)       */
/*                                              */
/* Passed:     void.                            */
/*                                              */
/* Returns:    void.                            */
/*                                              */
/* Action:    set bentime counts for on board time only    */
/************************************************/
void
timer_for_onboard_only(int onboard_only)
{
    time_onboard_only = onboard_only;
}


/************************************************/
/* Function:   void get_onboard_only()       */
/*                                              */
/* Passed:     void.                            */
/*                                              */
/* Returns:    void.                            */
/*                                              */
/* Action:     return state on time_onboard_only for timer*/
/************************************************/
unsigned int
get_onboard_only()
{
    return time_onboard_only;
}
