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

/* fdopen - "I'll give you a stream, you give me a fd."
 * Copyright (c) 1986 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <stdio.h>
#include <stdlib.h>
#include <std.h>

#define RWBITS	_IOREAD | _IOWRT | _IORW
#define TRUE	1
#define FALSE	0

FILE *fdopen(int fd, const char *mode)
{
    register int ascmode = TRUE;	/* ascii/binary mode; assume open ascii */
    int rwmode = 0;			/* read/write/update flag */
    int first = 0;			/* because of rw and wr stupidity */
    register unsigned mode_word;
    FILE *ptr;

    while (*mode) {
        switch (*mode++) {
            case 'b':
                ascmode = FALSE;	/* open in binary mode */
                continue;
            case 'r':
                if (!first) first = 1;	/* open not create */
                rwmode |= 1;
                continue;
            case 'a':			/* a+ equ ar */
                if (!first) first = 3;	/* open at end, create if reqd */
            case 'w':			/* w+ equ wr */
                if (!first) first = 2;	/* create the file */
                rwmode |= 2;
                continue;
            case '+':
                if (first) {		/* allow read & write */
                    rwmode |= 3;
                    continue;
                }
            default:			/* bad file mode */
                return NULL;
        }
    }

    mode_word = rwmode;

    /* allocate space for a new stream */
    ptr = (FILE *)malloc(sizeof(FILE));

    /* fill in information for the stream */
    ptr->_ptr = NULL;
    ptr->_cnt = 0;
    ptr->_base = NULL;
    ptr->_flag = mode_word;
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
