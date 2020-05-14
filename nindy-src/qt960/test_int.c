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
#include "defines.h"
#include "iac.h"
#include "qtcommon.h"

#define DUMMY_VECTOR 	8
#define CLOCK_TICKS	0x1333

extern unsigned int intr_table[];	/* interrupt table in rom */
static volatile unsigned int int_test_count;

/************************************************/
/* DEFAULT INTERRUPT SERVICE ROUTINE		*/
/************************************************/
default_isr()
{
}

/************************************************/
/* TIMER INTERRUPT SERVICE ROUTINE		*/
/************************************************/
test_isr()
{	

	store_byte (0xff, OCW1_A); /* disable bank A */
	store_byte(0x00, CNTL1);   /* disable GATE */
	store_byte (0x10, CWR2);   /* turn off counter */
	store_byte (0x02, CWR2);   /* turn off counter */
	int_test_count++; 
}



/************************************************/
/* 	INTERRUPT TEST A	    		*/
/************************************************/
interrupt_test_a()
{
iac_struct iac;
unsigned int *int_table;
unsigned int *prcb;
unsigned int system_base[2];
unsigned int overhead;
int i, j, q;
unsigned char dummy;

	int_test_count = 0;

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
		int_table[i] = (unsigned int) 0;

	/* fill the interrupt vector table with default */
	for (i = 8; i <=255; i++)
		int_table[i+1] = (unsigned int) default_isr;


	/* initialize the 82380 interrupt controller */

	/* do dummy read of mask register for banks  Not sure
	why this is necessary, but the 380 doesn't seem to work
	without it.
	*/

	dummy = load_byte(ICW1_B);
	dummy = load_byte(ICW2_B);
	
	dummy = load_byte(ICW1_A);
	dummy = load_byte(ICW2_A);

	/* set up a vector for the level 1.5 interrupt just in case */
	store_byte (DUMMY_VECTOR, VR1_5); /* interrupt vector 1.5 */

	/* set up ICW for Master controller (82380) */
	store_byte (0x11, ICW1_A); /* edge triggered, external */
				   /* cascade, ICW4 needed */

	/* neutralize all vectors */
	store_byte (DUMMY_VECTOR, VR0);   /* interrupt vector 0 */
	store_byte (DUMMY_VECTOR, VR1); /* interrupt vector 1 */
	store_byte (DUMMY_VECTOR, VR3); /* interrupt vector 3 */
	store_byte (DUMMY_VECTOR, VR4); /* interrupt vector 4 */
	store_byte (DUMMY_VECTOR, VR7); /* interrupt vector 7 */
		
	store_byte (0x20, ICW2_A );    	/* change ICW2 */
	store_byte (0x04, ICW2_A );    	/* change ICW3 */
	store_byte (0x12, ICW4_A );	/* change ICW4 */
	store_byte (0xfa, OCW1_A); 	/* mask bank A */

/* program slave interrupt controller */

	store_byte (0x11, ICW1_A); /* level triggered */
				   /*  ICW4 needed */

	/* neutralize all vectors */
	store_byte (DUMMY_VECTOR, VR8);  /* interrupt vector 8 */
	store_byte (DUMMY_VECTOR, VR9);  /* interrupt vector 9 */
	store_byte (DUMMY_VECTOR, VR11); /* interrupt vector 11 */
	store_byte (DUMMY_VECTOR, VR12); /* interrupt vector 12 */
	store_byte (DUMMY_VECTOR, VR13); /* interrupt vector 13 */
	store_byte (DUMMY_VECTOR, VR14); /* interrupt vector 14 */
	store_byte (DUMMY_VECTOR, VR15); /* interrupt vector 15 */
		
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

	store_byte(0x00, CNTL1);   /* disable GATE */
	
	/* now loop through each possible interrupt vector and 
		see if we can cause an interrupt */

	for (i=255; i>=8; i--) {

		int_test_count = 0;

		/* install interrupt vector */
	
		int_table[i+1] = (unsigned int) test_isr;
		if (i != 255 )
			int_table[i+2] = (unsigned int) default_isr;

		change_priority((i/8)-1);  /* set processor priority */
		
		start_ticks(i);

		/* interrupt should take approximately 500 us to occur 
		  we will give it about 3ms before signaling an error */

		for (j=0; j<50000; j++)
			if (int_test_count) 
				break;
	
		if (!int_test_count) {
#ifdef DEBUG
			prtf(" interrupt test #1: error, did not receive interrupt %B\n",i);
#endif
			/* restore table */
			for (q=0; q<=256; q++)
				int_table[q] = intr_table[q];
			return(ERROR);
		}
	}		
	store_byte(0x00, CNTL1);   /* disable GATE */
	
	/*   Next test: see if we can get posted interrupts */
	
	/* clear off any pending interrupts from interrupt table */
	for (i=0; i<=8; i++)
		int_table[i] =  (unsigned int) 0;

	/* fill the interrupt vector table with default */
	for (i=8; i<=255; i++)
		int_table[i+1] = (unsigned int) test_isr;

	change_priority(31);

	for (i=247; i>=8; i--) {
		start_ticks(i);

		/* interrupt should take approximately 500 us to occur 
		  we will give it about 3ms before signaling an error */

		for (j=0; j<5000; j++)
			q = int_test_count;	
	
		if (!((unsigned int)int_table[0] & (1 << (i/8)))) { 
#ifdef DEBUG
			prtf(" interrupt test #2: error, did not receive interrupt %B\n",i);
#endif
			/* restore table */
			for (q=0; q<=256; q++)
				int_table[q] = intr_table[q];
			return(ERROR);
		}
	}		

	/* now let all the interrupts through */
	int_test_count = 0;
	
	change_priority(0);
	for(j=0; j<1000000; j++)
		if (int_test_count == 240) 
			break;
	
	/* reset interrupt table  */
	for (i=8; i<=255; i++)
		int_table[i+1] = (unsigned int) default_isr;;

	/* restore table */
	for (q=0; q<=256; q++)
		int_table[q] = intr_table[q];
	if (int_test_count != 240) 
		return(ERROR);
	else
		return(0);
}
	
/************************************************/
/* 	START TICKS     	    		*/
/************************************************/
start_ticks(vector)
int vector;
{
/* first freeze timers 0, 1, and 2 to prevent them from ticking */

	store_byte (0x00, CWR1); /* latch,16-bit binary,timer 0 */
	store_byte (0x40, CWR1); /* latch,16-bit binary,timer 1 */
	store_byte (0x80, CWR1); /* latch,16-bit binary,timer 2 */
	
	/* turn on timer 3 for clock, mode 0 */
	/* timer will count to termination, then interrupt processor */

	store_byte (0x30, CWR2);   /* LSB,MSB,16-bit binary */

	/* LSB initial count */
	store_byte ((CLOCK_TICKS & 0xff), CR3); 

	/* MSB initial count */
	store_byte (((CLOCK_TICKS & 0xff00) >> 8), CR3);
	
	/* enable timer to cause interrupt */
	store_byte (0xfe, OCW1_A); /* enable bank A */
	store_byte (0xff, OCW1_B); /* disable bank B */

	/* put vector into 380 interrupt controller */
	store_byte (vector, VR0);   /* interrupt vector 0 */
	store_byte(0x7f, CNTL1);   /* enable GATE */
}
