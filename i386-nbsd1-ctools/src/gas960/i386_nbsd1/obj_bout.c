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
to the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. */

/* $Id: obj_bout.c,v 1.39 1996/01/05 18:51:58 paulr Exp $ */

#include "as.h"
#include "obstack.h"

const short /* in: segT   out: N_TYPE bits */
seg_N_TYPE[] = {
  N_ABS,
  N_TEXT,
  N_DATA,
  N_BSS,
  N_UNDF, /* unknown */
  N_UNDF, /* absent */
  N_UNDF, /* pass1 */
  N_UNDF, /* error */
  N_UNDF, /* bignum/flonum */
  N_UNDF, /* difference */
  N_REGISTER, /* register */
};

const segT N_TYPE_seg [N_TYPE+2] = {	/* N_TYPE == 0x1E = 32-2 */
	SEG_UNKNOWN,			/* N_UNDF == 0 */
	SEG_GOOF,
	SEG_ABSOLUTE,			/* N_ABS == 2 */
	SEG_GOOF,
	SEG_TEXT,			/* N_TEXT == 4 */
	SEG_GOOF,
	SEG_DATA,			/* N_DATA == 6 */
	SEG_GOOF,
	SEG_BSS,			/* N_BSS == 8 */
	SEG_GOOF,
	SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF,
	SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF,
	SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF,
	SEG_REGISTER,			/* dummy N_REGISTER for regs = 30 */
	SEG_GOOF,
};

/* seg_name: Used by error reporters, dumpers etc. 
 * in: segT   out: char* 
 */
char * const 
seg_name[] = {
	"absolute",
	"text",
	"data",
	"bss",
	"unknown",
	"absent",
	"pass1",
	"ASSEMBLER-INTERNAL-LOGIC-ERROR!",
	"bignum/flonum",
	"difference",
	"debug",
	"transfert vector preload",
	"transfert vector postload",
	"register",
	"",
};

#ifdef __STDC__
static void obj_bout_stab(int what);
static void obj_bout_line(void);
static void obj_bout_desc(void);
static void obj_bout_lomem(void);
#else /* __STDC__ */
static void obj_bout_desc();
static void obj_bout_stab();
static void obj_bout_line();
static void obj_bout_lomem();
#endif /* __STDC__ */

/* Global vars related to general segments */
extern segS	*segs;		/* Internal segment array */
extern int  	curr_seg;	/* Active segment (index into segs[]) */
extern int  	segs_size;	/* Number of segS's currently in use in segs */
extern int	DEFAULT_TEXT_SEGMENT; 	/* .text */
extern int	DEFAULT_DATA_SEGMENT; 	/* .data */
extern int	DEFAULT_BSS_SEGMENT; 	/* .bss  */

const pseudo_typeS obj_pseudo_table[] = {
 /* stabs (aka a.out aka b.out directives for debug symbols) */
	{ "desc",	obj_bout_desc,		0	}, /* def */
	{ "line",	obj_bout_line,		0	}, /* source code line number */
	{ "stabd",	obj_bout_stab,		'd'	}, /* stabs */
	{ "stabn",	obj_bout_stab,		'n'	}, /* stabs */
	{ "stabs",	obj_bout_stab,		's'	}, /* stabs */

 /* coff debugging directives.  Currently ignored silently */
	{ "def",        s_ignore,		0 },
	{ "dim",        s_ignore,		0 },
	{ "endef",      s_ignore,		0 },
	{ "ln",         s_ignore,		0 },
	{ "scl",        s_ignore,		0 },
	{ "size",       s_ignore,		0 },
	{ "tag",        s_ignore,		0 },
	{ "type",       s_ignore,		0 },
	{ "val",        s_ignore,		0 },

 /* other stuff we don't handle */
	{ "ABORT",      s_ignore,		0 },
	{ "ident",      s_ignore,		0 },
	{ "lomem",	obj_bout_lomem,		0 },

	{ NULL}	/* end sentinel */
}; /* obj_pseudo_table */

