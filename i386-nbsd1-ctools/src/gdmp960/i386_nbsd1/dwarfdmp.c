/*****************************************************************************
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
 ***************************************************************************/
 
/* Routines to dump Dwarf version 2 sections */
 
#include "gdmp960.h"
#include "bfd.h"
#include "dwarfdmp.h"
#include "dwarf2.h"
#include "libdwarf.h"
#include <limits.h>
#include <string.h>
 
/* globals -- both are used only in .debug_info dump */
struct output_buffer outbuf;
static Dwarf_Die comp_unit_die = NULL;   

void
build_libdwarf_section_list (abfd, sectp, sect_list)
     bfd *abfd;
     asection *sectp;
     PTR sect_list;
{
    Dwarf_Section sections = (Dwarf_Section) sect_list;
    sections[sectp->secnum - 1].name = (char *) bfd_section_name(abfd, sectp);
    sections[sectp->secnum - 1].size = bfd_section_size(abfd, sectp);
    sections[sectp->secnum - 1].file_offset = sectp->filepos;
    sections[sectp->secnum - 1].big_endian = bfd_get_section_flags(abfd, sectp) & SEC_IS_BIG_ENDIAN;
}

void
dmp_cu_name(die)
    Dwarf_Die die;
{
    printf("New Compilation unit: %s\n ", dwarf_diename(die));
    printf("\n");
}
 
void
attribute_error(attribute, val_type)
    Dwarf_Attribute attribute;
    char *val_type;
{
    error("attribute '%s' does not have type '%s'\n",
          lookup_attribute_name(attribute->at_name), val_type);
}
 
/* Actually print something for .debug_info.  This can be as fancy or as 
   primitive as you like.  For now, it just dumps the data buffer to the screen
   all on one line. 

   The sibling field is currently not being dumped for space saving reasons.
*/
flush_outbuf()
{
    char addrbuf[10];

    sprintf(addrbuf, "0x%x", outbuf.addr);

    /* checkme: *.* dangerous with windows compiler? */
    printf("%s%*.*s%-20s %-22s\n",
       addrbuf, outbuf.indent, outbuf.indent, " ", 
       (*outbuf.name != '\0') ? outbuf.name : "<>", outbuf.tag);  

    if (*outbuf.data) 
    {
       printf("%*s%s\n", outbuf.indent + strlen(addrbuf), "", outbuf.data);
    }
    printf("\n");
 
    outbuf.dp = outbuf.data;
    *outbuf.name = *outbuf.sibling = *outbuf.data = 0;
}
 
