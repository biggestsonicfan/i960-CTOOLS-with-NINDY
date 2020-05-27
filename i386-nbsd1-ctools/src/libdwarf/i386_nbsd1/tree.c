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

/* Support routines that build or manipulate DIE trees.
   Functions here are not intended to be called directly by the debugger.  */

#include "libdwarf.h"
#include "internal.h"

/* Function prototypes for functions used in this file only */
static void
add_abbrev PARAMS ((Dwarf_Debug, Dwarf_Abbrev *, Dwarf_Abbrev_Table *, 
		    Dwarf_Unsigned *, Dwarf_Unsigned));

static Dwarf_Abbrev *
lookup_abbrev PARAMS ((int, Dwarf_Abbrev *, Dwarf_Unsigned));

static int
assign_attr_value PARAMS ((Dwarf_Debug, Dwarf_CU_List *,
			   Dwarf_Attribute, Dwarf_Bool));

static void
assign_attr_block PARAMS ((Dwarf_Debug, Dwarf_Attribute, int));

static void
assign_attr_location PARAMS ((Dwarf_Debug, Dwarf_Attribute, int));

static void
assign_attr_location_list PARAMS ((Dwarf_Debug, Dwarf_CU_List *,
				   Dwarf_Attribute, int));

static void
assign_attr_leb128 PARAMS ((Dwarf_Debug, Dwarf_Attribute, Dwarf_Bool));

static Dwarf_Abbrev_Table * 
build_abbrev_table PARAMS ((Dwarf_Debug, Dwarf_Section));

static
check_for_location PARAMS ((Dwarf_Attribute));

static Dwarf_Loc
build_location_expression PARAMS ((Dwarf_Debug, Dwarf_Unsigned, Dwarf_Bool));

static void
sort_abbrev_table PARAMS ((Dwarf_Abbrev *, Dwarf_Unsigned));

/*
 * _dw_build_a_die -- builds an individual DIE.  The actual 'worker bee'
 * for building the DIE tree.
 *
 * ASSUMES that the Dwarf file pointer is aimed at the correct byte
 * in the file.
 *
 * RETURN:  A full, valid CU-DIE if one was successfully created, 
 * otherwise NULL.
 *
 * ERROR: Returns NULL. 
 */
Dwarf_Die
_dw_build_a_die(dbg, cu_node, parent_ptr)
    Dwarf_Debug dbg;
    Dwarf_CU_List *cu_node;
    Dwarf_Die parent_ptr;
{
    Dwarf_Die die;
    Dwarf_Attrib_Form *fp;
    Dwarf_Attribute tail;
    Dwarf_Abbrev *ap;
    Dwarf_Bool big_endian = cu_node->die_head->big_endian;
    Dwarf_Unsigned start = _dw_ftell(dbg->stream);

    RESET_ERR(); 
    die = (Dwarf_Die)_dw_malloc(dbg, sizeof(struct Dwarf_Die));

    /* Get the offset from the beginning of the CU */ 
    die->cu_offset = start - cu_node->die_head->file_offset;
    
    /* Get the offset from the beginning of the section */ 
    die->section_offset = die->cu_offset + cu_node->die_head->section_offset;

    die->code = _dw_leb128_to_ulong(dbg);
    
    ap = lookup_abbrev(die->code, cu_node->abbrev_table->abbrev_array, 
		       cu_node->abbrev_table->entry_count);
    if ( ap == NULL && LIBDWARF_ERR(DLE_NO_ABBR_ENTRY) == DLS_ERROR )
	return(NULL);

    die->tag = ap->tag;
    die->cu_node = cu_node;
    die->has_child = ap->child;
    die->has_sib_attr = FALSE;
    die->attr_list = NULL;
    fp = ap->attr_list;
    
    if (die->tag == 0 && 
	die->code != 0 &&
	LIBDWARF_ERR(DLE_NO_TAG) == DLS_ERROR)
	return(NULL);

    if ( die->tag )
    {
	/* assign all the attributes for this DIE */
        while(fp != NULL) 
	{
	    Dwarf_Attribute tmp = (Dwarf_Attribute)
		_dw_malloc(dbg, sizeof(struct Dwarf_Attribute));
	    if (die->attr_list == NULL)
		die->attr_list = tail = tmp;
	    else
		tail = tail->next = tmp;
	    tmp->at_name = fp->name;
	    tmp->at_form = fp->form;
	    tmp->next = NULL;
            if (!assign_attr_value(dbg, cu_node, tmp, big_endian))
		return(NULL);
	    if (tmp->at_name == DW_AT_sibling)
		die->has_sib_attr = TRUE;
            fp = fp->next;
        }
    }
    die->length = _dw_ftell(dbg->stream) - start;
    die->parent_ptr = parent_ptr; 
    die->child_ptr = NULL;
    die->sibling_ptr = NULL;
    return(die);
}


