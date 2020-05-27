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
#include "gvarargs.h"

#ifdef GCC20
#include "insn-config.h"
#include "insn-flags.h"
#include "hard-reg-set.h"
#include "i_jmp_buf_str.h"
#include "basic-block.h"
#include "i_dataflow.h"
#else
#include "insn_cfg.h"
#include "insn_flg.h"
#include "hreg_set.h"
#include "jmp_buf.h"
#include "basicblk.h"
#include "dataflow.h"
#endif

#include "recog.h"

#define IDT_LOOP 256

#define df_assert(x) assert(x)

void
init_shadow_time()
{
  extern char* dataflow_time_name[];

  dataflow_time_name[IDT_LOOP]  = "possible_loop";
}

/*
 * Return the size of the next biggest object required to hold this size.
 */

static enum machine_mode ob_container[65];

void
init_ob_container()
{
  enum machine_mode mode;

  int size = 0;

  if (ob_container[0] == 0)
    for (mode = GET_CLASS_NARROWEST_MODE(MODE_INT); mode != VOIDmode;
         mode = GET_MODE_WIDER_MODE (mode))
      while (GET_MODE_SIZE(mode) >= size)
      { df_assert (size < sizeof(ob_container)/sizeof(ob_container[0]));
        ob_container[size++] = mode;
      }
}

#define object_container(s) ob_container[s]

new_df_subst (insn, user, off, attrs, pts_to, new_expr, list)
int insn;
rtx user;
int off;
unsigned short attrs;
ud_set pts_to;
rtx new_expr;
df_subst_queue* list;
{
  int n = list->num_subst++;
  int s = MAP_SIZE(list->subst, df_subst_rec_map);
  df_subst_rec* r;
  
  df_assert (n <= s);

  if (n == s)
    REALLOC_MAP (&list->subst, (n+1)*2, df_subst_rec_map);

  r = list->subst+n;

  r->insn = insn;
  r->user = user;
  r->off  = off;
  r->attrs = attrs;
  r->pts_to = pts_to;
  r->new_expr = new_expr;

  return n;
}

do_df_subst (list)
df_subst_queue* list;
{
  int n;
  df_subst_rec *r;

  n = list->num_subst;
  list->num_subst = 0;

  for (r=list->subst; r < list->subst+n; r++)
  { rtx* context;

    context = &NDX_RTX (r->user, r->off);

    inactivate_uds_in_tree (r->insn, context);
    *context = r->new_expr;
    INSN_CODE(UID_RTX(r->insn)) = -1;
    scan_rtx_for_uds (r->insn, r->user, r->off, r->attrs, r->pts_to);
    bzero (r, sizeof (*r));
  }
}

replace_ud (r, u, l, h, load, store, queue)
rtx r;
ud_rec* u;
int l;
enum machine_mode h;
int load;
int store;
df_subst_queue *queue;
{
  enum machine_mode m;
  rtx insn, prv, nxt, *udr, seq,var,sav,reg;
  int boff, bsiz, ali;
  unsigned tmp;

  insn = UID_RTX(u->insn);
  prv  = PREV_INSN(insn);
  nxt  = NEXT_INSN(insn);
  udr  = &UD_RTX(*u);
  var  = UD_VRTX(*u);
  m    = GET_MODE(*udr);
  ali  = RTX_BYTES(r);

  tmp  = (UD_BYTES(*u) << UD_VAR_OFFSET(*u));
  df_assert (tmp);

  boff = 0;
  while ((tmp & 1) == 0)
  { boff++; tmp >>= 1; }

  boff -= l;
  df_assert (boff >= 0);

  bsiz = 0;
  while (tmp)
  { bsiz++; tmp >>= 1; }

  boff <<= 3;
  bsiz <<= 3;

  trace (replace_ud_entry, prv, nxt);

  df_assert ((u->attrs & (S_INACTIVE)) == 0);
  df_assert (prv && nxt);

  START_SEQUENCE (sav);

  if (load || store)
  { var = copy_rtx (var);
    RTX_ID(var)=RTX_VAR_ID(var)=0;
    var = change_address (var, h, plus_constant(XEXP(var,0), l + -UD_VAR_OFFSET(*u)));
  }

  if (load)
    emit_move_insn (pun_rtx(r,h), var);

  /* generate code to load from 'r', or to store 'r' */

  df_assert ((u->attrs & S_DEF) == 0 || (u->attrs & S_USE) == 0);

  if (boff==0 && bsiz==GET_MODE_BITSIZE(m) && m==GET_MODE(r) && GET_CODE(*udr)==MEM)
    reg = r;
  else
  {
    reg = gen_typed_reg_rtx (UD_TYPE(*u), m);
    REG_USERVAR_P(reg) = 1;

    if (u->attrs & S_DEF)
    {
      if (boff==0 && bsiz==GET_MODE_BITSIZE(m) &&
          (bsiz >= GET_MODE_BITSIZE(SImode) || bsiz==GET_MODE_BITSIZE(GET_MODE(r))))
        emit_move_insn (pun_rtx(r,m),reg);
      else
        store_bit_field (r, bsiz, boff, m, reg, ali, 0);
    }
    else
    {
      if (boff==0 && bsiz==GET_MODE_BITSIZE(m))
        emit_move_insn (reg,pun_rtx(r,m));
      else
        reg = extract_bit_field (r, bsiz, boff, !RTX_IS_SEXTEND(*udr), reg, m, m, ali, 0);
    }
  }

  if (store)
    emit_move_insn (copy_rtx (var), pun_rtx(r,h));

  seq = gen_sequence();
  END_SEQUENCE (sav);

  new_df_subst (u->insn, u->user, u->off, u->attrs, u->pts_at, reg, queue);

  /* Put the access sequence before if read, after if write. */
  if (u->attrs & S_DEF)
  { emit_insn_after (seq, insn);
    scan_insns_for_uds (insn, nxt, BLOCK_NUM(insn));
  }
  else
  { emit_insn_before (seq, insn);
    scan_insns_for_uds (prv, insn, BLOCK_NUM(insn));
  }

  trace (replace_ud_exit, prv, nxt);
}
int df_shad_time;

static enum machine_mode
pick_mode (siz)
{
  enum machine_mode m;

  if (siz > DI_BYTES)
    m = TImode;
  else if (siz > SI_BYTES)
    m = DImode;
  else if (siz > HI_BYTES)
    m = SImode;
  else if (siz > QI_BYTES)
    m = HImode;
  else
    m = QImode;

  return m;
}

static void
biggest_move (m, psiz, poff)
unsigned m;
int *psiz;
int *poff;
{
  int siz, off;
  unsigned msk;

  *psiz = 0;
  *poff = 0;

  df_assert (m != 0 && m <= 0xffff);

  for (siz = 16; siz != 0; siz >>= 1)
  { msk = (1<<siz)-1;
    for (off = 0; off < 16; (off += siz),(msk <<= siz))
      if ((m & msk) == msk)
      {
        *psiz = siz;
        *poff = off;
         return;
      }
  }
  df_assert (0);
}

static int
count_moves (m)
unsigned m;
{
  int c, siz, off;

  c = 0;
  while (m)
  {
    biggest_move (m, &siz, &off);
    m &= ~ (((1 << siz)-1) << off);
    c++;
  }
  return c;
}

static
rtx find_sym_ref (x)
rtx x;
{
  enum rtx_code code;
  rtx v;
  
  if (x == 0 || (code=GET_CODE(x))==SYMBOL_REF)
    v = x;
  else
  { char *fmt = GET_RTX_FORMAT (code);
    int i;

    v = 0;

    for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
      if (fmt[i] == 'e')
        if (v = find_sym_ref (XEXP (x, i)))
	  break;
  }
  return v;
}

void emit_move_seq (reg, addr, var, voff, is_load, in_struct, blk, mask, before)
rtx reg;
rtx addr;
int var;
int voff;
int is_load;
int in_struct;
int blk;
unsigned mask;
rtx before;
{
  rtx pat,prv,m,insn,*context;
  int u,siz,off;
  enum machine_mode mode;
  ud_info* mem;
  tree type;

  df_assert (var > 0);

  mem = df_info+IDF_MEM;

  if (GET_CODE(reg)==SUBREG)
    type = RTX_TYPE(SUBREG_REG(reg));
  else
    type = RTX_TYPE(reg);

  mode = GET_MODE(reg);
  biggest_move (mask, &siz, &off);
  df_assert ((off % 4) == 0 && (siz & (siz-1)) == 0);

  if (siz != GET_MODE_SIZE(mode) || off != 0)
  {
    rtx treg = reg;
    unsigned tmask = mask;

    mask = ((1 << siz)-1) << off;
    df_assert (mask != 0 && (mask &~tmask) == 0);

    mode = pick_mode (siz);
    df_assert (GET_MODE_SIZE(mode) == siz);

    reg  = pun_rtx_offset (before, reg,  mode, off/4);

    m = gen_typed_mem_rtx (type,mode,addr);
    set_in_struct (m, in_struct);
    m = pun_rtx_offset (before, m, mode, off/4);

    if (tmask &= ~mask)
      emit_move_seq (treg, copy_rtx(addr), var, voff, is_load, in_struct, blk, tmask, before);

    voff += off;
    mask >>= off;
  }
  else
  { m = gen_typed_mem_rtx (type,mode,addr);
    set_in_struct (m, in_struct);
  }

  prv = PREV_INSN(before);

  if (is_load)
  { pat = gen_rtx (SET, mode, reg, m);
    context = &SET_SRC(pat);
  }
  else
  { pat = gen_rtx (SET, GET_MODE(reg), m, reg);
    context = &SET_DEST(pat);
  }

  insn = emit_insn_before (pat, before);

  scan_insns_for_uds (prv, before, blk);

  /*  We need to record the variable number and offset for
      this reference so that we can use mem_ud_range to set
      up mrd info for the scheduler.  Find the mem we just
      scanned (it should be nearly the last one added) */

  u = NUM_UD(mem);
  while (--u > 0 && &UDN_RTX(mem,u) != context)
    ;

  df_assert (u > 0 && UDN_IRTX(mem,u)==insn);

  RTX_VAR_ID(m)         = var;
  UDN_VARNO(mem,u)      = var;
  UDN_BYTES(mem,u)      = mask;
  UDN_VAR_OFFSET(mem,u) = voff;
}

