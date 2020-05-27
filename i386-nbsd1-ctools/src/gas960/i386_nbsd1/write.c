/* write.c - emit .o file
   Copyright (C) 1986, 1987, 1990, 1991 Free Software Foundation, Inc.

This file is part of GAS, the GNU Assembler.

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

/* static const char rcsid[] = "$Id: write.c,v 1.36 1995/12/06 22:48:21 paulr Exp $"; */

/* 

   In order to cross-assemble the target machine must have an a.out header
   similar to the one in a.out.h on THIS machine.  Byteorder doesn't matter,
   we take special care of it, but the numbers must be the same SIZE (# of
   bytes) and in the same PLACE.  If this is not true, you will have some
   trouble.
 */

#include "as.h"

#include "obstack.h"
#ifdef GNU960
#	include "out_file.h"
#else
#	include "output-file.h"
#endif

/* Hook for machine dependent relocation information output routine.
   If not defined, the variable is allocated in BSS (Fortran common model).
   If some other module defines it, we will see their value.  */

extern void (*md_emit_relocations)();

/*
 * In: length of relocation (or of address) in chars: 1, 2 or 4.
 * Out: GNU LD relocation length code: 0, 1, or 2.
 */

unsigned char
nbytes_r_length [] = {
  42, 0, 1, 42, 2
  };

/* Global vars related to general segments */
extern segS	*segs;		/* Internal segment array */
extern int  	curr_seg;	/* Active segment (index into segs[]) */
extern int  	segs_size;	/* Number of segS's currently in use in segs */

long string_byte_count;

int magic_number_for_object_file = DEFAULT_MAGIC_NUMBER_FOR_OBJECT_FILE;

/* static long		length; JF unused */	/* String length, including trailing '\0'. */


#ifdef __STDC__

static int is_dnrange(struct frag *f1, struct frag *f2);
long fixup_segment(int   this_segment);
static relax_addressT relax_align(relax_addressT address, long alignment);
void relax_segment(int segment);

#else

static int is_dnrange();
long fixup_segment();
static relax_addressT relax_align();
void relax_segment();

#endif /* __STDC__ */

/*
 *			fix_new()
 *
 * Create a fixS in obstack 'notes'.
 */
fixS *fix_new(frag, where, size, add_symbol, sub_symbol, offset, pcrel, r_type)
fragS *frag;		/* Which frag? */
int where;		/* Where in that frag? */
short int size;		/* 1, 2  or 4 usually. */
symbolS *add_symbol;	/* X_add_symbol. */
symbolS *sub_symbol;	/* X_subtract_symbol. */
long offset;		/* X_add_number. */
int pcrel;		/* TRUE if PC-relative relocation. */
enum reloc_type	r_type;	/* Relocation type */
{
  register fixS *	fixP;

  fixP = (fixS *)obstack_alloc(&notes,sizeof(fixS));
  bzero(fixP, sizeof(*fixP));

  fixP->fx_frag		= frag;
  fixP->fx_where	= where;
  fixP->fx_size		= size;
  fixP->fx_addsy	= add_symbol;
  fixP->fx_subsy	= sub_symbol;
  fixP->fx_offset	= offset;
  fixP->fx_pcrel	= pcrel;
  fixP->fx_r_type	= r_type;
  fixP->fx_next		= NULL;
  fixP->fx_im_disp	= 0;
  fixP->fx_bit_fixP	= 0;

  if (*seg_fix_tailP)
    (*seg_fix_tailP)->fx_next = fixP;
  else
    *seg_fix_rootP = fixP;
  *seg_fix_tailP = fixP;
  fixP->fx_callj = 0;
  fixP->fx_calljx = 0;
  return fixP;
}
 



/*
 *			relax_segment()
 *
 * Relax the frags.
 *
 * After this, all frags in this segment have addresses that are correct
 * within the segment. Since segments live in different file addresses,
 * these frag addresses may not be the same as final object-file addresses.
 */
