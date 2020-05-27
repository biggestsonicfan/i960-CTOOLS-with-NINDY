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

#include "mon960.h"
#include "this_hw.h"


/*-------------------------------------------------------------
 * Function:    void pause(void)
 *
 * Action:      A software wait loop, used to make the LED blinks
 *              discernable.
 *-------------------------------------------------------------*/
void
pause(void)
{
#ifdef BLINK_PAUSE
    volatile int    ii;

    for (ii = BLINK_PAUSE; ii; --ii)
        if (ii < 0)
            break;
#endif
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
    volatile unsigned char *p = (unsigned char *)SWITCH_ADDR;

    return(*p ^ 0xff);
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
	/* Define the values that will be written by complementing the desired
			display value */

#ifdef LED_8SEG_ADDR
    static const unsigned char display_values[] =
	{ DISPLAY0^0xff, DISPLAY1^0xff, DISPLAY2^0xff, DISPLAY3^0xff,
	  DISPLAY4^0xff, DISPLAY5^0xff, DISPLAY6^0xff, DISPLAY7^0xff,
	  DISPLAY8^0xff, DISPLAY9^0xff, DISPLAYA^0xff, DISPLAYB^0xff,
	  DISPLAYC^0xff, DISPLAYD^0xff, DISPLAYE^0xff, DISPLAYF^0xff };
    volatile unsigned char *pp = (unsigned char *) LED_8SEG_ADDR;

    if (n == -1)                  /* Clear and leave */
        *pp = CLR_DISP^0xff;
    else if (n == 0xdd)
		*pp= DOT^0xff;
    else
		{
        if((n & 0xf0) == 0x10)
		        /* Digit plus the dot */
            *pp = display_values[n & 0xf] & (DOT^0xff);
        else    /* Digit only */
            *pp = display_values[n & 0xf];
        }
#else
#ifdef LED_1_ADDR
    if (n == -1)                  /* Clear and leave */
		leds(0, 0xff);
	else
        leds(n, 0xff);

#else
    /* no leds or 8seg led */
#endif
#endif
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
    led(n);
    pause();
    led(-1);
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
#ifdef LED_1_ADDR
/* keep the accumulated bit settings in current value 0,0xff to reset*/
static unsigned char current_val=0;

	volatile unsigned char	*pp;

	current_val = ((current_val & ~mask) | (val & mask));

#ifdef LEDS_ON_IS_0
    val = current_val;
#else
	val = ~current_val;
#endif

	pp = (unsigned char *)LED_1_ADDR;
	*pp = val & LEDS_MASK;
	val  >>= LEDS_SIZE;
	mask >>= LEDS_SIZE;

#ifdef LED_2_ADDR
	pp = (unsigned char *)LED_2_ADDR;
	*pp = val & LEDS_MASK;
	val  >>= LEDS_SIZE;
	mask >>= LEDS_SIZE;
#endif

#ifdef LED_3_ADDR
	pp = (unsigned char *)LED_3_ADDR;
	*pp = val & LEDS_MASK;
	val  >>= LEDS_SIZE;
	mask >>= LEDS_SIZE;
#endif

#ifdef LED_4_ADDR
	pp = (unsigned char *)LED_4_ADDR;
	*pp = val & LEDS_MASK;
#endif

#else /* LED_1_ADDR */
#ifdef LED_8SEG_ADDR
/* For boards with no leds send out value to 8seg led with a DOT */
if (val == 0)
	led(-1);
else
    led((val&0xf) + 0x10);

#else
    /* no leds or 8seg led */
	return;
#endif
#endif
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
    int i;

    if (size > 8 || size < 1) size = 8;

    led(0xdd); pause(); pause(); pause();
	for (i=size-1; i>=0; i--)
		{
        led((n >> i*4)&0xf); pause(); pause(); pause();
		led(-1); pause();
		}
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
    int i;

    if (size > 100 || size < 1) size = 0;

    led(0xdd); led(0xdd); pause(); pause(); pause();
	for (i=0; i<size; i++)
		{
        led((char_ptr[i] >> 4)&0xf); pause(); pause(); pause();
        led(char_ptr[i]&0xf); pause(); pause(); pause();
		led(-1); pause();
		}
    led(0xdd); led(0xdd); pause(); pause(); pause();
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
	int i;

    for (i=0; i<10; i++) pause();
    blink_hex((int)id,2);
    blink_hex(a,8);
    blink_hex(b,8);
    blink_hex(c,8);
    blink_hex(d,8);
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
#ifdef SWITCH_ADDR
	unsigned char entry_switch = read_switch();

    while(entry_switch == read_switch())
#else
	int i=4;

	while (i-- > 0)
#endif
        led_output(id, a, b, c, d);
}
