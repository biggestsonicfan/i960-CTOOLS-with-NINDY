/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994 Intel Corporation
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
/*)ce*/
/**************************************************************************
  Module Name:  EVCAPDRV.C

  Functions:  void post(void)
              static void ram_test(void)

  Post is called from the bootup code upon hardware reset and power-up.
  It reads the status of the DIP switches, in particular SW1, SW2, SW3,
  and SW4 (as labeled on the board) to determine what course of action
  should be taken.  The DIP switches may be set as follows:

  SWITCH	Function
  ------    -------------------------------------------------------------
   SW1      Enable DRAM tests
   SW2      Enable timer test
   SW3      Enable bus idle timer test
   SW4      Enable serial internal loopback test
   SW5      Enable serial external loopback test
   SW6      Loop on all enabled tests
   SW7      Loop on failing test
   SW8      Run diagnostics

   Diagnostics are run only if SW8 is in the ON position.

   The DRAM tests write a walking '1' pattern and a checkerboard
   pattern to memory to verify correct operation.
   The timer tests checks that the timer is counting down and that the
   OUT signal for each timer is operating correctly.
   The UART tests are done in two parts. The internal loopback test puts
   the UART in a local loopback mode and has the effect of internally
   connecting TxD with RxD.  For the UART external loopback test, the UART's
   TxD and Rxd must be connected at the serial port connector to allow testing
   of the driver circuitry as well as the UART.

   The identity of each test is displayed on the EP80960CXs seven segment LED
   as the test is executed. If an error is detected the "dot" character on the
   LED is illuminated.

   If a terminal is connected to the serial port a diagnostic message will be
   displayed indicating the error.  The terminal should be set for 9600 BAUD,
   8 bits, 1 stop bit and no parity. These diagnostic messages are not
   available during the two serial port tests.

   If SW7 is in the ON position a failing test will be executed
   continuously. If SW7 is in the OFF position, the 80960 will continue
   execution and try to initialize and run the monitor program even if there
   was a failure detected in a test.

   Post is entered via a BALX instruction.  The return address is
   stored in G14.  Internal RAM location 100H is utilized to save this
   return address in the event that the MONITOR will be re-entered upon
   successful completion of the diagnostic tests.

   *NOTE THAT jumpers should be set as follows for the Timer test to succeed:
	Should all be ON		Should both be OFF
	----------------		------------------

   **NOTE THAT YOU MUST make a physical connection of pin2 to pin3 of each
     serial port connector for the UART Interface test to succeed.

******************************************************************************/

#include "common.h"
#include "i960.h"
#include "epcx.h"
#include "epcxptst.h"
#include "retarget.h"
#include "16552.h"

static int serial_lp_test(void);
static int serial_if_test(void);
static unsigned long bus_idle_test(void);

int diag_write_string(char *pStringPtr);

extern led(), d_wait(), read_switch(), timer_test();
extern int inreg(), get_pending();

extern struct CONTROL_TABLE boot_control_table;
extern struct PRCB rom_prcb;
extern int curoffset;

/* Prototypes */

unsigned char *byte_test();
unsigned short *short_test();
unsigned long *word_test();
unsigned long *quad_word_test();
unsigned long *trip_word_test();
unsigned long *long_word_test();
unsigned char timer();
void diagerr(unsigned long, unsigned char);
static void post_led_display(void);
void dram_scan(void);


/*--------------------------------------------------------------
  Function:  unsigned long dram_test(int nLoopOnError)
  Action:    Acts as the main driver for performing the DRAM test.
  Passed:    Nothing.
  Returns:   Nothing.
--------------------------------------------------------------*/
static unsigned long dram_test(int nLoopOnError)
{
  unsigned long dram_size,result = 0;

  dram_size = (DRAM_SCAN_RESULT & 0x01f00000) - (unsigned long)DRAM_OFFSET;

  do{
    led(DRAM_BYTE_TEST);
	d_wait(200000);
    result = (unsigned long)byte_test((u_char *)DRAM_START, 256);
    diagerr(result, DRAM_BYTE_TEST);
  }while(result && nLoopOnError);

  if(result)
    return(result);

  do{
    led(DRAM_SHORT_TEST);
	d_wait(200000);
    result = (unsigned long)short_test((u_short *)DRAM_START, 256);
    diagerr(result, DRAM_SHORT_TEST);
  }while(result && nLoopOnError);

  if(result)
    return(result);
  do{
    led(DRAM_LONG_TEST);
	d_wait(200000);
    result = (unsigned long)long_word_test((u_long *)DRAM_START, 256);
    diagerr(result, DRAM_LONG_TEST);
  }while(result && nLoopOnError);

  if(result)
    return(result);

  do{
    led(DRAM_TRIP_TEST);
	d_wait(200000);
    result = (unsigned long)trip_word_test((u_long *)DRAM_START, 256);
    diagerr(result, DRAM_TRIP_TEST);
  }while(result && nLoopOnError);

  if(result)
    return(result);

  do{
    led(DRAM_QUAD_TEST);
	d_wait(200000);
    result = (unsigned long)quad_word_test((u_long *)DRAM_START, 256);
    diagerr(result, DRAM_QUAD_TEST);
  }while(result && nLoopOnError);

  if(result)
    return(result);

  do{
    led(DRAM_WORD_TEST);
	d_wait(200000);
    result = (unsigned long)word_test((u_long *)DRAM_START, dram_size);
    diagerr(result, DRAM_WORD_TEST);
  }while(result && nLoopOnError);

  return(result);

}


