/*****************************************************************************
 * Copyright (c) 1990, 1991, 1992, 1993 Intel Corporation
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
 ***************************************************************************/

/* This file contains functions to handle pretty-printing of attribute values.
   To "pretty print" means to look up a string from one of the association
   lists, rather than just printing a number.  Note that there should be
   a function here for each DW_AT_* constant, including user-defined 
   attributes.
*/

#include "dwarf2.h"
#include "libdwarf.h"
#include "dwarfdmp.h"
#include "ops.h"

/* Forward declarations of static functions so we don't have to worry
   about ordering within this file.  */
void print_location_value PARAMS((Dwarf_Debug, Dwarf_Attribute));

/* 
  Name: abstract_origin 
  Classes: REFERENCE
*/
void print_abstract_origin_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: accessibility 
  Classes: CONSTANT
*/
void print_accessibility_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    Dwarf_Unsigned uval;
    Dwarf_Signed sval;
    Dwarf_Small form = attribute->at_form;
    
    switch(form)
    {
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_udata:
        uval = attribute->at_value.val_unsigned;
        sprintf(outbuf.dp, "%s", lookup_access_name(uval)); move_dp;
        break;
    case DW_FORM_sdata:
        sval = attribute->at_value.val_signed;
        sprintf(outbuf.dp, "%s", lookup_access_name((long)sval)); move_dp;
        break;
    default:
        print_default_attribute_value(dbg, attribute);
    }
}

/* 
  Name: address_class 
  Classes: CONSTANT
*/
void print_address_class_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: artificial 
  Classes: FLAG
  Notes: Attributes that take FLAG forms are handled in the default
  attribute printer.
*/
void print_artificial_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: base_types 
  Classes: REFERENCE
*/
void print_base_types_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: bit_offset 
  Classes: CONSTANT
*/
void print_bit_offset_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: bit_size 
  Classes: CONSTANT
*/
void print_bit_size_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: byte_size 
  Classes: CONSTANT
*/
void print_byte_size_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: calling_convention 
  Classes: CONSTANT
*/
void print_calling_convention_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    Dwarf_Unsigned uval;
    Dwarf_Signed sval;
    Dwarf_Small form = attribute->at_form;
    
    switch(form)
    {
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_udata:
        uval = attribute->at_value.val_unsigned;
        sprintf(outbuf.dp, "%s", lookup_callingconvention_name(uval)); move_dp;
        break;
    case DW_FORM_sdata:
        sval = attribute->at_value.val_signed;
        sprintf(outbuf.dp, "%s", lookup_callingconvention_name((long)sval)); 
        move_dp;
        break;
    default:
        print_default_attribute_value(dbg, attribute);
    }
}

/* 
  Name: common_reference 
  Classes: REFERENCE
*/
void print_common_reference_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: comp_dir 
  Classes: STRING
*/
void print_comp_dir_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: const_value 
  Classes: STRING, CONSTANT, BLOCK
*/
void print_const_value_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: containing_type 
  Classes: REFERENCE
*/
void print_containing_type_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: count 
  Classes: CONSTANT, REFERENCE
*/
void print_count_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: data_member
  Classes: BLOCK,CONSTANT
  Notes: Value is location list or location expression. 
  See comment at print_location_value for more info.
*/
void print_data_member_location_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_location_value(dbg, attribute);
}

/* 
  Name: decl_column 
  Classes: CONSTANT
*/
void print_decl_column_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: decl_file 
  Classes: CONSTANT
*/
void print_decl_file_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: decl_line 
  Classes: CONSTANT
*/
void print_decl_line_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: declaration 
  Classes: FLAG
  Notes: Attributes that take FLAG forms are handled in the default
  attribute printer.
*/
void print_declaration_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: default_value 
  Classes: REFERENCE
*/
void print_default_value_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: discr 
  Classes: REFERENCE
*/
void print_discr_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: discr_list 
  Classes: BLOCK
*/
void print_discr_list_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: discr_value 
  Classes: CONSTANT
*/
void print_discr_value_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: encoding
  Classes: CONSTANT
