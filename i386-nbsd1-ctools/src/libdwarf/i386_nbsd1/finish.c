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

  ROUTINE:  dwarf_finish
  
  PURPOSE:  Release all 'libdwarf' internal resources associated with the
	    given 'Dwarf_Debug' handle and invalidate that handle.
  
 	    For now, what this means is that we will simply rip through
	    the table maintained by _dw_malloc and _dw_realloc and free
            everything we find there.
 	     
 	    FIXME, this is an *extremely* crude method (sigh) due to time
 	    constraints.  It would be much better to recursively go through
	    the internal trees calling dwarf_dealloc on everything.  That 
	    means that some brave volunteer will have to add DLA_* constants
	    for all internal structures (defined in internal.h) in addition 
	    to the ones for standard libdwarf structures (defined in 
	    libdwarf.h.)

  ARGUMENTS:

    dbg - The 'Dwarf_Debug' handle associated with the set of dwarf debugging
          records being closed.

  RETURN: VOID
*****************************************************************************/
void
dwarf_finish(dbg)
    Dwarf_Debug dbg;
{
    Dwarf_Memory_Table *memtab = dbg->memory_table;
    int i;

    if (memtab == NULL || memtab->table == NULL)
	return;
    for ( i = 0; i < memtab->count; ++i )
    {
	if (memtab->table[i])
	    free(memtab->table[i]);
    }
    free(memtab->table);
    free(memtab);
    free(dbg);
}

 
dump_memtab(memtab)
    Dwarf_Memory_Table *memtab;
{
    int i;
    for ( i = 0; i < memtab->count; ++i )
    {
	printf("%4d.     0x%x\n", i, memtab->table[i]);
    }
}
