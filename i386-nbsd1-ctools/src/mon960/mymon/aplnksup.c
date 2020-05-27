/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1995 Intel Corporation
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

#include "mon960.h"
#include "hdi_errs.h"

/*
 * $Header: /ffs/p1/dev/src/mon960/common/RCS/aplnksup.c,v 1.4 1995/09/11 19:28:13 cmorgan Exp $
 *
 * MODULE
 *     aplnksup.c
 *
 * PURPOSE
 *     Hardware independent routines designed primarily to support the 
 *     ApLink target, but which can be used in any target with an
 *     appropriate processor (i.e., don't attempt to modify the MCON
 *     register of a target with an SX/KX processor).
 */

/* -------------------------------------------------------------------- */

/*
 * FUNCTION 
 *   set_proc_mcon(unsigned int region, unsigned long value)
 *
 *   region - memory region, valid values are 0-15.  For the Jx 
 *            processor, this value will be divided by two prior to 
 *            being written in the processor's control tables.
 *
 *   value  - value to write into processor's memory control tables.
 *
 * RETURNS:
 *   OK if no errors are detected, else ERR, plus an appropriate HDI error 
 *   code via cmd_stat.
 *
 * DESCRIPTION:
 *   Writes a value to a specified memory control register and then,
 *   in the case of the Cx, causes the processor to reload that register.
 *   This function is primarily of value to users of the ApLink target,
 *   since ApLink cannot know in advance how the memory of the target under
 *   test will be configured.
 */

int
set_proc_mcon(unsigned int region, unsigned long value)
{
    if (region > 0xf)
    {
        cmd_stat = E_ARG;
        return(ERR);
    }

#if CXHXJX_CPU

#if JX_CPU
    region >>= 1;  /* Normalize region to 0-7 */
    {
        volatile unsigned int *pmcon = (unsigned int *) 0xff008600; 

        pmcon[region * 2] = value;
    }
#endif

#if HX_CPU
    {
        volatile unsigned int *pmcon = (unsigned int *) 0xff008600; 

        pmcon[region] = value;
    }
#endif

#if CX_CPU
    {
#define MCON_BASE 8

        PRCB *prcb;

        prcb = get_prcbptr();    
        prcb->cntrl_table_adr->control_reg[MCON_BASE + region] = value;
        send_sysctl(0x402 + region / 4,
                    0,
                    0);    /* processor reloads affected ctl table registers */
#undef MCON_BASE
    }
#endif   /* CX */

#endif   /* CXHXJX */

    return (OK);
}
