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

/* $Id: obj_coff.c,v 1.81 1995/12/19 18:52:24 paulr Exp $ */

#include "as.h"

#include "obstack.h"

lineno* lineno_rootP;

#ifdef __STDC__

char *s_get_name(symbolS *s);
void s_section(void);
void s_stab_stub(int what);
static symbolS *tag_find_or_make(char *name);
static symbolS* tag_find(char *name);
static void c_section_header_append(char **where, SCNHDR *header);
static void obj_coff_def(int what);
static void obj_coff_dim(void);
static void obj_coff_emit_lineno(lineno *line);
static void obj_coff_endef(void);
static void obj_coff_ident(void);
static void obj_coff_line(void);
static void obj_coff_ln(void);
static void obj_coff_scl(void);
static void obj_coff_size(void);
static void obj_coff_tag(void);
static void obj_coff_type(void);
static void obj_coff_val(void);
static void tag_init(void);
static void tag_insert(char *name, symbolS *symbolP);

#else

char *s_get_name();
void s_section();
void s_stab_stub();
static symbolS *tag_find();
static symbolS *tag_find_or_make();
static void c_section_header_append();
static void obj_coff_def();
static void obj_coff_dim();
static void obj_coff_emit_lineno();
static void obj_coff_endef();
static void obj_coff_ident();
static void obj_coff_line();
static void obj_coff_ln();
static void obj_coff_scl();
static void obj_coff_size();
static void obj_coff_tag();
static void obj_coff_type();
static void obj_coff_val();
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

static object_headers headers;
static long output_byte_count;

static struct hash_control *tag_hash;
static symbolS *def_symbol_in_progress = NULL;

static stack	*block_stack;
static stack	*fcn_stack;

const pseudo_typeS obj_pseudo_table[] = {
	{ "def",	obj_coff_def,		0	},
	{ "dim",	obj_coff_dim,		0	},
	{ "endef",	obj_coff_endef,		0	},
	{ "line",	obj_coff_line,		0	},
	{ "ln",		obj_coff_ln,		0	},
	{ "scl",	obj_coff_scl,		0	},
	{ "size",	obj_coff_size,		0	},
	{ "tag",	obj_coff_tag,		0	},
	{ "type",	obj_coff_type,		0	},
	{ "val",	obj_coff_val,		0	},

 /* stabs, aka a.out, aka b.out directives for debug symbols.
    Currently ignored silently.  Except for .line which we
    guess at from context. */
	{ "desc",	s_ignore,		0	}, /* def */
	{ "stabd",	s_stab_stub,		'd'	}, /* stabs */
    	{ "stabn",      s_stab_stub,		'n'	}, /* stabs */
	{ "stabs",	s_stab_stub,		's'	}, /* stabs */

 /* other stuff */
	{ "ABORT",      s_abort,		0 },
	{ "ident", 	obj_coff_ident,		0 },
    	{ "section", 	s_section, 		0 },

	{ NULL}	/* end sentinel */
}; /* obj_pseudo_table */

/* For use in default filename in the absence of .file directive. */
extern char *physical_input_filename;

/* obj dependant output values */
static SCNHDR bss_section_header;
static SCNHDR data_section_header;
static SCNHDR text_section_header;

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


/* Relocation. */

/*
 *		emit_relocations()
 *
 * Crawl along a fixS chain. Emit the segment's relocations.
 */

void obj_emit_relocations(fixP, dummy)
	fixS *fixP; /* Fixup chain for this segment. */
	unsigned long dummy; /* To match the b.out interface */
{
	RELOC ri;
	symbolS *symbolP;

	bzero((char *)&ri,sizeof(ri));
	for ( ; fixP;  fixP = fixP->fx_next ) 
	{
		if ( symbolP = fixP->fx_addsy )
		{
			ri.r_type = (fixP->fx_pcrel ?
				     R_IPRMED :
				     (fixP->fx_size ? R_RELLONG : 
				      R_RELSHORT));
			/* Protect against trying to relocate an undefined
			 * into a byte or a short 
			 */
			if ( ri.r_type == R_RELLONG && fixP->fx_size < sizeof(long) )
			{
				as_bad ("Can't emit a 32-bit relocation for %d bits: %s",
					fixP->fx_size * 8, 
					fixP->fx_addsy ? S_GET_PRTABLE_NAME(fixP->fx_addsy) :
					"<undefined>");
			}

			ri.r_vaddr = fixP->fx_frag->fr_address + fixP->fx_where;

			/* If symbol associated to relocation entry is undefined,
			   then just remember the index of the symbol.
			   Otherwise store the index of the symbol describing the
			   section the symbol belong to. This heuristic speeds up ld.
			   */

			if ( S_GET_SEGTYPE(symbolP) == SEG_TEXT ||
			     S_GET_SEGTYPE(symbolP) == SEG_DATA ||
			     S_GET_SEGTYPE(symbolP) == SEG_BSS )
			{
				/* Use the symbol index of the special static symbol
				 * associated with the symbol's segment 
				 */
				ri.r_symndx = (symbol_find (SEG_GET_NAME(S_GET_SEGMENT(symbolP))))->sy_number;
			}
			else
			{
				if ( SF_GET_LOCAL(symbolP) )
				{
					/* Use the symbol index of the special static
					 * symbol for this object file's .bss segment
					 */
					ri.r_symndx = (symbol_find (SEG_GET_NAME(DEFAULT_BSS_SEGMENT)))->sy_number;
				}
				else
				{
					/* Default when symbol is undefined is to use
					 * the symbol's existing symbol table index
					 */
					ri.r_symndx = symbolP->sy_number;
				}
			}

			/* Last step : write it out in md format */
			output_file_append((char *) &ri, (long) sizeof(ri), out_file_name);
			output_byte_count += sizeof(ri);

			if (fixP->fx_callj) 
				/* if it's a callj, do it again for the opcode */
			{
				ri.r_type = R_OPTCALL;
				ri.r_symndx = symbolP->sy_number;
				output_file_append((char *) &ri, (long) sizeof(ri), out_file_name);
				output_byte_count += sizeof(ri);
			}
			else 
				if (fixP->fx_calljx)
					/* same for 2-word calljx; note that the
					   opcode is 4 bytes away from the target */
				{
					if ( SF_GET_LOCAL(symbolP) )
					{
						/* Can't currently do a calljx to a local label */
						as_bad ("Calljx to local label not allowed; use callj\n");
					}
					ri.r_type = R_OPTCALLX;
					ri.r_vaddr -= 4;
					ri.r_symndx = symbolP->sy_number;
					output_file_append((char *) &ri, (long) sizeof(ri), out_file_name);
					output_byte_count += sizeof(ri);
				}
		} /* if there's an add symbol */
#ifdef R_RELLONG_SUB
		/* New subtraction relocation; if defined in coff.h, 
		   it will be implemented in the linker. */
		if ( symbolP = fixP->fx_subsy )
		{
		    /* FIXME: flesh out other subtraction types: R_IPRMED_SUB and R_RELSHORT_SUB */
			ri.r_type = R_RELLONG_SUB;

			/* Protect against trying to relocate an undefined
			 * into a byte or a short 
			 */
			if ( ri.r_type == R_RELLONG_SUB && fixP->fx_size < sizeof(long) )
			{
				as_bad ("Can't emit a 32-bit relocation for %d bits: %s",
					fixP->fx_size * 8, 
					fixP->fx_addsy ? S_GET_PRTABLE_NAME(fixP->fx_addsy) :
					"<undefined>");
			}

			ri.r_vaddr = fixP->fx_frag->fr_address + fixP->fx_where;

			/* If symbol associated to relocation entry is undefined,
			   then just remember the index of the symbol.
			   Otherwise store the index of the symbol describing the
			   section the symbol belong to. This heuristic speeds up ld.
			   */

			if ( S_GET_SEGTYPE(symbolP) == SEG_TEXT ||
			     S_GET_SEGTYPE(symbolP) == SEG_DATA ||
			     S_GET_SEGTYPE(symbolP) == SEG_BSS )
			{
				/* Use the symbol index of the special static symbol
				 * associated with the symbol's segment 
				 */
				ri.r_symndx = (symbol_find (SEG_GET_NAME(S_GET_SEGMENT(symbolP))))->sy_number;
			}
			else
			{
				if ( SF_GET_LOCAL(symbolP) )
				{
					/* Use the symbol index of the special static
					 * symbol for this object file's .bss segment
					 */
					ri.r_symndx = (symbol_find (SEG_GET_NAME(DEFAULT_BSS_SEGMENT)))->sy_number;
				}
				else
				{
					/* Default when symbol is undefined is to use
					 * the symbol's existing symbol table index
					 */
					ri.r_symndx = symbolP->sy_number;
				}
			}

			/* Last step : write it out in md format */
			output_file_append((char *) &ri, (long) sizeof(ri), out_file_name);
			output_byte_count += sizeof(ri);

		} /* if there's a subtract symbol */
#endif /* if (R_RELLONG_SUB) */		
	} /* for each fixP */

} /* obj_emit_relocations() */


/* obj_output_file_size()
 * 
 * There is one of these in obj_bout.c also;
 * Formerly macros, but in the post-general-segment world they are so
 * different that functions make for less confusion.
 */
long obj_output_file_size (headers)
object_headers *headers;
{
	long 	result;
	int  	seg;	/* for tracing through the segments */

	/* File header */
	result = FILHSZ;
	/* Section headers */
	result += SCNHSZ * (segs_size - FIRST_PROGRAM_SEGMENT);
	/* Section contents */
	for ( seg = FIRST_PROGRAM_SEGMENT; seg < segs_size; ++seg )
		result += SEG_GET_SIZE(seg);
	/* Relocations */
	result += H_GET_RELOCATION_SIZE(headers);
	/* Line numbers */
	result += H_GET_LINENO_SIZE(headers);
	/* Symbol Table */
	result += H_GET_SYMBOL_TABLE_SIZE(headers);
	/* String Table */
	result += headers->string_table_size;
	return result;
}

/* Coff file generation & utilities */

