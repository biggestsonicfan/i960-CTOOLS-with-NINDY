
/*(c**************************************************************************** *
 * Copyright (c) 1990, 1991, 1992, 1993 Intel Corporation
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
 ***************************************************************************c)*/

/* Serial support for target communications.  Assumes 8250 serial device
 *
 */

#include <dos.h>
#include <stdio.h>
#include <string.h>
#ifdef _INTELC32_  /* Intel CodeBuilder */
#include <i32.h>
#endif
#include <process.h>
#include "ttycntl.h"
#include "gnudos.h"

static char serial_device[20];
static int  serial_baud;
static int  serial_devtype; /* Device identifier. */
static int  serial_iobase;  /* I/O base value. */
static int  serial_ivec;    /* Host interrupt vector. */
static char serial_opened;  /* Flag set if file is opened */
static char serial_setup;   /* Flag set if setup for port has started */
static char serial_updated; /* Flag set if port values were altered */

static struct dupdef {
	int num;
	int max;
	int *fd;
} serial_dup = { 0, 5, NULL };

#ifdef __HIGHC__  /* Metaware C */
#define _outbyte _outb
#define _inbyte  _inb
#endif

/* 
 * DOS-Extender interrupt handling is messy.  Metaware requires you 
 * to save and restore BOTH the real-mode and protected-mode handlers. 
 */
#if defined(DOS) && defined(__HIGHC__)
static _real_int_handler_t 	orig_rm_serial_vec;
static _Far _INTERRPT void	(*orig_pm_serial_vec)();
static _real_int_handler_t 	orig_rm_timer_vec;
static _Far _INTERRPT void 	(*orig_pm_timer_vec)();
void 				lock_pages();
#else
static void 		(*orig_serial_vec)();
static void 		(*orig_timer_vec)();
#endif

#define IR3         	0x0b
#define IR4         	0x0c
#define IR5         	0x0d

#define	IMR_8259	0x21		/* int mask reg */

#define	LOOPERMS	33		/* rough loop count per ms */
#define	BREAKMS		1000		/* break length in ms */

/*
 * Map bauds onto counter values
 */
static struct baudmap {
	unsigned long baud;
	int code;
} baudmap[] ={
	{   1200, 0x060},
	{   2400, 0x030},
	{   4800, 0x018},
	{   9600, 0x00C},
	{  19200, 6},
	{  38400, 3},
	{  57600, 2},
	{ 115200, 1},
	{ 166600, -1},
	{ 250000, -1},
	{ 500000, -1},
	{ 750000, -1},
	{1500000, -1},
	{      0, 0}
};

/*
 * Input queue
 */
#define QBUFSZ	(3*1024)	/* queue size */

static struct queue {
	int 		q_cnt;		/* amount in buffer	*/
	unsigned	q_ovfl;		/* number of overflows	*/
	char *		q_in;		/* next free spot	*/
	char *		q_out;		/* next char out	*/
	char		q_buf[QBUFSZ];	/* character buffer	*/
} siq;		/* chars from remote port */


static int  get_port(int comwhich);
static void init_8259(unsigned int int_value);
static int  is_dupd(int);
static int  is_serial(int);
static void sendbreak(void);
static int  serial_read1(int, char *, int);
static int  set_mode( unsigned long, unsigned);
static void sputch(int);
static void uinit_8259(unsigned int int_value);


/* 8250 register addresses */
#define	LCNT_REG	(serial_iobase + 0)
#define	HCNT_REG	(serial_iobase + 1)
#define	IER_REG		(serial_iobase + 1)
#define	ISR_REG		(serial_iobase + 2)
#define	LCR_REG		(serial_iobase + 3)
#define	MCR_REG		(serial_iobase + 4)
#define ST_REG		(serial_iobase + 5)

/* 8250 status register bits */
#define	GETRDYBIT	0x01
#define	PUTRDYBIT	0x20

/* 8250 interrupt register bits */
#define	ENTXINT 	0x02
#define	ENRXINT		0x01

/* 8250 line control register bits */
#define	ENCNTREG	0x80
#define	SETBREAK	0x40
#define	CHLEN1 		0x02
#define	CHLEN0		0x01

/* 8250 modem control register btis */
#define	OUT2 		0x08
#define	RTS 		0x02		/* inverted */	
#define	DTR		0x01		/* inverted */

/*
 * TIMER ROUTINES
 */
#define	TKPSEC		17
#define	MSECPTK		(1000/TKPSEC)
#define	TMRVEC		0x1c

static int 	timecnt = 0;	/* timeout ticks remaining */
static int 	bad;		/* bad pointer, in case of bad timeout pointer */
static int 	*timeoutp = &bad;		/* timeout flag pointer */
static int 	inited = 0;

/******************************************************************************
 * settimeout()
 *	Set a "background" timer which sets requested flag when alloted 
 *	interval expires.
 *
 *	WARNING: Normal usage would settimeout( > 0 ), process until some event 
 *	occurs OR this flag sets (indicating times up), then settimeout(0)
 *	in order to turn the timer off.
 *	This is VERY important, because if the requester flag is for instance, 
 *	a stack variable, and the caller RETURNs before the timeout; then, when 
 *	the timer does go off, it will set some, now defunct location.
 ******************************************************************************/
