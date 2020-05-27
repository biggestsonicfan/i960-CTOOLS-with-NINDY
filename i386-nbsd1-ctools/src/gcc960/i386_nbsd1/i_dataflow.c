#include "config.h"

#ifdef IMSTG
/*
  This file will be part of GNU CC.  Until then, it is the
  property of Intel Corporation, Copyright (C) 1991.
  
  GNU CC is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 1, or (at your option)
  any later version.
  
  GNU CC is distributed in the hope that it will be useful,
  But WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with GNU CC; see the file COPYING.  If not, write to
  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "rtl.h"
#include "expr.h"
#include "regs.h"
#include "flags.h"
#include "assert.h"
#include <stdio.h>
#include "tree.h"
#if defined(WIN95)
#include <varargs.h>
#else
#include "gvarargs.h"
#endif

#ifdef GCC20
#include "i_jmp_buf_str.h"
#include "basic-block.h"
#include "insn-config.h"
#include "insn-flags.h"
#include "hard-reg-set.h"
#include "i_dataflow.h"
#else
#include "jmp_buf.h"
#include "basicblk.h"
#include "insn_cfg.h"
#include "insn_flg.h"
#include "hreg_set.h"
#include "dataflow.h"
#endif

#include "recog.h"

#define df_assert(x) assert(x)

#define IDT_NONE   0
#define IDT_MISC   1

#define IDT_RD     16 
#define IDT_EQUIV    21
#define IDT_RESTRICT 22

#define IDT_CEQUIV   31

#define IDT_SCAN     40

#define IDT_ADDR     60

#define IDT_REFINE   70

#define IDT_REBUILD  80

#define IDT_TYPE     90

#define IDT_MEM      100

int dataflow_time_vect[512];
int dataflow_time_count[512];
char* dataflow_time_name[512];

void
init_df_time()
{
  extern char* dataflow_time_name[];

  dataflow_time_name[IDT_NONE]      = "non-dataflow";
  dataflow_time_name[IDT_MISC+0]    = "misc dataflow.0";
  dataflow_time_name[IDT_MISC+1]    = "misc dataflow.1";
  dataflow_time_name[IDT_MISC+2]    = "misc dataflow.2";
  dataflow_time_name[IDT_MISC+3]    = "misc dataflow.3";
  dataflow_time_name[IDT_RD+0]      = "do_reaching_defs(sym)";
  dataflow_time_name[IDT_RD+1]      = "do_reaching_defs(reg)";
  dataflow_time_name[IDT_RD+2]      = "do_reaching_defs(mem)";
  dataflow_time_name[IDT_REFINE+0]  = "refine_mem_info.0";
  dataflow_time_name[IDT_REFINE+1]  = "refine_mem_info.1";
  dataflow_time_name[IDT_REFINE+2]  = "refine_mem_info.2";
  dataflow_time_name[IDT_REFINE+3]  = "refine_mem_info.3";
  dataflow_time_name[IDT_REFINE+4]  = "refine_mem_info.4";
  dataflow_time_name[IDT_REFINE+5]  = "refine_mem_info.5";
  dataflow_time_name[IDT_REFINE+6]  = "refine_mem_info.6";
  dataflow_time_name[IDT_REBUILD]   = "rebuild_slot_pts_to";
  dataflow_time_name[IDT_EQUIV]     = "equivalences";
  dataflow_time_name[IDT_RESTRICT+0]= "restrict_pts_to.0";
  dataflow_time_name[IDT_RESTRICT+1]= "restrict_pts_to.1";
  dataflow_time_name[IDT_RESTRICT+2]= "restrict_pts_to.2";
  dataflow_time_name[IDT_CEQUIV+0]  = "compute_equiv(sym)";
  dataflow_time_name[IDT_CEQUIV+1]  = "compute_equiv(reg)";
  dataflow_time_name[IDT_CEQUIV+2]  = "compute_equiv(mem)";
  dataflow_time_name[IDT_SCAN+0]    = "superflow_scan.0";
  dataflow_time_name[IDT_SCAN+1]    = "superflow_scan.1";
  dataflow_time_name[IDT_SCAN+2]    = "superflow_scan.2";
  dataflow_time_name[IDT_ADDR]      = "add_addr_term";
  dataflow_time_name[IDT_TYPE]      = "type_overlap";
  dataflow_time_name[IDT_MEM]       = "misc_mem_stuff";
}

void
print_df_time()
{ int i,lasti;

  lasti = 0;


  init_df_time();
  init_shadow_time();

#ifdef TMC_NOISY
  for (i=0; i<sizeof(dataflow_time_vect)/sizeof(dataflow_time_vect[0]);i++)
#else
  i = 0;
#endif
  {
    if (dataflow_time_name[i])
      lasti = i;

    if (dataflow_time_count[i] ||dataflow_time_name[i] || dataflow_time_vect[i])
    { fprintf (stderr, "%d calls; ", dataflow_time_count[i]);
      if (dataflow_time_name[i])
        print_time (dataflow_time_name[i], dataflow_time_vect[i]);
      else
      { char buf[255];
        sprintf (buf, "%s+%d", dataflow_time_name[lasti],i-lasti);
        print_time (buf, dataflow_time_vect[i]);
      }
    }
  }
}

#ifdef TMC_NOISY
int df_assign_run_time (new_index)
int new_index;
{
  static int cur_index, cur_time, call_number;
  int new_time, old_index;

  call_number++;

  if (cur_time == 0)
    cur_time = get_run_time();

  new_time  = get_run_time();
  old_index = cur_index;
  cur_index = new_index;

  dataflow_time_vect[old_index] += new_time-cur_time;
  dataflow_time_count[old_index]++;
  cur_time = new_time;

  return old_index;
}
#endif

unsigned last_xmalloc_report;

db_rec db_trace[(int)(init_trace+1)];

df_global_info df_data;

ud_info df_info[(int)NUM_DFK];
mem_slot_info empty_slot;
mem_slot_info* mem_slots;

/* Record the last asm vector we scan so we can avoid re-entering
   it in scan_rtx_for_uds. */

static struct rtvec_def *last_asm_vect;

#ifdef TMC_FLOD
#define LAST_FLOW_DEF(I) (I)->attr_sets_last[I_FLOD]
#else
#define LAST_FLOW_DEF(I) (I)->attr_sets_last[I_DEF]
#endif

char* attr_names[] = { "KILL", "DEF", "USE", "ADDR", "CALL", "FLOD", "INACTIVE" };

char* df_names[]   = { "sym", "reg", "mem", "???" };
int max_df_insn = 2500;
int iid_gap = 1;

rtx user_ud_position();
rtx optim_rtx();

static int dataflow();
int df_time;

#define LT_ROW_EQ(T,N,M) (( lt_row_eq(&(T),M,N) ))

int
lt_row_eq (lt,n,m)
lt_rec *lt;
int n;
int m;
{
  int i;

  for (i=0; i<NUM_DFK; i++)
  {
    char* p = LT_VECT(*lt, n, i);
    char* q = LT_VECT(*lt, m, i);

    int *pi, *qi;
    int   l = lt->lt_len[i];
    int   j,v;

    while (((unsigned) p) & (sizeof(int)-1))
    { if (*p++ != *q++)
        return 0;
      l--;
    }

    pi = (int *) p;
    qi = (int *) q;

    while (l > 0)
    { if (*pi++ != *qi++)
        return 0;
      l -= sizeof (int);
    }
  }

  return 1;
}

int push_lt (lt)
lt_rec* lt;
{
  if (lt->alloc_lt == lt->next_lt)
  { lt->alloc_lt += 16;
    lt->linear_terms = (signed_byte*) xrealloc (lt->linear_terms, lt->alloc_lt * LT_SIZE(*lt));
  }

  bzero (LT_VECT (*lt, lt->next_lt, 0), LT_SIZE(*lt));
  
  return lt->next_lt++;
}

int
map_lt (lt,df,u)
lt_rec* lt;
ud_info* df;
int u;
{
  int ret;

  if ((ret=u) != 0)
    if ((ret=lt->map_lt[df-df_info][u]) == 0)
    { ret = lt->lt_len[df-df_info]++;
      lt->map_lt[df-df_info][u] = ret;
      lt->unmap_lt[df-df_info][ret] = u;
    }

  return ret;
}

int
unmap_lt (lt,df,i)
lt_rec* lt;
ud_info* df;
int i;
{
  return lt->unmap_lt[df-df_info][i];
}

init_lt(lt)
lt_rec* lt;
{
  /* We try to reuse the LT space if it's still around. */

  { /* Do allocation for LT array space ... */
    int lt_size = LT_SIZE(*lt);

    /* Get totals of all uds to set up lt tables. */
    bzero (lt->n_lt, sizeof (lt->n_lt));

    { int dfk;
      for (dfk = 0; dfk < INUM_DFK; dfk++)
      { lt->n_lt[dfk] = LT_SIZE(*lt);
        LT_SIZE(*lt) += NUM_UD (&df_info[dfk]);
        ALLOC_MAP (&(lt->map_lt[dfk]), NUM_UD(&df_info[dfk]), ushort_map);
        ALLOC_MAP (&(lt->unmap_lt[dfk]), NUM_UD(&df_info[dfk]), ushort_map);

        lt->lt_len[dfk] = 1;
      }
    }

    /* Round row size to multiple of 4 bytes ... */
    LT_SIZE(*lt) = ROUND (LT_SIZE(*lt), sizeof(int));

    /* Figure out how much space is allocated already ... */
    lt->alloc_lt = (lt->alloc_lt * lt_size) / LT_SIZE(*lt);
  }

  /* Reserve the 0 LT entry. */
  lt->next_lt = 0;
  push_lt (lt);
}

#define MEM_VPD df_info[IDF_MEM].vars_per_def

static tree
canon_type (t, np, cp)
tree t;
int* np;
enum tree_code* cp;
{
  int n;
  enum tree_code c;
  static int int_bytes, short_bytes;

  while (t && TREE_CODE(t)==ARRAY_TYPE)
    t = TREE_TYPE (t);

  if (int_bytes == 0)
  { extern tree integer_type_node;
    extern tree short_integer_type_node;

    int_bytes = int_size_in_bytes (integer_type_node);
    short_bytes = int_size_in_bytes (short_integer_type_node);
  }

  if (t)
  { c = TREE_CODE(t);
    n = int_size_in_bytes (t);

    switch (c)
    {
      case FUNCTION_TYPE:
        break;

      case RECORD_TYPE:
      case UNION_TYPE:
        if (n > 1)	/* 0 or 1 byte union/struct falls thru to unknown */
          break;

      default:
        t = 0;		/* Unknown type; will alias everything. */
        break;

      case REAL_TYPE:
        if (flag_int_alias_real)
        { n = int_bytes;
          goto integer_type;	/* Treat as full int */
        }
        else
          break;

      case POINTER_TYPE:
        if (flag_int_alias_ptr)
        { n = int_bytes;
          goto integer_type;	/* Treat as full int */
        }
        else
          break;

      case ENUMERAL_TYPE:
      case INTEGER_TYPE:
      integer_type:
        c = INTEGER_TYPE;

        if (flag_int_alias_short && n==short_bytes)
          n = int_bytes;	/* Treat short as full int */

        if (n <= 1)		/* Treat char as unknown */
          t = 0;
    }
  }

  if (t == 0)
  { c = VOID_TYPE;
    n = 1;
  }

  *cp = c;
  *np = n;

  return t;
}

object_types_could_alias (a, b)
tree a;
tree b;
{
  int na, nb;
  tree p;
  enum tree_code ca, cb;

  a = canon_type (a, &na, &ca);
  b = canon_type (b, &nb, &cb);

  if (ca==FUNCTION_TYPE || cb==FUNCTION_TYPE)
    return (ca == cb);

  /* Any object can be accessed via char or void */
  if (ca==VOID_TYPE || cb==VOID_TYPE)
    return 1;

  assert (na > 1 && nb > 1);
  
  if (na == nb && ca == cb)
    return 1;

  if (ca==RECORD_TYPE || ca==UNION_TYPE)
    for (p = TYPE_FIELDS(a); p != 0; p = TREE_CHAIN(p))
      if (object_types_could_alias (TREE_TYPE(p), b)) return 1;

  if (cb==RECORD_TYPE || cb==UNION_TYPE)
    for (p = TYPE_FIELDS(b); p != 0; p = TREE_CHAIN(p))
      if (object_types_could_alias (TREE_TYPE(p), a)) return 1;

  /*
     Strictly speaking, if the base types are not compatible,
     the only remaining case which has to be allowed is
     that ordinals can be accessed by their signed/unsigned
     counterparts; e.g, a short object can be accessed via
     *(unsigned short *).  See ANSII's appendix F.2
     "Undefined Behaviour" re accessing objects thru other
     than the objects declared type.

     We are a bit more conservative;  we say that objects
     are possible aliases if the basic type codes are the same
     and the sizes are the same.

     Eg: pointers alias all pointers,
         structs alias all structs of the same size,
         unions alias all unions of the same size,
         ordinals alias all ordinals of the same size,
         floats alias all floats of the same size.
  */

  return 0;
}

int
type_could_point_to (p, base)
tree p;
tree base;
{
  /*
     If an expression of type 'p' could be or could contain a pointer
     to a variable of type 'base', return 1;  otherwise, return 0.

     Please note, this is a somewhat different flavor of question
     than that answered by object_types_could_alias;  we have an
     expression type in hand, and we want to know if a certain flavor
     of pointer could live there.  The rules about convertability
     between pointers disallow many cases that would have to be
     allowed if we simply applied object_types_could_alias (*p, base).
  */
     
  /* Array types point at what their component type points at */
  while (p && TREE_CODE(p)==ARRAY_TYPE)
    p = TREE_TYPE (p);

  /* If we don't know anything about p, it could point to any type.*/
  if (p == 0 || TREE_CODE(p)==VOID_TYPE)
    return 1;

  /* Structs and unions point at everything their fields could point at. */
  if (TREE_CODE(p)==RECORD_TYPE || TREE_CODE(p)==UNION_TYPE)
  { for (p = TYPE_FIELDS(p); p != 0; p = TREE_CHAIN(p))
      if (type_could_point_to (TREE_TYPE(p), base))
        return 1;
    return 0;
  }

  if (TREE_CODE(p)!=POINTER_TYPE)
    return 0;

  /* return object_types_could_alias (TREE_TYPE(p), base); */
  return 1;
}

dump_df_stuff (f, df)
FILE* f;
ud_info* df;
{
  dump_df_map (f, "\n\ngen", df->gen, df);
  dump_df_map (f, "\n\nkill", df->kill, df);
  dump_df_map (f, "\n\nin", df->in, df);
  dump_df_map (f, "\n\nout", df->out, df);
}

trace(va_alist)
va_dcl
{
  va_list ap;

  db_rec* t;
  FILE* f;
  dbt_kind where;

  va_start (ap);
  where = va_arg (ap, dbt_kind);

  if (db_trace[(int)init_trace].cnt == 0)
  { int i;
    for (i = 0; i <= (int)init_trace; i++)
    { db_trace[i].cnt = 0;
      db_trace[i].kind = (dbt_kind)i;
    }
    db_trace[(int)init_trace].cnt = 1;
  }

  t = db_trace+(int)where;
  t->cnt++;

  if ((f = df_data.dump_stream) != 0)
  { char buf[128];
    char** n = df_names;

    fprintf (f, "\ntrace hit %d:%d; ", where, t->cnt);

    switch (where)
    { ud_info* info;
      mem_slot_info* s;
      rtx *r;
      int i,j,u,w;
      lt_rec* lt;
      rtx x, y;

      case superflow_entry:       fprintf(f, "superflow_entry");       break;
      case refine_info_entry:     fprintf(f, "refine_info_entry");     break;
      case refine_info_pt_change: fprintf(f, "refine_info_pt_change"); break;
      case refine_info_no_change: fprintf(f, "refine_info_no_change"); break;
      case restrict_pt_no_change: fprintf(f, "restrict_pt_no_change"); break;
      case bad_addr_term:         fprintf(f, "bad_addr_term");         break;
      case add_addr_replace:      fprintf(f, "add_addr_replace");      break;
      case addr_multi_reach:      fprintf(f, "addr_multi_reach");      break;
      case addr_isdef:            fprintf(f, "addr_isdef");            break;
      case addr_expand_ok:        fprintf(f, "exit addr expanded ok"); break;
      case addr_expand_failed:    fprintf(f, "addr_expand_failed");    break;
      case addr_unexpanable:      fprintf(f, "addr_unexpanable");      break;
      case addr_not_flow:         fprintf(f, "addr_not_flow");         break;

      case addr_added:
        u = va_arg (ap, int);
        fprintf (f, "added %d to LT's", u);
        break;

      case addr_exit:
        u = va_arg (ap, int);
        fprintf (f, "exit add_addr_term; addr_is_ok==%d", u);
        break;

      case addr_flow_equiv:
        u = va_arg (ap, int);
        fprintf (f, "equivalent to %d", u);
        break;

      case add_addr_term_entry:
        i = va_arg (ap, int);
        u = va_arg (ap, int);

        fprintf (f, "add_addr_term_entry insn %d, ud is %d", i, u);
        break;

      case refine_info_slot_change:
        fprintf(f, "refine_info_slot_change");
        dump_slot_table (f);
        fprintf (f, "\n");
        break;

      case restrict_pt_change:
        fprintf(f, "restrict_pt_change");
        for (info = df_info; info != df_info + INUM_DFK; info++)
        { fprintf (f, "\n");
          dump_pts_to (f, info);
        }
        fprintf (f, "\n");
        break;

      case defs_reaching_entry:
        info = va_arg (ap, ud_info*);

        fprintf (f, "defs_reaching_entry for %ss", n[info-df_info]);
        dump_df_stuff (f,info);
        fprintf (f, "\n");
        break;

      case defs_reaching_exit:
        info = va_arg (ap, ud_info*);

        fprintf (f, "defs_reaching_exit for %ss", n[info-df_info]);
        dump_flex_map (f, "\n\nRD", info->maps.defs_reaching,NUM_UD(info));
        dump_flex_map (f, "\n\nRU", info->maps.uses_reached, NUM_UD(info));
        fprintf (f, "\n");
        break;

      case update_sets_exit:
        info = va_arg (ap, ud_info*);

        fprintf (f, "\nupdate_sets_exit for %ss.  ud info:", n[info-df_info]);
        dump_ud_info (f, info);
        fprintf (f, "\n");
        break;

      case get_preds_exit:
        dump_sized_flex_map (f, "\n\npredecessors", df_data.maps.preds, N_BLOCKS);
        dump_flex_map (f, "\n\nsuccessors", df_data.maps.succs, N_BLOCKS);
        dump_flex_map (f, "\n\ndominators", df_data.maps.dominators, N_BLOCKS);
        dump_flex_map (f, "\n\nout_dominators", df_data.maps.out_dominators, N_BLOCKS);
        fprintf (f, "\n");
        break;

      case no_reaching_def:
      case no_reached_use:
      case no_type_info:
      { char buf[256];
        rtx note;

        info = va_arg (ap, ud_info*);
        u    = va_arg(ap,int);

        note = user_ud_position (info, u);

        if (where == no_reaching_def)
          sprintf (buf, "ud %d is uninitialized", u);
        else if (where == no_reached_use)
          sprintf (buf, "ud %d is never used", u);
        else if (where == no_type_info)
          sprintf (buf, "ud %d has no type", u);
        else
          df_assert (0);

        if (note)
          fprintf (f, "\n%s:line %d %s\n", NOTE_SOURCE_FILE(note), NOTE_LINE_NUMBER(note), buf);
        else
          fprintf (f, "\n%s:line %d %s\n", "", 0, buf);

        break;
      }

      case equiv_use:
        info = va_arg (ap, ud_info*);
        u    = va_arg(ap,int);
        w    = va_arg(ap,int);

        fprintf (f, "found equivalent %s uses %d,%d\n", n[info-df_info], u, w);
        break;

      case new_linear_terms:
        s  = va_arg (ap, mem_slot_info*);
        u  = va_arg(ap,int);
        lt = va_arg (ap, lt_rec*);

        fprintf (f, "new linear terms %d\n", u);
        dump_lt_vect (f, s[u].terms, lt);
        fprintf (f, "\n");
        break;

      case replace_ud_entry:
      case replace_ud_exit:
        x = va_arg (ap, rtx);
        y = va_arg (ap, rtx);
        fprintf (f, "replace_ud %d,%d\n", INSN_UID(x), INSN_UID(y));
        break;
    }
  }
  va_end (ap);
}

output_fmt_insn (va_alist)
va_dcl
{
  va_list ap;
  rtx* operands;
  char* format;
  char buf [256];

  va_start (ap);

  format   = va_arg (ap, char*);
  operands = va_arg (ap, rtx*);

  vsprintf (buf, format, ap);
  output_asm_insn (buf, operands);

  va_end(ap);
}
  

debug_rtl (start, stop)
rtx start;
rtx stop;
{
  if (start == 0)
    start = get_insns();

  while (start != stop)
  {
    fprintf (stderr,"%x:", start);
    debug_rtx (start);
    start = NEXT_INSN(start);
  }

  if (stop)
  {
    fprintf (stderr,"%x:", stop);
    debug_rtx (stop);
  }
}

void
get_ud_position (info, u, file, line)
ud_info* info;
int u;
char** file;
int *line;
{
  rtx note;

  if (note = user_ud_position (info, u))
  { *line = NOTE_LINE_NUMBER (note);
    *file = NOTE_SOURCE_FILE (note);
  }
  else
  { *line = 0;
    *file = 0;
  }
}

void
fmt_ud_position (info, u, buf)
ud_info* info;
int u;
char* buf;
{
  char* us;
  int ul;

  get_ud_position (info, u, &us, &ul);
  format_file_and_line (us, ul, buf);
}

df_warning (where, info, u, v)
dbt_kind where;
ud_info* info;
int u;
int v;
{
  char mbuf[256];
  int uline;
  char *ufile, *p;

  (p=mbuf)[0] = 0;

  switch (where)
  {
    default:
      info->ud_name (u, p);
      break;

    case no_reaching_def:
    case no_reached_use:
    case no_type_info:
      trace (where, info, u);

      if (where==no_reached_use)
      {
        strcpy (p, "this value of ");
        p += strlen (p);
        info->ud_name (u, p);
        p += strlen (p);
        strcpy (p, " is not used");
      }
      else if (where==no_reaching_def)
      {
        info->ud_name (u, p);
        p += strlen (p);
        strcpy (p, " is uninitialized");
      }
      else if (where==no_type_info)
      {
        strcpy (p, "this value of ");
        p += strlen (p);
        info->ud_name (u, p);
        p += strlen (p);
        strcpy (p, " has no type information");
      }
      else
        df_assert (0);

      break;

    case illegal_address:
    {
      info->ud_name (u, p);
      p += strlen (p);
      strcpy (p, " has an illegal address");
    }
    break;

    case illegal_overlap:
    { int vl;
      char* vs;

      info->ud_name (u, p);
      p += strlen (p);

      strcpy (p, " could have an illegal overlap");
      p += strlen (p);

      get_ud_position (info, v, &vs, &vl);
      if (vs || vl)
      { strcpy (p, " at ");
        p += strlen (p);
        format_file_and_line (vs, vl, p);
      }

      strcat (p, " (under ANSI section 3.3) ");
      break;
    }
  }

  get_ud_position (info, u, &ufile, &uline);
  warning_with_file_and_line (ufile, uline, 0, mbuf, 0, 0);
}

