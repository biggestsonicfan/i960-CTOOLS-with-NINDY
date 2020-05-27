/*****************************************************************************
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
 ****************************************************************************/

#include "libdwarf.h"
#include "internal.h"

/* 
 * dwarf_offdie - Finds the DIE at the given *section* offset.
 * (see dwarf_cu_offdie for searching for a given CU offset.)
 *
 * If 'die' parameter is NULL search the entire .debug_info section.
 * Else search only the compilation unit 'die' belongs to.
 * 
 * RETURNS: The Dwarf_Die at 'offset' if it can find one, 
 * otherwise NULL.
 * 
 * ERROR: Return NULL
 */
Dwarf_Die
dwarf_offdie(dbg, die, offset)
    Dwarf_Debug dbg;
    Dwarf_Die die;
    Dwarf_Off offset;
{
    Dwarf_Die cudie;
    RESET_ERR();

    if (die)
    { 
	cudie = die->cu_node->die_head->die_list;
	return _dw_find_cu_offset(dbg, cudie, 
				  offset - cudie->section_offset
				  + DW_CU_HDR_LENGTH);
    }
    else
    {
	for (cudie = dbg->dbg_cu_list->die_head->die_list;
	     cudie;
	     cudie = cudie->sibling_ptr)
	{
	    if (offset == cudie->section_offset)
		return cudie;
	    else if (offset > cudie->section_offset
		     && (!cudie->sibling_ptr 
			 || offset < cudie->sibling_ptr->section_offset))
		return _dw_find_cu_offset(dbg, cudie, 
					  offset - cudie->section_offset
					  + DW_CU_HDR_LENGTH);
	}
	return NULL;
    }
}