void relax_segment(segment)
     int	segment; /* segs array index */
{
	register struct frag *	fragP;
	register relax_addressT	address;
	struct frag 		*segment_frag_root = SEG_GET_FRAG_ROOT(segment);
	register long		stretch; 
				/* May be any size, 0 or negative.
				 * Cumulative number of addresses we have 
				 * relaxed this pass.
				 * We may have relaxed more than one address.
				 */
	register long 		stretched;  
				/* Have we stretched on this pass? 
				 * This is 'cuz stretch may be zero, when,
				 * in fact some piece of code grew, and
				 * another shrank.  If a branch instruction
				 * doesn't fit anymore, we could be scrod 
				 */
	
	/*
	 * For each frag in segment: count and store  (a 1st guess of) fr_address.
	 */
	address = 0;
	for ( fragP = segment_frag_root;   fragP;   fragP = fragP->fr_next )
	{
		fragP->fr_address = address;
		address += fragP->fr_fix;
		switch (fragP->fr_type)
		{
		case rs_fill:
			address += fragP->fr_offset * fragP->fr_var;
			break;
			
		case rs_align:
			address += relax_align(address, fragP->fr_offset);
			break;
			
		case rs_org:
			/*
			 * Assume .org is nugatory. It will grow with 1st relax.
			 */
			break;
			
		case rs_machine_dependent:
			address += md_estimate_size_before_relax(fragP, segment);
			break;
			
		default:
			BAD_CASE( fragP->fr_type );
			break;
		}			/* switch(fr_type) */
	}				/* for each frag in the segment */
	
	do
	{
		stretch = stretched = 0;
		for (fragP = segment_frag_root;  fragP;  fragP = fragP->fr_next)
		{
			register long growth = 0;
			register unsigned long was_address;
			register long offset;
			register symbolS *symbolP;
			register long target;
			register long after;
			register long aim;
			
			was_address = fragP->fr_address;
			address = fragP->fr_address += stretch;
			symbolP = fragP->fr_symbol;
			offset = fragP->fr_offset;

			switch (fragP->fr_type)
			{
			case rs_fill:	/* .fill never relaxes. */
				growth = 0;
				break;
				
			case rs_align:
				growth = relax_align ((relax_addressT)(address + fragP->fr_fix), offset)
					- relax_align ((relax_addressT)(was_address +  fragP->fr_fix), offset);
				break;
				
			case rs_org:
				target = offset;
				if (symbolP)
				{
					target +=
						S_GET_VALUE(symbolP)
							+ symbolP->sy_frag->fr_address;
				}
				know( fragP->fr_next );
				after = fragP->fr_next->fr_address;
				growth = ((target - after ) > 0) ? (target - after) : 0;
				/* Growth may be -ve, but variable part */
				/* of frag cannot have < 0 chars. */
				/* That is, we can't .org backwards. */
				
				growth -= stretch;	/* This is an absolute growth factor */
				if (growth < 0) growth = 0;  /* stil can't be < 0 */
				break;
				
			case rs_machine_dependent:
			{
				register const relax_typeS *	this_type;
				register const relax_typeS *	start_type;
				register relax_substateT	next_state;
				register relax_substateT	this_state;
				
				start_type = this_type = md_relax_table + (this_state = fragP->fr_subtype);
				target = offset;
				if (symbolP)
				{
					target += S_GET_VALUE(symbolP) + symbolP->sy_frag->fr_address;
					
					/* If frag has yet to be reached on this pass,
					   assume it will move by STRETCH just as we did.
					   If this is not so, it will be because some frag
					   between grows, and that will force another pass.  */
					
					/* JF was just address */
					/* JF also added is_dnrange hack */
					/* There's gotta be a better/faster/etc way
					   to do this. . . */
					/* gnu@cygnus.com:  I changed this from > to >=
					   because I ran into a zero-length frag (fr_fix=0)
					   which was created when the obstack needed a new
					   chunk JUST AFTER the opcode of a branch.  Since
					   fr_fix is zero, fr_address of this frag is the same
					   as fr_address of the next frag.  This
					   zero-length frag was variable and jumped to .+2
					   (in the next frag), but since the > comparison
					   below failed (the two were =, not >), "stretch"
					   was not added to the target.  Stretch was 178, so
					   the offset appeared to be .-176 instead, which did
					   not fit into a byte branch, so the assembler
					   relaxed the branch to a word.  This didn't compare
					   with what happened when the same source file was
					   assembled on other machines, which is how I found it.
					   You might want to think about what other places have
					   trouble with zero length frags... */
					
					if (symbolP->sy_frag->fr_address >= was_address && is_dnrange(fragP,symbolP->sy_frag))
						target += stretch;
					
				}
				aim = target - address - fragP->fr_fix;
				if (aim < 0)
				{
					/* Look backwards. */
					for (next_state = this_type->rlx_more;  next_state;  )
					{
						if (aim >= this_type->rlx_backward)
							next_state = 0;
						else
						{	/* Grow to next state. */
							this_type = md_relax_table + (this_state = next_state);
							next_state = this_type->rlx_more;
						}
					}
				}
				else
				{
					/* Look forwards. */
					for (next_state = this_type->rlx_more;  next_state;  )
					{
						if (aim <= this_type->rlx_forward)
							next_state = 0;
						else
						{	/* Grow to next state. */
							this_type = md_relax_table + (this_state = next_state);
							next_state = this_type->rlx_more;
						}
					}
				}
				if ((growth = this_type->rlx_length - start_type->rlx_length) != 0)
					fragP->fr_subtype = this_state;
			} /* end rs_machine_dependent */
				
				break;
	
			default:
				BAD_CASE( fragP->fr_type );
				break;
			}
			if (growth) {
				stretch += growth;
				stretched++;
			}
		}			/* For each frag in the segment. */
	} while (stretched);	/* Until nothing further to relax. */

	/*
 	 * All fr_address's are correct, relative to their own segment.
 	 * We have made all the fixS we will ever make.
 	 */
}				/* relax_segment() */

