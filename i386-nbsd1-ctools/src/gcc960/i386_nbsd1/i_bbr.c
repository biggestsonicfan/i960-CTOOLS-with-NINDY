
#if defined(IMSTG)

/* Implement basic block rearrangement. */

#include <assert.h>
#include <stdio.h>

#include "config.h"
#include "rtl.h"
#include "tree.h"
#include "basic-block.h"

typedef struct {
	int	tail;		/* Block from which this arc comes. */
	int	head;		/* Block to which this arc leads. */
	double	weight;
} BBR_arc;

static BBR_arc	*bbr_arcs = 0;
static int	bbr_num_arcs;

static int	bbr_num_chains;
static int	*bbr_first_block_in_chain = 0;
static int	*bbr_next_block_in_chain = 0;
#define BBR_NULL	(-1)
#define BBR_PLACED	(-2)
	/* These arrays implement chains (ie, lists) of basic blocks.
	 * Indexed by chain number C, bbr_first_block_in_chain[C] is
	 * the block number of the first block in chain C.
	 * Indexed by block number B, bbr_next_block_in_chain[B] implements
	 * the list of blocks in a chain.  The value BBR_NULL indicates the
	 * end of a chain.  Initially each basic block is in its own chain.
	 * Gradually, most chains are concatenated, leaving unused (BBR_NULL)
	 * slots in bbr_first_block_in_chain.
	 *
	 * The value BBR_PLACED can occur in bbr_first_block_in_chain[C],
	 * indicating an order for chain C amongst all chains has
	 * been determined.  In this case, the first block in chain C
	 * is temporarily recorded elsewhere.  See bbr_order_chains().
	 */

static int	*bbr_containing_chain = 0;
#define BBR_NEW_BLOCK_ORDER(b)	bbr_containing_chain[(b)]

	/* bbr_containing_chain is an overloaded array.  While determining
	 * a new block order, bbr_containing_chain[B] gives the chain
	 * number (index into bbr_first_block_in_chain) of the chain
	 * containing block B.
	 *
	 * After the order is determined, in function bbr_order_chains(),
	 * the new block ordering is written into bbr_containing_chain[],
	 * ie bbr_containing_chain[N] is the N+1st block in the new order.
	 */

/* Define some data structures used to put NOTE_INSN_BLOCK_BEG/END notes
   back in reasonable places after reordering the blocks.  We treat each
   begin/end pair as a scope, and give each scope a unique index starting
   at 1 (index 0 is unused and represents the outermost scope).
   'bbr_scopes' is an array, indexed by scope index, describing each scope.
 */

static struct bbr_scope {
	rtx	block_begin;	/* NOTE_INSN_BLOCK_BEG note at start of scope */
	rtx	block_end;	/* NOTE_INSN_BLOCK_END note at end of scope */
	rtx	last_insn;	/* last insn in scope after reordering. */
	int	parent;		/* index of scope containing this scope. */
} *bbr_scopes;

static int	bbr_num_scopes;

static rtx	bbr_scope_head;
static rtx	bbr_scope_tail;
	/* List of block begin and end notes in the order they need to appear
	   in the insn stream.  We detach these notes from the insn stream,
	   and keep them here using the NEXT_INSN and PREV_INSN fields.
	 */

static int	bbr_max_original_insn_uid;
static int	*bbr_insn_scope;
	/* Indexed by INSN_UID, this gives the scope index of the scope
	   containing each insn.
	 */

extern FILE *bbr_dump_file;

extern void xunalloc();
extern void imstg_switch_jump();

static void bbr_create_arcs();
static void bbr_find_new_order();
static void bbr_handle_if_then_else();
static void bbr_order_chains();
static void bbr_physically_rearrange_blocks();
static void bbr_free_storage();

void
bbr_optimize(insns)
rtx insns;
{
	if (bbr_dump_file)
		(void)fprintf(bbr_dump_file,"\n;; starting function %s\n",
			IDENTIFIER_POINTER(DECL_NAME(current_function_decl)));

	/* Ensure instruction execution counts and jump
	 * probabilities are up to date.
	 */

	update_expect(current_function_decl, insns);

	prep_for_flow_analysis(insns);

	if (LAST_REAL_BLOCK <= 1) return;	/* Not worthwhile. */

	bbr_create_arcs();
	if (bbr_num_arcs == 0) return;

	bbr_find_new_order();

	bbr_physically_rearrange_blocks();

	bbr_free_storage();
}

/* Return the line number note corresponding to insn, or NULL.
 */

static rtx
bbr_effective_note(insn)
rtx	insn;
{
	while (insn && (GET_CODE(insn) != NOTE || NOTE_LINE_NUMBER(insn) < 0))
		insn = PREV_INSN(insn);

	return insn;
}

/* Similar to JUMP_THEN_PROB, return the probability (int 0 thru 100)
 * that the given jump insn jumps through the given labelref.
 */

