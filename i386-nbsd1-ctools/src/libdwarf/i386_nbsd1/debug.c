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

/*
  Print minimal information about DIEs.
  Print what's been expanded; i.e. built into the child and sibling
  pointers, not what is available in the Dwarf.
 */

#include "libdwarf.h"
#include "internal.h"
#include "pubnames.h"
#include "lines.h"
#include "aranges.h"

static FILE *_dump_stream = stdout;
static int _dump_indent_level;

void
_dump_die(die)
    Dwarf_Die die;
{
    Dwarf_Attribute name = dwarf_attr(die, DW_AT_name);
    Dwarf_Attribute sib = dwarf_attr(die, DW_AT_sibling);

    if ( dwarf_tag(die) == DW_TAG_compile_unit )
	fprintf(_dump_stream, "New Compilation Unit:\n");
    _dump_indent();
    if ( sib )
	fprintf(_dump_stream, "0x%x %s 0x%x\n", 
/* 		die->section_offset, */
		die->cu_offset,
		name ? name->at_value.val_string : "<>",
		sib->at_value.val_unsigned);
    else
	fprintf(_dump_stream, "0x%x %s\n", 
/* 		die->section_offset, */
		die->cu_offset,
		name ? name->at_value.val_string : "<>");
}

_dump_tree(root)
    Dwarf_Die root;
{
    Dwarf_Die die;
    _dump_die(root);
    _dump_indent_level += 2;
    for (die = root->child_ptr; die; die = die->sibling_ptr)
    {
	_dump_tree(die);
    }
    _dump_indent_level -= 2;
}

void 
_dump_abbrev(ab)
    Dwarf_Abbrev *ab;
{
    Dwarf_Attrib_Form *fp = ab->attr_list;
    char code[10];
    char tag[10];
    sprintf(code, "0x%x", ab->code);
    sprintf(tag, "0x%x", ab->tag);
    printf("%-10s%-10s", code, tag);
    while ( fp )
    {
	printf("0x%x ", fp->name);
	fp = fp->next;
    }
    printf("\n");
}

void
_dump_abbrev_table(tab)
    Dwarf_Abbrev_Table *tab;
{
    int i;
    for ( i = 0; i < tab->entry_count; ++i )
    {
	_dump_abbrev(&tab->abbrev_array[i]);
    }
}

void 
_dump_line(ln)
    Dwarf_Line ln;
{
    printf("%10d%15x\n", ln->line, ln->address);
}

void
_dump_lines(dbg)
    Dwarf_Debug dbg;
{
    Dwarf_CU_List *cu;

    for ( cu = dbg->dbg_cu_list; cu; cu = cu->next )
    {
	Dwarf_Line_Table *lntab = cu->line_table;
	int i;
	printf("Line numbers, new compilation unit: %s\n",
	       DIE_ATTR_STRING(cu->die_head->die_list, DW_AT_name));
	for ( i = 0; i < lntab->ln_count; ++i )
	    _dump_line(&lntab->ln_list[i]);
    }
}

void 
_dump_pubname(pn)
    Dwarf_Pn_Tuple pn;
{
    printf("%10d  %s\n", pn->offset, pn->name);
}

void
_dump_pubnames(dbg)
    Dwarf_Debug dbg;
{
    Dwarf_CU_List *cu;

    for ( cu = dbg->dbg_cu_list; cu; cu = cu->next )
    {
	Dwarf_Pubnames_Table *pntab = cu->pubnames_table;
	int i;
	printf("Pubnames, new compilation unit: %s\n",
	       DIE_ATTR_STRING(cu->die_head->die_list, DW_AT_name));
	for ( i = 0; i < pntab->pn_count; ++i )
	    _dump_pubname(&pntab->pn_list[i]);
    }
}

void
_dump_file(dbg)
    Dwarf_Debug dbg;
{
    Dwarf_Die cu = dwarf_child(dbg, NULL);

    for ( ; cu; cu = cu->sibling_ptr)
    {
	_dump_tree(cu);
    }

    if ( dbg->dbg_cu_list->pubnames_table )
    {
	_dump_pubnames(dbg);
    }
    if ( dbg->dbg_cu_list->line_table )
    {
	_dump_lines(dbg);
    }
}

_dump_indent()
{
    int i;
    for ( i = 0; i < _dump_indent_level; ++i )
	fputc(' ', _dump_stream);
}

