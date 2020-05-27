/******************************************************************/
/*       Copyright (c) 1990,1991,1992,1993 Intel Corporation

   Intel hereby grants you permission to copy, modify, and 
   distribute this software and its documentation.  Intel grants
   this permission provided that the above copyright notice 
   appears in all copies and that both the copyright notice and
   this permission notice appear in supporting documentation.  In
   addition, Intel grants this permission provided that you
   prominently mark as not part of the original any modifications
   made to this software or documentation, and that the name of 
   Intel Corporation not be used in advertising or publicity 
   pertaining to distribution of the software or the documentation 
   without specific, written prior permission.  

   Intel Corporation provides this AS IS, without any warranty,
   including the warranty of merchantability or fitness for a
   particular purpose, and makes no guarantee or representations
   regarding the use of, or the results of the use of, the software 
   and documentation in terms of correctness, accuracy, reliability, 
   currentness, or otherwise; and you rely on the software, 
   documentation and results solely at your own risk.
 */
/******************************************************************/

#include "cc_info.h"
#include "callgrph.h"
#include "gcdm960.h"
#include "subst.h"
#include "assert.h"

static int
arc_cmp(arc1_p, arc2_p)
call_graph_arc **arc1_p;
call_graph_arc **arc2_p;
{
  /*
   * return -1 if arc1's priority is greater than arc2s priority,
   * 0 if their the same, 1 is arc1's priority is greater.  This has
   * the effect of sorting the array with highest priority first.
   */

  if ((*arc1_p)->call_to->node_height < (*arc2_p)->call_to->node_height)
    return (-1);
  if ((*arc1_p)->call_to->node_height > (*arc2_p)->call_to->node_height)
    return (1);

  /*
   * If the call_fm is the same node, then make sure that any recursive
   * calls from this node are prioritized lower than the non-recursive
   * calls.  This makes
   * the sizing and tot_inlinable call calculations come out correctly.
   */
  if ((*arc1_p)->call_fm == (*arc2_p)->call_fm)
  {
    /* arc1 has higher priority if its not recursive, but arc2 is. */
    if ((*arc1_p)->call_fm != (*arc1_p)->call_to &&
        (*arc2_p)->call_fm == (*arc2_p)->call_to)
      return -1;

    /* arc2 has higher priority if its not recursive, but arc1 is. */
    if ((*arc1_p)->call_fm == (*arc1_p)->call_to &&
        (*arc2_p)->call_fm != (*arc2_p)->call_to)
      return 1;
  }

  /*
   * Sort based on the arc count information.  Arcs with higher count
   * get higher priority.
   */

  if ((*arc1_p)->arc_count > (*arc2_p)->arc_count)
    return (-1);
  if ((*arc1_p)->arc_count < (*arc2_p)->arc_count)
    return (1);

  /*
   * This last bit just guarantees no randomness due to hosts qsort
   * implementation.
   */
  if ((*arc1_p)->arc_id < (*arc2_p)->arc_id)
    return -1;
  return 1;
}

static int
prioritize_arcs (cg, top_arcs)
call_graph *cg;
call_graph_arc **top_arcs;
{
  call_graph_arc *arc_p;
  call_graph_arc *quit;
  int num_candidate_arcs = 0;

  /*
   * initialize the top_arc array, only put entries into the array that
   * are actually inlinable, and are not directly recursive.
   */
  arc_p = cg->cg_arcs;
  quit = &arc_p[cg->num_cg_arcs];
  for (; arc_p < quit; arc_p++)
  {

    /*
     * The node height requirement guarantees that no backwards call arcs
     * are inlined. Recursive routines are not considered for inlining
     * here.  Instead after all inlining for a routine has been done, the
     * compiler may take it upon itself to inline a recursive routine into
     * itself. Thus we are guaranteed that all inlining decisions made
     * here constitute a tree. 
     *
     * We don't actually restrict directly recursive routines from being
     * inlinable here, we just treat them differently in mark_inlinable.
     */
    if (arc_p->inlinable &&
        arc_p->call_to->node_height <= arc_p->call_fm->node_height)
    {
      if (arc_p->arc_count > 0.1 ||
          (arc_p->call_to->node_insns < 6 && arc_p->call_to->n_calls_out == 0))
      {
        top_arcs[num_candidate_arcs++] = arc_p;
        arc_p->inlinable = 1;
      }
      else
        arc_p->inlinable = 0;
    }
    else
      arc_p->inlinable = 0;
  }

  qsort(top_arcs, num_candidate_arcs, sizeof(call_graph_arc *), arc_cmp);

  return (num_candidate_arcs);
}