void obj_symbol_to_chars(symbolP)
symbolS *symbolP;
{
	SYMENT *syment = &symbolP->sy_symbol.ost_entry;
	int i;
	char numaux = syment->n_numaux;
	unsigned short type = S_GET_DATA_TYPE(symbolP);

	output_file_append((char *) syment, sizeof(*syment), out_file_name);
	output_byte_count += sizeof (*syment);

	/* Should do the following : if (.file entry) MD(..)... else if (static entry) MD(..) */
	if (numaux > OBJ_COFF_MAX_AUXENTRIES) {
		as_fatal("Internal error: too many auxents for symbol");
	} /* too many auxents */

	for (i = 0; i < numaux; ++i) 
	{
		output_file_append((char *) &symbolP->sy_symbol.ost_auxent[i], 
				   sizeof(symbolP->sy_symbol.ost_auxent[i]), out_file_name);
		output_byte_count += sizeof(symbolP->sy_symbol.ost_auxent[i]);
	}; /* for each aux in use */
} /* obj_symbol_to_chars() */

static void c_section_header_append(where, header)
char **where;
SCNHDR *header;
{
    append(where, (char *) header, sizeof(*header));
} /* c_section_header_append() */


void obj_emit_symbols(symbol_rootP, headers)
symbolS *symbol_rootP;
object_headers *headers;    /* for OR-ing in file header flags */
{
	symbolS *symbolP = symbol_rootP;
	symbolS *balP;
	char	*temp = S_GET_NAME(symbolP);
	unsigned short fh_flags;

	/* 
	 * Through some long-obscured pact with the devil, past generations
	 * of i960 tools have copied the FILE HEADER flags into the flags
	 * field of each and every SYMBOL.  We are now doomed to continue
	 * this arcane practice forever, or until some brave vampire slayer
	 * puts a stake through our hearts, or until the real reason for
	 * such an odd habit becomes clear, whichever comes first.
	 */
	fh_flags = H_GET_FLAGS(headers);

	/* 
	 * Special handling for .file symbol, which will always be first
	 * in the symbol chain.  To handle .file's longer than 14 chars,
	 * the .file's n_name field is masquerading as the actual file
	 * name. (The n_name field should be just ".file", and the file 
	 * name should be in the first aux entry) Swap the 2 strings,
	 * output the symbol, the swap them back so that the filename 
	 * string will get output in obj_emit_strings().
	 */
	know ( S_GET_STORAGE_CLASS(symbolP) == C_FILE );

	if ( strlen(temp) > FILNMLEN )
	{
		symbolP->sy_symbol.ost_auxent[0].x_file.x_n.x_offset = symbolP->sy_name_offset;
		symbolP->sy_symbol.ost_auxent[0].x_file.x_n.x_zeroes = 0;
	}
	else
	{
	    memset(symbolP->sy_symbol.ost_auxent[0].x_file.x_fname,0,FILNMLEN);
	    if (strlen(temp) == FILNMLEN)
		    strncpy (symbolP->sy_symbol.ost_auxent[0].x_file.x_fname, temp, FILNMLEN);
	    else if (strlen(temp))
		    strcpy(symbolP->sy_symbol.ost_auxent[0].x_file.x_fname, temp);
	    else
		    strcpy(symbolP->sy_symbol.ost_auxent[0].x_file.x_fname, "unknown");
	}

	bzero(symbolP->sy_symbol.ost_entry.n_name, SYMNMLEN);
	strcpy(symbolP->sy_symbol.ost_entry.n_name, ".file");

	/* Do the bizarre OR-ing in of the flags ritual */
	S_SET_FLAGS(symbolP, S_GET_FLAGS(symbolP) | fh_flags);

	/* Emit the .file symbol */
	S_SET_SEGMENT(symbolP, S_GET_SEGMENT(symbolP) - FIRST_PROGRAM_SEGMENT + 1);
	obj_symbol_to_chars(symbolP);

	/* Now put this symbol back the way it was.  A string table entry 
	 * will only be emitted if the name is longer than FILNMLEN.
	 */
	if ( strlen(temp) > FILNMLEN )
	{
		S_SET_ZEROES(symbolP, 0);
		S_SET_NAME(symbolP, temp); 
	}

	/*
	 * Emit all symbols left in the symbol chain.
	 */
	symbolP = symbol_next(symbolP);
	for (; symbolP; symbolP = symbol_next(symbolP)) 
	{
		if ( TC_S_IS_LEAFPROC(symbolP) ) 
		{
			balP = symbol_find(TC_S_GET_BALNAME(symbolP));
			if ( balP )
			{
			    /* second aux entry contains the bal entry point */
			    symbolP->sy_symbol.ost_auxent[1].x_bal.x_balntry = S_GET_VALUE(balP);
			    S_SET_DATA_TYPE(symbolP, S_GET_DATA_TYPE(symbolP) | (DT_FCN << N_BTSHFT));
			}
			else
			{
			    as_fatal("No bal entry point for leafproc: %s\n",
				     S_GET_PRTABLE_NAME(symbolP));
			}
		}
		
		if ( TC_S_IS_SYSPROC(symbolP) ) 
		{
		    /* second aux entry contains the bal entry point */
		    memset(&symbolP->sy_symbol.ost_auxent[1], 0, sizeof(AUXENT));
		    symbolP->sy_symbol.ost_auxent[1].x_sc.x_stindx = TC_S_GET_SYSINDEX(symbolP);
		    S_SET_DATA_TYPE(symbolP, S_GET_DATA_TYPE(symbolP) | (DT_FCN << N_BTSHFT));
		}
		
		/* Adjust the symbol's section number from the internal segment
		 * numbering to the output section numbering.  Note that the fatal 
		 * error determination depends on the ORDER of the MYTHICAL_* 
		 * macros in obj_coff.h
		 */
		if (S_GET_SEGMENT(symbolP) <= MYTHICAL_GOOF_SEGMENT)
		{
			if (flagseen ['Z'])
				as_warn ("Unrecognized section number %d for symbol %s", 
					 S_GET_SEGMENT(symbolP) - FIRST_PROGRAM_SEGMENT + 1, 
					 S_GET_PRTABLE_NAME(symbolP));
			else
				as_fatal ("Unrecognized section number: %d for symbol: %s", 
					  S_GET_SEGMENT(symbolP) - FIRST_PROGRAM_SEGMENT + 1, 
					  S_GET_PRTABLE_NAME(symbolP));
		}
		S_SET_SEGMENT(symbolP, S_GET_SEGMENT(symbolP) - FIRST_PROGRAM_SEGMENT + 1);

		/* Fix up names that are short enough to go directly into
		 * the n_name field.
		 */
		temp = S_GET_NAME(symbolP);
		if (SF_GET_STRING(symbolP))
		{
			S_SET_OFFSET(symbolP, symbolP->sy_name_offset);
			S_SET_ZEROES(symbolP, 0);
		} 
		else 
		{
			bzero(symbolP->sy_symbol.ost_entry.n_name, SYMNMLEN);
			strncpy(symbolP->sy_symbol.ost_entry.n_name, temp, SYMNMLEN);
		}

		/* Do the bizarre OR-ing in of the flags ritual */
		S_SET_FLAGS(symbolP, S_GET_FLAGS(symbolP) | fh_flags);
		
		obj_symbol_to_chars(symbolP);
		S_SET_NAME(symbolP,temp);
	}
} /* obj_emit_symbols() */

/* Merge a debug symbol containing debug information into a normal symbol. */

void c_symbol_merge(debug, normal)
symbolS *debug;
symbolS *normal;
{
	S_SET_DATA_TYPE(normal, S_GET_DATA_TYPE(debug));
	S_SET_STORAGE_CLASS(normal, S_GET_STORAGE_CLASS(debug));

	if (S_GET_NUMBER_AUXILIARY(debug) > S_GET_NUMBER_AUXILIARY(normal)) {
		S_SET_NUMBER_AUXILIARY(normal, S_GET_NUMBER_AUXILIARY(debug));
	} /* take the most we have */

	if (S_GET_NUMBER_AUXILIARY(debug) > 0) {
		memcpy((char*)&normal->sy_symbol.ost_auxent[0], (char*)&debug->sy_symbol.ost_auxent[0], S_GET_NUMBER_AUXILIARY(debug) * AUXESZ);
	} /* Move all the auxiliary information */

	normal->sy_forward = debug->sy_forward;

	/* Move the debug flags. */
	SF_SET_DEBUG_FIELD(normal, SF_GET_DEBUG_FIELD(debug));
	
	/* Mark the normal symbol as having been defined with a .def */
	SF_SET_DEFINED(normal);
} /* c_symbol_merge() */

void 
c_dot_file_symbol(filename)
	char *filename;
{
	symbolS		*symbolP;

	/* Workaround for file names > 14 chars:  put the filename 
	 * into the string table now; then special-case the .file symbol 
	 * in obj_crawl_symbol_chain() and obj_emit_symbols().
	 */
	symbolP = symbol_new(filename,
			     MYTHICAL_DEBUG_SEGMENT,
			     0,
			     &zero_address_frag);
	S_SET_STORAGE_CLASS(symbolP, C_FILE);
	S_SET_NUMBER_AUXILIARY(symbolP, 1);	
	SF_SET_DEBUG(symbolP);

	if ( symbol_rootP != symbolP )
	{
		if ( S_GET_STORAGE_CLASS(symbol_rootP) == C_FILE )
		{
			/* There is already a .file symbol, created by a .ident 
			 * directive.  Replace existing one with the one just 
			 * created, then copy the 2nd aux entry in.
			 */
			symbolS	*saveP = symbol_rootP;
			symbol_remove(saveP, &symbol_rootP, &symbol_lastP);
			if ( S_GET_NUMBER_AUXILIARY(saveP) == 2 )
			{
				S_SET_NUMBER_AUXILIARY(symbolP, 2);
				symbolP->sy_symbol.ost_auxent[1] = saveP->sy_symbol.ost_auxent[1];
			}
		}
		symbol_remove(symbolP, &symbol_rootP, &symbol_lastP);
		symbol_insert(symbolP, symbol_rootP, &symbol_rootP, &symbol_lastP);
	}
} /* c_dot_file_symbol() */

/* Line number handling */

int function_lineoff = -1;	/* Offset in line#s where the last function
				   started (the odd entry for line #0) */

int our_lineno_number = 0;

int
c_line_new(paddr, line_number, frag)
long paddr;
unsigned short line_number;
fragS* frag;
{
	lineno* new_line = (lineno*) xmalloc (sizeof(lineno));
	
	new_line->line.l_addr.l_paddr = paddr;
	new_line->line.l_lnno = line_number;
	new_line->line.padding[0] = 0;
	new_line->line.padding[1] = 0;
	new_line->frag = (char*) frag;
	new_line->next = NULL;
	
	if (SEG_GET_LINE_ROOT(curr_seg) == NULL)
	{
		SEG_SET_LINE_ROOT(curr_seg, new_line);
		SEG_SET_LINE_LAST(curr_seg, new_line);
	}
	else
	{
		SEG_GET_LINE_LAST(curr_seg)->next = new_line;
		SEG_SET_LINE_LAST(curr_seg, new_line);
	}
	return LINESZ * our_lineno_number++;
}

