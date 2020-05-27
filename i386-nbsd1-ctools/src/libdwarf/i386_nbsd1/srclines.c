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
#include "lines.h"

/*
    LIBRARY FUNCTION

	dwarf_srclines -- Read a block of line numbers for an entire
	    compilation unit

    SYNOPSIS

	Dwarf_Signed dwarf_srclines(Dwarf_Debug dbg,
				    Dwarf_Die die,
				    Dwarf_Line **linebuf);
    DESCRIPTION
    
	Allocate and assign contiguous storage to hold Dwarf_Line descriptors 
	for an entire compilation unit.  Set LINEBUF to point to the start of 
	the array of descriptors.  The DIE parameter must represent a
	compilation unit.  The DIE parameter must contain a DW_AT_stmt_list
	attribute (points to start of line number info for the CU.) 
	See _dw_build_ln_list for a description of which line number opcodes
	cause a new descriptor to be added to the array.

    RETURNS

	The number of descriptors in the block.  DLV_NOCOUNT is returned
	on error.

    BUGS

	If linebug is non-NULL on entry, this function short-circuits and
	returns, rather than waste time doing the same thing twice.  But in
	the future we may find a way to read in only some of the line number
	info for a CU, not all of it.  We need a way to tell what has been
	done and what hasn't.
*/

Dwarf_Signed 
dwarf_srclines(dbg, die, linebuf)
    Dwarf_Debug dbg;
    Dwarf_Die die; 
    Dwarf_Line *linebuf;
{
    RESET_ERR();
    if ( die->tag != DW_TAG_compile_unit )
    {
    	if(LIBDWARF_ERR(DLE_NOT_CU_DIE) == DLS_ERROR)
    	    return(DLV_NOCOUNT);
    }

    /* FIXME-SOMEDAY: the following test assumes that the entire table
       gets built all at one time.  */
    if ( die->cu_node->line_table )
    {
	*linebuf = die->cu_node->line_table->ln_list;
	return die->cu_node->line_table->ln_count;
    }

    if (DIE_HAS_ATTR(die, DW_AT_stmt_list))
    {
	/* The offset within the .debug_line section where the line
	   number info begins */
	Dwarf_Unsigned ln_offset = DIE_ATTR_UNSIGNED(die, DW_AT_stmt_list);
	Dwarf_Section ln_section = _dw_find_section(dbg, ".debug_line");
	Dwarf_Line_Table *ln_hdr;

	if (!ln_section && LIBDWARF_ERR(DLE_NOLINE) == DLS_ERROR)
	    return DLV_NOCOUNT;

	ln_hdr = die->cu_node->line_table = 
	    (Dwarf_Line_Table*) _dw_malloc(dbg, sizeof(Dwarf_Line_Table));
	ln_hdr->big_endian = ln_section->big_endian;

	_dw_fseek(dbg->stream, 
		  ln_section->file_offset + ln_offset, 
		  SEEK_SET);
	ln_hdr->ln_prolog = _dw_read_line_prolog(dbg, ln_hdr->big_endian);

	if(ln_hdr->ln_prolog) 
	{ 
	    ln_hdr->ln_list = 
		_dw_build_ln_list(dbg, ln_hdr->ln_prolog, 
				  &ln_hdr->ln_count, ln_hdr->big_endian);
	    *linebuf = ln_hdr->ln_list;
	    return ln_hdr->ln_count;
	}
	else 
	{
	    LIBDWARF_ERR(DLE_MLE);
	    return(DLV_NOCOUNT);
	}	
    }
    else
    {
	return DLV_NOCOUNT;	/* no DW_AT_stmt_list attribute */
    }
}

