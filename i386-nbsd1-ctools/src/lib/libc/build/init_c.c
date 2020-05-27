/*******************************************************************************
 * 
 * Copyright (c) 1993 Intel Corporation
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

/*
 */

init_c()
{
	_LL_init();
	_HL_init();
	_arg_init();	/* Found in Low-Level Lib for set up of argv,argc */
}

extern void atexit (void (*) (void));
static void do_global_ctors();
static void do_global_dtors();

_HL_init()
{
	/* Initialize thread first so that errno can be
	 * referenced by remaining initializations.
	 */
	_thread_init();	/* initialize thread data */
	_exit_init();	/* initialize exit structure */
	_stdio_init();	/* initialize stream data */
	do_global_ctors();	/* initialize static c++ objects */
}

static void
do_global_ctors()
{
  typedef (*pfunc)();
  extern pfunc _Bctors[];
  extern pfunc _Ectors[];
  pfunc *p;
  for (p = _Bctors; p < _Ectors; p++)
    {
      (*p)();
    }
  atexit(do_global_dtors);
}

static void
do_global_dtors ()
{
  typedef (*pfunc)();
  extern pfunc _Bdtors[];
  extern pfunc _Edtors[];
  pfunc *p;
  for (p = _Bdtors; p < _Edtors; p++)
    {
      (*p)();
    }
}						

