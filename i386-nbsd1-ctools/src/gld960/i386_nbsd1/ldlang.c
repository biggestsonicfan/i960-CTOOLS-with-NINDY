/* Copyright (C) 1991 Free Software Foundation, Inc.
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

/* $Id: ldlang.c,v 1.199 1995/10/13 22:54:36 paulr Exp $ */

#include "sysdep.h" 
#include "bfd.h"

#include "ld.h"
#include "ldmain.h"
#include "ldsym.h"
#include "ldgramtb.h"

#include "ldlang.h"
#include "ldexp.h"
#include "ldemul.h"
#include "ldlex.h"
#include "ldmisc.h"

/* FORWARDS */
PROTO(static void, default_section_order,(void));
    
/* LOCALS */
static int     manual_allocation;    /* This is set if the user wants to stipulate
					allocation (i.e. assigning output sections to
					memory regions / addresses). */
static CONST   char *startup_file;
static boolean made_default_region = false;
static lang_output_section_statement_type *default_common_section;
static lang_input_statement_type *first_file;
static CONST char *current_target;
static CONST char *output_target;
static lang_statement_list_type statement_list;
static boolean in_group = false;
static lang_group_info_type *current_group = (lang_group_info_type *)NULL;
static lang_output_section_statement_type *current_section;

    
/* EXPORTS */
lang_statement_list_type *stat_ptr = &statement_list;
lang_input_statement_type *script_file = 0;
boolean option_longmap = false;
lang_statement_list_type file_chain = {0};
CONST char *entry_symbol = 0;
bfd_size_type largest_section = 0;
boolean lang_has_input_file = false;
lang_output_section_statement_type *create_object_symbols = 0;
boolean lang_float_flag = false;
lang_statement_list_type lang_output_section_statement;
lang_statement_list_type input_file_chain;
    
/* IMPORTS */
extern char  *default_target;
extern unsigned int undefined_global_sym_count;
extern char *current_file;
extern bfd *output_bfd;
extern enum bfd_architecture ldfile_output_architecture;
extern enum target_flavour_enum output_flavor;
extern unsigned long ldfile_output_machine,ldfile_real_output_machine;
extern char *ldfile_output_machine_name;
extern ldsym_type *symbol_head;
extern unsigned int commons_pending;
extern args_type command_line;
extern ld_config_type config;
extern boolean had_script;
extern boolean write_map;
extern boolean C_switch_seen;
extern boolean B_switch_seen;
extern boolean suppress_all_warnings;
extern HOW_INVOKED invocation;
extern int host_byte_order_is_big_endian;
extern void check_for_section_split();
    
#if defined(__STDC__) || defined(WIN95)
#define cat(a,b) a##b
#else
#define cat(a,b) a/**/b
#endif
	    
#define new_stat(x,y) \
	    (cat(x,_type)*) new_statement(cat(x,_enum), sizeof(cat(x,_type)),y)
    
#define outside_section_address(q) \
            ((q) ? ( (q)->output_offset + (q)->output_section->vma ) : 0)
    
#define outside_symbol_address(q) \
	    ((q)->value +   outside_section_address(q->section))
    
/* bits in memory region structure flags that indicate attributes */
#define MEM_READ  1
#define MEM_WRITE 2
#define MEM_EXEC  4
#define MEM_LOAD  8
    
    
/*----------------------------------------------------------------------
 *  lang_for_each_statement walks the parse tree and calls the provided
 *  function for each node
 */
    
static void DEFUN(lang_for_each_statement_worker,(func,  s),
		  void (*func)() AND
		  lang_statement_union_type *s)
{
    for (; s != (lang_statement_union_type *)NULL ; s = s->next) {
	func(s);
	
	switch (s->header.type) {
    case lang_output_section_statement_enum:
	    lang_for_each_statement_worker (func, 
					    s->output_section_statement.children.head);
	    break;
    case lang_wild_statement_enum:
	    lang_for_each_statement_worker (func, 
					    s->wild_statement.children.head);
	    break;
    case lang_data_statement_enum:
    case lang_object_symbols_statement_enum:
    case lang_output_statement_enum:
    case lang_target_statement_enum:
    case lang_input_section_enum:
    case lang_input_statement_enum:
    case lang_fill_statement_enum:
    case lang_assignment_statement_enum:
    case lang_padding_statement_enum:
    case lang_address_statement_enum:
	    break;
    default:
	    FAIL();
	    break;
	}
    }
}

void DEFUN(lang_for_each_statement,(func),
	   void (*func)())
{
    lang_for_each_statement_worker(func, statement_list.head);
}

static void DEFUN(lang_list_init,(list),
		  lang_statement_list_type *list)
{
    list->head = (lang_statement_union_type *)NULL;
    list->tail = &list->head;
}


/*----------------------------------------------------------------------
 *
 *  build a new statement node for the parse tree
 */

static lang_statement_union_type* DEFUN(new_statement,(type, size, list),
					enum statement_enum type AND
					bfd_size_type size AND
					lang_statement_list_type *list)
{
    lang_statement_union_type *new =
	    (lang_statement_union_type *) ldmalloc(size);
    new->header.type = type;
    new->header.next = (lang_statement_union_type *)NULL;
    lang_statement_append(list, new, &new->header.next);
    return new;
}

/*
 * Build a new input file node for the language. There are several ways
 * in which we treat an input file, eg, we only look at symbols, or
 * prefix it with a -l etc.
 *
 * We can be supplied with requests for input files more than once;
 * they may, for example be split over serveral lines like foo.o(.text)
 * foo.o(.data) etc, so when asked for a file we check that we havn't
 * got it already so we don't duplicate the bfd.
 */
static lang_input_statement_type * DEFUN(new_afile, (name, file_type, target, abfd, dev_no, ino),
					 CONST char *CONST name AND
					 CONST lang_input_file_enum_type file_type AND
					 CONST char *CONST target  AND
					 bfd *abfd                 AND
					 unsigned long dev_no      AND
					 unsigned long ino)
{
    lang_input_statement_type *p = new_stat(lang_input_statement,stat_ptr);
    lang_has_input_file = true;
    p->target = target;
    p->the_bfd = (bfd *)0;
    p->dev_no = dev_no;
    p->ino = ino;
    p->prepend_lib = p->add_suffixes = p->magic_syslib_and_startup = false;
    p->file_extension = "";
    switch (file_type) {
 case  lang_input_file_is_symbols_only_enum:
	p->filename = name;
	p->real = true;
	p->local_sym_name= name;
	p->just_syms_flag = true;
	p->search_dirs_flag = false;
	break;
 case lang_input_file_is_fake_enum:
	p->filename = name;
	p->real = false;
	p->local_sym_name= name;
	p->just_syms_flag = false;
	p->search_dirs_flag =false;
	break;
 case lang_input_file_is_l_enum:
 case lang_input_file_is_l_1_enum:
	p->prepend_lib = true;
	p->add_suffixes = true;
	p->file_extension = ".a";
	p->filename = name;
	p->real = true;
	if (file_type == lang_input_file_is_l_enum)
		p->local_sym_name = concat("-l",name,"");
	else
		p->local_sym_name = name;  /* Clues us in that this is a 'l_1' enum. */
	p->just_syms_flag = false;
	p->search_dirs_flag = true;
	break;
 case lang_input_file_is_magic_syslib_and_startup_enum:
	p->magic_syslib_and_startup = true;
	/* Intentional fall through. */
 case lang_input_file_is_search_file_add_suffixes_enum:
	p->add_suffixes = true;
	p->file_extension = ".a";
	/* Intentional fall through. */
 case lang_input_file_is_search_file_enum:
	/* Intentional fall through. */
 case lang_input_file_is_marker_enum:
	p->filename = name;
	p->real = true;
	p->local_sym_name= name;
	p->just_syms_flag = false;
	p->search_dirs_flag =true;
	break;
 case lang_input_file_is_file_enum:
	p->the_bfd = abfd;
	p->filename = name;
	p->real = true;
	p->local_sym_name= name;
	p->just_syms_flag = false;
	p->search_dirs_flag =false;
	break;
 default:
	FAIL();
    }
    p->asymbols = (asymbol **)NULL;
    p->superfile = (lang_input_statement_type *)NULL;
    p->subfiles = (lang_input_statement_type *)NULL;
    p->next_real_file = (lang_statement_union_type*)NULL;
    p->next = (lang_statement_union_type*)NULL;
    p->symbol_count = 0;
    p->common_section = (asection *)NULL;
    p->common_output_section = (asection *)NULL;
    lang_statement_append(&input_file_chain, (lang_statement_union_type *)p,
			  &p->next_real_file);
    return p;
}

lang_input_statement_type *DEFUN(lang_add_input_file,(name, file_type, target, abfd, dev_no, ino),
				 char *name AND
				 lang_input_file_enum_type file_type AND
				 char *target                        AND
				 bfd *abfd                           AND
				 unsigned long dev_no                AND
				 unsigned long ino)
{
    /* Look it up or build a new one */
    lang_has_input_file = true;
    return  new_afile(name,file_type,target,abfd,dev_no,ino);
}


/* Build enough state so that the parser can build its tree */
void DEFUN_VOID(lang_init)
{
    stat_ptr= &statement_list;
    lang_list_init(stat_ptr);
    
    lang_list_init(&input_file_chain);
    lang_list_init(&lang_output_section_statement);
    lang_list_init(&file_chain);
    first_file = lang_add_input_file((char *)NULL, 
				     lang_input_file_is_marker_enum, (char*)NULL,(bfd *)0,-1,-1);
}

/*----------------------------------------------------------------------
 * A region is an area of memory declared with the 
 * MEMORY {  name:org=exp, len=exp ... } 
 * syntax. 
 * 
 * We maintain a list of all the regions here
 * 
 * If no regions are specified in the script, then the default is used
 * which is created when looked up to be the entire data space
 */

       lang_memory_region_type *lang_memory_region_list;
static lang_memory_region_type **lang_memory_region_list_tail =
	&lang_memory_region_list;
static int number_of_memory_regions;

/* This routine tells whether there already exists a memory region by the
   given name. */
static lang_memory_region_type *DEFUN(lang_memory_region_lookup,(name),
				      CONST char *CONST name)
{
    lang_memory_region_type *p;
    
    for (p = lang_memory_region_list; p; p = p->down)
	    if (strcmp(p->name, name) == 0)
		    return p;
    return ((lang_memory_region_type *)NULL);
}

/* This routine is called when we know it is OK to make a new memory region:
   when we are processing a MEMORY directive or when we have figured
   out that we aren't going to get one and have to create the default. */
lang_memory_region_type *DEFUN(lang_memory_region_create,(name),
			       CONST char *CONST name)
{
    lang_memory_region_type *new;
    
    if (!(new = lang_memory_region_lookup(name))){
	/* Nobody has asked to have this region created before, so
	   we will create it with values showing that it is new. */
	new = (lang_memory_region_type *)
		ldmalloc((bfd_size_type)(sizeof(lang_memory_region_type)));
	new->name = buystring(name);
	new->up = (*lang_memory_region_list_tail);
	new->up = new->down = (lang_memory_region_type *)NULL;
	
	*lang_memory_region_list_tail = new;
	lang_memory_region_list_tail = &new->down;
	/* The following values let us know that the origin, current,
	   length, and flags fields have not yet been set. */
	new->origin = 0xffffffff;
	new->length_lower_32_bits = 0;
	new->length_high_bit = 0;
	new->flags = MEM_READ | MEM_WRITE | MEM_EXEC | MEM_LOAD;
	number_of_memory_regions++;
    }
    return new;
}

static int cmp_mrs(left,right)
    lang_memory_region_type **left,**right;
{
    if ((*left)->origin > (*right)->origin)
	    return 1;
    else if ((*left)->origin == (*right)->origin)
	    return 0;
    else
	    return -1;
}

/* Adds two numbers together storing result in *sum.  Returns non-zero if the sum overflowed
   else 0. */
static int check_for_overflow(addend1,addend2,sum)
    unsigned long addend1,addend2,*sum;
{
#define HIGH_BIT     0x80000000
    
    *sum = addend1 + addend2;
    return ((addend1 & addend2 & HIGH_BIT) || /* Unsigned long overflow occurs when both addends have
						 their high bit set,  or .... */
	    (((addend1|addend2) & HIGH_BIT) &&  /* Either of the addend's high bit is set AND ... */
	     !((*sum) & HIGH_BIT)));            /* The resulting sum does not have the high bit set. */
}

/* Returns the last address of a memory region. */
static unsigned long last_address_of(p)
    lang_memory_region_type *p;
{
    if (p->length_high_bit)        /* This assumes that p->length_lower_32_bits is zero,
				      which is fine. */
	    return 0xffffffff;
    else if (p->length_lower_32_bits)
	    return p->origin + (p->length_lower_32_bits-1);
    else
	    /* This is a degenerate memory region.  This should never happen */
	    return p->origin;
}

/*
  Coputes the difference:
  length - s
*/
static void subtract_length(p,l)
    lang_memory_region_type *p;
    unsigned long l;
{
    if (p->length_high_bit) {    /* Again, assumes the lower 32 bits of the length is zero.
				    which again is ok. */
	p->length_high_bit = 0;
	p->length_lower_32_bits = ((unsigned long) 0xffffffff) - l;
    }
    else
	    p->length_lower_32_bits -= l;
}

/*
  Make sure that the user specified a kosher arrangement of memory.
  No over laps occur.
  Also, sort the list into ascending order.
  Lastly, check for integer overlow in origin + length arrangements.
  */
static void lang_check_memory_regions()
{
    int i;
    unsigned int prev_address,u;
    lang_memory_region_type *p,*p2;

    if (number_of_memory_regions > 1) {  /* Then we will need to sort it. */
	lang_memory_region_type **q = (lang_memory_region_type **)
		ldmalloc(sizeof(lang_memory_region_type *)*number_of_memory_regions);

	for (p=lang_memory_region_list,i=0;p;i++) {
	    q[i] = p;
	    p = p->down;
	    q[i]->up = q[i]->down = (lang_memory_region_type *) 0;
	}
	qsort(q,number_of_memory_regions,sizeof(lang_memory_region_type *),cmp_mrs);
	for (p2=(lang_memory_region_type *)0,lang_memory_region_list=p=q[0],i=0;
	     i < number_of_memory_regions;
	     p=q[++i]) {
	    if (p->up = p2)
		    p2->down = p;
	    p2 = p;
	}
	free(q);
    }

    /* Now check for overflow and also memory region overlaps. */
    for (p2=(lang_memory_region_type *)0,p=lang_memory_region_list; p;p=p->down) {
	unsigned long sum;

	if (check_for_overflow(p->origin,p->length_lower_32_bits ? (p->length_lower_32_bits-1) : 0,&sum))
		info("%P:%FMemory region: %s o=%x,l=%x overflows 32 bit address space.\n",
		     p->name,p->origin,p->length_lower_32_bits);
	if (p2) {
	    if (p->origin <= prev_address)
		    info("%FMemory regions: %s and %s overlap.\n",p2->name,p->name);
	}
	p2 = p;
	prev_address = sum;
    }
}