realloc_map (text, new_size, text_offset)
char** text;
int  new_size;
int text_offset;
{
  char* base;
  int  old_size;

  df_assert (text != 0 && new_size >= 0 && text_offset >= sizeof (int));

  /* If this map has not been deallocated, get the base of it,
     which is the address that we talk to realloc with.  The
     first bytes at the base contain the size, which had better
     be non-zero. */

  if ((base = *text) != 0)
  {
    base -= text_offset;
    old_size = *((int *)base);
    df_assert (old_size > 0);
  }
  else
    old_size = 0;

  if (new_size == 0)
  { if (base)
      free (base);
    *text = 0;
  }
  else 
  {
    base = (char*) xrealloc (base, new_size+text_offset);

    if (new_size > old_size)
      bzero (base + text_offset + old_size, new_size-old_size);

    *((int *)base) = new_size;
    *text = base + text_offset;
  }
}

alloc_map (text, new_size, text_offset)
char** text;
int  new_size;
int text_offset;
{
  char* base;
  int  old_size;

  df_assert (text != 0 && new_size >= 0 && text_offset >= sizeof (int));

  /* If this map has not been deallocated, get the base of it,
     which is the address that we talk to realloc with.  The
     first bytes at the base contain the size, which had better
     be non-zero. */

  if ((base = *text) != 0)
  {
    base -= text_offset;
    old_size = *((int *)base);
    df_assert (old_size > 0);
  }
  else
    old_size = 0;

  if (new_size == 0)
  { if (base)
      free (base);
    *text = 0;
  }
  else 
  {
    if (new_size > old_size || (old_size-new_size > 2048))
    {
      if (base)
        free (base);

      base = (char*) xmalloc (new_size+text_offset);
    }
    else
      new_size = old_size;

    bzero (base + text_offset, new_size);
    *((int *)base) = new_size;
    *text = base + text_offset;
  }
}

static ud_info *cur_sort;

#ifdef TMC_FLOD
int
reg_is_flod (d)
int d;
{
  int ret = 1;
  ud_info* df = df_info + IDF_REG;

  if ((UDN_ATTRS(df,d)&S_DEF)==0 || (df->maps.nuses[UDN_VARNO(df,d)]==0))
    ret = 0;
  else
    if (UDN_ATTRS(df,d) & S_KILL)
      ret = (df->maps.nkills[UDN_VARNO(df,d)] != 1);

  return ret;
}

int
mem_is_flod (d)
int d;
{
  int ret = 1;
  ud_info* df = df_info + IDF_MEM;

  if ((UDN_ATTRS(df,d)&S_DEF)==0)
    ret = 0;

  return ret;
}

int
sym_is_flod (d)
int d;
{
  int ret = 1;
  ud_info* df = df_info + IDF_SYM;

  if ((UDN_ATTRS(df,d)&S_DEF)==0)
    ret = 0;

  return ret;
}
#endif


int
order_uds (u1, u2)
ud_rec* u1;
ud_rec* u2;
{ /* Called from qsort for to order ud's. */

  int ret;
  extern int flag_df_order_by_var;

  int a1 = u1->attrs;
  int a2 = u2->attrs;

  /* inactives are always highest. */
  if (!(ret = (a1 & S_INACTIVE) - (a2 & S_INACTIVE)))

  /* by variable number. */
  if (!flag_df_order_by_var || !(ret = u1->varno - u2->varno))

  /* uses are higher than non-uses. */
  if (!(ret = (a1 & S_USE) - (a2 & S_USE)))

  /* defs are lower than non-defs. */
  if (!(ret = (a2 & S_DEF) - (a1 & S_DEF)))

#ifdef TMC_FLOD
  /* Flowable defs are lower than non-flowable defs */
  if (!(ret = (cur_sort->is_flod(u2-cur_sort->maps.ud_map)) -
              (cur_sort->is_flod(u1-cur_sort->maps.ud_map))))
#endif

  /* KILLS are the lowest among defs */
  if (!(ret = (a2 & S_KILL) - (a1 & S_KILL)))

  /* highest insns come last */
  if (!(ret = u2->insn - u1->insn))

  /* by variable number. */
  if (!(ret = u2->varno - u1->varno))

  /* by offset from variable base. */
  if (!(ret = u2->ud_var_offset - u1->ud_var_offset))

  /* Remaining tests are just for stability of sort when
     debugging and comparing dump files between runs or hosts. */

  if (!(ret = u2->ud_bytes - u1->ud_bytes))

  if (!(ret = u2->off - u1->off))

  if (!(ret = u2->attrs - u1->attrs))

  if (!(ret = u2->ud_align - u1->ud_align))

  if (!(ret = u2->ud_forw_align - u1->ud_forw_align))

  if (!(ret = u2->pts_at - u1->pts_at))
    ;

  return ret;
}

static
free_flow_space (info)
ud_info* info;
{
  /* Get rid of worn out flow information */
  ALLOC_MAP (&info->maps.equiv, 0, ud_map);
  ALLOC_MAP (&info->maps.defs_reaching, 0, flex_set_map);
  ALLOC_MAP (&info->maps.uses_reached, 0, flex_set_map);
  free_flex_pool (&info->pools.ud_pool);
  free_flex_pool (&info->pools.mondo_ud_pool);
}

#ifdef TMC_FLOD
count_kills (info)
ud_info* info;
{
  int n_id  = NUM_ID(info);
  ud_rec* p = info->maps.ud_map+NUM_UD(info);

  ALLOC_MAP (&info->maps.nkills, NUM_ID(info), uchar_map);
  ALLOC_MAP (&info->maps.nuses, NUM_ID(info), uchar_map);

  while (p-- != info->maps.ud_map)
  {
    unsigned char *c = 0;

    if (UD_ATTRS(*p) & S_KILL)
      c = info->maps.nkills;
    else if ((UD_ATTRS(*p) & S_DEF) == 0)
      c = info->maps.nuses;

    if (c)
    { int v = UD_VARNO(*p);

      if (c[v] < 255)
        c[v]++;
    }
  }
}
#endif

static
sort_uds (info)
ud_info* info;
{
  int n_ud  = NUM_UD(info);

  if (GET_DF_STATE(info) & DF_SORT)
    return 0;

#ifdef TMC_FLOD
  count_kills (info);
#endif
  cur_sort = info;
  qsort (info->maps.ud_map+1, n_ud-1, sizeof (info->maps.ud_map[0]), order_uds);

  SET_DF_STATE (info, DF_SORT);
  return 1;
}

rtx non_frame_mem_rtx;
rtx non_local_mem_rtx;

rebuild_df_helpers(info)
ud_info* info;
{
  int i,u,n_uid,n_id,n_ud;
  register ud_rec * p;
  register int *id_ud_list_pool;
  register int *uid_ud_list_pool;
  register int *uid_def_list_pool;

  free_flow_space (info);

  n_uid = NUM_UID;
  n_ud  = NUM_UD(info);
  n_id  = NUM_ID(info);

  /* Zero out trailing inactive uds ... */
  if ((p = &(info->maps.ud_map[n_ud-1]))->attrs & S_INACTIVE)
  {
    while (p[-1].attrs & S_INACTIVE) p--;

    NUM_UD(info) = p - info->maps.ud_map;
    bzero (p, (n_ud-NUM_UD(info)) * sizeof p[0]);
    n_ud = NUM_UD(info);
  }

  REALLOC_MAP (&info->maps.id_map, n_id, id_rec_map);
  REALLOC_MAP (&info->maps.uid_map, n_uid, uid_rec_map);

  /* allocate and initialise uds_in_insn sets ... */
  new_flex_pool (&info->pools.attr_sets_pool, n_ud, NUM_UD_SETS+3, F_DENSE);

  if (info->uds_used_by_pool != 0)
    free(info->uds_used_by_pool);

  /*
   * Since no ud can appear in more than 1 uds_used_by list, we simply
   * need to have n_ud elements in the list.  The list consists simply
   * of links of ud numbers.  So given ud u the next element in the
   * list is ud_pool[u].  This is extremely efficient both in terms of
   * space, and in terms of ease of access.
   *
   * Each of uds_used_by, uds_in_insn, and defs_in_insn are represented
   * in this way.
   *
   * Also since they are all allocated and freed as a group we simply
   * xmalloc and free once using uds_used_by_pool.  However we still
   * need uds_in_insn_pool, and defs_in_insn_pool for getting the next
   * element of the list.
   */
  id_ud_list_pool = (int *)xmalloc((n_ud + n_ud + n_ud) * sizeof(int));
  info->uds_used_by_pool = id_ud_list_pool;
  uid_ud_list_pool = id_ud_list_pool + n_ud;
  info->uds_in_insn_pool = uid_ud_list_pool;
  uid_def_list_pool = uid_ud_list_pool + n_ud;
  info->defs_in_insn_pool = uid_def_list_pool;

  for (i=0; i < NUM_UD_SETS; i++)
  { info->sets.attr_sets[i] = alloc_flex(info->pools.attr_sets_pool);
    info->attr_sets_cnt[i] = 0;
    info->attr_sets_last[i] = -1;
    info->attr_sets_first[i] = -1;
  }

  for (i = 0; i < n_uid; i++)
  {
    info->maps.uid_map[i].uds_in_insn = -1;
    info->maps.uid_map[i].defs_in_insn = -1;
  }

  /*
   * This next bit of code simply initializes all the uds_used_by entries to
   * point to the empty list, and sets up the vtype field for the
   * variable.
   */
  for (i=0; i < n_id; i++)
  {
    info->maps.id_map[i].uds_used_by = -1;
    info->maps.id_map[i].vtype = 0;
  }
  
  /* Look at each ud and put it into the sets it belongs in. */
  p = &(info->maps.ud_map[u=n_ud]);
  while ((p--),(u-- > 1))
  {
    if (p->attrs == 0 || (p->attrs & S_INACTIVE))
      bzero (p, sizeof (*p));
    else
    { int j,k,w; 
      id_rec* ir;

      /* If we never need to load or store the frame at calls, set up
         all call pseudos so they overlap all mem refs except the frame. */

      if (info==df_info+IDF_MEM && (p->attrs & S_CALL))
      { int v = df_data.frame_is_call_safe ? RTX_VAR_ID(non_frame_mem_rtx) : 0;

        RTX_VAR_ID(UD_VRTX(*p)) = v;
        p->varno = v;
      }

      ir = info->maps.id_map + p->varno;
  
      /* put ud onto list associated with ids. */
      id_ud_list_pool[u] = ir->uds_used_by;
      ir->uds_used_by = u;
  
      /* This code needs to produce a type that is the union of
         all references.  For now, if all references are not
         consistent, produce 0.  Note that the type from the
         id is used only for pseudos, anyway;  if we have a
         type at an ud, we always use that to evade overlap. */
  
      if (p->varno)
      { tree t;
  
        t = UD_TYPE(*p);
        df_assert (t);
  
        if ((ir->vtype != 0 && TYPE_UID(t) != TYPE_UID(ir->vtype)))
          ir->vtype = void_type_node;
        else
          ir->vtype = t;
      }
  
      uid_ud_list_pool[u] = info->maps.uid_map[p->insn].uds_in_insn;
      info->maps.uid_map[p->insn].uds_in_insn = u;
  
#ifdef TMC_FLOD
      p->attrs &= ~S_FLOD;
      if (info->is_flod(u))
        p->attrs |= S_FLOD;
#endif

      k = p->attrs;
  
      if (k & S_DEF)
      {
        uid_def_list_pool[u] = info->maps.uid_map[p->insn].defs_in_insn;
        info->maps.uid_map[p->insn].defs_in_insn = u;
      }
  
      j = -1;
      while ((i = (1 << ++j)) <= k)
        if (i & k)
        { set_flex_elt (info->sets.attr_sets[j], u);
          if (info->attr_sets_cnt[j]++ == 0)
            info->attr_sets_last[j] = u;
          info->attr_sets_first[j] = u;
        }
    }
  }
#if 0
fprintf (stderr,"%s: %d defs, %d flow defs [%d..%d]\n",
         df_names[info-df_info],
         info->attr_sets_last[I_DEF],
         (info->attr_sets_last[I_FLOD]-info->attr_sets_first[I_FLOD])+1,
         info->attr_sets_first[I_FLOD],
         info->attr_sets_last[I_FLOD]);
#endif
}

restart_dataflow (info)
ud_info* info;
{
  if (GET_DF_STATE(info) & DF_RESTART)
    return 0;

  info->setup_dataflow ();

  sort_uds (info);
  rebuild_df_helpers (info);

  trace (update_sets_exit, info);
  SET_DF_STATE (info, DF_RESTART);

  if (info == df_info+IDF_MEM)
    check_alias_assumptions();
    
  return 1;
}

check_alias_assumptions()
{
  register int i;
  register ud_info* mem = df_info + IDF_MEM;

  int prev_time = assign_run_time (IDT_TYPE);

  for (i = 1; i < NUM_ID(df_info+IDF_MEM); i++)
  { register int j;
    register int k;
    register int *uds_pool;
    register int uds_used;

    uds_used = mem->maps.id_map[i].uds_used_by;
    uds_pool = mem->uds_used_by_pool;

    if (is_empty_flex (mem_slots[i].slot_pt))
    {
      copy_flex (mem_slots[i].slot_pt, df_data.sets.all_symbols);

#ifdef MEM_WARNINGS
      /* Complain about objects whose addresses are crap */
      for (j = uds_used; j != -1; j = uds_pool[j])
        df_warning (illegal_address, mem, j);
#endif
    }

    for (j = uds_used; j != -1; j = uds_pool[j])
      for (k = uds_used; k != -1; k = uds_pool[k])
      { ud_rec*p, *q;
        p = mem->maps.ud_map+j;
        q = mem->maps.ud_map+k;

        if ((UD_BYTES(*p)<<UD_VAR_OFFSET(*p))&(UD_BYTES(*q)<<UD_VAR_OFFSET(*q)))
          if (!object_types_could_alias (UD_TYPE(*p), UD_TYPE(*q)))
          {
#if 0
            df_warning (illegal_overlap, mem, j, k);
#endif

            {
              register int t;
 
              for (t = mem->maps.id_map[p->varno].uds_used_by; t != -1;
                   t = uds_pool[t])
                UDN_TYPE(mem,t) = void_type_node;
            }
          }
      }
  }
  assign_run_time (prev_time);
}

/*  Mark memory references which are candidates for movement into
    the preceeding block by the scheduler. */

update_finfo()
{
  int v;

  bb_set blks = 0;
  bb_set doms = 0;
  id_set vars = 0;
  ud_set mems = 0;

  ud_info *mem = df_info + IDF_MEM;

  reuse_flex (&blks, N_BLOCKS, 1, F_DENSE);
  reuse_flex (&doms, N_BLOCKS, 1, F_DENSE);
  reuse_flex (&vars, NUM_ID(mem), 1, F_DENSE);
  reuse_flex (&mems, NUM_UD(mem), 1, F_DENSE);

  for (v = 1; v < NUM_ID(mem); v++)
    if (!in_flex (vars, v))
    {
      register int u = mem->maps.id_map[v].uds_used_by;

      if (u != -1)
      { int i,l,pc;
        register int *uds_pool = mem->uds_used_by_pool;
  
        /*  Decide whether the address of 'v' is a constant ... */
        pc = pure_constant_address(u);
  
        /* Get the LT number of v's address */
        l  = mem_slots[v].terms;
  
        clr_flex (blks);
        clr_flex (mems);
  
        for (i = v; i < NUM_ID(df_info+IDF_MEM); i++)
          if (mem_slots[i].terms == l)
          {
            set_flex_elt (vars, i);
            for (u = mem->maps.id_map[i].uds_used_by; u != -1; u = uds_pool[u])
            {
              int b = UDN_BLOCK(mem,u);
              int p = next_sized_flex(df_data.maps.preds+b,-1);
  
              /* If a reference has exactly 1 pred block, it is a candidate */
              if (p != -1 && next_sized_flex(df_data.maps.preds+b,p)== -1)
                set_flex_elt (mems, u);
  
              /* Keep track of all bb's using the same LTs */
              set_flex_elt (blks, UDN_BLOCK(mem,u));
            }
          }
  
        u = -1;
        while ((u=next_flex(mems,u)) != -1)
        { int ok = pc;
  
          if (!ok)
          { /* Address is not a pure constant;  if we can find a reference
               which in-dominates or out-dominates the predecessor, it is
               safe to move the reference into the predecessor */
  
            int b = next_sized_flex(df_data.maps.preds+UDN_BLOCK(mem,u),-1);
  
            clr_flex (doms);
            copy_flex (doms, df_data.maps.dominators[b]);
#if 0
            /* Our out dominators currently do not consider calls as being
               a possible exit.  Therefore, a longjump could happen which
               causes us to not actually reach the apparently out-dominating
               reference.  So for now, we only look at in dominators. */
            union_flex_into (doms, df_data.maps.out_dominators[b]);
#endif
            and_flex_into (doms, blks);
  
            ok = !is_empty_flex (doms);
          }
  
          if (ok)
            SET_CAN_MOVE_MEM(UDN_VRTX(mem,u));
        }
      }
    }

  reuse_flex (&mems, 0, 0, 0);
  reuse_flex (&vars, 0, 0, 0);
  reuse_flex (&doms, 0, 0, 0);
  reuse_flex (&blks, 0, 0, 0);
}

static int
rtx_could_be_pointer (r)
rtx r;
{
  enum machine_mode m;

  if (RTX_IS_LVAL_PARENT(r))
  {
    switch (GET_CODE(r))
    {
      case SIGN_EXTRACT:
      case ZERO_EXTRACT:
        return 0;

      case SIGN_EXTEND:
      case ZERO_EXTEND:
      case SUBREG:
        if ((m=GET_MODE(r)) != VOIDmode &&
            (GET_MODE_CLASS(m) != GET_MODE_CLASS(Pmode) ||
             GET_MODE_SIZE(m)  <  GET_MODE_SIZE(Pmode)))
          return 0;

        r = XEXP(r,0);
        break;
    }
  }

  if ((m=GET_MODE(r)) != VOIDmode &&
      (GET_MODE_CLASS(m) != GET_MODE_CLASS(Pmode) ||
       GET_MODE_SIZE(m)  <  GET_MODE_SIZE(Pmode)))
    return 0;

  if (RTX_IS_LVAL(r))
    if (RTX_TYPE(r) && !type_could_point_to (RTX_TYPE(r), 0))
      return 0;

  return 1;
}

static
new_ud (insn, user, off, varno, ud_var_offset, ud_offset, ud_size, init, pts_to, info)
int insn;
rtx user;
int off;
int varno;
int ud_var_offset;
int ud_offset;
int ud_size;
int init;
ud_set pts_to;
ud_info* info;
{
  int u = NUM_UD(info)++;
  ud_rec* udr;

  if (init & S_KILL)
    init |= S_DEF;

  /* For now, destroy all df state except sort. */
  SET_DF_STATE (info, GET_DF_STATE (info) & DF_SORT);

  if ((ud_offset & 7) || (ud_size & 7))
  {
    /* Should not get here on 960; if we do (i.e, if we start using
       sign-extract/zero-extract again), and if we really need it,
       the code probably works. */

    df_assert (0);

    /* Cannot call this a kill, because it doesn't really
       set all the bits that we are recording as being set. */

    init &= ~S_KILL;
    ud_size += (ud_offset & 7);
    ud_offset &= ~0x7l;
    ud_size = ROUND (ud_size, 8);
  }

  if (u >= MAP_SIZE(info->maps.ud_map, ud_rec_map))
    REALLOC_MAP (&info->maps.ud_map, (u * 2) + 1, ud_rec_map);

  udr = &(info->maps.ud_map[u]);

  df_assert (user && init);
  df_assert (insn > 0 && off >= 0 && varno >= 0 && ud_var_offset >= 0 && ud_size >= 0 && ud_offset >=0);

  ud_offset >>= 3;
  ud_size   >>= 3;

  df_assert (ud_var_offset + ud_offset + ud_size <= TI_BYTES);

  udr->user       = user;
  udr->off        = off;
  udr->insn       = insn;
  udr->varno      = varno;
  udr->ud_var_offset  = ud_var_offset;

  if (ud_size)
    /* Build a 16 bit mask representing the bytes referenced by this ud. */
    udr->ud_bytes = ((1 << (ud_offset+ud_size))-1) & ~ ((1 << ud_offset)-1);
  else
    /* Overlaps with everything at the same offset or above */
    udr->ud_bytes = ~ ((1 << ud_offset)-1);

  udr->attrs      = init;

  if (UD_TYPE(*udr) == 0)
  { enum machine_mode m = GET_MODE(UD_VRTX(*udr));
    extern tree integer_type_node, float_type_node,
                double_type_node,long_double_type_node;

    if (GET_MODE_CLASS(m) == MODE_CC)
      UD_TYPE(*udr) = integer_type_node;
    else if (GET_MODE_CLASS(m) == MODE_FLOAT)
    {
      if (GET_MODE_SIZE(m) == int_size_in_bytes(float_type_node))
        UD_TYPE(*udr) = float_type_node;
      else if (GET_MODE_SIZE(m) == int_size_in_bytes(double_type_node))
        UD_TYPE(*udr) = double_type_node;
      else if (GET_MODE_SIZE(m) == int_size_in_bytes(long_double_type_node))
        UD_TYPE(*udr) = long_double_type_node;
      else
        df_assert (0);
    }
  }

  if (UD_TYPE(*udr) == 0)
  {
#if 0
      if (info != df_info+IDF_REG || udr->varno < FIRST_PSEUDO_REGISTER)
        if (user && GET_CODE(user) != USE)
          df_warning (no_type_info, info, u);
#endif
    UD_TYPE(*udr) = void_type_node;
  }

  if (pts_to && !rtx_could_be_pointer (UD_RTX(*udr)))
    pts_to = 0;

  if (pts_to != 0 && FLEX_POOL(pts_to)==df_data.pools.shared_sym_pool)
    df_data.shared_syms++;

  udr->pts_at = pts_to;

  /* mark this insn as having some uds */
  df_data.maps.insn_info[insn].insn_has_uds = 1;
  return u;
}

extern rtx absolute_sym();

