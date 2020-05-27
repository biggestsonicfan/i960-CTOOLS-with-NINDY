
/* Super block arranger for GNU compiler.
   Copyright (C) 1987, 1988 Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef IMSTG

/*
 *
 * Module to create super blocks
 *
 * Implementation Overview Entry Point : sblock_optimize (insns)
 *
 */

#include "config.h"
#include "rtl.h"
#include "expr.h"
#include "insn-config.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "recog.h"
#include "flags.h"
#include "assert.h"
#include "insn-flags.h"
#include <stdio.h>
#include "basic-block.h"
#include "tree.h"
#include "gvarargs.h"
#include "i_dataflow.h"


/*
 * the data structures used to collect the superblock traces
 */
static int number_of_traces;	/* the number of traces found */
static int * first_in_trace;	/* array [number_of_traces] of block numbers */

static int * the_pred_percent;	/* gives pred_percent for the block (used by tail_dup) */

static int * next_in_trace;	/* block number of next block in trace or END_OF_TRACE */
#define END_OF_TRACE (-1)	/* end of trace list */

static int * which_trace;	/* trace number that this block is in else NO_TRACE */
#define NO_TRACE (-1)		/* not been examined yet */
#define DONT_TRACE (-2)		/* doesn't go in any trace */

/*
 * macros for moving thru successors and predecessors
 */
#define NEXT_SUCC(b, s) next_flex (df_data.maps.succs [b], s)
#define NEXT_PRED(b, p) next_sized_flex (df_data.maps.preds + b, p)

extern FILE * sblock_dump_file;

/*
 * forward references
 */
static int pred_percent ();
static int succ_percent ();

/*
 * development debug dump
 */
static void dump_trace ()
{
	int block;	/* the block being visited */
	int s;		/* the succecessor block */
	int p;		/* the predecessor block */
	int t;		/* the trace */
	int percent;	/* the percentage of ancestry of a predecessor to a block */
	rtx jump;	/* the jump from a predecessor */
	rtx x;		/* walk thru the instructions */
	static int dump_rtx = 0;	/* change for more verbose dump */
	FILE * f = sblock_dump_file;

	if ( ! f )
		return;

	fprintf (f, "Superblock dump for %s\n", IDENTIFIER_POINTER (DECL_NAME (current_function_decl)));

	for ( block = 0; block < N_BLOCKS; ++block) {

		/*
		 * print out block header line
		 */
		fprintf (f, "\nBlock B%d/%d ", block, INSN_EXPECT (BLOCK_END (block)));
		if ( block == ENTRY_BLOCK )
			fprintf (f, " is an entry block ");
		else if ( block == EXIT_BLOCK )
			fprintf (f, " is an exit block ");
		fprintf (f, "\n");

		/*
		 * print out predecessor info
		 */
		fprintf (f, "	preds :");
		for ( p = -1; (p = NEXT_PRED (block, p)) != -1; ) {
			percent = pred_percent (p, block);
			fprintf (f, " B%d/%d%%", p, percent);
		} /* end for */
		fprintf (f, "\n");


		/*
		 * print out succecessor info
		 */
		fprintf (f, "	succs :");
		for ( s = -1; (s = NEXT_SUCC (block, s)) != -1; ) {
			percent = succ_percent (block, s);
			fprintf (f, " B%d/%d%%", s, percent);
		} /* end for */
		fprintf (f, "\n");

		/*
		 * print out the instructions
		 */
		if ( dump_rtx ) {
			fprintf (f, "\n");
			for ( x = BLOCK_HEAD (block); x; x = NEXT_INSN (x) ) {
				fprint_rtx (f, x);
				if ( GET_CODE(x) == JUMP_INSN )
					fprintf (f, "\tJUMP_THEN_PROB=%d%%\n",JUMP_THEN_PROB (x));
				if ( x == BLOCK_END (block) )
					break;
			} /* end for */
			fprintf (f, "\n");
		} /* end if */

	} /* end for */

	/*
	 * print out the traces
	 */
	for ( t = 0; t < number_of_traces; ++t ) {
		fprintf (f, "Trace #%d :", t);
		for ( block = first_in_trace [t];
		      block != END_OF_TRACE;
		      block = next_in_trace [block] )
			fprintf (f, " B%d", block);
		fprintf (f, "\n");
	} /* end for */

	fprintf (f, "\n");
	fprintf (f, "\n");
	fprintf (f, "\n");
} /* end dump_trace */




/*
 * count the number of predecessor blocks for a block
 */
static int count_pred (block)
	int block;
{
	int count;
	int pred;

	for (count = 0, pred = -1; (pred = NEXT_PRED (block, pred)) != -1; ++count) ;
	return count;
} /* end count_pred */

