/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994, 1995 Intel Corporation
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
 * $Header: /ffs/p1/dev/src/hdilcomm/common/RCS/unix_io.c,v 1.20 1995/08/31 20:48:00 cmorgan Exp $
 */

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>  /* Needed by file.h on SYSV, drags in sys/select.h
                         * on Posix systems.  */
#include "serial.h"     /* Pulls in appropriate terminal-related .h files
                         * for POSIX, USG, or BSD.  */
#include "hdil.h"
#include "com.h"
#include "dev.h"

extern int xltbaud();
extern const short CRCtab[];

extern int _hdi_save_errno;
extern struct baudmap;

/* FIXME -- these decls are superfluous */
extern char *hdi_err_param1, *hdi_err_param2, *hdi_err_param3;

static char errparam[320], para_device[256];

/*
 * The implementation of serial_write() must deal with that fact that the
 * target's serial port is opened O_NDELAY.  For HP-UX, this is a nop and
 * serial_write() can simply be implemented as a call to write().
 * Other hosts, however, need a dedicated serial_write() routine.
 */
#ifdef HP700
#   ifdef __STDC__
#       define serial_write ((int (*)())write)
#   else
#       define serial_write write
#   endif
#else
static int serial_write   HDI_PARAMS((int fd, const unsigned char *, int));
#endif

#ifdef __STDC__

extern int open(const char *, int, ...);
extern int read(int, char *, int);
extern int write(int, char *, int);
extern int alarm(int);
extern int fcntl(int, int, ...);

static int serial_open();
static int serial_close(int);
static int serial_read(int fd, unsigned char *buf, int size, int timo);
static void serial_signal(int fd);
static int serial_intr_trgt(int fd);
#else /* __STDC__ */
#if !defined(RS6000)
extern int open();
extern int fcntl();
#endif /* RS6000 */
extern int read();
extern int write();
extern int ioctl();
extern int alarm();

static int serial_open();
static int serial_close();
static int serial_read();
static void serial_signal();
static int serial_intr_trgt();
#endif /* __STDC__ */


static const DEV ttydev = { serial_open, serial_close, serial_read,
                            serial_write, serial_signal, serial_intr_trgt };

static const struct baudmap baudmap[] = {
        300,	B300,
        1200,	B1200,
        2400,	B2400,
        4800,	B4800,
        9600,	B9600,
        19200,	B19200,
        38400,	B38400,
#if defined(SUNOS_SUN4) || defined(SOLARIS_SUN4)
        57600,	B75,      /* Magic values used by MAGMA to map useless Unix */
        115200,	B1800,    /* baud rates to high speed values supported by   */ 
						  /* their SBUS add-in cards.                       */
#endif
#ifdef HP700
        57600,	_B57600,
        115200,	_B115200,
#endif
        0
};



static int
report_serial_sys_err(str)
const char *str;
{
    sprintf(errparam, "%s %s", str, _com_config.device);
    hdi_err_param1  = errparam;
    com_stat        = E_SYS_ERR;
    _hdi_save_errno = errno;
    return (ERR);
}



static int
report_para_sys_err(str)
const char *str;
{
    sprintf(errparam, "%s %s", str, para_device);
    hdi_err_param1  = errparam;
    hdi_cmd_stat    = E_PARA_SYS_ERR;
    _hdi_save_errno = errno;
    return (ERR);
}

const DEV *
com_serial_dev()
{
    return(&ttydev);
}


