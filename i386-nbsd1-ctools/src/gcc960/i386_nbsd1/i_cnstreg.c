#include "config.h"

#ifdef IMSTG
#include <stdio.h>
#include "tree.h"
#include "rtl.h"
#include "regs.h"
#include "flags.h"

#ifdef GCC20
#include "i_list.h"
#include "i_set.h"
#include "i_dataflow.h"
#include "basic-block.h"
#include "hard-reg-set.h"
#else
#include "list.h"
#include "set.h"
#include "dataflow.h"
#include "basicblk.h"
#include "hreg_set.h"
#endif

#ifndef HARD_REGNO_REF
#define HARD_REGNO_REF(X, RGNO_P, RGSIZE_P) \
  generic_hard_regno_ref(X, RGNO_P, RGSIZE_P)
#endif

#define GET_LIST_USE(P) GET_LIST_ELT(P,rtx *)
#define GET_LIST_DEF(P) GET_LIST_ELT(P,cand_ptr)
#define GET_LIST_PRED(P) GET_LIST_ELT(P, int)
#define GET_LIST_SUCC(P) GET_LIST_ELT(P, int)

typedef struct cand_rec {
  rtx cand_insn;
  int loop_top_bb;
  int delta_expect;
  int cand_no;
  int dreg;
  int cand_size;
  int rewritable;
  int rewrite_crosses_call;
  list_p uses;
  struct cand_rec *assoc_defs[4];
  struct cand_rec *assoc_with;
  int moved;
} * cand_ptr;

static int s_n_cands;
static cand_ptr s_cands;

static list_p * s_preds;
static list_p * s_succs;

static set *rd_in;
static set rd_buf;

static set *lv_out;
static set lv_buf;
static int *lv_n_sets_in_bb;

static list_block_p list_blks;

static list_p * defs_using_reg;
static cand_ptr *insn_gens_def;

/* These two are simply used for temporary calculations. */
static set_array s_regs_insn_sets[SET_SIZE(FIRST_PSEUDO_REGISTER)];
static set_array s_regs_insn_uses[SET_SIZE(FIRST_PSEUDO_REGISTER)];

void
insn_reg_use(x, dst_context, regs_set, regs_used)
register rtx x;
int dst_context;
register set regs_set;
register set regs_used;
{
  register enum rtx_code code;
  int rgno;
  int rgsize;
  register t_rgno;
  register e_rgno;

  if (x == 0)
    return;

  switch (code = GET_CODE(x))
  {
    case NOTE:
    case CODE_LABEL:
    case BARRIER:
      CLR_SET(regs_used, SET_SIZE(FIRST_PSEUDO_REGISTER));
      CLR_SET(regs_set, SET_SIZE(FIRST_PSEUDO_REGISTER));
      break;

    case CALL_INSN:
      CLR_SET(regs_used, SET_SIZE(FIRST_PSEUDO_REGISTER));
      CLR_SET(regs_set, SET_SIZE(FIRST_PSEUDO_REGISTER));

      /*
       * All uses of registers due to parameters are taken care of
       * by the USE insns immediately preceding the call_insn.
       */

      /*
       * Any registers that are referenced in the call insn's pattern
       * must also be considered used or set.
       */
      insn_reg_use(PATTERN(x), 0, regs_set, regs_used);

      /*
       * The call can be considered to set any and all of the call-used
       * registers.
       */
      for (t_rgno = 0; t_rgno < FIRST_PSEUDO_REGISTER; t_rgno++)
      {
        if (call_used_regs[t_rgno] != 0)
          SET_SET_ELT(regs_set, t_rgno);
      }
      break;

    case JUMP_INSN:
    case INSN:
      CLR_SET(regs_used, SET_SIZE(FIRST_PSEUDO_REGISTER));
      CLR_SET(regs_set, SET_SIZE(FIRST_PSEUDO_REGISTER));
      insn_reg_use(PATTERN(x), 0, regs_set, regs_used);
      break;

    case CALL:
      insn_reg_use(XEXP(x,0), 0, regs_set, regs_used);
      break;

    case RETURN:
      /* make all possible return value registers be used. */  
      for (t_rgno = 0; t_rgno < 4; t_rgno++)
        SET_SET_ELT(regs_used, t_rgno);
      break;

    case SET:
      insn_reg_use(SET_DEST(x), 1, regs_set, regs_used);
      insn_reg_use(SET_SRC(x), 0, regs_set, regs_used);
      break;

    case CLOBBER:
      /* mark a CLOBBER as both a use and a set. */
      insn_reg_use(XEXP(x,0), 0, regs_set, regs_used);
      insn_reg_use(XEXP(x,0), 1, regs_set, regs_used);
      break;

    case ASM_OPERANDS:
    case ASM_INPUT:
      SET_SET_ALL(regs_set, SET_SIZE(FIRST_PSEUDO_REGISTER));
      SET_SET_ALL(regs_used, SET_SIZE(FIRST_PSEUDO_REGISTER));
      break;

    case MEM:
      insn_reg_use(XEXP(x,0), 0, regs_set, regs_used);
      break;

    case SUBREG:
    case REG:
      HARD_REGNO_REF(x, &rgno, &rgsize);
      if (dst_context)
      {
        for (t_rgno = rgno, e_rgno = t_rgno + rgsize; t_rgno < e_rgno; t_rgno++)
          SET_SET_ELT(regs_set, t_rgno);
      }
      else
      {
        for (t_rgno = rgno, e_rgno = t_rgno + rgsize; t_rgno < e_rgno; t_rgno++)
          SET_SET_ELT(regs_used, t_rgno);
      }
      break;

    default:
    {
      register char* fmt = GET_RTX_FORMAT (code);
      register int i     = GET_RTX_LENGTH (code);

      while (--i >= 0)
      {
        if (fmt[i] == 'e')
          insn_reg_use(XEXP(x,i), dst_context, regs_set, regs_used);
        else if (fmt[i] == 'E')
        {
          register int j = XVECLEN(x,i);
          while (--j >= 0)
            insn_reg_use(XVECEXP(x,i,j), dst_context, regs_set, regs_used);
        }
      }
    }
    break;
  }
}