static void obj_coff_emit_lineno(line)
	lineno *line;
{
	LINENO *line_entry;
	symbolS *symbolP;
	for (; line; line = line->next) 
	{
		line_entry = &line->line;

		/* FIXME-SOMEDAY Resolving the sy_number of function linno's 
		 * used to be done in write_object_file() but their symbols 
		 * need a fileptr to the lnno, so I moved this resolution 
		 * check here.  xoxorich. 
		 */

		if (line_entry->l_lnno == 0) 
		{
			/* There is a good chance that the symbol pointed to
			 * is not the one that will be emitted and that the
			 * sy_number is not accurate. 
			 */
			symbolP = (symbolS *) line_entry->l_addr.l_symndx;
			line_entry->l_addr.l_symndx = symbolP->sy_number;
			symbolP->sy_symbol.ost_auxent[0].x_sym.x_fcnary.x_fcn.x_lnnoptr = output_byte_count;
		} /* if this is a function linno */

		output_file_append((char *) line_entry, LINESZ, out_file_name);
		output_byte_count += LINESZ;

	} /* for each line number */

} /* obj_emit_lineno() */


void obj_symbol_new_hook(symbolP)
symbolS *symbolP;
{
	char underscore = 0;      /* Symbol has leading _ */

	/* Effective symbol */
	/* Store the pointer in the offset. */
	S_SET_ZEROES(symbolP, 0L);
	S_SET_DATA_TYPE(symbolP, T_NULL);
	S_SET_STORAGE_CLASS(symbolP, 0);
	S_SET_NUMBER_AUXILIARY(symbolP, 0);
	/* Additional information */
	symbolP->sy_symbol.ost_flags = 0;
	/* Auxiliary entries */
	bzero((char*)&symbolP->sy_symbol.ost_auxent[0], AUXESZ);

#if STRIP_UNDERSCORE
	/* Remove leading underscore at the beginning of the symbol.
	 * This is to be compatible with the standard librairies.
	 */
	if (*S_GET_NAME(symbolP) == '_') {
		underscore = 1;
		S_SET_NAME(symbolP, S_GET_NAME(symbolP) + 1);
	} /* strip underscore */
#endif /* STRIP_UNDERSCORE */

	if (S_IS_STRING(symbolP))
	    SF_SET_STRING(symbolP);

} /* obj_symbol_new_hook() */

 /* stack stuff */
stack* stack_init(chunk_size, element_size)
unsigned long chunk_size;
unsigned long element_size;
{
	stack* st;

	st = (stack *) xmalloc(sizeof(stack));
	st->data = (char *) xmalloc(chunk_size);
	st->pointer = 0;
	st->size = chunk_size;
	st->chunk_size = chunk_size;
	st->element_size = element_size;
	return st;
} /* stack_init() */

void stack_delete(st)
stack* st;
{
    free(st->data);
    free(st);
    return;
    
}

char *stack_push(st, element)
stack *st;
char *element;
{
	if (st->pointer + st->element_size >= st->size) 
	{
		st->size += st->chunk_size;
		st->data = xrealloc(st->data, st->size);
	}
	memcpy(st->data + st->pointer, element, st->element_size);
	st->pointer += st->element_size;
	return st->data + st->pointer;
} /* stack_push() */

char* stack_pop(st)
stack* st;
{
    if ( (long) (st->pointer -= st->element_size) < 0) 
    {
	st->pointer = 0;
	return NULL;
    }
    return st->data + st->pointer;
}

char* stack_top(st)
stack* st;
{
    return st->data + st->pointer - st->element_size;
}



/* 
 * Recognize the .ident directive.  Put the first following argument, the
 * producer identification string, into the symbol table as the 2nd aux 
 * entry for the .file symbol.  The assembler's ID string becomes the 
 * 3rd aux entry.
 */

static void obj_coff_ident()
{
	symbolS		*symbolP = symbol_rootP;
	long		timestamp;
	int		i = 0;
	char		*c;
	char 		*args[2];
	int		len;
	
	/*  Parse the rest of the .ident line.  
	 *  Input buffer pointer is aimed at the first char following .ident.
	 *  Assumptions about the input buffer (see in_scrub.c):
	 *      comments and labels have been removed.
	 *	strings of whitespace have been collapsed to a single blank.
	 */

	args[0] = demand_string (&len);

	if (args [0] == NULL)
	{
		/* .ident is on a line by itself */
		as_bad (".ident requires at least one argument\n");
		s_ignore(0);
		return;
	}
	
	SKIP_WHITESPACE();
	if ( *input_line_pointer == ',' )
		++input_line_pointer;
	timestamp = get_absolute_expression ();
	demand_empty_rest_of_line();
	
	/* Don't do anything further if the proper environment var isn't set. */
	if ( env_var("I960IDENT") == NULL )
		return;
	
	/* Is there a .file symbol? If not insert one at the beginning. */
	if (symbolP == NULL || S_GET_STORAGE_CLASS(symbolP) != C_FILE)
	{
		c_dot_file_symbol(physical_input_filename);
		symbolP = symbol_rootP;
	}

	if ( ! timestamp )
		/* producer did not provide a time stamp; make our own */
		time ( &timestamp );
	
	S_SET_NUMBER_AUXILIARY(symbolP, 2);
	SA_SET_IDENT_IDSTRING(symbolP, 1, args [0]);
	SA_SET_IDENT_TIMESTAMP(symbolP, 1, (unsigned long) timestamp);
	
} /* obj_coff_ident() */



/*
 * Handle .ln directives.
 */

static void obj_coff_ln() 
{
	long 		linenum;
	long 		address;
	segT		segtype;
	expressionS	exp;

	if (def_symbol_in_progress != NULL) 
	{
		as_warn(".ln pseudo-op inside .def/.endef");
		demand_empty_rest_of_line();
		return;
	}

	if ( (linenum = get_absolute_expression()) == 0 )
	{
		as_warn ("Invalid line number 0 ignored.");
		return;
	}

	if ( *input_line_pointer == ',' )
	{
		/* An address expression follows. */
		++input_line_pointer;
		segtype = expression(&exp);
		switch (segtype)
		{
		case SEG_ABSOLUTE:
			address = exp.X_add_number;
			break;
		case SEG_TEXT:
		case SEG_DATA:
		case SEG_BSS:
			/* Best we can do is evaluate it now.  
			 * It's too risky to generate a fixup in case 
			 * it is never resolved.
			 */
			address = S_GET_VALUE(exp.X_add_symbol) + exp.X_add_number;
			break;
		default:
			as_bad("Invalid address expression for .ln");
			demand_empty_rest_of_line();
			return;
		}
	}
	else
	{
		address = obstack_next_free(&frags) - curr_frag->fr_literal;
	}

	c_line_new(address, linenum, curr_frag);
	demand_empty_rest_of_line();

} /* obj_coff_ln() */

/*
 *			def()
 *
 * Handle .def directives.
 *
 * One might ask : why can't we symbol_new if the symbol does not
 * already exist and fill it with debug information.  Because of
 * the C_EFCN special symbol. It would clobber the value of the
 * function symbol before we have a chance to notice that it is
 * a C_EFCN. And a second reason is that the code is more clear this
 * way. (at least I think it is :-).
 *
 */

#define SKIP_SEMI_COLON()	while (*input_line_pointer++ != ';')
#define SKIP_WHITESPACES()	while (*input_line_pointer == ' ' || \
				      *input_line_pointer == '\t') \
                                         input_line_pointer++;

static void obj_coff_def(what)
int what;
{
    char name_end; /* Char after the end of name */
    char *symbol_name; /* Name of the debug symbol */
    char *symbol_name_copy; /* Temporary copy of the name */
    unsigned int symbol_name_length;
    /*$char*	directiveP;$ */		/* Name of the pseudo opcode */
    /*$char directive[MAX_DIRECTIVE];$ */ /* Backup of the directive */
    /*$char end = 0;$ */ /* If 1, stop parsing */

    if (def_symbol_in_progress != NULL) {
	as_bad("Mismatched .def/.endef directives");
	demand_empty_rest_of_line();
	return;
    } /* if not inside .def/.endef */

    SKIP_WHITESPACES();

    def_symbol_in_progress = (symbolS *) obstack_alloc(&notes, sizeof(*def_symbol_in_progress));
    bzero(def_symbol_in_progress, sizeof(*def_symbol_in_progress));

    symbol_name = input_line_pointer;
    name_end = get_symbol_end();
    symbol_name_length = strlen(symbol_name);
    if ( symbol_name_length == 0 )
    {
	    as_bad("Syntax error: Expected symbol name after .def");
	    *input_line_pointer = name_end;
	    demand_empty_rest_of_line();
	    return;
    }
    symbol_name_copy = xmalloc(symbol_name_length + 1);
    strcpy(symbol_name_copy, symbol_name);

    /* Initialize the new symbol */
#if STRIP_UNDERSCORE
    S_SET_NAME(def_symbol_in_progress, (*symbol_name_copy == '_'
					? symbol_name_copy + 1
					: symbol_name_copy));
#else /* STRIP_UNDERSCORE */
    S_SET_NAME(def_symbol_in_progress, symbol_name_copy);
#endif /* STRIP_UNDERSCORE */
    /* free(symbol_name_copy); */
    def_symbol_in_progress->sy_name_offset = ~0;
    def_symbol_in_progress->sy_number = ~0;
    def_symbol_in_progress->sy_frag = &zero_address_frag;
    
    if (S_IS_STRING(def_symbol_in_progress)) {
	SF_SET_STRING(def_symbol_in_progress);
    } /* "long" name */

    *input_line_pointer = name_end;

    /* Set the segment to unknown.  This will almost always be changed
     * in obj_coff_val().
     */
    S_SET_SEGMENT(def_symbol_in_progress, MYTHICAL_UNKNOWN_SEGMENT);

    demand_empty_rest_of_line();
    return;
} /* obj_coff_def() */

unsigned int dim_index;

