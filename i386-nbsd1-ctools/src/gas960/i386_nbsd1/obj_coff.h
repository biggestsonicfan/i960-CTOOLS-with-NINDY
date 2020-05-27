/* coff object file format
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

/* $Id: obj_coff.h,v 1.27 1994/08/11 17:10:16 peters Exp $ */

#define OBJ_COFF 1

#ifdef GNU960
#	include "tc_i960.h"
#	include "coff.h"
#else
#	include "targ-cpu.h"
#	include "intel-coff.h"
#endif


#ifdef USE_NATIVE_HEADERS
#include <filehdr.h>
#include <aouthdr.h>
#include <scnhdr.h>
#include <storclass.h>
#include <linenum.h>
#include <syms.h>
#include <reloc.h>
#include <sys/types.h>
#endif /* USE_NATIVE_HEADERS */

/* NOTE: target byte order is set in tc_i960.c:tc_coff_headers_hook() */
#define BYTE_ORDERING		0
#ifndef FILE_HEADER_MAGIC
#define FILE_HEADER_MAGIC	I960ROMAGIC  /* ... */
#endif /* FILE_HEADER_MAGIC */

#ifndef OBJ_COFF_MAX_AUXENTRIES
#define OBJ_COFF_MAX_AUXENTRIES 1
#endif /* OBJ_COFF_MAX_AUXENTRIES */

/* Use "a.out" when input is from stdin instead of a file */
#define OBJ_DEFAULT_OUTPUT_FILE_NAME	"a.out"

extern const short seg_N_TYPE[];
extern const segT  N_TYPE_seg[];

/* Magic number of paged executable. */
#define DEFAULT_MAGIC_NUMBER_FOR_OBJECT_FILE	(OMAGIC)

/* An OMF-specific function to write the object file. */
#define	write_object_file	write_coff_file

/* Add these definitions to have a consistent convention for all the
   types used in COFF format. */
#define AOUTHDR			struct aouthdr
#define AOUTHDRSZ		sizeof(AOUTHDR)

/* SYMBOL TABLE */

/* Symbol table entry data type */

typedef struct {
	SYMENT ost_entry;       /* Basic symbol */
	AUXENT ost_auxent[OBJ_COFF_MAX_AUXENTRIES];      /* Auxiliary entry. */
	unsigned int ost_flags; /* obj_coff internal use only flags */
} obj_symbol_type;

/* If compiler generate leading underscores, remove them. */

#ifndef STRIP_UNDERSCORE
#define STRIP_UNDERSCORE 0
#endif /* STRIP_UNDERSCORE */
#define DO_NOT_STRIP	0
#define DO_STRIP	1

/* Symbol table macros and constants */

/* Possible and useful section number in symbol table 
 * The values of TEXT, DATA and BSS may not be portable.
 */

#define C_TEXT_SECTION		((short)1)
#define C_DATA_SECTION		((short)2)
#define C_BSS_SECTION		((short)3)
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
#define S_IS_EXTERNAL(s)        (S_GET_SEGTYPE(s) == SEG_UNKNOWN || \
				 S_GET_STORAGE_CLASS(s) == C_EXT)

/* True if symbol has been defined, ie :
  section > 0 (DATA, TEXT or BSS)
  section == N_ABS  sac 
  section == 0 and value > 0 (external bss symbol) */
#define S_IS_DEFINED(s)	(S_GET_SEGMENT(s) >= FIRST_PROGRAM_SEGMENT || \
			 S_GET_SEGTYPE(s) == SEG_ABSOLUTE || \
			 (S_GET_SEGTYPE(s) == SEG_UNKNOWN && \
			  (s)->sy_symbol.ost_entry.n_value > 0))

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
#define S_IS_EXTERN(s)		 (S_GET_SEGTYPE(s) == SEG_UNKNOWN && (s)->sy_symbol.ost_entry.n_value == 0)

