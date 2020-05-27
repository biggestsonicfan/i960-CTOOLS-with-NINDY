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

/*
 * $Id: ldwrite.c,v 1.54 1996/01/17 01:21:48 alex Exp $ 
 */

/* 
 * This module writes out the final image by reading sections from the
 * input files, relocating them and writing them out
 *
 * There are two main paths through this module, one for normal
 * operation and one for partial linking. 
 *
 * During  normal operation, raw section data is read along with the
 * associated relocation information, the relocation info applied and
 * the section data written out on a section by section basis.
 *
 * When partially linking, all the relocation records are read to work
 * out how big the output relocation vector will be. Then raw data is
 * read, relocated and written section by section.
 *
 * Written by Steve Chamberlain steve@cygnus.com
 */

#include "sysdep.h"
#include "bfd.h"
#include "ldlang.h"
#include "ld.h"
#include "ldwrite.h"
#include "ldmisc.h"
#include "ldsym.h"
#include "ldgramtb.h"


extern bfd *output_bfd;
extern ld_config_type config;

#ifndef errno
#ifndef __HIGHC__
	extern int errno;
#endif
#endif

#ifdef __STDC__
	void lang_for_each_statement(void (*func)());
#else /* __STDC__ */
	void lang_for_each_statement();
#endif /* __STDC__ */

static struct cave_ref_node {
    asection *section_to_be_re_relocated;
    unsigned long offset_into_section_to_be_re_relocated;
    asymbol *cave_input_section_symbol;
    unsigned long cave_input_section_symbol_value,new_cave_input_section_offset;
} *cave_refs;
static int max_cave_refs,current_cave_ref_cnt;

static void
insert_into_cave_ref_list(is,o,sym,val)
    asection *is;
    unsigned long o;
    asymbol *sym;
    unsigned long val;
{
    struct cave_ref_node *p;

    if (cave_refs) {
	if (current_cave_ref_cnt+1 >= max_cave_refs)
		cave_refs = (struct cave_ref_node *) ldrealloc(cave_refs,
							       (max_cave_refs *= 2) *
							       sizeof(struct cave_ref_node));
    }
    else {
	max_cave_refs = 20;
	cave_refs = (struct cave_ref_node *) ldmalloc(max_cave_refs * sizeof(struct cave_ref_node));
    }

    p = cave_refs + current_cave_ref_cnt++;

    p->section_to_be_re_relocated = is;
    p->offset_into_section_to_be_re_relocated = o;
    p->cave_input_section_symbol = sym;
    p->cave_input_section_symbol_value = val;

    /* Make sure all cave sections are allocated to the same output section: */

    ASSERT(cave_refs->cave_input_section_symbol->section->output_section ==
	   sym->section->output_section);
}

static unsigned long cave_symbol_value(cs) 
    struct cave_ref_node *cs;
{
    unsigned long r = cs->cave_input_section_symbol->section->output_offset +
	    cs->cave_input_section_symbol_value;
    return r;
}


static int compare_cave_refs(l,r)
    struct cave_ref_node *l,*r;
{
    unsigned long l_value = cave_symbol_value(l);
    unsigned long r_value = cave_symbol_value(r);

    if (l_value < r_value)
	    return -1;
    else if (l_value == r_value)
	    return 0;
    else
	    return 1;
}

static void put_int(section,size,data,ptr)
    asection *section;
    int size,data;
    char *ptr;
{
    extern unsigned long _do_putl16(),_do_putb16();
    extern unsigned long _do_putl32(),_do_putb32();

    if (section->flags & SEC_IS_BIG_ENDIAN) {
	if (size == 2)
		_do_putb16(data,ptr);
	else if (size == 4)
		_do_putb32(data,ptr);
	else
		ASSERT(0);
    }
    else {
	if (size == 2)
		_do_putl16(data,ptr);
	else if (size == 4)
		_do_putl32(data,ptr);
	else
		ASSERT(0);

    }
}

static unsigned long get_int(section,size,data)
    asection *section;
    int size;
    char *data;
{
    extern unsigned long _do_getl16(),_do_getb16();
    extern unsigned long _do_getl32(),_do_getb32();

    if (section->flags & SEC_IS_BIG_ENDIAN) {
	if (size == 2)
		return _do_getb16(data);
	else if (size == 4)
		return _do_getb32(data);
	else
		ASSERT(0);
    }
    else {
	if (size == 2)
		return _do_getl16(data);
	else if (size == 4)
		return _do_getl32(data);
	else
		ASSERT(0);

    }
}