static void
settimeout(ms, flagp)
    unsigned ms;
    int *flagp;
{
	int pif;

	if (!inited) {
		init_timer_intr();
	}
	pif = change_if(0);
	if (ms == 0) {
		timeoutp = &bad;
		timecnt = 0;
	} else {
		timeoutp = flagp;
		timecnt = (ms + (MSECPTK - 1)) / MSECPTK + 1;
	}
	change_if(pif);
}

/******************************************************************************
 * TICK()
 *	C interrupt service routine.
 ******************************************************************************/
#ifdef	_INTEL32_
#pragma interrupt(TICK)
static void
#endif

#ifdef	__HIGHC__
/* 
 * Work around a Metaware bug. Compiler stack-check code sequence
 * assumes valid DS register.  But we are about to force-feed DS in
 * the interrupt handler, so we turn Check_stack off.
 */
#pragma Off(Check_stack);

_Far _INTERRPT void
#endif

TICK(void)
{
#ifdef  __HIGHC__
        /*
         * Workaround for Metaware _INTERRPT code-generation bug. Observation
         * is that DS does not come in with the correct value (for Phar Lap
         * DOS-Extender) when a HARDWARE interrupt occurs. So we force 0x17
         * into DS -- this is a fixed value for Phar Lap applications.
         * (See 6.4.2 of Phar Lap Ref Manual for further discussion).
	 * Note that the compiler-generated code saves and restores both EDX 
	 * and DS, so we can clobber them safely here.
         */
        _inline(0xba, 0x17, 0x00, 0x00, 0x00); /* mov  EDX,17h */
	_inline(0x8e, 0xda);                   /* mov  DS, DX  */  
#endif

	if ( timecnt && ! --timecnt )
	{
		*timeoutp = 1;
	}

#ifndef  __HIGHC__
	/*
	 * Metaware does not provide a _chain_intr() function.  We will
	 * somewhat naively ignore this until a workaround is found.
	 */
	_chain_intr(orig_timer_vec);
#endif
}

#ifdef __HIGHC__
#pragma Pop(Check_stack); /* reinstate original value of Check_stack */
#endif

/******************************************************************************
 * init_timer_intr()
 *	Initialize our timer support.  Return 1 on success, 0 on failure.
 ******************************************************************************/
int
init_timer_intr(void)
{
	if (inited)
		return 0;

#ifdef	__HIGHC__
	orig_rm_timer_vec = _getrvect(TMRVEC);
	orig_pm_timer_vec = _getpvect(TMRVEC);
	/* Workaround for Metaware's lack of a chain_intr() function.
	 * We'll install the original real mode handler into an 
	 * unused interrupt number, and then invoke that handler
	 * directly each time the timer interrupt goes off.
	 *
	_setrvect(CHAINVEC, orig_rm_timer_vec);
*/
	/* Phar Lap requires that we lock both code and data pages in memory
	   for hardware interrupt handlers. */
	lock_pages((void *) TICK, 256, 1);
	lock_pages((void *) &timecnt, 16, 0);

	/* Workaround for DPMI behavior for hardware interrupt 0x1c: 
	 * This interrupt is generated in real mode, and we process it
	 * here.  Then Windows echos the interrupt AGAIN in protected
	 * mode and we process it a second time.  We want to handle
	 * the interrupt only once.  
	 *
	 * So find out if a version of Windows >= 3.0 is running.  
	 * If so, install the handler so that it will get control only
	 * if the interrupt occurs in protected mode.  Otherwise, install 
	 * it to take control for both real and protected mode interrupts.
	 */
	if ( windows_check() )
	{
	    if ( _setpvect(TMRVEC, TICK) == -1 )
	    {
		fprintf(stderr, "Internal error: setpvect failed on timer init.\n");
		exitdos(1);
	    }
	}
	else
	{
	    if ( _setrpvectp(TMRVEC, TICK) == -1 )
	    {
		fprintf(stderr, "Internal error: setrpvectp failed on timer init.\n");
		exitdos(1);
	    }
	}
#else
	orig_timer_vec = _dos_getvect(TMRVEC);
	_dos_setvect(TMRVEC, TICK);
#endif
	inited = 1;
	return 1;
}

/******************************************************************************
 * term_timer_intr()
 *	Terminate our timer support, releasing any resources which were used.
 ******************************************************************************/
int
term_timer_intr(void)
{
	if (inited) 
	{
#ifdef	__HIGHC__
		_setrvect(TMRVEC, orig_rm_timer_vec);
		_setpvect(TMRVEC, orig_pm_timer_vec);
#else		
		_dos_setvect(TMRVEC, orig_timer_vec);
#endif
		inited = 0;
	}
	return 0;
}


#ifdef	__HIGHC__

#define WINDOWS_NOT_PRESENT1	0L
#define WINDOWS_NOT_PRESENT2	0x80L
#define WINDOWS_20_PRESENT1	1L
#define WINDOWS_20_PRESENT2	0xffL

/* windows_check()
 * Determine if Windows 3.0 or higher is currently running. 
 * Returns 1 if Windows is running, 0 if not.
 */
static int windows_check()
{
	union _REGS	regs;
	unsigned long	al_long;

	regs.x.ax = 0x1600;
	_int86( 0x2f, &regs, &regs );
	al_long = (unsigned long) regs.l.al & 0x000000ff;
	if (   ( al_long == WINDOWS_NOT_PRESENT1 )
	    || ( al_long == WINDOWS_NOT_PRESENT2 )
	    || ( al_long == WINDOWS_20_PRESENT1 )
	    || ( al_long == WINDOWS_20_PRESENT2 ) )
		return 0;
	return 1;
}


