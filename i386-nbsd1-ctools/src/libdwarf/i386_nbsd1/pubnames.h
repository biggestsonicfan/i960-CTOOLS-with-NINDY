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

#ifndef PUBNAMES_H
#define PUBNAMES_H

#define DW_PN_HDR_LENGTH	14  /* Size of Pubnames Prolog */

struct dwarf_pubnames_prolog
{
    /* length of this CU's chunk of pubnames entries, including the prolog */
    Dwarf_Unsigned length;

    /* Dwarf version (we want 2) */
    Dwarf_Half     version;

    /* offset into .debug_info */
    Dwarf_Unsigned offset;     	

    /* contribution in .debug_info for this CU */
    Dwarf_Unsigned info_size;  	
};

struct dwarf_pubnames_table
{
    /* an array of tuples */
    Dwarf_Pn_Tuple pn_list; 

    /* number of tuples in the tuplelist */
    Dwarf_Unsigned pn_count;	

    /* The CU DIE that corresponds to this chunk of pubnames */
    Dwarf_Die pn_cu_die;

    /* The pubnames prolog for this chunk of pubnames */
    Dwarf_Pubnames_Prolog *pn_prolog;

    /* endianness of the section */
    Dwarf_Bool big_endian;  
};

#endif /* PUBNAMES_H */