/*
 * _dw_build_tree -- Build all DIEs found in the file from the current
 * position in the file until the end of the compilation unit.
 *
 * ASSUMES that the Dwarf file pointer is aimed at the correct byte
 * in the file (the root DIE of the tree to be built.)
 *
 * Builds child pointer recursively.  Does NOT build sibling pointers
 * recursively since there can be so many of them.
 *
 * RETURNS: The root DIE of the tree just built.
 *
 * ERROR: Returns NULL.
 */
Dwarf_Die
_dw_build_tree(dbg, cu_node, parent_ptr)
    Dwarf_Debug dbg;
    Dwarf_CU_List *cu_node;
    Dwarf_Die parent_ptr;	
{
    Dwarf_Die die;

    RESET_ERR(); 

    die = _dw_build_a_die(dbg, cu_node, parent_ptr);

    while ( die && die->code )
    {
	if (die->has_sib_attr)
	{
	    /* If there's a sibling attribute, then we must build the 
	       intervening DIE's.  Else we can just call dwarf_siblingof,
	       which will build the intervening DIEs for us. */
	    if (die->has_child)
	    {
		die->child_ptr = _dw_build_tree(dbg, cu_node, die);
		if(!die->child_ptr)
		    return(NULL);
	    }
	}
	die->sibling_ptr = dwarf_siblingof(dbg, die);
	die = die->sibling_ptr;
    }
    return(die);
}

/*
 * Comparator function for qsort and bsearch,
 * for the abbrev table.
 */
static
compare_abbrevs(ab1, ab2)
    Dwarf_Abbrev *ab1, *ab2;
{
    return (int) ab1->code - (int) ab2->code;
}

/*
 * sort_abbrev_table -- qsorts the abbrev table entries.
 */
static void
sort_abbrev_table(abbrev_table, abbrev_count)
    Dwarf_Abbrev *abbrev_table;
    Dwarf_Unsigned abbrev_count;
{
    qsort((void *)abbrev_table, abbrev_count, 
	  sizeof(Dwarf_Abbrev), compare_abbrevs);
}

/*
 * lookup_abbrev -- bsearches the sorted abbrev table entries.
 */
static
Dwarf_Abbrev *
lookup_abbrev(key, abbrev_array, abbrev_count)
    int key;
    Dwarf_Abbrev *abbrev_array;
    Dwarf_Unsigned abbrev_count;
{
    Dwarf_Abbrev dummy;

    dummy.code = key;
    return (Dwarf_Abbrev *)
	bsearch((void*)&dummy, (void*) abbrev_array, abbrev_count, 
		sizeof(Dwarf_Abbrev), compare_abbrevs);
}


/*  
 * add_abbrev -- helper function for build_abbrev_table.
 * Add one entry to the abbrev-table array.
 * Reallocate the array if needed (if adding a new entry will exceed
 * the current size of the array.)
 *
 * Make sure to set all newly allocated storage to 0.  That way
 * unused entries will always contain NULL by default.
 */
static void
add_abbrev(dbg, entry, header, table_size, index)
    Dwarf_Debug dbg;
    Dwarf_Abbrev *entry;
    Dwarf_Abbrev_Table *header;
    Dwarf_Unsigned *table_size;
    Dwarf_Unsigned index;
{
    if (index >= *table_size) 
    {
        if (header->abbrev_array) 
	{
            /* Realloc for twice the current size */
            header->abbrev_array = (Dwarf_Abbrev*)
		_dw_realloc(dbg, (void*)header->abbrev_array,
			    *table_size * 
			    sizeof(Dwarf_Abbrev) * 2);
            *table_size *= 2;
        }
        else
        {
            /* First time this has been called.  Allocate a reasonable number
               of tuples; we'll realloc later if needed. */
            *table_size = DW_NUM_OF_ABBREVS;
            header->abbrev_array = (Dwarf_Abbrev*)
		_dw_malloc(dbg, *table_size * sizeof(Dwarf_Abbrev));
        }
    }
    header->abbrev_array[index++] = *entry;
}


