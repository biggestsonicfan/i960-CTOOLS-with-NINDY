#include "config.h"

#ifdef IMSTG
/*
  Copyright (C) 1987, 1988, 1989 Free Software Foundation, Inc.
  
  This file is part of GNU CC.
  
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

  This file contains code which implements an optimisation
  wherein memory references are replaced over large regions
  with a local 'shadow'.  The memory is updated from the shadow
  before any memory reference which could alias the memory
  being shadowed, and the shadow is reloaded after any such
  interference.  Function calls are treated as interference
  for all memory except locally allocated objects whose address
  isn't taken.  TMC, Spring '90.
*/

#ifdef macintosh
 #pragma segment GNUX
#endif

#include "rtl.h"
#include "expr.h"
#include "regs.h"
#include "flags.h"
#include "assert.h"
#include <stdio.h>

#ifdef GCC20
#include "insn-config.h"
#include "hard-reg-set.h"
#include "insn-flags.h"
#else
#include "insn_cfg.h"
#include "hreg_set.h"
#include "insn_flg.h"
#endif

#include "recog.h"

typedef enum { kill_context, set_context, use_context } ksu;

/*  small_set is on the way out.  These will be 24 bit fields
    in the future, when when we learn to split SESE regions
    when we get too many mem slots. TMC */

#define MAX_MEM_ID     (( (31) ))
#define MEM_ID_SET_WDS (( (MAX_MEM_ID+1)/HOST_BITS_PER_INT ))

typedef
  struct
  { unsigned a[MEM_ID_SET_WDS];
  }
  small_set;

typedef
  struct
  { small_set use_def;
    small_set def;
    small_set dead;
  }
  flow_set;

extern int lineno;

small_set all_zeroes;
small_set all_ones;
flow_set  all_ones_flow_set = { -1, -1, -1 };
small_set call_kills_addr;

static small_set
delete_elt (src, elt)
small_set  src;
register unsigned elt;
{
  assert (elt <= MAX_MEM_ID);

  src.a[elt/HOST_BITS_PER_INT] &= ~(1 << (elt % HOST_BITS_PER_INT));
  return src;
}

static unsigned
first_elt (src)
small_set  src;
{
  register unsigned i, t;

  for (i=0; i < MEM_ID_SET_WDS; i++)
    if (t = src.a[i])
    { i *= HOST_BITS_PER_INT;
      while ((t & 1) == 0)
      { t >>= 1;
        i++;
      }
      return i;
    }

  return MAX_MEM_ID+1;
}

static small_set
union_elt (src, elt)
small_set  src;
register unsigned elt;
{
  assert (elt <= MAX_MEM_ID);
  src.a[elt/HOST_BITS_PER_INT] |= (1 << (elt % HOST_BITS_PER_INT));
  return src;
}

static small_set
union_set (dst, src)
small_set dst;
small_set src;
{ register int i;
  for (i=0; i<MEM_ID_SET_WDS; i++)
    dst.a[i] |= src.a[i];
  return dst;
}

static small_set
and_not (dst, src)
small_set dst;
small_set src;
{ register int i;
  for (i=0; i<MEM_ID_SET_WDS; i++)
    dst.a[i] &= ~src.a[i];
  return dst;
}

static int
and_to (dst, src)
small_set* dst;
small_set src;
{
  register int i;
  register int ret;

  for (i=0,ret=0; i<MEM_ID_SET_WDS; i++)
  { register int d = dst->a[i];
    register int s = src.a[i];

    ret |= (d&s)-d;
    dst->a[i] = d&s;
  }
  return ret;
}

static int
union_to (dst, src)
small_set* dst;
small_set src;
{
  register int i;
  register int ret;

  for (i=0,ret=0; i<MEM_ID_SET_WDS; i++)
  { register int d = dst->a[i];
    register int s = src.a[i];

    ret |= (d|s)-d;
    dst->a[i] = d|s;
  }
  return ret;
}


#ifdef macintosh
static
elt_in_set (set, elt)
small_set set;
register unsigned elt;
{ 
  unsigned quot, rem ;
  assert (elt <= MAX_MEM_ID);
  quot = elt/HOST_BITS_PER_INT ;
  rem = elt%HOST_BITS_PER_INT ;
  return !! ((set.a[quot]) & (1 << (rem)));
}
#else
static
elt_in_set (set, elt)
small_set set;
register unsigned elt;
{ 
  assert (elt <= MAX_MEM_ID);
  return !! ((set.a[elt/HOST_BITS_PER_INT]) & (1 << (elt%HOST_BITS_PER_INT)));
}
#endif

static int
empty_set (set)
small_set  set;
{ register int i;
  for (i=0; i<MEM_ID_SET_WDS; i++)
    if (set.a[i])
      return 0;
  return 1;
}

static void
dump_small_set (file, src)
FILE* file;
small_set src;
{
  unsigned i;

  fprintf (file, "[");

  i=0;
  while (i<=MAX_MEM_ID)
  {
    if (elt_in_set (src, i))
    {
      fprintf (file, "%d", i);
      src = delete_elt (src, i);
      i++;

      if (i<=MAX_MEM_ID && elt_in_set (src,i))
      { while (i<=MAX_MEM_ID && elt_in_set (src,i))
        {
          src = delete_elt (src, i);
          i++;
        }
        fprintf (file, "..%d", i-1);
      }
      
      if (!empty_set (src))
        fprintf (file, ",");
    }
    i++;
  }
  fprintf (file, "]");
}

void
debug_small_set (src)
small_set* src;
{
  fprintf (stderr, "small_set %x: ", (long) src);
  dump_small_set (stderr, *src);
  fprintf (stderr, "\n");
}

typedef
  struct
  { int iid;
    small_set mem_info [3];
    small_set addr_kill;
    flow_set flow;
    rtx first_label;
    rtx last_label;
    rtx sese;
  }
  uid_data;

typedef
  struct _mem_rec
  {
    struct _mem_rec *prev;
    rtx insn;
    rtx* user;
  }
  mem_rec;

typedef
  struct _mem_slot
  { mem_rec* last_rec;
    rtx base;
    rtx reg1;
    rtx reg2;
    int offset;
    enum machine_mode mode;
  }
  mem_slot;

static mem_slot null_slot = { 0 };

static mem_rec*  free_mem_rec;
static mem_slot  mem_id_vec[MAX_MEM_ID+2];
static uid_data*  uid_info;
static rtx*      reg_notes;
static FILE*      dump_stream;
static rtx        cur_insn;
static rtx        start_label;
static rtx        end_label;

static int max_uid;
static int next_iid;
static int next_mem_id;
static int report_id = 32;
static int num_notes;

#define IID(INSN) (uid_info[INSN_UID(INSN)].iid)
#define FIRST(INSN) (uid_info[INSN_UID(INSN)].first_label)
#define LAST(INSN) (uid_info[INSN_UID(INSN)].last_label)
#define MEM_INFO(INSN,KIND) ((uid_info[INSN_UID(INSN)].mem_info[(int)KIND]))
#define MEM_KILL(INSN) ((uid_info[INSN_UID(INSN)].mem_info[(int)kill_context]))
#define MEM_SET(INSN) ((uid_info[INSN_UID(INSN)].mem_info[(int)set_context]))
#define MEM_USE(INSN) ((uid_info[INSN_UID(INSN)].mem_info[(int)use_context]))
#define ADDR_KILL(INSN) ((uid_info[INSN_UID(INSN)].addr_kill))
#define RANGE(I1,I2,I3) ((IID(I1)>=IID(I2)) && (IID(I1)<=IID(I3)))

#define SESE(I) ((uid_info[INSN_UID(I)].sese))
#define STARTS_SESE(I) (( ((SESE(I)) && (IID(SESE(I)) > IID(I))) ? SESE(I) : 0 ))
#define ENDS_SESE(I)   (( ((SESE(I)) && (IID(SESE(I)) < IID(I))) ? SESE(I) : 0 ))