static char *_dump_reg_name(r)
    int r;
{
    switch(r) {
 case DW_CFA_R0:  return "pfp"; break;
 case DW_CFA_R1:  return "sp";  break;
 case DW_CFA_R2:  return "rip"; break;
 case DW_CFA_R3:  return "r3";  break;
 case DW_CFA_R4:  return "r4";  break;
 case DW_CFA_R5:  return "r5";  break;
 case DW_CFA_R6:  return "r6";  break;
 case DW_CFA_R7:  return "r7";  break;
 case DW_CFA_R8:  return "r8";  break;
 case DW_CFA_R9:  return "r9";  break;
 case DW_CFA_R10: return "r10"; break;
 case DW_CFA_R11: return "r11"; break;
 case DW_CFA_R12: return "r12"; break;
 case DW_CFA_R13: return "r13"; break;
 case DW_CFA_R14: return "r14"; break;
 case DW_CFA_R15: return "r15"; break;
 case DW_CFA_R16: return "g0";  break;
 case DW_CFA_R17: return "g1";  break;
 case DW_CFA_R18: return "g2";  break;
 case DW_CFA_R19: return "g3";  break;
 case DW_CFA_R20: return "g4";  break;
 case DW_CFA_R21: return "g5";  break;
 case DW_CFA_R22: return "g6";  break;
 case DW_CFA_R23: return "g7";  break;
 case DW_CFA_R24: return "g8";  break;
 case DW_CFA_R25: return "g9";  break;
 case DW_CFA_R26: return "g10"; break;
 case DW_CFA_R27: return "g11"; break;
 case DW_CFA_R28: return "g12"; break;
 case DW_CFA_R29: return "g13"; break;
 case DW_CFA_R30: return "g14"; break;
 case DW_CFA_R31: return "fp";  break;
 case DW_CFA_R32: return "fp0"; break;
 case DW_CFA_R33: return "fp1"; break;
 case DW_CFA_R34: return "fp2"; break;
 case DW_CFA_R35: return "fp3"; break;
 default: return "unknown?"; break;
    }
}

void _dump_insns(insn,ninsns)
    Dwarf_Frame_Insn insn;
    unsigned long ninsns;
{
    if (!insn) {
	fprintf(_dump_stream,"insn is null.\n");
	return;
    }
    else {
	int i;
	static char *insn_map[] = {
	    "Dwarf_frame_offset",
	    "Dwarf_frame_restore",
	    "Dwarf_frame_undefined",
	    "Dwarf_frame_same",
	    "Dwarf_frame_register",
	    "Dwarf_frame_remember_state",
	    "Dwarf_frame_restore_state",
	    "Dwarf_frame_def_cfa",
	    "Dwarf_frame_def_cfa_register",
	    "Dwarf_frame_def_cfa_offset",
            "Dwarf_frame_pfp_offset",
	NULL };
	enum op_type { reg, offset };
	static struct opstruct {
	    int n_ops;
	    enum op_type ops[2];
	} ops[] = {
	    /* Dwarf_frame_offset */           {2,{offset,reg}},
	    /* Dwarf_frame_restore */          {1,{reg}},
	    /* Dwarf_frame_undefined */        {1,{reg}},
	    /* Dwarf_frame_same */             {1,{reg}},
	    /* Dwarf_frame_register */         {2,{reg,reg}},
	    /* Dwarf_frame_remember_state */   {0,{reg}},
	    /* Dwarf_frame_restore_state */    {0,{reg}},
	    /* Dwarf_frame_def_cfa */          {2,{reg,offset}},
	    /* Dwarf_frame_def_cfa_register */ {1,{reg}},
	    /* Dwarf_frame_def_cfa_offset */   {1,{offset}}, 
            /* Dwarf_frame_pfp_offset */       {2,{reg,offset}} };

	for (i=0;i < ninsns;i++) {
	    Dwarf_Frame_Insn in = insn+i;

	    fprintf(_dump_stream,"insn: ip: 0x%08x %s(",in->loc,insn_map[in->insn]);
	    if (ops[in->insn].n_ops) {
		int j,nop[2];
		struct opstruct *o = ops+in->insn;

		nop[0] = in->op1;
		nop[1] = in->op2;
		for (j=0;j < o->n_ops;j++) {
		    if (j)
			    fprintf(_dump_stream,",");
		    if (o->ops[j] == reg)
			    fprintf(_dump_stream,"%s",_dump_reg_name(nop[j]));
		    else
			    fprintf(_dump_stream,"%d",nop[j]);
		}
	    }
	    fprintf(_dump_stream,")\n");
	}
    }
}

void _dump_cie(cie)
    Dwarf_Frame_CIE cie;
{
    if (!cie) {
	fprintf(_dump_stream,"cie is null.\n");
	return;
    }
    fprintf(_dump_stream,"cie:ninsns: %d\n",cie->n_instructions);
    _dump_insns(cie->insns,cie->n_instructions);
}

void _dump_fde(fde)
    Dwarf_Frame_FDE fde;
{
    if (!fde) {
	fprintf(_dump_stream,"fde is null.\n");
	return;
    }
    fprintf(_dump_stream,"fde: cie:\n");
    _dump_cie(fde->cie);
    fprintf(_dump_stream,"fde: start_ip: %08x, address_range: %d, n_instructions: %d\n",fde->start_ip,
	    fde->address_range,fde->n_instructions);
    _dump_insns(fde->insns,fde->n_instructions);
}

void
_dump_arange(ar)
    Dwarf_Ar_Tuple ar;
{
    printf("0x%x: 0x%x   0x%x\n", (void *) ar, ar->low_addr, ar->high_addr);
}
 
void
_dump_aranges(dbg)
    Dwarf_Debug dbg;
{
    Dwarf_CU_List *cu;
 
    for ( cu = dbg->dbg_cu_list; cu; cu = cu->next )
    {
        Dwarf_Aranges_Table *artab = cu->aranges_table;
        int i;
        printf("Aranges, new compilation unit: %s\n",
               DIE_ATTR_STRING(cu->die_head->die_list, DW_AT_name));
        for ( i = 0; i < artab->ar_count; ++i )
            _dump_arange(&artab->ar_list[i]);
    }
}