static int tot_inlns(node_p)
register call_graph_node *node_p;
{
  int d_inlns = node_p->inln_calls;
  int r_inlns = node_p->recur_inln_calls;

  /* d_inlns is the number of direct inlines from a routine.
     r_inlns is the number of directly recursive inlines from a routine
     to itself.

     (d_inlns * r_inlns) + d_inlns + r_inlns

     is the formula fro deterimining the total number of routines that
     should be inlined by this routine. */

  return ((d_inlns * r_inlns) + d_inlns + r_inlns);
}

static void
mark_inlinable (inline_level, cg, top_arcs, num_arcs)
int inline_level;
call_graph * cg;
call_graph_arc ** top_arcs;
int num_arcs;
{
  int function_max_size;
  int function_ideal_size;
  int reg_use_max;

  call_graph_node * node_p;
  call_graph_node * node_quit;
  int i;

  switch (inline_level)
  {
    case 0:
      function_max_size = 0;
      function_ideal_size = 0;
      reg_use_max = 0;
      break;

    default:
    case 1:
      function_max_size = 1000;
      function_ideal_size = 250;
      reg_use_max = 28;
      break;

    case 2:
      function_max_size = 1500;
      function_ideal_size = 400;
      reg_use_max = 28;
      break;

    case 3:
      function_max_size = 2000;
      function_ideal_size = 600;
      reg_use_max = 30;
      break;

    case 4:
      function_max_size = 2300;
      function_ideal_size = 800;
      reg_use_max = 35;
      break;
  }

  /*
   * first mark all nodes as no longer inlinable.  We will use
   * this as a flag and set it to true when we actually make a
   * decision to inline an arc that points to this node.
   * Leave any routines that are directly recursive marked as
   * inlinable, so that the compiler can decide to to inlining
   * or not.
   */
  node_p = cg->cg_nodes;
  node_quit = &node_p[cg->num_cg_nodes];
  for (; node_p < node_quit; node_p++)
    node_p->inlinable = 0;

  for (i = 0; i < num_arcs; i++)
  {
    /*
     * based on size of routine being inlined and size of routine being
     * inlined compute whether this routine should be inlined.  Also
     * compute approximation of new size of routine being inlined into
     * after the inlining decision is made.
     */
    call_graph_arc *arc_p = top_arcs[i];
    int callee_size;
    int caller_size;
    int callee_parms;
    int arc_count;
    int directly_recursive;

    caller_size = arc_p->call_fm->node_insns;
    callee_size = arc_p->call_to->node_insns;
    callee_parms = arc_p->call_to->node_parms;

    caller_size -= 2 * callee_parms + 2;  /* get rid of call overhead */

    /*
     * get rid of call overhead on callee side.
     * two insns per param plus approximately 5 extra just for stuff.
     */
    callee_size -= 2 * callee_parms + 5;
    if (callee_size < 0)
      callee_size = 0;

    arc_count = cg->call_count
		? d_to_d(d_to_d(arc_p->arc_count * 100000) / cg->call_count)
		: 0;
    if (caller_size + callee_size < function_max_size)
    { int deletable;
      directly_recursive = arc_p->call_to == arc_p->call_fm;
      if (directly_recursive)
        arc_count >>= 1;
      deletable = arc_p->call_to->deletable &&
                  arc_p->call_to->n_calls_in == 1 &&
                  arc_p->call_fm->is_recomp &&
                  !directly_recursive;

      arc_p->inlinable = 0;
      /*
       * If the routine is deletable after being
       * inlined here then there is essentially no cost for inlining it,
       * and a virtually guaranteed increase in performance.
       */
      if (arc_p->regs_live + arc_p->call_to->n_regs_aft <= reg_use_max)
      {
        if (deletable)
          arc_p->inlinable = 1;
        else
        {
          /*
           * Just decide whether this is a good thing based on the size
           * of the caller, callee, and number of params.
           */
          if (arc_count > 100)
          {
            /* this arc is called more than .1% of the total calls in the
               program, inline it unless cost would be high. */
            if (!directly_recursive)
            {
              if (callee_size < function_ideal_size ||
                  !arc_p->call_to->head_of_cycle)
                arc_p->inlinable = 1;
            }
            else
            {
              if (callee_size < function_ideal_size / 4)
                arc_p->inlinable = 1;
            }
          }
          else if (arc_count > 10)
          {
            /* this arc is called between .01% and .1% of the total calls in the
             * program. Inline it if its relatively cheap. */
            if (!arc_p->call_to->head_of_cycle)
            {
              if (callee_size < function_ideal_size / 2)
                arc_p->inlinable = 1;
              else if (callee_size + caller_size < function_ideal_size)
                arc_p->inlinable = 1;
            }
          }
          else
          {
            if (!arc_p->call_to->head_of_cycle)
            {
              if (callee_size < function_ideal_size / 25)
                arc_p->inlinable = 1;
            }
          }
        }
      }

      if (arc_p->inlinable)
      {
        int new_reg_use;
        extern int num_inlined_arcs;

        num_inlined_arcs++;

        /* compute new size of arc doing the calling */
        arc_p->call_fm->node_insns = caller_size + callee_size;
 
        /* note the called node as being inlined. */
        arc_p->call_to->inlinable = 1;  /* just bolean now */

        /*
         * can't really say there are one fewer calls if the function
         * is directly recirsive or if the arc being inlined is in a library
         * which might not be recompiled.
         */
        if (!directly_recursive && arc_p->call_fm->is_recomp)
          arc_p->call_to->n_calls_in -= 1;  /* Essentially one fewer calls */

        /* update the number of inlinable calls from the caller */
        if (arc_p->call_fm == arc_p->call_to)
          arc_p->call_fm->recur_inln_calls += 1;
        else
          /*
           * Don't use tot_inlns here because no recursive inlines will be
           * expanded in this inlining, therefore only the non-recursive
           * inlines go into the count.
           */
          arc_p->call_fm->inln_calls += arc_p->call_to->inln_calls + 1;

        new_reg_use = arc_p->call_fm->n_regs + arc_p->call_to->n_regs_aft;
        if (new_reg_use > arc_p->call_fm->n_regs_aft)
          arc_p->call_fm->n_regs_aft = new_reg_use;
      }
    }
    else
      arc_p->inlinable = 0;
  }

  /*
   * update the data base to reflect all the inlining decisions
   * that were made.
   */
  for (node_p = cg->cg_nodes; node_p < node_quit; node_p++)
  {
    int j;
    call_graph_arc *arc_p;

    if (node_p->n_calls_in > 0)
      node_p->deletable = 0;

    if (node_p->deletable)
      db_fdef_set_can_delete (node_p->sym);

    if (tot_inlns(node_p) > 0)
    {
      db_rec_make_tot_inline_calls (node_p->sym, tot_inlns(node_p));

      /* mark the inlinable call arcs */
      arc_p = node_p->call_out_list;
      j = 0;
      for (arc_p = node_p->call_out_list, j = 0;
           arc_p != 0;
           arc_p = arc_p->out_arc_next, j++)
      {
        if (arc_p->inlinable)
          db_rec_make_arc_inlinable(node_p->sym, j,
                                    arc_p->call_to == arc_p->call_fm);
      }
    }
  }

  if (flag_print_summary)
  {
    int n_inlined = 0;

    for (i = 0; i < num_arcs; i++)
    {
      if (top_arcs[i]->inlinable)
        n_inlined += 1;
    }
    fprintf (decision_file.fd, "%d function call-sites were inlined.\n", n_inlined);
  }


  if (flag_print_decisions)
  {
    int lines;
    int n_inlined = 0;

    fprintf (decision_file.fd, "\nInlining Decisions\n\n");
    fprintf (decision_file.fd, 
"Inlined Arc                             Site Percent Deletable Call Height\n");
    fprintf (decision_file.fd,
"=======================================|====|=======|=========|===========\n");
    fprintf (decision_file.fd, "\n");
    lines = 6;

    for (i = 0; i < num_arcs; i++)
    {
      if (top_arcs[i]->inlinable)
      {
	double	cnt = cg->call_count
		? d_to_d(d_to_d(top_arcs[i]->arc_count * 100) / cg->call_count)
		: 0 ;

        if (lines > db_page_size())
        {
          fprintf (decision_file.fd, "\nInlining Decisions\n\n");
          fprintf (decision_file.fd, 
"Inlined Arc                             Site Percent Deletable Call Height\n");
          fprintf (decision_file.fd,
"=======================================|====|=======|=========|===========\n");
          fprintf (decision_file.fd, "\n");
          lines = 6;
        }

        fprintf (decision_file.fd, "%-20.20s->%-17.17s %4d %6.2f%%     %c    %8d\n",
                 top_arcs[i]->call_fm->sym->name,
                 top_arcs[i]->call_to->sym->name,
                 top_arcs[i]->site_id,
		 cnt,
                 top_arcs[i]->call_to->deletable ? 'Y' : 'N',
                 top_arcs[i]->call_to->node_height);
        lines++;
        n_inlined += 1;
      }
    }

    fprintf (decision_file.fd, "%d Arcs were inlined\n", n_inlined);
  }
}

