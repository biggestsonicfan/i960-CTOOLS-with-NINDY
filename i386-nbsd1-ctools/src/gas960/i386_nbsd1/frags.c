/* frags.c - manage frags -
   Copyright (C) 1987, 1990, 1991 Free Software Foundation, Inc.

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

/* static const char rcsid[] = "$Id: frags.c,v 1.11 1995/01/13 17:14:50 paulr Exp $"; */

#include "as.h"
#include "obstack.h"

struct obstack  frags;	/* All, and only, frags live here. */

/* Global vars related to general segments */
extern segS	*segs;		/* Internal segment array */
extern int  	curr_seg;	/* Active segment (index into segs[]) */

fragS zero_address_frag = {
	0,			/* fr_address */
	NULL,			/* fr_next */
	0,			/* fr_fix */
	0,			/* fr_var */
	0,			/* fr_symbol */
	0,			/* fr_offset */
	NULL,			/* fr_opcode */
	rs_fill,		/* fr_type */
	0,			/* fr_subtype */
	0,			/* fr_pcrel_adjust */
	0,			/* fr_bsr */
	0			/* fr_literal [0] */
};

/*
 *			frag_grow()
 *
 * Internal.
 * Try to augment current frag by nchars chars.
 * If there is no room, close of the current frag with a ".fill 0"
 * and begin a new frag. Unless the new frag has nchars chars available
 * do not return. Do not set up any fields of *curr_frag.
 */
static void frag_grow(nchars)
unsigned int nchars;
{
    if (obstack_room (&frags) < nchars) {
	unsigned int n,oldn;
	long oldc;

	frag_wane(curr_frag);
	frag_new(0);
	oldn=(unsigned)-1;
	oldc=frags.chunk_size;
	frags.chunk_size=2*nchars;
	while((n=obstack_room(&frags))<nchars && n<oldn) {
		frag_wane(curr_frag);
		frag_new(0);
		oldn=n;
	}
	frags.chunk_size=oldc;
    }
    if (obstack_room (&frags) < nchars)
	as_fatal("Internal error: can't extend frag %d chars", nchars);
} /* frag_grow() */
 
/*
 *			frag_finish()
 *
 * Call this to close off a completed frag.
 */
void frag_finish(old_frags_var_max_size)
	int old_frags_var_max_size;	
	/* Number of chars (already allocated on obstack frags) */
{

	if ( curr_frag )
	{
		/* Fix up old frag's fr_fix. */
		curr_frag->fr_fix = (char *) (obstack_next_free (&frags)) -
			(curr_frag->fr_literal) - old_frags_var_max_size;
	}
}

/*
 *	frag_init()
 *
 * Call this to start up a new (empty) frag in any segment.
 * Assumes that curr_frag and curr_seg have already been set.
 */
void frag_init()
{
	long	tmp;		/* JF: for alignment trickery */
	fragS	*sav_curr_frag = curr_frag;  /* for setting the fr_prev field */

	obstack_finish (&frags);
	/* This will align the obstack so the next struct we allocate 
	 * on it will begin at a correct boundary
	 */

	obstack_blank (&frags, SIZEOF_STRUCT_FRAG);
	/* We expect this will begin at a correct boundary for a struct.
	 */

	tmp = obstack_alignment_mask(&frags);
	obstack_alignment_mask(&frags) = 0;		
	/* Turn off alignment.  If we ever hit a machine where strings must be
	 * aligned, we Lose Big
	 */
	
	if ( curr_frag )
	{
		curr_frag->fr_next = (fragS *) obstack_finish(&frags);
		curr_frag = curr_frag->fr_next;
		SEG_SET_FRAG_LAST(curr_seg, curr_frag);
	}
	else
	{
		/* First time this has been called for this segment. */
		curr_frag = (fragS *) obstack_finish(&frags);
		SEG_SET_FRAG_ROOT(curr_seg, curr_frag);
		SEG_SET_FRAG_LAST(curr_seg, curr_frag);
	}
	obstack_alignment_mask(&frags) = tmp;		/* Restore alignment */
	/* Just in case we didn't get zero'd bytes */
	bzero(curr_frag, SIZEOF_STRUCT_FRAG);
	curr_frag->fr_prev = sav_curr_frag;
	curr_frag->fr_big_endian = (SEG_IS_BIG_ENDIAN(curr_seg) ? 1 : 0);
}	/* frag_init() */