rtx
check_addr_ok (addr, nuses, make_safe)
rtx addr;
int nuses;
int make_safe;
{
  rtx ret,r;

  /* For now, push it thru a reg.  This seems to generate better code. */
  nuses = 2;

  if (!address_operand(addr,TImode) ||
      (nuses > 1 && (GET_CODE(addr) != REG)) ||
      (make_safe && rtx_varies_p(addr)))
    ret = copy_addr_to_reg (addr);
  else
    ret = copy_rtx (addr);

  return ret;
}

rtx get_expr_seq (dst, src, off, nuses, make_safe)
rtx *dst;
rtx src;
int off;
{
  rtx sav, seq;

  START_SEQUENCE (sav);

  if (off)
    src = plus_constant (src, off);

  *dst = check_addr_ok (src, nuses, make_safe);

  seq = gen_sequence();
  END_SEQUENCE (sav);

  return seq;
}

static rtx
pick_ud_reg (users, siz, off, ref_mode, pin_struct)
ud_set users;
int siz;
int off;
enum machine_mode* ref_mode;
int *pin_struct;
{
  int u,in_struct;
  ud_info* df;
  tree type;
  enum machine_mode m;
  rtx ret_reg;

  df = df_info+IDF_MEM;

  /* Get the range of bit offsets the reg has to carry, and
     pick a type for the register. */

  type = 0;

  u = -1;
  m = VOIDmode;
  in_struct = 0;

  while ((u=next_flex(users,u)) != -1)
  {
    ud_rec* p = df->maps.ud_map + u;
    extern tree ptr_type_node,integer_type_node;
    rtx v = UD_VRTX(*p);
    
    if (m == VOIDmode)
      m = GET_MODE(v);
    else
      if (m != GET_MODE(v))
        m = BLKmode;

    if (type == 0)
    { if ((type = UD_TYPE(*p)) == 0)
        type = void_type_node;
    }
    else
      if (TYPE_UID(UD_TYPE(*p)) != TYPE_UID(type))
        type = void_type_node;

    /* Set in_struct to union of MEM_IN_STRUCT_P of all users */
    note_in_struct (v, &in_struct);
  }

  df_assert (type != 0 && siz > 0 && siz <= TI_BYTES && (siz & (siz-1)) == 0);

  /* If all modes are identical and the requested size is the
     same as the size of the mode, use that mode;  else, pick an integer mode
     based on the requested size. */

  if (GET_MODE_CLASS(m) == MODE_RANDOM || siz != GET_MODE_SIZE(m))
    m = pick_mode (siz);

  *ref_mode = m;
  ret_reg = gen_typed_reg_rtx (type, m);

  /*
   * Set this flag to indicate that this register is not a temporary whose
   * lifetime can't extend across a loop boundary.  The optimization code
   * in loop assumes that a compiler generated temporary cannot extend
   * across a loop.  This incorrect assumption was safe before we added
   * these more sophisticated optimizations like shadowing.
   *
   * We don't fix loop because loop doesn't have the necessary smarts
   * to do a good job without this information, so the fix in loop would
   * unduly affect code execution performance.
   * This problem reported in OMEGA #219.
   */
  REG_USERVAR_P(ret_reg) = 1;

  *pin_struct = in_struct;
  return ret_reg;
}

int
chk_dom (df, pn, qn, dom)
ud_info* df;
int pn;
int qn;
ud_set* dom;
{
  /* Return 1 if p dom q, -1 if q dom p, and 0 if neither. */

  ud_rec* p = df->maps.ud_map + pn;
  ud_rec* q = df->maps.ud_map + qn;

  rtx pi  = UD_INSN_RTX (*p);
  rtx qi  = UD_INSN_RTX (*q);

  int ret = 0;

  if (pi == qi)
    if (p >= q)
      ret = 1;
    else
      ret = -1;

  else
  { int pb = UD_BLOCK(*p);
    int qb = UD_BLOCK(*q);

    if (pb == qb)
      if (insn_comes_before (pi, qi))
        ret = 1;
      else
        ret = -1;

    else
    {
      if (in_flex (dom[qb], pb))
        ret = 1;
      else if (in_flex (dom[pb], qb))
        ret = -1;

      dom = 0;
    }
  }

  if (dom == df_data.maps.out_dominators)
    ret = -ret;

  return ret;
}

unsigned
get_kill_mask (s, head, tail)
ud_set s;
rtx head;
rtx tail;
{
  int u;
  unsigned mask;
  ud_info* df = df_info+IDF_MEM;

  /* First, take care of all kills which dominate the ends */
  u    = -1;
  mask = 0;

  while (mask != 0xffff && (u=next_flex(s,u)) != -1)
  { rtx insn = UDN_IRTX(df,u);

    if (UDN_ATTRS(df,u) & S_KILL)
      if (insn==head || insn==tail ||
          insn_dom (insn,head,1) || insn_dom (insn, tail,0))
        mask |= (UDN_BYTES(df,u) << UDN_VAR_OFFSET(df,u));
  }

  return mask;
}

/*
    s2 is a set of uds which refer to a single variable.

    Put the largest self-dominating subset of uds from s2 that
    we can find into s1, delete the new s2 from s1, and return the
    dominant ud for the new s1.

    For s1 to be "self-dominating" means that every def reaching
    an ud in s1 is either contained in s1 or blocked by an ud in
    s1.  Note that "blocked" does not mean "defined" (yet)
    because we can have uds in s1 or s2 which refer to disjoint
    parts of the variable.  In fact, we are hoping to see a lot of
    these cases, because that is how coalescion happens.

    After we discover s1, the caller will allocate the entire group
    to a single register, and the "blocks" will become definitions.
*/

unsigned overlap_mask (df, u, s)
ud_info* df;
int u;
ud_set s;
{
  unsigned m;

  m = (UDN_BYTES(df,u) << UDN_VAR_OFFSET(df,u));

  if (m)
  { unsigned n = 0;
    int t = -1;

    while (n != 0xffff && (t=next_flex(s,t)) != -1)
      if (df->maps.ud_map[u].varno != df->maps.ud_map[t].varno)
        n = 0xffff;
      else
        n |= (UDN_BYTES(df,t) << UDN_VAR_OFFSET(df,t));

    m &= n;
  }

  return m;
}

rtx*
chk_base_disp (x_addr, base)
rtx* x_addr;
rtx base;
{
  rtx* ret = 0;
  rtx x    = *x_addr;

  if (GET_CODE(x)==PLUS || GET_CODE(x)==MINUS)
  { if (GET_CODE(XEXP(x,1))==CONST_INT && GET_CODE(XEXP(x,0))==REG &&
        (base==0 || base==XEXP(x,0)))
    { int o = (GET_CODE(x)==PLUS) ? INTVAL(XEXP(x,1)) : -INTVAL(XEXP(x,1));
      if (o >= 0 && o < (1 << 12))
        ret = &XEXP(x,0);
    }
  }

  else if (GET_CODE(x) == REG)
    if (base==0 || base==x)
      ret = x_addr;

  return ret;
}

int 
insn_dom (i, t, kind)
rtx i;
rtx t;
{
  int ib = BLOCK_NUM(i);
  int tb = BLOCK_NUM(t);

  if (ib != tb)
    return in_flex (df_data.dom_relation[kind][tb], ib);
  else
    if (kind == 0)
      return insn_comes_before (i,t);
    else
      return insn_comes_before (t,i);
}

int
retrieve_ud (df, u, values, sizes, at_insns)
ud_info* df;
int u;
flex_set values;
int* sizes;
ud_set at_insns;
{
  int ret = -1;

  /* Return an ud from which we can get u's value,
     which is guaranteed to be executed before all
     of the insns in at_insns. */

  unsigned short atts = UDN_ATTRS (df,u);
  int idx = sizes[df-df_info]+u;
  ud_info* df_out = 0;

  if (in_flex (values, idx))
    return 0;

  set_flex_elt (values, idx);

  if ((atts & S_DEF)==0 || (atts & S_USE)==0)
  {
    ud_set equiv;
    int t;
    flex_pool* pool;
    pool = 0;
    new_flex_pool (&pool, NUM_UD(df), 1, F_DENSE);

    equiv = get_equiv (df, u, pool);

    t = -1;

    while (ret == -1 && (t=next_flex(equiv,t)) != -1)
    {
      unsigned short tatts = UDN_ATTRS (df, t);

      if (((tatts & S_DEF)==0 || (tatts & S_USE)==0))
      { int i = -1;

        if (tatts & S_USE)
          while ((i=next_flex(at_insns,i)) != -1 &&
                  insn_dom (UDN_IRTX(df,t), UID_RTX(i), 0));

        /* If i == -1, t dominates all of the insns we need. */
        if (i == -1)
        { 
          rtx t_insn = UDN_IRTX(df,t);
          rtx t_set  = PATTERN(t_insn);
          rtx t_val  = UDN_RTX(df,t);
          rtx t_var  = UDN_VRTX(df,t);

          if (GET_CODE(t_var)==MEM)
          { if (GET_CODE(t_set)==SET)
              if (t_val==SET_SRC(t_set))
                ret = lookup_ud (INSN_UID(t_insn), &SET_DEST(t_set), &df_out);
              else if (t_val==SET_DEST(t_set))
                ret = lookup_ud (INSN_UID(t_insn), &SET_SRC(t_set), &df_out);
          }
          else
          { ret = t;
            df_out = df;
          }
        }
      }
    }

    free_flex_pool (&pool);

    if (ret == -1)
      if (atts & S_DEF)
        return 0;
      else
      { ud_set reach = df->maps.defs_reaching[u];
  
        t = -1;
        while ((t = next_flex (reach, t)) != -1 &&
               check_equiv (df, df->maps.ud_map+t, df->maps.ud_map+u)  &&
               retrieve_ud (df, t, values, sizes, at_insns))
            ;
  
        clr_flex_elt (values, idx);
        return (t == -1);
      }

    else
    { clr_flex_elt (values, idx);
      set_flex_elt (values, sizes[df_out-df_info]+ret);
      return 1;
    }
  }

  return 0;
}

