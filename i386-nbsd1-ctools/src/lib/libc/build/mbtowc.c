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

/* mbtowc.c - converts the next one or more characters in a string to a
 *            wide character
 */

#include <stdlib.h>

int mbtowc(wchar_t *pwc, const char *s, size_t n)
{
    int size = sizeof(wchar_t);

	/* return 0 for NULL string */
	if (!s) {
		if (pwc)
			*pwc = (wchar_t) 0;
		return 0;
	}

	/**************************************************************
	**  Check to see if the multibyte char is greater than the 
	**  maximum number of characters available for the multibyte 
	**  character set.  If so, replace size with the max value.
	***************************************************************/
	if (MB_CUR_MAX < sizeof(wchar_t)) size = MB_CUR_MAX;

	if (*s)
	{
		if (pwc) {
			/*****************************************************
			**  Now make a trivial test to see if this multibyte 
			**  character is within the size_t boundary.  If not,
			**  return -1, else copy the multibyte char to pwc.  
			******************************************************/
			if (sizeof(wchar_t) <= n) 
				*pwc = (wchar_t) *((wchar_t *)s);
			else return(-1);
		}
		return (size);
	}
	else
		return 0;
}