void
scan_rtx_for_uds (insn, context, offset, attrs, pts_to)
int insn;
rtx context;
int offset;
unsigned long attrs;
ud_set pts_to;
{
  /* This subtree is new.  Completely (recursively)
     scan it, and add the uds found in it to the 
     appropriate maps. */

  rtx x, *x_addr;
  ud_info *df;

  x_addr  = &NDX_RTX (context, offset);

  if (*x_addr == 0)
    return;

  *x_addr = optim_rtx (*x_addr);
  x       = *x_addr;
  
  if ((df=DF_INFO(x)) != 0)
  { unsigned long ud_attrs;
    rtx v;
    int ud_size, ud_offset, ud_var_offset, var_id;
    tree type;

    ud_attrs      = attrs;
    v             = RTX_LVAL(x);
    ud_size       = RTX_BITS(v);
    ud_offset     = 0;
    ud_var_offset = 0;
    var_id        = 0;

    /* For now, if we have an open libcall note, treat this mem as volatile
       to keep from inadvertantly placing a store under libcalls.

       Also, treat BLKmode mems as volatile, so that we don't attempt
       replacement.
    */

    if (GET_CODE(v)==MEM && (ud_size==0 || UID_RETVAL_UID(insn)>0 || MEM_VOLATILE_P(v)))
      ud_attrs |= S_USE |S_DEF;

    if (offset)
      switch (GET_CODE(context))
      { /* Figure out what flavor of ud this is, from context. */
  
        default:
          /* As we get to production, we'll probably want to list all
             use cases explicitly, and make default: be USE|DEF. */
  
          ud_attrs |= S_USE;
          break;
  
        case SET:
          ud_attrs |= ((x_addr == &SET_SRC (context)) ? S_USE : S_KILL);
          break;
  
        case USE:
          ud_attrs |= S_USE;
          break;

        case CLOBBER:
          ud_attrs |= S_USE |S_DEF;
          break;
      }

    if (ud_attrs & S_KILL)
      ud_attrs |= S_DEF;

    if (ud_attrs & S_USE)
      ud_attrs &= ~S_KILL;

    if (GET_CODE(v)==MEM)
    { if (MEM_VOLATILE_P(v))
        UID_VOLATILE(insn) = 1;
    }
    else if (GET_CODE(v)==REG)
    { if ((ud_attrs & S_DEF) && (REGNO(v)==STACK_POINTER_REGNUM))
        UID_VOLATILE(insn) = 1;
    }

    var_id = RTX_VAR_ID(v);

    switch (GET_CODE(v))
    { default:
        df_assert (0);
        break;
  
      case MEM:
        /* We consider each mem to be the null variable until refine_mem_info
           gets a shot.  If we are scanning the function for the first time, we
           are supposed to be guaranteed that the mem is not shared with
           anything else;  the dataflow fields had better be zero. */
  
        RTX_VAR_ID(v) = var_id = 0;
        RTX_MRD_SET(v) = 0;
        CLR_CAN_MOVE_MEM(v);
        break;
  
      case REG:
        RTX_VAR_ID(v) = var_id = REGNO_VID(REGNO(v));
        ud_var_offset = REGNO_VOFFSET(REGNO(v));
  
        /* In the RTL, the high bits of a mini (< word) reg reference are ill
           defined;  we choose to say that they are defined for KILL. */
  
        if ((ud_attrs & S_KILL) && ud_size < BITS_PER_WORD)
          ud_size = BITS_PER_WORD;
        break;
  
      case SYMBOL_REF:
      {
        ud_size = BITS_PER_WORD;

        if (var_id == 0)
        {
          RTX_VAR_ID(v) = var_id = NUM_ID(df)++;
          df_data.maps.symbol_rtx[var_id] = v;

          df_assert (df_data.maps.symbol_set[var_id] == 0);
          pts_to = alloc_flex(df_data.pools.symbol_pool);
          df_data.maps.symbol_set[var_id] = pts_to;
          set_flex_elt (pts_to, var_id);
        }
        else
        { pts_to = df_data.maps.symbol_set[var_id];
          df_assert (pts_to);
        }

        if (v == absolute_sym())
          pts_to = df_data.sets.addr_syms;

        new_ud (ENTRY, &df_data.maps.symbol_rtx[var_id], 0, var_id,
                         0, 0, BITS_PER_WORD, S_DEF, pts_to, df);

        if (SYM_ADDR_TAKEN_P(v))
        { set_flex_elt (df_data.sets.addr_syms, var_id);
          set_flex_elt (df_data.sets.all_addr_syms_except_frame, var_id);
          set_flex_elt (df_data.sets.all_addr_syms_except_arg, var_id);
          set_flex_elt (df_data.sets.all_addr_syms_except_local, var_id);
        }

        set_flex_elt (df_data.sets.all_symbols, var_id);
        set_flex_elt (df_data.sets.all_syms_except_frame, var_id);
        set_flex_elt (df_data.sets.all_syms_except_arg, var_id);
        set_flex_elt (df_data.sets.all_syms_except_local, var_id);
        break;
      }
    }
  
    if (var_id >= NUM_ID(df))
      NUM_ID(df) = var_id + 1;
  
    switch (GET_CODE(x))
    { default:
        df_assert (0);
        break;
  
      case MEM:
      case REG:
      case SYMBOL_REF:
        break;
  
      case ZERO_EXTRACT:
      case SIGN_EXTRACT:
        ud_size    = INTVAL (XEXP (x,1));
        ud_offset += INTVAL (XEXP (x,2));
        break;
  
      case ZERO_EXTEND:
      case SIGN_EXTEND:
        ud_size    = RTX_BITS (XEXP(x,0));
        break;

      case SUBREG:
        ud_offset += SUBREG_WORD (x) * BITS_PER_WORD;
  
        /* If this is a subreg of a reg (i.e, usually), and if the mode is
           smaller than a word, we get our choice as to whether or not we say
           that the high bits of the register are really important.
  
           We choose to say that the aren't, except for KILLS - this means
           that a kill will kill off as much as posible, and non-kills will
           overlap as little as possible. */
  
        if (GET_CODE(v) != REG || RTX_BITS(x) >= BITS_PER_WORD)
          ud_size = RTX_BITS (x);
        else
          if (ud_attrs & S_KILL)
            ud_size = MAX (BITS_PER_WORD, RTX_BITS (x));
          else
            ud_size = MIN (ud_size, RTX_BITS (x));
        break;
    }
  
    new_ud
      (insn, context, offset, var_id, ud_var_offset, ud_offset, ud_size, ud_attrs, pts_to, df);

    /* If this mem is a pseudo, we don't want to look at the address */
    if (GET_CODE(v)==MEM && XEXP(v,0) != 0)
      scan_rtx_for_uds (insn, v, XEXP_OFFSET (v,0), S_ADDR | S_USE, df_data.sets.all_symbols);
  }

  else
  {
    enum rtx_code x_code = GET_CODE (x);

    switch (x_code)
    { int i;
      df_kind k;

      case CALL:
        RTX_TYPE(XEXP(x,0)) = void_type_node;

        /* If this is a libcall, just look at the address of the call target;
           otherwise, use the mem which denotes the call target as a USE-DEF
           pseudo to blow away everything. */

        if (UID_RETVAL_UID(insn) > 0)
          scan_rtx_for_uds(insn, XEXP(x,0), XEXP_OFFSET(x,0), S_USE|S_ADDR,df_data.sets.all_symbols);
        else
        { UID_VOLATILE(insn) = 1;
          scan_rtx_for_uds (insn, &XEXP(x,0), 0, S_CALL|S_USE|S_DEF,df_data.sets.all_symbols);
        }
        break;

      case ASM_OPERANDS:
      {
        struct rtvec_def *v;

        df_assert (attrs == 0);
        v = ASM_OPERANDS_INPUT_VEC(x);

        /* vector can be shared within the insn;  just scan it once. */
        if (v != last_asm_vect)
        { int j = (last_asm_vect=v)->num_elem;

          while (j--)
            scan_rtx_for_uds (insn, &(v->elem[j].rtx), 0, S_USE,df_data.sets.all_symbols);
        }
        break;
      }

      case CONST_DOUBLE:
        break;

      case CLOBBER:
        if (XEXP(x,0) == 0)
          XEXP(x,0) = gen_typed_mem_rtx (void_type_node, word_mode, 0);
      
      default:
      recurse:
      {
        char* fmt = GET_RTX_FORMAT (x_code);

        for (i=0; i < GET_RTX_LENGTH(x_code); i++)
          if (fmt[i] == 'e')
            scan_rtx_for_uds (insn, x, XEXP_OFFSET (x, i), attrs,df_data.sets.all_symbols);

          else if (fmt[i] == 'E')
          { int j = XVECLEN(x, i);
            while (--j >= 0)
              scan_rtx_for_uds (insn, &XVECEXP(x,i,j), 0, 0,df_data.sets.all_symbols);
          }
      }
    }
  }
}

inactivate_uds_in_tree (insn, context)
int insn;
rtx* context;
{
  ud_info *df;

  if (df = DF_INFO (*context))
  {
    register int lelt;

    lelt = df->maps.uid_map[insn].uds_in_insn;

    while (lelt != -1 && context != &UD_RTX(df->maps.ud_map[lelt]))
      lelt = df->uds_in_insn_pool[lelt];
    df_assert (lelt != -1);
    df->maps.ud_map[lelt].attrs |= S_INACTIVE;
    set_flex_elt (df->sets.attr_sets[I_INACTIVE], lelt);
    context = &UD_VRTX(df->maps.ud_map[lelt]);
    df->maps.ud_map[lelt].off  = 0;
    df->maps.ud_map[lelt].user = 0;

    /* For now, destroy all df state except sort. */
    SET_DF_STATE (df, GET_DF_STATE (df) & DF_SORT);
  }

  {
    rtx x = *context;
    enum rtx_code x_code = GET_CODE (x);

    char* fmt = GET_RTX_FORMAT (x_code);
    int i;

    switch (x_code)
    {
      case CONST_DOUBLE:
        break;

      default:
        for (i=0; i < GET_RTX_LENGTH(x_code); i++)
          if (fmt[i] == 'e')
            inactivate_uds_in_tree (insn, &XEXP(x,i));
    
          else if (fmt[i] == 'E')
          { int j = XVECLEN(x, i);
            while (--j >= 0)
              inactivate_uds_in_tree (insn, &XVECEXP(x,i,j));
          }
    }
  }
}

mem_next_df (m, i)
unsigned short* m;
int i;
{
  int j;

  ud_info* df = df_info+IDF_MEM;
  if (i >= df->attr_sets_last[I_DEF])
    return -1;

  m += ((i+1) * MEM_VPD);

  do
  {
    i++;

    for (j = 0; j < MEM_VPD; j++)
      if (*m++)
        return i;
  }
  while
    (i < df->attr_sets_last[I_DEF]);

  return -1;
}

reg_next_df (m, i)
unsigned short *m;
int i;
{
  int last_flow_def = LAST_FLOW_DEF(df_info+IDF_REG);

  i++;

  while (i <= last_flow_def)
  {
    if ((m[i>>2] & (0xf << ((i & 3) * 4))) != 0)
      return i;
    i++;
  }

  return -1;
}

sym_next_df (m, i)
unsigned short *m;
int i;
{
  df_assert (0);
  return -1;
}

dump_df_set (file, m, df)
FILE* file;
unsigned short* m;
ud_info* df;
{
  int prev  = -2;
  int i     = -1;
  int state = 0;

  fprintf (file, "[");

  while ((i=df->next_df(m,i)) != -1)
  {
    switch (state)
    {
      case 0:
        fprintf (file, "%d", i);
        state = 1;
        break;

      case 1:
        if (prev == i-1)
          state = 2;
        else
          fprintf (file, ",%d", i);
        break;

      case 2:
        if (prev != i - 1)
        {
          fprintf (file, "..%d,%d", prev, i);
          state = 1;
        }
        break;
    }
    prev = i;
  }

  if (state == 2)
    fprintf (file, "..%d", prev);

  fprintf (file, "]");
}

debug_df_set (m, df)
{
  dump_df_set (stderr, m, df);
  fprintf (stderr, "\n");
}

dump_flex_map (file, mess, m, n)
FILE* file;
flex_set* m;
int n;
{
  int i;

  if (mess)
    fprintf (file, "%s", mess);

  fprintf (file, " - %d entries:", n);

  if (m)
    for (i=0; i < n; i++)
      if (!is_empty_flex (m[i]))
      { fprintf (file, "\n%d ", i);
        dump_flex (file, m[i]);
      }
}

dump_df_map (file, mess, m, df)
FILE* file;
char* mess;
unsigned short* m;
ud_info* df;
{
  int i,j,rows,cols;

  if (mess)
    fprintf (file, "%s", mess);

  rows = N_BLOCKS;
  cols = df->bb_bytes / 2;

  fprintf (file, " - %d entries:", rows);

  for (i=0; i < rows; i++)
  {
    for (j=0; j < cols; j++)
      if (m[j])
        break;

    if (j != cols)
    { fprintf (file, "\n%d ", i);
      dump_df_set (file, m, df);
    }

    m += cols;
  }
}

debug_df_map (m, df)
unsigned short* m;
ud_info* df;
{
  dump_df_map (stderr, "", m, df);
  fprintf (stderr, "\n");
}


dump_sized_flex_map (file, mess, m, n)
FILE* file;
sized_flex_set* m;
int n;
{
  int i;

  if (mess)
    fprintf (file, "%s", mess);

  fprintf (file, " - %d entries:", n);

  for (i=0; i < n; i++)
    if (!is_empty_flex (m[i].body))
    { fprintf (file, "\n%d ", i);
      dump_sized_flex (file, m+i);
    }
}

debug_flex_map (m, n)
flex_set* m;
int n;
{
  dump_flex_map (stderr, "", m, n);
}

debug_sized_flex_map (m, n)
sized_flex_set* m;
int n;
{
  dump_sized_flex_map (stderr, "", m, n);
}

get_dominators()
{
  int i,j,change;
  flex_set ones, tmp;

  ALLOC_MAP (&df_data.maps.dominators, N_BLOCKS, flex_set_map);

  new_flex_pool (&df_data.pools.dom_pool, N_BLOCKS, N_BLOCKS + 2,F_DENSE);
  df_data.sets.dom_ones = ones = alloc_flex (df_data.pools.dom_pool);
  df_data.sets.dom_tmp  = tmp  = alloc_flex (df_data.pools.dom_pool);

  for (i = 0; i < N_BLOCKS; i++)
    set_flex_elt (ones, i);

  for (i = 0; i < N_BLOCKS; i++)
    df_data.maps.dominators[i] = alloc_flex (df_data.pools.dom_pool);

  for (i = 0; i < N_BLOCKS; i++)
  { if (i == ENTRY_BLOCK)
      set_flex_elt (df_data.maps.dominators[i], i);
    else
      copy_flex (df_data.maps.dominators[i], ones);
  }

  do
  { change = 0;
    for (i = 0; i < N_BLOCKS; i++)
      if (i != ENTRY_BLOCK)
      { if ((j=next_sized_flex(df_data.maps.preds+i,-1)) != -1)
        { copy_flex (tmp, df_data.maps.dominators[j]);

          while ((j=next_sized_flex(df_data.maps.preds+i,j)) != -1)
            and_flex_into (tmp, df_data.maps.dominators[j]);
        }
        else
          clr_flex (tmp);

        set_flex_elt (tmp, i);

        if (!change)
        { if (change = !equal_flex (tmp, df_data.maps.dominators[i]))
            copy_flex (df_data.maps.dominators[i], tmp);
        }
        else
          copy_flex (df_data.maps.dominators[i], tmp);
      }
  }
  while
    (change);

  df_data.dom_relation[0] = df_data.maps.dominators;
}

get_out_dominators()
{
  int i,j,change;
  ud_set ones,tmp;

  ALLOC_MAP (&df_data.maps.out_dominators, N_BLOCKS, flex_set_map);

  new_flex_pool (&df_data.pools.out_dom_pool, N_BLOCKS, 2 *N_BLOCKS + 2,F_DENSE);
  ones = alloc_flex (df_data.pools.out_dom_pool);
  tmp  = alloc_flex (df_data.pools.out_dom_pool);

  for (i = 0; i < N_BLOCKS; i++)
    set_flex_elt (ones, i);

  for (i = 0; i < N_BLOCKS; i++)
    df_data.maps.out_dominators[i] = alloc_flex (df_data.pools.out_dom_pool);

  for (i = 0; i < N_BLOCKS; i++)
  {
    if (i == EXIT_BLOCK)
      set_flex_elt (df_data.maps.out_dominators[i], i);
    else
      copy_flex (df_data.maps.out_dominators[i], ones);
  }

  do
  { change = 0;
    for (i = 0; i < N_BLOCKS; i++)
      if (i != EXIT_BLOCK)
      { if ((j=next_flex(df_data.maps.succs[i],-1)) != -1)
        { copy_flex (tmp, df_data.maps.out_dominators[j]);

          while ((j=next_flex(df_data.maps.succs[i],j)) != -1)
            and_flex_into (tmp, df_data.maps.out_dominators[j]);
        }
        else
          clr_flex (tmp);

        set_flex_elt (tmp, i);

        if (!change)
        { if (change = !equal_flex (tmp, df_data.maps.out_dominators[i]))
            copy_flex (df_data.maps.out_dominators[i], tmp);
        }
        else
          copy_flex (df_data.maps.out_dominators[i], tmp);
      }
  }
  while
    (change);

  for (i = 0; i < N_BLOCKS; i++)
  { /* This is a hack to keep us from taking wrong turns for
       disconnected graph regions.  Any node which is part of an
       infinite loop already has every unreachable node in its
       out_dominator set.  This is OK, because most of the time we are using
       out-dominators to find a place to put a store when execution
       leaves the region;  if it never leaves, we can place the store
       where we like - in fact we don't even have to do it at all.  But
       the algorithm we use falls apart if the entry node out-dominates
       any node except itself. */

    if (i != ENTRY_BLOCK)
      clr_flex_elt (df_data.maps.out_dominators[i], ENTRY_BLOCK);

    /* This is similar, except it covers regions which are dead
       code instead of infinite loops.  The question is where to
       place a load - the answer is, who cares, just survive.
       Hopefully, jump won't leave us any code that looks like this. */

    if (i != EXIT_BLOCK)
      clr_flex_elt (df_data.maps.dominators[i], EXIT_BLOCK);
  }

  df_data.dom_relation[1] = df_data.maps.out_dominators;
}


get_loops()
{
  int i;

  ALLOC_MAP (&df_data.maps.forw_succs, N_BLOCKS, flex_set_map);
  ALLOC_MAP (&df_data.maps.loop_info, N_BLOCKS, loop_rec_map);

  new_flex_pool (&df_data.pools.loop_pool, N_BLOCKS, 3 *N_BLOCKS + 3,F_DENSE);

  for (i = 0; i < N_BLOCKS; i++)
  {
    df_data.maps.forw_succs[i] = alloc_flex (df_data.pools.loop_pool);

    df_data.maps.loop_info[i].members = alloc_flex (df_data.pools.loop_pool);
    df_data.maps.loop_info[i].member_of = alloc_flex (df_data.pools.loop_pool);
    df_data.maps.loop_info[i].parent = -1;
    df_data.maps.loop_info[i].inner_loop = -1;
  }

  df_data.sets.natural_loops = alloc_flex (df_data.pools.loop_pool);
  df_data.sets.new_loop = alloc_flex (df_data.pools.loop_pool);

  /* Compute forw successors and natural loops. */
  compute_all_forw_succs();

  /* Thread loops */
  calculate_loop_parent_info ();
}


#define PCACHE_SLOTS 10

get_predecessors (want_loop)
int want_loop;
{
  int i,j;
  int pred_inx;
  int num_preds;
  int *preds_alt;

  prep_for_flow_analysis (get_insns ());

  revive_rtx (&ENTRY_NOTE_ZOMBIE);
  revive_rtx (&EXIT_NOTE_ZOMBIE);
  revive_rtx (&ENTRY_LABEL_ZOMBIE);
  revive_rtx (&EXIT_LABEL_ZOMBIE);

  ALLOC_MAP (&df_data.maps.path_cache, 0, ulong_map);
  ALLOC_MAP (&df_data.maps.out_dominators, 0, flex_set_map);

  ALLOC_MAP (&df_data.maps.loop_info, 0, loop_rec_map);
  ALLOC_MAP (&df_data.maps.forw_succs, 0, flex_set_map);
  ALLOC_MAP (&df_data.maps.dominators, 0, flex_set_map);

  ALLOC_MAP (&df_data.maps.preds, N_BLOCKS, sized_flex_set_map);
  ALLOC_MAP (&df_data.maps.succs, N_BLOCKS, flex_set_map);

  if (df_data.maps.preds_alt != 0)
  {
    free (df_data.maps.preds_alt);
    df_data.maps.preds_alt = 0;
  }

  new_flex_pool (&df_data.pools.pred_pool, N_BLOCKS, 2 *N_BLOCKS,F_DENSE);

  for (i = 0; i < N_BLOCKS; i++)
  {
    df_data.maps.preds[i] = alloc_sized_flex (df_data.pools.pred_pool);
    df_data.maps.succs[i] = alloc_flex (df_data.pools.pred_pool);
  }

  num_preds = 1;
  if (n_basic_blocks == 0)
    set_sized_flex_elt (df_data.maps.preds+EXIT_BLOCK, ENTRY_BLOCK);
  else
  {
    set_sized_flex_elt (df_data.maps.preds+0, ENTRY_BLOCK);

    for (i = 0; i < N_BLOCKS; i++)
    { rtx head, jump;
      int isjmp;
  
      head = BLOCK_HEAD(i);
      num_preds ++;
  
      if (GET_CODE(head)==CODE_LABEL)
        for (jump= LABEL_REFS (head); jump != head; jump = LABEL_NEXTREF (jump))
        {
          set_sized_flex_elt (df_data.maps.preds+i, BLOCK_NUM (CONTAINING_INSN(jump)));
          num_preds ++;
        }
  
      /* If this block ends in a RETURN, or if it is the last real block in
         the function and control falls off the end, say that this block is
         a predecessor of the exit block. */
  
      jump  = BLOCK_END(i);
      isjmp = (GET_CODE(jump)==JUMP_INSN);
  
      if ((isjmp && GET_CODE(PATTERN(jump))==RETURN) ||
          ((i==LAST_REAL_BLOCK) && (!isjmp || !simplejump_p(jump))))
      {
        set_sized_flex_elt (df_data.maps.preds+EXIT_BLOCK, i);
        num_preds ++;
      }
  
      if (BLOCK_DROPS_IN(i))
      { df_assert (i != 0);
        set_sized_flex_elt (df_data.maps.preds+i, i-1);
        num_preds ++;
      }
    }

    /* If exit block is unreachable, say that it is reachable from last
       lexical block. */

    if (next_sized_flex (df_data.maps.preds+EXIT_BLOCK,-1) == -1)
    { df_data.function_never_exits = 1;
      set_sized_flex_elt (df_data.maps.preds+EXIT_BLOCK, LAST_REAL_BLOCK);
      num_preds ++;
    }
  }

  for (i = 0; i < N_BLOCKS; i++)
  {
    /* Now, only the entry block should have no preds, if
       find_basic_blocks did its job. */

    for (j=next_sized_flex(df_data.maps.preds+i,-1); j!=-1;
         j=next_sized_flex(df_data.maps.preds+i,j))
      set_flex_elt (df_data.maps.succs[j], i);
  }

  /* Some blocks have no predecessors because of 'volatile' functions.  This
     basically drives us nuts, and it is totally useless information - we have
     to assume that functions might not return anyway. So, pretend such
     blocks have the next lexical block as a successor. */

  for (i = FIRST_REAL_BLOCK; i <= LAST_REAL_BLOCK; i++)
    if (is_empty_flex (df_data.maps.succs[i]))
    { int next;

      df_assert (i != ENTRY_BLOCK);
      
      if (i == LAST_REAL_BLOCK)
        next = EXIT_BLOCK;
      else
        next = i+1;

      set_flex_elt (df_data.maps.succs[i], next);
      set_sized_flex_elt (df_data.maps.preds+next, i);
      num_preds ++;
    }

  preds_alt = (int *)xmalloc(num_preds * sizeof(int));
  df_data.maps.preds_alt = preds_alt;
  pred_inx = 0;

  for (i = 0; i < N_BLOCKS; i++)
  {
    register int p;
    p = next_sized_flex(df_data.maps.preds+i, -1);
    while (1)
    {
      preds_alt[pred_inx] = p;
      pred_inx ++;
      
      if (p == -1)
        break;
      p = next_sized_flex(df_data.maps.preds+i, p);
    }
  }

  if (want_loop)
  {
    get_dominators();
    get_loops();
  }

  trace (get_preds_exit);

  mark_rtx_deleted (ENTRY_NOTE, &ENTRY_NOTE_ZOMBIE);
  mark_rtx_deleted (EXIT_NOTE, &EXIT_NOTE_ZOMBIE);
  mark_rtx_deleted (ENTRY_LABEL, &ENTRY_LABEL_ZOMBIE);
  mark_rtx_deleted (EXIT_LABEL, &EXIT_LABEL_ZOMBIE);
}

