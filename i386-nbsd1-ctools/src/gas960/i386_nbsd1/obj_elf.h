/* ELF object file format
   Copyright (C) 1989, 1990, 1991 Free Software Foundation, Inc.

This file is part of GAS.

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

/* $Id: obj_elf.h,v 1.6 1994/11/23 15:50:14 paulr Exp $ */

#define OBJ_ELF 1

#include "tc_i960.h"
#include "elf.h"

/* FIXME: TEMP! Macros from coff.h */
#define F_BIG_ENDIAN_TARGET	0x0400
#define F_CCINFO	0x0800
#define	F_I960TYPE	0xf000
#define	F_I960CORE	0x1000
#define	F_I960CORE1	0x1000
#define	F_I960KB	0x2000
#define	F_I960SB	0x2000
#define	F_I960CA	0x5000
#define	F_I960CX	0x5000
#define	F_I960KA	0x6000
#define	F_I960SA	0x6000
#define	F_I960JX	0x7000
#define	F_I960HX	0x8000
#define	F_I960CORE2	0x9000
#define F_PTRIZED       0xf000
/* End coff.h */

/* Use "e.out" when input is from stdin instead of a file */
#define OBJ_DEFAULT_OUTPUT_FILE_NAME	"e.out"

extern const short seg_N_TYPE[];
extern const segT  N_TYPE_seg[];

/* Magic number of paged executable. 
   FIXME: this is never used.  Remove from write.c */
#define DEFAULT_MAGIC_NUMBER_FOR_OBJECT_FILE	0

/* An OMF-specific function to write the object file. */
#define	write_object_file	write_elf_file

/* If compiler generate leading underscores, remove them. */

#ifndef STRIP_UNDERSCORE
#define STRIP_UNDERSCORE 0
#endif /* STRIP_UNDERSCORE */
#define DO_NOT_STRIP	0
#define DO_STRIP	1

/* Symbol table entry data type; used in gas960 as (symbolS *)->sy_symbol
   Elf32_Sym is declared in ../../include/elf.h */
typedef Elf32_Sym obj_symbol_type;

/* Symbol table macros and constants */

/* Possible and useful section number in symbol table 
 * The values of TEXT, DATA and BSS may not be portable.
 *
 * FIXME: this is for backwards-compatibility to the beginning of time.
 * there must be a workaround.
 */
#define N_UNDEF 0  /* undefined symbol */
#define N_ABS   -1 /* value of symbol is absolute */
#define N_DEBUG -2 /* debugging symbol -- value is meaningless */
#define N_TV    -3 /* indicates symbol needs preload transfer vector */
#define P_TV    -4 /* indicates symbol needs postload transfer vector*/

#define C_TEXT_SECTION		1
#define C_DATA_SECTION		2
#define C_BSS_SECTION		3
#define C_ABS_SECTION		N_ABS
#define C_UNDEF_SECTION		N_UNDEF
#define C_DEBUG_SECTION		N_DEBUG
#define C_NTV_SECTION		N_TV
#define C_PTV_SECTION		P_TV

/*
 *  Macros to extract information from a symbol table entry.
 *  This syntactic indirection allows independence regarding a.out or coff.
 *  The argument (s) of all these macros is a pointer to a symbol table entry.
 */

/* Predicates */
/* True if the symbol is external */

#define S_IS_EXTERNAL(s)  (S_GET_STORAGE_CLASS(s) == STB_GLOBAL)

/* True if symbol has been defined, ie :
  section > 0 (DATA, TEXT or BSS)
  section == N_ABS  sac 
  section == 0 and value > 0 (external bss symbol) */
#define S_IS_DEFINED(s)	(S_GET_SEGMENT(s) >= FIRST_PROGRAM_SEGMENT || \
			 S_GET_SEGTYPE(s) == SEG_ABSOLUTE || \
			 (S_GET_SEGTYPE(s) == SEG_UNKNOWN && \
			  (s)->sy_symbol.st_value > 0))

/* True if a debug special symbol entry */
#define S_IS_DEBUG(s)		(S_GET_SEGTYPE(s) == SEG_DEBUG)

/* True if a symbol is local symbol name.
 * A symbol name whose name begin with ^A is a gas internal pseudo symbol.
 * NOTE:  This macro must be used ONLY in symbols.c:colon().  Elsewhere
 * use SF_GET_LOCAL().
 */
#define S_IS_LOCAL(s)		(S_GET_NAME(s)[0] == '\001' || \
				 S_GET_NAME(s)[0] == 'L' || \
				 S_GET_NAME(s)[0] == '.')

/* True if a symbol is not defined in this file */
#define S_IS_EXTERN(s)		 (S_GET_SEGTYPE(s) == SEG_UNKNOWN && (s)->sy_symbol.st_value == 0)

/* True if a symbol can be multiply defined  */
#define S_IS_COMMON(s)		 (S_GET_SEGTYPE(s) == SEG_UNKNOWN && (s)->sy_symbol.st_value != 0)

/* If the user specifies a common symbol without an alignment, we place
   the following into the symbol's value field.  Later, this is emitted
   to the elf symbol table as 0. */
#define ELF_DFLT_COMMON_ALIGNMENT 3