/*****************************************************************************
 * dosport_term
 *	Remove serial and timer interrupt handlers and reinstall the originals. 
 *	Check to make sure the Metaware handlers are in fact installed first.
 *****************************************************************************/
void
dosport_term()
{
	if ( orig_rm_timer_vec )
		_setrvect(TMRVEC, orig_rm_timer_vec);
	if ( orig_pm_timer_vec )
		_setpvect(TMRVEC, orig_pm_timer_vec);

	if ( orig_rm_serial_vec )
		_setrvect(serial_ivec, orig_rm_serial_vec);
	if ( orig_pm_serial_vec )
		_setpvect(serial_ivec, orig_pm_serial_vec);
}
#endif	/* Metaware HIGHC */


static int
xltbaud(baud, baudmap)
    unsigned long baud;
    register struct baudmap *baudmap;
{
        for ( ; baudmap->baud; baudmap++){
                if (baudmap->baud == baud){
                        return baudmap->code;
		}
	}
        return -1;
}

/******************************************************************************
 * sputch()
 *	Put a serial character, wait if not currently ready to send.
 ******************************************************************************/
static void
sputch(ch)
    int	ch;
{

	while ( ! (_inbyte(ST_REG) & PUTRDYBIT) )
		;
	_outbyte( serial_iobase, ch );
}


/******************************************************************************
 * sendbreak()
 *	Send a break "character" (RS232 low for more than a character period).
 ******************************************************************************/
static void
sendbreak()
{
	unsigned char	ch;
	int	cnt;

	ch = _inbyte( LCR_REG );
	_outbyte( LCR_REG, ch|SETBREAK );
	for ( cnt = BREAKMS*LOOPERMS; cnt; --cnt ){
		;
	}
	_outbyte( LCR_REG, ch );
}

/******************************************************************************
 * serial_open()
 *	Open the serial device passed in as a parameter and prepare it for IO.
 *	Return -1 on error (unable to initialize requested device).
 ******************************************************************************/
static int
serial_open()
{
	if (strncmp(serial_device, "com", 3) == 0) {
		int port = serial_device[3] - '0';
		if (serial_iobase == 0){
			serial_iobase = get_port(port);
		}
		if (serial_ivec == 0) {
			switch (port) {
			case 1: serial_ivec = IR4; break; /* COM1 interrupt */
			case 2: serial_ivec = IR3; break; /* COM2 interrupt */
			case 3: serial_ivec = IR5; break; /* COM3 interrupt */
			}
		}
	} else if (strcmp(serial_device, "aux") == 0) {
		if (serial_iobase == 0){
			serial_iobase = 0x3e8;
		}
		if (serial_ivec == 0){
			serial_ivec = IR5;
		}
	}

	if ( (serial_iobase == 0)
	||   (serial_ivec   == 0)
	||   !set_mode(serial_baud,serial_ivec)
	||   !init_timer_intr()) {
		return -1;
	}
	return 0;
}

/******************************************************************************
 * open_com_port()
 *	Open the serial device passed in as a parameter and prepare it for IO.
 *	Return -1 on error (unable to initialize requested device).
 *      This routine was added for convenience to allow others outside
 *      this module to open serial port.
 ******************************************************************************/
int open_com_port()
{
       serial_open();
       return 0;
}

/******************************************************************************
 * serial_close()
 *	Close the serial device, terminating usage in current context.
 ******************************************************************************/
static void
serial_close()
{
	_outbyte( IER_REG, 0);     	/* disable ints */
	_outbyte( MCR_REG, 0);
	uinit_8259(serial_ivec);
	term_timer_intr();
}

/******************************************************************************
 * close_com_port()
 *	Close the serial device, terminating usage in current context.
 *      This routine was added for convenience to allow others outside
 *      of this module to turn-off serial port.
 ******************************************************************************/
void close_com_port()
{
         serial_close();
}

static int
serial_read1(int port, char *bufptr, int want)
{
	int cnt;
	int pif;

	pif = change_if( 0 );
	if ((cnt = siq.q_in - siq.q_out) < 0){
		cnt = &siq.q_buf[QBUFSZ] - siq.q_out;	/* don't wrap */
	}
	change_if( 1 );

	if (cnt == 0) {
		return 0;
	}
	if (want < cnt){
		cnt = want;
	}

	memcpy( bufptr, siq.q_out, cnt );

	pif = change_if( 0 );
	siq.q_cnt -= cnt;
	if ((siq.q_out += cnt) >= &siq.q_buf[QBUFSZ]) {
		siq.q_out = siq.q_buf;
	} else if (siq.q_in == siq.q_out) {
		siq.q_in = siq.q_out = siq.q_buf;	/* reduce wraps */
	}
	change_if( 1 );
	return cnt;
}

/******************************************************************************
 * serial_read()
 *	Attempt to read N bytes from the serial device, waiting up to
 *	timo (time-out) milliseconds for data to become available.  Return:
 *		0-N	Actual number of bytes read, 0 if none.
 *		-1	error condition
 ******************************************************************************/
static int
serial_read(int port, char *buf, int size, unsigned timo)
{
	int actual;
	int flag;

	flag = 0;
	settimeout(timo, &flag);
	do {
		actual = serial_read1(port, (char *)buf, size);
	} while ((actual == 0) && !flag);
	settimeout(0, &flag);
	return actual;
}