/* Routines for deleting/undeleting placeholder rtx's. */
mark_rtx_deleted (r, z)
rtx r;
rtx_zombie* z;
{
  df_assert (GET_CODE(r) != NOTE || NOTE_LINE_NUMBER(r) != NOTE_INSN_DELETED);

  z->s_rtx  = r;
  z->s_code = GET_CODE(r);
  z->s_line = NOTE_LINE_NUMBER(r);
  z->s_file = NOTE_SOURCE_FILE(r);

  PUT_CODE(r, NOTE);
  NOTE_LINE_NUMBER(r) = NOTE_INSN_DELETED;
  NOTE_SOURCE_FILE(r) = 0;
}

revive_rtx (z)
rtx_zombie* z;
{
  rtx r = z->s_rtx;

  df_assert (GET_CODE(r) == NOTE && NOTE_LINE_NUMBER(r) == NOTE_INSN_DELETED);

  PUT_CODE(r, z->s_code);
  NOTE_LINE_NUMBER(r) = z->s_line;
  NOTE_SOURCE_FILE(r) = z->s_file;
}

new_natural_loop (d, n)
int d;
int n;
{
  /* Get the natural loop for back edge n->d.  Called from compute_forw_succs */

  int s, lsp;

  if (in_flex (df_data.maps.dominators[n], d))
  {
  
    if ((s = MAP_SIZE(df_data.maps.lstk, short_map)) == 0)
      ALLOC_MAP (&df_data.maps.lstk, s=32, short_map);
  
    lsp = 0;
  
    clr_flex (df_data.sets.new_loop);
    set_flex_elt (df_data.sets.new_loop, d);
  
    set_flex_elt (df_data.maps.loop_info[d].member_of, d);
    set_flex_elt (df_data.sets.natural_loops, d);
  
    if (!in_flex (df_data.sets.new_loop, n))
    { set_flex_elt (df_data.sets.new_loop,n);
      set_flex_elt (df_data.maps.loop_info[n].member_of, d);
      df_data.maps.lstk[lsp++] = n;
    }
  
    while (lsp != 0)
    { int m,p;
  
      m = df_data.maps.lstk[--lsp];
      p = -1;
  
      while ((p=next_sized_flex(df_data.maps.preds+m,p)) != -1)
        if (!in_flex (df_data.sets.new_loop, p))
        { set_flex_elt (df_data.sets.new_loop, p);
          set_flex_elt (df_data.maps.loop_info[p].member_of, d);
  
          if (lsp == s)
            REALLOC_MAP (&df_data.maps.lstk, (s=(s+1)*2), short_map);
  
          df_data.maps.lstk[lsp++] = p;
        }
    }
  
    union_flex_into (df_data.maps.loop_info[d].members, df_data.sets.new_loop);
  }
}

int flex_subset (s1, s2)
{
  return equal_flex (and_flex (0, s1,s2), s2);
}

calculate_loop_parent_info ()
{
  int b;
  ud_set pile = 0;
  ud_set loops = 0;
  ud_set examined = 0;
  ud_set loop_members = 0;

  b = -1;
  while ((b = next_flex (df_data.sets.natural_loops, b)) != -1)
  { int p = df_data.maps.loop_info[b].parent;
    int m;

    m = -1;
    while ((m = next_flex(df_data.maps.loop_info[b].member_of,m)) != -1)
      if (m != b && flex_subset (df_data.maps.loop_info[m].members, df_data.maps.loop_info[b].members))
        if (p == -1 || flex_subset (df_data.maps.loop_info[p].members, df_data.maps.loop_info[m].members))
          df_data.maps.loop_info[b].parent = (p = m);
  }

  reuse_flex (&pile,  N_BLOCKS, 1, F_DENSE);
  reuse_flex (&loops, N_BLOCKS, 1, F_DENSE);
  reuse_flex (&examined, N_BLOCKS, 1, F_DENSE);
  reuse_flex (&loop_members, N_BLOCKS, 1, F_DENSE);

  copy_flex (pile, df_data.sets.natural_loops);

  while (!is_empty_flex (pile))
  {
    copy_flex (loops, pile);
    clr_flex (pile);

    b = -1;
    while ((b=next_flex (loops,b)) != -1)
    {
      clr_flex_elt (loops, b);

      if (is_empty_flex (and_flex(0,df_data.maps.loop_info[b].members,loops)))
      { int t;

        and_compl_flex (loop_members, df_data.maps.loop_info[b].members, examined);

        t = -1;
        while ((t = next_flex (loop_members,t)) != -1)
        {
          df_assert (df_data.maps.loop_info[t].parent == -1 || (df_data.maps.loop_info[t].parent == df_data.maps.loop_info[b].parent && in_flex (df_data.sets.natural_loops,t)));
          df_data.maps.loop_info[t].parent = df_data.maps.loop_info[b].parent;
          df_assert (df_data.maps.loop_info[t].inner_loop == -1);
          df_data.maps.loop_info[t].inner_loop = b;
        }

        union_flex_into (examined, loop_members);
        set_flex_elt (loops, b);
      }
      else
        set_flex_elt (pile, b);
    }
  }
  reuse_flex (&pile,0,0,0);
  reuse_flex (&loops,0,0,0);
  reuse_flex (&examined,0,0,0);
  reuse_flex (&loop_members,0,0,0);
}

char* sym_names();
dump_ud (file, info, u)
FILE* file;
ud_info* info;
int u;
{
  char buf[128];
  ud_rec* udr = &info->maps.ud_map[u];
  int i;
  rtx r, v;

  if (u == 0)
  { fprintf (file, "\n0");
    return;
  }

  v = UD_VRTX (*udr);
  df_assert (RTX_VAR_ID(v) == udr->varno);
  
  fprintf (file,"\n%2d @%3d v%2d.%d:%x", u, udr->insn, udr->varno,
                UD_VAR_OFFSET(*udr), UD_BYTES(*udr));

  for (i = 0; i < NUM_UD_SETS; i++)
    if (udr->attrs & (1 << i))
      fprintf (file, "%c", attr_names[i][0]);
    else
      fprintf (file, " ");

  if (r = UD_CONTEXT(*udr))
  { i = &UD_RTX(*udr) - &XEXP(r,0);
    fprintf (file, " XEXP(%s,%d):\t", GET_RTX_NAME(GET_CODE(r)), i);
  }
  else
    fprintf (file, " NON-RTX CONTEXT\t");
  
  fprint_rtx (file, UD_RTX(*udr));
  fprintf (file, "\t");

  fprintf (file, "->%s", sym_names (udr->pts_at, buf, sizeof (buf)));
}

rtx
user_ud_position (info, u)
ud_info* info;
int u;
{
  ud_rec* udr = &info->maps.ud_map[u];

  rtx insn = UID_RTX (udr->insn);

  if (df_data.have_line_info)
  {
    while (insn && (GET_CODE(insn) != NOTE || NOTE_LINE_NUMBER (insn) <= 0))
      insn = PREV_INSN(insn);
  }
  else
    insn = 0;

  return insn;
}

print_ud (file, info, u)
FILE* file;
ud_info* info;
int u;
{ rtx note;
  char buf[256];
  int comma = 0;
  ud_rec* udr = &info->maps.ud_map[u];
  int i;

  fprintf (file, info->ud_name (u, buf));

  note = user_ud_position (info,u);

  if (note)
    fprintf (file, " insn %4d, line %3d, file %s", udr->insn,
                   NOTE_LINE_NUMBER(note), NOTE_SOURCE_FILE(note));
  else
    fprintf (file, " insn %4d", udr->insn);

  for (i = 0; i < NUM_UD_SETS; i++)
  { if (udr->attrs & (1 << i))
    { if (comma)
        fprintf (file, ",");
      fprintf (file, " %s", attr_names[i]);
      comma = 1;
    }
  }

  fprintf (file, " -> ");
  dump_flex (file, info->maps.ud_map[u].pts_at);
}

dump_id (file, info, i)
FILE* file;
ud_info* info;
int i;
{
  id_rec* idr = &(info->maps.id_map[i]);

  fprintf (file,"%s_id %2d used by uds ", df_names[info-&df_info[0]], i);
}

char* sym_id_name();
char* sym_names (s, buf_in, buf_len)
id_set s;
char* buf_in;
int buf_len;
{
  char* p;
  char my_buf[128];
  int v;

  p = buf_in;
  v = -1;

  if (s == df_data.sets.addr_syms)
    strcpy (p, "addr_syms");

  else if (s == df_data.sets.all_symbols)
    strcpy (p, "all_symbols");
  
  else if (s == df_data.sets.all_syms_except_frame)
    strcpy (p, "all_syms_except_frame");

  else if (s == df_data.sets.all_syms_except_arg)
    strcpy (p, "all_syms_except_arg");

  else if (s == df_data.sets.all_syms_except_local)
    strcpy (p, "all_syms_except_local");

  else if (s == df_data.sets.all_addr_syms_except_frame)
    strcpy (p, "all_addr_syms_except_frame");

  else if (s == df_data.sets.all_addr_syms_except_arg)
    strcpy (p, "all_addr_syms_except_arg");

  else if (s == df_data.sets.all_addr_syms_except_local)
    strcpy (p, "all_addr_syms_except_local");
  
  else
  { p[0] = '\0';
    while ((v=next_flex(s,v)) != -1)
    { if (p != buf_in)
        *p++ = ',';
  
      sym_id_name (df_data.maps.symbol_rtx[v], my_buf);

      if ((strlen(my_buf) + (p-buf_in)) < buf_len-32)
      { strcpy (p, my_buf);
        p += strlen (p);
      }
      else
      { strcpy (p, "...<more>");
        break;
      }
    }
  }
  return buf_in;
}

dump_mem_info (file, i)
FILE* file;
int i;
{
  ud_info* mem = &(df_info[IDF_MEM]);
  int v,u;
  char buf[255];

  fprintf (file, "%2d - size %2d, terms %2d+%d; objects ",
            i, mem_slots[i].vsize, mem_slots[i].terms, mem_slots[i].lt_offset);

  fprintf (file, "%s", sym_names (mem_slots[i].slot_pt, buf, sizeof(buf)));
  fprintf (file, "\nreferences:");

  for (u=1; u < NUM_UD(df_info+IDF_MEM); u++)
    if (mem->maps.ud_map[u].varno == i)
    { int r = RTX_ID (UD_VRTX(mem->maps.ud_map[u]));

      fprintf (file, "\n\tud %d,rid %d ", u, r);
      print_ud (file, mem, u);
    }
}

dump_ud_info (file, info)
FILE* file;
ud_info* info;
{
  int i;

  dump_ud (file, info, 0);

  for (i = 1; i < NUM_UD(info); i++)
  { if ((info->maps.ud_map[i].attrs & (S_INACTIVE)) !=
        (info->maps.ud_map[i-1].attrs & (S_INACTIVE)))
      fprintf (file, "\n");
    dump_ud (file, info, i);
  }
}

int
check_range (b1, s1, b2, s2, k)
register b1;
register unsigned long s1;
register b2;
register unsigned long s2;
df_range_kind k;
{
  register int ret = -1;

  if ((s1 & 0x80000000) || (s2 & 0x80000000))
  { /* If hi bit set, we don't really know how long the reference is;
       we can't prove containment or equivalence. */

    if (k != df_overlap)
      return 0;

    /* Can do better than this later. */
    return 1;
  }

  if (b2 != b1)
  { /* Pedal the sets down until the low bits are ones ... */

    while (b2 != b1 && (s1 & 1) == 0)
    { s1 >>= 1; b1++; }

    while (b2 != b1 && (s2 & 1) == 0)
    { s2 >>= 1; b2++; }

    /* Since there are at most 16 elts, if the low bit offsets
       differ by more than that, the sets are totally disjoint,
       and we are done.  Otherwise, we line the sets up and do
       logical ops on them. */
  
    if (b2 > b1)
      if ((b2-b1) > 16)
        ret = 0;
      else
        s2 <<= (b2-b1);
    else
      if ((b1-b2) > 16)
        ret = 0;
      else
        s1 <<= (b1-b2);
  }

  if (ret < 0)
    switch (k)
    {
      case df_contains:
        ret = ((s2 & ~s1) == 0);
        break;

      case df_equivalent:
        ret = (s1==s2);
        break;

      case df_overlap:
        ret = ((s1 & s2) != 0);
    }

  return ret;
}

char* mem_ud_name (u, p)
int u;
char* p;
{ 
  ud_rec* pu;
  rtx v;
  char* ret;
  lt_rec* lt;

  lt  = &df_data.lt_info;
  pu  = df_info[IDF_MEM].maps.ud_map+u;
  v   = UD_VRTX (*pu);
  ret = p;
  *p  = 0;

  if (in_flex (df_data.sets.printing_mem_id, pu->varno))
    sprintf (p, " <recurses>");
  else
  { int k;
    int plus;

    k = -1;
    plus = 0;

    set_flex_elt (df_data.sets.printing_mem_id, pu->varno);

    sprintf (p, " (");
    p += strlen (p);

    if (lt != 0)
      while (++k < INUM_DFK)
      {
        signed_byte* q  = LT_VECT(*lt, mem_slots[pu->varno].terms, k);
        int n_ud = NUM_UD (&df_info[k]);
        int i    = -1;
    
        while (++i < n_ud)
          if (q && q[i])
          {
            if (plus)
            { strcpy (p, "+");
              p += strlen (p);
            }
            else
              plus = 1;
  
            if (q[i] != 1)
            { sprintf (p, "%d*", q[i]);
              p += strlen (p);
            }
  
            p += strlen (df_info[k].ud_name (unmap_lt (lt,df_info+k,i), p));
          }
      }

    sprintf (p, ")[");

    if (lt)
    { int lo,hi;
      unsigned m;

      m  = UD_BYTES(*pu) << UD_VAR_OFFSET(*pu);
      lo = mem_slots[pu->varno].lt_offset;

      while ((m & 1) == 0)
        (lo++, m >>= 1);

      hi = lo-1;

      while (m)
        (hi++, m >>= 1);

      p += strlen (p);
      sprintf (p, "%d..%d", lo, hi);
    }

    p += strlen (p);
    sprintf (p, "]");

    clr_flex_elt (df_data.sets.printing_mem_id, pu->varno);
  }

  return ret;
}

char* sym_id_name (r, p)
rtx r;
char* p;
{ 
  sprintf (p, "#%s", XSTR (r,0));
    
  return p;
}

char* reg_ud_name (u, p)
int u;
char* p;
{ 
  ud_rec* pu;
  rtx v;
  int n;

  char* ret = p;

  pu = df_info[IDF_REG].maps.ud_map+u;
  v  = UD_VRTX (*pu);
  n  = REGNO(v);

  *p = 0;

  if (n >= FIRST_PSEUDO_REGISTER)
  { char* s = REG_USERNAME(v);

    if (s)
    { int quo = '\"';
      sprintf (p, "%c%s%c", quo, s, quo);
    }
    else
      sprintf (p, "p%d", n);

    p += strlen (p);

    df_assert (UD_VAR_OFFSET(*pu) == 0);
  }
  else
    sprintf (p, "%s", reg_names[n]);

  return ret;
}

char* sym_ud_name (u, p)
int u;
char* p;
{ 
  ud_rec* pu;
  rtx v;
  ud_info* df = df_info+IDF_SYM;

  pu = df->maps.ud_map+u;
  v  = UD_VRTX (*pu);

  return sym_id_name (v, p);
}

/* Check containment, overlap for mem variables. */
chk_mem (v1, v1_bytes, v1_off, v2, v2_bytes, v2_off, k)
df_range_kind k;
{
  int ret;
  ud_info* mem = df_info+IDF_MEM;

  /* If one of the addresses is unknown, the variables overlap, and
     containment is not possible. */

  if (v1 == 0 || v2==0)
    ret = (k == df_overlap);

  else 
  {
    mem_slot_info* mu = &mem_slots[v1];
    mem_slot_info* mw = &mem_slots[v2];

    if (mu->terms != mw->terms)
    {
      /* LT's are different, but overlap may be impossible.  Can't
         say anything about containment, tho. */

      if (ret = (k == df_overlap))
      { id_set t;
        t = and_flex (0, mu->slot_pt, mw->slot_pt);
        ret = !is_empty_flex (t);
      }
    }

    else
    {
      /* Same LT's.  See if offset and sizes indicate overlap. */

      long blt, base, uoff, woff;

      /* Get the number of the lowest addressed variable using these LT's */
      blt = mu->unum;
      df_assert (mem_slots[blt].terms == mu->terms);

      /* Get offsets from LT's for base, u, and w. */
      base = mem_slots[blt].lt_offset;
      uoff = mu->lt_offset;
      woff = mw->lt_offset;

      df_assert (v1 != v2 || uoff == woff);
      
      uoff += v1_off;
      woff += v2_off;

      ret = check_range (uoff-base,v1_bytes, woff-base,v2_bytes,k);
    }
  }
  return ret;
}

/* These routines are used to check containment and overlap for ud's. */
mem_ud_range (u, w, k)
ud_rec* u;
ud_rec* w;
df_range_kind k;
{
  int ret;
  ud_info* mem = df_info+IDF_MEM;

  mem_slot_info* mu = &mem_slots[u->varno];
  mem_slot_info* mw = &mem_slots[w->varno];

  /* If one of the addresses is unknown, the uds overlap, and
     containment is not possible. */

  if (u->varno == 0 || w->varno==0)
    ret = (k == df_overlap);

  else 
  {
    if (mu->terms != mw->terms)
    {
      /* LT's are different, but overlap may be impossible.  Can't
         say anything about containment, tho. */

      if (ret = (k == df_overlap))
      { id_set t;
        t = and_flex (0, mu->slot_pt, mw->slot_pt);
        ret = !is_empty_flex (t);
      }
    }

    else
    {
      /* Same LT's.  See if offset and sizes indicate overlap. */

      long blt, base, uoff, woff;

      /* Get the number of the lowest addressed variable using these LT's */
      blt = mu->unum;
      df_assert (mem_slots[blt].terms == mu->terms);

      /* Get offsets from LT's for base, u, and w. */
      base = mem_slots[blt].lt_offset;
      uoff = mu->lt_offset;
      woff = mw->lt_offset;

      df_assert (u->varno != w->varno || uoff == woff);
      
      uoff += UD_VAR_OFFSET (*u);
      woff += UD_VAR_OFFSET (*w);

      ret = check_range (uoff-base,UD_BYTES(*u), woff-base,UD_BYTES(*w),k);
    }
  }

  if (ret != 0 && k == df_overlap)
    ret = object_types_could_alias (UD_TYPE(*u), UD_TYPE(*w));

  return ret;
}

reg_ud_range (u, w, k)
ud_rec* u;
ud_rec* w;
df_range_kind k;
{
  int ret;

  /* For registers, varno has to be the same to have either overlap
     or containment.  If it is the same, we consider the offset of 
     the register from its base (non-zero only for hard registers)
     as well as the bit offset of the ud from its base. */

  if (ret = (u->varno==w->varno))
  {
    int uoff = UD_VAR_OFFSET(*u);
    int woff = UD_VAR_OFFSET(*w);

    ret = check_range (uoff, UD_BYTES(*u), woff,UD_BYTES(*w),k);
  }

  return ret;
}

sym_ud_range (u, w, k)
ud_rec* u;
ud_rec* w;
df_range_kind k;
{
  return u->varno == w->varno;
}

static tree last_reg_df;

setup_reg_dataflow ()
{
  ud_info* reg;

  extern rtx *regno_reg_rtx;
  extern int reg_rtx_no;

  reg  = &df_info[IDF_REG];

  if (last_reg_df != current_function_decl)
  { int j;
    rtx* r;
    static rtx non_frame_reg_rtx;
    static rtx non_local_reg_rtx;

    for (j = 0; j < FIRST_PSEUDO_REGISTER; j++)
      if (df_hard_reg[j].reg)
      { ud_set p;

        if (df_hard_reg[j].is_parm)
          p = df_data.sets.all_addr_syms_except_local;

        else if (j==REGNO(frame_pointer_rtx) || j==REGNO(stack_pointer_rtx))
          p = df_data.sets.frame_symbols;

        else if (j==REGNO(arg_pointer_rtx))
          p = df_data.sets.arg_symbols;

        else
          p = 0;

        scan_rtx_for_uds (ENTRY, &(df_hard_reg[j].reg), 0, S_DEF, p);
      }

    for (j=FIRST_PSEUDO_REGISTER; j<reg_rtx_no; j++)
      if (regno_reg_rtx[j] != 0 && GET_CODE(regno_reg_rtx[j]) == REG)
        scan_rtx_for_uds (ENTRY,&(regno_reg_rtx[j]),0,S_DEF,0);

    r = &DECL_RTL(DECL_RESULT(current_function_decl));
    if (*r && !df_data.function_never_exits)
      if (GET_CODE(*r) == REG)
        scan_rtx_for_uds(EXIT, r, 0, S_USE,
                         df_data.sets.all_addr_syms_except_local);
      else if (GET_CODE(*r) == MEM)
        scan_rtx_for_uds (ENTRY, &struct_value_rtx, 0, S_DEF,
                          df_data.sets.all_syms_except_local);

    /* We say that all memory except frame is live at entry */
    non_frame_reg_rtx = gen_reg_rtx (Pmode);
    scan_rtx_for_uds (ENTRY, &non_frame_reg_rtx,0,S_DEF,
                             df_data.sets.all_syms_except_frame);

    non_frame_mem_rtx = gen_typed_mem_rtx (void_type_node,word_mode, 0);
    XEXP(ENTRY_NOTE,5) = non_frame_mem_rtx;
    scan_rtx_for_uds(INSN_UID(ENTRY_NOTE), ENTRY_NOTE,XEXP_OFFSET(ENTRY_NOTE,5),
                     S_DEF, df_data.sets.all_addr_syms_except_frame);
    XEXP(non_frame_mem_rtx,0)=non_frame_reg_rtx;
    scan_rtx_for_uds(INSN_UID(ENTRY_NOTE), non_frame_mem_rtx,
                     XEXP_OFFSET(non_frame_mem_rtx,0),
                     S_USE, df_data.sets.all_syms_except_frame);

    if (df_data.function_never_exits)
      XEXP(EXIT_NOTE,5) = 0;
    else
    {
      /* We say that only non-local memory is live at exit. */

      non_local_reg_rtx = gen_reg_rtx (Pmode);
      scan_rtx_for_uds (ENTRY, &non_local_reg_rtx, 0, S_DEF,
                        df_data.sets.all_syms_except_local);

      non_local_mem_rtx = gen_typed_mem_rtx (void_type_node,word_mode, 0);
      XEXP(EXIT_NOTE,5) = non_local_mem_rtx;
      scan_rtx_for_uds(INSN_UID(EXIT_NOTE), EXIT_NOTE,XEXP_OFFSET(EXIT_NOTE,5),
                       S_DEF|S_USE, df_data.sets.all_addr_syms_except_local);
      XEXP(non_local_mem_rtx,0)=non_local_reg_rtx;
      scan_rtx_for_uds(INSN_UID(EXIT_NOTE), non_local_mem_rtx,
                       XEXP_OFFSET(non_local_mem_rtx,0),
                       S_USE, df_data.sets.all_syms_except_local);
    }
  
    last_reg_df = current_function_decl;
  }
}

setup_mem_dataflow ()
{
}

setup_sym_dataflow ()
{
}

dump_slot_table (file)
FILE* file;
{
  int i;
  fprintf (file, "\nslot table:");
  for (i=0; i<NUM_ID(&(df_info[IDF_MEM])); i++)
  {
    fprintf (file, "\n");
    dump_mem_info (file, i);
  }
}

debug_slot_table ()
{
  dump_slot_table (stderr);
  fprintf (stderr, "\n");
}

static int mem_alloc_df();
static int reg_alloc_df();
static int mem_note_insn_gen_kill();
static int reg_note_insn_gen_kill();
static int mem_get_insn_reaches();
static int reg_get_insn_reaches();

