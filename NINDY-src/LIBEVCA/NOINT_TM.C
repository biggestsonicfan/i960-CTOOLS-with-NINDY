
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

/************************************************/
/*						*/
/* noint_timer.c  benchmark timing functions for*/
/* 		the ASV    			*/
/* 						*/
/* NOTE:					*/
/*   counters 0 and 1 are used to form a 32 	*/
/* bit timer.  The 82C54 counter rolls over 	*/
/* every seven minutes.  Calls to bentime() 	*/
/* that are over 7 minutes apart will be 	*/
/* inaccurate.					*/
/************************************************/

#include <types.h>
#include "t82c54.h"

#define  ROLL_32_BITS   0xFFFFFFFF

static int ticks_per_usec;


/************************************************
 *						
 * Function:   long bentime_noint(void)		
 *
 * Passed:     void.
 * 
 * Returns:    The current timer value, in microseconds,
 *             of the free running timer.
 *             
 * Action:     Reads the timer. Converts the time to 
 *             microseconds.
 *
 *
 ************************************************/
long
bentime_noint(void)
   {
   unsigned long new_count;
   static unsigned long old_count=ROLL_32_BITS; /* loaded in init */
   static unsigned long time=0;

                                        /* latch counter 0 and 1 */
   COUNTER_CONTROL = READBACK | LATCH_COUNT | CNT_0 | CNT_1; 
   new_count  = COUNTER_0;              /* read as 32-bit timer */
   new_count += COUNTER_0 <<8;
   new_count += COUNTER_1 <<16;
   new_count += COUNTER_1 <<24;


   /* take the difference to simulate a count up timer   */
   /* check for a 32 bit roll over                       */

   if(new_count > old_count)
      {                    /* 32 bit roll-over  */
      time += (ROLL_32_BITS/ticks_per_usec - 
              new_count/ticks_per_usec) + 
              old_count/ticks_per_usec;
      }
   else
      {
      time += (old_count - new_count)/ticks_per_usec;
      }
   old_count = new_count;
   return(time);
   }

/************************************************
 *
 * Function:   long init_bentime_noint(int Mhz)
 *
 * Passed:     int Mhz;  
 *             Passed the frequency of the couters oscillator,
 *             in Mhz.
 *
 * Returns:    The overhead value to call bentime()
 *             in a long integer. 
 *             
 *
 * Action:     Initializes the free-running timer.
 *
 *       
 ************************************************/
long 
init_bentime_noint(int Mhz)
   {
   long overhead;

   ticks_per_usec = Mhz;

   COUNTER_CONTROL = SC(0) | RW(3)| MODE(2); /* setup timers */
   COUNTER_CONTROL = SC(1) | RW(3)| MODE(2); /* see t82c54.h */

   COUNTER_0 = 0;    /* LSB of 16 bit value */
   COUNTER_0 = 0;    /* MSB of 16 bit value */
                
   COUNTER_1 = 0;    /* LSB of 16 bit value */
   COUNTER_1 = 0;    /* MSB of 16 bit value */
                
   do
      {
      COUNTER_CONTROL = READBACK | LATCH_STATUS | CNT_1; /* latch status */
      }
     while(COUNTER_1 & NULL_COUNT_BIT);   /* has counter been written ? */


   overhead = bentime_noint();
   overhead = bentime_noint() - overhead;

   return(overhead);
   }

/************************************************ 
 *
 * Function:   void term_bentime_noint(void)
 *
 * Passed:     void.
 * 
 * Returns:    void.
 *             
 * Action:     NULL. provided for compatability
 *
 *
 ************************************************/
void
term_bentime_noint(void)
   {
   }
