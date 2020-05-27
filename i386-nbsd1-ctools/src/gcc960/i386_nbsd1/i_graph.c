#include "config.h"
#ifdef IMSTG
#include <stdio.h>
#include "rtl.h" 
#include "real.h"
#include "assert.h" 

#ifdef GCC20
#ifndef GRAPH_INCLUDED
#include "i_graph.h"
#endif
#else
#ifndef GRAPH_INCLUDED
#include "graph.h"
#endif
#endif


static void graph_CreateEdge();
static void graph_AllocSuccs();
static void graph_AllocPreds();
static void graph_AllocProbs();
static int graph_JumpSucc();
static int graph_SuccCount();
static void graph_PrintNode();
static void graph_InitLoops();

static int *graph_loop_outerloop;
static int *graph_loop_id;

GRAPH_NODE GRAPH_entry = NULL;
GRAPH_NODE * GRAPH_nodes = NULL;
int GRAPH_node_count = 0;
int GRAPH_loop_count = 0;
int GRAPH_search_mark = 0;
static rtx ending_insn = 0;

static GRAPH_NODE next_node;
GRAPH_NODE GRAPH_packed_nodes;


GRAPH_NODE
GRAPH_BuildGraph(f)
rtx f;
{
  rtx insn;
  int id_count;
  int line_num = 0;
  int insn_count;
  int prev_was_jump;

  /*
   * It is possible to have a function which just falls off the end of
   * the function into the epilgoue.  This is a problem for the graph
   * building algorithm.  If this is such a function, then cheat and make
   * and insn up which we will later delete that can mark the end of the
   * function.
   */
  for (insn = get_last_insn(); insn != 0; insn = PREV_INSN(insn)) {
    if (GET_CODE(insn) == NOTE)
      continue;

    if (GET_CODE(insn) == JUMP_INSN) {
      if (GET_CODE(PATTERN(insn)) == RETURN)
	ending_insn = insn;
      break;
    }
  }
  if ( ! ending_insn ) {
      /* make up the insn, it will be deleted later. */
      if (regno_reg_rtx[FIRST_PSEUDO_REGISTER] == 0)
        gen_reg_rtx(word_mode);
      ending_insn = emit_insn_after(
                     gen_rtx(SET, SImode, regno_reg_rtx[FIRST_PSEUDO_REGISTER],
                                          regno_reg_rtx[FIRST_PSEUDO_REGISTER]),
                     get_last_insn());
  }

  /*
  * count the number of insns which will need a node
  */
  GRAPH_node_count = 2;	/* for the entry and exit nodes */
  for ( insn = f; insn ; insn = NEXT_INSN (insn) )
  {
    if ( GET_CODE (insn) == JUMP_INSN)
      GRAPH_node_count += 2;
    else if (GET_CODE (insn) == CODE_LABEL)
      GRAPH_node_count += 1;
  } /* end for */

  insn_count = get_max_uid() + 1;
  GRAPH_loop_count = 0;
  GRAPH_entry = NULL;

  next_node = GRAPH_packed_nodes =
	(GRAPH_NODE) xmalloc(sizeof(GRAPH_NODE_STRUCT) * GRAPH_node_count);
  bzero (GRAPH_packed_nodes, GRAPH_node_count * sizeof(GRAPH_NODE_STRUCT));

  GRAPH_nodes = (GRAPH_NODE *)
                xmalloc(sizeof(GRAPH_NODE) * insn_count);
  bzero (GRAPH_nodes, insn_count * sizeof(GRAPH_NODE));

  graph_loop_id = (int *) xmalloc(sizeof(int *) * insn_count);
  bzero (graph_loop_id, insn_count * sizeof(int *));

  id_count = 0;
  prev_was_jump = 0;
  for (insn = f; insn; insn = NEXT_INSN(insn))
  {
    switch (GET_CODE(insn))
    {
      case NOTE:
      {
        /*
         * Put the NOTE_INSN_FUNCTION_ENTRY note into the graph so that
         * we are insured to have an entry to the graph that
         * has no predecessors.
         */
        if (NOTE_LINE_NUMBER(insn) >= 0) 
          line_num = NOTE_LINE_NUMBER(insn); 
 
#if !defined(DEV_465)
        if (NOTE_LINE_NUMBER(insn) == NOTE_INSN_FUNCTION_ENTRY)
#else
        if (NOTE_LINE_NUMBER(insn) == NOTE_INSN_FUNCTION_BEG)
#endif
        {
          assert(GRAPH_entry == NULL);
          GRAPH_nodes[INSN_UID(insn)] = next_node++;
          GRAPH_nodes[INSN_UID(insn)]->insn = insn;
          GRAPH_entry = GRAPH_nodes[INSN_UID(insn)];
        }

        if (NOTE_LINE_NUMBER(insn) == NOTE_INSN_LOOP_BEG)
          GRAPH_loop_count++;
      }
      break;

      case INSN:
      case CALL_INSN:
      {
        if (prev_was_jump)
        {
          graph_loop_id[INSN_UID(insn)] = id_count++;
          GRAPH_nodes[INSN_UID(insn)] = next_node++;
          GRAPH_nodes[INSN_UID(insn)]->insn = insn;
          GRAPH_nodes[INSN_UID(insn)]->line_num = line_num;
          prev_was_jump = 0;
        }
      }
      break;

      case JUMP_INSN:
      {
        graph_loop_id[INSN_UID(insn)] = id_count++;
        GRAPH_nodes[INSN_UID(insn)] = next_node++;
        GRAPH_nodes[INSN_UID(insn)]->insn = insn;
        GRAPH_nodes[INSN_UID(insn)]->line_num = line_num;
        prev_was_jump = 1;
      }
      break;

      case CODE_LABEL:
      {
        graph_loop_id[INSN_UID(insn)] = id_count++;
        GRAPH_nodes[INSN_UID(insn)] = next_node++;
        GRAPH_nodes[INSN_UID(insn)]->insn = insn;
        GRAPH_nodes[INSN_UID(insn)]->line_num = line_num;
        prev_was_jump = 0;
      }
      break;
    }
  }

  /*
   * the exit insn may need to be allocated seperately
   */
  if ( ! GRAPH_nodes [INSN_UID (ending_insn)] ) {
          GRAPH_nodes[INSN_UID(ending_insn)] = next_node++;
          GRAPH_nodes[INSN_UID(ending_insn)]->insn = ending_insn;
  }


  for (insn = f; insn; insn = NEXT_INSN(insn))
  {
    rtx next_insn;
    GRAPH_NODE succ;
    GRAPH_NODE curr = GRAPH_nodes [INSN_UID(insn)];

    switch (GET_CODE(insn))
    {
      case NOTE :
          if ( ! GRAPH_nodes [INSN_UID (insn)] )
		break;
      case CODE_LABEL: 
      {
        succ = NULL;
        assert(curr->succ_count == 0);

        /* Find the fall through successor */

        for (next_insn = NEXT_INSN(insn); next_insn;
             next_insn = NEXT_INSN(next_insn))
        {
          if ( succ = GRAPH_nodes[INSN_UID(next_insn)] )
            break;
        }

        if (next_insn && GET_CODE(next_insn) != BARRIER && succ != GRAPH_entry)
        {
          graph_AllocSuccs(curr, 1);
          graph_AllocPreds(succ, 1);
          graph_CreateEdge(curr, succ);
          graph_AllocProbs(curr);
          curr->probs[0] = 1.0;
        }
      }
      break;

      case JUMP_INSN:
      {
        int succ_count;
        int i;

        assert(curr->pred_count <= 1);
        assert(curr->succ_count == 0);

        succ_count = graph_SuccCount(PATTERN(insn));
        assert(succ_count > 0 || GET_CODE(PATTERN(insn)) == RETURN);

        /*
         * The maximum number of successors that you can
         * have is succ_count + 1 for the fall through edge
         * if it exists.
         */
        graph_AllocSuccs(curr, succ_count +1); 

        /*
         * Initialize the edges for each successor
         */
        if (graph_JumpSucc(PATTERN(insn), curr))
        {
          GRAPH_FreeGraph();
          return NULL;
        }

        /*
         * At this point you should be able to distinguish
         * between the 3 types of jumps:
         *    unconditional  -  1 successor
         *    conditional    -  2 successors
         *    tablejump      -  multiple successors
         *
         * Calculate the probabilities for each successor
         * and add in the fall through edge for conditional
         * branches;
         */

        if (condjump_p(insn) && !simplejump_p(insn))
        {
          if (succ_count != 1)
            abort();

          /*
           * We need to add the fallthru successor to the
           * graph.  We need to be careful though, because
           * the fallthru successor, may be the end of the function,
           * which the rtl allows to just fall off into the
           * epilogue.  Therefore we cannot count on being able
           * to find a real insn for a successor.  At the beginning
           * of BuildGraph we assured that there was always one real
           * insn at end of the function.
           */
          /* Find the fall through successor */
          for (next_insn = NEXT_INSN(insn); next_insn;
               next_insn = NEXT_INSN(next_insn))
          {
            if ( succ = GRAPH_nodes[INSN_UID(next_insn)] )
              break;
          }

          if (next_insn == 0)
            abort();

          graph_AllocPreds(succ, 1);
          graph_CreateEdge(curr, succ);
          assert(curr->succ_count == 2);
          graph_AllocProbs(curr);
          curr->probs[0] =  (double)JUMP_THEN_PROB(insn)
                              / (double)PROB_BASE;
          curr->probs[1] = 1.0 - curr->probs[0];
        }
        else
        {
          /*
           * An unconditional jump to whatever label is specified,
           * or an unconditional tablejump.
           */
          graph_AllocProbs(curr);

          if (curr->succ_count > 0)
          {
            double prob = 1.0 / curr->succ_count;
            int i;
            for (i = 0; i < curr->succ_count; i++)
              curr->probs[i] = prob;
          }
        }
      }
      break;

      case INSN:
      case CALL_INSN:
      {
        if (curr != 0)
        {
          /*
           * Find the fall through successor, its possible that this
           * doesn't have one if this is the last insn in the function
           * already.
           */
          for (next_insn = NEXT_INSN(insn); next_insn;
               next_insn = NEXT_INSN(next_insn))
          {
            if ( succ = GRAPH_nodes[INSN_UID(next_insn)] )
              break;
          }

          if (next_insn != 0)
          {
            graph_AllocSuccs(curr, 1);
            graph_AllocPreds(succ, 1);
            graph_CreateEdge(curr, succ);
            graph_AllocProbs(curr);
            curr->probs[0] = 1.0;
          }
        }
      }
      break;
    }
  }

  assert(GRAPH_entry);

  if ( GRAPH_entry->succ_count != 0 ) {
    assert(GRAPH_entry->pred_count == 0);
    graph_InitLoops(f);
  }

  return GRAPH_entry;
}