int
mark_memory_region_used(origin,length)
    unsigned origin,length;
{
    lang_memory_region_type *p = lang_memory_region_list;
    unsigned ending_address;

    if (check_for_overflow(origin,length ? length-1 : 0,&ending_address)) {
 bug_out:
	/* This is a warning here because this code is called in ld960sym.c to
	   track sram usage.  And we do not require the user to specify sram regions
	   since if they are using it, they certainly know what they are doing and
	   we do not wish to make them jump through a hoop. */
	info("Warning: Can not find memory available at origin: %x length: %d\n",origin,length);
	return 0;
    }

    /* Find a memory region that can accommadate length bytes starting at origin: */
    while (p) {
	unsigned long end_of_memory_region = last_address_of(p);

	if (origin >= p->origin && origin <= end_of_memory_region &&
	    ending_address <= end_of_memory_region)
		break;
	p = p->down;
    }

    if (!p)
	    goto bug_out;

    /* When we place a section into memory, exactly one of four things can occur:
       
       1.  The section will take up all of the memory space in p.
       2.  The section will take up all of the first memory space in p,
       leaving some address space unused at the end, but none unused
       at the beginning.
       3.  The section will take up some of the memory space in p
       leaving some memory space unused at the beginning and the end.
       4.  The section will take up all of the last memory space in p,
       leaving some unused at the beginning, but none unused at the end.
       */
    if (p->length_lower_32_bits == length) {
	/* Must be case 1. */
	if (p->up)
		p->up->down = p->down;
	else
		lang_memory_region_list = p->down;
	if (p->down)
		p->down->up = p->up;
	free (p);
    }
    else if (p->origin == origin) {
	/* Must be case 2. */
	p->origin = origin + length;
	subtract_length(p, length);
    }
    else if (last_address_of(p) != ending_address) {
	
	/* Must be case 3.  This is a mutha.
	   
	   Here, we insert a node, q, into memory region list after p breaking it
	   up to account for the the new section's memory usage. */

	lang_memory_region_type *q = (lang_memory_region_type *)
		ldmalloc(sizeof(lang_memory_region_type));
	unsigned long old_length_lower = p->length_lower_32_bits;

	/* Set up the links: */
	
	q->up = p;               /* one */
	if (q->down = p->down)   /* two */
		p->down->up = q; /* three */
	p->down = q;             /* four. */

	/* Keep track of the name and attributes. */
	q->name = p->name;
	q->flags = p->flags;

	/* p's new length will be as indicated, its origin will not change: */
	    
	p->length_high_bit = 0;
	p->length_lower_32_bits = (origin - p->origin);

	/* q's origin is after that of the section we are inserting: */
	q->origin = (origin + length);

	/* Its length is the indicated difference: */
	q->length_high_bit = 0;
	q->length_lower_32_bits = old_length_lower - (length + p->length_lower_32_bits);
    }
    else
	    /* Must be case 4. */
	    subtract_length(p, length);
    return 1;
}


static bfd_vma
	DEFUN(insert_pad,(this_ptr, fill, power, output_section_statement, dot),
	      lang_statement_union_type **this_ptr AND
	      fill_type fill AND
	      unsigned int power AND
	      asection * output_section_statement AND
	      bfd_vma dot)
{
    /* Align this section first to the input sections requirement, then
     * to the output section's requirement. If this alignment is > than
     * any seen before, then record it too. Perform the alignment by
     * inserting a magic 'padding' statement.
     */
    
    unsigned int alignment_needed;
    if (power) {
	alignment_needed =  align_power(dot, power) - dot;
    }
    else if (!BFD_ELF_FILE_P(output_bfd)) {
	/* Align it to a word boundary in the absence of user
	   instructions to the contrary.  Also, we assign power here to
	   2 to possibly propogate the information to later links. */
        alignment_needed =  align_power(dot, power=2) - dot;
    }
    else {
	/* NOTE: don't assume any default alignment for ELF files; 
	   we need to link together input sections without padding. */
	alignment_needed = 0;
    }
    
    if (alignment_needed != 0) {
	lang_statement_union_type *new = (lang_statement_union_type *)
		ldmalloc((bfd_size_type)(sizeof(lang_padding_statement_type)));
	/* Link into existing chain */
	new->header.next = *this_ptr;
	*this_ptr = new;
	new->header.type = lang_padding_statement_enum;
	new->padding_statement.output_section =
		output_section_statement;
	new->padding_statement.output_offset = dot
		- output_section_statement->vma;
	new->padding_statement.fill = fill;
	new->padding_statement.size = alignment_needed;
    }
    
    
    /* Remember the most restrictive alignment */
    if (power > output_section_statement->alignment_power) {
	output_section_statement->alignment_power = power;
    }
    output_section_statement->size += alignment_needed;
    return alignment_needed + dot;
}

extern boolean option_v;

#ifdef PROCEDURE_PLACEMENT
#include "ldplace.h"
#endif

PROTO(static void,wild_doit,(lang_statement_list_type *,
			     asection *,
			     lang_output_section_statement_type *,
			     lang_input_statement_type *,
			     int));

#ifdef PROCEDURE_PLACEMENT

static lang_output_section_statement_type *procedure_placement_code_output_section;

/* DONT FORGET TO GET RID OF THIS BOOGER: ??? */
static void
print_out_orphaned_sections()
{
    lang_input_statement_type *f;

    for (f = (lang_input_statement_type *)file_chain.head; f;
	 f = (lang_input_statement_type *)f->next) {
	asection *s;

	for (s = f->the_bfd->sections; s; s = s->next)
		/* If the output section is null, then it has not yet been allocated to
		   an output section.  It must be a procedure placement section at this
		   point.  And therefore orphaned. */
		if (!s->output_section)
			info("ORPHANED SECTION: %B(%s)\n",f->the_bfd,s->name);
    }
    fflush(stderr);
}
#endif

PROTO(void,init_os,(lang_output_section_statement_type *));

static void DEFUN (place_section_into_memory,(this_section),
		   lang_output_section_statement_type *this_section)
{
    /* There will be no affect from zero length sections on the
       memory regions list. */

#ifdef PROCEDURE_PLACEMENT
    if (form_call_graph && this_section == procedure_placement_code_output_section) {
	lang_output_section_statement_type *os = procedure_placement_code_output_section;
	int number_of_lines = 0,number_of_relocs = 0;
	int nf_val;
	unsigned long addr = this_section->bfd_section->vma + this_section->bfd_section->size;
	lang_input_section_type section_file;

	while (nf_val=next_function(addr,&section_file)) {
	    if (nf_val == -1) {   /* Put the section_file into the output section at end of output
				     section, os. */
		if (output_flavor == BFD_COFF_FORMAT) {
		    if (((number_of_lines + section_file.section->lineno_count) > 0xffff) ||
			(config.relocateable_output &&
			 ((number_of_relocs + section_file.section->reloc_count) > 0xffff))) {
			lang_output_section_statement_type *new_os = 
				lang_output_section_statement_lookup(find_new_section_name(os->name),0);
			init_os(new_os);
			os->next_in_group = new_os;
			new_os->next_in_group = 
				(lang_output_section_statement_type *) 0xffffffff;
			new_os->next = os->next;
			os->next = (lang_statement_union_type *) new_os;
			new_os->section_was_split = os->section_was_split = true;
			new_os->bfd_section->vma = new_os->bfd_section->pma =
				os->bfd_section->vma + os->bfd_section->size;
			new_os->bfd_section->flags = os->bfd_section->flags;
			number_of_lines = number_of_relocs = 0;
			os = new_os;
		    }
		}
		number_of_relocs += section_file.section->reloc_count;
		number_of_lines += section_file.section->lineno_count;
		wild_doit(&os->children,section_file.section,os,section_file.ifile,1);
		section_file.section->output_offset = addr - os->bfd_section->vma;
		addr += section_file.section->size;
		os->bfd_section->size += section_file.section->size;
	    }
	    else if (nf_val > 0) {  /* Add some padding of size nf_val. */
		lang_statement_union_type *new = (lang_statement_union_type *)
			ldmalloc((bfd_size_type)(sizeof(lang_padding_statement_type)));
		/* Link into existing chain */
		*(os->children.tail) = new;
		os->children.tail = &(new->header.next);
 		new->header.next = 0;

		new->header.type = lang_padding_statement_enum;
		new->padding_statement.output_section =	os->bfd_section;
		new->padding_statement.output_offset = addr - os->bfd_section->vma;
		new->padding_statement.fill = os->fill;
		new->padding_statement.size = nf_val;
		addr += nf_val;
		os->bfd_section->size += nf_val;
	    }
	    else
		    abort();
	}

	/* DON'T FORGET TO REMOVE THIS BOOGER: ??? */
	print_out_orphaned_sections();
    }
#endif

    if (option_v)
	    printf("Allocating output section: %s to address: 0x%x size: %d\n",
		   this_section->bfd_section->name,
		 this_section->bfd_section->vma,this_section->bfd_section->size);
    
    if (!suppress_all_warnings && (this_section->bfd_section->vma & 3))
	    info("Warning: placing output section: %s at non-word-aligned address: %x\n",
		 this_section->bfd_section->name,this_section->bfd_section->vma);

    if (!this_section->bfd_section->size ||

	/* COPY and DSECT sections do not take up any memory space: */
	(this_section->bfd_section->flags & (SEC_IS_COPY | SEC_IS_DSECT)))

	    return;

    mark_memory_region_used(this_section->bfd_section->vma,this_section->bfd_section->size);
}

static int convert_to_flags(region_attributes)
    char *region_attributes;
{
    int flags_value = 0;
    
    /* We're looking for a memory region with specific
       attributes rather than one with a specific name. */
    region_attributes++;		/* skip past the '(' */
    while (*region_attributes != ')') {
	switch (*region_attributes) {
    case 'R':
	    flags_value |= MEM_READ;
	    break;
    case 'W':
	    flags_value |= MEM_WRITE;
	    break;
    case 'X':
	    flags_value |= MEM_EXEC;
	    break;
    case 'L':
    case 'I':
	    flags_value |= MEM_LOAD;
	    break;
    default:
	{
	    char bad_att[2];
	    
	    bad_att[0] = *region_attributes;
	    bad_att[1] = '\0';
	    info("%FBad memory region attribute %s\n",&bad_att[0]);
	}
	    break;
	}
	region_attributes++;
    }
    return flags_value;
}

static int count_bits(l)
    unsigned l;
{
    int bit_count = 0;

    for (;l;l >>= 1)
	    bit_count += (l & 1);
    return bit_count;
}

static bfd_vma next_address(region_name,region_attributes,
			    alignment_value,size,copy_or_dsect,ok,sect_name)
    char *region_name,*region_attributes;
    unsigned int alignment_value,size,copy_or_dsect;
    int *ok;
    char *sect_name;
{
    lang_memory_region_type *p;
    int lookfor_flags;

    if (region_attributes)
	    lookfor_flags = convert_to_flags(region_attributes);
    *ok = 0;
    if (count_bits(alignment_value) > 1)
	    info("Warning: output section: %s, aligned with value: %x is not a power of two.\n",
		 sect_name,alignment_value);

    for (p=lang_memory_region_list;p;p = p->down) {
	int this_one_is_ok = 0;
	
	if (region_attributes)
		this_one_is_ok = p->flags == lookfor_flags;
	else
		this_one_is_ok = (!region_name || !strcmp(p->name,region_name));
	
	if (this_one_is_ok) {
	    unsigned int u = ALIGN(p->origin,alignment_value);
	    unsigned long ending_address,end_of_memory = last_address_of(p);

	    if (!check_for_overflow(u,size ? size-1 : 0,&ending_address)) {
		if (u >= p->origin && ending_address <= end_of_memory) {
		    *ok = 1;
		    return u;
		}
	    }
	}
    }
    return 0;
}

static unsigned long size_group(s)
    lang_output_section_statement_type *s;
{
    unsigned long size = 0;
    
    while (s != (lang_output_section_statement_type *) 0xffffffff) {
	size = align_power(size,s->bfd_section->alignment_power);
	size += s->bfd_section->size;
	s = s->next_in_group;
    }
    return size;
}

int address_available(address,size,copy_or_dsect,mr_list)
    unsigned int address,size,copy_or_dsect;
    lang_memory_region_type *mr_list;
{
    lang_memory_region_type *p = mr_list;
    
    while (p) {
	unsigned long sum,end_of_memory_region = last_address_of(p);

	if (!check_for_overflow(address,size ? size-1 : 0,&sum)) {
	    if (address >= p->origin && sum <= end_of_memory_region)
		    return 1;
	}
	p = p->down;
    }
    return size == 0;
}

static int log_base_2(n)
    unsigned int n;
{
    int p = -1;

    for (;n;n = n >> 1)
	    p++;
    return p;
}


static int procedure_placement_manually_determine_alignment_reqs;

static int
ldlang_procedure_placement_determine_alignment_reqs(current_alignment)
    int current_alignment;
{
    lang_input_statement_type *f;
    int max_alignment = current_alignment;

    for (f = (lang_input_statement_type *)file_chain.head; f;
	 f = (lang_input_statement_type *)f->next) {
	asection *s;

	for (s = f->the_bfd->sections; s; s = s->next)
		if (!s->output_section) {
		    /* If the output section is null, then it has not yet been allocated to
		   an output section.  It must be a procedure placement section at this
		   point . */
		    if (s->alignment_power > max_alignment)
			    max_alignment = s->alignment_power;
		    if (s->size > largest_section)
			    largest_section = s->size;
		}
    }
    return max_alignment;
}

/*
  There are exactly 7 ways to allocate an output section:
  
  1.  By binding address.
  2.  By ALIGNment (first fit is used)
  3.  By region (first fit is used).
  4.  By attribute (first fit is used).
  5.  By ALIGNment and region (first fit is used).
  6.  By ALIGNment and attribute (first fit is used).
  7.  By not specifying any value (first fit is used (BUT THESE ARE ALLOCATED LAST)).
  
  GROUPs affect this menagerie also.
  */

