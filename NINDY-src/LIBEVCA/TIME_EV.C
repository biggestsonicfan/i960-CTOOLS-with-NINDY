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
extern unsigned int control_table[16];

unsigned volatile int interrupt_counter;
unsigned int interrupts_work;

#define ASV_CR0 0xdfe00000
#define ASV_CR1 0xdfe00001
#define ASV_CR2 0xdfe00002
#define ASV_CWR 0xdfe00003

#define TIMER_VECTOR 	226
#define	CRYSTALTIME	8	/* for the 8.0 MHz crystal */

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
#ifdef TIMER_0
	store_quick_byte (0x00, ASV_CWR);	/* latch timer 0 value */
	LSB = load_byte (ASV_CR0);	 	/* read timer value */
	MSB = load_byte (ASV_CR0); 
#else
	store_quick_byte (0x80, ASV_CWR);	/* latch timer 2 value */
	LSB = load_byte (ASV_CR2);		/* read timer value */
	MSB = load_byte (ASV_CR2);
#endif
	count = interrupt_counter;

	tot_time = (MSB << 8) + LSB; 	/* get number of uS on timer */
	tot_time = 50000 - tot_time;
	tot_time = tot_time / CRYSTALTIME; 

	/* add mS for each tick of interrupt counter */
	tot_time = (count * 50000/CRYSTALTIME) + tot_time;

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
unsigned int overhead;
unsigned int mask, pending;
int loop_counter;
int wait_counter;

int i;

extern void timer_isr();

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
overhead = bentime();	
overhead = bentime() - overhead;
return (overhead);
}
	
/************************************************/
/* INIT_INTERRUPTS				*/
/************************************************/
init_interrupts(unsigned int *control_table)
{
unsigned int mask;

	/* turn on interrupt level in control word.  for the ASV board,
		this level is hardwired at interrupt two (or three).  
		Therefore, we will hard set this code to modify the 
		control table for loading and initializing in dedicated 
		mode, with level two (or three) at highest priority
		0xe becomes interrupt vector 0xe2 or 226.
	*/
	
#ifdef TIMER_0
	control_table[4] = 0x00e00;	/* IMAP 0	*/
#else
	control_table[4] = 0x0e000;	/* IMAP 0	*/
#endif
	control_table[5] = 0x0;		/* IMAP 1	*/		
	control_table[6] = 0x0;		/* IMAP 2	*/
	control_table[7] = 0x043fc;	/* ICON programming */
	send_sysctl(0x401, 0, 0);	/* load processor */
	
	mask = get_mask();
	set_pending(get_pending() & 0xfffff000);
#ifdef nds_TIMER_0
	set_mask(mask | 0x04); 
#else
	set_mask(mask | 0x08); 
#endif

	change_priority(0);
}

/************************************************/
/* INIT_TIMER					*/
/************************************************/
init_timer()
{
#ifdef TIMER_0
	/* turn on timer 0 for clock, mode 2 */

	/* LSB,MSB,16-bit binary */
	store_byte (0x34, ASV_CWR); 

	/* LSB initial count */
	store_byte ((50000 & 0xff), ASV_CR0);

	/* MSB initial count */
	store_byte (((50000 >> 8) & 0xff), ASV_CR0);
#else
	/* turn on timer 2 for clock, mode 2 */

	/* LSB,MSB,16-bit binary */
	store_byte (0xb4, ASV_CWR);  

	/* LSB initial count */
	store_byte ((50000 & 0xff), ASV_CR2); 

	/* MSB initial count */
	store_byte (((50000 >> 8) & 0xff), ASV_CR2); 
#endif
}

/************************************************/
/* TERM_BENTIME					*/
/* 						*/
/* This routine turns off the counter		*/
/************************************************/
term_bentime()
{
/*	store_byte (0x00, ASV_CWR);     LSB,MSB,16-bit binary */
}

