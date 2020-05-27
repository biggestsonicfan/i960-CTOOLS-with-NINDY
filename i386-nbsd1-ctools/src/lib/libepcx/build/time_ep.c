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
extern unsigned int control_table[16];

unsigned volatile int interrupt_counter;
unsigned int interrupts_work;

static void
spinwait(int count){
  volatile unsigned foo;
  for(;count--;)
    foo = count;
}

/************************************************/
/* BENTIME					*/
/* 						*/
/* This routine reads the counter values and    */
/* returns the uS value.			*/
/************************************************/
unsigned int bentime()
{
unsigned int tot_time, LSB, MSB;
unsigned int count;

	/* quickly freeze counter values */
flush:
	count = interrupt_counter;
	store_quick_byte (0x80, CONTROL_REG);	/* latch timer 2 value */
	LSB = load_byte (COUNTER_2);		/* read timer value */
	MSB = load_byte (COUNTER_2);

	if(count != interrupt_counter)
	  goto flush;

	tot_time = (MSB << 8) + LSB; 	/* get number of uS on timer */
	tot_time = 50000 - tot_time;
	tot_time = tot_time / CRYSTALTIME; 

	/* add mS for each tick of interrupt counter */
	tot_time = (count * (50000/CRYSTALTIME)) + tot_time;

	return (tot_time);
}

/************************************************/
/* INIT_BENTIME					*/
/* 						*/
/* This routine initializes the CA    interrupt */
/* and 8254  registers and initializes the timer*/
/* count. It returns the overhead for the 	*/
/* bentime() call.				*/
/************************************************/
unsigned int init_bentime(x)
int x; 	/* not used for this timer, but kept for compatability */
	/* with the EVA init_bentime routine */
{
unsigned int *int_table;
unsigned int *prcb;
unsigned int *control_table;
unsigned int overhead, this_time;
unsigned int mask, pending;
int loop_counter;
int wait_counter;

int i;

extern void timer_isr();

unsigned int startsmall, stopsmall, startbig, stopbig, extrap_small, newoh;
int small, big;


/* this is a two state program. If interrupts_work is 0, this is the 
first time into this program, and we must do total initialization.  
Otherwise, just do partial initialization
*/


interrupt_counter = 0;

  if (!(interrupts_work)) {

	/* We need to locate the interrupt table in order to 
	enter our new vector into the table.  We will do this
	by first finding the PRCB (Processor control block) base
	and then indexing into it to find the interrupt table.
	Once we have the interrupt table, we can index into it
	and put the vector into the appropriate spot.  */
	

	/* move PRCB to prcb pointer */
	prcb = (unsigned int *)get_prcb();	

	/* Control table is here */
	control_table = (unsigned int *)prcb[1]; 

	/* Interrupt table is here */
	int_table = (unsigned int *)prcb[4]; 

	/* set our timer vector */
	int_table[TIMER_VECTOR+1] = (unsigned int) timer_isr;

	init_interrupts(control_table);
	do {	/* hit the timer til it's started */
		init_timer();
		for (wait_counter = 0; wait_counter < 500000; wait_counter++) {
			if (interrupt_counter != 0) {
				break;
			}
		}
	} while(interrupt_counter == 0);

	interrupts_work = 1;

  }
/* begin new calculation of bentime overhead */
	small = 10;
	big = 100000;
	overhead = 0;

	/* use the old way */
    for (i=1; i<=100 ; i++)
		{
    	this_time = bentime();
	    overhead += bentime() - this_time;
		}
	overhead = overhead / 100;
	/* average of 100 calls should be about right. */

    interrupt_counter = 0;  /* reset so counter dos not give an extra intr */
	return(overhead);

	/* and then check against the old way */
	spinwait(small);
	startsmall = bentime(); spinwait(small); stopsmall = bentime();
	spinwait(small); /* hope an interrupt comes along */
	startbig = bentime(); spinwait(big); stopbig = bentime();

	/* note: the odd structure below is intentional
	 */
	if(startbig >= stopbig){ /* broken bentime */
	  spinwait(1);
	  return(overhead);
	}
	if(startsmall >= stopsmall){ /* small was "too small" */
	  spinwait(2);
	  return(overhead);
	}
	if(((stopbig - startbig) / (stopsmall - startsmall)) < 1000){
	  spinwait(3);
	  return(overhead);
	}

 	/* large enuf numbers sufficiently far apart ... */
	extrap_small = small * (stopbig - startbig) / big;
	newoh = (stopsmall - startsmall) - extrap_small;

	spinwait(4);
	
	if(newoh > overhead)
	  return(newoh);

	return(overhead);
/* end new calculation of bentime overhead */
}
	
/************************************************/
/* INIT_INTERRUPTS				*/
/************************************************/
init_interrupts(unsigned int *control_table)
{
unsigned int mask;

	/* turn on interrupt level in control word.  for the EPCX board,
		this level is hardwired at interrupt five.  
		Therefore, we will hard set this code to modify the 
		control table for loading and initializing in dedicated 
		mode, with level five at the highest priority
		0xe becomes interrupt vector 0xe2 or 226.
	*/
	
	/*
	 * Only change the bits we need in IMAP 1.
	 */
	control_table[5] = (control_table[5] & 0xffffff0f) | 0x0e0;

	/*
	 * You may not want to mess with some bits in the ICON.
	 * However, the following line does currently mess w/ them all.
	 */
	control_table[7] = 0x043fc;	/* ICON programming */
	send_sysctl(0x401, 0, 0);	/* load processor */

	mask = get_mask();
	set_pending(get_pending() & 0xfffff000);
	set_mask(mask | 0x020); 

	change_priority(0);
}

/************************************************/
/* INIT_TIMER					*/
/************************************************/
init_timer()
{
	/* turn on timer 2 for clock, mode 2 */

	/* LSB,MSB,16-bit binary */
	store_byte (0xb4, CONTROL_REG);  

	/* LSB initial count */
	store_byte ((50000 & 0xff), COUNTER_2); 

	/* MSB initial count */
	store_byte (((50000 >> 8) & 0xff), COUNTER_2); 
}

/************************************************/
/* TERM_BENTIME					*/
/* 						*/
/* This routine turns off all counters		*/
/************************************************/
term_bentime()
{
unsigned int mask;

	mask = get_mask();
	set_mask(mask & 0xffffffdf);  /* disable bit 5 = 0 */
	set_pending(get_pending() & 0xffffffdf);
}

