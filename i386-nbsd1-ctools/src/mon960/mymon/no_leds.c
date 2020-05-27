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


/*-------------------------------------------------------------
 * Function:    void pause(void)
 *
 * Action:      A software wait loop, used to make the LED blinks
 *              discernable.
 *-------------------------------------------------------------*/
void
pause(void)
{
}


/*-------------------------------------------------------------
 * Function:    unsigned char read_switch(void)
 *
 * Action:      Returns the inverted value read from the DIP switch
 *
 *-------------------------------------------------------------*/
#ifdef SWITCH_ADDR
unsigned char
read_switch(void)
{
}
#endif


/*-------------------------------------------------------------
 * Function:     void led(int n)
 *
 * Passed:       Hex integer to display. 0 - f are display as is. Adding
 *               0x10 adds the DOT. n == (0xdd) displays just the dot,
 *               n == (-1) clears the display.
 *
 * Returns:      void
 *
 * Action:       Displays a number on the LED.
 *
 *
 *-------------------------------------------------------------*/
void
led(int n)
{
}


/*-------------------------------------------------------------
 * Function:     void blink(int)
 *
 * Passed:       Number of blink
 *
 * Returns:      void
 *
 * Action:       Displays a number on the LED for a short time.
 *               Mainly for help in tracing board initialization
 *               while retargeting.  Board initialization normally blinks
 *               1, 2, and 3 during initial boot, and 4 and 5 when db960ca
 *               begins communications.  Blink(0) is called for fatal errors.
 *-------------------------------------------------------------*/
void
blink(int n)
{
}


/*-------------------------------------------------------------
 * Function:    unsigned char leds(int, int)
 *
 * Action:      Sets the led block by the value and mask
 *              current_val is the current state of the leds
 *-------------------------------------------------------------*/
void
leds(int val, int mask)
{
}


/*-------------------------------------------------------------
 * Function:    void blink_hex(int n, int size)
 *
 * Passed:      n and size
 *
 * Returns:     always.
 *
 * Action:      This routine is for debugging help.
 *              Debug values are written to the 8-segment LED.
 *-------------------------------------------------------------*/
void
blink_hex(unsigned int n, int size)
{
}


/*-------------------------------------------------------------
 * Function:    void blink_string(char * char_ptr, int size)
 *
 * Passed:      char_ptr and size
 *
 * Returns:     always.
 *
 * Action:      This routine is for debugging help.
 *              Debug values are written to the 8-segment LED.
 *-------------------------------------------------------------*/
void
blink_string(char * char_ptr, int size)
{
}

/*-------------------------------------------------------------
 * Function:    void led_output(char id, int a,b,c,d)
 *
 * Action:      Outputs values to the 8_seg led or leds block
 *
 *-------------------------------------------------------------*/
void
led_output(char id, int a, int b, int c, int d)
{
}


/*-------------------------------------------------------------
 * Function:     void led_debug(char id, int a,b,c,d)
 *
 * Action:      Repeatedly outputs values to 8seg led or leds block
 *
 *-------------------------------------------------------------*/
void
led_debug(char id, int a, int b, int c, int d)
{
}
