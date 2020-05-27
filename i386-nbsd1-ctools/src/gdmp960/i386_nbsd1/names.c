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

/* Names!  Don't put any executable code in here.  This is just a
   holding place for long, boring global arrays that associate 
   numeric names with human-readable names.

   NOTE: We can't just do the obvious, something like
   tag_names[DW_TAG_class_type] = "Class Type";
   because DW_TAG_lo_user and DW_TAG_hi_user are defined to live at 
   the end of the range: i.e. 0x4080 - 0xffff, so the tag_names array
   would have to be 64K big.  Same thing for most of the other DW_*_*
   lists.  So we'll just assign these goofy association lists, sort them
   by numeric field on startup, and then binary-search them on request
   to look up the name.

   NOTE2: The size field of these lists is filled in at runtime in
   init_name_lists.  They are also sorted at the same time.  So you can
   just add new members to the end of the list.  

   ATTN MAINTAINERS: If you add another list to this file, also add the 
   same name to the master list at the end of the file, and to dwarfdmp.h.
   Then add the name of the lookup_ function to the macro list in dwarfdmp.h.

   NOTE3: I shortened some of the attribute names due to the incredible 
   volume of information that can be stored in Dwarf, resulting in 
   unmanagably large dumps.  The original names were just copies of the
   Dwarf constant, with the DW_*_ removed.
*/

#include <stdio.h>
#include "dwarfdmp.h"
#include "dwarf2.h"

struct name_assoc tag_names[] =
{
{ 0, "null" },
{ DW_TAG_array_type, "array_type" },
{ DW_TAG_class_type, "class_type" },
{ DW_TAG_entry_point, "entry_point" },
{ DW_TAG_enumeration_type, "enumeration_type" },
{ DW_TAG_formal_parameter, "formal_parameter" },
{ DW_TAG_imported_declaration, "imported_declaration" },
{ DW_TAG_label, "label" },
{ DW_TAG_lexical_block, "lexical_block" },
{ DW_TAG_member, "member" },
{ DW_TAG_pointer_type, "pointer_type" },
{ DW_TAG_reference_type, "reference_type" },
{ DW_TAG_compile_unit, "compile_unit" },
{ DW_TAG_string_type, "string_type" },
{ DW_TAG_structure_type, "structure_type" },
{ DW_TAG_subroutine_type, "subroutine_type" },
{ DW_TAG_typedef, "typedef" },
{ DW_TAG_union_type, "union_type" },
{ DW_TAG_unspecified_parameters, "unspecified_parameters" },
{ DW_TAG_variant, "variant" },
{ DW_TAG_common_block, "common_block" },
{ DW_TAG_common_inclusion, "common_inclusion" },
{ DW_TAG_inheritance, "inheritance" },
{ DW_TAG_inlined_subroutine, "inlined_subroutine" },
{ DW_TAG_module, "module" },
{ DW_TAG_ptr_to_member_type, "ptr_to_member_type" },
{ DW_TAG_set_type, "set_type" },
{ DW_TAG_subrange_type, "subrange_type" },
{ DW_TAG_with_stmt, "with_stmt" },
{ DW_TAG_access_declaration, "access_declaration" },
{ DW_TAG_base_type, "base_type" },
{ DW_TAG_catch_block, "catch_block" },
{ DW_TAG_const_type, "const_type" },
{ DW_TAG_constant, "constant" },
{ DW_TAG_enumerator, "enumerator" },
{ DW_TAG_file_type, "file_type" },
{ DW_TAG_friend, "friend" },
{ DW_TAG_namelist, "namelist" },
{ DW_TAG_namelist_item, "namelist_item" },
{ DW_TAG_packed_type, "packed_type" },
{ DW_TAG_subprogram, "subprogram" },
{ DW_TAG_template_type_param, "template_type_param" },
{ DW_TAG_template_value_param, "template_value_param" },
{ DW_TAG_thrown_type, "thrown_type" },
{ DW_TAG_try_block, "try_block" },
{ DW_TAG_variant_part, "variant_part" },
{ DW_TAG_variable, "variable" },
{ DW_TAG_volatile_type, "volatile_type" },
{ 0, NULL } };

