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
#include "aranges.h"

/*
    LIBRARY FUNCTION
 
        dwarf_cu_rangedie -- Searches the .debug_aranges section for an 
            address/length pair whose address is "addr". 
 
    SYNOPSIS
 
        Dwarf_Die dwarf_cu_rangedie(Dwarf_Debug dbg,
                                    Dwarf_Addr addr);

    DESCRIPTION
 
        For fast startup, we don't read in aranges routinely in dwarf_init.
        But for sanity, we'll build aranges all-or-nothing; if we are
        here for the first time, build aranges tables for ALL compilation 
        units, then run a simplified search algorithm.

    RETURNS
 
        The Dwarf_Die descriptor of the compilation unit that owns the die
        which "addr" refers to in the .debug_aranges section.  Returns
        NULL if no arange descriptors are found in any compilation unit
        with address = "addr".
*/

Dwarf_Die
dwarf_cu_rangedie(dbg, addr)
    Dwarf_Debug dbg; 
    Dwarf_Addr addr;
{
    Dwarf_CU_List  *cu_node = dbg->dbg_cu_list;

    RESET_ERR();

    /* If aranges have not been read in for the first file, they haven't
       been read in for any file.  Build tables for ALL files, then 
       continue with the search as if nothing happened. */
    if (!cu_node->aranges_table)
    {
        _dw_build_aranges(dbg);
    }

    /* go through the entire file, trying each CU's aranges table until
       we find a match or reach the end of the file */
    for (cu_node = dbg->dbg_cu_list; cu_node; cu_node = cu_node->next)
    {
        Dwarf_Aranges_Table *ar_table = cu_node->aranges_table;
 
        /*
         * Only do the search if we have a tuple list. Some compilers
         * may add a header in the .debug_aranges section for a CU, but 
         * without any tuples.
         */
        if (ar_table->ar_list)
        {
            if (_dw_lookup_ar_tuple(dbg, addr, ar_table->ar_list,
                                    ar_table->ar_count))
                return(ar_table->ar_cu_die);
        }
        else
        {
            /* no tuple list for this CU; don't error out, in case there's
               a list in a later CU */
        }
    }
    return(NULL);
}

