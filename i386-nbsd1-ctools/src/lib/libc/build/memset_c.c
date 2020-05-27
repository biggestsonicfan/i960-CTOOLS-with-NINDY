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

#include <string.h>

/*
 * memset() and bzero()
 *
 * [atw] written in a (moderately) efficient fashion using word
 * moves where possible.
 * If sufficiently motivated, could be rewritten for the 960 to
 * use long or quad-word instructions.
 */

#define MIN(a,b)	((a)<(b)?(a):(b))

typedef struct { long a, b, c, d; } QWORD ;
typedef struct { long a, b; } DWORD ;

void *
(memset)(void* b, int val, size_t n)
{
    long q, head;
    QWORD qword;
    void * p;

    char* char_b = b;
    QWORD* qword_b;
    
    p = b;
    if(head = ((long)char_b) & 0xf) /* [atw] check for unaligned pointer */
    {
      head = MIN(head, n);
      n -= head;
      while(head--)
	*char_b++ = (char)val;

    }

    if (n == 0) return;

    q = (n >> 4); /* # of quad words */
    n &= 0xf;     /* n == # of residual bytes */

    /*
     * construct a word of the desired value.
     */
    
    qword_b = (QWORD*)char_b;
    if (q)
    {
      qword.a = (unsigned char)val;
      qword.a |= (qword.a<<8);
      qword.a |= (qword.a<<16);
      qword.b = qword.a;
      qword.c = qword.a;
      qword.d = qword.b;
      while(q--)
	*qword_b++ = qword;
    }

    char_b = (char*)qword_b;

    while(n--)
	*char_b++ = (char)val;

    return p;
}

