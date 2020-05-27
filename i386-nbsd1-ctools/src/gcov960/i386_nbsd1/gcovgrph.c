/*(c**************************************************************************** *
 * Copyright (c) 1993 Intel Corporation
 *
 * Intel hereby grants you permission to copy, modify, and distribute this
 * software and its documentation.  Intel grants this permission provided
 * that the above copyright notice appears in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  In addition, Intel grants this permission provided that
 * you prominently mark as "not part of the original" any modifications
 * made to this software or documentation, and that the name of Intel
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software or the documentation without specific,
 * written prior permission.
 *
 * Intel Corporation provides this AS IS, WITHOUT ANY WARRANTY, EXPRESS OR
 * IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY
 * OR FITNESS FOR A PARTICULAR PURPOSE.  Intel makes no guarantee or
 * representations regarding the use of, or the results of the use of,
 * the software and documentation in terms of correctness, accuracy,
 * reliability, currentness, or otherwise; and you rely on the software,
 * documentation and results solely at your own risk.
 *
 * IN NO EVENT SHALL INTEL BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
 * LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
 * OF ANY KIND.  IN NO EVENT SHALL INTEL'S TOTAL LIABILITY EXCEED THE SUM
 * PAID TO INTEL FOR THE PRODUCT LICENSED HEREUNDER.
 *
 ***************************************************************************c)*/
/* $Id: gcovgrph.c,v 1.16 1995/10/16 16:46:02 kevins Exp $ */

#include <assert.h>
#include "gcov960.h"
#include "gcovrptr.h"

/*
 *  construct_call_graph()
 *
 *     Build a call graph based from the info extracted from the data base
 */

call_graph *
construct_call_graph (db)
dbase *db;
{
  int num_funcs = 0;
  int num_arcs = 0;  /* number of call graph arcs in the array */

  /* construct the call graph as completely as possible given
   * the information in the data base.  */

  static call_graph cg;
  
  call_graph_node * cg_nodes;    /* an array of call graph nodes */
  int num_nodes;           /* number of nodes in call graph node array. */
  call_graph_arc  *cg_arcs;      /* an array of call graph arcs */
  int cur_arc_id;
  
  call_graph_node *node_avail;  /* used in allocating call graph nodes */
  call_graph_arc  *arc_avail;   /* used in allocating call graph arcs */
  
  st_node *p;           /* loop induction var */
  int i;                /* loop var */
  
  /* figure out how many call graph nodes are needed, and how many 
     call graph arcs are needed.  Need 1 call graph node for every function.
     Need 1 call graph arc for each call in every function.  */

  for (i=0; i<CI_HASH_SZ; i++)
    for (p = db->db_stab[i]; p != 0; p = p->next)
      if (p->db_rec_size != 0)
      {
        if (CI_ISFUNC(p->rec_typ))
          num_funcs++;

        if (CI_ISFDEF(p->rec_typ))
          num_arcs += db_rec_n_calls(p);
      }

  num_nodes = num_funcs;
  
  /*
   * allocate the necessary nodes and arcs.
   */
  cg_nodes = (call_graph_node *) db_malloc(num_nodes * sizeof(call_graph_node));
  memset (cg_nodes, 0, num_nodes * sizeof(call_graph_node));

  cg_arcs = (call_graph_arc *) db_malloc(num_arcs * sizeof(call_graph_arc));
  memset (cg_arcs, 0, num_arcs * sizeof(call_graph_arc));

  node_avail = cg_nodes;
  arc_avail  = cg_arcs;

  for (i=0; i<CI_HASH_SZ; i++)
    for (p = db->db_stab[i]; p != 0; p = p->next)
      if (p->db_rec_size != 0)
        if (CI_ISFUNC (p->rec_typ))
        {
          /* allocate a call graph node for the symbol and build it. */
          p->extra = (char *)node_avail;
          node_avail->sym = p;
          node_avail->n_calls_out = db_rec_n_calls(p);
          node_avail->n_calls_in = 0;
          node_avail->call_out_list = 0;
          node_avail->call_in_list = 0;
          node_avail->first_bb = -1;
          node_avail->last_bb = -1;
          node_avail->node_count = 0;
          node_avail->traversed = 0;
          node_avail->addr_taken = db_rec_addr_taken(p);
          if (strcmp(p->name, "main") == 0)
          {
            node_avail->deletable = 0;
            node_avail->addr_taken = 1;
            cg.main_node = node_avail;
          }
          node_avail->deletable = !node_avail->addr_taken;
          node_avail++;
        }
  
  /*
   * Every symbol now has a call graph node.  Now run through all the call
   * graph nodes and build the arcs in the call graph.
   */

  cur_arc_id = 0;
  for (i=0; i < num_nodes; i++)
  {
    call_graph_node * cgn_p = &cg_nodes[i];
    st_node * sym = cgn_p->sym;
    int n_calls = db_rec_n_calls(sym);
    char *call_vec = db_rec_call_vec(sym);
    int j;
    call_graph_arc *list_end_p = 0;
    
    cgn_p->n_calls_out = n_calls;
    
    for (j = 0; j < n_calls; j++)
    {
      st_node * t;
      
      /* put the arc at the end of the outgoing call list */
      arc_avail->call_fm = cgn_p;
      arc_avail->out_arc_next = 0;
      if (list_end_p == 0)
        cgn_p->call_out_list = arc_avail;
      else
        list_end_p->out_arc_next = arc_avail;
      list_end_p = arc_avail;
      
      /* lookup the symbol pointed to by call_vec */
      t = dbp_lookup_sym (db, call_vec);
      
      /* put the arc in the incoming call list if any. */
      if (t == 0)
      {
        arc_avail->call_to = 0;
        arc_avail->in_arc_next = 0;
      }
      else
      {
        arc_avail->call_to = CGN_P(t);
        arc_avail->in_arc_next = CGN_P(t)->call_in_list;
        CGN_P(t)->call_in_list = arc_avail;
        
        /* also update the n_calls_in field */
        CGN_P(t)->n_calls_in ++;
        
      }
      
      arc_avail->arc_count = 0;
      arc_avail->arc_id = cur_arc_id ++;
      
      /* move call_vec up to next symbol */
      call_vec += strlen(call_vec)+1;
      arc_avail ++;
    }
  }
  
  cg.num_cg_nodes = num_nodes;
  cg.num_cg_arcs  = num_arcs;
  cg.cg_nodes     = cg_nodes;
  cg.cg_arcs      = cg_arcs;
  cg.call_count   = 0;

  return (&cg);
}