struct name_assoc attribute_names[] =
{
{ DW_AT_abstract_origin, "abstr_org" },
{ DW_AT_accessibility, "acc" },
{ DW_AT_address_class, "addr_class" },
{ DW_AT_artificial, "artif" },
{ DW_AT_base_types, "base_types" },
{ DW_AT_bit_offset, "bit_off" },
{ DW_AT_bit_size, "bit_sz" },
{ DW_AT_byte_size, "byte_sz" },
{ DW_AT_calling_convention, "c_c" },
{ DW_AT_common_reference, "comm_ref" },
{ DW_AT_comp_dir, "comp_dir" },
{ DW_AT_const_value, "const_val" },
{ DW_AT_containing_type, "contain_type" },
{ DW_AT_count, "count" },
{ DW_AT_data_member_location, "data_mem_loc" },
{ DW_AT_decl_column, "decl_col" },
{ DW_AT_decl_file, "decl_file" },
{ DW_AT_decl_line, "decl_line" },
{ DW_AT_declaration, "decl" },
{ DW_AT_default_value, "dflt_val" },
{ DW_AT_discr, "discr" },
{ DW_AT_discr_list, "discr_list" },
{ DW_AT_discr_value, "discr_val" },
{ DW_AT_encoding, "enc" },
{ DW_AT_external, "ext" },
{ DW_AT_frame_base, "frame_base" },
{ DW_AT_friend, "friend" },
{ DW_AT_high_pc, "hi_pc" },
{ DW_AT_identifier_case, "id_case" },
{ DW_AT_import, "import" },
{ DW_AT_inline, "inl" },
{ DW_AT_is_optional, "is_opt" },
{ DW_AT_language, "lang" },
{ DW_AT_location, "loc" },
{ DW_AT_low_pc, "lo_pc" },
{ DW_AT_lower_bound, "low_bound" },
{ DW_AT_macro_info, "mac_info" },
{ DW_AT_name, "name" },
{ DW_AT_namelist_item, "namelist_item" },
{ DW_AT_ordering, "order" },
{ DW_AT_priority, "prio" },
{ DW_AT_producer, "prod" },
{ DW_AT_prototyped, "proto" },
{ DW_AT_return_addr, "rtn_addr" },
{ DW_AT_segment, "seg" },
{ DW_AT_sibling, "sib" },
{ DW_AT_specification, "spec" },
{ DW_AT_start_scope, "st_sc" },
{ DW_AT_static_link, "stat_lnk" },
{ DW_AT_stmt_list, "stmt_list" },
{ DW_AT_stride_size, "stride_size" },
{ DW_AT_string_length, "strlen" },
{ DW_AT_type, "type" },
{ DW_AT_upper_bound, "up_bound" },
{ DW_AT_use_location, "use_loc" },
{ DW_AT_variable_parameter, "var_param" },
{ DW_AT_virtuality, "virt" },
{ DW_AT_visibility, "visib" },
{ DW_AT_vtable_elem_location, "vtable_elem_loc" },
{ 0, NULL } };
	
struct name_assoc form_names[] =
{
{ DW_FORM_addr, "addr" },
{ DW_FORM_block2, "block2" },
{ DW_FORM_block4, "block4" },
{ DW_FORM_data2, "data2" },
{ DW_FORM_data4, "data4" },
{ DW_FORM_data8, "data8" },
{ DW_FORM_string, "string" },
{ DW_FORM_block, "block" },
{ DW_FORM_block1, "block1" },
{ DW_FORM_data1, "data1" },
{ DW_FORM_flag, "flag" },
{ DW_FORM_sdata, "sdata" },
{ DW_FORM_strp, "strp" },
{ DW_FORM_udata, "udata" },
{ DW_FORM_ref_addr, "ref_addr" },
{ DW_FORM_ref1, "ref1" },
{ DW_FORM_ref2, "ref2" },
{ DW_FORM_ref4, "ref4" },
{ DW_FORM_ref8, "ref8" },
{ DW_FORM_ref_udata, "ref_udata" },
{ DW_FORM_indirect, "indirect" },
{ 0, NULL } };