/* Print one attribute value in the default format.  */
void
print_default_attribute_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    Dwarf_Unsigned ulong, ulong2;
    Dwarf_Signed slong;
    Dwarf_Block block;
    char *string_p;
    Dwarf_Bignum bignum;
    Dwarf_Small form = attribute->at_form;
    unsigned char *c;
    int i;
    Dwarf_Die offdie;

    switch ( form )
    {
    case 0:
        break;
        /* data* forms are printed with decimal formatting.
           addr* forms are printed in hex. */
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_udata:
        if (attribute->val_type != DLT_UNSIGNED) 
        {
            attribute_error(attribute, "DLT_UNSIGNED");
            break;
        }
        ulong = attribute->at_value.val_unsigned;
        sprintf(outbuf.dp, "%u", ulong); move_dp;
        break;
    case DW_FORM_data8:
        if (attribute->val_type != DLT_BIGNUM) 
        {
            attribute_error(attribute, "DLT_BIGNUM");
            break;
        }
        bignum = attribute->at_value.val_bignum;
        if (bignum->numwords != 2) 
        {
            printf("Error: number of words in Dwarf_Bignum not equal to 2\n");
            break;
        }
        ulong =  bignum->words[0];
        ulong2 = bignum->words[1];
        sprintf(outbuf.dp,
                "{ word[0] = 0x%lx, word[1] = 0x%lx }", ulong, ulong2);
        move_dp;
        break;
    case DW_FORM_addr:
        /* NOTE: We're bypassing the address_size BS since
           we know darn well how big an address is on the 960. */
        if (attribute->val_type != DLT_UNSIGNED) 
        {
            attribute_error(attribute, "DLT_UNSIGNED");
            break;
        }
        ulong = attribute->at_value.val_unsigned;
        sprintf(outbuf.dp, "0x%x", ulong); move_dp;
        break;
    case DW_FORM_sdata:
        if (attribute->val_type != DLT_SIGNED) 
        {
            attribute_error(attribute, "DLT_SIGNED");
            break;
        }
        slong = attribute->at_value.val_signed;
        sprintf(outbuf.dp, "%d", (long) slong); move_dp;
        break;
 
        /* ref* forms are all printed with hex formatting. */
    case DW_FORM_ref1:
    case DW_FORM_ref2:
    case DW_FORM_ref4:
    case DW_FORM_ref_udata:
        if (attribute->val_type != DLT_UNSIGNED) 
        {
            attribute_error(attribute, "DLT_UNSIGNED");
            break;
        }

        ulong = attribute->at_value.val_unsigned;
        offdie = dwarf_cu_offdie(dbg, comp_unit_die, (Dwarf_Off) ulong);
        if (dwarf_diename(offdie) != NULL) 
        {
            sprintf(outbuf.dp, "%s", dwarf_diename(offdie)); move_dp;
        }
        else 
        {
            sprintf(outbuf.dp, "0x%x", ulong); move_dp;
        }
        break;
    case DW_FORM_ref8:
        if (attribute->val_type != DLT_BIGNUM) 
        {
            attribute_error(attribute, "DLT_BIGNUM");
            break;
        }
        bignum = attribute->at_value.val_bignum;
        if (bignum->numwords != 2) 
        {
            printf("Error: number of words in Dwarf_Bignum not equal to 2\n");
            break;
        }
        ulong = bignum->words[0];
        ulong2 = bignum->words[1];
        sprintf(outbuf.dp,
                "{ word[0] = 0x%lx, word[1] = 0x%lx }", ulong, ulong2);
        move_dp;
        break;
    case DW_FORM_ref_addr:
        /* The ref_addr form is an offset from the start of the
           FILE, not from the start of the compilation unit. */
        /* NOTE: We're bypassing the address_size BS since
           we know darn well how big an address is on the 960. */
        if (attribute->val_type != DLT_UNSIGNED) 
        {
            attribute_error(attribute, "DLT_UNSIGNED");
            break;
        }
        ulong = attribute->at_value.val_unsigned;
        sprintf(outbuf.dp, "[0x%x]", ulong); move_dp;
        break;
 
        /* The default format for block forms is to just print the bytes
           in raw hex representation. */
    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
    case DW_FORM_block:
        if (attribute->val_type != DLT_BLOCK) 
        {
            attribute_error(attribute, "DLT_BLOCK");
            break;
        }
        block = attribute->at_value.val_block;
        c = (unsigned char *) block->bl_data;
 
        for (i = 0; i < block->bl_len; i++, c++)
        {
            sprintf(outbuf.dp, "0x%x ", *c);
            move_dp;
        }
        break;
    case DW_FORM_string:
        if (attribute->val_type != DLT_STRING) 
        {
            attribute_error(attribute, "DLT_STRING");
            break;
        }
        string_p = attribute->at_value.val_string;
        strcat(outbuf.dp, string_p); move_dp;
        break;
 
    case DW_FORM_flag:
        if (attribute->val_type != DLT_UNSIGNED) 
        {
            attribute_error(attribute, "DLT_FLAG");
            break;
        }
        ulong = attribute->at_value.val_unsigned;
        sprintf(outbuf.dp, "%s", lookup_flag_name(ulong)); move_dp;
        break;
 
        /* Remaining forms are unimplemented (yet) */
    case DW_FORM_strp:
        if (attribute->val_type != DLT_UNSIGNED) 
        {
            attribute_error(attribute, "DLT_UNSIGNED");
            break;
        }
        ulong = attribute->at_value.val_unsigned;
        sprintf(outbuf.dp, "DW_FORM_strp 0x%x", ulong); move_dp;
        break;
 
    default:
        printf ("Unrecognized FORM encoding\n");
        break;
    }
}
 
/* Print one attribute value.  */
void
print_attribute_value(dbg, attribute)
    Dwarf_Debug dbg;
    Dwarf_Attribute attribute;
{
    void (*pp)();  /* Pretty printer function */
 
    pp = lookup_pretty_printer(attribute->at_name);
 
    if ( pp == NULL )
        print_default_attribute_value(dbg, attribute);
    else
        (*pp)(dbg, attribute);
}
 