*/
void print_encoding_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    Dwarf_Unsigned uval;
    Dwarf_Signed sval;
    Dwarf_Small form = attribute->at_form;
    
    switch(form)
    {
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_udata:
        uval = attribute->at_value.val_unsigned;
        sprintf(outbuf.dp, "%s", lookup_basetype_name(uval)); move_dp;
        break;
    case DW_FORM_sdata:
        sval = attribute->at_value.val_signed;
        sprintf(outbuf.dp, "%s", lookup_basetype_name((long) sval)); move_dp;
        break;
    default:
        print_default_attribute_value(dbg, attribute);
    }
}

/* 
  Name: external 
  Classes: FLAG
  Notes: Attributes that take FLAG forms are handled in the default
  attribute printer.
*/
void print_external_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: frame_base 
  Classes: BLOCK,CONSTANT
  Notes: Value is location list or location expression. 
  See comment at print_location_value for more info.
*/
void print_frame_base_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_location_value(dbg, attribute);
}

/* 
  Name: friend 
  Classes: REFERENCE
*/
void print_friend_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: high_pc 
  Classes: ADDRESS
*/
void print_high_pc_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: identifier_case 
  Classes: CONSTANT
*/
void print_identifier_case_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    Dwarf_Unsigned uval;
    Dwarf_Signed sval;
    Dwarf_Small form = attribute->at_form;
    
    switch(form)
    {
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_udata:
        uval = attribute->at_value.val_unsigned;
        sprintf(outbuf.dp, "%s", lookup_idcase_name(uval)); move_dp;
        break;
    case DW_FORM_sdata:
        sval = attribute->at_value.val_signed;
        sprintf(outbuf.dp, "%s", lookup_idcase_name((long) sval)); move_dp;
        break;
    default:
        print_default_attribute_value(dbg, attribute);
    }
}

/* 
  Name: import 
  Classes: REFERENCE
*/
void print_import_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: inline 
  Classes: CONSTANT
*/
void print_inline_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    Dwarf_Unsigned uval;
    Dwarf_Signed sval;
    Dwarf_Small form = attribute->at_form;

    switch(form)
    {
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_udata:
        uval = attribute->at_value.val_unsigned;
        sprintf(outbuf.dp, "%s", lookup_inlining_name(uval)); move_dp;
        break;
    case DW_FORM_sdata:
        sval = attribute->at_value.val_signed;
        sprintf(outbuf.dp, "%s", lookup_inlining_name((long) sval)); move_dp;
        break;
    default:
        print_default_attribute_value(dbg, attribute);
    }
}

/* 
  Name: is_optional 
  Classes: FLAG
  Notes: Attributes that take FLAG forms are handled in the default
  attribute printer.
*/
void print_is_optional_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: language 
  Classes: CONSTANT
*/
void print_language_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    Dwarf_Unsigned uval;
    Dwarf_Signed sval;
    Dwarf_Small form = attribute->at_form;

    switch(form)
    {
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_udata:
        uval = attribute->at_value.val_unsigned;
        sprintf(outbuf.dp, "%s", lookup_language_name(uval)); move_dp;
        break;
    case DW_FORM_sdata:
        sval = attribute->at_value.val_signed;
        sprintf(outbuf.dp, "%s", lookup_language_name((long) sval)); move_dp;
        break;
    default:
        print_default_attribute_value(dbg, attribute);
    }
}