#define FLOW(I) ((uid_info[INSN_UID(I)].flow))
#define FLOW_THRU(I) (*( (STARTS_SESE(I)) ? &(FLOW(SESE(I))) : &(FLOW(I)) ))

#define ADDR_BASE(R)   (( (GET_CODE((R))==MULT) ? (XEXP((R),0)) : (R) ))
#define HAS_SCALE(X)   (( (X) && GET_CODE(X)==MULT ))
#define HAS_REG(X)     (( (X) && GET_CODE(ADDR_BASE(X))==REG ))
#define CALL_KILLS_REGNO(N) (( ((N)<FIRST_PSEUDO_REGISTER) \
   && call_used_regs[N] && !fixed_regs[N] ))

#define SAME_TERM(X,Y)   (( ((X)==(Y)) || ( \
  ((X)!=0 &&(Y)!=0) && \
  (GET_CODE(X)==MULT) && \
  (GET_CODE(X)==GET_CODE(Y)) && \
  (XEXP((X),0)==XEXP((Y),0)) && \
  (INTVAL(XEXP((X),1))==INTVAL(XEXP((Y),1))) \
) ))

#define SCALAR_MODE(M) (( (GET_MODE_CLASS(M))==MODE_INT ||\
                          (GET_MODE_CLASS(M))==MODE_FLOAT ))

#define REGOP(I) (( GET_CODE(I)==REG || \
 (GET_CODE(I)==SUBREG && GET_CODE(XEXP(I,0))==REG) ))

#define GET_NOTE(REG) (( ((REG)<num_notes) ?reg_notes[(REG)] :0 ))

/* There better be one ! */
#define REF_NOTE(REG) (( reg_notes[REG] ))

#define REAL_INSN(I) (GET_CODE(I)==INSN || GET_CODE(I)==JUMP_INSN \
                      || GET_CODE(I)==CALL_INSN)

#define BBLOCK(I) (I==0 || GET_CODE(I)==JUMP_INSN || \
  (GET_CODE(I)==CODE_LABEL))

#define CC0_INSN(I) (( \
  (I!=0) && \
  (GET_CODE(I)==INSN||GET_CODE(I)==JUMP_INSN|| \
  GET_CODE(I)==CALL_INSN) && \
  (reg_mentioned_p (cc0_rtx, PATTERN(I))) ))

static void 
dump_mem_rec_chain (file, m)
FILE* file;
mem_rec* m;
{
  while (m)
  {
    if (m->insn)
      fprintf (file, "%d(@%x)", INSN_UID(m->insn), (long)*(m->user));

    if (m->prev)
      fprintf (file, " -> ");

    m = m->prev;
  }
}

static void
dump_mem_slot (file, src)
FILE* file;
mem_slot src;
{
  fprintf (file, " (o=%2d,m=%2d,r=%x,%x,%x) uids ",
           src.offset,(int)src.mode,(long)src.base,(long)src.reg1,(long)src.reg2);

  dump_mem_rec_chain (file, src.last_rec);

  if (src.base)  fprint_rtx (file, src.base);
  if (src.reg1)  fprint_rtx (file, src.reg1);
  if (src.reg2)  fprint_rtx (file, src.reg2);
  fprintf (file, "\n");
}

static void dump_mem_id (file, id)
FILE* file;
int id;
{
  fprintf (file, "mem %2d", id);

  if (id >= 0 && id < next_mem_id)
    dump_mem_slot (file, mem_id_vec [id]);
  else
    fprintf (file, " is illegal; next_mem_id is %d\n", next_mem_id);
}

static void dump_mem_id_range (file, id1, id2)
FILE* file;
int id1;
int id2;
{
  int i;
  fprintf (file, "\nmem_id_vec entries [%d,%d]\n", id1, id2);
  for (i = id1;  i <= id2; i++)
  {
    dump_mem_id (file, i);
    fprintf (file, "\n");
  }
}

void debug_mem_id (id)
int id;
{ dump_mem_id (stderr, id);
}

void debug_mem_id_range (id1, id2)
int id1;
int id2;
{ dump_mem_id_range (stderr, id1, id2);
}

static void
dump_uid_data (file, src)
FILE* file;
uid_data src;
{
  fprintf (file, " iid%4d", src.iid);

  if (src.first_label)
    fprintf (file, " F%4d", INSN_UID(src.first_label));

  if (src.last_label)
    fprintf (file, " L%4d", INSN_UID(src.last_label));

  if (src.sese)
    fprintf (file, " S%4d%c", INSN_UID (src.sese),
                   ((IID(src.sese) < src.iid) ? '-' : '+'));

  if (!empty_set (src.mem_info [(int)kill_context]))
  { fprintf (file, " kill=");
    dump_small_set (file, src.mem_info [(int)kill_context]);
  }
  if (!empty_set (src.mem_info [(int)set_context]))
  { fprintf (file, " set=");
    dump_small_set (file, src.mem_info [(int)set_context]);
  }
  if (!empty_set (src.mem_info [(int)use_context]))
  { fprintf (file, " use=");
    dump_small_set (file, src.mem_info [(int)use_context]);
  }
  if (!empty_set (src.addr_kill))
  { fprintf (file, " akil=");
    dump_small_set (file, src.addr_kill);
  }
  if (!empty_set (src.flow.dead))
  { fprintf (file, " dead=");
    dump_small_set (file, src.flow.dead);
  }
  if (!empty_set (src.flow.def))
  { fprintf (file, " def=");
    dump_small_set (file, src.flow.def);
  }
  if (!empty_set (src.flow.use_def))
  { fprintf (file, " use_def=");
    dump_small_set (file, src.flow.use_def);
  }
  fprintf (file, "\n");
}

static void
dump_insn (file, insn)
FILE* file;
rtx insn;
{
  fprintf (file, "uid%4d", INSN_UID(insn));
  fflush (file);

  dump_uid_data (file, uid_info[INSN_UID(insn)]);
  fflush (file);
}

static void
dump_insn_range (file, insn, end)
FILE* file;
rtx insn;
rtx end;
{
  while (insn != end)
  { dump_insn (file, insn);
    insn = NEXT_INSN (insn);
  }

  if (end)
    dump_insn (file, end);
}

void
debug_insn (insn)
rtx insn;
{
  fprintf (stderr, "%8x: ", (long) insn);
  if (insn)
    debug_rtx (insn);
  else
    fprintf (stderr, "\n");
}

void
debug_insn_range (insn, end)
rtx insn;
rtx end;
{
  while (insn != end)
  { debug_insn (insn);
    insn = NEXT_INSN (insn);
  }

  if (end)
    debug_insn (end);
}

void
debug_chain (start, stop)
rtx start;
rtx stop;
{
  while (start != stop)
  { debug_rtx (start);
    start = NEXT_INSN (start);
  }
}

void dump_reg_notes (file)
FILE* file;
{
  int i = 0;
  int fnd = 0;

  fprintf (file, "space for %d notes;", num_notes);

  for (i=0;  i < num_notes;  i++)
    if (reg_notes[i])
      if (fnd == 0)
      { fprintf (file, " %d", i);
        fnd = 1;
      }
      else
        fprintf (file, ",%d", i);

  fflush (file);

  if (fnd)
    for (i = 0;  i < num_notes;  i++)
      if (reg_notes[i])
      { fprint_rtx (file, reg_notes[i]);
        fprintf (file, "  -> reg %d", i);
      }

  fprintf (file, "\n");
}

static int
addr_never_taken (x)
rtx x;
{
  int ret = 0;
  if (x != 0 && GET_CODE(x)==SYMBOL_REF)
    ret = !SYM_ADDR_TAKEN_P (x);
  return ret;
}

