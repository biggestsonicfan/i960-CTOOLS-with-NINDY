/* b.out object file format
   Copyright (C) 1989, 1990, 1991 Free Software Foundation, Inc.

This file is part of GAS, the GNU Assembler.

GAS is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 1,
or (at your option) any later version.

GAS is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public
License along with GAS; see the file COPYING.  If not, write
to the Free Software Foundation, 675 Mass Ave, Cambridge, MA
02139, USA. */

/* $Id: obj_bout.h,v 1.19 1995/12/06 22:48:21 paulr Exp $ */

#define OBJ_BOUT 1

#include "bout.h"

#ifdef GNU960
#	include "tc_i960.h"
#else
#	include "targ-cpu.h"
#endif

/* Use "b.out" when input is from stdin instead of a file */
#define OBJ_DEFAULT_OUTPUT_FILE_NAME	"b.out"

extern const short seg_N_TYPE[];
extern const segT  N_TYPE_seg[];

#ifndef DEFAULT_MAGIC_NUMBER_FOR_OBJECT_FILE
#define DEFAULT_MAGIC_NUMBER_FOR_OBJECT_FILE	(BMAGIC)
#endif /* DEFAULT_MAGIC_NUMBER_FOR_OBJECT_FILE */

/* An OMF-specific function to write the object file. */
#define	write_object_file	write_bout_file

typedef struct nlist obj_symbol_type;

/* If compiler generate leading underscores, remove them. */

#ifndef STRIP_UNDERSCORE
#define STRIP_UNDERSCORE 0
#endif /* STRIP_UNDERSCORE */

/*
 *  Macros to extract information from a symbol table entry.
 *  This syntaxic indirection allows independence regarding a.out or coff.
 *  The argument (s) of all these macros is a pointer to a symbol table entry.
 */

/* Predicates */
/* True if the symbol is external */
#define S_IS_EXTERNAL(s)	((s)->sy_symbol.n_type & N_EXT)

#define S_IS_COMMON(s)          (S_GET_SEGTYPE(s) == SEG_UNKNOWN && (s)->sy_symbol.n_value > 0)

/* True if symbol has been defined, ie is in N_{TEXT,DATA,BSS,ABS} or N_EXT */
#define S_IS_DEFINED(s)		((S_GET_TYPE(s) != N_UNDF) || (S_GET_DESC(s) != 0))
#define S_IS_REGISTER(s)	((s)->sy_symbol.n_type == N_REGISTER)

/* True if a debug special symbol entry */
#define S_IS_DEBUG(s)		((s)->sy_symbol.n_type & N_STAB)

/* True if a symbol is local symbol name.
 * A symbol name whose name begin with ^A is a gas internal pseudo symbol.
 * Nameless symbols come from .stab directives.
 */
#define S_IS_LOCAL(s)		(S_GET_NAME(s) && ! S_IS_DEBUG(s) && \
        	 (S_GET_NAME(s)[0] == '\001' || S_GET_NAME(s)[0] == 'L' || S_GET_NAME(s)[0] == '.'))

/* True if a symbol is not defined in this file */
#define S_IS_EXTERN(s)		((s)->sy_symbol.n_type & N_EXT)
/* True if the symbol has been generated because of a .stabd directive */
#define S_IS_STABD(s)		(S_GET_NAME(s) == NULL)

/* Accessors */
/* The value of the symbol */
#define S_GET_VALUE(s)		((long) ((s)->sy_symbol.n_value))
/* The name of the symbol */
#define S_GET_NAME(s)		((s)->sy_symbol.n_un.n_name)
/* The pointer to the string table */
#define S_GET_OFFSET(s)		((s)->sy_symbol.n_un.n_strx)
/* The type of the symbol */
#define S_GET_TYPE(s)		((s)->sy_symbol.n_type & N_TYPE)

/* A coff compatibility kludge */
#define SF_SET_LOCAL(s)         ;
#define SF_GET_LOCAL(s)		(S_IS_LOCAL(s))
 
/* Getting the symbol's segment and getting its segment TYPE are the
 * same thing in b.out.
 */
#define S_GET_SEGTYPE(s)	(N_TYPE_seg[S_GET_TYPE(s)])
/* The numeric value of the segment */
#define S_GET_SEGMENT(s)	(obj_bout_s_get_segment(s))
/* The n_other expression value */
#define S_GET_OTHER(s)		((s)->sy_symbol.n_other)
/* The n_desc expression value */
#define S_GET_DESC(s)		((s)->sy_symbol.n_desc)