static int
find_reg_uses(x_p, dst_context, repl_context, use_rout)
register rtx *x_p;
int dst_context;
int repl_context;
int (*use_rout)();
{
  register rtx x = *x_p;
  register enum rtx_code code;
  int rgno;
  int rgsize;
  register t_rgno;
  register e_rgno;

  if (x == 0)
    return;

  switch (code = GET_CODE(x))
  {
    case NOTE:
    case CODE_LABEL:
    case BARRIER:
      break;

    case CALL_INSN:
      /*
       * all the funny uses will already be taken care of by the use
       * insns occurring immediately before the call insn.
       */
      find_reg_uses(&PATTERN(x), 0, 1, use_rout);
      break;

    case JUMP_INSN:
    case INSN:
      find_reg_uses(&PATTERN(x), 0, 1, use_rout);
      break;

    case CALL:
      find_reg_uses(&XEXP(x,0), 0, 1, use_rout);
      break;

    case RETURN:
      /* make all possible return value registers be used. */  
      use_rout(0, 4, (rtx *)0);
      break;

    case SET:
      find_reg_uses(&SET_DEST(x), 1, 0, use_rout);
      find_reg_uses(&SET_SRC(x), 0, 1, use_rout);
      break;

    case USE:
    case CLOBBER:
      find_reg_uses(&XEXP(x,0), 0, 0, use_rout);
      break;

    case ASM_OPERANDS:
    case ASM_INPUT:
      for (t_rgno = 0; t_rgno < FIRST_PSEUDO_REGISTER; t_rgno++)
        use_rout(t_rgno, 1, (rtx *)0);
      break;

    case MEM:
      find_reg_uses(&XEXP(x,0), 0, 1, use_rout);
      break;

    case SUBREG:
    case REG:
      if (!dst_context)
      {
        HARD_REGNO_REF(x, &rgno, &rgsize);
        if (repl_context)
          use_rout(rgno, rgsize, x_p);
        else
          use_rout(rgno, rgsize, (rtx *)0);
      }
      break;

    default:
    {
      register char* fmt = GET_RTX_FORMAT (code);
      register int i     = GET_RTX_LENGTH (code);

      while (--i >= 0)
      {
        if (fmt[i] == 'e')
          find_reg_uses(&XEXP(x,i), dst_context, repl_context, use_rout);
        else if (fmt[i] == 'E')
        {
          register int j = XVECLEN(x,i);
          while (--j >= 0)
            find_reg_uses(&XVECEXP(x,i,j), dst_context, repl_context, use_rout);
        }
      }
    }
    break;
  }
}

static void
defs_insn_gens_kills(insn, gen_set, kill_set, set_size)
rtx insn;
set gen_set;
set kill_set;
int set_size;
{
  set insn_sets = s_regs_insn_sets;
  set insn_uses = s_regs_insn_uses;
  cand_ptr p = insn_gens_def[INSN_UID(insn)];
  int i;
  int t;

  /*
   * 0 - FIRST_PSEUDO_REGISTER def numbers are reserved for dummy defs
   * indicating arbitrary sets of the register number not associated with
   * any of the candidate defs.  These def numbers are only killed by an
   * insn that is one of the candidate defs.
   */
  CLR_SET(kill_set, set_size);
  CLR_SET(gen_set, set_size);

  insn_reg_use(insn, 0, insn_sets, insn_uses);

  FORALL_SET_BEGIN(insn_sets, SET_SIZE(FIRST_PSEUDO_REGISTER), i)
    if (i < FIRST_PSEUDO_REGISTER)
    {
      list_p def_p;

      for (def_p = defs_using_reg[i]; def_p != 0; def_p = def_p->next)
      {
        t = GET_LIST_DEF(def_p)->cand_no;
        SET_SET_ELT(kill_set, t);
      }

      /*
       * generate a dummy def for all the registers set.
       * Any of these dummies that are killed will be removed from
       * the gen_set when we calculate the dummy defs killed.
       */
      SET_SET_ELT(gen_set, i);
    }
  FORALL_SET_END;

  if (p != 0)
  {
    /*
     * See if any of the dummy defs are killed by this insn.
     */
    for (i = 0; i < p->cand_size; i++)
    {
      t = p->dreg + i;
      SET_SET_ELT(kill_set, t);
      CLR_SET_ELT(gen_set, t);
    }

    /* now get the real defs gened by this instruction. */
    SET_SET_ELT(gen_set, p->cand_no);
  }
}

static void
most_profitable_loop(insn, loop_info, loop_top_p, expect_delta_p)
rtx insn;
loop_rec *loop_info;
int *loop_top_p;
int *expect_delta_p;
{
  int loop_top;
  int best_expect = 0;
  int best_loop = -1;
  list_p * preds = s_preds;

  loop_top = loop_info[BLOCK_NUM(insn)].inner_loop;
  if (loop_top == -1)
    loop_top = loop_info[BLOCK_NUM(insn)].parent;

  while (loop_top != -1)
  {
    flex_set loop_members;
    list_p pred_p;
    int delta_expect = INSN_EXPECT(insn);

    loop_members = loop_info[loop_top].members;

    for (pred_p = preds[loop_top]; pred_p != 0; pred_p = pred_p->next)
    {
      int loop_pred = GET_LIST_PRED(pred_p);

      if (!in_flex(loop_members, loop_pred))
      {
        rtx expect_insn = BLOCK_END(loop_pred);

        while (expect_insn != 0)
        {
          if (GET_CODE(expect_insn) == INSN ||
              GET_CODE(expect_insn) == CALL_INSN ||
              GET_CODE(expect_insn) == JUMP_INSN ||
              GET_CODE(expect_insn) == CODE_LABEL)
          {
            delta_expect -= INSN_EXPECT(expect_insn);
            break;
          }

          if (GET_CODE(expect_insn) == NOTE &&
#if !defined(DEV_465)
              NOTE_LINE_NUMBER(expect_insn) == NOTE_INSN_FUNCTION_ENTRY)
#else 
              NOTE_LINE_NUMBER(expect_insn) == NOTE_INSN_FUNCTION_BEG)
#endif
          {
            delta_expect -= NOTE_CALL_COUNT(expect_insn) * PROB_BASE;
            break;
          }

          expect_insn = PREV_INSN(expect_insn);
        }

        if (delta_expect < best_expect)
          break;
      }
    }

    if (delta_expect > best_expect)
    {
      best_expect = delta_expect;
      best_loop = loop_top;
    }

    loop_top = loop_info[loop_top].parent;
  }

  *loop_top_p = best_loop;
  *expect_delta_p = best_expect;
}

int cmp_cands(t1, t2)
cand_ptr t1;
cand_ptr t2;
{
  if (t1->delta_expect != t2->delta_expect)
    return t1->delta_expect - t2->delta_expect;

#if 0
  /*
   * Sort by lowest register number.  This causes the even numbered
   * registers to always occur before the odd number registers, which
   * can help for associated registers.
   */
  if (t1->dreg != t2->dreg)
    return (t1->dreg - t2->dreg);
#endif

  return (t1->cand_no - t2->cand_no);
}

