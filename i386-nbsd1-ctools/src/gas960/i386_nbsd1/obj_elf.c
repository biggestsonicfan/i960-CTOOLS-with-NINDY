/* elf object file format
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

/* $Id: obj_elf.c,v 1.32 1996/01/05 18:56:11 paulr Exp $ */

#include "as.h"

#include "obstack.h"

#ifdef __STDC__

char *s_get_name(symbolS *s);
void s_stab_stub(int what);
void s_section(void);
static symbolS *tag_find_or_make(char *name);
static symbolS* tag_find(char *name);
static void obj_elf_def(int what);
static void obj_elf_dim(void);
static void obj_elf_endef(void);
static void obj_elf_ident(void);
static void obj_elf_line(void);
static void obj_elf_ln(void);
static void obj_elf_scl(void);
static void obj_elf_size(void);
static void obj_elf_tag(void);
static void obj_elf_type(void);
static void obj_elf_val(void);
static void tag_init(void);
static void tag_insert(char *name, symbolS *symbolP);

#else

char *s_get_name();
void s_stab_stub();
void s_section();
static symbolS *tag_find();
static symbolS *tag_find_or_make();
static void obj_elf_def();
static void obj_elf_dim();
static void obj_elf_endef();
static void obj_elf_ident();
static void obj_elf_line();
static void obj_elf_ln();
static void obj_elf_scl();
static void obj_elf_size();
static void obj_elf_tag();
static void obj_elf_type();
static void obj_elf_val();
static void tag_init();
static void tag_insert();

#endif /* __STDC__ */


/* Global vars related to general segments */
extern segS	*segs;		/* Internal segment array */
extern int  	curr_seg;	/* Active segment (index into segs[]) */
extern int  	segs_size;	/* Number of segS's currently in use in segs */
extern int	DEFAULT_TEXT_SEGMENT; 	/* .text */
extern int	DEFAULT_DATA_SEGMENT; 	/* .data */
extern int	DEFAULT_BSS_SEGMENT; 	/* .bss  */

/* File-scope globals for object file writing */
static object_headers headers;
static long output_byte_count;

/* File-scope globals for inter-section communication */
static int string_table_index;
static int section_string_table_index;
static int symbol_table_index;

const pseudo_typeS obj_pseudo_table[] = 
{
    /* Elf directives.  Currently there aren't any! */
    
    /* stabs, aka a.out, aka b.out directives for debug symbols.
       Currently ignored silently. */
{ "desc",	s_ignore,		0	},
{ "stabd",	s_stab_stub,		'd'	},
{ "stabn",	s_stab_stub,		'n'	},
{ "stabs",	s_stab_stub,		's'	},
    
    /* coff debugging directives.  Currently ignored silently */
{ "bb",		s_ignore,		0 },
{ "bf",		s_ignore,		0 },
{ "def",        s_ignore,		0 },
{ "dim",        s_ignore,		0 },
{ "eb",		s_ignore,		0 },
{ "ef",		s_ignore,		0 },
{ "endef",      s_ignore,		0 },
{ "line",	s_ignore,		0 },
{ "ln",         s_ignore,		0 },
{ "scl",        s_ignore,		0 },
{ "elf_size",   obj_elf_size,		0 },
{ "elf_type",   obj_elf_type,		0 },
{ "size",       s_ignore,		0 },
{ "tag",        s_ignore,		0 },
{ "type",       s_ignore,		0 },
{ "val",        s_ignore,		0 },
    
    /* other stuff */
{ "ABORT",      s_abort,		0 },
{ "ident", 	s_ignore,		0 },
{ "section", 	s_section, 		0 },
    
{ NULL}	/* end sentinel */
}; /* obj_pseudo_table */

/* For use in default filename in the absence of .file directive. */
extern char *physical_input_filename;

/* 80960 position-independence flags declared in tc_i960.c */
extern int 	pic_flag;
extern int 	pid_flag;
extern int 	link_pix_flag;

#ifdef	DEBUG
extern int	tot_instr_count;
extern int	mem_instr_count;
extern int	mema_to_memb_count;
extern int	FILE_run_count;
extern int	FILE_tot_instr_count;
extern int	FILE_mem_instr_count;
extern int	FILE_mema_to_memb_count;
extern char	*instr_count_file;
#endif

/* FIXME-SOMEDAY:  This is still needed in a few places in symbols.c.
 * There is a similar conversion system in obj_bout.c.
 * Leave it for now for backwards compatibility (pre-general-segments).
 */
const short seg_N_TYPE[] = { /* in: segT   out: N_TYPE bits */
	C_ABS_SECTION,
	C_TEXT_SECTION,
	C_DATA_SECTION,
	C_BSS_SECTION,
	C_UNDEF_SECTION,		/* SEG_UNKNOWN */
	C_UNDEF_SECTION,		/* SEG_ABSENT */
	C_UNDEF_SECTION,		/* SEG_PASS1 */
	C_UNDEF_SECTION,		/* SEG_GOOF */
	C_UNDEF_SECTION,		/* SEG_BIG */
	C_UNDEF_SECTION,		/* SEG_DIFFERENCE */
	C_DEBUG_SECTION,		/* SEG_DEBUG */
};


/* Add 4 to the real value to get the index and compensate the negatives */

const segT N_TYPE_seg [32] =
{
	SEG_GOOF,		/* Formerly NTV */
	SEG_GOOF,		/* Formerly PTV */
  SEG_DEBUG,			/* C_DEBUG_SECTION	== -2 */
  SEG_ABSOLUTE,			/* C_ABS_SECTION	== -1 */
  SEG_UNKNOWN,			/* C_UNDEF_SECTION	== 0 */
  SEG_TEXT,			/* C_TEXT_SECTION	== 1 */
  SEG_DATA,			/* C_DATA_SECTION	== 2 */
  SEG_BSS,			/* C_BSS_SECTION	== 3 */
  SEG_GOOF,SEG_GOOF,SEG_GOOF,SEG_GOOF,SEG_GOOF,SEG_GOOF,SEG_GOOF,SEG_GOOF,
  SEG_GOOF,SEG_GOOF,SEG_GOOF,SEG_GOOF,SEG_GOOF,SEG_GOOF,SEG_GOOF,SEG_GOOF,
  SEG_GOOF,SEG_GOOF,SEG_GOOF,SEG_GOOF,SEG_GOOF,SEG_GOOF,SEG_GOOF,SEG_GOOF
};
/* End FIXME-SOMEDAY backwards-compatibility block */


/* A table to wire in certain Elf section header fields based on the 
   section name.  This is accessed by elf_section_header_hook (only).
   Sections not listed here must have their type and flags fields set
   somewhere else.
   ATTN maintainers: the order of this table is somewhat arbitrary, 
   with an attempt made to put the most commonly-used names first. */
