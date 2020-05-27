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

/* bsearch - binary search
 * Copyright (c) 1986 Computer Innovations Inc, ALL RIGHTS RESERVED.
 *
 * Binary search routine -- searches a sorted array pointed to by base
 * for a match of key.  compare is a user supplied function that compares
 * two objects and returns 1 , 0 , -1 if ob1 is greater, less or equal
 * to ob2.  Function also requires size of array and width of an element.
 */

#include <stdio.h>
#include <stdlib.h>


#if 0
static int compare(cmpf, a, b)
int (*cmpf)(const void *, const void *);
const void *a;
const void *b;
{
    int dummy[2];			/* so compiler will clean up stack */
					/* correctly whether cmpf() is FPL */
					/* or VPL */

    return cmpf(a, b);
}
#endif

void *bsearch(const void *key, const void *base, size_t num, size_t width,
              int (*cmpf)(const void *, const void *))
{
    register int mid, high;
    int low, ret;
    char *look_ptr;

    low = 0;				/* Start and */
    high = num - 1;			/* End of search */
    
    while (low <= high) {  
        mid = (high + low) / 2;
        look_ptr = (char *)base + (width * mid); /* Mid point passed to comp */

        if ((ret = cmpf(key, look_ptr)) < 0)
            high = mid - 1;
        else if (ret > 0)
            low = mid + 1;
        else
            return look_ptr;		/* Point to matching element */
    }
    return NULL;			/* Couldn't find a match */
}