/******************************************************************************
 * serial_write()
 *	Attempt to write N bytes from the serial device, stopping as soon 
 *	as unable to send more without waiting.  Return:
 *		0-N	Actual number of bytes written, 0 if none.
 *		-1	error condition
 ******************************************************************************/
static int
serial_write( port, bufptr, want )
    int	  port;
    char *bufptr;
    int	  want;
{
	int i;

	for (i = 0; i < want; i++){
		sputch(*bufptr++);
	}
	return want;
}


/******************************************************************************
 * set_mode()
 *	Set the serial device to a specified operating mode.
 *	Return 0 on success, -1 on failure
 ******************************************************************************/
static int
set_mode(baud,ivec)
    unsigned long baud;	/* Buad rate	*/
    unsigned ivec;	/* Interrupt vector */
{
	int val;

	if ((val = xltbaud(baud, baudmap)) <= 0){
		return 0;
	}
	_outbyte( LCR_REG, ENCNTREG );		/* enable count regs */
	_outbyte( LCNT_REG, (unsigned char)val);
	_outbyte( HCNT_REG, (unsigned char)(val >>8) );
	_outbyte( LCR_REG, CHLEN1 | CHLEN0 );	/* n,8,1 */
	_outbyte( MCR_REG, OUT2 | RTS | DTR );

	if (baud >= 57600) {
		/* Enable receive and transmit FIFOs.
		 *
		 * FCR<7:6>     00      trigger level = 1 byte
		 * FCR<5:4>     00      reserved
		 * FCR<3>       0       mode 0 - interrupt on data ready
		 * FCR<2>       0
		 * FCR<1>       0
		 * FCR<0>       1       turn on fifo mode
		 */
		_outbyte(ISR_REG, 0x01);
	}

	/* initialize serial queue and chip */
	siq.q_cnt = siq.q_ovfl = 0;
	siq.q_in = siq.q_out = siq.q_buf;
	init_8259(ivec);
	_outbyte(IER_REG, ENRXINT); /* enable ints */
	_outbyte(IER_REG, ENRXINT); /* do twice because of chip bug */
	return 1;
}


/******************************************************************************
 * S_INT()
 *	C serial interrupt routine (can't use debug).
 *	Assumes that only Data ready interrupt is enabled.
 ******************************************************************************/
#ifdef	_INTEL32_
#pragma interrupt(S_INT)
void
#endif

#ifdef	__HIGHC__
/* 
 * Work around a Metaware bug. Compiler stack-check code sequence
 * assumes valid DS register.  But we are about to force-feed DS in
 * the interrupt handler, so we turn Check_stack off.
 */
#pragma Off(Check_stack);
 
_Far _INTERRPT void
#endif
S_INT()
{
	int c;
	
#ifdef  __HIGHC__
	struct queue *qp;
        /*
         * Workaround for Metaware _INTERRPT code-generation bug. Observation
         * is that DS does not come in with the correct value (for Phar Lap
         * DOS-Extender) when a HARDWARE interrupt occurs. So we force 0x17
         * into DS -- this is a fixed value for Phar Lap applications.
         * (See 6.4.2 of Phar Lap Ref Manual for further discussion).
	 * Note that the compiler-generated code saves and restores both EDX 
	 * and DS, so we can clobber them safely here.
         */
        _inline(0xba, 0x17, 0x00, 0x00, 0x00); /* mov  EDX,17h */
	_inline(0x8e, 0xda);                   /* mov  DS, DX  */  
	qp = &siq;
#else
	register struct queue *qp = &siq;
#endif

	while ( _inbyte(ST_REG) & GETRDYBIT ) 
	{
		c = _inbyte(serial_iobase);
		if (qp->q_cnt >= (QBUFSZ-1)) 
		{
			qp->q_ovfl++;
			return;
		}
		*qp->q_in++ = c;
		qp->q_cnt++;
		if (qp->q_in >= &qp->q_buf[QBUFSZ])
		{
			qp->q_in = qp->q_buf;	/* pre-advance pointer */
		}
	}
	outp(0x20, 0x20);
}

#ifdef __HIGHC__
#pragma Pop(Check_stack); /* reinstate original value of Check_stack */
#endif

/******************************************************************************
 * init_8259()
 *	Initailize the PCDOS 8259 serial vector for interrupt driven usage.
 *	return Boolean indicating success.
 ******************************************************************************/
static void
init_8259(ivec)
    unsigned ivec;	/* Interrupt vector */
{
#ifdef	__HIGHC__
	if ( ! orig_rm_serial_vec )
	{
		orig_rm_serial_vec = _getrvect(ivec);
		orig_pm_serial_vec = _getpvect(ivec);
	}
	/* Phar Lap requires that we lock both code and data pages in memory
	   for hardware interrupt handlers. */
	lock_pages((void *) S_INT, 1024, 1);
	lock_pages((void *) &siq, 4096, 0);

	if ( _setrpvectp(ivec, S_INT) == -1 )
	{
	    fprintf(stderr, "Internal error: setrpvectp failed on serial init.\n");
	    exitdos(1);
	}

#else
	if ( ! orig_serial_vec )
	{
		orig_serial_vec = _dos_getvect(ivec);
	}
	_dos_setvect(ivec, S_INT);
#endif
	change_if( 0 );
	_outbyte( IMR_8259, (_inbyte(IMR_8259) & ~(1 << ivec - 8)) );
	change_if( 1 );
}