static struct scnhook scnhook_table[] =
{
{ ".text", SHT_PROGBITS, (SHF_ALLOC | SHF_EXECINSTR | SHF_960_READ), 0 },
{ ".data", SHT_PROGBITS, (SHF_ALLOC | SHF_WRITE | SHF_960_READ), 0 },
{ ".bss", SHT_NOBITS, (SHF_ALLOC | SHF_WRITE | SHF_960_READ), 0 },
{ ".rel", SHT_REL, 0, sizeof(Elf32_Rel) },
{ ".symtab", SHT_SYMTAB, 0, sizeof(Elf32_Sym) },
{ ".strtab", SHT_STRTAB, 0, 0 },
{ ".shstrtab", SHT_STRTAB, 0, 0 },
{ ".debug", SHT_PROGBITS, 0, 0 },
{ ".init", SHT_PROGBITS, (SHF_ALLOC | SHF_EXECINSTR | SHF_960_READ), 0 },
{ ".fini", SHT_PROGBITS, (SHF_ALLOC | SHF_EXECINSTR | SHF_960_READ), 0 },
{ ".rodata", SHT_PROGBITS, (SHF_ALLOC | SHF_960_READ), 0 },
{ ".note", SHT_NOTE, 0, 0 },
{ ".hash", SHT_HASH, (SHF_ALLOC | SHF_960_READ), 0 },
{ ".comment", SHT_PROGBITS, 0, 0 },
{ ".got", SHT_PROGBITS, 0, 0 },
{ ".interp", SHT_PROGBITS, 0, 0 },
{ ".plt", SHT_PROGBITS, 0, 0 },
{ ".dynamic", SHT_DYNAMIC, (SHF_ALLOC | SHF_960_READ), 0 },
{ ".dynstr", SHT_STRTAB, (SHF_ALLOC | SHF_960_READ), 0 },
{ ".dynsym", SHT_DYNSYM, (SHF_ALLOC | SHF_960_READ), 0 },
{ ".data1", SHT_PROGBITS, (SHF_ALLOC | SHF_WRITE | SHF_960_READ), 0 },
{ ".rodata1", SHT_PROGBITS, (SHF_ALLOC | SHF_960_READ), 0 },
{ ".960.intel.ccinfo", SHT_960_INTEL_CCINFO, 0, 0 },
{ NULL, 0, 0, 0 } 
};


/* ELF file generation & utilities */

void obj_symbol_new_hook(symbolP)
symbolS *symbolP;
{
}

void obj_read_begin_hook() 
{
}

/* Syntax: .elf_type {symbol_name} , {'function' | 'object'} */
static void obj_elf_type()
{
    char *symbol_name = input_line_pointer,c = get_symbol_end();

    if (strlen(symbol_name) && c == ',') {
	symbolS * symbolP = symbol_find_or_make(symbol_name);
	char *attribute = ++input_line_pointer;
	int attrib = 0;

	c = get_symbol_end();
	if (!strcmp("function",attribute))
		attrib = STT_FUNC;
	else if (!strcmp("object",attribute))
		attrib = STT_OBJECT;
	else
		as_bad("Syntax error: expected 'function' or 'object' after .elf_type symbolname,");
	S_SET_DATA_TYPE(symbolP,attrib);
	*input_line_pointer = c;
    }
    else
	    as_bad("Syntax error: expected symbol name and comma after .elf_type");
    demand_empty_rest_of_line();
}

typedef enum { elf_size_type_constant, elf_size_type_expression } elf_size_type;

typedef struct {
    elf_size_type es_type;
    union {
	unsigned long es_constant_value;
	char *es_expression;
    } es_value;
} elf_size_value;

static char *buynchars(s,n)
    char *s;
    int n;
{
    char *p = (char *) xmalloc(n+2);

    strncpy(p,s,n);
    p[n] = '\n';
    p[n+1] = 0;
    return p;
}

#define IS_COMMON_SYMBOL(SP) ((S_GET_SEGTYPE(SP) == SEG_UNKNOWN) && (S_GET_VALUE(SP) != 0))

/* Syntax: .elf_size {symbol_name} , {absolute_expression} */
static void obj_elf_size()
{
    char *symbol_name = input_line_pointer,c = get_symbol_end();

    if (strlen(symbol_name) && c == ',') {
	expressionS exp;
	int segment;
	segT segtype;
	char *save;
	symbolS *symbolP = symbol_find_or_make(symbol_name);
	
 	*input_line_pointer = c;
 	input_line_pointer++;
	save = input_line_pointer;
	segtype = expression(&exp);
	segment = exp.X_segment;
	switch (segtype) {
    case SEG_BIG:
	    as_bad("%s number is invalid for .elf_size",
		   exp . X_add_number > 0 ? "Bignum" : "Floating-Point");
	    break;
    case SEG_ABSENT:
	    as_bad(".elf_size: Missing expression");
	    break;
    case SEG_PASS1:
    case SEG_DIFFERENCE:
    case SEG_UNKNOWN:
    case SEG_ABSOLUTE:
	    if (!IS_COMMON_SYMBOL(symbolP)) {
		elf_size_value *t = (elf_size_value *) xmalloc(sizeof(elf_size_value));

		S_SET_SIZE(symbolP,(unsigned long) t);

		if (segtype == SEG_ABSOLUTE) {
		    t->es_type = elf_size_type_constant;
		    t->es_value.es_constant_value = exp.X_add_number;
		}
		else {
		    t->es_type = elf_size_type_expression;
		    t->es_value.es_expression = buynchars(save,input_line_pointer-save);
		}
	    }
	    break;
    default:
	    as_bad ("Illegal .elf_size expression");
	    break;
	}
    }
    else
 	    as_bad("Syntax error: expected symbol name and comma after .elf_size");
    demand_empty_rest_of_line();
}

static symbolS *section_lastP;

/*
 * The output symbol table will be ordered like this:
 *	section names (binding is STB_LOCAL)
 *	.file symbol
 *	other STB_LOCALs
 *	STB_GLOBALs
 *
 * Touch every symbol and do various things to them, notably:
 * 	(1) remove gas temp symbols from the list.
 *	(2) reorder the list as depicted above.
 *	(3) add frag starting address to symbol's value. 
 *	(4) assign a number (symtab index) to each symbol; used for relocation.
 */
