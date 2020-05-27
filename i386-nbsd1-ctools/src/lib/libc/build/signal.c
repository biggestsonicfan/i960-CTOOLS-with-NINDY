/*******************************************************************************
 * 
 * Copyright (c) 1993-1995 Intel Corporation
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

/* signal - set response to a signal
 * Copyright (c) 1985,86,87 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <stddef.h>
#include <signal.h>
#include <errno.h>

extern void _Lsignal_init();

void (*signal(sig, func))()
int sig; 				/* signal type */
void (*func)(); 			/* response to signal */
{
    void (*ff)();

    _Lsignal_init();

    if (!func || sig < 0 || sig >= SIGSIZE) {
        errno = ESIGNAL;
        return SIG_ERR;
    }

    ff = _sig_eval[sig];

    if (func == SIG_IGN)
        _sig_eval[sig] = _sig_null;
    else if (func == SIG_DFL)
        _sig_eval[sig] = _sig_dfl(sig);
    else
        _sig_eval[sig] = (func);

    if (ff == _sig_null)
        ff = SIG_IGN;
    else if (ff == _sig_dfl(sig))
        ff = SIG_DFL;
    return ff;
}

/* Signal vector array: these are the routines which get
 * executed when the raise() function is called.
 */
void (*_sig_eval[SIGSIZE])();

/*
 * Default signal vector array; this is initialized once, then
 * left unchanged.  This isn't done statically since the addresses
 * that go in here can be affected by PIC code generation.
 */
void (*_sig_dfl(int signo))()
{
  switch (signo)
  {
    case SIGUSR1:
    case SIGUSR2:
    default:
      return _sig_null;

    case SIGILL:
      return _sig_ill_dfl;

    case SIGINT:
      return _sig_int_dfl;

    case SIGALLOC:
      return _sig_alloc_dfl;

    case SIGFREE:
      return _sig_free_dfl;

    case SIGTERM:
      return _sig_term_dfl;

    case SIGREAD:
      return _sig_read_dfl;

    case SIGWRITE:
      return _sig_write_dfl;

    case SIGFPE:
      return _sig_fpe_dfl;

    case SIGSEGV:
      return _sig_segv_dfl;

    case SIGABRT:
      return _sig_abrt_dfl;
  }
}

void _Lsignal_init()
{
  int i;
  static int init_ed;

  if (init_ed)
    return;

  for (i = 0; i < SIGSIZE; i++)
    _sig_eval[i] = _sig_dfl(i);

  init_ed = 1;
}
		
void _sig_err_dummy(int signo) {}	/* for macro definitions */
void _sig_dfl_dummy(int signo) {}
void _sig_ign_dummy(int signo) {}