int
dmp_die(dbg, die)
    Dwarf_Debug dbg;
    Dwarf_Die die;
{
    Dwarf_Off offset;
    Dwarf_Attribute attrlist;
    int semi = 1;                     /* Flag: was a semicolon just printed? */
 
    offset = dwarf_cu_dieoffset(die);
 
    outbuf.addr = offset;
    strcpy(outbuf.tag, lookup_tag_name(dwarf_tag(die)));
 
    dwarf_attrlist(die, &attrlist);
 
    for ( ; attrlist ; attrlist = attrlist->next) 
    {
        Dwarf_Half atname = attrlist->at_name;
 
        if (! semi) 
        {
             sprintf(outbuf.dp, " ; "); move_dp;
             semi = 1;
        }
 
        /* name and sibling attributes do not go into outbuf.name and
           outbuf.sibling respectively, and NOT outbuf.dp  */
        if (atname != DW_AT_name  && atname != DW_AT_sibling) 
        {
             sprintf(outbuf.dp, "%s ", lookup_attribute_name(atname)); move_dp;
             semi = 0;
        }
 
        print_attribute_value(dbg, attrlist);
    }
 
    flush_outbuf();
    return 1;
}
 
int
dmp_tree(dbg, root)
    Dwarf_Debug dbg;
    Dwarf_Die root;
{
    Dwarf_Die die;
    dmp_die(dbg, root);
    bump_indent();
 
    die = dwarf_child(dbg, root);
    for ( ; die && dwarf_tag(die) ; die = dwarf_siblingof(dbg, die)) 
    {
       if (!dmp_tree(dbg, die))
          return 0;
    }
    drop_indent();
    return 1;
}
 
/* Print an introductory notice before each dmp.  */
void
introduce_dmp(name)
    char *name;   /* section name */
{
    printf("\n");
    printf("Begin dump of %s section:\n", name);
    printf("\n");
}
 
void
dmp_debug_info_header()
{
   printf("%s %-20s %-22s\n%-5s%s\n\n",
      "Addr",
      "Name",
      "Tag",
      "",
      "Attribute/Value Pairs"); 
}

void
dmp_cu_debug_info_header(cu_die)
    Dwarf_Die cu_die;
{
    Dwarf_Off offset = dwarf_dieoffset(cu_die);

    printf("=============================================================\n");
    printf("0x%x  New Compilation Unit: %s\n", offset, dwarf_diename(cu_die));
    printf("=============================================================\n");
    printf("\n");
}

int
dmp_debug_info(dbg)
    Dwarf_Debug dbg;
{
    init_outbuf();
    init_name_lists();
    init_pretty_printers();
 
    introduce_dmp(".debug_info");
    dmp_debug_info_header(); 

    comp_unit_die = dwarf_siblingof(dbg, NULL);
    if (comp_unit_die == NULL)
        return 0;
 
    for ( ; comp_unit_die ; comp_unit_die = dwarf_siblingof(dbg, comp_unit_die))
    {
       dmp_cu_debug_info_header(comp_unit_die);
       dmp_tree(dbg, comp_unit_die);
    }
    return 1;
}
 
#define line_hdr_format "%7s%5s%10s%5s%6s%4s\n"
 
void
dmp_debug_line_header(die, line)
    Dwarf_Die  die;
    Dwarf_Line line;
{
    char *filename;
 
    if (line->file)
        filename = line->file;
    else
        filename = dwarf_diename(die);
 
    printf("File: %s\n", filename);
 
    if (line->dir)
        printf("Directory: %s\n", line->dir);
 
    printf("\n");
 
    printf(line_hdr_format,
       "Line", "Col", "Address", "    ", "Basic", "End");
    printf(line_hdr_format,
       "Num ", "Num", " (Hex) ", "Stmt", "Block", "Seq");
    printf(line_hdr_format,
       "====", "===", "=======", "====", "=====", "===");
}
 
