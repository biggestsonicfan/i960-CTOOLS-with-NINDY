/******************************************************************/
/* 		Copyright (c) 1991, Intel Corporation

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

#include "cyt_intr.h"
#include "c145.h"
#include "16552.h"

/*
 * Internal routines
 */
static int	bus_test(void);
static void	init_uart(void);
static void	uart_isr(void);

/* Routines in 16552.c */
extern int inreg(int reg);
extern void outreg(int reg, unsigned char val);
extern int serial_getc(void);
extern int serial_putc(int c);
extern void serial_init(void);
extern void serial_set(int speed);

extern void prtf();
extern long hexIn();

/*
 * Flag to indicate a uart interrupt
 */
static volatile int uart_int;


/************************************************/
/* BUS_TEST					*/
/* This routine performs a walking ones test	*/
/* on the given uart chip to test it's bus	*/
/* interface.  It writes to the scratchpad reg.	*/
/* then reads it back.  During	         	*/
/* this test all 8 data lines from the chip	*/
/* get written with both 1 and 0.		*/
/************************************************/
static
int
bus_test ()
{
    unsigned char	out, in;
    int			bitpos;
    volatile int 	junk;

    junk = (int) &junk;	/* Don't let compiler optimize or "registerize" */

    outreg(SCR,0);	/* Clear scratchpad register */

    for (bitpos = 0; bitpos < 8; bitpos++)
    {
	out = 1 << bitpos;

	outreg(SCR,out);		/* Write data to scratchpad reg. */

	junk = ~0;			/* Force data lines high */

	in = inreg(SCR);		/* Read data */

	prtf ("%B ", in);

	/* make sure it's what we wrote */
	if (in != out)
	    return (0);
    }
    outreg(SCR,0);	/* Clear scratchpad register */
    prtf ("\n");
    return (1);
}

/************************************************/
/* DISABLE_UART_INTS				*/
/* This routine disables uart interrupts		*/
/************************************************/
static
void
disable_uart_ints ()
{
    disable_intr (IRQ_UART);
    outreg(IER,0);		/* Make the uart shut up */
}

/************************************************/
/* UART_ISR					*/
/* This routine responds to uart interrupts	*/
/************************************************/
static
void
uart_isr ()
{
    disable_uart_ints ();
    uart_int = 1;
    return;
}

/************************************************/
/* INIT_UART					*/
/* This routine initializes the 16550 interrupt */
/* and uart registers and initializes the uart  */
/* count.					*/
/************************************************/
static
void
init_uart ()
{
    disable_uart_ints ();

/* assume already initialized */
#ifdef FOO
    serial_init();		/* initialize the device */
    serial_set(9600);		/* set baud rate */
#endif

    /*
     * Initialize interrupts
     */
    enable_intr (IRQ_UART);
    outreg(IER,0x02);		/* Enable Tx Empty interrupt -
				   should generate an interrupt since Tx is
				   empty to begin with */
}

/****************************************/
/* UART DIAGNOSTIC TEST			*/
/****************************************/
void
uart_test ()
{
    volatile int	loop;
    int			looplim;


    looplim = 400000;

    disable_intr (IRQ_UART);
    install_local_handler (IRQ_UART, uart_isr);
    
    if (!bus_test ())
	prtf ("\nERROR:  bus_test for UART failed\n");
    else
    {
	prtf ("\nbus_test for UART passed\n");

	uart_int = 0;
	init_uart ();
	
	loop = 0;
	while (!uart_int && (loop < looplim))
	    loop++;
	if (!uart_int)
	    prtf ("UART INTERRUPT test failed\n") ;
	else
	    prtf ("UART INTERRUPT test passed\n");
	serial_putc(' ');
	serial_init();		/* flush any junk... */
    }

    prtf ("UART tests done.\n");

    prtf ("Press return to continue.\n");
    (void) hexIn();
}
