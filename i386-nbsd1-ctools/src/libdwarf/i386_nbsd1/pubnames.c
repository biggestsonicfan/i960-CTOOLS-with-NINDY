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

/* Support routines for .debug_pubnames delivery functions.
   Functions here are not intended to be called directly by the debugger.  */

#include "libdwarf.h"
#include "internal.h"
#include "pubnames.h"

/* Functions used in this file only */
static
compare_tuples PARAMS ((Dwarf_Pn_Tuple, Dwarf_Pn_Tuple));

static 
sort_pn_tuples PARAMS ((Dwarf_Pn_Tuple, Dwarf_Unsigned));
/*
 * compare_tuples -- Comparator function for qsort and bsearch,
 * for the tuples in the pubnames table.
 */
static
compare_tuples(t1, t2)
    Dwarf_Pn_Tuple t1, t2;
{
    return strcmp(t1->name, t2->name);
}

/*
 * sort_pn_tuples -- sorts the tuples based on the name 
 * for fast access later 
 */ 
static 
sort_pn_tuples(tuple_table, tuple_count)
    Dwarf_Pn_Tuple tuple_table;  /* for tuple table */
    Dwarf_Unsigned tuple_count;   /* number of tuples in table */
{
    qsort((void *)tuple_table, tuple_count, sizeof(struct Dwarf_Pn_Tuple), compare_tuples);
}

Dwarf_Pn_Tuple
_dw_lookup_pn_tuple(dbg, key, tuple_table, tuple_count)
    Dwarf_Debug dbg;
    char* key;
    Dwarf_Pn_Tuple tuple_table;
    Dwarf_Unsigned tuple_count;
{
    /* Use same tuple over and over again for searching */
    static struct Dwarf_Pn_Tuple tuple;
    static int namesize = 256;
    int len = strlen(key);

    if ( ! tuple.name )
    {
	tuple.name = _dw_malloc(dbg, namesize);
    }
    if ( len >= namesize )
    {
	namesize *= 2;
	if ( len >= namesize )
	    namesize = len + 1;
	tuple.name = _dw_realloc(dbg, tuple.name, namesize);
    }
    strcpy(tuple.name, key);
    return (Dwarf_Pn_Tuple)
	bsearch((void*) &tuple, (void*) tuple_table, tuple_count,
		sizeof(struct Dwarf_Pn_Tuple), compare_tuples);
}

/* 
 * add_pn_tuple -- helper function for build_pubnames_table.
 * Add one tuple to the pubname-header's tuple list.
 * Reallocate the list if needed (if adding a new name will exceed
 * the current size of the list.)
 *
 */
static void
add_pn_tuple(dbg, tuple, tuple_list, tuple_list_size, index)
    Dwarf_Debug dbg;
    Dwarf_Pn_Tuple tuple;
    Dwarf_Pn_Tuple *tuple_list;
    Dwarf_Unsigned *tuple_list_size;
    Dwarf_Unsigned index;
{
    if ( index >= *tuple_list_size )
    {
	if ( *tuple_list )
	{
	    /* Realloc for twice the current size */
	    *tuple_list = (Dwarf_Pn_Tuple)
		_dw_realloc(dbg, (void*) *tuple_list,
			  *tuple_list_size * sizeof(struct Dwarf_Pn_Tuple) * 2);
	    *tuple_list_size *= 2;
	}
	else
	{
	    /* First time this has been called.  Allocate a reasonable number
	       of tuples; we'll realloc later if needed. */
	    *tuple_list_size = 256;
	    *tuple_list = (Dwarf_Pn_Tuple)
		_dw_malloc(dbg, *tuple_list_size * sizeof(struct Dwarf_Pn_Tuple));
	}
    }
    (*tuple_list)[index] = *tuple;
}


/*
  ROUTINE: build_pubnames_table()

  PURPOSE: parse the raw bits from one of the CU contributions to the
    .debug_pubnames section into a usable structure.
 	
    We'll do this by malloc'ing a reasonable number of headers
    and tuples per header, then start filling them in until we
    run out of memory and have to realloc.  When done, we'll
    have the number of headers and the number of tuples per header.
  
  RETURN: Dwarf_Pn_Tuple, which is an array of offset/name pairs.
 
*/