/* True if a symbol can be multiply defined  */
#define S_IS_COMMON(s)		 (S_GET_SEGTYPE(s) == SEG_UNKNOWN && (s)->sy_symbol.ost_entry.n_value != 0)

/* True if a symbol name is in the string table, i.e. its length is > 8. */
#define S_IS_STRING(s)		(strlen(S_GET_NAME(s)) > 8 ? 1 : 0)

/* True if a symbol represents a function name */
#define S_IS_FUNCTION(s)	(ISFCN((s)->sy_symbol.ost_entry.n_type))

/* True if a symbol represents a section name */
#define S_IS_SEGNAME(s)		((s)->sy_symbol.ost_entry.n_flags & F_SECT_SYM)

/* Accessors */
/* The name of the symbol */
#define S_GET_NAME(s)		((char*)(s)->sy_symbol.ost_entry.n_offset)
/* The pointer to the string table */
#define S_GET_OFFSET(s)         ((s)->sy_symbol.ost_entry.n_offset)
/* The zeroes if symbol name is longer than 8 chars */
#define S_GET_ZEROES(s)		((s)->sy_symbol.ost_entry.n_zeroes)
/* The value of the symbol */
#define S_GET_VALUE(s)		((s)->sy_symbol.ost_entry.n_value)
/* The symbol's flags */
#define S_GET_FLAGS(s)		((s)->sy_symbol.ost_entry.n_flags)
/* The index of the segment in the segs array */
#define S_GET_SEGMENT(s)        ((s)->sy_symbol.ost_entry.n_scnum)
/* The type, or logical contents, of the segment */
#define S_GET_SEGTYPE(s)        (segs[(s)->sy_symbol.ost_entry.n_scnum].seg_type)
/* The data type */
#define S_GET_DATA_TYPE(s)	((s)->sy_symbol.ost_entry.n_type)
/* The storage class */
#define S_GET_STORAGE_CLASS(s)	((s)->sy_symbol.ost_entry.n_sclass)
/* The number of auxiliary entries */
#define S_GET_NUMBER_AUXILIARY(s)	((s)->sy_symbol.ost_entry.n_numaux)

/* Modifiers */
/* Set the name of the symbol */
#define S_SET_NAME(s,v)		((s)->sy_symbol.ost_entry.n_offset = (unsigned long)(v))
/* Set the offset of the symbol */
#define S_SET_OFFSET(s,v)	((s)->sy_symbol.ost_entry.n_offset = (v))
/* The zeroes if symbol name is longer than 8 chars */
#define S_SET_ZEROES(s,v)		((s)->sy_symbol.ost_entry.n_zeroes = (v))
/* Set the value of the symbol */
#define S_SET_VALUE(s,v)	((s)->sy_symbol.ost_entry.n_value = (v))
/* Set the flags */
#define S_SET_FLAGS(s,v)	((s)->sy_symbol.ost_entry.n_flags = (v))
/* The index of the segment in the segs array */
#define S_SET_SEGMENT(s,v)      ((s)->sy_symbol.ost_entry.n_scnum = (v))
/* The data type */
#define S_SET_DATA_TYPE(s,v)	((s)->sy_symbol.ost_entry.n_type = (v))
/* The storage class */
#define S_SET_STORAGE_CLASS(s,v)	((s)->sy_symbol.ost_entry.n_sclass = (v))
/* The number of auxiliary entries */
#define S_SET_NUMBER_AUXILIARY(s,v)	((s)->sy_symbol.ost_entry.n_numaux = (v))
/* The symbol represents a section name */
#define S_SET_SEGNAME(s)	((s)->sy_symbol.ost_entry.n_flags |= F_SECT_SYM)

/* Additional modifiers */
/* The symbol is external (does not mean undefined) */
#define S_SET_EXTERNAL(s)       { S_SET_STORAGE_CLASS(s, C_EXT) ; SF_CLEAR_LOCAL(s); }

