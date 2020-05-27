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
#include "cc_info.h"
#include "cc_pinfo.h"
#include "assert.h"

/*
 * This module "stretches" a profile for a function, to fit a new
 * control flow graph.  This is useful because we assume that limited
 * changes were made to the control flow graph of the new function
 * versus the function that was actually profiled.
 *
 * Stretching is accomplished by building a weighted control flow
 * graph (CFG) for the profiled function, and then adding and
 * deleting nodes, and adding and deleting arcs to make the
 * weighted CFG match the new CFG.  After the weighted CFG has been
 * transformed into the new CFG, we then walk around the graph
 * computing any missing weights.  Lastly we transform the weighted
 * CFG back into the appropriate form for being stored into a
 * spf file.
 *
 * The function stretch_fdef_prof is the only entry point in this
 * file, and comtrols the stretching process.
 */

/* 
 * This enumeration is used by depth first search to classify the arcs.
 * This information is used to make better decisions about which nodes
 * to add/delete.
 */
#define unclassified 0
#define tree_edge 1
#define back_edge 2
#define forward_edge 3

#define state_no_weight 0
#define state_weighted  1
#define state_multiplier 2
#define state_deleted   3

/*
 * This struct defined the data struture used to represent
 * an arc in the CFG.
 */
typedef struct arc
{
  double weight;
  struct arc * out_arc_next;
  struct arc * in_arc_next;
  unsigned short fm_node;
  unsigned short to_node;
  unsigned short arc_id;
  unsigned char arc_t; 
  unsigned char state;
} * arc_p;

/*
 * This struct defines the data structure used to represent a
 * node in the CFG.
 */
typedef struct node
{
  double weight;
  arc_p out_arcs;
  arc_p in_arcs;
  unsigned short node_id;
  unsigned char state;
} * node_p;

/*
 * This data structure defines the CFG.  Sometimes only the arcs
 * exist.
 */
typedef struct graph
{
  int num_nodes;
  int num_nodes_allocated;
  node_p nodes;

  int num_arcs;
  int num_arcs_allocated;
  arc_p arcs;
} graph, * graph_p;

static void
graph_init(graph_ptr)
graph_p graph_ptr;
{
  graph_ptr->num_nodes = 0;
  graph_ptr->num_nodes_allocated = 0;
  graph_ptr->nodes = 0;
  graph_ptr->num_arcs = 0;
  graph_ptr->num_arcs_allocated = 0;
  graph_ptr->arcs = 0;
}

static void
free_nodes(gp)
graph_p gp;
{
  if (gp->nodes != 0)
    free(gp->nodes);

  gp->nodes = 0;
  gp->num_nodes = 0;
  gp->num_nodes_allocated = 0;
}

static void
free_arcs(gp)
graph_p gp;
{
  if (gp->arcs != 0)
    free(gp->arcs);

  gp->arcs = 0;
  gp->num_arcs = 0;
  gp->num_arcs_allocated = 0;
}

static void
free_graph(graph_ptr)
graph_p graph_ptr;
{
  free_nodes(graph_ptr);
  free_arcs(graph_ptr);
}

static int
arc_cmp(a1, a2)
arc_p a1;
arc_p a2;
{
  /* want all deleted nodes to rise to the top */
  if ((a1->state == state_deleted) < (a2->state == state_deleted))
    return -1;
  if ((a1->state == state_deleted) > (a2->state == state_deleted))
    return 1;

  if (a1->fm_node < a2->fm_node)
    return -1;
  if (a1->fm_node > a2->fm_node)
    return 1;
  if (a1->to_node < a2->to_node)
    return -1;
  if (a1->to_node > a2->to_node)
    return 1;

  return a1 - a2;
}

static void
sort_arcs(gp)
graph_p gp;
{
  int narcs;
  qsort(gp->arcs, gp->num_arcs, sizeof(struct arc), arc_cmp);

  /* get rid of any deleted arcs that rose to the top */
  narcs = gp->num_arcs;
  while (narcs > 0 && gp->arcs[narcs-1].state == state_deleted)
    narcs --;
  gp->num_arcs = narcs;
}

static void
allocate_nodes(graph_ptr, num)
graph_p graph_ptr;
int num;
{
  int i;
  int old_num = graph_ptr->num_nodes_allocated;

  if (graph_ptr->nodes == 0)
    graph_ptr->nodes = (node_p)db_malloc(num * sizeof(struct node));
  else
    graph_ptr->nodes = (node_p)db_realloc(graph_ptr->nodes, num * sizeof(struct node));

  graph_ptr->num_nodes_allocated = num;

  for (i = old_num; i < num; i++)
  {
    graph_ptr->nodes[i].out_arcs = 0;
    graph_ptr->nodes[i].in_arcs = 0;
  }
}

static void
new_node(graph_ptr, num)
graph_p graph_ptr;
int num;
{
  if (num >= graph_ptr->num_nodes_allocated)
    allocate_nodes(graph_ptr, ((num >> 7) + 1) << 7);

  if (num >= graph_ptr->num_nodes)
    graph_ptr->num_nodes = num + 1;
}

