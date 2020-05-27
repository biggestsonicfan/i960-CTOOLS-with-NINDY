/*******************************************************************************
 * 
 * Copyright (c) 1993 Intel Corporation
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

#include "t82c54.h"

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


unsigned long bentime_noint();
unsigned long init_bentime_noint();
void term_bentime_noint();


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
        unsigned int lsb0=0, msb0=0, lsb1=0, msb1=0;
        unsigned long new_count;


        store_byte (0xd6, CONTROL_REG);  /* latch timers 0 & 1 value */

        lsb0 = (unsigned long)load_byte (COUNTER_0);   /* read timer 0 value */
        msb0 = (unsigned long)load_byte (COUNTER_0);
        lsb1 = (unsigned long)load_byte (COUNTER_1);   /* read timer 1 value */
        msb1 = (unsigned long)load_byte (COUNTER_1);

        new_count = lsb0 + (msb0 << 8) + (lsb1 << 16) + (msb1 <<24);


        /* take the difference to simulate a count up timer   */
        /* check for a 32 bit roll over                       */
        /* Note:  the division is necessary because tot_time  */
        /* stores microseconds, while old_count and new_count */
        /* store timer ticks (at 8MHz, or CRYSTALTIME).       */

        if(new_count > old_count)
                tot_time += (ROLL_32_BITS - new_count + old_count)/CRYSTALTIME;
        else
                tot_time += (old_count - new_count)/CRYSTALTIME;

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

/* turn on timer 0 for clock, mode 2 */
        store_byte (0x34, CONTROL_REG); /* lsb,msb,16-bit binary */
        store_byte (0x00, COUNTER_0); /* lsb MAX initial count */
        store_byte (0x00, COUNTER_0); /* msb MAX initial count */

/* turn on timer 1 for clock, mode 2 */
        store_byte (0x74, CONTROL_REG); /* lsb,msb,16-bit binary */
        store_byte (0x00, COUNTER_1); /* lsb MAX initial count */
        store_byte (0x00, COUNTER_1); /* msb MAX initial count */

        tot_time = 0;
        old_count = ROLL_32_BITS;

	overhd = -bentime_noint() + bentime_noint();
        return(overhd);
}

/************************************************/
/* Function:   void term_bentime_noint(void)    */
/*                                              */
/* Passed:     void.                            */
/*                                              */
/* Returns:    void.                            */
/*                                              */
/* Action:     terminates counter values        */
/************************************************/
void
term_bentime_noint(void)
{
        /* timer 0, mode 0, count down to 0 and shut up */
        store_byte (0x30, CONTROL_REG); /* LSB,MSB,16-bit binary */
        store_byte (0x02, COUNTER_0);   /* LSB initial count */
        store_byte (0x00, COUNTER_0);   /* MSB initial count */

        /* timer 1, mode 0, count down to 0 and shut up */
        store_byte (0x70, CONTROL_REG); /* LSB,MSB,16-bit binary */
        store_byte (0x02, COUNTER_1);   /* LSB initial count */
        store_byte (0x00, COUNTER_1);   /* MSB initial count */
}