/* Relocation. */

/*
 * In: length of relocation (or of address) in chars: 1, 2 or 4.
 * Out: GNU LD relocation length code: 0, 1, or 2.
 */

static unsigned char
nbytes_r_length [] = {
  42, 0, 1, 42, 2
  };

static object_headers headers;
static char *the_object_file;
COMMON char *next_object_file_charP;

/*
 *		emit_relocations()
 *
 * Crawl along a fixS chain. Emit the segment's relocations.
 */
void obj_emit_relocations(fixP, segment_address_in_file)
fixS *fixP;	/* Fixup chain for this segment. */
relax_addressT segment_address_in_file;
{
	struct reloc_info_generic	ri;
	register symbolS *		symbolP;
	
	/* If a machine dependent emitter is needed, call it instead. */
	if (md_emit_relocations) 
	{
		(*md_emit_relocations) (fixP, segment_address_in_file);
		return;
	}
	
	
	/* JF this is for paranoia */
	bzero((char *)&ri,sizeof(ri));
	for (;  fixP;  fixP = fixP->fx_next) 
	{
		if ((symbolP = fixP->fx_addsy) != 0) 
		{
			ri . r_bsr		= fixP->fx_bsr;
			ri . r_disp		= fixP->fx_im_disp;
			ri . r_callj		= fixP->fx_callj;
			ri . r_calljx		= fixP->fx_calljx;
			ri . r_length		= nbytes_r_length [fixP->fx_size];
			ri . r_pcrel		= fixP->fx_pcrel;
			ri . r_address	= fixP->fx_frag->fr_address + fixP->fx_where - segment_address_in_file;
			
			if ( S_GET_TYPE(symbolP) == N_UNDF
			    || ri.r_callj 
			    || ri.r_calljx ) 
			{
				ri . r_extern	= 1;
				ri . r_index	= symbolP->sy_number;
			} 
			else 
			{
				ri . r_extern	= 0;
				ri . r_index	= S_GET_TYPE(symbolP);
			}
			
			/* Output the relocation information in machine-dependent form. */
			md_ri_to_chars(&ri);
		}
	}
} /* emit_relocations() */

/* obj_output_file_size()
 * 
 * There is one of these in obj_coff.c also;
 * Formerly macros, but in the post-general-segment world they are so 
 * different that functions make for less confusion.
 */
long obj_output_file_size (headers)
object_headers	*headers;
{
	return (long) 
		sizeof(struct exec) 
		+ H_GET_TEXT_SIZE(headers)
		+ H_GET_DATA_SIZE(headers)
		+ H_GET_SYMBOL_TABLE_SIZE(headers)
		+ H_GET_TEXT_RELOCATION_SIZE(headers)
		+ H_GET_DATA_RELOCATION_SIZE(headers)
		+ headers->string_table_size;
}

/* Aout file generation & utilities */

/* Append the bout header to the output file in md byte order */
void obj_header_append(headers)
object_headers *headers;
{
	/* Always leave in host byte order */

	headers->header.a_talign = SEG_GET_ALIGN(DEFAULT_TEXT_SEGMENT);

	if (headers->header.a_talign < 2)
	{
		headers->header.a_talign = 2;
	} /* force to at least 2 */

	headers->header.a_dalign = SEG_GET_ALIGN(DEFAULT_DATA_SEGMENT);
	headers->header.a_balign = SEG_GET_ALIGN(DEFAULT_BSS_SEGMENT);

	headers->header.a_tload = 0;
	headers->header.a_dload = md_segment_align(headers->header.a_text, headers->header.a_dalign);

	if ( i960_ccinfo_size() )
	{
		headers->header.a_ccinfo = N_CCINFO;
	}

	output_file_append((char *) &headers->header, sizeof(headers->header), out_file_name);
} /* obj_header_append() */

void obj_symbol_to_chars(symbolP)
symbolS *symbolP;
{
	output_file_append((char *)&symbolP->sy_symbol, sizeof(obj_symbol_type), out_file_name);
} /* obj_symbol_to_chars() */