static void
allocate_arcs(graph_ptr, num)
graph_p graph_ptr;
int num;
{
  int i;
  int old_num = graph_ptr->num_arcs_allocated;

  if (graph_ptr->arcs == 0)
    graph_ptr->arcs = (arc_p)db_malloc(num * sizeof(struct arc));
  else
    graph_ptr->arcs = (arc_p)db_realloc(graph_ptr->arcs, num * sizeof(struct arc));

  graph_ptr->num_arcs_allocated = num;

  for (i = old_num; i < num; i++)
  {
    graph_ptr->arcs[i].fm_node = 0;
    graph_ptr->arcs[i].to_node = 0;
    graph_ptr->arcs[i].arc_t = unclassified;
    graph_ptr->arcs[i].out_arc_next = 0;
    graph_ptr->arcs[i].in_arc_next = 0;
  }
}

static void
new_arc(graph_ptr, num)
graph_p graph_ptr;
int num;
{
  if (num >= graph_ptr->num_arcs_allocated)
    allocate_arcs(graph_ptr, ((num >> 7) + 1) << 7);

  if (num >= graph_ptr->num_arcs)
    graph_ptr->num_arcs = num + 1;
}

/*
 * read a graph from standard input, and build its internal representation.
 * graph is just represented as a list of pairs, fm_node, to_node.
 */
static void
read_arcs(gp, info)
graph_p gp;
unsigned char *info;
{
  int num_arcs;
  int arc_no;
  int fm_node;
  int to_node;
  int arc_id;

  CI_U32_FM_BUF(info, num_arcs);
  info += 4;
  /*
   * value in num_arcs is really length of information.
   * Convert this to actual number of arcs.
   * length = 4 + (num_arcs * (2+2+2)), so solve
   * for num_arcs.  We want to add an extra arc so that
   * we can have a separate entry node #0, with an arc from
   * this entry node to the first real node in the graph.
   */
  num_arcs = ((num_arcs - 4) / (2+2+2))+1;

  new_arc (gp, 0);
  gp->arcs[0].fm_node = 0;
  gp->arcs[0].to_node = 1;
  gp->arcs[0].arc_id  = -1;
  gp->arcs[0].weight = UNSET_WEIGHT;
  gp->arcs[0].state = state_no_weight;

  for (arc_no = 1; arc_no < num_arcs; arc_no++)
  {
    new_arc (gp, arc_no);

    CI_U16_FM_BUF(info, fm_node);
    CI_U16_FM_BUF(info+2, to_node);
    CI_U16_FM_BUF(info+4, arc_id);
    info += 6;

    {
      /* this code in here simply accounts for having
       * added my own node 0, and so all node numbers have
       * to move up by 1.  However, we want arcs to the exit
       * which are represented by having to_node being zero still
       * go to zero, which is the new entry node.  This connects
       * the graph perfectly for later weighting
       */
      fm_node += 1;

      if (to_node != 0)
        to_node += 1;
    }

    gp->arcs[arc_no].fm_node = fm_node;
    gp->arcs[arc_no].to_node = to_node;
    gp->arcs[arc_no].arc_id  = arc_id;
    gp->arcs[arc_no].weight  = UNSET_WEIGHT;
    gp->arcs[arc_no].state = state_no_weight;
  }
}

static void
node_init(gp)
graph_p gp;
{
  int i;
  int to_node;
  int fm_node;

  free_nodes(gp);

  for (i = 0; i < gp->num_arcs; i++)
  {
    fm_node = gp->arcs[i].fm_node;
    to_node = gp->arcs[i].to_node;

    new_node(gp, to_node);
    new_node(gp, fm_node);

    gp->arcs[i].in_arc_next = 0;
    gp->arcs[i].out_arc_next = 0;
    gp->arcs[i].arc_t = unclassified;

    gp->nodes[fm_node].node_id = 0;
    gp->nodes[fm_node].weight = UNSET_WEIGHT;
    gp->nodes[fm_node].state  = state_no_weight;

    gp->nodes[to_node].node_id = 0;
    gp->nodes[to_node].weight = UNSET_WEIGHT;
    gp->nodes[to_node].state  = state_no_weight;
  }
}

static void
graph_from_arcs(gp)
graph_p gp;
{
  int i;
  int fm_node;
  int to_node;

  for (i = 0; i < gp->num_nodes; i++)
  {
    gp->nodes[i].in_arcs = 0;
    gp->nodes[i].out_arcs = 0;
  }

  for (i = 0; i < gp->num_arcs; i++)
  {
    gp->arcs[i].in_arc_next = 0;
    gp->arcs[i].out_arc_next = 0;
  }

  for (i = 0; i < gp->num_arcs; i++)
  {
    fm_node = gp->arcs[i].fm_node;
    to_node = gp->arcs[i].to_node;

    gp->arcs[i].in_arc_next = gp->nodes[to_node].in_arcs;
    gp->nodes[to_node].in_arcs = &gp->arcs[i];

    gp->arcs[i].out_arc_next = gp->nodes[fm_node].out_arcs;
    gp->nodes[fm_node].out_arcs = &gp->arcs[i];

    if (gp->arcs[i].arc_id != 0)
      gp->nodes[fm_node].node_id = gp->arcs[i].arc_id;
  }
}