/* ARGSUSED */
static int
serial_open()
{
    int        baud_code, fd, n;
    TTY_STRUCT tty;

    if (_com_config.dev_type == 0)
        _com_config.dev_type = RS232;

#if defined(SUNOS_SUN4) || defined(SOLARIS_SUN4)
    if (_com_config.baud == 57600 || _com_config.baud == 115200)
    {
        char *cp;

        /* 
         * Normally, SunOS does not support these baud rates, but MAGMA
         * has add-in SBUS cards for Sparc Workstations that do.
         * If it appears the user is attempting to set a higher baud
         * rate for a MAGMA serial port, let him/her try...
         */

        if (! (
                  (cp = strrchr(_com_config.device, '/')) &&
                  cp[1] == 't' && 
                  cp[2] == 't' && 
                  cp[3] == 'y' && 
                  cp[4] == 'm'
               ))
        {
           /*
            * User is not accessing /dev/ttymXX, which is the serial port 
            * naming scheme used by MAGMA.  Time to quit.
            */

            com_stat = E_BAD_CONFIG;
            return(ERR);
        }
    }
#endif

    baud_code = xltbaud(_com_config.baud, baudmap);
    if (baud_code <= 0)
    {
        com_stat = E_BAD_CONFIG;
        return(ERR);
    }

#ifdef USG
    fd = OPEN_TTY(_com_config.device, O_RDWR | O_EXCL | O_NDELAY);
#else
    fd = OPEN_TTY(_com_config.device, O_RDWR | O_NDELAY);
#endif
    if (fd < 0)
        return (report_serial_sys_err("opening"));
    else
    {
#ifndef USG
#ifdef TIOCEXCL
        /* Set exclusive use mode (hp9000 does not support it) */

        if (ioctl(fd, TIOCEXCL, NULL) < 0)
            return (report_serial_sys_err("setting attributes of"));
#endif
#endif
    }
    if (TTY_GET(fd, tty) < 0)
        return (report_serial_sys_err("reading attributes of"));
    TTY_REMOTE(tty, baud_code);
    if (TTY_SET(fd, tty) < 0)
        return (report_serial_sys_err("setting attributes of"));
    return(fd);
}



static int
serial_close(fd)
int fd;
{
    int rc;

    rc = close(fd);
    if (rc == -1 && errno == EPERM)
    {
        /*
         * Filter this error, which shows up on SUN3's, for some reason.
         * This was not a problem for NINDY, because NINDY does not
         * close file descriptors and instead relies upon the OS to
         * do that when the application exits.
         */

        rc = 0;
    }
    return (rc);
}



static void (*prev_int)() = NULL, (*prev_alrm)() = NULL;
jmp_buf jb;

static void
mysig(sig)
int sig;
{
    void (*prev)();

    switch (sig)
    {
	case SIGALRM: prev = SIG_IGN; com_stat = E_COMM_TIMO; break;
	case SIGINT:  prev = prev_int; com_stat = E_INTR; break;
	default: prev = SIG_IGN; break;
    }

    if (prev != SIG_IGN && prev != SIG_DFL)
#ifdef SIG_HOLD
	if (prev != SIG_HOLD)
#endif
	    (*prev)(sig);

    longjmp(jb, 1);
}

/*
 * serial_read() reads up to size bytes into buf.
 *
 * The "timo" parameter has three distinct values that determines the
 * functions semantics:
 *
 * COM_POLL         -- perform a single nonblocking read and return
 *                     whatever data is available, if any.
 *
 * COM_WAIT_FOREVER -- wait until data arrives.
 *
 * positive value   -- wait up to "timo" milliseconds for data to
 *                     arrive.  Bear in mind that millisecond timeouts
 *                     are not available on stock USG (SystemV 3.2) 
 *                     Unix systems.
 *
 * If no data is available when a timeout expires, this function returns -1.
 * Otherwise, this function returns the number of characters read.
 */