void
old_make_glob_inline_decisions(cg, inline_level)
call_graph * cg;   /* pointer to call graph data structure */
int inline_level;
{
  call_graph_arc **arc_priority;
  arc_priority = (call_graph_arc **)
                 db_malloc((cg->num_cg_arcs > 0) /* Avoid malloc(0) */
			? cg->num_cg_arcs*sizeof(call_graph_arc *)
			: sizeof(call_graph_arc *));
  
  mark_inlinable (inline_level, cg, arc_priority,
                  prioritize_arcs(cg, arc_priority));
  free (arc_priority);
}

int
function_cost (n, psize, pregs, size_lim, regs_lim)
call_graph_node *n;
int *psize;
int *pregs;
{
  /* Calculate the cumulative size and worst case live register usage
     for function 'n' and all of the functions inlined into it. */

  call_graph_arc *a = n->call_out_list;

  int size = get_target (n->sym, TS_INL);
  int regs = n->n_regs;
  int recursions = 1;

  while (a && size <= size_lim && regs <= regs_lim)
  {
    if (a->inlinable > 1)
    { /* 'a' has been inlined.  */

      int tsize = 0;
      int tregs = 0;

      if (a->call_fm == a->call_to)
        recursions++;
      else
        if(!function_cost(a->call_to, &tsize, &tregs,
                          size_lim-size, regs_lim-a->regs_live))
          return 0;

      if ((a->regs_live + tregs) > regs)
        regs = a->regs_live + tregs;

      size += tsize;
    }
    a = a->out_arc_next;
  }

  size *= recursions;
  assert (size);

  if (size > size_lim || regs > regs_lim)
    return 0;

  *psize = size;
  *pregs = regs;

  return 1;
}

