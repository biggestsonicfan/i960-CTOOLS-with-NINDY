/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994, 1995 Intel Corporation
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
/*
 * Host side of logical communication layer.  These commands are used
 * by sdm to implement user commands.
 */
/* "$Header: /ffs/p1/dev/src/hdil/common/RCS/regs.c,v 1.3 1995/09/13 00:16:41 cmorgan Exp $$Locker:  $ */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "private.h"
#include "hdi_com.h"
#include "dbg_mon.h"

static int get_regs();
static int get_fp_regs();
#ifdef __STDC__
static int do_get_regs(const unsigned long mask[2]);
#else /* __STDC__ */
static int do_get_regs();
#endif /* __STDC__ */

/* INVALID must be 0 so the static flag arrays are properly initialized. */
enum register_state { INVALID = 0, VALID, DIRTY };
static enum register_state reg_state[NUM_REGS];
static UREG local_regs;

typedef struct {
    FPREG fp80;
    FPREG fp64;
} LOCAL_FPREG;

enum fp_register_state { FPINVALID = 0, FPDIRTY = 4,
	VALID80 = FP_80BIT, VALID64 = FP_64BIT, VALID_BOTH = VALID80 | VALID64,
	DIRTY80 = FPDIRTY | VALID80, DIRTY64 = FPDIRTY | VALID64 };
static enum fp_register_state fp_reg_state[NUM_FP_REGS];
static LOCAL_FPREG local_fp_regs[NUM_FP_REGS];



static int 
validate_sfr(reg_number)
int reg_number;
{
    if (_hdi_arch != ARCH_CA && _hdi_arch != ARCH_HX)
    {
        /* this uproc has no SFR's */

        hdi_cmd_stat = E_ARCH;
        return(FALSE);
    }
    if (_hdi_arch == ARCH_CA && reg_number > REG_SF2)
    {
        /* Cx has but 3 SFR's */

        hdi_cmd_stat = E_ARG;
        return(FALSE);
    }
    return (TRUE);
}



int
hdi_reg_get(reg_number, regval)
int reg_number;
REG *regval;
{
	if (reg_number < 0 || reg_number >= NUM_REGS)
	{
	    hdi_cmd_stat = E_ARG;
	    return(ERR);
	}

    if (reg_number >= REG_SF0 && (! validate_sfr(reg_number)))
        return (ERR);

	if (_hdi_running)
	{
	    hdi_cmd_stat = E_RUNNING;
	    return(ERR);
	}

	if (reg_state[reg_number] == INVALID)
	    if (get_regs() != OK)
	        return(ERR);

	*regval = local_regs[reg_number];
	return(OK);
}

int
hdi_reg_put(reg_number, regval)
int reg_number;
REG regval;
{
	if (reg_number < 0 || reg_number >= NUM_REGS)
	{
	    hdi_cmd_stat = E_ARG;
	    return(ERR);
	}

    if (reg_number >= REG_SF0 && (! validate_sfr(reg_number)))
        return (ERR);

	if (_hdi_running)
	{
	    hdi_cmd_stat = E_RUNNING;
	    return(ERR);
	}

	if (reg_number == REG_IP && (regval & 0x03) != 0)
	{
	    hdi_cmd_stat = E_ALIGN;
	    return(ERR);
	}

	if (reg_state[reg_number] == INVALID || regval != local_regs[reg_number])
	{
	    local_regs[reg_number] = regval;
	    reg_state[reg_number] = DIRTY;
	}

	return(OK);
}


int
hdi_regfp_get(reg_number, format, regval)
int reg_number;
int format;
FPREG *regval;
{
	if (_hdi_arch != ARCH_KB && _hdi_arch != ARCH_SB)
	{
	    hdi_cmd_stat = E_ARCH;
	    return(ERR);
	}

	if (reg_number < 0 || reg_number >= NUM_FP_REGS)
	{
	    hdi_cmd_stat = E_ARG;
	    return(ERR);
	}

	if (_hdi_running)
	{
	    hdi_cmd_stat = E_RUNNING;
	    return(ERR);
	}

	if (((int)fp_reg_state[reg_number] & format) == (int)FPINVALID)
	    if (get_fp_regs() != OK)
	        return(ERR);

	if (format == FP_80BIT)
	    *regval = local_fp_regs[reg_number].fp80;
	else
	    *regval = local_fp_regs[reg_number].fp64;

	return(OK);
}

