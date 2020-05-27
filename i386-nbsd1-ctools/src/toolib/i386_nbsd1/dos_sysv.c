
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

/*
 * System V emulation functions for DOS.
 *
 */

#include <dos.h>
#include <time.h>
#include <signal.h>
#include "gnudos.h"

/*
 * Timer interrupt vector
 */
#define TIMER_IVEC	0x4a	/* Address of timer interrupt vector */

/*
 * DOS BIOS real time clock functions
 */
#define RTCLOCK_FUNC	0x1a	/* BIOS interrrupt vector	*/
#define DISABLE_CLOCK	0x07	/* Subfunction code		*/
#define SET_ALARM	0x06	/* Subfunction code		*/

/* 
 * DOS-Extender interrupt handling is messy.  Metaware requires you 
 * to save and restore BOTH the real-mode and protected-mode handlers. 
 */
#ifdef	__HIGHC__
static _real_int_handler_t orig_rm_alarm_vec;
static _Far _INTERRPT void (*orig_pm_alarm_vec)();
#else
static void (*orig_alarm_vec)() = 0;
#endif


/**********************************************************************
 * _gnudos_timerint()
 *
 *	This routine raises the C catchable signal SIGALRM (which for
 *	DOS is really SIGUSR1).  Since the timer has gone off, it also
 *	automatically resets the timer interrupt routine to the 
 *	original one, in case another timer goes off.
 *
 *	Returns:
 *		raises SIGUSER1 (defined for the GNU DOS port as SIGALRM)
 ***************************************************************************/
#ifdef	_INTEL32_
#pragma interrupt(_gnudos_timerint)
#endif

#ifdef	__HIGHC__
_Far _INTERRPT void
_gnudos_timerint()
{
	_setrvect(TIMER_IVEC, orig_rm_alarm_vec);
	_setpvect(TIMER_IVEC, orig_pm_alarm_vec);
	orig_rm_alarm_vec = 0;
	orig_pm_alarm_vec = NULL;
	raise (SIGALRM);
}
#else
static void
_gnudos_timerint()
{
	_dos_setvect(TIMER_IVEC, orig_alarm_vec);
	orig_alarm_vec = 0;
	raise(SIGALRM);
}
#endif  /* Metaware High C */


/**********************************************************************
 * alarm()
 *	This routine handles a timer for dos equivalent to the 
 *	Unix System V alarm() timer functionality.
 *
 *	This routine always sets the "timer signal handler" by storing
 *	the signal handler routine in dos vector location 0x4a, then 
 *	clears the current DOS timer via a DOS BIOS function.  Finally,
 *	if the number of seconds is non-zero, it converts the seconds
 *	into BCD format, and calls DOS BIOS to set the timer.
 ***************************************************************************/
#define TO_BCD(value) ((unsigned short) ((((value)/10) << 4) | ((value)%10)))
#include <stdio.h>
unsigned
alarm(sec)
    unsigned sec;	/* number of seconds to delay */
{
#if defined(WIN95)
	/* Something should go here, no? */
#else
	union REGS inregs;
	union REGS outregs;

	/*
	 * Save the current timer interrupt vector, install ours
	 */
#ifdef	__HIGHC__
	if ( ! orig_rm_alarm_vec )
	{
		orig_rm_alarm_vec = _getrvect(TIMER_IVEC);
		orig_pm_alarm_vec = _getpvect(TIMER_IVEC);
		_setrpvectp(TIMER_IVEC, _gnudos_timerint);
			/* FIXME-SOON:  There should be a diagnostic here if setrpvectp fails */
	}
#else
	if ( ! orig_alarm_vec ) 
	{
		orig_alarm_vec = _dos_getvect(TIMER_IVEC);
		_dos_setvect(TIMER_IVEC, _gnudos_timerint);
	}
#endif
	/*
	 * Stop the current timer. if any.
	 * Ignore errors, in case there was not a previous timer started.
	 */
	inregs.h.ah = DISABLE_CLOCK;
#ifdef _INTELC32_
	int386(RTCLOCK_FUNC, &inregs, &outregs);
#else
	int86(RTCLOCK_FUNC, &inregs, &outregs);
#endif
	if (sec == 0) 
	{
		/*
		 * Turn the timer off: restore the original interrupt vector.
		 */
#ifdef	__HIGHC__
		_setrvect(TIMER_IVEC, orig_rm_alarm_vec);
		_setpvect(TIMER_IVEC, orig_pm_alarm_vec);
		orig_rm_alarm_vec = 0;
		orig_pm_alarm_vec = NULL;
#else
		_dos_setvect(TIMER_IVEC, orig_alarm_vec);
		orig_alarm_vec = 0;
#endif
	} 
	else 
	{
		/*
		 * Translate binary seconds to BCD, start timer.
		 */
		inregs.h.ah = SET_ALARM;
		inregs.h.ch = TO_BCD(sec/3600);	/* hours */
		sec = sec%3600;
		inregs.h.cl = TO_BCD(sec/60);	/* minutes */
		inregs.h.dh = TO_BCD(sec%60);	/* seconds */
#ifdef _INTELC32_
		int386(RTCLOCK_FUNC, &inregs, &outregs);
#else
		int86(RTCLOCK_FUNC, &inregs, &outregs);
#endif
		if (outregs.x.cflag != 0)
			return 0;
	}
#endif	/* !WIN95 */
	return 1;
}

int ext;

/***************************************************************************
 * sleep()
 *	This routine is replacement for the System V Unix call.
 *	It waits for the number of seconds requested.
 *
 ***************************************************************************/
void
sleep(secs) 
    int secs;		/* number of seconds to wait */
{
	time_t timeval;

 	for ( timeval = time(NULL)+secs; time(NULL) < timeval; )
		;
}

		
/************************************************************************
 * Various stubs
 ************************************************************************/

closedir() { }
endpwent() { }
getpwent() { }
getpwnam() { }
getuid() { }
getgid() { }
opendir() { }
output_character_function() { }
readdir() { }
setpwent() { }
sigsetmask() { }
tgoto() { }
tputs() { }

int
fork ( )
{
	return 0;
}

#ifndef __HIGHC__
int
kill( int pid )
{
	return 0;
}
#endif

int
pipe ( )
{
	return 0;
}

int
wait( int *pid )
{
	return 0;
}

/************************************************************************
 * Stubs for system V terminfo calls, needed primarily by with gdb960.
 ************************************************************************/
tgetent()
{
	return -1;	/* -1 implies dumb terminal */
}

tgetnum() 
{
	return -1;	/* -1 sets defaults */
}

tgetstr()
{
	return 0;
}

tgetflag()
{
	return 0;
}