static int is_bad_endian(sec_is_big_endian)
int sec_is_big_endian;
{
	short i = 0x0100;
	char* p = (char*) &i;

	return (*p > 0) != (sec_is_big_endian > 0);
}
	
static int compress_cave_output_section()
{
    					/* The cave code has been collected at 
					   the bottom of an output section */
	asection *cave_output_section = 
		cave_refs->cave_input_section_symbol->section->output_section;

    					/* This is for the UNCOMPRESSED cave 
					   section. */
	char *cave_section_contents;

    					/* This will refer to the size of the 
					   cave stuff that is at the bottom of 
					   the above section. */
	int size_of_uncompressed_cave_section, dtable_size;

					/* Refers to the start of the cave code
					   at the bottom of the section. */
	int offset_of_cave_code;

					/* First, sort the cave refs.  */
	qsort(cave_refs, current_cave_ref_cnt, sizeof(*cave_refs), 
		compare_cave_refs);

					/* The smallest refers to the first 
					   function in the cave code.  Minus 4 
					   to track over the function header: */
	offset_of_cave_code = cave_symbol_value(cave_refs) - 4;

					/* The size of the cave code is the 
					   total size of the section minus the 
					   offset of the cave code: */
	size_of_uncompressed_cave_section = cave_output_section->size - 
		offset_of_cave_code;

	cave_section_contents = ldmalloc(size_of_uncompressed_cave_section);

	if (!bfd_get_section_contents(output_bfd,cave_output_section,
		cave_section_contents, offset_of_cave_code,
		size_of_uncompressed_cave_section))
	{
		info("%F can not read cave output section's contents.\n");
	}
	else
	{
#include "decode.h"
#include "encode.h"

extern boolean option_v;

		Encode_table* etable;
		int compressed_bit;	/* 1 indicates successfull compression;
					   0 indicates aborted compression */
		int rv = 0;
					/* dot iterates through the uncompressed
					   functions in cave_section_contents */

					/* new_dot points at the new resting 
					   places of the functions in 
					   new_cave_section_contenets. */
		int dot = 0, new_dot = 0;
					/* points into the cave_refs array. */
		int symptr=0;
					/* keeps track of the value of current 
					   input section's value of dot. */
		int this_section_starts_at;
		int max_tmp_func_buff = 100;
		char *tmp_func_buff = ldmalloc(max_tmp_func_buff);
		char *new_cave_section_contents = 
			ldmalloc(size_of_uncompressed_cave_section);

		etable = select_compression_encoding(cave_section_contents,
			size_of_uncompressed_cave_section, 
			1,              /* section has frames in it */
			is_bad_endian(cave_output_section->flags & 
				SEC_IS_BIG_ENDIAN), option_v);

		for (dot=0; dot < size_of_uncompressed_cave_section; symptr++) 
		{
			unsigned long function_uncompressed_size;
			unsigned long function_compressed_size;

			function_uncompressed_size = 
				get_int( cave_output_section, 4, 
				cave_section_contents + dot);

					/* dot points at uncompressed function 
					   body now. */
			dot += 4;
					/* New_dot points at resting place of 
					   compressed function. */
			new_dot += 4;
			ASSERT(dot + offset_of_cave_code == 
				cave_symbol_value(cave_refs + symptr));

					/* First, fixup cave symbol values. */

			if ((cave_refs + symptr)->
				cave_input_section_symbol_value == 4)
			{
				this_section_starts_at = 
					new_dot + offset_of_cave_code - 4;
				(cave_refs + symptr)->
					new_cave_input_section_offset = 
					this_section_starts_at;
			}
			else
			{
				(cave_refs + symptr)->
					new_cave_input_section_offset = 
					this_section_starts_at;
				(cave_refs + symptr)->
					cave_input_section_symbol_value =
					(new_dot + offset_of_cave_code) - 
						this_section_starts_at;
			}

			if (function_uncompressed_size >= max_tmp_func_buff)
				tmp_func_buff = ldrealloc(tmp_func_buff,
					max_tmp_func_buff = 
						function_uncompressed_size);

			function_compressed_size = compress_buffer(etable,
				tmp_func_buff, cave_section_contents + dot, 
				function_uncompressed_size, 
				is_bad_endian(cave_output_section->flags & 
					SEC_IS_BIG_ENDIAN));

				/* if function was compressed, set the bit */
			if (function_compressed_size)
				compressed_bit = 1;
			else
				compressed_bit = 0;

				/* if function was compressed, set the low order
				   bit in the uncompressed size */
			put_int(cave_output_section, 4, 
				function_uncompressed_size | compressed_bit,
				new_dot - 4 + new_cave_section_contents);

			if (function_compressed_size)
			{
				memcpy(new_cave_section_contents + new_dot,
					tmp_func_buff, 
					function_compressed_size);

				new_dot += function_compressed_size;

					/* Fill with 0's and align new_dot to 
					   word alignment: */
				for (; new_dot & 3; new_dot++)
					*(new_cave_section_contents + new_dot) =
						0;
			}
			else
			{
					/* Just copy the function to its new 
					   resting place. */
				memcpy(new_cave_section_contents + new_dot,
					cave_section_contents+dot, 
					function_uncompressed_size);
				new_dot += function_uncompressed_size;
			}
					/* dot now points at the start of the 
					   next function header. */
			dot += function_uncompressed_size;
		}

		dtable_size = decompression_table_size(etable);
		if (size_of_uncompressed_cave_section - new_dot >= dtable_size)
		{
					/* Let's install decompression_table 
					   into new_dot location of 
					   new_cave_section_contents. */
			int i;
			extern boolean option_v;
			unsigned long bo_new_dot;
			ldsym_type *cdt = ldsym_get_soft("__decompression_table");

			ASSERT(cdt->sdefs_chain);

					/* Let's first patch the address of 
					   decompression_table with new_dot: */
			put_int((*cdt->sdefs_chain)->section->output_section,
				4, new_dot+ offset_of_cave_code+ 
					cave_output_section->vma,&bo_new_dot);
			if (!bfd_set_section_contents(output_bfd,
				(*cdt->sdefs_chain)->section->output_section,
				&bo_new_dot,
				(*cdt->sdefs_chain)->section->output_offset+
				(*cdt->sdefs_chain)->value, 4))
			{
				info("Whoops 2");
			}

					/* Zero out rest of data in the 
					   cave_output_section here. */
			memset(new_cave_section_contents + new_dot, 0,
				size_of_uncompressed_cave_section - new_dot);

			decompression_table_pack(new_cave_section_contents +
				new_dot, etable, 
				is_bad_endian(cave_output_section->flags & 
					SEC_IS_BIG_ENDIAN));

			new_dot += dtable_size;

			if (!bfd_set_section_contents(output_bfd,
				cave_output_section, new_cave_section_contents,
				offset_of_cave_code,
				size_of_uncompressed_cave_section))
			{
				info("bssc whoops.");
			}
			cave_output_section->size -= 
				size_of_uncompressed_cave_section - new_dot;
			if (option_v) {
			    printf("The uncompressed text size is %d bytes\n",
				   cave_output_section->size);
			    printf("The uncompressed cave size is %d bytes\n",
				   size_of_uncompressed_cave_section);
			    printf("The compressed cave section size is %d bytes\n",
				   new_dot - dtable_size);
			    printf("Decode table size is %d bytes\n", dtable_size);
			    printf("Total compression savings is %d bytes\n", 
				   size_of_uncompressed_cave_section - new_dot);
			    printf("Short alphabet has %d members\n",
				   etable->short_alpha);
			    printf("Char alphabet has %d members\n",
				   etable->char_alpha);
			}
	    		rv = 1;
		}
		else
		{
			info("Can't compress program: need %d bytes have: %d\n",
				dtable_size, 
				size_of_uncompressed_cave_section - new_dot);
			rv = 0;
		}
		compression_table_free(etable);
		compression_alphabet_free();
		free(cave_section_contents);
		free(new_cave_section_contents);
		free(tmp_func_buff);
		return rv;
	}
	return 0;
}