/* print_location_operator - Helper function for print_location_expression
        that prints operators and their operands (if they have any).
*/
void print_location_operator(loc)
    Dwarf_Loc loc;
{
    Dwarf_Small operator = loc->lr_operation;
    struct loc_op *op;
    
    op = &optable[operator];
    sprintf(outbuf.dp, "%s", lookup_operation_name(operator)); move_dp;

    if ( op->numops ) 
    {
        /* There was at least one operand.  The operation has been printed
           already.  Print the operand(s) in whatever format is appropriate
           from the context.  For now, there are only 3 categories:
           (1) hex address, (2) signed decimal, and (3) unsigned decimal.
           If you add a user-defined operation, you may want to special-
           case its printing in the switch statement below. */
        switch ( operator )
        {
        case DW_OP_addr:
            sprintf(outbuf.dp, "(0x%x)", loc->lr_operand1); move_dp;
            break;            
        case DW_OP_bregx:
            /* 2-operand oddball - first operand unsigned, second signed. */
            sprintf(outbuf.dp, "(%u,%d)", loc->lr_operand1, (long) loc->lr_operand2 );
            move_dp;
            break;
        /* Add user-defined operations here, if desired */
        default:
            if ( op->sign )
            {
                sprintf(outbuf.dp, "(%d)", (long) loc->lr_operand1); move_dp;
            }
            else
            {
                sprintf(outbuf.dp, "(%u)", loc->lr_operand1); move_dp;
            }
            break;    
        } /* end switch */
    } /* end if (one or more operands) */
    else
    {
        sprintf(outbuf.dp, " "); move_dp;
    }
}

/* print_location_expression - Helper function for print_location_value
        and print_location_list.  Print each location operation, and each 
        operand, on a separate line, in human-readable form.  */
void print_location_expression(locdesc)
    Dwarf_Locdesc locdesc; 
{
    Dwarf_Loc loc;

    for ( loc = locdesc->ld_loc; loc; loc = loc->next )            
        print_location_operator(loc);
}

/* print_location_list - Helper function for print_location_value.  */
void print_location_list(locdesc)
    Dwarf_Locdesc locdesc;
{
    for ( ; locdesc; locdesc = locdesc->next) {
        sprintf(outbuf.dp, "0x%x-0x%x: ", locdesc->ld_lopc, locdesc->ld_hipc); 
        move_dp;
        print_location_expression(locdesc);
    }
}

/* 
  Name: location 
  Classes: BLOCK, CONSTANT
  Notes: The way to tell the difference between a location expression
        and a location list is by the form of the location attribute.
        If BLOCK, then it's a location expression immediately following.
        If CONSTANT, the constant represents the offset into the .debug_loc
        section where the location list begins.  (this is my interpretation
        of the TIS Dwarf 2 spec, 2.4.6)
*/
void print_location_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    Dwarf_Small form = attribute->at_form;

    if (attribute->val_type != DLT_LOCLIST) {
        printf("Error: value is not a location description\n");
        return;
    }

    switch(form)
    {
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_udata:
    case DW_FORM_sdata:
    case DW_FORM_addr:
    case DW_FORM_ref1:
    case DW_FORM_ref2:
    case DW_FORM_ref4:
    case DW_FORM_ref8:
    case DW_FORM_ref_udata:
    case DW_FORM_ref_addr:
        print_location_list(attribute->at_value.val_loclist);
        break;
    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
    case DW_FORM_block:
        print_location_expression(attribute->at_value.val_loclist);
        break;
    default:
        print_default_attribute_value(dbg, attribute);
    }
}

/* 
  Name: low_pc 
  Classes: ADDRESS
*/
void print_low_pc_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: lower_bound 
  Classes: CONSTANT, REFERENCE
*/
void print_lower_bound_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: macro_info 
  Classes: CONSTANT
*/
void print_macro_info_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: name 
  Classes: STRING
  NOTES: We're going to intercept this attribute and append it to the
  "header" buffer of the outbuf, to save space and make the dump more useful.
*/
void print_name_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    char *die_name = attribute->at_value.val_string;;
    
    if ( strlen(die_name) > outbuf.namelen - 1 )
    {
        free(outbuf.name);
        outbuf.namelen = strlen(die_name) + 32;
        outbuf.name = (char *) malloc(outbuf.namelen);
    }
    strcpy(outbuf.name, die_name);
}

/* 
  Name: namelist_item 
  Classes: BLOCK 
*/
void print_namelist_item_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: ordering 
  Classes: CONSTANT
*/
void print_ordering_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    Dwarf_Unsigned uval;
    Dwarf_Signed sval;
    Dwarf_Small form = attribute->at_form;
    
    switch(form)
    {
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_udata:
        uval = attribute->at_value.val_unsigned;
        sprintf(outbuf.dp, "%s", lookup_ordering_name(uval)); move_dp;
        break;
    case DW_FORM_sdata:
        sval = attribute->at_value.val_signed;
        sprintf(outbuf.dp, "%s", lookup_ordering_name((long) sval)); move_dp;
        break;
    default:
        print_default_attribute_value(dbg, attribute);
    }
}

