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
/*
 *    serial support for target communications
 *    assumes 8250 serial device
 */
/* $Header: /ffs/p1/dev/src/hdilcomm/common/RCS/dos_io.c,v 1.24 1995/11/05 00:55:29 cmorgan Exp $$Locker:  $ */

#include <dos.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <conio.h>
#include <sys/types.h>
#include <sys/timeb.h>

#include "common.h"
#include "hdil.h"
#include "hdi_errs.h"
#include "8250.h"
#include "com.h"
#include "dev.h"
#include "funcdefs.h"

extern int xltbaud();
extern const unsigned short CRCtab[];

/* 
 * DOS-Extender interrupt handling is messy.  Metaware requires you 
 * to save and restore BOTH the real-mode and protected-mode handlers. 
 */
#ifdef    __HIGHC__
static _real_int_handler_t orig_rm_serial_vec;
static _Far _INTERRPT void (*orig_pm_serial_vec)();
#else
static intr_handler prev_serial_intr;
#endif    /* Metaware */

#ifdef _MSC_VER
static intr_handler prev_criticerr_intr;
#endif

#define RXINT        1
#define TXINT        0

#define IR3         0x0b
#define IR4         0x0c
#define IR5         0x0d
#define IR7         0x0f

#define CRITICERR_IV    0x24    /* DOS' Critical Error interrupt vector */

#define    ISR_8259    0x20        /* int status reg */
#define    IMR_8259    0x21        /* int mask reg */

#define    LOOPERMS    33        /* rough loop count per ms */
#define    BREAKMS        1000        /* break length */

#define DOS_PKT_TIMO 1000
#define DOS_ACK_TIMO  500

/*
 * Map bauds onto counter values
 */
static struct baudmap baudmap[] ={
    {50,    2304},
    {75,    1536},
    {110,    1024},
    {134,    896},
    {150,    0x300},
    {200,    0x240},
    {300,    0x180},
    {600,    0x0C0},
    {1200,    0x060},
    {1800,    0x040},
    {2400,    0x030},
    {4800,    0x018},
    {9600,    0x00C},
    {19200,    6},
    {38400,    3},
    {57600,  2},
    {115200, 1},
    {0, 0}
};

#if RXINT|TXINT
/*
 * Input queue
 * See ISR0.ASM for definition of a corresponding ASM STRUC. (DOS X only).
 */
#define QBUFSZ    (5*1024)    /* queue size */

struct queue {
    int         q_cnt;        /* amount in buffer */
    unsigned    q_ovfl;        /* number of overflows */
    char        *q_in;        /* next free spot */
    char        *q_out;        /* next char out */
    char        q_buf[QBUFSZ];    /* character buffer */
};

struct queue siq;    /* chars from remote port */
#endif

struct mode {
        unsigned long d_baud;
        unsigned long d_freq;
        unsigned char d_txint;
        unsigned char d_rxint;
        unsigned   int_vector;
} dev_mode;

#ifdef _MSC_VER
extern void INTR_HANDLER serial_intr_wrap(void);
#endif
void INTR_HANDLER serial_intr(void);

static int get_iobase(int port), get_irq(int port);
static void init_8259(unsigned int int_value);
static void uinit_8259(unsigned int int_value);
static int set_mode(struct mode *new_mode);
static void sputch(int);
static void sinitq(void);
static int serial_read1(int, char *, int);

#ifdef _MSC_VER
void grab_dos_vectors(void);
static void INTR_HANDLER cleanup_criticerr(void);
#endif

#if RXINT|TXINT
static void initq(struct queue *p);
static void enqueue(int c, struct queue *p);
#endif

int curbase;        /* base address of device registers. used by isr0 */
static int serIV_installed = 0;
static int intr_flag;

/*
 * Open the serial device passed in as a parameter and prepare it 
 * for IO. Global device parameters are set in dev_mode.
 * Returns a serial descriptor to be used in other IO calls for the device
 * or -1 on error (unable to initialize requested device).
 */