static void re_relocate_cave_refs()
{
    int i;

    for (i=0;i < current_cave_ref_cnt;i++) {
	unsigned long seek_address = cave_refs[i].section_to_be_re_relocated->output_offset +
			cave_refs[i].offset_into_section_to_be_re_relocated;
	unsigned long new_word;

	cave_refs[i].cave_input_section_symbol->section->output_offset =
		cave_refs[i].new_cave_input_section_offset;
	put_int(cave_refs[i].section_to_be_re_relocated->output_section,
		4,
		cave_symbol_value(cave_refs+i)+
		cave_refs[i].cave_input_section_symbol->section->output_section->vma,
		&new_word);
	bfd_set_section_contents(output_bfd,cave_refs[i].section_to_be_re_relocated->output_section,
				 &new_word,seek_address,4);
    }
}

boolean saw_decompression_table_symbol;

static int
DEFUN(perform_relocation,(ibfd, isec, data, symbols, outseciscopy),
      bfd *ibfd AND
      asection *isec AND
      char *data AND
      asymbol **symbols AND
      int outseciscopy)
{
#define MAX_ERRORS_IN_A_ROW 5
	static asymbol *error_symbol = (asymbol *)NULL;
	int reloc_errors = 0;
	static unsigned int error_count = 0;

	arelent **reloc_vector;
	arelent **parent;
	asection *os = isec->output_section;

	reloc_vector = (arelent **)ldmalloc( get_reloc_upper_bound(ibfd,isec) );

	if (!bfd_canonicalize_reloc(ibfd,isec,reloc_vector,symbols)) {
		free((char *)reloc_vector);
		return 0;
	} else if (outseciscopy) {
	    info("Warning: input file: %B has relocation directives and is in a COPY output section.\n",ibfd);
	    info("Relocation was not performed on this file.\n");
		free((char *)reloc_vector);
		return 0;
	}

	for (parent = reloc_vector; *parent; parent++) {
		bfd_reloc_status_enum_type r;
		asymbol *s;
		arelent *p = *parent;
		unsigned long previous_symbol_value;
		extern boolean do_cave;

		s = p->sym_ptr_ptr ? *(p->sym_ptr_ptr) : (asymbol *)NULL;
		
#define CAVE_NAME "cave"

		if (do_cave &&
		    saw_decompression_table_symbol &&
		    s &&
		    s->section &&
		    !strcmp(s->section->name,CAVE_NAME))
			previous_symbol_value = get_int(isec,4,data+p->address) - s->section->vma;

		r = bfd_perform_relocation( ibfd, p, data, isec,
			config.relocateable_output ? output_bfd : (bfd*)NULL);

		if (config.relocateable_output) {
			/* A partial link: keep the relocs */
			os->orelocation[os->reloc_count_iterator++] = p;
		}

		switch (r) {
		case bfd_reloc_ok:
		    if (do_cave && saw_decompression_table_symbol && s && s->section &&
			!strcmp(s->section->name,CAVE_NAME))
			    insert_into_cave_ref_list(isec,p->address,s,previous_symbol_value);
#undef CAVE_NAME
		    break;
		case bfd_reloc_undefined:
			/* Remember the symbol and never print more
			 * than a reasonable number of them in a row
			 */
			reloc_errors++;
			if (s == error_symbol) {
				error_count++;
			} else {
				config.make_executable = false;
				error_count = 0;
				error_symbol = s;
			}
			if (error_count < MAX_ERRORS_IN_A_ROW) {
				info("%C: undefined reference to `%T'\n",
					ibfd, isec, symbols, p->address, s);
			} else if (error_count == MAX_ERRORS_IN_A_ROW){
				info("%C: more references to `%T' follow\n",
					ibfd, isec, symbols, p->address, s);
			}
			break;
		case bfd_reloc_dangerous:
			info("%B: Relocation of type %s at offset %x\n",ibfd,p->howto->name,p->address);
			info("in input section: %s, resulted in flipping one or both of the least\n",isec->name);
			info("significant bits in a CTRL instruction.\n");
			break;
		case bfd_reloc_outofrange:
			info("%B: Relocation calculation of type %s at offset 0x%V\n   in input section %s failed due to out-of-range result\n",
				ibfd,p->howto->name,p->address,isec->name);
			info("symbol: %t\n",s);
			reloc_errors++;
			break;
		case bfd_reloc_overflow:
			info("%B: Relocation calculation of type %s at offset 0x%V\n   in input section %s failed due to overflow of available field\n",
				ibfd,p->howto->name,p->address,isec->name);
			info("symbol: %t\n",s);
			reloc_errors++;
			break;
		case bfd_reloc_no_code_for_syscall:
			info("%B: Relocation of type %s at offset 0x%V\n",ibfd,p->howto->name,p->address);
			info("in input section %s failed due to lack of a code address for the system call\n",
			     isec->name);
			info("symbol: %t\n",s);
			reloc_errors++;
			break;

		default:
			info("%B: Unknown relocation error, symbol `%T'\n", ibfd, s);
			reloc_errors++;
			break;
		}
	}
	if (!config.relocateable_output && isec->relocation) {
	    free(isec->relocation);
	    isec->relocation = 0;
	}
	free((char *)reloc_vector);
	return reloc_errors;
}


