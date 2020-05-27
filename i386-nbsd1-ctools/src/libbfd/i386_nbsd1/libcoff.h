/* Stuff local to libbfd but shared by COFF code */

/* Copyright (C) 1990, 1991 Free Software Foundation, Inc.

This file is part of BFD, the Binary File Diddler.

BFD is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

BFD is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with BFD; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#define coff_data(bfd)		((struct icofdata *) ((bfd)->tdata))
#define obj_raw_syments(bfd)	(coff_data(bfd)->raw_syments)

#define AUX_LNNOPTR		x_sym.x_fcnary.x_fcn.x_lnnoptr
#define AUX_ENDNDX		x_sym.x_fcnary.x_fcn.x_endndx
#define AUX_LNNO		x_sym.x_misc.x_lnsz.x_lnno
#define AUX_SIZE		x_sym.x_misc.x_lnsz.x_size
#define AUX_DIMEN		x_sym.x_fcnary.x_ary.x_dimen
#define AUX_TAGNDX		x_sym.x_tagndx
#define AUX_FNAME		x_file.x_fname

#define TAG_CLOSED        0x0001
#define TAG_REFERENCED    0x0002
#define TAG_IN_HASH_TABLE 0x0004

typedef struct coff_ptr_struct {
    unsigned long offset;
    unsigned short tag_flags;
    char fix_tag;
    char fix_end;
    struct coff_ptr_struct *lookup_equivalent_tag,*tags_are_equal_equivalent_tag;
    union {
	AUXENT auxent;
	SYMENT syment;
    } u;
} combined_entry_type;

typedef struct {
	asymbol symbol;
	struct lineno_cache_entry *lineno;
} coff_symbol_type;

#define fetch_native_coff_info(ASYM) ((combined_entry_type *) ((ASYM)->native_info))

typedef struct icofdata {
	coff_symbol_type  *symbols;     /* symtab for input bfd */
	unsigned int *conversion_table;
	file_ptr sym_filepos;
	file_ptr str_filepos;   /* Offset in file of first byte *following*
				 * symtab -- i.e., where the string table will
				 * be *if the file has one*
				 */
	combined_entry_type *raw_syments;
	struct lineno *raw_linenos;
	unsigned int raw_syment_count;
	char *string_table;
	unsigned short flags;

	/* These are only valid once writing has begun */
	long int relocbase;
} coff_data_type;

/* We take the address of the first element of a asymbol to ensure that the
 * macro is only ever applied to an asymbol.
 */
#define coffsymbol(asymbol) ((coff_symbol_type *)(&((asymbol)->the_bfd)))


PROTO (int,    coff_dmp_symtab, (bfd *, int, int, int));
PROTO (int,    coff_dmp_linenos, (bfd *, int));
PROTO (int, coff_dmp_full_fmt, (bfd *abfd, asymbol **, 
                                unsigned long , char ));