static void DEFUN_VOID(lang_allocate)
{
    lang_statement_union_type *u;
    unsigned long size_of_group;
    bfd_vma binding_address,dot = 0,dot_ok;
    etree_value_type result;
    lang_output_section_statement_type *v;
    int second_time_through,allocated;

#define its_a_group (v->next_in_group)
#define copy_or_dsect(X) ((X)->bfd_section->flags & (SEC_IS_COPY | SEC_IS_DSECT))
#define total_size(X)  ((0 == copy_or_dsect(X)) ? ((its_a_group) ? size_of_group : v->bfd_section->size) : 0)
    
    for (second_time_through=0;second_time_through < 2;second_time_through++) {
	dot = next_address(NULL,NULL,1,0,0,&dot_ok,"");
	for (u = lang_output_section_statement.head; u; u = v->next) {
	
	    allocated = 0;
	    v = &u->output_section_statement;
	    if (v->section_was_allocated) {
		dot = v->bfd_section->vma + v->bfd_section->size;
		continue;
	    }

	    if (v->flags & LDLANG_HAS_INPUT_FILES) {
		if ((v->bfd_section->flags & SEC_ALLOC) == 0) {
		    v->bfd_section->pma = v->bfd_section->vma = 0;
		    continue;
		}
	    }

	    v->flags |= LDLANG_ALLOC;
	    v->bfd_section->flags |= SEC_ALLOC;

#ifdef PROCEDURE_PLACEMENT
	    if (form_call_graph && v == procedure_placement_code_output_section &&
		!procedure_placement_manually_determine_alignment_reqs) {
		v->bfd_section->alignment_power =
			procedure_placement_manually_determine_alignment_reqs =
				ldlang_procedure_placement_determine_alignment_reqs(
								    v->bfd_section->alignment_power
										    );
	    }
#endif

	    if (its_a_group)
		    size_of_group = size_group(v);
	
	    if (v->addr_tree && v->addr_tree->type.node_code != ALIGN_K) {
		/* This is case 1 above, and it is easy: */
		exp_fold_tree(v->addr_tree,v,
			      lang_allocating_phase_enum,dot,
			      (bfd_vma*)0,&result);
		if (!result.valid)
			info("%FUnable to evaluate start address for output section: %s\n",
			     v->bfd_section->name);
		binding_address = result.value;
		dot_ok = address_available(binding_address,total_size(v),copy_or_dsect(v),
					   lang_memory_region_list);
		allocated = 1;
	    }
	    else if (v->region_name || v->region_attributes) {
		/* This is cases: 3, 4, 5, and 6 from above: */
		if (v->addr_tree && v->addr_tree->type.node_code == ALIGN_K) {
		    /* This is cases: 5 and 6. */
		    dot = next_address((v->region_name) ? v->region_name : NULL,
				       v->region_attributes,
				       1,0,copy_or_dsect(v),&dot_ok,v->bfd_section->name);
		    exp_fold_tree(v->addr_tree->unary.child,v,
				  lang_allocating_phase_enum,dot,
				  (bfd_vma*)0,&result);
		    if (!result.valid)
			    info("%FUnable to evaluate ALIGN() expression for output section: %s\n",
				 v->bfd_section->name);
		    /* The value in result.value corresponds to an alignment value.
		       Disregard input alignments and make the output section aligned on the
		       specific value. */

#define ASSIGN_BIGGER(x,y,z) ((x) = ((y) > (z)) ? (y) : (z))

		    ASSIGN_BIGGER(v->bfd_section->alignment_power,log_base_2(result.value),
				  v->bfd_section->alignment_power);

		    binding_address = next_address((v->region_name) ? v->region_name : NULL,
						   v->region_attributes,
						   1 << v->bfd_section->alignment_power,
						   total_size(v),
						   copy_or_dsect(v),
						   &dot_ok,
						   v->bfd_section->name);
		}
		else
			/* This is cases: 3 and 4. */
			binding_address = next_address(v->region_name ? v->region_name : NULL,
						       v->region_attributes,
						       1<<v->bfd_section->alignment_power,
						       total_size(v),copy_or_dsect(v),&dot_ok,
						       v->bfd_section->name);
		allocated = 1;
	    }
	    else if (v->addr_tree && v->addr_tree->type.node_code == ALIGN_K) {
		/* This is case 2 above: */
		exp_fold_tree(v->addr_tree->unary.child,v,
			      lang_allocating_phase_enum,dot,
			      (bfd_vma*)0,&result);
		if (!result.valid)
			info("%FUnable to evaluate ALIGN() expression for output section: %s\n",
			     v->bfd_section->name);

		ASSIGN_BIGGER(v->bfd_section->alignment_power,log_base_2(result.value),
				  v->bfd_section->alignment_power);

		binding_address = next_address(NULL,NULL,1 << v->bfd_section->alignment_power,
					       total_size(v),copy_or_dsect(v),
					       &dot_ok,v->bfd_section->name);
		allocated = 1;
	    }
	    else if (second_time_through) {
		/* This is case 7 from above. */
		/* Note that we do these sections last since the user really does not know where he
		   wants to put these, and nor does he appear to care.  */
		if (manual_allocation)
			binding_address = next_address(NULL,NULL,1<<v->bfd_section->alignment_power,
						       total_size(v),copy_or_dsect(v),&dot_ok,
						       v->bfd_section->name);
		else {
		    if (!(dot_ok=
			  address_available(binding_address=align_power(dot,
									v->bfd_section->alignment_power),
					    total_size(v),copy_or_dsect(v),lang_memory_region_list)))
			    binding_address = next_address(NULL,NULL,1<<v->bfd_section->alignment_power,
							   total_size(v),copy_or_dsect(v),&dot_ok,
							   v->bfd_section->name);
		}			
		allocated = 1;
	    }
	    if (allocated) {
		if (!dot_ok)
			info("%FCan not allocate output section: \"%s\" size: %d alignment: %d into configured memory\n",
			     v->bfd_section->name,v->bfd_section->size,1<<v->bfd_section->alignment_power);
		v->bfd_section->vma = v->bfd_section->pma = binding_address;
		place_section_into_memory(v);
		v->section_was_allocated = true;
		dot = binding_address + (copy_or_dsect(v) ? 0 : (v->bfd_section->size));
		if (v->next_in_group) {
		    lang_output_section_statement_type *g;
	    
		    for (g=v->next_in_group;g && g != (lang_output_section_statement_type *) 0xffffffff;
			 g = g->next_in_group) {
			dot = binding_address = align_power(dot,g->bfd_section->alignment_power);
			g->bfd_section->vma = g->bfd_section->pma = binding_address;
			place_section_into_memory(g);
			g->section_was_allocated = true;
			dot = binding_address + (copy_or_dsect(g) ? 0 : g->bfd_section->size);
		    }
		}
	    }
	}
    }
}

lang_output_section_statement_type *DEFUN(lang_output_section_find,(name),
					  CONST char * CONST name)
{
    lang_statement_union_type *u;
    lang_output_section_statement_type *lookup;
    
    for (u = lang_output_section_statement.head; u; u = lookup->next) {
	lookup = &u->output_section_statement;
	if (strcmp(name, lookup->name)==0) {
	    return lookup;
	}
    }
    return (lang_output_section_statement_type *)NULL;
}

lang_output_section_statement_type
	*DEFUN(lang_output_section_statement_lookup,(name,add_to_os_list),
	      CONST char * CONST name  AND
	       int add_to_os_list)
{
    lang_output_section_statement_type *lookup;
    
    lookup =lang_output_section_find(name);
    if (lookup == (lang_output_section_statement_type *)NULL) {
	extern int default_fill_value;
	

	lookup = (lang_output_section_statement_type *)
			new_stat(lang_output_section_statement, stat_ptr);
	lookup->region_attributes = NULL;
	lookup->region_name = NULL;
	lookup->fill = default_fill_value;
	lookup->flags = 0;
	lookup->name = name;
	lookup->next = (lang_statement_union_type*)NULL;
	lookup->bfd_section = (asection *)NULL;
	lookup->section_was_split = lookup->section_was_allocated =
		lookup->processed = false;
	
	lookup->addr_tree = (etree_type *)NULL;
	lookup->next_in_group = (lang_output_section_statement_type*)NULL;
	lang_list_init(&lookup->children);

	if (add_to_os_list)
		lang_statement_append(&lang_output_section_statement,
				      (lang_statement_union_type *)lookup,
				      &lookup->next);
    }
    return lookup;
}

/*
  New lang_map code starts here.   The old stuff was written very strangely, 
  and it lacked the basic feature of sorted output, so I tossed it and
  replaced it with the following.  Paul Reger.  Thu Jan 21 09:39:51 PST 1993
  */

/* The information in the output file's map is stored in a binary tree. */

static struct lang_map_node {
    char *output_section,*input_section,*label;
    unsigned long virtual_address,size,align;
    struct lang_map_node *left,*right;
} *lang_map_root;

static int lang_map_add_node(p,osn,is,va,s,a,l)
    struct lang_map_node **p;
    char *osn,*is,*l;
    unsigned long va,s,a;
{
    struct lang_map_node *q = (*p);
    
    if (q == (struct lang_map_node *) 0) {
	q = (struct lang_map_node *) ldmalloc(sizeof(struct lang_map_node));
	(*p) = q;
	q->left = q->right = (struct lang_map_node *) 0;
	q->output_section = osn;
	q->input_section = is;
	q->virtual_address = va;
	q->size = s;
	q->align = a;
	q->label = l;
	return 1;
    }
    if (va < q->virtual_address)
	    return lang_map_add_node(&q->left,osn,is,va,s,a,l);
    else if (va > q->virtual_address)
	    return lang_map_add_node(&q->right,osn,is,va,s,a,l);

#define STREQU(X,Y) (((X) == (Y)) || ((X) && (Y) && !strcmp((X),(Y))))

    else if (STREQU(q->output_section,osn) && STREQU(q->input_section,is) &&
 	     STREQU(q->label,l))
 	    return 1;

#undef STREQU

    else if (osn != NULL)
	    return lang_map_add_node(&q->left,osn,is,va,s,a,l);
    else if (is != NULL && q->output_section == NULL)
	    return lang_map_add_node(&q->left,osn,is,va,s,a,l);
    else
	    return lang_map_add_node(&q->right,osn,is,va,s,a,l);
}

static unsigned long lang_map_print_dot;

static int lang_map_find_range(r1,r2,start,length)
    unsigned long r1,r2,*start,*length;
{
    lang_memory_region_type *m;
#define lang_map_BIGGER(x,y) (((x) > (y)) ? (x) : (y))
#define lang_map_SMALLER(x,y) (((x) < (y)) ? (x) : (y))
    
    for (m = lang_memory_region_list; m; m = m->down) {
	unsigned long sum = m->origin + m->length_lower_32_bits;
	
	if (sum < m->origin)
		sum = 0xffffffff;
	if ((r1 >= m->origin && r1 <= sum) ||
	    (r2 >= m->origin && r2 <= sum)) {
	    unsigned long end;
	    
	    *start = lang_map_BIGGER(r1,m->origin);
	    end    = lang_map_SMALLER(r2,sum);
	    *length = end - *start;
	    if (*length)
		    return 1;
	}
    }
    return 0;
}

static void lang_map_print_available(f,x)
    FILE *f;
    unsigned long x;
{
    if (x > lang_map_print_dot) {
	unsigned long start,length;
	
	if (lang_map_find_range(lang_map_print_dot,x,&start,&length))
		fprintf(f," *available*      0x%08x 0x%05x\n",start,length);
    }
}

static void lang_map_print_tree(f,p)
    FILE *f;
    struct lang_map_node *p;
{
#define lang_map_STRING(x) ((x) ? (x) : "")
    if (p) {
	if (p->left)
		lang_map_print_tree(f,p->left);
	if (p->output_section) {
	    lang_map_print_available(f,p->virtual_address);
	    lang_map_print_dot = p->virtual_address + p->size;
	}
	fprintf(f,"%-8s %-8s 0x%08x ",lang_map_STRING(p->output_section),
		lang_map_STRING(p->input_section),p->virtual_address);
	if (p->size != -1)
		fprintf(f,"0x%05x 0x%04x ",p->size,p->align);
	else
		fprintf(f,"               ");
	fprintf(f,"%s\n",lang_map_STRING(p->label));
	if (p->right)
		lang_map_print_tree(f,p->right);
	free(p);
    }
}

static void lang_map_fill_tree();

static void lang_map_add_os_nodes(output_section_statement)
lang_output_section_statement_type *output_section_statement;
{
    asection *section = output_section_statement->bfd_section;
    char *label = NULL;

    if (section->flags & (SEC_IS_COPY | SEC_IS_DSECT)) {
	/* Neither COPY nor DSECT sections will appear in the map */
	return;
    }
    if (section->flags & SEC_IS_NOLOAD)
	    label = "NOLOAD";
    else if ((section->flags & SEC_IS_BSS) &&
	     (!(section->flags & (SEC_CODE | SEC_DATA))))
	    label = "Uninitialized";
    lang_map_add_node(&lang_map_root,output_section_statement->name,NULL,section->vma,
		      section->size,1 << section->alignment_power,label);
    lang_map_fill_tree(output_section_statement->children.head);
}

static void lang_map_add_wild_nodes(w)
    lang_wild_statement_type *w;
{
    lang_map_fill_tree(w->children.head);
}

static void lang_map_add_padding_node(s)
    lang_padding_statement_type *s;
{
    lang_map_add_node(&lang_map_root,NULL,"*fill*",
		      s->output_offset + s->output_section->vma,
		      s->size,s->fill,NULL);
}

static void
	lang_map_add_symbol_node(q)
asymbol *q;
{
    lang_map_add_node(&lang_map_root,NULL,
		      NULL,outside_symbol_address(q),
		      -1,-1,q->name);
}

static void lang_map_add_is_nodes(in)
    lang_input_section_type *in;
{
    asection *i = in->section;
    char *os = NULL;
    char *is = (char *)i->name;
    char *label = NULL;
    unsigned long va = 0,size = i->size;
    unsigned long align = 1 << i->alignment_power;
    
    if (i->size == 0) {
	return;
    }
    
    if (i->output_section) {
	va = i->output_section->vma + i->output_offset;
	if (in->ifile) {
	    bfd *abfd = in->ifile->the_bfd;
	    if (in->ifile->just_syms_flag == true) {
		return;
	    }
	    if(abfd->my_archive != (bfd *)NULL) {
		static char buff[1024];
		
		sprintf(buff,"[%s]%s", abfd->my_archive->filename,
			abfd->filename);
		label = buystring(buff);
	    } else {
		label = (char *)abfd->filename;
	    }
	    lang_map_add_node(&lang_map_root,os,is,va,size,align,label);
	}
    }
}

static void lang_map_fill_tree(s)
    lang_statement_union_type *s;
{
    for ( ; s; s = s->next)
	    switch (s->header.type) {
	case lang_output_section_statement_enum:
		lang_map_add_os_nodes(&s->output_section_statement);
		break;
	case lang_input_section_enum:
		lang_map_add_is_nodes(&s->input_section);
		break;
	case lang_padding_statement_enum:
		lang_map_add_padding_node(&s->fill_statement);
		break;
	case lang_wild_statement_enum:
		lang_map_add_wild_nodes(&s->wild_statement);
		break;
	    }
}