PTR data;
static int reloc_errs = 0;

static void
DEFUN(copy_and_relocate,(stmt),
      lang_statement_union_type *stmt)
{
    bfd_vma value;
    bfd_byte play_area[LONG_SIZE];
    unsigned int size;
    asection *isec;
    asection *osec;
    lang_input_statement_type *ifile;
    bfd *ibfd;

#define ERR(string,bfd) info("%F%B error %s section contents %E\n",bfd,string)

    switch (stmt->header.type) {
 case lang_padding_statement_enum:

	/*
	  Don't literally insert a pad if the section does not really have contents. 
	  Bss sections can have pads, but they do not really have contents.
	  */
	osec = stmt->padding_statement.output_section;
	if ((bfd_get_section_flags(output_bfd,
				   stmt->padding_statement.output_section) &
	     SEC_HAS_CONTENTS)                          &&
	    !(osec->flags & SEC_IS_DSECT)               &&
	    !(osec->flags & SEC_IS_NOLOAD)) {
	    char *temp = (char *) ldmalloc(stmt->padding_statement.size);
	    int i;
	    unsigned char two_bytes[2];
     
	    two_bytes[0] = stmt->padding_statement.fill & 0xff;
	    two_bytes[1] = (stmt->padding_statement.fill >> 8) & 0xff;
	    for (i=0;i < stmt->padding_statement.size;i++)
		    *(temp+i) = two_bytes[i % 2];

	    if (!bfd_set_section_contents(output_bfd,
					  stmt->padding_statement.output_section,
					  temp,
					  stmt->padding_statement.output_offset,
					  stmt->padding_statement.size))
		    ERR("Writing padding",output_bfd);
	    free(temp);
	}
	 break;
 case lang_data_statement_enum:
	 value = stmt->data_statement.value;
	 switch (stmt->data_statement.type) {
     case LONG:
	     bfd_put_32(output_bfd, value, play_area);
	     size = LONG_SIZE;
	     break;
     case SHORT:
	     bfd_put_16(output_bfd, value, play_area);
	     size = SHORT_SIZE;
	     break;
     case BYTE:
	     bfd_put_8(output_bfd, value, play_area);
	     size = BYTE_SIZE;
	     break;
	 }

	 if (!bfd_set_section_contents(output_bfd,
				  stmt->data_statement.output_section,
				  play_area,
				  stmt->data_statement.output_vma,
				  size))
		 ERR("Writing data statement",output_bfd);
	 break;

	case lang_input_section_enum:
		isec = stmt->input_section.section;
		osec = isec->output_section;
		ifile = stmt->input_section.ifile;

		/* The checks for SEC_IS_DSECT and SEC_IS_NOLOAD may seem
		redundant, but all the flags of the input sections get
		OR'd into the flags field of the output section, so there
		is no other way to weed them out than to check directly */
		if (!ifile->just_syms_flag
		&&   (osec->flags & SEC_HAS_CONTENTS)
		&&  !(osec->flags & SEC_IS_DSECT)
		&&  !(osec->flags & SEC_IS_NOLOAD)
		&&  (isec->size != 0)) {
			ibfd = ifile->the_bfd;
			if (!bfd_get_section_contents(ibfd,isec,data,0,
								isec->size))
			{
				ERR("reading",ibfd);
			}
			reloc_errs += perform_relocation(ibfd,isec,data,ifile->asymbols,
							 osec->flags & SEC_IS_COPY);
			if (!bfd_set_section_contents(output_bfd,osec,data,
						isec->output_offset,isec->size))
			{
				ERR("writing",output_bfd);
			}
		}
		break;
	default:
		/* All the others fall through */
		break;
	}
}


