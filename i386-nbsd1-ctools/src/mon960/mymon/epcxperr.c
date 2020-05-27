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
  Module Name: EPCXPERR.C

								 Display
  Failing Test          Fault Type      Failing Device(s)
  -----------------     ----------      -----------------
  DRAM Byte Test           1

  DRAM Short Test          2

  DRAM Long Test           3

  DRAM Trip Test           4

  DRAM Quad Test           5

  DRAM Word Test           6

  Timer Test               7

  Bus idle timer Test      8

  Serial Interface Test    9

  Serial Loopback Test     A


**********************************************************************/

#include <stdlib.h>
#include "epcxptst.h"
extern blink(), led(), d_wait(), diag_timer_error(), diag_write_string();

/*--------------------------------------------------------------------
  Function:  dram_error(u_long status)
  Action:    Displays the failing address of the dram test.

  Passed:    Gets the failing address from the DRAM test.
  Returns:   Nothing
--------------------------------------------------------------------*/
static void dram_error(u_long status){
#if 0
    unsigned long local=status;
    prtf("Failing address: ");
    prtf("%x ",local);
#endif
}


/*--------------------------------------------------------------------
  Function:  diagerr(u_long status, u_char test_type)
  Action:    Determines if a failure occured and causes the appropriate
             routines to be called to update the LEDs and print a
             message to the terminal.
  Passed:    Gets the test status and test type.
  Returns:   Nothing
--------------------------------------------------------------------*/
void diagerr(u_long status, u_char test_type)
{
  if (status != 0)    /* check for pass/fail */
  {
    blink(test_type + 0x10);           /* indicate fail code, turn on DOT LED */
    led(test_type + 0x10);           /* indicate fail code, turn on DOT LED */

    /*
       Decode test type, print error-type message on terminal, and
       call appropriate routine to finish interpreting fail information.
    */
    switch (test_type)
    {
      case DRAM_BYTE_TEST:
        diag_write_string("\n\r\n\r***** DRAM BYTE ERROR *****\n\r");
        dram_error(status);
        break;

      case DRAM_SHORT_TEST:
        diag_write_string("\n\r\n\r***** DRAM SHORT ERROR *****\n\r");
        dram_error(status);
        break;

      case DRAM_WORD_TEST:
        diag_write_string("\n\r\n\r***** DRAM WORD ERROR *****\n\r");
        dram_error(status);
        break;

      case DRAM_LONG_TEST:
        diag_write_string("\n\r\n\r***** DRAM LONG ERROR *****\n\r");
        dram_error(status);
        break;

      case DRAM_TRIP_TEST:
        diag_write_string("\n\r\n\r***** DRAM TRIPLE ERROR *****\n\r");
        dram_error(status);
        break;

      case DRAM_QUAD_TEST:
        diag_write_string("\n\r\n\r***** DRAM QUAD ERROR *****\n\r");
        dram_error(status);
        break;

      case TIMER_TEST:
        diag_write_string("\n\r\n\r***** TIMER ERROR *****\n\r");
        diag_timer_error(status);
        break;


      case BUS_IDLE_TEST:
        diag_write_string("\n\r\n\r***** BUS IDLE ERROR *****\n\r");
        diag_write_string("  No XINT0 interrupt posted in iPND register.\r\n");
        break;

      case SERIAL_LOOP_TEST:
        break;

      case SERIAL_INTFC_TEST:
        break;
    }
	d_wait(350000);				/* Pause so that an error code is visible */
  }
}