static int
find_candidates(insns, loop_info, cands)
rtx insns;
loop_rec * loop_info;
cand_ptr cands;
{
  register rtx t_insn;
  register enum rtx_code c;
  register int n_cands = 0;
  int dreg;
  int dreg_size;
  int i;

  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    if (GET_CODE(t_insn) == INSN &&
        (loop_info[BLOCK_NUM(t_insn)].inner_loop != -1 ||
         loop_info[BLOCK_NUM(t_insn)].parent != -1) &&
        GET_CODE(PATTERN(t_insn)) == SET &&
        ((c = GET_CODE(SET_SRC(PATTERN(t_insn)))) == CONST_INT ||
         c == CONST_DOUBLE || c == CONST))
    {
      HARD_REGNO_REF(SET_DEST(PATTERN(t_insn)), &dreg, &dreg_size);
      if (dreg >= 0)
      {
        if (cands != 0)
        {
          int loop_top;
          int delta_expect;

          /*
           * We have a potential candidate. See if it has an expect
           * that would make it profitable.
           */
          most_profitable_loop(t_insn, loop_info, &loop_top, &delta_expect);

          if (loop_top != -1 && delta_expect > 0)
          {
            int i;

            cands[n_cands].cand_insn = t_insn;
            cands[n_cands].loop_top_bb = loop_top;
            cands[n_cands].delta_expect = delta_expect;
            cands[n_cands].cand_no = n_cands + FIRST_PSEUDO_REGISTER;
            cands[n_cands].dreg = dreg;
            cands[n_cands].cand_size = dreg_size;
            cands[n_cands].rewritable = 1;
            cands[n_cands].uses = 0;
            cands[n_cands].assoc_with = 0;
            cands[n_cands].moved = 0;
          }
          else
            n_cands --;
        }

        n_cands ++;
      }
    }
  }

  if (cands != 0)
  {
    qsort (cands, n_cands, sizeof(struct cand_rec), cmp_cands);

    for (i = 0; i < n_cands; i++)
    {
      cands[i].assoc_defs[0] = &cands[i];
      cands[i].assoc_defs[1] = 0;
      cands[i].assoc_defs[2] = 0;
      cands[i].assoc_defs[3] = 0;
    }
  }

  return n_cands;
}

static set s_bb_in;

static int
note_use(rg, rgsize, rg_rtx_p)
int rg;
int rgsize;
rtx *rg_rtx_p;
{
  register int t_rgno;
  register int t_end;
  register list_p p;
  register cand_ptr cand;
  register set bb_in = s_bb_in;
  int is_rewritable;
  int def_bb;
  int cand_no;
  unsigned use_mask;
  cand_ptr defs[4];
  int cand_bb;

  defs[0] = defs[1] = defs[2] = defs[3] = 0;

  /*
   * find all defs that reach this use. If this use is not replaceable,
   * then mark all the uses as not being rewritable.
   *
   * If any dummy def reaches this use then all real defs that reach it
   * need to be marked as not being rewritable;
   *
   * Finally if several defs reach it, but some of them overlap, then
   * all defs get marked as not being rewritable.
   *
   * If multiple defs reach this use, but none of them overlap, and they
   * are all in the same basic block, then
   * they need to get marked as being associated with the def of the lowest
   * numbered reg, and they can be marked as rewritable.
   *
   * If only one def reaches, then it is still rewritable.
   *
   * Put this use in the list of uses for the rewritable def.
   */

  is_rewritable = (rg_rtx_p != 0);

  /* check if its reached by any dummy def. */
  if (is_rewritable)
  {
    for (t_rgno = rg, t_end = rg+rgsize; t_rgno < t_end; t_rgno++)
    {
      if (IN_SET(bb_in, t_rgno))
        is_rewritable = 0;
    }
  }

  /*
   * check if its reached by more than one of the constant defs, and
   * they overlap.  This is done by looking for a single def on each
   * of the component registers.  If there is only one, then its ok.
   */
  def_bb = -1;
  use_mask = (1 << rgsize) - 1;
  for (t_rgno = rg, t_end = rg+rgsize;
       t_rgno < t_end && is_rewritable; t_rgno++)
  {
    int num_reaches = 0;
    for (p = defs_using_reg[t_rgno]; p != 0; p = p->next)
    {
      cand = GET_LIST_DEF(p);
      cand_no = cand->cand_no;

      if (IN_SET(bb_in, cand_no))
      {
        cand_bb = BLOCK_NUM(cand->cand_insn);
        num_reaches += 1;
        use_mask &= ~(1 << (t_rgno - rg));
        defs[t_rgno - rg] = cand;
      }
    }

    if (num_reaches != 1)
      is_rewritable = 0;
    else
    {
      if (def_bb == -1)
        def_bb = cand_bb;
      else if (def_bb != cand_bb)
        is_rewritable = 0;
    }
  }

  if (use_mask != 0 || defs[0] == 0 || defs[0]->assoc_with != 0)
    is_rewritable = 0;

  for (t_rgno = 1; t_rgno < 4 && is_rewritable; t_rgno ++)
  {
    if (defs[t_rgno] != 0)
    {
      if (defs[0]->assoc_defs[t_rgno] != 0 &&
          defs[0]->assoc_defs[t_rgno] != defs[t_rgno])
        is_rewritable = 0;

      if (defs[t_rgno]->assoc_with != 0 &&
          defs[t_rgno]->assoc_with != defs[0])
        is_rewritable = 0;
    }
  }

  /*
   * record the is_rewritable info for all defs reaching here. If there is
   * any associated defs record this also.
   */
  if (!is_rewritable)
  {
    for (t_rgno = rg, t_end = rg+rgsize; t_rgno < t_end; t_rgno++)
    {
      for (p = defs_using_reg[t_rgno]; p != 0; p = p->next)
      {
        cand = GET_LIST_DEF(p);
        cand_no = cand->cand_no;

        if (IN_SET(bb_in, cand_no))
          cand->rewritable = 0;
      }
    }
  }
  else
  {
    list_p new_node;

    /* note that this def reaches this use */
    new_node = alloc_list_node(&list_blks);
    SET_LIST_ELT(new_node, rg_rtx_p);
    new_node->next = defs[0]->uses;
    defs[0]->uses = new_node;

    /* for any associated defs record the association. */
    for (t_rgno = 1; t_rgno < 4; t_rgno ++)
    {
       if (defs[t_rgno] != 0)
       {
         /* record the association if any. */
         defs[0]->assoc_defs[t_rgno] = defs[t_rgno];
         defs[t_rgno]->assoc_with = defs[0];
       }
    }
  }
}

