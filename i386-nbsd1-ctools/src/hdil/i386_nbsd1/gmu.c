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

/*
 * MODULE
 *	gmu.c
 *
 * PURPOSE
 *	Routines designed to support host access to the Hx
 *	Guarded Memory Unit. (GMU)
 *
 * DESCRIPTION
 *	This interface maintains the GMU protection and detection registers.
 *	The term "register" is used throughout to mean "register pair", 
 *	that is, either an address/mask pair (protection) or an upper/lower
 *	bound pair. (detection)
 *
 * MAINTENANCE NOTE
 *	The number of GMU registers is implicitly defined, i.e. there's 
 *	no easy way for mon960 to get this information on demand.
 *	Since only one 960 architecture (Hx) currently supports a GMU, 
 *	the number of registers is hard-coded. (via constants defined 
 *	in hdi_gmu.h)
 * 
 *	If a future generation of 960 architecture defines a GMU, 
 *	the maintainer of this file should switch on the architecture to
 *	determine how many GMU registers are supplied by the hardware.
 */

#include "hdil.h"
#include "private.h"

/* The following constants are hard-wired for the Hx architecture.
   See the maintenance note above. */
#define HX_GCON  0x8000	/* GMU Control reg */
#define HX_MPAR0 0x8010	/* First protection address reg */
#define HX_MPMR0 0x8014	/* First protection mask reg */
#define HX_MDUB0 0x8080	/* First detection upper bounds reg */
#define HX_MDLB0 0x8084	/* First detection lower bounds reg */

/* The following are bit offsets from bit 0 of the GMU control reg. (GCON)
   These are also Hx-specific. */
#define HX_DETECT_REG_BITOFF 16
#define HX_PROTECT_REG_BITOFF 0

static HDI_GMU_REG local_reg;
static HDI_GMU_REGLIST local_reglist;

/*
 * FUNCTION 
 *  hdi_set_gmu_reg(int type, int regnum, int *regused, HDI_GMU_REG *regval)
 *
 * DESCRIPTION
 *  Allows host to set the value of a GMU register.
 *
 * PARAMS
 *  type - protect or detect, valid values are HDI_GMU_DETECT or
 *	HDI_GMU_PROTECT (defined in hdi_gmu.h)
 *
 *  regnum - the register number to define.  Each register type (protect or 
 *	detect) has its own set of register numbers, beginning at 0.
 *
 *  regval - caller initializes this structure (defined in hdi_gmu.h) with
 *	the values to be placed in the specified GMU register.
 *
 * RETURNS
 *  OK if no errors are detected, else ERR, plus an appropriate HDI error 
 *  code via hdi_cmd_stat.
 */

int 
hdi_set_gmu_reg(type, regnum, regval)
    int type;
    int regnum;
    HDI_GMU_REG *regval;
{
    unsigned long mmr_offset; /* MMR offset for the desired GMU reg */
    unsigned long mmr_value;  /* MMR contents */
    unsigned long dummy; /* to satisfy hdi_set_mmr_reg prototype */
    
    if (_hdi_arch != ARCH_HX)
    {
        hdi_cmd_stat = E_ARCH;
        return(ERR);
    }
    switch (type)
    {
    case HDI_GMU_DETECT:
	if (regnum < 0 || regnum >= HX_MAX_GMU_DETECT_REGS)
	{
	    hdi_cmd_stat = E_GMU_BADREG;
	    return ERR;
	}
	mmr_offset = HX_MDUB0 + (regnum * 8);
	break;
    case HDI_GMU_PROTECT:
	if (regnum < 0 || regnum >= HX_MAX_GMU_PROTECT_REGS)
	{
	    hdi_cmd_stat = E_GMU_BADREG;
	    return ERR;
	}
	mmr_offset = HX_MPAR0 + (regnum * 8);
	break;
    default:
	hdi_cmd_stat = E_ARG;
	return ERR;
    }

    /* Set value of part 1 of register pair.  This part also 
       contains the access codes. */
    mmr_value = regval->loword & (~ 0xff);
    mmr_value |= regval->access;
    if (hdi_set_mmr_reg(mmr_offset, mmr_value, 0xffffffff, &dummy) == ERR)
	return ERR;

    /* Set value of part 2 of register pair */
    mmr_value = regval->hiword;
    if (hdi_set_mmr_reg(mmr_offset + 4, mmr_value, 0xffffff00, &dummy) == ERR)
	return ERR;

    /* Enable the new register */
    return hdi_update_gmu_reg(type, regnum, regval->enabled);
}


/*
 * FUNCTION 
 *  hdi_update_gmu_reg(int type, int regnum, int action)
 *
 * PARAMS
 *  type - protect or detect, valid values are HDI_GMU_DETECT or
 *	HDI_GMU_PROTECT.
 *
 *  regnum - the register number to enable/disable.
 *
 *  enable - boolean: 1 = enable the register, 0 = disable
 *
 * RETURNS
 *  OK if no errors are detected, else ERR, plus an appropriate HDI error 
 *  code via hdi_cmd_stat.
 *
 * DESCRIPTION
 *  Allows host to enable or disable an existing GMU register.
 */