char * _S_GET_PRTABLE_NAME(name)
    char *name;
{
    static char buff[30];

    if (name && name[2] == 1) {

	sprintf(buff,"`numeric_label_symbol : %d'",name[1]-'0');
	return buff;
    }
    return name;
}

/*
 * Relax_align. Advance location counter to next address that has 'alignment'
 * lowest order bits all 0s.
 */

 /* How many addresses does the .align take? */
static relax_addressT relax_align(address, alignment)
register relax_addressT address; /* Address now. */
register long alignment; /* Alignment (binary). */
{
  relax_addressT	mask;
  relax_addressT	new_address;

  mask = ~ ( (~0) << alignment );
  new_address = (address + mask) & (~ mask);
  return (new_address - address);
} /* relax_align() */
 
/* fixup_segment()

   Go through all the fixS's in a segment and see which ones can be
   handled now.  (These consist of fixS where we have since discovered
   the value of a symbol, or the address of the frag involved.)
   For each one, call md_apply_fix to put the fix into the frag data.

   Result is a count of how many relocation structs will be needed to
   handle the remaining fixS's that we couldn't completely handle here.
   These will be output later by emit_relocations().  */

long fixup_segment(this_segment)
int  	this_segment; /* segment we want to fix up (segs array index) */
{
	register int size;
	register char *place;
	register long where;
	register char pcrel;
	register fragS *fragP;
	register int add_symbol_segment = -1;
	symbolS *orig_addsy;
	symbolS *orig_subsy;

	fixS *fixP = SEG_GET_FIX_ROOT(this_segment); 
	fixS *topP = fixP;
	long reloc_count = 0;
	
	for ( ; fixP; fixP = fixP->fx_next )
	{
		fragP       = fixP->fx_frag;
		know(fragP);
		where	  = fixP->fx_where;
		place       = fragP->fr_literal + where;
		size	  = fixP->fx_size;
		pcrel	  = fixP->fx_pcrel;
		orig_addsy = fixP->fx_addsy;
		orig_subsy = fixP->fx_subsy;

		if (fixP->fx_addsy) {
			add_symbol_segment = S_GET_SEGMENT(fixP->fx_addsy);
		}	/* if there is an addend */
		
		if (fixP->fx_subsy) 
		{
			if (!fixP->fx_addsy) 
			{
			    /* It's just -sym */
#if (defined(OBJ_COFF) && defined(R_RELLONG_SUB)) || defined(OBJ_ELF)
			    /* Setting up new RELLONG_SUB relocation.  The linker
			       will relocate non-absolute symbols. */
			    switch ( S_GET_SEGTYPE(fixP->fx_subsy) ) 
			    {
			    case SEG_ABSOLUTE:
				if ( pcrel )
				    as_bad("PC-relative relocation with absolute symbol not allowed: %s", S_GET_PRTABLE_NAME(fixP->fx_subsy));
				else
				    fixP->fx_offset -= S_GET_VALUE(fixP->fx_subsy);
				/* Disable further processing */
				fixP->fx_subsy = NULL;
				pcrel = 0;
				break;
					
			    case SEG_BSS:
			    case SEG_DATA:
			    case SEG_TEXT:
				reloc_count ++;
				fixP->fx_offset -= S_GET_VALUE(fixP->fx_subsy);
				break;
				
			    case SEG_UNKNOWN:
				if ( SF_GET_LOCAL(fixP->fx_subsy) )
				{
				    /* Probably a forward reference to a local label
				     * that was never seen.
				     */
				    as_bad("Can't relocate local symbol %s. Check for unresolved numeric labels.", S_GET_PRTABLE_NAME(fixP->fx_subsy));
				    fixP->fx_subsy = NULL;  /* No relocations please. */
				    continue;
				}
				++reloc_count;
				break;
				
			    default:
				as_bad ("Invalid relocation: %s", S_GET_PRTABLE_NAME(fixP->fx_subsy));
				break;
			    } /* switch on subtract symbol seg */
#else /* Not Elf or COFF, or R_RELLONG_SUB not available */
			    if (S_GET_SEGTYPE(fixP->fx_subsy) != SEG_ABSOLUTE) {
				as_bad("Negative of non-absolute symbol %s", S_GET_PRTABLE_NAME(fixP->fx_subsy));
			    } /* not absolute */
			    
			    fixP->fx_offset -= S_GET_VALUE(fixP->fx_subsy);
#endif			    
			        /* if sub_symbol is in the same segment that add_symbol
				   and add_symbol is either in DATA, TEXT, BSS or ABSOLUTE */
			} else if ((S_GET_SEGMENT(fixP->fx_subsy) == add_symbol_segment)
				 && ((S_GET_SEGTYPE(fixP->fx_addsy) == SEG_DATA)
				     || (S_GET_SEGTYPE(fixP->fx_addsy) == SEG_TEXT)
				     || (S_GET_SEGTYPE(fixP->fx_addsy) == SEG_BSS)
				     || (S_GET_SEGTYPE(fixP->fx_addsy) == SEG_ABSOLUTE))) 
			{
				/* Difference of 2 symbols from same segment. */
				/* Makes no sense to use the difference of 2 arbitrary symbols
				 * as the target of a call instruction.
				 */
				if (fixP->fx_callj) {
					as_bad("callj to difference of 2 symbols");
				}
				fixP->fx_offset += S_GET_VALUE(fixP->fx_addsy) - 
				    S_GET_VALUE(fixP->fx_subsy);
				
				fixP->fx_addsy = NULL;
				fixP->fx_subsy = NULL;
			} 
			else 
			{
			    /* Different segments in subtraction. */
#if (defined(OBJ_COFF) && defined(R_RELLONG_SUB)) || defined(OBJ_ELF)
			    /* Setting up new RELLONG_SUB relocation.  Try this,
			       be willing to collapse redundant code together 
			       later.  */
			    switch ( S_GET_SEGTYPE(fixP->fx_subsy) ) 
			    {
			    case SEG_ABSOLUTE:
				if ( pcrel )
				    as_bad("PC-relative relocation with absolute symbol not allowed: %s", S_GET_PRTABLE_NAME(fixP->fx_subsy));
				else
				    fixP->fx_offset -= S_GET_VALUE(fixP->fx_subsy);
				/* Disable further processing */
				fixP->fx_subsy = NULL;
				pcrel = 0;
				break;
					
			    case SEG_BSS:
			    case SEG_DATA:
			    case SEG_TEXT:
				reloc_count ++;
				fixP->fx_offset -= S_GET_VALUE(fixP->fx_subsy);
				break;
				
			    case SEG_UNKNOWN:
				if ( SF_GET_LOCAL(fixP->fx_subsy) )
				{
				    /* Probably a forward reference to a local label
				     * that was never seen.
				     */
				    as_bad("Can't relocate local symbol %s. Check for unresolved numeric labels.", S_GET_PRTABLE_NAME(fixP->fx_subsy));
				    fixP->fx_subsy = NULL;  /* No relocations please. */
				    continue;
				}
				++reloc_count;
				break;
				
			    default:
				as_bad ("Invalid relocation: %s", S_GET_PRTABLE_NAME(fixP->fx_subsy));
				break;
			    } /* switch on subtract symbol seg */
#else /* Not Elf or COFF, or R_RELLONG_SUB not available */
			    if ((S_GET_SEGTYPE(fixP->fx_subsy) == SEG_ABSOLUTE)) 
			    {
				fixP->fx_offset -= S_GET_VALUE(fixP->fx_subsy);
			    } 
			    else 
			    {
				as_bad("Illegal relocation: symbols are external or not in same section: %s - %s",
				       S_GET_PRTABLE_NAME(fixP->fx_addsy),
				       S_GET_PRTABLE_NAME(fixP->fx_subsy));
			    } /* if sub symbol is absolute */
#endif
			}
		    }
		
		if (fixP->fx_addsy) 
		{
		        if ( add_symbol_segment == this_segment
			    && pcrel
			    && ( ! TC_S_IS_SYSPROC(fixP->fx_addsy) 
				|| TC_S_GET_SYSINDEX(fixP->fx_addsy) != -1 ) )
			{
				/*
				 * This fixup was made when the symbol's segment was
				 * SEG_UNKNOWN, but it is now in the local segment.
				 * So we know how to do the address without relocation.
				 */

				/* reloc_callj() may replace a 'call' with a 'calls' or a 'bal',
				 * in which cases it modifies *fixP as appropriate.  In the case
				 * of a 'calls', no further work is required, and *fixP has been
				 * set up to make the rest of the code below a no-op.
				 */
				reloc_callj(fixP);
				
				fixP->fx_offset += S_GET_VALUE(fixP->fx_addsy);
				fixP->fx_offset -= md_pcrel_from (fixP);
				/* Disable further processing */
				pcrel = 0;
				fixP->fx_addsy = NULL;
			} 
			else 
			{
				switch ( S_GET_SEGTYPE(fixP->fx_addsy) ) 
				{
				case SEG_ABSOLUTE:
				    if ( pcrel )
					as_bad("PC-relative relocation with absolute symbol not allowed: %s", S_GET_PRTABLE_NAME(fixP->fx_addsy));
				    else
				    {
					reloc_callj(fixP); /* See comment about reloc_callj() above*/
					fixP->fx_offset += S_GET_VALUE(fixP->fx_addsy);
				    }
				    /* Disable further processing */
				    fixP->fx_addsy = NULL;
				    pcrel = 0;
				    break;
				
				case SEG_BSS:
				case SEG_DATA:
				case SEG_TEXT:
					reloc_count ++;
					fixP->fx_offset += S_GET_VALUE(fixP->fx_addsy);
					break;
					
				case SEG_UNKNOWN:
#if defined( OBJ_COFF ) || defined( OBJ_ELF )
					if ( SF_GET_LOCAL(fixP->fx_addsy) )
					{
						/* Probably a forward reference to a local label
						 * that was never seen.
						 */
					    as_bad("Can't relocate local symbol %s. Check for unresolved numeric labels.", S_GET_PRTABLE_NAME(fixP->fx_addsy));
						fixP->fx_addsy = NULL;  /* No relocations please. */
						continue;
					}
#endif

					if ((int)fixP->fx_bit_fixP == 13) {
						/* This is a COBR instruction.  They have only a
						 * 13-bit displacement and are only to be used
						 * for local branches: flag as error, don't generate
						 * relocation.
						 */
						as_bad("can't use COBR format with external label");
						fixP->fx_addsy = NULL;	/* No relocations please. */
						continue;
					} /* COBR */

					++reloc_count;
					break;
					
				default:
					as_bad ("Invalid relocation: %s",
						S_GET_PRTABLE_NAME(fixP->fx_addsy));
					break;
				} /* switch on symbol seg */
			} /* if not in local seg */
		} /* if there was a + symbol */

		if (pcrel) {
			fixP->fx_offset -= md_pcrel_from(fixP);
			if (fixP->fx_addsy == 0) {
				fixP->fx_addsy = & abs_symbol;
				++reloc_count;
			} /* if there's an add_symbol */
		} /* if pcrel */
		
		if (!fixP->fx_bit_fixP) 
		{
			if ((size==1 &&
			     (fixP->fx_offset & ~0xFF)   && (fixP->fx_offset & ~0xFF!=(-1&~0xFF))) ||
			    (size==2 &&
			     (fixP->fx_offset & ~0xFFFF) && (fixP->fx_offset & ~0xFFFF!=(-1&~0xFFFF)))) 
			{
				as_bad("Value of %d too large for field of %d bytes at 0x%x",
					fixP->fx_offset, size, fragP->fr_address + where);
			} /* generic error checking */
		} /* not a bit fix */
		md_apply_fix(fixP, fixP->fx_offset, orig_addsy?
			     (S_GET_NAME(orig_addsy)):
			     "unknown");
	} /* For each fixS in this segment. */
	
#ifdef OBJ_COFF
	/* two relocs per callj under coff. */
	for (fixP = topP; fixP; fixP = fixP->fx_next) {
		if ( fixP->fx_addsy != 0 && (fixP->fx_callj || fixP->fx_calljx) ) {
		  	++reloc_count;
		} /* if callj and not already fixed. */
	} /* for each fix */
#endif /* OBJ_COFF */
	return(reloc_count);
} /* fixup_segment() */