static void
cnstreg_rd_dfa(insns)
rtx insns;
{
  set *in;
  set *out;
  set *gen;
  set *kill;
  set trd_buf;
  int set_size;
  int bb_num;
  int change;
  list_p *preds;
  register set tmp_gen_set;
  register set tmp_kill_set;
  register set in_bb;
  register set out_bb;
  register set gen_bb;
  register set kill_bb;
  int i;

  /* allocate insn gens def array */
  insn_gens_def = (cand_ptr *)xmalloc(i=(get_max_uid()*sizeof(cand_ptr)));
  bzero(insn_gens_def, i);

  /* allocate defs_using_reg */
  defs_using_reg = (list_p *)xmalloc(FIRST_PSEUDO_REGISTER * sizeof(list_p));
  bzero(defs_using_reg, FIRST_PSEUDO_REGISTER * sizeof(list_p));

  /* allocate gen, kill, in, out for reaching defs. */
  set_size = SET_SIZE(s_n_cands+FIRST_PSEUDO_REGISTER);
  trd_buf = rd_buf = ALLOC_SETS((N_BLOCKS * 4) + 2, set_size);
  in      = rd_in  = (set *)xmalloc((N_BLOCKS * 4) * sizeof(set));
  out     = in + N_BLOCKS;
  gen     = out + N_BLOCKS;
  kill     = gen + N_BLOCKS;

  tmp_gen_set = trd_buf; trd_buf += set_size;
  tmp_kill_set = trd_buf; trd_buf += set_size;

  /*
   * initialize insn_gens_def and defs_using_reg
   */
  for (i = s_n_cands-1; i >= 0; i--)
  {
    int t_rgno;
    int t_end;
    cand_ptr cand = &s_cands[i];
    list_p new_node;

    insn_gens_def[INSN_UID(cand->cand_insn)] = cand;

    for (t_rgno = cand->dreg, t_end = t_rgno + cand->cand_size;
         t_rgno < t_end; t_rgno++)
    {
      new_node = alloc_list_node(&list_blks);
      SET_LIST_ELT(new_node, cand);
      new_node->next = defs_using_reg[t_rgno];
      defs_using_reg[t_rgno] = new_node;
    }
  }

  /*
   * initialize gen, kill, in, out.
   *
   * gen is the set of all candidate defs that occur in basic block,
   * and aren't killed by a another def of the register later in the basic
   * block.
   *
   * kill is the set of all candidate defs whose destination register is
   * defed in the block, and it isn't gened later in the block.
   * 
   * in is initialized to the empty set.
   *
   * out is initialized to the gen set.
   */
  for (bb_num = 0; bb_num < N_BLOCKS; bb_num++)
  {
    rtx t_insn = BLOCK_HEAD(bb_num);

    in[bb_num]   = in_bb   = trd_buf; trd_buf += set_size;
    out[bb_num]  = out_bb  = trd_buf; trd_buf += set_size;
    gen[bb_num]  = gen_bb  = trd_buf; trd_buf += set_size;
    kill[bb_num] = kill_bb = trd_buf; trd_buf += set_size;

    CLR_SET(gen_bb, set_size);
    CLR_SET(kill_bb, set_size);
    CLR_SET(in_bb, set_size);

    if (bb_num == ENTRY_BLOCK)
    {
      /* make dummy defs for all possible incoming params. */
      int treg;
      for (treg = 0; treg < 12; treg++)
      {
        SET_SET_ELT(in_bb,  treg);
      }
    }

    while (1)
    {
      /*
       * get the set of defs that are killed by this insn.  Add them into
       * the kill set for this block, and delete them from the set of gens.
       */
      defs_insn_gens_kills(t_insn, tmp_gen_set, tmp_kill_set, set_size);
      OR_SETS(kill_bb, kill_bb, tmp_kill_set, set_size);
      AND_COMPL_SETS(gen_bb, gen_bb, tmp_kill_set, set_size);

      /*
       * get the set of defs that the insn gens. Add them into the set
       * of gens for the block.
       */
      OR_SETS(gen_bb, gen_bb, tmp_gen_set, set_size);

      if (t_insn == BLOCK_END(bb_num))
        break;
      t_insn = NEXT_INSN(t_insn);
    }

    COPY_SET(out_bb, gen_bb, set_size);
  }

  /* now run the dataflow */
  preds = s_preds;
  do
  {
    change = 0;
    for (bb_num = 0; bb_num < N_BLOCKS; bb_num ++)
    {
      if (bb_num != ENTRY_BLOCK)
      {
        list_p bb_pred;
        int is_equal;

        if ((bb_pred = preds[bb_num]) == 0)
          continue;  /* can only happen with unconnected graph */

        out_bb = out[GET_LIST_PRED(bb_pred)];
        COPY_SET(tmp_gen_set, out_bb, set_size);

        while ((bb_pred = bb_pred->next) != 0)
        {
          out_bb = out[GET_LIST_PRED(bb_pred)];
          OR_SETS (tmp_gen_set, tmp_gen_set, out_bb, set_size);
        }

        in_bb = in[bb_num];
        EQUAL_SETS(tmp_gen_set, in_bb, set_size, is_equal);

        if (!is_equal)
        {
          COPY_SET (in_bb, tmp_gen_set, set_size);

          /* generate new out. */
          out_bb  = out[bb_num];
          kill_bb = kill[bb_num];
          gen_bb  = gen[bb_num];
          AND_COMPL_SETS(out_bb, in_bb, kill_bb, set_size);
          OR_SETS(out_bb, out_bb, gen_bb, set_size);

          change = 1;
        }
      }
    }
  } while (change);

  /*
   * now want to for each candidate build up a list of the places that
   * that candidate's def reaches.
   */
  for (bb_num = 0; bb_num < N_BLOCKS; bb_num++)
  {
    rtx t_insn = BLOCK_HEAD(bb_num);
    
    in_bb = in[bb_num];
    s_bb_in = in_bb;

    while (1)
    {
      rtx dummy = t_insn;

      find_reg_uses(&dummy, 0, 1, note_use);

      /* kill all defs that this kills */
      defs_insn_gens_kills(t_insn, tmp_gen_set, tmp_kill_set, set_size);
      AND_COMPL_SETS(in_bb, in_bb, tmp_kill_set, set_size);

      /* add into in_bb all the defs that the insn gens */
      OR_SETS(in_bb, in_bb, tmp_gen_set, set_size);
  
      if (t_insn == BLOCK_END(bb_num))
        break;
      t_insn = NEXT_INSN(t_insn);
    }
  }

  /* at the end of the function note the uses of the return value. */
  /* make all possible return value registers be used. */  
  note_use(0, 4, (rtx *)0);

  /* free various allocations no longer needed. */
  free(rd_in);  
  free(rd_buf);
  free(insn_gens_def);
  free(defs_using_reg);
}