static void obj_coff_endef() 
{
	symbolS *symbolP;
	dim_index =0;
	if (def_symbol_in_progress == NULL) 
	{
		as_warn(".endef pseudo-op used outside of .def/.endef");
		demand_empty_rest_of_line();
		return;
	} /* if not inside .def/.endef */

	/* Set the section number according to storage class. */
	switch (S_GET_STORAGE_CLASS(def_symbol_in_progress)) 
	{
	case C_STRTAG:
	case C_ENTAG:
	case C_UNTAG:
		SF_SET_TAG(def_symbol_in_progress);
		/* intentional fallthrough */
	case C_FILE:
	case C_TPDEF:
		SF_SET_DEBUG(def_symbol_in_progress);
		S_SET_SEGMENT(def_symbol_in_progress, MYTHICAL_DEBUG_SEGMENT);
		break;

	case C_EFCN:
		SF_SET_LOCAL(def_symbol_in_progress);   /* Do not emit this symbol. */
		SF_SET_PROCESS(def_symbol_in_progress); /* Will need processing before writing */
		S_SET_SEGMENT(def_symbol_in_progress, SEG_GET_TYPE(curr_seg) == SEG_TEXT ? curr_seg : DEFAULT_TEXT_SEGMENT);
		break;
	case C_BLOCK:
		SF_SET_PROCESS(def_symbol_in_progress); /* Will need processing before writing */
		S_SET_SEGMENT(def_symbol_in_progress, SEG_GET_TYPE(curr_seg) == SEG_TEXT ? curr_seg : DEFAULT_TEXT_SEGMENT);
		if ( ! strcmp(S_GET_NAME(def_symbol_in_progress), ".bb") )
			stack_push(block_stack, (char *) &symbolP);
		else 
		{
			/* Better be ".eb": check for a matching .bb */
			if ( stack_pop(block_stack) == NULL )
				as_bad("mismatched .eb");
		}
		break;
	case C_FCN:
		S_SET_SEGMENT(def_symbol_in_progress, SEG_GET_TYPE(curr_seg) == SEG_TEXT ? curr_seg : DEFAULT_TEXT_SEGMENT);

		if ( ! strcmp(S_GET_NAME(def_symbol_in_progress), ".bf") )
		{ 
			stack_push(fcn_stack, (char *) &symbolP);
			if (function_lineoff < 0) 
			{
				as_bad("'.bf' symbol without preceding function\n");
			}
			function_lineoff = -1;
		}
		else
		{
			/* Better be ".ef": check for a matching .bf */
			if ( stack_pop(fcn_stack) == NULL )
				as_bad("mismatched .ef");
		}
		break;

	case C_AUTOARG:
	case C_AUTO:
	case C_REG:
	case C_MOS:
	case C_MOE:
	case C_MOU:
	case C_ARG:
	case C_REGPARM:
	case C_FIELD:
	case C_EOS:
		SF_SET_DEBUG(def_symbol_in_progress);
		S_SET_SEGMENT(def_symbol_in_progress, MYTHICAL_ABSOLUTE_SEGMENT);
		break;

	case C_LEAFEXT:
	case C_LEAFSTAT:
	case C_EXT:
	case C_STAT:
	case C_LABEL:	
	case C_SCALL:
		/* These have their own processing elsewhere */
		break;

	default:
		/* All others are unsupported. (they will be errors if 
		 * processed with BFD)  But there may be a good reason 
		 * why someone wants them, so issue a warning, not an error.
		 */
		SF_SET_DEBUG(def_symbol_in_progress);
		S_SET_SEGMENT(def_symbol_in_progress, MYTHICAL_ABSOLUTE_SEGMENT);
		as_warn ("Unsupported storage class: %d for symbol %s", 
			S_GET_STORAGE_CLASS(def_symbol_in_progress),
			S_GET_PRTABLE_NAME(def_symbol_in_progress));
		break;

	} /* switch on storage class */

	/* Now that we have built a debug symbol, try to
	   find if we should merge with an existing symbol
	   or not.  If a symbol is C_EFCN or SEG_ABSOLUTE or
	   untagged SEG_DEBUG it never merges. */

	/* Two cases for functions.  Either debug followed
	   by definition or definition followed by debug.
	   For definition first, we will merge the debug
	   symbol into the definition.  For debug first, the
	   lineno entry MUST point to the definition
	   function or else it will point off into space
	   when obj_crawl_symbol_chain() merges the debug
	   symbol into the real symbol.  Therefore, let's
	   presume the debug symbol is a real function
	   reference. */

	symbolP = symbol_find_base(S_GET_NAME(def_symbol_in_progress), DO_NOT_STRIP);
	if ( symbolP && SF_GET_DUPLICATE(def_symbol_in_progress) )
	{
		/* This symbol also exists in "real" form; 
		 * merge the debug symbol into the real symbol.
		 * Note that a debug symbol can merge with only
		 * ONE real symbol.
		 */

		c_symbol_merge(def_symbol_in_progress, symbolP);
		SF_CLEAR_DUPLICATE(symbolP);

		/* We're going to append the merged symbol to the end of 
		 * the symbol chain.  So remove the current instance of 
		 * like-named symbol from the chain.
		 */
		symbol_remove(symbolP, &symbol_rootP, &symbol_lastP);
		def_symbol_in_progress = symbolP;
	}
	else 
	{
		if ( SF_GET_TAG(def_symbol_in_progress) )
		{
			/* A forward reference to a tag will never be found
			 * with the logic of the previous test.  We need to
			 * check tags specifically for a potential merge. 
			 */
			symbolS	*tagP = tag_find(S_GET_NAME(def_symbol_in_progress));
			if ( tagP && ! SF_GET_TAG(tagP) )
			{
				/* There was a forward reference in a prior .def.
				 * There is a virgin tag. (has never merged)
				 * Merge this debug symbol with the prior tag
				 * and add merged symbol to normal symbol chain.
				 */
				c_symbol_merge(def_symbol_in_progress, tagP);
				symbol_remove(tagP, &symbol_rootP, &symbol_lastP);
				def_symbol_in_progress = tagP;
			}
		}
	} /* if (def_symbol should be merged with a real symbol) */
	
	SF_SET_DEFINED(def_symbol_in_progress);
	symbol_append(def_symbol_in_progress, symbol_lastP, &symbol_rootP, &symbol_lastP);

	if (SF_GET_TAG(def_symbol_in_progress))
	{
		if (tag_find(S_GET_NAME(def_symbol_in_progress)) == NULL) 
		{
			tag_insert(S_GET_NAME(def_symbol_in_progress), def_symbol_in_progress);
		} /* If symbol is a {structure,union} tag, associate symbol to its name. */
	}

	if (SF_GET_FUNCTION(def_symbol_in_progress)) 
	{
		know(sizeof(def_symbol_in_progress) <= sizeof(long));
		function_lineoff = c_line_new((long) def_symbol_in_progress, 0, &zero_address_frag);
		SF_SET_PROCESS(def_symbol_in_progress);

		if (symbolP == NULL) 
		{
			/* That is, if this is the first
			   time we've seen the function... */
			symbol_table_insert(def_symbol_in_progress);
		} /* definition follows debug */
	} /* Create the line number entry pointing to the function being defined */

	def_symbol_in_progress = NULL;
	demand_empty_rest_of_line();
} /* obj_coff_endef() */


static void 
obj_coff_dim() 
{
	long	arrlen;
	if (def_symbol_in_progress == NULL) 
	{
		as_warn(".dim pseudo-op used outside of .def/.endef");
		demand_empty_rest_of_line();
		return;
	} /* if not inside .def/.endef */

	S_SET_NUMBER_AUXILIARY(def_symbol_in_progress, 1);

	/*  Grab as many dims as we can fit, until ; or full */
	while ( dim_index < DIMNUM + 1 ) 
	{
		SKIP_WHITESPACES();
		arrlen = get_absolute_expression();
		if ( arrlen < 0 )
			as_warn ("Negative array dimension, symbol: %s",
				 S_GET_PRTABLE_NAME(def_symbol_in_progress));
		if ( arrlen > 65535 )
			as_warn (".dim argument > 0xffff, symbol's aux entry incorrect: %s",
				 S_GET_PRTABLE_NAME(def_symbol_in_progress));
		if ( dim_index < DIMNUM )
			SA_SET_SYM_DIMEN(def_symbol_in_progress, dim_index, arrlen);
		dim_index++;
		if ( *input_line_pointer == ';' || *input_line_pointer == '\n' )
			break;
		if (*input_line_pointer != ',') 
		{
			as_bad("badly formed .dim directive");
			break;
		}
		input_line_pointer++;
	}
	
	if (dim_index <= DIMNUM)
	{
		demand_empty_rest_of_line();
		return;
	}
	else
	{
		as_warn ("Array dimension too large for symbolic debug");
		s_ignore(0);
	}
} /* obj_coff_dim() */


static void obj_coff_line() {
	if (def_symbol_in_progress == NULL) {
		obj_coff_ln();
		return;
	} /* if it looks like a stabs style line */

	S_SET_NUMBER_AUXILIARY(def_symbol_in_progress, 1);
	SA_SET_SYM_LNNO(def_symbol_in_progress, get_absolute_expression());

	demand_empty_rest_of_line();
	return;
} /* obj_coff_line() */

static void obj_coff_size() {
	int	size;
	if (def_symbol_in_progress == NULL) {
		as_warn(".size pseudo-op used outside of .def/.endef");
		demand_empty_rest_of_line();
		return;
	} /* if not inside .def/.endef */

	S_SET_NUMBER_AUXILIARY(def_symbol_in_progress, 1);
	size = get_absolute_expression();
	if ( size < 0 )
		as_warn ("Negative array/struct/union size, symbol: %s",
			 S_GET_PRTABLE_NAME(def_symbol_in_progress));
	if ( size > 65535 )
		as_warn (".size argument > 0xffff, symbol's aux entry incorrect: %s",
			 S_GET_PRTABLE_NAME(def_symbol_in_progress));

	SA_SET_SYM_SIZE(def_symbol_in_progress, size);
	demand_empty_rest_of_line();
	return;
} /* obj_coff_size() */

static void obj_coff_scl() {
	if (def_symbol_in_progress == NULL) {
		as_warn(".scl pseudo-op used outside of .def/.endef");
		demand_empty_rest_of_line();
		return;
	} /* if not inside .def/.endef */

	S_SET_STORAGE_CLASS(def_symbol_in_progress, get_absolute_expression());
	demand_empty_rest_of_line();
	return;
} /* obj_coff_scl() */

