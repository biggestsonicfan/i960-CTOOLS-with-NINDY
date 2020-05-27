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

/* qsort - quicksort
 * Copyright (c) 1984,85,86 Computer Innovations Inc, ALL RIGHTS RESERVED.
 */

#include <search.h>
#include <stdlib.h>


#define DEPTH 32			/* The size of the auxialiary array
					 * simulates a stack.  This size
					 * allows the entire address space
					 * to be sorted and is therefore
					 * safe.  See Sedgwick's "Algorithms"
					 * 1983, Addison Welsley, pp 109-111
					 */

static _swapbyte(a, b, count)
unsigned char *a, *b;
unsigned count;
{
    register temp;

    while (count--) {
        temp = *a;
        *a++ = *b;
        *b++ = temp;
    }
}

void qsort(void *bas, size_t n, size_t width, int (*cmpf)(const void *, const void *)) 
{
    unsigned j, k, pivot, low[DEPTH], high[DEPTH], temp;
    register count;
    char *base=(char *)bas;

    if (n < 2) return;			/* already sorted */

    count = 1;				/* do initialization */
    low[0] = 0;
    high[0] = n - 1;

    while (count--) {
        pivot = low[count];
        j = pivot + 1;
        n = k = high[count];

        while (j < k) {
            while (j < k && cmpf(base + j * width, base + pivot * width) < 1)
                ++j;
            while (j <= k && cmpf(base + pivot * width, base + k * width) < 1)
                --k;
            if (j < k)
                _swapbyte(base + (j++ * width), base + (k-- * width), width);
        }

        if (cmpf(base + pivot * width, base + k * width) > 0)
            _swapbyte(base + pivot * width, base + k * width, width);
        if (k > pivot)
            --k;
        if (k > pivot && n > j && (k - pivot < n - j)) {
	    temp = k;
	    k = n;
	    n = temp;
	    temp = pivot;
	    pivot = j;
	    j = temp;

        }
        if (k > pivot) {
            low[count] = pivot;
            high[count++] = k;
        }
        if (n > j) {
            low[count] = j;
            high[count++] = n;
        }
    }
}