/*
 *			frag_new()
 *
 * Call this to close off a completed frag, and start up a new (empty)
 * frag, IN THE SAME SEGMENT, IN THE SAME FRAG CHAIN.
 *
 * Because this calculates the correct value of fr_fix by
 * looking at the obstack 'frags', it needs to know how many
 * characters at the end of the old frag belong to (the maximal)
 * fr_var: the rest must belong to fr_fix.
 * It doesn't actually set up the old frag's fr_var: you may have
 * set fr_var == 1, but allocated 10 chars to the end of the frag:
 * in this case you pass old_frags_var_max_size == 10.
 *
 * Make a new frag, initialising some components. Link new frag at end
 * of curr_frag.
 */
void frag_new(old_frags_var_max_size)
	int old_frags_var_max_size;	
	/* Number of chars (already allocated on obstack frags) */
{
	long	tmp;		/* JF: for alignment trickery */
	fragS	*sav_curr_frag = curr_frag;  /* for setting the fr_prev field */

	frag_finish(old_frags_var_max_size);

	obstack_finish (&frags);
	/* This will align the obstack so the next struct we allocate 
	 * on it will begin at a correct boundary
	 */

	obstack_blank (&frags, SIZEOF_STRUCT_FRAG);
	/* We expect this will begin at a correct boundary for a struct.
	 */

	tmp = obstack_alignment_mask(&frags);
	obstack_alignment_mask(&frags) = 0;		
	/* Turn off alignment.  If we ever hit a machine where strings must be
	 * aligned, we Lose Big
	 */
	
	if ( curr_frag )
	{
		curr_frag->fr_next = (fragS *) obstack_finish(&frags);
		curr_frag = curr_frag->fr_next;
		SEG_SET_FRAG_LAST(curr_seg, curr_frag);
	}
	else
	{
		/* First time this has been called for this segment. */
		curr_frag = (fragS *) obstack_finish(&frags);
		SEG_SET_FRAG_ROOT(curr_seg, curr_frag);
		SEG_SET_FRAG_LAST(curr_seg, curr_frag);
	}
	
	obstack_alignment_mask(&frags) = tmp;		/* Restore alignment */
	/* Just in case we didn't get zero'd bytes */
	bzero(curr_frag, SIZEOF_STRUCT_FRAG);
	curr_frag->fr_prev = sav_curr_frag;
	curr_frag->fr_big_endian = (SEG_IS_BIG_ENDIAN(curr_seg) ? 1 : 0);
}	/* frag_new() */
 
/*
 *			frag_more()
 *
 * Start a new frag unless we have n more chars of room in the current frag.
 * Close off the old frag with a .fill 0.
 *
 * Return the address of the 1st char to write into. Advance
 * curr_frag_growth past the new chars.
 */

char *frag_more (nchars)
int nchars;
{
    register char  *retval;

    frag_grow (nchars);
    retval = obstack_next_free (&frags);
    obstack_blank_fast (&frags, nchars);
    return (retval);
}				/* frag_more() */
 
/*
 *			frag_var()
 *
 * Start a new frag unless we have max_chars more chars of room in the current frag.
 * Close off the old frag with a .fill 0.
 *
 * Set up a machine_dependent relaxable frag, then start a new frag.
 * Return the address of the 1st char of the var part of the old frag
 * to write into.
 */

