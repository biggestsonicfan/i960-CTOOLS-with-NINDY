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
 * dwarf_cu_offdie - Finds the DIE at CU relative offset.
 * You must pass it a CU DIE from whence to start looking.
 *
 * RETURNS: The next Dwarf_Die at the given CU-relative 
 * offset if it can find it, otherwise NULL.
 * 
 * ERROR: Return NULL
 */
Dwarf_Die dwarf_cu_offdie(dbg, die, offset)
    Dwarf_Debug dbg;
    Dwarf_Die die;
    Dwarf_Off offset;
{
    RESET_ERR();
    if(!die) { 	/* if it's NULL, it doesn't know which CU to start in 
   	 	 * I'm going to error out for now */ 
        LIBDWARF_ERR(DLE_IA);
	return(NULL);
    }	
    else
        return _dw_find_cu_offset(dbg, die, offset);
}

/*
 * _dw_find_cu_offset -- Finds the DIE at the given CU-relative
 * offset.  Notice that this will build the CU tree for the 
 * particular CU if the information is not already there.
 *
 * RETURNS: The Dwarf_Die at the given CU-offset from 'root' if
 * it can find, otherwise NULL.
 * 
 * ERROR: Return NULL
 */
Dwarf_Die
_dw_find_cu_offset(dbg, root, offset)
    Dwarf_Debug dbg;
    Dwarf_Die root;
    Dwarf_Off offset;
{
    Dwarf_Die die;
    Dwarf_Bool build = TRUE;
    
    RESET_ERR();    

    if ( root->tag == DW_TAG_compile_unit ) 
    {
	/* Avoid marching down the sibling list if I'm a CU DIE */
	if ( root->cu_offset == offset )
	    return root;
	if ( root->has_child )
	{
	    Dwarf_Die child = dwarf_child(dbg, root);
	    return _dw_find_cu_offset(dbg, child, offset);
	}
	else
	    return(NULL);
    }

    /* else NOT a compilation unit DIE */
    for ( die = root; die && die->tag; die = dwarf_siblingof(dbg, die) )
    {
	if ( die->cu_offset == offset )
	    return die;
        if ( die->has_child )
	{
	    Dwarf_Die child = dwarf_child(dbg, die);
	    Dwarf_Die found = _dw_find_cu_offset(dbg, child, offset);
	    if ( found )
		return found;
	}
    }
    return(NULL);
}