static int
try_weight_arc(arc, fm_node, to_node)
arc_p arc;
node_p fm_node;
node_p to_node;
{
  int can_weight;
  arc_p arcs;
  double other_arcs_weights;

  if (fm_node->state == state_weighted)
  {
    /* see if all arcs but this one are weighted */
    can_weight = 1;
    other_arcs_weights = 0.0;
    for (arcs = fm_node->out_arcs; arcs != 0 && can_weight;
         arcs = arcs->out_arc_next)
    {
      if (arcs != arc)
        if (arcs->state == state_weighted)
          other_arcs_weights += arcs->weight;
        else
          can_weight = 0;
    }

    if (can_weight)
    {
      arc->weight = fm_node->weight - other_arcs_weights;
      arc->state = state_weighted;
      return 1;
    }
  }

  if (to_node->state == state_weighted)
  {
    /* see if all arcs but this one are weighted */
    can_weight = 1;
    other_arcs_weights = 0.0;
    for (arcs = to_node->in_arcs; arcs != 0 && can_weight;
         arcs = arcs->in_arc_next)
    {
      if (arcs != arc)
        if (arcs->state == state_weighted)
          other_arcs_weights += arcs->weight;
        else
          can_weight = 0;
    }

    if (can_weight)
    {
      arc->weight = to_node->weight - other_arcs_weights;
      arc->state = state_weighted;
      return 1;
    }
  }

  return 0;
}

static int 
weight_arcs(gp)
graph_p gp;
{
  int i;
  int change = 1;
  arc_p arcs = gp->arcs;
  int n_arcs = gp->num_arcs;
  int arcs_unweighted = gp->num_arcs;

  /*
   * try to find any arcs that we have enough information to
   * weight.
   */
  while (change && arcs_unweighted > 0)
  {
    change = 0;
    arcs_unweighted = gp->num_arcs;
    for (i = 0; i < n_arcs; i++)
    {
      switch (arcs[i].state)
      {
        case state_no_weight:
          {
            node_p fm_node = &gp->nodes[arcs[i].fm_node];
            node_p to_node = &gp->nodes[arcs[i].to_node];

            if (try_weight_arc(&arcs[i], fm_node, to_node) != 0)
            {
              arcs_unweighted -= 1;
              change = 1;
            }
          }
          break;

        case state_multiplier:
         {
           if (gp->nodes[arcs[i].fm_node].state == state_weighted)
           {
             double fm_nd_weight = gp->nodes[arcs[i].fm_node].weight;
             arcs[i].weight = arcs[i].weight * fm_nd_weight;
             arcs[i].state = state_weighted;
             arcs_unweighted -= 1;
             change = 1;
           }
         }
         break;

        default:
          arcs_unweighted -= 1;
          break;
      }
    }
  }

  return arcs_unweighted;
}

static int
try_weight_node(node)
node_p node;
{
  arc_p arcs;
  int   can_weight;
  double weight;
  
  switch (node->state)
  {
    case state_no_weight:
      {
        can_weight = 1;
        weight = 0.0;
        for (arcs = node->out_arcs; arcs != 0; arcs = arcs->out_arc_next)
        {
          if (arcs->state != state_weighted)
          {
            can_weight = 0;
            break;
          }
          weight += arcs->weight;
        }

        if (can_weight)
        {
          node->weight = weight;
          node->state = state_weighted;
          return 1;
        }

        can_weight = 1;
        weight = 0.0;
        for (arcs = node->in_arcs; arcs != 0; arcs = arcs->in_arc_next)
        {
          if (arcs->state != state_weighted)
          { 
            can_weight = 0;
            break;
          }
          weight += arcs->weight;
        }

        if (can_weight)
        {
          node->weight = weight;
          node->state = state_weighted;
          return 1;
        }
      }
      break;

    case state_multiplier:
      {
        /*
         * We just look at all the incoming arcs, except those that are
         * back edges, sum them, and then multiply the node weight the
         * this.
         */
        weight = 0.0;
        can_weight = 1;
        for (arcs = node->in_arcs; arcs != 0; arcs = arcs->in_arc_next)
        {
          if (arcs->arc_t != back_edge)
          {
            if (arcs->state != state_weighted)
            { 
              can_weight = 0;
              break;
            }
            weight += arcs->weight;
          }
        }

        if (can_weight)
        {
          node->weight = node->weight * weight;
          node->state = state_weighted;
          return 1;
        }
      }
      break; 
  }
  return 0;
}

static int
weight_nodes(gp)
graph_p gp;
{
  int i;
  node_p nodes = gp->nodes;
  int num_nodes = gp->num_nodes;
  int unweighted_nodes = num_nodes;

  for (i = 0; i < num_nodes; i++)
  {
    if (nodes[i].state == state_weighted ||
        try_weight_node(&nodes[i]) != 0)
      unweighted_nodes -= 1;
  }

  return unweighted_nodes;
}