static void obj_coff_tag() {
	char *symbol_name;
	char name_end;
	symbolS *tmpsym;

	if (def_symbol_in_progress == NULL) {
		as_warn(".tag pseudo-op used outside of .def/.endef");
		demand_empty_rest_of_line();
		return;
	} /* if not inside .def/.endef */

	S_SET_NUMBER_AUXILIARY(def_symbol_in_progress, 1);
	symbol_name = input_line_pointer;
	name_end = get_symbol_end();

	/* Check for valid arg */
	if ( strlen(symbol_name) == 0 )
	{
		as_bad("Syntax error: expected tag name after .tag");
		*input_line_pointer = name_end;
		demand_empty_rest_of_line();
		return;
	}

	/* Assume that the symbol referred to by .tag is always defined. */
	/* This was a bad assumption.  I've added find_or_make. xoxorich. */
	if ( (tmpsym = tag_find(symbol_name)) == NULL ) {
		/* Forward reference to (probably) struct or union tag.  
		 * Mark as debug symbol and it will be merged with a "real"
		 * symbol later when the real symbol is defined.
		 */
		tmpsym = tag_find_or_make(symbol_name);
		S_SET_SEGMENT (tmpsym, MYTHICAL_DEBUG_SEGMENT);
	}
	  	    
	SA_SET_SYM_TAGNDX(def_symbol_in_progress, (long) tmpsym);
	if (SA_GET_SYM_TAGNDX(def_symbol_in_progress) == 0L) {
		as_warn("tag not found for .tag %s", symbol_name);
	} /* not defined */

	SF_SET_TAGGED(def_symbol_in_progress);
	*input_line_pointer = name_end;

	demand_empty_rest_of_line();
	return;
} /* obj_coff_tag() */

static void obj_coff_type() {
	if (def_symbol_in_progress == NULL) {
		as_warn(".type pseudo-op used outside of .def/.endef");
		demand_empty_rest_of_line();
		return;
	} /* if not inside .def/.endef */

	S_SET_DATA_TYPE(def_symbol_in_progress, get_absolute_expression());

	if ( ISFCN(S_GET_DATA_TYPE(def_symbol_in_progress)) 
	    && S_GET_STORAGE_CLASS(def_symbol_in_progress) != C_TPDEF ) 
	{
		SF_SET_FUNCTION(def_symbol_in_progress);
	} /* is a function */

	demand_empty_rest_of_line();
	return;
} /* obj_coff_type() */

static void obj_coff_val() 
{
	if (def_symbol_in_progress == NULL) 
	{
		as_warn(".val pseudo-op used outside of .def/.endef");
		demand_empty_rest_of_line();
		return;
	} /* if not inside .def/.endef */

	if (is_name_beginner(*input_line_pointer)) 
	{
		char *symbol_name = input_line_pointer;
		char name_end = get_symbol_end();

		if ( ! strcmp(symbol_name, ".") )  
		{
			def_symbol_in_progress->sy_frag = curr_frag;
			S_SET_VALUE(def_symbol_in_progress, obstack_next_free(&frags) - curr_frag->fr_literal);
			S_SET_SEGMENT(def_symbol_in_progress, curr_seg);
		} 
		else if ( strcmp(S_GET_NAME(def_symbol_in_progress), symbol_name) )
 		{
			/* Giving the debug symbol a value from another symbol.
			 * The sy_forward will be resolved later.
			 */
			def_symbol_in_progress->sy_forward = symbol_find_or_make(symbol_name);
		}
		else
		{
			/* The .val name and the debug symbol's name are the same.
			 * This is a candidate for merging with a real symbol.
			 */
			SF_SET_DUPLICATE(def_symbol_in_progress);
		}
		*input_line_pointer = name_end;
	} 
	else 
	{
		S_SET_VALUE(def_symbol_in_progress, get_absolute_expression());
	} /* if symbol based */

	demand_empty_rest_of_line();
	return;
} /* obj_coff_val() */

/*
 * Maintain a list of the tagnames of the structures.
 */

static void tag_init() {
    tag_hash = hash_new();
    return ;
} /* tag_init() */

static void tag_insert(name, symbolP)
char *name;
symbolS *symbolP;
{
	register char *	error_string;

	if (*(error_string = hash_jam(tag_hash, name, (char *)symbolP))) {
		as_fatal("Inserting \"%s\" into structure table failed: %s",
			 name, error_string);
	}
	return ;
} /* tag_insert() */

static symbolS *tag_find_or_make(name)
char *name;
{
	symbolS *symbolP;

	if ((symbolP = tag_find(name)) == NULL) {
		symbolP = symbol_new(name,
				     MYTHICAL_UNKNOWN_SEGMENT,
				     0,
				     &zero_address_frag);

		tag_insert(S_GET_NAME(symbolP), symbolP);
	} /* not found */

	return(symbolP);
} /* tag_find_or_make() */

static symbolS *tag_find(name)
char *name;
{
#if STRIP_UNDERSCORE
	if (*name == '_') name++;
#endif /* STRIP_UNDERSCORE */
	return((symbolS*)hash_find(tag_hash, name));
} /* tag_find() */

void obj_read_begin_hook() 
{
	tag_init();

	/* Initialize the stack used to keep track of the matching .bb / .eb */
	block_stack = stack_init(512, sizeof(symbolS*));

	/* Same for the matching .bf / .ef */
	fcn_stack = stack_init(512, sizeof(symbolS*));

} /* obj_read_begin_hook() */