/******************************************************************************
 * uinit_8259()
 *	Unset the PCDOS 8259 interrupt controller serial interrupt vector 
 *	used for interrupt mode.  Return boolean indicating success.
 ******************************************************************************/
static void
uinit_8259(ivec)
    unsigned ivec;	/* Interrupt vector */
{
	change_if( 0 );
	_outbyte( IMR_8259, (_inbyte(IMR_8259) | (1 << ivec - 8)) ); /* mask */
	change_if( 1 );
#ifdef	__HIGHC__
	_setrvect(ivec, orig_rm_serial_vec);
	_setpvect(ivec, orig_pm_serial_vec);
#else		
	_dos_setvect(ivec, orig_serial_vec);
#endif
}


/******************************************************************************
 * get_port()
 *	equipment check bios call (bit #)
 *		15-14	printers
 *		   12	game io
 *		 11-9	com devices
 *		  7-6	disk drives (0=1)
 *		  5-4	init video mode
 *		  3-2	ram size
 *		    0	are any disk drives
 ******************************************************************************/
static int
get_port(comwhich)
    int comwhich;
{
	union REGS regs;
	int combase;

	/* Check the number of serial devices installed */
	int86( 0x11, &regs, &regs );
	if (comwhich > ((regs.x.ax >> 9) & 7))
	{
	    return 0;
	}
/* This method works for CodeBuilder, not for Phar Lap */
#ifdef  _INTEL32_
	combase = ((unsigned short *)0x400)[comwhich-1];
#else
	switch ( comwhich )
	{
	case 1: return 0x3f8;
	case 2: return 0x2f8;
	case 3: return 0x3e8;
	case 4: return 0x2e8;
	default: return 0;
	}
#endif
}

int
change_if(int f)
{

#if defined (_INTELC32_) /* CodeBuilder */
	   volatile unsigned int old_f = _getflags();
	   _setflags(f ? old_f | _FLAG_INTERRUPT : old_f & ~_FLAG_INTERRUPT);
	   return (old_f & _FLAG_INTERRUPT) != 0;
	
#elif defined (__HIGHC__) /* Metaware */

        /* This method uses cli/sti inline codes, a requirement under
         * DPMI hosts (such as Windows 3.x). See discussions under
         * 6.7 of Phar Lap DOSX Manual.
         */


         /* the following inline will retrieve the current
          * setting of the interrupt flag. The setting will be
          * stored in register EAX (EAX=1 if set, else EAX=0), which
          * also happens to be the return value of this function,
          * change_if()
          */

        _inline(0x66, 0xb8, 0x34, 0x25) ; /* mov ax, 2534h ; get int flag */
        _inline(0xcd, 0x21);              /* int 21h                      */

        if (f)

                _inline(0xfb) ;           /* sti */
        else
                _inline (0xfa);           /* cli */
#endif
}

/***************************************************************************
 * ioctl()
 *
 *	This routine is a dummy for the ioctl System V Unix call.
 *	The current implementation calls a serial driver package that
 *	actually talks to the port.
 *
 *	Returns:
 *		0 on success; -1 otherwise
 ***************************************************************************/
int
ioctl(fd, func, tty)
    int fd;		/* file descriptor for I/O		*/
    int func;		/* function to perform on file		*/
    TTY_STRUCT *tty;	/* tty structure to fill with data	*/
{
    if (fd == 0)        /* We do not support any manipulation of stdin on dos. */
	    return -1;

    switch (func) {
 case TCSBRK:	
	if ( is_serial(fd) ){
	    sendbreak();
	}
	break;
 case TIOCGETP:  
	if ( is_serial(fd) ){
	    tty->c_cflag = (serial_baud<<16) | CS8;
	}
	break;
 case TIOCSETP:  
	if ( is_serial(fd) ){
	    serial_updated = 1;
	    serial_baud = tty->c_cflag>>16;
	}
	break;
 case TCSETAW:	
	if ( is_serial(fd) ){
	    close_port(serial_iobase);
	    serial_setup = 1;
	}
	break;
 case TCXONC:	
 case TCFLSH:	
	break;
	default:
	printf("IOCTL Function not recognized: %X\n", func);
	return -1;
	break;
    }
    return 0;
}

/***************************************************************************
 * open_port()
 *	This routine is used to interface from the System V "open" format
 *	to the serial driver for the dos port we have provided.
 *
 *	Beacuse of the interface to the driver, this routine does not
 *	do the final open of the port.  Instead, it sets up a data structure
 *	that is filled with pertinent data.  The data structure is further
 *	filled by subsequent ioctl() calls.  When the program actually
 *	requests a read or write, the the port is implicitly reopened with 
 *	the correct values.
 *
 *	Returns:
 *		I/O register base value; -1 if error
 ***************************************************************************/
int
open_port(port_id, mode, baudrate)
    char *port_id;	/* Name of serial port to open	*/
    int  mode;		/* mode in which to open port */
    int  baudrate;      /* baud rate parameter not used */
{

	if (port_id == NULL){
		return -1;
	}

	if ((strlen(port_id) == 4)
	&& (!strcspn(port_id, "com") || !strcspn(port_id, "COM"))) {
		strcpy(serial_device, port_id);
		serial_iobase = 0;
		serial_ivec = 0;
		serial_opened = 1;
		serial_setup = 1;
		serial_updated = 0 ;
		return serial_open() == -1 ? 0 : (int)serial_iobase;
	}
	return open(port_id, mode);
}