static void DEFUN(lang_map_print_flags, (outfile, flags),
		  FILE *outfile AND
		  int  flags)
{
    if (flags & MEM_READ)  fprintf(outfile,"R");
    if (flags & MEM_WRITE) fprintf(outfile,"W");
    if (flags & MEM_EXEC)  fprintf(outfile,"X");
    if (flags & MEM_LOAD)  fprintf(outfile,"L");
}

void DEFUN(lang_map_memory_regions,(outfile,message),
	   FILE *outfile AND char *message )
{
    lang_memory_region_type *m;
    
    fprintf(outfile,"\n**%s**\n\n",message);
    fprintf(outfile,"name\t\torigin\t\tlength\t\tattributes\n");
    for (m = lang_memory_region_list; m; m = m->down) {
	fprintf(outfile,"%-16s0x%08x\t0x%08x\t", m->name,m->origin,m->length_lower_32_bits);
	lang_map_print_flags(outfile, m->flags);
	fprintf(outfile,"\n");
    }
}

void DEFUN(lang_map_output_sections,(outfile),
	   FILE *outfile)
{
    fprintf(outfile,"\n\n**LINK EDITOR MEMORY MAP**\n\n");
    fprintf(outfile,"output   input    virtual            alignment/\n");
    fprintf(outfile,"section  section  address    size    value\n");
    lang_map_fill_tree(statement_list.head);

    if (1) {
 	if (output_bfd->outsymbols) {
 	    asymbol **p;
 
 	    for (p = output_bfd->outsymbols; *p; p++) {
 		asymbol *q = *p;
 
 		if (!(q->flags & (BSF_DEBUGGING|BSF_FORT_COMM|BSF_UNDEFINED|BSF_SCALL|
 				  BSF_TMP_REL_SYM|BSF_ZERO_SYM)))
 			lang_map_add_symbol_node(q);
 	    }
 	}
    }

    lang_map_print_tree(outfile,lang_map_root);
    lang_map_print_available(outfile,0xffffffff);
}

void DEFUN(init_os,(s),
		  lang_output_section_statement_type *s)
{
    section_userdata_type *new = (section_userdata_type *)
	    ldmalloc((bfd_size_type)(sizeof(section_userdata_type)));
    
    s->bfd_section = bfd_make_section(output_bfd, s->name);
    if (s->bfd_section == (asection *)NULL) {
	info("%Foutput format %s can't represent section '%s'\n",
	     output_bfd->xvec->name, s->name);
    }
    s->bfd_section->output_section = s->bfd_section;

#define MAP(BFDISM,GLDISM) if (s->flags & GLDISM) s->bfd_section->flags |= BFDISM

    MAP(SEC_IS_DSECT,     LDLANG_DSECT);
    MAP(SEC_IS_COPY,      LDLANG_COPY);
    MAP(SEC_IS_NOLOAD,    LDLANG_NOLOAD);
    MAP(SEC_ALLOC,        LDLANG_ALLOC);
    MAP(SEC_HAS_CONTENTS, LDLANG_HAS_CONTENTS);
    MAP(SEC_LOAD,         LDLANG_LOAD);
    MAP(SEC_IS_CCINFO,    LDLANG_CCINFO);

#undef MAP
    
    /* We initialize an output sections output offset to minus its own */
    /* vma to allow us to output a section through itself */
    s->bfd_section->output_offset = 0;
    get_userdata(s->bfd_section) = (PTR *)new;
}

static void
emit_conflict_warning(file,section)
    lang_input_statement_type *file;
    asection *section;
{
    info("%P: Warning conflict of target byte order for section %B(%s)\n",file->the_bfd,
	 section->name);
#define BO(Z) ((Z->flags & SEC_IS_BIG_ENDIAN) ? "big" : "little")
    info("is %s-endian, and the output section is %s-endian.\n",
	 BO(section),BO(section->output_section));
#undef BO
}

extern int target_byte_order_is_big_endian;

/***********************************************************************
 * The wild routines.
 *
 * These expand statements like *(.text) and foo.o to a list of
 * explicit actions, like foo.o(.text), bar.o(.text) and
 * foo.o(.text,.data) .
 *
 * The toplevel routine, wild, takes a statement, section, file and
 * target. If either the section or file is null it is taken to be the
 * wildcard. Seperate lang_input_section statements are created for
 * each part of the expansion, and placed after the statement provided.
 */

static void DEFUN(wild_doit,(ptr, section, output, file, hard_error_for_multiple_allocations),
		  lang_statement_list_type *ptr              AND
		  asection *section                          AND
		  lang_output_section_statement_type *output AND
		  lang_input_statement_type *file            AND
		  int hard_error_for_multiple_allocations)
{
    if(output->bfd_section == (asection *)NULL) {
	init_os(output);
    }
    
    if (section && section->output_section == (asection *)NULL) {
	/* Add a section reference to the list */
	lang_input_section_type *new =
		new_stat(lang_input_section, ptr);
	new->section = section;
	new->ifile = file;
	section->output_section = output->bfd_section;
	if (file != script_file && !(section->flags & SEC_IS_BSS)) {
	    if (BFD_ELF_FILE_P(output_bfd) || BFD_COFF_FILE_P(output_bfd)) {
		if (output->flags & LDLANG_HAS_INPUT_FILES) {
		    if (((section->flags & SEC_IS_BIG_ENDIAN) !=
			 (section->output_section->flags & SEC_IS_BIG_ENDIAN)))
			    emit_conflict_warning(file,section);
		    /* If the input is a cave section, and the output is not a cave section. */
		    if ((!strcmp("cave",section->name) && (0 == (output->flags & LDLANG_CAVE_SECTION))) ||
			    /* If the input is NOT a cave section, and the output section is a cave section. */
			    (strcmp("cave",section->name) && (LDLANG_CAVE_SECTION ==
							      (output->flags & LDLANG_CAVE_SECTION)))) {
			info("%P: inconsistent placement of cave sections.  ALL Cave sections must be placed.\n");
			info("%P: in one output section, and that section only contain cave input sections.\n");
			info("%F%P: input section: %s(%s) output section: %s.\n",file->filename,section->name,
			     output->name);
		    }
		}
		else if ((section->flags & (SEC_LOAD|SEC_ALLOC))) {
		    if ((target_byte_order_is_big_endian && !(section->flags & SEC_IS_BIG_ENDIAN)) ||
			((!target_byte_order_is_big_endian) && (section->flags & SEC_IS_BIG_ENDIAN)))
			    emit_conflict_warning(file,section);
		    /* If the input is a cave section, and the output is not a cave section. */
		    if (!strcmp("cave",section->name))
			    output->flags |= LDLANG_CAVE_SECTION;
		}
	    }
	}
	if (!bfd_set_section_flags(output_bfd,section->output_section,
			      section->output_section->flags | section->flags)) {
	    if (BFD_BOUT_FILE_P(output_bfd) && (section->flags & SEC_IS_BIG_ENDIAN))
		    info("%P: The BOUT omf does not support Big Endian sections.\n"); 
	    info("%F%P:Can't set section flags from %B(%s) (%s)\n",
		 file->the_bfd,section->name,bfd_errmsg(bfd_error));
	}
	section->output_section->insec_flags |= section->insec_flags;
	if (section->alignment_power > output->bfd_section->alignment_power) {
	    output->bfd_section->alignment_power =
		    section->alignment_power;
	}
	output->flags |= LDLANG_HAS_INPUT_FILES;
    }
    else if (section && hard_error_for_multiple_allocations)
	    info("%F attempt to bind: %s(%s) to multiple output sections.\n",file->filename,section->name);
}

static asection *DEFUN(our_bfd_get_section_by_name,(abfd, section),
		       bfd *abfd AND
		       CONST char *section)
{
    return bfd_get_section_by_name(abfd, section);
}

static int  /* Returns 1 on success, 0 on error. */
	DEFUN(wild_section,(ptr, section, file , output, suppress_errors, explicit_reference),
	      lang_wild_statement_type *ptr              AND
	      CONST char *section                        AND
	      lang_input_statement_type *file            AND
	      lang_output_section_statement_type *output AND
	      int suppress_errors                        AND
	      int explicit_reference)
{
    asection *s;

    if (gnu960_check_format(file->the_bfd, bfd_archive)) {
	for (file=file->subfiles;file;file = file->chain)
		wild_section(ptr,section,file,output,1,0);
	return 1;
    }

    if (file->just_syms_flag == false) {
	if (section == (char *)NULL) {      /* This MUST BE the case of:   foo.o */
	    /* Do the creation to all sections in the file */
	    for (s = file->the_bfd->sections; s; s = s->next)  {
		/* Do this for all sections BUT the profile counters section. */
		if (strcmp("PROFILE_COUNTER",s->name))
			wild_doit(&ptr->children, s, output, file, 1);
	    }
	} else {
	    /* Do the creation to the named section only
	       This is the case of:   foo.o(sect_name)   */
	    asection *temp = our_bfd_get_section_by_name(file->the_bfd, section);;

#ifdef PROCEDURE_PLACEMENT
	    if (!strcmp(section,".text")) {
		/* This is the case of:   foo.o(.text)   */
		int success_count = 0;

		if (form_call_graph && !explicit_reference) {
		    return 1;
		}
		else {  /* We either are not doing procedure placement OR we are doing procedure placement
			   and we have an explicit reference:
			   foo.o(.text)    */
		    if (temp) {
			wild_doit(&ptr->children, temp, output, file, explicit_reference);
			success_count++;
		    }
		    for (s = file->the_bfd->sections; s; s = s->next)
			    if (IS_NAMED_TEXT_SECTION( s->name )) {
				/* Should we emit a warning here? ??? */
				wild_doit(&ptr->children, s, output, file, explicit_reference);
				success_count++;
			    }
		    if (!success_count && !suppress_errors)
			    info("%P%F: File: %B has no section named `%s'.\n",file->the_bfd,section);
		}
	    }
	    else
#endif
		    if (temp)
			    wild_doit(&ptr->children, temp, output, file, explicit_reference);
		    else {
			if (!suppress_errors)
				info("%P%F: File: %B has no section named `%s'.\n",file->the_bfd,section);
			return 0;
		    }
	}
    }
    else {
	info("Warning: ignoring reference to -R (symbols only) file/section: %B(%s)\n",file->the_bfd,
	     section ? section : "*");
    }
    return 1;
}


/* passed a file name (which must have been seen already and added to
 * the statement tree. We will see if it has been opened already and
 * had its symbols read. If not then we'll read it.
 *
 * Archives are pecuilar here. We may open them once, but if they do
 * not define anything we need at the time, they won't have all their
 * symbols read. If we need them later, we'll have to redo it.
 */

/* See below note near call to stat() for remarks. */
#ifdef DOS
#include <stdlib.h>
#endif

static lang_input_statement_type *DEFUN(lookup_name,(name,dev_no,ino),
					CONST char ** CONST name AND
					unsigned long dev_no     AND
					unsigned long ino)
{
    /* We are trying to find out if the requested filename given by *name is
       the same as one that was previously opened (ie a member of the
       input_file_chain list).  We do this to disambiguate paths to files.
       For example, we reference a.o twice here:

       gld960 a.o ../dir/a.o

       BUT WE ONLY LINK a.o in ONLY once given the above command.

       We disambiguate the names using stat() on UNIX and _fullpath() on DOS.

       Strcmp() does not work here due to oddness of
       referencing filenames, ../dir/a.o might be the same way to reference a.o
       or ./a.o.

       Notice how we go out of our way to minimize the calls to stat().  Each call
       is very expensive.  */

    lang_input_statement_type *search;
#ifndef DOS
    int dev_no_and_ino_are_valid = 0;
#else
    char namepath[_MAX_PATH];
    int namepath_valid = 0;
#endif

    /* First get dev_no and ino (and namepath) to correspond to *name: */

#ifndef DOS

#define SET_NAME_INFO()/* No need to stat() it if it is set on     \
			   the way in: */                          \
                         if (dev_no != -1 && ino != -1)            \
                             dev_no_and_ino_are_valid = 1;         \
                         else if (name && *name) {                 \
			       struct stat namestat;               \
                                                                   \
                               if (stat(*name,&namestat) == 0) {   \
			          dev_no_and_ino_are_valid = 1;    \
				  dev_no = namestat.st_dev;        \
				  ino = namestat.st_ino;           \
			       }                                   \
			 }

#else

#define SET_NAME_INFO() if (name && *name) {                                 \
			   if (_fullpath(namepath,*name,_MAX_PATH) != NULL)  \
			      namepath_valid = 1;                            \
		        }

#endif

    SET_NAME_INFO();

    /* Remember:
       namepath and dev_no and ino may not be valid at this point because
       they might correspond to a library kernel name -lcg (*name might equal
       "cg"). */

    /* Next, we loop through the input file chain searching for an instance
       of dev_no, ino (namepath). */

    for(search = (lang_input_statement_type *)input_file_chain.head;
	search;
	search = (lang_input_statement_type *)search->next_real_file) {
	int previously_opened = 0;
#ifdef DOS
	char searchpath[_MAX_PATH];
	int searchpath_valid = 0;
#else
	int search_dev_no_and_ino_valid = 0;
#endif

	/* A test for the 'first_file'. */
	if (search->filename == (char *)NULL && (name == (CONST char **)NULL || *name == (CONST char *) NULL))
		return search;

	/* Go ahead and open the file if search_dirs_flag is true.  BUT set a flag indicating that
	   we already called ldmain_open_file_read_symbol() on this input file element. */
	if (search->search_dirs_flag) {
	    ldmain_open_file_read_symbol(search);
	    /* ldmain_open_file() modifies the filename element to the true filename
	       if search_dirs is true. */
	    /* Are we aliased in the input_file chain?  */
	    if (name == &(search->filename)) {
		/* If so, let's see if we can now stat/get the fullpath of *name: */
		SET_NAME_INFO();
#ifndef DOS
		/* If we can now stat the filename, then let's bug out since we are aliased
		   and therefore must be the same. */
		if (dev_no_and_ino_are_valid) {
		    /* before we bug out, let's save off the ino and dev_no info for
		       next time through: */
		    search->dev_no = dev_no;
		    search->ino = ino;
		    return search;
		}
#endif
		previously_opened = 1;
	    }
	}
#ifndef DOS
	/* If we have not stat'd search, then let's do so: */
	if (search->dev_no == -1 && search->ino == -1) {
	    struct stat searchstat;

	    if (search->filename && stat(search->filename,&searchstat) == 0) {
		search_dev_no_and_ino_valid = 1;
		search->dev_no = searchstat.st_dev;
		search->ino = searchstat.st_ino;
	    }
	}
	else
		search_dev_no_and_ino_valid = 1;

	/* Remember searchstat info may not be valid at this point, since for example
	   the "script file" does not get opened per se, and yet is in the input_file chain.
	   */
#else
	/* On DOS, we fill in search_path instead of stat'ing it. */
	if (search->filename && _fullpath(searchpath,search->filename,_MAX_PATH) != NULL)
		searchpath_valid = 1;

	/* Remember that searchpath info may not be valid here for same reasons as above. */
#endif
#ifndef DOS
	if (dev_no_and_ino_are_valid && search_dev_no_and_ino_valid &&
	    search->dev_no == dev_no && search->ino == ino) {
	    if (!previously_opened)
		    ldmain_open_file_read_symbol(search);
	    return search;
	}
#else
	if (namepath_valid && searchpath_valid && !strcmp(namepath,searchpath)) {
	    if (!previously_opened)
		    ldmain_open_file_read_symbol(search);
	    return search;
	}
#endif
    }
    
    /* There isn't an afile entry for this file yet, this must be 
     * because the name has only appeared inside a linker directive
     * file and not on the command line.
     */
    search = new_afile(*name, lang_input_file_is_file_enum, default_target,
		       (bfd *) 0,-1,-1);
    /* No need to stat it here.  It will be stat'd sometime later if it needs to. */
    ldmain_open_file_read_symbol(search);
    return search;
}