static void
cnstreg_lv_dfa(insns)
rtx insns;
{
  set *in;
  set *out;
  set *gen;
  set *kill;
  set tlv_buf;
  int set_size;
  int bb_num;
  int change;
  list_p *succs;
  register set regs_set;
  register set regs_used;
  register set in_bb;
  register set out_bb;
  register set gen_bb;
  register set kill_bb;
  register int *n_sets_in_bb;
  int i;

  /* allocate n_sets_in_bb */
  lv_n_sets_in_bb = (int *)xmalloc(N_BLOCKS*
                                FIRST_PSEUDO_REGISTER*sizeof(int));
  bzero(lv_n_sets_in_bb, N_BLOCKS*FIRST_PSEUDO_REGISTER*sizeof(int));

  /* allocate gen, kill, in, out for reaching defs. */
  set_size = SET_SIZE(FIRST_PSEUDO_REGISTER);
  tlv_buf  = lv_buf = ALLOC_SETS((N_BLOCKS * 4), set_size);
  out      = lv_out = (set *)xmalloc((N_BLOCKS * 4) * sizeof(set));
  in       = out + N_BLOCKS;
  gen      = in + N_BLOCKS;
  kill     = gen + N_BLOCKS;

  regs_set = s_regs_insn_sets;
  regs_used = s_regs_insn_uses;

  /*
   * initialize gen, kill, in, out.
   *
   * gen is the set of all registers that are used before they are
   * redefined within the basic block.
   *
   * kill is the set of all registers that are defined before they are
   * used within the basic block.
   * 
   * in is initialized to the gen set for the basic block.
   *
   * out is initialized to the empty set.
   */
  for (bb_num = N_BLOCKS - 1; bb_num >= 0; bb_num--)
  {
    rtx t_insn = BLOCK_END(bb_num);

    n_sets_in_bb = &lv_n_sets_in_bb[bb_num * FIRST_PSEUDO_REGISTER];

    in[bb_num]   = in_bb   = tlv_buf; tlv_buf += set_size;
    out[bb_num]  = out_bb  = tlv_buf; tlv_buf += set_size;
    gen[bb_num]  = gen_bb  = tlv_buf; tlv_buf += set_size;
    kill[bb_num] = kill_bb = tlv_buf; tlv_buf += set_size;

    CLR_SET(gen_bb, set_size);
    CLR_SET(kill_bb, set_size);
    CLR_SET(out_bb, set_size);

    if (bb_num == EXIT_BLOCK)
    {
      /* make g0 - g3 used as return value. */
      SET_SET_ELT(gen_bb, 0);
      SET_SET_ELT(gen_bb, 1);
      SET_SET_ELT(gen_bb, 2);
      SET_SET_ELT(gen_bb, 3);
    }

    while (1)
    {
      /*
       * get the set of registers that are set by this insn.
       * Add them into the kill set for this block.  Any values that
       * are killed, but were genned must be taken out of the gen set.
       */
      insn_reg_use(t_insn, 0, regs_set, regs_used);
      OR_SETS(kill_bb, kill_bb, regs_set, set_size);
      AND_COMPL_SETS(gen_bb, gen_bb, regs_set, set_size);

      /*
       * get the set of registers that are used by this insn.
       * Add them into the set of gens for this block.
       * Also any registers that are now genned must be removed from
       * the kill set.
       */
      OR_SETS(gen_bb, gen_bb, regs_used, set_size);
      AND_COMPL_SETS(kill_bb, kill_bb, regs_used, set_size);

      /*
       * Update the number of sets for each register in this basic block.
       */
      FORALL_SET_BEGIN(regs_set, set_size, i)
        if (i < FIRST_PSEUDO_REGISTER)
          n_sets_in_bb[i]++;
      FORALL_SET_END;

      if (t_insn == BLOCK_HEAD(bb_num))
        break;
      t_insn = PREV_INSN(t_insn);
    }

    COPY_SET(in_bb, gen_bb, set_size);
  }

  /* now run the dataflow */
  /* during this loop regs_used is just used as a temporary set. */
  succs = s_succs;
  do
  {
    change = 0;
    for (bb_num = N_BLOCKS - 1; bb_num >= 0; bb_num --)
    {
      list_p bb_succ;
      int is_equal;

      if ((bb_succ = succs[bb_num]) == 0)
        continue;  /* can only happen with unconnected graph */

      in_bb = in[GET_LIST_SUCC(bb_succ)];
      COPY_SET(regs_used, in_bb, set_size);

      while ((bb_succ = bb_succ->next) != 0)
      {
        in_bb = in[GET_LIST_SUCC(bb_succ)];
        OR_SETS (regs_used, regs_used, in_bb, set_size);
      }

      out_bb = out[bb_num];
      EQUAL_SETS(regs_used, out_bb, set_size, is_equal);

      if (!is_equal)
      {
        COPY_SET (out_bb, regs_used, set_size);

        /* generate new in. */
        in_bb  = in[bb_num];
        kill_bb = kill[bb_num];
        gen_bb  = gen[bb_num];
        AND_COMPL_SETS(in_bb, out_bb, kill_bb, set_size);
        OR_SETS(in_bb, in_bb, gen_bb, set_size);

        change = 1;
      }
    }
  } while (change);
}