void
GRAPH_FreeGraph()
{
  if ( GET_CODE(PATTERN(ending_insn)) != RETURN ) {
    delete_insns_since(PREV_INSN(ending_insn));
  }
  ending_insn = 0;

  if ( GRAPH_packed_nodes )
  {
    int i;

    for (i = 0; i < GRAPH_node_count; i++) {
       GRAPH_NODE node = GRAPH_packed_nodes + i;
       if (node->preds != 0) free(node->preds);
       if (node->succs != 0) free(node->succs);
       if (node->probs != 0) free(node->probs);
    }
    free(GRAPH_packed_nodes);
  }
  free(GRAPH_nodes);

  GRAPH_packed_nodes = 0;
  GRAPH_nodes = 0;
  GRAPH_entry = 0;
  GRAPH_node_count = 0;
}

/*
 * Find all the label references in x and add them as successors of node.
 *
 * x should be the PATTERN of a JUMP_INSN.
 *
 * Returns 0 if there were no errors, 1 if there were errors
 */
static int
graph_JumpSucc (x, node)
rtx x;
GRAPH_NODE node;
{
  register RTX_CODE code = GET_CODE (x);
  register int i;
  register char *fmt;

  if (code == LABEL_REF)
  {
    register rtx label = XEXP (x, 0);
    GRAPH_NODE succ;

    if (GET_CODE (label) != CODE_LABEL)
      abort ();

    /* check for duplicate ref from same insn and don't insert.  */
    for (i = 0; i < node->succ_count; i++)
    {
      if (node->succs[i]->insn == label)
        return 0;
    }

    /* If this is a bad insn bail out */
    if (INSN_UID(label) == 0)
      return 0;
 
    succ = GRAPH_nodes [INSN_UID(label)];
    assert(succ->insn == label);
    graph_AllocPreds(succ, 1);
    graph_CreateEdge(node, succ);

    return 0;
  }

  fmt = GET_RTX_FORMAT (code);
  for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
  {
    if (fmt[i] == 'e' &&
        graph_JumpSucc (XEXP (x, i), node) != 0)
      return 1;

    if (fmt[i] == 'E')
    {
      register int j;
      for (j = 0; j < XVECLEN (x, i); j++)
        if (graph_JumpSucc (XVECEXP (x, i, j), node))
          return 1;
    }
  }
  return 0;
}