struct name_assoc operation_names[] =
{
{ DW_OP_addr, "addr" },
{ DW_OP_deref, "deref" },
{ DW_OP_const1u, "const1u" },
{ DW_OP_const1s, "const1s" },
{ DW_OP_const2u, "const2u" },
{ DW_OP_const2s, "const2s" },
{ DW_OP_const4u, "const4u" },
{ DW_OP_const4s, "const4s" },
{ DW_OP_const8u, "const8u" },
{ DW_OP_const8s, "const8s" },
{ DW_OP_constu, "constu" },
{ DW_OP_consts, "consts" },
{ DW_OP_dup, "dup" },
{ DW_OP_drop, "drop" },
{ DW_OP_over, "over" },
{ DW_OP_pick, "pick" },
{ DW_OP_swap, "swap" },
{ DW_OP_rot, "rot" },
{ DW_OP_xderef, "xderef" },
{ DW_OP_abs, "abs" },
{ DW_OP_and, "and" },
{ DW_OP_div, "div" },
{ DW_OP_minus, "minus" },
{ DW_OP_mod, "mod" },
{ DW_OP_mul, "mul" },
{ DW_OP_neg, "neg" },
{ DW_OP_not, "not" },
{ DW_OP_or, "or" },
{ DW_OP_plus, "plus" },
{ DW_OP_plus_uconst, "plus_uconst" },
{ DW_OP_shl, "shl" },
{ DW_OP_shr, "shr" },
{ DW_OP_shra, "shra" },
{ DW_OP_xor, "xor" },
{ DW_OP_skip, "skip" },
{ DW_OP_bra, "bra" },
{ DW_OP_eq, "eq" },
{ DW_OP_ge, "ge" },
{ DW_OP_gt, "gt" },
{ DW_OP_le, "le" },
{ DW_OP_lt, "lt" },
{ DW_OP_ne, "ne" },
{ DW_OP_lit0, "0" },
{ DW_OP_lit1, "1" },
{ DW_OP_lit2, "2" },
{ DW_OP_lit3, "3" },
{ DW_OP_lit4, "4" },
{ DW_OP_lit5, "5" },
{ DW_OP_lit6, "6" },
{ DW_OP_lit7, "7" },
{ DW_OP_lit8, "8" },
{ DW_OP_lit9, "9" },
{ DW_OP_lit10, "10" },
{ DW_OP_lit11, "11" },
{ DW_OP_lit12, "12" },
{ DW_OP_lit13, "13" },
{ DW_OP_lit14, "14" },
{ DW_OP_lit15, "15" },
{ DW_OP_lit16, "16" },
{ DW_OP_lit17, "17" },
{ DW_OP_lit18, "18" },
{ DW_OP_lit19, "19" },
{ DW_OP_lit20, "20" },
{ DW_OP_lit21, "21" },
{ DW_OP_lit22, "22" },
{ DW_OP_lit23, "23" },
{ DW_OP_lit24, "24" },
{ DW_OP_lit25, "25" },
{ DW_OP_lit26, "26" },
{ DW_OP_lit27, "27" },
{ DW_OP_lit28, "28" },
{ DW_OP_lit29, "29" },
{ DW_OP_lit30, "30" },
{ DW_OP_lit31, "31" },
#ifdef USE_GENERIC_REGNAMES
{ DW_OP_reg0, "reg0" },
{ DW_OP_reg1, "reg1" },
{ DW_OP_reg2, "reg2" },
{ DW_OP_reg3, "reg3" },
{ DW_OP_reg4, "reg4" },
{ DW_OP_reg5, "reg5" },
{ DW_OP_reg6, "reg6" },
{ DW_OP_reg7, "reg7" },
{ DW_OP_reg8, "reg8" },
{ DW_OP_reg9, "reg9" },
{ DW_OP_reg10, "reg10" },
{ DW_OP_reg11, "reg11" },
{ DW_OP_reg12, "reg12" },
{ DW_OP_reg13, "reg13" },
{ DW_OP_reg14, "reg14" },
{ DW_OP_reg15, "reg15" },
{ DW_OP_reg16, "reg16" },
{ DW_OP_reg17, "reg17" },
{ DW_OP_reg18, "reg18" },
{ DW_OP_reg19, "reg19" },
{ DW_OP_reg20, "reg20" },
{ DW_OP_reg21, "reg21" },
{ DW_OP_reg22, "reg22" },
{ DW_OP_reg23, "reg23" },
{ DW_OP_reg24, "reg24" },
{ DW_OP_reg25, "reg25" },
{ DW_OP_reg26, "reg26" },
{ DW_OP_reg27, "reg27" },
{ DW_OP_reg28, "reg28" },
{ DW_OP_reg29, "reg29" },
{ DW_OP_reg30, "reg30" },
{ DW_OP_reg31, "reg31" },
{ DW_OP_breg0, "breg0" },
{ DW_OP_breg1, "breg1" },
{ DW_OP_breg2, "breg2" },
{ DW_OP_breg3, "breg3" },
{ DW_OP_breg4, "breg4" },
{ DW_OP_breg5, "breg5" },
{ DW_OP_breg6, "breg6" },
{ DW_OP_breg7, "breg7" },
{ DW_OP_breg8, "breg8" },
{ DW_OP_breg9, "breg9" },
{ DW_OP_breg10, "breg10" },
{ DW_OP_breg11, "breg11" },
{ DW_OP_breg12, "breg12" },
{ DW_OP_breg13, "breg13" },
{ DW_OP_breg14, "breg14" },
{ DW_OP_breg15, "breg15" },
{ DW_OP_breg16, "breg16" },
{ DW_OP_breg17, "breg17" },
{ DW_OP_breg18, "breg18" },
{ DW_OP_breg19, "breg19" },
{ DW_OP_breg20, "breg20" },
{ DW_OP_breg21, "breg21" },
{ DW_OP_breg22, "breg22" },
{ DW_OP_breg23, "breg23" },
{ DW_OP_breg24, "breg24" },
{ DW_OP_breg25, "breg25" },
{ DW_OP_breg26, "breg26" },
{ DW_OP_breg27, "breg27" },
{ DW_OP_breg28, "breg28" },
{ DW_OP_breg29, "breg29" },
{ DW_OP_breg30, "breg30" },
{ DW_OP_breg31, "breg31" },
#else /* default is use 960 regnames */
{ DW_OP_reg0, "PFP" },
{ DW_OP_reg1, "SP" },
{ DW_OP_reg2, "RIP" },
{ DW_OP_reg3, "R3" },
{ DW_OP_reg4, "R4" },
{ DW_OP_reg5, "R5" },
{ DW_OP_reg6, "R6" },
{ DW_OP_reg7, "R7" },
{ DW_OP_reg8, "R8" },
{ DW_OP_reg9, "R9" },
{ DW_OP_reg10, "R10" },
{ DW_OP_reg11, "R11" },
{ DW_OP_reg12, "R12" },
{ DW_OP_reg13, "R13" },
{ DW_OP_reg14, "R14" },
{ DW_OP_reg15, "R15" },
{ DW_OP_reg16, "G0" },
{ DW_OP_reg17, "G1" },
{ DW_OP_reg18, "G2" },
{ DW_OP_reg19, "G3" },
{ DW_OP_reg20, "G4" },
{ DW_OP_reg21, "G5" },
{ DW_OP_reg22, "G6" },
{ DW_OP_reg23, "G7" },
{ DW_OP_reg24, "G8" },
{ DW_OP_reg25, "G9" },
{ DW_OP_reg26, "G10" },
{ DW_OP_reg27, "G11" },
{ DW_OP_reg28, "G12" },
{ DW_OP_reg29, "G13" },
{ DW_OP_reg30, "G14" },
{ DW_OP_reg31, "FP" },
{ DW_OP_breg0, "bregPFP" },
{ DW_OP_breg1, "bregSP" },
{ DW_OP_breg2, "bregRIP" },
{ DW_OP_breg3, "bregR3" },
{ DW_OP_breg4, "bregR4" },
{ DW_OP_breg5, "bregR5" },
{ DW_OP_breg6, "bregR6" },
{ DW_OP_breg7, "bregR7" },
{ DW_OP_breg8, "bregR8" },
{ DW_OP_breg9, "bregR9" },
{ DW_OP_breg10, "bregR10" },
{ DW_OP_breg11, "bregR11" },
{ DW_OP_breg12, "bregR12" },
{ DW_OP_breg13, "bregR13" },
{ DW_OP_breg14, "bregR14" },
{ DW_OP_breg15, "bregR15" },
{ DW_OP_breg16, "bregG0" },
{ DW_OP_breg17, "bregG1" },
{ DW_OP_breg18, "bregG2" },
{ DW_OP_breg19, "bregG3" },
{ DW_OP_breg20, "bregG4" },
{ DW_OP_breg21, "bregG5" },
{ DW_OP_breg22, "bregG6" },
{ DW_OP_breg23, "bregG7" },
{ DW_OP_breg24, "bregG8" },
{ DW_OP_breg25, "bregG9" },
{ DW_OP_breg26, "bregG10" },
{ DW_OP_breg27, "bregG11" },
{ DW_OP_breg28, "bregG12" },
{ DW_OP_breg29, "bregG13" },
{ DW_OP_breg30, "bregG14" },
{ DW_OP_breg31, "bregFP" },
#endif
{ DW_OP_regx, "regx" },
{ DW_OP_fbreg, "fbreg" },
{ DW_OP_bregx, "bregx" },
{ DW_OP_piece, "piece" },
{ DW_OP_deref_size, "deref_size" },
{ DW_OP_xderef_size, "xderef_size" },
{ DW_OP_nop, "nop" },
{ 0, NULL } };