int
dmp_srclines(dbg, cu_die)
    Dwarf_Debug dbg;
    Dwarf_Die cu_die;
{
    Dwarf_Line linetable = NULL;
    Dwarf_Line line;
 
    /* Set some locals to point to highly unlikely values.  Use them
       to check for back-to-back duplicates as they are read in. */
    char *lastfile = NULL;
    int numln;
 
    if (dwarf_tag(cu_die) != DW_TAG_compile_unit)
       return 0;
 
    dwarf_seterrarg("Libdwarf error in dwarf_srclines");
    numln = dwarf_srclines(dbg, cu_die, &linetable);
    dwarf_seterrarg(NULL);
 
    if ( numln == DLV_NOCOUNT )
        return 1;
 
    dmp_debug_line_header(cu_die, linetable);
    lastfile = linetable->file;
 
    for ( line = linetable; numln; --numln, ++line )
    {
        if ( line->file && line->file != lastfile )
        {
           /* Probably there is executable code in an included file.
              Dump a new header with the new file name. */
            dmp_debug_line_header(cu_die, line);
            lastfile = line->file;
        }
 
        printf("%7u%5u%10x%5s%6s%4s\n",
            line->line,
            line->column,
            line->address,
            (line->is_stmt ? "t" : "f"),
            (line->basic_block ? "t" : "f"),
            (line->end_sequence ? "t" : "f"));
 
    }
    printf("\n");
    return 1;
}
 
/*
    Scan each compilation unit DIE and print the lines for that CU
*/
int
dmp_debug_line(dbg)
    Dwarf_Debug dbg;
{
    Dwarf_Die die;
 
    introduce_dmp(".debug_line");
 
    die = dwarf_siblingof(dbg, NULL);
 
    for ( ; die ; die = dwarf_siblingof(dbg, die)) 
    {
        if (!dmp_srclines(dbg, die))
            return 0;
    }
    return 1;
}

/* 
 * add_cie_entry - helper function for build_cie_table.  Responsible for  
 *   allocating/reallocating memory.  Assumes that cie is unique and will
 *   add it to the table */ 
void
add_cie_entry(cie, cie_table, cie_table_size, index)
    Dwarf_Frame_CIE cie;
    Dwarf_Frame_CIE **cie_table;
    int *cie_table_size;
    int index;
{
    if (index >= *cie_table_size)
    {
        if (*cie_table)
        {
            /* Realloc for twice the current size */
            *cie_table = (Dwarf_Frame_CIE*) realloc((void *) *cie_table, 
                        *cie_table_size * sizeof(Dwarf_Frame_CIE*) * 2);
            *cie_table_size *= 2;
        }
        else
        {
            /* First time this has been called.  Allocate a reasonable
               number of tables entries.  We'll realloc later if needed. */
            *cie_table_size = 128;
            *cie_table = (Dwarf_Frame_CIE*) 
                       malloc(*cie_table_size * sizeof(Dwarf_Frame_CIE*));
        }
    }
    (*cie_table)[index] = cie;
}

/*
 * lookup_cie - Search cie_table for cie.  Return 0 if not found.
 */
int
lookup_cie(cie, nitems, cie_table)
    Dwarf_Frame_CIE cie;
    int nitems;
    Dwarf_Frame_CIE *cie_table;
{
    int i;

    for (i = 0; i < nitems; i++) {
        if (cie_table[i] == cie)
            return i+1;
    }
    return 0;
}
   
/*
 * build_cie_table - Do a pass over the DIE tree, find all unique CIE's
 *   and stick them into a table.  
 *
 *   Details: Each subprogram refers to one CIE.  An FDE is needed to get
 *   the CIE, so for each subprogram, get one FDE and get the CIE from there.  
 *   Only add the CIE to the cie_table if it is unique.
 */
