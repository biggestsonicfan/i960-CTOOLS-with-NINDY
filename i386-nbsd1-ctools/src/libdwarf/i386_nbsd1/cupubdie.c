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
#include "pubnames.h"

/* 
 * dwarf_cu_pubdie - Searches the .debug_pubnames section for an offset/name
 * pair whose name is "name".  This function uses the first occurrence
 * of "name" if there is more than one occurrence.  
 *
 * For fast startup, we don't read in pubnames routinely in dwarf_init.
 * But for sanity, we'll build pubnames all-or-nothing; i.e. if we are
 * here, build pubnames tables for ALL compilation units, then run a 
 * simplified search algorithm.
 *
 * RETURNS: The Dwarf_Die of the CU-DIE that owns the DIE that the 
 * offset/name pair refers to if it can find one.  Otherwise NULL.
 *
 * ERROR: Return NULL
 */
Dwarf_Die
dwarf_cu_pubdie(dbg, name)
    Dwarf_Debug dbg; 
    char *name; 
{
    Dwarf_CU_List  *cu_node = dbg->dbg_cu_list;

    RESET_ERR();

    /* If pubnames have not been read in for the first file, they 
       haven't been read in for any files.  Build tables for ALL files,
       then continue with search as if nothing had happened. */
    if (!cu_node->pubnames_table)
    {
	_dw_build_pubnames(dbg);
    }

    /* go through the entire file, trying each CU's pubnames table
       until we find a match or reach the end of the file */
    for (cu_node = dbg->dbg_cu_list; cu_node; cu_node = cu_node->next)
    {
	Dwarf_Pubnames_Table *pn_table = cu_node->pubnames_table;

	/* 
	 * Only do the search if we have a tuple list. Some compilers, 
	 * notably WATCOM, add a header in the .debug_pubnames 
	 * section for a CU, but without any tuples.
	 */
	if (pn_table->pn_list) 
	{
	    if (_dw_lookup_pn_tuple(dbg, name, pn_table->pn_list, 
				    pn_table->pn_count)) 
		return(pn_table->pn_cu_die);
	}
	else
	{
	    /* no tuple list for this CU; don't error out, in case there's
	       a list in a later CU */
        }
    }
    return(NULL);
}