int max_size = 6000;
int max_regs = 28;

static int
total_inline_cost (n, psize, pregs)
call_graph_node *n;
int *psize;
int *pregs;
{
  /* Calculate the cumulative size and worst case register usage of all
     functions which inline 'n', including other functions inlined into
     the callers of 'n'.  Basically, for all callers we walk back until
     we're not being inlined, and then we call 'function_cost'. */

  call_graph_arc *a = n->call_in_list;

  int size = 0;
  int regs = 0;

  while (a && size <= max_size && regs <= max_regs)
  {
    if (a->inlinable > 1 && a->call_fm != a->call_to)
    { int tsize = 0;
      int tregs = 0;

      if (!total_inline_cost (a->call_fm, &tsize, &tregs))
        return 0;

      assert (tsize);

      size += tsize;

      if (tregs > regs)
        regs = tregs;
    }

    a = a->in_arc_next;
  }

  if (size == 0)
  { 
    /* The root of an inline true must always be recompilable;
       we enforce this by never inlining a non-recompilable
       routine into another routine until the calling routine
       is inlined. */

    assert (n->is_recomp);
    if (!function_cost (n, &size, &regs, max_size, max_regs))
      return 0;
  }

  assert (size);

  if (size > max_size || regs > max_regs)
    return 0;

  *psize = size;
  *pregs = regs;

  return 1;
}

