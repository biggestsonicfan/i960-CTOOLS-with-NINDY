/* segs.c - segments -
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

/* $Id: segs.c,v 1.16 1995/12/05 09:23:21 peters Exp $ */

/*
 * Functions and data structures related to general segments,
 * ultimately to become general sections in the COFF world. 
 */

#include <ctype.h>
#include "as.h"
#include "obstack.h"

segS	*segs;		/* Logical base of internal segment "array" */
int  	curr_seg;	/* Active segment (index into segs[]) */
int  	segs_size;	/* Number of segS's currently in use in segs */
int  	max_segs;	/* Total number of segS's available in segs */

int	DEFAULT_TEXT_SEGMENT; 	/* .text */
int	DEFAULT_DATA_SEGMENT; 	/* .data */
int	DEFAULT_BSS_SEGMENT; 	/* .bss  */

static void segs_resize();

/* seg_new()
 *
 * Initialize a new segment structure using the next available spot in the segs
 * "array".  Return the array index of the new segment.  Set global variables 
 * that reference the array and resize it if needed.  In a program with
 * up to 8 user segments, this will happen only once, when segs_begin() is 
 * called.  When resizing, allocate in blocks of 20 segment structs.
 *
 * If this is a "LOMEM" segment, it needs to be located near the beginning 
 * of the object file. (so its addresses will actually be low.)  Now is the 
 * best time to do this.
 */
int  
seg_new(type, name, flags)
	segT type;
	char *name;
	int flags;
{
	segS	*segP;
	symbolS *symP;

	/* Check if there's enough room, allocate more if needed */
	if ( segs_size == max_segs )
		segs_resize ();

	/* Check if this is going to be a LOMEM segment.
	 * If so, assign this the first spot and move all existing
	 * (real not mythical) segments down one.
	 */
	if ( flags & SEG_LOMEM_MASK )
	{
		segs_push_down ( segs_size );
		segP = & segs[FIRST_PROGRAM_SEGMENT];
		memset((char *) segP, 0, sizeof(segS));
		segP->seg_no = FIRST_PROGRAM_SEGMENT;
		segs_size++;
	}
	else
	{
		segP = & segs[segs_size];
		segP->seg_no = segs_size++;
	}
	segP->seg_flags = (unsigned short) flags;
	segP->seg_type = type;

	/* All other fields will have been set to 0 in segs_resize */

	if ( name )
	{
		/* This is a named (user-created or default) segment.  
		 * For a COFF object, create a symbol-table entry that will
		 * eventually represent a section name.  The value field
		 * and the aux entry will get filled in later, in 
		 * obj_crawl_symbol_chain().  Also for COFF, allocate a
		 * section header struct.
		 *
		 * For a BOUT object, fields not already initialized 
		 * will never be referenced.
		 */
	    	segP->seg_name = xmalloc(strlen(name) + 1);
		strcpy (segP->seg_name, name);
#if defined(OBJ_COFF) || defined(OBJ_ELF)
		symP = symbol_find_or_make (segP->seg_name);
		S_SET_SEGMENT(symP, segP->seg_no);
		S_SET_SEGNAME(symP);
#if defined(OBJ_COFF)
		S_SET_STORAGE_CLASS(symP, C_STAT);
		S_SET_NUMBER_AUXILIARY(symP, 1);
		SF_SET_STATICS(symP);
#endif
		symbol_table_insert (symP);
#endif
	}

	/* Else if unnamed, this is a mythical segment used in expression 
	 * evaluation.  Make a segment but DON'T make a symbol table entry.
	 */

	return segP->seg_no;

} /* end seg_new */


/* segs_begin()
 *
 * Should be called only once in the program.  Sets up the segments we know
 * we will need.  Sets global variables so they are primed to create the first
 * user segment.  Starts a frag chain for each of .text, .data, .bss.
 * IMPORTANT:  
 *   (1) The ORDER in which the segments are created is significant and must
 *       parallel the order of the MYTHICAL_SEG macros defined in obj_coff.h.
 *   (2) This routine also sets the current seg to the default TEXT segment.
 */
void
segs_begin()
{
	obstack_begin(&frags, 5000);

	seg_new (SEG_REGISTER, NULL, 0);
	seg_new (SEG_ABSENT, NULL, 0);
	seg_new (SEG_PASS1, NULL, 0);
	seg_new (SEG_BIG, NULL, 0);
	seg_new (SEG_DIFFERENCE, NULL, 0);
	seg_new (SEG_GOOF, NULL, 0);
	seg_new (SEG_DEBUG, NULL, 0);
	seg_new (SEG_ABSOLUTE, NULL, 0);
	seg_new (SEG_UNKNOWN, NULL, 0);
	DEFAULT_TEXT_SEGMENT = seg_new (SEG_TEXT, ".text", SECT_ATTR_TEXT );
	DEFAULT_DATA_SEGMENT = seg_new (SEG_DATA, ".data", SECT_ATTR_DATA );
	DEFAULT_BSS_SEGMENT = seg_new (SEG_BSS, ".bss", SECT_ATTR_BSS );

	/* Fake up frag chains for each of .text, .data, and .bss */
	curr_frag = NULL;
	seg_change(DEFAULT_TEXT_SEGMENT);
	seg_change(DEFAULT_DATA_SEGMENT);
	seg_change(DEFAULT_BSS_SEGMENT);
	
	/* Now make ".text" the default starting segment. */
	seg_change(DEFAULT_TEXT_SEGMENT);
} /* segs_begin */