int
serial_open()
{
    if (_com_config.dev_type == 0)
        _com_config.dev_type = RS232;

    if (strncmp(_com_config.device, "COM", 3) == 0)
    {
        int port = _com_config.device[3] - '0';

        if (_com_config.iobase == 0)
        _com_config.iobase = get_iobase(port);

        if (_com_config.irq == 0)
        _com_config.irq = get_irq(port);
    }

    if (_com_config.iobase==0 || _com_config.irq==0)
    {
        com_stat = E_BAD_CONFIG;
        return(ERR);
    }

    if (_com_config.host_pkt_timo == 0)
        _com_config.host_pkt_timo = DOS_PKT_TIMO;

    if (_com_config.ack_timo == 0)
        _com_config.ack_timo = DOS_ACK_TIMO;

    curbase = _com_config.iobase;
    dev_mode.d_baud = _com_config.baud;
    dev_mode.d_freq = _com_config.freq;
    dev_mode.d_txint = TXINT;
    dev_mode.d_rxint = RXINT;
    dev_mode.int_vector = _com_config.irq;

#ifdef _MSC_VER
    /*
     * We must be ready to remove our interrupt routines on
     * a forced termination (i.e., Ctrl-Brk or critical error).
     * So we must patch these vectors BEFORE the serial & timer.
     */
    grab_dos_vectors();
#endif

    if (set_mode(&dev_mode) != OK)
        return(ERR);

    if (init_timer() != OK)
        return(ERR);

    return(0);
}

/*
 * Close the serial device, terminating usage in current context.
 * return
 *    0    indicating success
 *    -1    failure
 */
int
serial_close(int port)
{
#if TXINT|RXINT
    OUTP( curbase+IER_REG, 0);         /* disable ints */
    OUTP( curbase+MCR_REG, 0);
    uinit_8259(dev_mode.int_vector);
#endif

    term_timer();

    return(OK);
}

/*
 * Attempt to read N bytes from the serial device, waiting up to
 * timo (time-out) milliseconds for data to become available.
 * return
 *    0-N    Actual number of bytes read, 0 if none.
 *    -1    error condition
 */
int
serial_read(int port, unsigned char *buf, int size, int timo)
{
    int     actual = 0;
    int    timo_flag = 0;

    intr_flag = FALSE;
    switch (timo)
        {
        case COM_POLL:
            actual = serial_read1(port, (char *)buf, size);
            break;

        case COM_WAIT_FOREVER:
            do  {
                actual = serial_read1(port, (char *)buf, size);
                /* need to test dos io to allow CTRL_C handler to work */
                _kbhit();
                } while (actual == 0 && !intr_flag);
            break;

        default:
            settimeout(timo, &timo_flag);
            do  {
                actual = serial_read1(port, (char *)buf, size);
                } while (actual == 0 && !timo_flag && !intr_flag);
            settimeout(0, &timo_flag);
        }

    if (intr_flag)
        com_stat = E_INTR;
    else if (timo_flag)
        com_stat = E_COMM_TIMO;

    return(actual);
}

void
serial_signal(int port)
{
    intr_flag = TRUE;
    com_stat = E_INTR;
}

#if RXINT
static int
serial_read1(int port, char *bp, int want)
{
    int cnt;
    int pif;

    pif = CHANGE_IF( 0 );
    if ((cnt = siq.q_in - siq.q_out) < 0)
        cnt = &siq.q_buf[QBUFSZ] - siq.q_out;    /* don't wrap */
    CHANGE_IF( pif );
    if (cnt == 0) {
        return(0);
    }

    if (want < cnt)
        cnt = want;

    memcpy( bp, siq.q_out, cnt );

    pif = CHANGE_IF( 0 );
    siq.q_cnt -= cnt;
    if ((siq.q_out += cnt) >= &siq.q_buf[QBUFSZ]) {
        siq.q_out = siq.q_buf;
    } else if (siq.q_in == siq.q_out) {
        siq.q_in = siq.q_out = siq.q_buf;    /* reduce wraps */
    }
    CHANGE_IF( pif );

    return(cnt);
}
#endif


/*
 * Attempt to write N bytes from the serial device, stopping as soon 
 * as unable to send more without waiting.
 * return
 *    0-N    Actual number of bytes written, 0 if none.
 *    -1    error condition
 */