struct name_assoc basetype_names[] =
{
{ DW_ATE_address, "addr" },
{ DW_ATE_boolean, "bool" },
{ DW_ATE_complex_float, "cplx_flt" },
{ DW_ATE_float, "flt" },
{ DW_ATE_signed, "sign" },
{ DW_ATE_signed_char, "sign_char" },
{ DW_ATE_unsigned, "uns" },
{ DW_ATE_unsigned_char, "uns_char" },
{ 0, NULL } };

struct name_assoc access_names[] =
{
{ DW_ACCESS_public, "pub" },
{ DW_ACCESS_protected, "prot" },
{ DW_ACCESS_private, "priv" },
{ 0, NULL } };

struct name_assoc visibility_names[] =
{
{ DW_VIS_local, "loc" },
{ DW_VIS_exported, "export" },
{ DW_VIS_qualified, "qual" },
{ 0, NULL } };

struct name_assoc virtuality_names[] =
{
{ DW_VIRTUALITY_none, "none" },
{ DW_VIRTUALITY_virtual, "virt" },
{ DW_VIRTUALITY_pure_virtual, "pure_virt" },
{ 0, NULL } };

struct name_assoc language_names[] =
{
{ DW_LANG_C89, "C89" },
{ DW_LANG_C, "C" },
{ DW_LANG_Ada83, "Ada83" },
{ DW_LANG_C_plus_plus, "C++" },
{ DW_LANG_Cobol74, "Cobol74" },
{ DW_LANG_Cobol85, "Cobol85" },
{ DW_LANG_Fortran77, "Fortran77" },
{ DW_LANG_Fortran90, "Fortran90" },
{ DW_LANG_Pascal83, "Pascal83" },
{ DW_LANG_Modula2, "Modula2" },
{ 0, NULL } };