superflow_onetime_init()
{
  init_flex_temps();
  init_ob_container();

  if (df_data.universal_sym == 0)
  {
    extern tree ptr_type_node;

    /* universal sym is never seen in the rtl stream, but it is in every
       initial pts_to.  If it doesn't get elided from a pts_to for a
       given ptr use or def, that use or def's appearance in an address
       will overlap everything else which has the universal symbol
       in its pts_to.  Mems which have no overlaps in their pts_tos
       are not considered as allowed as alias candidates, even if we
       can't tell a thing about their addresses. */

    df_data.universal_sym = gen_typed_symref_rtx (ptr_type_node, Pmode, "___universal_sym");
    SYM_ADDR_TAKEN_P (df_data.universal_sym) = 1;

    df_data.fp_sym = gen_typed_symref_rtx (ptr_type_node, Pmode, "___fp_sym");
    SYM_ADDR_TAKEN_P (df_data.fp_sym) = 1;

    df_data.ap_sym = gen_typed_symref_rtx (ptr_type_node, Pmode, "___ap_sym");
    SYM_ADDR_TAKEN_P (df_data.ap_sym) = 1;

    MEM_DF.setup_dataflow = setup_mem_dataflow;
    REG_DF.setup_dataflow = setup_reg_dataflow;
    SYM_DF.setup_dataflow = setup_sym_dataflow;
  
    MEM_DF.ud_range = mem_ud_range;
    REG_DF.ud_range = reg_ud_range;
    SYM_DF.ud_range = sym_ud_range;
  
    MEM_DF.alloc_df = mem_alloc_df;
    REG_DF.alloc_df = reg_alloc_df;
    SYM_DF.alloc_df = 0;

    MEM_DF.get_insn_reaches = mem_get_insn_reaches;
    REG_DF.get_insn_reaches = reg_get_insn_reaches;
    SYM_DF.get_insn_reaches = 0;

    MEM_DF.note_insn_gen_kill = mem_note_insn_gen_kill;
    REG_DF.note_insn_gen_kill = reg_note_insn_gen_kill;
    SYM_DF.note_insn_gen_kill = 0;

    MEM_DF.ud_name = mem_ud_name;
    REG_DF.ud_name = reg_ud_name;
    SYM_DF.ud_name = sym_ud_name;

    MEM_DF.next_df = mem_next_df;
    REG_DF.next_df = reg_next_df;
    SYM_DF.next_df = sym_next_df;

#ifdef YMC_FLOD
    MEM_DF.is_flod = mem_is_flod;
    REG_DF.is_flod = reg_is_flod;
    SYM_DF.is_flod = sym_is_flod;
#endif
  }
}


scan_insns_for_uds (a, b, bb)
rtx a;
rtx b;
int bb;
{ /* Scan the insns between a & b.  If a is 0, it means
     the first insn, and if b is zero, it means the last insn. */

  rtx start, end;

  last_asm_vect = 0;

  if (a && (a==b || NEXT_INSN(a)==b))
    return;

  if (a == 0)
  { a = get_insns();
    if (b == 0)
      df_data.iid_invalid = 0;
  }
  else
    a = NEXT_INSN(a);

  if (MAP_SIZE(df_data.maps.insn_info, uid_info_rec_map) < NUM_UID)
    REALLOC_MAP (&df_data.maps.insn_info, NUM_UID, uid_info_rec_map);

  if (bb >= 0 && alloc_insn_flow (-1) < NUM_UID)
    alloc_insn_flow(NUM_UID);

  df_assert (a != b);
  start = a;
  end   = a;

  do
  {
    UID_RTX (INSN_UID(a)) = a;

    if (PREV_INSN(a)==0)
      IID(a) = 0;
    else
    {
      if (IID(a) <= IID(PREV_INSN(a)))
        IID(a) = IID(PREV_INSN(a)) + iid_gap;

      /* Propagate the libcall info forward. */
      INSN_RETVAL_UID(a) = INSN_RETVAL_UID(PREV_INSN(a));
    }
  
    if (bb >= 0)
    { df_assert (BLOCK_NUM(a)<0);
      BLOCK_NUM(a) = bb;
    }

    if (GET_CODE(a) == NOTE && NOTE_LINE_NUMBER(a) > 0)
      df_data.have_line_info = 1;

    if (REAL_INSN(a))
    {
      if (bb==EXIT_BLOCK)
        df_data.code_at_exit = 1;

      /* If we don't have an open libcall note, look for one. */
      if (INSN_RETVAL_UID(a) <= 0)
      { rtx note;
        if (note = find_reg_note (a, REG_LIBCALL, 0))
          INSN_RETVAL_UID(a)=INSN_UID(XEXP(note,0));
        scan_rtx_for_uds (INSN_UID(a), a, PAT_OFFSET (a), 0, df_data.sets.all_symbols);
#ifdef CALL_INSN_FUNCTION_USAGE
        if (GET_CODE(a)==CALL_INSN)
          scan_rtx_for_uds (INSN_UID(a), a, CUSE_OFFSET (a), 0, df_data.sets.all_symbols);
#endif
      }

      /* If we have one, and this is its retval, remember that we no longer
         have one open. */

      else if (INSN_RETVAL_UID(a) == INSN_UID(a))
      { df_assert (find_reg_note (a, REG_RETVAL, 0));
        scan_rtx_for_uds (INSN_UID(a), a, PAT_OFFSET (a), 0, df_data.sets.all_symbols);
#ifdef CALL_INSN_FUNCTION_USAGE
        if (GET_CODE(a)==CALL_INSN)
          scan_rtx_for_uds (INSN_UID(a), a, CUSE_OFFSET (a), 0, df_data.sets.all_symbols);
#endif
        INSN_RETVAL_UID(a) = -INSN_RETVAL_UID(a);
      }
      else
      {
        scan_rtx_for_uds (INSN_UID(a), a, PAT_OFFSET (a), 0, df_data.sets.all_symbols);
#ifdef CALL_INSN_FUNCTION_USAGE
        if (GET_CODE(a)==CALL_INSN)
          scan_rtx_for_uds (INSN_UID(a), a, CUSE_OFFSET (a), 0, df_data.sets.all_symbols);
#endif
      }

      end = a;
    }

    a = NEXT_INSN(a);
  }
  while
    (a != b);

  /* If iids aren't reliable, don't sweat it, just remember so that
     when we get around to needing them, we can renumber. */

  if (a != 0 && IID(a) <= IID(PREV_INSN(a)))
    df_data.iid_invalid = 1;

  if (bb >= 0)
  { if (insn_comes_before (start, BLOCK_HEAD(bb)))
      BLOCK_HEAD(bb) = start;

    if (insn_comes_before (BLOCK_END(bb), end))
      BLOCK_END(bb) = end;
  }
  last_asm_vect = 0;
}

static
superflow_scan()
{
  /* Some rtl may have changed since mem_info,reg_info, and
     sym_info were updated last.

     First, number basic blocks, set up BLOCK_HEAD, BLOCK_END,
     BLOCK_DROPS_IN, etc. Also, delete unreachable blocks. */

  ud_info *info, *reg, *sym, *mem;
  int insns, i, v;
  int next_insn_iid = 1;
  rtx insn;
  int prev_time = assign_run_time (IDT_SCAN+0);

  get_predecessors (df_data.mem_df_level > 0);
  assign_run_time (IDT_SCAN+1);

  superflow_onetime_init();
  /*  Allocate ud map and uid space.  For ud map space, apply the
      observed ratio of uds/insns from the previous run.

      One of the effects of this is that if the space is still
      around, and if we are still working on the function and it
      has not changed in size very much, the allocations will
      probably be left alone. */

  insns = NUM_UID;

  ALLOC_MAP (&df_data.maps.insn_info, insns, uid_info_rec_map);

  for (info = &(df_info[0]); info != &(df_info[INUM_DFK]); info++)
  { ALLOC_MAP (&info->maps.ud_map,  insns * 2, ud_rec_map);
    ALLOC_MAP (&info->maps.uid_map, insns, uid_rec_map);
    NUM_UD(info) = 1;
    info->df_state = 0;
  }

  mem = &(df_info[IDF_MEM]);
  reg = &(df_info[IDF_REG]);
  sym = &(df_info[IDF_SYM]);

  new_flex_pool (&df_data.pools.symbol_pool, 32, 32,F_DENSE);
  new_flex_pool (&df_data.pools.shared_sym_pool, 32, 32,F_DENSE);

  ALLOC_MAP (&df_data.maps.symbol_set, LAST_RTX_ID(sym)+1, flex_set_map);
  ALLOC_MAP (&df_data.maps.symbol_rtx, LAST_RTX_ID(sym)+1, rtx_map);

  df_data.maps.symbol_set[0] = get_empty_flex (df_data.pools.symbol_pool);

  df_data.sets.addr_syms   = alloc_flex (df_data.pools.shared_sym_pool);
  df_data.sets.all_symbols = alloc_flex (df_data.pools.shared_sym_pool);

  df_data.sets.frame_symbols = alloc_flex (df_data.pools.shared_sym_pool);
  df_data.sets.arg_symbols = alloc_flex (df_data.pools.shared_sym_pool);
  df_data.sets.local_symbols = alloc_flex (df_data.pools.shared_sym_pool);
  df_data.sets.no_symbols = alloc_flex (df_data.pools.shared_sym_pool);

  df_data.sets.all_syms_except_frame = alloc_flex (df_data.pools.shared_sym_pool);
  df_data.sets.all_syms_except_arg = alloc_flex (df_data.pools.shared_sym_pool);
  df_data.sets.all_syms_except_local = alloc_flex (df_data.pools.shared_sym_pool);

  df_data.sets.all_addr_syms_except_frame = alloc_flex (df_data.pools.shared_sym_pool);
  df_data.sets.all_addr_syms_except_arg = alloc_flex (df_data.pools.shared_sym_pool);
  df_data.sets.all_addr_syms_except_local = alloc_flex (df_data.pools.shared_sym_pool);

  {
    new_flex_pool (&df_data.pools.pm_pool, LAST_RTX_ID(sym)+1, 1, F_DENSE);
    df_data.sets.printing_mem_id = alloc_flex (df_data.pools.pm_pool);
  }

  LAST_RTX_ID(reg) = 0;
  LAST_RTX_ID(mem) = 0;

  NUM_ID(reg) = 0;

  clear_symref_var_ids();

  RTX_VAR_ID(df_data.universal_sym)=1;
  df_data.maps.symbol_rtx[1] = df_data.universal_sym;

  set_flex_elt (df_data.sets.addr_syms, 1);
  set_flex_elt (df_data.sets.all_symbols, 1);
  set_flex_elt (df_data.sets.all_syms_except_frame, 1);
  set_flex_elt (df_data.sets.all_syms_except_arg, 1);
  set_flex_elt (df_data.sets.all_syms_except_local, 1);
  set_flex_elt (df_data.sets.all_addr_syms_except_frame, 1);
  set_flex_elt (df_data.sets.all_addr_syms_except_arg, 1);
  set_flex_elt (df_data.sets.all_addr_syms_except_local, 1);

  RTX_VAR_ID(df_data.fp_sym)=2;
  df_data.maps.symbol_rtx[2] = df_data.fp_sym;

  set_flex_elt (df_data.sets.addr_syms, 2);
  set_flex_elt (df_data.sets.all_symbols, 2);
  set_flex_elt (df_data.sets.all_syms_except_arg, 2);
  set_flex_elt (df_data.sets.all_addr_syms_except_arg, 2);
  set_flex_elt (df_data.sets.frame_symbols, 2);

  RTX_VAR_ID(df_data.ap_sym)=3;
  df_data.maps.symbol_rtx[3] = df_data.ap_sym;

  set_flex_elt (df_data.sets.addr_syms, 3);
  set_flex_elt (df_data.sets.all_symbols, 3);
  set_flex_elt (df_data.sets.all_syms_except_frame, 3);
  set_flex_elt (df_data.sets.all_addr_syms_except_frame, 3);
  set_flex_elt (df_data.sets.arg_symbols, 3);

  NUM_ID(sym) = 4;
  for (v = 1; v < 4; v++)
  { ud_set t = alloc_flex(df_data.pools.symbol_pool);
    set_flex_elt ((df_data.maps.symbol_set[v]=t), v);
  }
  df_data.shared_syms = 0;

  ALLOC_MAP (&mem_slots, MAX(32, NUM_ID(mem)), mem_slot_info_map);
  NUM_ID(mem) = 1;
  mem_slots[0].slot_pt = df_data.sets.all_symbols;

  /* Scan the insns in the function and build up the ud structures. */
  insn = get_insns ();
  df_data.have_line_info = 0;
  df_data.code_at_exit = 0;
  
  assign_run_time (IDT_SCAN+2);
  scan_insns_for_uds (0, 0, -1);

  assign_run_time (prev_time);
}

static
get_sym_defs_reaching(df)
ud_info *df;
{
  /* No fancy dataflow is needed for symbols, all defs reach all uses of the
   * same symbol.  Each use of a symbol is reached only by its single
   * definition.
   */
  register int n_id;
  register int i;
  register int n_ud;
  register int u;

  n_id = NUM_ID(df);
  n_ud = NUM_UD(df);

  if (n_id == 0 || n_ud == 0)
    return 0;

  df_assert (df->maps.defs_reaching == 0 && df->maps.uses_reached == 0);
  df_assert (df->pools.ud_pool == 0 && df->pools.mondo_ud_pool == 0);

  ALLOC_MAP (&df->maps.defs_reaching, n_ud, flex_set_map);
  ALLOC_MAP (&df->maps.uses_reached, n_ud, flex_set_map);

  /* We use 2 pools - a sparse one for small (<= SPARSE_REACH_ELTS) reach
     sets, and a dense one for large reach sets.  There's lots of room to
     improve this. */

  new_flex_pool (&df->pools.ud_pool, SPARSE_REACH_ELTS, n_ud * 2, F_SPARSE);
  new_flex_pool (&df->pools.mondo_ud_pool, n_ud, 16, F_DENSE);

  df->sets.sparse_reach = alloc_flex (df->pools.ud_pool);
  df->sets.dense_reach = alloc_flex (df->pools.mondo_ud_pool);

  {
    ud_set t = get_empty_flex (df->pools.ud_pool);
    for (u = 0; u < n_ud; u++)
    {
      df->maps.defs_reaching[u] = t;
      df->maps.uses_reached[u] = t;
    }
  }

  for (i = 0; i < n_id; i++)
  {
    if (df->maps.id_map[i].uds_used_by != -1)
    {
      int def = -1;
      int cnt = 0;
      ud_set defs_reach = alloc_flex(df->pools.ud_pool);

      /* the set defs_reach will contain the singular element def */
      clr_flex(defs_reach);
      clr_flex(df->sets.sparse_reach);
      clr_flex(df->sets.dense_reach);

      for (u = df->maps.id_map[i].uds_used_by; u != -1;
           u = df->uds_used_by_pool[u])
      {
        if ((UDN_ATTRS(df, u) & S_DEF) != 0)
          def = u;
        if ((UDN_ATTRS(df, u) & S_USE) != 0)
        {
          df->maps.defs_reaching[u] = defs_reach;
          if (cnt++ < SPARSE_REACH_ELTS)
            set_flex_elt(df->sets.sparse_reach, u);
          set_flex_elt(df->sets.dense_reach, u);
        }
      }

      df_assert(def != -1);

      /* now we just set the single def in t */
      set_flex_elt(defs_reach, def);

      if (cnt <= SPARSE_REACH_ELTS)
      {
        df->maps.uses_reached[def] = df->sets.sparse_reach;
        df->sets.sparse_reach = alloc_flex(df->pools.ud_pool);
      }
      else
      {
        df->maps.uses_reached[def] = df->sets.dense_reach;
        df->sets.dense_reach = alloc_flex(df->pools.mondo_ud_pool);
      }
    }
  }

  trace (defs_reaching_exit, df);
  SET_DF_STATE (df, DF_REACHING_DEFS);
  return 1;
}

static
get_defs_reaching (df)
ud_info* df;
{
  /* For every ud u, compute defs_reaching[u], which is the set of all
     ud's which could define u. */

  int n_ud = NUM_UD(df);
  int bb_words = df->bb_bytes/2;

  df_assert (df->maps.defs_reaching == 0 && df->maps.uses_reached == 0);
  df_assert (df->pools.ud_pool == 0 && df->pools.mondo_ud_pool == 0);

  ALLOC_MAP (&df->maps.defs_reaching, n_ud, flex_set_map);
  ALLOC_MAP (&df->maps.uses_reached, n_ud, flex_set_map);

  /* We use 2 pools - a sparse one for small (<= SPARSE_REACH_ELTS) reach
     sets, and a dense one for large reach sets.  There's lots of room to
     improve this. */

  new_flex_pool (&df->pools.ud_pool, SPARSE_REACH_ELTS, n_ud * 2, F_SPARSE);
  new_flex_pool (&df->pools.mondo_ud_pool, n_ud, 16, F_DENSE);

  df->sets.sparse_reach = alloc_flex (df->pools.ud_pool);
  df->sets.dense_reach = alloc_flex (df->pools.mondo_ud_pool);

  { int u;
    ud_set t = get_empty_flex (df->pools.ud_pool);
    for (u = 0; u < n_ud; u++)
      df->maps.defs_reaching[u] = t;
  }

  { register int bb_num;
    register unsigned short* in = df->in;

    ud_set reach = 0;

    reuse_flex (&reach,  n_ud, 1, F_DENSE);

    for (bb_num=0; bb_num < N_BLOCKS; bb_num++, in += bb_words)
    {
      register rtx insn = BLOCK_HEAD (bb_num);
      register rtx end  = BLOCK_END  (bb_num);
      register rtx prev;
  
      do
      {
        register int lelt;
        register int rc;
  
        if ((lelt = df->maps.uid_map[INSN_UID(insn)].uds_in_insn) != -1)
        {
          do
          {
            if (rc = df->get_insn_reaches(df->maps.ud_map+lelt,in))
            {
              if (rc <= SPARSE_REACH_ELTS)
              { /* Clear the current dense set, use the current sparse set, and
                   then allocate a new current sparse set. */
      
                clr_flex (df->sets.dense_reach);
      
                df->maps.defs_reaching[lelt] = df->sets.sparse_reach;
                df->sets.sparse_reach = alloc_flex(df->pools.ud_pool);
              }
              else
              { /* Clear the current sparse set, use the current dense set, and
                   then allocate a new current dense set. */
      
                clr_flex (df->sets.sparse_reach);
      
                df->maps.defs_reaching[lelt] = df->sets.dense_reach;
                df->sets.dense_reach = alloc_flex(df->pools.mondo_ud_pool);
              }
            }
            lelt = df->uds_in_insn_pool[lelt];
          } while (lelt != -1);

          df->note_insn_gen_kill (insn, in, 0);
        }

        prev = insn;
        insn = NEXT_INSN (insn);
      }
      while
        (insn != 0 && prev != end);
    }
    reuse_flex (&reach,  0, 0, 0);
  }

  { /* For every ud u, compute uses_reached[u], which is the set of all ud's
       which could use u. */
  
    register int u;
    register int r = -1;
    ud_set reuse_ud = 0;
  
    reuse_flex (&reuse_ud, n_ud, 1, F_DENSE);
  
    /* For all uds u,
         For all defs d reaching u,
           put u in uses_reached[d].  */
  
    for (u = 0; u < n_ud; u++)
    { register int d = -1;
      while ((d=next_flex(df->maps.defs_reaching[u],d)) != -1)
      { register ud_set* uses = df->maps.uses_reached + d;
  
        if (!FLEX_SET_NUM(*uses))
        { /* no set allocated yet. */
  
          if (r == -2)
            /* -2 means it is worth looking for a reuse. */
            r = next_flex (reuse_ud, -1);
  
          if (r == -1)
            /* grab a small one. */
            *uses = alloc_flex(df->pools.ud_pool);
          else
          { /* we found one ! */
            clr_flex (*uses = get_flex (df->pools.ud_pool, r));
            clr_flex_elt (reuse_ud, r);
            r = -2;
          }
        }
        else
        { if (FLEX_POOL(*uses) == df->pools.ud_pool && FULL_SPARSE(*uses))
          { /* set allocated, but it is full. */
            set_flex_elt (reuse_ud, r = FLEX_SET_NUM(*uses));
  
            *uses = copy_flex (alloc_flex(df->pools.mondo_ud_pool), *uses);
          }
        }
        set_flex_elt (*uses, u);
      }
    }

    reuse_flex (&reuse_ud, 0, 0, 0);
  }

  trace (defs_reaching_exit, df);
}

static int
mem_note_insn_kills (insn, kill, msk)
rtx insn;
unsigned short* kill;
int msk;
{
  ud_info* info = df_info+IDF_MEM;
  uid_rec* i = &(info->maps.uid_map[INSN_UID(insn)]);
  register int lelt;

  for (lelt = i->defs_in_insn; lelt != -1;
       lelt = info->defs_in_insn_pool[lelt])
  { /* This loop sets the bits in 'kill' for each def killed by 
       this insn. */
       
    ud_rec* u   = info->maps.ud_map+lelt;
    int v = UD_VARNO(*u);

    if (v != 0 && (UD_ATTRS(*u) & S_KILL) != 0)
    {
      unsigned uw = UD_BYTES(*u) << UD_VAR_OFFSET(*u);
      unsigned short *stop
        = kill+MEM_VPD*(info->attr_sets_last[I_DEF]+1);
      unsigned short *k;
      int d;

      for ((k=kill+MEM_VPD),(d=1); k!=stop;
           (d++, k += MEM_VPD))
      {
        /* If this variable kills a piecewise trackable variable, we
           can blow off all the bits in every def of the pieces. */

        if (v < MEM_VPD)
          if (msk)
            k[v] |= uw;
          else
            k[v] &= ~uw;

        /* We can always blow off the bits gen'd by the same variable. */
        if (v == UDN_VARNO(info,d))
        { 
          if (msk)
            k[0] |= uw;
          else
            k[0] &= ~uw;

          /* If the def is contained by the kill, we can blow away
             all variables the def would define ... they get redefined
             by the kill. */

          if (((UDN_BYTES(info,d) << UDN_VAR_OFFSET(info,d)) & ~uw) == 0)
          { int j;
            for (j = 0; j < MEM_VPD; j++)
              k[j] = msk;
          }
        }
      }
    }
  }
}

static int
mem_note_insn_defs (insn, defs)
rtx insn;
unsigned short* defs;
{
  register ud_info* info = df_info+IDF_MEM;
  register uid_rec* i = &(info->maps.uid_map[INSN_UID(insn)]);
  register int lelt;

  for (lelt = i->defs_in_insn; lelt != -1;
       lelt = info->defs_in_insn_pool[lelt])
  { /* This loop puts all defs made in 'insn' into 'defs'. */

    ud_rec* u   = info->maps.ud_map+lelt;
    int v,uv;

    if ((uv=UD_VARNO(*u)) == 0)
      for (v = 0; v < MEM_VPD; v++)
        (defs+lelt * MEM_VPD)[v] = -1;
    else
    {
      unsigned uw = UD_BYTES(*u) << UD_VAR_OFFSET(*u);

      /* 'vars' is the set of all variables that *u could def.  Loop
         over 'vars' and set appropriate bits in defs for each variable v. */

      for (v = 1; v < MEM_VPD; v++)
      {
        if (v != uv)
          (defs+lelt * MEM_VPD)[v] = -1;
        else
          (defs+lelt * MEM_VPD)[v] |= uw;
      }

      (defs+lelt * MEM_VPD)[0] |= uw;
    }
  }
}