int
serial_write(int port, const unsigned char *bp, int want)
{
#if TXINT
#else
    int i;

    for (i = 0; i < want; i++)
        sputch(*bp++);

    return(want);
#endif
}


/*
 * Interrupt the target:
 * Send a break (RS232 low for more than a character period).
 */
int
serial_intr_trgt(int port)
{
    unsigned char    ch;
    unsigned int    cnt;

    ch = INP( curbase+LCR_REG );
    OUTP( curbase+LCR_REG , (ch | SETBREAK) );
    for ( cnt = (unsigned)BREAKMS*LOOPERMS; cnt; --cnt )
        ;
    OUTP( curbase+LCR_REG, ch );
    return(OK);
}


/*
 * Put a serial character, wait if not currently ready to send.
 */
static void
sputch(int ch)
{
    while ( !(INP(curbase+ST_REG) & PUTRDYBIT) )
        ;
    OUTP( curbase, ch );
}



#if RXINT|TXINT
/*
 * initialize the serial queue(S)
 */
static void
sinitq(void)
{
    initq(&siq);
}

/*
 * Initialize queue structure
 */
static void
initq(register struct queue *qp)
{
    qp->q_cnt = 0;
    qp->q_ovfl = 0;
    qp->q_in = qp->q_out = qp->q_buf;
}



#ifdef _MSC_VER
/* Called from interrupt routine -- stack is not where you think,
 * therefore, stack check would be erroneous (and meaningless).
 */
#pragma check_stack(off)
#endif
#ifdef    __HIGHC__
#pragma Off(Check_stack);
#endif
/*
 * enqueue a character into given queue structure
 * NOTE: cannot put debug, called from interrupt
 *
 * NOTE: This functionality is duplicated in the comIsr routine in
 *         ISR0.ASM.  Changes to this code should be reflected in similiar
 *         changes in comIsr.
 */
static void
enqueue(int c, register struct queue *qp)
{
    if (qp->q_cnt >= (QBUFSZ-1)) {
        qp->q_ovfl++;
        return;
    }

    *qp->q_in++ = c;
    qp->q_cnt++;

    if (qp->q_in >= &qp->q_buf[QBUFSZ])
        qp->q_in = qp->q_buf;        /* pre-advance pointer */

    return;
}
#ifdef _MSC_VER
#pragma check_stack()    /* Restore to previous setting */
#endif
#ifdef __HIGHC__
#pragma Pop(Check_stack); /* reinstate original value of Check_stack */
#endif

#endif /*RXINT|TXINT*/



/*
 * Set the serial device to a specified operating mode.
 * return
 *    0    indicating success
 *    -1    failure
 */
static int
set_mode(struct mode *new_mode)
{
    unsigned char    ch;
    int    val;

    /* Validate baud rate, and get divisor */
        if ((val = xltbaud(new_mode->d_baud, baudmap)) <= 0)
        {
        com_stat = E_BAD_CONFIG;
        return(ERR);
        }


    /* If caller specified alternate frequency, calculate divisor. */
    if (new_mode->d_freq != 0)
        val = (int)(new_mode->d_freq / (new_mode->d_baud * 16UL));

    OUTP( curbase+LCR_REG, ENCNTREG );        /* enable count regs */
    OUTP( curbase+LCNT_REG, (unsigned char)val);
    OUTP( curbase+HCNT_REG, (unsigned char)(val >>8) );

    OUTP( curbase+LCR_REG, CHLEN1 | CHLEN0 );    /* n,8,1 */
    
    ch = ((new_mode->d_txint)?ENTXINT:0) | ((new_mode->d_rxint)?ENRXINT:0);
    OUTP( curbase+MCR_REG, (ch ?OUT2:0) | RTS | DTR );

    if (new_mode->d_baud > 9600) {
#define NS_FIFO_CTL_REG 2
#define I_BANK_REG 2
#define I_INTERNAL_MODE_REG 4
        /*
        * Enable receive and transmit FIFOs if UART is a NS16550A.
        * This seems to be innocuous for other UARTS.
        *
        * FCR<7:6>     00      trigger level = 1 byte
        * FCR<5:4>     00      reserved
        * FCR<3>       0       mode 0 - interrupt on data ready
        * FCR<2>       0
        * FCR<1>       0
        * FCR<0>       1       turn on fifo mode
        */
        OUTP(curbase+NS_FIFO_CTL_REG, 0x01);

        /* Check whether it was in fact a NS16550 by checking whether
         * the FIFO enabled bits are set. */
        if ((INP(curbase+ISR_REG) & 0xc0) == 0xc0)
            ;
        else
        {
        /* Check whether it is a I82510, by writing to the bank
         * register and reading it back. */
            OUTP(curbase+I_BANK_REG, 0x40);
        if ((INP(curbase+I_BANK_REG) & 0x60) == 0x40)
        {
            /* It is an 82510; enable the FIFO. */
            OUTP(curbase+I_INTERNAL_MODE_REG,
                 INP(curbase+I_INTERNAL_MODE_REG) & ~0x04);
            /* Change back to bank 0. */
                OUTP(curbase+I_BANK_REG, 0x00);
        }
        }
    }

#if RXINT|TXINT
    if (ch) {
        sinitq();
        init_8259(new_mode->int_vector);
    }

    OUTP( curbase+IER_REG, ch );    /* enable ints */
    OUTP( curbase+IER_REG, ch );    /* must do twice because of chip bug */
#endif

    return(OK);
}