struct name_assoc idcase_names[] =
{
{ DW_ID_case_sensitive, "case_sens" },
{ DW_ID_up_case, "up_case" },
{ DW_ID_down_case, "down_case" },
{ DW_ID_case_insensitive, "case_insens" },
{ 0, NULL } };

struct name_assoc callingconvention_names[] =
{
{ DW_CC_normal, "norm" },
{ DW_CC_program, "prog" },
{ DW_CC_nocall, "nocall" },
{ 0, NULL } };

struct name_assoc inlining_names[] =
{
{ DW_INL_not_inlined, "not_inl" },
{ DW_INL_inlined, "inl" },
{ DW_INL_declared_not_inlined, "decl_not_inl" },
{ DW_INL_declared_inlined, "decl_inl" },
{ 0, NULL } };

struct name_assoc ordering_names[] =
{
{ DW_ORD_row_major, "row" },
{ DW_ORD_col_major, "col" },
{ 0, NULL } };

struct name_assoc descriptor_names[] =
{
{ DW_DSC_label, "label" },
{ DW_DSC_range, "range" },
{ 0, NULL } };

struct name_assoc flag_names[] =
{
{ 0, "f" },
{ 1, "t" },
{ 0, NULL } };

struct name_assoc line_standard_names[] =
{
{ DW_LNS_copy, "copy" },
{ DW_LNS_advance_pc, "advance_pc" },
{ DW_LNS_advance_line, "advance_line" },
{ DW_LNS_set_file, "set_file" },
{ DW_LNS_set_column, "set_column" },
{ DW_LNS_negate_stmt, "negate_stmt" },
{ DW_LNS_set_basic_block, "set_basic_block" },
{ DW_LNS_const_add_pc, "const_add_pc" },
{ DW_LNS_fixed_advance_pc, "fixed_advance_pc" },
{ 0, NULL} };