static int ALIAS (id, x)
unsigned id;
rtx x;
{
  /*  Return true if there is any chance id has an alias
      should control reach x. */

  mem_slot* m = &mem_id_vec[id];
  int ret;

  /* First, if this insn kills id or its address, it aliases id. */
  if ((ret = elt_in_set(union_set(MEM_KILL(x),ADDR_KILL(x)), id)) == 0)
  {
    small_set s;
    unsigned candid;

    s = delete_elt
      (union_set(union_set(MEM_USE(x),MEM_SET(x)),MEM_KILL(x)), id);

    /* Look at all other mems besides m which appear in the insn ... */
    while (ret == 0 && (candid = first_elt (s)) <= MAX_MEM_ID)
    {
      mem_slot* n = &mem_id_vec[candid];
      s = delete_elt (s, candid);

      /* If either of (m,n) has no symbol, or if both (m,n) have the
         same symbol, (m,n) are potential aliases unless they are
         identical except for a constant. */

      if ((m->base==0 && !addr_never_taken (n->base)) ||
          (n->base==0 && !addr_never_taken (m->base)) || m->base == n->base)
      { ret = 1;

        if (m->base==n->base
        &&  SAME_TERM(m->reg1,n->reg1) && SAME_TERM(m->reg2,n->reg2))

          /* If offsets are different, see if m runs over n, or n runs
             over m.  If offsets are the same, m and n are aliases
             because they have different id's but the same address. */

          if (m->offset < n->offset)
            ret = ((!SCALAR_MODE(m->mode)) ||
                   (m->offset+GET_MODE_SIZE(m->mode)) > n->offset);
          else if (n->offset < m->offset)
            ret = ((!SCALAR_MODE(n->mode)) ||
                   (n->offset+GET_MODE_SIZE(n->mode)) > m->offset);
      }
    }
  }
  return ret;
}

static int USABLE_MEM (id, x)
unsigned id;
rtx x;
{
  /* Return true if this insn or SESE region has a set/def
     that we can shadow.  Address is constant, no aliases,
     guaranteed that we can load/store. */

  int ret = 0;

  if (elt_in_set (MEM_SET(x), id))

    /* If id is known to be an object, or if its entire address is
       a constant, then if we saw a store, we are going to say it is
       OK to issue a store after the shadow region if we can prove
       only that we would do a memory access - even if we can't
       prove a store.  The user is cheating something awful if this
       actually breaks his code. */

    if (mem_id_vec[id].base || !mem_id_vec[id].reg1)
      ret = elt_in_set (FLOW_THRU(x).use_def, id);
    else
      ret = elt_in_set (FLOW_THRU(x).def, id);

  else if (elt_in_set (MEM_USE(x), id))
    ret = elt_in_set (FLOW_THRU(x).use_def, id);

  if (ret)
    ret = (!ALIAS (id,x));

  return ret;
}

static
merge_addr_info (y, x)
mem_slot* y;
mem_slot* x;
{
  /* x and y are legal mem_slots. merge y into x. */

  if (y->base)
  { if (x->base || (x->reg2 && HAS_REG (y->base)))
      return 0;

    x->base = y->base;
  }

  if (y->reg2)
  { if (x->reg1 || HAS_REG (x->base))
      return 0;

    x->reg1 = y->reg1;
    x->reg2 = y->reg2;
  }

  else
    if (y->reg1)
    { if (x->reg2 || x->reg1==y->reg1
      || (x->reg1 && HAS_REG (x->base))
      || (HAS_SCALE(x->reg1) && HAS_SCALE(y->reg1)))
        return 0;

      if (x->reg1==0)
        x->reg1 = y->reg1;
      else
        if (REGNO (ADDR_BASE(x->reg1)) < REGNO(ADDR_BASE(y->reg1)))
          x->reg2 = y->reg1;
        else
        { x->reg2 = x->reg1;
          x->reg1 = y->reg1;
        }
    }

  x->offset += y->offset;
  return 1;
}

static
add_addr_info (x, info)
rtx x;
mem_slot* info;
{
  /* Add the addressing stuff in x to *info.

     If there is a note in the note table which equivs registers used
     in this address to something else, we use the rhs of the note
     instead of the register.  We must do this because when we set
     up the equivalence, we explicitly looked the other way about
     the fact that the address of the symbol on the rhs was being
     put into a register.  Were we to now use the register instead of
     the note, the current mem would be an alias for those mems using the
     symbol explicitly, and we wouldn't know it. */

  enum rtx_code code;
  rtx note;

  while ((code=GET_CODE(x))==REG && (note=GET_NOTE(REGNO(x))))
    x=XEXP(note,0);

  switch (code)
  {
    case REG:
    case LABEL_REF:
    case SYMBOL_REF:
    case MULT:
    {
      mem_slot tmp;
      tmp = null_slot;

      if (code==MULT)
      { rtx scale;

        if (GET_CODE(scale=XEXP(x,1))==CONST_INT)
        { if (!add_addr_info(XEXP(x,0),&tmp))  return 0;
        }
        else if (GET_CODE(scale=XEXP(x,0))==CONST_INT)
        { if (!add_addr_info(XEXP(x,1),&tmp))  return 0;
        }
        else return 0;

        if (INTVAL(scale)==0)
          tmp = null_slot;

        else
          if (INTVAL(scale) != 1)
          { 
            if (tmp.base || tmp.reg2 || HAS_SCALE(tmp.reg1))  return 0;

            if (tmp.reg1)
              tmp.reg1 = gen_rtx (MULT, GET_MODE(scale), tmp.reg1, scale);

            tmp.offset *= INTVAL (scale);
          }
      }

      else if (GET_CODE(x)==REG
       && REGNO(x)!=FRAME_POINTER_REGNUM && REGNO(x)!=ARG_POINTER_REGNUM)
        tmp.reg1 = x;

      else
        tmp.base = x;

      return merge_addr_info (&tmp, info);
    }

    case CONST_INT:
      info->offset += INTVAL (x);
      return 1;

    case CONST:
      return add_addr_info (XEXP(x,0),info);

    case PLUS:
      return add_addr_info (XEXP(x,0),info) && add_addr_info (XEXP(x,1),info);

    default:
      return 0;
  }
}

static int 
find_mem (mem)
rtx mem;
{
  /*  The caller has just found a fresh mem.

      If it might be dangerous to shadow across this mem,
      return 0.

      If we know that the mem has a good address, and if we know
      that we don't want to shadow it and that it can never be an
      alias for things we might shadow (currently, only switch
      table references fit these criteria) then return -1.

      Otherwise, if we've got the address of the mem somewhere in
      mem_id_vec already, return the appropriate index;  else,
      place it in the table with index next_mem_id.  In the
      latter case, we let the caller bump next_mem_id, so that
      he can tell when the mem is a new one.
  */

  register mem_slot* x = &mem_id_vec[next_mem_id];
  register mem_slot* y = x;

  *x = null_slot;
  x->mode = GET_MODE(mem);

  if (add_addr_info(XEXP(mem,0),x)==0)          return 0;
  if (x->base && GET_CODE(x->base)==LABEL_REF)  return -1;

  while (x-- != mem_id_vec)
    if (x->base==y->base
    &&  SAME_TERM(x->reg1,y->reg1) && SAME_TERM(x->reg2,y->reg2)
    &&  x->mode==y->mode && x->offset == y->offset)
      return x-mem_id_vec;

  if (next_mem_id > report_id && report_id > 0)
  { fprintf (stderr, "exceeded %d mem_ids at line %d\n", report_id, lineno);
    report_id = -next_mem_id;
  }

  if (report_id < 0 && (-next_mem_id) < report_id)
    report_id = -next_mem_id;

  if (next_mem_id > MAX_MEM_ID)
  { if (dump_stream)
      fprintf (dump_stream, "ran out of mem_ids at line %d\n", lineno);
    return 0;
  }

  return next_mem_id;
}

