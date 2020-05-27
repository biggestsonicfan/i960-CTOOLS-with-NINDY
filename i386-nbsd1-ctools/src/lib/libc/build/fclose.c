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

/* fclose - close a file
 * Copyright (c) 1984,85,86,87 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <stdio.h>
#include <stdlib.h>
#include <std.h>

int fclose(FILE *stream)
{
    FILE **ptr;

    /*
     * Write any buffered data (if necessary) to the file and call the
     * target dependent close routine:
     */
    fflush(stream);
    close(stream->_fd);
    /*
     * Release any resources held by the stream:
     */
    if (stream->_temp_name)
        remove(stream->_temp_name);
    if (stream->_base && !(stream->_flag & _IOMYBUF))
        free(stream->_base);
    if (stream->_sem)
        _semaphore_delete(&stream->_sem);

    /*
     * If the stream being closed is one of the standard streams,
     * then no more processing is required:
     */
    if (stream == stdin || stream == stdout || stream == stderr)
        return 0;

    /*
     * Search through the linked list of open streams, remove it
     * from the list, and release its memory:
     */
    _semaphore_wait(&_EXIT_PTR->open_stream_sem);
    ptr = &_EXIT_PTR->open_stream_list;
    while (*ptr) {
        if (*ptr == stream) {		/* found the stream */
            *ptr = stream->_next_stream;
            free(stream);		/* free current stream */
            break;
        }
        ptr = &(*ptr)->_next_stream;	/* advance to next stream */
    }
    _semaphore_signal(&_EXIT_PTR->open_stream_sem);

    return 0;
}