struct name_assoc line_extended_names[] =
{
{ DW_LNE_end_sequence, "end_sequence" },
{ DW_LNE_set_address, "set_address" },
{ DW_LNE_define_file, "define_file" },
{ 0, NULL} };

struct name_assoc macinfo_names[] =
{
{ DW_MACINFO_define, "define" },
{ DW_MACINFO_undef , "undef " },
{ DW_MACINFO_start_file, "start_file" },
{ DW_MACINFO_end_file, "end_file" },
{ DW_MACINFO_vendor_ext, "vendor_ext" },
{ 0, NULL} };

struct name_assoc frameinfo_names[] =
{
{ DW_CFA_advance_loc, "advance_loc" },
{ DW_CFA_offset, "offset" },
{ DW_CFA_restore, "restore" },
{ DW_CFA_set_loc, "set_loc" },
{ DW_CFA_advance_loc1, "advance_loc1" },
{ DW_CFA_advance_loc2, "advance_loc2" },
{ DW_CFA_advance_loc4, "advance_loc4" },
{ DW_CFA_offset_extended, "offset_extended" },
{ DW_CFA_restore_extended, "restore_extended" },
{ DW_CFA_undefined, "undefined" },
{ DW_CFA_same_value, "same_value" },
{ DW_CFA_register, "register" },
{ DW_CFA_remember_state, "remember_state" },
{ DW_CFA_restore_state, "restore_state" },
{ DW_CFA_def_cfa, "def_cfa" },
{ DW_CFA_def_cfa_register, "def_cfa_register" },
{ DW_CFA_def_cfa_offset, "def_cfa_offset" },
{ DW_CFA_nop, "nop" },
{ DW_CFA_lo_user, "lo_user" },
{ DW_CFA_hi_user, "hi_user" },
{ 0, NULL} };

