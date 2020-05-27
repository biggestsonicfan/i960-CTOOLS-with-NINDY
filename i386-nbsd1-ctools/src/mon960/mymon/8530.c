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
/******************************************************************************
 *
 *    Serial I/O for monitor running on a Heurikon HK80/V960E.
 *        (Z85C30 Serial ports A-D)
 *
 * Note:  the HK80/V960E hardware assures us that we cannot re-access the
 *    UART too quickly -- no need to worry about coding delays between
 *    accesses.
 *
 ******************************************************************************/

/* 8530 Device Driver */

#include "common.h"
#include "i960.h"
#include "retarget.h"
#include "8530.h"
#include "this_hw.h"

extern suspend_dcache(), restore_dcache(), init_sio_buf();
extern leds();


/*
 * forward references
 */

        void    SCCinit(volatile unsigned char *port, unsigned long baud);
        void    SCCput(volatile unsigned char *port, char c);

static  int     SCCoutrdy(volatile unsigned char *port);
static  void    SCCbaud(volatile unsigned char *port, unsigned long baud);
static    void    SCCreset(volatile unsigned char *port);

static volatile unsigned char *port = SCC_PORTA;
/*static volatile unsigned char *reset_AB = SCC_PORTB;
static volatile unsigned char *reset_CD = SCC_PORTD; */
static unsigned long global_baud;    /* save baud setting for reset */


/* Read a received character if one is available.  Return -1 otherwise. */
int
serial_getc()
{
    int c = -1;
    int i;

    /* see if character available */
    suspend_dcache();
    *port = 0;
    i = (*port);

    if (i & 0x01) {
      c = *(port + SCC_DATA_OFFSET);
    }
    restore_dcache();
    return c;
}

/* Transmit a character. */
void
serial_putc(int c)
{
    int i;

    suspend_dcache();
    *port = 0;
    while ( !(i = (*port) & 0x04)) {
           ;       /* Wait for transmit buffer to empty */
    }
    *(port + SCC_DATA_OFFSET) = (char) c;
    restore_dcache();
}

/*
 * Initialize the device.
 */
void
serial_init(void)
{
    SCCinit(port, (unsigned long) 9600);

    /* (re)initialize the character buffer pointer */
    init_sio_buf();     
}

/*
 * Re-Initialize the device.
 * Separate function so that we re-establish the correct Baud.
 */
void
serial_reinit(void)
{
    SCCinit(port, global_baud);

    /* (re)initialize the character buffer pointer */
    init_sio_buf();
}



/*
 * Set the baud rate of the port.
 */
void
serial_set(unsigned long baud)
{
    SCCbaud(port, baud);
}

/*
 * Serial_intr is called by clear_break_condition (in *_hw.c) when
 * the monitor is entered because of an interrupt from the serial port.
 * It checks whether the interrupt was caused by a BREAK condition; if
 * so, it clears the BREAK condition in the UART.
 */
int
serial_intr(void)
{
    int flag = FALSE;
    char rr0, trash;    /* UART Read Register 0 */

    suspend_dcache();

    *( port) = 0;    /* Select Read Reg 0 */
    rr0 = *( port);    /* Read it */

    if (rr0 & 0x80)    /* if bit set, then Break Interrupt */
    {
            /* Acknowledge the interrupt */
            *( port) = 0x00;    /* Select Write Reg 0 */
            *( port) = 0x10;    /* Reset External/Status Interrupts */

            /* If we have received a break, wait until the end of break */
        while(rr0 & 0x80) {
        /* loop while Break Interrupt set */
            *( port) = 0;    /* Select Read Reg 0 */
            rr0 = *( port);    /* Read it */
        }
    
            /* Acknowledge the interrupt */
            *( port) = 0x00;    /* Select Write Reg 0 */
            *( port) = 0x10;    /* Reset External/Status Interrupts */

        /* clear the garbage character */
        trash = *(port + SCC_DATA_OFFSET);

        flag = TRUE;
    }

    restore_dcache();
    return(flag);
}


/*
 * This routine is used by calc_looperms to put the UART in loopback mode.
 * If your UART doesn't have a loopback mode you will need to change
 * calc_looperms in the file serial.c.
 */

void
serial_loopback(int flag)
{
    suspend_dcache();
    if (flag)
    {
    /* enable loop back mode */
    *port = 0x0e;    /* Select WR14 */

    /* Local Loopback + Baud Rate Source + Baud Rate Enable */
    *port = 0x13;  
    }
    else
    {
    /* enable loop back mode */
    *port = 0x0e;    /* Select WR14 */

    /* Baud Rate Source + Baud Rate Enable */
    *port = 0x03;  
    }
    restore_dcache();
}


/****************************************/
/* init_console:            */
/*                    */
/* this routine initializes the console    */
/*port for operation in the polled    */
/* mode, interrupts disabled, except Break.*/
/****************************************/
void
init_console(baud)
    int baud;
{
    SCCinit(CONSOLE,(unsigned long) baud);

    /* (re)initialize the character buffer pointer */
    init_sio_buf();
}

