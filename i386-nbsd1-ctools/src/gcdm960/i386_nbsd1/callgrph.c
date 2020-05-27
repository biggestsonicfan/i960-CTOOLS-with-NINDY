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

#include <stdio.h>
#include <string.h>
#include "cc_info.h"
#include "callgrph.h"
#include "assert.h"
#include "gcdm960.h"

extern int num_funcs;
extern dbase prog_db;

static call_graph cg;
static call_graph_node *node_avail;  /* used in allocating call graph nodes */

static int
compute_call_height(cgn_p, cga_p)
call_graph_node *cgn_p;
call_graph_arc  *cga_p;
{
  call_graph_arc *arc_p;
  int max_height = 0;

  /*
   * compute the call height outgoing arcs in a call
   * graph node, then for the call graph node.  return the
   * call height for the call graph node.
   */
  
  if (cgn_p == 0) /* unknown call */
    return 1;

  if (cgn_p->node_height > 0)
    return cgn_p->node_height;

  if (cgn_p->n_calls_out == 0)
  {
    cgn_p->node_height = 1;
    return 1;
  }

  if (cgn_p->node_height == -1)
  {
    /* we recursed in the call graph */
    /* mark this node as being at the head of some backwards edge */
    cgn_p->head_of_cycle = 1;
    if (cga_p != 0)
      cga_p->is_back_arc = 1;
    return 0;
  }

  /* compute the count for all the nodes on outgoing arcs */
  cgn_p->node_height = -1;
  for (arc_p = cgn_p->call_out_list; arc_p != 0; arc_p = arc_p->out_arc_next)
  {
    int t = compute_call_height(arc_p->call_to, arc_p);

    if (t > max_height)
      max_height = t;
  }

  cgn_p->node_height = max_height + 1;
  return (max_height + 1);
}

static void
check_call_target(t)
st_node* t;
{
  /* Make sure the thing we're calling is a function. It's
     too late to recover if it isn't */

  if (!CI_ISFUNC(t->rec_typ))
    if (CI_ISVAR(t->rec_typ))
      db_fatal ("call target '%s' is declared as a variable somewhere in your program", t->name);
    else
      db_fatal ("call target '%s' is not a function", t->name);
}

void
callee_check(p)
st_node *p;
{
  if (CI_ISFDEF(p->rec_typ))
  {
    int n_calls = db_rec_n_calls(p);
    char *call_vec = db_rec_call_vec(p);
    int j;

    cg.num_cg_arcs += n_calls;

    for (j = 0; j < n_calls; j++)
    {
      st_node * t;

      /* lookup the symbol pointed to by call_vec */
      if ((t = dbp_add_sym(&prog_db, call_vec, CI_FREF_REC_FIXED_SIZE)) == 0)
      { t = dbp_lookup_sym(&prog_db, call_vec);
        assert (t);
        check_call_target (t);
      }
      else
      { int ex = extra_size (CI_FREF_REC_TYP);
        t->is_static = 0;
        t->rec_typ = CI_FREF_REC_TYP;
        t->extra = db_malloc (ex);
        memset (t->extra, 0, ex);
        cg.num_cg_nodes += 1;
      }
      call_vec += strlen(call_vec)+1;
    }
  }
}

void
assign_cg_node(p)
st_node * p;
{
  if (CI_ISFDEF(p->rec_typ) || p->rec_typ == CI_FREF_REC_TYP)
  {
    call_graph_node *node;

    /* allocate a call graph node for the symbol and build it. */
    node = node_avail;
    assert (p->extra);
    CGN_P(p) = node;
    node->sym = p;
    node->n_calls_out = db_rec_n_calls(p);
    node->n_calls_in = 0;
    node->call_out_list = 0;
    node->call_in_list = 0;
    node->first_bb = -1;
    node->last_bb = -1;
    node->node_count = 0;
    node->node_height = 0;
    node->inln_calls = 0;
    node->recur_inln_calls = 0;
    node->node_insns = db_rec_n_insns(p);
    node->file_name  = (char *) 0;
    node->node_parms = db_rec_n_parms(p);
    node->n_regs = db_rec_n_regs_used(p);
    node->n_regs_aft = node->n_regs;

    /* Note whether or not the node is an fdef which is guaranteed to
       be compiled again. */

    if (CI_ISFDEF(node->sym->rec_typ))
      node->is_recomp = (recomp_kind(node->sym)>=2);
    else
      node->is_recomp = 0;

    node->addr_taken = (!node->is_recomp || db_rec_addr_taken(p));
    node->deletable = !node->addr_taken;
    node->head_of_cycle = 0;
    node->visited = 0;

    /* Ask the subst module if this routine is inlinable. */
    node->inlinable = CI_ISFDEF(p->rec_typ) && fdef_inlinable(p);

    if (strcmp(p->name, "main") == 0)
    {
      node->inlinable = 0;
      node->addr_taken = 1;
      node->deletable = 0;
      cg.main_node = node;
    }

    node_avail++;
  }
}