struct name_assoc framereg_names[] = 
{
{ DW_CFA_R0,  "pfp" },
{ DW_CFA_R1,  "sp"  },
{ DW_CFA_R2,  "rip" },
{ DW_CFA_R3,  "r3"  },
{ DW_CFA_R4,  "r4"  },
{ DW_CFA_R5,  "r5"  },
{ DW_CFA_R6,  "r6"  },
{ DW_CFA_R7,  "r7"  },
{ DW_CFA_R8,  "r8"  },
{ DW_CFA_R9,  "r9"  },
{ DW_CFA_R10, "r10" },
{ DW_CFA_R11, "r11" },
{ DW_CFA_R12, "r12" },
{ DW_CFA_R13, "r13" },
{ DW_CFA_R14, "r14" },
{ DW_CFA_R15, "r15" },
{ DW_CFA_R16, "g0"  },
{ DW_CFA_R17, "g1"  },
{ DW_CFA_R18, "g2"  },
{ DW_CFA_R19, "g3"  },
{ DW_CFA_R20, "g4"  },
{ DW_CFA_R21, "g5"  },
{ DW_CFA_R22, "g6"  },
{ DW_CFA_R23, "g7"  },
{ DW_CFA_R24, "g8"  },
{ DW_CFA_R25, "g9"  },
{ DW_CFA_R26, "g10" },
{ DW_CFA_R27, "g11" },
{ DW_CFA_R28, "g12" },
{ DW_CFA_R29, "g13" },
{ DW_CFA_R30, "g14" },
{ DW_CFA_R31, "fp"  },
{ DW_CFA_R32, "fp0" },
{ DW_CFA_R33, "fp1" },
{ DW_CFA_R34, "fp2" },
{ DW_CFA_R35, "fp3" },
{ 0, NULL} };

struct association_list tag_name_list = { 0, tag_names };
struct association_list attribute_name_list = { 0, attribute_names };
struct association_list form_name_list = { 0, form_names };
struct association_list operation_name_list = { 0, operation_names };
struct association_list basetype_name_list = { 0, basetype_names };
struct association_list access_name_list = { 0, access_names };
struct association_list visibility_name_list = { 0, visibility_names };
struct association_list virtuality_name_list = { 0, virtuality_names };
struct association_list language_name_list = { 0, language_names };
struct association_list idcase_name_list = { 0, idcase_names };
struct association_list callingconvention_name_list = { 0, callingconvention_names };
struct association_list inlining_name_list = { 0, inlining_names };
struct association_list ordering_name_list = { 0, ordering_names };
struct association_list descriptor_name_list = { 0, descriptor_names };
struct association_list flag_name_list = { 0, flag_names };
struct association_list line_standard_name_list = { 0, line_standard_names };
struct association_list line_extended_name_list = { 0, line_extended_names };
struct association_list macinfo_name_list = { 0, macinfo_names };
struct association_list frameinfo_name_list = { 0, frameinfo_names };
struct association_list framereg_name_list = { 0, framereg_names };

struct association_list *master_name_list[] =
{
    &tag_name_list,
    &attribute_name_list,
    &form_name_list,
    &operation_name_list,
    &basetype_name_list,
    &access_name_list,
    &visibility_name_list,
    &virtuality_name_list,
    &language_name_list,
    &idcase_name_list,
    &callingconvention_name_list,
    &inlining_name_list,
    &ordering_name_list,
    &descriptor_name_list,
    &flag_name_list,
    &line_standard_name_list,
    &line_extended_name_list,
    &macinfo_name_list,
    &frameinfo_name_list,
    &framereg_name_list,
    NULL
};