static int
mem_note_insn_gen_kill (insn, gen, kill)
rtx insn;
unsigned short* gen;
unsigned short* kill;
{
  ud_info* mem = df_info+IDF_MEM;

  mem_note_insn_kills (insn, gen, 0);
  mem_note_insn_defs  (insn, gen);

  if (kill)
    mem_note_insn_kills (insn, kill, -1);
}

static unsigned long
reg_nibble_mask (elt)
int elt;
{
  register ud_rec*  u  = &df_info[IDF_REG].maps.ud_map[elt];
  register unsigned uw = UD_BYTES(*u) << UD_VAR_OFFSET(*u);

  register unsigned long mskf = 0xf;
  register unsigned long msk1 = 0x1;
  register unsigned long ret  = 0;

  while (uw)
  {
    if (uw & mskf)
    { ret |= msk1;
      uw &= ~mskf;
    }

    msk1 <<= 1;
    mskf <<= 4;
  }

  df_assert (ret < 16);
  return ret;
}

static int
reg_note_insn_gen_kill (insn, gen, kill)
rtx insn;
register unsigned short* gen;
register unsigned short* kill;
{
  register int iuid = INSN_UID(insn);
  register int last_def = LAST_FLOW_DEF(df_info+IDF_REG);
  register int lelt;

  for (lelt = df_info[IDF_REG].maps.uid_map[iuid].defs_in_insn; lelt != -1;
       lelt = df_info[IDF_REG].defs_in_insn_pool[lelt])
  {
    register ud_rec*  u  = &df_info[IDF_REG].maps.ud_map[lelt];
    register unsigned uw = reg_nibble_mask (lelt);

    if (lelt <= last_def)
      gen[lelt/4] |= uw << ((lelt & 3) * 4);
  
    if (UD_ATTRS(*u) & S_KILL)
    {
      register int i;
      register int *uds_pool;

      uds_pool = df_info[IDF_REG].uds_used_by_pool;
      for (i = df_info[IDF_REG].maps.id_map[UD_VARNO(*u)].uds_used_by;
           i != -1 && i <= last_def;
           i = uds_pool[i])
      {
        if (kill)
          kill[i/4] |= uw << ((i & 3) * 4);

        /* Everything defined in this insn gets into gen, until
           somebody later in the block wipes it out. */

        if (iuid != UDN_INSN_UID(&df_info[IDF_REG], i))
          gen[i/4] &= ~(uw << ((i & 3) * 4));
      }
    }
  }
}

static int
mem_alloc_df ()
{
  /* Allocate space for gen,kill, in, and out.  We allocate
     16 bits per def per variable per block, and if we can't
     handle that, we limit the number of variables we track. */

  ud_info* info = df_info+IDF_MEM;
  int nv, nd, lim;
  jmp_buf_struct* save_env = xmalloc_handler;

  df_assert (xmalloc_handler);
  xmalloc_handler = &xmalloc_ret_zero;

  nd = info->attr_sets_last[I_DEF]+1;

  lim = MAX ((65536 * 3)/(N_BLOCKS * nd), 1);

  /* lim is the number of variables such that the total allocations
     for gen, kill, in, out would take about 1.5 meg, given the number
     of basic blocks and defs. */

  /* For now, lets just call it a day if we would go above 1.5 meg. */
  if (NUM_ID(info) > lim)
    df_error (df_capacity, NUM_ID(info), lim);

  nv = MIN (lim, NUM_ID(info));


  MEM_VPD = nv;
  info->bb_bytes     = ROUND (nd * nv * 2, 4);

  xunalloc (&info->kill);
  xunalloc (&info->gen);
  xunalloc (&info->out);
  xunalloc (&info->in);

  /* alloc space for info->gen, info->kill, in, and out ... */
  while (
         !(info->in   =(unsigned short *)xmalloc (N_BLOCKS * info->bb_bytes)) ||
         !(info->out  =(unsigned short *)xmalloc (N_BLOCKS * info->bb_bytes)) ||
         !(info->gen  =(unsigned short *)xmalloc (N_BLOCKS * info->bb_bytes)) ||
         !(info->kill =(unsigned short *)xmalloc (N_BLOCKS * info->bb_bytes)))

  {
    xunalloc (&info->kill);
    xunalloc (&info->gen);
    xunalloc (&info->out);
    xunalloc (&info->in);

    /* If we get down to a single variable and can't get the room, we have to
       give up. */

    if (MEM_VPD == 1)
    { 
      xmalloc_handler = save_env;
      longjmp (xmalloc_handler->jmp_buf, 1);
    }

#ifdef SELFHOST
    warning("reducing MEM_VPD from %d to %d",MEM_VPD,nv/2);
#endif
    MEM_VPD = (nv /= 2);
    info->bb_bytes     = ROUND (nd * nv * 2, 4);
  }

  xmalloc_handler = save_env;
}

static int
reg_alloc_df ()
{
  /* Allocate space for gen,kill, in, and out.  We allocate
     16 bits per def per register per block, and if we can't
     handle that, we give up. */

  ud_info* info = df_info+IDF_REG;
  int nd = LAST_FLOW_DEF(info)+1;

  int old_bb_bytes = ROUND((info->attr_sets_last[I_DEF]+1)*2,4);

  info->bb_bytes   = ROUND (((nd+1)/2), 4);

  xunalloc (&info->kill);
  xunalloc (&info->gen);
  xunalloc (&info->out);
  xunalloc (&info->in);

  /* For now, lets just call it a day if we would go above 1.5 meg. */
  /* FIXTHIS keep the test as as it was before conversion to 1 nibble/def,
     so same code will be generated during testing */
  if ((4 * N_BLOCKS * old_bb_bytes) > (65536 * 4 * 6))
    df_error (df_capacity, 4 * N_BLOCKS * old_bb_bytes, (65536 * 4 * 6));

  /* alloc space for info->gen, info->kill, in, and out ... */
  if    (
         !(info->in   = (unsigned short *)malloc (N_BLOCKS * info->bb_bytes)) ||
         !(info->out  = (unsigned short *)malloc (N_BLOCKS * info->bb_bytes)) ||
         !(info->gen  = (unsigned short *)malloc (N_BLOCKS * info->bb_bytes)) ||
         !(info->kill = (unsigned short *)malloc (N_BLOCKS * info->bb_bytes)))

    { df_assert (xmalloc_handler);
      longjmp (xmalloc_handler->jmp_buf, 1);
    }
}

static int
mem_get_insn_reaches (u, in_out)
ud_rec* u;
unsigned short * in_out;
{
  ud_info* mem;
  int dcnt, wpd, rcnt, uv;
  unsigned uw;
  unsigned short *p;
  ud_rec* d, *stop;

  mem  = df_info+IDF_MEM;
  uw   = UD_BYTES(*u) << UD_VAR_OFFSET(*u);
  uv   = UD_VARNO(*u);
  wpd  = MEM_VPD;
  dcnt = mem->attr_sets_last[I_DEF]+1;
  rcnt = 0;

  if (uv == 0 || uv >= wpd)
  { for (((p=in_out),(d=mem->maps.ud_map)),(stop=d+dcnt);d != stop; d++, p+=wpd)
      if (p[0])
      {
        int      dv = UD_VARNO(*d);
        unsigned dw = UD_BYTES(*d) << UD_VAR_OFFSET(*d);
  
        if ((dv!=uv && mem->ud_range(d,u,df_overlap)) || (uw & dw))
        {
          if (rcnt++ < SPARSE_REACH_ELTS)
            set_flex_elt (mem->sets.sparse_reach, d-mem->maps.ud_map);

          set_flex_elt (mem->sets.dense_reach, d-mem->maps.ud_map);
        }
      }
  }
  else
  { for (((p=in_out),(d=mem->maps.ud_map)),(stop=d+dcnt);d != stop; d++, p+=wpd)
      if (p[0] && (p[uv] & uw) && mem_ud_range (d,u,df_overlap))
      {
        if (rcnt++ < SPARSE_REACH_ELTS)
          set_flex_elt (mem->sets.sparse_reach, d-mem->maps.ud_map);

        set_flex_elt (mem->sets.dense_reach, d-mem->maps.ud_map);
      }
  }
  return rcnt;
}

static int
reg_get_insn_reaches (u, in_out)
ud_rec* u;
unsigned short * in_out;
{
  int last_flow_def,last_def,rcnt,d, *uds_pool;
  unsigned uw;
  ud_info* df;

  /*
   * an interesting and useful feature is that for the register dataflow
   * each short in the dataflow set is essentially the reach set for that
   * particular ud number.  This allows us to not have to go through the
   * whole set looking for stuff with the same variable number, but to simply
   * use the list already set up that maps variable ids to all the uds
   * using it.
   */

  df = df_info+IDF_REG;

  last_def      = df->attr_sets_last[I_DEF];
  last_flow_def = LAST_FLOW_DEF(df);
  rcnt = 0;
  uw   = reg_nibble_mask(u-df->maps.ud_map);
  uds_pool = df->uds_used_by_pool;

  for (d = df->maps.id_map[UD_VARNO(*u)].uds_used_by; d != -1;
       d = uds_pool[d])
  {
    /* If a def does not have S_FLOD, it reaches all uses */
    if (d <= last_def && (d > last_flow_def ||
                         (in_out[d>>2] & (uw << ((d & 3) * 4))) != 0))
    {
      if (rcnt++ < SPARSE_REACH_ELTS)
        set_flex_elt (df->sets.sparse_reach, d);

      set_flex_elt (df->sets.dense_reach, d);
    }
  }

  return rcnt;
}

do_reaching_defs (info)
ud_info* info;
{ /* Calculate RD's using as input a map indicating the sets of defs
     each insn produces, and a map indicating the variable associated
     with each def.  Alias effects have been fully exposed.

     Place in defs_reaching a map indexed by ud which indicates the defs
     reaching each ud. */

  register int bb_words, bb_longs, bb_bytes, n_blocks;
  register int bb_num;
  int *preds;
  register int pred_inx;
  int pred_blk_size;
  register unsigned long *tin;
  
  int prev_time = assign_run_time (IDT_RD+(info-df_info));

  if (GET_DF_STATE(info) & DF_REACHING_DEFS)
    return assign_run_time (prev_time), 0;

  restart_dataflow (info);

  free_flow_space (info);

  if (info == df_info + IDF_SYM)
  {
    int r = get_sym_defs_reaching(info);
    return assign_run_time (prev_time), r;
  }

  info->alloc_df ();

  bb_bytes = info->bb_bytes;
  bb_words = bb_bytes / 2;
  bb_longs = bb_words / 2;
  n_blocks = N_BLOCKS;
  tin = (unsigned long *)xmalloc(bb_bytes);

  preds = df_data.maps.preds_alt;

  /* compute gen and kill, and initialize in and out. */
  bzero (info->in,   n_blocks * bb_bytes);
  bzero (info->out,  n_blocks * bb_bytes);
  bzero (info->gen,  n_blocks * bb_bytes);
  bzero (info->kill, n_blocks * bb_bytes);

  {
    register unsigned short* gen  = info->gen;
    register unsigned short* kill = info->kill;
    register unsigned short* out  = info->out;
  
    for (bb_num=0; bb_num < n_blocks; 
         bb_num++,(gen += bb_words),(kill += bb_words),(out += bb_words))
    { register rtx insn = BLOCK_HEAD(bb_num);
      register rtx end  = BLOCK_END(bb_num);
      register rtx prev;
  
      do
      { if (info->maps.uid_map[INSN_UID(insn)].uds_in_insn != -1)
          info->note_insn_gen_kill (insn, gen, kill);
        prev = insn;
        insn = NEXT_INSN (insn);
      }
      while
        (insn != 0 && prev != end);
  
      /*  Initialise out[bb_num]. */
      bcopy (gen, out, bb_bytes);
    }
  }

  /* Main DFA loop. */
  { register int change;
    register unsigned long *lgen, *lkill, *lin, *lout, *out;
    register int j;
    out = (unsigned long *)info->out;

    do
    {
      change = 0;

      bb_num = 0;
      pred_inx = 0;
      lgen = (unsigned long *)info->gen;
      lkill = (unsigned long *)info->kill;
      lin = (unsigned long *)info->in;
      lout = (unsigned long *)info->out;

      while (bb_num < n_blocks)
      {
        register int p;

        /* Set in[bb_num] to union of outs of preds of bb_num ... */

        if ((p = preds[pred_inx++]) != -1)
        {
          for (j = 0; j < bb_longs; j++)
            tin[j] = (out+p*bb_longs)[j];

          while ((p=preds[pred_inx++]) != -1)
            for (j = 0; j < bb_longs; j++)
              tin[j] |= (out+p*bb_longs)[j];

          for (j = 0; j < bb_longs; j++)
          {
            if (lin[j] != tin[j])
            {
              change = 1;
              lin[j] = tin[j];
              lout[j] = (tin[j] & ~lkill[j]) | lgen[j];
            }
          }
        }

        lkill += bb_longs;
        lgen  += bb_longs;
        lin   += bb_longs;
        lout  += bb_longs;

        bb_num++;
      }
    }
    while (change);
  }
  free(tin);

  if (info != df_info + IDF_SYM)
    trace (defs_reaching_entry, info);

  /* discard unneeded space, but we still need the in sets. */
  xunalloc (&info->kill);
  xunalloc (&info->gen);
  xunalloc (&info->out);

  get_defs_reaching (info);

  /* now discard the in sets. */
  xunalloc (&info->in);

  SET_DF_STATE (info, DF_REACHING_DEFS);
  return assign_run_time (prev_time), 1;
}

lookup_ud (insn, context, df)
int insn;
rtx* context;
ud_info** df;
{
  register int *l_p;
  register int lelt = -1;
  *df = DF_INFO (*context);

  if (*df != 0)
  {
    lelt = (*df)->maps.uid_map[insn].uds_in_insn;
    while (lelt != -1 &&
           context != &UD_RTX((*df)->maps.ud_map[lelt]))
      lelt = (*df)->uds_in_insn_pool[lelt];
  }

  return lelt;
}

dump_lt_vect (file, n, lt)
FILE* file;
int n;
lt_rec* lt;
{
  int k = 0;
  int plus = 0;

  while (k < INUM_DFK)
  {
    signed_byte* p = LT_VECT(*lt, n, k);
    int n = NUM_UD (&df_info[k]);
    int i = 0;
    char* name = df_names[k];

    while (i < n)
    {
      if (p[i])
      { if (plus) fprintf (file, " + ");  plus = 1;

        if (p[i] == 1)
          fprintf (file, "%s:%d", name, unmap_lt(lt,df_info+k,i));
        else
          fprintf (file, "(%s:%d * %d)", name, unmap_lt(lt,df_info+k,i), p[i]);
      }
      i++;
    }
    k++;
  }
}

debug_lt_vect (n, lt)
int n;
lt_rec* lt;
{
  dump_lt_vect (stderr, n, lt);
  fprintf (stderr, "\n");
}

insn_comes_before (p, q)
rtx p;
rtx q;
{
  df_assert (BLOCK_NUM(p) == BLOCK_NUM(q));

  if (df_data.iid_invalid)
  { rtx i = get_insns();

    IID(i) = 0;
    
    while (i = NEXT_INSN(i))
      IID(i) = IID(PREV_INSN(i)) + iid_gap;

    df_data.iid_invalid = 0;
  }
  return (IID(p) < IID(q));
}

/* Return 1 iff the execution pattern is guaranteed to be a -> b -> c.  */

int
b_hides_a_from_c (a,b,c)
rtx a;
rtx b;
rtx c;
{
  int an = BLOCK_NUM(a);
  int bn = BLOCK_NUM(b);
  int cn = BLOCK_NUM(c);
  int i;

  i = 0;
  if ( an == bn ) i += 4;
  if ( bn == cn ) i += 2;
  if ( an == cn ) i += 1;
  switch (i)
  {
    case 0x0:
      return !path_from_a_to_c_not_b (an, bn, cn);

    case 0x1:
      /* a and c are in the same block, and b isn't in that block.  If a
         comes before c, b can't possibly hide a.  Otherwise, b may hide a
         if there is no path from an to an which evades bn. */

      if (insn_comes_before(a,c))
        return 0;

      for (cn=next_flex(df_data.maps.succs[an],-1); cn!= -1; cn=next_flex(df_data.maps.succs[an],cn))
        if (path_from_a_to_c_not_b (cn,bn,an))
          return 0;

      return 1;

    case 0x2:
      /* b and c are in the same block, and a is nowhere around. */
      return insn_comes_before (b,c);

    case 0x4:
      /* c can't be between a and b.  Whichever of a,b is last in the
         block is the one that most likely actually reaches c. */
      return insn_comes_before (a,b);

    case 0x7:
      if (insn_comes_before (a,b))
        return insn_comes_before (b,c) || insn_comes_before (c,a);

      if (insn_comes_before (b,c) && insn_comes_before (c,a))
        return 1;

      return 0;

    case 0x3:
    case 0x5:
    case 0x6:
      df_assert (0);
      return 0;
  }
}

int
insn_path_from_a_to_c_not_b (a,b,c)
rtx a;
rtx b;
rtx c;
{
  int an = BLOCK_NUM(a);
  int bn = BLOCK_NUM(b);
  int cn = BLOCK_NUM(c);
  int i;

  i = 0;
  if ( an == bn ) i += 4;
  if ( bn == cn ) i += 2;
  if ( an == cn ) i += 1;
  switch (i)
  {
    case 0x0:
      return path_from_a_to_c_not_b (an, bn, cn);

    case 0x1:
      /* a and c are in the same block, and b isn't in that block.  If a
         comes before c, b can't possibly hide a.  Otherwise, b may hide a
         if there is no path from an to an which evades bn. */

      if (insn_comes_before(a,c))
        return 1;

      for (cn=next_flex(df_data.maps.succs[an],-1); cn!= -1; cn=next_flex(df_data.maps.succs[an],cn))
        if (path_from_a_to_c_not_b (cn,bn,an))
          return 1;

      return 0;

    case 0x2:
      /* b and c are in the same block, and a is nowhere around. */
      return !insn_comes_before (b,c);

    case 0x4:
      /* c can't be between a and b.  Whichever of a,b is last in the
         block is the one that most likely actually reaches c. */
      return !insn_comes_before (a,b);

    case 0x7:
      if (insn_comes_before (a,b))
        return !insn_comes_before (b,c) && !insn_comes_before (c,a);

      if (insn_comes_before (b,c) && insn_comes_before (c,a))
        return 0;

      return 1;

    case 0x3:
    case 0x5:
    case 0x6:
      df_assert (0);
      return 0;
  }
}

int
find_equiv (info, z)
ud_info* info;
int z;
{
  int ret;

  if (info->maps.equiv && (ret=info->maps.equiv[z]))
    ;

  else if (info->maps.ud_map[z].off==0 || (info->maps.ud_map[z].attrs & S_DEF))
    ret = 0;

  else if ((df_data.mem_df_level < 2 && info==df_info+IDF_MEM) ||
           (GET_DF_STATE(info) & DF_REACHING_DEFS) == 0)
    ret = z;

  else
  {
    int prev_time = assign_run_time (IDT_CEQUIV+(info-df_info));
    ud_set dom    = df_data.sets.dom_tmp;
    ud_set* reach = info->maps.defs_reaching;
    int n_ud      = NUM_UD(info);
    int* uds_pool = info->uds_used_by_pool;
    int u;

    df_assert (dom && reach);

    if (info->maps.equiv == 0)
      ALLOC_MAP (&(info->maps.equiv), n_ud, ud_map);

    /* Go ahead and get equivalences for all users of this variable... */

    for (u=info->maps.id_map[UDN_VARNO(info,z)].uds_used_by;u!=-1;u=uds_pool[u])
      if (info->maps.ud_map[u].off && !(info->maps.ud_map[u].attrs & S_DEF) &&
          !info->maps.equiv[u])
      {
        rtx ui;
        int c,d,p,n,t,ub;
    
        /* Equivalence u to itself, and find all uses equivalent to it. */
        info->maps.equiv[u] = u;
    
        /* Consider all uninitialized variables as being distinct ... */
        if ((t = next_flex (reach[u],-1)) == -1)
          continue;
    
        ui = UDN_IRTX (info,u);
        ub = BLOCK_NUM(ui);
    
        /* Equivalence u to a def if exactly 1 def reaches u, that
           def is equivalent to u, that def dominates u, and there is
           no way to get from the def back to itself without going thru u. */
    
        if (next_flex(reach[u],t) == -1 &&
            check_equiv (info, info->maps.ud_map+t, info->maps.ud_map+u))
        {
          int tb = UDN_BLOCK(info,t);
          int ok;
          
          if (tb == ub)
            ok=insn_comes_before (UDN_IRTX(info,t), ui);
          else
            ok=in_flex(df_data.maps.dominators[ub],tb) && (!possible_loop(tb,ub));
    
          if (ok)
          { info->maps.equiv[u] = t;
            continue;
          }
        }
    
        for (c=u+1; c < n_ud;  c++)
          if (UD_CONTEXT(info->maps.ud_map[c]) && !info->maps.equiv[c] &&
               !(info->maps.ud_map[c].attrs & S_DEF) &&
               equal_flex (reach[u], reach[c]) &&
               check_equiv (info, info->maps.ud_map+u, info->maps.ud_map+c))
          {
            rtx ci = UID_RTX (info->maps.ud_map[c].insn);
            int cb = BLOCK_NUM(ci);
            int ok = 1;
    
            if (cb == ub)
            { /* Both uses are in the same basic block ... */
    
              rtx i = BLOCK_END(cb);
              rtx j;
              
              /* Find one of the 2 uses by scanning forward in the block... */
              while (i != ci && i != ui)
                i = PREV_INSN(i);
    
              j = i;
    
              /* If c and u aren't both in the same insn, look at all defs made
                 between them (except those made in the last of the 2 insns).
    
                 If any such defs overlap reach[u], c and u are different. */
    
              /* If we don't have 'em both together ... */
              if (j != ci || j != ui)
                do
                { register int defs;
    
                  j    = PREV_INSN(j);
                  defs = info->maps.uid_map[INSN_UID(j)].defs_in_insn;
    
                  if (defs != -1)
                  {
                    register int *defs_pool;
                    register int lelt;
                    register int d2;
                    defs_pool = info->defs_in_insn_pool;
    
                    for (d2=next_flex(reach[u],-1); d2!= -1;
                         d2=next_flex(reach[u],d2))
                    {
                      for (lelt = defs; lelt != -1; lelt = defs_pool[lelt])
                      {
                        if (info->ud_range(info->maps.ud_map+lelt,
                                           info->maps.ud_map+d2,df_overlap))
                          ok = 0;
                      }
                    }
                  }
                }
                while (ok && j != ci && j != ui);
            }
            else
            {
              /* The uses are in different blocks.  Get the intersection of
                 the dominators for the blocks holding both uses... */
    
              and_flex (dom, df_data.maps.dominators[ub],
                             df_data.maps.dominators[cb]);
    
              for (d=next_flex(reach[u],-1); d!= -1 && !is_empty_flex(dom);
                   d=next_flex(reach[u],d))
              { 
                /* Remove from dom any node which is not guaranteed to be
                   executed AFTER every def that reaches u and c. */
    
                rtx di = UID_RTX (info->maps.ud_map[d].insn);
                int db = BLOCK_NUM (di);
    
                if (db == ub)
                { /* When a use is in the same block with one of its defs, we
                     can keep that block only if the def is clearly before the
                     use in the block. */
    
                  if (!insn_comes_before (di, ui))
                    clr_flex_elt (dom, db);
                }
                else
                  /* When a use is not in the same block as one of its defs,
                     we can keep only those blocks in dom thru which control
                     must have passed after the def was made and before the
                     use. */
    
                  for (p=next_flex(dom,-1); p != -1; p=next_flex(dom,p))
                    if (path_from_a_to_c_not_b(db, p, ub)||(possible_loop(p,ub)))
                      clr_flex_elt (dom, p);
    
                if (db == cb)
                { if (!insn_comes_before (di, ci))
                    clr_flex_elt (dom, db);
                }
                else
                  for (p=next_flex(dom,-1); p != -1; p=next_flex(dom,p))
                    if (path_from_a_to_c_not_b(db, p, cb)||(possible_loop(p,cb)))
                      clr_flex_elt (dom, p);
              }
    
              /* If there is anything left in dom, we have found a node that is
                 guaranteed to execute AFTER all defs which will eventually
                 reach c & u;  we can safely say c and u are equivalent. */
    
              if (is_empty_flex (dom))
                ok = 0;
            }
    
            if (ok)	/* Hot shit !! */
              info->maps.equiv[c] = u;
          }
      }
    ret = info->maps.equiv[z];
    df_assert (ret);
    assign_run_time (prev_time);
  }
  return ret;
}

