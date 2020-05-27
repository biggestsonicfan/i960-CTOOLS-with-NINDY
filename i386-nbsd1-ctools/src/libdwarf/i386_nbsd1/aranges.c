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

/* Support routines for .debug_aranges delivery functions.
   Functions here are not intended to be called directly by the debugger. */

/*
 * add_ar_tuple -- helper function for build_aranges_table.
 * Add one tuple to this CU's tuple list.
 * Reallocate the list if needed (if adding a new name will exceed
 * the current size of the list.)
 *
 */
static
void
add_ar_tuple(dbg, tuple, tuple_list, tuple_list_size, index)
    Dwarf_Debug dbg;
    Dwarf_Ar_Tuple tuple;
    Dwarf_Ar_Tuple *tuple_list;
    Dwarf_Unsigned *tuple_list_size;
    Dwarf_Unsigned index;
{
    if ( index >= *tuple_list_size )
    {
        if ( *tuple_list )
        {
            /* Realloc for twice the current size */
            *tuple_list = (Dwarf_Ar_Tuple)
                _dw_realloc(dbg, (void*) *tuple_list,
                          *tuple_list_size * sizeof(struct Dwarf_Ar_Tuple) * 2);
            *tuple_list_size *= 2;
        }
        else
        {
            /* First time this has been called.  Allocate a reasonable number
               of tuples; we'll realloc later if needed. */
            *tuple_list_size = 256;
            *tuple_list = (Dwarf_Ar_Tuple) _dw_malloc(
                         dbg, *tuple_list_size * sizeof(struct Dwarf_Ar_Tuple));
        }
    }
    (*tuple_list)[index] = *tuple;
}

/*
    ROUTINE: build_aranges_table()

    PURPOSE: parse the raw bits from one of the CU contributions to the 
    .debug_aranges section into a usable structure.  

    We'll do this by initially malloc'ing a reasonable number of tuples
    for this CU and then start filling them in until we run out of memory
    and have to realloc.  

    RETURN: Dwarf_Ar_Tuple, containing an array of low_addr/high_addr pairs.
    (Note: high_addr rather than length is stored in Dwarf_Ar_Tuple.)

    FIXME: Does prolog padding really need to be in Dwarf_Aranges_Prolog?
*/

static
Dwarf_Ar_Tuple
build_aranges_table(dbg, prolog, tuple_count, byte_order)
    Dwarf_Debug dbg;
    Dwarf_Aranges_Prolog *prolog;
    Dwarf_Unsigned *tuple_count;
    Dwarf_Bool byte_order;
{
    Dwarf_Ar_Tuple tuple_list = NULL;    /* The result */
    struct Dwarf_Ar_Tuple tuple;         /* A temporary working aranges tuple */
    Dwarf_Unsigned tuple_list_size = 0;  /* Will be set by add_ar_tuple */
    Dwarf_Unsigned index = 0;            /* Current index into the tuple list */
    long total_bytes;                    /* Total tuple bytes, this CU */
    long byte_count;                     /* How many tuple bytes have we read */
    long start;                          /* File position on entry */

    total_bytes = prolog->length - DW_AR_HDR_LENGTH - prolog->padding;

    for (byte_count = 0, start = _dw_ftell(dbg->stream);
         byte_count < total_bytes;
         byte_count = _dw_ftell(dbg->stream) - start)
    {
        Dwarf_Unsigned length;          

        tuple.low_addr = (Dwarf_Addr) 
                         _dw_read_constant(dbg, prolog->addr_size, byte_order);

        length = _dw_read_constant(dbg, prolog->addr_size, byte_order);

        if (tuple.low_addr == 0 && length == 0)
        {
            /* This signals the end of this set of aranges.  (7.20)
               It should match the calculated byte_count also. */
            byte_count += (2 * prolog->addr_size);
            break;
        }
        tuple.high_addr = (Dwarf_Addr) 
                           ((unsigned long) tuple.low_addr + length);
        add_ar_tuple(dbg, &tuple, &tuple_list, &tuple_list_size, index++);
    }
    *tuple_count = index;
    return tuple_list;
}

/*
 * _dw_lookup_ar_tuple
 *
 * Searches tuple_table for "addr" and returns the tuple (or NULL if not
 * found.  
 *
 * FIXME: some brave volunteer should sort this list so that bsearch can
 * be used to find ar_tuples!
 */