static int
node_size_cmp(n1_p, n2_p)
call_graph_node **n1_p;
call_graph_node **n2_p;
{
  call_graph_node* n1 = *n1_p;
  call_graph_node* n2 = *n2_p;

  int t = get_fdef_size (n1->sym) - get_fdef_size (n2->sym);

  if (t > 0)
    return -1;
  else if (t < 0)
    return 1;

  return 0;
}

static int
arc_freq_cmp(arc1_p, arc2_p)
call_graph_arc **arc1_p;
call_graph_arc **arc2_p;
{
  call_graph_arc *a1 = *arc1_p;
  call_graph_arc *a2 = *arc2_p;

  /* Prefer more frequently executed arcs */
  if (a1->arc_count > a2->arc_count)
    return (-1);
  if (a1->arc_count < a2->arc_count)
    return (1);

  /* Prefer leaves */
  if (a1->call_to->node_height < a2->call_to->node_height)
    return (-1);
  if (a1->call_to->node_height > a2->call_to->node_height)
    return (1);

  /* Prefer non-recursive arcs */
  if (a1->call_fm != a1->call_to &&
      a2->call_fm == a2->call_to)
    return -1;

  if (a1->call_fm == a1->call_to &&
      a2->call_fm != a2->call_to)
    return 1;

  if (a1->arc_id < a2->arc_id)
    return -1;

  return 1;
}

int
is_better_arc (cand, arc, space_remaining, calls_remaining)
call_graph_arc *cand;
call_graph_arc *arc;
int space_remaining;
double calls_remaining;
{
  /* *cand is a lower frequency, lower cost arc than *arc.
     Return true iff it seems like a good idea to inline *cand instead of
     *arc. */

  double cost_ratio, freq_ratio;

  assert (cand->cost < arc->cost);
  assert (cand->arc_count <= arc->arc_count);

  cost_ratio = ((double) arc->cost)      / ((double) cand->cost);
  freq_ratio = ((double) arc->arc_count) / ((double) cand->arc_count);

  assert ((cost_ratio >= 1) && (freq_ratio >= 1));
  return (cost_ratio > freq_ratio);
}

static void
mark_non_deletable(n)
call_graph_node *n;
{
  call_graph_arc *a;

  /* check if this changes the deletability of any of the nodes
     that this node connects to.  */

  for (a = n->call_out_list; a != 0; a = a->out_arc_next)
    if (a->call_to == n)
      a->call_to->deletable = 0;

    else if (a->call_to && a->call_to->deletable)
    { 
      if (a->inlinable < 2)
      { assert (a->call_to->deletable != 2);
        a->call_to->deletable = 0;
      }
      mark_non_deletable (a->call_to);
    }
}

