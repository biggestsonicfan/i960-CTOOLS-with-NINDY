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


#include <stdio.h>


/* static	unsigned int qaz[100]; */
static void
spinwait(int n)
{
    volatile int dummy;
    while (n--)
        dummy = 0;
}



main()
{
	unsigned int	overhead, start, stop, time;
	int	i;
#define NUMBER 12
	unsigned int	loops[NUMBER] =
			{ 1,
			  5,
			  10,
			  20,
			  100,
			  1000,
			  10000,
			  100000,
			  1000000,
			  10000000,
			  100000000,
			  1000000000
			};

	overhead = init_bentime(0);

    printf("Overhead is %u\n", overhead);

    for (i=0; i<NUMBER-3; i++)
    {
	start = bentime();

	spinwait(loops[i]);

	stop  = bentime();

	/*Calculate the time taken for the timed call, in microseconds.*/
	time = stop - start - overhead;
	printf("Time for %i loops was\t %u microseconds.\n", loops[i], time);

    }

	term_bentime();
}