rtx get_lt_addr_seq (paddr, l, offset, at_in, nuses, make_safe)
rtx* paddr;
int l;
int offset;
int at_in;
{
  rtx seq,sav;
  rtx addr = 0;
  rtx cons = 0;
  int v = 0;
  rtx si_reg = 0;
  ud_info* df;
  
  ud_set at = 0;

  reuse_flex (&at, NUM_UID, 1, F_DENSE);
  set_flex_elt (at, at_in);

  if (df_data.maps.value_stack == 0)
  { int i = 10;

    ALLOC_MAP (&df_data.maps.value_stack, i, char_map);
    new_flex_pool (&df_data.pools.value_pool, LT_SIZE(df_data.lt_info), i, F_DENSE);

    while (i--)
      alloc_flex (df_data.pools.value_pool);
  }

  /* Pedal the LT's for l and generate rtx as follows:

       - Once to determine that every value is retrievable, and to
         figure out where to place the insns to retrieve the value,
         and which uds to actually get them from;

       - Once again to actually place the insns to retrieve the values.

       After that, generate a sequence to sum the retrieved values
       which the caller can place at 'at'. */

  for (df=df_info; df != df_info+INUM_DFK; df++)
  { int j;
    int k = df-df_info;
    signed_byte* lt_ptr = LT_VECT (df_data.lt_info, l, k);
    int n_ud = df_data.lt_info.lt_len[k];

    for (j = 0; j < n_ud; j++)
      if (lt_ptr[j])
      { int m; ud_set s;

        m = unmap_lt (&(df_data.lt_info), df,j);
        s = get_flex (df_data.pools.value_pool, ++v);
        clr_flex (s);

        if (retrieve_ud (df, m, s, df_data.lt_info.n_lt, at))
        { df_data.maps.value_stack[v-1] = lt_ptr[j];

          if (MAP_SIZE(df_data.maps.value_stack, char_map) == v)
          { int i = v;
            REALLOC_MAP (&df_data.maps.value_stack, i+i, char_map);
            while (i--)
              alloc_flex (df_data.pools.value_pool);
          }
        }
        else
          return (reuse_flex (&at,0,0,0), (seq=0));
      }
  }

  while (v)
  { ud_set s;
    int n, u;
    rtx r;

    s = get_flex (df_data.pools.value_pool, v--);
    n = df_data.maps.value_stack[v];
    u = next_flex (s, -1);
    r = 0;

    df_assert (u != -1);

    while (u != -1)
    { int j, w;
      ud_rec *p;
      rtx i, src, set;

      df = df_info;
      while (u >= df_data.lt_info.n_lt[(df-df_info)+1])
      { df++;
        df_assert (df != df_info+INUM_DFK);
      }

      j = u - df_data.lt_info.n_lt[df-df_info];
      p = df->maps.ud_map + j;
      i = UD_INSN_RTX(*p);
      w = next_flex (s, u);

      src = UD_RTX(*p);

      /* If we haven't assigned rtx for this value of v yet, and if
         there is only 1 place the value has to be retrieved from,
         and if the value is constant, we don't need to use a
         register. */
        
      if (r==0 && w == -1 && n==1 && cons==0 && CONSTANT_P(src))
        cons = r = copy_rtx (src);

      else
      { 
        if (r == 0)
        { r = gen_reg_rtx (Pmode);
          REG_USERVAR_P(r) = 1;

          if (addr)
            addr = gen_rtx (PLUS, Pmode, addr, r);
          else
            addr = r;
        }

        assert (GET_CODE(r)==REG);

        if (CONSTANT_P(src) ||
            GET_MODE_SIZE(GET_MODE(src)) == GET_MODE_SIZE(Pmode))
          set = gen_rtx (SET, Pmode, r, src);

        else
        {
          START_SEQUENCE(sav);
          df_assert (GET_MODE_SIZE(GET_MODE(src)) == GET_MODE_SIZE(SImode));

          if (si_reg == 0)
            si_reg = gen_reg_rtx (SImode);

          emit_move_insn(si_reg, src);
          emit_move_insn(r, gen_rtx(SIGN_EXTEND, Pmode, si_reg));
          set = gen_sequence ();
          END_SEQUENCE(sav);
        }

        if (UD_ATTRS(*p) & S_USE)
          set = emit_insn_before (set, i);
        else
          set = emit_insn_after (set, i);

        scan_insns_for_uds (PREV_INSN(set), NEXT_INSN(set), BLOCK_NUM(i));

        if (n!=1)
        { i = set;
          if (n <= 0 || (n & (n-1)))
            set = gen_rtx (MULT,Pmode,r,gen_rtx(CONST_INT,VOIDmode,n));
          else
          { int t = 1;
            while ((1<<t) != n)
              t++;
            set = gen_rtx(ASHIFT,Pmode,r,gen_rtx(CONST_INT,VOIDmode,t));
          }

          set = gen_rtx (SET,  Pmode,r,set);
          set = emit_insn_after (set, i);
          scan_insns_for_uds (PREV_INSN(set), NEXT_INSN(set), BLOCK_NUM(i));
        }
      }
      u = w;
    }
  }

  START_SEQUENCE (sav);

  /* If we have a constant already, fold offset into it. */
  if (cons && offset)
  { cons = plus_constant (cons, offset);
    offset = 0;
  }
  
  if (cons && addr)
    addr = gen_rtx (PLUS, Pmode, addr, cons);

  else
    if (cons)
      addr = cons;

  if (addr == 0)
    addr = gen_rtx (CONST_INT, VOIDmode, offset);
  else
    if (offset)
      addr = plus_constant (addr, offset);

  *paddr = check_addr_ok (addr, nuses, make_safe);
  seq = gen_sequence();
  END_SEQUENCE (sav);

  return (reuse_flex (&at, 0, 0, 0), seq);
}

/* If u's address isn't safe over [h..t], return 0. */
   
int 
constant_address (u, h, t)
int u;
int h;
int t;
{
  rtx head, tail;
  ud_info* df, *mem;
  int l;
  rtx ui;

  if (h > 0 && t > 0)
    return 1;

  head = DF_DOMI(h);
  tail = DF_DOMI(t);
  mem  = df_info+IDF_MEM;
  l    = mem_slots[mem->maps.ud_map[u].varno].terms;
  ui   = UDN_IRTX(mem,u);

  for (df=df_info; df != df_info+INUM_DFK; df++)
  { int j;
    int k = df-df_info;
    signed_byte* lt_ptr = LT_VECT (df_data.lt_info, l, k);
    int n_ud = df_data.lt_info.lt_len[k];

    for (j = 0; j < n_ud; j++)
      if (lt_ptr[j])
      { int m = unmap_lt (&(df_data.lt_info),df,j);
        if (UDN_ATTRS(df,m) & S_DEF)
        { rtx ji = UDN_IRTX(df,m);

          if ((h<=0 && ji != head && !b_hides_a_from_c (ji, head, ui)) ||
              (t<=0 && ji != tail && !b_hides_a_from_c (ui, tail, ji)))
            return 0;
        }
        else
        { int d = -1;
          while ((d=next_flex(df->maps.defs_reaching[m],d)) != -1)
          { rtx di = UDN_IRTX(df,d);

            if ((h<=0 && di != head && !b_hides_a_from_c (di, head, ui)) ||
                (t<=0 && di != tail && !b_hides_a_from_c (ui, tail, di)))
              return 0;
          }
        }
      }
  }
  return 1;
}

int 
pure_constant_address (u)
int u;
{ /* Return 1 iff address is a true constant. */

  ud_info* df, *mem;
  int l;
  rtx ui;

  mem  = df_info+IDF_MEM;
  l    = mem_slots[mem->maps.ud_map[u].varno].terms;
  ui   = UDN_IRTX(mem,u);

  for (df=df_info; df != df_info+INUM_DFK; df++)
    if (df != df_info+IDF_SYM)
    { int j,k,n_ud;
      signed_byte* lt_ptr;

      k      = df-df_info;
      lt_ptr = LT_VECT (df_data.lt_info, l, k);
      n_ud   = df_data.lt_info.lt_len[k];

      for (j = 0; j < n_ud; j++)
        if (lt_ptr[j])
          return 0;
    }
  return 1;
}