/*
 * count the number of succecessor blocks for a block
 */
static int count_succ (block)
	int block;
{
	int count;
	int succ;

	for (count = 0, succ = -1; (succ = NEXT_SUCC (block, succ)) != -1; ++count)
		;
	return count;
} /* end count_succ */



/*
 * report if the branch instruction jumps to the given label
 */
static int jumps_to (jump, code_label)
	rtx jump;
	rtx code_label;
{
	if ( GET_CODE (jump) != JUMP_INSN )
		return 0;
	if ( simplejump_p (jump) ) {
		if ( GET_CODE (SET_SRC (PATTERN (jump))) == LABEL_REF &&
		     XEXP (SET_SRC (PATTERN (jump)), 0) == code_label )
			return 1;
	} else if ( condjump_p (jump) ) {
		if ( GET_CODE (XEXP (SET_SRC (PATTERN (jump)), 1)) == LABEL_REF &&
		     XEXP (XEXP (SET_SRC (PATTERN (jump)), 1), 0) == code_label )
			return 1;
		if ( GET_CODE (XEXP (SET_SRC (PATTERN (jump)), 2)) == LABEL_REF &&
		     XEXP (XEXP (SET_SRC (PATTERN (jump)), 2), 0) == code_label )
			return 1;
	} /* end if */
	return 0;
}


/*
 * if a conditional jump return it's PROB
 * else return 100
 *
 * note for :
 *	pc = (condition) ? label : pc           return PROB
 *	pc = (condition) ? pc : label           return 100 - PROB
 */
static int jump_percent (jump)
	rtx jump;
{
	if ( GET_CODE (jump) != JUMP_INSN || ! condjump_p (jump) )
		return 100;
	if ( GET_CODE (XEXP (SET_SRC (PATTERN (jump)), 1)) == LABEL_REF )
		return JUMP_THEN_PROB (jump);
	return 100 - JUMP_THEN_PROB (jump);
} /* jump_percent */



/*
 * return the percent of the time that block goes to succ
 */
static int succ_percent (block, succ)
	int block;
	int succ;
{
	int succ_count;
	rtx first_insn;
	rtx jump;

	succ_count = count_succ (block);
	assert ( succ_count > 0 );

	/*
	 * if there is only 1 succecessor than the percent is 100
	 */
	if ( succ_count == 1 )
		return 100;

	/*
	 * JUMP_THEN_PROB for switches is bogus
	 * so if succ_count > 2 return 0
	 */
	if ( succ_count > 2 )
		return 0;

	/*
	 * if not a condjump return 0
	 */
	jump = BLOCK_END (block);
	if ( ! condjump_p (jump) )
		return 0;

	/*
	 * if succ starts with a label and block jumps to that label
	 * return JUMP_THEN_PROB
	 */
	first_insn = BLOCK_HEAD (succ);
	if ( GET_CODE (first_insn) == CODE_LABEL && jumps_to (jump, first_insn) )
		return jump_percent (jump);

	/*
	 * fall thru gets the reverse
	 */
	return 100 - jump_percent (jump);

} /* end succ_percent */



/*
 * return the percent of executions of block for
 * which pred is the predecessor as :
 *	succ_percent (pred, block) * EXPECT(pred) / EXPECT(block)
 *
 * note the parentheses and casts used to prevent integer overflow/underflow
 */
static int pred_percent (pred, block)
	int pred;
	int block;
{
	rtx first_insn;
	rtx last_insn;
	int percent;

	percent = succ_percent (pred, block);
	last_insn = BLOCK_END (pred);
	first_insn = BLOCK_HEAD (block);
	if ( INSN_EXPECT (first_insn) == 0 )
		return 0;
	return DTOI_EXPECT ((double) percent * INSN_EXPECT (last_insn) / INSN_EXPECT (first_insn));
} /* end pred_percent */



/*
 * put a block into a trace either at the beginning or end
 */
static void put_in_trace (block, trace, at_end)
	int block;
	int trace;
	int at_end;
{
	int after;	/* the point after which block is inserted */

	/*
	 * mark this block as belonging to this trace
	 */
	which_trace [block] = trace;

	/*
	 * do we insert at the beginning?
	 */
	if ( first_in_trace [trace] == END_OF_TRACE || at_end == 0 ) {
		next_in_trace [block] = first_in_trace [trace];
		first_in_trace [trace] = block;
		return;
	} /* end if */

	/*
	 * insert at the end of the list
	 */
	for ( after = first_in_trace [trace]; ; after = next_in_trace [after] ) {
		assert ( after != END_OF_TRACE );
		if ( next_in_trace [after] == END_OF_TRACE )
			break;
	} /* end for */

	next_in_trace [block] = next_in_trace [after];
	next_in_trace [after] = block;

} /* end put_in_trace */