char *frag_var(type, max_chars, var, subtype, symbol, offset, opcode)
relax_stateT type;
int max_chars;
int var;
relax_substateT subtype;
symbolS *symbol;
long offset;
char *opcode;
{
    register char  *retval;

    frag_grow (max_chars);
    retval = obstack_next_free (&frags);
    obstack_blank_fast (&frags, max_chars);
    curr_frag->fr_var = var;
    curr_frag->fr_type = type;
    curr_frag->fr_subtype = subtype;
    curr_frag->fr_symbol = symbol;
    curr_frag->fr_offset = offset;
    curr_frag->fr_opcode = opcode;
    /* default this to zero. */
    curr_frag->fr_bsr = 0;
    frag_new (max_chars);
    return (retval);
}				/* frag_var() */
 
/*
 *			frag_variant()
 *
 * OVE: This variant of frag_var assumes that space for the tail has been
 *      allocated by caller.
 *      No call to frag_grow is done.
 *      Two new arguments have been added.
 */

char *frag_variant(type, max_chars, var, subtype, symbol, offset, opcode, ignored, bsr)
     relax_stateT	 type;
     int		 max_chars;
     int		 var;
     relax_substateT	 subtype;
     symbolS		*symbol;
     long		 offset;
     char		*opcode;
     int		 ignored;
     char		 bsr;
{
    register char  *retval;

    retval = obstack_next_free (&frags);
    curr_frag->fr_var = var;
    curr_frag->fr_type = type;
    curr_frag->fr_subtype = subtype;
    curr_frag->fr_symbol = symbol;
    curr_frag->fr_offset = offset;
    curr_frag->fr_opcode = opcode;
    curr_frag->fr_bsr = bsr;
    frag_new (max_chars);
    return (retval);
}				/* frag_variant() */
 
/*
 *			frag_wane()
 *
 * Reduce the variable end of a frag to a harmless state.
 */
void frag_wane(fragP)
register    fragS * fragP;
{
    fragP->fr_type = rs_fill;
    fragP->fr_offset = 0;
    fragP->fr_var = 0;
}
 
/*
 *			frag_align()
 *
 * Make a frag for ".align foo,bar". Call is "frag_align (foo,bar);".
 * Foo & bar are absolute integers.
 *
 * Call to close off the current frag with a ".align", then start a new
 * (so far empty) frag, in the same segment as the last frag.
 */

void frag_align(alignment, fill_character)
int alignment;
int fill_character;
{
	char *buf = frag_var (rs_align, 1, 1, 0, NULL, alignment, NULL);
	*buf = fill_character;
	
	/* For the listing: this will be a fill-type frag eventually.
	 * We can get the details we need from the frag later.
	 */
	if ( listing_now ) listing_info ( fill, 1, buf, curr_frag->fr_prev, 0 );
} /* frag_align() */


/*
 *			frag_alignfill()
 *
 * Make a frag for .alignfill foo, "bar".
 *
 * Call to close off the current frag with a ".align", then start a new
 * (so far empty) frag, in the same segment as the last frag.
 *
 * PS: Note some trickery here: we're setting the var field to its negative
 * inverse.  This is a flag so that later processing (write_coff_file() and
 * write_bout_file()) that emits code knows to completely fill the gap 
 * created by the location counter alignment, even if the gap is not an 
 * exact multiple of the fill pattern size. 
 */
void frag_alignfill(alignment, fill_pattern, pattern_len)
int alignment;
char * fill_pattern;
int pattern_len;
{
	char * buf = frag_var (rs_align, pattern_len, 0 - pattern_len, 
			       0, NULL, (long) alignment, NULL);
	memcpy (buf, fill_pattern, pattern_len);

	/* For the listing: this will be a fill-type frag eventually.
	 * We can get the details we need from the frag later.
	 */
	if ( listing_now ) listing_info ( fill, pattern_len, buf, curr_frag->fr_prev, LIST_BIGENDIAN );
} /* frag_alignfill() */

/* end: frags.c */