/*
 * Return the maximum number of successors for x.
 *
 * x must be the PATTERN of a JUMP_INSN.
 */

static int
graph_SuccCount(x)
rtx x;
{
  register RTX_CODE code = GET_CODE (x);
  register int i, count;
  register char *fmt;

  if (code == LABEL_REF)
    return 1;

  count = 0;

  fmt = GET_RTX_FORMAT (code);
  for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
  {
    if (fmt[i] == 'e')
      count += graph_SuccCount (XEXP (x, i));
    else if (fmt[i] == 'E')
    {
      register int j;
      for (j = 0; j < XVECLEN (x, i); j++)
        count += graph_SuccCount (XVECEXP (x, i, j));
    }
  }

  return count;
}

/*
 * x must have only one successor, ignoring the fall thorugh case,
 * find the successor and return it 
 *
 * x must be the PATTERN of a JUMP_INSN.
 */
rtx
GRAPH_Successor(x)
rtx x;
{
  register RTX_CODE code = GET_CODE (x);
  register int i;
  register char *fmt;
  register rtx label = NULL;
  assert(graph_SuccCount(x) <= 1);

  if (code == LABEL_REF)
  {
    label = XEXP (x, 0);
    return label;
  }

  fmt = GET_RTX_FORMAT (code);
  for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
  {
    assert(fmt[i] != 'E');
    if (fmt[i] == 'e')
      if ((label = GRAPH_Successor (XEXP (x, i))) != 0)
        return label;
  }

  return NULL;
}

