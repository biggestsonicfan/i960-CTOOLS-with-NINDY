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
 * dwarf_siblingof -- returns the sibling of the given DIE.
 * If the given DIE has a sibling attribute, then just seek
 * directly to the correct spot in the file.
 *
 * If the DIE dos *not* have a sibling attribute, the only way 
 * to tell if it has a sibling is by seeing if there is anything
 * after the last child in its sub-tree, because of the linear way 
 * that dwarf information is laid out in the file.
 *
 *						|
 * 					+-------+-------+
 * 	+-----+				|	        |
 * 	|  A  |				A	    	D
 * 	+-----+				|
 * 	|  B  |			+-------+-------+
 * 	+-----+			|	        |
 * 	|  C  |			B		C
 * 	+-----+
 * 	|  D  |
 * 	+-----+
 *
 * The tree on the right will be layed out in the file in the
 * manner shown on the left.  Where all the children of a DIE 
 * follow the DIE until there are no more children, then, if there
 * is still information in the particular CU contribution in the
 * file, it must start with the sibling.
 *
 * RETURNS: The Dwarf_Die sibling of the given DIE if it can 
 * find one, otherwise NULL.
 * 
 * ERROR: Return NULL
 */
Dwarf_Die
dwarf_siblingof(dbg, die)
    Dwarf_Debug dbg;
    Dwarf_Die die;
{
    Dwarf_Bool build = TRUE;
    Dwarf_Attribute sib_attr;

    RESET_ERR();    
    if (die == NULL) 		
	/* return the first DIE of the first CU */
        return(dbg->dbg_cu_list->die_head->die_list);
    if (die->sibling_ptr)
	/* been here, done that */
        return(die->sibling_ptr);
    if (die->tag == 0 || die->tag == DW_TAG_compile_unit)
    {
	/* special case null sibling-chain terminator,
	   or special case LAST compilation unit DIE */
	return NULL;
    }

    if ( die->has_sib_attr ) 
    { 
   	/* It has a sibling attribute, but it hasn't been expanded.
	   The sibling attribute is one of two reference forms, CU-relative
	   or object relative.  Ignore the object-relative ones. (FIXME) */
	sib_attr = dwarf_attr(die, DW_AT_sibling);
	switch ( sib_attr->at_form )
	{
	case DW_FORM_ref1:
	case DW_FORM_ref2:
	case DW_FORM_ref4:
	case DW_FORM_udata:
	    /* CU-relative */
	    _dw_fseek(dbg->stream, 
		      die->cu_node->die_head->file_offset +
		      sib_attr->at_value.val_unsigned,
		      SEEK_SET);
	    break;		    
	    
	case DW_FORM_ref_addr:
	    /* object-relative, this is unsupported for now */
	    LIBDWARF_ERR(DLE_UNSUPPORTED_FORM);
	    return(NULL);
	default:
	    LIBDWARF_ERR(DLE_UNSUPPORTED_FORM);
	    return(NULL);
	}
        die->sibling_ptr = 
	    _dw_build_a_die(dbg, 
			die->cu_node, 
			die->parent_ptr);
        return(die->sibling_ptr);
    }
    else 
    { 	
	/* No sibling attribute; read from the file until the sibling
	   is found. */
	_dw_fseek(dbg->stream,
		  die->cu_node->die_head->file_offset +
		  die->cu_offset +
		  die->length,
		  SEEK_SET);
	if(die->has_child) 
	{
            if ( ! _dw_build_tree(dbg, die->cu_node, die) ) 
                return(NULL);       
	}
	/* Now the file pointer will be pointing at the sibling
	   of the DIE we have in hand. */
	die->sibling_ptr = 
	    _dw_build_a_die(dbg, die->cu_node, die->parent_ptr);
	return(die->sibling_ptr);
    }
}