cnstreg_cand_crosses_call(cands, n_cands, loop_info)
cand_ptr cands;
int n_cands;
loop_rec *loop_info;
{
  int i;
  int bb_num;
  unsigned char *calls_in_bb;
  unsigned char any_calls = 0;

  calls_in_bb = (unsigned char *)xmalloc(N_BLOCKS);
  for (bb_num = 0; bb_num < N_BLOCKS; bb_num++)
  {
    rtx t_insn = BLOCK_HEAD(bb_num);
    calls_in_bb[bb_num] = 0;

    while (1)
    {
      if (GET_CODE(t_insn) == CALL_INSN)
      {
        calls_in_bb[bb_num] = 1;
        any_calls = 1;
        break;
      }

      if (t_insn == BLOCK_END(bb_num))
        break;
      t_insn = NEXT_INSN(t_insn);
    }
  }

  if (!any_calls)
  {
    for (i = 0; i < n_cands; i++)
      cands[i].rewrite_crosses_call = 0;
  }
  else
  {
    for (i = 0; i < n_cands; i++)
      cands[i].rewrite_crosses_call = -1;

    for (i = 0; i < n_cands; i++)
    {
      if (cands[i].rewrite_crosses_call == -1)
      {
        int loop_top_bb = cands[i].loop_top_bb;
        flex_set loop_members = loop_info[loop_top_bb].members;
        int loop_has_call = 0;
        int j;

        for (bb_num = next_flex(loop_members, -1); bb_num != -1;
             bb_num = next_flex(loop_members, bb_num))
        {
          if (calls_in_bb[bb_num])
          {
            loop_has_call = 1;
            break;
          }
        }

        cands[i].rewrite_crosses_call = loop_has_call;
        for (j = i+1; j < n_cands; j++)
        {
          if (cands[j].rewrite_crosses_call == -1 &&
              cands[j].loop_top_bb == loop_top_bb)
            cands[j].rewrite_crosses_call = loop_has_call;
        }
      }
    }
  }

  free(calls_in_bb);
}

static void
move_insn_out_of_loop (insn, loop_top)
rtx insn;
int loop_top;  /* basic block number of the top of the loop */
{
  rtx insn_pat;
  int num_moves = 0;
  list_p l_preds;
  int loop_pred;
  flex_set loop_members;

  insn_pat = PATTERN(insn);

  /* delete the insn from inside the loop */
  PUT_CODE(insn, NOTE);
  NOTE_LINE_NUMBER (insn) = NOTE_INSN_DELETED;
  NOTE_SOURCE_FILE (insn) = 0;

  if (loop_top == ENTRY_BLOCK)
    abort();

  loop_members = df_data.maps.loop_info[loop_top].members;
  for (l_preds = s_preds[loop_top]; l_preds != 0; l_preds = l_preds->next)
  {
    loop_pred = GET_LIST_PRED(l_preds);

    if (!in_flex(loop_members, loop_pred))
    {
      rtx t_insn;

      if (num_moves > 0)
        insn_pat = copy_rtx(insn_pat);

      /* find place in basic block to put the new insn */
      /*
       * put it at the end of the basic block, but before any jump
       * instructions.
       */
      t_insn = BLOCK_END(loop_pred);
      if (GET_CODE(t_insn) == JUMP_INSN)
        emit_insn_before(insn_pat, t_insn);
      else
        BLOCK_END(loop_pred) = emit_insn_after(insn_pat, t_insn);

      num_moves ++;
    }
  }
}

static int
rewrite_equivs(cand, repl_reg)
cand_ptr cand;
int repl_reg;
{
  cand_ptr cands = s_cands;
  int      n_cands = s_n_cands;
  int i;
  int n_rewritten = 0;

  for (i = 0; i < n_cands; i++)
  {
    if (!cands[i].moved && cands[i].rewritable && cands[i].assoc_with == 0)
    {
      /* See if they have the same constant value, and loop top. */
      if (cands[i].loop_top_bb == cand->loop_top_bb &&
          GET_MODE(SET_DEST(PATTERN(cands[i].cand_insn))) ==
          GET_MODE(SET_DEST(PATTERN(cand->cand_insn))) &&
          rtx_equal_p(SET_SRC(PATTERN(cands[i].cand_insn)),
                      SET_SRC(PATTERN(cand->cand_insn))))
      {
        int is_equiv = 1;
        int j;

        for (j = 1; j < 4; j++)
        {
          cand_ptr tcand1 = cands[i].assoc_defs[j];
          cand_ptr tcand2 = cand->assoc_defs[j];
          if (tcand1 != 0 &&
              (tcand2 == 0 ||
               !rtx_equal_p(SET_SRC(PATTERN(tcand1->cand_insn)),
                            SET_SRC(PATTERN(tcand2->cand_insn)))))
            is_equiv = 0;
        }

        if (is_equiv)
        {
          for (j = 0; j < 4; j++)
          {
            list_p uses;
            cand_ptr t_cand = cands[i].assoc_defs[j];
            if (t_cand == 0)
              break;

            /*
             * Now replace all uses of the register that this reaches with the
             * newly chosen register.
             */
            for (uses = t_cand->uses; uses != 0; uses = uses->next)
            {
              rtx *x_p = GET_LIST_USE(uses);
              *x_p = gen_rtx(REG, GET_MODE(*x_p), repl_reg+j);
            }

            /*
             * now delete the candidate instruction.
             */
            PUT_CODE(t_cand->cand_insn, NOTE);
            NOTE_LINE_NUMBER (t_cand->cand_insn) = NOTE_INSN_DELETED;
            NOTE_SOURCE_FILE (t_cand->cand_insn) = 0;

            t_cand->moved = 1;
            n_rewritten += 1;
          }
        }
      }
    }
  }
  return n_rewritten;
}