call_graph *
construct_call_graph()
{
  /*
   * construct the call graph as completely as possible given
   * the information in the data base.
   */
  call_graph_arc  *arc_avail;   /* used in allocating call graph arcs */
  int cur_arc_id;
  int i;                /* loop var */

  /*
   * figure out how many call graph nodes are needed, and how many call
   * call graph arcs are needed.  Need 1 call graph node for every function.
   * Need 1 call graph arc for each call in every function.
   *
   * Also run through the call graph and add a CI_FREF_REC_TYP node for
   * any function that is referenced in the call-graph, but isn't already
   * in the symbol table.
   */
  cg.num_cg_nodes = num_funcs;
  cg.num_cg_arcs  = 0;
  dbp_for_all_sym(&prog_db, callee_check);

  /*
   * allocate the necessary nodes and arcs.  Avoid malloc(0).
   */
  cg.cg_nodes = (call_graph_node *)db_malloc(
			(cg.num_cg_nodes > 0)
				? cg.num_cg_nodes * sizeof(call_graph_node)
				: sizeof(call_graph_node));
  cg.cg_arcs = (call_graph_arc *)db_malloc(
			(cg.num_cg_arcs > 0)
				? cg.num_cg_arcs * sizeof(call_graph_arc)
				: sizeof(call_graph_arc));
  node_avail = cg.cg_nodes;
  arc_avail  = cg.cg_arcs;

  /*
   * run through every symbol table node assigning it a call graph node.
   */
  dbp_for_all_sym(&prog_db, assign_cg_node);

  /*
   * run through the call-graph nodes and build the arcs in the call-graph.
   */
  cur_arc_id = 0;
  for (i=0; i < cg.num_cg_nodes; i++)
  {
    call_graph_node * cgn_p = &cg.cg_nodes[i];
    st_node * sym = cgn_p->sym;
    int n_calls = db_rec_n_calls(sym);
    char *call_vec = db_rec_call_vec(sym);
    unsigned char *reg_live_vect = db_rec_reg_pressure_info(sym);
    int j;
    call_graph_arc *list_end_p = 0;

    cgn_p->n_calls_out = n_calls;

    for (j = 0; j < n_calls; j++)
    {
      st_node * t;

      /* put the arc at the end of the outgoing call list */
      arc_avail->call_fm = cgn_p;
      arc_avail->site_id = j + 1;
      arc_avail->out_arc_next = 0;
      arc_avail->regs_live = reg_live_vect[j];
      if (list_end_p == 0)
        cgn_p->call_out_list = arc_avail;
      else
        list_end_p->out_arc_next = arc_avail;
      list_end_p = arc_avail;

      /* lookup the symbol pointed to by call_vec */
      t = dbp_lookup_sym(&prog_db, call_vec);
      check_call_target (t);

      /* put the arc in the incoming call list if any. */
      if (t == 0)
      {
        arc_avail->call_to = 0;
        arc_avail->in_arc_next = 0;
        arc_avail->inlinable = 0;
      }
      else
      {
        arc_avail->call_to = CGN_P(t);
        arc_avail->in_arc_next = CGN_P(t)->call_in_list;
        CGN_P(t)->call_in_list = arc_avail;

        /* also update the n_calls_in field */
        CGN_P(t)->n_calls_in ++;

        /*
         * the arc is inlinable only if the routine it calls
         * is inlinable, and if the arc originates from a FUNC_DEF_REC
         * node.
         */
        arc_avail->inlinable = (CGN_P(t)->inlinable);
      }

      arc_avail->arc_count   = 0;
      arc_avail->arc_id      = cur_arc_id ++;
      arc_avail->visited = 0;
      arc_avail->is_back_arc = 0;
      arc_avail->cost = 0;

      /* move call_vec up to next symbol */
      call_vec += strlen(call_vec)+1;
      arc_avail ++;
    }
  }

  /*
   * compute call heights for all nodes in the call graph.
   */
  if (cg.main_node != 0)
    compute_call_height(cg.main_node, (call_graph_arc *)0);

  for (i=0; i < cg.num_cg_nodes; i++)
  {
    call_graph_node * cgn_p = &cg.cg_nodes[i];
    if (cgn_p->node_height == 0 && cgn_p->addr_taken && cgn_p->n_calls_in == 0)
      compute_call_height(cgn_p, (call_graph_arc *)0);
  }

  for (i = 0; i < cg.num_cg_nodes; i++)
  {
    call_graph_node * cgn_p = &cg.cg_nodes[i];
    if (cgn_p->node_height == 0 && cgn_p->addr_taken)
      compute_call_height(cgn_p, (call_graph_arc *)0);	
  }

  for (i = 0; i < cg.num_cg_nodes; i++)
  {
    call_graph_node * cgn_p = &cg.cg_nodes[i];
    if (cgn_p->node_height == 0)
      compute_call_height(cgn_p, (call_graph_arc *)0);	
  }

  cg.call_count   = 0;  /* for now */

  return (&cg);
}

