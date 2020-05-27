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

/* fgetpos - Store the current file postition pointer
 */

#include <stdio.h>

/**************************************************************
**
**  4.9.9.1  FGETPOS
**
**  The fgetpos function stores the current value of the file
**  position indicator for the stream pointed to by "stream",
**  in the object pointed to by "pos".  The value stored is
**  used by fsetpos() to get back to the current location in
**  the stream.
**
***************************************************************/

int fgetpos(FILE *stream, fpos_t *pos)
{
    long int retval;

	/*******************************************************
	**  Call ftell() to get the current position information.
	**  ftell() returns the correct position or a -1, if there
	**  is an error.
	*********************************************************/
	retval = ftell(stream);
	if (retval != -1L) {

		/**************************************************
		**  There was no error, so pass the position info
		**  back to the caller, and set the "all clear"
		**  return value.
		***************************************************/
		*pos = (fpos_t)retval;
		retval = 0L;
	}

	/**********************************************************
	**  Return back to the caller here.  If there was an error,
	**  we will be returning a non-zero (-1), otherwise a zero
	**  as per the spec.
	***********************************************************/
	return((int)retval);

}
