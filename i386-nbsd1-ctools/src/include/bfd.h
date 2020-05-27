/* bfd.h -- The only header file required by users of the bfd library */

/* Copyright (C) 1990, 1991 Free Software Foundation, Inc.
 *
 * This file is part of BFD, the Binary File Diddler.
 *
 * BFD is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * BFD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with BFD; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __BFD_H_SEEN__
#define __BFD_H_SEEN__

#include "ansidecl.h"

/* Make it easier to declare prototypes (puts conditional here) */
#ifndef PROTO
#	if __STDC__
#		define PROTO(type, name, arglist) type name arglist
#	else
#		define PROTO(type, name, arglist) type name ()
#	endif
#endif


/* forward declaration */
typedef struct _bfd bfd;

typedef int boolean;
#define false	0
#define true	1

/* Try to avoid breaking stuff */
typedef  long int file_ptr;
typedef struct {int a,b;} bfd_64_type;
typedef unsigned long rawdata_offset;
typedef unsigned long bfd_vma;
typedef unsigned long bfd_offset;
typedef unsigned long bfd_word;
typedef unsigned long bfd_size;
typedef unsigned long symvalue;
typedef unsigned long bfd_size_type;
typedef unsigned int flagword;	/* 32 bits of flags */

/* Note: the macro argument 'q' was 'x' - but the rs6000 compiler
 * expanded the 'x' argument in "%08x" - so the names were changed
 * to protect the innocent.
 */
#define printf_vma(q)	 printf(    "%08x", q)
#define fprintf_vma(s,q) fprintf(s, "%08x", q)

/** File formats */

typedef enum bfd_format {
	bfd_unknown = 0,	/* file format is unknown		*/
	bfd_object,		/* linker/assember/compiler output	*/
	bfd_archive,		/* object archive file			*/
	bfd_core,		/* core dump				*/
	bfd_type_end		/* marks the end; don't use it!		*/
} bfd_format;

/* Object file flag values */
#define NO_FLAGS	           00000000
#define HAS_RELOC	           00000001
#define EXEC_P		           00000002
#define HAS_LINENO	           00000004
#define HAS_DEBUG	           00000010
#define HAS_SYMS	           00000020
#define HAS_LOCALS	           00000040
#define DYNAMIC		           00000100
#define WP_TEXT		           00000200
#define D_PAGED		           00000400
#define HAS_CCINFO	           00001000  /* i960 only: 2 pass gcc optimization info */
#define SUPP_W_TIME                00002000  /* COFF files will suppress write time stamp. */
#define SUPP_BAL_OPT	           00004000  /* COFF files will suppress bal optimization from
						OPTCALL / OPTCALLX directives. */
#define SUPP_CALLS_OPT	           00010000  /* COFF files will suppress calls optimization from
						OPTCALL / OPTCALLX directives. */
#define HAS_PID		           00020000  /* COFF file contains PID. */
#define HAS_PIC		           00040000  /* COFF file contains PIC. */
#define CAN_LINKPID                00100000  /* COFF file is suitable for linking w/ PIC or PID. */
#define STRIP_LINES                00200000  /* Strip lines from output bfd (for -x option in linker.) */
#define STRIP_DUP_TAGS             00400000  /* Remove duplicated tags (for coff only.) */
#define DO_NOT_STRIP_ORPHANED_TAGS 01000000  /* DO NOT strip orphaned tags
						default is to strip orphaned tags.
						Asserting this flag also has the effect of not
						compressing string tables in coff and elf. */
#define SYMTAB_COMPRESSED          02000000  /* Input file contains a compressed symbol table. */
#define ADDED_FILE_SYMBOL          04000000  /* Added a 'file symbol' when writing locals from one OMF to
						another's symbol table. */
#define DO_NOT_ALTER_RELOCS       010000000  /* Leave my bloomin' relocations alone!.  Do not munge into */
					     /* bfd_perform_relocation() forms.  */
#define STRIP_DEBUG               020000000  /* Strips debug from elf/dwarf files. */
#define WRITE_CONTENTS            040000000  /* Causes BFD to write the contents of the file. */

/* This enum gives the object file's CPU architecture, in a global sense.
 * E.g. what processor family does it belong to?  There is another field,
 * which indicates what processor within the family is in use.
 */
enum bfd_architecture {
	bfd_arch_unknown,	/* File arch not known */
	bfd_arch_obscure,	/* File arch known, not one of these */
	bfd_arch_i960,		/* Intel 960 */
				/* The order of the following is important.
				 * A lower number indicates a machine type
				 * that only accepts a subset of the
				 * instructions available to machines with
				 * higher numbers.
				 *
				 * The exception is the "ca", which is
				 * incompatible with all other machines except
				 * "core".
				 */
#define	bfd_mach_i960_core	1
#define	bfd_mach_i960_ka_sa	2
#define	bfd_mach_i960_kb_sb	3
#define	bfd_mach_i960_ca	4
#define	bfd_mach_i960_core2	5
#define	bfd_mach_i960_jx	6
#define	bfd_mach_i960_hx	7
	bfd_arch_last
};

#define BFD_960_MACH_CORE_1  0x00000100
#define BFD_960_MACH_CORE_2  0x00000200
#define BFD_960_MACH_KX      0x00000400
#define BFD_960_MACH_CX      0x00000800
#define BFD_960_MACH_JX      0x00001000
#define BFD_960_MACH_HX      0x00002000
#define BFD_960_MACH_FP1     0x00004000
#define BFD_960_SA           0x00000001
#define BFD_960_SB           0x00000002
#define BFD_960_KA           0x00000003
#define BFD_960_KB           0x00000004
#define BFD_960_MC           0x00000005
#define BFD_960_CA           0x00000006
#define BFD_960_CF           0x00000007
#define BFD_960_JA           0x00000008
#define BFD_960_JD           0x00000009
#define BFD_960_JF           0x0000000a
#define BFD_960_HA           0x0000000b
#define BFD_960_HD           0x0000000c
#define BFD_960_HT           0x0000000d
#define BFD_960_RP           0x0000000e
#define BFD_960_JL           0x0000000f
#define BFD_960_GENERIC      0x000000ff
#define BFD_960_ARCH_MASK    0x000000ff