void obj_emit_symbols(symbol_rootP, headers)
symbolS *symbol_rootP;
object_headers *headers; /* dummy parameter; see coff version */
{
    symbolS *	symbolP;

    /*
     * Emit all symbols left in the symbol chain.
     */
    for(symbolP = symbol_rootP; symbolP; symbolP = symbol_next(symbolP)) {
	/* Used to save the offset of the name. It is used to point
	   to the string in memory but must be a file offset. */
	char *temp;


	temp = S_GET_NAME(symbolP);
	S_SET_OFFSET(symbolP, symbolP->sy_name_offset);

	/* Any symbol still undefined and is not a dbg symbol is made N_EXT. */
	if (!S_IS_DEBUG(symbolP) && !S_IS_DEFINED(symbolP)) S_SET_EXTERNAL(symbolP);

	obj_symbol_to_chars(symbolP);
	S_SET_NAME(symbolP,temp);
    }
} /* emit_symbols() */

void obj_symbol_new_hook(symbolP)
symbolS *symbolP;
{
	S_SET_OTHER(symbolP, 0);
	S_SET_DESC(symbolP, 0);
	return;
} /* obj_symbol_new_hook() */

static void obj_bout_line() {
	/* Assume delimiter is part of expression. */
	/* BSD4.2 as fails with delightful bug, so we */
	/* are not being incompatible here. */
	new_logical_line ((char *)NULL, (int)(get_absolute_expression ()));
	demand_empty_rest_of_line();
} /* obj_bout_line() */


/* 
 * obj_bout_s_get_segment:  Translate from old-style (pre-general-segments)
 * segment designations to newer (segs[] array for coff) style.
 *
 * In: an N_TYPE (constants are in bout.h)
 * Out: Index into segs array
 */
int  
obj_bout_s_get_segment(symP)
symbolS *symP;
{
	segT	segtype = S_GET_SEGTYPE(symP);
	switch ( segtype )
	{
	case SEG_UNKNOWN:
		return MYTHICAL_UNKNOWN_SEGMENT;
	case SEG_ABSOLUTE:
		return MYTHICAL_ABSOLUTE_SEGMENT;
	case SEG_TEXT:
		return DEFAULT_TEXT_SEGMENT;
	case SEG_DATA:
		return DEFAULT_DATA_SEGMENT;
	case SEG_BSS:
		return DEFAULT_BSS_SEGMENT;
	case SEG_REGISTER:
		return MYTHICAL_REGISTER_SEGMENT;
	default:
		return MYTHICAL_GOOF_SEGMENT;
	}
}

/* 
 * obj_bout_s_set_segment:  Translate from old-style (pre-general-segments)
 * segment designations to newer (segs[] array for coff) style.
 *
 * In: a symbol pointer
 *     an index into segs array
 */
void
obj_bout_s_set_segment(symP, seg)
symbolS *symP;
int   seg;
{
	segT	segtype;
	switch ( seg )
	{
	case MYTHICAL_UNKNOWN_SEGMENT:
		segtype = SEG_UNKNOWN;
		break;
	case MYTHICAL_ABSOLUTE_SEGMENT:
		segtype = SEG_ABSOLUTE;
		break;
	case MYTHICAL_REGISTER_SEGMENT:
		segtype = SEG_REGISTER;
		break;
	case 9:		/* DEFAULT_TEXT_SEGMENT */
		segtype = SEG_TEXT;
		break;
	case 10:	/* DEFAULT_DATA_SEGMENT */
		segtype = SEG_DATA;
		break;
	case 11:	/* DEFAULT_BSS_SEGMENT */
		segtype = SEG_BSS;
		break;
	default:
		segtype = SEG_GOOF;
	}
	symP->sy_symbol.n_type &= ~N_TYPE;
	symP->sy_symbol.n_type |= SEGMENT_TO_SYMBOL_TYPE(segtype);
} /* obj_bout_s_set_segment */

