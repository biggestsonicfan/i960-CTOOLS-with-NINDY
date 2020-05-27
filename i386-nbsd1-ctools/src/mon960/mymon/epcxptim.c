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
/*********************************************************************
  Module Name: EVCAPTIM.C

  Function:   u_char timer_test(void)
              void d_wait(u_long count)

    This module contains the test for the 82C54 timer.  The 82C54
    contains three timers, T0, T1, and T2.  The output of T0 is jumpered
    to the clock input of T1.  The clock inputs of T0 and T2 are
    driven directly by an oscillator.

    The test first puts T0 in mode 0 and writes an initial count to the
    timer.  OUT0 should be low until the count expires after which it
    will go high and remain high.  Once the initial count is written
    the timer begins to count down.  The CPU first checks to see that
    OUT went low after the initial count was written and then reads
    the timer twice in succession to verify that the count is decrementing.

    T1 is tested similar to T0 except that T0 must first be programmed
    to be in mode 3 and act as the clock input for T1.

    T2 is tested in indentically the same manner as T0.

    The value returned from timer() is an u_char.  The upper three bits
    correspond to each timer, bit 7 for T0, bit 6 for T1, and bit 5 for T2.
    A zero (0) in any of these bit positions indicates that corresponding
    timer failed.

*********************************************************************/

#include "epcx.h"
#include "82c54.h"
#include "epcxptst.h"
extern diag_write_string(), term_bentime();

#define COUNTL 0x00
#define COUNTH 0xA0
#define STATUS_MASK ~(OUTPUT_BIT)

/*--------------------------------------------------------------------
  Function:  d_wait(unsigned long)
  Action:    Create a time delay.
  Passed:    Nothing
  Returns:   nothing
--------------------------------------------------------------------*/

void d_wait(u_long cnt)
{
  volatile u_long dummy;

  for (dummy = cnt; dummy > 0; dummy--)
    ;
}