#define bfd_get_target_attributes(BFD)     ((BFD)->obj_machine_2 & (~BFD_960_ARCH_MASK))
#define bfd_get_target_arch(BFD)           ((BFD)->obj_machine_2 & BFD_960_ARCH_MASK)

#define bfd_set_target_attributes(BFD,ATTR)((BFD)->obj_machine_2 = bfd_get_target_attributes(BFD) | (ATTR) | \
					                              bfd_get_target_arch(BFD))
#define bfd_set_target_arch(BFD,TA)        ((BFD)->obj_machine_2 = bfd_get_target_attributes(BFD) | \
					                              (BFD_960_ARCH_MASK & (TA)))

/* symbols and relocation */

typedef unsigned long symindex;

#define BFD_NO_MORE_SYMBOLS ((symindex) ~0)

typedef enum {
	bfd_symclass_unknown = 0,
	bfd_symclass_fcommon, /* fortran common symbols */
	bfd_symclass_global, /* global symbol, what a surprise */
	bfd_symclass_debugger, /* some debugger symbol */
	bfd_symclass_undefined /* none known */
} symclass;

typedef int symtype;		/* Who knows, yet? */

/* Symbol cache classifications: (Bfd-Symbol-Flag_FOOBAR) */
#define BSF_NO_FLAGS     0x00000000
#define BSF_LOCAL	 0x00000001	/* bfd_symclass_unknown */
#define BSF_GLOBAL	 0x00000002	/* bfd_symclass_global */
#define BSF_IMPORT	 0x00000004
#define BSF_EXPORT	 0x00000008
#define BSF_UNDEFINED	 0x00000010	/* bfd_symclass_undefined */
#define BSF_FORT_COMM	 0x00000020	/* bfd_symclass_fcommon */
#define BSF_DEBUGGING	 0x00000040	/* bfd_symclass_debugger */
#define BSF_ABSOLUTE	 0x00000080
/* Pairs of related symbols are lain out as follows:

   BALsym/CALLsym:

   for example:

   .leafproc x,y

   x:
   lda 0,g0
   y:
   ret

          Name:  Flags:         Value:    related_symbol:
CALLsym:  x      global/local   x's value points at y
                 depends on x
		 BSF_HAS_BAL
BALsym:   y      global/local   y's value points at y
                 depends on
		 BSF_HAS_CALL

SYSPROC/CALLsym:

   for example:

   .sysproc x,123
x:
   lda 0,g0

          Name:  Flags:         Value:     related_symbol:
CALLsym:  x      global/local   x:'s value points at SYSPROCsym
                 depends on x
		 BSF_HAS_SCALL
SYSPROCsym: ''   global/local   123        points at x
                 depends on x
		 BSF_SCALL
*/

#define BSF_SCALL        0x00000100      /* System call (calls value). */
#define BSF_HAS_SCALL    0x00000200      /* related_symbol contains pointer to calls entry point. */
#define BSF_HAS_BAL      0x00000400      /* related_symbol contains pointer to bal entry point. */
#define BSF_HAS_CALL     0x00000800      /* related_symbol contains pointer to call entry point. */
#define BSF_ALREADY_EMT  0x00001000      /* For bfd.  It can mark symbols already written if it wants to. */
#define BSF_ALL_OMF_REP  0x00002000      /* All omfs: bout, coff and elf can represent this symbol. */
#define BSF_TMP_REL_SYM  0x00004000      /* Temporary symbol used only for relocation. */
#define BSF_BAL          0x00008000
#define BSF_KEEP         0x00010000
#define BSF_WARNING      0x00020000

/* in some files the type of a symbol sometimes alters its location
 * in an output file - ie in coff a ISFCN symbol which is also C_EXT
 * symbol appears where it was declared and not at the end of a section.
 * This bit is set by the target bfd part to convey this information.
 */
#define BSF_NOT_AT_END   0x00040000



#define BSF_KEEP_G       0x00080000
#define BSF_WEAK         0x00100000
#define BSF_CTOR         0x00200000	/* Symbol is a con/destructor */
#define BSF_FAKE         0x00400000	/* Symbol doesn't really exist */
#define BSF_OLD_COMMON   0x00800000	/* Symbol was common, now allocated */
#define BSF_XABLE        0x01000000      /* Symbol is 'compressable' see -x option in gld960. */
#define BSF_HAS_DBG_INFO 0x02000000      /* Symbol is a common symbol and has debug info
					   associated with it (COFF only).  For gld960. */
#define BSF_ZERO_SYM     0x04000000      /* An elf symbol table 0 entry. */
#define BSF_SECT_SYM     0x08000000      /* A SECTION symbol. */

#define BSF_SYM_REFD_DEB_SECT 0x10000000      /* This symbol was referenced by a debug section. */
#define BSF_SYM_REFD_OTH_SECT 0x20000000      /* This symbol was referenced by another section. */

/* If symbol is fort_comm, then value is size, and this is the contents */
#define BFD_FORT_COMM_DEFAULT_VALUE 0

/* general purpose part of a symbol;
 * target specific parts will be found in libcoff.h, liba.out h, etc.
 */
typedef struct symbol_cache_entry {
	struct _bfd *the_bfd;	/* Just a way to find out host type */
	CONST char *name;
	symvalue value,	/* Primary value of symbol. */
	value_2;        /* Secondary value of symbol.  Currently only used
			 for alignment of common symbols, but can be used
			 for other things. */
	flagword flags;
	struct sec *section;
	PTR udata;			/* Target-specific stuff */
	struct symbol_cache_entry *related_symbol;
	unsigned long sym_tab_idx;
	char *native_info;
} asymbol;

