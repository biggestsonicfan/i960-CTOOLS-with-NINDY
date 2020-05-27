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
#include "assert.h"
#include "cc_info.h"
#include "callgrph.h"
#include "cgprof.h"
#include "gcdm960.h"

extern int num_funcs;
extern dbase prog_db;

void
count_dynamic_calls(cg)
call_graph *cg;
{
  /*
   * Run through every node in the call graph, and count the total number
   * of dynamic calls that were recorded.
   */
  double count;
  register call_graph_node *node_p;
  register call_graph_node *node_quit;

  /*
   * figure out multiplier and divisor such that the total arc count
   * when multiplied by multiplier and divided by divisor come out
   * to about 100000.
   */

  count = 0;
  node_p = cg->cg_nodes;
  node_quit = &node_p[cg->num_cg_nodes];
  for (; node_p < node_quit; node_p++)
    count = d_to_d(count + node_p->node_count);

  cg->call_count = count;
}

/*
 * This routine incorporates the profile information for a single
 * function into the call-graph.
 */
void
get_func_prof(cgn_p)
call_graph_node *cgn_p;
{
  call_graph_arc * arc_p;
  register unsigned char *prof_info;
  register char *file_name;
  st_node * prof_p;
  register int prof_index;

  file_name = db_rec_prof_info(cgn_p->sym);
  if (file_name == 0)
    return;

  prof_info = (unsigned char *)(file_name + strlen(file_name) + 1);

  /* look up the base file index in the profile table */
  prof_p = dbp_lookup_sym(&prog_db, file_name);

  if (prof_p == 0)
    return;

  /* note: the "file_name" used above is actually the module name. */
  /* Get the real file name here, and stuff it into the node. */
  cgn_p->file_name = prof_p->file_name;

  /* record the nodes count */
  CI_U32_FM_BUF(prof_info, cgn_p->first_bb);
  prof_info += 4;
  CI_U32_FM_BUF(prof_info, cgn_p->last_bb);
  prof_info += 4;

  assert (cgn_p->first_bb != -1);
  if (db_fdef_has_prof (cgn_p->sym))
    cgn_p->node_count = db_fdef_prof_counter (cgn_p->sym, 0);
  else
    cgn_p->node_count = 0;

  /* now get the counts for each of the outgoing arcs */
  for (arc_p = cgn_p->call_out_list;
       arc_p != 0; arc_p = arc_p->out_arc_next)
  {
    double t;

    CI_U32_FM_BUF(prof_info, prof_index);
    prof_info += 4;
    if (db_fdef_has_prof (cgn_p->sym))
      t = db_fdef_prof_counter (cgn_p->sym, prof_index - cgn_p->first_bb);
    else
      t = 0;
    arc_p->arc_count = t;
  }
}

/*
 * This routine runs through the call-graph and associates profile
 * information with the arcs in the call-graph, thus weighting the call
 * graph.  This weighted information will be used to aid in making
 * good inlining decisions.
 */
void
prof_call_count(cg)
call_graph *cg;
{
  /*
   * Run through the call graph and incorporate the call counts
   * from the profile information into the nodes and arcs in the
   * call graph.
   */
  register call_graph_node *cgn_p = cg->cg_nodes;
  register call_graph_node *quit  = &cgn_p[cg->num_cg_nodes];
  register call_graph_arc * arc_p;

  for (cgn_p = cg->cg_nodes; cgn_p < quit; cgn_p++)
  {
    if (!cgn_p->deletable)
      get_func_prof(cgn_p);
  }

  /*
   * Now run through the call graph nodes, and for any CI_FREF_REC_TYP
   * node, simply add any incoming arcs up to get the count for the
   * node.
   */
  for (cgn_p = cg->cg_nodes; cgn_p < quit; cgn_p++)
  {
    if (!cgn_p->deletable && cgn_p->sym->rec_typ == CI_FREF_REC_TYP)
    {
      for (arc_p = cgn_p->call_in_list;
           arc_p != 0; arc_p = arc_p->in_arc_next)
        cgn_p->node_count += arc_p->arc_count;
    }
  }
}