void obj_crawl_symbol_chain(headers)
object_headers *headers;
{
    symbolS 	*sp;
    symbolS 	*symbol_extern_rootP = NULL;
    symbolS 	*symbol_extern_lastP = NULL;
    symbolS 	*symbol_undef_rootP = NULL;
    symbolS 	*symbol_undef_lastP = NULL;
    symbolS	*section_rootP = NULL;
    symbolS	*save;
    int	    	symnum, local_count = 0;

    section_lastP = NULL;
    /* Is there a .file symbol?  If not, add one at the start of the chain
       using the basename of the input file */
    if ( symbol_rootP == NULL || ! S_IS_DOTFILE(symbol_rootP) )
	elf_dot_file_symbol(physical_input_filename);

    /* Touch every symbol. */
    for ( sp = symbol_rootP; sp; sp = sp->sy_next, ++local_count )
    {
	/* Test that the symbol has a valid section number.  Note that the
	   error severity depends on the ORDER of the MYTHICAL_* macros
	   in segs.h */
	if (S_GET_SEGMENT(sp) <= MYTHICAL_GOOF_SEGMENT)
	{
	    if (flagseen ['Z'])
		as_warn ("Unrecognized section number %d for symbol %s", 
			 S_GET_SEGMENT(sp) - FIRST_PROGRAM_SEGMENT + 1, 
			 S_GET_PRTABLE_NAME(sp));
	    else
		as_bad ("Unrecognized section number: %d for symbol: %s", 
			  S_GET_SEGMENT(sp) - FIRST_PROGRAM_SEGMENT + 1, 
			  S_GET_PRTABLE_NAME(sp));
	}

	S_SET_VALUE(sp, S_GET_VALUE(sp) + sp->sy_frag->fr_address);

	if ( S_IS_SEGNAME(sp) )
	{
	    /* Preserve the chain, since sp's next and prev pointers are 
	       about to be changed. */
	    save = sp->sy_previous;
	    symbol_remove(sp, &symbol_rootP, &symbol_lastP);
	    symbol_append(sp, section_lastP, &section_rootP, &section_lastP);
	    /* Restore the original chain */
	    sp = save;
	}
	   
	/* FIXME: nip this COFF pollution in the bud, please. (SF_GET_LOCAL)  */
	if ( ! S_IS_DEFINED(sp) && ! SF_GET_LOCAL(sp) )
	{
	    S_SET_EXTERNAL(sp);
	} 
	/* FIXME: COFF pollution: SF_GET_LOCAL does not mean STB_LOCAL
	   binding in the Elf sense.  It means a gas local symbol, like
	   L12 or .I12 */
	if ( SF_GET_LOCAL(sp) )
	{
	    /* Remove from the chain permanently.  Preserve the chain,
	       since sp's next and prev pointers are about to be changed. */
	    save = sp->sy_previous;
	    symbol_remove(sp, &symbol_rootP, &symbol_lastP);
	    --local_count;
	    sp = save;
	} 
	else if ( S_IS_EXTERNAL(sp) )
	{	
	    /* Remove from the chain, but link into a separate chain
	       for use later.  Preserve the chain, since sp's next and
	       prev pointers are about to be changed. */
	    save = sp->sy_previous;
	    symbol_remove(sp, &symbol_rootP, &symbol_lastP);
	    if ( !S_IS_COMMON(sp)) {
		if (S_GET_SEGTYPE(sp) == SEG_UNKNOWN)
			symbol_append(sp, symbol_undef_lastP, &symbol_undef_rootP, &symbol_undef_lastP);
		else
			symbol_append(sp, symbol_extern_lastP, &symbol_extern_rootP, &symbol_extern_lastP);
		--local_count;
		sp = save;
	    }
	}
	else
	{
	    /* It's a real-live STB_LOCAL symbol.  local_count will get 
	       bumped once by the for loop.  Check if leafproc or sysproc,
	       in which case another symbol will pop out later. */
	    if ( TC_S_IS_LEAFPROC(sp) || TC_S_IS_SYSPROC(sp) )
		++local_count;
	}
    }
    
    /* OK.  Externs are removed from main list and have a list of their own.
       Same for section names.  Join the lists, making one long chain again. */
    if ( section_rootP )
    {
	/* Slip section names in before .file, locals and globals. */
	symbolS *tmp = symbol_rootP;
	symbol_rootP = section_rootP;
	section_rootP->sy_previous = NULL;   /* 1 */
	section_lastP->sy_next = tmp;        /* 2 */
	if ( tmp ) {
	    tmp->sy_previous = section_lastP;  /* 3 */
	    symbol_lastP->sy_next = NULL;      /* 4 */
	}
	else
	    symbol_lastP = section_lastP;
    }
    if ( symbol_extern_rootP )
    {
	symbol_extern_rootP->sy_previous = symbol_lastP;
	if ( symbol_lastP )
	    symbol_lastP->sy_next = symbol_extern_rootP;
	else
	    symbol_rootP = symbol_extern_rootP;
	symbol_lastP = symbol_extern_lastP;
    }
    if (1) {
	extern symbolS *top_of_comm_sym_chain();

	for (sp=top_of_comm_sym_chain();sp;sp=top_of_comm_sym_chain())
		symbol_append(sp, symbol_lastP, &symbol_rootP, &symbol_lastP);
    }
    if ( symbol_undef_rootP )
    {
	symbol_undef_rootP->sy_previous = symbol_lastP;
	if ( symbol_lastP )
	    symbol_lastP->sy_next = symbol_undef_rootP;
	else
	    symbol_rootP = symbol_undef_rootP;
	symbol_lastP = symbol_undef_lastP;
    }

    /* Rip through the list one more time setting the symbol number. */
    for ( sp = symbol_rootP, symnum = 1; sp; sp = sp->sy_next, ++symnum )
    {
	sp->sy_number = symnum;
	if ( TC_S_IS_LEAFPROC(sp) || TC_S_IS_SYSPROC(sp) )
	    /* There will be another symbol tagging along after this one */
	    ++symnum;
    }
}

/* Forward declaration: */

void output_file_align();


/* Write the .symtab section. 
 */
void obj_emit_symbols(dummy1, dummy2)
    symbolS *dummy1;	/* not used */
    object_headers *dummy2;	/* not used */
{
    symbolS *sp;
    symbolS *scn_hdr_rootP, *scn_hdr_tailP;
    int start_byte_count;
    int seg,one_past_locals_symbol_index;

    seg = seg_find(".symtab");  /* Section was created in write_elf_file */
    output_file_align();
    SEG_SET_SCNPTR(seg, output_byte_count);
    start_byte_count = output_byte_count;

    /* Assign final symbol values. */
    for ( sp = symbol_rootP; sp; sp = sp->sy_next )
    {
	if ( S_IS_SEGNAME(sp) )
	{
	    /* Must do this NOW, not in obj_crawl_symbol_chain, 
	       because many sections are not created until after
	       (e.g. all the .rel* sections.) */
	    S_SET_VALUE(sp, 0);
	    S_SET_DATA_TYPE(sp, STT_SECTION);
	}
	else
	{
	    /* Subtract off the section's address from the value.
	       (For relocatable files in Elf, symbol values are
	       section-relative.) */
	    unsigned long scn_start = 
		SEG_GET_FRAG_ROOT(S_GET_SEGMENT(sp)) ?
		    (SEG_GET_FRAG_ROOT(S_GET_SEGMENT(sp)))->fr_address :
			0;
	    S_SET_VALUE(sp, S_GET_VALUE(sp) - scn_start);
	}
    }
	   
    /* Emit symbols. */
    /* Always write a dummy symbol at index 0. */
    elf_write_symbol_0();

    for ( sp = symbol_rootP; sp; sp = sp->sy_next )
    {
	if (!S_IS_EXTERNAL(sp))
		one_past_locals_symbol_index = sp->sy_number + 1;

	if (S_IS_DOTFILE(sp))
		elf_write_dot_file(sp);
	else {
	    /* Translate the symbol's internal symbol info into Elf-specific
	       format. (the Elf-specific form is contained within *sp) */
	    elf_write_symbol(sp);

	    /* Write this one out */
	    output_file_append((char *) &sp->sy_symbol, sizeof(Elf32_Sym), out_file_name);
	    output_byte_count += sizeof(Elf32_Sym);
	
	    /* If one of the special cases, write the companion out now too */
	    if ( TC_S_IS_LEAFPROC(sp) || TC_S_IS_SYSPROC(sp) )
		{
		    sp = sp->sy_next;
		    output_file_append((char *) &sp->sy_symbol, sizeof(Elf32_Sym), out_file_name);
		    output_byte_count += sizeof(Elf32_Sym);
		}
	}
    }

    /* Fill in section size for .symtab section */
    SEG_SET_SIZE(seg, output_byte_count - start_byte_count);
    /* Fill in string table index (sh_link field) */
    SEG_SET_LINK(seg, string_table_index);
    /* Fill in "first extern symbol index" (sh_info field) Actually, this is
       "one number beyond last local symbol index" to be precise */
    SEG_SET_INFO(seg, one_past_locals_symbol_index);
}


/* Write the .strtab section.
   Sets the sh_name field of all symbols still in the symbol chain.
   This has to be done before obj_emit_symtab. */