/****************************************************************************
*
*  ROUTINE: build_abbrev_table() 
*
*  PURPOSE: parse the raw bits from the .debug_abbrev section into 
*	    a usable structure per compilation unit. 
*
*  	The abbrev table is distinguished by having fixed-sized 
*	records (unlike DIEs).  It is represented as an array.  
*	The attribute name/form pairs are represented as linked 
*	lists, each one rooted in its particular abbrev table entry. 
*
*  ARGUMENTS:
*
*  RETURN: The abbrev table for the particular CU 
*****************************************************************************/
static
Dwarf_Abbrev_Table * 
build_abbrev_table(dbg, debug_abbrev)
    Dwarf_Debug    dbg;
    Dwarf_Section  debug_abbrev;
{
    Dwarf_Unsigned att_code, att_name, att_form;
    Dwarf_Bool assigned_zero = FALSE;
    Dwarf_Abbrev_Table *abbrev_table; 
    Dwarf_Abbrev abbrev; 
    Dwarf_Attrib_Form *tail; /* temp for chaining */
    Dwarf_Unsigned table_size = 0, index = 0;

    RESET_ERR(); 
    abbrev_table = (Dwarf_Abbrev_Table*)
	_dw_malloc(dbg, sizeof(Dwarf_Abbrev_Table));
    _dw_clear((char *) abbrev_table, sizeof(Dwarf_Abbrev_Table));
    abbrev_table->big_endian = debug_abbrev->big_endian;

    for (att_code = _dw_leb128_to_ulong(dbg);
	 att_code;
	 att_code = _dw_leb128_to_ulong(dbg))
    {	
        abbrev.code = att_code;
        abbrev.tag = (Dwarf_Half) _dw_leb128_to_ulong(dbg);
        abbrev.child = _dw_read_constant(dbg, 1, 0);
	abbrev.attr_list = NULL;

	/* 
	 * Now do the attribute list.  From the Dwarf spec (7.5.3)
	 * the list ends when both name and form are encoded as zero.
	 * For sanity, and also to save space, we will not store the 
	 * last (zero) attribute, we'll just set the previous abbrev's
	 * next pointer to NULL.
	 */ 
        att_name = _dw_leb128_to_ulong(dbg);
        att_form = _dw_leb128_to_ulong(dbg);
		
        while (att_name && att_form) 
	{
	    Dwarf_Attrib_Form *tmp = (Dwarf_Attrib_Form*)
		_dw_malloc(dbg, sizeof(Dwarf_Attrib_Form));
	    if (abbrev.attr_list == NULL)
		abbrev.attr_list = tail = tmp;
	    else
		tail = tail->next = tmp;
	    tmp->name = att_name;
	    tmp->form = att_form;
	    tmp->next = NULL;
	    att_name = _dw_leb128_to_ulong(dbg);
	    att_form = _dw_leb128_to_ulong(dbg);
	}

        add_abbrev(dbg, &abbrev, abbrev_table, &table_size, index++);
    }

    /* Throw in a dummy abbrev for att_code == 0 
       FIXME: there must be a better way to do this. */
    _dw_clear((char *) &abbrev, sizeof(Dwarf_Abbrev));
    add_abbrev(dbg, &abbrev, abbrev_table, &table_size, index++);

    sort_abbrev_table(abbrev_table->abbrev_array, index);
    abbrev_table->entry_count = index;
    return(abbrev_table);
}


/****************************************************************************
*
*  ROUTINE: _dw_build_cu_list()
*
*  PURPOSE: Build a skeletal structure for each compilation unit.
*
*  DESCRIPTION: Creates the Dwarf_CU_List chain.  Builds the abbrev table
*	for each CU.  Builds a DIE for each CU and chains the DIEs together
*	as siblings.  
*
*  ARGUMENTS:
*
*  dbg     - To attach the dbg_cu_list.
*
*  RETURN:  NULL is returned on error.
*****************************************************************************/
int
_dw_build_cu_list(dbg)
    Dwarf_Debug dbg;
{
    Dwarf_Section debug_info = _dw_find_section(dbg, ".debug_info");
    Dwarf_Section debug_abbrev = _dw_find_section(dbg, ".debug_abbrev");
    Dwarf_Unsigned sect_ptr; /* running total of the scn length */
    Dwarf_CU_List *tail; /* for chaining CU nodes */
    Dwarf_Die last_cu_die = NULL; /* for chaining CU DIEs */
    long savepos; /* Save current file position within .debug_info */

    RESET_ERR();
    /* get the .debug_info information */
    if ( ! debug_info && LIBDWARF_ERR(DLE_NOINFO) == DLS_ERROR )
	return 0;
    if ( ! debug_abbrev && LIBDWARF_ERR(DLE_NOABBR) == DLS_ERROR )
	return 0;

    sect_ptr = debug_info->file_offset; 

    while (sect_ptr - debug_info->file_offset < debug_info->size) 
    {
	/* While more compilation units */
	Dwarf_CU_List *cu_node = (Dwarf_CU_List*)
	    _dw_malloc(dbg, sizeof(Dwarf_CU_List));
	_dw_clear((char *) cu_node, sizeof(Dwarf_CU_List));

	if (dbg->dbg_cu_list)
	    tail = tail->next = cu_node;
	else
	    dbg->dbg_cu_list = tail = cu_node;

        /* malloc the die_head structure for this CU */
	cu_node->die_head = (Dwarf_Die_Tree*)
	    _dw_malloc(dbg, sizeof(Dwarf_Die_Tree));

	cu_node->die_head->big_endian = debug_info->big_endian;
	cu_node->die_head->file_offset = sect_ptr;
	cu_node->die_head->section_offset = sect_ptr - debug_info->file_offset;
	cu_node->die_head->cu_header =
	    (Dwarf_Info_CU_Hdr*)_dw_malloc(dbg, sizeof(Dwarf_Info_CU_Hdr));
	_dw_fseek(dbg->stream, sect_ptr, SEEK_SET);
	
	/* The Dwarf spec (7.5.1) says that the length field does not
	   include the 4 bytes for the length field. (?)  Add those 
	   4 bytes to our version of the length so we can use it to 
	   find the next compilation unit. */
	cu_node->die_head->cu_header->length = 
	    _dw_read_constant(dbg, 4, debug_info->big_endian) + 4;
	cu_node->die_head->cu_header->version = 
	    _dw_read_constant(dbg, 2, debug_info->big_endian);
	cu_node->die_head->cu_header->abbrev_offset = 
	    _dw_read_constant(dbg, 4, debug_info->big_endian);
	cu_node->die_head->cu_header->target_addr_size = 
	    _dw_read_constant(dbg, 1, debug_info->big_endian);

        /* check version */
	if (!_dw_check_version(cu_node->die_head->cu_header->version))
	    return 0;
 
	/* Save file position within .debug_info */
	savepos = _dw_ftell(dbg->stream);

        /* build the abbrev table for this CU */
	_dw_fseek(dbg->stream, 
		  debug_abbrev->file_offset +
		  cu_node->die_head->cu_header->abbrev_offset, 
		  SEEK_SET);
	cu_node->abbrev_table = 
	    build_abbrev_table(dbg, debug_abbrev);
	
	/* Restore file position within .debug_info */
	_dw_fseek(dbg->stream, savepos, SEEK_SET);

        /* build the CU DIE */
	cu_node->die_head->die_list =
	    _dw_build_a_die(dbg, cu_node, (Dwarf_Die)cu_node->die_head);

	if (cu_node->die_head->die_list->tag != DW_TAG_compile_unit && 
	    LIBDWARF_ERR(DLE_NOT_CU_DIE) == DLS_ERROR)
	    return 0;

	/* Make this new CU DIE the sibling of the previous one */
	if ( last_cu_die )
	{
	    last_cu_die->sibling_ptr = cu_node->die_head->die_list;
	    last_cu_die->has_sib_attr = TRUE;
	}
	last_cu_die = cu_node->die_head->die_list;

	sect_ptr += cu_node->die_head->cu_header->length;
	cu_node->dbg_ptr = dbg;
    }
    return 1;
}