/* Auxiliary entry macros. SA_ stands for symbol auxiliary */
/* Omit the tv related fields */
/* Accessors */
#define SA_GET_SYM_TAGNDX(s)	((s)->sy_symbol.ost_auxent[0].x_sym.x_tagndx)
#define SA_GET_SYM_LNNO(s)	((s)->sy_symbol.ost_auxent[0].x_sym.x_misc.x_lnsz.x_lnno)
#define SA_GET_SYM_SIZE(s)	((s)->sy_symbol.ost_auxent[0].x_sym.x_misc.x_lnsz.x_size)
#define SA_GET_SYM_FSIZE(s)	((s)->sy_symbol.ost_auxent[0].x_sym.x_misc.x_fsize)
#define SA_GET_SYM_LNNOPTR(s)	((s)->sy_symbol.ost_auxent[0].x_sym.x_fcnary.x_fcn.x_lnnoptr)
#define SA_GET_SYM_ENDNDX(s)	((s)->sy_symbol.ost_auxent[0].x_sym.x_fcnary.x_fcn.x_endndx)
#define SA_GET_SYM_DIMEN(s,i)	((s)->sy_symbol.ost_auxent[0].x_sym.x_fcnary.x_ary.x_dimen[(i)])
#define SA_GET_SCN_SCNLEN(s)	((s)->sy_symbol.ost_auxent[0].x_scn.x_scnlen)
#define SA_GET_SCN_NRELOC(s)	((s)->sy_symbol.ost_auxent[0].x_scn.x_nreloc)
#define SA_GET_SCN_NLINNO(s)	((s)->sy_symbol.ost_auxent[0].x_scn.x_nlinno)
#define SA_GET_IDENT_IDSTRING(s,i)	((s)->sy_symbol.ost_auxent[(i)].x_ident.x_idstring)
#define SA_GET_IDENT_TIMESTAMP(s,i)	((s)->sy_symbol.ost_auxent[(i)].x_ident.x_timestamp)

/* Modifiers */
#define SA_SET_SYM_TAGNDX(s,v)	((s)->sy_symbol.ost_auxent[0].x_sym.x_tagndx=(v))
#define SA_SET_SYM_LNNO(s,v)	((s)->sy_symbol.ost_auxent[0].x_sym.x_misc.x_lnsz.x_lnno=(v))
#define SA_SET_SYM_SIZE(s,v)	((s)->sy_symbol.ost_auxent[0].x_sym.x_misc.x_lnsz.x_size=(v))
#define SA_SET_SYM_FSIZE(s,v)	((s)->sy_symbol.ost_auxent[0].x_sym.x_misc.x_fsize=(v))
#define SA_SET_SYM_LNNOPTR(s,v)	((s)->sy_symbol.ost_auxent[0].x_sym.x_fcnary.x_fcn.x_lnnoptr=(v))
#define SA_SET_SYM_ENDNDX(s,v)	((s)->sy_symbol.ost_auxent[0].x_sym.x_fcnary.x_fcn.x_endndx=(v))
#define SA_SET_SYM_DIMEN(s,i,v)	((s)->sy_symbol.ost_auxent[0].x_sym.x_fcnary.x_ary.x_dimen[(i)]=(v))
#define SA_SET_SCN_SCNLEN(s,v)	((s)->sy_symbol.ost_auxent[0].x_scn.x_scnlen=(v))
#define SA_SET_SCN_NRELOC(s,v)	((s)->sy_symbol.ost_auxent[0].x_scn.x_nreloc=(v))
#define SA_SET_SCN_NLINNO(s,v)	((s)->sy_symbol.ost_auxent[0].x_scn.x_nlinno=(v))
#define SA_SET_IDENT_IDSTRING(s,i,v)	(strncpy((s)->sy_symbol.ost_auxent[(i)].x_ident.x_idstring,(v),19))
#define SA_SET_IDENT_TIMESTAMP(s,i,v)	((s)->sy_symbol.ost_auxent[(i)].x_ident.x_timestamp = (v))