static void
weight_nodes_from_pf(gp, cnt_info)
graph_p gp;
unsigned char *cnt_info;
{
  int i;
  unsigned long weight;

  /*
   * remember, we added an extra node for the entry point.  We will
   * weight this the same as the weight for the actual block 0.
   * Also remember that all node numbers are moved up by 1 compared
   * to the way the rest of the system numbers them.
   *
   * I don't increment cnt_info after this first read so that node 0
   * and node 1 get the same weight.
   */
  CI_U32_FM_BUF(cnt_info, weight);
  gp->nodes[0].weight = (double)weight;
  gp->nodes[0].state  = state_weighted;

  for (i = 1; i < gp->num_nodes; i++)
  {
    CI_U32_FM_BUF(cnt_info, weight);
    cnt_info += 4;
    gp->nodes[i].weight = (double)weight;
    gp->nodes[i].state  = state_weighted;
  }
}

static void
dfs_walk(graph_ptr, color, node)
graph_p graph_ptr;
unsigned char *color;
int node;
{
  int white = 0;
  int gray  = 1;
  int black = 2;

  arc_p arcs;
  color[node] = gray;
  for (arcs = graph_ptr->nodes[node].out_arcs; arcs != 0;
       arcs = arcs->out_arc_next)
  {
    int to = arcs->to_node;

    if (color[to] == white)
    {
      arcs->arc_t = tree_edge;
      dfs_walk(graph_ptr, color, arcs->to_node);
    }
    else
    {
      if (color[to] == black)
        arcs->arc_t = forward_edge;
      else
        arcs->arc_t = back_edge;
    }
  }
  color[node] = black;
}

static void
dfs(graph_ptr)
graph_p graph_ptr;
{

  /*
   * Standard depth first walk of graph.  Only purpose of walk
   * is to classify arcs.
   */
  unsigned char *color;

  color = (unsigned char *)db_malloc(graph_ptr->num_nodes);
  memset(color, 0, graph_ptr->num_nodes);

  dfs_walk(graph_ptr, color, 0);

  free(color);
}

static void
del_node(gp, node_num)
graph_p gp;
int node_num;
{
  int i;
  int old_num = gp->num_nodes;

#ifdef DEBUG
  printf ("deleting node %d\n", node_num);
#endif

  for (i = node_num+1; i < old_num; i++)
    gp->nodes[i-1] = gp->nodes[i];

  gp->num_nodes -= 1;

  /* renumber the arcs, and delete any arcs that refer to this node */
  for (i = 0; i < gp->num_arcs; i++)
  {
    if (gp->arcs[i].fm_node > node_num)
      gp->arcs[i].fm_node -= 1;
    else if (gp->arcs[i].fm_node == node_num)
      gp->arcs[i].state = state_deleted;

    if (gp->arcs[i].to_node > node_num)
      gp->arcs[i].to_node -= 1;
    else if (gp->arcs[i].to_node == node_num)
      gp->arcs[i].state = state_deleted;
  }
}

static void
add_node(gp, node_num)
graph_p gp;
int node_num;
{
  int i;
  int old_num = gp->num_nodes;

#ifdef DEBUG
  printf ("adding node %d\n", node_num);
#endif

  new_node(gp, old_num);

  /* move the nodes out of the way. */
  for (i = old_num - 1; i >= node_num; i--)
    gp->nodes[i+1] = gp->nodes[i];

  new_node(gp, node_num);
  gp->nodes[node_num].out_arcs = 0;
  gp->nodes[node_num].in_arcs  = 0;
  gp->nodes[node_num].node_id  = 0;
  gp->nodes[node_num].weight   = UNSET_WEIGHT;
  gp->nodes[node_num].state    = state_no_weight;

  /* renumber the node numbers in the graph's arcs */
  for (i = 0; i < gp->num_arcs; i++)
  {
    if (gp->arcs[i].fm_node >= node_num)
      gp->arcs[i].fm_node += 1;

    if (gp->arcs[i].to_node >= node_num)
      gp->arcs[i].to_node += 1;
  }
}

static void
node_vect(gp, vec)
graph_p gp;
int *vec;
{
  int i;

  /* do depth first search to get better information */
  dfs(gp);

  /* node 0 must always match. So set it to a funny number */
  vec[0] = 0xFFFFFFFF;

  for (i = 1; i < gp->num_nodes; i++)
  {
    int out_cnt = 0;
    int in_cnt = 0;
    int out_is_back = 0;
    arc_p arcs;

    for (arcs = gp->nodes[i].out_arcs; arcs != 0;
         arcs = arcs->out_arc_next)
    {
      out_cnt += 1;
      if (arcs->arc_t == back_edge)
        out_is_back = 1;
    }

#if 0
    for (arcs = gp->nodes[i].in_arcs; arcs != 0;
         arcs = arcs->in_arc_next)
      in_cnt += 1;
#endif

    /*
     * only interested in whether nodes has 0, 1, 2 or many out_arcs.
     * This corresponds roughly to direct jump, conditional jump, or
     * switch jump.
     */
    if (out_cnt > 3)
      out_cnt = 3;

    if (in_cnt > 2)
      in_cnt = 2;

    vec[i] = (gp->nodes[i].node_id << 5) |
             (out_cnt << 3) |
             (in_cnt << 1) |
             (out_is_back);
  }
}

