#include "config.h"

#ifdef IMSTG
#include "rtl.h"
#include "flags.h"
#include "assert.h"

#include "i_graph.h"

extern double d_to_d();

/**
*** Local type definitions
**/

typedef struct loop_tag  *LOOP, LOOP_STRUCT;

struct loop_tag {
    GRAPH_NODE  loop_head;
    int le_size;                   /* allocated size of loop entrances vector */
    int le_count;		   /* loop entrance count */
    GRAPH_NODE *loop_entrances;
    int lx_size;                   /* allocated size of loop exits vector */
    int lx_count;		   /* loop exit count     */
    GRAPH_NODE *loop_exits;
    double trip_count;
};

/**
*** Local variables
**/
static LOOP expect_loops = 0;

/**
*** Forward Declarations
**/

static void expect_adjust_loop_exits();

static double expect_node_calc();
static double expect_edge_calc();

static void expect_init_loops();
static void expect_loop_entry();
static void expect_loop_exit();
static void expect_free_loops();

static void fill_expect (node, expect)
	GRAPH_NODE node;
	int expect;
{
	rtx insn;

	INSN_EXPECT (node->insn) = expect;
	if ( node == GRAPH_packed_nodes + GRAPH_node_count - 1 )
		return;
	for ( insn = node->insn; insn; insn = NEXT_INSN (insn) ) {
		/*
		 * only these have a field
		 */
		if ( GET_CODE (insn) != CALL_INSN &&
		     GET_CODE (insn) != JUMP_INSN &&
		     GET_CODE (insn) != INSN &&
		     GET_CODE (insn) != CODE_LABEL )
			continue;
		if ( insn == (node+1)->insn )
			break;
		INSN_EXPECT (insn) = expect;
	} /* end for */
} /* end fill_expect */


void
expect_calculate (f, used_prof_info)
rtx f;
int used_prof_info;
{
  GRAPH_NODE node;
  rtx insn;
  int i;

  assert(GRAPH_entry);

  expect_init_loops(f);
  
  /*
   * Don't even try to adjust loop exit probabilities if we used profiling
   * information to get them.  They are already more accurate than anything
   * we could compute.
   */
  if (!used_prof_info)
    expect_adjust_loop_exits();
  
  /*
   * Set the expect for graph entry node
   */

#if !defined(DEV_465)
  assert(GET_CODE(GRAPH_entry->insn) == NOTE &&
         NOTE_LINE_NUMBER(GRAPH_entry->insn) == NOTE_INSN_FUNCTION_ENTRY);
#else
  assert(GET_CODE(GRAPH_entry->insn) == NOTE &&
         NOTE_LINE_NUMBER(GRAPH_entry->insn) == NOTE_INSN_FUNCTION_BEG);
#endif
	 GRAPH_entry->expect = NOTE_CALL_COUNT(GRAPH_entry->insn);

  /*
   * Run through all nodes in the flow graph, calculate their
   * 'expect' and shove it down into the rtl.
   */

  GRAPH_search_mark++;
  for (node = GRAPH_packed_nodes, i = 0; i < GRAPH_node_count; node++, i++)
  {
    double tmp;

    if (!node->insn) continue;
    if (node == GRAPH_entry) continue;

    tmp = expect_node_calc(node);
    if (tmp > (2147483647 / PROB_BASE))
      fill_expect (node, 2147483647);
    else if (tmp < 0.01)
      fill_expect (node, 0);
    else
      fill_expect(node, (int)d_to_d(tmp * PROB_BASE));
  }

  expect_free_loops();
}

expect_break()
{
}

int expect_break_number = -1;

