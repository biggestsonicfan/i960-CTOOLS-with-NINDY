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

/* setvbuf - set the size of an I/O buffer
 * Copyright (c) 1985,86,87 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>

int setvbuf(FILE *stream, char *buf, int type, size_t size)
{
    int flag = 0;
    int buf_size;
    unsigned char *base = NULL;
    
    /*
     * This function can be invoked only if it is the first operation
     * to stream after it has been opened:
     */
    if (stream->_cnt || stream->_base)
        return -1;

    if (type == _IOFBF) {		/* I/O will be fully buffered */
        flag &= ~_IOLBF & ~_IONBF;
        flag |= _IOFBF;
        buf_size = BUFSIZ;

    } else if (type == _IOLBF) {	/* I/O will be line buffered */
        flag &= ~_IONBF & ~_IOFBF;
        flag |= _IOLBF;
        buf_size = BUFSIZ;

    } else if (type == _IONBF) {	/* I/O will be unbuffered */
        flag &= ~_IOFBF & ~_IOLBF;
        flag |= _IONBF;
        buf_size = 1;

    } else {
        return -1;
    }

    if (buf) {
        base = (unsigned char *)buf;
        flag |= _IOMYBUF;
    
        if (size > INT_MAX) {
            errno = E2BIG;
            return -1;
        }
        buf_size = size;
    }

    /*
     * The options were correctly specified.
     * Update the File structure:
     */
    _semaphore_wait(&stream->_sem);
    stream->_flag |= flag;
    stream->_size  = buf_size;
    if (base)  {
    	stream->_ptr = base;
    	stream->_base = base;
    }
    _semaphore_signal(&stream->_sem);

    return 0;
}