/* This structure is filled in by bfd_more_symbol_info().  It currently
   only support the ability to easily fetch the value of a sysproc symbol,
   storage class, or type  of a symbol.   But this interface can easily 
   grow because it returns this union. 
*/

typedef enum { 
    bfd_unknown_info = 0, 
    bfd_sysproc_val,        /* return sysproc symbol value  */
    bfd_function_size,	    /* return (coff) function size */
    bfd_storage_class,      /* return symbol storage class  */
    bfd_symbol_type,        /* return symbol type           */
    bfd_set_sysproc_val     /* Write sysproc symbol value into symbol. */
} bfd_sym_more_info_type;

typedef union more_symbol_info {
    long function_size;     /* return (coff) function size here     */
    unsigned long sym_type; /* return symbol type here             */
    int sysproc_value;      /* return sysproc_value  here          */
    char sym_class;         /* return storage class of symbol here */  
} more_symbol_info;

#define bfd_get_section(x) ((x)->section)
#define bfd_get_output_section(x) ((x)->section->output_section)
#define bfd_set_section(x,y) ((x)->section) = (y)
#define bfd_asymbol_base(x) ((x)->section?((x)->section->vma):0)
#define bfd_asymbol_value(x) (bfd_asymbol_base(x) + x->value)
#define bfd_asymbol_name(x) ((x)->name)

/* This is a type pun with struct ranlib on purpose! */
typedef struct carsym {
	char *name;
	file_ptr file_offset;		/* look here to find the file */
} carsym;			/* to make these you call a carsymogen */

/* Relocation stuff */

/* Either: sym will point to a symbol and isextern will be 0, *OR*
 * sym will be NULL and isextern will be a symbol type (eg N_TEXT)
 * which means the location should be relocated relative to the
 * segment origin.  This is because we won't necessarily have a symbol
 * which is guaranteed to point to the segment origin.
 */
typedef enum bfd_reloc_status {
	bfd_reloc_ok,
	bfd_reloc_overflow,
	bfd_reloc_outofrange,
	bfd_reloc_continue,
	bfd_reloc_notsupported,
	bfd_reloc_other,
	bfd_reloc_undefined,
	bfd_reloc_dangerous,
	bfd_reloc_no_code_for_syscall
} bfd_reloc_status_enum_type;

typedef enum reloc_type {
    bfd_reloc_type_unknown,
    bfd_reloc_type_none,      /* Is a no-op for relocation. */
    bfd_reloc_type_opt_call,  /* Both of these are complex relocations.  Do both the */
    bfd_reloc_type_opt_callx, /* call -> bal opcode change and the displacement adjustment. */
    bfd_reloc_type_32bit_abs,
    bfd_reloc_type_12bit_abs,
    bfd_reloc_type_24bit_pcrel,
    bfd_reloc_type_32bit_abs_sub,
	} bfd_reloc_type;

typedef CONST struct rint {
	char *name;		/* Useful when debugging */
	unsigned int type;
	bfd_reloc_type reloc_type;
} reloc_howto_type;

typedef unsigned char bfd_byte;

typedef struct reloc_cache_entry {
    asymbol **sym_ptr_ptr;	/* pointer into the canonicalized symbol table*/
    rawdata_offset address;	/* offset in section */
    reloc_howto_type *howto;
} arelent;

typedef struct relent_chain {
	arelent relent;
	struct   relent_chain *next;
} arelent_chain;

/* Used in generating armaps.  Perhaps just a forward definition would do? */
struct orl {			/* output ranlib */
	char **name;		/* symbol name */
	file_ptr pos;		/* bfd* or file position */
	int namidx;		/* index into string table */
};

/* Linenumber stuff */
typedef struct lineno_cache_entry {
	unsigned int line_number;	/* Linenumber from start of function*/
	union {
		asymbol *sym;		/* Function name */
		unsigned long offset;	/* Offset into section */
	} u;
} alent;

/* object and core file sections */

/* Section flag definitions */

#define SEC_ALLOC	        0x000001   /* Linker will allocate memory for this section. */
#define SEC_LOAD	        0x000002   /* The loader will write this section into the target's memory. */
#define SEC_RELOC	        0x000004   /* The section has relocations. */
#define SEC_CODE	        0x000008   /* The section contains executable code. */
#define SEC_DATA	        0x000010   /* The section contains initialized data. */
#define SEC_HAS_CONTENTS        0x000020   /* If the section actually represents stuff stored in the file. */
#define SEC_IS_DSECT            0x000040   /* A dummy section (from linker).  */
#define SEC_IS_COPY             0x000080   /* A COPY section (from linker).  */
#define SEC_IS_NOLOAD           0x000100   /* A NOLOAD section (from linker).  */
#define SEC_IS_BSS              0x000200   /* A bss section. */
#define SEC_IS_INFO             0x000400   /* This is set if the STYP_INFO is set in coff. */
#define SEC_IS_BIG_ENDIAN       0x000800   /* The section contains big endian byte order information. */
#define SEC_IS_READABLE         0x001000   /* In elf this is SHF_960_READ. */
#define SEC_IS_WRITEABLE        0x002000   /* In elf this is SHF_WRITE. */
#define SEC_IS_EXECUTABLE       0x004000   /* In elf this is SHF_EXECINSTR. */
#define SEC_IS_SUPER_READABLE   0x008000   /* In elf this is SHF_960_READ. */
#define SEC_IS_SUPER_WRITEABLE  0x010000   /* In elf this is SHF_WRITE. */
#define SEC_IS_SUPER_EXECUTABLE 0x020000   /* In elf this is SHF_EXECINSTR. */
#define SEC_IS_PI               0x040000   /* In elf this is SHF_960_PI. */
#define SEC_IS_CCINFO           0x080000   /* In elf this corresponds to the .960.intel.ccinfo section. */
#define SEC_IS_DEBUG            0x100000   /* In elf this corresponds to the .debug*           sections. */ 
#define SEC_IS_LINK_PIX         0x200000   /* In elf this corresponds to the .debug*           sections. */ 