/* We save off any section : { *(.text) ...} statements to figure out where
   to place the sraminit code. We save them here.  */
static lang_wild_statement_type           *wild_text_statement;
static lang_output_section_statement_type *wild_text_output_statement;

static void DEFUN(wild,(s, section, file, target, output),
		  lang_wild_statement_type *s AND
		  CONST char *CONST section AND
		  CONST char *CONST file AND
		  CONST char *CONST target AND
		  lang_output_section_statement_type *output)
{
    lang_input_statement_type *f;
    
    if (file == (char *)NULL) {    /* This is the case of *(.text) for example. */
	int success_count = 0;
	/* Perform the iteration over all files in the list */

#ifdef PROCEDURE_PLACEMENT

	if (form_call_graph && !strcmp(".text",section)) { /* With -b, and *(.text), we do
							     not tie anything.  Leave everything
							     orphaned for later procedure placement
							     done in lang_allocate(). */
	    procedure_placement_code_output_section = output;
	    return;
	}
#endif

	if (!strcmp(".text",section)) {
	    /* We have a *(.text) statement.  We save it off in these static variables
	       so later when we link in the sraminit code, we'll know where to put it. */
	    wild_text_statement = s;
	    wild_text_output_statement = output;
	}

	for (f = (lang_input_statement_type *)file_chain.head; f;
	     f = (lang_input_statement_type *)f->next) {
	    success_count += wild_section(s, section, f, output, 1, 0);
	}
	if (success_count == 0 && !suppress_all_warnings)
		info("%P: warning: section `%s' was not found in any input file.\n",section);
	if (!strcmp(section,"COMMON")
	    &&  default_common_section == (lang_output_section_statement_type*)NULL)
	    {
		/* Remember the section that common is going to incase we later
		 * get something which doesn't know where to put it
		 */
		default_common_section = output;
	    }
    } else 
	    /* This could be the case of:
	       foo.o      or
	       foo.o(.text)    */
	{
	    /* Perform the iteration over a single file */
	    wild_section( s, section, lookup_name((CONST char **)&file,-1,-1), output, 0, 1);
	}
}

static void     check_for_output_filename_is_an_input_filename(name)
    char *name;
{
#ifndef DOS
    struct stat name_stat;
#else
    char namepath[_MAX_PATH];
#endif

    if (
#ifndef DOS
	stat(name,&name_stat) != 0
#else
	_fullpath(namepath,name,_MAX_PATH) == NULL
#endif
	)  /* Output filename does not exist.  This means it can not be in the input file list. */
	    return;
    else {
	lang_input_statement_type *search;

	for(search = (lang_input_statement_type *)input_file_chain.head;
	    search;
	    search = (lang_input_statement_type *)search->next_real_file) {
#ifdef DOS
	    char searchpath[_MAX_PATH];
#else
	    struct stat search_stat;
#endif

	    if (search->filename &&
#ifndef DOS
		(((search->dev_no == -1 && search->ino == -1) ?
		 (stat(search->filename,&search_stat) == 0 &&
		  ((search->dev_no = search_stat.st_dev,
		   search->ino = search_stat.st_ino) || 1)) : 1) &&
		 name_stat.st_dev == search->dev_no &&
		 name_stat.st_ino == search->ino)
#else
		((_fullpath(searchpath,search->filename,_MAX_PATH) != NULL) && !stricmp(searchpath,namepath))
#endif
		)
		    info("%P%F: fatal error.  Output file: %s is the same as the input file: %s\n",
			 name,search->filename);
	}
    }
}

int rm_output_filename;

/*
 * read in all the files 
 */
static bfd *DEFUN(open_output,(name),
		  CONST char *CONST name)
{
    extern CONST char *output_filename;
    bfd *output;
    
    if (output_target == (char *)NULL) {
	output_target = current_target ? current_target : default_target;
    }

    check_for_output_filename_is_an_input_filename(name);

    unlink( name );
    rm_output_filename = true;
    output = bfd_openrw(name, output_target);
    output_filename = name;
    
    if (output == (bfd *)NULL) {
	if (bfd_error == invalid_target) {
	    info("%P%F target %s not found\n", output_target);
	}
	info("%P%F problem opening output file %s, %E\n", name);
    }
    output->flags |= D_PAGED | WRITE_CONTENTS;
    bfd_set_format(output, bfd_object);
    return output;
}

static void DEFUN(ldlang_open_output,(statement),
		  lang_statement_union_type *statement)
{
    switch (statement->header.type) {
 case  lang_output_statement_enum:
	output_bfd = open_output(statement->output_statement.name);
	ldemul_set_output_arch();
	break;
 case lang_target_statement_enum:
	current_target = statement->target_statement.target;
	break;
 default:
	break;
    }
}

static void DEFUN(open_input_bfds,(statement),
		  lang_statement_union_type *statement)
{
    switch (statement->header.type) {
 case lang_target_statement_enum:
	current_target = statement->target_statement.target;
	break;
 case lang_wild_statement_enum:
	/* Maybe we should load the file's symbols */
	if (statement->wild_statement.filename) {
	    (void)lookup_name(&statement->wild_statement.filename,-1,-1);
	}
	break;
 case lang_input_statement_enum:
	if (statement->input_statement.real) {
	    statement->input_statement.target = current_target;
	    lookup_name(&statement->input_statement.filename,
			statement->input_statement.dev_no,
			statement->input_statement.ino);
	}
	break;
 default:
	break;
    }
}

/*
 * Add the supplied name to the symbol table as an undefined reference.
 * Remove items from the chain as we open input bfds
 */
typedef struct ldlang_undef_chain_list_struct {
    struct ldlang_undef_chain_list_struct *next;
    char *name;
} ldlang_undef_chain_list_type;

static ldlang_undef_chain_list_type *ldlang_undef_chain_list_head;

void
	DEFUN(ldlang_add_undef,(name),	
	      CONST char *CONST name)
{
    ldlang_undef_chain_list_type *new = (ldlang_undef_chain_list_type *)
	    ldmalloc((bfd_size_type)(sizeof(ldlang_undef_chain_list_type)));
    
    new->next = ldlang_undef_chain_list_head;
    ldlang_undef_chain_list_head = new;
    new->name = buystring(name);
}


/* Run through the list of undefineds created above and place them
 * into the linker hash table as undefined symbols belonging to the
 * script file.
 */
static void
	DEFUN_VOID(lang_place_undefineds)
{
    ldlang_undef_chain_list_type *ptr;
    
    for (ptr=ldlang_undef_chain_list_head; ptr; ptr = ptr->next) {
	ldsym_type *sy = ldsym_get(ptr->name);
	asymbol *def;
	asymbol **def_ptr = (asymbol **)
		ldmalloc((bfd_size_type)(sizeof(asymbol **)));
	def = (asymbol *)bfd_make_empty_symbol(script_file->the_bfd);
	*def_ptr= def;
	def->name = ptr->name;
	def->flags = BSF_UNDEFINED;
	def->section = (asection *)NULL;
	def->udata = (PTR)0;
	Q_enter_global_ref(def_ptr,0,lang_first_phase_enum);
    }
}

/* Copy important data from out internal form to the bfd way. Also
 * create a section for the dummy file
 */
static void
	DEFUN_VOID(lang_create_output_section_statements)
{
    lang_statement_union_type *os;
    lang_output_section_statement_type *s;
    
    for (os = lang_output_section_statement.head; os;
	 os = os->output_section_statement.next) {
	s = &os->output_section_statement;
	init_os(s);
    }
}


static void
	DEFUN_VOID(lang_init_script_file)
{
    script_file = lang_add_input_file("script file",
				      lang_input_file_is_fake_enum, (char *)NULL,(bfd *)0,-1,-1);
    script_file->the_bfd = bfd_create("script file", output_bfd);
    script_file->common_section = bfd_make_section(script_file->the_bfd, "COMMON");
    script_file->the_bfd->usrdata = (PTR) script_file;
    script_file->symbol_count = 0;
    ldlang_add_file(script_file);
}


/* Open input files and attach to output sections
 */
static void
	DEFUN(map_input_to_output_sections,(s, target, output_section_statement),
	      lang_statement_union_type *s AND
	      CONST char *target AND
	      lang_output_section_statement_type *output_section_statement)
{
    lang_output_section_statement_type *os;
    
    for (; s != (lang_statement_union_type *)NULL ; s = s->next) {
	switch (s->header.type) {
    case lang_wild_statement_enum:
	    wild(&s->wild_statement,
		 s->wild_statement.section_name,
		 s->wild_statement.filename,
		 target,
		 output_section_statement);
	    break;
    case lang_output_section_statement_enum:
	    map_input_to_output_sections(s->output_section_statement.children.head,
					 target,
					 &s->output_section_statement);
	    break;
    case lang_target_statement_enum:
	    target = s->target_statement.target;
	    break;
    case lang_output_statement_enum:
    case lang_fill_statement_enum:
    case lang_input_section_enum:
    case lang_object_symbols_statement_enum:
    case lang_data_statement_enum:
    case lang_assignment_statement_enum:
    case lang_padding_statement_enum:
    case lang_address_statement_enum:
	    break;
    case lang_afile_asection_pair_statement_enum:
	    FAIL();
	    break;
    case lang_input_statement_enum:
	    /* A standard input statement, has no wildcards */
	    /*ldmain_open_file_read_symbol(&s->input_statement);*/
	    break;
	}
    }
}

/* Work out how much this section will move the dot point */
static bfd_vma 
	DEFUN(size_input_section, (this_ptr, output_section_statement, fill,  dot),
	      lang_statement_union_type **this_ptr AND
	      lang_output_section_statement_type*output_section_statement AND
	      unsigned short fill AND
	      bfd_vma dot)
{
    lang_input_section_type *is = &((*this_ptr)->input_section);
    asection *i = is->section;
    
    if ( !is->ifile->just_syms_flag) {
	if (i ->size > 0) {
	    dot = insert_pad(this_ptr, fill, i->alignment_power,
			      output_section_statement->bfd_section, dot);
	}
	/* Remember where in output section this input
	   section goes */
	i->output_offset = dot -
		output_section_statement->bfd_section->vma;
	/* remember largest size so we can malloc the largest area
	 * needed for the output stage
	 */
	if (i->size > largest_section) {
	    largest_section = i->size;
	}
	
	/* Mark how big output section must be to contain this now */
	dot += i->size;
	output_section_statement->bfd_section->size =
		dot - output_section_statement->bfd_section->vma;
    } else {
	i->output_offset = i->vma -
		output_section_statement->bfd_section->vma;
    }
    
    return dot ;
}

static lang_output_section_statement_type *group_top(os)
    lang_output_section_statement_type *os;
{
    lang_statement_union_type *u;
    lang_output_section_statement_type *v;

    for (u = lang_output_section_statement.head; u; u = v->next) {
	v = &u->output_section_statement;
	if (v->next_in_group) {
	    lang_output_section_statement_type *top = v;
	    lang_output_section_statement_type *t = v;

	    while (t->next_in_group != (lang_output_section_statement_type *) 0xffffffff)
		    t = t->next_in_group;
	    if (t == os)
		    return top;
	}
    }
    return (lang_output_section_statement_type *) 0;
}
/* Work out the size of the output sections 
 * from the sizes of the input sections
 */