static double
expect_node_calc(node)
    GRAPH_NODE node;
{
    if (node->expect) return node->expect;
    if (node->search_mark == GRAPH_search_mark) return node->expect;

    if (expect_break_number == INSN_UID(node->insn)) expect_break();
        /**
        *** Prevent cycles.  This is necessary because
        *** we do not yet find every loop in the graph and
        *** there may be dead code
        **/
    node->search_mark = GRAPH_search_mark;

    if (node->loop_head) {
        int                  loop_number = node->loop_number;
        LOOP                 loop        = expect_loops + loop_number;
        double entered     = 0.0;
        int                  i;
        assert(GET_CODE(node->insn) == CODE_LABEL);

        for (i = 0; i < loop->le_count; i++) {
            GRAPH_NODE loop_entrance  = loop->loop_entrances[i];
            GRAPH_NODE loop_succ = 0;
            int j;

            for (j = 0; j < loop_entrance->succ_count; j++) {
                loop_succ = loop_entrance->succs[j];
                if (loop_succ->loop_number == loop_number) break;
            }
            assert(j < loop_entrance->succ_count);

            entered = d_to_d(entered +
                        d_to_d(expect_node_calc(loop_entrance) *
                               expect_edge_calc(loop_entrance, loop_succ)));
        }

        node->expect = d_to_d(entered * loop->trip_count);
    }
    else {
        double sum = 0.0;
        GRAPH_NODE           pred;
        int                  i;

        for (i = 0; i < node->pred_count; i++) {
            pred = node->preds[i];
            sum = d_to_d(sum +
                    d_to_d(expect_node_calc(pred) *
                           expect_edge_calc(pred, node)));
        }
        node->expect = sum;
    }
    return node->expect;
}

/**
*** Return the probability of traversing the edge from pred to succ.
**/

static double
expect_edge_calc(pred, succ)
    GRAPH_NODE pred;
    GRAPH_NODE succ;
{
    int i;

    for (i = 0; i < pred->succ_count; i++) {
        if (pred->succs[i] == succ) return pred->probs[i];
    }
    abort();
}

static void
expect_init_loops (f)
    rtx f;
{
    int i;

    if (GRAPH_loop_count == 0) return;

    expect_loops = (LOOP) xmalloc(GRAPH_loop_count * sizeof(LOOP_STRUCT));
    bzero(expect_loops, GRAPH_loop_count * sizeof(LOOP_STRUCT));

    for (i = 0; i < GRAPH_node_count; i++) {
        GRAPH_NODE node = GRAPH_packed_nodes + i;

        if (node->loop_head) {
            rtx x;
            LOOP loop = expect_loops + node->loop_number;
            loop->loop_head = node;
            for (x = PREV_INSN(node->insn); x; x = PREV_INSN(x)) {
               if (   GET_CODE(x) == NOTE
                   && NOTE_LINE_NUMBER(x) == NOTE_INSN_LOOP_BEG
               ) {
                   loop->trip_count = d_to_d((double)NOTE_TRIP_COUNT(x) /
                                             PROB_BASE);
                   break;
               }
            }
            assert(x && GET_CODE(x) == NOTE);
        }

        if (node->loop_entrance) {
            expect_loop_entry(node);
        } 

        if (node->loop_exit) {
            expect_loop_exit(node);
        } 
    }
}

static int loop_increment = 4;

static void
expect_loop_entry(entrance)
    GRAPH_NODE entrance;
{
    int loop_number = entrance->loop_number;
    int i;

    for (i = 0; i < entrance->succ_count; i++) {
        GRAPH_NODE succ = entrance->succs[i];
        if (   succ->loop_number != loop_number
            && succ->loop_number != -1
        ) {
            LOOP loop = expect_loops + succ->loop_number;
            assert(succ->loop_number >= 0);
            assert(succ->loop_number < GRAPH_loop_count);
    
            if (loop->le_count >= loop->le_size) {
                if (loop->loop_entrances) {
	            loop->loop_entrances = (GRAPH_NODE *) xrealloc(
                        loop->loop_entrances,
                        (loop->le_count + loop_increment) * sizeof(GRAPH_NODE)
                    );
                }
                else {
	            loop->loop_entrances = (GRAPH_NODE *) xmalloc(
                        loop_increment * sizeof(GRAPH_NODE)
                    );
                }
                loop->le_size += loop_increment;
            }
            assert(loop->le_count < loop->le_size);
            loop->loop_entrances[loop->le_count++] = entrance;
        }
    }
}

static void
expect_loop_exit(exit)
    GRAPH_NODE exit;
{
    LOOP loop = expect_loops + exit->loop_number;

    if (loop->lx_count >= loop->lx_size) {
        if (loop->loop_exits) {
	   loop->loop_exits = (GRAPH_NODE *) xrealloc(
               loop->loop_exits,
               (loop->lx_count + loop_increment) * sizeof(GRAPH_NODE)
           );
        }
        else {
	   loop->loop_exits = (GRAPH_NODE *) xmalloc(
               loop_increment * sizeof(GRAPH_NODE)
           );
        }
        loop->lx_size += loop_increment;
    }
    assert(loop->lx_count < loop->lx_size);
    loop->loop_exits[loop->lx_count++] = exit;
}