/* True if a symbol represents a function name */
#define S_IS_FUNCTION(s)	(S_GET_DATA_TYPE(s) == STT_FUNC)

/* True if a symbol represents a section name */
#define S_IS_SEGNAME(s)		((s)->is_segname)

/* True if a symbol represents the file name */
#define S_IS_DOTFILE(s)		((s)->is_dot_file)

/* Accessors */
/* The name of the symbol */
#define S_GET_NAME(s)		((s)->sy_name)
/* The value of the symbol */
#define S_GET_VALUE(s)		((s)->sy_symbol.st_value)
/* The size of the symbol */
#define S_GET_SIZE(s)		((s)->sy_symbol.st_size)
/* The index of the segment in the segs array */
#define S_GET_SEGMENT(s)        ((s)->sy_segment)
/* The type, or logical contents, of the segment */
#define S_GET_SEGTYPE(s)        (segs[S_GET_SEGMENT(s)].seg_type)
/* The data type */
#define S_GET_DATA_TYPE(s)	(ELF32_ST_TYPE((s)->sy_symbol.st_info))
/* The storage class */
/* For ELF, this is really the data binding */
#define S_GET_STORAGE_CLASS(s)	(ELF32_ST_BIND((s)->sy_symbol.st_info))
/* The st_other field: */
#define S_GET_ST_OTHER(s)       ((s)->sy_symbol.st_other)

/* Modifiers */
/* Set the name of the symbol */
#define S_SET_NAME(s,v)		((s)->sy_name = (v))
/* Set the value of the symbol */
#define S_SET_VALUE(s,v)	((s)->sy_symbol.st_value = (v))
/* Set the size of the symbol */
#define S_SET_SIZE(s,v)		((s)->sy_symbol.st_size = (v))
/* Set the flags */
#define S_SET_FLAGS(s,v)	((s)->sy_symbol.n_flags = (v))
/* The index of the segment in the segs array */
#define S_SET_SEGMENT(s,v)      ((s)->sy_segment = (v))
/* The section index within the output Elf symbol */
#define S_SET_ELF_SECTION(s,v)	((s)->sy_symbol.st_shndx = (v))
/* The data type */
#define S_SET_DATA_TYPE(s,v)	((s)->sy_symbol.st_info = ELF32_ST_INFO(ELF32_ST_BIND((s)->sy_symbol.st_info),(v)))
/* The storage class */
/* For ELF, this is really the data binding */
#define S_SET_STORAGE_CLASS(s,v)	((s)->sy_symbol.st_info = ELF32_ST_INFO((v),ELF32_ST_TYPE((s)->sy_symbol.st_info)))
/* The st_other field: */
#define S_SET_ST_OTHER(s,v)     ((s)->sy_symbol.st_other = (v))

/* Additional modifiers */
/* The symbol is external (does not mean undefined) */
#define S_SET_EXTERNAL(s)   	S_SET_STORAGE_CLASS((s), STB_GLOBAL)
/* The symbol is a section name */
#define S_SET_SEGNAME(s)	((s)->is_segname = 1)

/* FIXME: coff compatibility kludges */
#define SF_SET_LOCAL(s)         ((s)->is_local = 1)
#define SF_CLEAR_LOCAL(s)	((s)->is_local = 0)
#define SF_GET_LOCAL(s)		((s)->is_local)

#define SF_SET_LOMEM(s)         ((s)->is_lomem = 1)
#define SF_CLEAR_LOMEM(s)	((s)->is_lomem = 0)
#define SF_GET_LOMEM(s)		((s)->is_lomem)

/* Same general idea for Elf relocation structures */
/* Accessors */
#define R_GET_OFFSET(r)		((r)->r_offset)
#define R_GET_SYMBOL(r)		(ELF32_R_SYM((r)->r_info))
#define R_GET_TYPE(r)		(ELF32_R_TYPE((r)->r_info))

/* Modifiers */
#define R_SET_OFFSET(r, v)	((r)->r_offset = (v))
#define R_SET_SYMBOL(r, v)	((r)->r_info = ELF32_R_INFO((v), R_GET_TYPE(r)))
#define R_SET_TYPE(r, v)	((r)->r_info = ELF32_R_INFO(R_GET_SYMBOL(r), (v)))

/* A structure used to wire in certain Elf section header fields based on the 
   section name.  See declaration of scnhook_table[] in obj_elf.c. */
struct scnhook 
{
    char *name;
    Elf32_Word type;
    Elf32_Word flags;
    Elf32_Word entsize;
};

/* Segment flipping */
#define segment_name(v)	(segs[(v)].seg_name)

typedef struct {
    Elf32_Ehdr		elfhdr;	/* ELF header */
    unsigned long	string_table_size;
    unsigned long	relocation_size;  /* Total size of relocation
					     information for all sections in
					     bytes. */
} object_headers;

#ifdef __STDC__

void c_dot_file_symbol(char *filename);
void obj_extra_stuff(object_headers *headers);
void output_file_align(void);
void tc_coff_headers_hook(object_headers *headers);
void tc_coff_symbol_emit_hook(struct symbol *symbolP);

#else /* __STDC__ */

void c_dot_file_symbol();
void obj_extra_stuff();
void output_file_align();
void tc_coff_headers_hook();
void tc_coff_symbol_emit_hook();

#endif /* __STDC__ */