rtx
dom_insn (blk,kind)
int blk;
int kind;
{
  rtx insn, tmp;

  df_assert (blk >= 0 && blk < N_BLOCKS);

  if (kind == 0)
  { /* blk is an in_dominator block.  Want insn at end of basic block,
       after which a load can be placed. */

    insn = BLOCK_END(blk);

    if (GET_CODE(insn)==CODE_LABEL)
      df_assert (BLOCK_HEAD(blk)==insn);

    if (GET_CODE(insn)==JUMP_INSN &&
        (GET_CODE(PATTERN(insn))==ADDR_DIFF_VEC||
         GET_CODE(PATTERN(insn))==ADDR_VEC))
    { /* Case table.  Find the indirect jump that uses it, and place the
         load before it. */

      /* First, pedal back to the head of the block containing the table */
      insn = PREV_INSN(insn);
      while (insn && GET_CODE(insn) != CODE_LABEL)
        insn = PREV_INSN(insn);
      df_assert (insn && BLOCK_NUM(insn)==blk);

      /* Hopefully, won't see more than 1 jump on the ref chain.  Note that
         we will still have to get thru a dominance df_assert (indirect jump 
         must dominate it's table) at the of this routine. */

      tmp = LABEL_REFS(insn);
      df_assert (tmp != 0 && LABEL_NEXTREF(tmp) == insn);

      insn = CONTAINING_INSN(tmp);
      df_assert (GET_CODE(insn)==JUMP_INSN);
    }
     
    while (DF_CC0_INSN(insn) || GET_CODE(insn)==JUMP_INSN)
      insn = PREV_INSN(insn);

    for (tmp = insn; tmp && GET_CODE(tmp)==NOTE; tmp=PREV_INSN(tmp))
      if (NOTE_LINE_NUMBER(tmp)==NOTE_INSN_LOOP_BEG)
        insn = PREV_INSN(tmp);
  }
  else
  { /* blk is an out_dominator block.  Want insn at head of block,
       before which a store can be placed. */

    insn = BLOCK_HEAD(blk);

    if (GET_CODE(insn)==CODE_LABEL)
      insn = NEXT_INSN(insn);

    for (tmp=insn; tmp && GET_CODE(tmp)==NOTE; tmp=NEXT_INSN(tmp))
      if (NOTE_LINE_NUMBER(tmp)==NOTE_INSN_LOOP_END)
        insn = NEXT_INSN(tmp);

    if (insn && GET_CODE(insn)==JUMP_INSN)
    { rtx p = PATTERN(insn);
      /* Since we are looking for out dominator (store placement),
         we should find the indirect jump before we see the table.
         Can't put a store here;  have to back up to before the
         indirect jump. */

      if (GET_CODE(p)==ADDR_DIFF_VEC || GET_CODE(p)==ADDR_VEC)
        return 0;
    }
  }

  df_assert (in_flex (df_data.dom_relation[kind][blk], BLOCK_NUM(insn)));
  return insn;
}

int
partition_ok (df, s, head, tail)
ud_info* df;
ud_set s;
int head;
int tail;
{
  int u;
  rtx headi, taili;

  headi = (head >= 0) ? UDN_IRTX(df,head) : UID_RTX(-head);
  taili = (tail >= 0) ? UDN_IRTX(df,tail) : UID_RTX(-tail);

  u = -1;
  while ((u = next_flex (s,u)) != -1)
  { rtx ui = UDN_IRTX(df,u);

    if (ui != taili)
      if (UDN_ATTRS(df,u) & S_DEF)
      { int r = -1;

        /* If path from this def to head without going thru tail, load at head
           could destroy value. */

        if (ui!=headi && insn_path_from_a_to_c_not_b (ui,taili,headi))
          return 0;

        while ((r = next_flex (df->maps.uses_reached[u],r)) != -1)
          if (UDN_ATTRS(df,r) & S_USE)
            if (in_flex (s, r))
              ;
            else
              if (UDN_IRTX(df,r)!=taili && !b_hides_a_from_c(ui,taili,UDN_IRTX(df,r)))
                return 0;
      }
  
    if (ui != headi)
      if (UDN_ATTRS(df,u) & S_USE)
      { int r = -1;

        while ((r = next_flex (df->maps.defs_reaching[u],r)) != -1)
          if (UDN_ATTRS(df,r) & S_DEF)
            if (in_flex (s, r))
              ;
            else
              if (UDN_IRTX(df,r)!=headi && !b_hides_a_from_c(UDN_IRTX(df,r),headi,ui))
                return 0;
      }
  }

  return 1;
}

/* Return the set of all offsets from the base of u's variable
   at which a register containing u could be located.  The intent
   is to avoid both illegal and uncomfortable references to u. */

unsigned get_var_bases (u, do_coalesce)
ud_rec* u;
int do_coalesce;
{
  unsigned ret;
  rtx r;
  int i,uvo, uvs;

  ret = 0;
  r   = UD_VRTX (*u);

#ifdef ALLOW_BYTE_COALESCE
  if (TARGET_H_SERIES)
    uvs = RTX_BYTES(r);
  else
#endif
    uvs = MAX (RTX_BYTES(r), UNITS_PER_WORD);

  uvo = UD_VAR_OFFSET(*u);

  /* Work down from current offset in multiples of
     reference size;  for char and short variables, consider
     only multiples of 4 bytes, to avoid awkward field references.

     This is the basic set of legal offsets
     for the bases of registers which contain u. */

  if (do_coalesce)
  {
    i = uvo;
    while (i >= 0)
    { ret |= (1 << i);
      i -= uvs;
    }
  }
  else
    ret = (1 << uvo);

  return ret;
}

int check_mem_align (size, insn, r, offset)
int size;
int insn;
rtx r;
int offset;
{
  int ok,ali,need;

  /*
   * size is in bytes, compute the maximum alignment possibly needed
   * for an object of this size on the target architecture.
   */

  need = GET_MODE_ALIGNMENT(object_container(size))/BITS_PER_UNIT;
  ali  = compute_mem_align (insn, r, offset);

  ok = !((need - 1) & ali);

  return ok;
}

int should_coalesce (u, msk, off, siz, var_bases, modes, do_coalesce)
ud_rec* u;
unsigned *msk;
int* off;
int* siz;
int* var_bases;
unsigned* modes;
{
  /*  off, siz represent the lowest offset and longest extent
      of a partition of memory references to u's variable.

      If we can put u in that group and have the memory
      cell which is the coalescion of u with the extant mems
      still be a legal memory reference, adjust off and siz
      appropriately and return true;  else, return false.

      Note that it is OK to extend past either end, if that has to
      be done.  We'll fill the register with the extra when
      we load the cell, so that when we write it back, the extra
      bytes are correct.
  */

  int ok, bases;
  unsigned m;

  rtx ur = UD_VRTX(*u);

  m  = 1 << (int)GET_MODE(ur);
  ok = (bases = *var_bases & get_var_bases (u, do_coalesce));

  if (ok)
  { enum rtx_code c = GET_CODE(UD_RTX(*u));

#if 0
    if (ok = (c != ZERO_EXTEND && c != SIGN_EXTEND))
#endif
    {
      int ui, uvo, uvs,uoff,usiz,voff,vsiz;
      ui = UD_INSN_UID(*u);
      uvo = UD_VAR_OFFSET(*u);
      uvs = RTX_BYTES(ur);
    
      voff = *off;
      vsiz = *siz;
    
      /* Set voff,vsiz to denote the extent of the extant mems plus the
         new one.
    
         If there is overlap, or if there isn't but coalescion was
         requested, see if it is possible to coalesce the mem with the
         others.
      */
    
#ifdef ALLOW_BYTE_COALESCE
      if (ok = (do_coalesce && (uvs >= SI_BYTES || (TARGET_H_SERIES))))
#else
      if (ok = (do_coalesce && (uvs >= SI_BYTES)))
#endif
        if (uvo < voff)
        { vsiz = MAX ((uvo+uvs), (voff+vsiz)) - uvo;
          voff = uvo;
        }
        else
          vsiz = MAX ((uvo+uvs), (voff+vsiz)) - voff;
      else
        ok = (uvo == voff) && (uvs == vsiz);
  
      if (ok)
      {
        /* Pedal offset down until a vsiz reference would be ok. */
        while (voff>= 0 && (!(bases & (1<<voff)) ||
                            !check_mem_align(vsiz,ui,ur,voff-uvo)))
          (voff--,vsiz++);
    
        if (ok = (voff >= 0))
        { *siz = vsiz;
          *off = voff;
          *msk |= UD_BYTES(*u) << uvo;
          *var_bases = bases;
          *modes |= m;
        }
      }
    }
  }

  return ok;
}

/* Return "Can we get from a back to a without going thru b ?" */
possible_loop(an, bn)
int an;
int bn;
{
  ud_set a_member_of, b_member_of;
  int cn;

  int prev_time = assign_run_time (IDT_LOOP);

  /* If a isn't in a loop at all, can't get from a to a. */
  if (df_data.maps.loop_info[an].inner_loop == -1)
    return assign_run_time (prev_time),0;

  /* If a is a member of any loop that b is not a member of,
     we can clearly get from a back to a without going thru b. */

  if (df_data.maps.loop_info[bn].inner_loop == -1)
    return assign_run_time (prev_time),1;

  a_member_of = df_data.maps.loop_info[an].member_of;
  b_member_of = df_data.maps.loop_info[bn].member_of;

  if (!is_empty_flex (and_compl_flex (0, a_member_of, b_member_of)))
    return assign_run_time (prev_time),1;

  assign_run_time (IDT_LOOP+1);

  /* b is on a's loop or on a loop nested inside a's loop. */
  for (cn=next_flex(df_data.maps.succs[an],-1); cn!= -1;
       cn=next_flex(df_data.maps.succs[an],cn))
    if (path_from_a_to_c_not_b (cn,bn,an))
      return assign_run_time (prev_time), 1;

  return assign_run_time (prev_time),0;
}

int
next_closest_dom (b1, b2, kind)
int b1,b2;
int kind;
{
  int i;
  bb_set all_doms = 0;

  bb_set* dom = df_data.dom_relation[kind];

  if (BORDER(b1)>BORDER(b2))
  { int t = b1;
    b1 = b2;
    b2 = t;
  }

  reuse_flex (&all_doms, N_BLOCKS, 1, F_DENSE);

  and_flex (all_doms, dom[b1], dom[b2]);

  clr_flex_elt (all_doms, b1);
  clr_flex_elt (all_doms, b2);

  if (kind == 0)
    for (i=BORDER(b1)-1; i >= 0 && !in_flex(all_doms,BUNORDER(i)); i--)
      ;
  else
    for (i=BORDER(b2)+1; i < N_BLOCKS && !in_flex(all_doms,BUNORDER(i)); i++)
      ;

  reuse_flex (&all_doms, 0,0,0);
  df_assert (i != -1);
  return BUNORDER(i);
}