static int
serial_read(fd, buf, size, timo)
int fd;
unsigned char *buf;
int size;
int timo;
{
    int  r = -1;

#if (defined(USG) | defined(POSIX))   /* See comment below at matching #else */

    int      alarm_set = FALSE;
    unsigned secs;

    if (setjmp(jb) == 0)
    {
        prev_int = signal(SIGINT, mysig);
        if (timo == COM_POLL)
        {
            TTY_NBREAD(fd, size, (char *)buf);   /* NBREAD retval stored in
                                                  * "size".
                                                  */
            r = size;

            /*
             * You don't want to check for errors here, because on some
             * systems "r" will be set to -1 and errno set to EWOULDBLOCK
             * when no data is available.  We'll use the NINDY model here,
             * which is to just do the read and let the chips fall where
             * they will.
             */
        }
        else if (timo == COM_WAIT_FOREVER)
        {
            if ((r = read(fd, (char *)buf, size)) < 0)
            {
                perror("blocking read");
                com_stat = E_COMM_ERR;
            }
        }
        else
        {
            prev_alrm = signal(SIGALRM, mysig);
            alarm_set = TRUE;
            if (timo < 1000)
                secs = 1;
            else
                secs = timo / 1000;
#if defined(HP9000)
            secs++;     /* More time needed. */
#endif
            alarm(secs);
	    r = read(fd, (char *)buf, size);
            if (r < 0)
            {
                perror("timed read");
                com_stat = E_COMM_ERR;
            }
        }
    }
    if (alarm_set)
    {
        alarm(0);
        (void) signal(SIGALRM, prev_alrm);
    }
    (void) signal(SIGINT, prev_int);

#else             
    /* 
     * We have no working code for BSD-style serial I/O .  But, that's not
     * a big deal because all Unix hosts support USG-style serial I/O.
     *
     * Emit error message using (essentially) a compiler syntax error.  We
     * can't use the #error directive because that's an ANSI C feature.
     */

    error  "BSD-sytle serial I/O not supported!"

#endif 

    return(r);
}



#ifndef HP700
static int
serial_write(fd, buf, size)
int fd;
register const unsigned char *buf;
register int size;
{
    /*
     * Since the serial port was opened with O_NDELAY, we must be prepared
     * to handle partial writes to the port and/or full buffer queues.
     */

    int          blocks;   /* Track no. of times I/O is blocked. */
    int          sent;
    register int orig_size = size;
    int          written;

    blocks = sent = 0;
    while (sent < orig_size)
    {
        written = write(fd, (char *) buf, size);
        if (written < 0)
        {
            if ((errno == EAGAIN || errno == EWOULDBLOCK) && blocks < 2)
            {
                /* 
                 * Serial port driver's output queue is full, wait
                 * a bit for it to drain.
                 */

                sleep(1);
                blocks++;
            }
            else
            {
                (void) report_serial_sys_err("writing");
                hdi_cmd_stat = com_stat;
                return (ERR);
            }
        }
        else if (written == 0)
        {
            hdi_cmd_stat = E_COMM_ERR;
            return (ERR);
        }
        else if (written != size)
        {
            sleep(1);     /* Wait for device's output buffers to drain. */
            buf    += written;
            size   -= written;
            blocks  = 0;  /* Some data xferred, reset port block count. */
        }
        sent += written;
    }
}
#endif /* !defined(HP700) */



/* ARGSUSED */
static void
serial_signal(fd)
int fd;
{
}



#ifndef USG
#ifndef POSIX

static void
bsd_break_alarm_handler()
{
    return;
}

#endif /* POSIX */
#endif /* USG   */



/* ARGSUSED */
static int
serial_intr_trgt(fd)
int fd;
{
#ifdef POSIX

    TTY_FLUSH(fd);
    tcsendbreak( fd, 250 );

#else
#ifdef USG

    TTY_FLUSH(fd);
    ioctl( fd, TCSBRK, 0 );

#else /* BSD */

    struct itimerval t;
    void (*old_alarm)();    /* Alarm signal handler on entry */

	old_alarm = signal( SIGALRM, bsd_break_alarm_handler );

    /* Set timer for 1/4 second break */
    t.it_value.tv_sec = 0;
    t.it_value.tv_usec = 250000;
    t.it_interval.tv_sec = t.it_interval.tv_usec = 0;

    /* Assert break for the duration of the timer */
    ioctl( fd, TIOCSBRK, 0 );
    setitimer( ITIMER_REAL, &t, 0 );
    sigpause(0);
    ioctl( fd, TIOCCBRK, 0 );

    signal( SIGALRM, old_alarm );

#endif /* USG */
#endif /* POSIX */

    return(OK);
}


/* ---------------- Begin Unix Parallel I/O Routines ----------------- */

static int par_generic_end();
static int par_none_end();
static int par_none_init();
static int par_none_put();

