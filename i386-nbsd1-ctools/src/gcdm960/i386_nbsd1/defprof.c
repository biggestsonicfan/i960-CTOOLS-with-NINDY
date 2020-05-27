/******************************************************************/
/*       Copyright (c) 1990-1995 Intel Corporation

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

#include "assert.h"
#include "cc_info.h"
#include "gcdm960.h"
#include "callgrph.h"
#include "cgprof.h"

/*
 * Design of default profile generation.
 *
 * 1.  Default profile generation will be handled solely by gcdm960.
 *     No other program builds the call graph, and there is no real
 *     reason to save a default profile, you wouldn't want to stretch
 *     it, and it can always be regenerated.
 * 
 * 2.  So gcdm960 will read in the data base, and build a call graph
 *     as it normally does.  In addition, it will obtain any real/or
 *     default profiles that exist in the data base, and annotate the
 *     call graph with this profile information.  In doing so, gcdm960
 *     will note for each function, whether the profile information
 *     associated with it is real, or default.  For the purposes of
 *     this discussion it is assumed that any stretched profile is
 *     real.
 *
 * 3.  Creation of a default profile is really just attempting to
 *     estimate the function entry count for any function that we
 *     don't have that information on.  The rest of a functions
 *     profile will just be considered to be scaled by the function
 *     entry count.
 *
 * 4.  So we will create table of all functions which have "default"
 *     profile information.  This table of functions is the set of
 *     functions which we must estimate a function entry count for.
 *     If we consider each function to have a call height which is
 *     the maximum number of arcs in the call graph that must be traversed
 *     in the call graph in order to reach a leaf function, not counting
 *     recursion cycles in the callgraph, then visting the functions
 *     from higheset to lowest call-height number is a breadth first
 *     walk of the call graph visiting only those functions which
 *     need to have default profiles generated.
 *
 * 5.  When a function is visited as in 4 its function call entry is
 *     estimated by summing the weights of all its incoming arcs call
 *     arcs.  This becomes the estimated function entry count.  Then
 *     the profile information for the function is scaled by the
 *     estimated function entry count, and all the outgoing call arcs
 *     are updated to reflect this scaling.  Since the nodes are being
 *     visited according to call-height, all incoming arcs, except those
 *     that are a recursion cycle will have weights.  Any recursion cycle
 *     arcs must not be summed as an incoming arc.  Any function that
 *     has an incoming recursion arc will simply be given a multiplier
 *     much as a loop would be given a loop count for estimation
 *     purposes.  When all functions have been visited, the the call graph
 *     has essentially been weighted with the default profile.
 *     This default profile must be put back into each function in the
 *     second pass data base, so that SRAM variable allocation, and
 *     the second pass compilations will use the same information.
 */

static int cmp_def();
static double estimate_call_count();
static void scale_profile();

void
gen_default_profile(cg)
call_graph *cg;
{
  call_graph_node ** need_def_prof = 0;
  int n_need_prof = 0;
  int i = 0;

  /*
   * first find all functions in data base that need a default profile
   * generated.
   */
  if (cg->num_cg_nodes <= 0)
    return;

  need_def_prof = (call_graph_node **)db_malloc(cg->num_cg_nodes *
                                               sizeof(call_graph_node *));
  n_need_prof = 0;

  for (i = 0; i < cg->num_cg_nodes; i++)
  {
    st_node *st_p = cg->cg_nodes[i].sym;

    if (st_p != 0 && CI_ISFDEF(st_p->rec_typ) && fdef_prof_quality(st_p) <= 1)
    {
      need_def_prof[n_need_prof] = &cg->cg_nodes[i];
      n_need_prof += 1;
    }
  }

  if (n_need_prof == 0)
    return;

  /*
   * now sort the functions needing default profiles in descending order
   * according to call height.
   */
  qsort(need_def_prof, n_need_prof, sizeof(call_graph_node *), cmp_def);


  /*
   * now functions should be sorted in descending order by call height.
   * visit each function estimating the functions call count and then
   * scaling the rest of the functions profile information by the
   * estimated call count.
   */
  for (i = 0; i < n_need_prof; i++)
  {
    double call_estimate = estimate_call_count(cg, need_def_prof[i]);

    scale_profile(call_estimate, need_def_prof[i]);
  }

  /* free up data structures */
  free(need_def_prof);
}

static int
cmp_def(p1, p2)
call_graph_node **p1;
call_graph_node **p2;
{
  if ((*p1)->node_height > (*p2)->node_height)
    return -1;

  if ((*p1)->node_height < (*p2)->node_height)
    return 1;

  if (p1 > p2)
    return 1;

  if (p1 < p2)
    return -1;
  
  return 0;
}

static double
estimate_call_count(cg, cgn_p)
call_graph *cg;
call_graph_node *cgn_p;
{
  double estimate_val = 0.0;
  call_graph_arc * cga_p;

  /* sum the counts of all the incoming arcs which are not back edges */
  for (cga_p = cgn_p->call_in_list; cga_p != 0; cga_p = cga_p->in_arc_next)
  {
    if (cga_p->is_back_arc == 0)
      estimate_val  = d_to_d(estimate_val + cga_p->arc_count);
  }

  /* If this is main, assume 1 + calls */
  if (cg->main_node == cgn_p)
    estimate_val = d_to_d(1.0 + estimate_val);

  /*
   * just assume that if its address was taken that it will be called
   * an extra 50 times, unless this is main.
   */
  if (cgn_p->addr_taken && cg->main_node != cgn_p)
    estimate_val = d_to_d(estimate_val + 50.0);

  /*
   * If this is the head of some recursion loop, then multiply the count.
   * We use 3.0 as an average recusion count.
   */
  if (cgn_p->head_of_cycle)
    estimate_val = d_to_d(estimate_val * 3.0);

  return estimate_val;
}

static void
scale_profile(call_estimate, cgn_p)
double call_estimate;
call_graph_node *cgn_p;
{
  unsigned char * f_prof;
  double scale;
  double count;
  unsigned long cnt;
  int n_counts;
  int i;

  assert (cgn_p->sym != 0);
  assert (CI_ISFDEF(cgn_p->sym->rec_typ));

  /*
   * first run through the profile counters and update them all to
   * reflect the new call count.
   */
  f_prof = CI_LIST_TEXT (cgn_p->sym, CI_FDEF_PROF_LIST);
  CI_U32_FM_BUF(f_prof-4, n_counts);
  n_counts -= 4;
  n_counts >>= 2;

  CI_U32_FM_BUF(f_prof, cnt);
  count = cnt;
  call_estimate = d_to_d(call_estimate * 100);
  scale = d_to_d(call_estimate / count);

  for (i = 0; i < n_counts; i++)
  {
    CI_U32_FM_BUF(f_prof, cnt);
    count = cnt;
    count = d_to_d(count * scale);
    cnt = count;
    CI_U32_TO_BUF(f_prof, cnt);

    f_prof += 4;
  }

  /*
   * now incorporate this new profile information into the
   * call-graph for this particular function.
   */
  get_func_prof(cgn_p);
}