/*
 * assign_attr_value -- Sets up the at_value field for each attribute 
 * for each DIE.
 * 
 * NOTICE I'm treating what the spec calls 'references' and 'constants'
 * pretty much the same, this may have to be changed later.
 * 
 * The deal here is, for references and constant forms, if the attribute
 * implies that its value should be a loction expression, then we go to 
 * the .debug_loc section and read a location list.  For block forms with
 * location expression values, the information is in .debug_info and it 
 * just has to be built up.
 *
 * RETURNS: 1 if successful 0 if not.
 */
static int
assign_attr_value(dbg, cu_node, attr, byte_order)
    Dwarf_Debug dbg;
    Dwarf_CU_List *cu_node;
    Dwarf_Attribute attr;
    Dwarf_Bool  byte_order;
{
    int constant_size = 0;

    RESET_ERR();	

    switch(attr->at_form) 
    {
    case 0:
	if (LIBDWARF_ERR(DLE_NO_FORM) == DLS_ERROR)
	    return(0);
	break; 
    case DW_FORM_data1:
    case DW_FORM_ref1:
    case DW_FORM_flag:
	constant_size = 1;
	break;
    case DW_FORM_data2:
    case DW_FORM_ref2:
	constant_size = 2;
	break;
    case DW_FORM_data4:
    case DW_FORM_ref4:
    case DW_FORM_strp:		/* this is a 4-byte offset into the strtab */
    case DW_FORM_addr:
    case DW_FORM_ref_addr:
	constant_size = 4;
	break;
    case DW_FORM_data8:
    case DW_FORM_ref8:
	constant_size = 8;
	break;
    case DW_FORM_string:
	attr->val_type = DLT_STRING;
	attr->at_value.val_string = 
	    _dw_build_a_string(dbg);
	break;
    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
    case DW_FORM_block:	
	if(check_for_location(attr))
	    assign_attr_location(dbg, attr, byte_order);
	else
	    assign_attr_block(dbg, attr, byte_order);
	break;
    case DW_FORM_udata:
    case DW_FORM_ref_udata:
	if(check_for_location(attr))
	    assign_attr_location_list(dbg, cu_node, attr, byte_order);
	else 
	    assign_attr_leb128(dbg, attr, 0);
	break;
    case DW_FORM_sdata:
	if(check_for_location(attr))
	    assign_attr_location_list(dbg, cu_node, attr, byte_order);
	else 
	    assign_attr_leb128(dbg, attr, 1);
	break;
    case DW_FORM_indirect:
	/* Goofy one.  This form is actually contained as an LEB128 number
	   as the first entry in the DIE in the .debug_info section.  So,
	   we'll get the form, overwrite the DW_FORM_indirect with the real 
	   form, then call ourselves recursively to assign the attribute 
	   structures. */
	attr->at_form = _dw_leb128_to_ulong(dbg);
	assign_attr_value(dbg, cu_node, attr, byte_order); 
	break;
    default:
	if(LIBDWARF_ERR(DLE_UNSUPPORTED_FORM) == DLS_ERROR)
	    return(0); 
	break;
    }
    if (constant_size)
    {
	if(check_for_location(attr))
	    assign_attr_location_list(dbg, cu_node, attr, byte_order);
	else if (constant_size <= 4)
	{
	    attr->val_type = DLT_UNSIGNED;
	    attr->at_value.val_unsigned =
		_dw_read_constant(dbg, constant_size, byte_order);
	}
	else
	{
	    attr->val_type = DLT_BIGNUM;
	    attr->at_value.val_bignum =
		_dw_read_large_constant(dbg, constant_size, byte_order);
	}
    }
    return(1);
}

