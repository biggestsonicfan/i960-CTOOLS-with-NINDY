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

/* fopen - open a stream
 * Copyright (c) 1984,85,86,87 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <std.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stat.h>

unsigned _Lmodeparse(const char *openmode)
{
    register unsigned mode = 0;
    register int state;

    switch (*openmode++) {
        case 'r': mode |= (unsigned)O_RDONLY;
                  state = 0;
                  break;
        case 'w': mode |= (unsigned)(O_WRONLY | O_CREAT | O_TRUNC);
                  state = 1;
                  break;
        case 'a': mode |= (unsigned)(O_APPEND | O_WRONLY | O_CREAT);
                  state = 1;
                  break;
        default:  return 0xFFFFu;
    }

    while (*openmode) {
        switch (*openmode++) {
            case 'r': if (!state) return 0xFFFFu;
                      mode &= (unsigned)(~O_WRONLY);
                      mode |= (unsigned)O_RDWR;
                      break;
            case 'w': if (state) return 0xFFFFu;
                      mode &= (unsigned)(~O_RDONLY);
                      mode |= (unsigned)O_RDWR;
                      break;
            case '+': mode &= (unsigned)(~(O_RDONLY | O_WRONLY));
                      mode |= (unsigned)O_RDWR;
                      break;
            case 'b': mode |= (unsigned)O_BINARY;
                      break;
            case 't': mode &= (unsigned)(~O_BINARY);
                      break;
            default:  return 0xFFFFu;
        }
    }
    return mode;
}

FILE *fopen(const char *filename, const char *openmode)
{
    int fd;
    unsigned mode;
    FILE *ptr;

    if ((mode = _Lmodeparse(openmode)) == 0xFFFFu) {
        errno = EINVAL;
        return NULL;
    }

    /* open a file */
    fd = open((char *)filename, mode,
              (unsigned)(S_IRUSR | S_IWUSR |
                         S_IRGRP | S_IWGRP |
                         S_IROTH | S_IWOTH));

    if (fd == -1)	{		/* file opened unsuccessfully */
		errno = ENOENT;
		return NULL;
	 }
    else {				/* file opened successfully */
        /* create a new stream for the stream list */
        if (!(ptr = (FILE *)malloc(sizeof(FILE)))) {
	    close(fd);
	    errno = ENOMEM;
	    return NULL;
	}

        /* fill in information for this stream */
        ptr->_ptr = NULL;
        ptr->_cnt = 0;
        ptr->_base = NULL;
        ptr->_flag = 0;
        if (mode & (unsigned)O_RDWR)
            ptr->_flag |= _IORW;
        else if (mode & (unsigned)O_WRONLY)
            ptr->_flag |= _IOWRT;
        else
            ptr->_flag |= _IOREAD;
        ptr->_fd = fd;
        ptr->_size = (isatty(fd)) ? 1 : BUFSIZ;
        ptr->_temp_name = NULL;
        _semaphore_init(&ptr->_sem);
  
        _semaphore_wait(&_EXIT_PTR->open_stream_sem);
        ptr->_next_stream = _EXIT_PTR->open_stream_list; /* push this stream on stream stack */
        _EXIT_PTR->open_stream_list = ptr; /* set top of stream stack */
        _semaphore_signal(&_EXIT_PTR->open_stream_sem);
  
        return ptr;
    }
}