/*
 * Create the edge from pred to succ.  Add succ to pred's vector of
 * successors and add pred to succ's vector of predecessors.
 */
static void
graph_CreateEdge(pred, succ)
GRAPH_NODE pred;
GRAPH_NODE succ;
{
  pred->succs[pred->succ_count++] = succ;
  assert(succ->pred_size > succ->pred_count);
  succ->preds[succ->pred_count++] = pred;
}

/*
 * Allocate space for the vector of 'count' successors.  This routine should
 * only be called once for each graph node.
 */
static void
graph_AllocSuccs(node, count)
GRAPH_NODE node;
int count;
{
  assert(node->succs == NULL);
  if (count == 0)
    return;
  node->succs = (GRAPH_NODE *) xmalloc(count * sizeof(GRAPH_NODE));
}

/*
 * Allocate space for the vector of probabilities.  There is one probability
 * for each successor. This routine should only be called once for each
 * graph node.
 */
static void
graph_AllocProbs(node)
GRAPH_NODE node;
{
  assert(node->probs == NULL);
  if (node->succ_count == 0)
    return;
  node->probs = (double *) xmalloc(node->succ_count * sizeof(double));
}

/*
 * Allocate space for the vector of 'count' predecessors.  This routine may
 * be called more than once for a graph node whose corresponding rtx is
 * a CODE_LABEL.
 */
static pred_increment = 4;

static void
graph_AllocPreds(node, count)
GRAPH_NODE node;
int count;
{
  assert(node->insn);

  if (GET_CODE(node->insn) == CODE_LABEL)
  {
    if (node->pred_count + count < node->pred_size)
      return;

    if (pred_increment < count)
      pred_increment = count;

    if (node->preds)
    {
      node->preds = (GRAPH_NODE *)
                    xrealloc(node->preds,
                             (node->pred_size + pred_increment) *
                             sizeof(GRAPH_NODE));
    }
    else
    {
      node->preds = (GRAPH_NODE *)xmalloc(pred_increment * sizeof(GRAPH_NODE));
    }
    node->pred_size += pred_increment;
  }
  else
  {
    assert(node->preds == NULL);
    if (count == 0)
      return;
    node->preds = (GRAPH_NODE *) xmalloc(count * sizeof(GRAPH_NODE));
    node->pred_size = count;
  }
}