#define PAR_TIMEOUT      6    /* Download times out after six seconds */

#if defined(RS6000)
#   include <sys/lpio.h>

    static int par_rs6000_init(), par_rs6000_put();

#endif

#if defined(SUNOS_SUN4)

#   include <stropts.h>
#   include <sbusdev/bpp_io.h>

#   define SUN_BPP    0
#   define MAGMA_PM   1

    static int par_sun_mbus_hw_init(), /* Interface to Sparc 4, 5,  */
               par_sun_mbus_hw_put();  /* 10, and 20 hosts.         */
    static int par_magma_hw_init(),    /* Interface to MAGMA Sp and */
               par_magma_hw_put();     /* 2+1 Sp SBUS cards         */
#endif

#if defined(SOLARIS_SUN4)

#   include <stropts.h>
#   include <sys/conf.h>
#   include <sys/ioccom.h>
#   include <sys/bpp_io.h>

#   define SUN_BPP    0
#   define MAGMA_PM   1

    static int par_sun_mbus_hw_init(), /* Interface to Sparc 4, 5,  */
               par_sun_mbus_hw_put();  /* 10, and 20 hosts.         */
    static int par_magma_hw_init(),    /* Interface to MAGMA Sp and */
               par_magma_hw_put();     /* 2+1 Sp SBUS cards         */
#endif

#if defined(HP700)

    static int par_hp700_init(), par_hp700_put();

#endif

/*
 * Setup arrays of function pointers to handle multiple parallel drivers
 * per OS.  Example:  there are at least two different types of parallel
 * ports and drivers for Sun workstations running SunOS 4.1.3 .
 */
static int (*com_par_init[])() = 
{ 
#if defined(RS6000)
    par_rs6000_init,
#else
#if defined(SUNOS_SUN4) || defined(SOLARIS_SUN4)
    par_sun_mbus_hw_init,
    par_magma_hw_init,
#else
#if defined(HP700)
    par_hp700_init,
#else
    par_none_init,
#endif
#endif
#endif
};

static int (*com_par_put[])() = 
{ 
#if defined(RS6000)
    par_rs6000_put,
#else
#if defined(SUNOS_SUN4) || defined(SOLARIS_SUN4)
    par_sun_mbus_hw_put,
    par_magma_hw_put,
#else
#if defined(HP700)
    par_hp700_put,
#else
    par_none_put,
#endif
#endif
#endif
};

static int (*com_par_end[])() = 
{ 
#if defined(RS6000) || defined(SUNOS_SUN4) || defined(HP700) || \
                                                        defined(SOLARIS_SUN4)
    par_generic_end,
#else
    par_none_end,
#endif
};


static int parallel_fd = 0;
static int (*par_end)(), (*par_init)(), (*par_put)();

/* ------------------------------------------------------------------ */
/* ------------ Common Parallel Routines (all OS's & HW) ------------ */
/* ------------------------------------------------------------------ */

int 
com_parallel_init(device)
char * device;
{
#if defined(SUNOS_SUN4) || defined(SOLARIS_SUN4)

    /*
     * Handle the case where a single OS has more than one HW/driver
     * capability (e.g., SunOS).  Currently, we support the bpp driver
     * provided by Sun Microsystems and the pm driver supplied by MAGMA. 
     * For now, we'll select interface routines based upon the name of the
     * device passed to this routine.  We may need a different selection
     * algorithm in the future if we encounter 3rd party HW that uses the
     * same device names as Sun & MAGMA, but requires different drivers.
     */
    
    char *cp;

    cp = strrchr(device, '/');
    if (! cp)
        cp = device;
    else
        cp++;
    if (strncmp(cp, "bpp", 3) == 0)
    {
        par_end  = par_generic_end;
        par_init = com_par_init[SUN_BPP];
        par_put  = com_par_put[SUN_BPP];
    }
    else if (strncmp(cp, "pm", 2) == 0 || strncmp(cp, "pnm", 3) == 0)
    {
        par_end  = par_generic_end;
        par_init = com_par_init[MAGMA_PM];
        par_put  = com_par_put[MAGMA_PM];
    }
    else
    {
        par_end  = par_none_end;
        par_init = par_none_init;
        par_put  = par_none_put;
    }

#else

    par_end  = com_par_end[0];
    par_init = com_par_init[0];
    par_put  = com_par_put[0];

#endif

    return ((* par_init)(device));
}