Dwarf_Frame_CIE
*build_cie_table(dbg, cie_count)
    Dwarf_Debug dbg;
    int *cie_count;
{
    Dwarf_Die cu_die;

    /* For either (1) relocatable files, or (2) files with inlined functions,
       the algorithm below can search for IP == 0.  This generates a libdwarf
       error that will pop out numerous times, cluttering the display.  
       Change the severity of this error to warning, and then the error 
       handler will ignore it. */
    dwarf_seterrsev(DLE_NO_FRAME, DLS_WARNING);

    cu_die = dwarf_siblingof(dbg, NULL);

    for ( ; cu_die ; cu_die = dwarf_siblingof(dbg, cu_die)) 
    {
        /* Scan each CU for the subprograms */
        Dwarf_Die die = dwarf_child(dbg, cu_die);

        for ( ; die ; die = dwarf_siblingof(dbg, die)) 
        {
            if (dwarf_tag(die) == DW_TAG_subprogram) 
            {
                Dwarf_Frame_FDE fde;
                unsigned long low_pc = (unsigned long) dwarf_lowpc(die);

                fde = dwarf_frame_fetch_fde(dbg, low_pc);

                /* Search table to see if this cie exists */
                if (fde) {
		    Dwarf_Frame_CIE *cie_table = NULL;  /* The result */
		    int index = 0;                      /* Current index into cie list */
		    int cie_table_size = 0;             /* Set by add_cie_entry */

		    while (fde->previous_FDE)
			    fde = fde->previous_FDE;

		    while (fde) {
			if (!lookup_cie(fde->cie, index, cie_table)) 
				add_cie_entry(fde->cie, &cie_table, &cie_table_size, index++);
			fde = fde->next_FDE;
		    }
		    *cie_count = index;
		    return cie_table;
		}
	    }
	}
    }
    *cie_count = 0;
    return (Dwarf_Frame_CIE *) 0;
}
    
static char *insn_map[] = {
    "Dwarf_frame_offset",		/* DW_CFA_offset */
    "Dwarf_frame_restore",		/* DW_CFA_restore */
    "Dwarf_frame_undefined",		/* DW_CFA_undefined */
    "Dwarf_frame_same",			/* DW_CFA_same_value */
    "Dwarf_frame_register",		/* DW_CFA_register */
    "Dwarf_frame_remember_state",	/* DW_CFA_remember_state */
    "Dwarf_frame_restore_state",	/* DW_CFA_restore_state */
    "Dwarf_frame_def_cfa",		/* DW_CFA_def_cfa */
    "Dwarf_frame_def_cfa_register",	/* DW_CFA_def_cfa_register */
    "Dwarf_frame_def_cfa_offset",	/* DW_CFA_def_cfa_offset */
    "Dwarf_frame_pfp_offset",		/* DW_CFA_i960_pfp_offset */
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
    /* Dwarf_frame_pfp_offset */       {2,{reg,offset}}
};
 
static void dmp_insns(insn, ninsns, emit_address)
    Dwarf_Frame_Insn insn;
    unsigned long ninsns;
    int emit_address;
{
    int i;
 
    if (!insn) 
    {
        return;
    }
 
    for (i=0;i < ninsns; i++) 
    {
        Dwarf_Frame_Insn in = insn+i;
 
        printf("         ");
	if (emit_address)
            printf("0x%.8x ", in->loc);
        printf("%s(", insn_map[in->insn]);
        if (ops[in->insn].n_ops) 
        {
            int j, nop[2];
            struct opstruct *o = ops+in->insn;
 
            nop[0] = in->op1;
            nop[1] = in->op2;
            for (j=0;j < o->n_ops; j++) 
            {
                if (j)
                    printf(",");
                if (o->ops[j] == reg)
                    printf("%s", lookup_framereg_name(nop[j]));
                else
                    printf("%d",nop[j]);
            }
        }
        printf(")\n");
    }
}

void dmp_cie(cie, id)
    Dwarf_Frame_CIE cie;
    int id;
{
    if (!cie)
    {
        printf("   CIE: %d (NULL)\n", id);
        printf("\n");
        return;
    }
 
    printf("   CIE: #%d\n", id);
    printf("      return address register: %s%s\n", lookup_framereg_name(cie->return_address_register),
	   cie->return_address_register == DWARF_FRAME_UNKNOWN_AUG ? " (has unknown augmentation)" : "");
 
    dmp_insns(cie->insns, cie->n_instructions, 0);
    printf("\n");
}
 
void dmp_fde(fde, cie_nitems, cie_table)
    Dwarf_Frame_FDE fde;
    int cie_nitems;
    Dwarf_Frame_CIE *cie_table;
{
    int cie_index;

    if (!fde)
    {
        printf("   FDE: (NULL)\n");
        printf("\n");
        return;
    }

    cie_index = lookup_cie(fde->cie, cie_nitems, cie_table);

    printf("   FDE:\n");

    printf("      cie:                %8d\n", cie_index); 
    printf("      initial_location: 0x%.8x\n", fde->start_ip);
    printf("      address_range:      %8d\n", fde->address_range);

    dmp_insns(fde->insns, fde->n_instructions, 1);
    printf("\n");
}