/*
 * if the trace is a loop, start with the lexically earliest node
 */
static void adjust_loop (trace)
	int trace;
{
	int b;			/* used to walk blocks */
	int first_block;	/* lexically earliest block in loop */
	int next_block;		/* used to walk blocks */

	/*
	 * see if the last block in the trace jumps to the first
	 * also find the first lexical block in the trace 
	 */
	for ( first_block = b = first_in_trace [trace]; ;b = next_in_trace [b] ) {
		if ( b < first_block )
			first_block = b;
		if ( next_in_trace [b] == END_OF_TRACE )
			break;
	} /* end for */

	/*
	 * nothing to do if already first_in_trace
	 */
	if ( first_in_trace [trace] == first_block )
		return;

	/*
	 * do we have a loop?
	 */
	if ( ! in_flex (df_data.maps.succs [b], first_in_trace [trace]) )
		return;

	/*
	 * put all the blocks before first_block at the end of the trace
	 */
	for ( b = first_in_trace [trace]; b != first_block; b = next_block ) {
		next_block = next_in_trace [b];
		first_in_trace [trace] = next_block;
		put_in_trace (b, trace, 1);
	} /* end for */
} /* end adjust_loop */



/*
 * traces are found with the following algorithm
 *
 *	mark the ENTRY and EXIT blocks so they don't go in any trace
 *
 * 	while ( there are blocks not in any trace )
 * 		seed = block with largest execution count not yet in any trace
 * 		put seed in a new trace
 *
 * 		// grow the trace forward
 *
 * 		block = seed
 * 		loop
 * 			successor = best_successor_of (block)
 * 			if no good successor then exit loop
 * 			put successor in the trae
 * 			set block to successor
 * 		end loop
 *
 * 		// grow the trace backward
 *
 * 		block = seed
 * 		loop
 * 			predessor = best_predessor_of (block)
 * 			if no good predessor then exit loop
 * 			put predessor in the trae
 * 			set block to predessor
 * 		end loop
 * 	end while
 *
 *
 */
static void find_traces ()
{
	int seed;
	int best_score;
	int current_score;
	int block;
	int succ_block;
	int pred_block;
	int best_block;
	int trace;

	/*
	 * find the traces
	 */
	
	which_trace [ENTRY_BLOCK] = DONT_TRACE;
	which_trace [EXIT_BLOCK] = DONT_TRACE;

	/* Don't put tablejumps into a trace,
	 * since we don't duplicate them correctly.
	 */

	for (block = LAST_REAL_BLOCK; block >= FIRST_REAL_BLOCK; block--) {
		rtx	lr;
		rtx	lasti = BLOCK_END(block);

		if (GET_CODE(lasti) != JUMP_INSN
		    || (GET_CODE(PATTERN(lasti)) != ADDR_VEC
		        && GET_CODE(PATTERN(lasti)) != ADDR_DIFF_VEC))
			continue;

		which_trace[block] = DONT_TRACE;

		/* Also mark the jump_insn(s) using this table */

		assert(GET_CODE(BLOCK_HEAD(block)) == CODE_LABEL);
		lr = LABEL_REFS(BLOCK_HEAD(block));
		for ( ; lr != BLOCK_HEAD(block); lr = LABEL_NEXTREF(lr))
		{
			int b = BLOCK_NUM(CONTAINING_INSN(lr));
			which_trace[b] = DONT_TRACE;
		}
	}

	number_of_traces = 0;
	for ( ; ; ) {

		/*
		 * find the seed block, the highest EXPECT block not yet in a trace
		 */
		seed = -1;
		best_score = -1;
		for ( block = 0; block < N_BLOCKS; ++block ) {
			if ( which_trace [block] != NO_TRACE )
				continue;
			current_score = INSN_EXPECT (BLOCK_HEAD(block));
			if ( best_score < current_score ) {
				seed = block;
				best_score = current_score;
			} /* end if */
		} /* end for */
		if ( seed == -1 )
			break;

		/*
		 * put this block in the trace
		 */
		trace = number_of_traces;
		++number_of_traces;
		put_in_trace (seed, trace, 0);

		/*
		 * add successors to the trace
		 */
		block = seed;
		for (;;) {

			/*
			 * find the best successor, which is
			 * the highest PROB block if not yet in a trace
			 */
			best_score = 0;
			best_block = -1;
			succ_block = -1;
			while ( (succ_block = NEXT_SUCC (block, succ_block)) != -1 ) {
				current_score = succ_percent (block, succ_block);
				if ( best_score < current_score ) {
					best_block = succ_block;
					best_score = current_score;
				} /* end if */
			} /* end shile */

			/*
			 * if none found then quit adding successors
			 */
			if ( best_block == -1  || which_trace [best_block] != NO_TRACE )
				break;

			/*
			 * add this to the trace and make it the current block
			 */
			put_in_trace (best_block, trace, 1);
			the_pred_percent [best_block] = pred_percent (block, best_block);
			block = best_block;
			
		} /* end for */

		/*
		 * add predecessors to the trace
		 */
		block = seed;
		for (;;) {

			/*
			 * find the best predecessor, which is 
			 * the highest PROB block not yet in a trace
			 */
			best_score = 0;
			best_block = -1;
			pred_block = -1;
			while ( (pred_block = NEXT_PRED (block, pred_block)) != -1 ) {
				current_score = pred_percent (pred_block, block);
				if ( best_score < current_score ) {
					best_block = pred_block;
					best_score = current_score;
				} /* end if */
			} /* end while */

			/*
			 * if none found then quit adding predecessors
			 */
			if ( best_block == -1  || which_trace [best_block] != NO_TRACE )
				break;

			/*
			 * add this to the trace and make it the current block
			 */
			put_in_trace (best_block, trace, 0);
			the_pred_percent [block] = pred_percent (best_block, block);
			block = best_block;
			
		} /* end for */

		/*
		 * if only one block went into the trace we really don't want
		 * to pay attention to it, so erase the trace
		 */
		if ( next_in_trace [first_in_trace[trace]] == END_OF_TRACE ) {
			which_trace [seed] = DONT_TRACE;
			first_in_trace [trace] = END_OF_TRACE;
			--number_of_traces;
			continue;
		} /* end if */

		/*
		 * if the superblock is also a loop, adjust the loop ordering
		 */
		adjust_loop (trace);

	} /* end for */
} /* end find_traces */