/* 
 * assign_attr_block
 *
 * helper function for assign_attr_value.  Builds a DLT_BLOCK value
 * and attaches it to the given attribute.
 *
 */
static void
assign_attr_block(dbg, attr, byte_order)
    Dwarf_Debug dbg;
    Dwarf_Attribute attr;
    int byte_order;
{
    Dwarf_Unsigned formlen;  /* the form that gives you the block length */
    Dwarf_Unsigned blocklen;
    
    attr->val_type = DLT_BLOCK;
    attr->at_value.val_block = 
	(Dwarf_Block) _dw_malloc(dbg, sizeof(struct Dwarf_Block));
    if ( attr->at_form == DW_FORM_block )
    {
	blocklen = _dw_leb128_to_ulong(dbg);
    }
    else
    {
	switch ( attr->at_form )
	{
	case DW_FORM_block1:
	    formlen = 1;
	    break;
	case DW_FORM_block2:
	    formlen = 2;
	    break;
	case DW_FORM_block4:
	    formlen = 4;
	    break;
	}
	blocklen = _dw_read_constant(dbg, formlen, byte_order);
    }
    attr->at_value.val_block->bl_len = blocklen;
    attr->at_value.val_block->bl_data = (Dwarf_Addr)
	_dw_malloc(dbg, blocklen);
    _dw_fread(attr->at_value.val_block->bl_data, blocklen, 1, dbg->stream);
}

/* 
 * assign_attr_location
 *
 * helper function for assign_attr_value.  Builds a DLT_LOCLIST value
 * with only one location expression in the list.  Sets lopc to 0;
 * Sets hipc to UINT_MAX (fills DW_FORM_addr with 1 bits).  This creates
 * the impression to the client that the location is valid at any
 * address.
 */
static void
assign_attr_location(dbg, attr, byte_order)
    Dwarf_Debug dbg;
    Dwarf_Attribute attr;
    int byte_order;
{
    Dwarf_Unsigned formlen; /* the form that gives you the block length */
    Dwarf_Unsigned blocklen;
    
    attr->val_type = DLT_LOCLIST;
    attr->at_value.val_loclist =
	(Dwarf_Locdesc) _dw_malloc(dbg, sizeof(struct Dwarf_Locdesc));

    if ( attr->at_form == DW_FORM_block )
    {
	blocklen = _dw_leb128_to_ulong(dbg);
    }
    else
    {
	switch ( attr->at_form )
	{
	case DW_FORM_block1:
	    formlen = 1;
	    break;
	case DW_FORM_block2:
	    formlen = 2;
	    break;
	case DW_FORM_block4:
	    formlen = 4;
	    break;
	}
	blocklen = _dw_read_constant(dbg, formlen, byte_order);
    }

    attr->at_value.val_loclist->ld_lopc = 0;
    attr->at_value.val_loclist->ld_hipc = (Dwarf_Addr) 0xffffffff;
    attr->at_value.val_loclist->next = NULL;
    attr->at_value.val_loclist->ld_loc = 
	build_location_expression(dbg, blocklen, byte_order); 
}

/* 
 * assign_attr_location_list
 *
 * helper function for assign_attr_value.  Builds a DLT_LOCLIST value
 * with any number of location expressions in the list.  Fields lopc and hipc
 * are as found in the Dwarf file.
 *
 * NOTE: In the Dwarf spec, 2.4.6, it says that the beginning and ending
 * addresses are relative to the base address of the compilation unit
 * that references the location list.  That's why we're passing in a cu_node
 * so that this value can get added in and the client won't have to be
 * bothered with knowing whether a location list is CU-relative or not.
 */