static void 
DEFUN(read_relocs,(abfd, section, symbols),
      bfd *abfd AND
      asection *section AND
      asymbol **symbols)
{
	bfd_size_type reloc_size;
	arelent **reloc_vector;

	/* Work out the output section ascociated with this input section */
	asection *output_section = section->output_section;

	if (output_section && (output_section->flags & SEC_IS_DSECT)) {
		reloc_size = 0;
		reloc_vector = (arelent **)NULL;
	}
	reloc_size = get_reloc_upper_bound(abfd, section);
	/* By adding sizeof(unsigned long) below, we prevent malloc(0) / free(that_ptr). */
	reloc_vector = (arelent **)ldmalloc(reloc_size+sizeof(unsigned long));

	if (bfd_canonicalize_reloc(abfd, section, reloc_vector, symbols)) {
		output_section->reloc_count += section->reloc_count;
	}
}


static  void
DEFUN_VOID(write_rel)
{
	asection *sec;
	bfd *abfd;
	lang_input_statement_type *stmt;
	extern lang_statement_list_type file_chain;

	/*
	 * Run through each section of each file and work out the total
	 * number of relocation records which will finally be in each
	 * output section.
	 */
	for (stmt = (lang_input_statement_type *)file_chain.head;
	     stmt;
	     stmt = (lang_input_statement_type *)stmt->next)
	{
		abfd = stmt->the_bfd;
		for (sec = abfd->sections; sec; sec = sec->next) {
			read_relocs(abfd, sec, stmt->asymbols);
		}
	}

	/*
	 * Now run though all the output sections and allocate the space
	 * for all the relocations
	 */
	for (sec = output_bfd->sections; sec; sec = sec->next){
	    if ((sec->flags & SEC_HAS_CONTENTS) && sec->reloc_count) {
		sec->orelocation = (arelent **)
			ldmalloc((sizeof(arelent**)*sec->reloc_count));
		output_bfd->flags |= HAS_RELOC;
	    }
	    sec->reloc_count_iterator = 0;
	    /* COMMENTED OUT.	    
	       sec->flags |= SEC_HAS_CONTENTS;
	       */
	}
}