void obj_emit_strings()
{
    int strtab_index; /* The name field in an Elf symbol is an index
			 into the string table we are about to write. */
    symbolS *sp = symbol_rootP;
    int seg = seg_find(".strtab");  /* Section was created in write_elf_file */
    SEG_SET_SCNPTR(seg, output_byte_count);

    /* Write a 0 for mythical string 0. */
    output_file_append("", 1, out_file_name);

    /* Write all the symbol names out to the file. Fill in the symbols' 
       name field (index into the .strtab section) as you go. */
    for ( strtab_index = 1; sp; sp = sp->sy_next )
    {
	if ( sp->sy_name && *sp->sy_name )
	{
	    output_file_append(sp->sy_name, strlen(sp->sy_name) + 1, out_file_name);
	    sp->sy_symbol.st_name = strtab_index;
	    strtab_index += strlen(sp->sy_name) + 1;
	}
	else
	    sp->sy_symbol.st_name = 0;
    }
    /* Fill in section size for .strtab section */
    SEG_SET_SIZE(seg, strtab_index);
    output_byte_count += strtab_index;
}

/* Write the .shstrtab section. */
void obj_emit_section_strings()
{
    int shstrtab_index; /* The name field in an Elf section header is an index
			   into the string table we are about to write. */
    int scnstr_seg;	/* Segment number of the section string table */
    int seg;		/* Index into segs array */

    /* Create a new section and add to the section list.  Add its section
       number to the Elf header. */
    scnstr_seg = seg_find(".shstrtab");  /* Section was created in write_elf_file */
    SEG_SET_SCNPTR(scnstr_seg, output_byte_count);
    headers.elfhdr.e_shstrndx = section_string_table_index;

    /* Write a 0 for name of mythical section 0. */
    output_file_append("", 1, out_file_name);

    /* Write all the section names out to the file. Fill in the section headers'
       name field (index into the .shstrtab section) as you go. */
    for ( seg = FIRST_PROGRAM_SEGMENT, shstrtab_index = 1; seg < segs_size; ++seg )
    {
	segS *sp = & segs[seg];
	output_file_append(sp->seg_name, strlen(sp->seg_name) + 1, out_file_name);
	sp->seg_elf_name = shstrtab_index;
	shstrtab_index += strlen(sp->seg_name) + 1;
    }
    /* Fill in section size for .shstrtab section */
    SEG_SET_SIZE(scnstr_seg, shstrtab_index);
    output_byte_count += shstrtab_index;
}


static int has_relocs(fixP)
    fixS *fixP;
{
    for ( ; fixP;  fixP = fixP->fx_next )
	    if ( fixP->fx_addsy || fixP->fx_subsy )
		    return 1;
    return 0;
}

/*
 * Crawl along a fixS chain, emitting relocations for one section.
 * NOTE: each fixS may emit up to 2 relocations.  So process both 
 * the "add" symbol and the "subtract" symbol for each one.
 */
void obj_emit_relocations(fixP, rawseg)
    fixS *fixP; /* Fixup chain for this segment. */
    unsigned long rawseg; /* Segment number of program image segment you are 
			     writing relocations for */
{
    Elf32_Rel elfrel;	/* Elf-specific relocation structure */
    Elf32_Rel elfextra;	/* Extra relocation for callj and calljx */
    int pos_saved = 0;
    long start_byte_count;

    for ( ; fixP;  fixP = fixP->fx_next ) 
    {
	while ( fixP->fx_addsy || fixP->fx_subsy )
	{
	    if (!pos_saved) {
		output_file_align();
		start_byte_count = output_byte_count; /* Save starting file pos */
		pos_saved = 1;
	    }
	    /* translate into elf-specific format */
	    elf_write_relocation(fixP, rawseg, &elfrel);
	    output_file_append ((char *) &elfrel, sizeof(Elf32_Rel), out_file_name);
	    output_byte_count += sizeof(Elf32_Rel);
	    if ( fixP->fx_addsy )
		/* Next time process the subsy if any */
		fixP->fx_addsy = NULL;
	    else if ( fixP->fx_subsy )
		/* You're done with this fixS */
		fixP->fx_subsy = NULL;
	}
    }
    if ( pos_saved )
    {
	/* We wrote at least one relocation; write some info into scn header.
	   Naming is dictated by the Elf spec: ".rel" + section_name where
	   section_name is the section you are writing relocs for */
	int seg;
	char *segname = (char *) xmalloc(strlen(SEG_GET_NAME(rawseg)) + 5);
	strcpy(segname, ".rel");
	strcat(segname, SEG_GET_NAME(rawseg));
	seg = seg_find(segname);  /* Section was created in write_elf_file */
	free(segname);
	SEG_SET_SCNPTR(seg, start_byte_count);
	SEG_SET_SIZE(seg, output_byte_count - start_byte_count);

	/* Punch in some special fields required for reloction sections */
	/* Section header table index for symbol table */
	SEG_SET_LINK(seg, symbol_table_index);
	/* Section header table index for the program image section 
	   that we are writing relocs for */
	SEG_SET_INFO(seg, rawseg - FIRST_PROGRAM_SEGMENT + 1);
    }
}

/*
 * Write an OMF-dependent object file to the disk file.  
 * Do lots of other bookkeeping stuff.
 * The call to this in the source will be "write_object_file()".
 */