/* #define S_GET_SEGTYPE(s)	(N_TYPE_seg[S_GET_TYPE(s)]) */

/*
 *			stab()
 *
 * Handle .stabX directives, which used to be open-coded.
 * So much creeping featurism overloaded the semantics that we decided
 * to put all .stabX thinking in one place. Here.
 *
 * We try to make any .stabX directive legal. Other people's AS will often
 * do assembly-time consistency checks: eg assigning meaning to n_type bits
 * and "protecting" you from setting them to certain values. (They also zero
 * certain bits before emitting symbols. Tut tut.)
 *
 * If an expression is not absolute we either gripe or use the relocation
 * information. Other people's assemblers silently forget information they
 * don't need and invent information they need that you didn't supply.
 *
 * .stabX directives always make a symbol table entry. It may be junk if
 * the rest of your .stabX directive is malformed.
 */
static void obj_bout_stab(what)
int what;
{
  register symbolS *	symbolP = 0;
  register char *	string;
	   int saved_type = 0;
  	   int length;
  	   int goof;	/* TRUE if we have aborted. */
	   long longint;

/*
 * Enter with input_line_pointer pointing past .stabX and any following
 * whitespace.
 */
	goof = 0; /* JF who forgot this?? */
	if (what == 's') {
		string = demand_string(& length);
		SKIP_WHITESPACE();
		if (*input_line_pointer == ',')
			input_line_pointer ++;
		else {
			as_bad("I need a comma after symbol's name");
			goof = 1;
		}
	} else
		string = "";

/*
 * Input_line_pointer->after ','.  String->symbol name.
 */
	if (!goof) {
		symbolP = symbol_new(string,
				     MYTHICAL_UNKNOWN_SEGMENT,
				     0,
				     (struct frag *)0);
		switch (what) {
		case 'd':
			S_SET_NAME(symbolP,NULL); /* .stabd feature. */
			S_SET_VALUE(symbolP,obstack_next_free(&frags) -
				    curr_frag->fr_literal);
			symbolP->sy_frag = curr_frag;
			break;

		case 'n':
			symbolP->sy_frag = &zero_address_frag;
			break;

		case 's':
			symbolP->sy_frag = & zero_address_frag;
			break;

		default:
			BAD_CASE(what);
			break;
		}
		if (get_absolute_expression_and_terminator(& longint) == ',')
			symbolP->sy_symbol.n_type = saved_type = longint;
		else {
			as_bad("I want a comma after the n_type expression");
			goof = 1;
			input_line_pointer--; /* Backup over a non-',' char. */
		}
	}
	if (! goof) {
		if (get_absolute_expression_and_terminator (& longint) == ',')
			S_SET_OTHER(symbolP,longint);
		else {
			as_bad("I want a comma after the n_other expression");
			goof = 1;
			input_line_pointer--; /* Backup over a non-',' char. */
		}
	}
	if (! goof) {
		S_SET_DESC(symbolP, get_absolute_expression ());
		if (what == 's' || what == 'n') {
			if (* input_line_pointer != ',') {
				as_bad("I want a comma after the n_desc expression");
				goof = 1;
			} else {
				input_line_pointer ++;
			}
		}
	}
	if ((! goof) && (what=='s' || what=='n')) {
		pseudo_set(symbolP);
		symbolP->sy_symbol.n_type = saved_type;
	}
	if (goof)
		ignore_rest_of_line ();
	else
		demand_empty_rest_of_line ();
} /* obj_bout_stab() */

static void obj_bout_desc() {
	register char *name;
	register char c;
	register char *p;
	register symbolS *	symbolP;
	register int temp;

	/*
	 * Frob invented at RMS' request. Set the n_desc of a symbol.
	 */
	name = input_line_pointer;
	c = get_symbol_end();
	p = input_line_pointer;
	* p = c;
	SKIP_WHITESPACE();
	if (*input_line_pointer != ',') {
		*p = 0;
		as_bad("Expected comma after name \"%s\"", name);
		*p = c;
		ignore_rest_of_line();
	} else {
		input_line_pointer ++;
		temp = get_absolute_expression ();
		*p = 0;
		symbolP = symbol_find_or_make(name);
		*p = c;
		S_SET_DESC(symbolP,temp);
	}
	demand_empty_rest_of_line();
} /* obj_bout_desc() */