/*--------------------------------------------------------------
  Function:  void post(void)
  Action:    Diagnostic executive which determines which tests will
             be performed according the settings on the DIP switches.
             Called on hardware reset or power-up.
  Passed:    Nothing.
  Returns:   Nothing.
--------------------------------------------------------------*/
void _post_test(void){

  unsigned char dip_sw;
  int loop_on_err,i;
  unsigned long result;

				       /* post() called with BALX */
  asm("ldconst  0x100, r4");           /* save g14 in internal RAM */
  asm("st       g14, (r4)");           /* locaton 100H */
  asm("ldconst  0, g14");              /* zero g14 for compiler */

  led(-1);                           /* Blank LED */

  /* Read the DIP switches to determine if the POST is to be run */

  dip_sw = read_switch();				/* Now read the switch */
  if(dip_sw & SW_8){

  	asm("lda  _dram_scan,g0");
  	asm("balx (g0),g14");                   /* Branch to dram_scan */


	do{
  		dip_sw = read_switch();				/* Now read the switch as part of
											   the possibly repeating loop */
		post_led_display();

        curoffset = 0;         /* need to initialize for 16652.c code to work */
        serial_init();                  /* Set up port to 9600,N,8,1 */
    	serial_set(9600L);

    	loop_on_err = dip_sw & SW_7;
		result = 0;



    	for(i=0;(i<5);i++){

        	switch (dip_sw & (1<<i) ){        /* NON-zero result indicates a failure */

            	case 1:                  /* DRAM test */
                	result = dram_test(loop_on_err);
            	break;

            	case 2:                  /* Timer test */
            	do{
                	led(TIMER_TEST);
					d_wait(200000);
                	result = timer_test();
                	diagerr(result,TIMER_TEST);
            	}while(loop_on_err && result);
            	break;

            	case 4:                  /*  bus idle timer test */
            	do{
                	led(BUS_IDLE_TEST);
					d_wait(200000);
                	result = bus_idle_test();
                	diagerr(result,BUS_IDLE_TEST);
            	}while(loop_on_err && result);
            	break;

            	case 8:                  /* 16550 test */
            	do{
                	led(SERIAL_INTFC_TEST);
					d_wait(200000);
                	result = serial_if_test();
                	diagerr(result,SERIAL_INTFC_TEST);
            	}while(loop_on_err && result);
            	break;

            	case 16:                 /* Serial port driver test */
            	do{
                	led(SERIAL_LOOP_TEST);
					d_wait(200000);
                	result = serial_lp_test();
                	diagerr(result,SERIAL_LOOP_TEST);
            	}while(loop_on_err && result);
            	break;

        	}
    	}
  	}while (dip_sw & SW_6);
  }
  asm("ldconst  0x100, r4");           /* retrieve return address */
  asm("ld       (r4), r4");            /* from internal RAM */
  asm("bx       (r4)");                /* resume MONITOR */

}


/*--------------------------------------------------------------
  Function:  static void post_led_display(void)
  Action:    Provides a display on the hex LED to let the user know that
             the POST is going to run.
             Called on hardware reset or power-up.
  Passed:    Nothing.
  Returns:   Nothing.
--------------------------------------------------------------*/
static void post_led_display(void){

	int i;
	unsigned char *pLED = (unsigned char *)LED_8SEG_ADDR;

	for(i=0;i<4;i++){
		*pLED = 0xcf;
		d_wait(150000);
		*pLED = 0xf6;
		d_wait(150000);
		*pLED = 0xf9;
		d_wait(150000);
		*pLED = 0xbf;
		d_wait(150000);

	}

}


