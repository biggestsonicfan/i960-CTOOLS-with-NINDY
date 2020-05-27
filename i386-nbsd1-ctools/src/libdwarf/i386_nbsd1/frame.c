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
#include "frame.h"

/* Used by qsort and bsearch to sort and find an FDE. */
static int
compare_fdes(l,r)
    Dwarf_Frame_FDE l,r;
{
    unsigned long start_of_l = l->start_ip;
    unsigned long end_of_l   = l->start_ip + (l->address_range ? (l->address_range-1) : 0);
    unsigned long start_of_r = r->start_ip;
    unsigned long end_of_r   = r->start_ip + (r->address_range ? (r->address_range-1) : 0);

    if (end_of_l < start_of_r)
	    return -1;
    else if (end_of_r < start_of_l)
	    return 1;
    else  /* The two ranges overlap - we consider them equal. */
	    return 0;
}

/* Puts a new unique CIE into the CIE offsets tree. */
static void dwarf_frame_putin_tree(dbg,p,offset,the_cie)
    Dwarf_Debug dbg;
    Dwarf_Frame_Real_CIE_Tree_Node *p;
    int offset;
    Dwarf_Frame_Real_CIE the_cie;
{
    if (*p) {
	if (offset < (*p)->offset_in_section)
		dwarf_frame_putin_tree(dbg,&(*p)->left,offset,the_cie);
	else if (offset > (*p)->offset_in_section)
		dwarf_frame_putin_tree(dbg,&(*p)->right,offset,the_cie);
	/* Else, this is equal to another in the tree.  Let's do nothing. */
    }
    else {
	/* Put it into the tree here. */
	(*p) = (Dwarf_Frame_Real_CIE_Tree_Node) _dw_malloc(dbg,
							   sizeof(struct Dwarf_Frame_Real_CIE_Tree_Node));
	(*p)->left = (*p)->right = (Dwarf_Frame_Real_CIE_Tree_Node) 0;
	(*p)->offset_in_section = offset;
	(*p)->the_cie = the_cie;
    }
}

/* Looks up a cie in the CIE offsets tree. */
static Dwarf_Frame_Real_CIE dwarf_frame_lookup_only(p,offset)
    Dwarf_Frame_Real_CIE_Tree_Node *p;
    int offset;
{
    if (*p) {
	if (offset < (*p)->offset_in_section)
		return dwarf_frame_lookup_only(&(*p)->left,offset);
	else if (offset > (*p)->offset_in_section)
		return dwarf_frame_lookup_only(&(*p)->right,offset);
	else
		return (*p)->the_cie;
    }
    return (Dwarf_Frame_Real_CIE) 0;
}

/* frees the cie offsets tree. */
static void dwarf_frame_free_tree(dbg,p)
    Dwarf_Debug dbg;
    Dwarf_Frame_Real_CIE_Tree_Node p;
{
    if (!p)
	    return;
    dwarf_frame_free_tree(dbg,p->left);
    dwarf_frame_free_tree(dbg,p->right);
    _dw_free(dbg,(char *)p);
}

/* Read insns from dbg until _dw_ftell() says we are over the end.
   Returns 1 on success, 0 on error (bad opcode e.g.). */