char *
set_dos_baud(baudrate)
char * baudrate;
{
	
	char *q;
	char tmp[80];
	int n;
	int baudSpeed[] = {
		38400,
		19200,
		9600,
		4800,
		2400,
		1200,
		600,
		300,
		110,
		0       };
        
        /* 
         * Create a temporary envrionment variable for spawned tasks that
         * need to know the baud rate of the serial connection.
	 * If "baudrate" parameter is non-null, it means the user specified
	 * a baudrate on the command line, so use that, even if G960BAUD
	 * is set in the user's environment.
         */
	if ( baudrate )
	{
	    serial_baud = atoi(baudrate);
#ifdef DEBUG_HIGHC
	    printf ("Got baud from baudrate variable\n");
#endif
	}
	else if ( q = getenv("G960BAUD") )
	{
	    serial_baud = atoi(q);
#ifdef DEBUG_HIGHC
	    printf ("Got baud from G960BAUD\n");
#endif
	}
	else
	{
	    serial_baud = atoi(GBAUD_DFLT);
#ifdef DEBUG_HIGHC
	    printf ("Got baud from default\n");
#endif
	}

#ifdef DEBUG_HIGHC
	printf ("Baud rate is %d\n", serial_baud);
#endif

	/* 
	 * Check for a valid baud rate.  We don't want to do this in the
	 * spawned downloader (sx).
	 */
	for ( n = 0; baudSpeed[n] != 0; n++ )
		if (baudSpeed[n] == serial_baud)
			break;

	if ( baudSpeed[n] == 0 )
		serial_baud = atoi(GBAUD_DFLT);

	sprintf(tmp, "%s%u", "G960BAUD=",serial_baud);
#ifdef DEBUG_HIGHC
	printf ("Going to putenv: %s\n", tmp);
#endif
	putenv( tmp );

	if ( baudrate == NULL )
	{
		/* Provide caller with a char * as well as a number */
		baudrate = malloc (10);
		itoa(serial_baud, baudrate, 10);
	}

#ifdef DEBUG_HIGHC
	printf ("Char * baud rate is %s\n", baudrate);
#endif
	return baudrate;
}


static int
serial_check()
{
	if (!serial_setup){
		return 0;
	}

	if (serial_updated) {
		close_port(serial_iobase);
		serial_setup = 1;
	}
	if (!serial_opened) {
		serial_opened = 1;
		if (serial_open() == -1){
			return 0;
		}
	}
	return 1;
}

/***************************************************************************
 * read_port()
 *	Read from the serial port of an MS/DOS system, using lower-level
 *	driver that handles all serial I/O.
 *
 *	The routine checks to see if the port has been opened for
 *	use yet.  If not, it opens the device and prepares it for reading.
 *	If so, it passes the information on to the serial driver to actually
 *	get the data.
 *
 *	Returns:
 *		int number of bytes read, or -1 if error
 ***************************************************************************/

#define TIMEOUT 1000

int
read_port(devindex, buffer, n)
    unsigned int devindex;	/* Index into device array for port */
    char *buffer;		/* Buffer for data read from port   */
    int n;			/* Number of bytes to read	    */
{
	if (!buffer)
	{
		return -1;
	}
	
	if (is_serial(devindex)) 
	{
		return serial_check() ? 
			serial_read(serial_iobase, buffer, n, TIMEOUT) : -1;
	} 
	else 
	{
		/* Not the serial device, do normal I/O */
		if (devindex == fileno(stdin)) 
		{
			return kbhit() ? read(devindex, buffer, n) : 0;
		} 
		else 
		{
			return read(devindex, buffer, n);
		}
		
	}
}

/***************************************************************************
 * timed_read_port()
 *	This routine is used to read from the serial port of an MS/DOS
 *	system.  It interfaces to a driver that handles all port communication.
 *
 *	First, the routine checks to see if the port has been opened for
 *	use yet.  If not, it opens the device and prepares it for reading.
 *	If so, it passes the information on to the serial driver to actually
 *	get the data.
 *
 *	Returns:
 *		int number of bytes read, or -1 if error
 ***************************************************************************/
int
timed_read_port(devindex, buffer, n, timeout)
    unsigned int devindex;	/* Index into device array for port */
    char *buffer;		/* Buffer for data read from port   */
    int n;			/* Number of bytes to read	    */
    int timeout;		/* Timeout value in seconds         */
                                /* NOTE: the low lever driver is    */
                                /* expecting milliseconds.          */
{
	int	dummy;  /* for debugging the rtn val of serial_read */

	if (!buffer)
		return -1;
	
	if (is_serial(devindex)) 
	{
		if ( serial_check() )
		{
			dummy = serial_read(serial_iobase, buffer, n, 1000*timeout);
			return  dummy;
		}
		else 
			return -1;
	}
		
	else 
	{
		/* Not the serial device, do normal I/O */
		if ( devindex == fileno(stdin) )
			return -2;
		else 
			return read(devindex, buffer, n);
	}
}


/***************************************************************************
 * write_port()
 *	This routine is used to write to the serial port of an MS/DOS
 *	system.  It interfaces to a driver that handles all port communication.
 *
 *	First, the routine checks to see if the port has been opened for
 *	use yet.  If not, it opens the device and prepares it for writing.
 *	If so, it passes the information on to the serial driver to actually
 *	send the data.
 *
 *	Returns:
 *		int  number of bytes actually written; -1 if error.
 ***************************************************************************/
