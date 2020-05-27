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
#ifndef CALLGRPH_H
#define CALLGRPH_H

typedef struct cgn
{
  double node_count;        /* execution count for this node */

  st_node *sym;             /* pointer to the call graph node's symbol */
  char *file_name;          /* name of the file where this function defined */
  int n_calls_out;          /* static number of calls from this routine */
  int n_calls_in;           /* static number of known calls to this routine */
  int first_bb;             /* first basic block for this function. */
  int last_bb;              /* last basic block for this function. */
  struct cgarc *call_out_list; /* list of arcs for called routines */
  struct cgarc *call_in_list;  /* list of arcs for incoming calls */
  short node_insns;         /* rough number of insns in the function */
  short node_parms;         /* number of parms the function has */
  short node_height;        /* height of node from a leaf node.
                               1 for a leaf node, > 1 for a non-leaf node */
  short inln_calls;         /* number of non recursive calls that are inlined 
                               from this routine. */
  short recur_inln_calls;   /* total number of inlined directly
                               recursive calls from this routine. */
  unsigned char n_regs;     /* number of register used before inline. */
  unsigned char n_regs_aft; /* number of regs used after all inlining. */

  unsigned char addr_taken;
  unsigned char deletable;
  unsigned char head_of_cycle;
  unsigned char inlinable;  /* is node inlinable? */
  unsigned char visited;
  char is_recomp;
} call_graph_node;

typedef struct cgarc
{
  struct cgarc * in_arc_next;  /* pointer to next element on incoming call
                                  arc list */
  struct cgarc * out_arc_next; /* pointer to next element on outgoing call
                                  arc list */
  call_graph_node * call_to;   /* pointer to call graph node for routine
                                  this arc calls */
  call_graph_node * call_fm;   /* pointer to call graph node for routine
                                  this arc comes from */
  double arc_count;            /* number of times this arc is executed
                                  in running compiler. */
  unsigned int arc_id;         /* unique number for qsort consistency across
                                  hosts */
  unsigned int site_id;        /* number of call site in calling function */
  int inlinable;	       /* should this arc be inlined? */
  unsigned char regs_live;     /* number of registers live across this arc */
  unsigned char visited;       /* prevent recursion when doing depth-first */
  unsigned char is_back_arc;   /* indicates if this is back-edge in call-graph
                                  to recursive function */
  int cost;
} call_graph_arc;

typedef struct
{
  int num_cg_nodes;                /* number of nodes in call graph */
  int num_cg_arcs;                 /* number of arcs in call graph */
  call_graph_node * cg_nodes;      /* array of nodes in call graph */
  call_graph_arc * cg_arcs;        /* array of arcs in call graph */
  call_graph_node * main_node;     /* the head of the call graph */
  double call_count;               /* total dynamic calls in the graph */
} call_graph;

#define CGN_P(X) (( *((call_graph_node **) ((X)->extra + sizeof(int))) ))

extern int cur_arc_id;

extern call_graph *
construct_call_graph DB_PROTO((void));

extern void
destroy_call_graph DB_PROTO((call_graph *cg));

extern void
print_cg_closure DB_PROTO((call_graph *cg));

extern void
print_call_graph DB_PROTO((call_graph *cg, char *msg));

extern void
prune_deletable_nodes DB_PROTO((call_graph *cg));

extern double
d_to_d DB_PROTO((double in));

#endif