void write_elf_file() 
{
    int  		seg;		/* Track along all segments. */
    fragS 		*fragP;		/* Track along all frags. */
    unsigned long 	vaddr; 		/* Virtual addr of start of section */
    unsigned long	slide; 		/* Temp for address calculations */
    
    /*
     * We have several user segments. Do one last frag_new (yes, it's wasted)
     * to close off the fixed part of the last frag.  
     *
     * Then, relax addresses in each segment.
     * Since you now finally know exactly how big this section will be, 
     * set its size field.
     */
    frag_wane (curr_frag);
    frag_new (0);
    for ( seg = FIRST_PROGRAM_SEGMENT; seg < segs_size; ++seg )
    {
	seg_change ( seg );
	relax_segment ( seg );
	
	/* Size setting depends on there being an empty last frag */
	know( SEG_GET_FRAG_LAST(seg) );
	know( SEG_GET_FRAG_LAST(seg)->fr_type == rs_fill && SEG_GET_FRAG_LAST(seg)->fr_offset == 0 );
	
	/* Size setting for the one-and-only bss segment is special. 
	 * Do an extra seg_change to make sure there is both a root 
	 * AND a last frag (both empty) 
	 */
	if ( seg == DEFAULT_BSS_SEGMENT )
	{
	    seg_change (seg);
	    SEG_GET_FRAG_LAST(seg)->fr_address = SEG_GET_SIZE(DEFAULT_BSS_SEGMENT);
	}
#if 0
	We used to 'round up' the sizes of sections so that they are a multiple of 2^2:

	SEG_SET_SIZE(seg, md_segment_align((long) SEG_GET_FRAG_LAST(seg)->fr_address, 2));
#endif
	SEG_SET_SIZE(seg, SEG_GET_FRAG_LAST(seg)->fr_address);
    }
    
    /*
     * Now the addresses of frags are correct within each segment.
     * Join all user segments into 1 huge segment.  We won't actually
     * connect the links into a linked list, but we fix up frag addresses
     * as if we had.
     * NOTE on aligning sections:  The section's starting address must be
     * aligned on a word boundary. (this is for compatibility with COFF
     * assembler, not for any technical reason.)
     */
    for ( slide=0, seg = FIRST_PROGRAM_SEGMENT; seg <= segs_size - 1; ++seg ) {
	if (SEG_GET_FLAGS(seg) & SECT_ATTR_ALLOC) {
	    slide = md_segment_align (slide, SEG_GET_ALIGN(seg) >= 2 ? SEG_GET_ALIGN(seg) : 2);

	    /* Now adjust all the frag addresses in the NEXT segment. */
	    for ( fragP = SEG_GET_FRAG_ROOT(seg); fragP; fragP = fragP->fr_next )
		    fragP->fr_address += slide;
	    slide += SEG_GET_SIZE(seg);
	}
    } /* for each program segment except the last one */

    /* 
     * Create special Elf sections NOW, before they are really needed.
     * This makes it easier to group their symbols together in the 
     * symbol table.
     */
    if (i960_ccinfo_size())
	    seg_new(SEG_DEBUG, ".960.intel.ccinfo", 0) - FIRST_PROGRAM_SEGMENT + 1;

    /* Create special sections always needed. */
    /* Save string table index for use by symbol table writer,
       symbol table index for use by relocation sections,
       and section string table index for use by Elf header. */
    section_string_table_index = 
	seg_new(SEG_DEBUG, ".shstrtab", 0) - FIRST_PROGRAM_SEGMENT + 1;
    string_table_index = 
	seg_new(SEG_DEBUG, ".strtab", 0) - FIRST_PROGRAM_SEGMENT + 1;
    symbol_table_index = 
	seg_new(SEG_DEBUG, ".symtab", 0) - FIRST_PROGRAM_SEGMENT + 1;

    /*
     * Scan the frags, converting any ".org"s and ".align"s to ".fill"s.
     * Also convert any machine-dependent frags using md_convert_frag();
     */
    
    for ( seg = FIRST_PROGRAM_SEGMENT; seg < segs_size; ++seg )
    {
	seg_change(seg);
	for (fragP = SEG_GET_FRAG_ROOT(seg);  fragP;  fragP = fragP->fr_next) 
	{
	    switch (fragP->fr_type) 
	    {
	    case rs_align:
	    case rs_org:
		/* PS: 6/4/93 Some trickery here:
		 * Currently, fr_var is always 1, UNLESS we have
		 * a special alignfill type rs_align, in which case 
		 * fr_var will be negative.  In that case we handle 
		 * fr_offset as if fr_var were 1 and then deal 
		 * with it when we emit code.  Otherwise, don't assume
		 * that fr_var is 1 because it doesn't have to be.
		 */
		fragP->fr_type = rs_fill;
		fragP->fr_offset = fragP->fr_next->fr_address - fragP->fr_address - fragP->fr_fix;
		if ( fragP->fr_var > 0 )
		    fragP->fr_offset /= fragP->fr_var;
		break;
		
	    case rs_fill:
		break;
		
	    case rs_machine_dependent:
		md_convert_frag (fragP);
		/*
		 * After md_convert_frag, we make the frag into a ".space 0".
		 * Md_convert_frag() should set up any fixSs and constants
		 * required.
		 */
		frag_wane (fragP);
		break;
		
	    default:
		BAD_CASE( fragP->fr_type );
		break;
	    } /* switch (fr_type) */
	} /* for each frag. */
    } /* for each segment. */
    
    /*
     * Now move along the symbol list doing the following:
     * 	- Remove temp symbols from the list
     *	- Count symbols and assign each one a number
     *  - Assign symbol values
     *  - Reorder the list so that locals come out first, then 
     *	  section names, then globals
     */
    
    obj_crawl_symbol_chain(&headers);

    md_check_leafproc_list();

    /* 
     * Fix any .set expressions that could not be evaluated during 
     * the first pass. (do_delayed_evaluation is in write.c)
     */
    do_delayed_evaluation();

    /* 
     * Scan the relocations for each segment performing as many fixups
     * as can be done now.  Remove those from the relocation list.
     */
    for ( seg = FIRST_PROGRAM_SEGMENT; seg < segs_size; ++seg )
    {
	if ( SEG_GET_TYPE(seg) == SEG_TEXT || SEG_GET_TYPE(seg) == SEG_DATA )
	{
	    fixup_segment(seg);
	}
    }
    
    if (1) {
	symbolS *former_last = symbol_lastP;

	for ( seg = FIRST_PROGRAM_SEGMENT; seg < segs_size; ++seg ) {
	    if ( has_relocs(SEG_GET_FIX_ROOT(seg)) ) {
		/* This error should never occur as in the rest of the
		   assembler, guards are made against allowing fixups
		   for BSS type sections.  However, ... just in case,
		   ... this is left here. */
		if (SEG_IS_BSS(seg))
			as_bad("Relocations found for bss-type section");
		else {
		    char *segname = (char *) malloc(strlen(SEG_GET_NAME(seg)) + 5);
		    strcpy(segname, ".rel");
		    strcat(segname, SEG_GET_NAME(seg));
		    seg_new(SEG_DEBUG, segname, 0);
		}
	    }
	}

	if (former_last != symbol_lastP) {
	    /* We have added relocation sections and the current symbol table is hosed.
	       The current symbol table looks like this, with the indicated variables
	       pointing at the indicated locations:

	       section1_sym       <- symbol_rootP
	       section2_sym
	       section3_sym
	       ...
	       sectionN_sym       <- section_lastP
	       FILE SYMBOL        <-----+
	       [locals (if any)]  <--+  |
	       [globals (if any)] <- former_last (may point at the FILE SYMBOL, or the last local
	                             or the last global).
	       .rel_section1_sym
	       .rel_section2_sym
	       .rel_section3_sym
	       ...
	       .rel_sectionN_sym  <- symbol_lastP

	       We need to move the .rel_section*_sym's from their current position to the tail
	       of the other section*_sym's (first in the list).

	       Then once done, we need to renumber all of the symbols. */

	    symbolS *end_of_new_dot_rel_section_syms       = symbol_lastP;
	    symbolS *start_of_new_dot_rel_section_syms     = former_last->sy_next;
	    symbolS *file_symbol                           = section_lastP->sy_next;
	    symbolS *new_last                              = former_last;
	    symbolS *sp;
	    int i;

	    end_of_new_dot_rel_section_syms->sy_next       = file_symbol;                       /* 1 */
	    file_symbol->sy_previous                       = end_of_new_dot_rel_section_syms;   /* 2 */
	    start_of_new_dot_rel_section_syms->sy_previous = section_lastP;                     /* 3 */
	    section_lastP->sy_next                         = start_of_new_dot_rel_section_syms; /* 4 */
	    section_lastP                                  = end_of_new_dot_rel_section_syms;
	    new_last->sy_next                              = NULL;
	    symbol_lastP                                   = new_last;

	    for (i=1,sp=symbol_rootP;sp;sp = sp->sy_next,i++) {
		sp->sy_number = i;
		if ( TC_S_IS_LEAFPROC(sp) || TC_S_IS_SYSPROC(sp) )
			i++;
	    }
	}
    }

    /* 
     * Only generate an output file if there were no errors; UNLESS
     * the -Z flag was given telling us to go ahead and try to create
     * an output file.
     */
    if ( had_errors() ) 
    {
	if ( flagseen['Z'] )
	    fprintf (stderr, "Warning: %d error%s, %d warning%s, generating bad object file.\n",
		     had_errors(), had_errors() == 1 ? "" : "s",
		     had_warnings(), had_warnings() == 1 ? "" : "s");
	else
	    exit ( had_errors() );
    } /* on error condition */
    
    /* 
     * Open the output file and start emitting program raw data.  Skip
     * enough space to fit the Elf header at the beginning of the file.
     */
    output_file_create(out_file_name);
    output_byte_count = sizeof(Elf32_Ehdr);
    output_file_seek(output_byte_count, out_file_name);
    
    for ( seg = FIRST_PROGRAM_SEGMENT; seg < segs_size; ++seg )
    {
	SEG_SET_SCNPTR(seg, output_byte_count);
	for (fragP = SEG_GET_FRAG_ROOT(seg);  fragP;  fragP = fragP->fr_next) 
	{
	    register long		count;
	    register char *		fill_literal;
	    register long		fill_size;
	    
	    know( fragP->fr_type == rs_fill );
	    /* Do the fixed part. */
	    if (SEG_IS_BSS(seg))
		    MAKE_SURE_ZERO(fragP->fr_literal,fragP->fr_fix);
	    else {
		output_file_append (fragP->fr_literal, fragP->fr_fix, out_file_name);
		output_byte_count += fragP->fr_fix;
	    }
	    
	    /* Do the variable part(s). */
	    fill_literal = fragP->fr_literal + fragP->fr_fix;
	    if ( fragP->fr_var < 0 )
	    {
		/* If fr_var is negative, it is a flag from 
		 * s_alignfill() that we have the special form
		 * of an rs_align here.  We have to completely 
		 * fill the alignment gap, which is not always
		 * an even multiple of the fill pattern size.
		 */
		fill_size = 0 - fragP->fr_var;
		for ( count = fragP->fr_offset;  count > 0;  count -= fill_size )
		{
		    if (SEG_IS_BSS(seg))
			    MAKE_SURE_ZERO(fill_literal,count > fill_size ? fill_size : count);
		    else {
			output_file_append (fill_literal, 
					    count > fill_size ? fill_size : count,
					    out_file_name);
			output_byte_count += (count > fill_size ? fill_size : count);
		    }
		}
	    }
	    else
	    {
		/* The normal case.  fr_offset is the repeat 
		 * factor for a memory chunk fr_var bytes long.
		 */
		fill_size = fragP->fr_var;
		for (count = fragP->fr_offset;  count;  count --)
		{
		    if (SEG_IS_BSS(seg))
			    MAKE_SURE_ZERO(fill_literal,fill_size);
		    else {
			output_file_append (fill_literal, fill_size, out_file_name);
			output_byte_count += fill_size;
		    }
		}
	    }
	} /* for each frag in this segment. */
    } /* for each segment. */
    
    if (i960_ccinfo_size()) {
	int seg = seg_find(".960.intel.ccinfo");

	SEG_SET_SCNPTR(seg,output_byte_count);

	i960_emit_ccinfo();
	SEG_SET_SIZE(seg,i960_ccinfo_size());
	output_byte_count += i960_ccinfo_size();
    }

    /* Write out .shstrtab (section header strings).  */
    obj_emit_section_strings();

    /* Write out .strtab (program strings) */
    obj_emit_strings();

    /* Write out .symtab. */
    obj_emit_symbols(symbol_rootP, NULL);

    /* Write out the relocations.  Fill in section headers as you go. */
    for ( seg = FIRST_PROGRAM_SEGMENT; seg < segs_size; ++seg )
    {
	/* FIXME: make sure this test also works with Dwarf sections. */
	if ( SEG_GET_TYPE(seg) == SEG_TEXT || SEG_GET_TYPE(seg) == SEG_DATA )
	    obj_emit_relocations(SEG_GET_FIX_ROOT(seg), seg);
    }

    /* Write out the section headers.  Include mythical section 0.
       Set the seg_vaddr and seg_align fields of all sections.
       Save the current file position in the Elf header.  */
    output_file_align();
    headers.elfhdr.e_shoff = output_byte_count;
    elf_write_section_0();

    for ( seg = FIRST_PROGRAM_SEGMENT; seg < segs_size; ++seg )
    {
	static void elf_write_scnhdr();
	Elf32_Shdr shdr; /* Elf-specific section header */
	unsigned long seg_align;

	SEG_SET_VADDR(seg, SEG_GET_FRAG_ROOT(seg) ? SEG_GET_FRAG_ROOT(seg)->fr_address : 0);
	/* transform power-of-2 alignment to a number alignment */
	if ( SEG_GET_ALIGN(seg) )
		seg_align = md_segment_align(1, SEG_GET_ALIGN(seg));
	else
		seg_align = 0;
	elf_write_scnhdr(seg, &shdr, seg_align);
 	output_file_append ((char *) &shdr, sizeof(Elf32_Shdr), out_file_name);
	output_byte_count += sizeof(Elf32_Shdr);
    }
    
    /* Finally, write the Elf header at the start of the file. */
    elf_headers_hook(&headers);
    tc_elf_headers_hook(&headers);
    
    output_file_seek (0L, out_file_name);
    output_file_append ((char *) &headers.elfhdr, sizeof headers.elfhdr, out_file_name);
    output_file_close(out_file_name);
}