/*
 * Build up the loop information for the flow graph
 *
 * This will change when TMC gets the dominator stuff in.  Right now
 * the only loops that are recognized are those that are marked with
 * LOOP_BEG and LOOP_END notes.
 *
 */
static void
graph_InitLoops(f)
rtx f;
{
  rtx insn;
  int current_loop = -1;
  int next_loop    = -1;
  int loop_head    =  0;
  int loop_count;
  GRAPH_NODE node;
  int i;

  graph_loop_outerloop = (int *)
                         xmalloc((GRAPH_loop_count + 1) * sizeof(int));
  bzero(graph_loop_outerloop, (GRAPH_loop_count + 1) * sizeof(int));

  for (insn = f, loop_count = 0; insn; insn = NEXT_INSN(insn))
  {
    switch (GET_CODE(insn))
    {
      case NOTE:
      {
        switch (NOTE_LINE_NUMBER(insn))
        {
#if !defined(DEV_465)
          case NOTE_INSN_FUNCTION_ENTRY:
#else
          case NOTE_INSN_FUNCTION_BEG:
#endif
          {
            assert(current_loop == -1);
            node = GRAPH_nodes [INSN_UID(insn)];
            node->loop_number = current_loop;
          }
          break;

          case NOTE_INSN_LOOP_END:
          {
            /*
             * it may be possible, if the loop associated with
             * a loop node was deleted, for current_loop to
             * be -1, indicating were not really in a loop.
             * If this is the case we must not try to update
             * current loop.
             */
            if(!loop_head && current_loop != -1)
              current_loop = graph_loop_outerloop[current_loop];
            loop_head = 0;
          }
          break;

          case NOTE_INSN_LOOP_BEG:
          {
            loop_head = 1;
          }
          break;
        }
      }
      break;

      case JUMP_INSN:
      {
        node = GRAPH_nodes [INSN_UID(insn)];
        node->loop_number = current_loop;
      }
      break;

      case CODE_LABEL:
      {
        node = GRAPH_nodes [INSN_UID(insn)];
        if (loop_head) {
          next_loop++;
          graph_loop_outerloop[next_loop] = current_loop;
          current_loop = next_loop;
          loop_head = 0;
          node->loop_head = 1;
        }
        node->loop_number = current_loop;
      }
      break;

      case INSN:
      case CALL_INSN:
      {
        if ((node = GRAPH_nodes [INSN_UID(insn)]) != 0)
          node->loop_number = current_loop;
      }
      break;
    }
  }

  /**
  ***  At this point all loop_headers have been marked and each node's
  ***  loop number has been noted.
  ***
  ***  Now find and mark all loop entrances, loop exits and loop
  ***  backedges.
  **/ 
  for (node = GRAPH_packed_nodes, i = 0;  i < GRAPH_node_count; i++, node++)
  {
    int j;
    for (j = 0; j < node->succ_count; j++)
    {
      GRAPH_NODE succ = node->succs[j];

      if (node->loop_number == succ->loop_number)
      {
        /*
         *  Look for a backedge
         */
        if (succ->loop_head)
        {
          assert(graph_loop_id[INSN_UID(node->insn)] >
                 graph_loop_id[INSN_UID(succ->insn)]);
          assert(node->loop_number >= 0);
          node->loop_backedge = 1;
        }
      }
      else
      {
        /*
         *  Look for entrances and exits
         *
         *  node->loop_number != succ->loop_number
         */
        int k;
 
        if (node->loop_number == -1)
          node->loop_entrance = 1;
        else if (succ->loop_number == -1)
          node->loop_exit = 1;
        else
        {
          /*
           * If we are going from an outer loop to an
           * inner loop this is a loop entrance
           *
           * If we are going from an inner loop to an
           * outer loop this is a loop exit
           *
           * NOTE: because of the way the RTL is currently
           * laid out inner loop numbers must be larger than
           * outer loop numbers.
           */
          int dest;
          int inner_loop = succ->loop_number;
          int outer_loop = node->loop_number;
          if (node->loop_number < succ->loop_number)
          {
            inner_loop = succ->loop_number;
            outer_loop = node->loop_number;
          }
          else
          {
            inner_loop = node->loop_number;
            outer_loop = succ->loop_number;
          }
          assert(inner_loop >= -1 && inner_loop < GRAPH_loop_count); 
          assert(outer_loop >= -1 && outer_loop < GRAPH_loop_count); 
          for (dest = inner_loop; dest != -1 && dest != outer_loop;
               dest = graph_loop_outerloop[dest]);
          {
            /*
             * Are we branching between two loops?  If so we
             * are exiting the current loop and entering
             * the destination loop.
             */
            if (dest == -1)
            {
              node->loop_exit = 1;
              node->loop_entrance = 1;
            }
            else if (node->loop_number < succ->loop_number)
            {
              /**
              *** outer to inner
              **/
              node->loop_entrance = 1;
            }
            else
            {
              /**
              *** inner to outer
              **/
              node->loop_exit = 1;
            }
          }
        }
      }
    }
  }
}