void obj_crawl_symbol_chain(headers)
object_headers *headers;
{
	int symbol_number = 0;
	lineno *lineP;
	symbolS *last_functionP = NULL;
	symbolS *last_tagP;
	symbolS *symbolP;
	symbolS *symbol_def_extern_rootP = NULL;
	symbolS *symbol_def_extern_lastP = NULL;
	symbolS *symbol_undef_extern_rootP = NULL;
	symbolS *symbol_undef_extern_lastP = NULL;
	symbolS *symbol_first_externP = NULL;
	symbolS *symbol_static_rootP = NULL;
	symbolS *symbol_static_lastP = NULL;
	symbolS *symbol_balname_rootP = NULL;
	symbolS *symbol_balname_lastP = NULL;
	extern symbolS *top_of_comm_sym_chain();
	symbolS *common_symbolP = top_of_comm_sym_chain();


	/* Take care of forward references to values first */
	for (symbolP = symbol_rootP; symbolP; symbolP = symbol_next(symbolP)) 
	{
		if (symbolP->sy_forward) 
		{
			S_SET_VALUE(symbolP, (S_GET_VALUE(symbolP)
					      + S_GET_VALUE(symbolP->sy_forward)
					      + symbolP->sy_forward->sy_frag->fr_address));
			S_SET_SEGMENT(symbolP, S_GET_SEGMENT(symbolP->sy_forward));
			symbolP->sy_forward=0;
		} /* if it has a forward reference */
	} /* walk the symbol chain */

	/* Check some i960 leafproc stuff (formerly in tc_crawl_symbol_chain) */
	for (symbolP = symbol_rootP; symbolP; symbolP = symbol_next(symbolP)) 
	{
	    if ( S_GET_STORAGE_CLASS(symbolP) == C_SCALL && ! TC_S_IS_SYSPROC(symbolP) )
	    {
		/* No .sysproc was seen for this dude */
		as_warn("No .sysproc directive seen for symbol: %s\nReverting to normal extern function.",
			S_GET_PRTABLE_NAME(symbolP));
		S_SET_STORAGE_CLASS(symbolP, C_EXT);
		S_SET_NUMBER_AUXILIARY(symbolP, 1);
		continue;
	    }
	    
	    if ( S_GET_STORAGE_CLASS(symbolP) == C_LEAFEXT ||
		S_GET_STORAGE_CLASS(symbolP) == C_LEAFSTAT )
	    {
		if (!S_IS_DEFINED(symbolP)) 
		{
		    as_bad("leafproc symbol '%s' undefined", S_GET_PRTABLE_NAME(symbolP));
		}
		
		/* Its storage class is C_LEAFEXT or C_LEAFSTAT; but 
		   was there ever a .leafproc for it? */
		if ( ! TC_S_IS_LEAFPROC(symbolP) )
		{
		    as_warn("No .leafproc directive seen for symbol: %s\nReverting to normal extern function.", S_GET_PRTABLE_NAME(symbolP));
		    S_SET_STORAGE_CLASS(symbolP, C_EXT);
		    S_SET_NUMBER_AUXILIARY(symbolP, 1);
		    SF_CLEAR_CALLNAME(symbolP);
		}
		continue;
	    }
	    if ( TC_S_IS_LEAFPROC(symbolP) )
	    {
		/* The reverse: there was a .leafproc, but was there a .scl? */
		if (!S_IS_DEFINED(symbolP)) 
		{
		    as_bad("leafproc symbol '%s' undefined", S_GET_PRTABLE_NAME(symbolP));
		}
		switch ( S_GET_STORAGE_CLASS(symbolP) )
		{
		case C_EXT:
		    S_SET_STORAGE_CLASS(symbolP,C_LEAFEXT);
		    break;
		case C_NULL:
		case C_STAT:
		case C_LABEL:
		    S_SET_STORAGE_CLASS(symbolP,C_LEAFSTAT);
		    break;
		default:
		    ;
		}
	    }
	}

	/* Is there a .file symbol? If not insert one at the beginning using
	 * the name of the input file.  NOTE: this means that we know that 
	 * symbol_rootP always points to the .file symbol.  This will become
	 * important later.
	 */
	if (symbol_rootP == NULL || S_GET_STORAGE_CLASS(symbol_rootP) != C_FILE)
	    c_dot_file_symbol(physical_input_filename);
	
	/* Special handling for .file's longer than 8 chars. */
	if ( SF_GET_STRING(symbol_rootP) && strlen(S_GET_NAME(symbol_rootP)) <= FILNMLEN )
		SF_CLEAR_STRING(symbol_rootP);

#if defined(DEBUG)
	verify_symbol_chain(symbol_rootP, symbol_lastP);
#endif /* DEBUG */

	/* For a relocatable COFF file (i.e. our output) the symbol
	 * table sequence is:
	 *	.file
	 *	func1 name
	 *	autos, statics, and debugs for func1
	 *	func2 name
	 *	autos, statics, and debugs for func2
	 *	 ...
	 *	statics
	 *	defined externs
	 *      common symbols
	 *	undefined externs
	 *
	 * Go through the symbol chain, putting externs and statics
	 * into their own temporary chains and removing them from the
	 * main chain.  Then append them to the main chain in the order
	 * just given, numbering them as we go.  Finally, patch pointers
	 * to symbols (using sym indexes.)
	 */

	for (symbolP = symbol_rootP; symbolP; symbolP = symbolP ? symbol_next(symbolP) : symbol_rootP)
	{
		if ( ! SF_GET_DEBUG(symbolP) ) 
		{
			symbolS* real_symbolP;

			/* Merge .def symbols with their real counterparts
			 * where appropriate.
			 */
			if ( SF_GET_DUPLICATE(symbolP) )
			{
				/* Move the debug data from the debug symbol to the
				 * real symbol.  Replace the debug symbol with the 
				 * newly-merged symbol in the same position as the 
				 * debug symbol.
				 */
				if ( (real_symbolP = symbol_find_base(S_GET_NAME(symbolP), DO_NOT_STRIP)) 
				&& real_symbolP != symbolP )
				{
					c_symbol_merge(symbolP, real_symbolP);
					SF_CLEAR_DUPLICATE(real_symbolP);
					symbol_remove(real_symbolP, &symbol_rootP, &symbol_lastP);
					symbol_insert(real_symbolP, symbolP, &symbol_rootP, &symbol_lastP);
					symbol_remove(symbolP, &symbol_rootP, &symbol_lastP);
					symbolP = real_symbolP;
				}
			}

			S_SET_VALUE(symbolP, S_GET_VALUE(symbolP) + symbolP->sy_frag->fr_address);

			if ( ! S_IS_DEFINED(symbolP) 
			    && ! TC_S_IS_SYSPROC(symbolP) 
			    && !SF_GET_LOCAL(symbolP) )
			{
				S_SET_EXTERNAL(symbolP);
			} 
			else if ( S_GET_STORAGE_CLASS(symbolP) == C_NULL ) 
			{
				if ( S_GET_SEGTYPE(symbolP) == SEG_TEXT )
					S_SET_STORAGE_CLASS(symbolP, C_LABEL);
				else
					S_SET_STORAGE_CLASS(symbolP, C_STAT);
			}

			if ( SF_GET_PROCESS(symbolP) ) 
			{
				/* If we are able to identify the type of a function, and we
				   are out of a function (last_functionP == NULL) then, the
				   function symbol will be associated with an auxiliary
				   entry. */
				if ( last_functionP == NULL && SF_GET_FUNCTION(symbolP) )
				{
					last_functionP = symbolP;

					if (S_GET_NUMBER_AUXILIARY(symbolP) < 1)
						S_SET_NUMBER_AUXILIARY(symbolP, 1);

					/* Clobber possible stale .dim information. */
					bzero(symbolP->sy_symbol.ost_auxent[0].x_sym.x_fcnary.x_ary.x_dimen,
					      sizeof(symbolP->sy_symbol.ost_auxent[0].x_sym.x_fcnary.x_ary.x_dimen));
				}
				switch ( S_GET_STORAGE_CLASS(symbolP) )
				{
				case C_BLOCK:
					/* Handle the nested blocks' auxiliary info. 
					 * Proper nesting level is guaranteed at this
					 * point so don't waste time checking it.
					 */
					if (!strcmp(S_GET_NAME(symbolP), ".bb"))
						stack_push(block_stack, (char *) &symbolP);
					else 
					 	/* .eb */
						SA_SET_SYM_ENDNDX( *(symbolS**)stack_pop(block_stack), symbol_number+2 );
					break;
				case C_EFCN:
					if ( last_functionP == NULL )
					{
						as_bad("C_EFCN symbol out of scope");
						break;
					}
					SA_SET_SYM_FSIZE(last_functionP,
							 (long)(S_GET_VALUE(symbolP) -
								S_GET_VALUE(last_functionP)));
					SA_SET_SYM_ENDNDX(last_functionP, symbol_number);
					last_functionP = (symbolS*)0;
					break;
				default:
					break;
				}
			} /* if SF_PROCESS */
		} /* if NOT debug */
		else if (SF_GET_TAG(symbolP)) 
		{
			/* First descriptor of a structure must point to
			   the first slot after the structure description. */
			last_tagP = symbolP;

		} 
		else if (S_GET_STORAGE_CLASS(symbolP) == C_EOS) 
		{
			/* + 2 take into account the current symbol */
			SA_SET_SYM_ENDNDX(last_tagP, symbol_number + 2);
		} 
		else if (S_GET_STORAGE_CLASS(symbolP) == C_FILE) 
		{
			if (S_GET_VALUE(symbolP)) 
			{
				S_SET_VALUE((symbolS *) S_GET_VALUE(symbolP), symbol_number);
				S_SET_VALUE(symbolP, 0);
			} /* no one points at the first .file symbol */
		} /* if debug or tag or eos or file */

		/* We must put the external symbols apart. The loader
		   does not bomb if we do not. But the references in
		   the endndx field for a .bb symbol are not corrected
		   if an extern symbol is removed between .bb and .be.
		   I.e in the following case :
		   [20] .bb endndx = 22
		   [21] foo (external)
		   [22] .be
		   ld will move the symbol 21 to the end of the list but
		   endndx will still be 22 instead of 21. */

		if ( SF_GET_LOCAL(symbolP) )
		{
			symbol_remove(symbolP, &symbol_rootP, &symbol_lastP);
		} 
		else if ( ! S_IS_DEBUG(symbolP) 
			 && ! S_IS_FUNCTION(symbolP) 
			 && ! SF_GET_DEFINED(symbolP)
			 && (S_GET_STORAGE_CLASS(symbolP) == C_STAT 
			     || S_GET_STORAGE_CLASS(symbolP) == C_LEAFSTAT) )
		{
			/* Static, or static leafproc, but NOT a function. */
			symbolS *save = symbol_previous(symbolP);
			symbol_remove(symbolP, &symbol_rootP, &symbol_lastP);
			symbol_append(symbolP, symbol_static_lastP, &symbol_static_rootP, &symbol_static_lastP);
			symbolP = save;
		}
		else if ( ! S_IS_DEBUG(symbolP)  
			 && ! S_IS_FUNCTION(symbolP)
			 && (S_GET_STORAGE_CLASS(symbolP) == C_EXT 
			     || S_GET_STORAGE_CLASS(symbolP) == C_LEAFEXT
			     || S_GET_STORAGE_CLASS(symbolP) == C_SCALL) )
			 
		{
			/* External, or external leafproc, or system proc,
			 * but NOT a function. 
			 */
			symbolS *hold = symbol_previous(symbolP);
			symbol_remove(symbolP, &symbol_rootP, &symbol_lastP);

			if ( S_GET_SEGTYPE(symbolP) == SEG_UNKNOWN ) {
			    if ( ! S_IS_COMMON(symbolP) )
				    symbol_append(symbolP, symbol_undef_extern_lastP,
						  &symbol_undef_extern_rootP,
						  &symbol_undef_extern_lastP);
			}
			else
				symbol_append(symbolP, symbol_def_extern_lastP, &symbol_def_extern_rootP, &symbol_def_extern_lastP);
			symbolP = hold;
		}
		else if ( S_GET_STORAGE_CLASS(symbolP) == C_LABEL
			 && TC_S_IS_BALNAME(symbolP)
			 && ! S_IS_DEBUG(symbolP) 
			 && ! S_IS_FUNCTION(symbolP) )
		{
			/* Label, but special case for leafproc BAL-entry points. 
			 * Put them at the end like a global label.
			 */
			symbolS *hold = symbol_previous(symbolP);
			symbol_remove(symbolP, &symbol_rootP, &symbol_lastP);
			symbol_append(symbolP, symbol_balname_lastP, &symbol_balname_rootP, &symbol_balname_lastP);
			symbolP = hold;
		} 
		else 
		{
			if (SF_GET_STRING(symbolP)) 
			{
				symbolP->sy_name_offset = string_byte_count;
				string_byte_count += strlen(S_GET_NAME(symbolP)) + 1;
			} 
			else 
			{
				symbolP->sy_name_offset = 0;
			} /* fix "long" names */

			symbolP->sy_number = symbol_number;
			symbol_number += 1 + S_GET_NUMBER_AUXILIARY(symbolP);
		}
		
	} /* traverse the symbol list */

	/* Minor kludge to make .file's value field consistent with linked
	 * executables.  Its value should contain the symbol index of the
	 * first external symbol.  If there are no externs, then set it to 0
	 * (already the case).  Note that at this point, we don't know what
	 * the correct symbol number is.  At the time we do, we don't have
	 * a pointer to the first external symbol.  Hence the mild hackery.
	 */
	if ( symbol_def_extern_rootP )
		symbol_first_externP = symbol_def_extern_rootP;
	else if ( common_symbolP )
		symbol_first_externP = common_symbolP;
	else if ( symbol_undef_extern_rootP )
		symbol_first_externP = symbol_undef_extern_rootP;
	
	/* Now append the special symbols to the main list in the right order.
	 * You can also number them at this time, and fix up the string table
	 * if need be.
	 */
	for ( ; symbol_static_rootP; ) 
	{
		symbolS *tmp = symbol_static_rootP;

		symbol_remove(tmp, &symbol_static_rootP, &symbol_static_lastP);
		symbol_append(tmp, symbol_lastP, &symbol_rootP, &symbol_lastP);

		/* and process */
		if (SF_GET_STRING(tmp)) 
		{
			tmp->sy_name_offset = string_byte_count;
			string_byte_count += strlen(S_GET_NAME(tmp)) + 1;
		} 
		else 
		{
			tmp->sy_name_offset = 0;
		} /* fix "long" names */

		tmp->sy_number = symbol_number;
		symbol_number += 1 + S_GET_NUMBER_AUXILIARY(tmp);
	} /* append the statics chain */

	for ( ; symbol_balname_rootP; ) 
	{
		symbolS *tmp = symbol_balname_rootP;

		symbol_remove(tmp, &symbol_balname_rootP, &symbol_balname_lastP);
		symbol_append(tmp, symbol_lastP, &symbol_rootP, &symbol_lastP);

		/* and process */
		if (SF_GET_STRING(tmp)) 
		{
			tmp->sy_name_offset = string_byte_count;
			string_byte_count += strlen(S_GET_NAME(tmp)) + 1;
		} 
		else 
		{
			tmp->sy_name_offset = 0;
		} /* fix "long" names */

		tmp->sy_number = symbol_number;
		symbol_number += 1 + S_GET_NUMBER_AUXILIARY(tmp);
	} /* append the BAL-entrypoints chain */

	for ( ; symbol_def_extern_rootP; ) 
	{
		symbolS *tmp = symbol_def_extern_rootP;

		symbol_remove(tmp, &symbol_def_extern_rootP, &symbol_def_extern_lastP);
		symbol_append(tmp, symbol_lastP, &symbol_rootP, &symbol_lastP);

		/* and process */
		if (SF_GET_STRING(tmp)) 
		{
			tmp->sy_name_offset = string_byte_count;
			string_byte_count += strlen(S_GET_NAME(tmp)) + 1;
		} 
		else 
		{
			tmp->sy_name_offset = 0;
		} /* fix "long" names */

		tmp->sy_number = symbol_number;
		symbol_number += 1 + S_GET_NUMBER_AUXILIARY(tmp);
	} /* append the defined externs chain */

	for ( ; common_symbolP; common_symbolP = top_of_comm_sym_chain() ) 
	{
		symbolS *tmp = common_symbolP;

		symbol_append(tmp, symbol_lastP, &symbol_rootP, &symbol_lastP);

		/* and process */
		if (SF_GET_STRING(tmp)) 
		{
			tmp->sy_name_offset = string_byte_count;
			string_byte_count += strlen(S_GET_NAME(tmp)) + 1;
		} 
		else 
		{
			tmp->sy_name_offset = 0;
		} /* fix "long" names */

		tmp->sy_number = symbol_number;
		symbol_number += 1 + S_GET_NUMBER_AUXILIARY(tmp);
	} /* append the commons externs chain */

	for ( ; symbol_undef_extern_rootP; ) 
	{
		symbolS *tmp = symbol_undef_extern_rootP;

		symbol_remove(tmp, &symbol_undef_extern_rootP, &symbol_undef_extern_lastP);
		symbol_append(tmp, symbol_lastP, &symbol_rootP, &symbol_lastP);

		/* and process */
		if (SF_GET_STRING(tmp)) 
		{
			tmp->sy_name_offset = string_byte_count;
			string_byte_count += strlen(S_GET_NAME(tmp)) + 1;
		} 
		else 
		{
			tmp->sy_name_offset = 0;
		} /* fix "long" names */

		tmp->sy_number = symbol_number;
		symbol_number += 1 + S_GET_NUMBER_AUXILIARY(tmp);
	} /* append the undefined externs chain */

	/* Minor kludge to make .file's value field consistent with linked
	 * executables.  Its value should contain the symbol index of the
	 * first external symbol.  If there are no externs, then set it to 0
	 * (already the case).  NOTE: symbol_rootP is guaranteed to point
	 * to the .file symbol at this point.
	 */
	if ( symbol_first_externP )
		S_SET_VALUE(symbol_rootP, symbol_first_externP->sy_number);
	
	/* When a tag reference preceeds the tag definition, the definition 
	 * will not have a number at the time we process the reference 
	 * during the first traversal so we must number them now.
	 */
	for (symbolP = symbol_rootP; symbolP; symbolP = symbol_next(symbolP)) 
	{
		int   storclass;
		if (SF_GET_TAGGED(symbolP)) 
		{
			storclass = S_GET_STORAGE_CLASS((symbolS*) SA_GET_SYM_TAGNDX(symbolP));
			if ( storclass != C_STRTAG && storclass != C_UNTAG && storclass != C_ENTAG )
				as_bad("Tag '%s' has no definition.  Referenced by symbol '%s'.", 
				       S_GET_PRTABLE_NAME((symbolS *) SA_GET_SYM_TAGNDX(symbolP)), 
				       S_GET_PRTABLE_NAME(symbolP));
			SA_SET_SYM_TAGNDX(symbolP, ((symbolS*) SA_GET_SYM_TAGNDX(symbolP))->sy_number);
		} /* If the symbol has a tagndx entry, resolve it */
	} 

	H_SET_SYMBOL_TABLE_SIZE(headers, symbol_number);

} /* obj_crawl_symbol_chain() */