static int
jump_to_label_prob(jump, labref)
rtx	jump;
rtx	labref;
{
	rtx	pat = PATTERN(jump);

	assert(GET_CODE(jump) == JUMP_INSN);
	if (CONTAINING_INSN(labref) != jump)
		return 0;

	if (simplejump_p(jump))
	{
		return 100;
	}
	else if (condjump_p(jump))
	{
		assert(GET_CODE(pat) == SET);
		if ((XEXP(SET_SRC(pat),1)) == pc_rtx)
			return 100 - JUMP_THEN_PROB(jump);
		return JUMP_THEN_PROB(jump);
	}

	/* Check for the indirect branch used in a tablejump. */
	else if (GET_CODE(pat) == PARALLEL)
		return 100;

	/* jump must be an addr_vec or addr_diff_vec.  Since these label
	 * refs are not individually weighted, let's simply distribute this
	 * block's execution count proportionately among its elements.
	 */

	{
		int	i, length, oprnd, this_one = 0;

		if (GET_CODE(pat) == ADDR_DIFF_VEC)
		{
			if (XEXP(pat,0) == labref) return 0;
			oprnd = 1;
		}
		else
		{
			assert(GET_CODE(pat) == ADDR_VEC);
			oprnd = 0;
		}

		if ((length = XVECLEN(pat, oprnd)) <= 0) return 0;

		for (i = length-1; i >= 0; i--)
			if (XVECEXP(pat, oprnd, i) == labref)
				this_one++;
		return DTOI_EXPECT((double)this_one / (double)length);
	}
}

/* Used by qsort to sort arcs by weight.
 * Sort in ascending order, ie: return +1 if a1 > a2, -1 if a1 < a2.
 *
 * To break ties, sort in descending order by the tail, then head,
 * block numbers.  This gives priority to blocks occurring textually
 * first, tending to maintain the original block ordering.
 */
static int
bbr_compare_arcs(a1, a2)
void* a1;
void* a2;
{
	register BBR_arc *arc1 = (BBR_arc*)a1;
	register BBR_arc *arc2 = (BBR_arc*)a2;

	if (arc1->weight < arc2->weight) return -1;
	if (arc1->weight > arc2->weight) return 1;

	if (arc1->tail > arc2->tail) return -1;
	if (arc1->tail < arc2->tail) return 1;
	if (arc1->head > arc2->head) return -1;
	if (arc1->head < arc2->head) return 1;
	return 0;
}

/* Determine if 'block's predecessor can actually fall into block.
   We can't trust BLOCK_DROPS_IN, because that is overly conservative.
   For example, in the absence of a BARRIER, BLOCK_DROPS_IN will say
   that a (jump_insn (addr_vec ...) ...) drops into its successor.
   We'll get that connectivity information from the label refs.

   We currently ignore connectivity from computed gotos and forced labels,
   because we don't support the GNU language extensions that generate
   these constructs.
 */

static int
bbr_block_drops_into(block)
int block;
{
	rtx x;

	if (block <= FIRST_REAL_BLOCK || !BLOCK_DROPS_IN(block))
		return 0;

	x = PREV_INSN(BLOCK_HEAD(block));

	while (x && GET_CODE(x) == NOTE)
		x = PREV_INSN(x);

	if (x)
	{
		if (GET_CODE(x) == BARRIER)
			return 0;

		if (GET_CODE(x) == JUMP_INSN
		    && (!condjump_p(x) || simplejump_p(x)))
			return 0;
	}

	return 1;
}

static int
bbr_number_of_predecessors(block)
int block;
{
	int num = 0;
	rtx lab = BLOCK_HEAD(block);

	/* Use fall-thru and code_label LABEL_REFS to determine predecessors.
	 * Fortunately, a code_label's LABEL_REFS list contains only one
	 * entry for each distinct label_ref in an addr[_diff]_vec.
	 */

	if (bbr_block_drops_into(block))
		num++;
	if (GET_CODE(lab) == CODE_LABEL)
	{
		rtx	from = LABEL_REFS(lab);
		for ( ; from != lab; from = LABEL_NEXTREF(from))
			num++;
	}

	return num;
}