/* seg_change()
 *
 * Set global variables used by other functions to reflect the current 
 * segment.  Create a new frag chain if one does not exist already for
 * this segment.  
 */
void
seg_change (newseg)
	int   newseg;
{
	frag_finish (0);
	curr_seg = newseg;

	curr_frag = SEG_GET_FRAG_LAST(curr_seg);
	frag_init ();
	seg_fix_rootP = & SEG_GET_FIX_ROOT(curr_seg);
	seg_fix_tailP = & SEG_GET_FIX_LAST(curr_seg);
}  /* seg_change */


/*
 *			seg_finish()
 *
 * FIXME-SOMEDAY: This can create harmless, but annoying, extra 
 * empty frags.  Need to find and plug the frag leak someday.
 *
 * Same as frag_new() except that the cleanup of fr_fix is not done.
 * Expects to have a non-NULL <seg>_last_frag.
 */
void 
seg_finish (seg)
int   seg; /* index into segs array */
{
	long	tmp;		/* JF: for alignment trickery */
	fragS   **last_fragP;

	last_fragP = & SEG_GET_FRAG_LAST(seg);
	know (*last_fragP);

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
	
	(*last_fragP)->fr_next = (fragS *) obstack_finish(&frags);
	*last_fragP = (*last_fragP)->fr_next;

	obstack_alignment_mask(&frags) = tmp;		/* Restore alignment */
	/* Just in case we didn't get zero'd bytes */
	bzero(*last_fragP, SIZEOF_STRUCT_FRAG);
}	/* seg_finish() */


/* segs_push_down()
 *
 * This is to head off a potential problem related to lomem segments.
 * These are segments created by the user (compiler) specifically 
 * targetted for placement into low addresses.  We want to generate
 * MEMA instructions when these addresses are referenced.  The COFF 
 * format requires that these symbols actually be located within the 
 * first 4K of the object file.
 * 
 * If this function was called, we need to make room for a new segment
 * at the START of the real (non-mythical) segment array.
 */
void
segs_push_down(new_max)
	int new_max;   /* New largest segment number */
{
	register int 		i;
	register symbolS	*sp;
	int   			seg;

	for ( i = new_max; i > FIRST_PROGRAM_SEGMENT; --i )
	{
		segs [i] = segs [i - 1];
		segs [i].seg_no = i;
	}

	/* Now fix up those symbols that already have a section number. */
	for ( sp = symbol_rootP; sp; sp = symbol_next(sp) )
	{
		if ( (seg = S_GET_SEGMENT(sp)) >= FIRST_PROGRAM_SEGMENT )
			S_SET_SEGMENT(sp, ++seg);
	}
	
	/* You just pushed these globals off by one */
	++DEFAULT_TEXT_SEGMENT;
	++DEFAULT_DATA_SEGMENT;
	++DEFAULT_BSS_SEGMENT;

} /* segs_push_down */


/* segs_resize()
 *
 * If we are here, seg_new has determined that the current segs array is not big
 * enough to hold another segment struct.  So reallocate the entire array with
 * realloc().  This is assumed to copy all existing memory into the new memory 
 * but is not assumed to set new memory space to zero.  Fix up the appropriate 
 * global variables.
 */
static void
segs_resize()
{
    int	incr = 20;		/* Pick a number; this is a round one */
    if ( max_segs + incr > 32768 )  /* 2e16 is coff-imposed limit on # of sections */
    {
	if ( max_segs == 32768 )
	{
	    /* We're full! */
	    as_fatal ("Maximum number of sections exceeded");
	}
	else
	{
	    max_segs = 32768;
	}
    }
    else
    {
	max_segs += incr;
    }
    
    segs = segs ? 
	(segS *) realloc (segs, max_segs * sizeof (segS)) :
	    (segS *) malloc (max_segs * sizeof (segS));
    if ( segs == NULL )
	as_fatal ("Memory allocation failed in segs_resize()");
   
    /* Zero out the new entries */
    memset((char *) &segs[segs_size], 0, incr * sizeof(segS));
}


/* seg_find()
 *
 * Search the segs array looking for a segment name.  If found, return 
 * the array index.  Otherwise, return -1.  Search only the 
 * program segments, i.e. from ".text" on.  FIRST_PROGRAM_SEGMENT
 * is defined in as.h.
 */


seg_find(name)
	char *name;
{
	int	i = FIRST_PROGRAM_SEGMENT;
	for ( ; i < segs_size; ++i )
		if ( ! strcmp (segs[i].seg_name, name) )
			return i;
	return -1;
} /* seg_find */

void segs_assert_msb()
{
    segs[DEFAULT_TEXT_SEGMENT].seg_flags |= SECT_ATTR_MSB;
    segs[DEFAULT_DATA_SEGMENT].seg_flags |= SECT_ATTR_MSB;
    segs[DEFAULT_BSS_SEGMENT].seg_flags |= SECT_ATTR_MSB;
    curr_frag->fr_big_endian = 1;  /* We can do this because curr_frag is now the first
				      frag and it corresponds to the first frag of the
				      .text section. */
}

void segs_assert_pix()
{
    extern int pic_flag,pid_flag;

    if (pic_flag)
	segs[DEFAULT_TEXT_SEGMENT].seg_flags |= SECT_ATTR_PI;
    if (pid_flag)
    {
	segs[DEFAULT_DATA_SEGMENT].seg_flags |= SECT_ATTR_PI;
	segs[DEFAULT_BSS_SEGMENT].seg_flags |= SECT_ATTR_PI;
    }
}
