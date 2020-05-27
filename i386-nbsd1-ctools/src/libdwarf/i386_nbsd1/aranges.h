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

#ifndef ARANGES_H
#define ARANGES_H

#define DW_AR_HDR_LENGTH        12 /* Size of Aranges Prolog */
                                   /* Does NOT including padding */

struct dwarf_aranges_prolog
{
    /* length of this CU's chunk of aranges entries, including the prolog */
    Dwarf_Unsigned length;

    /* Dwarf version (we want 2) */
    Dwarf_Half     version;

    /* offset into .debug_info */
    Dwarf_Unsigned offset;

    /* size in bytes of a target address */
    Dwarf_Small    addr_size;

    /* size in bytes of a target segment descriptor */
    Dwarf_Small    seg_size;

    /* number of bytes between the prolog header and the first tuple */
    Dwarf_Signed  padding; 
};

struct dwarf_aranges_table 
{
    /* an array of tuples */
    Dwarf_Ar_Tuple  ar_list; 

    /* number of tuples in the tuplelist */
    Dwarf_Unsigned  ar_count;	

    /* The CU DIE that corresponds to this chunk of aranges */
    Dwarf_Die	    ar_cu_die;

    /* The arangees prolog for this chunk of aranges */
    Dwarf_Aranges_Prolog    *ar_prolog;

    /* endianness of the section */
    Dwarf_Bool	    big_endian;  
};

#endif  /* ARANGES_H */