int
write_port(devindex, buffer, n)
    unsigned int devindex;	/* Index into device array for port */
    char *buffer;		/* Buffer of data to write to port  */
    int n;			/* Number of bytes to write	    */
{
	int	dummy; /* for debugging, for the rtn val of serial_write */
	if (!buffer)
		return -1;
	
	if ( is_serial(devindex) ) 
	{
		return serial_check() ? serial_write(serial_iobase,buffer,n) : -1;
	}
	else 
	{
		/* Not the serial device */
		return write(devindex, buffer, n);
	}
}

/***************************************************************************
 * close_port()
 *	This routine is used to interface from the System V "close" format
 *	to the serial driver for the dos port we have provided.
 ***************************************************************************/
close_port(devindex)
    unsigned int devindex;	/* serial port to close	*/
{
	if (is_serial(devindex)) {
		serial_close();
		serial_opened = 0;
		serial_setup = 0;
		serial_updated = 0;
	} else {
#ifdef	__HIGHC__
/* FIXME !! This is a workaround for a Metaware bug; if close()
 * is called on file handle 0, then the next open() fails.
 */
		if (devindex)
#endif
			close(devindex);
	}
}

/***************************************************************************
 * dup2_port()
 *	This routine is emulates the System V "dup2" function for our
 *	DOS serial driver.  It causes all reads or writes to fd2 to be
 *	issued as if they were thru fd1.
 ***************************************************************************/
dup2_port(fd1, fd2)
    unsigned int fd1;	/* file descriptor to be duplicated */
    unsigned int fd2;	/* file descriptor to be replaced */
{
	if (fd1 != serial_iobase) {
		/*
		 * fd1 is not assigned to the serial port:
		 * we can ship this off to the real dup2() routine.
		 */
		dup2(fd1, fd2);
	} else {
		/*
		 * Keep track of the number of dup'ed fds seen so far
		 */
		if (serial_dup.fd == NULL) {
			serial_dup.fd = (int*)
					malloc(serial_dup.max * sizeof(int*));
		}
		if (serial_dup.num >= serial_dup.max) {
			serial_dup.max *= 2;
			serial_dup.fd = (int*) realloc(serial_dup.fd,
						serial_dup.max * sizeof(int*));
		}
		serial_dup.fd[serial_dup.num++] = fd2;
	}
}


/**********************************************************************
 * is_dupd();
 *	Return true (1) iff fd is a dup2'd file descriptor.
 **********************************************************************/
static int
is_dupd(fd)
    int fd;
{
	int i;

	for (i = 0; i < serial_dup.num; i++) {
		if (serial_dup.fd[i] == fd){
			return 1;
		}
	}
	return 0;
}


/**********************************************************************
 * is_serial();
 *	Return true (1) iff fd is a descriptor associated with the
 *	serial port.
 **********************************************************************/
static int
is_serial(fd)
    int fd;
{
	return (serial_iobase? (fd == serial_iobase) : 0 ) || is_dupd(fd);
}

/**********************************************************************
 * putchar_port()
 *	This routine is used to interface from the System V "putchar" format
 *	to the serial driver for the dos port we have provided.
 *
 *	It makes all writes thru stdout to be issued as if they
 *	were thru the serial port.
 *
 *	Returns:
 *		the value of the character written, or EOF if error.
 ***************************************************************************/
int
putchar_port(ch)
    char ch;		/* character to be printed	*/
{
	int numchar;

	if (is_dupd(fileno(stdout)) ){
		/* stdout is the same as the serial port */
		numchar = serial_write(serial_iobase, &ch, 1);
		return numchar ? ch : EOF;
	} else {
		return putchar(ch);
	}
}


/**********************************************************************
 * printf_port()
 *	This routine is used to interface from the System V "printf" format
 *	to the serial driver for the dos port we have provided.
 *
 *	This routine is necessary because the download function (sz)
 *	sets stdout to the serial port, then does some printf's to set
 *	up some data transfer information.  This is a simplistic
 *	implementation, as it only supports one or two parameters.
 ***************************************************************************/
printf_port(string, parm)
    char *string;	/* string to be printed	*/
    char *parm;		/* parameter to be inserted in string*/
{
	int numchar;
	char *buffer;

	if (is_dupd(fileno(stdout))){
		/* stdout is the same as the serial port */
		buffer = (char *)malloc( 2*strlen(string) );
		sprintf(buffer, string, parm);
		numchar = serial_write(serial_iobase, buffer, strlen(buffer));
		free(buffer);
	} else {
		return printf(string, parm);
	}
}

/*
 * Must close the serial port on DOS to restore the original serial
 * interrupt vectors.
 */
exitdos(n)
{
	close_port(serial_iobase);
	exit(n);
}


/***************************************************************************
 * exec_sx()
 *	Special handling for spawning of for sx download subprocess.
 *	Needed because sx needs extra parameters passed to it when running
 *	from DOS, since the information must get thru to the child process,
 *	which must take charge of our serial driver.
 ***************************************************************************/