static bfd_vma
	DEFUN(lang_size_sections,(s, output_section_statement, prev, fill, fillset, dot),
	      lang_statement_union_type *s AND
	      lang_output_section_statement_type * output_section_statement AND
	      lang_statement_union_type **prev AND
	      unsigned short fill AND
	      boolean fillset AND
	      bfd_vma dot)
{
    bfd_vma before,after;
    lang_output_section_statement_type *os;
    unsigned int size;
    bfd_vma newdot;
    
    /* Size up the sections from their constituent parts */
    for ( ; s; s = s->next) {
	switch (s->header.type) {
    case lang_output_section_statement_enum:
	    
	    os = &(s->output_section_statement);
	    
	    /* The start of a section */
	    
	    /* We'll figure out real start addresses later.
	       For now, assume all sections start at 0. */
	    
	    os->bfd_section->vma = dot = 0;
	    os->bfd_section->output_offset = 0;
	    
	    (void)lang_size_sections(os->children.head, os,
				     &os->children.head, fillset?fill:os->fill, fillset, dot);
	    
	    if (os->section_was_split) {
		before = os->bfd_section->size;
		after = align_power( os->bfd_section->size,
				    os->bfd_section->alignment_power);
		if (before != after) {
		    insert_pad(os->children.tail,fillset?fill:os->fill,
			       os->bfd_section->alignment_power,
			       os->bfd_section,before);
		}
	    }
	    else {
		/* The end of the section should be sized up to be a multiple of four.
		   We do this only for COFF and BOUT: */

		int alignment_factor = (!BFD_ELF_FILE_P(output_bfd)) ? 2 : 0;

		before = os->bfd_section->size;
		after = align_power( os->bfd_section->size,alignment_factor);
		if (after != before) {
		    insert_pad(os->children.tail,fillset?fill:os->fill,
			       alignment_factor,
			       os->bfd_section,before);
		}
	    }
	    
	    os->bfd_section->size = after - os->bfd_section->vma;
	    os->processed = true;
	    
	    break;
	    
    case lang_data_statement_enum:
	    s->data_statement.output_vma = dot
		    - output_section_statement->bfd_section->vma;
	    s->data_statement.output_section =
		    output_section_statement->bfd_section;
	    output_section_statement->bfd_section->flags |= SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD;
	    
	    switch (s->data_statement.type) {
	case LONG:
		size = LONG_SIZE;
		break;
	case SHORT:
		size = SHORT_SIZE;
		break;
	case BYTE:
		size = BYTE_SIZE;
		break;
	    }
	    dot += size;
	    output_section_statement->bfd_section->size += size;
	    break;
    case lang_wild_statement_enum:
	    
	    dot = lang_size_sections(
				     s->wild_statement.children.head,
				     output_section_statement,
				     &s->wild_statement.children.head,
				     fill,
				     fillset,
				     dot);
	    break;
    case lang_object_symbols_statement_enum:
	    create_object_symbols = output_section_statement;
	    break;
    case lang_output_statement_enum:
    case lang_target_statement_enum:
	    break;
    case lang_input_section_enum:
	    dot =	size_input_section(prev,
					   output_section_statement,
					   fillset ? fill : output_section_statement->fill, dot);
	    break;
    case lang_input_statement_enum:
	    break;
    case lang_fill_statement_enum:
	    fillset = true;
	    fill = s->fill_statement.fill;
	    break;
    case lang_assignment_statement_enum:
	{
	    etree_value_type result;
	    
	    newdot = dot;
	    exp_fold_tree(s->assignment_statement.exp,
			  output_section_statement,
			  lang_allocating_phase_enum,
			  dot,
			  &newdot,
			  &result);
	}
	    if (newdot != dot) {
		/* We've been moved ! so insert a pad */
		lang_statement_union_type *new = 
			(lang_statement_union_type *)
				ldmalloc((bfd_size_type)(sizeof(lang_padding_statement_type)));
		/* Link into existing chain */
		new->header.next = *prev;
		*prev = new;
		new->header.type = lang_padding_statement_enum;
		new->padding_statement.output_section =
			output_section_statement->bfd_section;
		new->padding_statement.output_offset = dot
			- output_section_statement->bfd_section->vma;
		new->padding_statement.fill = fill;
		new->padding_statement.size = newdot - dot;
		output_section_statement->bfd_section->size +=
			new->padding_statement.size;
		dot = newdot;
		/* We Or in SEC_HAS_CONTENTS to make sure this section gets
		   written (here, filler will be written). */
		output_section_statement->bfd_section->flags |= SEC_HAS_CONTENTS | SEC_LOAD | SEC_ALLOC;
	    }
	    break;
    case lang_address_statement_enum:
	    /* Mark the specified section with the supplied address
	     */
	    os = lang_output_section_statement_lookup(
						      s->address_statement.section_name,1);
	    if (!os->section_was_split && os->next_in_group) {
		if (os->next_in_group == (lang_output_section_statement_type *)0xffffffff)
			os = group_top(os);
		else
			info("%F%P: can't use -B to set start address of section %s inside GROUP\n",
			     s->address_statement.section_name);
	    }
	    if (os->addr_tree || os->region_name || os->region_attributes) {
		info("Warning: -B and -T on command line overrides allocation specified\n");
		info("in linker directive files. (output section name: %s, overriding allocation: %s)\n",
		     os->name,(os->addr_tree) ? "binding expression" : "region or region attribute");
	    }
	    os->addr_tree = s->address_statement.address;
	    if (os->bfd_section == (asection *)NULL) {
		info( "%F%P: can't set address of undefined section %s\n",
		     s->address_statement.section_name);
	    }
	    break;
    case lang_padding_statement_enum:
    default:
	    FAIL();
	    break;
	}
	prev = &s->header.next;
    }
    return dot;
}

static bfd_vma
	DEFUN(lang_do_assignments,(s, output_section_statement, fill, dot),
	      lang_statement_union_type *s AND
	      lang_output_section_statement_type * output_section_statement AND
	      unsigned short fill AND
	      bfd_vma dot)
{
    lang_output_section_statement_type *os;
    etree_value_type value;
    int assignment_errors = 0;
    
    for (; s; s = s->next) {
	switch (s->header.type) {
    case lang_output_section_statement_enum:
	    os = &(s->output_section_statement);
	    dot = os->bfd_section->vma;
	    (void) lang_do_assignments(os->children.head, os,
				       os->fill, dot);
	    dot = os->bfd_section->vma + os->bfd_section->size;
	    break;
    case lang_wild_statement_enum:
	    dot = lang_do_assignments(
				      s->wild_statement.children.head,
				      output_section_statement,
				      fill,
				      dot);
	    break;
    case lang_object_symbols_statement_enum:
    case lang_output_statement_enum:
    case lang_target_statement_enum:
	    break;
    case lang_data_statement_enum:
	    exp_fold_tree(s->data_statement.exp, 0,
			  lang_final_phase_enum, dot, &dot, &value);
	    s->data_statement.value = value.value;
	    if (!value.valid) {
		info("%F%P: Invalid data statement\n");
	    }
	    switch (s->data_statement.type) {
	case LONG:  dot += LONG_SIZE;  break;
	case SHORT: dot += SHORT_SIZE; break;
	case BYTE:  dot += BYTE_SIZE;  break;
	    }
	    break;
    case lang_input_section_enum:
	{
	    asection *in = s->input_section.section;
	    dot += in->size;
	}
	    break;
    case lang_fill_statement_enum:
	    fill = s->fill_statement.fill;
	    break;
    case lang_assignment_statement_enum:
	{
	    etree_value_type result;
	    
	    exp_fold_tree(
			  s->assignment_statement.exp,
			  output_section_statement,
			  lang_final_phase_enum,
			  dot,
			  &dot,
			  &result);
	    if (!result.valid)
		    assignment_errors++;
	}
	    break;
    case lang_padding_statement_enum:
	    dot += s->padding_statement.size;
	    break;
    case lang_input_statement_enum:
    case lang_address_statement_enum:
	    break;
    default:
	    FAIL();
	    break;
	}
	
    }
    if (assignment_errors)
	    info("%FIllegal assignment(s).\n");

    return dot;
}

static void DEFUN_VOID(lang_relocate_globals)
{
    /*
     * Each ldsym_type maintains a chain of pointers to asymbols which
     * references the definition.  Replace each pointer to the referenence
     * with a pointer to only one place, preferably the definition. If
     * the defintion isn't available then the common symbol, and if
     * there isn't one of them then choose one reference.
     */
    
    FOR_EACH_LDSYM(lgs) {
	asymbol *it;
	if (lgs->sdefs_chain) {
	    it = *(lgs->sdefs_chain);
	} else if (lgs->scoms_chain) {
	    it = *(lgs->scoms_chain);
	} else if (lgs->srefs_chain) {
	    it = *(lgs->srefs_chain);
	} else {
	    /* This can happen when the command line asked for a
	     * symbol to be -u
	     */
	    it = (asymbol *)NULL;
	}
	
	if (it) {
	    asymbol **ptr= lgs->srefs_chain;
	    while (ptr) {
		asymbol *ref = *ptr;
		*ptr = it;
		ptr = (asymbol **)(ref->udata);
	    }
	}
    }
}

static void DEFUN_VOID(lang_finish)
{
    ldsym_type *lgs = (ldsym_type *) 0;
    static char *name_array[3] = {NULL,"start","_main"};
    int i;
    bfd_vma start_address = 0;
    
    name_array[0] = (char *)entry_symbol;
    for(i=0;i < 3;i++)
	    if (name_array[i] && *name_array[i]) {
		if ((lgs = ldsym_get_soft(name_array[i])) && lgs->sdefs_chain) {
		    asymbol *sy = *(lgs->sdefs_chain);
		    start_address = sy->section ? outside_symbol_address(sy) :
			    sy->value;
		    break;
		}
		else
			lgs = (ldsym_type *) 0;
		if (i == 0)
			info("%P: WARNING Entry point symbol (%s) not found.\n",
			     entry_symbol);
	    }
    
    if (!lgs) {
	/* Can't find anything reasonable, 
	 * use the first address in the .text section (if there is one):
	 */
	asection *ts = bfd_get_section_by_name(output_bfd, ".text");
	if (ts)
		start_address = ts->vma;
    }
    bfd_set_start_address(output_bfd, start_address);
}


/* By now we know the target architecture, and we may have an
 * ldfile_output_machine_name
 */
static void DEFUN_VOID(lang_check)
{
    lang_statement_union_type *file;
    unsigned long max_machine = bfd_get_machine(output_bfd);
    unsigned long max_machine_seen = 0,input_file_attributes = 0,incomp_found = 0;
    bfd * input_bfd;
    unsigned long input_machine;

    for (file = file_chain.head; file; file=file->input_statement.next) {
	input_bfd = file->input_statement.the_bfd;
	input_machine = bfd_get_machine(input_bfd);

	input_file_attributes |= bfd_get_target_attributes(input_bfd);

	if (input_machine > max_machine_seen) {
	    max_machine_seen = input_machine;
	}
	
	/* Inspect the architecture and ensure we're linking like
	 * with like
	 */
	if ((input_machine > max_machine)
	    ||  !bfd_arch_compatible(input_bfd,output_bfd,&ldfile_output_architecture,NULL)){
	    enum bfd_architecture this_architecture =
		    bfd_get_architecture(file->input_statement.the_bfd);
	    unsigned long this_machine =
		    bfd_get_machine(file->input_statement.the_bfd);

	    incomp_found = 1;
	    info("Warning: %I: architecture %s", file,
		 bfd_printable_arch_mach(this_architecture,this_machine));
	    info(" incompatible with output %s\n",
		 bfd_printable_arch_mach(ldfile_output_architecture,ldfile_output_machine));
	}
    }
    if (incomp_found)
	    bfd_set_arch_mach(output_bfd,ldfile_output_architecture,
			      bfd_mach_i960_core,
			      BFD_960_GENERIC);
    else
	    bfd_set_arch_mach(output_bfd,ldfile_output_architecture,
			      max_machine_seen,
			      ldfile_real_output_machine);
    bfd_set_target_attributes(output_bfd,input_file_attributes);
}

extern boolean force_profiling;  /* defined in ldmain.c */

static unsigned long log2(x)
    unsigned long x;
{
    ASSERT(x);
    if (x == 1)
	return 0;
    else {
	unsigned long l = 0;

	x >>= 1;
	while (x) {
	    l++;
	    x >>= 1;
	}
	return l;
    }
}

/*
 * run through all the global common symbols and tie them 
 * to the output section requested. 
 *
 * As an experiment we do this 4 times, once for all the byte sizes,
 * then all the two  bytes, all the four bytes and then everything else
 */
static void DEFUN_VOID(lang_common)
{
    ldsym_type *lgs;
    size_t power;
    
    if ( config.relocateable_output
	&&   !command_line.force_common_definition && !force_profiling) {
	return;
    }
    
    for (power = 1; (config.sort_common && power == 1) || (power <= 16); power <<=1) {
	for (lgs = symbol_head; lgs; lgs=lgs->next) {
	    asymbol *com ;
	    unsigned  int power_of_two;
	    size_t size;
	    size_t align;
	    if (lgs->scoms_chain) {
		com = *(lgs->scoms_chain);
		size = com->value;
		if (!com->value_2)
			switch (size) {
		    case 0:
		    case 1:
			    align = 1;
			    power_of_two = 0;
			    break;
		    case 2:
			    power_of_two = 1;
			    align = 2;
			    break;
		    case 3:
		    case 4:
			    power_of_two =2;
			    align = 4;
			    break;
		    case 5:
		    case 6:
		    case 7:
		    case 8:
			    power_of_two = 3;
			    align = 8;
			    break;
		    default:
			    power_of_two = 4;
			    align = 16;
			    break;
			}
		else
			power_of_two = log2(align = com->value_2);
		
		if (config.sort_common && align != power) {
		    continue;
		}
		
		/* Change from a common symbol into a
		 * definition of a symbol
		 */
		lgs->sdefs_chain = lgs->scoms_chain;
		lgs->scoms_chain = (asymbol **)NULL;
		commons_pending--;
		
		if (ld960sym_is_a_profile_counter(lgs->name)) {
		    com->section = bfd_get_section_by_name(com->the_bfd,"PROFILE_COUNTER");
		    align = 4;
		    com->section->alignment_power = power_of_two = 2;
		}
		/* Point to the correct common section */
		else if (com->the_bfd->usrdata)
			com->section = ((lang_input_statement_type *)
					(com->the_bfd->usrdata))->common_section;
		    
		/*  Fix the size of the common section */
		com->section->size =
			ALIGN(com->section->size, align);
		    
		/* Remember if this is the biggest alignment
		 * ever seen
		 */
		if (power_of_two > com->section->alignment_power) {
		    com->section->alignment_power = power_of_two;
		}
		com->value = com->section->size;
		com->section->size += size;

		/* Symbol stops being common and starts being
		 * global, but we remember that it was common
		 * once.
		 */
 		com->flags &= ~BSF_FORT_COMM;
 		com->flags |= (BSF_OLD_COMMON|BSF_GLOBAL|BSF_EXPORT);
	    }
	}
    }
}

/*
 * run through the input files and ensure that every input 
 * section has somewhere to go. If one is found without
 * a destination then create an input request and place it
 * into the statement tree.
 */
static void DEFUN_VOID(lang_place_orphan_input_sections)
{
    lang_input_statement_type *file;
    asection *s;
    
    for (file = (lang_input_statement_type*)file_chain.head;
	 file;
	 file = (lang_input_statement_type*)file->next) {
	
	for (s = file->the_bfd->sections; s; s = s->next) {
	    if (s->output_section
#ifdef PROCEDURE_PLACEMENT
		|| (form_call_graph && (!strcmp(".text",s->name) || IS_NAMED_TEXT_SECTION( s->name )))
#endif
		)
		    continue;

	    /* This section of the file is not attached,
	     * root around for a sensible place for it to go
	     */
	    
	    if (file->common_section == s) {
		/* This is a lonely common section which must
		 * have come from an archive. We attach to the
		 * section with the wildcard
		 */
		if (!config.relocateable_output || 
		    (config.relocateable_output && command_line.force_common_definition)) {
		    if ( !default_common_section) {
			default_common_section = 
				lang_output_section_statement_lookup(".bss",1);
		    }
		    wild_doit(
			      &default_common_section->children,
			      s, 
			      default_common_section,
			      file, 1);
		}
	    } else {
		lang_output_section_statement_type *os;
		char *oname = (char *)s->name;

#ifdef PROCEDURE_PLACEMENT
		if ( IS_NAMED_TEXT_SECTION( s->name ) ) {
		    info("%P:Warning: section: %B(%s) attached to .text output section.\n",file->the_bfd,
			 s->name);
		    oname = ".text";
		}
#endif
		os = lang_output_section_statement_lookup(oname,1);
		wild_doit(&os->children, s, os, file, 1);
	    }
	}
    }
}