/******************************************************************************
 * 
 * The following local functions will operate on any of the serial ports;
 * MON960 normally only accesses the CONSOLE port.
 * 
 ******************************************************************************/


#ifdef _NOT_USED
/****************************************/
/* SCCget:                */
/*                    */
/* Get a character from port.        */
/****************************************/
static
unsigned char
SCCget(volatile unsigned char *port)
{
    unsigned char c;

    suspend_dcache();
    while ( !SCCinrdy(port) ){
        ;    /* Wait for a character to be present */
    }
    c = *(port + SCC_DATA_OFFSET);
    restore_dcache();
    return c;
}

/****************************************/
/* SCCinrdy:                */
/*                    */
/* Return nonzero if there is a        */
/* character in the input register.    */
/****************************************/
static
int
SCCinrdy(volatile unsigned char *port)
{
    int i;

    *port = 0;
    suspend_dcache();
    i = (*port) & 0x01;
    restore_dcache();
    return i;
}


#endif

/****************************************/
/* SCCput:                */
/*                    */
/* Send a character to port.        */
/****************************************/
void
SCCput(volatile unsigned char *port, char c)
{
    suspend_dcache();
    while ( !SCCoutrdy(port) ){
        ;    /* Wait for transmit buffer to empty */
    }
    *(port + SCC_DATA_OFFSET) = c;
    restore_dcache();
}

/****************************************/
/* SCCoutrdy:                */
/*                    */
/* Return nonzero if transmit buffer of    */
/*port is clear.            */
/****************************************/
static
int
SCCoutrdy(volatile unsigned char *port)
{
    int i;

    *port = 0;
    suspend_dcache();
    i = (*port) & 0x04;
    restore_dcache();
    return i;
}
/****************************************/
/* SCCreset:                */
/*                    */
/* Reset a serial port.            */
/* NOTE: Reset clears the Baud Rate bits*/
/*       So, don't forget to re-enable  */
/****************************************/
void
SCCreset(volatile unsigned char *port)
{
    int dummy;

    *port = 0x00;    /* select and read RR00*/
    dummy = *port; 
    *port = 0x01;    /* select and read RR01*/
    dummy = *port; 

    *port = 0x09;
    *port = 0xc0;            /* reset both*/
    *port = 0x09;            /* delay for reset */
    *port = 0x00;            /* delay for reset */
}

/****************************************/
/* SCCbaud:                */
/*                    */
/* Change the baud rate for serial port.*/
/****************************************/
static
void
SCCbaud(volatile unsigned char *port, unsigned long baud)
{
    int tc, rem; 

    global_baud = baud;
    /* baud rate calculations from Heurikon manual */
    tc = ((unsigned long)500000 / baud) - 2;
    rem = (unsigned long)500000 % baud;
    if(rem >= (baud/2))
      tc++;

    suspend_dcache();
    *( port) = 0x0c;    /* Select WR12 */
    *( port) = tc;        /* Set lower time constant */
    *( port) = 0x0d;    /* Select WR13 */
    *( port) = tc >> 8;    /* Set upper time constant */
    restore_dcache();
}

/****************************************/
/* SCCinit:                */
/*                    */
/*  Initializes port to known state.    */
/****************************************/
void
SCCinit(volatile unsigned char *port, unsigned long baud)
{

/*
 * The initialization commands were changed to support BREAK interrupts.
 */

    static unsigned char cmds[] = {
        /*    wreg#    value                        */
        /*    -----    -----                        */
                /*** modes ***/
        0x04,    0x44,    /* 16x clock, 1 stop bits        */
        0x01,    0x00,    /* No interrupts            */
        0x02,    0x00,    /* No interrupt vector            */
        0x03,    0xC0,    /* Read 8 bit data            */
        0x05,    0xE2,    /* Write 8 bit data, DTR, RTS        */
        0x06,    0x00,    /* No Sync character            */
        0x07,    0x00,    /* No Sync character            */
        0x09,    0x26,    /* No ACK, no chain, no vector        */
        0x0A,    0x00,    /* NRZ                    */
        0x0B,    0x55,    /* TxClk = RxClk = Baud Rate Generator    */
        0x0C,    0x19,    /* Default Time constant (lo)         */
        0x0D,    0x00,    /* Default Time constant (hi)         */
        0x0E,    0x02,    /* Baud Rate Generator Source        */

                /*** enables ***/
        0x0E,    0x03,    /* Start Baud Rate Generator        */
        0x03,    0xC1,    /* Enable Receiver            */
        0x05,    0xEA,    /* Enable Transmitter, DTR, RTS        */

                /*** interrupts ***/
        0x0F,    0x80,    /* Enable Break interrupt        */
        0x00,    0x10,    /* Reset External/Status interrupts    */
        0x00,    0x10,    /* (twice)                */
        0x01,    0x01,    /* Enable External/Status interrupts    */
        0x09,    0x2E,    /* Master interrupt enable        */
    };
    int i;

    SCCreset(port);
    for ( i = 0; i < sizeof(cmds); i++ ){
        *port = cmds[i];
    }
    SCCbaud(port, baud);
}