#define SEC_ALL_FLAGS           0x3fffff   /* Must correspond to the presence of all flags above. */

typedef struct sec {
	CONST char *name;
	struct sec *next;
	flagword flags;
	flagword insec_flags;	/* if this is an output section, this field
				is where we remember the flags fields of input
				sections */
	bfd_vma vma;
	bfd_vma pma;
	bfd_size_type size;

	/* The output_offset is the indent into the output section of
	 * this section. If this is the first section to go into
	 * an output section, this value will be 0...
	 */
	bfd_vma output_offset;
	struct sec *output_section;
	unsigned int alignment_power; /* eg 4 aligns to 2^4*/

	arelent *relocation;		/* for input files */
	arelent **orelocation;	/* for output files */

	unsigned reloc_count,
	reloc_count_iterator;
	file_ptr filepos;		/* File position of section data */
	file_ptr rel_filepos;		/* File position of relocation info */
	file_ptr line_filepos;
	PTR *userdata;
	struct lang_output_section *otheruserdata;
	int secnum;			/* Which section is it 0..nth */
	alent *lineno;
	unsigned int lineno_count;

	/* When a section is being output, this value changes as more
	 * linenumbers are written out
	 */
	file_ptr moving_line_filepos;

	/* what the section number is in the target world */
	unsigned int target_index;

	PTR used_by_bfd;

	/* If this is a constructor section then here is a list of relents */
	arelent_chain *constructor_chain;

#ifdef PROCEDURE_PLACEMENT
	/* Procedure placement needs a ptr back to the parent file. */
	void	*file;
#endif
	asymbol *sect_sym;
	unsigned long paddr_file_offset;
} asection;

#define BAD_PADDR_FILE_OFFSET (-1)

#define	align_power(addr, align) (((addr) + ((1<<(align))-1)) & (-1 << (align)))

typedef struct sec *sec_ptr;

#define bfd_section_name(bfd, ptr) ((ptr)->name)
#define bfd_section_size(bfd, ptr) ((ptr)->size)
#define bfd_section_vma(bfd, ptr) ((ptr)->vma)
#define bfd_section_pma(bfd, ptr) ((ptr)->pma)
#define bfd_section_alignment(bfd, ptr) ((ptr)->alignment_power)
#define bfd_get_section_flags(bfd, ptr) ((ptr)->flags)
#define bfd_get_section_userdata(bfd, ptr) ((ptr)->userdata)

#define bfd_set_section_vma(bfd, ptr, val) (((ptr)->vma = (val)), true)
#define bfd_set_section_pma(bfd, ptr, val) (((ptr)->pma = (val)), true)
#define bfd_set_section_alignment(bfd, ptr, val) (((ptr)->alignment_power = (val)),true)
#define bfd_set_section_userdata(bfd, ptr, val) (((ptr)->userdata = (val)),true)

/** Error handling */

typedef enum {
	no_error = 0,
	system_call_error,
	invalid_target,
	wrong_format,
	invalid_operation,
	no_memory,
	no_symbols,
	no_relocation_info,
	no_more_archived_files,
	malformed_archive,
	symbol_not_found,
	file_not_recognized,
	file_ambiguously_recognized,
	no_contents,
	bfd_error_nonrepresentable_section,
	invalid_error_code
} bfd_ec;

extern bfd_ec bfd_error;

typedef struct bfd_error_vector {
  PROTO(void,(* nonrepresentable_section ),(CONST bfd  *CONST abfd,
					    CONST char *CONST name));
} bfd_error_vector_type;

PROTO (char *, bfd_errmsg, ());
PROTO (void, bfd_perror, (CONST char *message));




typedef enum bfd_print_symbol {
	bfd_print_symbol_name_enum,
	bfd_print_symbol_type_enum,
	bfd_print_symbol_all_enum
} bfd_print_symbol_enum_type;

/* The BFD target structure.
 *
 * This structure is how to find out everything BFD knows about a target.
 * It includes things like its byte order, name, what routines to call
 * to do various operations, etc.
 *
 * Every BFD points to a target structure with its "xvec" member.
 */

/* Shortcut for declaring fields which are prototyped function pointers,
 * while avoiding anguish on compilers that don't support protos.
 */
#define SDEF(ret, name, arglist) PROTO(ret,(*name),arglist)
#if HOST_SYS==HP9000_SYS
#define SDEF_FMT(ret, name, arglist) PROTO(ret,(*name[(int)bfd_type_end]),arglist)
#else
#define SDEF_FMT(ret, name, arglist) PROTO(ret,(*name[bfd_type_end]),arglist)
#endif

/* These macros are used to dispatch to functions through the bfd_target
 * vector.  They are used in a number of macros further down in bfd.h,
 * and are also used when calling various routines by hand inside the
 * bfd implementation.  The "arglist" argument must be parenthesized;
 * it contains all the arguments to the called function.
 */
#define BFD_SEND(bfd, message, arglist) ((*((bfd)->xvec->message)) arglist)
/* For operations which index on the bfd format */
#define BFD_SEND_FMT(bfd, message, arglist) \
                 (((bfd)->xvec->message[(int)((bfd)->format)]) arglist)

typedef struct {
    bfd_vma  address;
    CONST char     *func_name,*file_name;
    unsigned int line_number,num_hits;
} bfd_ghist_info;

/*  This is the struct which defines the type of BFD this is.  The
 *  "xvec" member of the struct bfd itself points here.  Each module
 *  that implements access to a different target under BFD, defines
 *  one of these.
 */
