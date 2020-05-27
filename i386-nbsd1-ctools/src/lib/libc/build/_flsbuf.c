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

/* _flsbuf - flush the buffer
 * Copyright (c) 1984,85,86 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <stdio.h>
#include <errno.h>
#include <std.h>
#include <stdlib.h>

int _flsbuf(unsigned char c, FILE *s)
{
    int count;
    char tmp[1];

    if (&s->_sem)
        _semaphore_wait(&s->_sem);

    if (s->_size <= 1) {
	/*
	 * This stream is unbuffered:  write the character
	 * out to the file.
	 */
        s->_cnt = 0;
        tmp[0] = c;
        if (write(s->_fd, tmp, 1) != 1)
            goto err;
	goto ret;
    }

    if (!(s->_base)) {
        /*
         * No buffer has yet been allocated to this stream.
         * Allocate dynamic memory for a buffer:
         */
        s->_ptr = s->_base = malloc(s->_size);
        s->_cnt = s->_size;
        if (!s->_base) {
            errno = ENOMEM;
            goto err;
        }
    }

    if (s->_ptr == s->_base + s->_size) {
	/*
	 * The buffer may be full, if so write it out to the file:
	 */
        if (write(s->_fd, (char *)s->_base, s->_size) != s->_size) {
            goto err;
        }
        s->_ptr = s->_base;
        s->_cnt = s->_size;
        s->_flag &= ~_IODIRTY;
    }

    if (s->_flag & _IOWRT || s->_flag & _IORW) {
	/*
	 * If the file has write access, then the character can be
	 * copied to the buffer:
	 */
        s->_flag |= _IODIRTY; 
        *s->_ptr++ = c;
        s->_cnt--;
    }

    if ((s->_flag & (_IOLBF | _IODIRTY)) == (_IOLBF | _IODIRTY)) {
	/*
	 * If the file is line buffered, then we need to check for '\n'
	 * to see if the buffer needs to be written:
	 */
       if (c == '\n') {
           count = s->_ptr - s->_base;
           if (write(s->_fd, (char *)s->_base, count) != count) { 
               goto err;
           }
           s->_ptr = s->_base;
           s->_flag &= ~_IODIRTY;
       }
       s->_cnt = 0;
    } 

ret:
    if (&s->_sem)
        _semaphore_signal(&s->_sem);
    return (int)c;

err:
    s->_flag |= _IOERR;
    if (&s->_sem)
        _semaphore_signal(&s->_sem);
    return (int)EOF;
}
