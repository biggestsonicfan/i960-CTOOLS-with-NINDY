/*
 * Copyright (C) 1991 Free Software Foundation, Inc.
 *
 * This file is part of GLD, the Gnu Linker.
 *
 * GLD is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * GLD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GLD; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __LDLANG_H__

#define __LDLANG_H__

typedef enum {
	lang_input_file_is_l_enum,
	lang_input_file_is_symbols_only_enum,
	lang_input_file_is_marker_enum,
	lang_input_file_is_fake_enum,
	lang_input_file_is_search_file_enum,
	lang_input_file_is_search_file_add_suffixes_enum,
	lang_input_file_is_magic_syslib_and_startup_enum,
	lang_input_file_is_file_enum,
	lang_input_file_is_l_1_enum
} lang_input_file_enum_type;

typedef unsigned short fill_type;

typedef struct statement_list {
	union lang_statement_union *head;
	union lang_statement_union **tail;
} lang_statement_list_type;


typedef struct memory_region_struct {
	char *name;
	struct memory_region_struct *up,*down;
	bfd_vma origin;
	bfd_offset length_lower_32_bits; /* This quantity refers to the lowermost 32 bits of
					    length of this memory region. */
	bfd_offset length_high_bit; /* This quantity refers to the uppermost 1 bit of the
					  length of the memory region.  This bizarrness is
					  needed in order to represent the entire 32 bit
					  address range, from 0 AND THROUGH 0xffffffff. */
	int flags;
} lang_memory_region_type ;

typedef struct lang_statement_header_struct {
	union  lang_statement_union  *next;
	enum statement_enum {
		lang_output_section_statement_enum,
		    lang_assignment_statement_enum,
		    lang_input_statement_enum,
		    lang_address_statement_enum,
		    lang_wild_statement_enum,
		    lang_input_section_enum,
		    lang_object_symbols_statement_enum,
		    lang_fill_statement_enum,
		    lang_data_statement_enum,
		    lang_target_statement_enum,
		    lang_output_statement_enum,
		    lang_padding_statement_enum,
		    lang_afile_asection_pair_statement_enum
	} type;
} lang_statement_header_type;


typedef struct {
	lang_statement_header_type header;
	union etree_union *exp;
}  lang_assignment_statement_type;


typedef struct lang_target_statement_struct {
	lang_statement_header_type header;
	CONST char *target;
} lang_target_statement_type;


typedef struct lang_output_statement_struct {
	lang_statement_header_type header;
	CONST char *name;
} lang_output_statement_type;


typedef struct lang_output_section_statement_struct {
	lang_statement_header_type header;
	union etree_union *addr_tree;
	lang_statement_list_type children;
	CONST char *memspec;
	union lang_statement_union *next;
	CONST char *name;
	unsigned long subsection_alignment;
	boolean processed;
	boolean section_was_split;
	boolean section_was_allocated;

	asection *bfd_section;

/* These all go into flags output section flags: */

#define LDLANG_HAS_INPUT_FILES 0x00001
#define LDLANG_DSECT           0x00002
#define LDLANG_COPY            0x00004
#define LDLANG_NOLOAD          0x00008
#define LDLANG_ALLOC           0x00010
#define LDLANG_HAS_CONTENTS    0x00020
#define LDLANG_LOAD            0x00040
#define LDLANG_CCINFO          0x00080
#define LDLANG_CAVE_SECTION    0x00100

/* This is used also by the parser to indicate that the fill_opt is the default
   value. */

#define LDLANG_DFLT_FILL       0x10000

	int flags;

	char *region_attributes;
	char *region_name;
	fill_type fill;
	struct lang_output_section_statement_struct *next_in_group;
} lang_output_section_statement_type;

typedef struct lang_group_section_link {
	lang_output_section_statement_type *this_section;
	struct lang_group_section_link *next_link;
} lang_group_section_link_type;

typedef struct lang_group_info {
	union etree_union *addr_tree;
	char *region_name;
	char *region_attributes;
	fill_type fill;
	lang_group_section_link_type first_link;
	lang_group_section_link_type *current_link;
} lang_group_info_type;

typedef struct {
	lang_statement_header_type header;
} lang_common_statement_type;

typedef struct {
	lang_statement_header_type header;
} lang_object_symbols_statement_type;

typedef struct {
	lang_statement_header_type header;
	fill_type fill;
} lang_fill_statement_type;

typedef struct {
	lang_statement_header_type header;
	unsigned int type;
	union  etree_union *exp;
	bfd_vma value;
	asection *output_section;
	bfd_vma output_vma;
} lang_data_statement_type;