int 
com_parallel_put(buf, size, crc)
unsigned char *buf;
unsigned int size;
unsigned short *crc;
{
    return ((* par_put)(buf, size, crc));
}



int 
com_parallel_end()
{
    return ((* par_end)());
}

/* ------------------------------------------------------------------ */
/* ------------- HW/OS-specific Parallel Init Routines -------------- */
/* ------------------------------------------------------------------ */

#if defined(RS6000)

static int 
par_rs6000_init(device)
char * device;
{
    struct lprmod lprmode;

    strcpy(para_device, device);

    /* 
     * Note that it's necessary to open the parallel port in O_NDELAY
     * mode because earlier versions of the monitor did not properly
     * init the SELECT and PAPER OUT bits....  This has implications
     * for par_rs6000_put().
     */
    parallel_fd = openx(device, O_WRONLY|O_EXCL|O_NDELAY, 0, 0);
    if (parallel_fd < 0)
        return (report_para_sys_err("opening"));

    lprmode.modes = PLOT;
    if (ioctl(parallel_fd, LPRMODS, &lprmode) < 0)
        return (report_para_sys_err("setting attributes of"));
    
    /*
     * Note:  attempting to set a parallel port timeout using the LPRSTOV
     * ioctl() does not appear to work.  We'll workaround this problem using
     * setjmp/longjmp in par_rs6000_put().
     */
    return(OK);
}

#else
#if defined(SUNOS_SUN4) || defined(SOLARIS_SUN4)

static int
par_sun_mbus_hw_init(device)
char *device;
{
    struct bpp_transfer_parms bpp;
    struct bpp_pins pins;

    strcpy(para_device, device);
    parallel_fd = open(device, O_WRONLY|O_EXCL);
    if (parallel_fd < 0)
        return (report_para_sys_err("opening"));

    bpp.write_handshake    = BPP_BUSY_HS;
    bpp.write_timeout      = PAR_TIMEOUT; /* seconds                        */
    bpp.write_strobe_width = 1000;        /* Empirically derived (it works) */
    bpp.write_setup_time   = 40;          /* Empirically derived (it works) */
    bpp.read_handshake     = BPP_NO_HS;
    bpp.read_strobe_width  = 1000;
    bpp.read_setup_time    = 40;

    pins.output_reg_pins = BPP_INIT_PIN;
    pins.input_reg_pins  = 0;

    if ((ioctl(parallel_fd, BPPIOC_SETPARMS, &bpp) < 0)     ||
                       (ioctl(parallel_fd, BPPIOC_SETOUTPINS, &pins) < 0))
    {
        return (report_para_sys_err("setting attributes of"));
    }

    return(OK);
}



static int
par_magma_hw_init(device)
char *device;
{
    strcpy(para_device, device);

    /* 
     * Note that it's necessary to open the parallel port in O_NDELAY
     * mode because earlier versions of the monitor did not properly
     * init the SELECT and PAPER OUT bits....  This has implications
     * for par_magma_put().
     */
    if ((parallel_fd = open(device, O_WRONLY|O_EXCL|O_NDELAY)) < 0)
        return (report_para_sys_err("opening"));
    
    /* 
     * MAGMA parallel device driver will not relinquish the parallel
     * port if the monitor dies during a parallel download _and_ the
     * ttcompat and ldterm STREAMS modules are installed.  Pop these 
     * unneeded modules now....
     *
     * Oh, by the way, popping these modules will ensure that the
     * ttcompat/ldterm modules do not strip the high bits of downloaded
     * data and that no postprocessing of data (CR -> NL, etc.) is 
     * attempted.
     */
    while (ioctl(parallel_fd, I_POP, 0) >= 0)
        ;

    return (OK);
}