/*
 * Internal use only definitions. SF_ stands for symbol flags.
 *
 * These values can be assigned to sy_symbol.ost_flags field of a symbolS.
 *
 * You'll break i960 if you shift the SYSPROC bits anywhere else.  for
 * more on the balname/callname hack, see tc-i960.h.  b.out is done
 * differently.
 */

#define SF_I960_MASK	(0x000001ff) /* Bits 0-8 are used by the i960 port. */
#define SF_LOMEM	(0x00000002)         /* Low memory (mema-reachable) */
#define SF_BALNAME	(0x00000080)	     /* bit 7 marks BALNAME symbols */
#define SF_CALLNAME	(0x00000100)	     /* bit 8 marks CALLNAME symbols */

#define SF_NORMAL_MASK	(0x0000ffff) /* bits 12-15 are general purpose. */

#define SF_STATICS	(0x00001000)	     /* Mark the .text & all symbols */
#define SF_DEFINED	(0x00002000)	     /* Symbol is defined in this file */
#define SF_STRING	(0x00004000)	     /* Symbol name length > 8 */
#define SF_LOCAL	(0x00008000)	     /* Symbol must not be emitted */

#define SF_DEBUG_MASK	(0xffff0000) /* bits 16-31 are debug info */

#define SF_FUNCTION	(0x00010000)	     /* The symbol is a function */
#define SF_PROCESS	(0x00020000)	     /* Process symbol before write */
#define SF_TAGGED	(0x00040000)	     /* Is associated with a tag */
#define SF_TAG		(0x00080000)	     /* Is a tag */
#define SF_DEBUG	(0x00100000)	     /* Is in debug or abs section */
#define SF_DUPLICATE    (0x00200000)	     /* Debug symbol == real symbol */

/* All other bits are unused. */

/* Accessors */
#define SF_GET(s)		((s)->sy_symbol.ost_flags)
#define SF_GET_NORMAL_FIELD(s)	((s)->sy_symbol.ost_flags & SF_NORMAL_MASK)
#define SF_GET_DEBUG_FIELD(s)	((s)->sy_symbol.ost_flags & SF_DEBUG_MASK)
#define SF_GET_FILE(s)		((s)->sy_symbol.ost_flags & SF_FILE)
#define SF_GET_STATICS(s)	((s)->sy_symbol.ost_flags & SF_STATICS)
#define SF_GET_DEFINED(s)	((s)->sy_symbol.ost_flags & SF_DEFINED)
#define SF_GET_STRING(s)	((s)->sy_symbol.ost_flags & SF_STRING)
#define SF_GET_LOCAL(s)		((s)->sy_symbol.ost_flags & SF_LOCAL)
#define SF_GET_FUNCTION(s)      ((s)->sy_symbol.ost_flags & SF_FUNCTION)
#define SF_GET_PROCESS(s)	((s)->sy_symbol.ost_flags & SF_PROCESS)
#define SF_GET_DEBUG(s)		((s)->sy_symbol.ost_flags & SF_DEBUG)
#define SF_GET_DUPLICATE(s)	((s)->sy_symbol.ost_flags & SF_DUPLICATE)
#define SF_GET_TAGGED(s)	((s)->sy_symbol.ost_flags & SF_TAGGED)
#define SF_GET_TAG(s)		((s)->sy_symbol.ost_flags & SF_TAG)
#define SF_GET_I960(s)		((s)->sy_symbol.ost_flags & SF_I960_MASK) /* used by i960 */
#define SF_GET_LOMEM(s)		((s)->sy_symbol.ost_flags & SF_LOMEM)	/* Low memory (mema-reachable) */