static int
score_del_group(gp, cur_first, cur_last)
graph_p gp;
int cur_first;
int cur_last;
{
  int score = 0;
  int i;
  arc_p arcs;

  /*
   * return the number of arcs that these nodes have, that are either
   * to or from arcs outside of the range [cur_first,cur_last].
   */
  for (i = cur_first; i <= cur_last; i++)
  {
    for (arcs = gp->nodes[i].out_arcs; arcs != 0; arcs = arcs->out_arc_next)
      if (arcs->to_node < cur_first || arcs->to_node > cur_last)
        score += 1;

    for (arcs = gp->nodes[i].in_arcs; arcs != 0; arcs = arcs->in_arc_next)
      if (arcs->fm_node < cur_first || arcs->fm_node > cur_last)
        score += 1;
  }

  return score;
}

static void
move_deletions(gp, vec, delete)
graph_p gp;
int *vec;
unsigned char *delete;
{
  /*
   * Move groups of deletions so as to minimize the number of arcs coming
   * into and going out of the group.  Objective is to have 1 arc going into
   * group and 1 arc coming into the group.
   *
   * The reason for doing this is that its more likely that code was added
   * as a connected block, rather than randomly around the graph.
   *
   * Also we only do this where the nodes on each edge of the deletion
   * are equivalent, so that there is some kind of a "judgement" decision
   * about which edit is the best.
   *
   * Key is that after the group is deleted, the remaining nodes in the
   * vector must be equivalent.
   */
  int first;
  int last;
  int change = 1;

  /* lets do this until it settles. */
  while (change)
  {
    change = 0;

    /* first find a delete group */
    for (first = 0; first < gp->num_nodes; first++)
    {
      if (delete[first])
      {
        int min_arcs;
        int cur_arcs;
        int best_first;
        int best_last;
        int cur_first;
        int cur_last;

        for (last = first+1; last < gp->num_nodes; last++)
          if (!delete[last])
            break;
        last -= 1;

        /* first, last define group of deletes inclusively */

        /*
         * first see if we can move this group, if we can't then
         * there is no sense even scoring it.
         */
        if ((first > 0 && vec[first-1] == vec[last]) ||
            (last < gp->num_nodes-1 && vec[first] == vec[last+1]))
        {
          min_arcs = score_del_group(gp, first, last);
          best_first = first;
          best_last = last;

          /*
           * Try moving the delete group back.
           * Check delete[cur_first-1] to keep from
           * moving this delete group back on top of another one.
           * Butting them together is OK though.
           */
          cur_first = first;
          cur_last  = last;
          while (cur_first > 0 &&
                 delete[cur_first-1] == 0 &&
                 vec[cur_first-1] == vec[cur_last] &&
                 min_arcs > 2)
          {
            cur_first -= 1;
            cur_last -= 1;

            cur_arcs = score_del_group(gp, cur_first, cur_last);

            if (cur_arcs < min_arcs)
            {
              min_arcs = cur_arcs;
              best_first = cur_first;
              best_last = cur_last;
            }
          }

          /*
           * Try moving the delete group forward.
           * Check delete[cur_last+1] to keep from
           * moving this delete group forward on top of another one.
           * Butting them together is OK though.
           */
          cur_first = first;
          cur_last  = last;
          while (cur_last < gp->num_nodes-1 &&
                 delete[cur_last+1] == 0 &&
                 vec[cur_first] == vec[cur_last+1] &&
                 min_arcs > 2)
          {
            cur_first += 1;
            cur_last += 1;

            cur_arcs = score_del_group(gp, cur_first, cur_last);

            if (cur_arcs < min_arcs)
            {
              min_arcs = cur_arcs;
              best_first = cur_first;
              best_last = cur_last;
            }
          }

          if (best_first != first)
          {
            int i;
            for (i = first; i <= last; i++)
              delete[i] = 0;

            for (i = best_first; i <= best_last; i++)
              delete[i] = 1;

            change = 1;
            last = best_last;
          }
        }
        /*
         * Set first to last so that at next iteration, first is one
         * past the end of this delete group.
         */
        first = last;
      }
    }
  }
}