/*
 * dmp_subprogram_fde - get the low_pc and high_pc for this subprogram.
 *   Then iteratate from low_pc to high_pc and call dwarf_frame_fetch_fde
 *   for each ip.  Only dump unique FDE's.
 */
void
dmp_subprogram_fde(dbg, subprogram_die, cie_nitems, cie_table)
    Dwarf_Debug dbg;
    Dwarf_Die subprogram_die;
    int cie_nitems;
    Dwarf_Frame_CIE *cie_table;
{
    Dwarf_Addr low_pc, hi_pc;
    Dwarf_Frame_FDE fde;
    Dwarf_Frame_FDE last_fde = NULL;
    unsigned long ip;
    char *die_name;

    die_name = dwarf_diename(subprogram_die);
    low_pc = dwarf_lowpc(subprogram_die);
    hi_pc = dwarf_highpc(subprogram_die);
 
    printf("Subprogram: %s    lowpc = 0x%x, hipc = 0x%x\n",
        (die_name) ? (die_name) : "<>", low_pc, hi_pc);
    
    ip = (unsigned long) low_pc;
    last_fde = dwarf_frame_fetch_fde(dbg, ip);
    ip += 4; 

    do 
    {
        fde = dwarf_frame_fetch_fde(dbg, ip);
        if (! fde || last_fde != fde) 
        {
            dmp_fde(last_fde, cie_nitems, cie_table);
            last_fde = fde;
        }
        ip += 4;
    } while (fde && ip <= (unsigned long)hi_pc);
}

void
dmp_cie_table(nitems, cie_table)
    int nitems;
    Dwarf_Frame_CIE *cie_table;
{
    int i;

    printf("CIE Table\n");
    printf("=========\n");
    printf("\n");

    for (i = 0 ; i < nitems ; i++) 
    {
        dmp_cie(cie_table[i], i+1);
    }
}
    
/*
 * dmp_debug_frame - Each subprogram has one or more FDE's.  Each FDE
 *   references a CIE that may be shared by other FDE's.  Therefore, first
 *   dump all unique CIE's and assign each a unique ID.  Then dump all
 *   FDE's, and have each FDE reference its CIE by this unique ID number.
 *   We dump only unique CIE's because it cuts down on the size of the
 *   dump considerably.
 *
 *   There are no libdwarf routines to access the CIE and FDE list.  Therefore,
 *   the algorithm scans the DIE tree for subprogram die's and calls
 *   dwarf_frame_fetch_fde to get an FDE at a particular ip.  Each FDE
 *   has a pointer to a CIE.  Two passes are made over the DIE tree, the 
 *   first to get the unique CIE's (build_cie_table) and the second to dump 
 *   the FDE's.
 *
 *   BUGS: It is a possibility that some FDE's (not referenced by a subprogram 
 *   DIE) will NOT be dumped.
 */
int
dmp_debug_frame(dbg)
    Dwarf_Debug dbg;
{
    Dwarf_Die cu_die;
    Dwarf_Frame_CIE *cie_table;          /* A unique list of CIE's */
    int num_items;                       /* #items in CIE table */

    introduce_dmp(".debug_frame");

    /* This function accesses the framereg_names table in names.c. 
       The table is used by dmp_insns. */
    init_name_lists();

    cie_table = build_cie_table(dbg, &num_items);
    dmp_cie_table(num_items, cie_table);

    /* Scan the DIE tree again and dump all FDE's for each subprogram DIE */
    cu_die = dwarf_siblingof(dbg, NULL);
       
    for ( ; cu_die ; cu_die = dwarf_siblingof(dbg, cu_die)) 
    {
        Dwarf_Die die = dwarf_child(dbg, cu_die);
        for ( ; die ; die = dwarf_siblingof(dbg, die)) 
        {
            if (dwarf_tag(die) == DW_TAG_subprogram) 
            {
                dmp_subprogram_fde(dbg, die, num_items, cie_table);
            }
        }
    }

    free(cie_table); 
    return 1;
}