/* 
 * obj_bout_lomem: 
 * "Actively ignore" the coff general-section directive ".lomem"
 * (as opposed to silently ignoring it)
 */
static void obj_bout_lomem() 
{
	as_warn (".lomem is not supported for b.out. (directive ignored)");
	s_ignore(0);
}


void obj_read_begin_hook() {
	return;
} /* obj_read_begin_hook() */

void obj_crawl_symbol_chain(headers)
object_headers *headers;
{
	symbolS **symbolPP;
	symbolS *symbolP;
	symbolS *symbol_undef_extern_rootP = NULL;
	symbolS *symbol_undef_extern_lastP = NULL;
	extern symbolS *top_of_comm_sym_chain();
	int symbol_number = 0;

	/* JF deal with forward references first... */
	for (symbolP = symbol_rootP; symbolP;) {
		if (symbolP->sy_forward) {
			S_SET_VALUE(symbolP, S_GET_VALUE(symbolP)
				    + S_GET_VALUE(symbolP->sy_forward)
				    + symbolP->sy_forward->sy_frag->fr_address);

			symbolP->sy_forward=0;
		} /* if it has a forward reference */
		if (!S_IS_DEBUG(symbolP) && S_GET_SEGTYPE(symbolP) == SEG_UNKNOWN) {
		    /* Remove all undefineds and commons from the symbol list.  Insert them
		       in common definition order, followed by undef order. */
		    symbolS *prev = symbol_previous(symbolP);

		    if (!prev)
			    prev = symbol_next(symbolP);
		    symbol_remove(symbolP, &symbol_rootP, &symbol_lastP);
		    if (S_IS_COMMON(symbolP)) {
		    }
		    else {
			symbol_append(symbolP, symbol_undef_extern_lastP,
				      &symbol_undef_extern_rootP,
				      &symbol_undef_extern_lastP);
		    }
		    symbolP = prev;
		}
		else
			symbolP = symbol_next(symbolP);
	} /* walk the symbol chain */

	for (symbolP=top_of_comm_sym_chain();symbolP;symbolP=top_of_comm_sym_chain())
		/* Insert the commons in the order of their declaration. */
		symbol_append(symbolP, symbol_lastP, &symbol_rootP, &symbol_lastP);

	for ( ; symbol_undef_extern_rootP; ) {
	    symbolS *tmp = symbol_undef_extern_rootP;

	    symbol_remove(tmp, &symbol_undef_extern_rootP, &symbol_undef_extern_lastP);
	    symbol_append(tmp, symbol_lastP, &symbol_rootP, &symbol_lastP);
	}

	/* Check some i960 leafproc stuff (formerly in tc_crawl_symbol_chain) */
	for (symbolP = symbol_rootP; symbolP; symbolP = symbol_next(symbolP)) 
	{
	    if ( TC_S_IS_LEAFPROC(symbolP) )
	    {
		symbolS *balp = symbol_find(TC_S_GET_BALNAME(symbolP));
		if ( symbolP != balp )
		{
		    /* Normal case, 2 different entry points.
		       Make sure both are local or both are global
		       (push to global if different) */
		    if (S_IS_EXTERNAL(symbolP) != S_IS_EXTERNAL(balp)) 
		    {
			S_SET_EXTERNAL(symbolP);
			S_SET_EXTERNAL(balp);
		    }
		    S_SET_OTHER(symbolP, N_CALLNAME);
		    S_SET_OTHER(balp, N_BALNAME);
		    /* Make sure bal symbol immediately follow call symbol */
		    if ( symbol_next(symbolP) != balp )
		    {
			symbol_remove(balp, &symbol_rootP, &symbol_lastP);
			symbol_append(balp, symbolP, &symbol_rootP, &symbol_lastP);
		    }
		}
		else
		{
		    /* Else no leaf entry point was given, or same symbol
		       was given for both call and leaf entry point */
		    S_SET_OTHER(symbolP, N_BALNAME);
		}
	    }
	    
	    if ( TC_S_IS_SYSPROC(symbolP) )
	    {
		S_SET_OTHER(symbolP, TC_S_GET_SYSINDEX(symbolP));
	    }
	}

	symbolPP = & symbol_rootP;	/*->last symbol chain link. */
	while ((symbolP  = *symbolPP) != NULL)
	{
		S_SET_VALUE(symbolP, S_GET_VALUE(symbolP) + symbolP->sy_frag->fr_address);

		/* OK, here is how we decide which symbols go out into the
		   brave new symtab.  Symbols that do are:

		   * symbols with no name (stabd's?)
		   * symbols with debug info in their N_TYPE
 		   * Both args of .leafproc (callname and balname).

		   Symbols that don't are:
		   * symbols that are registers
		   * symbols with \1 as their 3rd character (numeric labels)
		   * "local labels" as defined by SF_GET_LOCAL(name)
		   unless the -d switch was passed to gas.

		   All other symbols are output.  We complain if a deleted
		   symbol was marked external. */

		if  ( ! S_IS_REGISTER(symbolP)
		    && ( ! S_GET_NAME(symbolP)
			|| S_IS_DEBUG(symbolP)
			|| ! S_IS_DEFINED(symbolP)
			|| S_IS_EXTERNAL(symbolP)
 			|| TC_S_IS_LEAFPROC(symbolP)
			|| TC_S_IS_BALNAME(symbolP)
			|| ! SF_GET_LOCAL(symbolP) 
			|| flagseen['d'] ) ) 
		{
			symbolP->sy_number = symbol_number++;

			/* The + 1 after strlen account for the \0 at the
			   end of each string */
			if (!S_IS_STABD(symbolP)) {
				/* Ordinary case. */
				symbolP->sy_name_offset = string_byte_count;
				string_byte_count += strlen(S_GET_NAME(symbolP)) + 1;
			}
			else	/* .Stabd case. */
			    symbolP->sy_name_offset = 0;
			symbolPP = &(symbol_next(symbolP));
		} else {
			if (S_IS_EXTERNAL(symbolP) || !S_IS_DEFINED(symbolP)) {
				as_bad("Local symbol %s never defined", S_GET_PRTABLE_NAME(symbolP));
			} /* oops. */

			/* Unhook it from the chain */
			*symbolPP = symbol_next(symbolP);
		} /* if this symbol should be in the output */
	} /* for each symbol */

	H_SET_SYMBOL_TABLE_SIZE(headers, symbol_number);

	return;
} /* obj_crawl_symbol_chain() */

