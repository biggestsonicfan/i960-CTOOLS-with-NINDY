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

/* ungetc - unget a char from the input stream
 * Copyright (c) 1984,85,86,87 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int ungetc(int cc, FILE *stream)
{
    if (cc == EOF)
	/*
	 * If the input is EOF, then return EOF and leave the input
	 * stream unchanged
	 */
        return (EOF);

    if (stream->_ptr == stream->_base) {
	/*
	 * Special case:  either the stream buffer has not yet been
	 * allocated or the buffer pointer is pointing to its start.
	 */
	if (!stream->_base) {
	    /*
	     * Allocate a buffer for this stream:
	     */
	    stream->_base = malloc(stream->_size);
	    if (!stream->_base) {
		errno = ENOMEM;
		return (EOF);
	    }
	    stream->_ptr = stream->_base;
	}
        /*
         * Clear the end-of-file indicator and write the character into
         * the first character position in the buffer:
         */
        if (&stream->_sem)
            _semaphore_wait(&stream->_sem);
        stream->_flag &= (~_IOEOF);
        if (stream->_cnt == 0)
	    stream->_cnt = 1;
        *(stream->_ptr) = (unsigned char)cc;
        if (&stream->_sem)
            _semaphore_signal(&stream->_sem);
    } else {
    	/*
    	 * Clear the end-of-file indicator and write the character into
    	 * the previous character position in the buffer:
    	 */
        if (&stream->_sem)
            _semaphore_wait(&stream->_sem);
        stream->_flag &= (~_IOEOF);
        stream->_cnt++;
        *(--stream->_ptr) = (unsigned char)cc;
        if (&stream->_sem)
            _semaphore_signal(&stream->_sem);
    }
    return *(stream->_ptr);
}