static void
print_section_map(allsecs,nsecs)
    asection **allsecs;
		/* Pointer to array of pointers to output section statements,
		 * assumed to be sorted in ascending starting address order
		 * and assumed not to contain any zero-length sections.
		 */
    int nsecs;	/* Number of entries in array pointed to by allsecs */
{
	int i;
	char *overlap;
	asection *prev, *cur, *next;
		
	fprintf( stderr, "\n\tbegin addr | end addr   | section\n" );
	fprintf( stderr,   "\t-----------+------------+--------\n" );

	for ( i = 0; i < nsecs; i++ ){
		prev = next = 0;
		if ( i > 0 ){
			prev=allsecs[i-1];
		}

		cur = allsecs[i];

		if ( i < nsecs - 1 ){
			next=allsecs[i+1];
		}

		if ( (prev && (prev->vma + prev->size > cur->vma))
		||   (next && (cur->vma + cur->size > next->vma)) ){
			overlap = "OVERLAP";
		} else {
			overlap = "";
		}
		fprintf( stderr, "%s\t0x%08x | 0x%08x | %s\n",
			overlap, cur->vma, cur->vma + cur->size - 1, cur->name);
	}
}

static
int
precious_names_sorter(s1,s2,name,multiplier)
    char *s1,*s2,*name;
    int multiplier;
{
    if (strcmp(s1,name) == 0)
	    return -multiplier;
    if (strcmp(s2,name) == 0)
	    return multiplier;
    return 0;
}

