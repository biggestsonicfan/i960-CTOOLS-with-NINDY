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
 * dwarf_child -- Finds the child of the given DIE.
 * Notice that this will build the CU tree for the particular
 * CU if the information is not already there.
 *
 * RETURNS: The Dwarf_Die child of the given DIE if it can 
 * find one, otherwise NULL.
 * 
 * ERROR: Return NULL
 */
Dwarf_Die 
dwarf_child(dbg, die) 
    Dwarf_Debug dbg;
    Dwarf_Die die;
{
    Dwarf_Bool build = TRUE;
 
    RESET_ERR();
    if (die == NULL)     
	/* return the first DIE of the first CU */
        return dbg->dbg_cu_list->die_head->die_list;
    if ( die->has_child )
    {
	if ( die->child_ptr )
	    return die->child_ptr;

	_dw_fseek(dbg->stream, 
		  die->cu_node->die_head->file_offset +
		  die->cu_offset +
		  die->length,
		  SEEK_SET);
	die->child_ptr =
            _dw_build_a_die(dbg, die->cu_node, die);

	return(die->child_ptr);
    }
    else
	/* No child */
        return(NULL);
}