static int is_dnrange(f1,f2)
struct frag *f1;
struct frag *f2;
{
	while (f1) {
		if (f1->fr_next==f2)
			return 1;
		f1=f1->fr_next;
	}
	return 0;
} /* is_dnrange() */

/* Append a string onto another string, bumping the pointer along.  */
void
append (charPP, fromP, length)
char	**charPP;
char	*fromP;
unsigned long length;
{
	if (length) {		/* Don't trust bcopy() of 0 chars. */
		bcopy(fromP, *charPP, (int) length);
		*charPP += length;
	}
}

/*
 * This routine records the largest alignment seen for each segment.
 * If the beginning of the segment is aligned on the worst-case
 * boundary, all of the other alignments within it will work.  At
 * least one object format really uses this info.
 */
void record_alignment(seg, align)
int   seg;	/* Segment to which alignment pertains */
int   align;	/* Alignment, as a power of 2
		 *	(e.g., 1 => 2-byte boundary, 2 => 4-byte boundary, etc.)
		 */
{
	
	if ( align > (int) SEG_GET_ALIGN(seg) )
	{
		SEG_SET_ALIGN(seg, align);
	} /* if highest yet */
	
} /* record_alignment() */


/* 
 * do_delayed_evaluation()
 *
 * This the infamous "pseudo pass 2" you've heard about. 
 * It is used (for now) for .set's only.
 *
 * You have a list of zero or more expressions that should evaluate
 * to an absolute number at this point.  If not, there is probably 
 * a dependency on one of the other entries in the list.  So you 
 * may have to traverse the list more than once.  The algorithm is not
 * efficient, but it's adequate when there are only a few entries.
 * We can't just remove items from the list after they are processed,
 * because the ORDER in which they are evaluated must reflect the 
 * original source file.
 *
 */