static
examine_insn (user, usage)
rtx* user;
ksu usage;
{
  /* Put every usable memory reference in x into the memory table.

     If we see a mem that we don't understand, or if we run out,
     put it in the table as the a deref of the 0 pointer, and
     say that this insn kills all mems.

     If we see a mem that we understand but do not wish to shadow,
     and if we know that it cannot alias anything we wish to
     shadow, we just pretend we never saw it.  */

  rtx x = *user;

  enum rtx_code code = GET_CODE (x);

  if (code==MEM)
  { int i = find_mem(x);

    if (i >= 0)
    { unsigned id = i;
      mem_slot* m = &mem_id_vec [id];
      mem_rec*  new;
      /* temp variable added to simplify an expression for the macintosh compiler */
      small_set *memInfoAddr ;

      if (id == 0)
        MEM_KILL(cur_insn) = all_ones;

      else if (id == next_mem_id)
      { /* A new one. */
        register rtx r;
        register int i;

        if ((r=m->reg1) != 0)
          if (((r=ADDR_BASE(r)),i=REGNO(r)), CALL_KILLS_REGNO(i))
            call_kills_addr = union_elt (call_kills_addr, id);

        if ((r=m->reg2) != 0)
          if (((r=ADDR_BASE(r)),i=REGNO(r)), CALL_KILLS_REGNO(i))
            call_kills_addr = union_elt (call_kills_addr, id);

        next_mem_id++;
      }

      if ((new = free_mem_rec) != 0)
        free_mem_rec = new->prev;
      else
        new = (mem_rec*) xmalloc (sizeof (mem_rec));

      new->prev  = mem_id_vec[id].last_rec;
      new->insn  = cur_insn;
      new->user  = user;

      mem_id_vec[id].last_rec = new;

      if (id == 0 || MEM_VOLATILE_P(x) || !SCALAR_MODE(GET_MODE(x)))
        usage = kill_context;

      memInfoAddr = &MEM_INFO(cur_insn,usage) ;
      *memInfoAddr = union_elt (*memInfoAddr, id);
    }
  }

  else if (code == SYMBOL_REF)
    SYM_ADDR_TAKEN_P (x) = 1;

  else if (code == REG)
  {
    rtx note = (REGNO(x)>=num_notes) ? 0 : reg_notes[REGNO(x)];

    if (note)
    { assert (usage != set_context);
      examine_insn (&XEXP(note,0), usage);
    }
  }

  else if (code == SUBREG       || code == SIGN_EXTRACT ||
           code == ZERO_EXTRACT || code == STRICT_LOW_PART)
    examine_insn (&XEXP(x,0), usage);

  else if (code == LABEL_REF)
  {
    rtx label = XEXP(x,0);
/* variables added to simplify expressions for the macintosh compiler */
	rtx firstOne, lastOne ;

    assert (GET_CODE (label) == CODE_LABEL);


    if (INSN_UID (label) == 0)
      return;

	firstOne = FIRST(cur_insn) ;
    if (firstOne==0
    ||  IID(label) < IID(firstOne))
      FIRST (cur_insn) = label;

	lastOne = LAST(cur_insn) ;
    if (lastOne==0
    ||  IID(label) > IID(lastOne))
      LAST (cur_insn) = label;

    if (FIRST(label)==0)
      FIRST (label) = cur_insn;

    LAST(label) = cur_insn;

    CONTAINING_INSN (x) = cur_insn;
    LABEL_NEXTREF (x) = LABEL_REFS (label);
    LABEL_REFS (label) = x;
  }

  else if (code == SET)
  { rtx note;

    /* If this SET sets up an EQUIV between a reg and a constant,
       we don't examine the operands.  If the reg gets indirected on
       later, add_addr_info will use the note and not the register.

       We explicitly do not examine the rhs because if we were to
       examine it, we would ruin any symbol found there because
       we would have to record that the symbol had its address taken.

       Any non-indirection use of the target (except as the
       source of another EQUIV) will cause the rhs to get marked as
       having had its address taken. */

    if (GET_CODE (XEXP(x,0))==REG)
    { for (note = REG_NOTES (cur_insn); note; note = XEXP (note,1))
        if (REG_NOTE_KIND(note) == REG_EQUIV)
          if (!rtx_unstable_p (XEXP(note,0)))
            break;
    }
    else
      note = 0;

    if (note)
    { int regno = REGNO(XEXP(x,0));

      if (num_notes == 0)
      { num_notes = regno<=100 ? 200 : regno*2;
        reg_notes = (rtx*) xmalloc (num_notes * sizeof (reg_notes[0]));
        bzero ((char*)reg_notes,    num_notes * sizeof (reg_notes[0]));
      }
      else
        if (regno >= num_notes)
        { int new_size, old_size;

          old_size  = num_notes * sizeof(reg_notes[0]);
          num_notes = regno * 2;
          new_size  = num_notes * sizeof(reg_notes[0]);

          reg_notes = (rtx*) xrealloc ((char*)reg_notes, new_size);
          bzero (((char*)reg_notes)+old_size, new_size-old_size);
        }

      assert (reg_notes[regno] == 0);
      reg_notes[regno] = note;
    }
    else if (GET_CODE(XEXP(x,1)) == ASM_OPERANDS)
    { examine_insn (&XEXP(x,0), kill_context);
      examine_insn (&XEXP(x,1), kill_context);
    }
    else
    { examine_insn (&XEXP(x,0), set_context);
      examine_insn (&XEXP(x,1), use_context);
    }
  }

  else if (code == CLOBBER)
  {
    if (XEXP (x, 0) == 0)
      MEM_KILL(cur_insn) = all_ones;
    else
      examine_insn (&XEXP(x,0), kill_context);
  }

  else if (code == CALL)
    MEM_KILL(cur_insn) = all_ones;

  else
  { char* fmt = GET_RTX_FORMAT (code);
    int i;

    if (code == ASM_OPERANDS)
      usage = kill_context;

    for (i=0; i < GET_RTX_LENGTH(code); i++)
      if (fmt[i] == 'e')
        examine_insn (&XEXP(x,i), usage);

      else if (fmt[i] == 'E')
      { int j = XVECLEN(x,i);
        while (--j >= 0)
          examine_insn (&XVECEXP(x,i,j), usage);
      }
  }
  return;
}

static rtx*
make_reg_list (x, next)
register rtx x;
register rtx* next;
{
  /* List the registers used in this rtx in the vector at
     next[0], next[1], ... */

  enum rtx_code code = GET_CODE(x);

  if (code == REG)
    *next++ = x;

  else
  { char* fmt = GET_RTX_FORMAT (code);
    int i;

    for (i=0; i < GET_RTX_LENGTH(code); i++)
      if (fmt[i] == 'e')
        next = make_reg_list (XEXP(x,i), next);

      else if (fmt[i] == 'E')
      { int j = XVECLEN(x,i);
        while (--j >= 0)
          next = make_reg_list (XVECEXP(x,i,j), next);
      }
  }

  return next;
}

static int
rtx_kills_reg (x, reg)
register rtx x;
register rtx reg;
{
  enum rtx_code code = GET_CODE(x);
  int  ret = 0;

  /* If there is an chance this rtx changes the value of
     reg, return true. */

  if (code == CALL)
    if (REGNO(reg) < FIRST_PSEUDO_REGISTER)
      if (call_used_regs[REGNO(reg)] && !fixed_regs[REGNO(reg)])
        return 1;

  if ((code == SET || code == CLOBBER))
  { register rtx dest = SET_DEST (x);

    /* CLOBBER w/0 arg means kill all mems.  Not a problem
       for registers. */

    if (dest != 0)
    { while (GET_CODE (dest) == SUBREG
	     || GET_CODE (dest) == ZERO_EXTRACT
	     || GET_CODE (dest) == SIGN_EXTRACT
	     || GET_CODE (dest) == STRICT_LOW_PART)
        dest = XEXP (dest, 0);

      if (GET_CODE(dest) == REG)
        ret = (dest==reg);
    }
  }
  else
  { char* fmt = GET_RTX_FORMAT (code);
    int i;

    for (i=0, ret=0; ret==0 && i < GET_RTX_LENGTH(code); i++)
      if (fmt[i] == 'e')
        ret |= rtx_kills_reg (XEXP(x,i), reg);

      else if (fmt[i] == 'E')
      { int j = XVECLEN(x,i);
        while (--j >= 0 && ret==0)
          ret |= rtx_kills_reg (XVECEXP(x,i,j), reg);
      }
  }
  return ret;
}

