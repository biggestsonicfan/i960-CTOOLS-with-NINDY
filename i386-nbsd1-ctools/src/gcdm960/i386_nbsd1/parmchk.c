/******************************************************************/
/*       Copyright (c) 1995 Intel Corporation

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

/*
 * check_call_parms checks the parameter information of the callee against
 * what is actually being passed, and determines whether of not this
 * call-arc may be a candidate for inlining.  It also may print a warning.
 * It returns 0 if this call-arc may not be inlined, 1 otherwise.
 */
static int
check_call_parms(fm_p, to_p, formal_info, actual_info, call_num)
call_graph_node *fm_p;
call_graph_node *to_p;
unsigned char *formal_info;
unsigned char *actual_info;
int call_num;
{
  int formal_is_varargs = db_parm_is_varargs(formal_info);
  int actual_is_unknown = db_parm_is_varargs(actual_info);
  int formal_len = db_parm_len(formal_info);
  int actual_len = db_parm_len(actual_info);

  if (formal_is_varargs || actual_is_unknown)
  {
    return 0;
  }

  if (formal_len != actual_len)
  {
#if 0
    db_warning ("parameter number mismatch in call from %s to %s.",
                fm_p->sym->name, to_p->sym->name);
#endif
    return 0;
  }

  /* ignore return types for now */
  formal_info += 4;
  actual_info += 4;
  if (memcmp(formal_info, actual_info, formal_len-4) != 0)
  {
#if 0
    db_warning ("parameter type mismatch in call from %s to %s.",
                fm_p->sym->name, to_p->sym->name);
#endif
    return 0;
  }

  /* now check return type specially, for now  */
  formal_info -= 2;
  actual_info -= 2;
  if (formal_info[0] != actual_info[0] || formal_info[1] != actual_info[1])
  {
#if 0
    db_warning ("return type mismatch in call from %s to %s.",
                fm_p->sym->name, to_p->sym->name);
#endif
    return 0;
  }

  return 1;
}

void
check_all_calls (cg)
call_graph *cg;
{
  call_graph_node *cgn_p = cg->cg_nodes;
  call_graph_node *quit  = &cgn_p[cg->num_cg_nodes];


  for (cgn_p = cg->cg_nodes; cgn_p < quit; cgn_p++)
  {
    unsigned char *actual_info;
    unsigned char *begin_actual_info;
    call_graph_arc * arc_p;
    int j;
    
    begin_actual_info = db_fdef_call_ptype(cgn_p->sym);

    for (arc_p = cgn_p->call_out_list, j = 0,
           actual_info = db_fdef_next_ptype_info(begin_actual_info, 0);
         arc_p != 0;
         arc_p = arc_p->out_arc_next, j++,
           actual_info = db_fdef_next_ptype_info(begin_actual_info, actual_info))
    {
      unsigned char *formal_info;
      int t;

      formal_info = db_fdef_ftype(arc_p->call_to->sym);
      t = check_call_parms(cgn_p, arc_p->call_to, formal_info, actual_info, j);
      if (t == 0)
        arc_p->inlinable = 0;
    }
  }
}
