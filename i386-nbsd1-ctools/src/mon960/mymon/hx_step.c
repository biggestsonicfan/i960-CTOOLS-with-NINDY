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

extern char *step_string;
extern unsigned int __cpu_speed;

#define HA_ID 0x8840
#define HD_ID 0x8841
#define HT_ID 0x8842

/* boot_g0 is the value in register g0 at power-on */
void
set_step_string(int boot_g0)
{
	int proc, step;
	static char step_buffer[25];

	proc = (boot_g0 >> 12) & 0xffff;
	step = (boot_g0 >> 28) & 0xf;

	switch (proc)
	    {
	    case HA_ID:
	    	strcpy(step_buffer, "HA ");
		    break;

	    case HD_ID:
	    	strcpy(step_buffer, "HD ");
            __cpu_speed *= 2;
		    break;

	    case HT_ID:
	    	strcpy(step_buffer, "HT ");
            __cpu_speed *= 3;
		    break;

	    default:
	    	strcpy(step_buffer, "Unknown processor ");
		    return;
	    }

    strcat(step_buffer, "step number ");
    step_buffer[15] = '0' + step / 10;
    step_buffer[16] = '0' + step % 10;
    step_buffer[17] = '\0';
	step_string = step_buffer;
}