void
destroy_call_graph (cg)
call_graph *cg;
{
  free (cg->cg_arcs);
  free (cg->cg_nodes);
  cg->cg_arcs = 0;
  cg->cg_nodes = 0;
  cg->num_cg_nodes = 0;
  cg->num_cg_arcs = 0;
}

static void
cg_closure_page(line_p)
int *line_p;
{
  fprintf (decision_file.fd, "\n");
  fprintf (decision_file.fd, "Call Graph Closure\n");
  fprintf (decision_file.fd, "Function\n");
  fprintf (decision_file.fd, "  Callee\n");
  fprintf (decision_file.fd, "\n");

  *line_p = 5;
}

static void
traverse_closure(node_p, line_p)
call_graph_node *node_p;
int *line_p;
{
  call_graph_arc  *arc_p;

  if (node_p == 0 || node_p->visited)
    return;

  if (*line_p > db_page_size())
    cg_closure_page(line_p);

  fprintf (decision_file.fd, "  %s\n", node_p->sym->name);
  node_p->visited = 1;

  for (arc_p = node_p->call_out_list; arc_p != 0; arc_p = arc_p->out_arc_next)
    traverse_closure(arc_p->call_to, line_p);
}

void
print_cg_closure(cg)
call_graph * cg;
{
  call_graph_node *node_p;
  call_graph_node *tnode_p;
  call_graph_node *quit;
  call_graph_arc  *arc_p;
  int n_lines;

  if (flag_print_closure)
  {
    n_lines = 70;

    node_p = cg->cg_nodes;
    quit = &cg->cg_nodes[cg->num_cg_nodes];
    for (; node_p < quit; node_p++)
    {
      tnode_p = cg->cg_nodes;
      for (; tnode_p < quit; tnode_p++)
        tnode_p->visited = 0;

      if (n_lines > db_page_size())
        cg_closure_page(&n_lines);

      fprintf (decision_file.fd, "%s\n", node_p->sym->name);
      node_p->visited = 1;

      for (arc_p = node_p->call_out_list; arc_p != 0;
           arc_p = arc_p->out_arc_next)
        traverse_closure (arc_p->call_to, &n_lines);
    }
  }
}

static void
mark_non_deletable(cgn_p)
call_graph_node *cgn_p;
{
  call_graph_arc *arc_p;
  cgn_p->deletable = 0;

  /*
   * check if this changes the deletability of any of the nodes
   * that this node connects to.
   */
  for (arc_p = cgn_p->call_out_list; arc_p != 0; arc_p = arc_p->out_arc_next)
  {
    /* if the callee isn't already non-deletable make it so. */
    if (arc_p->call_to != 0 && arc_p->call_to->deletable)
      mark_non_deletable (arc_p->call_to);
  }
}

void
prune_deletable_nodes(cg)
call_graph *cg;
{
#if 0
  /*
   * Run through the call graph and mark every node that has its address
   * taken as non-deletable.
   */
  call_graph_node *cgn_p = cg->cg_nodes;
  call_graph_node *quit  = &cgn_p[cg->num_cg_nodes];
  call_graph_arc *arc_p;

  for (; cgn_p < quit; cgn_p++)
  {
    if (cgn_p->addr_taken)
      mark_non_deletable(cgn_p);
  }

  for (cgn_p = cg->cg_nodes; cgn_p < quit; cgn_p++)
  {
    /*
     * If a deletable node is one of the callers of a node that
     * isn't deletable, then decrement the n_calls_in field of the callee
     * to indicate the actual number of known incoming calls.  I
     * do not actually take the deletable routine incoming arc out of
     * the in_arc_list because that would be work which I don't believe
     * is necessary.
     */
    if (cgn_p->deletable)
    {
      for (arc_p = cgn_p->call_out_list;
           arc_p != 0;
           arc_p = arc_p->out_arc_next)
      {
        if (arc_p->call_to != 0)
          arc_p->call_to->n_calls_in --;
      }
    }
  }
#endif
}

/*
 * This routine forces rounding to double.
 */
double
d_to_d(in)
double in;
{
  return in;
}