static void
assign_attr_location_list(dbg, cu_node, attr, byte_order)
    Dwarf_Debug dbg;
    Dwarf_CU_List *cu_node;
    Dwarf_Attribute attr;
    int byte_order;
{
    int formlen;    /* size of the form that gives you the block length */
    int blocklen;
    int firsttime = 1;	/* loop terminator */
    long savepos;   /* current (.debug_info) position in file */
    Dwarf_Section section = _dw_find_section(dbg, ".debug_loc");
    Dwarf_Bool	loc_byte_order;
    Dwarf_Off	loc_offset;
    Dwarf_Die	cu_die;
    Dwarf_Unsigned  cu_base_addr;   /* base address of referencing CU */
    Dwarf_Locdesc   locdesc_root;   /* this will be the attribute value */
    Dwarf_Locdesc   locdesc;	    /* currently working on this one */

    if ( ! section )
    {
	if ( LIBDWARF_ERR(DLE_NOLOC) == DLS_ERROR )
	return; 
    }

    /* Get an integer offset into .debug_loc from which to get
       location expressions. */
    if ( attr->at_form == DW_FORM_udata || attr->at_form == DW_FORM_ref_udata )
    {
	loc_offset = (Dwarf_Off) _dw_leb128_to_ulong(dbg);
    }
    else if ( attr->at_form == DW_FORM_sdata )
    {
	loc_offset = (Dwarf_Off) _dw_leb128_to_long(dbg);
    }
    else
    {
	switch ( attr->at_form )
	{
	case DW_FORM_data1:
	case DW_FORM_ref1:
	case DW_FORM_flag:
	    formlen = 1;
	    break;
	case DW_FORM_data2:
	case DW_FORM_ref2:
	    formlen = 2;
	    break;
	case DW_FORM_data4:
	case DW_FORM_ref4:
	case DW_FORM_addr:
	case DW_FORM_ref_addr:
	    formlen = 4;
	    break;
	}
	loc_offset = _dw_read_constant(dbg, formlen, byte_order);
    }

    savepos = _dw_ftell(dbg->stream);
    _dw_fseek(dbg->stream, section->file_offset + loc_offset, SEEK_SET);
    loc_byte_order = section->big_endian;

    /* Malloc a new location description.  There will always be 
       at least one, even if its location expression is empty. */
    locdesc = locdesc_root = 
	(Dwarf_Locdesc) _dw_malloc(dbg, sizeof(struct Dwarf_Locdesc));
    _dw_clear((char *) locdesc, sizeof(struct Dwarf_Locdesc));
    locdesc->ld_hipc = (Dwarf_Addr) 0xffffffff;
    attr->val_type = DLT_LOCLIST;
    attr->at_value.val_loclist = locdesc_root;
    
    /* Figure out the compilation unit base address. */
    cu_die = cu_node->die_head->die_list;
    cu_base_addr = DIE_ATTR_UNSIGNED(cu_die, DW_AT_low_pc);

    while ( 1 )
    {
	/* In the Dwarf spec (7.7.2) the form of the bounding addresses 
	   is defined to be DW_FORM_addr (we are defaulting to 4 bytes).
	   The size of the block length for each location expression 
	   is defined to be 2 bytes. */
	Dwarf_Unsigned lopc, hipc;
	lopc = _dw_read_constant(dbg, 4, loc_byte_order);
	hipc = _dw_read_constant(dbg, 4, loc_byte_order);

	if ( lopc == 0 && hipc == 0 )
	    break;

	/* Add compilation unit base address before filling in locdesc. */
	lopc += cu_base_addr;
	hipc += cu_base_addr;

	/* Malloc a new location description, except for the first
	   time through (sigh) */
	if ( firsttime )
	    firsttime = 0;
	else
	{
	    locdesc->next =
		(Dwarf_Locdesc) _dw_malloc(dbg, sizeof(struct Dwarf_Locdesc));
	    locdesc = locdesc->next;
	}

	/* Get block length of this location expression */
	blocklen = _dw_read_constant(dbg, 2, loc_byte_order);

	locdesc->next    = 0;
	locdesc->ld_lopc = (Dwarf_Addr) lopc;
	locdesc->ld_hipc = (Dwarf_Addr) hipc;
	locdesc->ld_loc = 
	    build_location_expression(dbg, blocklen, loc_byte_order); 
    }
    /* Restore (.debug_info) file pointer */
    _dw_fseek(dbg->stream, savepos, SEEK_SET);
}

/*
 * assign_attr_leb128
 *
 * Helper function for assign_attr_value.  Reads an leb128 when the size
 * of the resulting number is unknown.  Builds a result that is either
 * DLT_UNSIGNED or DLT_BIGNUM, depending on the ultimate size of the input.
 *
 * FIXME: There's an arbitrary limit of 3 words (12 bytes).  Hence the weird
 * limit of 14 "tidbits" in the code below.  There really should be no limit 
 * to the size of the number that can be represented.
 */