int
hdi_regfp_put(reg_number, format, regval)
int reg_number;
int format;
const FPREG *regval;
{
	if (_hdi_arch != ARCH_KB && _hdi_arch != ARCH_SB)
	{
	    hdi_cmd_stat = E_ARCH;
	    return(ERR);
	}

	if (reg_number < 0 || reg_number >= NUM_FP_REGS)
	{
	    hdi_cmd_stat = E_ARG;
	    return(ERR);
	}

	if (_hdi_running)
	{
	    hdi_cmd_stat = E_RUNNING;
	    return(ERR);
	}

	if (format == FP_80BIT)
	{
	    local_fp_regs[reg_number].fp80 = *regval;
	    fp_reg_state[reg_number] = DIRTY80;
	}
	else
	{
	    local_fp_regs[reg_number].fp64 = *regval;
	    local_fp_regs[reg_number].fp64.fp64.flags = 0;
	    fp_reg_state[reg_number] = DIRTY64;
	}

	return(OK);
}


int
hdi_regs_get(regs)
UREG regs;
{
	if (_hdi_running)
	{
	    hdi_cmd_stat = E_RUNNING;
	    return(ERR);
	}

	if (get_regs() != OK)
	    return(ERR);

	memcpy(regs, local_regs, sizeof(UREG));
	return(OK);
}

int
hdi_regs_put(regs)
const UREG regs;
{
	int i;

	if (_hdi_running)
	{
	    hdi_cmd_stat = E_RUNNING;
	    return(ERR);
	}

	if ((regs[REG_IP] & 0x03) != 0)
	{
	    hdi_cmd_stat = E_ALIGN;
	    return(ERR);
	}

	memcpy(local_regs, regs, sizeof(UREG));
	for (i = 0; i < NUM_REGS; i++)
	    reg_state[i] = DIRTY;

	return(OK);
}

void
_hdi_set_ip_fp(ip, fp)
REG ip;
REG fp;
{
	local_regs[REG_IP] = ip; reg_state[REG_IP] = VALID;
	local_regs[REG_FP] = fp; reg_state[REG_FP] = VALID;
}

void
_hdi_invalidate_registers()
{
	int i;

	for (i=0; i < NUM_REGS; i++) 
	    reg_state[i] = INVALID;
	for (i=0; i < NUM_FP_REGS; i++) 
	    fp_reg_state[i] = FPINVALID;
}


static int
get_regs()
{
	int i;
	unsigned long mask[2];

	mask[0] = mask[1] = 0;
	for (i = 0; i < NUM_REGS; i++)
	    if (reg_state[i] == INVALID)
		if (i < 32)
	            mask[0] |= 1L << i;
		else
		    mask[1] |= 1L << (i-32);

	i = do_get_regs(mask);

	return(i);
}

static int
get_fp_regs()
{
	int i;
	unsigned long mask[2];

	for (i = 0; i < NUM_FP_REGS; i++)
	    if (((int)fp_reg_state[i] & (int)FPDIRTY) != 0)
 	        _hdi_put_regs();

	mask[0] = mask[1] = 0;
	for (i = 0; i < NUM_FP_REGS; i++)
	    if (fp_reg_state[i] != VALID_BOTH)
		if (NUM_REGS+i < 32)
	            mask[0] |= 1L << (NUM_REGS+i);
		else
		    mask[1] |= 1L << (NUM_REGS+i-32);

	i = do_get_regs(mask);

	return(i);
}