static int
insn_kills_reg (i, reg)
register rtx i;
register rtx reg;
{
  if (REAL_INSN(i))
    return rtx_kills_reg (PATTERN (i), reg);
  else
    return 0;
}

static void
note_addr_kills (x, kills)
register rtx x;
small_set* kills;
{
  /* Remove from *kills all slots whose addresses
     are invalidated by this rtx. */

  enum rtx_code code = GET_CODE(x);

  if ((code == SET || code == CLOBBER))
  { register rtx dest = SET_DEST (x);

    /* CLOBBER w/0 arg means kill all mems.  Not a problem
       for addresses. */

    if (dest != 0)
    { while (GET_CODE (dest) == SUBREG
	     || GET_CODE (dest) == ZERO_EXTRACT
	     || GET_CODE (dest) == SIGN_EXTRACT
	     || GET_CODE (dest) == STRICT_LOW_PART)
        dest = XEXP (dest, 0);

      if (GET_CODE(dest) == REG)
      { register unsigned i;
        for (i=1; i<next_mem_id; i++)
        { mem_slot* m =  &mem_id_vec[i];

          if ((m->reg1 != 0 && ADDR_BASE(m->reg1)==dest)
            ||(m->reg2 != 0 && ADDR_BASE(m->reg2)==dest)
            ||(m->base==dest))
            *kills = union_elt (*kills, i);
        }
      }
    }
  }
  else if (code == CALL)
    *kills = union_set (*kills, call_kills_addr);

  else if (code == PARALLEL)
  { register i = XVECLEN(x,0);
    while (--i >= 0)
      note_addr_kills (XVECEXP(x,0,i), kills);
  }
}

int 
simple_target (t)
rtx t;
{
  rtx i;

  /* t is a label.  return "there is absolutely no way
     control can get to t without branching here"    */

  i=PREV_INSN(t);
  while (i!=0 && GET_CODE(i)==NOTE)
    i = PREV_INSN(i);

  return i!=0 && (GET_CODE(i)==BARRIER ||
        ((GET_CODE(i)==JUMP_INSN && simplejump_p (i))));
}

static void
init_flow_set (x)
rtx x;
{
#if defined(WIN95)
  flow_set fs;
  assert (!SESE(x));
  fs = FLOW_THRU(x);
  fs.use_def = and_not (union_set (MEM_USE(x), MEM_SET(x)), ADDR_KILL (x));
  fs.def = and_not (MEM_SET(x), ADDR_KILL (x));
#else
  assert (!SESE(x));
  FLOW_THRU(x).use_def = and_not (union_set (MEM_USE(x), MEM_SET(x)), ADDR_KILL (x));
  FLOW_THRU(x).def = and_not (MEM_SET(x), ADDR_KILL (x));
#endif
}

static void
intersect_flow_set (dst, src)
flow_set* dst;
flow_set  src;
{
  /*  Intersect *dst with src. */
  and_to (&(dst->use_def), src.use_def);
  and_to (&(dst->def), src.def);
}

static int
add_to_flow_set (to, s)
rtx to;
flow_set s;
{
  /*  Update flow info in 'to'.  Don't put anything in the sets
      which is illegal; i.e, if the address dies, we can't
      use flow info about it. */

  int ret;

#if defined(WIN95)
  flow_set fs;

  fs = FLOW_THRU(to);
  ret  = union_to (&(fs.use_def), and_not (s.use_def, ADDR_KILL(to)));
  ret |= union_to (&(fs.def), and_not (s.def, ADDR_KILL(to)));
#else
  ret  = union_to (&(FLOW_THRU(to).use_def), and_not (s.use_def, ADDR_KILL(to)));
  ret |= union_to (&(FLOW_THRU(to).def), and_not (s.def, ADDR_KILL(to)));
#endif

  return ret;
}

static int
flow_implication (to, from)
rtx to;
rtx from;
{
  /*  Should 'to' be executed, 'from' will be executed.  Update
      to's flow stuff accordingly;  return 1 iff anything changed. */

  return add_to_flow_set (to, FLOW_THRU (from));
}

static int
flow_equivalence (to, from)
rtx to;
rtx from;
{
  /*  Should 'to' be executed, 'from' will be executed and vv.  Update
      both 'to' and 'from' accordingly;  return 1 iff any changes. */

  return flow_implication (to,from) | flow_implication (from,to);
}

static void
push_sese_context (t)
rtx t;
{
  /* Now, SESE regions have been determined, and each region has its
     flow_thru set to indicate the valid mems found within the
     region as well as those known to be valid from flow in the
     immediate parent region.  We are almost done, but this is not quite
     the whole story; every mem which is valid in a region which contains
     another region is also valid throughout the contained region.

     Propagate this stuff down and thru each region as appropriate. */

  rtx p = t;

  t=NEXT_INSN(t);
  while (NEXT_INSN(t))
  {
    /* Retrieve the parent region when we reach the the end of each region. */
    if (SESE(t) && IID(SESE(t)) < IID(t))
    { rtx r = p;
      p = SESE(t);
      SESE(t) = r;
    }

    /* Put callers stuff into this node. */
    flow_implication (t, p);

    /* Save the current parent in the end of the region when we
       start a new region, then reset the parent. */

    if (SESE(t) && IID(SESE(t)) > IID(t))
    { SESE(SESE(t))=p;
      p = t;
    }
    t=NEXT_INSN(t);
  }
}

