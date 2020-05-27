/* struct_symbol.h - Internal symbol structure
   Copyright (C) 1987 Free Software Foundation, Inc.

This file is part of GAS, the GNU Assembler.

GAS is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GAS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GAS; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* $Id: strucsym.h,v 1.14 1995/07/21 22:22:43 paulr Exp $ */

struct symbol			/* our version of an nlist node */
{
    /* the name we carry around with us; objects have various ways of
       representing the name in the object file */
    char *sy_name;		

    /* what we write in .o file */
    obj_symbol_type sy_symbol;

    /* 4-origin position of sy_name in symbols part of object file.
       0 for (nameless) .stabd symbols.  Used in write_object_file() */
    unsigned long sy_name_offset;
    
    /* 24 bit symbol number.  Symbol numbers start at 0 and are unsigned. */
    long sy_number;
    
    /* forward chain, or NULL */
    struct symbol *sy_next;

    /* backward chain, or NULL */
    struct symbol *sy_previous;

    /* NULL or -> frag this symbol attaches to. */
    struct frag *sy_frag;

    /* value is really that of this other symbol */
    struct symbol *sy_forward;

    /* Segment number (internal number system; OMF numbering is different */
    short sy_segment;

    /* 80960 extensions for internal use */
    /* Value of sysproc's system procedure table index (-1 - 259) */
    short i960_sys_index;


    /* The leaf entry point. (if same as call entry point, this is set to
       the call entry point) */
    struct symbol *bal_entry_symbol;

    /* A series of flags for internal use */
    /* General use flags */
    /* Does this symbol represent a section name? */
    unsigned is_segname : 1;

    /* Does this symbol represent a file name? */
    unsigned is_dot_file : 1;

    /* Local symbol; don't emit to the object file. */
    unsigned is_local : 1;

    /* 80960-specific flags */
    /* Does this symbol represent a leafproc call entry point? */
    unsigned is_leafproc : 1;

    /* Does this symbol represent a leafproc bal entry point? */
    unsigned is_balname : 1;

    /* Does this symbol represent a system procedure? */
    unsigned is_sysproc : 1;

    /* Can this symbol be reached with a MEMA instruction? (0 - 4K) */
    unsigned is_lomem : 1;
};

typedef struct symbol symbolS;

typedef unsigned valueT;	/* The type of n_value. Helps casting. */

#define SEGMENT_TO_SYMBOL_TYPE(seg)  ( seg_N_TYPE [(int) (seg)] )
extern const short seg_N_TYPE[]; /* obj_XXX.c */

#define	N_REGISTER	30	/* Fake N_TYPE value for SEG_REGISTER */

#ifdef __STDC__

void symbol_clear_list_pointers(symbolS *symbolP);
void symbol_insert(symbolS *addme, symbolS *target, symbolS **rootP, symbolS **lastP);
void symbol_remove(symbolS *symbolP, symbolS **rootP, symbolS **lastP);
void verify_symbol_chain(symbolS *rootP, symbolS *lastP);

#else /* __STDC__ */

void symbol_clear_list_pointers();
void symbol_insert();
void symbol_remove();
void verify_symbol_chain();

#endif /* __STDC__ */

#define symbol_previous(s) ((s)->sy_previous)

#ifdef __STDC__
void symbol_append(symbolS *addme, symbolS *target, symbolS **rootP, symbolS **lastP);
#else /* __STDC__ */
void symbol_append();
#endif /* __STDC__ */

#define symbol_next(s)	((s)->sy_next)

/*
 * Local Variables:
 * comment-column: 0
 * fill-column: 131
 * End:
 */

/* end of struc-symbol.h */
