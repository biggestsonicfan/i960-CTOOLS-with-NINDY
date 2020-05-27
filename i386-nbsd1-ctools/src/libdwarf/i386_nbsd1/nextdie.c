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
 * dwarf_nextdie -- returns the next DIE found in the Dwarf file.
 *
 * Because libdwarf represents the tree structure of the user's program,
 * and not the linear view of the Dwarf file, this function works backwards
 * from the tree structure to find the DIE which *would have* come next 
 * in the file.
 *
 * RETURNS: The next DIE if there is one, otherwise NULL.
 * 
 * ERROR: Return NULL
 */
Dwarf_Die dwarf_nextdie(dbg, die)
    Dwarf_Debug  dbg;
    Dwarf_Die    die;
{
    Dwarf_Die nextdie;

    RESET_ERR();    

    if (die == NULL)	/* by convention, rtn the first DIE of the first CU */
        return(dbg->dbg_cu_list->die_head->die_list);

    if (nextdie = dwarf_child(dbg, die))
	return nextdie;

    if (nextdie = dwarf_siblingof(dbg, die))
	return nextdie;

    /* The next DIE is the sibling of my parent, or the sibling of
       my parent's parent ... */
    for ( die = die->parent_ptr; die; die = die->parent_ptr )
    {
	if (nextdie = dwarf_siblingof(dbg, die))
	    return nextdie;
	if (die->tag == DW_TAG_compile_unit)
	    /* No more compilation units to search! */
	    break;
    }
    return NULL;
}

