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

/* freopen - close the stream and attempt to open a new one.
 * Copyright (c) 1984,85,86,87 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <std.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stat.h>

extern unsigned _Lmodeparse(const char *);

FILE *freopen(const char *filename, const char *type, FILE *stream)
{
    unsigned mode;

    if(stream == (FILE *)NULL) {
	/*
	 * There is no open stream to associate with 'filename':  return
	 * to the caller with the NULL pointer.
	 */
        return NULL;
    }

    fflush(stream);
    close(stream->_fd);
    if (stream->_temp_name)
        remove(stream->_temp_name);

    if((mode = _Lmodeparse(type)) == 0xFFFFu) {
        errno = EINVAL;
        return NULL;
    }

    _semaphore_wait(&stream->_sem);
    stream->_cnt = 0;
    stream->_flag = 0;
    if (mode & (unsigned)O_RDWR)
        stream->_flag |= _IORW;
    else if (mode & (unsigned)O_WRONLY)
        stream->_flag |= _IOWRT;
    else
        stream->_flag |= _IOREAD;
    stream->_fd = open((char *)filename, mode,
                       (unsigned)(S_IRUSR | S_IWUSR |
                                  S_IRGRP | S_IWGRP |
                                  S_IROTH | S_IWOTH));

    if (stream->_fd == -1)
        return NULL;
    else {
        stream->_size = (isatty(stream->_fd)) ? 1 : BUFSIZ;
        _semaphore_signal(&stream->_sem);

        return stream;
    }
}