int
dmp_srcpubs(dbg, cu_die)
    Dwarf_Debug dbg;
    Dwarf_Die cu_die;
{
    Dwarf_Pn_Tuple pubtable;
    Dwarf_Signed numpubs;
    char minibuf[16];
    int i;

    if (dwarf_tag(cu_die) != DW_TAG_compile_unit)
       return 0;

    dwarf_seterrarg("Libdwarf error in dwarf_srcpubs");
    numpubs = dwarf_srcpubs(dbg, cu_die, &pubtable);
    dwarf_seterrarg(NULL);

    /* The error handler will dump any errors, so we don't have to do so here */
    if ( numpubs == DLV_NOCOUNT )
        return 1;

    printf("%4s%-25s%4s%s\n", "", "Name", "", "Offset");
    printf("%4s%-25s%4s%s\n", "", "====", "", "======");

    for (i = 0; i < numpubs; i++) 
    {
        Dwarf_Pn_Tuple pn = &pubtable[i];
        sprintf(minibuf, "0x%x", pn->offset);
        printf("%4s%-25s%4s%s\n", "", pn->name, "", minibuf);
    }
    printf("\n");
    return 1;
}

int
dmp_debug_pubnames(dbg)
    Dwarf_Debug dbg;
{
    Dwarf_Die die;

    introduce_dmp(".debug_pubnames");

    die = dwarf_siblingof(dbg, NULL);

    for ( ; die ; die = dwarf_siblingof(dbg, die)) 
    {
        dmp_cu_name(die);
        if (!dmp_srcpubs(dbg, die))
            return 0;;
    }
    return 1;
}

int
dmp_srcranges(dbg, cu_die)
    Dwarf_Debug dbg;
    Dwarf_Die cu_die;
{
    Dwarf_Ar_Tuple rangetable;
    Dwarf_Signed num;
    int i;
 
    if (dwarf_tag(cu_die) != DW_TAG_compile_unit)
       return 0;
 
    dwarf_seterrarg("Libdwarf error in dwarf_srcranges");
    num = dwarf_srcranges(dbg, cu_die, &rangetable);
    dwarf_seterrarg(NULL);
 
    /* The error handler will dump any errors, so we don't have to do so here */
    if ( num == DLV_NOCOUNT )
        return 1;
 
    printf("%4s%s%4s%s\n", "", "Low addr", "", "High Addr");
    printf("%4s%s%4s%s\n", "", "========", "", "=========");
 
    for (i = 0; i < num; i++)
    {
        Dwarf_Ar_Tuple ar = &rangetable[i];
        printf("%4s%x%4s%x\n", "", ar->low_addr, "", ar->high_addr);
    }
    printf("\n");
    return 1;
}
 
int
dmp_debug_aranges(dbg)
    Dwarf_Debug dbg;
{
    Dwarf_Die die;
 
    introduce_dmp(".debug_aranges");
 
    die = dwarf_siblingof(dbg, NULL);
 
    for ( ; die ; die = dwarf_siblingof(dbg, die))
    {
        dmp_cu_name(die);
        if (!dmp_srcranges(dbg, die))
            return 0;;
    }
    return 1;
}
 
int
dmp_debug_macinfo(dbg)
    Dwarf_Debug dbg;
{
    introduce_dmp(".debug_macinfo");
    printf("Not yet implemented in libdwarf.  Nothing to dump.\n");
    printf("\n");
    return 1;
}

/*
 *
 * The following are utility functions for dumping .debug_info section.
 *
 */

init_name_lists()
{
    struct association_list     **listp = &master_name_list[0];
    struct name_assoc           *ap;
    int                         count;
 
    for ( ; *listp; ++listp ) 
    {
      /* Count the number of elements */
      for ( count = 0, ap = (*listp)->list; ap->name; ++count, ++ap )
         ;
      (*listp)->size = count;
      /* Sort the list for fast searching later */
      qsort((void *) (*listp)->list, count, sizeof(struct name_assoc), compare_asses);
    }
}
 
init_pretty_printers()
{
    struct func_assoc          *fp;
    int                         count;
 
    /* Count the number of elements */
    for ( count = 0, fp = pretty_printer_list.list; fp->func; ++count, ++fp )
        ;
    pretty_printer_list.size = count;
    /* Sort the list for fast searching later */
    qsort((void *) pretty_printer_list.list, count, sizeof(struct func_assoc), compare_pretties);
}