/* 
  Name: priority 
  Classes: REFERENCE
*/
void print_priority_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: producer 
  Classes: STRING
*/
void print_producer_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: prototyped 
  Classes: FLAG
  Notes: Attributes that take FLAG forms are handled in the default
  attribute printer.
*/
void print_prototyped_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: return_addr
  Classes: BLOCK,CONSTANT
  Notes: Value is location list or location expression. 
  See comment at print_location_value for more info.
*/
void print_return_addr_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_location_value(dbg, attribute);
}

/* 
  Name: segment 
  Classes: BLOCK,CONSTANT
  Notes: Value is location list or location expression. 
  See comment at print_location_value for more info.
*/
void print_segment_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_location_value(dbg, attribute);
}

/* 
  Name: sibling 
  Classes: REFERENCE
  NOTES: We're going to intercept this attribute and write it into the
  "sibling" buffer of the outbuf, to save space and make the dump more useful.
*/
void print_sibling_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    Dwarf_Unsigned ulong;
    Dwarf_Small form = attribute->at_form;

    switch ( form )
    {
    case DW_FORM_ref1:
    case DW_FORM_ref2:
    case DW_FORM_ref4:
    case DW_FORM_ref_udata:
        ulong = attribute->at_value.val_unsigned;
        sprintf(outbuf.sibling, "C.U. + 0x%x", ulong);
        break;
    case DW_FORM_ref_addr:
        /* The ref_addr form is an offset from the start of the
           FILE, not from the start of the compilation unit. */
        ulong = attribute->at_value.val_unsigned;
        sprintf(outbuf.sibling, "0x%x", ulong);
        break;
    default:
        /* don't think a sibling will ever be a DW_FORM_ref8, but 
           weirder things have been known to happen */
        print_default_attribute_value(dbg, attribute);
    }
}

/* 
  Name: specification 
  Classes: REFERENCE
*/
void print_specification_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: start_scope 
  Classes: CONSTANT
*/
void print_start_scope_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: static_link
  Classes: BLOCK,CONSTANT
  Notes: Value is location list or location expression. 
  See comment at print_location_value for more info.
*/
void print_static_link_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_location_value(dbg, attribute);
}

/* 
  Name: stmt_list 
  Classes: CONSTANT
*/
void print_stmt_list_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: stride_size 
  Classes: CONSTANT
*/
void print_stride_size_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: string_length 
  Classes: BLOCK, CONSTANT
*/
void print_string_length_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: type 
  Classes: TYPE
*/
void print_type_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: upper_bound 
  Classes: CONSTANT, REFERENCE
*/
void print_upper_bound_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: use_location
  Classes: BLOCK,CONSTANT
  Notes: Value is location list or location expression. 
  See comment at print_location_value for more info.
*/
void print_use_location_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_location_value(dbg, attribute);
}

/* 
  Name: variable_parameter 
  Classes: FLAG
  Notes: Attributes that take FLAG forms are handled in the default
  attribute printer.
*/
void print_variable_parameter_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

/* 
  Name: virtuality 
  Classes: CONSTANT
*/
void print_virtuality_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    Dwarf_Unsigned uval;
    Dwarf_Signed sval;
    Dwarf_Small form = attribute->at_form;
    
    switch(form)
    {
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_udata:
        uval = attribute->at_value.val_unsigned;
        sprintf(outbuf.dp, "%s", lookup_virtuality_name(uval)); move_dp;
        break;
    case DW_FORM_sdata:
        sval = attribute->at_value.val_signed;
        sprintf(outbuf.dp, "%s", lookup_virtuality_name((long)sval)); move_dp;
        break;
    default:
        print_default_attribute_value(dbg, attribute);
    }
}