/* Modifiers */
#define SF_SET(s,v)		((s)->sy_symbol.ost_flags = (v))
#define SF_SET_NORMAL_FIELD(s,v)((s)->sy_symbol.ost_flags |= ((v) & SF_NORMAL_MASK))
#define SF_SET_DEBUG_FIELD(s,v)	((s)->sy_symbol.ost_flags |= ((v) & SF_DEBUG_MASK))
#define SF_SET_FILE(s)		((s)->sy_symbol.ost_flags |= SF_FILE)
#define SF_SET_STATICS(s)	((s)->sy_symbol.ost_flags |= SF_STATICS)
#define SF_SET_DEFINED(s)	((s)->sy_symbol.ost_flags |= SF_DEFINED)
#define SF_SET_STRING(s)	((s)->sy_symbol.ost_flags |= SF_STRING)
#define SF_CLEAR_STRING(s)	((s)->sy_symbol.ost_flags &= ~SF_STRING)
#define SF_SET_LOCAL(s)		((s)->sy_symbol.ost_flags |= SF_LOCAL)
#define SF_CLEAR_LOCAL(s)	((s)->sy_symbol.ost_flags &= ~SF_LOCAL)
#define SF_SET_FUNCTION(s)      ((s)->sy_symbol.ost_flags |= SF_FUNCTION)
#define SF_SET_PROCESS(s)	((s)->sy_symbol.ost_flags |= SF_PROCESS)
#define SF_SET_DEBUG(s)		((s)->sy_symbol.ost_flags |= SF_DEBUG)
#define SF_SET_DUPLICATE(s)	((s)->sy_symbol.ost_flags |= SF_DUPLICATE)
#define SF_CLEAR_DUPLICATE(s)	((s)->sy_symbol.ost_flags &= ~SF_DUPLICATE)
#define SF_SET_TAGGED(s)	((s)->sy_symbol.ost_flags |= SF_TAGGED)
#define SF_SET_TAG(s)		((s)->sy_symbol.ost_flags |= SF_TAG)
#define SF_SET_I960(s,v)	((s)->sy_symbol.ost_flags |= ((v) & SF_I960_MASK)) /* used by i960 */
#define SF_CLEAR_CALLNAME(s)	((s)->sy_symbol.ost_flags &= ~SF_CALLNAME)
#define SF_SET_LOMEM(s)		((s)->sy_symbol.ost_flags |= SF_LOMEM)	/* Low memory (mema-reachable) */

/* File header macro and type definition */

#ifdef OBJ_COFF_OMIT_OPTIONAL_HEADER
#define OBJ_COFF_AOUTHDRSZ (0)
#else
#define OBJ_COFF_AOUTHDRSZ (AOUTHDRSZ)
#endif /* OBJ_COFF_OMIT_OPTIONAL_HEADER */

/* Accessors */
/* aouthdr */
#define H_GET_MAGIC_NUMBER(h)           ((h)->aouthdr.magic)
#define H_GET_VERSION_STAMP(h)		((h)->aouthdr.vstamp)
#define H_GET_TEXT_SIZE(h)              ((h)->aouthdr.tsize)
#define H_GET_DATA_SIZE(h)              ((h)->aouthdr.dsize)
#define H_GET_BSS_SIZE(h)               ((h)->aouthdr.bsize)
#define H_GET_ENTRY_POINT(h)            ((h)->aouthdr.entry)
#define H_GET_TEXT_START(h)		((h)->aouthdr.text_start)
#define H_GET_DATA_START(h)		((h)->aouthdr.data_start)
/* filehdr */
#define H_GET_FILE_MAGIC_NUMBER(h)	((h)->filehdr.f_magic)
#define H_GET_NUMBER_OF_SECTIONS(h)	((h)->filehdr.f_nscns)
#define H_GET_TIME_STAMP(h)		((h)->filehdr.f_timdat)
#define H_GET_SYMBOL_TABLE_POINTER(h)	((h)->filehdr.f_symptr)
#define H_GET_SYMBOL_TABLE_SIZE(h)	((h)->filehdr.f_nsyms)
#define H_GET_SIZEOF_OPTIONAL_HEADER(h)	((h)->filehdr.f_opthdr)
#define H_GET_FLAGS(h)			((h)->filehdr.f_flags)
/* Extra fields to achieve bsd a.out compatibility and for convenience */
#define H_GET_RELOCATION_SIZE(h)   	((h)->relocation_size)
#define H_GET_STRING_SIZE(h)            ((h)->string_table_size)
#define H_GET_LINENO_SIZE(h)            ((h)->lineno_size)