static void
bbr_create_arcs()
{
	int	block;
	rtx	firsti;
	int	i;

	/* Count the number of arcs connecting basic blocks. */

	bbr_num_arcs = 0;
	for (block = FIRST_REAL_BLOCK; block <= LAST_REAL_BLOCK; block++)
		bbr_num_arcs += bbr_number_of_predecessors(block);

	if (bbr_num_arcs == 0) return;

	bbr_arcs = (BBR_arc*) xmalloc(bbr_num_arcs * sizeof(BBR_arc));

	/* Fill in the arc info. */

	i = 0;

	for (block = FIRST_REAL_BLOCK; block <= LAST_REAL_BLOCK; block++)
	{
		if (bbr_block_drops_into(block))
		{
			rtx	branch = BLOCK_END(block-1);
			double	weight = (double)INSN_EXPECT(branch);

			/* If not always a fall thru, then weight is
			 * ((100-jump_to_label_prob(branch))*weight)/100.
			 */

			if (GET_CODE(branch) == JUMP_INSN)
			{
				/* Must be conditional jump, it falls thru. */

				int prob = JUMP_THEN_PROB(branch);
				assert(GET_CODE(PATTERN(branch)) == SET);
				assert(GET_CODE(SET_SRC(PATTERN(branch)))
							== IF_THEN_ELSE);
				if (XEXP(SET_SRC(PATTERN(branch)),1) != pc_rtx)
					prob = 100 - prob;
				weight = (double)prob * (weight / (double)100);
			}

			bbr_arcs[i].tail = block-1;
			bbr_arcs[i].head = block;
			bbr_arcs[i].weight = weight;
			i++;
		}

		firsti = BLOCK_HEAD(block);
		if (GET_CODE(firsti) == CODE_LABEL)
		{
			rtx	from = LABEL_REFS(firsti);
			for ( ; from != firsti; from = LABEL_NEXTREF(from))
			{
				rtx	branch = CONTAINING_INSN(from);
				int	prob;

				bbr_arcs[i].tail = BLOCK_NUM(branch);
				bbr_arcs[i].head = block;

				prob = jump_to_label_prob(branch, from);
				bbr_arcs[i].weight = (double)prob
						* (double)INSN_EXPECT(branch)
						/ (double)100;
				i++;
			}
		}
	}

	assert(i == bbr_num_arcs);

	/* Sort the arcs in ascending order by weight. */

	if (bbr_num_arcs > 1)
		qsort((void*)bbr_arcs, bbr_num_arcs, sizeof(BBR_arc),
						bbr_compare_arcs);

	if (bbr_dump_file)
	{
		(void)fprintf(bbr_dump_file,
			"\n;; Displaying %d arcs\n",bbr_num_arcs);
		(void)fprintf(bbr_dump_file, ";; tail  head  weight\n");
		for (i=0; i < bbr_num_arcs; i++)
		{
			(void)fprintf(bbr_dump_file, ";;%5d %5d  %5g\n",
			bbr_arcs[i].tail,bbr_arcs[i].head,bbr_arcs[i].weight);
		}
	}
}

/* Return non-zero if the chain containing block b2 can be appended
 * onto the chain containing block b1.
 */

static int
bbr_can_concat(b1, b2)
int b1;
int b2;
{
	int	b1_chain = bbr_containing_chain[b1];
	int	b2_chain = bbr_containing_chain[b2];

	/* If b1 is the end of its chain, and b2 is the start of a
	 * different chain, then we can concatenate the chains.
	 * But don't put the function's first block in the middle of a chain.
	 */

	if (b2 != FIRST_REAL_BLOCK && b1_chain != b2_chain
	    && bbr_first_block_in_chain[b2_chain] == b2
	    && bbr_next_block_in_chain[b1] == BBR_NULL)
		return 1;
	return 0;
}

static void
bbr_concat_chains(c1, c2)
int c1;
int c2;
{
	int	b;

	/* Mark c2's blocks as a member of chain c1. */

	b = bbr_first_block_in_chain[c2];
	for( ; b != BBR_NULL; b = bbr_next_block_in_chain[b])
		bbr_containing_chain[b] = c1;

	b = bbr_first_block_in_chain[c1];

	while (bbr_next_block_in_chain[b] != BBR_NULL)
		b = bbr_next_block_in_chain[b];

	bbr_next_block_in_chain[b] = bbr_first_block_in_chain[c2];
	bbr_first_block_in_chain[c2] = BBR_NULL;

	bbr_num_chains--;
}

/* Determining the new block ordering is done in two stages.
 *
 * First, we make a pass over the blocks looking for special cases.
 * For example, we may treat an if-then-else special if the then and else
 * parts are taken with near equal frequency.
 *
 * Second, we employ the "bottom-up" algorithm described in "Profile Guided
 * Code Positioning" by HP's Karl Pettis and Robert Hansen (ACM SIGPLAN '90).
 * Briefly, we form chains of basic blocks bottom up by examining all flow
 * graph arcs in order by weight, connecting the blocks at each end of an arc.
 * When all blocks have been placed in chains, we then determine an order
 * amongst the chains, which implicitly gives an order for all blocks.
 */