static int
create_node_correspondence(gp1, gp2)
graph_p gp1;
graph_p gp2;
{
  int *vec1;
  int *vec2;
  unsigned char *change1;
  unsigned char *change2;
  int i;
  int new_node;
  int gp1_nodes = gp1->num_nodes;
  int gp2_nodes = gp2->num_nodes;
  int nodes_diff = 0;

  vec1 = (int *)db_malloc((gp1_nodes + gp2_nodes) * sizeof(int));
  vec2 = vec1 + gp1_nodes;
  change1 = (unsigned char *)db_malloc((gp1_nodes+1)+(gp2_nodes+1));
  change2 = change1 + (gp1_nodes+1);

  /* first form 2 vectors of nodes.  We will use this vector
   * for diffing the nodes to find out which nodes need to be
   * added to or deleted from gp1 to make it match gp2.
   */
  node_vect(gp1, vec1);
  node_vect(gp2, vec2);

  /*
   * Now diff the vectors. changed1 and changed2 indicate
   * which nodes need to be added/deleted to accomplish the
   * correspondence needed.
   */
  pf_vdiff(vec1, vec2, change1, change2, gp1_nodes, gp2_nodes);

  /*
   * move groups of deletions to minimize arcs in and out of the
   * group.
   */
  move_deletions(gp1, vec1, change1);
  move_deletions(gp2, vec2, change2);

  /*
   * figure out which ones to add and which ones to delete
   */
  new_node = 0;
  for (i = 0, new_node; i < gp1_nodes; i++, new_node++)
  {
    if (change1[i])
    {
      del_node(gp1, new_node);
      new_node--;
      nodes_diff += 1;
    }
  }

  for (i = 0; i < gp2_nodes; i++)
  {
    if (change2[i])
    {
      add_node(gp1, i);
      nodes_diff += 1;
    }
  }

  free(vec1);
  free(change1);

  return nodes_diff;
}

static void
arc_val(gp, arc_no, fm_p, to_p)
graph_p gp;
int arc_no;
int *fm_p;
int *to_p;
{
  if (arc_no < gp->num_arcs)
  {
    *fm_p = gp->arcs[arc_no].fm_node;
    *to_p = gp->arcs[arc_no].to_node;
  }
  else
  {
    *fm_p = gp->num_nodes;
    *to_p = gp->num_nodes;
  }
}

static void
unweight_node(gp, node, follow_arcs)
graph_p gp;
int node;
int follow_arcs;
{
  arc_p arcs;
  /*
   * unweight node and any arcs coming into or going out of it.
   */
  gp->nodes[node].weight = UNSET_WEIGHT;
  gp->nodes[node].state = state_no_weight;

  for (arcs = gp->nodes[node].out_arcs; arcs != 0; arcs = arcs->out_arc_next)
  {
    if (arcs->state != state_deleted)
    {
      arcs->weight = UNSET_WEIGHT;
      arcs->state = state_no_weight;
    }
  }

  for (arcs = gp->nodes[node].in_arcs; arcs != 0; arcs = arcs->in_arc_next)
  {
    if (arcs->state != state_deleted)
    {
      arcs->weight = UNSET_WEIGHT;
      arcs->state = state_no_weight;
    }
  }
}

static void
del_arc(gp, arc_no)
graph_p gp;
int arc_no;
{
  int fm = gp->arcs[arc_no].fm_node;
  int to = gp->arcs[arc_no].to_node;
#ifdef DEBUG
  printf ("deleting arc from %d to %d\n", fm, to);
#endif

  if (fm != 0)
    unweight_node(gp, fm, 0);

  if (to != 0)
    unweight_node(gp, to, 0);
    
  gp->arcs[arc_no].state = state_deleted;
}

static void
add_arc(gp, fm, to)
graph_p gp;
int fm;
int to;
{
  int arc_no = gp->num_arcs;

#ifdef DEBUG
  printf ("adding arc from %d to %d\n", fm, to);
#endif

  new_arc(gp, arc_no);
  gp->arcs[arc_no].fm_node = fm;
  gp->arcs[arc_no].to_node = to;
  gp->arcs[arc_no].arc_id  = 0;
  gp->arcs[arc_no].arc_t = 0;
  gp->arcs[arc_no].out_arc_next = 0;
  gp->arcs[arc_no].in_arc_next = 0;
  gp->arcs[arc_no].weight = UNSET_WEIGHT;
  gp->arcs[arc_no].state  = state_no_weight;

  if (fm != 0)
    unweight_node(gp, fm, 0);

  if (to != 0)
    unweight_node(gp, to, 0);
}

static int
add_del_arcs(gp1, gp2)
graph_p gp1;
graph_p gp2;
{
  int gp1_i = 0;
  int gp2_i = 0;
  int gp1_narcs;
  int gp2_narcs;
  int arcs_diff = 0;

  sort_arcs(gp1);
  sort_arcs(gp2);

  /*
   * These have to be set here, sort_arcs may get rid of some
   * arcs that were deleted, but not reflected as such in the
   * num_arcs field.
   */
  gp1_narcs = gp1->num_arcs;
  gp2_narcs = gp2->num_arcs;

  while (gp1_i < gp1_narcs || gp2_i < gp2_narcs)
  {
    int gp1_fm;
    int gp1_to;

    int gp2_fm;
    int gp2_to;

    arc_val(gp1, gp1_i, &gp1_fm, &gp1_to);
    arc_val(gp2, gp2_i, &gp2_fm, &gp2_to);

    if (gp1_fm < gp2_fm ||
        (gp1_fm == gp2_fm && gp1_to < gp2_to))
    {
      del_arc(gp1, gp1_i);
      gp1_i += 1;
      arcs_diff += 1;
    }
    else if (gp1_fm > gp2_fm ||
        (gp1_fm == gp2_fm && gp1_to > gp2_to))
    {
      add_arc(gp1, gp2_fm, gp2_to);
      gp2_i += 1;
      arcs_diff += 1;
    }
    else
    {
      gp1_i += 1;
      gp2_i += 1;
    }
  }

  sort_arcs(gp1);
  return arcs_diff;
}