typedef struct bfd_target {
  /* identifies the kind of target, eg SunOS4, Ultrix, etc */
  char *name;

  enum target_flavour_enum {
    bfd_target_aout_flavour_enum,
    bfd_target_coff_flavour_enum,
    bfd_target_elf_flavour_enum	} flavour;

  boolean byteorder_big_p;	/* Order of bytes in data sections */
  boolean header_byteorder_big_p; /* Order of bytes in header */

  flagword object_flags;	/* these are the ones that may be set */
  flagword section_flags;	/* ditto */

  char ar_pad_char;		/* filenames in archives padded w/this char */
  unsigned short ar_max_namelen; /* this could be a char too! */

  /* Byte swapping for data */
  /* Note that these don't take bfd as first arg.  Certain other handlers
     could do the same. */
  SDEF (int,		bfd_getx64, (bfd_byte *,bfd_64_type *));
  SDEF (void,		bfd_putx64, (bfd_64_type, bfd_byte *));
  SDEF (unsigned int,	bfd_getx32, (bfd_byte *));
  SDEF (void,		bfd_putx32, (unsigned long, bfd_byte *));
  SDEF (unsigned int,	bfd_getx16, (bfd_byte *));
  SDEF (void,		bfd_putx16, (int, bfd_byte *));

  /* Byte swapping for headers */
  SDEF (int,		bfd_h_getx64, (bfd_byte *,bfd_64_type *));
  SDEF (void,		bfd_h_putx64, (bfd_64_type, bfd_byte *));
  SDEF (unsigned int,	bfd_h_getx32, (bfd_byte *));
  SDEF (void,		bfd_h_putx32, (unsigned long, bfd_byte *));
  SDEF (unsigned int,	bfd_h_getx16, (bfd_byte *));
  SDEF (void,		bfd_h_putx16, (int, bfd_byte *));

  /* Format-dependent */
  /* Check the format of a file being read.  Return bfd_target * or zero. */
  SDEF_FMT (struct bfd_target *, _bfd_check_format, (bfd *));
  /* Set the format of a file being written.  */
  SDEF_FMT (boolean,		 _bfd_set_format, (bfd *));
  /* Write cached information into a file being written, at bfd_close.  */
  SDEF_FMT (boolean,		 _bfd_write_contents, (bfd *));

  /* All these are defined in JUMP_TABLE */

  /* Archives */
  SDEF (boolean, _bfd_slurp_armap, (bfd *));
  SDEF (boolean, _bfd_slurp_extended_name_table, (bfd *));
  SDEF (void,	 _bfd_truncate_arname, (bfd *, CONST char *, char *));
  SDEF (boolean, write_armap, (bfd *arch, unsigned int elength,
			       struct orl *map, int orl_count, int
			       stridx));

  /* All the standard stuff */
  SDEF (boolean, _close_and_cleanup, (bfd *)); /* free any allocated data */
  SDEF (boolean, _bfd_set_section_contents, (bfd *, sec_ptr, PTR,
					     file_ptr, bfd_size_type));
  SDEF (boolean, _bfd_get_section_contents, (bfd *, sec_ptr, PTR,
					     file_ptr, bfd_size_type));
  SDEF (boolean, _new_section_hook, (bfd *, sec_ptr));

  /* Symbols and relocation */
  SDEF (unsigned int, _get_symtab_upper_bound, (bfd *));
  SDEF (unsigned int, _bfd_canonicalize_symtab, (bfd *, asymbol **));
  SDEF (unsigned int, _get_reloc_upper_bound, (bfd *, sec_ptr));
  SDEF (unsigned int, _bfd_canonicalize_reloc,
				(bfd *, sec_ptr, arelent **, asymbol**));
  SDEF (asymbol *, _bfd_make_empty_symbol, (bfd *));
  SDEF (void,	   _bfd_print_symbol,
			(bfd *, PTR, asymbol *, bfd_print_symbol_enum_type));
  SDEF (alent *,   _get_lineno, (bfd *, asymbol *));
  SDEF (boolean,   _bfd_set_arch_mach,
			(bfd *, enum bfd_architecture, unsigned long, unsigned long));
  SDEF (bfd *,	 openr_next_archived_file, (bfd *arch, bfd *prev));
  SDEF (boolean, _bfd_find_nearest_line,
        (bfd *abfd, asection *section, asymbol **symbols,bfd_vma offset,
	 CONST char **file, CONST char **func, unsigned int *line));
  SDEF (int,	 _bfd_stat_arch_elt, (bfd *, PTR));

  SDEF (int,	 _bfd_sizeof_headers, (bfd *, boolean));

  /* Dump info to stdout */
  SDEF (int,	 _bfd_dmp_symtab, (bfd *, int, int, int));
  SDEF (int,	 _bfd_dmp_linenos, (bfd *, int));
  SDEF (int,     _bfd_dmp_full_fmt, (bfd *, asymbol **, unsigned long, char));

  /* More symbol information. */
  SDEF (int,	 _bfd_more_symbol_info, (bfd *, asymbol *, more_symbol_info *, bfd_sym_more_info_type));

  /* Zero line number info */
  SDEF (int,     _bfd_zero_line_info, (bfd *));

  /* Miscellaneous. */
  SDEF(bfd_ghist_info *, _bfd_fetch_ghist_info,  (bfd *abfd,unsigned int *nelements,
						  unsigned int *nlinenumbers));

  SDEF(asymbol **, _bfd_set_sym_tab, (bfd *abfd, asymbol **location,
				 unsigned int *symcount));
} bfd_target;

/* The code that implements targets can initialize a jump table with this
 * macro.  It must name all its routines the same way (a prefix plus
 * the standard routine suffix), or it must #define the routines that
 * are not so named, before calling JUMP_TABLE in the initializer.
 */