static int
compute_equiv (info)
ud_info* info;
{
  if (GET_DF_STATE(info) & DF_EQUIV)
    return 0;

  ALLOC_MAP (&(info->maps.equiv), 0, ud_map);
  do_reaching_defs (info);
  SET_DF_STATE(info, DF_EQUIV);
}

add_addr_term (insn, xp, xoff, addr, lt)
int insn;
rtx xp;
int xoff;
mem_slot_info* addr;
lt_rec* lt;
{
  rtx* x;
  enum rtx_code c;

  ud_info* info       = 0;
  int      prev_time  = assign_run_time (IDT_ADDR);
  int      addr_is_ok = 0;
  int      u          = -1;

  while (1)
  { ud_info* winfo;
    int w = lookup_ud (insn,&NDX_RTX(xp,xoff),&winfo);
    int e;

    if (w == -1)
      break;

    if (e=find_equiv(winfo,w))
    { w    = e;
      insn = winfo->maps.ud_map[w].insn;
      xp   = winfo->maps.ud_map[w].user;
      xoff = winfo->maps.ud_map[w].off;
    }

    info = winfo;
    u    = w;

    if (GET_CODE(xp)==SET && xoff==XEXP_OFFSET(xp,0))
      xoff = XEXP_OFFSET(xp,1);
    else
      break;
  }

  x = &NDX_RTX(xp, xoff);
  c = GET_CODE(*x);

  switch (c)
  {
    case CONST_INT:
      addr->lt_offset += INTVAL(*x);
      addr_is_ok = 1;
      break;

    case CONST:
      addr_is_ok = add_addr_term (insn, *x, XEXP_OFFSET(*x,0), addr, lt);
      break;

    case LO_SUM:
      addr_is_ok = add_addr_term (insn, *x, XEXP_OFFSET(*x,1), addr, lt);
      break;

    case PLUS:
    case MINUS:
    case MULT:
    case ASHIFT:
    case LSHIFT:
    {
      mem_slot_info l;
      mem_slot_info r;

      l = empty_slot;
      r = empty_slot;

      l.terms = push_lt(lt);
      l.lt_offset = 0;

      if (addr_is_ok = add_addr_term (insn, *x, XEXP_OFFSET(*x,0), &l, lt))
      { r.terms  = push_lt(lt);
        r.lt_offset = 0;
        addr_is_ok &= add_addr_term (insn, *x, XEXP_OFFSET(*x,1), &r, lt);
        lt->next_lt--;
      }
      lt->next_lt--;

      if (addr_is_ok && (c==ASHIFT ||c==LSHIFT || c==MULT))
      {
        /* Need terms of rhs to all be zero ... */
        addr_is_ok = LT_ROW_EQ (*lt, r.terms, 0);

        /* Look for 0 on left of multiply, too */
        if (!addr_is_ok && c==MULT)
        { mem_slot_info t;
          addr_is_ok = LT_ROW_EQ (*lt, l.terms, 0);
          t = l;
          l = r;
          r = t;
        }
       
        if (c != MULT)
          if (r.lt_offset < 0 || r.lt_offset > 31)
            addr_is_ok = 0;
          else
            /* Handle it like a multiply ... */
            r.lt_offset = 1 << r.lt_offset;
      }

      if (addr_is_ok)
      { /* Add, subtract, or multiply l by r into addr */

        int ndf;
        for (ndf = NUM_DFK-1; ndf >= 0; ndf--)
        {
          int   i = lt->lt_len[ndf];
          int   j = 0;

          signed_byte *x = LT_VECT(*lt,l.terms,ndf);
          signed_byte *y = LT_VECT(*lt,r.terms,ndf);
          signed_byte *z = LT_VECT(*lt,addr->terms,ndf);

          if (c==PLUS)
          { while ((addr_is_ok &= FITS_CHAR (j)) && i--)
              z[i] = (j = z[i] + x[i] + y[i]);
          }

          else if (c==MINUS)
          { while ((addr_is_ok &= FITS_CHAR (j)) && i--)
              z[i] = (j = z[i] + x[i] + -y[i]);
          }

          else
          { addr_is_ok &= FITS_CHAR(r.lt_offset);

            while ((addr_is_ok &= FITS_CHAR (j)) && i--)
              z[i] = (j = z[i] + x[i] * r.lt_offset);
          }
        }

        if (addr_is_ok)
        { if (c==PLUS)
            addr->lt_offset += l.lt_offset + r.lt_offset;
          else if (c==MINUS)
            addr->lt_offset += l.lt_offset - r.lt_offset;
          else
            addr->lt_offset += l.lt_offset * r.lt_offset;
        }
        else
          bzero (LT_VECT(*lt,addr->terms,0), LT_SIZE(*lt));
      }
    }
  }

  if (!addr_is_ok && u != -1)
  {
    /*  Either u is a pure use, or it is a def which is not a SET target,
        or it is a SET target whose rhs we can't expand.  Just add him into
        the LT vector and call it good. */

    addr_is_ok = 1;
    LT_VECT(*lt, addr->terms, (info - &df_info[0]))[map_lt(lt,info,u)]++;
    trace (addr_added, u);
  }
  assign_run_time (prev_time);
  return addr_is_ok;
}

ud_set
get_equiv (df, u, p)
ud_info* df;
int u;
flex_pool* p;
{
  ud_set ret;
  int e;

  ret = alloc_flex (p);

  if (e = find_equiv(df,u))
  {
    register int i;

    for (i = df->maps.id_map[df->maps.ud_map[u].varno].uds_used_by; i != -1;
         i = df->uds_used_by_pool[i])
      if (find_equiv(df,i) == e)
        set_flex_elt (ret, i);
  }

  set_flex_elt (ret, u);

  return ret;
}



free_df_info (df)
ud_info* df;
{
  int i;

  ALLOC_MAP(&df->maps.ud_map, 0, ud_rec_map);
  ALLOC_MAP(&df->maps.uid_map, 0, uid_rec_map);
  ALLOC_MAP(&df->maps.id_map, 0, id_rec_map);
  ALLOC_MAP(&df->maps.defs_reaching, 0, flex_set_map);
  ALLOC_MAP(&df->maps.uses_reached, 0, flex_set_map);
  ALLOC_MAP(&df->maps.equiv, 0, ud_map);
  ALLOC_MAP (&df->maps.ud_req_rng, 0, rng_map);
  ALLOC_MAP (&df->maps.ud_full_rng, 0, rng_map);
#ifdef TMC_FLOD
  ALLOC_MAP (&df->maps.nkills, 0, uchar_map);
  ALLOC_MAP (&df->maps.nuses, 0, uchar_map);
#endif
  
  if (df->uds_used_by_pool)
    free(df->uds_used_by_pool);

  df->uds_used_by_pool = 0;
  df->uds_in_insn_pool = 0;
  df->defs_in_insn_pool = 0;

  /* This will ensure an abort if we forget to free something in the object */
  for (i = 0; i < sizeof (df->maps); i++)
    df_assert (0 == (((char*)(&df->maps))[i]));

  bzero (df, sizeof (*df));
}

static tree last_df_error_function;

free_all_flow_space()
{
  int i;
  ud_info* df;

  for (df = df_info; df != df_info + INUM_DFK; df++)
  { free_df_info (df);
    ALLOC_MAP (&(df_data.lt_info.map_lt[df-df_info]), 0, ushort_map);
    ALLOC_MAP (&(df_data.lt_info.unmap_lt[df-df_info]), 0, ushort_map);
  }

  ALLOC_MAP(&df_data.maps.loop_info, 0, loop_rec_map);
  ALLOC_MAP(&df_data.maps.succs, 0, flex_set_map);
  ALLOC_MAP(&df_data.maps.dominators, 0, flex_set_map);
  ALLOC_MAP(&df_data.maps.out_dominators, 0, flex_set_map);
  ALLOC_MAP(&df_data.maps.forw_succs, 0, flex_set_map);
  ALLOC_MAP(&df_data.maps.insn_info, 0, uid_info_rec_map);
  ALLOC_MAP(&df_data.maps.preds, 0, sized_flex_set_map);
  ALLOC_MAP(&df_data.maps.symbol_set, 0, flex_set_map);
  ALLOC_MAP(&df_data.maps.symbol_rtx, 0, rtx_map);
  ALLOC_MAP(&df_data.maps.value_stack, 0, char_map);
  ALLOC_MAP(&df_data.maps.lstk, 0, short_map);
  ALLOC_MAP(&df_data.maps.ctrl_dep, 0, flex_set_map);
  ALLOC_MAP(&df_data.maps.insn_reach, 0, flex_set_map);
  ALLOC_MAP(&df_data.maps.insn_rng_dep, 0, flex_set_map);
  ALLOC_MAP(&df_data.maps.path_cache, 0, ulong_map);
#if 0
  ALLOC_MAP (&df_data.maps.region, 0, ushort_map);
#endif

  if (df_data.maps.preds_alt)
  {
    free (df_data.maps.preds_alt);
    df_data.maps.preds_alt = 0;
  }

  for (i = 0; i != INUM_DFK; i++)
    ALLOC_MAP (&df_data.maps.old_pt[i], 0, flex_set_map);

  for (i = 0; i < sizeof (df_data.maps); i++)
    df_assert (0 == (((char*)(&df_data.maps))[i]));

  if (df_data.lt_info.linear_terms)
    free (df_data.lt_info.linear_terms);

  alloc_insn_flow (0);

  bzero (&df_data, sizeof (df_data));


  /* Free every pool in flexp[]. */
  for (i=0; i < sizeof(flex_data.flexp)/sizeof(flex_data.flexp[0]); i++)
    if (flex_data.flexp[i].text)
      free (flex_data.flexp[i].text);

  bzero (&flex_data, sizeof (flex_data));

  if (last_df_error_function == current_function_decl)
    for (df = df_info; df != df_info+INUM_DFK; df++)
      SET_DF_STATE (df, DF_ERROR);
}

int
order_slots (s1, s2)
mem_slot_info *s1;
mem_slot_info *s2;
{ /* Called from qsort for final sort after LT's have been
     unified in preparation for var numbering. 

     Return <0,0,>0 if s1 is <,=,> than s2.

     When we are all done, lower numbered slots can overlap the ones
     that follow, but not vv. */

  int ret;

  /* First by lowest LT# ... */
  if ((ret = s1->terms - s2->terms) == 0)

    /* ... then by lowest offset from LT# ... */
    if ((ret = s1->lt_offset - s2->lt_offset) == 0)

      /* ... by HIGHEST ref size first... */
      if ((ret = s2->vsize - s1->vsize) == 0)

        /* ... finally, by order of appearance. */
        ret = s1->unum - s2->unum;

  return ret;
}

dump_pts_to (file, info)
FILE* file;
ud_info *info;
{
  int u;

  fprintf (file, "\n\n%s pts_to:", df_names[info-df_info]);

  for (u=0; u != NUM_UD(info); u++)
    if (!is_empty_flex (info->maps.ud_map[u].pts_at))
    {
      int i;
      fprintf (file, "\n%d - ", u);
      dump_flex (file, info->maps.ud_map[u].pts_at);
    }
}

rtx absolute_sym()
{
  rtx s;

  s = gen_symref_rtx (Pmode, "@@absolute_address");
  SYM_ADDR_TAKEN_P(s) = 1;
  SYMREF_ADDRTAKEN(s) = 1;
  SYMREF_SIZE(s)=TI_BYTES;
  RTX_TYPE(s) = ptr_type_node;

  return s;
}

/* This routine is called from expand_expr for all occurances of
   NOP_EXPR, CONVERT_EXPR, INTEGER_CST, and REFERENCE_EXPR.  These
   places indicate the possible construction of a pointer from an
   integer value.  We need to hang a special symbol off of such
   expressions so that the points-to propagation will consider
   all pointers constructed from integers as aliasing each other. */

rtx cvt_expr (t, e, modifier)
tree t;
rtx e;
enum expand_modifier modifier;
{
  tree type = TREE_TYPE(t);
  rtx r,s;

  if (TREE_CODE(type) == POINTER_TYPE && !IS_CONST_ZERO_RTX(e))
    if (TREE_CODE(t) == CONVERT_EXPR)
    {
      tree src_type = TREE_TYPE(TREE_OPERAND(t,0));

      if (TREE_CODE(src_type) != POINTER_TYPE)
      {
        /*  Integer-to-pointer conversion;  generate
            as magic symbol (hidden 0) + e */
      
#ifdef TMC_NOISY2
        warning ("pointer created from integer");
#endif
        if (GET_CODE(e)==CONST_INT || GET_CODE(e)==CONST_DOUBLE)
          e = gen_rtx (CONST, Pmode, gen_rtx (PLUS, Pmode, absolute_sym(), e));
        else
        {
          if (modifier != EXPAND_NORMAL)
            e = gen_rtx(PLUS, Pmode, absolute_sym(), e);
          else
          {
            r = gen_typed_reg_rtx (src_type, Pmode);
            emit_move_insn (r, e);
            e = gen_rtx (PLUS, GET_MODE(r), r, absolute_sym());
          }
        }
      }
    }
    else if (TREE_CODE(t) == INTEGER_CST)
    {
#ifdef TMC_NOISY2
      warning ("pointer created from integer");
#endif
      df_assert (GET_CODE(e)==CONST_INT || GET_CODE(e)==CONST_DOUBLE);
      e = gen_rtx (CONST, Pmode, gen_rtx (PLUS, Pmode, absolute_sym(), e));
    }
  return e;
}

static void
compute_expr_pts_to(insn, x_p, changed, r0)
rtx insn;
rtx *x_p;
flex_set changed;
flex_set r0;
{
  /*  Compute the set of symbols the expression at *x_p points at */
      
  if (*x_p == 0)
    clr_flex (r0);
  else
  {
    rtx x;
    ud_info *df;
    int u,d;
    enum machine_mode m;
    enum rtx_code c;
    flex_set r1;

    r1  = alloc_flex (df_data.pools.ud_pt_tmp_pool);
    u   = lookup_ud(INSN_UID(insn), x_p, &df);
    x   = *x_p;
    m   = GET_MODE(x);
    c   = GET_CODE(x);
  
    if (u != -1)
    { flex_set* pt = &UDN_PTS_AT(df,u);
  
      if (GET_RTX_LENGTH(c) > 0 && GET_RTX_FORMAT(c)[0]=='e' && XEXP(x,0))
        compute_expr_pts_to (insn, &XEXP(x,0),changed,r0);
  
      if (*pt == 0)
        clr_flex (r0);
  
      else
        if (df==df_info+IDF_SYM || (UDN_ATTRS(df,u) & S_DEF) ||
            !(GET_DF_STATE(df) & DF_REACHING_DEFS))
          copy_flex (r0, *pt);
  
        else
        { df_assert (FLEX_POOL(*pt)==df_data.pools.ud_pt_pool);
    
          d = -1;
          clr_flex (r0);
  
          while ((d=next_flex(UDN_DEFS(df,u),d)) != -1)
            if (UDN_PTS_AT(df,d))
              union_flex_into (r0, UDN_PTS_AT(df,d));
  
          copy_flex (*pt, r0);
        }
    }
  
    else
      switch (c)
      {
        enum { PT_NOTHING, PT_UNION, PT_EVERYTHING } pt_kind;
  
        case ASM_OPERANDS:
        case CALL:
        case UNKNOWN:
        case UNSPEC:
        case UNSPEC_VOLATILE:
          pt_kind = PT_EVERYTHING;
          goto walk_args;
  
        case CONST:
        case MINUS:
        case MULT:
        case NEG:
        case PLUS:
        case POST_DEC:
        case POST_INC:
        case PRE_DEC:
        case PRE_INC:
        case IOR:
        case XOR:
        case AND:
          pt_kind = rtx_could_be_pointer(x) ? PT_UNION : PT_NOTHING;
          goto walk_args;
  
        default:
          pt_kind = PT_NOTHING;
          goto walk_args;
  
        walk_args:
        { char* fmt = GET_RTX_FORMAT (c);
          int i;
  
          if (pt_kind == PT_EVERYTHING)
            copy_flex (r0, df_data.sets.addr_syms);
          else
            clr_flex (r0);
  
          for (i=0; i < GET_RTX_LENGTH(c); i++)
            if (fmt[i] == 'e')
            { compute_expr_pts_to (insn, &XEXP(x,i), changed,r1);
              if (pt_kind == PT_UNION)
                union_flex_into (r0,r1);
            }
  
            else if (fmt[i] == 'E')
            { int j = XVECLEN(x, i);
              while (--j >= 0)
              { compute_expr_pts_to (insn, &XVECEXP(x,i,j), changed,r1);
                if (pt_kind == PT_UNION)
                  union_flex_into (r0,r1);
              }
            }
  
          break;
        }
  
        case SET:
        {
          compute_expr_pts_to(insn, &SET_SRC(x),changed, r1);
          compute_expr_pts_to(insn, &SET_DEST(x),changed, r0);
  
          d = lookup_ud(INSN_UID(insn), &SET_DEST(x), &df);
  
          if (d != -1)
          { ud_set* pt = &UDN_PTS_AT(df,d);
  
            if (*pt && (GET_DF_STATE(df)&DF_REACHING_DEFS) &&
                       (UDN_ATTRS(df,d) & S_KILL) && !equal_flex(*pt,r1))
            {
              copy_flex (*pt, copy_flex (r0, r1));
  
              u = -1;
              while ((u=next_flex(UDN_USES(df,d),u)) != -1)
                if ((UDN_ATTRS(df,u)&S_USE) && !(UDN_ATTRS(df,u)&S_DEF) &&
                    UDN_CONTEXT(df,u) && REAL_INSN(UDN_INSN_RTX(df,u)))
                  set_flex_elt (changed, UDN_INSN_UID(df,u));
            }
          }
          break;
        }
  
        case LSHIFT:
        case ASHIFT:
        case LSHIFTRT:
        case UDIV:
          compute_expr_pts_to (insn, &XEXP(x,0),changed, r0);
          compute_expr_pts_to (insn, &XEXP(x,1),changed, r1);
          break;
  
        case LO_SUM:
          compute_expr_pts_to (insn, &XEXP(x,0),changed, r1);
          compute_expr_pts_to (insn, &XEXP(x,1),changed, r0);
          break;
  
      }
  
    r0 = pop_flex (df_data.pools.ud_pt_tmp_pool);
    df_assert (FLEX_SET_NUM(r0) == FLEX_SET_NUM(r1)-1);
  }
}

static int
restrict_pts_to ()
{
  int prev_time,i,ret;
  rtx r;

  flex_set change1 = 0;
  flex_set change2 = 0;
  flex_set dummy   = 0;

  if (df_data.last_restrict_pts_to == current_function_decl)
    return 0;
  
  prev_time = assign_run_time (IDT_RESTRICT);

  df_data.last_restrict_pts_to = current_function_decl;

  if ((GET_DF_STATE(df_info+IDF_REG) & DF_REACHING_DEFS) == 0 ||
      (GET_DF_STATE(df_info+IDF_SYM) & DF_RESTART)       == 0 ||
      (GET_DF_STATE(df_info+IDF_MEM) & DF_RESTART)       == 0)
      
    return assign_run_time (prev_time), 0;

  assign_run_time (IDT_RESTRICT+1);

  reuse_flex (&change1, NUM_UID, 1, F_DENSE);
  reuse_flex (&change2, NUM_UID, 1, F_DENSE);

  new_flex_pool (&df_data.pools.ud_pt_tmp_pool,NUM_ID(df_info+IDF_SYM),10, F_DENSE);
  dummy = alloc_flex (df_data.pools.ud_pt_tmp_pool);

  if (df_data.pools.ud_pt_pool == 0)
    new_flex_pool (&df_data.pools.ud_pt_pool, NUM_ID(df_info+IDF_SYM),
                   df_data.shared_syms,F_DENSE);

  for (i=0; i < NUM_DFK; i++)
  { int k = NUM_UD(df_info+i);
    int j;

    for (j=0; j<k; j++)
    { ud_rec *p = df_info[i].maps.ud_map+j;
      flex_set *pt = &UD_PTS_AT(*p);

      if (*pt)
      { int is_shared = (FLEX_POOL(*pt)==df_data.pools.shared_sym_pool);

        if (is_shared)
          df_data.shared_syms--;

        if (is_empty_flex (*pt))
          *pt = 0;
        else
          if (i==IDF_SYM || !(GET_DF_STATE(df_info+i)&DF_REACHING_DEFS) ||
              !UD_CONTEXT(*p) || !REAL_INSN(UD_INSN_RTX(*p))||
              ((UD_ATTRS(*p) & S_DEF) && (UD_ATTRS(*p) & S_USE)))
          { if (is_shared)
              *pt = copy_flex (alloc_flex (df_data.pools.ud_pt_pool), *pt);
          }
          else
          {
            set_flex_elt (change1, UD_INSN_UID(*p));

            /* This ud is a candidate for pts_to improvement.  Start it with
               an empty pts_to initially. */

            if (is_shared)
              *pt = alloc_flex (df_data.pools.ud_pt_pool);
            else
               clr_flex (*pt);
          }
      }
    }
  }

  df_assert (df_data.shared_syms == 0);
  assign_run_time (IDT_RESTRICT+2);

  ret = 0;
  while (!is_empty_flex(change1))
  {
    copy_flex (change2, change1);
    clr_flex  (change1);

    for (r=get_insns(); r; r=NEXT_INSN(r))
    { ret++;
      if (in_flex (change2, INSN_UID(r)))
      { compute_expr_pts_to (r, &PATTERN(r), change1, dummy);
#ifdef CALL_INSN_FUNCTION_USAGE
        if (GET_CODE(r)==CALL_INSN)
          compute_expr_pts_to (r, &CALL_INSN_FUNCTION_USAGE(r), change1, dummy);
#endif
      }
    }
  }

  reuse_flex (&change2, 0, 0, 0);
  reuse_flex (&change1, 0, 0, 0);

  assign_run_time (prev_time);

  return 1;
}

rebuild_slot_pt (lt)
lt_rec* lt;
{ 
  /* rebuild slot_pt[] for all slots... */

  int n_id = NUM_ID(&(df_info[IDF_MEM]));
  int i;

  int prev_time = assign_run_time (IDT_REBUILD);
  new_flex_pool (&df_data.pools.slot_pt_pool, NUM_ID(df_info+IDF_SYM), n_id, F_DENSE);

  mem_slots[0].slot_pt=copy_flex(alloc_flex (df_data.pools.slot_pt_pool),df_data.sets.all_symbols);

  for (i=1; i < n_id; i++)
  {
    int num_pt = 0;

    /* Say that pts_to[slot] is pts_to of everything in it's LT's */

    ud_info* info;
    mem_slots[i].slot_pt = alloc_flex (df_data.pools.slot_pt_pool);

    for (info= &(df_info[0]); info != &(df_info[INUM_DFK]); info++)
    { int j;
      signed_byte* p = LT_VECT (*lt, mem_slots[i].terms, info-&(df_info[0]));
      int l = lt->lt_len[info-df_info];

      for (j = 0; j < l; j++)
      { int m = unmap_lt (lt,info,j);
        if (p[j] && info->maps.ud_map[m].pts_at && !is_empty_flex (info->maps.ud_map[m].pts_at))
        { num_pt++;
          union_flex_into (mem_slots[i].slot_pt, info->maps.ud_map[m].pts_at);
        }
      }
    }
  }
  assign_run_time (prev_time);
}

