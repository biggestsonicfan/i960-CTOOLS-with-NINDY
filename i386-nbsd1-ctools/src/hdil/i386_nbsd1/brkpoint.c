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
/*
 * MODULE NAME: brkpoint.c
 * DESCRIPTION:
 *	contains the routines and data for handling break points
 */
/* $Header: /ffs/p1/dev/src/hdil/common/RCS/brkpoint.c,v 1.2 1994/08/16 22:37:12 gorman Exp $$Locker:  $ */

#ifdef __STDC__
#include <stdlib.h>
#else /* __STDC__ */
extern char *malloc();
extern void free();
#endif /* __STDC__ */

#include "private.h"
#include "dbg_mon.h"
#include "hdi_com.h"

/*
 * EXTERNAL VARIABLES
 */

typedef struct brk_desc {
	ADDR addr;
	unsigned char type;
	unsigned char mode; 
	unsigned long instr;
	struct brk_desc *next;
} BRK_DESC;

static BRK_DESC *brk_tbl = NULL;

/*
 * LOCAL FUNCTIONS
 */

#ifdef __STDC__
static int set_sw_bp(BRK_DESC *bp);
static int set_hw_bp(BRK_DESC *bp);
static int del_bp(BRK_DESC *bp);
static int del_hw_bp(BRK_DESC *bp);
#else /* __STDC__ */
static int set_sw_bp();
static int set_hw_bp();
static int del_bp();
static int del_hw_bp();
#endif /* __STDC__ */

#define FMARK   0x66003e00L     /* breakpoint instruction */

void
_hdi_bp_init()
{
	brk_tbl = NULL;
}



int
hdi_bp_type(addr)
ADDR addr;
{
	register BRK_DESC *bp;

	for (bp = brk_tbl; bp; bp = bp->next)
	    if (bp->addr == addr)
		return bp->type;

	return(BRK_NONE);
}


/*
 * FUNCTION NAME: set_brk_pt
 * ACTION:
 *	set_brk_pt sets a user breakpoint at the specified address after
 *	insuring that there is not an existing breakpoint at the address.
 */

int
hdi_bp_set(addr, type, mode)
ADDR addr;
int type, mode;
{
	long instr;

	register BRK_DESC *bp;
	int r;

	if ((type == BRK_SW || type == BRK_HW) && (addr & 3) != 0)
	{
	    hdi_cmd_stat = E_ALIGN;
	    return(ERR);
	}

	if (hdi_bp_type(addr) != BRK_NONE)
	{
	    hdi_cmd_stat = E_BPSET;
	    return(ERR);
	}

	if (hdi_mem_read(addr, &instr, 4, FALSE, 4) == OK)
	{
	    if (instr == FMARK)
			{
	    	hdi_cmd_stat = E_BPSET;
		    return(ERR);
			}
	}

	if ((bp = (BRK_DESC *)malloc(sizeof(BRK_DESC))) == NULL)
	{
	    hdi_cmd_stat = E_NOMEM;
	    return(ERR);
	}

	bp->addr = addr;
	bp->type = type;
	bp->mode = mode;

	if (type == BRK_SW)
	    r = set_sw_bp(bp);
	else
	    r = set_hw_bp(bp);

	if (r != OK) {
	    free((char *)bp);
	    return(ERR);
	}

	bp->next = brk_tbl;
	brk_tbl = bp;

	return(OK);
}


int
hdi_bp_del(addr)
ADDR addr;		/* address of breakpoint to be deleted */
{
	register BRK_DESC *bp, *t;

	t = NULL;
	for (bp = brk_tbl; bp && bp->addr != addr; bp = bp->next)
	    t = bp;

	if (bp == NULL)
	{
	    hdi_cmd_stat = E_BPNOTSET;
	    return(ERR);
	}

	if (del_bp(bp) != OK)
	      return(ERR);

	if (t != NULL)
	    t->next = bp->next;
	else
	    brk_tbl = bp->next;
	free((char *)bp);

	return(OK);
}

int
hdi_bp_rm_all()
{
	register BRK_DESC *bp;
	int removed = FALSE;

	while (brk_tbl != NULL)
	{
	    bp = brk_tbl;
	    if (del_bp(bp) != OK)
		return(ERR);
	    removed = TRUE;
	    brk_tbl = bp->next;
	    free((char *)bp);
	}

	return(removed);
}

static int
del_bp(bp)
BRK_DESC *bp;
{
	switch (bp->type) {
	    case BRK_SW:
		    /* Set bp type to something other than BRK_SW so that
		     * hdi_mem_write doesn't give an error saying you can't
		     * write over a SW breakpoint. */
		    bp->type = BRK_NONE;
		    if (hdi_mem_write(bp->addr,&bp->instr,4,TRUE,FALSE,4)!=OK)
		    {
			bp->type = BRK_SW;
			return(ERR);
		    }
		    return(OK);

	    case BRK_HW:
	    case BRK_DATA:
		    del_hw_bp(bp);
		    return(OK);
	}

	return(ERR);	/* This should not be reached. */
} 