static
Dwarf_Pn_Tuple
build_pubnames_table(dbg, prolog, tuple_count, byte_order)
    Dwarf_Debug dbg;
    Dwarf_Pubnames_Prolog *prolog;
    Dwarf_Unsigned *tuple_count;
    Dwarf_Bool byte_order;
{
    Dwarf_Pn_Tuple tuple_list = NULL;    /* The result */
    struct Dwarf_Pn_Tuple tuple;   /* A temporary working pubnames tuple */
    Dwarf_Unsigned tuple_list_size = 0;	/* Will be set by add_pn_tuple */
    Dwarf_Unsigned index = 0;	/* Current index into the tuple list */
    long total_bytes; /* Tuple bytes, this CU */
    long bytecount; /* How many tuple bytes have we read? */
    long start; /* file position on entry */

    for (bytecount = 0, total_bytes = prolog->length - DW_PN_HDR_LENGTH,
	 start = _dw_ftell(dbg->stream);
	 bytecount < total_bytes;
	 bytecount = _dw_ftell(dbg->stream) - start)
    {
	tuple.offset = _dw_read_constant(dbg, 4, byte_order);
	if ( tuple.offset == 0 )
	{
	    /* This signals the end of this set of pubnames. (7.19)
	       It should match the calculated bytecount also. */
	    bytecount += 4;
	    break;
	}
	tuple.name = _dw_build_a_string(dbg);
	add_pn_tuple(dbg, &tuple, &tuple_list, &tuple_list_size, index++);
    }
    sort_pn_tuples(tuple_list, index);
    *tuple_count = index;
    return tuple_list;
}

static
Dwarf_Pubnames_Prolog *
read_pubnames_prolog(dbg, byte_order)
    Dwarf_Debug dbg;
    int byte_order;
{
    Dwarf_Pubnames_Prolog *prolog = (Dwarf_Pubnames_Prolog *)
	_dw_malloc(dbg, sizeof(Dwarf_Pubnames_Prolog));
    prolog->length = _dw_read_constant(dbg, 4, byte_order) + 4;
    prolog->version = (Dwarf_Half) _dw_read_constant(dbg, 2, byte_order);
    prolog->offset = _dw_read_constant(dbg, 4, byte_order);
    prolog->info_size = _dw_read_constant(dbg, 4, byte_order);
    return prolog;
}

/* 
 * _dw_build_pubnames
 *
 * Build ALL pubnames tables for ALL compilation units all at once.
 * This is most efficient because there doesn't appear to be a way
 * to find a specific CU's offset into the .debug_pubnames section. 
 */
void
_dw_build_pubnames(dbg)
    Dwarf_Debug dbg;
{
    Dwarf_Section section = _dw_find_section(dbg, ".debug_pubnames");
    Dwarf_CU_List *cu_node;

    if (!section && LIBDWARF_ERR(DLE_NOPUB) == DLS_ERROR)
	return;

    /* Set file pointer to start of .debug_pubnames */
    _dw_fseek(dbg->stream, section->file_offset, SEEK_SET);

    for (cu_node = dbg->dbg_cu_list; cu_node; cu_node = cu_node->next)
    {
	Dwarf_Pubnames_Table *pn_table = (Dwarf_Pubnames_Table *)
	    _dw_malloc(dbg, sizeof(Dwarf_Pubnames_Table));

	pn_table->big_endian = section->big_endian;
	pn_table->pn_cu_die = cu_node->die_head->die_list;
	pn_table->pn_prolog = read_pubnames_prolog(dbg, section->big_endian);
	pn_table->pn_list =
	    build_pubnames_table(dbg, pn_table->pn_prolog,
				 &pn_table->pn_count,
				 pn_table->big_endian);
	cu_node->pubnames_table = pn_table; 
    }
}
