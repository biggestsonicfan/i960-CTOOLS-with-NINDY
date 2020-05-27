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
/* $Id: gcovgrph.h,v 1.4 1995/02/08 19:40:45 timc Exp $ */

/*
 * gcovgrph.h
 *
 *   typedefs, defines, and prototypes for dealing with call graphs.
 */

typedef struct cgn {
  double node_count;        /* execution count for this node */
  char *file_name;          /* name of the file where this function defined */
  st_node *sym;             /* pointer to the call graph node's symbol */
  int n_calls_out;          /* static number of calls from this routine */
  int n_calls_in;           /* static number of known calls to this routine */
  int first_bb;             /* first basic block for this function. */
  int last_bb;              /* last basic block for this function. */
  struct cgarc *call_out_list; /* list of arcs for called routines */
  struct cgarc *call_in_list;  /* list of arcs for incoming calls */
  unsigned char addr_taken;
  unsigned char deletable;
  unsigned char traversed;
} call_graph_node;

#define CGN_P(X) ((call_graph_node *)((X)->extra))

typedef struct cgarc {
  struct cgarc * in_arc_next;  /* ptr to next elt on incoming call arc list */
  struct cgarc * out_arc_next; /* ptr to next elt on outgoing call arc list */
  call_graph_node * call_to;   /* ptr to call graph node for callees */
  call_graph_node * call_fm;   /* ptr to call graph node for callers */
  double arc_count;            /* number of times this arc is executed */
  unsigned int arc_id;         /* unique number for qsort consistency */
} call_graph_arc;

typedef struct {
  int num_cg_nodes;                /* number of nodes in call graph */
  int num_cg_arcs;                 /* number of arcs in call graph */
  call_graph_node * cg_nodes;      /* array of nodes in call graph */
  call_graph_arc * cg_arcs;        /* array of arcs in call graph */
  call_graph_node * main_node;     /* the head of the call graph */
  double call_count;               /* total dynamic calls in the graph */
} call_graph;

void prof_call_count();
call_graph *construct_call_graph();
void display_call_graph();