static void
assign_attr_leb128(dbg, attr, rtn_val_signed)
    Dwarf_Debug dbg;
    Dwarf_Attribute attr;
    Dwarf_Bool rtn_val_signed;
{
    Dwarf_Unsigned tidbit[14];	/* 7-bit "bytes" used by leb128s */
    Dwarf_Unsigned result[3];	/* We are currently handling at most 3 words */
    Dwarf_Bool negative = 0;	/* Must read to the end to find out */
    int tidbit_count = 0;
    Dwarf_Unsigned buf = 0xff;
    int t, r, shift;

    for (tidbit_count = 0; 
	 tidbit_count < 14 && buf & 0x80;
	 tidbit_count++)
    {
	unsigned char tmp[1];
	_dw_fread(tmp, 1, 1, dbg->stream);
    	buf = tmp[0];
	tidbit[tidbit_count] = buf & 0x7f;
    }

    if (tidbit_count == 14 && (buf & 0x80))
    {
	/* Can't represent this one; probably the byte counter is off */
	LIBDWARF_ERR(DLE_HUGE_LEB128);
    }

    if (rtn_val_signed && tidbit[tidbit_count - 1] & 0x40)
    {
	negative = 1;
    }

    for (t = r = 0, result[0] = result[1] = result[2] = 0;
	 t < tidbit_count;
	 t++)
    {
	shift = (t * 7) % 32;
	result[r] |= tidbit[t] << shift;
	if ((32 - shift) <= 7)
	    result[++r] |= tidbit[t] >> (32 - shift);
    }

    /* If negative, continue shifting algorithm, but shift in 1 bits
       to sign extend to maximum possible size. */
    if (negative)
    {
	for ( ; t < 14; ++t )
	{
	    shift = (t * 7) % 32;
	    result[r] |= 0x7f << shift;
	    if ((32 - shift) <= 7)
		result[++r] |= 0x7f >> (32 - shift);
	}
    }

    if (negative && result[1] == 0xffffffff && result[2] == 0xffffffff
	|| ! negative && result[1] == 0 && result[2] == 0)
    {
	/* Used only one word.  Return result as an integer. */
	if (rtn_val_signed)
	{
	    attr->val_type = DLT_SIGNED;
	    attr->at_value.val_signed = result[0];
	}
	else
	{
	    attr->val_type = DLT_UNSIGNED;
	    attr->at_value.val_unsigned = result[0];
	}
    }
    else
    {
	/* Used more than one word.  Store result in a bignum. */
	Dwarf_Bignum bignum = (Dwarf_Bignum)
	    _dw_malloc(dbg, sizeof(struct Dwarf_Bignum));
	bignum->words = (Dwarf_Unsigned *) _dw_malloc(dbg, 12);
	bignum->words[0] = result[0];
	bignum->words[1] = result[1];
	bignum->words[2] = result[2];
	attr->val_type = DLT_BIGNUM;
	attr->at_value.val_bignum = bignum;
    }
}

/*
 * check_for_location -- Checks to see if the given form 
 * is actually a location based on the name.
 * 
 * If any new attributes are added that can be locations
 * remember to add them to this list!
 */
static int
check_for_location(attr_list)
    Dwarf_Attribute attr_list;
{
    switch(attr_list->at_name) {
	case DW_AT_segment:
	case DW_AT_location:
	case DW_AT_frame_base:
	case DW_AT_return_addr:
	case DW_AT_static_link:
	case DW_AT_use_location:
	case DW_AT_vtable_elem_location:
	case DW_AT_data_member_location:
	    return(1);
	default:
	    return(0);
    }
}

/*
 * build_location_expression
 *
 * helper function for assign_attr_value
 * Builds and returns a linked list of struct Dwarf_Loc.  The return value
 * represents one location expression.
 */