static void
cg_page (line_p, extra)
int *line_p;
int extra;
{
  static char *cg_header[2][3] =
{
"Function                  Site #     Count     Percent\n",
"  Calle%c\n",
"=========================|======|=============|=======\n",

"Function                  Site #     Count     Percent  Size  Regs Inline\n",
"  Calle%c\n",
"=========================|======|=============|=======|======|====|======\n",
};

  fprintf (decision_file.fd, "\n");
  fprintf (decision_file.fd, "Call Graph\n\n");
  fprintf (decision_file.fd, cg_header[extra][0]);
  fprintf (decision_file.fd, cg_header[extra][1],
                   flag_print_reverse_call_graph ? 'r' : 'e');
  fprintf (decision_file.fd, cg_header[extra][2]);
  fprintf (decision_file.fd, "\n");

  *line_p = 7;
}

void
print_cg_counts(cg, extra)
call_graph *cg;
int extra;
{
  static char *cg_nd_fmt[2] = 
  {
    "%-25.25s        %13.0f %6.2f%%\n",
    "%-25.25s        %13.2f %6.2f%% %6d %4d %4d\n",
  };

  static char *cg_arc_fmt[2] =
  { 
    "  %-23.23s %6d %13.0f %6.2f%%\n",
    "  %-23.23s %6d %13.2f %6.2f%%        %4d\n"
  };

  double tot_count = cg->call_count;
  call_graph_node *node_p;
  call_graph_node *quit;
  call_graph_arc  *arc_p;
  int n_lines;

  if (flag_print_call_graph || flag_print_reverse_call_graph)
  {
    cg_page(&n_lines, extra);

    node_p = cg->cg_nodes;
    quit = &cg->cg_nodes[cg->num_cg_nodes];
    for (; node_p < quit; node_p++)
    {
      double	n_cnt;

      if (n_lines > db_page_size())
        cg_page(&n_lines, extra);

      n_cnt = tot_count
		? d_to_d(d_to_d(node_p->node_count * 100) / tot_count)
		: 0;

      fprintf (decision_file.fd, cg_nd_fmt[extra],
               node_p->sym->name,
               node_p->node_count,
               n_cnt,
               node_p->node_insns,
               node_p->n_regs,
               node_p->inlinable);
      n_lines ++;

      if (flag_print_reverse_call_graph)
      {
        arc_p = node_p->call_in_list;
        for (; arc_p != 0; arc_p = arc_p->in_arc_next)
        {
          if (n_lines > db_page_size())
            cg_page(&n_lines, extra);

          if (arc_p->call_fm != 0)
          {
	    double a_cnt = tot_count
			? d_to_d(d_to_d(arc_p->arc_count * 100) / tot_count)
			: 0;

            fprintf (decision_file.fd, cg_arc_fmt[extra],
                     arc_p->call_fm->sym->name,
                     arc_p->site_id,
                     arc_p->arc_count,
                     a_cnt,
                     arc_p->regs_live);
            n_lines++;
          }
        }
      }
      else
      {
        arc_p = node_p->call_out_list;
        for (; arc_p != 0; arc_p = arc_p->out_arc_next)
        {
          if (n_lines > db_page_size())
            cg_page(&n_lines, extra);

          if (arc_p->call_to != 0)
          {
	    double a_cnt = tot_count
			? d_to_d(d_to_d(arc_p->arc_count * 100) / tot_count)
			: 0;

            fprintf (decision_file.fd, cg_arc_fmt[extra],
                     arc_p->call_to->sym->name,
                     arc_p->site_id,
                     arc_p->arc_count,
                     a_cnt,
                     arc_p->regs_live);
            n_lines++;
          }
        }
      }
    }
  }
}