/*
 * C serial interrupt routine
 * Assumes that only Data ready interrupt is enabled.
 * called from asm interrupt (can't use debug).
 * This routine is not used when running as a DOS extender self-hosted
 * application. (See init_8259).
 */

#ifdef _MSC_VER
/* Called from interrupt routine -- stack is not where you think,
 * therefore, stack check would be erroneous (and meaningless).
 */
#pragma check_stack(off)
#endif
#ifdef    __HIGHC__
/* 
 * Work around a Metaware bug. Compiler stack-check code sequence
 * assumes valid DS register.  But we are about to force-feed DS in
 * the interrupt handler, so we turn Check_stack off.
 */
#pragma Off(Check_stack);
#endif

void INTR_HANDLER
serial_intr(void)
{
#if RXINT|TXINT
    int pif;

#ifdef  __HIGHC__
    /* 
     * Workaround for Metaware _INTERRPT code-generation bug.
         * Observation is that DS does not come in with the correct value
     * (for Phar Lap DOS-Extender) when a HARDWARE interrupt occurs.
     * So we force 0x17 into DS -- this is a fixed value for Phar Lap 
     * applications (See 6.4.2 of the Phar Lap Ref Manual). 
     * Note that the compiler saves and restores both EDX and DS,
     * so we can clobber them safely here.
         */
    _inline(0x32,0xf6,0xb2,0x17);          /* mov  DX, 0017h */
    _inline(0x8e, 0xda);                   /* mov  DS, DX  */  
#endif

#if ! defined(__HIGHC__)
    pif = CHANGE_IF(0);
#endif

    while (INP(curbase + ST_REG) & GETRDYBIT)
        enqueue( INP(curbase), &siq);

    OUTP(0x20, 0x20);
#if ! defined(__HIGHC__)
    CHANGE_IF(pif);
#endif
#endif
}

#ifdef _MSC_VER
#pragma check_stack()    /* Restore to previous setting */
#endif
#ifdef __HIGHC__
#pragma Pop(Check_stack); /* reinstate original value of Check_stack */
#endif


#if RXINT|TXINT
/*
 * Initialize the PCDOS 8259 serial vector for interrupt driven usage.
 */
static void
init_8259(unsigned int int_vector)
{
    int    pif;

    if (!serIV_installed) {
#if    defined(__HIGHC__)
    orig_rm_serial_vec = _getrvect(int_vector);
    orig_pm_serial_vec = _getpvect(int_vector);
    _setrpvectp(int_vector, serial_intr);
#else    /* not Metaware */
    prev_serial_intr = GET_IV(int_vector);
    SET_IV(int_vector, serial_intr_wrap);
#endif

        serIV_installed = 1;
    }
    pif = CHANGE_IF( 0 );
    OUTP( IMR_8259, (INP(IMR_8259) & ~(1 << int_vector - 8)) );
    CHANGE_IF( pif );
}