/* Modifiers */
/* aouthdr */
#define H_SET_MAGIC_NUMBER(h,v)         ((h)->aouthdr.magic = (v))
#define H_SET_VERSION_STAMP(h,v)	((h)->aouthdr.vstamp = (v))
#define H_SET_TEXT_SIZE(h,v)            ((h)->aouthdr.tsize = (v))
#define H_SET_DATA_SIZE(h,v)            ((h)->aouthdr.dsize = (v))
#define H_SET_BSS_SIZE(h,v)             ((h)->aouthdr.bsize = (v))
#define H_SET_ENTRY_POINT(h,v)          ((h)->aouthdr.entry = (v))
#define H_SET_TEXT_START(h,v)		((h)->aouthdr.text_start = (v))
#define H_SET_DATA_START(h,v)		((h)->aouthdr.data_start = (v))
/* filehdr */
#define H_SET_FILE_MAGIC_NUMBER(h,v)	((h)->filehdr.f_magic = (v))
#define H_SET_NUMBER_OF_SECTIONS(h,v)	((h)->filehdr.f_nscns = (v))
#define H_SET_TIME_STAMP(h,v)		((h)->filehdr.f_timdat = (v))
#define H_SET_SYMBOL_TABLE_POINTER(h,v)	((h)->filehdr.f_symptr = (v))
#define H_SET_SYMBOL_TABLE_SIZE(h,v)    ((h)->filehdr.f_nsyms = (v))
#define H_SET_SIZEOF_OPTIONAL_HEADER(h,v) ((h)->filehdr.f_opthdr = (v))
#define H_SET_FLAGS(h,v)		((h)->filehdr.f_flags = (v))
/* Extra fields to achieve bsd a.out compatibility and for convenience */
#define H_SET_RELOCATION_SIZE(h,v) 	((h)->relocation_size = (v))
#define H_SET_STRING_SIZE(h,v)          ((h)->string_table_size = (v))
#define H_SET_LINENO_SIZE(h,v)          ((h)->lineno_size = (v))

typedef struct {
    AOUTHDR	   aouthdr;             /* a.out header */
    FILHDR	   filehdr;		/* File header, not machine dep. */
    long       string_table_size;   /* names + '\0' + sizeof(int) */
    long	   relocation_size;	/* Cumulated size of relocation
					   information for all sections in
					   bytes. */
    long	   lineno_size;		/* Size of the line number information
					   table in bytes */
} object_headers;

/* A struct to line number info. */

typedef struct internal_lineno {
    LINENO line;			/* The lineno structure itself */
    char* frag;				/* Frag to which the line number is related */
    struct internal_lineno* next;	/* Forward chain pointer */
} lineno;


 /* stack stuff */
typedef struct {
    unsigned long chunk_size;
    unsigned long element_size;
    unsigned long size;
    char*	  data;
    unsigned long pointer;
} stack;

#ifdef __STDC__

char *stack_pop(stack *st);
char *stack_push(stack *st, char *element);
char *stack_top(stack *st);
stack *stack_init(unsigned long chunk_size, unsigned long element_size);
void c_dot_file_symbol(char *filename);
void obj_extra_stuff(object_headers *headers);
void output_file_align(void);
void stack_delete(stack *st);
void tc_coff_headers_hook(object_headers *headers);
void tc_coff_symbol_emit_hook(struct symbol *symbolP);

#else /* __STDC__ */

char *stack_pop();
char *stack_push();
char *stack_top();
stack *stack_init();
void c_dot_file_symbol();
void obj_extra_stuff();
void output_file_align();
void stack_delete();
void tc_coff_headers_hook();
void tc_coff_symbol_emit_hook();

#endif /* __STDC__ */