static void
compute_node_multipliers(gp1)
graph_p gp1;
{
  int i;
  arc_p arcs;

  for (i = 0; i < gp1->num_nodes; i++)
  {
    if (gp1->nodes[i].state == state_no_weight)
    {
      int can_mult = 0;
      for (arcs = gp1->nodes[i].in_arcs; arcs != 0; arcs = arcs->in_arc_next)
      {
        if (arcs->arc_t == back_edge)
        {
          can_mult = 1;
          break;
        }
      }

      if (can_mult)
      {
        gp1->nodes[i].weight = LOOP_WEIGHT;
        gp1->nodes[i].state  = state_multiplier;
      }
    }
  }
}

static void
estimate_arc_weights(gp1)
graph_p gp1;
{
  int i;
  arc_p arcs1;

  for (i = 0; i < gp1->num_arcs; i++)
  {
    if (gp1->arcs[i].state == state_no_weight &&
        gp1->nodes[gp1->arcs[i].fm_node].state == state_weighted)
    {
      double weight = gp1->nodes[gp1->arcs[i].fm_node].weight;
      int n_unknown_arcs = 0;

      for (arcs1 = gp1->nodes[gp1->arcs[i].fm_node].out_arcs;
           arcs1 != 0; arcs1 = arcs1->out_arc_next)
      {
        if (arcs1->state == state_no_weight)
          n_unknown_arcs += 1;
        else
          weight -= arcs1->weight;
      }

      weight = weight / (double)n_unknown_arcs;

      for (arcs1 = gp1->nodes[gp1->arcs[i].fm_node].out_arcs;
           arcs1 != 0; arcs1 = arcs1->out_arc_next)
      {
        if (arcs1->state == state_no_weight)
        {
          arcs1->weight = weight;
          arcs1->state  = state_weighted;
        }
      }
    }
  }
}

#ifdef DEBUG
/*
 * This routine is only advisory.  In some cases, the graph weighting
 * algorithm will not give a weighting that statisfies this routine.
 * This can happen because of many factors.  Two of these are imperfection
 * with transforming the graph in the sense of which nodes/arcs are reused.
 * Another is that the algorithm itself can lead to impossible situations,
 * due to constraints that weighting one part of the graph can have on
 * legal weightings of another part of the graph.
 */
static void
check_weights(gp)
graph_p gp;
{
  /*
   * check all the node weights to make sure they add up to the
   * outgoing an incoming arc weights.
   */
  int i;
  double weight;
  arc_p arcs;

  for (i = 0; i < gp->num_nodes; i++)
  {
    weight = 0.0;
    for (arcs = gp->nodes[i].in_arcs; arcs != 0; arcs = arcs->in_arc_next)
      weight += arcs->weight;

    if ((int)weight != (int)gp->nodes[i].weight)
      db_warning("node %d weight %f in_arc_weight %f\n",
                 i, gp->nodes[i].weight, weight);

    weight = 0.0;
    for (arcs = gp->nodes[i].out_arcs; arcs != 0; arcs = arcs->out_arc_next)
      weight += arcs->weight;

    if ((int)weight != (int)gp->nodes[i].weight)
      db_warning("node %d weight %f out_arc_weight %f\n",
                 i, gp->nodes[i].weight, weight);
  }
}
#endif

static unsigned
reweight_graph(old_quality, gp1, arcs_added_deleted, nodes_added_deleted)
unsigned old_quality;
graph_p gp1;
int arcs_added_deleted;
int nodes_added_deleted;
{
  int new_quality;
  int unweighted_arcs;
  int unweighted_nodes;
  int multi;
  int n;

  unweighted_arcs = weight_arcs(gp1);

  multi = 0xFFFF / gp1->num_arcs;
  new_quality = old_quality -
                ((arcs_added_deleted + nodes_added_deleted) * multi);
  new_quality -= unweighted_arcs;
  if (new_quality < 2)
    new_quality = 2;

  n = weight_nodes(gp1);
  unweighted_nodes = n  + 1;

  while (unweighted_nodes > n && n > 0)
  {
    unweighted_nodes = n;
    weight_arcs(gp1);
    n = weight_nodes(gp1);
  }

  if (unweighted_nodes > 0)
  {
    estimate_arc_weights(gp1);
    dfs(gp1);
    compute_node_multipliers(gp1);

    weight_arcs(gp1);
    while ((n = weight_nodes(gp1)) != 0)
    {
      weight_arcs(gp1);
      estimate_arc_weights(gp1);
    }
    /* one last weight arcs to get them all */
    weight_arcs(gp1);
  }

#ifdef DEBUG
  check_weights(gp1);
#endif
  return new_quality;
}