/* 
  Name: visibility 
  Classes: CONSTANT
*/
void print_visibility_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    Dwarf_Unsigned uval;
    Dwarf_Signed sval;
    Dwarf_Small form = attribute->at_form;
 
    switch(form)
    {
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_udata:
        uval = attribute->at_value.val_unsigned;
        sprintf(outbuf.dp, "%s", lookup_visibility_name(uval)); move_dp;
        break;
    case DW_FORM_sdata:
        sval = attribute->at_value.val_signed;
        sprintf(outbuf.dp, "%s", lookup_visibility_name((long)sval)); move_dp;
        break;
    default:
        print_default_attribute_value(dbg, attribute);
    }
}

/* 
  Name: vtable_elem_location 
  Classes: BLOCK, REFERENCE
*/
void print_vtable_elem_location_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    print_default_attribute_value(dbg, attribute);
}

static struct func_assoc pretty_printers[] =
{
{ DW_AT_abstract_origin, print_abstract_origin_value },
{ DW_AT_accessibility, print_accessibility_value },
{ DW_AT_address_class, print_address_class_value },
{ DW_AT_artificial, print_artificial_value },
{ DW_AT_base_types, print_base_types_value },
{ DW_AT_bit_offset, print_bit_offset_value },
{ DW_AT_bit_size, print_bit_size_value },
{ DW_AT_byte_size, print_byte_size_value },
{ DW_AT_calling_convention, print_calling_convention_value },
{ DW_AT_common_reference, print_common_reference_value },
{ DW_AT_comp_dir, print_comp_dir_value },
{ DW_AT_const_value, print_const_value_value },
{ DW_AT_containing_type, print_containing_type_value },
{ DW_AT_count, print_count_value },
{ DW_AT_data_member_location, print_data_member_location_value },
{ DW_AT_decl_column, print_decl_column_value },
{ DW_AT_decl_file, print_decl_file_value },
{ DW_AT_decl_line, print_decl_line_value },
{ DW_AT_declaration, print_declaration_value },
{ DW_AT_default_value, print_default_value_value },
{ DW_AT_discr, print_discr_value },
{ DW_AT_discr_list, print_discr_list_value },
{ DW_AT_discr_value, print_discr_value_value },
{ DW_AT_encoding, print_encoding_value },
{ DW_AT_external, print_external_value },
{ DW_AT_frame_base, print_frame_base_value },
{ DW_AT_friend, print_friend_value },
{ DW_AT_high_pc, print_high_pc_value },
{ DW_AT_identifier_case, print_identifier_case_value },
{ DW_AT_import, print_import_value },
{ DW_AT_inline, print_inline_value },
{ DW_AT_is_optional, print_is_optional_value },
{ DW_AT_language, print_language_value },
{ DW_AT_location, print_location_value },
{ DW_AT_low_pc, print_low_pc_value },
{ DW_AT_lower_bound, print_lower_bound_value },
{ DW_AT_macro_info, print_macro_info_value },
{ DW_AT_name, print_name_value },
{ DW_AT_namelist_item, print_namelist_item_value },
{ DW_AT_ordering, print_ordering_value },
{ DW_AT_priority, print_priority_value },
{ DW_AT_producer, print_producer_value },
{ DW_AT_prototyped, print_prototyped_value },
{ DW_AT_return_addr, print_return_addr_value },
{ DW_AT_segment, print_segment_value },
{ DW_AT_sibling, print_sibling_value },
{ DW_AT_specification, print_specification_value },
{ DW_AT_start_scope, print_start_scope_value },
{ DW_AT_static_link, print_static_link_value },
{ DW_AT_stmt_list, print_stmt_list_value },
{ DW_AT_stride_size, print_stride_size_value },
{ DW_AT_string_length, print_string_length_value },
{ DW_AT_type, print_type_value },
{ DW_AT_upper_bound, print_upper_bound_value },
{ DW_AT_use_location, print_use_location_value },
{ DW_AT_variable_parameter, print_variable_parameter_value },
{ DW_AT_virtuality, print_virtuality_value },
{ DW_AT_visibility, print_visibility_value },
{ DW_AT_vtable_elem_location, print_vtable_elem_location_value },
{ 0, NULL } };

struct association_list pretty_printer_list = { 0, pretty_printers };