/* Modifiers */
/* Set the value of the symbol */
#define S_SET_VALUE(s,v)	((s)->sy_symbol.n_value = (unsigned long) (v))
/* Assume that a symbol cannot be simultaneously in more than one segment */
 /* set segment */
#define S_SET_SEGMENT(s,seg)	(obj_bout_s_set_segment(s, seg))
/* The symbol is external */
#define S_SET_EXTERNAL(s)	((s)->sy_symbol.n_type |= N_EXT)
/* The symbol is not external */
#define S_CLEAR_EXTERNAL(s)	((s)->sy_symbol.n_type &= ~N_EXT)
/* Set the name of the symbol */
#define S_SET_NAME(s,v)		((s)->sy_symbol.n_un.n_name = (v))
/* Set the offset in the string table */
#define S_SET_OFFSET(s,v)	((s)->sy_symbol.n_un.n_strx = (v))
/* Set the n_other expression value */
#define S_SET_OTHER(s,v)	((s)->sy_symbol.n_other = (v))
/* Set the n_desc expression value */
#define S_SET_DESC(s,v)		((s)->sy_symbol.n_desc = (v))

/* File header macro and type definition */

#define H_GET_FILE_SIZE(h)	(sizeof(struct exec) + \
				 H_GET_TEXT_SIZE(h) + H_GET_DATA_SIZE(h) + \
				 H_GET_SYMBOL_TABLE_SIZE(h) + \
				 H_GET_TEXT_RELOCATION_SIZE(h) + \
				 H_GET_DATA_RELOCATION_SIZE(h) + \
				 (h)->string_table_size)

#define H_GET_TEXT_SIZE(h)		((h)->header.a_text)
#define H_GET_DATA_SIZE(h)		((h)->header.a_data)
#define H_GET_BSS_SIZE(h)		((h)->header.a_bss)
#define H_GET_TEXT_RELOCATION_SIZE(h)	((h)->header.a_trsize)
#define H_GET_DATA_RELOCATION_SIZE(h)	((h)->header.a_drsize)
#define H_GET_SYMBOL_TABLE_SIZE(h)	((h)->header.a_syms)
#define H_GET_MAGIC_NUMBER(h)		((h)->header.a_info)
#define H_GET_ENTRY_POINT(h)		((h)->header.a_entry)
#define H_GET_STRING_SIZE(h)		((h)->string_table_size)
#ifdef EXEC_MACHINE_TYPE
#define H_GET_MACHINE_TYPE(h)		((h)->header.a_machtype)
#endif /* EXEC_MACHINE_TYPE */
#ifdef EXEC_VERSION
#define H_GET_VERSION(h)		((h)->header.a_version)
#endif /* EXEC_VERSION */

#define H_SET_TEXT_SIZE(h,v)		((h)->header.a_text = (v))
#define H_SET_DATA_SIZE(h,v)		((h)->header.a_data = (v))
#define H_SET_BSS_SIZE(h,v)		((h)->header.a_bss = (v))

#define H_SET_RELOCATION_SIZE(h,t,d)	(H_SET_TEXT_RELOCATION_SIZE((h),(t)),\
					 H_SET_DATA_RELOCATION_SIZE((h),(d)))

#define H_SET_TEXT_RELOCATION_SIZE(h,v)	((h)->header.a_trsize = (v))
#define H_SET_DATA_RELOCATION_SIZE(h,v)	((h)->header.a_drsize = (v))
#define H_SET_SYMBOL_TABLE_SIZE(h,v)	((h)->header.a_syms = (v) * \
					 sizeof(struct nlist))

#define H_SET_MAGIC_NUMBER(h,v)		((h)->header.a_magic = (v))

#define H_SET_ENTRY_POINT(h,v)		((h)->header.a_entry = (v))
#define H_SET_STRING_SIZE(h,v)		((h)->string_table_size = (v))
#ifdef EXEC_MACHINE_TYPE
#define H_SET_MACHINE_TYPE(h,v)		((h)->header.a_machtype = (v))
#endif /* EXEC_MACHINE_TYPE */
#ifdef EXEC_VERSION
#define H_SET_VERSION(h,v)		((h)->header.a_version = (v))
#endif /* EXEC_VERSION */

typedef struct {
    struct exec	header;			/* a.out header */
    long	string_table_size;	/* names + '\0' + sizeof(int) */
} object_headers;

/* unused hooks. */
#define OBJ_EMIT_LINENO(a, b, c)	;
#define obj_pre_write_hook(a)		;

/* FIXME: these are obsoleted by S_IS_LOMEM */
#define SF_GET_LOMEM(s)		(0)
#define SF_SET_LOMEM(s)		;

/* FIXME: COFF Pollution */
#define S_IS_SEGNAME(s)		(0)