static void
bbr_find_new_order()
{
	int i;

	bbr_num_chains = LAST_REAL_BLOCK - FIRST_REAL_BLOCK + 1;
		/* Assumes FIRST_REAL_BLOCK is 0 */
	bbr_first_block_in_chain = (int*) xmalloc(bbr_num_chains*sizeof(int));
	bbr_next_block_in_chain = (int*) xmalloc(bbr_num_chains*sizeof(int));
	bbr_containing_chain = (int*) xmalloc(bbr_num_chains*sizeof(int));

	for (i = LAST_REAL_BLOCK; i >= FIRST_REAL_BLOCK; i--)
	{
		bbr_first_block_in_chain[i] = i;
		bbr_next_block_in_chain[i] = BBR_NULL;
		bbr_containing_chain[i] = i;
	}

	/* Phase 1: special cases. */

	for (i = FIRST_REAL_BLOCK; i <= LAST_REAL_BLOCK; i++)
	{
		/* Make sure to retain the adjacency of indirect jumps
		 * and their associated dispatch table.
		 * The Pettis/Hansen algorithm should handle this,
		 * but it's cheap to do here and safe in the event
		 * we change algorithms.
		 */

		rtx     lasti = BLOCK_END(i);

		if (GET_CODE(lasti) == JUMP_INSN
		    && (GET_CODE(PATTERN(lasti)) == ADDR_VEC
			|| GET_CODE(PATTERN(lasti)) == ADDR_DIFF_VEC))
		{
			rtx	lr;

			assert(GET_CODE(BLOCK_HEAD(i)) == CODE_LABEL);
			lr = LABEL_REFS(BLOCK_HEAD(i));

			for ( ; lr != BLOCK_HEAD(i); lr = LABEL_NEXTREF(lr))
			{
				int pred = BLOCK_NUM(CONTAINING_INSN(lr));
				if (pred == i - 1 && bbr_can_concat(pred, i))
				{
					bbr_concat_chains(
						bbr_containing_chain[pred],
						bbr_containing_chain[i]);
				}
			}
			continue;
		}

		bbr_handle_if_then_else(i);
	}

	/* Phase 2: */

	/* Walk the arcs from heaviest to lightest. */

	for (i = bbr_num_arcs-1; i >= 0; i--)
	{
		if (bbr_can_concat(bbr_arcs[i].tail, bbr_arcs[i].head))
		{
			bbr_concat_chains(
				bbr_containing_chain[bbr_arcs[i].tail],
				bbr_containing_chain[bbr_arcs[i].head]);
		}
	}

	if (bbr_dump_file)
	{
		(void) fprintf(bbr_dump_file,
				"\n;; Chains after Pettis/Hansen\n");

		for (i = FIRST_REAL_BLOCK; i <= LAST_REAL_BLOCK; i++)
		{
			int	j = bbr_first_block_in_chain[i];
			if (j < 0) continue;

			(void)fprintf(bbr_dump_file, ";; %5d:  ", i);

			do {
				(void)fprintf(bbr_dump_file, " %d", j);
				j = bbr_next_block_in_chain[j];
			} while (j != BBR_NULL);
			(void)fprintf(bbr_dump_file, "\n");
		}
	}

	/* Now determine an order for all chains. */
	/* Chain 0 is always first, since it contains the first basic block. */

	bbr_order_chains();

	if (bbr_dump_file)
	{
		(void)fprintf(bbr_dump_file, ";; New block ordering\n;; ");
		for (i = FIRST_REAL_BLOCK; i <= LAST_REAL_BLOCK; i++)
			(void)fprintf(bbr_dump_file, " %d",
						BBR_NEW_BLOCK_ORDER(i));
		(void)fprintf(bbr_dump_file, "\n");
	}
}

/* Keep the 'then' and 'else' blocks of a simple if-then-else
 * contiguous if 40 <= JUMP_THEN_PROB <= 60.
 * 'condblock' is the block containing the conditional jump.
 */

static void
bbr_handle_if_then_else(condblock)
int condblock;
{
	rtx	condjump = BLOCK_END(condblock);
	int	fallthrublock = condblock + 1;
	int	jumptoblock, jumptoblockprob, endblock;
	rtx	jump;

	if (condblock == LAST_REAL_BLOCK
	    || GET_CODE(condjump) != JUMP_INSN
	    || simplejump_p(condjump) || !condjump_p(condjump)
	    || JUMP_THEN_PROB(condjump) < 40
	    || JUMP_THEN_PROB(condjump) > 60
	   )
	{
		return;
	}

	jumptoblock = BLOCK_NUM(JUMP_LABEL(condjump));
	endblock = jumptoblock + 1;

	/* To be the simple if-then-else we're looking for, fallthrublock and
	 * jumptoblock must not have any predecessors other than condblock,
	 * and must share a common successor which fallthrublock jumps to
	 * and which jumptoblock falls into.
	 */

	if (bbr_number_of_predecessors(fallthrublock) != 1
	    || bbr_number_of_predecessors(jumptoblock) != 1
	    || GET_CODE(jump = BLOCK_END(fallthrublock)) != JUMP_INSN
	    || endblock > LAST_REAL_BLOCK
	    || !bbr_block_drops_into(endblock)
	    || bbr_number_of_predecessors(endblock) != 2
	    || JUMP_LABEL(jump) != BLOCK_HEAD(endblock)
	   )
	{
		return;
	}

	/* Determine which arc (if->then or if->else) is heavier */

	jumptoblockprob = JUMP_THEN_PROB(condjump);
	if (XEXP(SET_SRC(PATTERN(condjump)),1) == pc_rtx)
		jumptoblockprob = 100 - jumptoblockprob;

	if (jumptoblockprob < 50) {
		int	tmp = jumptoblock;
		jumptoblock = fallthrublock;
		fallthrublock = tmp;
	}

	/* Check that we can concatenate blocks in the order condblock,
	 * jumptoblock, fallthrublock, endblock.
	 */

	if (!bbr_can_concat(condblock, jumptoblock)
	    || !bbr_can_concat(jumptoblock, fallthrublock)
	    || !bbr_can_concat(fallthrublock, endblock))
		return;

	/* Cat 'em! */
	if (bbr_dump_file)
		(void)fprintf(bbr_dump_file,
			";; catting if-then-else blocks %d, %d, %d, %d\n",
			condblock, jumptoblock, fallthrublock, endblock);

	bbr_concat_chains(bbr_containing_chain[condblock],
				bbr_containing_chain[jumptoblock]);
	bbr_concat_chains(bbr_containing_chain[jumptoblock],
				bbr_containing_chain[fallthrublock]);
	bbr_concat_chains(bbr_containing_chain[fallthrublock],
				bbr_containing_chain[endblock]);
}