void
do_delayed_evaluation()
{
	delayed_exprS	*delP, *savP, *badP;
	expressionS 	exp;
	int		successes = 0;
	int		total = 99;
	int		last_time = -1;

	if ( del_expr_rootP )
		/* At least one expression was delayed */
		need_pseudo_pass_2 = 1;

	while ( successes < total )
	{
		for ( successes = 0, total = 0, delP = del_expr_rootP; delP; ++total, delP = delP->next )
		{
			bzero (&exp, sizeof(exp));
			input_line_pointer = delP->exprP;
			switch ( expression(&exp) )
			{
			case SEG_ABSOLUTE:
				/* Luck is with us */
				S_SET_VALUE(delP->symP, exp.X_add_number);
				S_SET_SEGMENT(delP->symP, MYTHICAL_ABSOLUTE_SEGMENT);
				++successes;
				break;

			case SEG_TEXT:
			case SEG_DATA:
			case SEG_BSS:
				/* These are, strictly, not allowed.  But they are
				 * historically used by b.out .stabs directive. 
				 */
				S_SET_SEGMENT(delP->symP, exp.X_segment);
				S_SET_VALUE(delP->symP, exp.X_add_number + S_GET_VALUE(exp.X_add_symbol));
				++successes;
				break;

			default:
				/* This one may get resolved by a later entry */
				badP = delP;
				break;
			}
		}
		if ( successes == last_time )
		{
			/* OOPS, nothing more was done on this pass */
			as_bad("Illegal .set expression: .set %s, %s", S_GET_PRTABLE_NAME(badP->symP),
			       badP->exprP);
			need_pass_2 = 1;
			break;
		}
		last_time = successes;
	}
} /* do_delayed_evaluation */