get_region (head, tail)
int* head;
int* tail;
{
  if (*head != *tail)
  { 
    rtx headi = DF_DOMI(*head);
    rtx taili = DF_DOMI(*tail);
    int hblk  = BLOCK_NUM(headi);
    int tblk  = BLOCK_NUM(taili);
    int ohblk = hblk;
    int otblk = tblk;

#if 0
    struct
    { short hblk;
      short tblk;
    } region_stack [DF_MAX_NBLOCKS], *pstk = region_stack;
#endif

    for (;;)
    { int i;

#if 0
      if (i = REGION_MAP(hblk, tblk))
      { headi = dom_insn(BLOCK_NUM(UID_RTX(i)), 0);
        taili = dom_insn(BLOCK_NUM(UID_RTX(REGION_MAP(tblk, hblk))), 1);
        hblk  = BLOCK_NUM(headi);
        tblk  = BLOCK_NUM(taili);
        break;
      }

      df_assert (BORDER(hblk) <= BORDER(tblk));
      df_assert (pstk-region_stack <sizeof(region_stack)/sizeof(region_stack[0]));

      /* This pair will need to be marked when we find the bounds ... */
      pstk->hblk = hblk;
      pstk->tblk = tblk;
      pstk++;
#endif

      /* While we can get from head back to head without going thru tail,
         back up to the closest in-dominator of both head & tail; and

         while we can get from tail back to tail without going thru head,
         go to the closest out-dominator of both head & tail. */

      if (!in_flex(df_data.maps.dominators[tblk],hblk))
        do
        { df_assert (hblk != ENTRY_BLOCK);
          hblk = BUNORDER (BORDER(hblk)-1);
        }
        while (!in_flex(df_data.maps.dominators[tblk],hblk));

      else if (!in_flex (df_data.maps.out_dominators[hblk], tblk))
        do
        { df_assert (tblk != EXIT_BLOCK);
          tblk = BUNORDER (BORDER(tblk)+1);
        }
        while (!in_flex (df_data.maps.out_dominators[hblk], tblk));

      else if (hblk != tblk && possible_loop(hblk, tblk))
      { df_assert (hblk != ENTRY_BLOCK);
        hblk = BUNORDER (BORDER(hblk)-1);
      }

      else if (hblk != tblk && possible_loop(tblk, hblk))
      { df_assert (tblk != EXIT_BLOCK);
        tblk = BUNORDER (BORDER(tblk)+1);
      }

      else if (!(headi=dom_insn(hblk,0)))
      { df_assert (hblk != ENTRY_BLOCK);
        hblk = BUNORDER (BORDER(hblk)-1);
      }

      else if (!(taili=dom_insn(tblk,1)))
      { df_assert (tblk != EXIT_BLOCK);
        tblk = BUNORDER (BORDER(tblk)+1);
      }
      else
        break;
    }

#if 0
    while (pstk-- != region_stack)
    { REGION_MAP(pstk->hblk,pstk->tblk) = INSN_UID(headi);
      REGION_MAP(pstk->tblk,pstk->hblk) = INSN_UID(taili);
    }
#endif

    if (ohblk != hblk)
      *head = -INSN_UID(headi);

    if (otblk != tblk)
      *tail = -INSN_UID(taili);
  }
}

int
get_partition_masks (s1, headp, tailp, msk, siz, off, ld_msk, st_msk, n_ud)
ud_set s1;
int *headp;
int* tailp;
unsigned msk;
int siz;
int off;
int* ld_msk;
int* st_msk;
int n_ud;
{
  int u,i,hblk,tblk,lblk;
  unsigned lmask,smask;
  rtx headi, taili;
  ud_info* mem;

  mem = df_info+IDF_MEM;

  headi = DF_DOMI(*headp);
  taili = DF_DOMI(*tailp);

  calc_masks (s1, headi, taili, msk, siz, off, &lmask, &smask);

  u = next_flex (s1,-1);
  i = mem->maps.ud_map [u].varno;

  /*  If the store at the end of the region will overwrite memory
      which is written by code executing inside the region but which
      is not being shadowed (not in s1), we have to give up on the
      partition.

      Note that reached uses of the defs in s1 is not sufficient;
      we care about what the final store reaches.  For one thing,
      there are sometimes bits in the store that are never used in s1. */

  if (smask != 0 && headi != taili)
  { int u;

    for (u = 1; u < n_ud; u++)
      if (!in_flex (s1,u) && (UDN_ATTRS(mem,u)&S_DEF) != 0)
      { rtx ui = UDN_IRTX(mem,u);

        if (ui != headi && ui != taili)
        { int uv = mem->maps.ud_map[u].varno;

          if (chk_mem(uv,UDN_BYTES(mem,u),UDN_VAR_OFFSET(mem,u),i,smask,0,df_overlap))
            if (insn_path_from_a_to_c_not_b (headi, taili, ui))
              return 0;
        }
      }

    /* If any loads or stores have appeared in the partition because of
       shadowing, and if they overlap our memory, give up on the partition */

    for (u = n_ud; u < NUM_UD(mem); u++)
    { rtx ui = UDN_IRTX(mem,u);

      if (ui != headi && ui != taili)
      { int uv = mem->maps.ud_map[u].varno;

        if(chk_mem(uv,UDN_BYTES(mem,u),UDN_VAR_OFFSET(mem,u),i,smask,0,df_overlap))
          if (insn_path_from_a_to_c_not_b (headi, taili, ui))
            return 0;
      }
    }
  }

  *ld_msk = lmask;
  *st_msk = smask;

  hblk = BLOCK_NUM(headi);
  tblk = BLOCK_NUM(taili);
  lblk = df_data.maps.loop_info[hblk].inner_loop;

  /* Try hoisting loads/stores out of loops */

  if (lblk != -1)
  {
    int head, tail, ok;
    rtx tblki,hblki;

    lblk = df_data.maps.loop_info[lblk].parent;

    while (df_data.maps.loop_info[hblk].inner_loop != lblk)
      if (hblk==ENTRY_BLOCK)
        return 1;
      else
        hblk = BUNORDER(BORDER(hblk)-1);

    while (tblk!=ENTRY_BLOCK && df_data.maps.loop_info[tblk].inner_loop != lblk)
      if (tblk==EXIT_BLOCK)
        return 1;
      else
        tblk = BUNORDER(BORDER(tblk)+1);

    while ((hblki=dom_insn(hblk,0))==0 && hblk != ENTRY_BLOCK)
      hblk = BUNORDER(BORDER(hblk)-1);

    while ((tblki=dom_insn(tblk,1))==0 && tblk != EXIT_BLOCK)
      tblk = BUNORDER(BORDER(tblk)+1);

    df_assert (hblki && tblki);

    head = -INSN_UID(hblki);
    tail = -INSN_UID(tblki);

    get_region (&head, &tail);

    hblk = BLOCK_NUM(DF_DOMI(head));
    tblk = BLOCK_NUM(DF_DOMI(tail));

    if ((ok = pure_constant_address (u)) == 0)
    {
      /* Have to find a guaranteed utterance of the address. */
      register int t;

      for (t = mem->maps.id_map[i].uds_used_by; t != -1;
           t = mem->uds_used_by_pool[t])
      { int tb = UDN_BLOCK(mem,t);
        if (in_flex (df_data.maps.dominators[hblk],tb) ||
            in_flex (df_data.maps.out_dominators[tblk], tb))
          break;
      }

      ok = (t != -1 && constant_address (u, head, tail));
    }

    if (ok)
      if (hblk != ENTRY_BLOCK || tblk != EXIT_BLOCK)
        if (partition_ok (mem, s1, head, tail))
          /* Try it again one level out ? */
          if(get_partition_masks(s1,&head,&tail,msk,siz,off,&lmask,&smask,n_ud))
          { 
            *headp = head;
            *tailp = tail;
            *ld_msk = lmask;
            *st_msk = smask;
          }
  }

  return 1;
}

