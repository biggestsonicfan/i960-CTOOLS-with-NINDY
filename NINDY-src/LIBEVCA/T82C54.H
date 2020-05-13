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
/*-------------------------------------------------------------*/
/*
 * t82c54.h header for 82c54-2 timer/couter
 *
 */
/*-------------------------------------------------------------*/

                              /* for ASV */
#define TIMER_BASE            0xDFE00000

typedef volatile unsigned char * const vol_const_ptr;

#define COUNTER_0       (*(vol_const_ptr)(TIMER_BASE))   
#define COUNTER_1       (*(vol_const_ptr)(TIMER_BASE+1)) 
#define COUNTER_2       (*(vol_const_ptr)(TIMER_BASE+2)) 
#define COUNTER_CONTROL (*(vol_const_ptr)(TIMER_BASE+3)) 

/* Control Word Format */
#define SC(sc)    ((sc)<<6)
#define RW(rw)    ((rw)<<4)
#define MODE(m)   ((m)<<1)
#define BCD       (1)

/* Readback Commands */
#define READBACK     (3<<6)
#define LATCH_COUNT  (1<<4)
#define LATCH_STATUS (1<<5)
#define CNT_0        (1<<1)
#define CNT_1        (1<<2)
#define CNT_2        (1<<3)

/* Status Bits */
#define OUTPUT_BIT      (1<<7)
#define NULL_COUNT_BIT  (1<<6)
#define RW1_BIT         (1<<5)
#define RW0_BIT         (1<<4)
#define M2_BIT          (1<<3)
#define M1_BIT          (1<<2)
#define M0_BIT          (1<<1)
#define BCD_BIT         (1<<0)