#if 0


/*
 * put a note insn into the rtl after the given place
 */
static void paste_note_after (note, after)
	rtx note;
	rtx after;
{
	NEXT_INSN (note) = NEXT_INSN (after);
	PREV_INSN (note) = after;
	NEXT_INSN (PREV_INSN (note)) = note;
	PREV_INSN (NEXT_INSN (note)) = note;
} /* end paste_note_after */
#endif


/*
 * this is a routine to handle all the grossness and hacks
 * having to do with duplicating rtl operations
 */
static void make_copy_after (source, expect_adjust, block)
	rtx source;
	int expect_adjust;
	int block;
{
	rtx copy;
	rtx retval_note;
	rtx libcall_note;
	static rtx libcall_insn = 0;
	double adjust;
	rtx end_point;
	rtvec orig_asm_ops;
	rtvec copy_asm_ops;
	rtvec copy_asm_cons;

	/*
	 * put the new insn after any notes following the end of the block
	 */
	end_point = BLOCK_END (block);
	while ( NEXT_INSN (end_point) && GET_CODE (NEXT_INSN (end_point)) == NOTE )
		end_point = NEXT_INSN (end_point);
	BLOCK_END (block) = end_point;

	/*
	 * duplicate line number notes
	 */
	if ( GET_CODE (source) == NOTE ) {
		copy = emit_note_after (NOTE_LINE_NUMBER (source), BLOCK_END (block));
		NOTE_SOURCE_FILE (copy) = NOTE_SOURCE_FILE (source);
		NOTE_TRIP_COUNT (copy) = NOTE_TRIP_COUNT (source);
		BLOCK_END (block) = copy;
		return;
	} /* end if */

	/*
	 * generate a copy and paste it into the source, getting an insn
	 */
	orig_asm_ops = 0;
	copy = imstg_copy_rtx (PATTERN (source),
                               &orig_asm_ops,
                               &copy_asm_ops,
                               &copy_asm_cons);
	switch ( GET_CODE (source) ) {
	case JUMP_INSN:
		copy = emit_jump_insn_after (copy, BLOCK_END (block));
		break;
	case CALL_INSN:
		copy = emit_call_insn_after (copy, BLOCK_END (block));
		orig_asm_ops = 0;
		CALL_INSN_FUNCTION_USAGE (copy) =
                  imstg_copy_rtx(CALL_INSN_FUNCTION_USAGE(source),
                                 &orig_asm_ops,
                                 &copy_asm_ops,
                                 &copy_asm_cons);
		break;
	default :
		copy = emit_insn_after (copy, BLOCK_END (block));
		break;
	} /* end switch */
	BLOCK_END (block) = copy;

	/*
	 * update the many magic fields
	 */
	INSN_CODE (copy) = -1;
	LOG_LINKS (copy) = NULL;
	adjust = INSN_EXPECT (source) * 0.01;
	INSN_EXPECT (copy) = DTOI_EXPECT (expect_adjust * adjust);
	INSN_EXPECT (source) = DTOI_EXPECT ((100-expect_adjust) * adjust);
	if ( GET_CODE (source) == JUMP_INSN )
		JUMP_THEN_PROB (copy) = JUMP_THEN_PROB (source);

        /*
         * Delete any REG_EQUIV notes on stuff that is being copied.
         * Unfortunately this can cause bugs down the line if there
         * are multiple sets to a pseudo-register that has a REG_EQUIV
         * note.
         */
	{
		rtx eqv;
                if ((eqv = find_reg_note(source, REG_EQUIV, NULL_RTX)) != 0)
                  remove_note(source, eqv);
	}

	/*
	 * hack for REG_LIBCALL not getting created properly.
	 */
	if ( REG_NOTES (source) ) {
		orig_asm_ops = 0;
		REG_NOTES (copy) = imstg_copy_rtx(REG_NOTES(source),
                                                  &orig_asm_ops,
                                                  &copy_asm_ops,
                                                  &copy_asm_cons);
		if ( ! libcall_insn ) {
			if ( find_reg_note (copy, REG_LIBCALL, 0) )
				libcall_insn = copy;
		} else {
			retval_note = find_reg_note (copy, REG_RETVAL, 0);
			if ( retval_note ) {
				libcall_note = find_reg_note (libcall_insn, REG_LIBCALL, 0);
				assert ( libcall_note );
				XEXP (libcall_note, 0) = copy;
				XEXP (retval_note, 0) = libcall_insn;
				libcall_insn = 0;
			} /* end if */
		} /* end if */
	} /* end if */
	
	/*
	 * need a barrier after any jump_insn that is not conditional
	 */
	if (GET_CODE(source) == JUMP_INSN
	    && (!condjump_p(source) || simplejump_p(source)))
		BLOCK_END(block) = emit_barrier_after(BLOCK_END(block));
	return;

} /* end make_copy_after */