calc_masks (s1, headi, taili, msk, siz, off, ld_msk, st_msk)
ud_set s1;
rtx headi;
rtx taili;
unsigned msk;
int siz;
int off;
unsigned* ld_msk;
unsigned* st_msk;
{
  ud_info* mem = df_info+IDF_MEM;

  int i,u;
  unsigned kmsk,lmsk,smsk;

  lmsk = 0;
  smsk = 0;

  df_assert (siz+off <= 16);
  kmsk = get_kill_mask (s1, headi, taili);

  /* msk represents the bytes read or written in the partition.
     Look around and see what needs to be loaded and/or stored
     at the edges. */

  u = -1;
  while ((u = next_flex(s1,u)) != -1)
  { ud_set t1;

    if (UDN_ATTRS(mem,u) & S_USE)
    { int t = -1;
      while ((msk & ~lmsk) && (t=next_flex(mem->maps.defs_reaching[u],t))!= -1)
        if (UDN_ATTRS(mem,t) & S_DEF)
          if (!in_flex (s1,t) || (UDN_IRTX(mem,t)!=headi && !b_hides_a_from_c (headi, UDN_IRTX(mem,t), UDN_IRTX(mem,u))))
            if (mem->maps.ud_map[t].varno != mem->maps.ud_map[u].varno)
              lmsk |=  (UDN_BYTES(mem,u) << UDN_VAR_OFFSET(mem,u));
            else
              lmsk |= ((UDN_BYTES(mem,u) << UDN_VAR_OFFSET(mem,u)) &
                       (UDN_BYTES(mem,t) << UDN_VAR_OFFSET(mem,t)));
    }

    if (UDN_ATTRS(mem,u) & S_DEF)
    { int t = -1;
      while ((msk && ~smsk) && (t=next_flex(mem->maps.uses_reached[u],t))!= -1)
        if (UDN_ATTRS(mem,t) & S_USE)
          if (!in_flex (s1,t) || (UDN_IRTX(mem,u)!=headi && !b_hides_a_from_c (headi, UDN_IRTX(mem,u), UDN_IRTX(mem,t))))
            if (mem->maps.ud_map[t].varno != mem->maps.ud_map[u].varno)
              smsk |= (UDN_BYTES(mem,u) << UDN_VAR_OFFSET(mem,u));
            else
              smsk |= ((UDN_BYTES(mem,u) << UDN_VAR_OFFSET(mem,u)) &
                       (UDN_BYTES(mem,t) << UDN_VAR_OFFSET(mem,t)));
    }

    /* If we store, we have to initialise all bits which the load will
       catch which don't get killed in the region. */

    lmsk |= (smsk & ~kmsk);
  }

  df_assert (((lmsk >> off)<<off) == lmsk);
  df_assert (((smsk >> off)<<off) == smsk);
  df_assert (((kmsk    >> off)<<off) == kmsk);

  lmsk >>= off;
  smsk >>= off;
  kmsk >>= off;

#if 0
  /* Use of pragma pack, and routine use of no-strict-align, will trigger
     this df_assert.  So we had to turn it off. */
  df_assert ((lmsk|smsk|kmsk) <= 15 || (off%4)==0);
#endif

  /* Fix up those cases where we can't store part of a reg without
     shifting (by enlarging the store and the load). */

  for (i = 0; i < TI_BYTES; i+=4)
  { unsigned tmsk;

    /* Store lo byte, lo short, or full 32 bits ... */
    if ((tmsk = (smsk >> i) & 0xf) > 1)
      if (tmsk <= 3) tmsk = 3;
      else           tmsk = 15;

    smsk |= (tmsk << i);
    lmsk |= (smsk & ~kmsk);
  }

  lmsk <<= off;
  smsk <<= off;
  kmsk <<= off;

  /* If already loading anyway, or if store would take more
     than 2 moves, do store in 1 move if possible by loading the
     extra bits.  The worst case is a load and a store, unless
     something is funny. */

  if (lmsk != 0 || count_moves(smsk) > 2)
  { unsigned tmsk = smsk;
    tmsk = (((tmsk & 0xf000) != 0) << 3) |
           (((tmsk & 0x0f00) != 0) << 2) |
           (((tmsk & 0x00f0) != 0) << 1) |
           (((tmsk & 0x000f) != 0) << 0);
  
    /* If multi-word... */
    if (tmsk & (tmsk-1))
      if (tmsk == 0x3 && off == 0)
        smsk = 0x00FF;
      else if (tmsk == 0xC && off <= 8)
        smsk = 0xFF00;
      else if (off == 0 && tmsk > 0x3)
        smsk = 0xFFFF;
  
    lmsk |= (smsk & ~kmsk);
  }

  /* Try not to issue more than a single load. */
  { unsigned tmsk;

    lmsk >>= off;
    for (i = 0; i < TI_BYTES; i+=4)
    { /* Load lo byte, lo short, or full 32 bits ... */
      if ((tmsk = (lmsk >> i) & 0xf) > 1)
        if (tmsk <= 3) tmsk = 3;
        else           tmsk = 15;
  
      lmsk |= (tmsk << i);
    }
    lmsk <<= off;

    tmsk = lmsk;
    tmsk = (((tmsk & 0xf000) != 0) << 3) |
           (((tmsk & 0x0f00) != 0) << 2) |
           (((tmsk & 0x00f0) != 0) << 1) |
           (((tmsk & 0x000f) != 0) << 0);
  
    /* If multi-word... */
    if (tmsk & (tmsk-1))
      if (tmsk == 0x3 && off == 0)
        lmsk = 0x00FF;
      else if (tmsk == 0xC && off <= 8)
        lmsk = 0xFF00;
      else if (off == 0 && tmsk > 0x3)
        lmsk = 0xFFFF;
  }
  *ld_msk = lmsk;
  *st_msk = smsk;
}

static void
get_range (df, s, ph, pt)
ud_info* df;
ud_set s;
int *ph;
int *pt;
{
  int h  = next_flex (s, -1);
  int t  = h;

  if (h != -1)
  { 
    int hi, ti, u;

    if (df_data.iid_invalid)
      insn_comes_before (ENTRY_LABEL, ENTRY_LABEL);

    hi = IID(UDN_IRTX(df,h));
    ti = hi;
    u  = t;

    while ((u=next_flex(s,u)) != -1)
    { int ui = IID(UDN_IRTX(df,u));

      if (ui < hi || (ui==hi && u < h))
      { h = u;
        hi = ui;
      }

      else if (ui > ti || (ui==ti && u > t))
      { t = u;
        ti = ui;
      }
    }
  }
  *ph = h;
  *pt = t;
}

find_mem_partition (s1, s2, ld_msk, st_msk, sizp, offp, headp, tailp, n_ud, do_coalesce)
ud_set s1;
ud_set s2;
unsigned* ld_msk;
unsigned* st_msk;
int* sizp;
int* offp;
int* headp;
int* tailp;
{
  int n,u,ok, msk,siz,off,bases,modes;

  ud_set ns1 = 0;
  ud_set blks = 0;
  ud_set nblks = 0;
  ud_set start = 0;
  ud_info* mem;
  rtx ud_rec_ptr_tmp;

  mem = df_info+IDF_MEM;

  *ld_msk = 0;
  *st_msk = 0;
  *offp   = 0;
  *sizp   = 0;

  clr_flex (s1);
  u = next_flex (s2, -1);

  *headp = *tailp = u;

  if (u == -1)
    return 0;

  if (df_data.iid_invalid)
    insn_comes_before (ENTRY_LABEL, ENTRY_LABEL);

  reuse_flex (&ns1, NUM_UD(mem), 1, F_DENSE);
  reuse_flex (&start, NUM_UD(mem), 1, F_DENSE);
  reuse_flex (&blks, N_BLOCKS, 1, F_DENSE);
  reuse_flex (&nblks, N_BLOCKS, 1, F_DENSE);

  off = UDN_VAR_OFFSET(mem,u);
  ud_rec_ptr_tmp = UDN_VRTX(mem,u);
  siz = RTX_BYTES(ud_rec_ptr_tmp);
  msk = UDN_BYTES(mem,u) << off;
  bases = get_var_bases (mem->maps.ud_map+u, do_coalesce);
  modes = 1 << (int) GET_MODE(UDN_VRTX(mem,u));

  set_flex_elt (s1, u);
  clr_flex_elt (s2, u);
  set_flex_elt (blks, UDN_BLOCK(mem,u));
  n = 1;
  copy_flex (start, s2);
  u = next_flex (start, u);

  *offp = off;
  *sizp = siz;
  ok = get_partition_masks (s1,headp,tailp,msk,siz,off,ld_msk,st_msk,n_ud);
  df_assert (ok);

  /* Basically, we grab uds, add them to s1 if we can, and stop when
     we cannot add any more. To make it into the s1 partition,
     there must be an in_dominant ud or an out_dominant_ud in
     the new partition, or alternatively, every path from the in-dominating
     insn to the out-dominating insn must hit an ud in the partition.

     Further, all defs reaching/uses reached for the candidate must either be
     in the partition already, or must be blocked by the insns dominating
     the region. */

  while (u != -1)
  {
    int nmsk, noff, nsiz, nld, nst,nbases,nmodes;

    clr_flex_elt (start, u);

    nmsk = msk;
    noff = off;
    nsiz = siz;
    nbases = bases;
    nmodes = modes;
  
    if (should_coalesce
        (mem->maps.ud_map+u, &nmsk, &noff, &nsiz, &nbases, &nmodes,do_coalesce))
    { int head, tail, ncnt, hblk, tblk, ohblk, otblk;

      ncnt = n+1;
      copy_flex (ns1, s1);
      copy_flex (nblks, blks);
      set_flex_elt (ns1, u);
      set_flex_elt (nblks, UDN_BLOCK(mem,u));

      get_range (df_info+IDF_MEM, ns1, &head, &tail);
      get_region (&head, &tail);

      hblk = BLOCK_NUM(DF_DOMI(head));
      tblk = BLOCK_NUM(DF_DOMI(tail));

      if (ok = constant_address (u, head, tail))
      { int t;
        t = -1;
        while (ok && (t=next_flex(s2,t)) != -1)
        {
          int tb = UDN_BLOCK(mem,t);

          df_assert (!df_data.iid_invalid);
          if (IID(DF_DOMI(head)) <= IID(UDN_IRTX(mem,t)) &&
              IID(DF_DOMI(tail)) >= IID(UDN_IRTX(mem,t)))
          {
            int tmsk = nmsk;
            int toff = noff;
            int tsiz = nsiz;
            int tbases = nbases;
            int tmodes = nmodes;
  
            df_assert (!(UDN_ATTRS(mem,t) & S_USE)==!!(UDN_ATTRS(mem,t)&S_DEF));
            if (t != u)
              if (should_coalesce (mem->maps.ud_map+t, &tmsk, &toff,
                                        &tsiz,&tbases, &tmodes, do_coalesce))
              {
                set_flex_elt (ns1, t);
                set_flex_elt (nblks, tb);
                ncnt++;
      
                nmsk = tmsk;
                noff = toff;
                nsiz = tsiz;
                nbases = tbases;
                nmodes = tmodes;
  
                if (head < 0 && DF_DOMI(head) == DF_DOMI(t))
                  head = t;
  
                if (tail < 0 && DF_DOMI(tail) == DF_DOMI(t))
                  tail = t;
              }
          }
        }

        if (ok)
          if ((ok = (head > 0) || (tail > 0)) == 0)
            ok = !path_from_a_to_c_around_b (hblk, nblks, tblk);
    
        if (ok)
          if (ok = partition_ok (mem, ns1, head, tail))
            ok=get_partition_masks(ns1,&head,&tail,nmsk,nsiz,noff,&nld,&nst,n_ud);
      }
    
      if (ok)
      { df_assert (ncnt > n);
        n = ncnt;
        copy_flex (s1, ns1);
        copy_flex (blks, nblks);
        *headp = head;
        *tailp = tail;
        *ld_msk = nld;
        *st_msk = nst;
        and_compl_flex_into (s2, s1);
        and_compl_flex_into (start, s1);
        msk = nmsk;
        bases = nbases;
        *offp = off = noff;
        *sizp = siz = nsiz;
      }
    }

    u = next_flex (start,u);

    if (u == -1 && !is_empty_flex (start))
      u = next_flex(start, -1);
  }

  *sizp = GET_MODE_SIZE(object_container(*sizp));

  reuse_flex (&ns1,0,0,0);
  reuse_flex (&start,0,0,0);
  reuse_flex (&blks,0,0,0);
  reuse_flex (&nblks,0,0,0);
  
  return n;
}