int
exec_sx(exe, p0, p1, p2, p3)
    char *exe;
    char *p0;
    char *p1;
    char *p2;
    char *p3;
{
	int st;
	char dashB[40];
	char dashP[80];
	char dashD[80];
	int i;
	char *comma;

	sprintf(dashB, "-B%d", serial_baud);

	sprintf(dashP, "-P%s", serial_device);

	if (serial_dup.num > 0) {
		sprintf(dashD, "-D%d:", serial_iobase);
		comma = "";
		for ( i=0; i < serial_dup.num; i++ ) {
			sprintf( &dashD[strlen(dashD)], "%s%d",
						comma, serial_dup.fd[i]);
			comma = ",";
		}
	}
	else 
		dashD[0] = 0;

#ifdef  __HIGHC__
	/* Must do this prior to the spawn() to de-install interrupt handlers */
	close_port(serial_iobase);
#endif

	st = spawnlp(P_WAIT, exe, p0, dashB, dashP, dashD, p1, p2, p3, NULL);

	open_port(serial_device,0,B38400);

	switch ( st )
	{
	case -1:
		/* The spawn itself failed */
		return -1;
	case 0:
		/* sx ran OK */
		break;
	default:
		/* sx returned an error status
		 * FIXME: Caller doesn't interpret any rtn values other than -1
		 */
		break;
	}

	if (serial_dup.fd != NULL) {
		/* Reset counters, free up dynamically allocated space */
		free(serial_dup.fd);
		serial_dup.num = 0;
		serial_dup.fd = NULL;
	}
	return 1;
}

/******************************************************************************
 * read_stdin_noecho()
 *	Reads up to the number of specified characters from stdin (or until
 *	no more characters are pending) without echoing them to stdout.
 *	Returns number of characters read.
 ******************************************************************************/
int
read_stdin_noecho(bufp,bufsz)
    char *bufp;     /* Pointer to buffer in which to save chars */
    int   bufsz;    /* Number of bytes available in buffer      */
{
	int i;

	for ( i=0; kbhit() && i<bufsz; i++ ){
		*bufp++ = getch();
	}
	return i;
}

#if defined(DOS) && defined(__HIGHC__)
/*****************************************************************************
 * lock_pages()
 *	For building with Phar Lap DOS-Extender.  Hardware interrupt handlers
 * 	apparently have to lock their code and data into memory or risk a 
 *	fatal error if a page fault occurs while the handler has control.
 *	Register voodoo is specified in Phar Lap "Libraries and System Call
 *	Reference"
 ******************************************************************************/
void
lock_pages(addr, size, code_seg)
    void *addr;		/* NEAR pointer to start of region */
    int size;		/* size of region to lock */
    int code_seg;	/* nonzero means use CS segment, else use DS segment */
{
    union REGS		regs;
    struct SREGS	sregs;

    /* Set up registers for int 21h call */
    regs.x.ax = 0x252b;	/* Phar Lap magic number */
    regs.h.bh = 5;	/* Phar Lap magic sub-number */
    regs.l.bl = 1;	/* means address is in ES:ECX */

    /* Parameters for this call */
    regs.w.edx = size;				/* size of region */
    regs.w.ecx = (unsigned int) addr;		/* offset within segment*/
    segread(&sregs);
    sregs.es = code_seg ? sregs.cs : sregs.ds; 	/* segment */

    /* Do it */
    intdosx(&regs, &regs, &sregs);

    if ( regs.w.cflag )
    {
	/* error from int 21h */
	switch ( regs.w.eax )
	{
	case 8:
	    fprintf (stderr, "Internal error: lock_pages: memory error\n");
	    break;
	case 9:
	    fprintf (stderr, "Internal error: lock_pages: invalid memory region\n");
	    break;
	default:
	    break;
	}
    }
}

/* Install a Phar Lap protected-mode interrupt handler for CTRL-BREAK key;
   will always take effect in protected mode.  This works around severe
   problems with trying to trap SIGINT for CTRL-BREAK key with signal().
   Words of advice:  The handler should do as little as possible (avoid
   DOS calls).  For example, just set a flag in the handler and test the
   flag often in the application code. 

   Note on locking pages:  the address of the handler itself will be 
   locked into conventional memory.  This call also provides a data segment
   address for you to lock some key data used by the handler.  See discussion
   of lock_pages above.  */

static _real_int_handler_t orig_rm_ctrlbrk_handler;
static _Far _INTERRPT void (*orig_pm_ctrlbrk_handler)();
static int armed;

void
install_ctrlbrk_handler(handler, handler_size, key_data, key_data_size)
_Far _INTERRPT void (*handler)();
void *key_data;		/* Optional address in data segment to lock */
int handler_size;	/* Number of code bytes to lock (required) */
int key_data_size;	/* Number of data bytes to lock (optional) */
{
    if ( armed )
	/* Protect against accidentally calling more than once */
	return;
    orig_rm_ctrlbrk_handler = _getrvect(0x1b);
    orig_pm_ctrlbrk_handler = _getpvect(0x1b);
    lock_pages((void *) handler, handler_size, 1);
    if ( key_data != NULL && key_data_size > 0 )
	lock_pages(key_data, key_data_size, 0);
    if ( _setrpvectp(0x1b, handler) == -1 )
	fprintf(stderr, "Internal error: setrpvectp failed on ctrl-break handler.\n");
    else
	armed = 1;
}

void
de_install_ctrlbrk_handler()
{
    if ( ! armed )
	return;
    if ( orig_rm_ctrlbrk_handler )
	_setrvect(0x1b, orig_rm_ctrlbrk_handler);
    if ( orig_pm_ctrlbrk_handler != NULL )
	_setpvect(0x1b, orig_pm_ctrlbrk_handler);
    armed = 0;
}

#endif