static void
find_sese (x, stop)
rtx x;
rtx stop;
{
  /* Mark all sese regions between x and stop.  For each such region,
     collect the union of the sets/defs/kills sets into the node which
     heads the region. */

  while (x != stop)
  { rtx next;

    /*  Get the start of the next basic block after x. */
    for (next=NEXT_INSN(x); !BBLOCK(next);  next=NEXT_INSN(next));

    /* If x is a forward jump or backward label, it may start a region. */
    if (IID(FIRST(x)) > IID(x))
    { rtx y,t;

      /* Examine each basic block on (x, y] ... */
      for (t=next,y=LAST(x); t!=stop && IID(t)<=IID(y); )
      {
        /* If we find a branch to a point before x, or a label which can
           be reached from before x, x cannot head a region. */
        if (IID(FIRST(t)) < IID(x)) break;

        /* If we find a branch beyond y or a label which is used beyond
           y, we extend y and keep going. */
        if (IID(LAST(t)) > IID(y)) y = LAST(t);

        /* Get the next block on (x, y] to look at. */
        for (t=NEXT_INSN(t); !BBLOCK(t); t=NEXT_INSN(t));
      }

      /* If we pedaled all the way thru, [x,y] must be sese. */
      if (t==0 || IID(t)>IID(y))
      { rtx p;
        int change;

        assert (IID(y) > IID(x));

        find_sese(next,y);
        next=t;

        /* If first insn uses cc, we include the set to cc0 in the region. */
        if (CC0_INSN(x))
          for (t=PREV_INSN(x); !BBLOCK(t) && CC0_INSN(t); x=t,t=PREV_INSN(t));

        /* Don't get in the way of loop notes if we can help it. */
        for (t=PREV_INSN(x); t != 0 && GET_CODE(t)==NOTE; t=PREV_INSN(t))
          if (NOTE_LINE_NUMBER(t)==NOTE_INSN_LOOP_BEG)
            x=t;

        for (t=NEXT_INSN(y); t != 0 && GET_CODE(t)==NOTE; t=NEXT_INSN(t))
          if (NOTE_LINE_NUMBER(t)==NOTE_INSN_LOOP_END)
            y=t;

        for (t=x; t!=NEXT_INSN(y); t=NEXT_INSN(SESE(t)?SESE(t):t))
        {
          if (REAL_INSN (t))
            if (!SESE(t))
            { note_addr_kills (PATTERN(t), &ADDR_KILL(t));

              if (t!=x && t!=y)
                init_flow_set (t);
            }

          MEM_USE(x)   = union_set (MEM_USE(x),   MEM_USE(t));
          MEM_SET(x)   = union_set (MEM_SET(x),   MEM_SET(t));
          MEM_KILL(x)  = union_set (MEM_KILL(x),  MEM_KILL(t));
          ADDR_KILL(x) = union_set (ADDR_KILL(x), ADDR_KILL(t));
        }

        SESE(x)=y;
        SESE(y)=x;

        MEM_USE(y)=MEM_USE(x);
        MEM_SET(y)=MEM_SET(x);
        MEM_KILL(y)=MEM_KILL(x);
        ADDR_KILL(y)=ADDR_KILL(x);

        /* Propagate valid address information thru each interior
           SESE region.  After this is done, flow and flow_thru 
           for each SESE region will both contain all flow info known
           to be valid for that region.
     
           We do this as follows:

             For any SESE region, we are guaranteed to come in the top
             and go out the bottom.  The implication is that if any
             dereference is known to reach the top or the bottom, that
             memory address must be usable throughout the entire region,
             since the address is constant.  So, peddle around the
             region, plugging the known good stuff that comes out the
             bottom of the region back into the top, until we don't see
             any more changes.

             To propagate information thru one interior SESE region into
             another, we use only flow_thru;  when the dust settles, this
             leaves us with the flow set for every interior SESE region
             containing only those mems actually hit by that region, while
             the flow_thru set for each region has in addition that info
             known in that region because of control flow thru it.
        */

        p = x;
        t = NEXT_INSN(x);

        do
        {
          /* At the top of the region, reset the change indicator;
             the stuff that reached the end of the region is already
             associated with the top because both nodes share the
             same thru sets. */

          if (p==x)
            change = 0;

          if (GET_CODE(t)==CODE_LABEL)
          { flow_set preds;
            rtx j;

            if (simple_target (t))
              /* All possible preds will be in label ref chain. */
              preds = all_ones_flow_set;
            else
              /* Can fall-thru.  All preds may not be in label ref chain. */
              preds = FLOW_THRU(p);

            /* If we will fall into t via p, flow p into t and vv. */
            if (IID(FIRST(t))>IID(t) && (ENDS_SESE(p) || GET_CODE(p)!=JUMP_INSN))
              change |= flow_equivalence (p, t);

            /* Collect all the mems which are hit in every predecessor into preds;
               push the mems known to be hit at this label into every predecessor
               which branches here unconditionally. */

            for (j=LABEL_REFS(t);  j!=t;  j=LABEL_NEXTREF(j))
            {
              rtx cj = CONTAINING_INSN(j);
              intersect_flow_set (&preds, FLOW_THRU(cj));

              if (simplejump_p (cj))
                change |= flow_implication (cj, t);
            }

            /* Put the stuff which survived every predecessor into the label. */
            change |= add_to_flow_set (t, preds);
          }

          /* In all other cases besides labels, if the predecessor is
             not a jump, or if the predecessor is an sese region,
             we propagate thru to/from the predecessor, because
             execution of the predecessor imples execution of of the current
             insn or region, and vv. */

          else if (ENDS_SESE(p) || GET_CODE(p)!=JUMP_INSN)
            change |= flow_equivalence (p, t);

          t=NEXT_INSN (p=(SESE(t)?SESE(t):t));
        }
        while (p!=x || change);

        FLOW(x)=FLOW_THRU(x);
      }
    }
    x=next;
  }
}

static rtx
pick_region (id, next, load, store)
unsigned id;
rtx* next;
int* load;
int* store;
{
  rtx i, x, y, p;
  int need_load, need_store;

  /*  Pick the next shadow region for id from [*next, ...).  Indicate
      whether a load before/store after is required. */

  *load  = 0;
  *store = 0;

  /* Find an INSN or an entire SESE region that uses this mem. */
  for (x = *next, p = 0; x != 0; x = NEXT_INSN(x))
    if (INSN_UID(x) <= max_uid && IID(x) != 0 && USABLE_MEM(id,x))
      if (SESE(x) && IID(SESE(x))>=IID(x))
      { y = SESE(x);
        break;
      }
      else if (SESE(x)==0 && !BBLOCK (x))
      { y = x;
        break;
      }

  if (x == 0)
    return (*next = 0);

  need_store = elt_in_set (MEM_SET(x), id);

  if (SESE(x))
    /* Don't know for sure if region kills it */
    need_load = need_store || elt_in_set (MEM_USE(x), id);
  else
    need_load = elt_in_set (MEM_USE(x), id);

  /* Take in as many hits in the following straight line blocks as
     we can.  This means, eat up every SESE block or non-BBLOCK insn
     that does not hit an alias for id. */

  p = y;
  while ((i=NEXT_INSN(y)) != 0)
  {
    while (i != 0 && (INSN_UID(i)>max_uid || IID(i)==0))
      i=NEXT_INSN(i);

    if ((i==0) || ALIAS(id, i))
      break;

    if (SESE(i) && IID(SESE(i))>=IID(i))
      y = SESE(i);
    else
      if (!BBLOCK(i))
        y = i;
      else
        break;

    if (elt_in_set (MEM_USE(i), id) || elt_in_set (MEM_SET(i), id))
    {
      need_store |= elt_in_set (MEM_SET(i), id);

      p = y;
    }
  }

  *load  = need_load;
  *store = need_store;
  *next  = p;

  if (dump_stream)
  { fprintf (dump_stream, "[%d,%d] l=%d,s=%d\n", (x?INSN_UID(x):0),
            (*next?INSN_UID(*next):0),*load,*store);
    fflush (dump_stream);
  }

  return x;
}

void
note_in_struct (m, in_struct)
rtx m;
int *in_struct;
{
  /*  If settings of MEM_IN_STRUCT_P are not consistent, set *in_struct to
      -2, else set to 1 if MEM_IN_STRUCT_P and -1 if not. */

  if (*in_struct != -2)
  { if (MEM_IN_STRUCT_UNSAFE(m))
      *in_struct = -2;
    else if (*in_struct >= 0 && MEM_IN_STRUCT_P (m))
      *in_struct = 1;
    else if (*in_struct <= 0 && !MEM_IN_STRUCT_P (m))
      *in_struct = -1;
    else
      *in_struct = -2;
  }
}

void set_in_struct (m, in_struct)
rtx m;
int in_struct;
{
    MEM_IN_STRUCT_P(m) = (in_struct > 0);
    MEM_IN_STRUCT_UNSAFE(m) = (in_struct == -2);
}

/* This is a hack job to accomodate the fact that we can't replace
   sign extended mems anymore.  Since this entire module (shadow.c) is almost
   obsolete, this will do for now.  If we miss any extends, we will get
   an error because extends on regs won't match. */

replace_mem (insn, user, reg)
rtx insn;
rtx* user;
rtx reg;
{
  rtx src;

  INSN_CODE(insn) = -1;

  if (GET_CODE(src=PATTERN(insn)) == SET)
    if (GET_CODE(src=SET_SRC(src))==SIGN_EXTEND || GET_CODE(src)==ZERO_EXTEND)
      if (XEXP(src,0)==*user)
      { rtx sav, seq, treg;
        START_SEQUENCE (sav);
        
        treg = gen_reg_rtx (GET_MODE(src));
        convert_move (treg, reg, (GET_CODE(src)==ZERO_EXTEND));

        seq = gen_sequence();
        END_SEQUENCE (sav);

        emit_insn_before (seq, insn);
        SET_SRC(PATTERN(insn)) = treg;
        reg = 0;	/* The extend is gone, so try to cause a
			   core dump if anybody gets ahold of it. */
      }

  *(user) = reg;
}