/*
 * does a block start with a label
 */
#define has_label(b) (GET_CODE (BLOCK_HEAD ((b))) == CODE_LABEL)



/*
 * find the label ref in an rtx expression
 */
static rtx find_label (x)
	rtx x;
{
	RTX_CODE code;
	char * format;
	rtx label;
	int i;
	int j;

	code = GET_CODE (x);

	if ( code == LABEL_REF )
		return XEXP (x, 0);

	format = GET_RTX_FORMAT (code);
	for ( i = GET_RTX_LENGTH (code) - 1; i >= 0; i-- ) {
		if ( format [i] == 'e' ) {
			label = find_label (XEXP (x, i));
			if ( label )
				return label;
		} /* end if */
		if ( format [i] == 'E' ) {
			for ( j = 0; j < XVECLEN (x, i); ++j ) {
				label = find_label (XVECEXP (x, i, j));
				if ( label )
					return label;
			} /* end for */
		} /* end if */
	} /* end for */
	return NULL;
}



/*
 * Invert the jump condition of rtx X,
 * and replace OLABEL with NLABEL throughout.
 * This was taken from pre 2.0 jump.c.
 */

static void invert_exp (x, olabel, nlabel)
     rtx x;
     rtx olabel;
     rtx nlabel;
{
  register RTX_CODE code;
  register int i;
  register char *fmt;

  if (x == 0)
    return;

  code = GET_CODE (x);
  if (code == IF_THEN_ELSE)
    {
      /* Inverting the jump condition of an IF_THEN_ELSE
         means exchanging the THEN-part with the ELSE-part.  */
      register rtx tem = XEXP (x, 1);
      XEXP (x, 1) = XEXP (x, 2);
      XEXP (x, 2) = tem;
    }

  if (code == LABEL_REF)
    {
      if (XEXP (x, 0) == olabel)
        XEXP (x, 0) = nlabel;
      return;
    }

  fmt = GET_RTX_FORMAT (code);
  for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
    {
      if (fmt[i] == 'e')
        invert_exp (XEXP (x, i), olabel, nlabel);
      if (fmt[i] == 'E')
        {
          register int j;
          for (j = 0; j < XVECLEN (x, i); j++)
            invert_exp (XVECEXP (x, i, j), olabel, nlabel);
        }
    }
}


/*
 * change the condition of a conditional jump to
 * and branch to new_label
 *
 * this was copied from invert_jump() in jump.c, but
 * had to be changed since LABEL_NUSES isn't set up yet
 *
 * also had to copy code for invert_exp()
 */
void
imstg_switch_jump (jump, nlabel)
	rtx jump, nlabel;
{
	register rtx olabel;

	olabel = find_label (PATTERN (jump));
	assert ( olabel != NULL );

	invert_exp (PATTERN (jump), olabel, nlabel);

	INSN_CODE (jump) = -1;
}