static void
bbr_order_chains()
{
	int	i, idx = 0;
	int	an_unplaced_chain, prev_chain;
	int	*chain_order = (int*)xmalloc(bbr_num_chains*sizeof(int));
	double	*in_weights = (double*)xmalloc(n_basic_blocks*sizeof(double));

	/* Put first the chain containing the function's first block. */
	chain_order[idx++] = bbr_first_block_in_chain[0];
	bbr_first_block_in_chain[0] = BBR_PLACED;

	prev_chain = 0;
	an_unplaced_chain = 1;

	while (idx < bbr_num_chains)
	{
		/* Find an unplaced chain with the heaviest connections
		 * to the previously placed chain.  If there are no
		 * connections, simply use the next unplaced chain.
		 */

		int	max_chain;
		double	w, max_weight = 0.0;

		while (bbr_first_block_in_chain[an_unplaced_chain] < 0)
			an_unplaced_chain++;
		max_chain = an_unplaced_chain;

		(void)bzero(in_weights, n_basic_blocks*sizeof(double));

		for (i = bbr_num_arcs-1; i >= 0; i--)
		{
			/* Skip this arc if it doesn't connect the previously
			 * placed chain with an unordered chain.
			 */

			int head_chain = bbr_containing_chain[bbr_arcs[i].head];
			int tail_chain = bbr_containing_chain[bbr_arcs[i].tail];

			if (tail_chain != prev_chain
			    || bbr_first_block_in_chain[head_chain] < 0)
				continue;

			w = bbr_arcs[i].weight;
			if ((in_weights[head_chain] += w) > max_weight)
			{
				max_weight = in_weights[head_chain];
				max_chain = head_chain;
			}
		}

		chain_order[idx++] = bbr_first_block_in_chain[max_chain];
		bbr_first_block_in_chain[max_chain] = BBR_PLACED;
	}

	xunalloc(&in_weights);

	/* bbr_first_block_in_chain[] now contains BBR_NULL's and BBR_PLACED's,
	 * while chain_order[C] contains the first block in chain C.
	 * bbr_first_block_in_chain is no longer needed.
	 *
	 * bbr_containing_chain is no longer needed to determine chain
	 * membership, so write the new block ordering into it.
	 * It is now accessed only via the macro BBR_NEW_BLOCK_ORDER.
	 */

	{
		int	c, b, n = 0;
		for (c=0; c < bbr_num_chains; c++)
		{
			b = chain_order[c];
			for (; b != BBR_NULL; b = bbr_next_block_in_chain[b])
				BBR_NEW_BLOCK_ORDER(n++) = b;
		}
		assert(n == LAST_REAL_BLOCK - FIRST_REAL_BLOCK + 1);
	}

	xunalloc(&chain_order);
}