static
shadow (id, first_insn)
unsigned id;
rtx first_insn;
{
  rtx insn, end_insn;
  mem_rec* mem;

  /*  Shadow memory slot 'id' throughout the function body headed
      by first_insn. */

  int load   = 0;
  int store  = 0;
  rtx reg    = 0;
  int ret_val = 0;

  for (end_insn=first_insn;
         (first_insn=pick_region (id, &end_insn, &load, &store)) != 0;
           end_insn=NEXT_INSN(end_insn))
  {
    int loop         = 0;
    int use_cnt      = 0;
    int set_cnt      = 0;
    rtx exp          = 0;
    rtx store_after  = end_insn;
    rtx load_before  = first_insn;
    rtx load_after;
    rtx addr_seq;
    int in_struct    = 0;

    /*  We say that shadowing is worthwhile if the load before/store
        cost (0-2) is less than the static count of references in the
        region.  If the region contains a loop and any use or set,
        we say the id is worth shadowing. */

    for (insn=first_insn; insn != NEXT_INSN(end_insn); insn=NEXT_INSN(insn))
    { rtx note;
      if ((GET_CODE(insn)==NOTE && NOTE_LINE_NUMBER(insn)==NOTE_INSN_LOOP_BEG)||
          (GET_CODE(insn)==CODE_LABEL && IID(FIRST(insn)) > IID(insn)))
        loop = 1;

      /* Expand region for loads/stores to totally enclose libcall pairs */
      if (REAL_INSN (insn))
        if (note = find_reg_note (insn,REG_LIBCALL,0))
        { note = XEXP(note,0);
          if (IID(note) > IID(store_after))
            store_after = note;
        }
        else if (note = find_reg_note (insn,REG_RETVAL,0))
        { note = XEXP(note,0);
          if (IID(note) < IID(load_before))
            load_before = note;
        }

      if (INSN_UID(insn) <= max_uid && IID(insn) && !SESE (insn))
        if (elt_in_set (MEM_SET(insn), id))
          set_cnt++;
        else if (elt_in_set (MEM_USE(insn), id))
          use_cnt++;
    }

    assert ((set_cnt+use_cnt) != 0);

    if (loop==0 && (set_cnt+use_cnt) <= (load+store))
      continue;

    ret_val += (set_cnt + use_cnt);

    mem = mem_id_vec[id].last_rec;
    while (IID(mem->insn) > IID(end_insn))
      mem=mem->prev;

    if (reg == 0)
    {
      reg = gen_reg_rtx (mem_id_vec[id].mode);
      /*
       * This is done so that loop knows that this isn't a simple temp
       * register with very short lifetime.  If this is taken out a bug
       * can occur because of inadequate analysis in loop.  We make the fix
       * in this way because making the change to fix loop causes much
       * worse code to be generated in the cases (which are frequent) where
       * the registers really are single use temps.
       * This problem reported in OMEGA #219.
       */
      REG_USERVAR_P(reg) = 1;
    }

    load_after = PREV_INSN(load_before);
    addr_seq   = 0;

    assert (load_after);

    in_struct = 0;

    while (mem != 0 && IID(mem->insn) >= IID (first_insn))
    {
      if ((load || store) && !exp)
      { /* We don't have a good address yet.  If none of the regs in this
           mem get trashed in the region, we can use its address. */

        rtx reg_list [512], *end_list, *p, q;
        end_list = make_reg_list (exp = *(mem->user), reg_list);

        for (p=reg_list;  exp!=0 && p!=end_list;  p++)
          for (q=load_before;  exp!=0 && q!=NEXT_INSN(store_after); q=NEXT_INSN (q))
            if (insn_kills_reg (q, *p))
              exp = 0;
      }

      note_in_struct (*mem->user, &in_struct);
      replace_mem (mem->insn, mem->user, reg);
      mem = mem->prev;
    }

    /* Get rid of all notes in the sese region which refer to memory */
    remove_mem_notes (load_before, store_after);

    if (load || store)
      if (exp)
        exp = copy_rtx (exp);
      else
      { /* The address lived, but the registers actually appearing
           in the MEMS died.  We have to roll our own address from
           the components we know about. */

        rtx base = mem_id_vec[id].base;
        rtx reg1 = mem_id_vec[id].reg1; 
        rtx reg2 = mem_id_vec[id].reg2; 
        rtx addr, sav;

        if (reg2)
          reg1 = gen_rtx (PLUS, GET_MODE(reg1), reg1, reg2);

        if (exp=reg1)
        { if (base)
            exp = gen_rtx (PLUS, GET_MODE(exp), exp, base);
        }
        else
          if ((exp=base)==0)
            exp = gen_rtx (CONST_INT, VOIDmode, 0);

        addr = plus_constant (exp, mem_id_vec[id].offset);

        START_SEQUENCE (sav);
        addr = memory_address (mem_id_vec[id].mode, addr);
        addr_seq = gen_sequence();
        END_SEQUENCE (sav);

        exp = gen_rtx (MEM, mem_id_vec[id].mode, addr);
      }

    set_in_struct (exp, in_struct);

    if (load)
      emit_insn_after (gen_rtx (SET, exp->mode, reg, exp), load_after);

    if (store)
    {
      if (load)
        exp = copy_rtx (exp);

      set_in_struct (exp, in_struct);
      emit_insn_after  (gen_rtx (SET, exp->mode, exp, reg), store_after);
      end_insn = NEXT_INSN(end_insn);
    }

    if (addr_seq)
      emit_insn_after (addr_seq, load_after);
  }
  return ret_val;
}

void
free_shadow_space ()
{
  /* Shadow can be used more than once per function.  We don't free
     anything when we are done;  we just use it again next time.  Caller
     can explicitly free space after shadow is called with
     free_shadow_space if he wants the space back.  */

  if (max_uid != 0)
  { int i;

    for (i=0;  i < next_mem_id;  i++)
    { mem_rec* p=mem_id_vec[i].last_rec;
      mem_rec* q;

      while (p)
      { q = p->prev;
        free ((char*) p);
        p = q;
      }
    }
  }
  
  if (reg_notes)    free ((char*)reg_notes);	reg_notes  = 0;
  if (uid_info)     free ((char*)uid_info);	uid_info   = 0;

  dump_stream  = 0;
  max_uid      = 0;
  next_iid     = 0;
  next_mem_id  = 0;
  num_notes = 0;
}

