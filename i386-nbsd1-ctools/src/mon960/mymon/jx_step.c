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
#include "mon960.h"

extern void jx_tight_loop(unsigned int);

#define JA_ID   0x0821
#define JF_ID   0x0820
#define JFJD_ID 0x8820


/* Returns Boolean.  T -> uproc is JF, F -> uproc is JD */
static int
determine_if_jd(void)
{
#define LOOP_COUNT  4000
#define INIT_COUNT  0x1000000
#define CNTR        0

    int actual_cycles, expected_cycles;

    set_tmr(CNTR, 0);            /* Kill CPU counter                 */
    set_tcr(CNTR, 0);            /* Zero CPU counter register        */
    set_trr(CNTR, INIT_COUNT);   /* Setup auto-loaded counter value  */
    set_tmr(CNTR, 0xe);          /* Enable CPU counter               */

    jx_tight_loop(LOOP_COUNT);

    set_tmr(CNTR, 0);                         /* Kill CPU counter    */
    actual_cycles    = INIT_COUNT - get_tcr(CNTR);
    expected_cycles  = LOOP_COUNT * 4;        /* 4 cycles per loop   */
    expected_cycles += expected_cycles / 5;   /* allow for 20% error */

    return (expected_cycles >= 2 * actual_cycles);

#undef CNTR
#undef INIT_COUNT
#undef LOOP_COUNT
}



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
	    case JA_ID:
	    	strcpy(step_buffer, "JA ");
		    break;

	    case JFJD_ID:
            {
                /*
                 * Workaround JF/JD microcode feature -- both uprocs have 
                 * same IEEE device ID (uh huh).
                 */

                int is_jd = determine_if_jd();

                strcpy(step_buffer, (is_jd) ? "JD " : "JF ");
                if (is_jd)
                    __cpu_speed *= 2;
            }
		    break;

	    case JF_ID:
	    	strcpy(step_buffer, "JF ");
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