/*
 * change any label_ref nodes in this insn which are to olabel
 * to reference nlabel
 */
static void change_label_ref (x, olabel, nlabel)
	rtx x;
	rtx olabel;
	rtx nlabel;
{
	RTX_CODE code;
	char * format;
	int i;
	int j;

	code = GET_CODE (x);
	if ( code == LABEL_REF ) {
		if ( XEXP (x, 0) == olabel )
			XEXP (x, 0) = nlabel;
		return;
	} /* end if */

	format = GET_RTX_FORMAT (code);
	for ( i = GET_RTX_LENGTH (code) - 1; i >= 0; i-- ) {
		if ( format [i] == 'e' ) {
			change_label_ref (XEXP (x, i), olabel, nlabel);
		} else if ( format [i] == 'E' ) {
			for ( j = 0; j < XVECLEN (x, i); ++j )
				change_label_ref (XVECEXP (x, i, j), olabel, nlabel);
		} /* end if */
	} /* end for */
} /* end change_label_ref */

/*
 * do tail duplication to create superblocks
 */
static void tail_duplication ()
{
	int trace;
	int block;
	int next_block;
	int last_block;
	int fall_thru;
	rtx jump;
	rtx barrier;
	rtx label;
	rtx source;
	rtx copy;
	int expect_adjust;

	for ( trace = 0; trace < number_of_traces; ++trace ) {

		/*
		 * first skip past any blocks which are already fall thru's
		 * and won't need to be copied
		 */
		expect_adjust = 100;
		for ( block = first_in_trace [trace];
		      (next_block = next_in_trace [block]) != END_OF_TRACE;
		      block = next_block
		) {

			expect_adjust *= the_pred_percent [next_block];
			expect_adjust /= 100;

			/*
			 * if block falls thru to next_block and next_block
			 * has block as it's only predecessor
			 */
			if ( block != next_block - 1 )
				break;

			if ( count_pred (next_block) != 1 )
				break;

		} /* end for */

		/*
		 * need to copy all remaining code into "block"
		 * however, we will not duplicate a block which contains a
		 * loop begin note (or is immediately preceeded by a loop begin note),
		 * so that the loop optimizer still works.
		 */
		for ( last_block = block;
		      next_block != END_OF_TRACE;
                      next_block = next_in_trace [last_block = next_block] ) {

			/*
			 * check for a NOTE_INSN_LOOP_BEG, break if found
			 */
			for ( source = PREV_INSN (BLOCK_HEAD (next_block));
			      source != NEXT_INSN (BLOCK_END (next_block));
			      source = NEXT_INSN (source)
			) {
				if ( GET_CODE (source) != NOTE )
					continue;
				if ( NOTE_LINE_NUMBER (source) != NOTE_INSN_LOOP_BEG )
					continue;
				if ( sblock_dump_file )
					fprintf (sblock_dump_file,
						"Trace %d, ended by loop begin insn=%d\n",
							trace, INSN_UID (source));
				goto done_duping;
			} /* end for */

			expect_adjust *= the_pred_percent [next_block];
			expect_adjust /= 100;

			if ( sblock_dump_file )
				fprintf (sblock_dump_file, "Trace %d, duping B%d adjust %d\n",
								trace, next_block, expect_adjust);

			/*
			 * look at how "block" gets to "next_block"
			 */
			jump = BLOCK_END (block);
			while (GET_CODE(jump) == NOTE
			       || GET_CODE(jump) == BARRIER)
				jump = PREV_INSN (jump);

			/*
			 * if a jump then delete the jump, and its barrier
			 */
			if ( GET_CODE (jump) == JUMP_INSN && simplejump_p (jump) ) {

				BLOCK_END (block) = PREV_INSN (jump);
				PUT_CODE (jump, NOTE);
				NOTE_LINE_NUMBER (jump) = NOTE_INSN_DELETED;
				NOTE_SOURCE_FILE (jump) = 0;

				barrier = NEXT_INSN (jump);
				while ( GET_CODE (barrier) == NOTE )
					barrier = NEXT_INSN (barrier);
				if ( GET_CODE (barrier) == BARRIER ) {
					NEXT_INSN (PREV_INSN (barrier)) = NEXT_INSN (barrier);
					PREV_INSN (NEXT_INSN (barrier)) = PREV_INSN (barrier);
/*
					PUT_CODE (barrier, NOTE);
					NOTE_LINE_NUMBER (barrier) = NOTE_INSN_DELETED;
					NOTE_SOURCE_FILE (barrier) = 0;
*/
				} /* end if */

				if ( sblock_dump_file )
					fprintf (sblock_dump_file, "deleting jump\n");

			/*
			 * if a taken conditional branch then put a label
			 * in the old fall thru and make the branch in block be
			 * to the fall thru
			 */
			} else if ( has_label (next_block) &&
				    jumps_to (jump, BLOCK_HEAD (next_block)) ) {

				if ( ! has_label (last_block + 1) ) {
					label = gen_label_rtx ();
					emit_label_after (label, 
						PREV_INSN (BLOCK_HEAD (last_block + 1)));
					BLOCK_HEAD (last_block + 1) = label;
				} /* end if */
				label =  BLOCK_HEAD (last_block + 1);

				if ( sblock_dump_file ) {
					fprintf (sblock_dump_file, "switching fall thru");
					fprintf (sblock_dump_file, " jump_insn=%d", INSN_UID (jump));
					fprintf (sblock_dump_file, " code_label=%d\n", INSN_UID (label));
				} /* end if */

				imstg_switch_jump (jump, label);
			} /* end if */

			/*
			 * copy the code in next_block into block, deleting
			 * labels and notes (except line number notes)
			 */
			for ( source = BLOCK_HEAD (next_block);
			      source != NEXT_INSN (BLOCK_END (next_block));
			      source = NEXT_INSN (source)
			) {
				if ( GET_CODE (source) == CODE_LABEL )
					continue;

				if ( GET_CODE (source) == NOTE && NOTE_LINE_NUMBER (source) <= 0 )
					continue;

				make_copy_after (source, expect_adjust, block);
			} /* end for */
		} /* end for */

done_duping:

		/*
		 * if no duplicated code go to next trace
		 */
		if ( last_block == block )
			continue;


		/*
		 * if the last duplicated block had a fall thru
		 * then we need a jump at the end of the superblock
		 */
		fall_thru = last_block + 1;
		if ( fall_thru == ENTRY_BLOCK ) 	/* skip past this - an ordering klitch */
			fall_thru = EXIT_BLOCK;

		if ( BLOCK_DROPS_IN (fall_thru) ) {

			if ( ! has_label (fall_thru) ) {
				label = gen_label_rtx ();
				emit_label_after (label, PREV_INSN (BLOCK_HEAD (fall_thru)));
				BLOCK_HEAD (fall_thru) = label;
			} /* end if */
			label =  BLOCK_HEAD (fall_thru);

			jump = emit_jump_insn_after (gen_jump (label), BLOCK_END (block));
			BLOCK_END (block) = jump;
			emit_barrier_after (jump);
			if ( sblock_dump_file ) {
				fprintf (sblock_dump_file, "adding jump to B%d", fall_thru);
				fprintf (sblock_dump_file, " jump_insn=%d", INSN_UID (jump));
				fprintf (sblock_dump_file, " code_label=%d\n", INSN_UID (label));
			} /* end if */

		} /* end if */

	} /* end for */
} /* end tail_duplication */