static void
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

  count = 0;
  node_p = cg->cg_nodes;
  node_quit = &node_p[cg->num_cg_nodes];
  for (; node_p < node_quit; node_p++)
    count += node_p->node_count;

  cg->call_count = count;
}

void
prof_call_count(db, cg)
dbase* db;
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
    /* if (!cgn_p->deletable) */
    {
      register unsigned char *prof_info;
      register char *file_name;
      st_node * prof_p;
      register int prof_index;

      file_name = db_rec_prof_info(cgn_p->sym);
      if (file_name == 0)
        continue;

      prof_info = (unsigned char *)(file_name + strlen(file_name) + 1);

      /* look up the base file index in the profile table */
      prof_p = dbp_lookup_sym(db, file_name);

      if (prof_p == 0)
        continue;

      /* note: the "file_name" used above is actually the module name. */
      /* Get the real file name here, and stuff it into the node. */
      cgn_p->file_name = prof_p->file_name;

      /* record the nodes count */
      CI_U32_FM_BUF(prof_info, cgn_p->first_bb);
      prof_info += 4;
      CI_U32_FM_BUF(prof_info, cgn_p->last_bb);
      prof_info += 4;

      assert (cgn_p->first_bb != -1);
      if (db_fdef_prof_quality (cgn_p->sym) >= cmd_line.min_quality)
        cgn_p->node_count = db_fdef_prof_counter (cgn_p->sym, 0);
      else
        cgn_p->node_count = 0;

      /* now get the counts for each of the outgoing arcs */
      for (arc_p = cgn_p->call_out_list;
           arc_p != 0; arc_p = arc_p->out_arc_next)
      {
        CI_U32_FM_BUF(prof_info, prof_index);
        prof_info += 4;
        if (db_fdef_prof_quality (cgn_p->sym) >= cmd_line.min_quality)
          arc_p->arc_count = db_fdef_prof_counter
            (cgn_p->sym, prof_index - cgn_p->first_bb);
        else
          arc_p->arc_count = 0;
      }
    }
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

  count_dynamic_calls(cg);
}


/*
 *  Below are routines and structs for displaying call graphs
 */


typedef  struct
{
    unsigned int index;
    call_graph_node *g_node;
} index_type;

static   index_type *indices;
static  int max_index = 0;


/*
 * get_index()
 *    
 *     Return the index number assigned to a function name
 */
static unsigned int 
get_index( func_node )
call_graph_node *func_node;
{
  int i;

  if (func_node == 0) return 0;

  for (i = 0 ; i < max_index ; i++)
  {
    if (indices[i].g_node == func_node)
    {
      return (indices[i].index);
    }
  }
  return 0;
}

static int index_cmp(I1, I2)
index_type *I1;
index_type *I2;
{
  return strcmp(I1->g_node->sym->name, I2->g_node->sym->name);
}

/*
 *  display_call_graph()
 *
 *      Traverses the call graph, assigns index number to each node traversed.
 *  Displays the call graph in a gprof-like format.
 */