/* Align each output section so that it will start on a word boundary.
 * Emit zeros if needed to push the next byte emitted to a word boundary.
 * Adjust global output_byte_count if zeros were emitted.
 */
void output_file_align()
{
    int	zero = 0;
    long 	slide = md_segment_align(output_byte_count, 2) - output_byte_count;
    output_byte_count += slide;
    for ( ; slide; --slide )
    {
	output_file_append((char *) &zero, 1L, out_file_name);
    }
}

#define BIG_ENDIAN 1
#define LITTLE_ENDIAN 0

/* Write Elf-specific info into the Elf header. */
elf_headers_hook(headers)
    object_headers *headers;
{
    headers->elfhdr.e_ident[EI_MAG0] = ELFMAG0;
    headers->elfhdr.e_ident[EI_MAG1] = ELFMAG1;
    headers->elfhdr.e_ident[EI_MAG2] = ELFMAG2;
    headers->elfhdr.e_ident[EI_MAG3] = ELFMAG3;
    headers->elfhdr.e_ident[EI_CLASS] = ELFCLASS32;

    if ( host_byte_order() == BIG_ENDIAN )
	headers->elfhdr.e_ident[EI_DATA] = ELFDATA2MSB;
    else
	headers->elfhdr.e_ident[EI_DATA] = ELFDATA2LSB;
    headers->elfhdr.e_ident[EI_VERSION] = EV_CURRENT;
    headers->elfhdr.e_type = ET_REL;
    headers->elfhdr.e_machine = EM_960;
    headers->elfhdr.e_version = EV_CURRENT;
    headers->elfhdr.e_entry = 0;
    headers->elfhdr.e_phoff = 0;
    headers->elfhdr.e_phnum = 0;
    headers->elfhdr.e_ehsize = sizeof(Elf32_Ehdr);
    headers->elfhdr.e_phentsize = sizeof(Elf32_Phdr);
    headers->elfhdr.e_shentsize = sizeof(Elf32_Shdr);
    headers->elfhdr.e_shnum = segs_size - FIRST_PROGRAM_SEGMENT + 1; 
}

host_byte_order()
{
    char tmp[4];
    unsigned long ul;
    tmp[0] = 0;
    tmp[1] = 1;
    tmp[2] = 2;
    tmp[3] = 3;
    ul = *((unsigned long *) tmp);
    if ( ul == 0x10203 )
	return BIG_ENDIAN;
    else if ( ul == 0x3020100 )
	return LITTLE_ENDIAN;
    else
	as_fatal("Internal error: Can't determine host byte order.\n");
}

/* Every elf program has an empty section 0. */
elf_write_section_0()
{
    Elf32_Shdr shdr; /* Elf-specific section header */
    memset((char *) &shdr, 0, sizeof(Elf32_Shdr));
    output_file_append ((char *) &shdr, sizeof(Elf32_Shdr), out_file_name);
    output_byte_count += sizeof(Elf32_Shdr);
}

