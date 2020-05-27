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

/* exit - exit for DOS 2.0 and above
 * Copyright (c) 1984,85,86,87,88 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <stdio.h>
#include <stdlib.h>
#include <reent.h>

void exit(int code)
{
    struct _exit	*e_ptr = _EXIT_PTR;
    struct _thread	*t_ptr = _THREAD_PTR;
    register int i;
    FILE *f;
    
    _semaphore_wait(&e_ptr->exit_handler_sem);
    while (1) {
        i = e_ptr->exit_handler_count - 1;
        if (i < 0)
          break;
        e_ptr->exit_handler_count = i;
        _semaphore_signal(&e_ptr->exit_handler_sem);
        (*e_ptr->exit_handler_list[i])(); /* execute "atexit" functions */
        _semaphore_wait(&e_ptr->exit_handler_sem);
    }
    _semaphore_signal(&e_ptr->exit_handler_sem);

    /* close all open streams */
    while (1) {
        _semaphore_wait(&e_ptr->open_stream_sem);
        f = e_ptr->open_stream_list;
        if (f == NULL)
          break;
        e_ptr->open_stream_list = f->_next_stream;
        _semaphore_signal(&e_ptr->open_stream_sem);
        fclose(f);
    }

    c_term(code);
    _exit(code);			/* quit */
}