/*
 *  Debugging functions
 */
void
GRAPH_DebugGraph(file)
char *file;
{
  FILE *fd;

  if (!file) file = "graph.dump";
  fd = fopen(file, "w");
  assert(fd);
  GRAPH_PrintGraph(fd);
  fclose(fd);
}

void
GRAPH_PrintGraph(outf)
FILE *outf;
{
  int i,j;
  rtx insn;

  if (!outf) outf = stderr;
  GRAPH_search_mark++;

#ifndef GCC20
  print_set_outfile(outf);
#endif
 
  for (insn = get_insns(); insn; insn = NEXT_INSN(insn))
  {
    enum rtx_code code = GET_CODE(insn);

    if (INSN_UID(insn) < GRAPH_node_count &&
        (code == INSN || code == JUMP_INSN ||
         code == CODE_LABEL || code == CALL_INSN))
    {
      graph_PrintNode(GRAPH_nodes [INSN_UID(insn)], outf);
    }
    else
    {
#ifdef GCC20
      fprint_rtx(outf,insn);
#else
      print_rtx(insn);
#endif
      fprintf(outf, "\n");
    }
  }
}

void
GRAPH_DebugNode(node)
GRAPH_NODE node;
{
  graph_PrintNode(node, 0);
}

static void
graph_PrintNode(node, outf)
GRAPH_NODE node;
FILE *outf;
{
  int j;

  if (!outf) outf = stderr;

#ifdef GCC20
  fprint_rtx(outf, node->insn);
#else
  print_rtx(node->insn);
#endif
  fprintf(outf, "\n");

  fprintf(outf, "PREDS = ");
  for (j = 0; j <node->pred_count; j++)
  {
    fprintf(outf, "%d ", INSN_UID(node->preds[j]->insn));
  }
  fprintf(outf, "\n");

  fprintf(outf, "SUCCS = ");
  for (j = 0; j <node->succ_count; j++)
  {
    fprintf(outf, "%d ", INSN_UID(node->succs[j]->insn));
  }
  fprintf(outf, "\n");
  fprintf(outf, "PROBS = ");
  for (j = 0; j <node->succ_count; j++)
  {
    fprintf(outf, "%f ", node->probs[j]);
  }
  fprintf(outf, "\n");

  fprintf(outf, "LOOP %d\n", node->loop_number);
  fprintf(outf, "EDGE ", node->loop_number);
  if (node->loop_head)
    fprintf(outf, "HEAD ");
  if (node->loop_entrance)
    fprintf(outf, "ENTRANCE ");
  if (node->loop_exit)
    fprintf(outf, "EXIT ");
  if (node->loop_backedge)
    fprintf(outf, "BACKEDGE ");
  fprintf(outf, "\n");

  fprintf(outf, "EXPECT %f\n", node->expect);
  fprintf(outf, "SBLOCK %i\n", node->sblock_num);
}
#endif