void
delete_dead_cg_nodes(cg)
call_graph *cg;
{
  /* We want to get the space back for unconnected routines.  We've done
     some inlining and perhaps caused some nodes to become
     unconnected, or we haven't started yet and we just want to get the
     space back for the totally unused ones.

     First, mark all non-addr taken nodes not yet actually deleted as
     'deletable'.

     Then, run the call graph and mark every node that is
     connected to an addr_taken node as non-deletable.  Finally,
     'delete' the deletable nodes and retrieve their space.

     Note that any nodes which have to hang around (e.g, nodes which are
     in library files which won't be recompiled) have been marked as
     addr_taken already.
   */

  call_graph_node *nstop = cg->cg_nodes + cg->num_cg_nodes;
  call_graph_node *n;

  for (n = cg->cg_nodes; n != nstop; n++)
    if (n->addr_taken == 0 && n->deletable != 2)
      n->deletable = 1;

  for (n = cg->cg_nodes; n != nstop; n++)
    if (n->addr_taken)
    { assert (n->deletable == 0);
      mark_non_deletable(n);
    }

  for (n = cg->cg_nodes; n != nstop; n++)
    if (n->deletable == 1)
    { int t, sz, ru;
      n->deletable = 2;
      t = function_cost (n, &sz, &ru, 0x7fffffff, 0x7fffffff);
      assert (t);
      ts_grp(n->sym)->grp_ts += sz;
    }
}

void
prune_inline_arcs (cg)
call_graph *cg;
{
  call_graph_arc *astop = cg->cg_arcs + cg->num_cg_arcs;
  call_graph_arc *arc;

  /* Mark all backward call arcs and all arcs going between ts groups
     as non-inlinable */

  for (arc=cg->cg_arcs; arc != astop; arc++)
    if (arc->inlinable)
      if (arc->call_to->node_height > arc->call_fm->node_height ||
          arc->call_to->sym == 0 ||
          ts_grp(arc->call_to->sym) != ts_grp(arc->call_fm->sym))
        arc->inlinable = 0;
}

