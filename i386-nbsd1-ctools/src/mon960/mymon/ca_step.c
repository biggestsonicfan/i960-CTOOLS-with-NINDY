/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994 Intel Corporation
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
/*)ce*/

#include <string.h>
#include "common.h"

extern void enable_dcache();
extern char *step_string;

#define CA_ID (('C' << 8) | 'A')
#define CF_ID (('C' << 8) | 'F')

/* boot_g0 is the value in register g0 at power-on */
void
set_step_string(int boot_g0)
{
	int proc, step;
	static char step_buffer[25];

	proc = (boot_g0 >> 8) & 0xffff;
	step = boot_g0 & 0xff;

	/* Sanity check */
	if (boot_g0 < 0 || step >= 100)
	    return;

	switch (proc)
	{
	    case CA_ID:
		    strcpy(step_buffer, "CA ");
		    break;

	    case CF_ID:
		    strcpy(step_buffer, "CF ");
            enable_dcache();                 /* Enable data cache on CF */
		    break;

	    default:
		return;
	}

    strcpy(step_buffer+3, "step number ");
    step_buffer[15] = '0' + step / 10;
    step_buffer[16] = '0' + step % 10;
    step_buffer[17] = '\0';
	step_string = step_buffer;
}