/* Every elf symbol table has an empty symbol at index 0. */
elf_write_symbol_0()
{
    Elf32_Sym sym; /* Elf-specific symbol */
    memset((char *) &sym, 0, sizeof(Elf32_Sym));
    output_file_append ((char *) &sym, sizeof(Elf32_Sym), out_file_name);
    output_byte_count += sizeof(Elf32_Sym);
}

/* Special handling for .file. */
static elf_write_dot_file(sp)
    symbolS *sp;
{
    S_SET_ELF_SECTION(sp, SHN_ABS);
    S_SET_DATA_TYPE(sp, STT_FILE);
    S_SET_STORAGE_CLASS(sp, STB_LOCAL);
    output_file_append ((char *) &sp->sy_symbol, sizeof(Elf32_Sym), out_file_name);
    output_byte_count += sizeof(Elf32_Sym);
}
    
/* translate an internal, generic section header into ELF-specific format. 
*/
static void elf_write_scnhdr(seg, ep, seg_align)
    int seg;   	    /* index into segs[] array */
    Elf32_Shdr *ep;     /* ELF-specific section header struct */
    unsigned long seg_align;
{
    struct segment *sp = &segs[seg];

    /* Do not assume that ELF fields have been zeroed */
    memset((char *) ep, 0, sizeof(Elf32_Shdr));

    ep->sh_name = sp->seg_elf_name;
    ep->sh_addr = sp->seg_vaddr;	/* NOTE: seg_paddr is not used */
    ep->sh_offset = sp->seg_scnptr;
    ep->sh_size = sp->seg_size;
    ep->sh_link = sp->seg_link;
    ep->sh_info = sp->seg_info;
    if (!(ep->sh_addralign = seg_align)) {
	/* If the user did not use .align for this section we set default alignments of
	   CODE, DATA we set it to word alignment and BSS sections to quadword alignment: */
	if (((SEG_GET_FLAGS(seg) & SECT_ATTR_DATA) == SECT_ATTR_DATA) ||
	    ((SEG_GET_FLAGS(seg) & SECT_ATTR_TEXT) == SECT_ATTR_TEXT))
		ep->sh_addralign = 4;
	else if ((SEG_GET_FLAGS(seg) & SECT_ATTR_BSS) == SECT_ATTR_BSS)
		ep->sh_addralign = 16;
    }
    else if (((SEG_GET_FLAGS(seg) & SECT_ATTR_BSS) == SECT_ATTR_BSS) &&
	     ep->sh_addralign < 16)
	    ep->sh_addralign = 16;

    switch (SEG_GET_TYPE(seg)) {
 case SEG_TEXT:
 case SEG_DATA:
	ep->sh_type = SHT_PROGBITS;
    }

    if ((sp->seg_flags & SECT_ATTR_BSS) == SECT_ATTR_BSS)
	    ep->sh_type = SHT_NOBITS;

    if (1) {
	static struct attrib_map_to_elf_flags {
	    unsigned long sect_attr,elf_sect_flag;
	} attribute_map[] = {
        {SECT_ATTR_READ,        SHF_960_READ},
	{SECT_ATTR_WRITE,       SHF_WRITE},
        {SECT_ATTR_EXEC,        SHF_EXECINSTR},
        {SECT_ATTR_SUPER_READ,  SHF_960_SUPER_READ},
        {SECT_ATTR_SUPER_WRITE, SHF_960_SUPER_WRITE},
        {SECT_ATTR_SUPER_EXEC,  SHF_960_SUPER_EXECINSTR},
        {SECT_ATTR_ALLOC,       SHF_ALLOC},
        {SECT_ATTR_MSB,         SHF_960_MSB},
        {SECT_ATTR_PI,          SHF_960_PI},
        {SECT_ATTR_BSS,        (SHF_WRITE|SHF_ALLOC|SHF_960_READ)},
        {0,                     0} };
	struct attrib_map_to_elf_flags *t = attribute_map;

	for (;t->sect_attr;t++)
		if ((sp->seg_flags & t->sect_attr) == t->sect_attr)
			ep->sh_flags |= t->elf_sect_flag;
    }

    if (link_pix_flag &&
	(((sp->seg_flags & SECT_ATTR_TEXT) == SECT_ATTR_TEXT) ||
	((sp->seg_flags & SECT_ATTR_DATA) == SECT_ATTR_DATA)))
	    ep->sh_flags |= SHF_960_LINK_PIX;

    /* Fill in some fields if this is a special section (do nothing if not) */
    elf_section_header_name_hook(ep, sp->seg_name);
    
    /* Make sure this section's SHF_960_MSB flag is set.  
       The byte order of symbol table and relocation sections tracks the byte-order of
       the host system.  The string table has no byte order. */
    if ( ep->sh_type == SHT_SYMTAB || ep->sh_type == SHT_REL )
	    ep->sh_flags |= ((host_byte_order() == BIG_ENDIAN) ? SHF_960_MSB : 0);
}

/* Certain section names in elf are special.  Use the table initialized
   above to set the section type field and the section flags field. */
elf_section_header_name_hook(ep, name)
    Elf32_Shdr *ep; /* Elf section header */
    char *name; /* Section name */
{
    struct scnhook *sp = scnhook_table;
    while ( sp->name && strncmp(sp->name, name, strlen(sp->name)) )
	++sp;
    if ( sp->name )
    {
	ep->sh_type = sp->type;
	ep->sh_entsize = sp->entsize;
	ep->sh_flags |= sp->flags;
    }
}

/* 
 * Translate a generic, internal symbol into Elf-specific format. 
 */