static int
do_get_regs(mask)
const unsigned long mask[2];
{
	int i;

	if (mask[0] != 0 || mask[1] != 0)
	{
	    const unsigned char *rp;

	    com_init_msg();
	    if (com_put_byte(GET_REGS) != OK
	        || com_put_byte(~OK) != OK
	        || com_put_long(mask[0]) != OK
	        || com_put_long(mask[1]) != OK)
	    {
		hdi_cmd_stat = com_get_stat();
		return(ERR);
	    }

	    if ((rp = _hdi_send(0)) == NULL)
		return(ERR);

	    for (i = 0; i < NUM_REGS; i++) 
	    {
	        if (i < 32 ? (mask[0] & (1L << i)) : (mask[1] & (1L << (i-32))))
		{
		    local_regs[i] = get_long(rp);
		    reg_state[i] = VALID;
		}
	    }

	    for (i = 0; i < NUM_FP_REGS; i++) 
	    {
	        if (NUM_REGS+i < 32
		    ? mask[0] & (1L << (NUM_REGS+i))
		    : mask[1] & (1L << (NUM_REGS+i-32)))
		{
		    memcpy((local_fp_regs[i].fp80.fp80), rp, 10), rp += 10;
		    memcpy((local_fp_regs[i].fp64.fp64.value), rp, 8), rp += 8;
		    local_fp_regs[i].fp64.fp64.flags = get_byte(rp);
		    fp_reg_state[i] = VALID_BOTH;
	    }
		}
	}

	return(OK);
}

int
_hdi_put_regs()
{
	int i;
	int num_fp_regs;
	unsigned long mask0, mask1;

	num_fp_regs = (_hdi_arch == ARCH_KB || _hdi_arch == ARCH_SB) ? NUM_FP_REGS : 0;

	mask0 = mask1 = 0;
	for (i = 0; i < NUM_REGS; i++)
	    if (reg_state[i] == DIRTY)
		if (i < 32)
	            mask0 |= 1L << i;
		else
		    mask1 |= 1L << (i-32);

	for (i = 0; i < num_fp_regs; i++)
	    if (((int)fp_reg_state[i] & (int)FPDIRTY) != 0)
		if (NUM_REGS+i < 32)
	            mask0 |= 1L << (NUM_REGS+i);
		else
		    mask1 |= 1L << (NUM_REGS+i-32);

	if (mask0 != 0 || mask1 != 0)
	{
	    com_init_msg();
	    if (com_put_byte(PUT_REGS) != OK
	        || com_put_byte(~OK) != OK
	        || com_put_long(mask0) != OK
	        || com_put_long(mask1) != OK)
	    {
		hdi_cmd_stat = com_get_stat();
		return(ERR);
	    }

	    for (i=0; i < NUM_REGS; ++i) 
	        if (reg_state[i] == DIRTY)
	            if (com_put_long(local_regs[i]) != OK)
		    {
		        hdi_cmd_stat = com_get_stat();
		        return(ERR);
		    }

	    for (i=0; i < num_fp_regs; ++i) 
	    {
		/* An fp register may be marked as DIRTY80 (FPDIRTY | VALID80)
		 * or as DIRTY64 (FPDIRTY | VALID64).  We must send a format
		 * code indicating which format is coming, and then the proper
		 * value. */
		if (((int)fp_reg_state[i] & (int)FPDIRTY) != 0)
		{
		    int stat;

		    if ((int)fp_reg_state[i] & (int)VALID80)
		    {
			if ((stat = com_put_byte(FP_80BIT)) == OK)
			    stat = com_put_data
				(local_fp_regs[i].fp80.fp80, 10);
		    }
		    else
		    {
			if ((stat = com_put_byte(FP_64BIT)) == OK)
			    stat = com_put_data
				(local_fp_regs[i].fp64.fp64.value, 8);
		    }

		    if (stat != OK)
		    {
			hdi_cmd_stat = com_get_stat();
			return(ERR);
		    }
		}
	    }

            if (_hdi_send(0) == NULL)
		return(ERR);

	    /* We could do this in the loops above, but then if _hdi_send
	     * failed, the registers would be incorrectly marked valid. */
	    for (i=0; i < NUM_REGS; ++i) 
	        if (reg_state[i] == DIRTY)
		    reg_state[i] = VALID;

	    for (i=0; i < num_fp_regs; ++i) 
	        if (((int)fp_reg_state[i] & (int)FPDIRTY) != 0)
		    fp_reg_state[i] = (enum fp_register_state)((int)fp_reg_state[i] & (int)~FPDIRTY);
	}

	return(OK);
}