static void
expect_free_loops()
{
  LOOP	loop;
  int	i;

  if (expect_loops)
  {
    for (loop = expect_loops, i = 0; i < GRAPH_loop_count; loop++, i++)
    {
      if (loop->loop_exits)
        free(loop->loop_exits);
      if (loop->loop_entrances)
        free(loop->loop_entrances);
    }

    free(expect_loops);
    expect_loops = 0;
  }
}

static void
expect_adjust_loop_exits()
{
  /*
   * There is some fundamental icorrectness associated with this algorithm.
   * In particular the loop_trip_count isn't necessarily directly related
   * to the jump probabailities of the individual exits.  Depending on how
   * deeply the exit happens to be buried in a branching construct, the
   * probability of ever reaching the exit may be very low although the
   * branch probability of leaving the loop from here may be high.
   *
   * So we are going to adjust the jump_probabilities only for jumps
   * (presumably conditional) which are both backedges, and loop exits.
   * In addition we will never even call this routine if the probabilities,
   * and trip counts were obtained from a real profile, I assume they will
   * always be more accurate than any adjustment that would be made.
   */
  int i;
  for (i = 0; i < GRAPH_loop_count; i++)
  {
    int j;
    LOOP loop = expect_loops + i;
    double adj_exit_prob;
 
    if (loop->lx_count == 0)
      continue;  /* don't bother worrying about loops with no exits. */

    /*
     * The total exit probability of the loop is:
     *  1 - ((loop_trip_count-1)/loop_trip_count).
     * This is because the loop is fallen through at the top once at loop
     * entry, so a loop that has a trip count of 2.0 comes in once at the top
     * and only loops back around once.
     * However if loop_trip_count <= 1 the the exit probability is 1.
     * The probability of each exit is therefore approximately
     * total exit probability / number of exits.  Again this is an 
     * extreme simplification since the probability that this particular
     * exit is reached may vary significantly.  However in the case
     * of a loop with a small number of exits this maybe isn't too
     * far off, and in the case of a loop with a single exit it is very
     * close.  These should be the more common cases anyway.
     */
    if (loop->trip_count <= 1.0)
      adj_exit_prob = 1.0;
    else
    {
      adj_exit_prob = d_to_d(1.0 -
                        d_to_d(d_to_d(loop->trip_count - 1) /
                               loop->trip_count));
      adj_exit_prob = d_to_d(adj_exit_prob / loop->lx_count);

      if (adj_exit_prob > 1.0)
        adj_exit_prob = 1.0;
    }

    for (j = 0; j < loop->lx_count; j++)
    {
      GRAPH_NODE exit = loop->loop_exits[j];
      int k;
      int n_exits = 0;
      double non_exit_prob = 1.0;

      /* adjust probabilities for exits */
      for (k = 0; k < exit->succ_count; k++)
      {
        if (exit->succs[k]->loop_number != exit->loop_number)
        {
          /* this successor is an exit. */
          exit->probs[k] = adj_exit_prob;
          non_exit_prob = d_to_d(non_exit_prob - adj_exit_prob);
          n_exits += 1;
        }
      }

      /* adjust probabilities for all non-exits. */
      if (exit->succ_count - n_exits > 0)
      {
        non_exit_prob = d_to_d(non_exit_prob / (exit->succ_count - n_exits));
        for (k = 0; k < exit->succ_count; k++)
        {
          if (exit->succs[k]->loop_number == exit->loop_number)
            exit->probs[k] = non_exit_prob;
        }
      }

      if (GET_CODE(exit->insn) == JUMP_INSN)
      {
        register rtx x = PATTERN (exit->insn);
        int prob = PROB_BASE;

        if (GET_CODE (x) == SET &&
            GET_CODE (SET_DEST (x)) == PC &&
            GET_CODE (SET_SRC (x)) == IF_THEN_ELSE)
        {
          if (XEXP (SET_SRC (x), 2) == pc_rtx)
            prob = (int)d_to_d(exit->probs[0] * PROB_BASE);

          if (XEXP (SET_SRC (x), 1) == pc_rtx)
            prob = PROB_BASE - ((int)d_to_d(exit->probs[0] * PROB_BASE));
        }

        JUMP_THEN_PROB(exit->insn) = prob;
      }
    }
  }
}
#endif /* IMSTG */
