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

/*
 * dwarf_version -- This gets the version information from the
 * cu_node's header (where the original CU-header was read into).
 * This will be accessible from all DIEs instead of from just the
 * CU-DIE. 
 *
 * RETURNS: The version from the CU-header for this CU.
 *
 * ERROR: Returns -1.
 */
Dwarf_Signed
dwarf_version(die)
    Dwarf_Die die;
{
    RESET_ERR();
    if(die) {
        if(die->cu_node->die_head->cu_header)
            return((Dwarf_Signed)die->cu_node->die_head->cu_header->version); 
        else {
            LIBDWARF_ERR(DLE_MDE);
            return(-1);
	}
    }
    else { 
	LIBDWARF_ERR(DLE_IA);
        return(-1);
    }
}