/*
 * fixup the order of jumps from
 *	bXX.f	L1
 *	b	L2
 * to
 *	bYY.t	L2
 *	b	L1
 */
static void fixup_branches (insns)
	rtx insns;
{
	rtx cond;
	rtx simple;
	rtx next;
	rtx label1;
	rtx label2;

	for ( cond = insns; cond; cond = next ) {
		next = NEXT_INSN (cond);
		if ( GET_CODE (cond) != JUMP_INSN )
			continue;
		if ( ! condjump_p (cond) )
			continue;
		if ( JUMP_THEN_PROB (cond) >= 50 )
			continue;
		for ( simple = NEXT_INSN (cond); simple; simple = NEXT_INSN (simple) ) {
			if ( GET_CODE (simple) == NOTE )
				continue;
			if ( GET_CODE (simple) != JUMP_INSN || ! simplejump_p (simple) ) {
				next = simple;
				break;
			} /* end if */

			/*
			 * reverse the jumps
			 */
			if ( sblock_dump_file ) {
				fprintf (sblock_dump_file, "fixing jump\n");
				fprint_rtx (sblock_dump_file, cond);
				fprint_rtx (sblock_dump_file, simple);
			} /* end if */

			label1 = find_label (PATTERN (cond));
			label2 = find_label (PATTERN (simple));
			imstg_switch_jump (cond, label2);
			change_label_ref (PATTERN (simple), label2, label1);

			if ( sblock_dump_file ) {
				fprintf (sblock_dump_file, "fixed jump\n");
				fprint_rtx (sblock_dump_file, cond);
				fprint_rtx (sblock_dump_file, simple);
			} /* end if */

			next = NEXT_INSN (simple);
			break;
		} /* end for */
	} /* end for */
} /* end fixup_branches */