int tot_new_loads = 0;
int tot_new_stores = 0;
int tot_removed_loads = 0;
int tot_removed_stores = 0;

print_shadow_stats()
{
  char buf[255];
  sprintf (buf, "shadower removed %d references",
                 (tot_removed_loads+tot_removed_stores)-(tot_new_loads+tot_new_stores));
  warning (buf);
}

static
rtx find_rtx (x, c)
rtx x;
enum rtx_code c;
{
  enum rtx_code code;
  rtx v;
  
  if (x == 0 || (code=GET_CODE(x))==c)
    v = x;
  else
  { char *fmt = GET_RTX_FORMAT (code);
    int i;

    v = 0;

    for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
      if (fmt[i] == 'e')
        if (v = find_rtx (XEXP (x, i), c))
	  break;
  }
  return v;
}

void
remove_mem_notes (start, stop)
rtx start;
rtx stop;
{
  /* Find and remove all notes on [start...stop]
     which refer to memory. */

  rtx insn;

  insn = start;
  stop = NEXT_INSN(stop);

  while (insn != stop)
  {
    if (GET_RTX_CLASS (GET_CODE (insn)) == 'i')
    { rtx *link = &REG_NOTES(insn);
      while (*link)
      { enum reg_note rnk = REG_NOTE_KIND(*link);
        if ((rnk==REG_EQUAL || rnk==REG_EQUIV) && find_rtx (XEXP(*link,0), MEM))
          *link = XEXP(*link,1);
        else
          link = &XEXP(*link,1);
      }
    }
    insn = NEXT_INSN(insn);
  }
}

int
shadow_mem (do_coalesce, fsetup)
int do_coalesce;
int fsetup;
{
  ud_info* mem;
  int run_df;
  int n_ud;

  ud_set lcandid = 0;
  ud_set rest    = 0;
  ud_set loads   = 0;
  ud_set stores  = 0;
  ud_set tmp     = 0;

  static df_subst_queue queue;

  int new_loads = 0;
  int new_stores = 0;
  int removed_loads = 0;
  int removed_stores = 0;

  if (GET_DF_STATE(df_info+IDF_MEM) & DF_ERROR)
    return 0;

  revive_rtx (&ENTRY_LABEL_ZOMBIE);
  revive_rtx (&EXIT_LABEL_ZOMBIE);

  run_df = 0;
  mem    = df_info+IDF_MEM;

  n_ud = NUM_UD(mem);

  reuse_flex (&lcandid, n_ud, 1, F_DENSE);
  reuse_flex (&rest, n_ud, 1, F_DENSE);
  reuse_flex (&tmp, n_ud, 1, F_DENSE);

  if (df_data.dump_stream)
    fprintf (df_data.dump_stream, "\nlooking for coalescions ...");

  { int i;

    for (i = 1; i < NUM_ID (mem); i++)
    { int u,d,mc,siz,off;
      unsigned ri_msk, ro_msk, k_msk;

      /* Get rid of everything which is both def and use (volatile, etc). */
      {
        register int j;
        register flex_set defs = mem->sets.attr_sets[I_DEF];
        register flex_set uses = mem->sets.attr_sets[I_USE];

        clr_flex(rest);
        for (j = mem->maps.id_map[i].uds_used_by; j != -1;
             j = mem->uds_used_by_pool[j])
        {
          /*
           * only put the the element in the set if its either a USE or
           * a def but not both, this is the same as exclusive OR.
           */
          if (in_flex(defs, j) ^ in_flex(uses, j))
            set_flex_elt(rest, j);
        }
      }

      if (df_data.dump_stream)
      { fprintf (df_data.dump_stream, "\nvar %d, used by ", i);
        dump_flex (df_data.dump_stream, rest);
      }


      while (mc = find_mem_partition
            (lcandid,rest,&ri_msk,&ro_msk,&siz,&off,&u,&d,n_ud,do_coalesce))

      { int lc,sc, ok;

        if (df_data.dump_stream)
        { fprintf (df_data.dump_stream, "\npartition %d:", u);
          dump_flex (df_data.dump_stream, lcandid);
        }

        lc = count_moves (ri_msk);
        sc = count_moves (ro_msk);
#if 0
        /* Possible to get this w/#pragma pack and/or -mstrict-align */
        df_assert (lc + sc <= 2 && lc <= 1);
#endif

        if (df_data.dump_stream)
          fprintf (df_data.dump_stream,  "  mc:%d, u:%d, d:%d, umsk:%x, dmsk:%x\n", mc,u,d,ri_msk,ro_msk);

        if ((ok = (lc+sc < mc)) == 0)
        { int head_loop,b;

          head_loop = df_data.maps.loop_info[BLOCK_NUM(DF_DOMI(u))].inner_loop;

          b = -1;
          while (ok==0 && (b=next_flex(lcandid,b)) != -1)
            ok=df_data.maps.loop_info[UDN_BLOCK(mem,b)].inner_loop!=head_loop;
        }

        if (ok)
        {
          enum machine_mode m;
          int t,l,blk,oops,in_struct;
          rtx reg, areg, seq, prv, nxt;

          /* All these guys refer to the same value of the same variable.
             Pick a reg large enough to accomodate everybody, then 
             replace each ud as appropriate. */

          reg  = pick_ud_reg (lcandid, siz, off, &m, &in_struct);
          areg = 0;
          oops = 0;

          /* Hang loads */
          if (ri_msk && !oops)
          {
            if (u <= 0)
            { /* No address available;  concoct one from LT's. */

              prv  = UID_RTX(-u);
              nxt  = NEXT_INSN (prv);
              blk  = BLOCK_NUM(prv);

              seq = get_lt_addr_seq (&areg,mem_slots[i].terms,mem_slots[i].lt_offset+off,-u, lc+sc, (ro_msk!=0));
              oops = !seq;
            }
            else
            { /* Use the address of the leader, but remember that it has
                 to be adjusted to reflect the offset of the fragment we
                 need to load, which might be different from the actual
                 leader address (by as much as TI_BYTES) */

              rtx addr = copy_rtx (XEXP(UDN_VRTX(mem,u),0));

              nxt = UDN_IRTX (mem, u);
              prv = PREV_INSN(nxt);
              blk = BLOCK_NUM(nxt);

              seq = get_expr_seq (&areg, addr, off - UDN_VAR_OFFSET(mem,u), lc+sc, (ro_msk|=0));
            }

            if (!oops)
            { emit_insn_before (seq, nxt);
              scan_insns_for_uds (prv, nxt, blk);

              df_assert (((ri_msk >> off) << off) == ri_msk);
              emit_move_seq (reg, areg, i, off, 1, in_struct, blk, ri_msk>>off, nxt);
            }
          }

          /* Hang stores */
          if (ro_msk && !oops)
          {
            if (d > 0)
            {
              prv = UDN_IRTX (mem, d);
              nxt = NEXT_INSN(prv);
              blk = BLOCK_NUM(prv);
            }
            else
            {
              nxt = UID_RTX(-d);
              prv = PREV_INSN(nxt);
              blk = BLOCK_NUM(nxt);
            }

            /* Use address calculated for leader if available;  if not, use
               trailer address if available;  else, build it from LT's. */

            if (areg)
              areg = copy_rtx (areg);
            else
            {
              if (d <= 0)
              { seq = get_lt_addr_seq (&areg,mem_slots[i].terms,mem_slots[i].lt_offset+off,-d, sc+lc, 1);
                oops = !seq;
                if (!oops)
                { emit_insn_before (seq, nxt);
                  scan_insns_for_uds (prv, nxt, blk);
                }
              }
              else
              {
                rtx addr = copy_rtx (XEXP(UDN_VRTX(mem,d),0));

                seq = get_expr_seq (&areg, addr, off - UDN_VAR_OFFSET(mem,d),lc+sc,1);
                if (!oops)
                { rtx t = PREV_INSN(prv);

                  /* Last insn could change some component in addr (address of the object
                     being shadowed) so we always place the code to retrieve addr
                     directly before the last insn in the region. */

                  emit_insn_before (seq, prv);
                  scan_insns_for_uds (t, prv, blk);
                }
              }
            }

            if (!oops)
            { df_assert (((ro_msk >> off) << off) == ro_msk);
              emit_move_seq (reg, areg, i, off, 0, in_struct, blk, ro_msk>>off, nxt);
            }
          }

          if (!oops)
          {
            new_loads  += lc;
            new_stores += sc;

            t = -1;
            while ((t=next_flex(lcandid,t)) != -1)
            { if (UDN_ATTRS(mem,t) & S_USE)
                removed_loads++;
              else
                removed_stores++;
              replace_ud (reg, MEM_DF.maps.ud_map+t, off, m, 0, 0, &queue);
            }

            /* Remove all notes which refer to memory between the ld & st */
            remove_mem_notes(DF_DOMI(u),DF_DOMI(d));
            run_df = 1;
          }
        }
      }
    }
  }

  if (df_data.dump_stream)
    fprintf (df_data.dump_stream, "\n");

  do_df_subst (&queue);

  /* Set up MRD keys for scheduler */
#ifdef TMC_FLOD
  count_kills (df_info+IDF_MEM);
#endif
  /* We currently do sched_setup because loop uses the MRD info. */
  rebuild_df_helpers (df_info+IDF_MEM);
  imstg_sched_setup (get_insns());

  if (fsetup)
    update_finfo();
    
  if (df_data.code_at_exit)
  { rtx insn;
    rtx ruse = 0;

    /* Replace all returns with jump to exit label at end. */
    for (insn = get_last_insn(); insn; insn=PREV_INSN(insn))
    if (GET_CODE(insn)==JUMP_INSN && GET_CODE(PATTERN(insn))==RETURN)
    {
      rtx p = PREV_INSN(insn);
      if (p != 0 && GET_CODE(p)==INSN && GET_CODE(PATTERN(p))==USE)
      { /* Delete use at old return; put one on at new one. */
        if (ruse == 0)
          emit_insn (ruse=copy_rtx(PATTERN(p)));

        PUT_CODE (p, NOTE);
        NOTE_LINE_NUMBER (p) = NOTE_INSN_DELETED;
        NOTE_SOURCE_FILE (p) = 0;
      }

      PATTERN(insn) =
        gen_rtx (SET, VOIDmode, pc_rtx, gen_rtx(LABEL_REF,VOIDmode,EXIT_LABEL));

      JUMP_LABEL(insn) = EXIT_LABEL;
      LABEL_NUSES(EXIT_LABEL) += 1;
      INSN_CODE(insn) = -1;
    }
    /* Put return back on at the very end */
    emit_jump_insn (gen_return());
  }
  else
    mark_rtx_deleted (EXIT_LABEL, &EXIT_LABEL_ZOMBIE);

  mark_rtx_deleted (ENTRY_LABEL, &ENTRY_LABEL_ZOMBIE);

  reuse_flex (&lcandid,0,0,0);
  reuse_flex (&rest,0,0,0);
  reuse_flex (&loads,0,0,0);
  reuse_flex (&stores,0,0,0);
  reuse_flex (&tmp,0,0,0);

#if 0
  fprintf (stderr,"\nfinish shadow_mem function %s\n",
                  IDENTIFIER_POINTER (DECL_NAME (current_function_decl)));
#endif

  if (removed_loads|removed_stores)
  {
    char buf[255];

    tot_new_loads += new_loads;
    tot_new_stores += new_stores;
    tot_removed_loads += removed_loads;
    tot_removed_stores += removed_stores;
#ifdef TMC_NOISY2
    sprintf (buf, "shadower: +(%d+%d),-(%d+%d), now +(%d+%d),-(%d+%d)",
             new_loads,new_stores,
             removed_loads,removed_stores,
             tot_new_loads,tot_new_stores,
             tot_removed_loads,tot_removed_stores);
    warning (buf);
#endif
  }

  return run_df;
}