#else
#if defined(HP700)

static int
par_hp700_init(device)
char *device;
{
    strcpy(para_device, device);
    parallel_fd = open(device, O_WRONLY|O_EXCL);
    if (parallel_fd < 0)
        return (report_para_sys_err("opening"));
    io_reset(parallel_fd);

    /* 
     * Note:  io_timeout_ctl() does not appear to work for parallel ports.
     *
     * We'll use setjmp/lonjmp in par_hp700_put() to workaround the problem.
     */

    return(OK);
}

#endif
#endif
#endif



static int 
par_none_init(device)
char * device;
{
    hdi_cmd_stat = E_PARA_NOCOMM;
    return (ERR);
}

/* ------------------------------------------------------------------ */
/* ------------- HW/OS-specific Parallel Put Routines --------------- */
/* ------------------------------------------------------------------ */

#if defined(RS6000)

static int 
par_rs6000_put(buf, size, crc)
unsigned char *buf;
unsigned int size;
unsigned short *crc;
{
    register unsigned char *orig_buf = buf;
    register unsigned long orig_size = size;

    if (setjmp(jb) == 0)
    {
        int           blocks = 0;   /* Track no. of times I/O is blocked. */
        unsigned long sent   = 0;
        long          written;

        /*
         * Since parallel port was opened O_NDELAY, must be prepared to
         * handle the case where number of bytes written does not match
         * buffer size.
         */
        while (sent < orig_size)
        {
            prev_alrm = signal(SIGALRM, mysig);

            /*
             * Adjust AIX alarm timeout for the slowest version of the 
             * parallel driver, which is ~9 KB/sec (groan) on AIX 3.2 .
             */
            alarm(MAX_DNLOAD_BUF / (9*1024) + 1);
            written = (long) write(parallel_fd, (char *) buf, size);
            alarm(0);
            (void) signal(SIGALRM, prev_alrm);
            if (written < 0)
            {
                if (errno == EAGAIN && blocks < 2)
                {
                    /* 
                     * Parallel port driver's output queue is full, wait
                     * a bit for it to drain.
                     */

                    sleep(1);
                    blocks++;
                }
                else
                    return (report_para_sys_err("writing"));
            }
            else if (written == 0)
            {
                hdi_cmd_stat = E_PARA_DNLOAD_TIMO;
                return (ERR);
            }
            else if (written != size)
            {
                sleep(1);     /* Wait for device's output buffers to drain. */
                buf    += written;
                size   -= written;
                blocks  = 0;  /* Some data xferred, reset port block count. */
            }
            sent += written;
        }
    }
    else
    {
        /* Timed out */

        alarm(0);
        (void) signal(SIGALRM, prev_alrm);
        hdi_cmd_stat = E_PARA_DNLOAD_TIMO;
        return (ERR);
    }
    if (crc != NULL)
    {
        register unsigned short sum;

        sum = *crc;
        while (orig_size--)
            sum = CRCtab[(sum^(*orig_buf++))&0xff] ^ sum >> 8;
        *crc = sum;
    }
    return (OK);
}

#else
#if defined(SUNOS_SUN4) || defined(SOLARIS_SUN4)

static int
par_sun_mbus_hw_put(buf, size, crc)
register unsigned char *buf;
register unsigned long size;
unsigned short *crc;
{
    long written;

    written = (long) write(parallel_fd, (char *) buf, size);
    if (written < 0)
        return (report_para_sys_err("writing"));
    else if (written == 0)
    {
        hdi_cmd_stat = E_PARA_DNLOAD_TIMO;
        return (ERR);
    }
    else if (written != size)
    {
        hdi_cmd_stat = E_PARA_WRITE;
        return (ERR);
    }
    if (crc != NULL)
    {
        register unsigned short sum;

        sum = *crc;
        while (size--)
            sum = CRCtab[(sum^(*buf++))&0xff] ^ sum >> 8;
        *crc = sum;
    }
    return (OK);
}