/* Semi-portable string concatenation in cpp */
#ifndef CAT
#if defined(__STDC__) || defined(WIN95)
#define CAT(a,b) a##b
#else
#define CAT(a,b) a/**/b
#endif
#endif

#define JUMP_TABLE(NAME)\
CAT(NAME,_slurp_armap),\
CAT(NAME,_slurp_extended_name_table),\
CAT(NAME,_truncate_arname),\
CAT(NAME,_write_armap),\
CAT(NAME,_close_and_cleanup),	\
CAT(NAME,_set_section_contents),\
CAT(NAME,_get_section_contents),\
CAT(NAME,_new_section_hook),\
CAT(NAME,_get_symtab_upper_bound),\
CAT(NAME,_get_symtab),\
CAT(NAME,_get_reloc_upper_bound),\
CAT(NAME,_canonicalize_reloc),\
CAT(NAME,_make_empty_symbol),\
CAT(NAME,_print_symbol),\
CAT(NAME,_get_lineno),\
CAT(NAME,_set_arch_mach),\
CAT(NAME,_openr_next_archived_file),\
CAT(NAME,_find_nearest_line),\
CAT(NAME,_generic_stat_arch_elt),\
CAT(NAME,_sizeof_headers), \
CAT(NAME,_dmp_symtab), \
CAT(NAME,_dmp_linenos), \
CAT(NAME,_dmp_full_fmt), \
CAT(NAME,_more_symbol_info), \
CAT(NAME,_zero_line_info), \
CAT(NAME,_fetch_ghist_info), \
CAT(NAME,_set_sym_tab)


/* Finally!  The BFD struct itself.  This contains the major data about
 * the file, and contains pointers to the rest of the data.
 *
 * To avoid dragging too many header files into every file that
 * includes bfd.h, IOSTREAM has been declared as a "char *", and MTIME
 * as a "long".  Their correct types, to which they are cast when used,
 * are "FILE *" and "time_t".
 */

struct _bfd {
  CONST char *filename;		/* could be null; filename user opened with */
  bfd_target *xvec;		/* operation jump table */
  char *iostream;		/* stdio FILE *, unless an archive element */

  boolean cacheable;		/* iostream can be closed if desired */
  struct _bfd *lru_prev;	/* Used for file caching */
  struct _bfd *lru_next;	/* Used for file caching */
  file_ptr where;		/* Where the file was when closed */
  boolean opened_once;
  boolean mtime_set;		/* Flag indicating mtime is available */
  long mtime;			/* File modified time */
  int ifd;			/* for output files, channel we locked. */
  bfd_format format;
  enum bfd_direction {no_direction = 0,
			read_direction = 1,
			write_direction = 2,
			both_direction = 3} direction;

  flagword flags;		/* format_specific */

  /* Currently my_archive is tested before adding origin to anything. I
     believe that this can become always an add of origin, with origin set
     to 0 for non archive files.  FIXME-soon. */
  file_ptr origin;		/* for archive contents */
  boolean output_has_begun;	/* cf bfd_set_section_size */
  asection *sections;		/* Pointer to linked list of sections */
  unsigned int section_count;	/* The number of sections */

  /* Some object file stuff */
  bfd_vma start_address;	/* for object files only, of course */
  unsigned int symcount;	/* used for input and output */
  asymbol **outsymbols;		/* symtab for output bfd */
  enum bfd_architecture obj_arch; /* Architecture of object machine, eg m68k */
  unsigned long obj_machine;	/* Particular machine within arch, e.g. 68010 */
  unsigned long obj_machine_2;  /* Or-in obj_machine bit flags. */

  /* Archive stuff.  strictly speaking we don't need all three bfd* vars,
     but doing so would allow recursive archives! */
  PTR arelt_data;		/* needed if this came from an archive */
  struct _bfd *my_archive;	/* if this is an archive element */
  struct _bfd *next;		/* output chain pointer */
  struct _bfd *archive_head;	/* for output archive */
  boolean has_armap;		/* if an arch; has it an armap? */

  PTR tdata;			/* target-specific storage */
  PTR usrdata;			/* application-specific storage */

  /* Should probably be enabled here always, so that library may be changed
     to switch this on and off, while user code may remain unchanged */
#ifdef BFD_LOCKS
  struct flock *lock;
  char *actual_name;		/* for output files, name given to open()  */
#endif

};

#define bfd_get_filename(abfd) ((char *) (abfd)->filename)
#define bfd_get_format(abfd) ((abfd)->format)
#define bfd_get_target(abfd) ((abfd)->xvec->name)
#define bfd_get_file_flags(abfd) ((abfd)->flags)
#define bfd_applicable_file_flags(abfd) ((abfd)->xvec->object_flags)
#define bfd_applicable_section_flags(abfd) ((abfd)->xvec->section_flags)
#define bfd_my_archive(abfd) ((abfd)->my_archive);
#define bfd_has_map(abfd) ((abfd)->has_armap)
#define bfd_valid_reloc_types(abfd) ((abfd)->xvec->valid_reloc_types)
#define bfd_usrdata(abfd) ((abfd)->usrdata)

#define bfd_get_start_address(abfd) ((abfd)->start_address)
#define bfd_get_symcount(abfd) ((abfd)->symcount)
#define bfd_get_outsymbols(abfd) ((abfd)->outsymbols)
#define bfd_count_sections(abfd) ((abfd)->section_count)
#define bfd_get_architecture(abfd) ((abfd)->obj_arch)
#define bfd_get_machine(abfd) ((abfd)->obj_machine)