void DEFUN(lang_set_flags,(ptr, flags),
	   int  *ptr AND
	   CONST char *flags)
{
    
    char error_string[2];
    
    /* initialize to zero */
    *ptr= 0;
    
    while (*flags) {
	switch (*flags) {
    case 'R':
	    *ptr |= MEM_READ;
	    break;
    case 'W':
	    *ptr |= MEM_WRITE;
	    break;
    case 'X':
	    *ptr |= MEM_EXEC;
	    break;
    case 'L':
    case 'I':
	    *ptr |= MEM_LOAD;
	    break;
    default:
	    error_string[0] = *flags;
	    error_string[1] = '\0';
	    info("%S%F Bad memory region attribute %s\n",&error_string[0]);
	    break;
	}
	flags++;
    }
}

void DEFUN(lang_for_each_file,(func),
	   PROTO(void, (*func),(lang_input_statement_type *)))
{
    lang_input_statement_type *f;
    
    for (f = (lang_input_statement_type *)file_chain.head; 
	 f;
	 f = (lang_input_statement_type *)f->next)
	{
	    func(f);
	}
}


void DEFUN(ldlang_add_file,(entry),
	   lang_input_statement_type *entry)
{
    lang_statement_append(&file_chain, (lang_statement_union_type *)entry,
			  &entry->next);
}


void DEFUN(lang_add_output,(name),
	   char *name)
{
    lang_output_statement_type *new;
    
    new = new_stat(lang_output_statement, stat_ptr);
    new->name = name;
}

void DEFUN(lang_enter_group_statement,(start_address),
	   etree_type *start_address)
{
    /* Set flag saying we are inside a GROUP */
    in_group = true;
    current_group = (lang_group_info_type *)ldmalloc(sizeof(lang_group_info_type));
    
    /* Remember the group-wide information specified here */
    current_group->addr_tree = start_address;
    manual_allocation |= (start_address != (etree_type *) 0);

    current_group->first_link.this_section =
	    (lang_output_section_statement_type *)NULL;
    current_group->first_link.next_link =
	    (lang_group_section_link_type *)NULL;
    current_group->current_link
	    = (lang_group_section_link_type *)&(current_group->first_link.this_section);
    return;
}

void DEFUN(lang_leave_group_statement,(fill,memspec),
	   bfd_vma fill AND
	   CONST char *memspec)
{
    lang_output_section_statement_type *last_section, *this_section, *next_section;

    /* Clear the default fill value flag: */
    fill &= (~LDLANG_DFLT_FILL);

    /* Clear flag saying we are inside a GROUP, since we're not any more */
    in_group = false;
    
    /* Fill in any additional information */
    current_group->fill = fill;
    current_group->region_name = NULL;
    current_group->region_attributes = NULL;
    if (memspec && *memspec != '(') {
	if (!lang_memory_region_lookup(memspec))
		info("%F%S No such region as %s\n",memspec);
	current_group->region_name = (char*)memspec;
	manual_allocation = 1;
    }
    else if (memspec) {
	current_group->region_attributes = (char*)memspec;
	manual_allocation = 1;
    }
    
    /* Exit if we have an empty GROUP. */
    if (current_group->first_link.this_section ==
	(lang_output_section_statement_type *)NULL) {
	return;
    }
    
    /* Sweep through the list of sections in this list, filling in
       information they should inherit from the GROUP, including the
       contents of next_in_this_group from the linked list. */
    last_section = this_section = current_group->first_link.this_section;
    next_section = current_group->first_link.next_link->this_section;
    current_group->current_link = &(current_group->first_link);
    while (this_section) {
	this_section->addr_tree = current_group->addr_tree;
	this_section->region_name = current_group->region_name;
	this_section->region_attributes = current_group->region_attributes;
	if (this_section->flags & LDLANG_DFLT_FILL)
		this_section->fill = current_group->fill;
	this_section->next_in_group = next_section;
	current_group->current_link = current_group->current_link->next_link;
	last_section = this_section;
	this_section = current_group->current_link->this_section;
	if (this_section) {
	    next_section = current_group->current_link->next_link->this_section;
	}
    }
    /* Terminate list of sections in group with 0xFFFFFFFF so that any
       section in a group will have a non-zero next_in_group. We use this
       to recognize when we are trying to do things to sections in a group
       that should not be done to sections in a group. */
    if (last_section) {
	last_section->next_in_group =
		(lang_output_section_statement_type *)0xFFFFFFFF;
    }
    /* Free the now unnecessary GROUP-related structures */
    current_group->current_link = current_group->first_link.next_link;
    while (current_group->current_link){
	lang_group_section_link_type *temp;
	
	temp = current_group->current_link;
	current_group->current_link = current_group->current_link->next_link;
	free(temp);
    }
    free(current_group);
    current_group = (lang_group_info_type *)NULL;
    return;
}

void DEFUN(lang_enter_output_section_statement,
	   (output_section_statement_name, address_exp, flags),
	   char *output_section_statement_name AND
	   etree_type *address_exp AND
	   int flags)
{
    lang_output_section_statement_type *os;
    
    current_section = os =
            lang_output_section_statement_lookup(output_section_statement_name,1);
    if (in_group){
	lang_group_section_link_type *temp;
	if (address_exp) {
	    info("%F%S Cannot specify start address for section %s inside GROUP\n",current_section->name);
	}
	if (flags & SEC_IS_COPY) {
	    info("%F%S Cannot include COPY-type section %s in GROUP\n",current_section->name);
	}
	/* Add an entry for this section to the list of
	   sections in this group */
	current_group->current_link->this_section = current_section;
	temp = (lang_group_section_link_type *)ldmalloc(sizeof(lang_group_section_link_type));
	temp->this_section = (lang_output_section_statement_type *)NULL;
	temp->next_link = (lang_group_section_link_type *)NULL;
	current_group->current_link->next_link = temp;
	current_group->current_link = temp;
    }
    
    if (os->addr_tree == (etree_type *)NULL) {
	manual_allocation |= (address_exp != (etree_type *) 0);
	os->addr_tree = address_exp;
    }
    os->flags = flags;
    stat_ptr = & os->children;
}


asymbol *DEFUN(create_symbol,(name, flags, section),
	       CONST char *name AND
	       flagword flags AND
	       asection *section)
{
    extern lang_input_statement_type *script_file;
    asymbol **def_ptr;
    asymbol *def;
    
    def_ptr = (asymbol **)ldmalloc((bfd_size_type)(sizeof(asymbol **)));
    /* Add this definition to script file */
    def = (asymbol*) bfd_make_empty_symbol(script_file->the_bfd);
    def->name = buystring(name);
    def->udata = 0;
    def->flags = flags;
    def->section = section;
    *def_ptr = def;
    Q_enter_global_ref(def_ptr,0,lang_first_phase_enum);
    return def;
}

static void DEFUN_VOID(lang_circular_lib_search)
{
    extern unsigned int undefined_global_sym_count,global_symbol_count;
    
    if (undefined_global_sym_count) {
	unsigned int prev_undefined_global_sym_count,prev_global_symbol_count;
	
	do {
	    lang_input_statement_type *search;
	    
	    prev_undefined_global_sym_count = undefined_global_sym_count;
	    prev_global_symbol_count = global_symbol_count;

	    for(search = (lang_input_statement_type *)input_file_chain.head;
		search;
		search = (lang_input_statement_type *)search->next_real_file){
		if (search->the_bfd && 
		    gnu960_check_format(search->the_bfd, bfd_archive)) {
		    if (option_v)
			    printf("circular searching lib: %s\n",search->filename);
		    search_library(search);
		}
	    }
	} while (undefined_global_sym_count &&
		 (prev_undefined_global_sym_count != undefined_global_sym_count ||
		  prev_global_symbol_count != global_symbol_count));
    }
}

static int  target_byte_order_error = 0;

/* This code produces the __B{section} and __E{section} symbols. */

static char *Section_Name(s1,s2)
    char *s1,*s2;
{
    static int ret_val_idx = 0;
    static char *ret_val[2] = {NULL,NULL};
    static int ret_val_length[2] = {0,0};
    int length = strlen(s1) + strlen(s2) + 1;

    ret_val_idx = (ret_val_idx + 1) & 1;
    if (length > ret_val_length[ret_val_idx]) {
	if (ret_val_length[ret_val_idx])
		ret_val[ret_val_idx] = ldrealloc(ret_val[ret_val_idx], ret_val_length[ret_val_idx] = length);
	else
		ret_val[ret_val_idx] = ldmalloc(ret_val_length[ret_val_idx] = length);
    }

    strcpy(ret_val[ret_val_idx],s1);

    if (BFD_ELF_FILE_P(output_bfd))
	    strcat(ret_val[ret_val_idx],s2);
    else
	    strncat(ret_val[ret_val_idx],s2,8);

    return ret_val[ret_val_idx];
}

#define Begin_Name(S) Section_Name("__B",S)
#define End_Name(S)   Section_Name("__E",S)

/* When producing the sraminit code, we _can_ emit code to clear named bss sections too.
   We call this code to do so from ld960sym.c */

static int named_bss_sections(begin_name,end_name,idx)
    char **begin_name,**end_name;
    int idx;
{
    int nfound = 0;
    lang_statement_union_type *u;
    lang_output_section_statement_type *v;

    (*begin_name) = (*end_name) = NULL;

    for (u = lang_output_section_statement.head; u; u = v->next) {
	v = &u->output_section_statement;

	if (strcmp(".bss",v->name)	&&
	    (v->flags & LDLANG_HAS_INPUT_FILES) &&
	    (0 == (v->flags & LDLANG_HAS_CONTENTS)) &&
	    (v->bfd_section->flags & SEC_IS_BSS)) {
	    if (nfound++ == idx) {
		(*begin_name) = Begin_Name(v->name);
		(*end_name) = End_Name(v->name);
		return idx;
	    }
	}
    }
    return (idx == -1) ? (nfound) : -1;
}