/*
 * Find strings by crawling along symbol table chain.
 * Note that the size is always written, even if 0.
 */

void obj_emit_strings()
{
	symbolS *symbolP;

	output_file_append ((char *) &string_byte_count, sizeof(string_byte_count), out_file_name);
	output_byte_count += sizeof(string_byte_count);

	if ( string_byte_count )
	{
		for (symbolP = symbol_rootP; symbolP; symbolP = symbol_next(symbolP)) 
		{
			if (SF_GET_STRING(symbolP)) 
			{
				output_file_append(S_GET_NAME(symbolP), strlen(S_GET_NAME(symbolP)) + 1, out_file_name);
				output_byte_count += strlen(S_GET_NAME(symbolP)) + 1;
			} /* if it has a string */
		} /* walk the symbol chain */
	} /* if there are any strings */

	return;
} /* obj_emit_strings() */



/*
 * Write an OMF-dependent object file to the disk file.  
 * Do lots of other bookkeeping stuff.
 * The call to this in the source will be "write_object_file()".
 */
void write_coff_file() 
{
	int  		seg;		/* Track along all segments. */
	fragS 		*fragP;		/* Track along all frags. */
	relax_addressT	slide;  	/* Adjust starting frag addresses */
	short		num_sections;   /* number of output sections */
	short		filhdr_flags;	/* temp related to file header */
	short		relocs;		/* temp related to file header */
	short		linenos;	/* temp related to file header */
	long		tot_relocs;	/* temp related to file header */
	long		tot_linenos;	/* temp related to file header */
	symbolS		*symP;		/* general-purpose symbol ptr */

	/*
	 * Check the .bb/.eb balance and also the .bf/.ef balance
	 */
	if ( block_stack->pointer != 0 )
		as_bad ("Unbalanced .bb/.eb symbols (too many .bb's)");
	if ( fcn_stack->pointer != 0 )
		as_bad ("Unbalanced .bf/.ef symbols (too many .bf's)");

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

		SEG_SET_SIZE(seg, md_segment_align((long) SEG_GET_FRAG_LAST(seg)->fr_address, 2));
	}

	/*
	 * Now the addresses of frags are correct within each segment.
	 * Join all user segments into 1 huge segment.
	 *
	 * This has to be done for COFF, because some symbol values reflect
	 * the symbol's object file address, not just the offset within the 
	 * section.  We won't actually connect the links into a linked list, 
	 * but we fix up frag addresses as if we had.
	 * 
	 */
	for ( slide = 0, seg = FIRST_PROGRAM_SEGMENT; seg < segs_size - 1; ++seg )
	{
		slide = md_segment_align (SEG_GET_FRAG_LAST(seg)->fr_address, 2);
		slide = md_segment_align (slide, SEG_GET_ALIGN(seg+1));

		/* Now adjust all the frag addresses in the NEXT segment. */
		for ( fragP = SEG_GET_FRAG_ROOT(seg + 1); fragP; fragP = fragP->fr_next )
		{
			fragP->fr_address += slide;
		}
	} /* for each program segment except the last one */


	/*
	 * Fill in value fields for segment (section) names.
	 * Fill in their auxiliary entries with the section length.
	 */
	for ( seg = FIRST_PROGRAM_SEGMENT; seg < segs_size; ++seg )
	{
		symbolS *symP = symbol_find (SEG_GET_NAME(seg));
		know ( symP );
		S_SET_VALUE(symP, SEG_GET_FRAG_ROOT(seg)->fr_address);
		SA_SET_SCN_SCNLEN(symP, SEG_GET_SIZE(seg));
	}

	/*
	 *
	 * Crawl the symbol chain.
	 *
	 * For each symbol whose value depends on a frag, take the address of
	 * that frag and subsume it into the value of the symbol.
	 *
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

	string_byte_count = sizeof(string_byte_count);

	obj_crawl_symbol_chain(&headers);

	md_check_leafproc_list();

	/* This weird logic is because the first word of the string table is
	 * the size of the string table; if there are no strings, set the size
	 * to 0.
	 */
	if ( string_byte_count == sizeof(string_byte_count) )
		string_byte_count = 0;

	/*
	 * Addresses of frags now reflect addresses we use in the object file.
	 * Symbol values are correct.
	 * Scan the frags, converting any ".org"s and ".align"s to ".fill"s.
	 * Also convert any machine-dependent frags using md_convert_frag();
	 */
  
	for ( seg = FIRST_PROGRAM_SEGMENT; seg < segs_size; ++seg )
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
	} /* for each segment. */


	/* 
	 * Fix any .set expressions that could not be evaluated during 
	 * the first pass. (do_delayed_evaluation is in write.c)
	 */
	do_delayed_evaluation();  

	/* 
	 * Scan the relocations for each segment performing as many fixups
	 * as can be done now.  Keep a count of the relocations that still
	 * must be written to the object file.
	 *
	 * Use same loop to scan the line number chain for each segment.
	 *
	 * For each segment, after the above tasks are done fill in the 
	 * section header and section symbol fields for relocs and line nos.
	 *
	 * For the file header flags, we need to know if there are ANY line
	 * numbers or relocations.
	 */
	for ( tot_relocs = tot_linenos = 0, seg = FIRST_PROGRAM_SEGMENT; seg < segs_size; ++seg )
	{
		relocs = 0;
		linenos = 0;
		symP = symbol_find ( SEG_GET_NAME(seg) );
		know ( symP );
		if ( SEG_GET_TYPE(seg) == SEG_TEXT || SEG_GET_TYPE(seg) == SEG_DATA )
		{
			relocs = fixup_segment (seg);
			/* This error should never occur as in the rest of the
			   assembler, guards are made against allowing fixups
			   for BSS type sections.  However, ... just in case,
			   ... this is left here. */			
			if (SEG_IS_BSS(seg) && relocs)
				as_bad("Relocations found for bss-type section");
		}
		SEG_SET_NRELOC(seg, relocs);
		SA_SET_SCN_NRELOC(symP, relocs);

		/* NOTE: linno's representing functions can not be resolved
		 * now. So, resolve them on emit.
		 */
		if ( SEG_GET_TYPE(seg) == SEG_TEXT )
		{
			lineno	*lineP;
			for ( lineP = SEG_GET_LINE_ROOT(seg); lineP; lineP = lineP->next )
			{
				++linenos;
				if (lineP->line.l_lnno > 0) 
				{
					lineP->line.l_addr.l_paddr += ((fragS*)lineP->frag)->fr_address;
				} 
			}
		}
		SEG_SET_NLNNO(seg, linenos);
		SA_SET_SCN_NLINNO(symP, linenos);
		tot_relocs += relocs;
		tot_linenos += linenos;
	}

	/* 
	 * Fill in whatever file header fields you can now.
	 * The rest will be filled in as the object file is written.
	 * Then the file header itself will be written out last.
	 */
	H_SET_FILE_MAGIC_NUMBER(&headers, FILE_HEADER_MAGIC);
	num_sections = segs_size - FIRST_PROGRAM_SEGMENT;
	H_SET_NUMBER_OF_SECTIONS(&headers, num_sections);
	if ( flagseen ['z'] )
		/* Turn off time/date stamp (set it to 0) */
		H_SET_TIME_STAMP(&headers,0);
	else
		H_SET_TIME_STAMP(&headers, (long)time((long*)0));

	H_SET_SIZEOF_OPTIONAL_HEADER(&headers, 0);

	filhdr_flags = 0;
	filhdr_flags |= tot_relocs ? 0 : F_RELFLG;
	filhdr_flags |= tot_linenos ? 0 : F_LNNO;
	filhdr_flags |= BYTE_ORDERING;
	filhdr_flags |= pic_flag ? F_PIC : 0;
	filhdr_flags |= pid_flag ? F_PID : 0;
	filhdr_flags |= link_pix_flag ? F_LINKPID : 0;

	H_SET_FLAGS(&headers, filhdr_flags);

	/* OR the target architecture into the flags */
	tc_coff_headers_hook(&headers);

	/* 
	 * Fill in whatever section header fields you can now.
	 * The rest will be filled in as the object file is written.
	 * Then the section headers themselves will be written out last.
	 */

	for ( seg = FIRST_PROGRAM_SEGMENT; seg < segs_size; ++seg )
	{
		SEG_SET_PADDR(seg, SEG_GET_FRAG_ROOT(seg)->fr_address);
		SEG_SET_VADDR(seg, SEG_GET_FRAG_ROOT(seg)->fr_address);
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
	 * enough space to fit the file header and section headers at the 
	 * beginning of the file.
	 *
	 * Old method had us figuring out to the byte how big the output file
	 * would be, then allocating a huge buffer to hold it, writing the 
	 * object file into the buffer, then writing the buffer to the output
	 * file all at once.
	 *
	 * Instead, we will write the output file as we go.  This saves the
	 * memory used by the buffer, and also precludes the need to figure 
	 * out exactly how big the output file will be beforehand.
	 */

	output_file_create(out_file_name);
	output_byte_count = FILHSZ + num_sections * SCNHSZ;
	output_file_seek(output_byte_count, out_file_name);

	for ( seg = FIRST_PROGRAM_SEGMENT; seg < segs_size; ++seg )
	{
		/* First, adjust the byte count (and the file) to reach the 
		 * next word boundary.
		 */
		output_file_align();
		SEG_SET_SCNPTR(seg, SEG_IS_BSS(seg) ? 0 : output_byte_count);
		for (fragP = SEG_GET_FRAG_ROOT(seg);  fragP;  fragP = fragP->fr_next) 
		{
			register long		count;
			register char *		fill_literal;
			register long		fill_size;
			
			know( fragP->fr_type == rs_fill );
			/* Do the fixed part. */

			if (SEG_IS_BSS(seg)) {
			    MAKE_SURE_ZERO(fragP->fr_literal,fragP->fr_fix);
			}
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
					    MAKE_SURE_ZERO(fill_literal,
								     count > fill_size ? fill_size : count);
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

	/* Make sure last emitted section ends on a word boundary. */
	output_file_align();

	/*
	 * Write out the relocations.  Fill in section headers as you go.
	 */
	for ( seg = FIRST_PROGRAM_SEGMENT; seg < segs_size; ++seg )
	{
		SEG_SET_RELPTR(seg, output_byte_count);
		obj_emit_relocations(SEG_GET_FIX_ROOT(seg), 0);

		/* Set the relptr field to NULL if there are no relocations */
		if ( SEG_GET_NRELOC(seg) == 0 )
			SEG_SET_RELPTR(seg, 0);
	} /* for each segment. */	  
	  
	/*
	 * Write out the line number entries.  Fill in section headers.
	 */
	for ( seg = FIRST_PROGRAM_SEGMENT; seg < segs_size; ++seg )
	{
		SEG_SET_LNNOPTR(seg, output_byte_count);
		obj_coff_emit_lineno(SEG_GET_LINE_ROOT(seg));

		/* Set the linnoptr field to NULL if there are no line numbers */
		if ( SEG_GET_NLNNO(seg) == 0 )
			SEG_SET_LNNOPTR(seg, 0);
	}

	/*
	 * Write out symbols.  Fill in file header start-of-symbol-table field.
	 */
	H_SET_SYMBOL_TABLE_POINTER(&headers, output_byte_count);
	obj_emit_symbols(symbol_rootP, &headers);
	
	/*
	 * Write out strings.
	 */
	obj_emit_strings();

#ifdef GNU960
	i960_emit_ccinfo();
#endif

	/*
	 * We've written everything we're going to write to the output file.
	 * File header and section headers are now correct.
	 * Go back to the beginning of the file and put out the file header 
	 * and section headers.
	 */
	output_file_seek (0L, out_file_name);

	output_file_append ((char *) &headers.filehdr, FILHSZ, out_file_name);
	
	/* Translate internal segment structures into COFF-specific
	   section header structures. */
	for ( seg = FIRST_PROGRAM_SEGMENT; seg < segs_size; ++seg )
	{
	    unsigned long seg_align;
	    SCNHDR scnhdr; /* COFF-specific section header */

	    /* Adjust seg alignment field from power-of-2 to the 
	       actual number */

	    seg_align = SEG_GET_ALIGN(seg) ? md_segment_align(1L, SEG_GET_ALIGN(seg)) : 0;
	    /* Plus special case alignment for bss section */
	    if ( SEG_GET_TYPE(seg) == SEG_BSS && seg_align < 16 )
		    seg_align = 16;

	    coff_write_scnhdr(seg, &scnhdr, seg_align);
	    output_file_append ((char *) &scnhdr, SCNHSZ, out_file_name);
	}

	/*
	 *  VOILA!  What could be simpler or more pleasant?  :-}
	 */
	output_file_close(out_file_name);

#ifdef	DEBUG
	if ( flagseen ['T'] )
	{
		FILE	*fp = fopen (instr_count_file, "r");
		if ( fp != NULL )
		{
			fscanf (fp, "%d ", & FILE_run_count);
			fscanf (fp, "%d ", & FILE_tot_instr_count);
			fscanf (fp, "%d ", & FILE_mem_instr_count);
			fscanf (fp, "%d ", & FILE_mema_to_memb_count);
			rewind (fp);
		}
		/* These will be zero by default if not initialized from the file */
		FILE_run_count++;
		FILE_tot_instr_count += tot_instr_count;
		FILE_mem_instr_count += mem_instr_count;
		FILE_mema_to_memb_count += mema_to_memb_count;
		fclose (fp);

		fp = fopen (instr_count_file, "w");
		if ( fp == NULL )
			as_fatal ("Can not open trace file for writing: %s\n", instr_count_file);
		fprintf (fp, "%d\n", FILE_run_count);
		fprintf (fp, "%d\n", FILE_tot_instr_count);
		fprintf (fp, "%d\n", FILE_mem_instr_count);
		fprintf (fp, "%d\n", FILE_mema_to_memb_count);
		fclose (fp);

		if ( ! flagseen ['Q'] )
		{
			fprintf (stderr, "Total instructions assembled: %d\n", tot_instr_count);
			fprintf (stderr, "Total MEM-format instructions: %d\n", mem_instr_count);
			fprintf (stderr, "MEMA to MEMB conversions: %d\n", mema_to_memb_count);
		}
	}
#endif
} /* write_coff_file() */


/* translate an internal, generic section header into COFF-specific format. 
*/
coff_write_scnhdr(seg, cp, seg_align)
    int seg;   	    /* index into segs[] array */
    SCNHDR *cp;     /* COFF-specific section header struct */
    unsigned long seg_align;
{
    struct segment *sp = &segs[seg];

    /* Do not assume that COFF fields have been zeroed */
    memset((char *) cp, 0, SCNHSZ);

    strncpy(cp->s_name, sp->seg_name, 8);
    cp->s_paddr = sp->seg_paddr;
    cp->s_vaddr = sp->seg_vaddr;
    cp->s_size = sp->seg_size;
    cp->s_scnptr = sp->seg_scnptr;
    cp->s_relptr = sp->seg_relptr;
    cp->s_lnnoptr = sp->seg_lnnoptr;
    cp->s_nreloc = sp->seg_nreloc;
    cp->s_nlnno = sp->seg_nlnno;
    cp->s_align = seg_align;
    cp->s_flags = STYP_REG;
    if (1) {
	static struct seg_flags_value_map { unsigned long attr,coffism; }
	sfvm[] = {
	    { SECT_ATTR_TEXT, STYP_TEXT },
	    { SECT_ATTR_DATA, STYP_DATA },
	    { SECT_ATTR_BSS,  STYP_BSS },
	    { SECT_ATTR_INFO, STYP_INFO },
	    { 0, 0}
	};
	struct seg_flags_value_map *p = sfvm;

	for (;p->attr;p++)
		if ((sp->seg_flags & p->attr) == p->attr)
			cp->s_flags |= p->coffism;
    }
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


#ifdef DEBUG
char *s_get_name(s)
symbolS *s;
{
	return((s == NULL) ? "(NULL)" : S_GET_NAME(s));
} /* s_get_name() */

void symbol_dump() {
	symbolS *symbolP;

	for (symbolP = symbol_rootP; symbolP; symbolP = symbol_next(symbolP)) {
		printf("%3ld: 0x%lx \"%s\" type = %ld, class = %d, segment = %d\n",
		       symbolP->sy_number,
		       (unsigned long) symbolP,
		       S_GET_NAME(symbolP),
		       (long) S_GET_DATA_TYPE(symbolP),
		       S_GET_STORAGE_CLASS(symbolP),
		       (int) S_GET_SEGTYPE(symbolP));
	} /* traverse symbols */

	return;
} /* symbol_dump() */
#endif /* DEBUG */

/*
 * Local Variables:
 * comment-column: 0
 * fill-column: 131
 * End:
 */

/* end of obj-coff.c */