/*--------------------------------------------------------------------
  Function:  timer_test(void)
  Action:    Test the three timers present on the 82C54.
  Passed:    Nothing
  Returns:   unsigned char

	        7     6     5     4     3     2    1   0
	     +-------------------------------------------+
	     | T0a | T1a | T2a | T0b | T1b | T2b |T1c| X |
	     +-------------------------------------------+
               T0a = Timer 0 pass/fail flag Test 1
               T1a = Timer 1 pass/fail flag Test 1
               T2a = Timer 2 pass/fail flag Test 1
               T0b = Timer 0 pass/fail flag Test 2
               T1b = Timer 1 pass/fail flag Test 2
               T2b = Timer 2 pass/fail flag Test 2
               T1c = Timer 1 pass/fail flag Test 3

                 A one in any bit position indicates failures

--------------------------------------------------------------------*/
u_char timer_test(void)
{
  u_char status = 0, tmp;

  u_short cnt1, cnt2, cnt1a, cnt2a;

  /*
     Begin test for T0.  Configure T0 for mode 0, write the initial
     count and wait for OUT0 to go high.  Note, COUNTER0 and COUNTER_0
     all refer to T0.
  */


  /* T0a Timer Zero Test 1 */

  write_ctrl_reg(CTRL_COUNTER0 | RW_BOTH | MODE0 | BINARY);  /* OUT0 is now low */
  write_timer(COUNTER_0, COUNTL);                          /* countdown begins */
  write_timer(COUNTER_0, COUNTH);

  /* read OUT0 and verify that it is low */
  write_ctrl_reg(READ_BACK | DONT_LATCH_COUNT | RB_COUNTER0);
  tmp = read_timer(COUNTER_0);
  if ( (tmp & STATUS_MASK) != (RW_BOTH | MODE0 | BINARY) ) /* Check programmed*/
    status |= (1<<7);                                      /* values correct. */
  if ( (tmp & OUTPUT_BIT) != 0 )	/* Check output */
    status |= (1<<7);             /* is low. */


  /* T0b Timer Zero Test 2 */

  /* read a count value */
  write_ctrl_reg(READ_BACK | DONT_LATCH_STATUS | RB_COUNTER0);
  cnt1 = read_timer(COUNTER_0);
  cnt1a = read_timer(COUNTER_0);
  cnt1 = cnt1 + (cnt1a << 8);

  /* read another count value */
  write_ctrl_reg(READ_BACK | DONT_LATCH_STATUS | RB_COUNTER0);
  cnt2 = read_timer(COUNTER_0);
  cnt2a = read_timer(COUNTER_0);
  cnt2 = cnt2 + (cnt2a << 8);

  /* if the second count value read is >= the first, the test fails */
  if (cnt2 >= cnt1)
    status |= (1<<4);


  /*
     Begin test for T1.  Configure T0 for mode 3 and use it as the clock
     input to T1.  Configure T1 for mode 0, write the initial count and
     wait for OUT1 to go high.
  */
  /* T1a Timer One Test 1 */

  write_ctrl_reg(CTRL_COUNTER0 | RW_LSB | MODE3 | BINARY);
  write_ctrl_reg(CTRL_COUNTER1 | RW_BOTH | MODE0 | BINARY);  /* OUT1 is now low */
  write_timer(COUNTER_0, 0x02);                            /* T0 now clocking T1 */
  write_timer(COUNTER_1, COUNTL);                        /* countdown begins */
  write_timer(COUNTER_1, COUNTH);

  /* read OUT1 and verify that it is low */
  write_ctrl_reg(READ_BACK | DONT_LATCH_COUNT | RB_COUNTER1);
  tmp = read_timer(COUNTER_1);
  if ( (tmp & STATUS_MASK) != (RW_BOTH | MODE0 | BINARY) ) /* Check programmed*/
	status |= (1<<6);                                    /* values correct. */
  if ( (tmp & OUTPUT_BIT) != 0 )                            /* Check output */
   	status |= (1<<6);                                 /* is low. */



  /* T1b Timer One Test 2 */

  /* read a count value */
  write_ctrl_reg(READ_BACK | DONT_LATCH_STATUS | RB_COUNTER1);
  cnt1 = read_timer(COUNTER_1);
  cnt1a = read_timer(COUNTER_1);
  cnt1 = cnt1 + (cnt1a << 8);

  /* read another count value */
  write_ctrl_reg(READ_BACK | DONT_LATCH_STATUS | RB_COUNTER1);
  cnt2 = read_timer(COUNTER_1);
  cnt2a = read_timer(COUNTER_1);
  cnt2 = cnt2 + (cnt2a << 8);

  /* if the second count value read is >= the first, the test fails */
  if (cnt2 >= cnt1)
    status |= (1<<3);


  /* T1c Timer One Test 3 wait a while, OUT1 should go high  */
  d_wait(500000);


  /* read OUT1 and verify that it is high */
  write_ctrl_reg(READ_BACK | DONT_LATCH_COUNT | RB_COUNTER1);
  tmp = read_timer(COUNTER_1);
  if ( (tmp & (1<<7)) == 0 )                        /* Check output */
   	status |= (1<<1);                                 /* is high. */



  /*
    Begin test for T2.  Configure T2 for mode 0, write the initial count and
    wait for OUT2 to go high.
  */
  /* T2a Timer Two Test 1 */

  write_ctrl_reg(CTRL_COUNTER2 | RW_BOTH | MODE0 | BINARY);  /* OUT2 is now low */
  write_timer(COUNTER_2, COUNTL);                          /* countdown begins */
  write_timer(COUNTER_2, COUNTH);

  /* read OUT2 and verify that it is low */
  write_ctrl_reg(READ_BACK | DONT_LATCH_COUNT | RB_COUNTER2);
  tmp = read_timer(COUNTER_2);
  if ( (tmp & STATUS_MASK) != (RW_BOTH | MODE0 | BINARY) ) /* Check programmed*/
    status |= (1<<5);                                      /* values correct. */
  if ( (tmp & OUTPUT_BIT) != 0 )                            /* Check output */
    status |= (1<<5);                                     /* is low. */


  /* T2b Timer Two Test 2 */

  /* read a count value */
  write_ctrl_reg(READ_BACK | DONT_LATCH_STATUS | RB_COUNTER2);
  cnt1 = read_timer(COUNTER_2);
  cnt1a = read_timer(COUNTER_2);
  cnt1 = cnt1 + (cnt1a << 8);

  /* read another count value */
  write_ctrl_reg(READ_BACK | DONT_LATCH_STATUS | RB_COUNTER2);
  cnt2 = read_timer(COUNTER_2);
  cnt2a = read_timer(COUNTER_2);
  cnt2 = cnt2 + (cnt2a << 8);

  /* if the second count value read is >= the first, the test fails */
  if (cnt2 >= cnt1)
    status |= (1<<2);
  term_bentime();
  return(status);
}


/*--------------------------------------------------------------------------

 Function: diag_timer_error(unsigned char result)


 Action: Displays a diagnostic message based on the result of the timer test.
		 A non-zero result indicates a failure. This function is called from
		diagerr().

 Passed: Result of timer test.

 Returns: Nothing.

---------------------------------------------------------------------------*/
void diag_timer_error(unsigned char result){

	int i;

	for(i=1;i<8;i++){


		switch(result & (1<<i)){
			case 2:
				diag_write_string("Timer 1 output bit failed to go high following terminal count in mode 0\r\n");
			   break;

			case 4:
				diag_write_string("Timer 2 counter failed to decrement.\r\n");
			   break;

			case 8:
				diag_write_string("Timer 1 counter failed to decrement.\r\n");
			   break;

			case 0x10:
				diag_write_string("Timer 0 counter failed to decrement.\r\n");
			   break;

			case 0x20:
				diag_write_string("Timer 2 output bit failed to go low during mode 0 count.\r\n");
			   break;

			case 0x40:
				diag_write_string("Timer 1 output bit failed to go low during mode 0 count.\r\n");
			   break;

			case 0x80:
				diag_write_string("Timer 0 output bit failed to go low during mode 0 count.\r\n");
			   break;
		}
	}

}