static void
bbr_physically_rearrange_blocks()
{
	int	b, i;
	rtx	insn, prev_insn;

	/* If order hasn't changed, don't do anything. */
	/* This allows us to preserve notes perfectly. */

	for (b = LAST_REAL_BLOCK; b >= FIRST_REAL_BLOCK; b--)
		if (BBR_NEW_BLOCK_ORDER(b) != b)
			break;
	if (b < FIRST_REAL_BLOCK)
		return;

	/* First step is to patch fall-thru blocks that will fall thru
	 * to a different block in the new ordering.  This is done by
	 * adding a label (if needed) to the old fall-thru target block,
	 * and adding to the fall-thru source block an unconditional jump 
	 * to the old target block.
	 *
	 * We add all new insns to the insn stream before reordering
	 * the blocks.  We need to keep BLOCK_HEAD and BLOCK_END up to date,
	 * but for these new insns, BLOCK_NUM() is not valid.
	 */

	for (b = LAST_REAL_BLOCK; b >= FIRST_REAL_BLOCK; b--)
	{
		int	new_succ;
		rtx	old_succs_label, barrier;
		rtx	jump = BLOCK_END(b);

		if ((b < LAST_REAL_BLOCK && !bbr_block_drops_into(b+1))
		    || (b == LAST_REAL_BLOCK
		        && GET_CODE(jump) == JUMP_INSN
			&& (simplejump_p(jump) || !condjump_p(jump))))
			continue;	/* b is not a fall thru */

		new_succ = LAST_REAL_BLOCK + 1;
		for (i = LAST_REAL_BLOCK - 1; i >= FIRST_REAL_BLOCK; i--)
			if (BBR_NEW_BLOCK_ORDER(i) == b)
			{
				new_succ = BBR_NEW_BLOCK_ORDER(i+1);
				break;
			}

		if (new_succ == b+1)
			continue;	/* new successor is same as old one */

		/* At this point, a modification is needed because block b
		 * falls through to a different block, or falls off the end
		 * of the function, in the new order.  We must define a
		 * label for the old fall-through successor or for the
		 * end of the function, and at some point create a jump
		 * instruction with that label as its target.
		 */

		if (b < LAST_REAL_BLOCK
		    && GET_CODE(BLOCK_HEAD(b+1)) == CODE_LABEL)
		{
			old_succs_label = BLOCK_HEAD(b+1);
		}
		else
		{
			old_succs_label = gen_label_rtx();
			if (b < LAST_REAL_BLOCK)
			{
				emit_label_after(old_succs_label,
						PREV_INSN(BLOCK_HEAD(b+1)));
				INSN_EXPECT(old_succs_label) =
					INSN_EXPECT(BLOCK_HEAD(b+1));
				BLOCK_HEAD(b+1) = old_succs_label;
			}
			else
			{
				/* Put label after last basic block. */
				/* Don't separate a block and its barrier. */
				barrier = next_nonnote_insn(BLOCK_END(LAST_REAL_BLOCK));
				if (barrier && GET_CODE(barrier) == BARRIER)
					emit_label_after(old_succs_label,
								barrier);
				else
					emit_label_after(old_succs_label,
						BLOCK_END(LAST_REAL_BLOCK));
				INSN_EXPECT(old_succs_label) =
					INSN_EXPECT(BLOCK_END(LAST_REAL_BLOCK));
			}
		}

		LABEL_NUSES(old_succs_label)++;

		/* If b ends with a conditional jump, and its old branch
		 * target will be its new fall thru target, simply
		 * reverse the jump targets, and change the target label
		 * to old_succs_label.
		 * NOTE: BLOCK_NUM is valid here.
		 */

		if (GET_CODE(jump) == JUMP_INSN
		    && new_succ == BLOCK_NUM(JUMP_LABEL(jump)))
		{
			imstg_switch_jump(jump, old_succs_label);
			/* JUMP_THEN_PROB(jump) = 100 - JUMP_THEN_PROB(jump);
			 * JUMP_THEN_PROB is fine as is.
			 */
			LABEL_NUSES(JUMP_LABEL(jump))--;
			JUMP_LABEL(jump) = old_succs_label;
			/* TBD, check for new insn's validity.
			 * If not valid, just add a simple jump instead.
			 */
			continue;
		}

		/* Just add a simple jump at the end of block b */
		jump = emit_jump_insn_after(gen_jump(old_succs_label),
						BLOCK_END(b));
		JUMP_LABEL(jump) = old_succs_label;
		JUMP_THEN_PROB(jump) = 100;
		INSN_EXPECT(jump) = INSN_EXPECT(BLOCK_END(b));
		BLOCK_END(b) = jump;
		emit_barrier_after(jump);
	}

	/* Detach the block begin and block end notes before
	   reordering the blocks.
	 */

	bbr_num_scopes = 0;
	bbr_scopes = NULL;

	for (insn = get_insns(); insn; insn = NEXT_INSN(insn))
	{
		if (GET_CODE(insn) == NOTE
		    && NOTE_LINE_NUMBER(insn) == NOTE_INSN_BLOCK_BEG)
			bbr_num_scopes++;
	}


	if (bbr_num_scopes > 0)
	{
		int	next_scope = 1;
		int	active_scope = 0;
		rtx	next_insn;

		bbr_scopes = (struct bbr_scope*) xmalloc(
			(bbr_num_scopes+1) * sizeof(struct bbr_scope));

		/* First entry is unused. */
		(void)bzero(&bbr_scopes[0], sizeof(struct bbr_scope));

		bbr_max_original_insn_uid = get_max_uid();
		bbr_insn_scope = (int*) xmalloc(
			(bbr_max_original_insn_uid+1) * sizeof(int));
		(void)bzero(bbr_insn_scope,
			(bbr_max_original_insn_uid+1) * sizeof(int));

		bbr_scope_head = bbr_scope_tail = NULL_RTX;

		for (insn = get_insns(); insn; insn = next_insn)
		{
			next_insn = NEXT_INSN(insn);

			if (GET_CODE(insn) == NOTE
			    && NOTE_LINE_NUMBER(insn) == NOTE_INSN_BLOCK_BEG)
			{
				bbr_scopes[next_scope].parent = active_scope;
				bbr_scopes[next_scope].block_begin = insn;
				bbr_scopes[next_scope].block_end = NULL_RTX;
				bbr_scopes[next_scope].last_insn = NULL_RTX;
				active_scope = next_scope++;

				/* Detach this note from the insn stream. */
				if (PREV_INSN(insn))
				  NEXT_INSN(PREV_INSN(insn)) = NEXT_INSN(insn);
				if (NEXT_INSN(insn))
				  PREV_INSN(NEXT_INSN(insn)) = PREV_INSN(insn);

				/* Put the note on our scope list. */
				PREV_INSN(insn) = bbr_scope_tail;
				NEXT_INSN(insn) = NULL_RTX;
				if (bbr_scope_tail)
					NEXT_INSN(bbr_scope_tail) = insn;
				else
					bbr_scope_head = insn;
				bbr_scope_tail = insn;
			}

			bbr_insn_scope[INSN_UID(insn)] = active_scope;

			if (GET_CODE(insn) == NOTE
			    && NOTE_LINE_NUMBER(insn) == NOTE_INSN_BLOCK_END)
			{
				/* Find the scope index that started this scope.
				*/
				int s = next_scope-1;
				for (; s > 0 && bbr_scopes[s].block_end; s--)
					;
				assert(s > 0);
				bbr_scopes[s].block_end = insn;
				active_scope = bbr_scopes[s].parent;

				/* Detach this note from the insn stream. */
				if (PREV_INSN(insn))
				  NEXT_INSN(PREV_INSN(insn)) = NEXT_INSN(insn);
				if (NEXT_INSN(insn))
				  PREV_INSN(NEXT_INSN(insn)) = PREV_INSN(insn);

				/* Put the note on our scope list. */
				assert(bbr_scope_tail);

				PREV_INSN(insn) = bbr_scope_tail;
				NEXT_INSN(insn) = NULL_RTX;
				NEXT_INSN(bbr_scope_tail) = insn;
				bbr_scope_tail = insn;
			}
		}
	}

	/* Now reorder the blocks by changing links in the insn stream.
	   To maintain line number and other notes, first include all
	   insns within some basic block.  Barriers that fall between blocks
	   b and b+1 are placed in block b.  Notes after the barrier, or all
	   notes if there is no barrier, are placed in block b+1.  No need
	   to worry about such insns before the first block (since it doesn't
	   get reordered, or after the last block.)
	   Note that this means BLOCK_HEAD and BLOCK_END may now point to
	   NOTE insns or BARRIERS.  This is okay, since no one uses the
	   basic block data structures in the state we leave them.
	 */

	assert(BBR_NEW_BLOCK_ORDER(FIRST_REAL_BLOCK) == 0);

	for (i = 0; i <= LAST_REAL_BLOCK; i++)
	{
		/* Look for a barrier following this block. */

		rtx	lim = NULL_RTX;
		if (i != LAST_REAL_BLOCK)
			lim = BLOCK_HEAD(i+1);

		insn = NEXT_INSN(BLOCK_END(i));
		for (; insn != lim && GET_CODE(insn) != BARRIER;)
			insn = NEXT_INSN(insn);

		if (insn != lim)
		{
			/* Found a barrier, put it in block i. */
			BLOCK_END(i) = insn;
			if (i < LAST_REAL_BLOCK)
				BLOCK_HEAD(i+1) = NEXT_INSN(insn);
		}
		else if (i < LAST_REAL_BLOCK)
			BLOCK_HEAD(i+1) = NEXT_INSN(BLOCK_END(i));

		/* Now make sure the correct line number note will be applied
		   to the insns in block i+1 after it gets reordered.
		 */

		if (i < LAST_REAL_BLOCK
		    && BBR_NEW_BLOCK_ORDER(i) != BBR_NEW_BLOCK_ORDER(i+1) - 1)
		{
			rtx	note = BLOCK_HEAD(i+1);

			lim = NEXT_INSN(BLOCK_END(i+1));

			/* If there's a line number note in the block before
			   any active insns, then there's nothing to do.
			 */
			for (; note != lim && GET_CODE(note) == NOTE
					   && NOTE_LINE_NUMBER(note) < 0;)
				note = NEXT_INSN(note);

			if ((note == lim || GET_CODE(note) != NOTE
					 || NOTE_LINE_NUMBER(note) < 0)
			     && (note = bbr_effective_note(BLOCK_HEAD(i+1))))
			{
			  /* Need to put a copy of the effective note
			     at the start of block i + 1.
			   */
			  rtx	copy;

			  copy = emit_line_note_after(
					NOTE_SOURCE_FILE(note),
					NOTE_LINE_NUMBER(note),
					BLOCK_END(i));
			  if (copy)
			  {
			    BLOCK_HEAD(i+1) = copy;
			    NOTE_LISTING_START(copy) = NOTE_LISTING_START(note);
			    NOTE_LISTING_END(copy) = NOTE_LISTING_END(note);
#if defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
			    if (flag_linenum_mods)
			    {
			      NOTE_AUX_SRC_INFO(copy) = NOTE_AUX_SRC_INFO(note);
			      NOTE_CLEAR_STMT_BEGIN(copy);
			    }
#endif
			  }
			}
		}
	}

	prev_insn = BLOCK_END(0);

	for (i = 1; i <= LAST_REAL_BLOCK; i++)
	{
		b = BBR_NEW_BLOCK_ORDER(i);

		if (prev_insn != PREV_INSN(BLOCK_HEAD(b)))
			reorder_insns(BLOCK_HEAD(b), BLOCK_END(b), prev_insn);
		prev_insn = BLOCK_END(b);
	}

	/* Put back the block begin and end notes. */

	if (bbr_num_scopes > 0)
	{
	  int	scope;

	  /* Find the last insn in each scope. */

	  for (insn = get_insns(); insn; insn = NEXT_INSN(insn))
	  {
		if (INSN_UID(insn) <= bbr_max_original_insn_uid
		    && (scope = bbr_insn_scope[INSN_UID(insn)]) > 0)
			bbr_scopes[scope].last_insn = insn;
	  }

	  for (insn = get_insns(); insn; insn = NEXT_INSN(insn))
	  {
		if (INSN_UID(insn) > bbr_max_original_insn_uid
		    || (scope = bbr_insn_scope[INSN_UID(insn)]) == 0)
			continue;

		/* Should we emit the block begin note for this insn's scope? */
		if (bbr_scopes[scope].block_begin)
		{
		  rtx	x;
		  int	s;

		  /* Flush begin and end notes for prior scopes
		     which MUST occur before this begin scope.
		   */

		  x = bbr_scope_head;
		  for (; x != bbr_scopes[scope].block_begin; x = NEXT_INSN(x))
		  {
			/* Set the scope's begin and end notes to NULL_RTX,
			   indicating the notes have been reattached to
			   the insn stream.
			 */

			s = bbr_insn_scope[INSN_UID(x)];
			if (NOTE_LINE_NUMBER(x) == NOTE_INSN_BLOCK_BEG)
				bbr_scopes[s].block_begin = NULL_RTX;
			else
				bbr_scopes[s].block_end = NULL_RTX;
		  }

		  bbr_scopes[scope].block_begin = NULL_RTX;
		  PREV_INSN(bbr_scope_head) = PREV_INSN(insn);
		  if (PREV_INSN(insn))
			NEXT_INSN(PREV_INSN(insn)) = bbr_scope_head;
		  else
			abort(); /* TBD, bbr_scope_head is now first insn */

		  bbr_scope_head = NEXT_INSN(x);
		  if (bbr_scope_head)
			PREV_INSN(bbr_scope_head) = NULL_RTX;
		  else
			bbr_scope_tail = NULL;

		  NEXT_INSN(x) = insn;
		  PREV_INSN(insn) = x;
		}

		/* Should we emit the block end note for this insn's scope?
		   Do so only if this is the last insn in the scope,
		   and emitting this note won't force premature emission
		   of prior scopes.
		 */
		if (bbr_scopes[scope].block_end
		    && insn == bbr_scopes[scope].last_insn
		    && bbr_scopes[scope].block_end == bbr_scope_head)
		{
			bbr_scope_head = NEXT_INSN(bbr_scope_head);
			if (bbr_scope_head)
				PREV_INSN(bbr_scope_head) = NULL_RTX;
			else
				bbr_scope_tail = NULL;
			add_insn_after(bbr_scopes[scope].block_end, insn);
			bbr_scopes[scope].block_end = NULL_RTX;
		}
	  }

	  /* Put the remaining notes at the end of the function.  */
	  if (bbr_scope_head)
	  {
		insn = get_last_insn();

		for (; PREV_INSN(insn)
		       && (INSN_UID(insn) > bbr_max_original_insn_uid
			   || bbr_insn_scope[INSN_UID(insn)] == 0); )
			insn = PREV_INSN(insn);

		NEXT_INSN(bbr_scope_tail) = NEXT_INSN(insn);
		if (NEXT_INSN(insn))
			PREV_INSN(NEXT_INSN(insn)) = bbr_scope_tail;
		NEXT_INSN(insn) = bbr_scope_head;
		PREV_INSN(bbr_scope_head) = insn;
		bbr_scope_head = bbr_scope_tail = NULL_RTX;
	  }
	}

	/* Count simple branch-to-next cases.  If there are many of
	 * them, maybe we can simplify them here and run just before sched2,
	 * leaving larger blocks for sched2.
	 */

	if (bbr_dump_file)
	{
		rtx	jump, x;
		int	num_simple_btn = 0;
		int	num_cond_btn = 0;

		for (jump = get_insns(); jump; jump = NEXT_INSN(jump))
		{
			if (GET_CODE(jump) != JUMP_INSN ||
			    (!simplejump_p(jump) && !condjump_p(jump)))
				continue;
			assert(JUMP_LABEL(jump));
			x = NEXT_INSN(jump);
			while (x &&
				(GET_CODE(x) == NOTE || GET_CODE(x) == BARRIER))
				x = NEXT_INSN(x);
			if (x == JUMP_LABEL(jump))
				if (simplejump_p(jump))
					num_simple_btn++;
				else
					num_cond_btn++;
		}
		(void)fprintf(bbr_dump_file,
			"\n;; branch-to-nexts: %d simple, %d cond\n\n",
					num_simple_btn, num_cond_btn);
	}
}

static void
bbr_free_storage()
{
	xunalloc(&bbr_arcs);
	xunalloc(&bbr_first_block_in_chain);
	xunalloc(&bbr_next_block_in_chain);
	xunalloc(&bbr_containing_chain);
	xunalloc(&bbr_scopes);
	xunalloc(&bbr_insn_scope);
}

#endif	/* IMSTG */