/*
 * Unset the PCDOS 8259 interrupt controller serial interrupt vector 
 * used for interrupt mode.
 *
 * Note that by restoring the vector to it's previous value, we are
 * assuming that no new program has chained into the serial interrupt
 * since we did.
 *
 * This may be called during an interrupt (to clean up before termination),
 * so no DOS calls allowed (e.g., debug printing).
 */
static void
uinit_8259(unsigned int int_vector)
{
    int    pif;

    pif = CHANGE_IF( 0 );
    OUTP( IMR_8259, (INP(IMR_8259) | (1 << int_vector - 8)) );    /* mask */
    CHANGE_IF( pif );

    if (serIV_installed) {
#if    defined(__HIGHC__)
    _setrvect(int_vector, orig_rm_serial_vec);
    _setpvect(int_vector, orig_pm_serial_vec);
#else    /* not Metaware */
    SET_IV(int_vector, prev_serial_intr);
#endif
        serIV_installed = 0;
    }
}
#endif

static int
get_iobase(int port)
{
    switch (port)
    {
    case 1: return 0x3f8;
    case 2: return 0x2f8;
    case 3: return 0x3e8;
    case 4: return 0x2e8;
    }
    return 0;
}

static int
get_irq(int port)
{
    switch (port)
    {
    case 1: return IR4;
    case 2: return IR3;
    case 3: return IR4;
    case 4: return IR3;
    }
    return 0;
}


#ifdef __HIGHC__
#define _FLAG_INTERRUPT 0x0200
#endif

/* 
 * CHANGE_IF: Change the state of the interrupt flag.
 * Return the state of the flag on entry. (1 = interrupts were
 * enabled on entry, 0 = interrupts were disabled on entry.)
 *
 * If not CodeBuilder or Metaware, this is defined in intvects.asm.
 */
#if    defined(__HIGHC__)
int
CHANGE_IF(int f)
{
    /* This method uses cli/sti inline codes, a requirement under
     * DPMI hosts (such as Windows 3.x). See discussions under
     * 6.7 of Phar Lap DOSX Manual.
     *
     * the following inline will retrieve the current
     * setting of the interrupt flag. The setting will be
     * stored in register EAX (EAX=1 if set, else EAX=0), which
     * also happens to be the return value of this function,
     * CHANGE_IF()
     */
    _inline(0x66, 0xb8, 0x34, 0x25) ; /* mov ax, 2534h ; get int flag */
    _inline(0xcd, 0x21);              /* int 21h                      */

    if (f)
        _inline(0xfb) ;           /* sti */
    else
        _inline (0xfa);           /* cli */
/* never add a return stmt here to remove compiler warning */
/* this causes untold problems in high c by returning intrs turned off */
}
#endif     /*  Metaware. */


#ifdef _MSC_VER
void
grab_dos_vectors(void)
{
    /*
     * The only one we have to grab is for the Critical Error handler
     * (e.g., reading A: when no floppy is present).  We don't have
     * to grab the Ctrl-Brk handler because we've done a SIGINT for
     * that (so we're already handling it).
     */

    /*Store current vector and place new one.*/
    prev_criticerr_intr = GET_IV(CRITICERR_IV);
    SET_IV(CRITICERR_IV, cleanup_criticerr);
}


static void INTR_HANDLER
cleanup_criticerr(void)
{
    /*
     * As it turns out, we don't actually have to do anything in this
     * handler.  We just need to prevent the normal Critical Error
     * handler from executing.
     * Reason:  If the normal handler says "Abort, Retry, Ignore" and
     * the user says "Abort", then we get dumped out to DOS immediately,
     * without having a chance to clean up by removing our interrupt
     * handlers (timer and serial interrupts).  This is really bad,
     * because then when one of those interrupts happens, DOS vectors
     * to what USED to be our code for the interrupt handlers and is
     * now free memory.
     * By not doing anything here, we essentially force an "Ignore"
     * response.  That is, whatever operation caused the Critical Error
     * will get back a Failed response.  This will cause our program
     * to terminate in a controlled fashion (assuming the rest of the
     * code is properly written to check return values, etc.).
     *
     * Note that we don't have to restore the Critical Error vector
     * because DOS automatically does that when our program exits.
     */
}
#endif