typedef struct
{
  rtx* reg[5];
  rtx  pat[5];
  rtx  mem[5];
  int  is_load;
}
mm_struct;
mm_struct *mm_info;

int next_mnum;

int
alloc_multi_move()
{
  static int prev_bytes;
  int need_bytes;

  ++next_mnum;

  need_bytes = (next_mnum+1) * sizeof (mm_struct);

  if (need_bytes > prev_bytes)
  {
    need_bytes *= 2;
  
    mm_info = (mm_struct*) xrealloc (mm_info, need_bytes);
    bzero (((char*)mm_info)+prev_bytes, need_bytes-prev_bytes);

    prev_bytes = need_bytes;
  }

  return next_mnum;
}

record_multi_move(m, insn, is_load)
mm_struct *m;
rtx insn;
{
  rtx p = PATTERN(insn);

  int i = 0;

  while (m->pat[i])
    i++;

  df_assert (i < 4);
  m->pat[i] = p;
  m->is_load = is_load;

  if (is_load)
  {
    m->reg[i] = &SET_DEST(p);
    m->mem[i] = SET_SRC(p);
  }
  else
  {
    m->reg[i] = &SET_SRC(p);
    m->mem[i] = SET_DEST(p);
  }
}

/*  Traverse the rtl, splitting all multi-word memory-register
    transfers.  Mark them;  we will put them back together later
    with marry_mem_refs */

split_mem_refs (pass_num)
int pass_num;
{
  rtx i = get_insns();

  while (i != 0)
  {
    if (GET_CODE(i)==INSN)
    { rtx p = PATTERN(i);

      /*  This field has to be cleared because global inlining uses it. */
      if (pass_num == 1)
        INSN_MULTI_MOVE(i) = 0;

      if (GET_CODE(p)==SET)
      { rtx *d,*s;
        enum machine_mode m;
        int ld,st;

        d = &SET_DEST(p);
        s = &SET_SRC(p);
        m = GET_MODE(*d);

        if (m==DImode||m==TImode && GET_MODE(*d)==GET_MODE(*s))
        {
          INSN_MULTI_MOVE(i) = 0;

          /* Split subreg to DI of TI, too */
          if (GET_CODE(*d)==SUBREG && GET_MODE(SUBREG_REG(*d))==TImode)
            d = &SUBREG_REG(*d);
  
          if (GET_CODE(*s)==SUBREG && GET_MODE(SUBREG_REG(*s))==TImode)
            s = &SUBREG_REG(*s);
  
          ld = (GET_CODE(*s)==MEM && GET_CODE(*d)==REG);
          st = (GET_CODE(*d)==MEM && GET_CODE(*s)==REG);

          if (ld+st == 1)
          { int n,mv;
            rtx t;

            /* Allocate space to record what we are doing */
            mv = alloc_multi_move();

            if (GET_CODE(*d)==MEM)
            { *d = copy_rtx(*d);
              RTX_ID(*d) = 0;
            }

            if (GET_CODE(*s)==MEM)
            { *s = copy_rtx(*s);
              RTX_ID(*s) = 0;
            }

            d = &SET_DEST(p);
            s = &SET_SRC(p);

            /* Word copies */
            for ((n=0),(t=i); n < GET_MODE_SIZE(m)/SI_BYTES; n++)
            {
              p = gen_rtx (SET, SImode, pun_rtx_offset (t, *d, SImode, n),
                                        pun_rtx_offset (t, *s, SImode, n));
              if (n == 0)
                PATTERN(i) = p;
              else
                i = emit_insn_after (p, i);
  
              INSN_CODE(i) = -1;
              INSN_MULTI_MOVE(i) = mv;
              record_multi_move (&mm_info[mv], i, ld);
            }
          }
        }
      }
    }
    i = NEXT_INSN(i);
  }
}

static int
reg_word (r, n, base, offset)
rtx r;
int n;
rtx* base;
int *offset;
{
  int ret = 0;

  *offset = 0;

  if (GET_CODE(*base=r) == SUBREG)
  { *offset = SUBREG_WORD(r);
    *base   = SUBREG_REG(r);
  }

  if (ret = (GET_CODE(*base)==REG && GET_MODE(r)==SImode))
    if (REGNO(*base) < FIRST_PSEUDO_REGISTER)
    { *offset += REGNO(*base);
      *base = 0;
    }
    else
      if (n)
        ret = GET_MODE_SIZE(GET_MODE(*base)) >= (n+*offset) * SI_BYTES;

  if (n)
    ret &= (*offset % n) == 0;

  return ret;
}

static int
check_group (info)
mm_struct *info;
{
  int i,words,offset,ok,toff;
  rtx base_reg,t;

  words = 0;
  while (info->pat[words])
    words++;

  df_assert (words==2 || words==4);

  if (ok = reg_word (*info->reg[0], words, &base_reg, &offset))
    for (i = 1; i < words && ok; i++)
      ok = reg_word(*info->reg[i],0,&t,&toff) &&
                    t==base_reg && toff==(offset+i);

  return ok;
}

marry_mem_refs ()
{
  rtx i = get_insns();

  while (i != 0)
  {
    if (GET_CODE(i)==INSN)
    {
      int mv=INSN_MULTI_MOVE(i);
      int words = 0;
     
      if (mv)
        while (mm_info[mv].pat[words])
          words++;

      if (words)
      { 
        int ok,j,is_ld;
        rtx t,p;
        mm_struct info;

        info = mm_info[mv];
        is_ld = info.is_load;

        df_assert (words==2 || words==4);
  
        /* See if the sequence has been tampered with */
        for ((j=0),(ok=1),(t=i); j < words && ok; (j++,t=NEXT_INSN(t)))
        { ok = 0;
            
          if (j==0 || (GET_CODE(t)==INSN && INSN_MULTI_MOVE(t) == mv))
          { p = PATTERN(t);
            if (info.pat[j] == p)
              if (is_ld)
                ok = info.mem[j] == SET_SRC(p);
              else
                ok = info.mem[j] == SET_DEST(p);
          }
        }

        bzero (&mm_info[mv], sizeof (mm_struct));

        if (ok)
          /* Group has not been tampered with;  we have a sequence of
             moves of the recorded length, to or from the proper
             memory location. */

          if ((ok = check_group (&info)) == 0)
            if (words == 4)
            {
              words = 2;

              /*  Try the current move as DImode */
              bzero (&info, sizeof (mm_struct));
              record_multi_move (&info, i, is_ld);
              record_multi_move (&info, t=NEXT_INSN(i), is_ld);
              if ((ok = check_group (&info)) == 0)
                i = t;

              /* Set up the remaining 2 words as DImode;  we will try
                 them on the next trip around the loop. */

              record_multi_move (&mm_info[mv], t=NEXT_INSN(t), is_ld);
              record_multi_move (&mm_info[mv], t=NEXT_INSN(t), is_ld);
            }

        if (ok)
        {
          enum machine_mode mode = (words==2 ? DImode : TImode);

          p = PATTERN(i);

          /* Pun the first move up to the width of the entire sequence */
          SET_SRC(p) = pun_rtx_offset (i,SET_SRC(p), mode, 0);
          SET_DEST(p) = pun_rtx_offset (i,SET_DEST(p), mode, 0);
          GET_MODE(p) = mode;
          INSN_MULTI_MOVE(i) = 0;
          INSN_CODE(i) = -1;

          /* Delete the rest of the moves */
          for (j=1; j < words; j++)
          {
            i=NEXT_INSN(i);
	    PUT_CODE (i, NOTE);
	    NOTE_LINE_NUMBER (i) = NOTE_INSN_DELETED;
	    NOTE_SOURCE_FILE (i) = 0;
          }
        }
      }
    }

    i=NEXT_INSN(i);
  }
}

#endif