typedef struct lang_input_statement_struct {
	lang_statement_header_type header;

	CONST char *filename;	/* Name of this file.  */

	CONST char *local_sym_name;
			/* Name to use for the symbol giving address of text
			 * start.  Usually the same as filename, but for a
			 * file spec'd with -l this is the -l switch itself
			 * rather than the filename
			 */

	bfd *the_bfd;
	boolean closed;
	file_ptr passive_position;

	/* SYMBOL TABLE OF THE FILE.  */
	asymbol **asymbols;
	unsigned int symbol_count;

	struct lang_input_statement_struct *subfiles;
			/* For library, points to chain of entries for the
			 * library members.
			 */

	bfd_size_type total_size;
			/* Size of contents of this file, if library member */

	struct lang_input_statement_struct *superfile;
			/* For library member, points to the library's own
			 * entry.
			 */

	struct lang_input_statement_struct *chain;
			/* For library member, points to next entry for next
			 * member.
			 */

	union lang_statement_union  *next;
			/* Points to the next file - whatever it is,
			 * wanders up and down archives
			 */

	union  lang_statement_union  *next_real_file;
			/* Point to the next file, but skips archive contents */

	boolean search_dirs_flag;
			/* 1 means search a set of directories for this file */

	boolean add_suffixes,magic_syslib_and_startup;

	char    *file_extension;

	boolean prepend_lib;

	boolean just_syms_flag;
			/* 1 means this is base file of incremental load.
			 * Do not load this file's text or data.
			 * Also default text_start to after this file's bss.
			 */

	boolean loaded;
	CONST char *target;
	boolean real;
	asection *common_section;
	asection *common_output_section;
	unsigned long dev_no,ino;
} lang_input_statement_type;


typedef struct {
	lang_statement_header_type header;
	asection *section;
	lang_input_statement_type *ifile;
} lang_input_section_type;


typedef struct {
	lang_statement_header_type header;
	asection *section;
	union lang_statement_union *file;
} lang_afile_asection_pair_statement_type;

typedef struct lang_wild_statement_struct {
	lang_statement_header_type header;
	CONST char *section_name;
	CONST char *filename;
	lang_statement_list_type children;
} lang_wild_statement_type;

typedef struct lang_address_statement_struct {
	lang_statement_header_type header;
	CONST  char *section_name;
	union  etree_union *address;
} lang_address_statement_type;

typedef struct {
	lang_statement_header_type header;
	bfd_vma output_offset;
	size_t size;
	asection *output_section;
	fill_type fill;
} lang_padding_statement_type;

typedef union lang_statement_union {
	lang_statement_header_type header;
	union lang_statement_union *next;
	lang_wild_statement_type wild_statement;
	lang_data_statement_type data_statement;
	lang_address_statement_type address_statement;
	lang_output_section_statement_type output_section_statement;
	lang_afile_asection_pair_statement_type afile_asection_pair_statement;
	lang_assignment_statement_type assignment_statement;
	lang_input_statement_type input_statement;
	lang_target_statement_type target_statement;
	lang_output_statement_type output_statement;
	lang_input_section_type input_section;
	lang_common_statement_type common_statement;
	lang_object_symbols_statement_type object_symbols_statement;
	lang_fill_statement_type fill_statement;
	lang_padding_statement_type padding_statement;
} lang_statement_union_type;


PROTO(void,lang_init,(void));
PROTO(struct memory_region_struct, *lang_memory_region_lookup,(CONST char *CONST));
PROTO(void ,lang_map,(FILE *));
PROTO(void,lang_set_flags,(int  *, CONST char *));
PROTO(void,lang_add_output,(char *));
PROTO(struct symbol_cache_entry *,create_symbol,(CONST char *, unsigned int, struct sec *));
PROTO(void ,lang_section_start,(CONST char *, union etree_union *));
PROTO(void,lang_add_entry,(CONST char *));
PROTO(void,lang_add_target,(CONST char *));
PROTO(void,lang_add_wild,(CONST char *CONST , CONST char *CONST));
PROTO(void,lang_add_fill,(int));
PROTO(void,lang_add_assignment,(union etree_union *));
PROTO(void,lang_add_attribute,(enum statement_enum));
PROTO(void,lang_startup,(char *));
PROTO(void,lang_float,(int));
PROTO(void,lang_leave_output_section_statement,(bfd_vma, CONST char *));
PROTO(unsigned long,lang_abs_symbol_at_end_of,(CONST char *, CONST char *));
PROTO(void,lang_abs_symbol_at_beginning_of,(CONST char *, CONST char *));
PROTO(void,lang_statement_append,(struct statement_list *, union lang_statement_union *, union lang_statement_union **));
PROTO(void, lang_for_each_file,(void (*dothis)(lang_input_statement_type *)));
PROTO(void, lang_process,(void));
PROTO(void, ldlang_add_file,(lang_input_statement_type *));
PROTO(lang_output_section_statement_type *,lang_output_section_find,(CONST char * CONST));

PROTO(lang_input_statement_type *,
      lang_add_input_file,(char *name,
			   lang_input_file_enum_type file_type,
			   char *target,
			   bfd *abfd,
			   unsigned long dev_no,
			   unsigned long ino));
PROTO(lang_output_section_statement_type *,
      lang_output_section_statement_lookup,(CONST char * CONST name,int));

PROTO(void, ldlang_add_undef,(CONST char *CONST name));

#endif