static int dwarf_frame_parse_insns(dbg,ninsns,insns,end_offset,be,loc,caf,daf)
    Dwarf_Debug dbg;
    int *ninsns;
    Dwarf_Frame_Insn *insns;
    int end_offset,be;
    unsigned long loc,caf,daf;
{
    unsigned long cfa_offset = 0;
    int maxinsns = 10;

    (*ninsns) = 0;

    (*insns) = (Dwarf_Frame_Insn)
	    _dw_malloc(dbg,maxinsns * sizeof(struct Dwarf_Frame_Insn));

    while (_dw_ftell(dbg->stream) < end_offset) {
	int insert_insn = 0;
	Dwarf_Frame_Insn_Type insn_type;
	Dwarf_Unsigned op1,op2 = 0;
	unsigned char opcode = _dw_read_constant(dbg,1,be);

#define HIGH_2_BITS(B) ((B) & 0xc0)
#define LOW_6_BITS(B)  ((B) & 0x3f)

	switch (HIGH_2_BITS(opcode)) {
	    /* See 7.23 of the spec for deets on this stuff.
	       Sorry this code is so dense and complex. */
    case DW_CFA_advance_loc:
	    loc += (caf * LOW_6_BITS(opcode));
	    break;
    case DW_CFA_offset:
	    insert_insn = 1;
	    insn_type = Dwarf_frame_offset;
	    op1 = daf*_dw_leb128_to_ulong(dbg);
	    op2 = LOW_6_BITS(opcode);
	    break;
    case DW_CFA_restore:
	    insert_insn = 1;
	    insn_type = Dwarf_frame_restore;
	    op1 = LOW_6_BITS(opcode);
	    break;
    case 0 :
	    switch (opcode) {
	case DW_CFA_set_loc:
		loc = _dw_read_constant(dbg,4,be);
		break;
	case DW_CFA_advance_loc1:
		loc += (caf * _dw_read_constant(dbg,1,be));
		break;
	case DW_CFA_advance_loc2:
		loc += (caf * _dw_read_constant(dbg,2,be));
		break;
	case DW_CFA_advance_loc4:
		loc += (caf * _dw_read_constant(dbg,4,be));
		break;
	case DW_CFA_offset_extended:
		insert_insn = 1;
		insn_type = Dwarf_frame_offset;
		op2 = _dw_leb128_to_ulong(dbg);        /* Register first. */
		op1 = (daf*_dw_leb128_to_ulong(dbg));  /* Offset second. */
		break;
	case DW_CFA_restore_extended:
		insert_insn = 1;
		insn_type = Dwarf_frame_restore;
		op1 = _dw_leb128_to_ulong(dbg);
		break;
	case DW_CFA_undefined:
		insert_insn = 1;
		insn_type = Dwarf_frame_undefined;
		op1 = _dw_leb128_to_ulong(dbg);
		break;
	case DW_CFA_same_value:
		insert_insn = 1;
		insn_type = Dwarf_frame_same;
		op1 = _dw_leb128_to_ulong(dbg);
		break;
	case DW_CFA_register:
		insert_insn = 1;
		insn_type = Dwarf_frame_register;
		op1 = _dw_leb128_to_ulong(dbg);
		op2 = _dw_leb128_to_ulong(dbg);
		break;
	case DW_CFA_remember_state:
		insert_insn = 1;
		insn_type = Dwarf_frame_remember_state;
		op1 = 0;
		break;
	case DW_CFA_restore_state:
		insert_insn = 1;
		insn_type = Dwarf_frame_restore_state;
		op1 = 0;
		break;
	case DW_CFA_def_cfa:
		insert_insn = 1;
		insn_type = Dwarf_frame_def_cfa;
		op1 = _dw_leb128_to_ulong(dbg);
		cfa_offset = op2 = (daf*_dw_leb128_to_ulong(dbg));
		break;
	case DW_CFA_def_cfa_register:
		insert_insn = 1;
		insn_type = Dwarf_frame_def_cfa;
		op1 = _dw_leb128_to_ulong(dbg);
		op2 = cfa_offset;
		break;
	case DW_CFA_def_cfa_offset:
		insert_insn = 1;
		insn_type = Dwarf_frame_def_cfa_offset;
		cfa_offset = op1 = (_dw_leb128_to_ulong(dbg)*daf);
		break;
	case DW_CFA_i960_pfp_offset:
		insert_insn = 1;
		insn_type = Dwarf_frame_pfp_offset;
		op1 = (_dw_leb128_to_ulong(dbg)*daf);
		op2 = (_dw_leb128_to_ulong(dbg)*daf);
		break;
	case DW_CFA_nop:
		break;
	default:
		return 0;
		break;
	}
	    break;
    default:  /* This code is unreachable by virtue of the formation of the DW_CFA_* opcodes. */
	    return 0;
	    break;
	}

	if (insert_insn) {
	    Dwarf_Frame_Insn p;

	    if (++(*ninsns) > maxinsns)
		    (*insns) = (Dwarf_Frame_Insn)
			    _dw_realloc(dbg,(char *)*insns,
					(maxinsns *= 2) * sizeof(struct Dwarf_Frame_Insn));
	    p = ((*insns) + ((*ninsns)-1));
	    p->loc = loc;
	    p->insn = insn_type;
	    p->op1 = op1;
	    p->op2 = op2;
	}
    }
    if (maxinsns > (*ninsns)) {
	if (!(*ninsns)) {
	    _dw_free(dbg,(char *)*insns);
	    (*insns) = (Dwarf_Frame_Insn) 0;
	}
	else
		(*insns) = (Dwarf_Frame_Insn)
			_dw_realloc(dbg,(char *)*insns,
				    (*ninsns) * sizeof(struct Dwarf_Frame_Insn));
    }
    return 1;
}

/* Compare the passed real_cie, t with all of them we already know about.  If t is among them,
   free t's instructions and return the one that we already know about.  Else, insert t into the
   cie_table by mallocing space for it and copying it, and then return the one we just malloced. */