static int
base_dataflow (mask)
int mask;
{
  int c = 0;
  int prev_time = assign_run_time (IDT_MISC+0);

  if (df_data.iid_invalid)
    insn_comes_before (ENTRY_LABEL, ENTRY_LABEL);

  assign_run_time (IDT_MISC+1);

  /* If users wants mem df, all other df has to be updated first */
  if (mask & MK_SET(DF_MEM))
    mask = -1;

  if (mask & MK_SET(DF_SYM))
    c |= do_reaching_defs (df_info+IDF_SYM);

  if (mask & MK_SET(DF_REG))
    c |= do_reaching_defs (df_info+IDF_REG);

  /* If we reran reg or sym df, that means mem df is totally out of date. */
  if (c)
    SET_DF_STATE (df_info+IDF_MEM, GET_DF_STATE (df_info+IDF_MEM) & DF_SORT);

  if ((mask & MK_SET(DF_MEM)) &&
      ((GET_DF_STATE(df_info+IDF_MEM) & DF_REACHING_DEFS) == 0))
    if (df_data.mem_df_level < 2)
    {
      if ((GET_DF_STATE(df_info+IDF_MEM) & DF_REACHING_DEFS) == 0)
      {
        restart_dataflow (df_info+IDF_MEM);
        restrict_pts_to();

        c |= do_reaching_defs (df_info+IDF_MEM);
      }
    }
    else
    {
      c |= do_reaching_defs (df_info+IDF_MEM);
      /* If we ran any dataflow, if pts_to changes, it could affect mem df. */

      if (c && restrict_pts_to())
      { SET_DF_STATE (df_info+IDF_MEM, DF_RESTART);
        do_reaching_defs (df_info+IDF_MEM);
      }
    }
  return assign_run_time (prev_time), c;
}

static int
equivalences ()
{
  int c;
  int prev_time = assign_run_time(IDT_EQUIV);

  base_dataflow (-1);

  c  = compute_equiv (df_info+IDF_SYM);
  c |= compute_equiv (df_info+IDF_REG);
  c |= compute_equiv (df_info+IDF_MEM);

  return assign_run_time (prev_time), c;
}

pick_offset_and_align (u, offset, align)
ud_rec* u;
int* offset;
int* align;
{
  int off = 0;
  int ali = 0;

  if (UD_CONTEXT(*u))
  {
    rtx r = UD_VRTX(*u);
    int change = 1;

    if (ali = RTX_BYTES(r))
    {
      df_assert (ali >= 1 && ((ali & (ali-1))==0));
  
      while (change && ali < 16)
      {
        int i;
        change = 0;
  
        while (ali < 16 && check_mem_align (ali * 2, UD_INSN_UID(*u), r, off))
        { change=1; ali *= 2; }
  
        if (ali < 16)
          for (i = off+RTX_BYTES(r); i + RTX_BYTES(r) <= 16; i += RTX_BYTES(r))
            if (check_mem_align (ali * 2, UD_INSN_UID(*u), r, i))
            { change=1; ali *= 2; off=i; break; }
      }
    }
  }

  if (off)
    off = ali-off;
  *offset = off;
  *align  = ali;
}


refine_mem_info ()
{ /*
      Apply the linear terms algorithm to the extant mem, reg,
      and sym flow information to renumber RTX_VAR_ID for all mem ud's.

      We build up the new slot table in *slots, then we free
      mem_slots and set mem_slots = slots. */

  ud_info* mem;
  int n_ud;
  mem_slot_info* slots;
  int prev_time = assign_run_time (IDT_REFINE);

  /* Get RD's and compute equiv for all universes. */

  equivalences ();

  trace (refine_info_entry);

  slots = 0;
  mem   = &df_info[IDF_MEM];
  n_ud  = NUM_UD(mem);

  init_lt (&df_data.lt_info);

  /* We need new space for all possible new slots - i.e, 1 per ud. */
  ALLOC_MAP (&slots, n_ud, mem_slot_info_map);

  {  /* Loop thru all mem ud's, examining their addresses to get their
        LT's, and record the information in *slots. */

    int u;
    assign_run_time (IDT_REFINE+1);
    for (u = 0; u < n_ud; u++)
    {
      ud_rec* udr = &mem->maps.ud_map[u];
  
      /* Get the linear terms for this mem, and record the offset of
         this mem from the terms. */

      if (UD_CONTEXT(*udr) && TREE_CODE(UD_TYPE(*udr))!=FUNCTION_TYPE)
      {
        rtx v = UD_VRTX (*udr);
  
        slots[u].unum = u;

        /* Get space for a set of new LT's */
        slots[u].terms  = push_lt (&df_data.lt_info);
  
        /* Get LT's for this ud by examination of it's address */
        if (add_addr_term (UD_INSN_UID(*udr), v, XEXP_OFFSET(v,0), &slots[u], &df_data.lt_info))
        { int k;
          slots[u].vsize = RTX_BITS(v) / 8;

          trace (new_linear_terms, slots, u, &df_data.lt_info);
  
          /* See if this set of LT's exists already;  if so, use it. */
          for (k = 1; k < slots[u].terms; k++)
            if(LT_ROW_EQ(df_data.lt_info, k, slots[u].terms))
            { slots[u].terms = k;
              df_data.lt_info.next_lt--;
              break;
            }
        }
        else
        {
          /* Terms vector has already been zeroed.  Address is screwed up. */
          slots[u].terms = 0;
          df_data.lt_info.next_lt--;
        }
      }
    }
  }

  /* Sort *slots:

     1) First by LT# (lowest comes first),
     2) Then by offset from LT.  Again, lowest comes first.
     3) Then, by reference size,  biggest first.
     4) Finally, by ud number, lowest first.  */

  assign_run_time (IDT_REFINE+2);
  qsort (slots, n_ud, sizeof (slots[0]), order_slots);

  /* Assign variable numbers.  For now, we assign each triplet
     of size, lt, offset to the same variable, and we say that
     the offset of each MEM from it's variable base is zero.

     While we are about this, we collapse *slots so that there
     is only 1 entry per variable. */

  {
    mem_slot_info *s, *v, *e;

    int n_id,v_ali;

    assign_run_time (IDT_REFINE+3);
    s = slots;
    e = slots + n_ud;

    /* Must have at *? as first entry in the table. */
    df_assert (s != e && s->terms==0);

    /* Handle all *? variables ... */
    do
    { ud_rec* mu=mem->maps.ud_map+s->unum;

      mu->varno = 0;

      if (UD_CONTEXT(*mu))
      { rtx t = UD_VRTX(*mu);
        UD_VAR_OFFSET(*mu) = 0;
        RTX_VAR_ID(t) = 0;
      }
      (s++)->vsize = 0;
    }
    while (s!=e && s->terms==0);

    v = slots;
    df_assert (s-v > 0 && s->terms > 0);

    assign_run_time (IDT_REFINE+4);
    while (s!=e)
    { int v_off;

      ud_rec* mu=mem->maps.ud_map+s->unum;

      if (s->terms!=v->terms ||
          /* Don't allow variable size to exceed its known alignment. */
          (v_off=(s->lt_offset-v->lt_offset)) + s->vsize >v_ali)
      {
        *(++v) = *s;	/* Careful; ++v can alias s here */

        /* Record in v->unum the first variable using these LT's */
        if (v->terms == (v-1)->terms)
          v->unum = (v-1)->unum;
        else
          v->unum = v-slots;

        /* Find the biggest alignment known for this variable, based on
           the known alignment of this particular reference.  Offset this
           reference from the beginning of the variable, such that the
           base of the variable is aligned as large as possible. */

        pick_offset_and_align (mu, &v_off, &v_ali);
        df_assert (v_ali<=16 && v_off>=0 && v_off<=16 && v_off+s->vsize <= v_ali);
        v->lt_offset -= v_off;
      }

      mu->varno = v-slots;

      if (UD_CONTEXT(*mu))
      { rtx t = UD_VRTX(*mu);
        UD_VAR_OFFSET(*mu) = v_off;
        RTX_VAR_ID(t) = v-slots;
      }

      v->vsize = MAX(v->vsize, v_off+s->vsize);
      df_assert (v->vsize <= v_ali);

      s++;
    }

    assign_run_time (IDT_REFINE+5);
    n_id = (v-slots) + 1;

    NUM_ID(mem) = n_id;
    REALLOC_MAP (&mem_slots, 0, mem_slot_info_map);
    mem_slots = slots;
    assign_run_time (IDT_REFINE+6);
    rebuild_slot_pt (&df_data.lt_info);

    /* We don't want the uds sorted again, because if we did that the
       LTs which refer to mems would be wrong. */

    SET_DF_STATE (mem, DF_SORT);
    equivalences ();

    trace (refine_info_slot_change);
  }

  push_lt (&df_data.lt_info);
  assign_run_time (prev_time);
}

#define IS_MEM(X) ((  GET_CODE(RTX_LVAL(X)) == MEM ))

rtx
optim_rtx (p)
rtx p;
{
  if (GET_CODE(p) == SET)
  {
    rtx d = SET_DEST(p);
    rtx s = SET_SRC(p);

    if (GET_CODE(d)==SUBREG)
    {
      enum machine_mode dm = GET_MODE(d);
      enum machine_mode sm = GET_MODE(s);
      extern rtx pun_rtx();

      if (dm==sm && GET_MODE_SIZE(dm) < UNITS_PER_WORD)
        if (GET_CODE(s)==REG || GET_CODE(s)==SUBREG)
        { if (GET_MODE_SIZE(dm=GET_MODE(XEXP(d,0))) > UNITS_PER_WORD)
            dm = word_mode;
        }

      if (dm != GET_MODE(d))
      { SET_DEST(p) = pun_rtx (d, dm);
        SET_SRC(p)  = pun_rtx (s, dm);
      }
    }
  }
  else
    df_assert (!RTX_IS_LVAL_PARENT(p) || RTX_IS_LVAL(XEXP(p,0)));

  return p;
}

static jmp_buf_struct* df_error_env;

df_error (va_alist)
va_dcl
{
  va_list ap;

  int err_num;
  ud_info* df;
  char buf[127];
  char* u, *d;

  va_start (ap);

  err_num = va_arg (ap, int);

  buf[0] = '\0';
  d = 0;
  u = 0;

  df_assert (last_df_error_function != current_function_decl);

  switch (err_num)
  {
    case df_too_much_memory:
      max_df_insn /= 2;
      sprintf(u=d=buf, "memory exhausted - dataflow abandoned");
      break;

    case df_too_many_blocks:
      sprintf (d=buf, "basic block count %d exceeds %d; no dataflow attempted",
                  N_BLOCKS, DF_MAX_NBLOCKS);
      break;

    case df_too_many_insns:
      sprintf (d=buf, "insn count %d exceeds %d; no dataflow attempted",
                  NUM_UID, max_df_insn);
      break;

    case df_assertion:
      sprintf (u=d=buf, "dataflow consistency error;  dataflow aborted");
      break;

    case df_capacity:
    { int cnt = va_arg (ap, int);
      int lim = va_arg (ap, int);

      sprintf (d=buf, "require %d, exceeded limit %d", cnt, lim);
      break;
    }
  }

#ifdef TMC_REPORT
report_error_function (DECL_SOURCE_FILE (current_function_decl));
fprintf (stderr, "%s\n", d);
#endif

  if (u)
  { report_error_function (DECL_SOURCE_FILE (current_function_decl));
    fprintf (stderr, "%s\n", u);
  }

  if (d && df_data.dump_stream)
    fprintf (df_data.dump_stream, "%s\n", d);

  last_df_error_function = current_function_decl;

  free_all_flow_space ();

  va_end(ap);

  longjmp (df_error_env->jmp_buf, err_num+1);
}

#ifdef TMC_REPORT
double dftot_blocks;
int dftot_funcs;
#endif

update_dataflow (mask, mem_df_level)
int mask;
int mem_df_level;
{
  int ret;
  ud_info* df;
  extern FILE* dataflow_dump_file;

  jmp_buf_struct this_xmalloc_env;
  jmp_buf_struct* save_xmalloc_env;

  jmp_buf_struct this_df_error_env;
  int prev_time = assign_run_time (IDT_MISC+2);

  if (last_df_error_function == current_function_decl)
    return assign_run_time (prev_time), 0;

  df_data.dump_stream = dataflow_dump_file;

  save_xmalloc_env = xmalloc_handler;
  xmalloc_handler = &this_xmalloc_env;
  df_error_env = &this_df_error_env;

#ifdef TMC_REPORT
dftot_blocks += N_BLOCKS;
dftot_funcs++;
fprintf(stderr," %d blocks; average %g\n", N_BLOCKS, dftot_blocks/dftot_funcs);
#endif

  if (setjmp (this_df_error_env.jmp_buf))
  {
    /* This is the final exit point for all dataflow errors */

    xmalloc_handler = save_xmalloc_env;
    return assign_run_time (prev_time), 0;
  }

  if (setjmp (this_xmalloc_env.jmp_buf))
    df_error(df_too_much_memory);

  if (NUM_UID > max_df_insn)
  {
#if 0
fprintf (stderr,"no dataflow at real %d, total %d, max %d\n",count_real_insns(get_insns(),0x7fffffff),NUM_UID,max_df_insn);
#endif
    df_error (df_too_many_insns);
  }

  if (N_BLOCKS > DF_MAX_NBLOCKS)
    df_error (df_too_many_blocks);

  if (mem_df_level > 1 && N_BLOCKS > 64)
    mem_df_level = 1;

  df_data.num_refine   = (mask & MK_SET(DF_MEM)) ? mem_df_level : 0;
  df_data.mem_df_level = (mask & MK_SET(DF_MEM)) ? mem_df_level : 0;

  /* Consider flow effects of memory, registers, and symbols
     to determine variable numbering and aliasing
     information for symbols, mems, and registers. */

  /* First, scan the insn stream to produce new ud's for all insns
     which haven't yet been scanned. */

  superflow_scan ();

  if (df_data.mem_df_level > 0)
    get_out_dominators();

  /* run data flow for regs, syms, and mems.

     The steps involved in running dataflow for each universe are
     basically as follows:

       1) Remove existing temp uds;
       2) Add new temp uds for call effects, returns, and function entry;
       3) Sort all uds and build various helper sets to categorize them;
       4) Run reaching defs and reached uses.

     The initial mem rd's are not very good, of course, because all
     mems have slot zero, which overlaps everything.  So any mem reaches
     pretty much all over the place initially.  */

  trace (superflow_entry);

  /* Go until things settle, or we get tired, but make sure
     that if we get tired, dataflow is accurate. */
      
  assign_run_time (IDT_MISC+3);
  ret = base_dataflow (mask);

  while (df_data.num_refine-- > 0)
    refine_mem_info ();

  last_reg_df = 0;
  df_data.last_mem_df = 0;
  df_data.last_restrict_pts_to = 0;

  xmalloc_handler = save_xmalloc_env;

  return assign_run_time (prev_time),ret;
} 
/* Return 1 iff it is possible to get from source
   to any node in target_nodes without going thru any of the nodes in
   visited_nodes.

   visited_nodes, target_nodes, and tmp are trashed by this routine. */

int
base_path_check (source, visited_nodes, target_nodes, tmp)
int source;
ud_set visited_nodes;
ud_set target_nodes;
ud_set tmp;
{
  and_compl_flex_into (target_nodes, visited_nodes);

  while (!is_empty_flex (target_nodes))
  {
    union_flex_into (visited_nodes, target_nodes);

    if (in_flex (target_nodes, source))
      return 1;
    else
    { int p = -1;

      clr_flex (tmp);
      while ((p = next_flex (target_nodes, p)) != -1)
        union_flex_into (tmp, df_data.maps.preds[p].body);
  
      and_compl_flex_into (tmp, visited_nodes);
      SWAP_FLEX (tmp, target_nodes);
    }
  }

  return 0;
}

int
path_from_a_to_c_not_b (a,b,c)
{
  ud_set bs = 0;
  ud_set cs = 0;
  ud_set ts = 0;

  int ret;
  int v,hash,i,n;
  unsigned int t;

  df_assert (a < 1024 && b < 1024 && c < 1024);

  if (!df_data.maps.path_cache)
  { df_data.pcache_entries = 63 * N_BLOCKS;
    ALLOC_MAP (&df_data.maps.path_cache, df_data.pcache_entries * PCACHE_SLOTS, ulong_map);
  }

  v = (a<<2) + (b << 12) + (c << 22);
  hash = v % df_data.pcache_entries;
  n = PCACHE_SLOTS;

  for (i=0; i<n; i++)
    if ((t=df_data.maps.path_cache[hash+i])==0)
      break;
    else
      if ((t & 0xfffffffc) == v)
        return (t & 1);

  if (i==n)
  {
#ifdef TMC_NOISY2
    warning ("path cache wrapped at %d\n", PCACHE_SLOTS);
#endif
    i=0;
  }

  reuse_flex (&bs, N_BLOCKS, 1, F_DENSE);
  reuse_flex (&cs, N_BLOCKS, 1, F_DENSE);
  reuse_flex (&ts, N_BLOCKS, 1, F_DENSE);

  set_flex_elt (bs, b);
  set_flex_elt (cs, c);

  ret = base_path_check (a, bs, cs, ts);

  df_data.maps.path_cache[hash+i] = v |2 |(ret != 0);

  reuse_flex (&ts, 0, 0, 0);
  reuse_flex (&cs, 0, 0, 0);
  reuse_flex (&bs, 0, 0, 0);

  return ret;
}

int path_from_a_to_c_around_b (a,b,c)
ud_set b;
{
  ud_set bs = 0;
  ud_set cs = 0;
  ud_set ts = 0;

  int ret;

  reuse_flex (&bs, N_BLOCKS, 1, F_DENSE);
  reuse_flex (&cs, N_BLOCKS, 1, F_DENSE);
  reuse_flex (&ts, N_BLOCKS, 1, F_DENSE);

  copy_flex (bs, b);
  set_flex_elt (cs, c);

  ret = base_path_check (a, bs, cs, ts);

  reuse_flex (&ts, 0, 0, 0);
  reuse_flex (&cs, 0, 0, 0);
  reuse_flex (&bs, 0, 0, 0);

  return ret;
}

int find_def (info, u, a, b)
ud_info* info;
int u;
rtx a;
rtx b;
{
  /* Scan the insns between a & b for defs which overlap u. */

  if (a == 0)
    a = get_insns();

  while (a != 0 && PREV_INSN(a) != b)
  {
    register int lelt;

    for (lelt = info->maps.uid_map[INSN_UID(a)].defs_in_insn; lelt != -1;
         lelt = info->defs_in_insn_pool[lelt])
    {
      if (info->ud_range(info->maps.ud_map+lelt,
                         info->maps.ud_map+u,df_overlap))
        return 1;
    }

    a = NEXT_INSN(a);
  }

  return 0;
}

#define ISEXTRACT(p) ((GET_CODE(p)==ZERO_EXTRACT||GET_CODE(p)==SIGN_EXTRACT))
#define IS_UP_EXTEND(m1,m2) (( GET_MODE_CLASS(m1)==MODE_INT && \
                               GET_MODE_CLASS(m2)==MODE_INT && \
                               GET_MODE_SIZE(m1) >= SI_BYTES && \
                               GET_MODE_SIZE(m2) > GET_MODE_SIZE(m1) ))

check_equiv (info, p, q)
ud_info* info;
ud_rec* p;
ud_rec* q;
{
  int ok;

  if (ok = (info->ud_range (p, q, df_equivalent) &&
            equal_flex(UD_PTS_AT(*p),UD_PTS_AT(*q))))
  {
    rtx pr = UD_RTX(*p);
    rtx pq = UD_RTX(*q);
    enum machine_mode m1,m2;

    /* If we get this far, the uds have the same number of bits and the
       same starting and ending bit offset.  We want to disallow all
       cases involving extraction/extension where the extensions are not
       identical, except for the case of sign-extension on 32 bit objects. */

    if (GET_CODE(pr) != GET_CODE(pq))
      if (GET_CODE(pr)==ZERO_EXTEND || ISEXTRACT(pr) ||
          GET_CODE(pq)==ZERO_EXTEND || ISEXTRACT(pq))
        ok = 0;

      else
        /* Only one side can be sign extend because GET_CODEs were
           are different.  The other cases are REG,SUBREG,MEM,SYM.  In
           all of these cases, normal ud_range equivalence is good enough. */
        if (GET_CODE(pr)==SIGN_EXTEND)
          ok = IS_UP_EXTEND(GET_MODE(XEXP(pr,0)), GET_MODE(pr));

        else if (GET_CODE(pq)==SIGN_EXTEND)
          ok = IS_UP_EXTEND(GET_MODE(XEXP(pq,0)), GET_MODE(pq));
  }

  return ok;
}

current_func_exports_frame_addr()
{
  /*  Return 0 if the current function is one in which
      it is safe to disregard the effects of function
      calls on local variables.

      Our conservative implementation returns 0 if we
      can prove that no transmission register and no
      non-frame memory definition can point into the frame. */

#if 0
  ud_info* mem = df_info+IDF_MEM;
  ud_info* reg = df_info+IDF_REG;

  if ((GET_DF_STATE(mem) & DF_REACHING_DEFS) &&
      (GET_DF_STATE(reg) & DF_REACHING_DEFS))
  {
    int u = -1;

    /* Look at all calls except LIBCALLS;  examine the USE rtls before each
       call.  If any USE points into the frame, assume the call cares about the
       frame, no matter what flavor the USE is. */

    while ((u = next_flex (mem->sets.attr_sets[I_CALL],u)) != -1)
    { rtx urtx = UDN_IRTX(mem,u);
      rtx irtx = PREV_INSN(urtx);

      while (irtx)
        if (GET_CODE(irtx) == INSN && GET_CODE(PATTERN(irtx)) == USE)
        { ud_info* info = 0;
          int i = lookup_ud (INSN_UID(irtx), &XEXP(PATTERN(irtx),0), &info);
          df_assert (i != -1);

          if (in_flex (info->maps.ud_map[i].pts_at, RTX_VAR_ID(df_data.fp_sym)))
            return 1;

          irtx = PREV_INSN(irtx);
        }
        else
          irtx = 0;
    }

    /* Look at all of the MEMs.  If we find a non-stack def of a MEM that
       reaches a call and which points into the frame, we're toast. */

    u = NUM_UD(mem);
    while (u-- > 1)
    { 
      int v = UDN_VARNO (mem,u);

      if ((UDN_ATTRS(mem,u) & S_DEF)!=0 && (UDN_ATTRS(mem,u) & S_CALL)==0)
        if (!equal_flex (mem_slots[v].slot_pt, df_data.sets.frame_symbols))
          if (in_flex (mem->maps.ud_map[u].pts_at, RTX_VAR_ID(df_data.fp_sym)))
            if (!is_empty_flex (and_flex (0,mem->sets.attr_sets[I_CALL],
                                            mem->maps.uses_reached[u])))
              return 1;
    }
    return 0;
  }
#endif

  return 1;
}
#endif