elf_write_symbol(sp)
    symbolS *sp;
{
    /* Set the elf size st_size first. */
    if (!IS_COMMON_SYMBOL(sp) && S_GET_SIZE(sp)) {
	elf_size_value *t = (elf_size_value *) S_GET_SIZE(sp);

	if (t->es_type == elf_size_type_constant)
		S_SET_SIZE(sp,t->es_value.es_constant_value);
	else {
	    expressionS exp;
	    segT segtype;
	
	    input_line_pointer = t->es_value.es_expression;
	    bzero(&exp,sizeof(exp));
	    need_pseudo_pass_2 = 1;
	    segtype = expression(&exp);
	    switch (segtype) {
	case SEG_ABSOLUTE:
		S_SET_SIZE(sp,exp.X_add_number);
		break;
	default:
	    as_bad ("Illegal .elf_size expression.");
		break;
	    }
	}
    }

#if 0
    /* FIXME?  Should we emit the size of the section into the symbol's
       sh_size field?  If so, we have to find the correct size for .rel
       sections, and .symtab sections (which does not appear to be
       available here). */
    if (S_IS_SEGNAME(sp))
	    S_SET_SIZE(sp,segs[sp->sy_segment].seg_size);
#endif

    switch ( S_GET_SEGTYPE(sp) )
    {
    case SEG_ABSOLUTE:
	S_SET_ELF_SECTION(sp, SHN_ABS);
	break;
    case SEG_DEBUG:
	/* This is one of the special sections created in write_elf_file */
	if ( ! S_IS_SEGNAME(sp) )
	    as_fatal("Internal error: elf_write_symbol: DEBUG section remains in symbol list");
	/* Intentional fallthrough for segnames */
    case SEG_TEXT:
    case SEG_DATA:
    case SEG_BSS:
	S_SET_ELF_SECTION(sp, S_GET_SEGMENT(sp) - FIRST_PROGRAM_SEGMENT + 1);
	break;
    case SEG_UNKNOWN:
	if ( S_GET_VALUE(sp) ) {
	    /* Common symbol; the value is the address alignment */
	    S_SET_ELF_SECTION(sp, SHN_COMMON);
	    /* The user specified no alignment.  So we reset it
	       now to 0. */
	    if (S_GET_VALUE(sp) == ELF_DFLT_COMMON_ALIGNMENT)
		    S_SET_VALUE(sp, 0);
	}
	else
	    /* Undefined external */
	    S_SET_ELF_SECTION(sp, SHN_UNDEF);
	break;
    default:
	/* If a bogus section made it this far then user has given -Z flag;
	   So let it go through; see obj_crawl_symbol_chain */
	; 
    }

    /* For leafprocs, create phony symbol that gets emitted immediately
       following the call entry point that contains the bal entry point.
       Put it in a special section to alert the linker to it. */
    if ( TC_S_IS_LEAFPROC(sp) )
    {
	symbolS *dummy = (symbolS *) xmalloc(sizeof(symbolS));
	symbolS *balp = symbol_find(TC_S_GET_BALNAME(sp));
	memset((char *) dummy, 0, sizeof(symbolS));
	S_SET_ELF_SECTION(dummy, sp->sy_symbol.st_shndx);
	S_SET_STORAGE_CLASS(dummy, S_GET_STORAGE_CLASS(sp));
	S_SET_VALUE(dummy, balp ? S_GET_VALUE(balp) : S_GET_VALUE(sp));
	S_SET_ST_OTHER(sp,STO_960_HAS_LEAFPROC);
	S_SET_ST_OTHER(dummy,STO_960_IS_LEAFPROC);
	symbol_append(dummy, sp, &symbol_rootP, &symbol_lastP);
    }
    
    /* For sysprocs, create phony symbol that gets emitted immediately
       following the sysproc symbol that contains the sysproc index.
       Put it in a special section to alert the linker to it. */
    if ( TC_S_IS_SYSPROC(sp) )
    {
	symbolS *dummy = (symbolS *) xmalloc(sizeof(symbolS));
	memset((char *) dummy, 0, sizeof(symbolS));
	S_SET_ELF_SECTION(dummy, SHN_ABS);
	S_SET_STORAGE_CLASS(dummy, S_GET_STORAGE_CLASS(sp));
	S_SET_VALUE(dummy, TC_S_GET_SYSINDEX(sp));
	S_SET_ST_OTHER(sp,STO_960_HAS_SYSPROC);
	S_SET_ST_OTHER(dummy,STO_960_IS_SYSPROC);
	symbol_append(dummy, sp, &symbol_rootP, &symbol_lastP);
    }
}

/*
 * Translate one relocation structure from internal gas structure into
 * Elf-specific format.  You are processing either an "add" relocation
 * or a "subtract" relocation; you can tell which it is from the passed-in
 * fixS structure.
 * NOTE on offset: this is the offset within the section, not a virtual address.
 */
elf_write_relocation(fixP, seg, ep)
    fixS *fixP;		/* Internal format */
    unsigned long seg;  /* What segment are we writing relocations for? */
    Elf32_Rel *ep;	/* Elf-specific format */
{
    symbolS *sp;

    /* Don't assume that the elf structure has been cleared */
    memset((char *) ep, 0, sizeof(Elf32_Rel));
    
    sp = fixP->fx_addsy ? fixP->fx_addsy : fixP->fx_subsy;

    R_SET_OFFSET(ep, fixP->fx_frag->fr_address + fixP->fx_where - SEG_GET_FRAG_ROOT(seg)->fr_address);
    if (fixP->fx_callj) 
    {
	/* Use OPTCALL relocation type.  This tells the linker to do
	   an R_960_IP24 and a call -> bal opcode transformation 
	   all in one step. */
	R_SET_TYPE(ep, R_960_OPTCALL);
	R_SET_SYMBOL(ep, sp->sy_number);
    }
    else if (fixP->fx_calljx)
    {
	/* Use OPTCALLX relocation type.  This tells the linker to do
	   an R_960_32 and a callx -> balx opcode transformation 
	   all in one step.  NOTE: Move address of relocation back 
	   4 bytes since the relocation originally pointed to the 
	   second word. */
	if ( SF_GET_LOCAL(sp) )
	{
	    /* Can't currently do a calljx to a local label */
	    as_bad ("Calljx to local label not allowed; use callj\n");
	}
	R_SET_TYPE(ep, R_960_OPTCALLX);
	R_SET_OFFSET(ep, R_GET_OFFSET(ep) - 4);
	R_SET_SYMBOL(ep, sp->sy_number);
    }
    else 
    {
	/* Not leafproc or sysproc relocation. */
	if ( fixP->fx_pcrel )
	    R_SET_TYPE(ep, R_960_IP24);
	else if ( fixP->fx_size )
	{
	    /* NOTE: the following logic depends on the add symbol being
	       processed FIRST when there are both add and sub symbols. */
	    R_SET_TYPE(ep, fixP->fx_addsy ? R_960_32 : R_960_SUB);
	    if ( fixP->fx_size < sizeof(long) )
	    {
		as_bad ("Can't emit a 32-bit relocation for %d bits: %s",
			fixP->fx_size * 8, 
			S_GET_PRTABLE_NAME(sp));
	    }
	}
	else
	    R_SET_TYPE(ep, R_960_12);
	
	/* If symbol associated with relocation entry is undefined,
	   then just remember the index of the symbol.
	   Otherwise store the index of the symbol describing the 
	   section the symbol belong to. This heuristic speeds up ld.
	   */
	if ( S_GET_SEGTYPE(sp) == SEG_TEXT ||
	    S_GET_SEGTYPE(sp) == SEG_DATA ||
	    S_GET_SEGTYPE(sp) == SEG_BSS )
	{
	    /* Use the symbol index of the special static symbol
	       associated with the symbol's segment */
	    R_SET_SYMBOL(ep, symbol_find (SEG_GET_NAME(S_GET_SEGMENT(sp)))->sy_number);
	}
	else
	{
	    /* FIXME: COFF Pollution */
	    if ( SF_GET_LOCAL(sp) )
	    {
		/* Use the symbol index of the special static
		   symbol for this object file's .bss segment */
		R_SET_SYMBOL(ep, (symbol_find (SEG_GET_NAME(DEFAULT_BSS_SEGMENT)))->sy_number);
	    }
	    else
	    {
		/* Default when symbol is undefined is to use
		   the symbol's existing symbol table index */
		R_SET_SYMBOL(ep, sp->sy_number);
	    }
	}
    }
}

/* 
 * Handle .file directive.  According to TIS spec, V 1.1, pg 1-19,
 * we should create a symbol with local binding, in section SHN_ABS,
 * with symbol type STT_FILE.  The one potentially buggy feature is 
 * that it is supposed to precede other local symbols, same as coff.
 * To pull this off we'll insert it first in the chain now, and then 
 * test for its presence in obj_emit_symbols. 
 *
 * Don't worry about duplicate .file's, that is handled in read.c:s_file().
 * Don't worry about duplicate symbols with this name; create a new one.
 */
elf_dot_file_symbol(filename)
    char *filename;
{
    symbolS *sp = symbol_new(filename, MYTHICAL_ABSOLUTE_SEGMENT, 0, &zero_address_frag);
    
    S_IS_DOTFILE(sp) = 1;

    /* Make sure this is first in the symbol chain */
    if ( sp != symbol_rootP )
    {
	symbol_remove(sp, &symbol_rootP, &symbol_lastP);
	symbol_insert(sp, symbol_rootP, &symbol_rootP, &symbol_lastP);
    }
}