/*--------------------------------------------------------------
  Function: unsigned long dram_scan(void)
  Action:  Determines the amount of DRAM present on board. This is done
		   by writing and then reading the last byte of each megabyte.
 		   This board does not decode address bits 24 - 28 so all the checking
           done here is done inside of a 16 meg area. This function is expected to be
		   called using a branch and link instruction through register g14.
		   The result of the scan is the number of bytes of DRAM detected.
		   The uppermost nibble of the return value indicates the type of SIMM
		   detected. 1 = 1Mb SIMM, 4 = 4Mb SIMM.
		   It is written into the internal data RAM of the 960 at address 0x40.

  Passed:  Nothing
  Returns: Nothing
--------------------------------------------------------------*/
void dram_scan(void) {

	unsigned long i;
	volatile unsigned char *pTarget ;

  	asm("ldconst  0x104, r4");           /* save g14 in internal RAM */
  	asm("st       g14, (r4)");           /* locaton 104H */
  	asm("ldconst  0, g14");              /* zero g14 for compiler */

   pTarget = (unsigned char *) 0xc0f80000;

   i=16ul;
   d_wait(30000);
   for (      ;i>=1ul;i--,pTarget -= 0x100000){
	  *pTarget = (unsigned char)i;
	  *(pTarget + 4) = (unsigned char)((~i)&0xff);
   }

   pTarget = (unsigned char *) 0xc0080000;

   /* check for all bytes valid up to largest memory tested */
   /* fails we use the next lowest size */
   i=1ul;
   for(     ;i<=16ul;i++,pTarget += 0x100000)
	  {
	  if ((*pTarget != (unsigned char)i) ||
		  (*(pTarget + 4) != (unsigned char)((~i)&0xff)) )
		  {
       	  switch (i-1)
		      {
		      default:
                  led(0x1f);
                  break;
		      case 2:
		      case 3:
		         i=2ul;
		         break;
		      case 4:
	      	  case 5:
      		  case 6:
	      	  case 7:
		      	  i=4ul;
			      break;
		      case 8:
		      case 9:
	      		  i=8ul;
	      		  break;
	      	  case 10:
		      case 11:
		      case 12:
	      	  case 13:
	      	  case 14:
	      	  case 15:
	      		  i=10ul;
		      	  break;
		      }
		  break;
		  }
      else if (i==16)
		  /* maximum memory is 16meg so we stop at 16 */
		  break;
      }
	/* <<20 = size * 1mb to get dram_size saved in DRAM_SCAN_RESULT */
    DRAM_SCAN_RESULT = i<<20;

	asm("ldconst  0x104, r4");           /* retrieve return address */
  	asm("ld       (r4), r4");            /* from internal data RAM */
  	asm("bx       (r4)");                /* resume POST */
}


/***************************************************************************
               ###### SERIAL DIAGNOSTICS ########


***************************************************************************/

/*--------------------------------------------------------------------
  Function:  serial_if_test(void)
  Action:    Test the internal loopback capability of the serial port.
  Passed:    Nothing.
  Returns:   0 if test passes, 1 if test fails.
--------------------------------------------------------------------*/
static int serial_if_test(void)
{
  int ch, tmp;
  int count = 3;

  curoffset = 0; /* need to initialize for 16652.c code to work */

  serial_loopback(TRUE); /* configure for local loopback */
  for (ch = 32; ch < 128; ch++){
    serial_putc(ch);            /* write a character */
    d_wait(30000);             /* pause */

    /* read the character back, if fail return 1 */
	while (((tmp = serial_getc()) == -1) && (count-- > 0))
	    blink(SERIAL_INTFC_TEST); /* wait for character */

	led(SERIAL_INTFC_TEST);
	if (tmp != ch)
	  {
      serial_loopback(FALSE);/* test failed, clear mode */
      return(1L);
	  }
  }

  serial_loopback(FALSE);/* test passed, clear mode */
  return(0L);                   /* return 0 */
}

/*--------------------------------------------------------------------
  Function:  serial_lp_test(void)
  Action:    Test the UART and its associated interface circuitry.
  Passed:    Nothing.
  Returns:   0 if test passes, 1 if test fails.
--------------------------------------------------------------------*/
static int serial_lp_test(void)
{
  int ch, tmp;
  int count = 3;

  curoffset = 0; /* need to initialize for 16652.c code to work */

  for (ch = 32; ch < 127; ch++){
    serial_putc(ch);            /* write character */
    d_wait(30000);             /* pause */

    /* read character back, if fail return 1 */
	while (((tmp = serial_getc()) == -1) && (count-- > 0))
	    blink(SERIAL_LOOP_TEST); /* wait for character */

	led(SERIAL_LOOP_TEST);
	if (tmp != ch)
        return(1L);
  }
  return(0L);                   /* test passed, return 0 */
}


/*--------------------------------------------------------------------
  Function:  diag_write_string(char *pStringPtr)
  Action:    Output the indicated string to the serial port.
  Passed:    Pointer to string.
  Returns:   Nothing
--------------------------------------------------------------------*/
int diag_write_string(char *pStringPtr){
  while(*pStringPtr){
	serial_putc(*pStringPtr++);
  }

}
  

/*--------------------------------------------------------------
  Function: unsigned long bus_idle_test(void)
  Action:  Test the bus idle timer circuit by generating a data access to an
		   unused memory region. This action causes the bus idle timer to
		   expire and cause an interrupt. The test installs an interrupt
		   handler which incrememnts a memory location each time a bad data
		   access is attempted. The test then checks to see if the value of
		   the memory location matches the number of accesses attempted. The
		   test returns zero if it passed and the number of accesses detected
		   if it does not.

  Passed:  Nothing
  Returns: Zero if passed, non-zero if failed.
--------------------------------------------------------------*/
static unsigned long bus_idle_test(void) {

	volatile unsigned long *pVPtr;

	asm("mov 0, sf1");              /* Mask XINT 1 */
	asm("mov 0, sf2");      /* Clear pending interrupts */

	pVPtr = (volatile unsigned long *)0x10008000;  /* use region 1 */
	*pVPtr = 0;                             /* Access the invalid address */

	if(get_pending() & 1)
		return(0);
	else
		return(1);
}
