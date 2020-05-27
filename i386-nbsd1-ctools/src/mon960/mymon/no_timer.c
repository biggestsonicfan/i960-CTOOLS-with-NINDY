/*******************************************************************************
 * 
 * Copyright (c) 1995 Intel Corporation
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

#include "retarget.h"

unsigned int bentime_counter;

unsigned int bentime()
{
    return(ERR);
}

unsigned int init_bentime(x)
int x;     /* not used for this timer, but kept for compatability */
        /* with the EVA init_bentime routine */
{
    return(ERR);
}
    

void
term_bentime(void)
{
}

unsigned long
bentime_noint()
{
    return(ERR);
}

unsigned long
init_bentime_noint(int Mhz)
{
    return(ERR);
}

void
bentime_onboard_only(int onboard)
{
    return;
}

void
timer_suspend(void)
{
}

void
timer_resume(void)
{
}

int
timer_supported(void)
{
    return (TIMER_API_UNAVAIL);
}

void 
ghist_reload_timer(unsigned int count)
{
}

void
ghist_suspend_timer(void)
{
}

void
_init_p_timer(int count)
{
}

void
_term_p_timer(void)
{
}

void
_bentime_isr(void)
{
}

int
mon_set_timer(int dummy1, int dummy2, void * timer_isr)
{
   return ERR;
}