/*
 * Find strings by crawling along symbol table chain.
 */

void obj_emit_strings()
{
	symbolS *symbolP;

	if ( string_byte_count == sizeof(string_byte_count) )
		string_byte_count = 0;

	output_file_append((char *) &string_byte_count, sizeof(string_byte_count), out_file_name);

	if ( string_byte_count )
	{
		for(symbolP = symbol_rootP; symbolP; symbolP = symbol_next(symbolP)) 
		{
			if(S_GET_NAME(symbolP))
				output_file_append(S_GET_NAME(symbolP), strlen (S_GET_NAME(symbolP)) + 1, out_file_name);
		} /* walk symbol chain */
	}
} /* obj_emit_strings() */


/*
 * Write an OMF-dependent object file to the disk file.  
 * Do lots of other bookkeeping stuff.
 * The call to this in the source will be "write_object_file()".
 */
void write_bout_file()
{
	fragS 		*fragP;		/* Track along all frags. */
	fragS		*text_frag_root, *text_frag_last;
	fragS		*data_frag_root, *data_frag_last;
	int		seg;		/* temp for stepping thru segs array */
        long 		object_file_size;
	long		byte_count;	/* number bytes emitted to output file */
	long		zero = 0;	/* for padding section ends */

	/*
	 * We have several user segments. Do one last frag_new (yes, it's wasted)
	 * to close off the fixed part of the last frag. 
	 */
	frag_wane (curr_frag);
	frag_new (0);

	for ( seg = FIRST_PROGRAM_SEGMENT; seg < segs_size; ++seg )
	{
		seg_change ( seg );
		relax_segment ( seg );

		/* For bout, there are always only 3 substantive segments. */
		switch ( seg )
		{
		case FIRST_PROGRAM_SEGMENT:
			text_frag_root = SEG_GET_FRAG_ROOT(DEFAULT_TEXT_SEGMENT);
			text_frag_last = SEG_GET_FRAG_LAST(DEFAULT_TEXT_SEGMENT);
			H_SET_TEXT_SIZE(&headers, md_segment_align(text_frag_last->fr_address, 2));
			break;
		case FIRST_PROGRAM_SEGMENT + 1:
			data_frag_root = SEG_GET_FRAG_ROOT(DEFAULT_DATA_SEGMENT);
			data_frag_last = SEG_GET_FRAG_LAST(DEFAULT_DATA_SEGMENT);
			H_SET_DATA_SIZE(&headers, md_segment_align(data_frag_last->fr_address, 2));
			break;
		case FIRST_PROGRAM_SEGMENT + 2:
			H_SET_BSS_SIZE(&headers,SEG_GET_SIZE(DEFAULT_BSS_SEGMENT));
			break;
		default:
			BAD_CASE(seg);
		}
	}



	/*
	 * Now the addresses of frags are correct within each segment.
	 */

	/*
	 * Join the 2 segments into 1 huge segment.
	 * To do this, re-compute every rn_address in the SEG_DATA frags.
	 * Then join the data frags after the text frags.
	 * Set the root address for the (one and only) SEG_BSS frag.
	 *
	 * Determine a_data [length of data segment].
	 */
	if ( data_frag_root )
	{
		register relax_addressT	slide;
		
		slide = md_segment_align ( text_frag_last->fr_address, 
					  SEG_GET_ALIGN(DEFAULT_DATA_SEGMENT) > 2 ?
					  SEG_GET_ALIGN(DEFAULT_DATA_SEGMENT) :
					  2 );
		
		for (fragP = data_frag_root; fragP; fragP = fragP->fr_next)
		{
			fragP->fr_address += slide;
		}
	} 
	else 
	{
		H_SET_DATA_SIZE(&headers,0);
	}
	
	/* 
	 * Now set the bss section starting address
	 */
	SEG_GET_FRAG_ROOT(DEFAULT_BSS_SEGMENT)->fr_address = 
	    md_segment_align( data_frag_last->fr_address,
			     SEG_GET_ALIGN(DEFAULT_BSS_SEGMENT) > 2 ?
			     SEG_GET_ALIGN(DEFAULT_BSS_SEGMENT) :
			     2 );
	

	/*
	 * Crawl the symbol chain.
	 *
	 * For each symbol whose value depends on a frag, take the address of
	 * that frag and subsume it into the value of the symbol.
	 * After this, there is just one way to lookup a symbol value.
	 * Values are left in their final state for object file emission.
	 * We adjust the values of 'L' local symbols, even if we do
	 * not intend to emit them to the object file, because their values
	 * are needed for fix-ups.
	 *
	 * Unless we saw a -L flag, remove all symbols that begin with 'L'
	 * from the symbol chain.  (They are still pointed to by the fixes.)
	 *
	 * Count the remaining symbols.
	 * Assign a symbol number to each symbol.
	 * Count the number of string-table chars we will emit.
	 * Put this info into the headers as appropriate.
	 *
	 */
	know(zero_address_frag.fr_address == 0);
	string_byte_count = sizeof(string_byte_count);
	
	obj_crawl_symbol_chain(&headers);

	md_check_leafproc_list();
  
	/*
	 * Addresses of frags now reflect addresses we use in the object file.
	 * Symbol values are correct.
	 * Scan the frags, converting any ".org"s and ".align"s to ".fill"s.
	 * Also convert any machine-dependent frags using md_convert_frag();
	 */
	for ( seg = FIRST_PROGRAM_SEGMENT; seg < DEFAULT_BSS_SEGMENT; ++seg )
	{
		seg_change (seg);
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
	}

	/* 
	 * FIXME-SOMEDAY: Hey, let's do this right and make this thing
	 * into a real 2-pass assembler.
	 *
	 * Until such a glorious day comes, if there were any .set expressions
	 * that could not be evaluated during the first pass do them now.
	 * (do_delayed_evaluation is in write.c)
	 */
	do_delayed_evaluation();  

	/*
	 * Scan every FixS performing fixups. We had to wait until now to do
	 * this because md_convert_frag() may have made some fixSs.
	 */
	H_SET_RELOCATION_SIZE(&headers,
			      md_reloc_size * fixup_segment(DEFAULT_TEXT_SEGMENT),
			      md_reloc_size * fixup_segment(DEFAULT_DATA_SEGMENT));
	  
	H_SET_MAGIC_NUMBER(&headers, DEFAULT_MAGIC_NUMBER_FOR_OBJECT_FILE);
	H_SET_ENTRY_POINT(&headers,0);
	  
#ifdef EXEC_MACHINE_TYPE
	H_SET_MACHINE_TYPE(&headers,EXEC_MACHINE_TYPE);
#endif
#ifdef EXEC_VERSION
	H_SET_VERSION(&headers,EXEC_VERSION);
#endif
	  
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

	object_file_size = H_GET_FILE_SIZE(&headers);
#ifdef GNU960
	object_file_size += i960_ccinfo_size();
#endif

	output_file_create(out_file_name);
	obj_header_append (&headers);
	  
	/*
	 * Emit code.
	 */
	for ( seg = FIRST_PROGRAM_SEGMENT; seg < DEFAULT_BSS_SEGMENT; ++seg )
	{
		byte_count = 0;
		for (fragP = SEG_GET_FRAG_ROOT(seg); fragP; fragP = fragP->fr_next) 
		{
			register long		count;
			register char *		fill_literal;
			register long		fill_size;
			
			know( fragP->fr_type == rs_fill );
			/* Do the fixed part. */
			output_file_append (fragP->fr_literal, (unsigned long)fragP->fr_fix, out_file_name);
			byte_count += fragP->fr_fix;

			/* Do the variable part(s). */
			fill_literal= fragP->fr_literal + fragP->fr_fix;
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
					output_file_append (fill_literal, 
							    count > fill_size ? fill_size : count,
							    out_file_name);
					byte_count += (count > fill_size ? fill_size : count);
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
					output_file_append (fill_literal, (unsigned long)fill_size, out_file_name);
					byte_count += fill_size;
				}
			}
		} /* for each code frag. */

		/* Now pad the end of this section with zeroes as needed to
		 * reach a word boundary.
		 */
		for ( ; byte_count < md_segment_align(byte_count, 2); ++byte_count )
		{
			output_file_append((char *) &zero, 1L, out_file_name);
		}
	} /* for each segment. */
	  
	/*
	 * Emit relocations.
	 */
	obj_emit_relocations (SEG_GET_FIX_ROOT(DEFAULT_TEXT_SEGMENT), 0);
	obj_emit_relocations (SEG_GET_FIX_ROOT(DEFAULT_DATA_SEGMENT), data_frag_root->fr_address);
	  
	/*
	 * Emit symbols.
	 */
	obj_emit_symbols (symbol_rootP, &headers);
	  
	/*
	 * Emit strings.
	 */
	obj_emit_strings ();

#ifdef GNU960
	i960_emit_ccinfo();
#endif
	  
	output_file_close(out_file_name);

} /* write_bout_file() */