shadow_optimize (f, dumpfile)
rtx f;
FILE *dumpfile;
{
  /*  Run the shadow optimizer over the function headed by f.

      This means that we try to replace memory references 
      in the function with REGS over the largest regions
      we can find where we know that the address of the memory is
      constant, and where we can be sure that the semantics of
      the program don't require us to update the actual memory
      locations.  We rely on later phases of the compiler to
      actually allocate the shadow into hard registers, just
      like any other local variables.

      Basically, what we do is:

	1) Count the insns in the functions, noting the highest uid;

	2) Allocate a mapping vector indexed by uid.  This vector
	   will eventually have for each insn:

	    - An id number which indicates the lexical index of 
              the insn within the function, starting at 1;

            - Several sets indicating memory effects of the insn
	      on each memory slot in the function, including:

		- uses, defs, kills within the insn itself;

	        - a set of the memory slots which are known to
		  be loaded/stored sometime before/after the
		  insn executes.

		- a set of the memory slots whose addresses are
		  invalidated by the insn.

	    - The first and last branches out of/into the insn, if
	      the insn is a JUMP_INSN or CODE_LABEL.

	    - If the insn heads a single-entry, single-exit region
	      which contains jumps, a pointer to the end of the region.

	      If the insn ends such a region, a pointer to the beginning
	      of the region.

	3) Examine every insn in the function, adding each new memory
           slot encountered to the memory slot table, noting the
	   slots used by the insn in the use/def/kill sets, and noting
           possible branches in/out of the insn.

	   A memory slot contains:

	    - The mode of the slot (e.g, SImode)

	    - The addressing components for the slot; i.e,
		- base (object name, frame pointer, or 0)
		- 1 scaled index and 1 unscaled index
		- hard constant offset

	    - A pointer to a chain of the MEMs which use or set the slot.

	4) Using the information gathered above, find and mark all SESE
	   regions in the function noting the effects of program flow
	   thru each region and each insn in the 'will load/will store'
	   sets.  Also, note the effects of sets to registers on the
	   validity of mem slot address across each insn and SESE region.

	5) For each slot in the function, walk the function finding the 
	   longest group of insns/SESE regions for which the slot is valid -
	   that is, for which

	    - The slot address is constant;
	    - The slot has no potential aliases;
	    - The slot is known to be loadable/storable before/after the
	      region if it isn't known to be dead in/out.

	   As each such region is found, if the use/def count seems to pay
	   for any load before/store after the region, we replace each MEM
	   in the region with a REG, and insert code to load the REG from
	   the slot before the region, and store the slot from the REG after
	   the region, should such be needed.  */

  int old_max_uid      = max_uid;
  int ret_val          = 0;
  rtx ruse             = 0;

  rtx insn;
  int i,go;

  if (report_id < 0)
    report_id = -report_id;

  dump_stream  = dumpfile;
  max_uid      = 0;
  next_iid     = 1;

  for (i=0; i<MEM_ID_SET_WDS;i++)
    all_ones.a[i] = -1;

  if (num_notes)
    bzero ((char*)reg_notes, num_notes * sizeof (reg_notes[0]));

  /* Put all mem records now in use on the free list. */
  while (next_mem_id)
  {
    mem_rec* last_free = mem_id_vec[--next_mem_id].last_rec;

    if (last_free)
    { while (last_free->prev)
        last_free = last_free->prev;

      last_free->prev = free_mem_rec;
      free_mem_rec = mem_id_vec[next_mem_id].last_rec;
    }
  }

  /* Replace all returns with jumps to labels at end. */
  for (insn = get_last_insn(); insn; insn=PREV_INSN(insn))
    if (GET_CODE(insn)==JUMP_INSN && GET_CODE(PATTERN(insn))==RETURN)
    { 
      rtx p = PREV_INSN(insn);
      if (p != 0 && GET_CODE(p)==INSN && GET_CODE(PATTERN(p))==USE)
      { /* Delete use at old return; remember for real return */
        if (ruse == 0)
          ruse=copy_rtx(PATTERN(p));

        PUT_CODE (p, NOTE);
        NOTE_LINE_NUMBER (p) = NOTE_INSN_DELETED;
        NOTE_SOURCE_FILE (p) = 0;
      }

      PATTERN(insn) =
        gen_rtx (SET, VOIDmode, pc_rtx,
          gen_rtx (LABEL_REF, VOIDmode, emit_label (gen_label_rtx())));
      INSN_CODE(insn) = -1;
    }

  /* Get past the stuff in the prologue ... */
  insn=f;
  do
  { if (go = (NEXT_INSN(insn) != 0))
    { if (GET_CODE(insn)==INSN)
        if (GET_CODE(PATTERN(insn))==SET)
          go = (REGOP(SET_DEST(PATTERN(insn))) && REGOP(SET_SRC(PATTERN(insn))));
        else
          go = (GET_CODE(PATTERN(insn))==USE ||GET_CODE(PATTERN(insn))==CLOBBER);
      else
        go = (GET_CODE(insn)==NOTE);

      if (go)
        insn = NEXT_INSN(insn);
    }
  }
  while (go);

  /* Surround the function with unused labels.  Now the function
     body will be strictly SESE.  We'll add a return later, then hope
     jump opt undoes all of this if it proves to not be needed. */

  emit_label_after (gen_label_rtx(), PREV_INSN(insn));
  start_label = PREV_INSN(insn);
  end_label   = emit_label (gen_label_rtx());

  /* Set up the first mem entry as the null pointer.  We use this
     for any mem too complicated to understand, or when we get
     more mems than we can number in a set. */

  next_mem_id = 1;
  mem_id_vec[0] = null_slot;

  /* Find highest UID */
  for (insn=f; insn; insn = NEXT_INSN (insn))
  { if (INSN_UID (insn) > max_uid)
      max_uid = INSN_UID (insn);
  }

  if (uid_info == 0)
    uid_info = (uid_data*) xmalloc ((max_uid+1) * sizeof (uid_info[0]));
  else
    if (max_uid > old_max_uid)
      uid_info = (uid_data*)
        xrealloc ((char*)uid_info, (max_uid+1) * sizeof (uid_info[0]));

  bzero ((char*)uid_info, ((max_uid+1) * sizeof (uid_info[0])));

  for (insn=f; insn; insn = NEXT_INSN (insn))
  { IID(insn) = next_iid;
    if (GET_CODE(insn)==CODE_LABEL)
      LABEL_REFS(insn)=insn;
    next_iid++;
  }

  FIRST(start_label)=end_label;
  LAST (start_label)=end_label;

  FIRST(end_label)=start_label;
  LAST (end_label)=start_label;

  /* Set up mem tables. */
  call_kills_addr = all_zeroes;

  for (cur_insn=NEXT_INSN(start_label); cur_insn != end_label; cur_insn=NEXT_INSN(cur_insn))
    if (REAL_INSN (cur_insn))
      examine_insn (&PATTERN (cur_insn), use_context);

  /* Get rid of unused labels. */
  for (insn=NEXT_INSN(start_label); insn != end_label; insn=NEXT_INSN(insn))
    if (GET_CODE(insn)==CODE_LABEL && LABEL_REFS(insn)==insn)
    { PUT_CODE (insn, NOTE);
      NOTE_LINE_NUMBER (insn) = NOTE_INSN_DELETED;
      NOTE_SOURCE_FILE (insn) = 0;
    }

  /* Initialize block boundary info. */
  find_sese (start_label, (rtx)0);

  if (dumpfile)
  { fprintf (dumpfile, "\nshadow info after examine_insn and find_sese\n\n");
    fflush (dumpfile);
    dump_insn_range (dumpfile, f, (rtx) 0);
    dump_mem_id_range (dumpfile, 0, next_mem_id-1);
    dump_reg_notes (dumpfile);
    fflush (dumpfile);
  }

  /* Propagate address info down into contained regions. */
  push_sese_context (start_label);

  if (dumpfile)
  { fprintf (dumpfile, "\nshadow info after push_sese_context\n\n");
    fflush (dumpfile);
    dump_insn_range (dumpfile, f, (rtx) 0);
    dump_mem_id_range (dumpfile, 0, next_mem_id-1);
    dump_reg_notes (dumpfile);
    fflush (dumpfile);
  }

  /* Shadow each mem_id in the function. */
  for (i = 1;  i < next_mem_id;  i++)
    ret_val += shadow ((unsigned)i, start_label);

  /* Put out use for return reg */
  if (ruse)
    emit_insn (ruse);

  /* Put return back on at the end */
#ifdef HAVE_return
  emit_jump_insn (gen_return());
  emit_barrier ();
#endif

  jump_optimize (f, 0, 0, 0);

  if (old_max_uid > max_uid) max_uid = old_max_uid;
  return ret_val;
}

static rtx
find_lval (x)
rtx x;
{
  if (x)
    while (GET_CODE(x)==SUBREG ||
           GET_CODE(x)==ZERO_EXTRACT ||
           GET_CODE(x)==SIGN_EXTRACT ||
           GET_CODE(x)==STRICT_LOW_PART)
      x = XEXP(x,0);
  return x;
}

static rtx
find_reg (x)
rtx x;
{
  if (x = find_lval (x))
    if (GET_CODE(x) != REG)
      x = 0;

  return x;
}

int
is_mem (x)
rtx x;
{
  return GET_CODE(find_lval(x)) == MEM;
}

int
is_reg (x)
rtx x;
{
  return GET_CODE(find_lval(x)) == REG;
}

#endif