static Dwarf_Frame_Real_CIE dwarf_frame_lookup_cie(dbg,t)
    Dwarf_Debug dbg;
    Dwarf_Frame_Real_CIE t;
{
    Dwarf_Frame_Information dfi = DWARF_FRAME_INFORM(dbg);
    int i;

#define CMP(ELEMENT) if ((dfi->CIE_table[i])->ELEMENT != t->ELEMENT) continue

    for (i=0;i < dfi->n_CIE;i++) {
	int j;
	int insns_ok = 1;

	CMP(real_cie.n_instructions);
	CMP(code_alignment_factor);
	CMP(data_alignment_factor);
	CMP(real_cie.return_address_register);
	for (j=0;j < t->real_cie.n_instructions;j++) {
	    Dwarf_Frame_Insn i1 = (dfi->CIE_table[i])->real_cie.insns+j,i2 = t->real_cie.insns+j;

#define CMPI(ELEMENT) if (i1->ELEMENT != i2->ELEMENT) {	insns_ok = 0; break; }

	    CMPI(loc);
	    CMPI(insn);
	    CMPI(op1);
	    CMPI(op2);
	}
	if (insns_ok) {
	    _dw_free(dbg,(char *)t->real_cie.insns);
	    return *(dfi->CIE_table+i);
	}
    }
    dfi->n_CIE++;
    if (dfi->CIE_table)
	    dfi->CIE_table = (Dwarf_Frame_Real_CIE *)
		    _dw_realloc(dbg,(char *)dfi->CIE_table,
				dfi->n_CIE * sizeof(Dwarf_Frame_Real_CIE));
    else
	    dfi->CIE_table = (Dwarf_Frame_Real_CIE *)
		    _dw_malloc(dbg,dfi->n_CIE * sizeof(Dwarf_Frame_Real_CIE));
    if (1) {
	Dwarf_Frame_Real_CIE p = (Dwarf_Frame_Real_CIE) _dw_malloc(dbg,
								   sizeof(struct Dwarf_Frame_Real_CIE));

	/* Hope we can do structure assignment. */
	(*p) = (*t);

	dfi->CIE_table[(dfi->n_CIE-1)] = p;

	return p;
    }
}

/* Lookup a CIE.  We know the offset of the CIE, we want to know if:
   a.) we have already read it or
   b.) already read one just like it or
   c.) we have never read this CIE, nor one like it

If a.) is the case, then we return the previous CIE's address and don't read it again.
If b.) is the case, then we record an association between this offset and the equivalent CIE and
                         return the equivalent CIE.
If c.) is the case, then we record this CIE for later reference, and malloc the CIE and return it. */
static Dwarf_Frame_Real_CIE dwarf_frame_lookup_real_cie(dbg,offset,be)
    Dwarf_Debug dbg;
    int offset,be;
{
    Dwarf_Frame_Information dfi = DWARF_FRAME_INFORM(dbg);
    Dwarf_Frame_Real_CIE r = dwarf_frame_lookup_only(&dfi->root_of_cie_offsets_tree,offset);

    if (r)  /* If the CIE is in the offset tree, we bug out right away. */
	    return r;
    else {  /* We have to read it. */
	long before_seek = _dw_ftell(dbg->stream),CIE_id, length_of_CIE;
	struct Dwarf_Frame_Real_CIE t;

	_dw_fseek(dbg->stream,offset,SEEK_SET);
	length_of_CIE = _dw_read_constant(dbg,4,be);

	CIE_id = _dw_read_constant(dbg,4,be);

	if (CIE_id == DW_CIE_ID) {
	    int end_of_this_cie = length_of_CIE + _dw_ftell(dbg->stream) - 4;
	    char version = _dw_read_constant(dbg,1,be);
	    char *augmentation = _dw_build_a_string(dbg);
	    Dwarf_Frame_Real_CIE p;

	    if (!augmentation || *augmentation == 0 ||
		!strcmp(DW_CFA_i960_ABI_augmentation,augmentation)) {

		if (augmentation)
			_dw_free(dbg,(char *)augmentation);
		t.code_alignment_factor = _dw_leb128_to_ulong(dbg);
		t.data_alignment_factor = _dw_leb128_to_ulong(dbg);
		t.real_cie.return_address_register = _dw_read_constant(dbg,1,be);
		if (!dwarf_frame_parse_insns(dbg,&t.real_cie.n_instructions,
					     &t.real_cie.insns,end_of_this_cie,be,0,t.code_alignment_factor,
					     t.data_alignment_factor)) {
		    dwarf_frame_putin_tree(dbg,&dfi->root_of_cie_offsets_tree,offset,0);
		    _dw_fseek(dbg->stream,before_seek,SEEK_SET);
		    return (Dwarf_Frame_Real_CIE) 0;
		}
		/* The following line of code looks for an equivalent CIE among the members of the
		   CIE_table. If it is among the CIE_table, the code returns the equivalent one. */
		p = dwarf_frame_lookup_cie(dbg,&t);
		dwarf_frame_putin_tree(dbg,&dfi->root_of_cie_offsets_tree,offset,p);
		_dw_fseek(dbg->stream,before_seek,SEEK_SET);
		return p;
	    }
	    else {
		/* Can't read it, due to unknown augmentation. */
		_dw_free(dbg,(char *)augmentation);
		memset(&t,0,sizeof(t));
		t.real_cie.return_address_register = DWARF_FRAME_UNKNOWN_AUG;
		p = dwarf_frame_lookup_cie(dbg,&t);
		dwarf_frame_putin_tree(dbg,&dfi->root_of_cie_offsets_tree,offset,p);
		_dw_fseek(dbg->stream,before_seek,SEEK_SET);
		return (Dwarf_Frame_Real_CIE) p;
	    }
	}   
	else {
	    /* Can't read it, because it is not a CIE. */
	    dwarf_frame_putin_tree(dbg,&dfi->root_of_cie_offsets_tree,offset,0);
	    _dw_fseek(dbg->stream,before_seek,SEEK_SET);
	    return (Dwarf_Frame_Real_CIE) 0;
	}
    }
}