init_outbuf()
{
   outbuf.addr = 0;
   outbuf.tag = (char *) malloc(256);
   outbuf.name = (char *) malloc(256);
   outbuf.sibling = (char *) malloc(32);
   outbuf.data = (char *) malloc(16384);
   outbuf.namelen = 256;
   outbuf.dp = outbuf.data;
   *outbuf.name = *outbuf.sibling = *outbuf.data = *outbuf.tag = 0;
   outbuf.indent = 2;
}
 
/* Comparator function for qsort and bsearch,
   for name_assoc lists. */
compare_asses(ass1, ass2)
    struct name_assoc *ass1, *ass2;
{
    return (int) ass1->code - (int) ass2->code;
}
 
/* Comparator function for qsort and bsearch,
   for the pretty_printer list. */
compare_pretties(p1, p2)
    struct func_assoc *p1, *p2;
{
    return (int) p1->code - (int) p2->code;
}
 
/* This function searches a sorted association list and returns a string.
   Usually, this will be called via macro. (found in dwarf2dmp.h)
 
   IMPORTANT: This function does not allocate storage for the return value!
   i.e. either (1) it points to statically initialized memory, or
   (2) it will change the next time this guy is called.
   If you are just going to print the return value, then perfect!
   Otherwise, you (the caller) must allocate your own storage and copy the
   return value into it.
   */
char *lookup_association_name(list, num_elem, key)
    struct name_assoc list[];
    unsigned long num_elem;
    unsigned long key;
{
  struct name_assoc *found, dummy;
  dummy.code = key;
  found = (struct name_assoc *)
     bsearch(&dummy, list, num_elem, sizeof(struct name_assoc), compare_asses);
  return found ? found->name : "Unrecognized";
}
 
/* This function searches the pretty-printer list for an attribute and
   returns a function.  The return value can then be called to print the
   attribute value in whatever format is appropriate for that attribute.
*/
void (*lookup_pretty_printer(attr))()
    unsigned long attr;
{
    struct func_assoc *found, dummy;
    dummy.code = attr;
    found = (struct func_assoc *)
    bsearch(&dummy,
        pretty_printer_list.list,
        pretty_printer_list.size,
        sizeof(struct func_assoc),
        compare_pretties);
    return found ? found->func : NULL;
}

/* dwarf2_errhand -  The error handler, stolen from gdb960's dwarf2read.c.
 
   If the same error occurs more than ERROR_THRESHOLD times,
   return FATAL.  This guarantees a program termination even if the
   Dwarf file is hopelessly corrupted.
   
   Do nothing for libdwarf warnings - they are either harmless or 
   reported elsewhere.
*/
 
#define ERROR_THRESHOLD 40
 
Dwarf_Signed
dwarf2_errhand (error_desc)
    Dwarf_Error error_desc;
{
    static Dwarf_Unsigned lasterror = 0xffffffff;
    static int errcount;
    char error_buf[1024];
 
    if ( error_desc )
    {
        sprintf(error_buf,
                "%s %s, line %d: %s\n",
                "Error in libdwarf file",
                error_desc->file,
                error_desc->line,
                error_desc->msg);
 
        switch ( error_desc->severity )
        {
        case DLS_WARNING:
	    /* Don't print anything for warnings. */
	    break;
        case DLS_ERROR:
            /* Print error message and return the severity. */
            fprintf(stderr, "%s", error_buf);
	    if (error_desc->num != lasterror)
	    {
		errcount = 1;
		lasterror = error_desc->num;
	    }
	    else if (++errcount > ERROR_THRESHOLD)
	    {
		return(DLS_FATAL);
	    }
            break;
        case DLS_FATAL:
            /* Dump the error message and exit gdmp960 */
            fprintf(stderr, "%s", error_buf);
            break;
        default:
            /* Ay yi yi.  God only knows what's going on here. */
            fprintf(stderr, 
		    "Internal error: Unexpected libdwarf error severity: %d\n",
		    error_desc->severity);
            return DLS_FATAL;
        }
        return(error_desc->severity);
    }
    else
    {
        fprintf(stderr, 
		"Internal error: NULL error descriptor in dwarf2_errhand.\n");
        return DLS_FATAL;
    }
}