static const DEV serial_dev =
    { serial_open, serial_close, serial_read, serial_write,
      serial_signal, serial_intr_trgt };


const DEV *
com_serial_dev()
{
    return(&serial_dev);
}

/*
    PARALLEL io routines for dos 
*/

static unsigned int i_pp_data, i_pp_strobe, i_pp_status;

/* 
 * Assign parallel download timeout interval, which is computed as:
 *
 *    max buffer xferred / slowest observed DOS download rate + fudge
 */
#define PAR_TIMEOUT ((unsigned long) ((MAX_DNLOAD_BUF / (20 * 1024)) + 2))

int EXPORT
com_parallel_init(device)
char * device;
{
int FAR *ptr_port;
int device_offset;

    if ((strcmp(device, "lpt1") !=0) && (strcmp(device , "LPT1") != 0) &&
        (strcmp(device, "lpt2") !=0) && (strcmp(device , "LPT2") != 0) &&
        (strcmp(device, "lpt3") !=0) && (strcmp(device , "LPT3") != 0))
        {
        hdi_cmd_stat = E_ARG;
        return(ERR);
        }

    device_offset = device[3] - '0';
/* Expect pp_data at 0x378 for lpt 1 */
/* dos stores lpt1,... addr at 40,8; 40,a; 40,c; ... */
#ifdef __HIGHC__
    _FP_SEG(ptr_port) = 0x34;
    _FP_OFF(ptr_port) = 0x408 + (device_offset - 1) * 2;
#else
    _FP_SEG(ptr_port) = 0x40;
    _FP_OFF(ptr_port) = 0x08 + (device_offset - 1) * 2;
#endif

    i_pp_data = *ptr_port;
    i_pp_status = i_pp_data + 1;
    i_pp_strobe = i_pp_data + 2;

    if (i_pp_data == 0)
        {
        hdi_cmd_stat = E_ARG;
        return(ERR);
        }

    _outp(i_pp_strobe, 0);

    return(OK);
}


int EXPORT
com_parallel_end()
{
    i_pp_data = i_pp_status = i_pp_data = 0;
    return(OK);
}


int EXPORT
com_parallel_put(buf, size, crc)
register unsigned char *buf;
register unsigned long size;
unsigned short *crc;
{
#define ERR_BIT 0x08
#define BUSY_BIT 0x80
#define READY_MASK BUSY_BIT ^ ERR_BIT
register unsigned short sum;
register const unsigned int pp_data = i_pp_data;
register const unsigned int pp_status = i_pp_status;
register const unsigned int pp_strobe = i_pp_strobe;

register unsigned int status=_inp(i_pp_status);
register unsigned long loopcount;
struct _timeb first_tm, second_tm;

    if (crc != NULL)
        sum = *crc;

    _ftime(&first_tm);

    while (size-- > 0)
        {
        loopcount = 0;
        while (!((status=_inp(pp_status)) & BUSY_BIT))
            {
            if (!(status & ERR_BIT))
                {
                hdi_cmd_stat = E_FAST_DNLOAD_ERR;
                return(ERR);
                }

            if (++loopcount > 1000000)
                {
                _ftime(&second_tm);
#ifdef __HIGHC__
                /* MetaWare uses "long" as time_t (groan). */
                if ((unsigned) second_tm.time - (unsigned) first_tm.time 
                                                                > PAR_TIMEOUT)
#else
                if (second_tm.time - first_tm.time > PAR_TIMEOUT)
#endif
                    {
                    hdi_cmd_stat = E_PARA_DNLOAD_TIMO;
                    return(ERR);
                    }
                /* Ck elapsed time more often, but not always */
                loopcount -= 50000;
                }
            }
        _outp(pp_data, *buf);
        _outp(pp_strobe, 1);
        _outp(pp_strobe, 0);
        sum = CRCtab[(sum^(*buf++))&0xff] ^ sum >> 8;
        }
    
    if (crc != NULL)
        *crc = sum;

    return(OK);
}
