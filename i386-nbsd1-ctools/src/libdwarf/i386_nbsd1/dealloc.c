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

/****************************************************************************
*
*  ROUTINE:  dwarf_dealloc
*
*  PURPOSE:  Release all libdwarf internal resources associated with the
*            given SPACE.  TYP tells you what was originally allocated.
*
*  ARGUMENTS:
*
*    space - The area to be deallocated.
*
*    typ - The type of space to be deallocated (see libdwarf.h for more
*          information).
*
*  RETURN: VOID
*
*  FIXME:  Disabled for now; See dwarf_finish() -- it uses a quick-and-dirty 
*          approach that deallocs everything.
*****************************************************************************/
void
dwarf_dealloc(space, typ)
void *space;
Dwarf_Unsigned typ;
{
    RESET_ERR(); 
    
    switch(typ) 
    {
    case DLA_STRING:      /* points to char* */
    case DLA_DIE:         /* points to CU Dwarf_Die */
    case DLA_LOC:         /* points to Dwarf_Loc */
    case DLA_LOCDESC:     /* points to Dwarf_Locdesc */
    case DLA_ELLIST:      /* points to Dwarf_Ellist */
    case DLA_BOUNDS:      /* points to Dwarf_Bounds */
    case DLA_BLOCK:       /* points to Dwarf_Block */
    case DLA_LINE:        /* points to Dwarf_Line */
    case DLA_ATTR:        /* points to Dwarf_Attribute */
    case DLA_TYPE:        /* points to Dwarf_Type */
    case DLA_SUBSCR:      /* points to Dwarf_Subscr */
    case DLA_GLOBAL:      /* points to Dwarf_Global */
    case DLA_ERROR:       /* points to Dwarf_Error */
    case DLA_LIST:        /* points to a list */
    default:
	LIBDWARF_ERR(DLE_NI); 
	break;
    } 	
}

#if 0
/* _dw_free calls are suspect */

/*
 * _dw_dealloc_attr -- helper function for _dw_dealloc_die.
 * [currently unused]
 *
 * deallocates from 'attr' to the end of the attribute 
 * list.  We must have at least one attr.
 */
static void
_dw_dealloc_attr(attr)
    Dwarf_Attribute attr;
{
    Dwarf_Attribute tmpattr;
    Dwarf_Loc tmploc;   
    Dwarf_Loc tmploc2;   
    Dwarf_Locdesc tmplocdesc;   
    Dwarf_Locdesc tmplocdesc2;   
   
    do {
        tmpattr = attr->next;
        switch(attr->val_type) {
	case DLT_LOCLIST:
	    tmplocdesc = attr->at_value.val_loclist;
	    while(tmplocdesc) { 
		tmplocdesc2 = tmplocdesc->next;
		tmploc = tmplocdesc->ld_loc;
		while(tmploc) {
		    tmploc2 = tmploc->next;
		    _dw_free(tmploc);
		    tmploc = tmploc2; 
		}
		_dw_free(tmplocdesc);
		tmplocdesc = tmplocdesc2;
	    } 
	    break;
	case DLT_BLOCK:
	    _dw_free(attr->at_value.val_block);
	    break;
	case DLT_STRING:
	    _dw_free(attr->at_value.val_string);
	    break;
	case DLT_UNSIGNED:
	case DLT_SIGNED:
	default:
	    break;
	}
        _dw_free(attr);
	attr = tmpattr;
    } while(attr);
}


/*
 * _dw_dealloc_die -- helper function for dwarf_dealloc.
 * [currently unused]
 *
 * deallocates all DIEs in a particular CU-DIE tree.
 *
 */
static void
_dw_dealloc_die(die)
    Dwarf_Die die;
{
    if(die->attr_list)
	dealloc_attr(die->attr_list);
    _dw_free(die);
}


/*
 * _dw_dealloc_tree -- helper function for dwarf_dealloc.
 * [currently unused]
 *
 * It deallocates all DIEs in a particular CU-DIE tree.
 *
 */
void
_dw_dealloc_tree(die)
    Dwarf_Die die;
{
    Dwarf_Die tmpdie;

    if ( die->child_ptr )
    {
        _dw_dealloc_tree(die->child_ptr);
        if(die->tag != DW_TAG_compile_unit) 
	{
	    if ( die->sibling_ptr )
	    {
                tmpdie = die->sibling_ptr;
                _dw_dealloc_die(die);
		if(tmpdie)
		    _dw_dealloc_tree(tmpdie);
    	    }
	    else 
                _dw_dealloc_die(die);
	}
	else 
	    _dw_dealloc_die(die);
    }
    else if ( die->tag != DW_TAG_compile_unit && die->sibling_ptr )
    {
	tmpdie = die->sibling_ptr;
	_dw_dealloc_die(die);
	_dw_dealloc_tree(tmpdie);
    }
    else 
	_dw_dealloc_die(die);
    return;
} 

#endif /* #if 0 */