static int
par_magma_hw_put(buf, size, crc)
register unsigned char *buf;
register unsigned long size;
unsigned short *crc;
{
    register unsigned char *orig_buf = buf;
    register unsigned long orig_size = size;

    if (setjmp(jb) == 0)
    {
        int           blocks = 0;   /* Track no. of times I/O is blocked. */
        unsigned long sent   = 0;
        long          written;

        /*
         * Since parallel port was opened O_NDELAY, must be prepared to
         * handle the case where number of bytes written does not match
         * buffer size.
         */
        while (sent < orig_size)
        {
            prev_alrm = signal(SIGALRM, mysig);

            /*
             * Note that the timeout value passed to alarm() is not
             * necessarily pertinent, as the MAGMA parallel device driver
             * has its own hardwired timeout value and won't relinquish
             * control until that timeout has expired (the timeout is
             * hardwired to ensure that line printer daemons don't
             * prematurely timeout when xferring large postscript files). 
             * Yes, a configurable device driver timeout would be nice --
             * but it's not available.
             */
            alarm(PAR_TIMEOUT);
            written = (long) write(parallel_fd, (char *) buf, size);
            alarm(0);
            (void) signal(SIGALRM, prev_alrm);
            if (written < 0)
            {
                if ((errno == EAGAIN || errno == EWOULDBLOCK) && blocks < 2)
                {
                    /* 
                     * Parallel port driver's output queue is full, wait
                     * a bit for it to drain.
                     */

                    sleep(1);
                    blocks++;
                }
                else
                    return (report_para_sys_err("writing"));
            }
            else if (written == 0)
            {
                hdi_cmd_stat = E_PARA_DNLOAD_TIMO;
                return (ERR);
            }
            else if (written != size)
            {
                sleep(1);     /* Wait for device's output buffers to drain. */
                buf    += written;
                size   -= written;
                blocks  = 0;  /* Some data xferred, reset port block count. */
            }
            sent += written;
        }
    }
    else
    {
        /* Timed out */

        alarm(0);
        (void) signal(SIGALRM, prev_alrm);
        hdi_cmd_stat = E_PARA_DNLOAD_TIMO;
        return (ERR);
    }
    if (crc != NULL)
    {
        register unsigned short sum;

        sum = *crc;
        while (orig_size--)
            sum = CRCtab[(sum^(*orig_buf++))&0xff] ^ sum >> 8;
        *crc = sum;
    }
    return (OK);
}



#else
#if defined(HP700)

static int
par_hp700_put(buf, size, crc)
register unsigned char *buf;
register unsigned long size;
unsigned short *crc;
{
    if (setjmp(jb) == 0)
    {
        long written;

        prev_alrm = signal(SIGALRM, mysig);
        alarm(PAR_TIMEOUT);
        written = (long) write(parallel_fd, (char *) buf, size);
        alarm(0);
        (void) signal(SIGALRM, prev_alrm);
        if (written < 0)
            return (report_para_sys_err("writing"));
        else if (written == 0)
        {
            hdi_cmd_stat = E_PARA_DNLOAD_TIMO;
            return (ERR);
        }
        else if (written != size)
        {
            hdi_cmd_stat = E_PARA_WRITE;
            return (ERR);
        }
    }
    else
    {
        /* Timed out */

        alarm(0);
        (void) signal(SIGALRM, prev_alrm);
        hdi_cmd_stat = E_PARA_DNLOAD_TIMO;
        return (ERR);
    }
    if (crc != NULL)
    {
        register unsigned short sum;

        sum = *crc;
        while (size--)
            sum = CRCtab[(sum^(*buf++))&0xff] ^ sum >> 8;
        *crc = sum;
    }
    return (OK);
}

#endif
#endif
#endif



static int 
par_none_put(buf, size, crc)
unsigned char *buf;
unsigned int size;
unsigned short *crc;
{
    return (par_none_init());
}

/* ------------------------------------------------------------------ */
/* ------------ Routines to terminate parallel download ------------- */
/* ------------------------------------------------------------------ */

static int
par_generic_end()
{
    close(parallel_fd);
    return (OK);
}



static int
par_none_end()
{
    return (par_none_init());
}