/*
    LIBRARY FUNCTION
    dwarf_frame_fetch_fde - fetch an FDE for a given IP - read the debug_frame section
    if it has not already been read.  If it has lookup the IP from the information
    already read..

    SYNOPSIS

    Dwarf_Frame_FDE
    dwarf_frame_fetch_fde(dbg,func_ip)
                          Dwarf_Debug dbg;
                          Dwarf_Unsigned func_ip;


    DESCRIPTION

    Read read in the .debug_frame section.  Search the fde's read in for an FDE for
    the given func_ip.  If one can not be found, return NULL and set error message
    appropriately.

    RETURNS

    The Fde containing the func_ip (if it found it).  Else null.

    BUGS

    Currently does not support demand loading because this is not in the dwarf
    2 spec.

*/

Dwarf_Frame_FDE
dwarf_frame_fetch_fde(dbg,func_ip)
    Dwarf_Debug dbg;
    Dwarf_Unsigned func_ip;
{
    Dwarf_Frame_Information dfi = DWARF_FRAME_INFORM(dbg);

#define DFI_NO_FDES ((Dwarf_Frame_Information) -1)

    RESET_ERR();

    if (dfi) {
	if (dfi == DFI_NO_FDES) {         /* If we have already tried to read FDE's from this
					     file and failed. */
	    LIBDWARF_ERR(DLE_BAD_FRAME);  /* Then we bug out right away. */
	    return (Dwarf_Frame_FDE) 0;       
	}

	else {
	    struct Dwarf_Frame_FDE t;
	    Dwarf_Frame_FDE t1;

	    t.start_ip = func_ip;
	    t.address_range = 0;
	    if (t1 = (Dwarf_Frame_FDE)
		    bsearch(&t,dfi->FDE_table,dfi->n_FDE,sizeof(t),compare_fdes)) {
#define HAS_UNKNOWN_AUG(FDE) ((FDE)->cie->return_address_register == DWARF_FRAME_UNKNOWN_AUG)
		if (HAS_UNKNOWN_AUG(t1))
			LIBDWARF_ERR(DLE_UNK_AUG);
		return t1;
	    }
	    else {
		LIBDWARF_ERR(DLE_NO_FRAME);
		return t1;
	    }
	}
    }
    else {
	Dwarf_Section frame_section = _dw_find_section(dbg, ".debug_frame");
	int max_fdes = 10;

	if (!frame_section) {
	    LIBDWARF_ERR(DLE_NOFRAME);
	    return (Dwarf_Frame_FDE) 0;
	}

	dbg->frame_info = (Dwarf_Frame_Info *)
		_dw_malloc(dbg,sizeof(struct Dwarf_Frame_Information));

	dfi = DWARF_FRAME_INFORM(dbg);

	_dw_clear((char *)dbg->frame_info,sizeof(struct Dwarf_Frame_Information));

	_dw_fseek(dbg->stream, 
		  frame_section->file_offset,
		  SEEK_SET);

	while (_dw_ftell(dbg->stream) < frame_section->file_offset+frame_section->size) {
	    Dwarf_Unsigned length = _dw_read_constant(dbg,4,frame_section->big_endian);
	    Dwarf_Unsigned offset = _dw_read_constant(dbg,4,frame_section->big_endian);

	    if (offset == DW_CIE_ID)
		    /* Don't read this CIE for now, just in case it is never referenced in any FDE. */
		    _dw_fseek(dbg->stream,length-4,SEEK_CUR);
	    else {
		Dwarf_Frame_Real_CIE rc = dwarf_frame_lookup_real_cie(dbg,offset+frame_section->file_offset,
								      frame_section->big_endian);

		if (!rc)
			/* Can't find the CIE, so we have to skip this FDE. */
			_dw_fseek(dbg->stream,length-4,SEEK_CUR);
		else {
		    int end_of_this_fde = _dw_ftell(dbg->stream) + length - 4; /* -4 because we have
										  already read the offset. */
		    Dwarf_Frame_FDE t;

		    if (++dfi->n_FDE == 1)
			    dfi->FDE_table = (Dwarf_Frame_FDE)
				    _dw_malloc(dbg,max_fdes * sizeof(struct Dwarf_Frame_FDE));
		    else if (dfi->n_FDE > max_fdes)
			    dfi->FDE_table = (Dwarf_Frame_FDE)
				    _dw_realloc(dbg,(char *)dfi->FDE_table,
						 (max_fdes *= 2) * sizeof(struct Dwarf_Frame_FDE));
		    t = dfi->FDE_table + (dfi->n_FDE-1);

		    t->cie = &rc->real_cie;
		    t->start_ip = _dw_read_constant(dbg,4,frame_section->big_endian);
		    t->address_range = _dw_read_constant(dbg,4,frame_section->big_endian);
		    if (rc->real_cie.return_address_register != DWARF_FRAME_UNKNOWN_AUG) {
			if (!dwarf_frame_parse_insns(dbg,&t->n_instructions,&t->insns,end_of_this_fde,
						     frame_section->big_endian,
						     t->start_ip,
						     rc->code_alignment_factor,
						     rc->data_alignment_factor)) {
			    dfi->n_FDE--;
			    _dw_fseek(dbg->stream,end_of_this_fde,SEEK_SET);
			}
		    }
		    else {
			/* The CIE for this FDE has unknown augmentation.  So we set only those values
			   we can set, and bug out. */
			t->n_instructions = 0;
			t->insns = NULL;
			_dw_fseek(dbg->stream,end_of_this_fde,SEEK_SET);
		    }
		}
	    }
	}
	if (max_fdes > dfi->n_FDE) {
	    if (!dfi->n_FDE) {
		_dw_free(dbg,(char *)dfi->FDE_table);
		_dw_free(dbg,(char *)dfi);
		dbg->frame_info = (Dwarf_Frame_Info *) DFI_NO_FDES;
		LIBDWARF_ERR(DLE_BAD_FRAME);
		return (Dwarf_Frame_FDE) 0;
	    }
	    else {
		dfi->FDE_table = (Dwarf_Frame_FDE)
			_dw_realloc(dbg,(char *)dfi->FDE_table,
				    dfi->n_FDE * sizeof(struct Dwarf_Frame_FDE));
	    }
	}
	qsort(dfi->FDE_table,dfi->n_FDE,sizeof(struct Dwarf_Frame_FDE),compare_fdes);
	if (1) {
	    int i;

	    for (i=0;i < dfi->n_FDE;i++) {
		dfi->FDE_table[i].previous_FDE = (i > 0) ? (dfi->FDE_table+(i-1)) :
			((Dwarf_Frame_FDE) 0);
		dfi->FDE_table[i].next_FDE = (i == (dfi->n_FDE-1)) ? ((Dwarf_Frame_FDE) 0) :
			dfi->FDE_table+(i+1);
	    }
	}
	dwarf_frame_free_tree(dbg,dfi->root_of_cie_offsets_tree);
	_dw_free(dbg,(char *)dfi->CIE_table);
	dfi->CIE_table = 0;
	dfi->root_of_cie_offsets_tree = 0;
	if (1) {
	    struct Dwarf_Frame_FDE t;
	    Dwarf_Frame_FDE t1;

	    t.start_ip = func_ip;
	    t.address_range = 0;
	    if (t1 = (Dwarf_Frame_FDE)
		bsearch(&t,dfi->FDE_table,dfi->n_FDE,sizeof(t),compare_fdes)) {
		if (HAS_UNKNOWN_AUG(t1))
			LIBDWARF_ERR(DLE_UNK_AUG);
		return t1;
	    }
	    else {
		LIBDWARF_ERR(DLE_NO_FRAME);
		return t1;
	    }
	}
    }
}