/*
 * the loop optimizer is not very smart about labels and loops
 * if we have the following
 *
 *	note LOOPBEG
 *	label "foo"
 *	...
 *	jump "foo"
 *	note LOOPEND
 *	...
 *	jump "foo"
 *
 * change it to:
 *
 *	label "foo2"
 *	note LOOPBEG
 *	label "foo"
 *	...
 *	jump "foo"
 *	note LOOPEND
 *	...
 *	jump "foo2"
 *
 */
static void fixup_loops (insns)
	rtx insns;
{
	rtx old_label;	/* the label in the jump */
	rtx new_label;	/* the new label for the jump */
	rtx jump;	/* the jump insn */
	rtx note;	/* the loop begin note */
	int level;	/* used for scanning nested loops */
	rtx insn;	/* insns in the loops */

	/*
	 * examine all jump insns
	 */
	for ( jump = insns; jump; jump = NEXT_INSN (jump) ) {
		if ( GET_CODE (jump) != JUMP_INSN )
			continue;
		if ( ! condjump_p (jump) && ! simplejump_p (jump) )
			continue;

		/*
		 * see if the branch target is immediately
		 * preceeded by a loop begin note
		 */
		old_label = find_label (PATTERN (jump));
		for ( note = PREV_INSN (old_label); ; note = PREV_INSN (note) ) {
			if ( GET_CODE (note) != NOTE ||
			     NOTE_LINE_NUMBER (note) != NOTE_INSN_DELETED )
				break;
		} /* end for */
		if ( GET_CODE (note) != NOTE ||
		     NOTE_LINE_NUMBER (note) != NOTE_INSN_LOOP_BEG )
			continue;

		/*
		 * scan from loop begin to loop end, if we cross "jump" then
		 * the jump is in the loop and shouldn't be modified
		 */
		level = 0;
		for ( insn = NEXT_INSN (old_label); ; insn = NEXT_INSN (insn) ) {
			if ( insn == jump )
				break;
			if ( GET_CODE (insn) == NOTE ) {
				if ( NOTE_LINE_NUMBER (insn) == NOTE_INSN_LOOP_END ) { 
					if ( level == 0 )
						break;
					--level;
				} else if ( NOTE_LINE_NUMBER (insn) == NOTE_INSN_LOOP_BEG)
					++level;
			} /* end if */
		} /* end for */
		if ( insn == jump )
			continue;

		/*
		 * put a new label in front of the note LOOP BEG
		 * and have jump branch to that label
		 */
		new_label = gen_label_rtx ();
		emit_label_after (new_label, PREV_INSN (note));
		change_label_ref (PATTERN (jump), old_label, new_label);
	} /* end for */
} /* fixup_loops */


/*
 * the external entry point to superblock organizer
 */
void sblock_optimize (insns)
	rtx insns;
{
	int block;

	/*
	 * calculate the expected execution counts and probabilities
	 */
	update_expect (current_function_decl, insns);

	/*
	 * get the basic block/flow graph information
	 */
	get_predecessors (0);

	/*
	 * allocate and initialize the trace data structures using N_BLOCKS as the
	 * upper bound on the number of traces
	 */
	first_in_trace = (int *) malloc (N_BLOCKS * sizeof (int *));
	if ( ! first_in_trace )
		return;
	the_pred_percent = (int *) malloc (N_BLOCKS * sizeof (int *));
	if ( ! the_pred_percent ) {
		free (first_in_trace);
		return;
	} /* end if */
	which_trace = (int *) malloc (N_BLOCKS * sizeof (int *));
	if ( ! which_trace ) {
		free (first_in_trace);
		free (the_pred_percent);
		return;
	} /* end if */
	next_in_trace = (int *) malloc (N_BLOCKS * sizeof (int *));
	if ( ! next_in_trace ) {
		free (first_in_trace);
		free (the_pred_percent);
		free (which_trace);
		return;
	} /* end if */
	for ( block = 0; block < N_BLOCKS; ++block ) {
		first_in_trace [block] = END_OF_TRACE;
		next_in_trace [block] = END_OF_TRACE;
		which_trace [block] = NO_TRACE;
		the_pred_percent [block] = 100;
	} /* end for */

	/*
	 * find the traces
	 */
	find_traces ();

	/*
	 * generate a debug dump if asked for
	 */
	dump_trace ();

	/*
	 * do tail duplication
	 */
	tail_duplication ();


	/*
	 * release the storage used to find traces
	 */
	free (first_in_trace);
	free (which_trace);
	free (next_in_trace);
	free (the_pred_percent);

	/*
	 * fixup the order of jumps from
	 *	bXX.f	L1
	 *	b	L2
	 * to
	 *	bYY.t	L2
	 *	b	L1
	 * so that conditional branches are more often taken
	 */
	fixup_branches (insns);

	/*
	 * fixup loops
	 */
	fixup_loops (insns);
} /* end sblock_optimize */

#endif