void display_call_graph( help_me )
int help_me; /* if 1 then display call-graph help text */
{
  call_graph *cg = cmd_line.cg;
  call_graph_node *quit, *node_p;
  call_graph_arc  *arc_p;
  int i;
  int name_cnt = 0;
  count_type fnc_cnt;
  char buf[FNC_NAME_WIDTH + 1];

  indices = (index_type *) db_malloc(sizeof(index_type) * cg->num_cg_nodes);
  memset (indices, 0, sizeof(index_type) * cg->num_cg_nodes);

  display_header();

  center_header(stdout, "Call Graph Summary");

  if (help_me)
  {
    gcov960_help_cg();
    fprintf(stdout, "_______    _______    _____________________    ____________________\n\n");
  }
  fprintf(stdout, "                         Called/Total             Parents\n");
  fprintf(stdout, " Index     %%Cov       Called                   Name [Index]\n");
  fprintf(stdout, "                         Called/Total             Children\n");
  fprintf(stdout, "_______    _______    _____________________    ____________________\n\n");

#define GRAPH_FMT_1     "%6s     %7s    %-21s    %s %s\n"
#define GRAPH_FMT_2     "%25s%-18s       %-s %-s\n"
 
  quit = &cg->cg_nodes[cg->num_cg_nodes];

  /* assign indices to the nodes */
  node_p = cg->cg_nodes;
  for ( ; node_p < quit ; node_p++)
  {
    if (node_p->sym->name)
    {
      indices[max_index].index = max_index + 1; 
      indices[max_index].g_node = node_p;
      max_index++;
    } 
  }

  node_p = cg->cg_nodes;    
  for ( ; node_p < quit ; node_p++)
  {

    /* for formatting */
    char index_buf[15];
    char cov_buf[15];
    char called_buf[30];

    int Index;

    /* display the function's parent */
    arc_p = node_p->call_in_list;
    for (; arc_p && arc_p->call_fm; arc_p = arc_p->in_arc_next)
    {
      if ((Index = get_index(arc_p->call_fm)) > 0)
      {
        sprintf(index_buf,"[%d]", Index);
        sprintf(called_buf,"%d/%d", (int)arc_p->arc_count,
          (int)node_p->node_count);
        fprintf(stdout, GRAPH_FMT_2," ", called_buf,
          truncate_str(arc_p->call_fm->sym->name, FNC_NAME_WIDTH - 3), index_buf);
      }
    }
    /* display the current function */
    if (node_p->sym->name)
    {
      compute_fnc_cnts(node_p, &fnc_cnt, 
           find_src(node_p->file_name, 0));

      if ((Index = get_index(node_p)) > 0) 
      {
        sprintf(index_buf,"[%d]", Index);
        sprintf(cov_buf, "%5.2f%%", fnc_cnt.c1);
        sprintf(called_buf,"%d", (int)node_p->node_count);
        fprintf(stdout, GRAPH_FMT_1, index_buf,
          cov_buf, called_buf, truncate_str(node_p->sym->name, FNC_NAME_WIDTH - 3),
          index_buf);
      }
    }
    /* display the function's children */
    arc_p = node_p->call_out_list;
    for (; arc_p && arc_p->call_to; arc_p = arc_p->out_arc_next)
    {
      if ((Index = get_index(arc_p->call_to)) > 0)
      {
        sprintf(index_buf,"[%d]", Index);
        sprintf(called_buf,"%d/%d", (int)arc_p->arc_count,
          (int)arc_p->call_to->node_count);
        fprintf(stdout, GRAPH_FMT_2," ", called_buf,
          truncate_str(arc_p->call_to->sym->name, FNC_NAME_WIDTH - 3), index_buf);
      }
    }
    fprintf(stdout, "\n-----------------------------------------------------------------------------\n\n");
  }

  /* Now display the symbols and their indices in alphabetical 
         * order
         */
  qsort(indices, max_index, sizeof(index_type), index_cmp);

  fprintf(stdout,"\nIndex by Function Name:\n");
  fprintf(stdout, "\n%-30s    %-30s    Index\n", "Function Name", "Filename");
  fprintf(stdout, "______________________________    ______________________________    _____\n\n");

  for (i=0 ; i < max_index ; i++)
  {
    strcpy(buf, truncate_str(indices[i].g_node->file_name, FNC_NAME_WIDTH));
    fprintf(stdout, "%-30s    %-30s    [%d]\n", truncate_str(indices[i].g_node->sym->name, FNC_NAME_WIDTH), 
      buf, indices[i].index);
  }
  fprintf(stdout,"\n");

  compute_program_coverage(&fnc_cnt);
  display_a_coverage(stdout, &fnc_cnt, PROGRAM_COV);

  display_trailer(stdout);

  free(indices);
}