/* 
 * del_expr_append()
 *
 * Add one item to the delayed expression list.
 */
void
del_expr_append(symP, str)
	symbolS	*symP;	/* The symbol to assign later */
	char	*str;   /* The input buffer as it appeared in the first pass */
{
	register char *c;
	if ( del_expr_rootP == NULL )
	{
		del_expr_rootP = del_expr_lastP = 
			(delayed_exprS *) xmalloc (sizeof (delayed_exprS));
	}
	else
	{
		del_expr_lastP->next = 
			(delayed_exprS *) xmalloc (sizeof (delayed_exprS));
		del_expr_lastP = del_expr_lastP->next;
	}
	del_expr_lastP->symP = symP;
	del_expr_lastP->next = NULL;

	/* Find the first newline or semi in the input buffer */
	for ( c = str; *c; ++c )
		if ( *c == '\n' || *c == ';' )
			break;

	del_expr_lastP->exprP = (char *) xmalloc (c - str + 1);
	strncpy(del_expr_lastP->exprP, str, c - str);
	del_expr_lastP->exprP [c - str] = 0;
} /* del_expr_append */


#ifdef  NEED_DEL_EXPR_REMOVE
/* This is currently not needed by the delayed evaluator system.
 * But here it is, if it becomes useful in the future.
 */
void
del_expr_remove(delP)
	delayed_exprS	*delP;	/* The link to remove from the chain */
{
	delayed_exprS	*p;
	if ( delP == del_expr_rootP )
	{
		del_expr_rootP = del_expr_rootP->next;
		if ( del_expr_rootP == NULL )
			del_expr_lastP = NULL;
		free (delP);
	}
	else 
	{
		/* We know there is at least one link preceding delP.
		 * Unfortunately this is only a singly-linked list.
		 */
		for ( p = del_expr_rootP; p && p->next != delP; p = p->next )
			;
		/* p points to the link right before delP */
		if ( p->next == del_expr_lastP )
			del_expr_lastP = p;
		p->next = delP->next;
		free (delP);
	}
} /* del_expr_remove */

#endif  /* DEL_EXPR_REMOVE */