static
Dwarf_Loc
build_location_expression(dbg, length, byte_order)
    Dwarf_Debug dbg;
    Dwarf_Unsigned length;
    Dwarf_Bool byte_order;
{
    int bytecount = 0;
    long savepos;
    Dwarf_Loc	loc_root = NULL; /* return this */
    Dwarf_Loc	loc_tail; /* place-holder */
    Dwarf_Loc	loc; /* currently working on this location */

    RESET_ERR();
    while ( bytecount < length )
    {
	savepos = _dw_ftell(dbg->stream);
	loc = (Dwarf_Loc) _dw_malloc(dbg, sizeof(struct Dwarf_Loc));
	_dw_clear((char *) loc, sizeof(struct Dwarf_Loc));

	if ( loc_root == NULL )
	    loc_root = loc_tail = loc;
	else
	    loc_tail = loc_tail->next = loc;

	loc->lr_operation = _dw_read_constant(dbg, 1, byte_order);

    	switch ( loc->lr_operation )
	{
	/* 
	 * operations with 0 operands 
	 */   				
	case DW_OP_deref:
	case DW_OP_dup:
	case DW_OP_drop:
	case DW_OP_over:
	case DW_OP_swap:
	case DW_OP_rot:
	case DW_OP_xderef:
	case DW_OP_abs:
	case DW_OP_and:
	case DW_OP_div:
	case DW_OP_minus:
	case DW_OP_mod:
	case DW_OP_mul:
	case DW_OP_neg:
	case DW_OP_not:
	case DW_OP_or:
	case DW_OP_plus:
	case DW_OP_shl:
	case DW_OP_shr:
	case DW_OP_shra:
	case DW_OP_xor:
	case DW_OP_eq:
	case DW_OP_ge:
	case DW_OP_gt:
	case DW_OP_le:
	case DW_OP_lt:
	case DW_OP_ne:
	case DW_OP_lit0:
	case DW_OP_lit1:
	case DW_OP_lit2:
	case DW_OP_lit3:
	case DW_OP_lit4:
	case DW_OP_lit5:
	case DW_OP_lit6:
	case DW_OP_lit7:
	case DW_OP_lit8:
	case DW_OP_lit9:
	case DW_OP_lit10:
	case DW_OP_lit11:
	case DW_OP_lit12:
	case DW_OP_lit13:
	case DW_OP_lit14:
	case DW_OP_lit15:
	case DW_OP_lit16:
	case DW_OP_lit17:
	case DW_OP_lit18:
	case DW_OP_lit19:
	case DW_OP_lit20:
	case DW_OP_lit21:
	case DW_OP_lit22:
	case DW_OP_lit23:
	case DW_OP_lit24:
	case DW_OP_lit25:
	case DW_OP_lit26:
	case DW_OP_lit27:
	case DW_OP_lit28:
	case DW_OP_lit29:
	case DW_OP_lit30:
	case DW_OP_lit31:
	case DW_OP_reg0:
	case DW_OP_reg1:
	case DW_OP_reg2:
	case DW_OP_reg3:
	case DW_OP_reg4:
	case DW_OP_reg5:
	case DW_OP_reg6:
	case DW_OP_reg7:
	case DW_OP_reg8:
	case DW_OP_reg9:
	case DW_OP_reg10:
	case DW_OP_reg11:
	case DW_OP_reg12:
	case DW_OP_reg13:
	case DW_OP_reg14:
	case DW_OP_reg15:
	case DW_OP_reg16:
	case DW_OP_reg17:
	case DW_OP_reg18:
	case DW_OP_reg19:
	case DW_OP_reg20:
	case DW_OP_reg21:
	case DW_OP_reg22:
	case DW_OP_reg23:
	case DW_OP_reg24:
	case DW_OP_reg25:
	case DW_OP_reg26:
	case DW_OP_reg27:
	case DW_OP_reg28:
	case DW_OP_reg29:
	case DW_OP_reg30:
	case DW_OP_reg31:
	case DW_OP_nop:
	    break;

	/* 
	 * operations with 1 operand 
	 */
	case DW_OP_const1u:
	case DW_OP_const1s:
	case DW_OP_pick:
	case DW_OP_deref_size:
	case DW_OP_xderef_size:
	    /* These have a fixed-size (1 byte) operand */
	    loc->lr_operand1 = _dw_read_constant(dbg, 1, byte_order);
	    break;
	case DW_OP_const2u:
	case DW_OP_const2s:
	case DW_OP_skip:
	case DW_OP_bra:
	    /* These have a fixed-size (2 byte) operand */
	    loc->lr_operand1 = _dw_read_constant(dbg, 2, byte_order);
	    break;
	case DW_OP_addr:
	case DW_OP_const4u:
	case DW_OP_const4s:
	    /* These have a fized-size (4 byte) operand */
	    loc->lr_operand1 = _dw_read_constant(dbg, 4, byte_order);
	    break;
	case DW_OP_constu:
	case DW_OP_plus_uconst:
	case DW_OP_regx:
	case DW_OP_piece:
	    /* These take an unsigned LEB128 operand */
	    loc->lr_operand1 = _dw_leb128_to_ulong(dbg);
	    break;
	case DW_OP_consts:
	case DW_OP_fbreg:
	case DW_OP_breg0:
	case DW_OP_breg1:
	case DW_OP_breg2:
	case DW_OP_breg3:
	case DW_OP_breg4:
	case DW_OP_breg5:
	case DW_OP_breg6:
	case DW_OP_breg7:
	case DW_OP_breg8:
	case DW_OP_breg9:
	case DW_OP_breg10:
	case DW_OP_breg11:
	case DW_OP_breg12:
	case DW_OP_breg13:
	case DW_OP_breg14:
	case DW_OP_breg15:
	case DW_OP_breg16:
	case DW_OP_breg17:
	case DW_OP_breg18:
	case DW_OP_breg19:
	case DW_OP_breg20:
	case DW_OP_breg21:
	case DW_OP_breg22:
	case DW_OP_breg23:
	case DW_OP_breg24:
	case DW_OP_breg25:
	case DW_OP_breg26:
	case DW_OP_breg27:
	case DW_OP_breg28:
	case DW_OP_breg29:
	case DW_OP_breg30:
	case DW_OP_breg31:
	    /* These take a signed LEB128 operand */
	    loc->lr_operand1 = _dw_leb128_to_long(dbg);
	    break;

	/* 
	 * operations with 2 operands 
	 */   				
	case DW_OP_bregx:
	    /* First operand identifies the base register:
	       it is an unsigned LEB128 */
	    loc->lr_operand1 = _dw_leb128_to_ulong(dbg);
	    /* Second operand is an offset to be added to the contents 
	       of the base register: it is a signed LEB128 */
	    loc->lr_operand2 = _dw_leb128_to_long(dbg);
	    break;

	/*
	 * error cases 
	 */
	case DW_OP_const8u:
	case DW_OP_const8s:
	    /* We don't know how to deal with these */
	    if ( LIBDWARF_ERR(DLE_BAD_LOC) == DLS_ERROR )
		return loc_root;
	    else 
		bytecount += 8;
	    break;
	default:
	    if ( LIBDWARF_ERR(DLE_BAD_LOC) == DLS_ERROR )
		return loc_root;
	    break;
	}
	bytecount += _dw_ftell(dbg->stream) - savepos;
    }
    return loc_root;
}