/* The various callable routines */
PROTO (bfd_size_type, bfd_alloc_size,(bfd *abfd));
PROTO (char *,	bfd_printable_arch_mach,(enum bfd_architecture, unsigned long));
PROTO (char *,	bfd_format_string, (bfd_format format));
PROTO (void,	bfd_center_header, (CONST char *hdr));
PROTO (char**,	bfd_target_list, ());
PROTO (bfd *,	bfd_openr, (CONST char *filename, CONST char *target));
PROTO (bfd *,	bfd_openrw, (CONST char *filename, CONST char *target));
PROTO (bfd *,	bfd_fdopenr,(CONST char *filename, CONST char *target, int fd));
PROTO (bfd *,	bfd_openw, (CONST char *filename, CONST char *target));
PROTO (bfd *,	bfd_create, (CONST char *filename, CONST bfd *abfd));
PROTO (boolean,	bfd_close, (bfd *abfd));
PROTO (long,	bfd_get_mtime, (bfd *abfd));
PROTO (bfd *,	bfd_openr_next_archived_file, (bfd *obfd, bfd *last_file));
PROTO (boolean,	bfd_set_archive_head, (bfd *output_archive, bfd *new_head));
PROTO (boolean,	bfd_check_format, (bfd *abfd, bfd_format format));
PROTO (boolean,	bfd_set_format, (bfd *abfd, bfd_format format));
PROTO (sec_ptr,	bfd_get_section_by_name, (bfd *abfd, CONST char *name));
PROTO (void,	bfd_map_over_sections, (bfd *abfd, void (*operation)(),
				     PTR user_storage));
PROTO (sec_ptr,	bfd_make_section, (bfd *abfd, CONST char *CONST name));
PROTO (boolean,	bfd_set_section_flags, (bfd *abfd, sec_ptr section,
					flagword flags));
PROTO (boolean,	bfd_set_file_flags, (bfd *abfd, flagword flags));
PROTO (boolean,	bfd_arch_compatible,  (bfd *abfd, bfd *bbfd,
	    enum bfd_architecture *res_arch, unsigned long *res_machine));

PROTO (boolean,	bfd_set_section_size, (bfd *abfd, sec_ptr ptr,
				       unsigned long val));
PROTO (boolean,	bfd_get_section_contents, (bfd *abfd, sec_ptr section,
					   PTR location, file_ptr offset,
					   bfd_size_type count));
PROTO (boolean,	bfd_set_section_contents, (bfd *abfd, sec_ptr section,
					   PTR location, file_ptr offset,
					   bfd_size_type count));

PROTO (unsigned long, bfd_get_next_mapent, (bfd *abfd, symindex prev,
					    carsym **entry));
PROTO (bfd *,	bfd_get_elt_at_index, (bfd *abfd, int index));
PROTO (asymbol **,	bfd_set_symtab, (bfd *abfd, asymbol **location,
				 unsigned int *symcount));
PROTO (unsigned int, get_reloc_upper_bound, (bfd *abfd, sec_ptr asect));
PROTO (unsigned int, bfd_canonicalize_reloc, (bfd *abfd, sec_ptr asect,
					      arelent **location,
					      asymbol **canon));
PROTO (void,	bfd_set_reloc, (bfd *abfd, sec_ptr asect, arelent **location,
			     unsigned int count));
PROTO (boolean,	bfd_set_start_address, (bfd *,bfd_vma));
PROTO (void,	bfd_print_symbol_vandf, (PTR, asymbol *));
PROTO (bfd_vma,	bfd_log2, (bfd_vma));
PROTO (boolean,	bfd_scan_arch_mach,(CONST char *, enum bfd_architecture *,
							   unsigned long *));

/* For speed we turn calls to these interface routines
 * directly into jumps through the transfer vector.
 */

#define bfd_dmp_symtab(abfd, no_hdrs, no_trans, scn_num) \
     BFD_SEND(abfd, _bfd_dmp_symtab, (abfd, no_hdrs, no_trans, scn_num))

#define bfd_dmp_linenos(abfd, no_hdrs) \
     BFD_SEND(abfd, _bfd_dmp_linenos, (abfd, no_hdrs))

#define bfd_more_symbol_info(abfd, asym, m, info) \
     BFD_SEND(abfd, _bfd_more_symbol_info, (abfd, asym, m, info))

#define bfd_zero_line_info(abfd) \
     BFD_SEND(abfd, _bfd_zero_line_info, (abfd))

#define bfd_fetch_ghist_info(abfd,nelements,nlinenumbers) \
     BFD_SEND(abfd, _bfd_fetch_ghist_info, (abfd,nelements,nlinenumbers))

PROTO(bfd_reloc_status_enum_type, bfd_perform_relocation,
      (bfd *, arelent *, PTR, asection *, bfd *));

#define bfd_set_arch_mach(abfd, arch, mach, realmach) \
     BFD_SEND (abfd, _bfd_set_arch_mach, (abfd, arch, mach, realmach))

#define bfd_sizeof_headers(abfd, reloc) \
     BFD_SEND (abfd, _bfd_sizeof_headers, (abfd, reloc))

#define get_symtab_upper_bound(abfd) \
     BFD_SEND (abfd, _get_symtab_upper_bound, (abfd))

#define bfd_canonicalize_symtab(abfd, location) \
     BFD_SEND (abfd, _bfd_canonicalize_symtab, (abfd, location))

#define bfd_make_empty_symbol(abfd) \
     BFD_SEND (abfd, _bfd_make_empty_symbol, (abfd))

#define bfd_print_symbol(abfd, file, symbol, how) \
     BFD_SEND (abfd, _bfd_print_symbol, (abfd, file, symbol, how))

#define bfd_get_lineno(abfd, symbol) \
     BFD_SEND (abfd, _get_lineno, (abfd, symbol))

#define bfd_stat_arch_elt(abfd, buf) \
     BFD_SEND (abfd, _bfd_stat_arch_elt, (abfd, buf))

#define bfd_find_nearest_line(abfd,sec,syms,offset,fn,func,line) \
     BFD_SEND(abfd, _bfd_find_nearest_line, (abfd,sec,syms,offset,fn,func,line))