void
make_glob_inline_decisions(cg, inline_level)
call_graph * cg;
int inline_level;
{
  extern int num_inlined_arcs;

  int narcs = cg->num_cg_arcs;
  int nnodes = cg->num_cg_nodes;

  call_graph_arc *astop = cg->cg_arcs + narcs;
  call_graph_arc *arc;

  call_graph_node *nstop = cg->cg_nodes + nnodes;
  call_graph_node *node;

  call_graph_arc**  pvec=(call_graph_arc**)db_malloc((narcs+1)*sizeof(*pvec));

  module_name *ts = inline_level ? mname_head[mn_size] : 0;

  FILE* f = (flag_print_summary || flag_print_decisions) ? decision_file.fd : 0;

  num_inlined_arcs = 0;

  /* Loop thru all ts groups, inlining only within the groups.  Normally,
     there will be only 1 group.  If there is more than one, each is
     a self-contained set of inlines. */

  while (ts != 0)
  {
    call_graph_arc** parc = pvec;
    call_graph_arc** pstop = pvec;

    int tot_insns = 0;
    int tot_bytes[NUM_TS];
    int i;

    memset (tot_bytes, 0, sizeof (tot_bytes));

    /* Get all inlinable arcs for this ts */
    for (arc=cg->cg_arcs; arc != astop; arc++)
      if (arc->inlinable==1 && ts == ts_grp(arc->call_to->sym))
        *pstop++ = arc;

    /* Sort the arc priorities for this ts by frequency */
    qsort (parc, pstop-parc, sizeof(call_graph_arc *), arc_freq_cmp);

    for (node=cg->cg_nodes; node != nstop; node++)
      if (node->inlinable && ts_grp(node->sym) == ts)
      { tot_insns += node->node_insns;
        for (i = 0; i < NUM_TS; i++)
          tot_bytes[i] += get_target (node->sym, i);
        node->inlinable = 0;
      }

#ifdef DEBUG
    if (f)
    {
      fprintf (f, "%d initial insns in ts group %d\n", tot_insns, ts->id);

      for (i = 0; i < NUM_TS; i++)
        fprintf (f,"%d pass %d bytes in ts group %d\n", tot_bytes[i],i,ts->id);
    }
#endif

    max_size = db_size_ratio (tot_bytes[TS_INL], tot_insns, 2000);

#ifdef DEBUG
    if (f)
      fprintf (f, "limiting routines in ts group %d to %d pass %d bytes\n",
                   ts->id, max_size, TS_INL);
#endif

    while (1)
    {
      call_graph_arc **pcur, **pf, **pl;
      int regs, sz_before, sz_after, tot_cost, num_ok;
      double tot_calls;

  
      /* This loop finds one or more inline candidates, and totals the
         hits of all remaining inlinable arcs.  Additional arcs
         found after the first take increasingly less room to inline.

         We will pick an arc after the loop is done, based on
         cost/benefit. */

      pf =  pl  = 0;
      pcur      = parc;
      num_ok    = 0;
      tot_calls = 0.0;
      tot_cost  = 0;

      while (1)
      { 
        while (pstop > parc && pstop[-1]->inlinable != 1)
          pstop--;
  
        while (parc < pstop && parc[0]->inlinable != 1)
          parc++;

        if (pcur >= pstop)
          break;

        if (num_ok > 9 || (pf && (*pf)->arc_count >= 2 * (*pcur)->arc_count))
          break;

        (*pcur)->cost = 0x7fffffff;

        if ((*pcur)->inlinable==1 &&

           /* When a routine will not be recompiled, any inlines of calls to or
              from that routine are allowed to happen only after after the
              caller is inlined.

              This makes the inlining happen "top down" in non-recompilable
              routines, and such inlines are always "real" in the sense that
              the immediate recompilable callers of a web of non-recompilable
              inlines will always instantiate the web (even though the
              actual function bodies of the non-recompilable routines will not
              have any inlining).

              Without this "top down" inlining for non-recompilable routines,
              our costing scheme breaks down because inline webs develop that
              would never be instantiated.  With the "top down" scheme, we
              can always accurately evaluate the cost of inlining a new
              arc by trying it and looking at the before/after size.
          */

           ((*pcur)->call_fm->inlinable ||
            ((*pcur)->call_fm->is_recomp && ((*pcur)->call_to->is_recomp))))
        {
          /* Get the cost without inlining this arc */
          if (!total_inline_cost ((*pcur)->call_fm, &sz_before, &regs))
            (*pcur)->inlinable = 0;
  
          else
          { (*pcur)->inlinable = 2;
  
            assert (sz_before > 0);

            /* Get the cost after this arc is inlined */
            if (!total_inline_cost ((*pcur)->call_fm, &sz_after, &regs))
              (*pcur)->inlinable = 0;
            else
            {
              (*pcur)->inlinable = 1;

              assert (sz_after > sz_before);
              sz_after -= sz_before;

              /* If this is a cheaper arc to inline than what we had, keep
                 it by marking it with a good cost.  Otherwise,
                 we already know the arc is slower than what we had, so
                 just toss it. */
                 
              if (sz_after <= ts->grp_ts && (pl==0 || (*pl)->cost > sz_after))
              { num_ok++;

                (*(pl=pcur))->cost = sz_after;
                tot_calls += (*pl)->arc_count;
                tot_cost += (*pl)->cost;

                if (pf == 0)
                  pf = pl;
              }
            }
          }
        }
        pcur++;
      }

      if (num_ok == 0)
        break;

      /* We found one or more good arcs.  Get the one with the best CBR. */

      assert (pf && pl);
      for (pcur=pf+1; pcur <= pl; pcur++)
        if ((*pcur)->inlinable==1 && (*pcur)->cost < (*pf)->cost)
          if (is_better_arc (*pcur, *pf, tot_cost, tot_calls))
            pf = pcur;

      ts->grp_ts -= (*pf)->cost;

      (*pf)->call_to->inlinable = 1;

      /* Remember the order we picked it in */
      (*pf)->inlinable = 2 + num_inlined_arcs++;

      delete_dead_cg_nodes (cg);
    }

    ts = ts->next;
  }

  /* working from lowest to highest call height, count inline calls */
  { int i = 1;
    int j = 1;

    for (; i<=j; i++)
      for (arc=cg->cg_arcs; arc != astop; arc++)
      { if (arc->call_to->node_height > j)
          j = arc->call_to->node_height;
  
        if (arc->inlinable > 1)
          if (arc->call_to->node_height == i)
            if (arc->call_fm == arc->call_to)
              arc->call_fm->recur_inln_calls += 1;
            else
    
              /* Don't use tot_inlns here because no recursive inlines will be
                 expanded in this inlining. */
              arc->call_fm->inln_calls += arc->call_to->inln_calls + 1;
      }
  }

  /* update the data base to reflect all the inlining decisions */
  for (node = cg->cg_nodes; node < nstop; node++)
    if (CI_ISFDEF (node->sym->rec_typ))
    { int sz, ru;

      if (node->deletable)
      { assert (node->deletable == 2);
        db_fdef_set_can_delete (node->sym);
        sz = 0;
      }
      else
        function_cost (node, &sz, &ru, 0x7fffffff, 0x7fffffff);
  
      /* save away the fully expanded size of the function. */
      set_fdef_size (node->sym, sz);
  
#if 0
      if (tot_inlns(node) > 0)
#endif
      { int i = 0;
        db_rec_make_tot_inline_calls (node->sym, tot_inlns(node));
  
        /* mark the inlinable call arcs */
        for ((arc=node->call_out_list),i=0; arc; (arc=arc->out_arc_next),i++)
          if (arc->inlinable > 1)
            db_rec_make_arc_inlinable (node->sym,i,arc->call_to==arc->call_fm);
          else
            arc->inlinable = 0;
      }
    }

  if (flag_print_decisions)
  {
    int lines, which, i, is_tot, was_tot;

    call_graph_node*  n;
    call_graph_node** nvec=
                      (call_graph_node**)db_malloc((nnodes+1)*sizeof(*nvec));

    for (node=cg->cg_nodes; node != nstop; node++)
      nvec[node-cg->cg_nodes] = node;

    qsort (nvec, nnodes, sizeof(call_graph_node *), node_size_cmp);

    is_tot  = 0;
    was_tot = 0;

    fprintf (f, "\n");
    fprintf (f, "%-43.43s before /  after global inlining\n",
    "Estimated code sizes for database functions");
    fprintf (f, "%-43.43s========/=======================\n",
    "===========================================");

    for (i = 0; i < nnodes; i++)
    { n = nvec[i];
      if (CI_ISFDEF (n->sym->rec_typ))
      { int is  = get_fdef_size (n->sym);
        int was = get_target (n->sym, TS_INL);

        fprintf (f, "%-43.43s %6d / %6d\n", n->sym->name, was, is);

        is_tot += is;
        was_tot += was;
      }
    }

    fprintf (f, "\n");
    fprintf (f, "%-43.43s %6d / %6d\n", "Code size for all database functions", was_tot, is_tot);

    fprintf (f, "\nInlined arcs:\n\n");
    fprintf (f, 
"Caller              [Site]->Callee                   Calls(Percent) Height\n");
    fprintf (f,
"===================================================================|======\n");
    fprintf (f, "\n");
    lines = 6;

    for (which=0; which < num_inlined_arcs; which++)
      for (arc=cg->cg_arcs; arc!=astop; arc++)
        /* Print them out in the order picked. */
        if (arc->inlinable == 2 + which)
        {
          double cnt = cg->call_count
  		? d_to_d(d_to_d(arc->arc_count * 100) / cg->call_count)
  		: 0 ;
  
          if (lines > db_page_size())
          {
            fprintf (f, "\nInlined arcs:\n\n");
    fprintf (f, 
"Caller              [Site]->Callee                   Calls(Percent) Height\n");
    fprintf (f,
"===================================================================|======\n");
            lines = 6;
          }
  
          fprintf (f, "%-20.20s[%04d]->%-17.17s %12.2f(%6.2f%%) %d\n",
                   arc->call_fm->sym->name, arc->site_id,
                   arc->call_to->sym->name,
                   (double) arc->arc_count, cnt,
                   arc->call_to->node_height);
          lines++;
        }
  }

  if (f)
    fprintf (f, "There were %d call-sites, of which %d were inlined.\n",
             narcs, num_inlined_arcs);
}