int 
hdi_update_gmu_reg(type, regnum, enable)
    int type;
    int regnum;
    int enable;
{
    unsigned long gcon_bitoffset; /* position within GCON of reg enable bit */
    unsigned long dummy; /* to satisfy hdi_set_mmr_reg prototype */

    hdi_cmd_stat = 0;
    if (_hdi_arch != ARCH_HX)
    {
        hdi_cmd_stat = E_ARCH;
        return ERR;
    }
    switch (type)
    {
    case HDI_GMU_DETECT:
	if (regnum < 0 || regnum >= HX_MAX_GMU_DETECT_REGS)
	{
	    hdi_cmd_stat = E_GMU_BADREG;
	    return ERR;
	}
	gcon_bitoffset = HX_DETECT_REG_BITOFF + regnum;
	break;
    case HDI_GMU_PROTECT:
	if (regnum < 0 || regnum >= HX_MAX_GMU_PROTECT_REGS)
	{
	    hdi_cmd_stat = E_GMU_BADREG;
	    return ERR;
	}
	gcon_bitoffset = HX_PROTECT_REG_BITOFF + regnum;
	break;
    default:
	hdi_cmd_stat = E_ARG;
	return ERR;
    }

    /* Set/clear the appropriate bit in GCON */
    return hdi_set_mmr_reg(HX_GCON, 
			   enable ? 0xffffffff : 0, 
			   1 << gcon_bitoffset,
			   &dummy);
}


/*
 * FUNCTION 
 *  hdi_get_gmu_reg(int type, int regnum, HDI_GMU_REG *reg)
 *
 * PARAMS
 *  type - protect or detect, valid values are HDI_GMU_DETECT or
 *	HDI_GMU_PROTECT
 *
 *  regnum - the register number to report.  Each register type (protect or 
 *	detect) has its own set of register numbers, beginning at 0.
 *
 *  reg - a pointer to caller's area to store the value of the register.
 *
 * RETURNS
 *  OK if no errors are detected, else ERR, plus an appropriate HDI error 
 *  code via hdi_cmd_stat.
 *
 * DESCRIPTION
 *  Allows host to query the value of a specific GMU register.
 */

int 
hdi_get_gmu_reg(type, regnum, reg)
    int type;
    int regnum;
    HDI_GMU_REG *reg;
{
    unsigned long mmr_offset; /* MMR offset for the desired GMU reg */
    unsigned long mmr_value;  /* MMR contents */
    unsigned long gcon_bitoffset; /* position within GCON of reg enable bit */

    if (_hdi_arch != ARCH_HX)
    {
        hdi_cmd_stat = E_ARCH;
        return(ERR);
    }
    switch (type)
    {
    case HDI_GMU_DETECT:
	if (regnum < 0 || regnum >= HX_MAX_GMU_DETECT_REGS)
	{
	    hdi_cmd_stat = E_GMU_BADREG;
	    return ERR;
	}
	mmr_offset = HX_MDUB0 + (regnum * 8);
	gcon_bitoffset = HX_DETECT_REG_BITOFF + regnum;
	break;
    case HDI_GMU_PROTECT:
	if (regnum < 0 || regnum >= HX_MAX_GMU_PROTECT_REGS)
	{
	    hdi_cmd_stat = E_GMU_BADREG;
	    return ERR;
	}
	mmr_offset = HX_MPAR0 + (regnum * 8);
	gcon_bitoffset = HX_PROTECT_REG_BITOFF + regnum;
	break;
    default:
	hdi_cmd_stat = E_ARG;
	return ERR;
    }

    /* Get value of part 1 of register pair.  This part also 
       contains the access codes. */
    if (hdi_set_mmr_reg(mmr_offset, 0, 0, &mmr_value) == ERR)
	return ERR;
    reg->loword = mmr_value & 0xffffff00;
    reg->access = mmr_value & 0xff;

    /* Get value of part 2 of register pair */
    if (hdi_set_mmr_reg(mmr_offset + 4, 0, 0, &mmr_value) == ERR)
	return ERR;
    reg->hiword = mmr_value;

    /* Get enable bit status */
    if (hdi_set_mmr_reg(HX_GCON, 0, 0, &mmr_value) == ERR)
	return ERR;
    reg->enabled = (mmr_value & (1 << gcon_bitoffset)) ? 1 : 0;
    return (OK);
}


/*
 * FUNCTION 
 *  hdi_get_gmu_regs(HDI_GMU_REGLIST *reglist)
 *
 * PARAMS
 *  reglist - a pointer to caller's area to store the values of the registers.
 *
 * RETURNS
 *  OK if no errors are detected, else ERR, plus an appropriate HDI error 
 *  code via hdi_cmd_stat.
 *
 * DESCRIPTION
 *  Allows host to query the value of all GMU registers in one call.
 */

int 
hdi_get_gmu_regs(reglist)
    HDI_GMU_REGLIST *reglist;
{
    int i;
    if (_hdi_arch != ARCH_HX)
    {
        hdi_cmd_stat = E_ARCH;
        return(ERR);
    }

    reglist->num_dregs = HX_MAX_GMU_DETECT_REGS;
    reglist->num_pregs = HX_MAX_GMU_PROTECT_REGS;

    for (i = 0; i < reglist->num_dregs; ++i)
    {
	if (hdi_get_gmu_reg(HDI_GMU_DETECT, i, &reglist->dreg[i]) == ERR)
	    return ERR;
    }

    for (i = 0; i < reglist->num_pregs; ++i)
    {
	if (hdi_get_gmu_reg(HDI_GMU_PROTECT, i, &reglist->preg[i]) == ERR)
	    return ERR;
    }
    return (OK);
}
