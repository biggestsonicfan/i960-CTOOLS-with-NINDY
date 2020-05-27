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

/* strxfrm - Transform and replace string 
*/

#include <string.h>

/**************************************************************
**
**  4.11.4.5  STRXFRM()
**
**  The strxfrm() function transforms the string pointed to by
**  s2, and stores the result into s1.  The transformation is
**  such that if the strcmp() function is applied on the two
**  transformed strings it returns the equivalent value that
**  the strcoll() function returns on the non-transformed
**  strings.
**
**  NOTE:
**    In reality, however, we only support the 'C' locale at 
**    this time.  Thus, the strcoll() is simply passed through
**    to the strcmp() function, which provides equivalent functionality
**    for the 'C' locale.  Thus, no transformation is necessary;
**    only the correct returns are calculated.
**
***************************************************************/

size_t strxfrm(char *s1, const char *s2, size_t n)
{
    size_t count;

	/*****************************************************************
	**  Loop thru and copy the "transformation" (see note above).
	**  Get a count and return it to the caller.
	*******************************************************************/
	count = strlen(s2);
	if (count < n) {
		strcpy(s1, s2);
	}
	return(count);
}