static unsigned
transform(old_quality, gp1, gp2)
unsigned old_quality;
graph_p gp1;
graph_p gp2;
{
  int new_quality;
  int nodes_diff;
  int arcs_diff;
  int i;

  /*
   * this will cause the necessary nodes to be added or deleted
   * such that node i in gp1 corresponds to node i in gp2.
   */
  nodes_diff = create_node_correspondence(gp1, gp2);

  /*
   * Now that the nodes correspond, we can simply sort the arcs,
   * and walk the list to figure out which arcs need to be added
   * or deleted.
   */
  arcs_diff = add_del_arcs(gp1, gp2);

  /*
   * Now we must rebuild graph gp1 from the arcs, but without losing
   * the already existing nodes.  This is to get the lists in the
   * nodes correct, and all the links between the arcs correct again.
   */
  graph_from_arcs(gp1);

  for (i = 0; i < gp1->num_nodes; i++)
  {
    if (gp1->nodes[i].state == state_no_weight)
      unweight_node(gp1, i, 1);
  }

  /*
   * now the graphs are the same.  However in the process we have
   * possibly created some arcs/nodes for which profile information
   * is not available.  Synthesize this information.
   */
  new_quality = reweight_graph(old_quality, gp1, arcs_diff, nodes_diff);

  return new_quality;
}

static int
doub_to_int(d)
double d;
{
  if (d > 0x7fffffff)
    return 0x7fffffff;
  else
    return (int)d;
}

static void
graph_to_prof(gp, new_pf)
graph_p gp;
unsigned char *new_pf;
{
  int i;

  /*
   * We start at one here because when the graph was created a dummy
   * entry node was created numbered 0, thus node 1 now corresponds
   * to what was originally node 0. Starting at 1 undoes this mapping
   * as we create the new profile.
   */
  for (i = 1; i < gp->num_nodes; i++)
  {
    unsigned long t = doub_to_int(gp->nodes[i].weight);
    CI_U32_TO_BUF(new_pf, t);
    new_pf += 4;
  }
}

unsigned
stretch_fdef_prof (fdef, new_fdef)
st_node* fdef;
st_node* new_fdef;
{
  /* Stretch the profile associated with function 'fdef' to fit function
     'out' and return an estimate of the quality of the resultant profile
     information.

     A quality level of 0 means no profile info is available; 0xffff means
     we have unstretched numbers.  The quality level of 'fdef' never
     improves in this routine; it only stays the same or gets worse.

     The caller is going to assume that after calling this routine to make
     a group of fdefs with the same name (representing perhaps different
     versions of the same function) fit the same new graph, he can additively
     merge the profile information for those fdefs whose quality level is
     similiar (see db_fdef_prof_quality).

     Of the lists in a CI_FDEF record, only the cfg and profile counts are to
     be relied upon in this routine.  When more info needs to be kept in the
     fdefs for spf files, the routine 'del_fmt' in gmpf960.c must be changed;
     this routine causes the deletion of unwanted lists when spf files are
     written out. */

  graph in_graph;
  graph in1_graph;

  db_list_node* fdef_prof = GET_FDEF_PROF (fdef);
  db_list_node* new_fdef_prof = GET_FDEF_PROF (new_fdef);

  unsigned char *fdef_graf = db_fdef_cfg (fdef);
  unsigned char *new_fdef_graf = db_fdef_cfg (new_fdef);

  int fdef_ncounts = fdef_prof_size (fdef) / 4;
  int fdef_quality = fdef_prof_quality (fdef);
  int new_quality;

  int new_graf_len, fdef_graf_len;

  assert ((fdef_ncounts > 0) == (fdef_quality > 0));

  CI_U32_FM_BUF (new_fdef_graf, new_graf_len);
  CI_U32_FM_BUF (fdef_graf, fdef_graf_len);

  /* make a quick check to see if graphs are the same. */
  if (fdef_quality == 0 ||
      (new_graf_len == fdef_graf_len &&
       memcmp(new_fdef_graf, fdef_graf, new_graf_len) == 0))
  {
    /*
     * functions have the same graphs, just make new_fdef_prof
     * be fdef_prof.
     */
    new_fdef_prof = fdef_prof;
    new_quality = fdef_quality;
  }
  else
  {
    graph_init(&in_graph);
    read_arcs(&in_graph, fdef_graf);
    sort_arcs(&in_graph);
    node_init(&in_graph);
    graph_from_arcs(&in_graph);
    weight_nodes_from_pf(&in_graph, fdef_prof->text + 4);
    weight_arcs(&in_graph);

    graph_init(&in1_graph);
    read_arcs(&in1_graph, new_fdef_graf);
    sort_arcs(&in1_graph);
    node_init(&in1_graph);
    graph_from_arcs(&in1_graph);

#if 0
    db_warning("stretching profile for function %s", fdef->name);
#endif
    new_quality = transform(fdef_quality, &in_graph, &in1_graph);
    graph_to_prof(&in_graph, new_fdef_prof->text + 4);

    free_graph(&in_graph);
    free_graph(&in1_graph);
  }

  if (new_fdef_prof != fdef_prof)
    SET_FDEF_PROF (fdef, new_fdef_prof);

  if (fdef_quality != new_quality)
  {
    if (new_quality == 0)
      set_fdef_prof_voters  (fdef, 0);

    set_fdef_prof_quality (fdef, new_quality);
  }

  return new_quality;
}