static int
try_rewrite(cand)
cand_ptr cand;
{
  /*
   * Try to move the candidate instruction into the loop header(s).
   * Do this by allocating a previously unallocated register, and
   * rewriting all the uses of the constant def to use this new
   * register, rewriting the def to use the register, then moving
   * the def into the loop header.  This can be done whenever there
   * is an unallocated register, and all uses that are reached by the def
   * are reached only by that def.  Also note that we must be careful not
   * to allocate a call-clobbered register if the loop body contains any
   * calls.
   */
  enum machine_mode reg_mode;
  int repl_reg;
  enum machine_mode repl_mode;
  rtx repl_reg_rtx;
  list_p uses;
  int i;
  int n_rewritten = 0;
  
  if (!cand->rewritable ||
      cand->assoc_with != 0 ||
      cand->moved)
    return 0;

  /* if there are no uses, then this instruction can just be deleted. */
  if ((uses = cand->uses) == 0)
  {
    PUT_CODE(cand->cand_insn, NOTE);
    NOTE_LINE_NUMBER (cand->cand_insn) = NOTE_INSN_DELETED;
    NOTE_SOURCE_FILE (cand->cand_insn) = 0;
    cand->moved = 1;
    return 1;
  }

  /* first find mode of register that is necessary. */
  /* this involves looking at all uses and finding the one with the biggest
   * mode. */
  repl_mode = SImode;
  for (; uses != 0; uses = uses->next)
  {
    enum machine_mode use_mode = GET_MODE(*GET_LIST_USE(uses));
    if (((int)use_mode) > ((int)repl_mode))
      repl_mode = use_mode;
  }

  repl_reg = FIND_UNUSED_REGISTER(repl_mode, cand->rewrite_crosses_call);
  if (repl_reg < 0)
    return 0;

  for (i = 0; i < 4; i++)
  {
    cand_ptr t_cand = cand->assoc_defs[i];

    if (t_cand == 0)
      break;

    reg_mode  = GET_MODE(SET_DEST(PATTERN(t_cand->cand_insn)));
    /*
     * Now replace all uses of the register that this reaches with the
     * newly chosen register.
     */
    for (uses = t_cand->uses; uses != 0; uses = uses->next)
    {
      rtx *x_p = GET_LIST_USE(uses);
      *x_p = gen_rtx(REG, GET_MODE(*x_p), repl_reg+i);
    }

    /* mark candidate as moved here so that rewrite won't try to do it. */
    t_cand->moved = 1;
    n_rewritten += 1;

    /*
     * Search and replace any equivalent candidates.
     */
    n_rewritten += rewrite_equivs(t_cand, repl_reg+i);

    /*
     * now rewrite the candidate def insn with the new register.
     */
    SET_DEST(PATTERN(t_cand->cand_insn)) = gen_rtx(REG, reg_mode, repl_reg+i);

    /*
     * now move the instruction as far out of the loop as possible.  If it
     * is within nested loops move it out as far as possible, provided the
     * expect stays profitable.
     */
    move_insn_out_of_loop(t_cand->cand_insn, t_cand->loop_top_bb);
 
    /* update regs_ever_live */
    regs_ever_live[repl_reg+i] = 1;
  }

  /* return the number of candidates rewritten */
  return n_rewritten;
}

static void
update_reg_dead_info(cand, loop_pred)
cand_ptr cand;
int loop_pred;
{
  register flex_set loop_members;
  register int mem_bb;
  register int t_rgno;
  register rtx rg_note;
  register rtx prev_note;
  register rtx t_insn;
  register set bb_lv_out;
  int rg_begin;
  int rg_end;

  loop_members = df_data.maps.loop_info[cand->loop_top_bb].members;
  rg_begin = cand->dreg;
  rg_end = rg_begin + cand->cand_size;

  /*
   * mark the lv_out set of the loop predecessor node to show
   * that the registers are live.
   */
  bb_lv_out = lv_out[loop_pred];
  for (t_rgno = rg_begin; t_rgno < rg_end; t_rgno++)
    SET_SET_ELT(bb_lv_out, t_rgno);

  /*
   * Run through all basic blocks that are members of the loop.
   * For each insn in each basic block, check if it has a REG_DEAD note
   * on it for the register(s) that we moved, and if so delete the
   * REG_DEAD note.
   * Also for each basic block mark the live_out set of the basic block
   * to indicate that the registers moved are now live.
   */
  for (mem_bb = next_flex(loop_members, -1); mem_bb != -1;
       mem_bb = next_flex(loop_members, mem_bb))
  {
    bb_lv_out = lv_out[mem_bb];
    for (t_rgno = rg_begin; t_rgno < rg_end; t_rgno++)
      SET_SET_ELT(bb_lv_out, t_rgno);

    t_insn = BLOCK_HEAD(mem_bb);

    while (1)
    {
      if (GET_CODE(t_insn) == INSN ||
          GET_CODE(t_insn) == JUMP_INSN ||
          GET_CODE(t_insn) == CALL_INSN)
      {
        for (prev_note = 0, rg_note = REG_NOTES(t_insn); rg_note != 0;
             prev_note = rg_note, rg_note = XEXP(rg_note, 1))
        {
          if (REG_NOTE_KIND(rg_note) == REG_DEAD)
          {
            rtx rg;
            if (GET_CODE((rg = XEXP(rg_note,0))) == REG &&
                REGNO(rg) >= rg_begin && REGNO(rg) < rg_end)
            {
              /* delete it. */
              if (prev_note == 0)
                REG_NOTES(t_insn) = XEXP(rg_note, 1);
              else
                XEXP(prev_note, 1) = XEXP(rg_note, 1);
            }
          }
        }
      }

      if (t_insn == BLOCK_END(mem_bb))
        break;
      t_insn = NEXT_INSN(t_insn);
    }
  }
}

