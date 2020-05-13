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
#include "defines.h"
#include "test_def.h"


/************************************************/
/*	TIMER TEST A				*/
/* this routine will verify that the timers are */
/* functioning 					*/
/************************************************/
timer_test_a()
{
int i;
unsigned char *count0;
unsigned char *count1;
unsigned char *count2;
unsigned char *count3;
unsigned short start_count;
unsigned short end_count;

   	count0 = (unsigned char *)CR0;
   	count1 = (unsigned char *)CR1;
   	count2 = (unsigned char *)CR2;
   	count3 = (unsigned char *)CR3;
   	start_count = 0xffff;

       	/* selects counter 0; read/write MSB,LSB */
        /* sets mode 0; uses 16-bit binary       */
       	store_byte (0x30, CWR1);

	/* gives counter MSB initial count */
       	store_byte ((start_count & 0xff), CR0);

	/* gives LSB initial count   */
       	store_byte (((start_count >> 8) & 0xff), CR0);

       	for (i=0; i<1000; i++);

       	/* reads back count; latch count */
        /* don't latch status; counter selected */
       	store_byte (0x00, CWR1);

       	/* gives MSB end count */
       	end_count = (load_byte(CR0) << 8);

       	/* gives LSB end count */
       	end_count = (end_count | load_byte(CR0));

       	if (end_count == start_count) {
#ifdef DEBUG
      		prtf("timer test:  error in timer #0\n");
#endif
          	return(ERROR);
        }

       	store_byte (0x70, CWR1);

	/* gives counter MSB initial count */
       	store_byte ((start_count & 0xff), CR1);

	/* gives LSB initial count   */
       	store_byte (((start_count >> 8) & 0xff), CR1);

       	for (i=0; i<1000; i++);

       	/* reads back count; latch count */
       	/* don't latch status; counter selected */
       	store_byte (0x40, CWR1);

       	/* gives MSB end count */
       	end_count = (load_byte(CR1) << 8);

       	/* gives LSB end count */
       	end_count = (end_count | load_byte(CR1));

       	if (end_count == start_count) {
#ifdef DEBUG
      		prtf("timer test:  error in timer #1\n");
#endif
          	return(ERROR);
       	}

       	/* enable gates */
       	store_byte (0x41, CNTL1);
       	store_byte (0xb0, CWR1);

	/* gives counter MSB initial count */
       	store_byte ((start_count & 0xff), CR2);

	/* gives LSB initial count   */
        store_byte (((start_count >> 8) & 0xff), CR2);

       	for (i=0; i<1000; i++);

       	/* reads back count; latch count */
        /* don't latch status; counter selected */
       	store_byte (0x80, CWR1);

       	/* gives MSB end count */
       	end_count = (load_byte(CR2) << 8);

       	/* gives LSB end count */
       	end_count = (end_count | load_byte(CR2));

       	if (end_count == start_count) {
#ifdef DEBUG
      		prtf("timer test: error in timer #2, end count=%X",end_count);
      		prtf(" start count=%X\n", start_count);
#endif
	  	store_byte (0x00, CNTL1);
          	return(ERROR);
       	}

       	store_byte (0x30, CWR2);

	/* gives counter MSB initial count */
       	store_byte ((start_count & 0xff), CR3);

	/* gives LSB initial count   */
       	store_byte (((start_count >> 8) & 0xff), CR3);

       	for (i = 0; i < 1000; i++);

       	/* reads back count; latch count */
        /* don't latch status; counter selected */
       	store_byte (0x00, CWR2);   

       	/* gives MSB end count */
       	end_count = (load_byte(CR3) << 8);

       	/* gives LSB end count */
       	end_count = (end_count | load_byte(CR3));

       	if (end_count == start_count) {
#ifdef DEBUG
      		prtf("timer test:  error in timer #3\n");
#endif
	  	store_byte (0x00, CNTL1);
          	return(ERROR);
       	}

	/* disable gates */
	store_byte (0x00, CNTL1);

	return(0);
}

/****************************************/
/*	CSR TEST A			*/
/* this test will verify that the 	*/
/* control-status registers are properly*/
/* functioning 				*/
/****************************************/
csr_test_a()
{
volatile unsigned int *led_01;
volatile unsigned int *led_23;
volatile unsigned int *burst_cycle;
volatile unsigned int *first_cycle;
volatile unsigned int *write_stretch;
int i;

	led_01 = (unsigned int *)USER_LED01;
   	led_23 = (unsigned int *)USER_LED23;
   	burst_cycle = (unsigned int *)BURST_CYCLE;
   	first_cycle = (unsigned int *)FIRST_CYCLE;
   	write_stretch = (unsigned int *)WRITE_STRETCH;

   	for (i=0; i<=3; i++) {                             
        	*led_01 = (unsigned int)i;
        	*led_23 = (unsigned int)i;
        	*burst_cycle = (unsigned int)i;
        	*first_cycle = (unsigned int)i; 

/* verification */
        	if ((*led_01 & 0x3) != i) {
#ifdef DEBUG
          		prtf("error in %X\n", led_01);
#endif
              		while (TRUE)
				led_test_a();
           	}
        	if ((*led_23 & 0x3) != i) {
#ifdef DEBUG
          		prtf("error in %X", led_23);
#endif
	              	while(TRUE)
				led_test_a();
           	}
        	if ((*burst_cycle & 0x3) != i) {
#ifdef DEBUG
          		prtf("error in %X\n", burst_cycle);
#endif
              		while(TRUE)
				led_test_a();
           	}
        	if ((*first_cycle & 0x3) != i) {
#ifdef DEBUG
          		prtf("error in %X\n", first_cycle);
#endif
              		while(TRUE)
				led_test_a();
           	}
      	}

   	/* assigns 0-1 to verify write-stretch register */
   	for (i=0; i<=1; i++) { 
          	*write_stretch = i;

       		/* verification for correct value */
       		if ((*write_stretch & 0x3) != i) {
#ifdef DEBUG
          		prtf("error in %X  %X\n", write_stretch, i );
#endif
		     	while(TRUE)
				led_test_a();
	      	}
      	}
 }