void DEFUN_VOID(lang_process)
{
    extern boolean circular_lib_search;  /* defined in ldmain.c */
    extern unsigned ld960sym_sizeof_profile_table;

    if (!had_script) {
	parse_line(ldemul_get_script());
    }
    current_target = default_target;
    
    output_target = bfd_make_targ_name(output_flavor, (output_flavor == BFD_COFF_FORMAT) ?
				       host_byte_order_is_big_endian : 0,
				       target_byte_order_is_big_endian);

    lang_for_each_statement(ldlang_open_output); /* Open the output file */

    /* For each output section statement, create a section in the output
     * file
     */
    lang_create_output_section_statements();
    
    /* Create a dummy bfd for the script */
    lang_init_script_file();
    
    /* Add to the hash table all undefineds on the command line */
    lang_place_undefineds();

    /* Create a bfd for each input file */
    current_target = default_target;
    lang_for_each_statement(open_input_bfds);
    
    if (circular_lib_search)
	    lang_circular_lib_search();

    if (1) {
	extern strip_symbols_type strip_symbols;

	if (strip_symbols == STRIP_DEBUGGER ||
	    (!BFD_ELF_FILE_P(output_bfd))) {
	    lang_input_statement_type *f;

	    /* Remove all of the sections from all of the input files that are marked
	       SEC_IS_DEBUG. */

	    for (f = (lang_input_statement_type *)file_chain.head; f;
		 f = (lang_input_statement_type *)f->next) {
		asection *s,**previous_section = &f->the_bfd->sections;

		for (s = f->the_bfd->sections; s; s = s->next) {

		    if (s->flags & SEC_IS_DEBUG)
			    (*previous_section) = s->next;
		    else
			    previous_section = &s->next;
		}
	    }
	}
    }

    if (ld960sym_sizeof_profile_table) {
	/* The following, adds to the definition of .bss, adding to it the PROFILE_COUNTER
	   input sections.  If there were other things defined in the section for example
	   a binding address, an alignment, wild statements etc.  This information will add
	   to that information. */
	char *start_expr,*length_expr;
	static char line_template[] = "  \
                     SECTIONS {          \
		      %s : {             \
		       %s                \
		       [PROFILE_COUNTER] \
                       %s                \
		      }                  \
	             }";
	char *com_section_name = (char *)(default_common_section ? default_common_section->name : ".bss");
	char *temp;
	if (!config.relocateable_output || (config.relocateable_output && force_profiling)) {
	    start_expr = "___profile_data_start = ALIGN(4) ;";
	    length_expr = "___profile_data_length = . -  ___profile_data_start;";
	}
	else {
	    start_expr = length_expr = "";
	}
	temp = ldmalloc(sizeof(line_template) + strlen(com_section_name) +
			strlen(start_expr) + strlen(length_expr) + 1);
	sprintf(temp,line_template,com_section_name,start_expr,length_expr);
	parse_line(temp);
	free(temp);
    }
    else if (!config.relocateable_output || (config.relocateable_output && force_profiling))
	    parse_line("___profile_data_start=0;___profile_data_length=0;");

    /* Read the ccinfo sections from the object files */
    gnu960_ccinfo_build();
    
    /* Run through the contours of the script and attach input sections
     * that have been explicitly allocated to output sections.
     */
    map_input_to_output_sections(statement_list.head, (char *)NULL, 
				 (lang_output_section_statement_type *)NULL);
    
#ifdef PROCEDURE_PLACEMENT
    if (form_call_graph)
	    ldplace_form_call_graph();
#endif

    /* Find any sections not attached explicitly and handle them */
    lang_place_orphan_input_sections();

    if (!config.relocateable_output || force_profiling) {
	extern char * gnu960_gen_linktime_code();
	char *fn;
	boolean need_sram_init = false;
	boolean need_clear_named_bss_sections = false;
	ldsym_type *temp;
	
	/* need_sram_init is TRUE is ___sram_init is present in
	   the list of symbols, but undefined */
	temp = ldsym_get_soft("___sram_init");
	if (temp) {
	    need_sram_init = !(temp->sdefs_chain);
	}
	temp = ldsym_get_soft("___clear_named_bss_sections");
	if (temp) {
	    need_clear_named_bss_sections = !(temp->sdefs_chain);
	}
	if ( (need_sram_init || need_clear_named_bss_sections) &&
	    (fn = gnu960_gen_linktime_code(need_sram_init,need_clear_named_bss_sections,
					   named_bss_sections))) {
	    lang_statement_union_type *input_statement;

	    open_input_bfds(input_statement = (lang_statement_union_type *)lang_add_input_file(fn,
				lang_input_file_is_file_enum,NULL,(bfd *)0,-1,-1));

	    /* In map_input_to_output_sections, we may have been directed by the user where to place
	       .text sections (from a *(.text) directive).  If so, then wild_text_statement is assigned
	       and so we tie the sraminit code to this output section: */
	    if (wild_text_statement) {
		wild_section(wild_text_statement,".text",&input_statement->input_statement,
			     wild_text_output_statement,0,0);
	    }
	    else {
		/* We did not get a *(.text) section, so we treat the sraminit code as if it were orphaned and
		   tie it to the .text output section. */		   
		lang_output_section_statement_type *os = lang_output_section_statement_lookup(".text",1);

		wild_doit(&os->children,our_bfd_get_section_by_name(input_statement->input_statement.the_bfd,
								    ".text"),os,
			  &input_statement->input_statement,1);
			  
	    }
	}
    }

    /* Size up the common data */
    lang_common();
    
#ifdef PROCEDURE_PLACEMENT
    if (form_call_graph) {
	if (!procedure_placement_code_output_section)
		procedure_placement_code_output_section =
			lang_output_section_statement_lookup(".text",1);
	if (procedure_placement_code_output_section->bfd_section == (asection *)NULL)
		init_os(procedure_placement_code_output_section);
	procedure_placement_code_output_section->bfd_section->flags |= SEC_CODE;
    }
#endif

    if (!manual_allocation) {
	/* If we didn't get SECTIONS directives that forced allocation
	   in any specific manner,
	   then all sections are associated with fake regions.
	   Before getting them placed in memory, sort them by
	   type. */
	default_section_order();
    }
    if (output_flavor == BFD_COFF_FORMAT) {
	check_for_section_split();
    }
    
    /* Size up the sections */
    lang_size_sections(statement_list.head,
		       (lang_output_section_statement_type *)NULL,
		       &(statement_list.head), 0, 0, (bfd_vma)0);
    
    /* If we have no memory regions defined, we need to get one for
       subsequent processing to work properly. */
    if (!lang_memory_region_list) {
	
	/* Create default memory region */
	lang_memory_region_type *temp1 =
		lang_memory_region_create("*default*");
	temp1->origin = 0;
	/* The default memory region is the entire 32 bit address space; 0 and through
	   0xffffffff. */
	temp1->length_lower_32_bits = 0;
	temp1->length_high_bit = 1;
	temp1->flags = (int)(MEM_READ | MEM_WRITE | MEM_EXEC | MEM_LOAD);
	/* set this to keep from issuing warnings
	   about default placement if the user didn't
	   tell us anything about memory configuration
	   in the first place */
	made_default_region = true;
    }

    lang_check_memory_regions();
    
    if (write_map) {
	extern FILE *map_file;
	
	ldmain_emit_filelist();
	lang_map_memory_regions(map_file,"MEMORY CONFIGURATION (total)");
    }

    ld960sym_save_off_memory_regions();

    /* Allocate output sections to memory addresses. */
    lang_allocate();

    /* Update the sram portions of memory, emitting warnings along the way. */
    ld960sym_mark_sram_memory_regions_used();

    if (write_map) {
	extern FILE *map_file;
	
	lang_map_memory_regions(map_file,"MEMORY CONFIGURATION (unused portions)");
    }    
    if ( !config.relocateable_output ) {
	lang_statement_union_type *u;
	lang_output_section_statement_type *v;
	char *w;
	
	/* Both GNU and I960 environments want to see _etext and
	   _edata */
	lang_abs_symbol_at_end_of(".text","_etext");
	lang_abs_symbol_at_end_of(".data","_edata");
	if (invocation == as_gld960) {
	    lang_abs_symbol_at_beginning_of(".bss","_bss_start");
	}
	
	/* Run through the list of sections, generating a
	   begin and end symbol for each. */
	for (u = lang_output_section_statement.head; u; u = v->next) {
	    int fudge = 0;

	    v = &u->output_section_statement;
	    w = (char *)v->bfd_section->name;
	    /* To observe precedent, the leading period
	       will be dropped from begin/end symbols for
	       the standard section names. */
	    if ((!strcmp(w,".text")) || (!strcmp(w,".data"))
		|| (!strcmp(w,".bss")))
		    fudge = 1;
	    lang_abs_symbol_at_beginning_of(w,Begin_Name(w+fudge));
	    lang_abs_symbol_at_end_of(w,End_Name(w+fudge));
	}

	if (ldsym_undefined("__Bctors")) {
		create_symbol("__Bctors", 
			BSF_GLOBAL | BSF_EXPORT | BSF_ABSOLUTE, 0)->value = 0;
	} if (ldsym_undefined("__Ectors")) {
		create_symbol("__Ectors", 
			BSF_GLOBAL | BSF_EXPORT | BSF_ABSOLUTE, 0)->value = 0;
	} if (ldsym_undefined("__Bdtors")) {
		create_symbol("__Bdtors", 
			BSF_GLOBAL | BSF_EXPORT | BSF_ABSOLUTE, 0)->value = 0;
	} if (ldsym_undefined("__Edtors")) {
		create_symbol("__Edtors", 
			BSF_GLOBAL | BSF_EXPORT | BSF_ABSOLUTE, 0)->value = 0;
	}
    }
    /* If this is a final link and the user hasn't defined _end, put it
       just beyond the end of .bss .*/
    if (!(config.relocateable_output) && ldsym_undefined("_end")) {
	
	unsigned long wheres_end;
	
	wheres_end = lang_abs_symbol_at_end_of(".bss","_end");
    }
    
    /* Do all the assignments, now that we know the final restingplaces
     * of all the symbols
     */
    lang_do_assignments(statement_list.head,
			(lang_output_section_statement_type *)NULL,
			0, (bfd_vma)0);
    
    /* Make sure that we're not mixing architectures */
    lang_check();
    
    /* Move the global symbols around */
    lang_relocate_globals();
    
    /* Final stuffs */
    lang_finish();

    gnu960_gen_prof_map();
}

/* EXPORTED TO YACC */

void
	DEFUN(lang_add_wild,(section_name, filename),
	      CONST char *CONST section_name AND
	      CONST char *CONST filename)
{
    lang_wild_statement_type *new = new_stat(lang_wild_statement, stat_ptr);

    if (section_name && !strcmp(section_name,"COMMON") && !filename)
	    default_common_section = current_section;
    if (filename)
	    lang_add_input_file((char *)filename,lang_input_file_is_file_enum,NULL,(bfd *)0,-1,-1);
    new->section_name = section_name;
    new->filename = filename;
    lang_list_init(&new->children);
}

void DEFUN(lang_section_start,(name, address),
	   CONST char *name AND
	   etree_type *address)
{
    lang_address_statement_type *ad;
    
    ad = new_stat(lang_address_statement, stat_ptr);
    ad->section_name = name;
    ad->address = address;
}


void DEFUN(lang_add_entry,(name),
	   CONST char *name)
{
    entry_symbol = name;
}


void DEFUN(lang_add_fill,(exp),
	   int exp)
{
    lang_fill_statement_type *new;
    
    new = new_stat(lang_fill_statement, stat_ptr);
    new->fill = exp;
}

void DEFUN(lang_add_data,(type, exp),
	   int type AND
	   union etree_union *exp)
{
    
    lang_data_statement_type *new;
    
    new = new_stat(lang_data_statement, stat_ptr);
    new->exp = exp;
    new->type = type;
}


void DEFUN(lang_add_assignment,(exp),
	   etree_type *exp)
{
    lang_assignment_statement_type *new;
    
    new = new_stat(lang_assignment_statement, stat_ptr);
    new->exp = exp;
}


void DEFUN(lang_add_attribute,(attribute),
	   enum statement_enum attribute)
{
    new_statement(attribute, sizeof(lang_statement_union_type),stat_ptr);
}


void DEFUN(lang_startup,(name),
	   char *name)
{
    if (C_switch_seen) {
	/* Command line -C switch overrides and renders moot
	   all subsequent STARTUP directives. */
	return;
    }
    if (startup_file) {
	info("%P%FMultiple STARTUP files\n");
    }
    /* Set lang_has_input_file so that we know we have at least one file
       to link */
    lang_has_input_file = true;
    if (name && name[strlen(name)-1] == '*') {
	name[strlen(name)-1] = 0;
	first_file->add_suffixes = true;
	first_file->file_extension = ".o";
	first_file->magic_syslib_and_startup = true;
    }
    first_file->filename = name;
    first_file->local_sym_name = name;
    first_file->search_dirs_flag = true;
    startup_file= name;
}

void DEFUN(lang_float,(maybe),
	   boolean maybe)
{
    lang_float_flag = maybe;
}


void
	DEFUN(lang_leave_output_section_statement,(fill, memspec),
	      bfd_vma fill AND
	      CONST char *memspec)
{
    int is_default_fill = ((fill & LDLANG_DFLT_FILL) == LDLANG_DFLT_FILL);

    /* Clear the default fill value flag: */
    fill &= ~LDLANG_DFLT_FILL;

    if (in_group) {
	/* If we're inside a group, individual sections can't have
	   memspecs. */
	if (memspec) {
	    info("%F%S Cannot specify memory region for section %s inside GROUP\n",current_section->name);
	}
    }
    current_section->fill = fill;
    current_section->flags |= is_default_fill ? LDLANG_DFLT_FILL : 0;

    if ((current_section->addr_tree != NULL) && memspec) {
	/* Note that the following if and the analogous if in
	   lang_allocate_by_start_address assume that ALIGN() will
	   occur only by itself, not as part of an expression, and
	   that it will replace, not supplement, a specific start
	   address. */
	if (current_section->addr_tree->type.node_code != ALIGN_K) {
	    info("%F%S Cannot specify both start address and memory region for section %s\n",current_section->name);
	}
    }

    /* Only if the user named a place to put this section should we
       set its region pointer. Otherwise, we'll fill it in later by looking
       through the list of available regions and, if possible, choosing one
       big enough to hold this section. */
    if (!current_section->region_name && !current_section->region_attributes &&
	memspec) {
	if (*memspec != '(') {
	    if (!lang_memory_region_lookup(memspec))
		    info("%F%S No such memory region as %s\n",memspec);
	    current_section->region_name = (char *)memspec;
	}
	else
		current_section->region_attributes = (char *)memspec;
	manual_allocation = 1;
    }
    stat_ptr = &statement_list;
}

/*
 * Create an absolute symbol with the given name with the value of the
 * address of first byte of the section named.
 * 
 * If the symbol already exists, then do nothing.
 */
void DEFUN(lang_abs_symbol_at_beginning_of,(section, name),
	      CONST char *section AND
	      CONST char *name)
{
    extern bfd *output_bfd;
    extern asymbol *create_symbol();
    asection *s;
    asymbol *def;
    
    s = bfd_get_section_by_name(output_bfd, section);
    def = create_symbol(name, BSF_GLOBAL|BSF_EXPORT|BSF_ABSOLUTE,
			(asection *)NULL);
    def->value = s ? s->vma :  0;
}

/*
 * Create an absolute symbol with the given name with the value of the
 * address of the first byte after the end of the section named.
 *
 * If the symbol already exists, then do nothing.
 */
unsigned long DEFUN(lang_abs_symbol_at_end_of,(section, name),
	      CONST char *section AND
	      CONST char *name)
{
    extern bfd *output_bfd;
    extern asymbol *create_symbol();
    asection *s;
    asymbol *def;
    
    s = bfd_get_section_by_name(output_bfd, section);
    /* Add a symbol called name */
    def = create_symbol(name, BSF_GLOBAL|BSF_EXPORT|BSF_ABSOLUTE,
			(asection *)NULL);
    def->value = s ? s->vma + s->size : 0;
    return(def->value);
}


void DEFUN(lang_statement_append,(list, element, field),
	      lang_statement_list_type *list AND
	      lang_statement_union_type *element AND
	      lang_statement_union_type **field)
{
    *(list->tail) = element;
    list->tail = field;
}


static int
compare_output_section_statements(l1,r1)
    lang_output_section_statement_type **l1,**r1;
{
    lang_output_section_statement_type *l = (*l1), *r = (*r1);
    int left_has_code = SEC_CODE == (l->bfd_section->flags & SEC_CODE),
       right_has_code = SEC_CODE == (r->bfd_section->flags & SEC_CODE);
    int left_has_data = SEC_DATA == (l->bfd_section->flags & SEC_DATA),
       right_has_data = SEC_DATA == (r->bfd_section->flags & SEC_DATA);
    int left_has_bss  = SEC_IS_BSS == (l->bfd_section->flags & SEC_IS_BSS),
        right_has_bss = SEC_IS_BSS == (r->bfd_section->flags & SEC_IS_BSS);
    int lefts_score  = left_has_code*4  + left_has_data*2  + left_has_bss;
    int rights_score = right_has_code*4 + right_has_data*2 + right_has_bss;

#define SACRED(SECT_NAME) (!strcmp(SECT_NAME,l->bfd_section->name)) ? -1 : \
    (!strcmp(SECT_NAME,r->bfd_section->name)) ? 1 : strcmp(l->bfd_section->name,r->bfd_section->name)

    if (lefts_score != rights_score)
	    return rights_score - lefts_score;
    else if (left_has_code)
	    return SACRED(".text");
    else if (left_has_data)
	    return SACRED(".data");
    else if (left_has_bss) {
	if (!strcmp(".bss",l->bfd_section->name))
		return 1;
	else if (!strcmp(".bss",r->bfd_section->name))
		return -1;
	else
		return strcmp(l->bfd_section->name,r->bfd_section->name);
    }
    else
	    return strcmp(l->bfd_section->name,r->bfd_section->name);
}

/* This routine, when the user gives us no guidance whatsoever, orders
   sections with .text (if any) first, followed by text-type sections with other
   names, .data (if any), data-type sections with other names, bss-type sections
   with other names, and .bss (if any). Note that the order of Groups are not
   changed. */

static void DEFUN_VOID (default_section_order)
{
    lang_output_section_statement_type *group_list = (lang_output_section_statement_type *)NULL,
    **group_bottom = &group_list,**bottom_of_statements = 
	    (lang_output_section_statement_type **)&(lang_output_section_statement.head);
    int other_sect_count = 0;
    lang_output_section_statement_type *p,**other_sections =
	    (lang_output_section_statement_type **)ldmalloc(sizeof(lang_output_section_statement_type *)*
							    output_bfd->section_count);
    for (p=(lang_output_section_statement_type *)lang_output_section_statement.head;
	 p;
	 p = (lang_output_section_statement_type *)p->next) {
	while (p->next_in_group) {
	    (*group_bottom) = p;
	    group_bottom = (lang_output_section_statement_type **)&(p->next);
	    if (p->next_in_group == (lang_output_section_statement_type*) 0xffffffff)
		    break;
	    p = p->next_in_group;
	}
	if (p->next_in_group)
		continue;
	other_sections[other_sect_count++] = p;
    }
    if (other_sect_count) {
	int i;

	if (other_sect_count >= 2)
		qsort(other_sections,
		      other_sect_count,
		      sizeof(lang_output_section_statement_type *),
		      compare_output_section_statements
		      );

	for (i=0;i < other_sect_count;i++) {
	    (*bottom_of_statements) = other_sections[i];
	    bottom_of_statements = (lang_output_section_statement_type **)&(other_sections[i]->next);
	}
    }
    free(other_sections);
    (*group_bottom) = (lang_output_section_statement_type *) 0;
    (*bottom_of_statements) = group_list;
}
