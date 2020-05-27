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

/* fsetpos - Set the file postition pointer
 */

#include <stdio.h>
#include <errno.h>

/**************************************************************
**
**  4.9.9.3  FSETPOS
**
**  The fsetpos function sets the file position pointer for the 
**  stream pointed to by "stream", according to the object 
**  pointed to by "pos", which is a value obtained from a 
**  previous call to fgetpos() on the same stream. 
**
***************************************************************/

int fsetpos(FILE *stream, const fpos_t *pos)
{

	/*******************************************************
	**  Call fseek() to use the position information.
	**  fseek() returns non-zero only if the function cannot
	**  be satisfied.
	*********************************************************/
	if (!fseek(stream, *pos, SEEK_SET)) return(0);

	/*********************************************************
	**  If there was an error processing the seek, set the
	**  errno with invalid operation or value and return a non-zero.
	************************************************************/
	errno = EINVAL;
	return (-1);
}