/* Some byte-swapping i/o operations */
#define LONG_SIZE	4
#define SHORT_SIZE	2
#define BYTE_SIZE	1

#define bfd_put_8(abfd, val, ptr)	(*((char *)ptr) = (char)val)
#define bfd_get_8(abfd, ptr)		(*((char *)ptr))

#define bfd_put_32(abfd, val, ptr)	BFD_SEND(abfd, bfd_putx32,   (val,ptr))
#define bfd_get_32(abfd, ptr)		BFD_SEND(abfd, bfd_getx32,   (ptr))

#define bfd_put_64(abfd, val, ptr)	BFD_SEND(abfd, bfd_putx64,   (val,ptr))
#define bfd_get_64(abfd, ptr)		BFD_SEND(abfd, bfd_getx64,   (ptr))

#define bfd_put_16(abfd, val, ptr)	BFD_SEND(abfd, bfd_putx16,  (val,ptr))
#define bfd_get_16(abfd, ptr)		BFD_SEND(abfd, bfd_getx16,  (ptr))

#define	bfd_h_put_8(abfd, val, ptr)	bfd_put_8 (abfd, val, ptr)
#define	bfd_h_get_8(abfd, ptr)		bfd_get_8 (abfd, ptr)

#define bfd_h_put_32(abfd, val, ptr)	BFD_SEND(abfd, bfd_h_putx32, \
							(val, (bfd_byte *) ptr))
#define bfd_h_get_32(abfd, ptr)		BFD_SEND(abfd, bfd_h_getx32, \
							((bfd_byte *) ptr))

#define bfd_h_put_64(abfd, val, ptr)	BFD_SEND(abfd, bfd_h_putx64, \
							(val, (bfd_byte *) ptr))
#define bfd_h_get_64(abfd, ptr)		BFD_SEND(abfd, bfd_h_getx64, \
							((bfd_byte *) ptr))

#define bfd_h_put_16(abfd, val, ptr)	BFD_SEND(abfd, bfd_h_putx16, (val, (bfd_byte *) ptr))
#define bfd_h_get_16(abfd, ptr)		BFD_SEND(abfd, bfd_h_getx16, ((bfd_byte *) ptr))


/* General purpose one fits all.  The do { } while (0) makes a single
 * statement out of it, for use in things like nested if-statements.
 *
 * The idea is to create your external ref as a byte array of the
 * right size eg:
 * char foo[4];
 * then you may do things like:
 * bfd_h_put_x(abfd, 1, &foo);
 */

#define bfd_h_put_x(abfd, val, ptr) \
  do {  \
       if (sizeof((ptr)) == 8) \
		bfd_h_put_64  (abfd, val, (ptr));\
       if (sizeof((ptr)) == 4) \
		bfd_h_put_32  (abfd, val, (ptr));\
  else if (sizeof((ptr)) == 2) \
		bfd_h_put_16 (abfd, val, (ptr));\
  else if (sizeof((ptr)) == 1) \
		bfd_h_put_8  (abfd, val, (ptr));\
  else abort(); } while (0)


#define BFD_COFF_FORMAT	bfd_target_coff_flavour_enum
#define BFD_BOUT_FORMAT	bfd_target_aout_flavour_enum
#define BFD_ELF_FORMAT  bfd_target_elf_flavour_enum

/*
 * Return nonzero iff specified bfd is for big-endian HOST
 */
#define BFD_BIG_ENDIAN_FILE_P(abfd) ((abfd)->xvec->header_byteorder_big_p)

/*
 * Return non-zero iff bfd is for big-endian TARGET (COFF only)
 */
#define BFD_BIG_ENDIAN_TARG_P(abfd) ((abfd)->xvec->byteorder_big_p)

/*
 * Return nonzero iff specified bfd is for coff target
 */
#define BFD_COFF_FILE_P(abfd)	((abfd)->xvec->flavour == BFD_COFF_FORMAT)
#define BFD_BOUT_FILE_P(abfd)	((abfd)->xvec->flavour == bfd_target_aout_flavour_enum)
#define BFD_ELF_FILE_P(abfd)	((abfd)->xvec->flavour == bfd_target_elf_flavour_enum)

/*
 * The names of the only targets the GNU/960 release cares about
 */
#define BFD_BIG_COFF_TARG		"coff-Intel-big"
#define BFD_LITTLE_COFF_TARG		"coff-Intel-little"
#define BFD_BIG_BIG_COFF_TARG		"coff-Intel-big/big"
#define BFD_LITTLE_BIG_COFF_TARG	"coff-Intel-little/big"
#define BFD_BIG_BOUT_TARG		"b.out.big"
#define BFD_LITTLE_BOUT_TARG		"b.out.little"
#define BFD_BIG_ELF_TARG		"elf.big"
#define BFD_LITTLE_ELF_TARG		"elf.little"

#if !defined(NO_BIG_ENDIAN_MODS)
extern PROTO (char *, bfd_make_targ_name,( enum target_flavour_enum format, int hostbigendian, int targbigendian));
#else
extern PROTO (char *, bfd_make_targ_name,( enum target_flavour_enum format, int bigendian));
#endif

typedef struct hashed_str_tab_bucket {
    unsigned offset;
    struct hashed_str_tab_bucket *next_bucket;
} hashed_str_tab_bucket;
    
typedef struct {
    int current_size,max_size,min_size;
    char *strings;
    hashed_str_tab_bucket **buckets;
} hashed_str_tab;

PROTO(hashed_str_tab *,_bfd_init_hashed_str_tab,(bfd *,int));

PROTO(void,_bfd_release_hashed_str_tab,(bfd *,hashed_str_tab *));

PROTO(int, _bfd_lookup_hashed_str_tab_offset,(bfd *,hashed_str_tab *,CONST char *,int));

#endif /* __BFD_H_SEEN__ */
