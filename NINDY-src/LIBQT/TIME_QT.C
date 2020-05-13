/******************************************************************/
/* 		Copyright (c) 1989, Intel Corporation

   Intel hereby grants you permission to copy, modify, and 
   distribute this software and its documentation.  Intel grants
   this permission provided that the above copyright notice 
   appears in all copies and that both the copyright notice and
   this permission notice appear in supporting documentation.  In
   addition, Intel grants this permission provided that you
   prominently mark as not part of the original any modifications
   made to this software or documentation, and that the name of 
   Intel Corporation not be used in advertising or publicity 
   pertaining to distribution of the software or the documentation 
   without specific, written prior permission.  

   Intel Corporation does not warrant, guarantee or make any 
   representations regarding the use of, or the results of the use
   of, the software and documentation in terms of correctness, 
   accuracy, reliability, currentness, or otherwise; and you rely
   on the software, documentation and results solely at your own 
   risk.							  */
/******************************************************************/
#include "qtcommon.h"

/* iac structure */
typedef struct {
	unsigned short 	field2;
	unsigned char 	field1;
	unsigned char	message_type;
	unsigned int	field3;
	unsigned int	field4;
	unsigned int	field5;
} iac_struct;

#define TIMER_VECTOR 	255
unsigned int bentime_interrupts;

/*
 * Interrupt routines MUST be written in assembler!!
 * (to avoid clobbering global registers).
 */

extern void default_isr(void), timer_isr(void);

/************************************************/
/* BENTIME					*/
/* 						*/
/* This routine reads the counter values and    */
/* returns the uS value.			*/
/************************************************/
unsigned int bentime()
{
int tot_time;
unsigned int LSB, MSB;
unsigned int count;

	/* quickly freeze counter values */
	store_quick_byte (0x00, CWR2);  /* latch timer value */
	count = bentime_interrupts;

	LSB = load_byte (CR3); 	/* read timer value */
	MSB = load_byte (CR3);

	tot_time = (MSB << 8) + LSB; 	/* get number of uS on timer */
	tot_time = 0xc000 - tot_time;
	tot_time = (float)tot_time * CRYSTALTIME;

	/* add 5mS for each tick of interrupt counter */
	tot_time = (count * 5000) + tot_time;

	return (tot_time);
}

/************************************************/
/* INIT_BENTIME					*/
/* 						*/
/* This routine initializes the 82380 interrupt */
/* and timer registers and initializes the timer*/
/* count. It returns the overhead for the 	*/
/* bentime() call.				*/
/************************************************/
unsigned int init_bentime(x)
int x; 	/* not used for this timer, but kept for compatability */
	/* with the EVA init_bentime routine */
{
iac_struct iac;
unsigned int *int_table;
unsigned int *prcb;
unsigned int system_base[2];
unsigned int overhead;
int i;

	bentime_interrupts = 0;

/*	We need to locate the interrupt table in order to 
	enter our new vector into the table.  We will do this
	by first finding the PRCB (Processor control block) base
	and then indexing into it to find the interrupt table.
	Once we have the interrupt table, we can index into it
	and put the vector into the appropriate spot.
*/
	
	/* Store System Base IAC */
	iac.message_type = 0x80;

	/* place address of buffer */
	iac.field3 = (unsigned int)system_base;

	/* issue the IAC message */
	send_iac((int)&iac);

	/* move PRCB to prcb pointer */
	prcb = (unsigned int *)system_base[1];	

	/* Interrupt table is here */
	int_table = (unsigned int *)prcb[5]; 

	/* clear off any pending interrupts from interrupt table */
	for (i=0 ; i <= 8 ; i++)
		int_table[i] =  0;
	
	/* set our timer vector */
	int_table[TIMER_VECTOR] = (unsigned int) default_isr;

	/* set our timer vector */
	int_table[TIMER_VECTOR+1] = (unsigned int) timer_isr;

	init_interrupts();
	init_timer();

	overhead = bentime();
	overhead = bentime() - overhead;

	return (overhead);
}
	