static int
sorter(s1, s2)
    asection **s1;
    asection **s2;
{
    if ((*s1)->vma < (*s2)->vma)
	    return -1;
    else if ((*s1)->vma == (*s2)->vma) {
	/* This retains the name-dependent order between sections
	   at the same address in the absence of user directives. */
	int rv,i;
	static struct { char *name; int multiplier; } sect_sorter_data[3]
		= {{".text",1},{".data",1},{".bss",-1}};

	/* if s2 is a code section and s1 is not then s2 is bigger than s1. */
	if (((*s1)->flags & SEC_CODE) != (rv=(*s2)->flags & SEC_CODE))
		return rv ? 1 : -1;
	/* Also, if s2 is a data section and s1 is not then s2 is bigger than s1. */
	if (((*s1)->flags & SEC_DATA) != (rv=(*s2)->flags & SEC_DATA))
		return rv ? 1 : -1;

	/* Both s1 and s2 are code or data sections.   Here, we sort by the precious names:
	   The .text section should precede the other text sections.  The .data section
	   should precede the other data sections, and .bss should appear last:
	   .text
	   text sections   
	   ...
	   .data
	   data sections
	   ...
	   .bss
	   */

	for (i=0;i < 3;i++)
		if (rv=precious_names_sorter((*s1)->name,(*s2)->name,
					     sect_sorter_data[i].name,
					     sect_sorter_data[i].multiplier))
			return rv;
	/* Else, we order sections alphabetically by name if their start
	   addresses are the same and section type are the same*/
	if (!(rv=strcmp((*s1)->name,(*s2)->name))) {
	    info("%F Linker found two sections with the same name %s at the same address 0x%lx\n",(*s1)->name,(*s1)->vma);
	}
	return rv < 0 ? -1 : 1;
    }
    else
	    return 1;
}


/* Check for overlapping output sections.
 * Fatal error (and print section map) if any do overlap.
 *
 * 'allsecs' is a pointer to a dynamically allocated array of pointers
 * to the output section statements' section pointers. 
 * We sort the array array in order of starting address
 * of sections, then run through it checking for overlaps.
 */
static void
DEFUN_VOID(check_section_overlap)
{
    lang_statement_union_type *u;
    lang_output_section_statement_type *v;
    int nsecs,i;
    extern lang_statement_list_type lang_output_section_statement;
    asection **allsecs;

    nsecs = output_bfd->section_count;
    allsecs = (asection **)
	    ldmalloc(nsecs * sizeof(asection *));

    i = 0;
    for (u = lang_output_section_statement.head; u; u = v->next) {
	v = &u->output_section_statement;
	allsecs[i++] = v->bfd_section;
    }

    ASSERT(i == nsecs);

    qsort((char *) allsecs, nsecs, sizeof(allsecs[0]), sorter);

    for ( i = 1; i < nsecs; i++ ){
	asection *s1, *s2;
		
	s1 = allsecs[i-1];
	s2 = allsecs[i];
	if ((s1->flags & SEC_ALLOC & s2->flags) &&
	    (s1->size) && (s2->size) && (s1->vma + s1->size > s2->vma)
	    && (!(s1->flags & SEC_IS_DSECT)) &&
	    (!(s1->flags & SEC_IS_COPY)) &&
	    (!(s2->flags & SEC_IS_DSECT)) &&
	    (!(s2->flags & SEC_IS_COPY))){
	    print_section_map( allsecs, nsecs);
	    info( "\n%F OUTPUT SECTIONS OVERLAP\n" );
	}
    }

    if (1) {
	asection *this;

	/* This code re-orders sections by start address */
	output_bfd->sections = this = allsecs[0];
	for (i = 0; i < nsecs; i++) {
	    this->secnum = i;
	    if (i == nsecs-1) {
		this->next = (struct sec *)NULL;
	    }
	    else {
		this->next = allsecs[i+1];
		this = allsecs[i+1];
	    }
	}
    }
}


void
DEFUN_VOID(ldwrite)
{
	extern bfd_size_type largest_section;

	/* If the largest section is 0, then we will prevent malloc(0) and free(that_ptr)
	   by adding 4 bytes: */
	data = (PTR) ldmalloc(largest_section + sizeof(unsigned long));
	if (config.relocateable_output) {
		write_rel();
	}

	check_section_overlap();

	/* Output the symbol table (both globals and locals) */
	ldsym_write ();

	/* Output the text and data segments, relocating as we go */
	lang_for_each_statement(copy_and_relocate);
	if (reloc_errs)
		info("%P%X: Can not perform relocation.\n");
	free(data);
	if (cave_refs) {
	    if (compress_cave_output_section())
		    re_relocate_cave_refs();
	}
}
