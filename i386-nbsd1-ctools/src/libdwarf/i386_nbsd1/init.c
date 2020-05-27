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

/****************************************************************************
*
*  ROUTINE:  dwarf_init
*
*  PURPOSE:  Return an instance of a 'Dwarf_Debug' type that represents a
*            handle for accessing debugging records associated with the
*            object file associated with the given open file descriptor.
*
*  ARGUMENTS:
*
*    stream   - An open FILE pointer associated with the object file
*               containing the dwarf debugging records.  We make no
*		assumptions about the OMF of the file.
*
*    access   - Allowed access: DLC_READ now, DLC_RDWR and DCL_WRITE soon.
*
*    sect_ptr - Pointer to an array of structures containing the pertinant 
*		section header information (name, offset, size, endian-ness)
*
*    sect_nbr - Number of sections passed.
*
*  RETURN:  NULL is returned if the object file contains no dwarf records or
*           if some other error occurred.  Otherwise, a handle that can be
*           used to reference dwarf records from the given object file is
*           returned.
* 
*  ERROR:  Returns NULL.
*****************************************************************************/
Dwarf_Debug
dwarf_init(stream, access, secs, numsec)
    FILE *stream;
    Dwarf_Unsigned access;
    Dwarf_Section secs;
    Dwarf_Unsigned numsec;
{
    Dwarf_Debug    dbg;
    int i, dw_sec_idx;
  
    RESET_ERR(); 

    /* make sure we got something, if 0 sections, error out */
    if (numsec <= 0) 
    { 
	if(LIBDWARF_ERR(DLE_MOF) == DLS_ERROR)  
	    return(NULL);
    }

    /*  Allocate a control record for this C.U. */
    dbg = (Dwarf_Debug)_dw_malloc(NULL, sizeof(struct Dwarf_Debug));
    _dw_clear((char *) dbg, sizeof(struct Dwarf_Debug));

    /* count the number of .debug_* sections */
    for ( i = 0; i < numsec; ++i )
        if ( ! strncmp(secs[i].name, ".debug_", 7) )
	    dbg->dbg_num_dw_scns++;

    dbg->dbg_sections =
        (Dwarf_Section) _dw_malloc(dbg, dbg->dbg_num_dw_scns * sizeof(struct Dwarf_Section)); 
    dbg->stream = stream;
    
    /* assign only the Dwarf sections to the dbg's sections array */
    for ( i = 0, dw_sec_idx = 0; i < numsec; ++i )
    {
        if ( ! strncmp(secs[i].name, ".debug_", 7) ) 
	{
	    dbg->dbg_sections[dw_sec_idx].file_offset = secs[i].file_offset;
	    dbg->dbg_sections[dw_sec_idx].size = secs[i].size;
	    dbg->dbg_sections[dw_sec_idx].name = secs[i].name;
	    dbg->dbg_sections[dw_sec_idx].big_endian = 
		secs[i].big_endian ? 1 : 0;
	    ++dw_sec_idx;
	} 
    }

    if ( _dw_build_cu_list(dbg) ) 
        return (Dwarf_Debug)dbg;
    else 
	return(NULL);
}