/************************************************/
/* INIT_INTERRUPTS				*/
/*						*/
/* Initialize 82380.  this code is specific to  */
/* the 82380 and the QT960 board.  This code 	*/
/* will probably have to be modified if ported  */
/* to other hardware 				*/
/************************************************/
init_interrupts()
{
unsigned char dummy;

	/* do dummy read of mask register for banks  Not sure
	why this is necessary, but the 380 doesn't seem to work
	without it.
	*/

	dummy = load_byte(ICW1_B);
	dummy = load_byte(ICW2_B);
	
	dummy = load_byte(ICW1_A);
	dummy = load_byte(ICW2_A);

	/* set up a vector for the level 1.5 interrupt just in case */
	store_byte (TIMER_VECTOR-1, VR1_5); /* interrupt vector 1.5 */

	/* set up ICW for Master controller (82380) */
	store_byte (0x11, ICW1_A); /* edge triggered, external */
				   /* cascade, ICW4 needed */

	/* neutralize all vectors */
	store_byte (TIMER_VECTOR, VR0);   /* interrupt vector 0 */
	store_byte (TIMER_VECTOR-1, VR1); /* interrupt vector 1 */
	store_byte (TIMER_VECTOR-1, VR3); /* interrupt vector 3 */
	store_byte (TIMER_VECTOR-1, VR4); /* interrupt vector 4 */
	store_byte (TIMER_VECTOR-1, VR7); /* interrupt vector 7 */
		
	store_byte (0x20, ICW2_A );    	/* change ICW2 */
	store_byte (0x04, ICW2_A );    	/* change ICW3 */
	store_byte (0x12, ICW4_A );	/* change ICW4 */
	store_byte (0xfa, OCW1_A); 	/* mask bank A */

/* program slave interrupt controller */

	store_byte (0x11, ICW1_A); /* level triggered */
				   /*  ICW4 needed */

	/* neutralize all vectors */
	store_byte (TIMER_VECTOR-1, VR8);  /* interrupt vector 8 */
	store_byte (TIMER_VECTOR-1, VR9);  /* interrupt vector 9 */
	store_byte (TIMER_VECTOR-1, VR11); /* interrupt vector 11 */
	store_byte (TIMER_VECTOR-1, VR12); /* interrupt vector 12 */
	store_byte (TIMER_VECTOR-1, VR13); /* interrupt vector 13 */
	store_byte (TIMER_VECTOR-1, VR14); /* interrupt vector 14 */
	store_byte (TIMER_VECTOR-1, VR15); /* interrupt vector 15 */
		
	store_byte (0x20, ICW2_A );    	/* change ICW2 */
	store_byte (0x02, ICW2_A );    	/* change ICW3 */
	store_byte (0x12, ICW4_A );	/* change ICW4 */
	store_byte (0xfe, OCW1_A); 	/* mask bank B */

	/* clear off 1.5 level interrupts */

	dummy = load_byte (ICW1_B);
	dummy = load_byte (ICWR_B);
	dummy = load_byte (ICW1_A);
	dummy = load_byte (ICWR_A);
	dummy = load_byte (ICWR_C);

}

/************************************************/
/* INIT_TIMER					*/
/*						*/
/* Initialize 82380.  this code is specific to  */
/* the 82380 and the QT960 board.  This code 	*/
/* will probably have to be modified if ported  */
/* to other hardware 				*/
/************************************************/
init_timer()
{
	/* first freeze timers 0, 1, and 2 to prevent them from
	   ticking
	*/

	store_byte (0x00, CWR1); /* latch,16-bit binary,timer 0 */
	store_byte (0x40, CWR1); /* latch,16-bit binary,timer 1 */
	store_byte (0x80, CWR1); /* latch,16-bit binary,timer 2 */
	
	/* turn on timer 3 for clock, mode 2 */

	store_byte (0x34, CWR2);   /* LSB,MSB,16-bit binary */
	store_byte (0x00, CR3);    /* LSB initial count */
	store_byte (0xc0, CR3);    /* MSB initial count */
	
	/* enable timer to cause interrupt */

	store_byte (0xfe, OCW1_A); /* enable bank A */
	store_byte (0xff, OCW1_B); /* disable bank B */

	store_byte(0x7f, CNTL1);   /* enable GATE */

}

/************************************************/
/* TERM_BENTIME					*/
/* 						*/
/* This routine turns off the counter		*/
/************************************************/
term_bentime()
{
	store_byte (0xff, OCW1_A); /* disable bank A */
	store_byte(0x00, CNTL1);   /* disable GATE */
	store_byte (0x10, CWR2);   /* turn off counter */
	store_byte (0x02, CWR2);   /* turn off counter */
}

/************************************************/
/* Wait States					*/
/*             					*/
/* This function changes the waitstates on the  */
/* QT960 board.  The waitstates can be varied   */
/* from 0 to 3 independently for the first 	*/
/* memory access and subsequent burst accesses  */
/* in multiple word accesses.  First is the 	*/
/* number of waitstates for the first access	*/
/* and burst is the waitstates for subsequent   */
/* accesses.					*/
/************************************************/
wait_states(first, burst, stretch)
int first, burst, stretch;
{
int *f, *b, *s;

	f = (int *)0x2800000c;
	b = (int *)0x28000008;
	s = (int *)0x28000010;
	*f = first;
	*b = burst;
	*s = stretch;
}

/************************************************/
/* Reset Priority				*/
/*             					*/
/* This routine sets the priority of the process*/
/* it is called from to zero.                   */
/************************************************/
reset_priority()
{
	lower_priority(0);
}

