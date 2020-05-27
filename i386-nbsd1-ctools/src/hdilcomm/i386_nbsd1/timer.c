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
 * Timer module
 *	Entry points are:
 *		init_timer()
 *		term_timer()
 *		settimeout()
 *	Implemented by capturing the DOS timer tick "chain vector" with
 *	an asm routinue which calls into local function tick.
 */

/* $Header: /ffs/p1/dev/src/hdilcomm/common/RCS/timer.c,v 1.4 1995/02/15 18:59:03 gorman Exp $$Locker */

#if defined(__HIGHC__)
#include <dos.h>
#endif

#include "common.h"
#include "funcdefs.h"

/* 
 * DOS-Extender interrupt handling is messy.  Metaware requires you 
 * to save and restore BOTH the real-mode and protected-mode handlers. 
 */
#ifdef	__HIGHC__
static _real_int_handler_t orig_rm_timer_vec;
static _Far _INTERRPT void (*orig_pm_timer_vec)();
#else
static intr_handler prev_timer_intr;
#endif	/* Metaware */


#ifdef _MSC_VER
extern void INTR_HANDLER timer_intr_wrap(void);
#endif
void INTR_HANDLER timer_intr(void);

#define DBG_FLAG IFLAG

# define	TKPSEC		17
# define	MSECPTK		(1000/TKPSEC)
# define	TMRVEC		0x1c
# define 	CHAINVEC	0x48
		/* CHAINVEC is an otherwise unused vector to work
		 * around Metaware's lack of a chain_intr() function.
		 */

/* variables */
static int	bad;			/* bad pointer, in case 
					   of bad timeout pointer */

static int		*timeoutp = &bad;	/* timeout flag pointer */
static unsigned short	timecnt = 0;		/* timeout ticks remaining */

static int	timerIV_installed = 0;		/* Timer intr vector initialized? */

static int	occurcnt = 0;		/* debug, time out count */

/*
 * Set a "background" timer which sets requested flag when alloted 
 * interval expires.
 *
 * WARNING: Normal usage would settimeout( > 0 ), process until some event 
 * occurs OR this flag sets (indicating times up), then settimeout(0)
 * in order to turn the timer off.
 * This is VERY important, because if the requester flag is for instance, 
 * a stack variable, and the caller returns before the timeout; then, when 
 * the timer does go off, it will set some, now defunct location.
 */
int
settimeout(unsigned int Msecs, int *flagp)
{
	int pif;

	if (!timerIV_installed) {
		return(ERR);
	}

	pif = CHANGE_IF(0);
	if (Msecs == 0) {
		timeoutp = &bad;
		timecnt = 0;
	}
	else
	{
		timeoutp = flagp;
		timecnt = (Msecs + (MSECPTK - 1)) / MSECPTK + 1;
	}
	CHANGE_IF(pif);

	return(OK);
}


/*
 * C interrupt service routinue
 */

#ifdef _MSC_VER
/* Called from interrupt routine -- stack is not where you think,
 * therefore, stack check would be erroneous (and meaningless).
 */
#pragma check_stack(off)
#endif
#ifdef	__HIGHC__
/* 
 * Work around a Metaware bug. Compiler stack-check code sequence
 * assumes valid DS register.  But we are about to force-feed DS in
 * the interrupt handler, so we turn Check_stack off.
 */
#pragma Off(Check_stack);
#endif

void INTR_HANDLER
timer_intr(void)
{
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
	if ( timecnt && ! --timecnt )
	{
		*timeoutp = 1;
		++occurcnt;		/* for debug */
	}

#ifndef  __HIGHC__
	/*
	 * Metaware does not provide a _chain_intr() function.  We will
	 * somewhat naively ignore this until a workaround is found.
	 */
	_chain_intr(prev_timer_intr);
#endif
}

#ifdef _MSC_VER
#pragma check_stack()	/* Restore to previous setting */
#endif
#ifdef __HIGHC__
#pragma Pop(Check_stack); /* reinstate original value of Check_stack */
#endif



/*
 * Initialize our timer support.
 */
init_timer(void)
{
	if (!timerIV_installed)
	{
#if	defined(__HIGHC__)
		orig_rm_timer_vec = _getrvect(TMRVEC);
		orig_pm_timer_vec = _getpvect(TMRVEC);

		/* Workaround for DPMI behavior for hardware interrupt 0x1c: 
		 * This interrupt is generated in real mode, Phar Lap swiches
		 * to protected mode and gives us control here.  Then Windows 
		 * echoes the interrupt AGAIN in protected mode and we 
		 * process it a second time.  We want to handle the interrupt 
		 * only once.  
		 *
		 * So find out if a version of Windows >= 3.0 is running.  
		 * If so, install the handler so that it will get control only
		 * if the interrupt occurs in protected mode.  Otherwise, 
		 * install it to take control in either mode.
		 */
		if ( windows_check() )
			_setpvect(TMRVEC, timer_intr);
		else
			_setrpvectp(TMRVEC, timer_intr);
	
#else	/* Neither Codebuilder nor Metaware */
		prev_timer_intr = GET_IV(TMRVEC);
		SET_IV(TMRVEC, timer_intr_wrap);
#endif
		timerIV_installed = 1;
		return(OK);
	} else {
		/* Already initialized */
		return(ERR);
	}
}


/*
 * Terminate our timer support, releasing any resources which were used.
 * Note that by restoring the vector to it's previous value, we are
 * assuming that no new program has chained into the timer interrupt
 * since we did.
 *
 * This may be called during an interrupt (to clean up before termination),
 * so no DOS calls allowed (e.g., debug printing).
 */
term_timer(void)
{
	if (!timerIV_installed)
		return(ERR);

#if defined(__HIGHC__)
	_setrvect(TMRVEC, orig_rm_timer_vec);
	_setpvect(TMRVEC, orig_pm_timer_vec);
#else /* not Metaware */
	SET_IV(TMRVEC, prev_timer_intr);
#endif

	timerIV_installed = 0;
	return(OK);
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

#endif
