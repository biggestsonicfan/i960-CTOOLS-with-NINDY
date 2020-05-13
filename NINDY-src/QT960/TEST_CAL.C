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
#include "test_def.h"
#include "defines.h"
#include "qtcommon.h"

/****************************************/
/*	RESET ERROR			*/
/****************************************/
reset_error()
{
volatile unsigned char *led_ptr;
int i;

	led_ptr = (unsigned char *) USER_LED23;
    	while (TRUE) {
        	*led_ptr = 0x0;     /* blinks user LED 2-3 if error */
        	for (i=0; i<1000000; i++);
        	*led_ptr = 0x3;
        	for (i=0; i<1000000; i++);
    	}
}

/****************************************/
/*	PRE MAIN   			*/
/* this is the procedure which will test*/
/* reset flags and determine which if   */
/* any tests are to be run 		*/
/****************************************/
pre_main ()
{
volatile unsigned int *reset_ptr;
volatile unsigned char *test_ptr;

    	reset_ptr = (unsigned int *)RESET_CSR;
    	test_ptr = (unsigned char *)TEST_POINT;

    	if ((*reset_ptr & 0x3) == 0) {/* Power up board reset */

		do {
			a_tests();   /* run self test */
		} while ((*test_ptr & 0x1) == 1);
		/* while test point grounded, run as burn-in tests */

     		prtf("\rconfidence tests complete\n");
		reset_test();
		reset_error();
	}
	else
		main();	    /* other reset, do nothing */
}

/****************************************/
/*	A TESTS    			*/
/****************************************/
a_tests()
{
   	serial_test_a();
	prtf("\r                                 ");
	prtf("                                   ");

	prtf ("\rtesting LEDs       ");
     	led_test_a();

	prtf ("\rtesting EPROMs     ");
     	eprom_test_a();

	prtf ("\rtesting CSRs       ");
     	csr_test_a();

	prtf ("\rtesting FLASH      ");
     	flash_test_a();

	prtf ("\rtesting interrupts ");
     	interrupt_test_a();

	prtf ("\rtesting timers     ");
     	timer_test_a();

	prtf ("\rtesting PWSG       ");
     	pwsg_test_a();
}

/****************************************/
/*	RESET TEST 			*/
/* this test  sends the reset 		*/
/* command to the 960 through the 380   */
/****************************************/
reset_test()
{
   	prtf("self test complete -- resetting Processor\n");

   	/* sets flags to branch to main in a_call after reset */
	*((unsigned char *)RESET_CSR) = 3;

	/* reset board */
	*((unsigned char *)RESET_ADDR) = RESET_DATA;
}
