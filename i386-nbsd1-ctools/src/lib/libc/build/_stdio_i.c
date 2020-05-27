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

/* _stdio_init() - initialize the standard streams
 */

#include <errno.h>
#include <std.h>
#include <stdio.h>
#include <stdlib.h>
#include <reent.h>

/**********************************************************************
*                             _stdio_init                             *
**********************************************************************/

int _stdio_init(void)
{
    FILE *ptr;
    struct _stdio *st_ptr;              /* pointer to struct _stdio */

    /*
     * allocate memory and initialize the _stdio structure:
     */
    if ((st_ptr = _stdio_create(sizeof(struct _stdio))) ==  NULL)
	return errno;

    /* initialize _stdin */
    st_ptr->_stdin._ptr  = malloc(BUFSIZ);
    st_ptr->_stdin._cnt  = 0;
    st_ptr->_stdin._base = st_ptr->_stdin._ptr;
    st_ptr->_stdin._flag = _IOREAD | _IOFBF;
    st_ptr->_stdin._fd   = _stdio_stdopen(0);
    st_ptr->_stdin._size = BUFSIZ;
    st_ptr->_stdin._temp_name = NULL;   /* indicates is it a temporary file */
    _semaphore_init(&st_ptr->_stdin._sem);

    /* initialize _stdout */
    st_ptr->_stdout._ptr  = NULL;
    st_ptr->_stdout._cnt  = 0;
    st_ptr->_stdout._base = NULL;
    st_ptr->_stdout._fd   = _stdio_stdopen(1);
    if (isatty(st_ptr->_stdout._fd))
    	st_ptr->_stdout._flag  = _IOWRT | _IOLBF;
    else
    	st_ptr->_stdout._flag  = _IOWRT | _IOFBF;
    st_ptr->_stdout._size      = BUFSIZ;
    st_ptr->_stdout._temp_name = NULL;  /* indicates is it a temporary file */
    _semaphore_init(&st_ptr->_stdout._sem);

    /* initialize _stderr */
    st_ptr->_stderr._ptr  = NULL;
    st_ptr->_stderr._cnt  = 0;
    st_ptr->_stderr._base = NULL;
    st_ptr->_stderr._flag = _IOWRT | _IONBF;
    st_ptr->_stderr._fd   = _stdio_stdopen(2);
    st_ptr->_stderr._size = (isatty(st_ptr->_stderr._fd)) ? 0 : BUFSIZ;
    st_ptr->_stderr._temp_name = NULL;  /* indicates is it a temporary file */
    _semaphore_init(&st_ptr->_stderr._sem);

    /*
     * initialize the linked list of open stream:
     */
    _semaphore_wait(&_EXIT_PTR->open_stream_sem);
    st_ptr->_stdin._next_stream   = _EXIT_PTR->open_stream_list;
    st_ptr->_stdout._next_stream  = stdin;
    st_ptr->_stderr._next_stream  = stdout;
    _EXIT_PTR->open_stream_list = stderr;
    _semaphore_signal(&_EXIT_PTR->open_stream_sem);
    return _INIT_OK;
}