static int
try_move(cand)
cand_ptr cand;
{
  register int t_rgno;
  register int t_rg_begin;
  register int t_end;
  register int pred_bbnum;

  register list_p preds;
  register flex_set loop_members;
  register int mem_bb;
  register int *n_sets;
  register rtx t_insn;
  register set regs_used;
  register set regs_set;
  register set out_bb_lv;

  rtx new_insn;

  if (cand->moved)
    return 0;

  if (cand->loop_top_bb == ENTRY_BLOCK)
    abort();

  preds = s_preds[cand->loop_top_bb];
  loop_members = df_data.maps.loop_info[cand->loop_top_bb].members;
  n_sets = lv_n_sets_in_bb;
  t_rg_begin = cand->dreg;
  t_end = t_rg_begin + cand->cand_size;

  /*
   * Try to simply move the candidate into the loop header.
   * For simplicity, we won't attempt this if there is more than
   * one predecessor of the top of the loop that is outside the loop.
   */

  /*
   * First need to make sure that there are no other sets to the
   * register within the loop.
   */
  for (mem_bb = next_flex(loop_members, -1); mem_bb != -1;
       mem_bb = next_flex(loop_members, mem_bb))
  {
    for (t_rgno = t_rg_begin; t_rgno < t_end; t_rgno++)
    {
      if (n_sets[mem_bb*FIRST_PSEUDO_REGISTER+t_rgno] <= 1)
      {
        if (n_sets[mem_bb*FIRST_PSEUDO_REGISTER+t_rgno] == 0)
          continue;

        /*
         * there is one set. make sure it is the one caused by
         * the candidate.
         */
        if (mem_bb == BLOCK_NUM(cand->cand_insn))
          continue;
      }

      return 0;
    }
  }

  /*
   * find the single predecessor of the loop top that is outside of
   * the loop.  Record the basic block number of the predecessor.  If
   * one isn't found then return.
   */

  pred_bbnum = -1;
  for (; preds != 0; preds = preds->next)
  {
    int tmp_bb;
    tmp_bb = GET_LIST_PRED(preds);

    if (!in_flex(loop_members, tmp_bb))
    {
      if (pred_bbnum != -1)
        return 0;
      pred_bbnum = tmp_bb;
    }
  }

  /*
   * We now have a single predecessor. We must make sure that the register(s)
   * we care about are dead at the end of the basic blockt.  Then we must
   * move backwards through the basic block until we can place the
   * instruction assuring that the registers are dead at every instruction.
   */
  t_insn = BLOCK_END(pred_bbnum);
  regs_used = s_regs_insn_uses;
  regs_set  = s_regs_insn_sets;
  out_bb_lv = lv_out[pred_bbnum];

  while (1)
  {
    for (t_rgno = t_rg_begin; t_rgno < t_end; t_rgno ++)
      if (IN_SET(out_bb_lv, t_rgno))
        return 0;  /* it was live. */

    /*
     * the registers are dead after the current t_insn, see if it is
     * OK to place the candidate here.
     */
    if (GET_CODE(t_insn) != JUMP_INSN &&
        (GET_CODE(t_insn) != INSN || GET_CODE(PATTERN(t_insn)) != SET ||
         GET_CODE(SET_DEST(PATTERN(t_insn))) != CC0))
    {
      /* can place it after the current instruction. */
      new_insn = emit_insn_after(copy_rtx(PATTERN(cand->cand_insn)), t_insn);

      if (t_insn == BLOCK_END(pred_bbnum))
        BLOCK_END(pred_bbnum) = new_insn;

      /* delete the insn from inside the loop */
      t_insn = cand->cand_insn;
      PUT_CODE(t_insn, NOTE);
      NOTE_LINE_NUMBER (t_insn) = NOTE_INSN_DELETED;
      NOTE_SOURCE_FILE (t_insn) = 0;

      update_reg_dead_info(cand, pred_bbnum);

      cand->moved = 1;
      /*
       * Updating cand_insn must be done in case this candidate ends
       * up being associated with another candidate that wasn't
       * moved.
       */
      cand->cand_insn = new_insn;
      return 1;
    }

    /* update the out_bb_lv set */
    insn_reg_use(t_insn, 0, regs_set, regs_used);

    /*
     * new out_bb_lv = (out_bb_lv - regs_set) U regs_used, since
     * know that for all the registers that we care about they are
     * not in out_bb_lv, we don't need to do the set difference.
     */
    OR_SETS(out_bb_lv, out_bb_lv, regs_used, SET_SIZE(FIRST_PSEUDO_REGISTER));

    if (t_insn == BLOCK_HEAD(pred_bbnum))
    {
      /*
       * we can insert the statement before this one provided that
       * the registers are still dead.
       */
      for (t_rgno = t_rg_begin; t_rgno < t_end; t_rgno ++)
        if (IN_SET(out_bb_lv, t_rgno))
          return 0;  /* it was live. */

      if ((t_insn = PREV_INSN(t_insn)) == 0)
        return 0;

      new_insn = emit_insn_after(copy_rtx(PATTERN(cand->cand_insn)), t_insn);

      /* delete the insn from inside the loop */
      t_insn = cand->cand_insn;
      PUT_CODE(t_insn, NOTE);
      NOTE_LINE_NUMBER (t_insn) = NOTE_INSN_DELETED;
      NOTE_SOURCE_FILE (t_insn) = 0;

      update_reg_dead_info(cand, pred_bbnum);

      cand->moved = 1;
      /*
       * Updating cand_insn must be done in case this candidate ends
       * up being associated with another candidate that wasn't
       * moved.
       */
      cand->cand_insn = new_insn;
      return 1;
    }

    /*
     * before we go backwards we must make sure that this insn doesn't
     * set one of the registers we are interested in.
     */
    for (t_rgno = t_rg_begin; t_rgno < t_end; t_rgno ++)
      if (IN_SET(regs_set, t_rgno))
        return 0;  /* it was set. */

    t_insn = PREV_INSN(t_insn);
  }
}

void
const_reg_alloc(insns)
rtx insns;
{
  cand_ptr cands = 0;
  int n_cands;
  int i;
  int n_changes = 0;

  /* set up loop information. */
  get_predecessors(1);

  /* set up predecessor and successor list */
  s_preds = (list_p *)xmalloc(N_BLOCKS * sizeof(list_p));
  s_succs = (list_p *)xmalloc(N_BLOCKS * sizeof(list_p));
  bzero(s_preds, N_BLOCKS * sizeof(list_p));
  bzero(s_succs, N_BLOCKS * sizeof(list_p));
  
  compute_preds_succs(s_preds, s_succs, &list_blks);

  /*
   * Run through all the instructions making an array of candidates for
   * instructions that assign constants to registers that can be moved
   * out of loops.  First call gets a rought number of candidates. Then
   * we allocate space for them.  Second call gets all candidates, fills
   * in the candidate array and prioritizes them.
   */
  n_cands = find_candidates(insns, df_data.maps.loop_info, cands);
  if (n_cands  == 0)
    goto finish;

  cands = (cand_ptr)xmalloc(n_cands * sizeof(struct cand_rec));
  n_cands = find_candidates(insns, df_data.maps.loop_info, cands);
  if (n_cands == 0)
    goto finish;

  s_n_cands = n_cands;
  s_cands = cands;

  /* first see if any of the candidates can be moved without rewriting. */
  cnstreg_lv_dfa(insns);

  n_changes = 0;
  for (i = 0; i < n_cands; i++)
    n_changes += try_move(&cands[i]);

  /* lv sets not needed any further */
  free (lv_out);
  free (lv_buf);

  if (n_changes != n_cands)
  {
    /* now run the necessary dfa for rewriting */
    cnstreg_rd_dfa(insns);
    cnstreg_cand_crosses_call(cands, n_cands, df_data.maps.loop_info);

    /*
     * now try to move instructions out of loops by rewriting the set register
     * with an unallocated register, and replacing the uses with the new
     * register.
     */
    n_changes = 0;
    for (i = 0; i < n_cands; i++)
      n_changes += try_rewrite(&cands[i]);

    /*
     * Its quite possible that the rewriting caused some previously unmovable
     * candidates to be movable.  Lets see.
     */
    if (n_changes != 0 && n_changes != n_cands)
    {
      cnstreg_lv_dfa(insns);

      for (i = 0; i < n_cands; i++)
        try_move(&cands[i]);

      /* lv sets not needed any further */
      free (lv_out);
      free (lv_buf);
    }
  }

  finish:

  if (cands != 0) free(cands);
  free(s_preds);
  free(s_succs);
  free_lists(&list_blks);
  free_all_flow_space();
}
#endif