/*
 * Actually install a software breakpoint into the target environment
 */
static int
set_sw_bp(bp)
BRK_DESC *bp;
{
	unsigned long tmp = FMARK;

	if (hdi_mem_read(bp->addr, &bp->instr, 4, FALSE, 4) != OK
	    || hdi_mem_write(bp->addr, &tmp, 4, TRUE, FALSE, 4) != OK)
	{
	    if (hdi_cmd_stat == E_VERIFY_ERR)
		hdi_cmd_stat = E_SWBP_ERR;
	    return(ERR);
	}

	return(OK);
}

static int
set_hw_bp(bp)
BRK_DESC *bp;
{
        com_init_msg();
        if (com_put_byte(SET_HW_BP) != OK
            || com_put_byte(~OK) != OK
            || com_put_byte(bp->type) != OK
            || com_put_byte(bp->mode) != OK
            || com_put_long(bp->addr) != OK)
        {
            hdi_cmd_stat = com_get_stat();
            return(ERR);
        }

        if (_hdi_send(0) == NULL)
            return(ERR);

	return(OK);
}

static int
del_hw_bp(bp)
BRK_DESC *bp;
{
        com_init_msg();
        if (com_put_byte(CLR_HW_BP) != OK
            || com_put_byte(~OK) != OK
            || com_put_long(bp->addr) != OK)
        {
            hdi_cmd_stat = com_get_stat();
            return(ERR);
        }

        if (_hdi_send(0) == NULL)
            return(ERR);

	return(OK);
}


_hdi_bp_instr(ip, instr)
ADDR ip;
unsigned long *instr;
{
	register BRK_DESC *bp;

	for (bp = brk_tbl; bp; bp = bp->next)
	{
	    if (bp->type == BRK_SW && bp->addr == ip)
	    {
		*instr = bp->instr;
		return TRUE;
	    }
	}
	return FALSE;
}


void
_hdi_replace_bps(buf, addr, sz, mem_size)
#if defined(VAX_ULTRIX_SYS)
/* vax-ultrix /bin/cc chokes on void *buf */
char *buf;
#else
void *buf;
#endif /* VAX_ULTRIX_SYS */
ADDR addr;
unsigned sz;
int mem_size;
{
	register BRK_DESC *bp;

	for (bp = brk_tbl; bp; bp = bp->next)
	{
	    if (bp->type == BRK_SW
		&& bp->addr+3 >= addr && bp->addr < addr+sz)
	    {
		if (mem_size >= 4)
		{
		    register unsigned long *lp = (unsigned long *)buf;
		    register int offset = (int)(bp->addr - addr) / 4;

		    lp[offset] = bp->instr;
		}
		else if (mem_size == 2)
		{
		    register unsigned short *sp = (unsigned short *)buf;
		    register int offset = (int)(bp->addr - addr) / 2;
		    register int i;

		    for (i = 0; i<2 && (long)offset*2 < (long)sz; i++,offset++)
			if (offset >= 0)
			{
			    /* Place the appropriate half of the original
			     * instruction in the buffer, according to
			     * whether the target is big-endian or little-
			     * endian, which we can tell by comparing the
			     * contents to half of an FMARK instruction. */
			    if (sp[offset] == (unsigned short)(FMARK >> 16))
				sp[offset] = (unsigned short)(bp->instr >> 16);
			    else
				sp[offset] = (unsigned short)bp->instr;
			}
		}
		else
		{
		    register unsigned char *p = (unsigned char *)buf;
		    register int offset = (int)(bp->addr - addr);
		    register int be;	/* True if target is big-endian */
		    register int i;

		    if (offset >= 0)
			be =  *(p + offset) == (unsigned char)(FMARK >> 24);
		    else  /* Note: offset is negative; thus this shift is
			   * either 16, 8, or 0 bits. */
			be =  *p == (unsigned char)(FMARK >> (24+(offset*8)));

		    /* Place the appropriate bytes of the original instruction
		     * in the buffer according to whether the target is
		     * big-endian or little-endian. */
		    for (i = 0; i<4 && (long)offset<(long)sz; i++, offset++)
			if (offset >= 0)
			    p[offset] = (unsigned char)
				          (bp->instr >> ((be ? 3-i : i) * 8));
		}
	    }
	}
}

int
_hdi_check_bps(addr, sz)
ADDR addr;
unsigned sz;
{
	register BRK_DESC *bp;

	for (bp = brk_tbl; bp; bp = bp->next)
	{
	    if (bp->type == BRK_SW
		&& bp->addr+3 >= addr && bp->addr < addr+sz)
	    {
		return TRUE;
	    }
	}

	return FALSE;
}
