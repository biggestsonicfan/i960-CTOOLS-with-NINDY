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

/* _filbuf - fill an I/O buffer
 * Copyright (c) 1984,85,86 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <reent.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <std.h>

int _filbuf(FILE *s)
{
    int char_read	= EOF;
    FILE *f;				/* used for check for LBF io */
    struct _exit *e_ptr = _EXIT_PTR;	/* used for check for LBF io */

    if (&s->_sem)
        _semaphore_wait(&s->_sem);
    /*
     * Check if the file is open:
     */
    if (!(s->_flag & (_IOREAD | _IOWRT | _IORW))) {
        errno = EBADF;
	goto ret;
    }

    if (!s->_base) {
        /*
         * No buffer has yet been allocated to this stream.
         * Allocate dynamic memory for a buffer:
         */
        s->_base = malloc(s->_size);
        if (!s->_base) {
            errno = ENOMEM;
	    goto ret;
        }
    } else if (((int)s->_cnt) > 0) {
        /*
         * There is still an un-read character in the buffer.
         * Return this character:
         */
        --s->_cnt;
        char_read = *s->_ptr++;
        goto ret;
    }

    /*
     * Check for and flush and Line Buffered I/O
     */
    _semaphore_wait(&e_ptr->open_stream_sem);
    for (f = e_ptr->open_stream_list; f != NULL; f = f->_next_stream) {
         if (f->_flag & _IOLBF) {
             _semaphore_signal(&e_ptr->open_stream_sem);
             fflush(f);
             _semaphore_wait(&e_ptr->open_stream_sem);
         }
    }
    _semaphore_signal(&e_ptr->open_stream_sem);

    /*
     * Fill the buffer with the next block from the file:
     */
    s->_flag &= ~_IODIRTY;
    if (s->_size) {
        s->_cnt = read(s->_fd, (char *)(s->_ptr = s->_base), s->_size);
    } else {
	/*
	 * The buffer does not have a valid size, so a read cannot be done.
	 * (Note that a value of zero is set by the sscanf() function)
	 */
    	s->_cnt = 0;
    }
    if (s->_cnt < 0) {		/* a negative return indicates an error */
	s->_flag |= _IOERR;
    } else if (!s->_cnt) {	/* a zero return indicated end-of-file */
        s->_flag |= _IOEOF;
    } else {			/* valid data was read from the file */
        s->_flag &= ~_IOEOF;
        s->_cnt--;
        char_read = *s->_ptr++;
    }

ret:
    if (&s->_sem)
        _semaphore_signal(&s->_sem);
    return char_read;
}