Dwarf_Ar_Tuple
_dw_lookup_ar_tuple(dbg, addr, tuple_table, tuple_count)
    Dwarf_Debug dbg;
    Dwarf_Addr addr;
    Dwarf_Ar_Tuple tuple_table;
    Dwarf_Unsigned tuple_count;
{
    Dwarf_Ar_Tuple tuple;
    int i;

    for (i = 0; i < tuple_count; i++) 
    {
        tuple = &tuple_table[i];
        if (addr >= tuple->low_addr && addr < tuple->high_addr) 
            return tuple;
    }
    return NULL;
}

/*
 * pad_to_double_word
 *
 * The prolog of the .debug_aranges section must be padded (if necessary)
 * so the address/length data begin at mod 8 from the beginning of
 * the .debug_aranges section.  The "aranges_start" parameter is set to
 * the very beginning of .debug_aranges (before ANY data has been read).
 *
 * RETURNS: the number of bytes between the prolog header and the first
 * tuple for this CU.
 *
 * FIXME: Do we really need to return anything here?
 */
static
long
pad_to_double_word(dbg, addr_size, aranges_start)
    Dwarf_Debug dbg;
    Dwarf_Small addr_size;
    long aranges_start;
{
    long n = _dw_ftell(dbg->stream) - aranges_start; 
    long padding = 0;
    if ( n & ((addr_size * 2) - 1) ) 
    {
        padding = 8 - (n & ((addr_size * 2) - 1));
        _dw_fseek(dbg->stream, padding, SEEK_CUR);
    }
    return padding;
}

/*
 * read_aranges_prolog
 *
 * Read the 12 byte header.  Advance the current file pointer to the
 * first tuple for this C.U. and save the number of "padding" bytes 
 * (see pad_to_double_word above for details).
 */
static
Dwarf_Aranges_Prolog *
read_aranges_prolog(dbg, aranges_start, byte_order)
    Dwarf_Debug dbg;
    long aranges_start;
    int byte_order;
{
    Dwarf_Aranges_Prolog *prolog = (Dwarf_Aranges_Prolog *)
        _dw_malloc(dbg, sizeof(Dwarf_Aranges_Prolog));

    prolog->length = _dw_read_constant(dbg, 4, byte_order) + 4;
    prolog->version = (Dwarf_Half) _dw_read_constant(dbg, 2, byte_order);
    prolog->offset = _dw_read_constant(dbg, 4, byte_order);
    prolog->addr_size = (Dwarf_Small) _dw_read_constant(dbg, 1, byte_order);
    prolog->seg_size = (Dwarf_Small) _dw_read_constant(dbg, 1, byte_order);

    prolog->padding = pad_to_double_word(dbg, prolog->addr_size, aranges_start);

    return prolog;
}

/*
 * _dw_build_aranges
 *
 * Build ALL arange tables for ALL compilation units all at once.
 * This is most efficient because there doesn't appear to be a way
 * to find a specific CU's offset into the .debug_aranges section.
 */
void
_dw_build_aranges(dbg)
    Dwarf_Debug dbg;
{
    Dwarf_Section section = _dw_find_section(dbg, ".debug_aranges");
    Dwarf_CU_List *cu_node;

    /* Remember the file position at the beginning of the .debug_aranges
       section.  This is used in pad_to_double_word.  */
    long aranges_start; 

    if (!section && LIBDWARF_ERR(DLE_NOARANG) == DLS_ERROR)
        return;

    /* Set file pointer to start of .debug_aranges */
    _dw_fseek(dbg->stream, section->file_offset, SEEK_SET);

    aranges_start = _dw_ftell(dbg->stream);

    for (cu_node = dbg->dbg_cu_list; cu_node; cu_node = cu_node->next)
    {
        Dwarf_Aranges_Table *ar_table = (Dwarf_Aranges_Table *)
            _dw_malloc(dbg, sizeof(Dwarf_Aranges_Table));
 
        ar_table->big_endian = section->big_endian;
        ar_table->ar_cu_die = cu_node->die_head->die_list;
        ar_table->ar_prolog = 
            read_aranges_prolog(dbg, aranges_start, section->big_endian);
        ar_table->ar_list =
            build_aranges_table(dbg, ar_table->ar_prolog,
                                 &ar_table->ar_count,
                                 ar_table->big_endian);
        cu_node->aranges_table = ar_table;
    }
}

