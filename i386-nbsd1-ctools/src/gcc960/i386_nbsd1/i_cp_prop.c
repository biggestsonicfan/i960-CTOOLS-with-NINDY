#include "config.h"

#ifdef IMSTG
/*
 * Copy propagation.
 */
#include <setjmp.h>
#include "rtl.h"
#include "regs.h"
#include "flags.h"

#ifdef GCC20
#include "basic-block.h"
#include "hard-reg-set.h"
#include "i_list.h"
#include "i_set.h"
#else
#include "basicblk.h"
#include "hreg_set.h"
#include "list.h"
#include "set.h"
#endif

#ifndef HARD_REG_VAR_ADJ
#define HARD_REG_VAR_ADJ(R,MASK) ((R) = -1, (MASK) = 0)
#endif

static int
calc_reg_var(x, reg_p, regmask_p, do_adj)
rtx x;
int *reg_p;
int *regmask_p;
int do_adj;
{
  int rgno;
  int rgmask;
  int word_size = GET_MODE_SIZE(word_mode);
  rtx r = x;
  enum rtx_code c;

  c = GET_CODE(r);
  while (c==SUBREG       || c==STRICT_LOW_PART ||
         c==ZERO_EXTRACT || c==SIGN_EXTRACT)
    c = GET_CODE (r=XEXP(r,0));

  if (c != REG)
    return 0;

  if (GET_CODE(x) == SUBREG)
    rgmask = (((1 << RTX_BYTES(x)) - 1) << (XINT(x,1) * word_size));
  else
    rgmask = ((1 << RTX_BYTES(r)) - 1);

  rgno = REGNO(r);

  if (do_adj && rgno < FIRST_PSEUDO_REGISTER)
    HARD_REG_VAR_ADJ(rgno, rgmask);

  if (rgmask == 0)
    return 0;

  *reg_p = rgno;
  *regmask_p = rgmask;
  return 1;
}

static list_block_p copy_list_blks;
static list_block_p pred_list_blks;

static list_p *s_preds;

static int word_size;

static void
add_pred_succ(preds, succs, list_blk, pred_bb, succ_bb)
list_p *preds;
list_p *succs;
list_block_p * list_blk;
int pred_bb;
int succ_bb;
{
  register list_p new_node;

  if (preds != 0)
  {
    new_node = alloc_list_node(list_blk);
    SET_LIST_ELT(new_node, pred_bb);
    new_node->next = preds[succ_bb];
    preds[succ_bb] = new_node;
  }

  if (succs != 0)
  {
    new_node = alloc_list_node(list_blk);
    SET_LIST_ELT(new_node, succ_bb);
    new_node->next = succs[pred_bb];
    succs[pred_bb] = new_node;
  }
}

void
compute_preds_succs(preds, succs, list_blk)
list_p *preds;
list_p *succs;
list_block_p * list_blk;
{
  register int i;

  for (i = 0; i < N_BLOCKS; i++)
  {
    if (preds != 0)
      preds[i] = 0;
    if (succs != 0)
      succs[i] = 0;
  }

  for (i = 0; i < N_BLOCKS; i++)
  {
    rtx head;
    rtx jump;

    head = BLOCK_HEAD(i);

    if (GET_CODE(head)==CODE_LABEL)
      for (jump = LABEL_REFS (head); jump != head; jump = LABEL_NEXTREF (jump))
        add_pred_succ(preds, succs, list_blk,
                      BLOCK_NUM(CONTAINING_INSN(jump)), i);

    jump = BLOCK_END(i);
    if ((GET_CODE(jump) == JUMP_INSN && GET_CODE(PATTERN(jump)) == RETURN) ||
        (GET_CODE(jump) != JUMP_INSN && (i == n_basic_blocks-1)))
      add_pred_succ(preds, succs, list_blk, i, EXIT_BLOCK);

    if (BLOCK_DROPS_IN(i))
      add_pred_succ(preds, succs, list_blk, i-1, i);
  }

  add_pred_succ(preds, succs, list_blk, ENTRY_BLOCK, 0);
}

static void
build_cp_prop_bb_info(insns)
rtx insns;
{
  /*
   * get basic block information necessary for running this dataflow.
   * This includes breaking into basic blocks, and getting a way to
   * find the predecessor blocks of a block.  Also need to know the
   * entry basic block number.
   */
  prep_for_flow_analysis (insns);

  /* figure out how many pred nodes are needed. */
  s_preds = (list_p *)xmalloc(N_BLOCKS * sizeof(list_p));

  compute_preds_succs(s_preds, (list_p *)0, &pred_list_blks);
}

struct cp_prop_node
{
  int dreg;            /* destination register number */
  int dreg_mask;       /* destination register mask. */
  int sreg;            /* source register number */
  int sreg_mask;       /* source register mask. */
  rtx src_rtx;         /* rtx for original source operand. */
  rtx insn;            /* insn that is the copy */
  int copy_no;         /* a number that uniquely identifies a copy */
  int copy_size;       /* size in SImodes of the copy */
};

#define GET_LIST_COPY(P) GET_LIST_ELT(P,struct cp_prop_node *)

struct cpy_info
{
  int n_copies;
  struct cp_prop_node *cpy_nodes;
  short *insn_node_no;
  set *in;
  set copyprop_pool;
  int set_size;
  set tmp_set;
  unsigned char *rg_local;
  list_p * copy_uses;
};

static struct cpy_info cp_prop_info;

#ifdef VERY_CONSERVATIVE_PROPAGATION
/*
 * This code all works, but was ifdefed out because the heuristics were
 * much too conservative.
 */
static void
partition_scan_rtx(x, bb_num, last_bb_use, n_sets)
rtx x;
int bb_num;
int *last_bb_use;
int *n_sets;
{
  enum rtx_code code;
  int rgno;
  int rgmask;

  switch (code = GET_CODE(x))
  {
    case SET:
      {
        rtx dst = SET_DEST(x);
        if (GET_CODE(dst) == SUBREG)
          dst = XEXP(x,0);

        if (GET_CODE(dst) == REG)
          n_sets[REGNO(dst)] += 1;
        else
          partition_scan_rtx(dst, bb_num, last_bb_use, n_sets);

        partition_scan_rtx(SET_SRC(x), bb_num, last_bb_use, n_sets);
      }
      break;

    case REG:
      if (calc_reg_var(x, &rgno, &rgmask, 1) == 0)
        break;

      if (last_bb_use[rgno] == -1)
        last_bb_use[rgno] = bb_num;
      else if (last_bb_use[rgno] != bb_num)
        last_bb_use[rgno] = -2;
      break;

    default:
      {
        register char* fmt = GET_RTX_FORMAT (code);
        register int i     = GET_RTX_LENGTH (code);

        while (--i >= 0)
        {
          if (fmt[i] == 'e')
            partition_scan_rtx(XEXP(x,i), bb_num, last_bb_use, n_sets);
          else if (fmt[i] == 'E')
          {
            register int j = XVECLEN(x,i);
            while (--j >= 0)
              partition_scan_rtx(XVECEXP(x,i,j), bb_num, last_bb_use, n_sets);
          }
        }
      }
      break;
  }
}

static void
partition_regs(insns, n_regs)
rtx insns;
int n_regs;
{
  /*
   * partition the registers into ones which are used globally, and
   * ones which are used locally.  A register which is used locally
   * is only used in one basic block, and only has 1 assignment to it.
   * All other registers are considered to be used globally.
   */
  int *n_sets;
  int *last_bb_use;
  unsigned char *rg_local;
  rtx t_insn;
  int i;

  n_sets = (int *)xmalloc(n_regs * sizeof(int));
  last_bb_use = (int *)xmalloc(n_regs * sizeof(int));
  rg_local = (unsigned char *)xmalloc(n_regs * sizeof(unsigned char));

  for (i = 0; i < n_regs; i++)
  {
    n_sets[i] = 0;
    last_bb_use[i] = -1;
    rg_local[i] = 1;
  }

  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    enum rtx_code c;

    c = GET_CODE(t_insn);
    if (c == INSN || c == CALL_INSN || c == JUMP_INSN)
      partition_scan_rtx(PATTERN(t_insn), BLOCK_NUM(t_insn),
                         last_bb_use, n_sets);
  }

  for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
    rg_local[i] = 0;

  for (i = FIRST_PSEUDO_REGISTER; i < n_regs; i++)
  {
    if (last_bb_use[i] == -2 || n_sets[i] > 1)
      rg_local[i] = 0;
  }

  free (last_bb_use);
  free (n_sets);
  cp_prop_info.rg_local = rg_local;
}
#endif

static int
find_bounds(insns, max_uid_p, n_copies_p)
rtx insns;
int *max_uid_p;
int *n_copies_p;
{
  register int n_insns = 0;
  register int max_uid = 0;
  register rtx t_insn;
  register enum rtx_code c;

  /*
   * get quick upper bound on number of copies. Also get
   * maximum uid seen.
   */
  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    if (INSN_UID(t_insn) > max_uid)
      max_uid = INSN_UID(t_insn);

    if (GET_CODE(t_insn) == INSN &&
        GET_CODE(PATTERN(t_insn)) == SET &&
        !INSN_PROFILE_INSTR_P(t_insn) &&
        ((c = GET_CODE(SET_DEST(PATTERN(t_insn)))) == SUBREG || c == REG) &&
        ((c = GET_CODE(SET_SRC(PATTERN(t_insn)))) == SUBREG || c == REG))
      n_insns ++;
  }

  *max_uid_p = max_uid;
  *n_copies_p = n_insns;
}

static void
find_copies(insns)
rtx insns;
{
  register rtx t_insn;
  register struct cp_prop_node * cpy_nodes;
  int tot_copies = 0;
  short *insn_node_no;
  int cur_copy_no = 0;
  int max_uid = 0;

  cpy_nodes = cp_prop_info.cpy_nodes;
  insn_node_no = cp_prop_info.insn_node_no;

  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    int dreg;
    int dreg_mask;
    int sreg;
    int sreg_mask;
    register rtx x;
    register list_p p;

    insn_node_no[INSN_UID(t_insn)] = -1;

    if (GET_CODE(t_insn) != INSN ||
        GET_CODE(PATTERN(t_insn)) != SET ||
        INSN_PROFILE_INSTR_P(t_insn))  /* don't propagate profiling code */
      goto next_insn;

    x = SET_DEST(PATTERN(t_insn));
    switch (GET_CODE(x))
    {
      case SUBREG:
      case REG:
        if (calc_reg_var(x, &dreg, &dreg_mask, 1) == 0)
          goto next_insn;

        if (dreg < FIRST_PSEUDO_REGISTER)
          goto next_insn;
        break;

      default:
        goto next_insn;
    }

    /* now see if the source is register. */
    x = SET_SRC(PATTERN(t_insn));
    switch (GET_CODE(x))
    {
      case SUBREG:
      case REG:
        if (calc_reg_var (x, &sreg, &sreg_mask, 1) == 0)
          goto next_insn;

#ifdef VERY_CONSERVATIVE_PROPAGATION
        if (cp_prop_info.rg_local[sreg] && !cp_prop_info.rg_local[dreg])
          goto next_insn;
#endif

        /*
         * don't bother making a reassignment of the same register a candidate
         * for propagation, its killed right away.
         */
        if (sreg == dreg && (sreg_mask & dreg_mask) != 0)
          goto next_insn;
        break;
  
      default:
        goto next_insn;
    }

    /*
     * we have a candidate.  Add it into the list of copies.
     */
    cpy_nodes[tot_copies].dreg = dreg;
    cpy_nodes[tot_copies].dreg_mask = dreg_mask;
    cpy_nodes[tot_copies].sreg = sreg;
    cpy_nodes[tot_copies].sreg_mask = sreg_mask;
    cpy_nodes[tot_copies].src_rtx = SET_SRC(PATTERN(t_insn));
    cpy_nodes[tot_copies].insn = t_insn;
    cpy_nodes[tot_copies].copy_no = -1;
    cpy_nodes[tot_copies].copy_size = 
        (RTX_BYTES(SET_DEST(PATTERN(t_insn))) + word_size - 1) / word_size;

    /*
     * try to find another copy the is the same.  We can do this easiest
     * by looking down the list of copies of either the source or dest
     * registers.
     */
    for (p = cp_prop_info.copy_uses[dreg]; p != 0; p = p->next)
    {
      if (GET_LIST_COPY(p)->dreg == dreg &&
          GET_LIST_COPY(p)->sreg == sreg &&
          rtx_equal_p(PATTERN(GET_LIST_COPY(p)->insn), PATTERN(t_insn)))
        cpy_nodes[tot_copies].copy_no = GET_LIST_COPY(p)->copy_no;
    }

    if (cpy_nodes[tot_copies].copy_no == -1)
    {
      list_p new_node;
      int hreg;

      cpy_nodes[tot_copies].copy_no = cur_copy_no;
      cur_copy_no += cpy_nodes[tot_copies].copy_size;

      /* add the copy to the kill list for the registers. */
      new_node = alloc_list_node(&copy_list_blks);
      new_node->next = cp_prop_info.copy_uses[dreg];
      SET_LIST_ELT(new_node, &cpy_nodes[tot_copies]);
      cp_prop_info.copy_uses[dreg] = new_node;

      new_node = alloc_list_node(&copy_list_blks);
      new_node->next = cp_prop_info.copy_uses[sreg];
      SET_LIST_ELT(new_node, &cpy_nodes[tot_copies]);
      cp_prop_info.copy_uses[sreg] = new_node;
    }

    insn_node_no[INSN_UID(t_insn)] = tot_copies;
    tot_copies ++;

    next_insn: ;
  }

  cp_prop_info.n_copies = cur_copy_no;
}

static void
calc_copy_kill(kill_set, copy_no, copy_mask, use_mask)
set kill_set;
int copy_no;
int copy_mask;
int use_mask;
{
  /* may only kill part of a copy. */
  int word_mask;

  /*
   * if the use_mask (that which is killed) wholly contains copy_mask, then
   * it means that the whole copy is killed.  However since that which is
   * killed could be larger than the copy gen'd we need to make sure
   * we only kill parts of the copy, and not overflow into some other
   * copies bits.  This is done by setting use_mask to copy_mask if
   * use_mask wholly contains copy_mask.
   */
  if ((copy_mask & use_mask) == copy_mask)
    use_mask = copy_mask;

  /*
   * first we normalize the copy_mask by shifting all the zero bits on
   * the right out.  This makes the copy mask only represent that part
   * of the register actually involved in the copy.  use_mask must be
   * kept in sync.
   */
  word_mask = (1 << word_size) - 1;
  while ((copy_mask & word_mask) == 0)
  {
    copy_mask >>= word_size;
    use_mask >>= word_size;
  }

  /*
   * now we figure how many words of the copy were killed.
   * Each word in the copy is represented by word_size bits set in the
   * mask.  We just loop through seeing with words are set.
   */
  while (use_mask != 0)
  {
    if ((use_mask & word_mask) != 0)
      SET_SET_ELT(kill_set, copy_no);
    use_mask >>= word_size;
    copy_no ++;
  }
}

static void
calc_reg_kill(kill_set, rgno, rgmask)
set kill_set;
register int rgno;
register int rgmask;
{
  register list_p p;

  for (p = cp_prop_info.copy_uses[rgno]; p != 0; p = p->next)
  {
    register struct cp_prop_node * cpy_node;
    cpy_node = GET_LIST_COPY(p);

    if (cpy_node->sreg == rgno)
    {
      if ((cpy_node->sreg_mask & rgmask) != 0)
        calc_copy_kill(kill_set, cpy_node->copy_no,
                       cpy_node->sreg_mask, rgmask);
    }

    if (cpy_node->dreg == rgno)
    {
      if ((cpy_node->dreg_mask & rgmask) != 0)
        calc_copy_kill(kill_set, cpy_node->copy_no,
                       cpy_node->dreg_mask, rgmask);
    }
  }
}

static void
copies_insn_kills(insn, kill_set, set_size)
rtx insn;
set kill_set;
int set_size;
{
  /*
   * build a set with all the copies that this insn kills.
   */
  enum rtx_code c;
  int i;
  int rgno;
  int rgmask;

  CLR_SET(kill_set, set_size);

  switch (GET_CODE(insn))
  {
    default:
      break;

    case CALL_INSN:
      /* make sure all other call used registers are killed. */
      for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
        if (call_used_regs[i] != 0)
        {
          rgno = i;
          rgmask = (1 << word_size) - 1;
          HARD_REG_VAR_ADJ(rgno, rgmask);
          if (rgmask != 0 && rgno >= 0)
            calc_reg_kill(kill_set, rgno, rgmask);
        }
      /* Fall through to let the pattern be analyzed normally */

    case INSN:
      c = GET_CODE(PATTERN(insn));
      if (c == SET || c == CLOBBER)
      {
        if (calc_reg_var(SET_DEST(PATTERN(insn)), &rgno, &rgmask, 1) != 0)
          calc_reg_kill(kill_set, rgno, rgmask);
      }
      else if (c == PARALLEL)
      {
        rtx x = PATTERN(insn);
        int l = XVECLEN(x, 0);
        int i;
        for (i = 0; i < l; i++)
        {
          c = GET_CODE(XVECEXP(x,0,i));
          if (c == SET || c == CLOBBER)
          {
            if (calc_reg_var(SET_DEST(XVECEXP(x,0,i)), &rgno, &rgmask, 1) != 0)
              calc_reg_kill(kill_set, rgno, rgmask);
          }
        }
      }
      break;
  }
}

static void
copies_insn_gens(insn, gen_set, set_size)
rtx insn;
set gen_set;
int set_size;
{
  int node_no;

  CLR_SET(gen_set, set_size);

  if (GET_CODE(insn) == INSN &&
      (node_no = cp_prop_info.insn_node_no[INSN_UID(insn)]) >= 0)
  {
    int copy_size;
    int copy_no;
    int i;

    copy_no = cp_prop_info.cpy_nodes[node_no].copy_no;
    copy_size = cp_prop_info.cpy_nodes[node_no].copy_size;
    for (i = 0; i < copy_size; i++)
    {
      int t = copy_no + i;
      SET_SET_ELT(gen_set, t);
    }
  }
}

static void
copyprop_dfa ()
{
  int n_copies;
  int change;
  set *in;
  set *out;
  set *gen;
  set *kill;
  register set tmp_set;
  list_p *preds;
  int bb_num;
  set copyprop_pool;
  register int set_size;
  register set in_bb;
  register set out_bb;
  register set gen_bb;
  register set kill_bb;

  n_copies = cp_prop_info.n_copies;

  /*
   * allocate gen, kill, in, out.
   */
  in   = cp_prop_info.in;
  out  = in + N_BLOCKS;
  gen  = out + N_BLOCKS;
  kill = gen + N_BLOCKS;

  set_size = SET_SIZE(n_copies);
  copyprop_pool = ALLOC_SETS((N_BLOCKS * 4) + 1, set_size);
  cp_prop_info.copyprop_pool = copyprop_pool;
  cp_prop_info.set_size = set_size;

  /*
   * initialize gen, kill. in, out
   * kill[bb_num] is the set of copies that have their source
   * operand def'ed in bb_num.
   * gen[bb_num] is the set of copies that happen in bb_num and
   * whose source isn't defed later in bb_num.
   */
  tmp_set = copyprop_pool;
  copyprop_pool += set_size;

  for (bb_num=0; bb_num < N_BLOCKS; bb_num++)
  {
    rtx t_insn;

    t_insn = BLOCK_HEAD(bb_num);

    in[bb_num] = in_bb = copyprop_pool;     copyprop_pool += set_size;
    out[bb_num] = out_bb = copyprop_pool;   copyprop_pool += set_size;
    gen[bb_num] = gen_bb = copyprop_pool;   copyprop_pool += set_size;
    kill[bb_num] = kill_bb = copyprop_pool; copyprop_pool += set_size;

    CLR_SET(gen_bb, set_size);
    CLR_SET(kill_bb, set_size);

    while (1)
    {
      /*
       * get the set of copies that the insn kills. Add them into the set
       * of kills for the block and delete them from the set of copies this
       * block gens.
       */
      copies_insn_kills (t_insn, tmp_set, set_size);
      OR_SETS(kill_bb, kill_bb, tmp_set, set_size);
      AND_COMPL_SETS(gen_bb, gen_bb, tmp_set, set_size);

      /*
       * Add the copies this insn gens into the basic block gens.
       */
      copies_insn_gens(t_insn, tmp_set, set_size);
      OR_SETS(gen_bb, gen_bb, tmp_set, set_size);

      if (t_insn == BLOCK_END(bb_num))
        break;
      t_insn = NEXT_INSN(t_insn);
    }

    if (bb_num == ENTRY_BLOCK)
    {
      /* in[0] = {}, out[0] = gen[0] */
      CLR_SET(in_bb, set_size);
      COPY_SET(out_bb, gen_bb, set_size);
    }
    else
    {
      /* in[bb_num] = Universal set, out[bb_num] = U - kill[bb_num] */
      FILL_SET(in_bb, set_size);
      AND_COMPL_SETS(out_bb, in_bb, kill_bb, set_size);
    }
  }

  /* now do the dataflow */
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

        out_bb = out[GET_LIST_ELT(bb_pred, int)];
        COPY_SET(tmp_set, out_bb, set_size);

        while ((bb_pred = bb_pred->next) != 0)
        {
          out_bb = out[GET_LIST_ELT(bb_pred, int)];
          AND_SETS (tmp_set, tmp_set, out_bb, set_size);
        }

        in_bb = in[bb_num];
        EQUAL_SETS(tmp_set, in_bb, set_size, is_equal);

        if (!is_equal)
        {
          COPY_SET (in_bb, tmp_set, set_size);

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

  cp_prop_info.in = in;
  cp_prop_info.tmp_set = tmp_set;
}

static int
copy_replaceable(cpy_node, use_mask, copy_in)
struct cp_prop_node * cpy_node;
int use_mask;
set copy_in;
{
  int copy_no;
  int dreg_mask;
  int word_mask;

  dreg_mask = cpy_node->dreg_mask;
  if ((dreg_mask & use_mask) != use_mask)
    return 0;

  /* shift low order zero representing words out of dreg_mask */
  word_mask = (1 << word_size) - 1;
  while ((dreg_mask & word_mask) == 0)
  {
    dreg_mask >>= word_size;
    use_mask >>= word_size;
  }

  copy_no = cpy_node->copy_no;
  while (use_mask != 0)
  {
    if ((use_mask & word_mask) != 0)
      if (!IN_SET(copy_in, copy_no))
        return 0;

    copy_no++;
    use_mask >>= word_size;
  }

  return 1;
}

int try_sub(insn, repl_p, repl_rtx)
rtx insn;
rtx *repl_p;
rtx repl_rtx;
{
  rtx old_rtx = *repl_p;
  int old_code = INSN_CODE(insn);
  int insn_code;
  int dummy;

  if (old_code < 0)
    old_code = recog(PATTERN(insn), insn, &dummy);

  *repl_p = repl_rtx;
  
  INSN_CODE(insn) = -1;
  insn_code = recog (PATTERN(insn), insn, &dummy);

  if (insn_code < 0 && old_code >= 0)
  {
    INSN_CODE(insn) = old_code;
    *repl_p = old_rtx;
    return 0;
  }

  INSN_CODE(insn) = insn_code;
  return 1;
}

static int 
replace_in_rtx(insn, repl_ok, parent_p, x_p, copy_in)
rtx insn;
int repl_ok;
rtx *parent_p;
rtx *x_p;
set copy_in;
{
  int n_repl = 0;
  rtx x = *x_p;
  enum rtx_code code;

  if (x == 0)
    return 0;

  code = GET_CODE(x);

  switch (code)
  {
    case SET:
      n_repl += replace_in_rtx(insn, 1, x_p, &SET_SRC(x), copy_in);
      n_repl += replace_in_rtx(insn, 0, x_p, &SET_DEST(x), copy_in);
      break;

    case CLOBBER:
    case USE:
      n_repl += replace_in_rtx(insn, 0, x_p, &XEXP(x,0), copy_in);
      break;

    case MEM:
      n_repl += replace_in_rtx(insn, 1, x_p, &XEXP(x,0), copy_in);
      break;

    case REG:
      if (repl_ok)
      {
        register list_p p;
        int rg_no;
        int use_mask;

        /* search the copies to see if there is one that applies. */
        if (GET_CODE(*parent_p) == SUBREG)
        {
          x_p = parent_p;
          x = *x_p;
        }
     
        if (calc_reg_var(x, &rg_no, &use_mask, 1) == 0)
          break;

        for (p = cp_prop_info.copy_uses[rg_no]; p != 0; p = p->next)
        {
          if (GET_LIST_COPY(p)->dreg == rg_no &&
              copy_replaceable(GET_LIST_COPY(p), use_mask, copy_in))
          {
            rtx src_rtx;

            /* it's replaceable */
            src_rtx = GET_LIST_COPY(p)->src_rtx;

            if (GET_MODE(src_rtx) == GET_MODE(x))
            {
              return try_sub(insn, x_p, copy_rtx(src_rtx));
            }
            else
            {
              rtx src_reg;
              int dreg_mask;
              int sreg_mask;
              int word_offset;
              int srg_no;

              if (RTX_BYTES(src_rtx) < RTX_BYTES(x))
                abort();

              src_reg = src_rtx;
              if (GET_CODE(src_rtx) == SUBREG)
                src_reg = XEXP(src_reg,0);

              if (GET_MODE(src_reg) == GET_MODE(x))
              {
                return try_sub(insn, x_p, copy_rtx(src_reg));
              }
              
              dreg_mask = GET_LIST_COPY(p)->dreg_mask;
              word_offset = 0;

              if (calc_reg_var(src_rtx, &srg_no, &sreg_mask, 0) == 0)
                abort();

              while ((dreg_mask & 1) == 0)
              {
                dreg_mask >>= word_size;
                use_mask  >>= word_size;
              }

              while ((sreg_mask & 1) == 0)
              {
                sreg_mask >>= word_size;
                word_offset += 1;
              }

              while ((use_mask & 1) == 0)
              {
                use_mask >>= word_size;
                word_offset += 1;
              }

              return try_sub(insn, x_p,
                             gen_rtx(SUBREG, GET_MODE(x),
                                     copy_rtx(src_reg), word_offset));
            }
          }
        }
      }
      break;

    default:
      {
        register char* fmt = GET_RTX_FORMAT (code);
        register int i     = GET_RTX_LENGTH (code);

        while (--i >= 0)
        {
          if (fmt[i] == 'e')
            n_repl += replace_in_rtx(insn, repl_ok, x_p, &XEXP(x,i), copy_in);
          else if (fmt[i] == 'E')
          {
            register int j = XVECLEN(x,i);
            while (--j >= 0)
              n_repl += replace_in_rtx(insn, repl_ok,
                                       x_p, &XVECEXP(x,i,j), copy_in);

          }
        }
      }
      break;
  }

  return n_repl;
}

static int
replace_in_insn(insn, copy_in)
rtx insn;
set copy_in;
{
  rtx x;
  int any_replaced = 0;

  /*
   * look through instruction and replace any uses of registers that
   * are the left side of copies that reach to to this insn.
   *
   * copy_in is the set of copies that reach this instruction.
   */

  switch (GET_CODE(insn))
  {
    case CALL_INSN:
      if (replace_in_rtx(insn, 1, (rtx *)0, &CALL_INSN_FUNCTION_USAGE(insn),
                     copy_in))
        any_replaced = 1;
      /* Fall-Thru */

    case INSN:
    case JUMP_INSN:
      if (replace_in_rtx(insn, 1, (rtx *)0, &PATTERN(insn), copy_in))
        any_replaced = 1;
      /* any register notes to worry about keeping OK? */
      break;

    default:
      break;
  }
  return any_replaced;
}

static int
copyprop_rewrite()
{
  int bb_num;
  int any_rewrites = 0;
  set tmp_set = cp_prop_info.tmp_set;
  int set_size = cp_prop_info.set_size;
  
  for (bb_num = 0; bb_num < N_BLOCKS; bb_num++)
  {
    set bb_in;
    rtx t_insn;
    rtx end_insn;

    t_insn = BLOCK_HEAD(bb_num);
    end_insn = BLOCK_END(bb_num);
    bb_in = cp_prop_info.in[bb_num];

    while (1)
    {
      /*
       * replace any copies that can be replaced here.
       */
      if (replace_in_insn(t_insn, bb_in))
        any_rewrites = 1;

      /*
       * update bb_in to kill all copies that this insn causes
       * to die.
       */
      copies_insn_kills(t_insn, tmp_set, set_size);
      AND_COMPL_SETS(bb_in, bb_in, tmp_set, set_size);

      /*
       * update bb_in to include any copies that this instruction
       * gens.
       */
      copies_insn_gens(t_insn, tmp_set, set_size);
      OR_SETS(bb_in, bb_in, tmp_set, set_size);

      if (t_insn == end_insn)
        break;

      t_insn = NEXT_INSN(t_insn);
    }
  }

  return any_rewrites;
}

int
do_copyprop(insns)
rtx insns;
{
  int change = 1;
  int n_regs = max_reg_num();
  int max_copies;
  int max_uid;

  if (n_regs == 0)
    return 0;

  word_size = GET_MODE_SIZE(word_mode);

  build_cp_prop_bb_info(insns);
#ifdef VERY_CONSERVATIVE_PROPAGATION
  partition_regs(insns, n_regs);
#endif
  find_bounds(insns, &max_uid, &max_copies);

  if (max_copies == 0)
    return 0;

  cp_prop_info.copy_uses = (list_p *)xmalloc(n_regs * sizeof(list_p));
  cp_prop_info.cpy_nodes = (struct cp_prop_node *)
                           xmalloc(max_copies * sizeof(struct cp_prop_node));
  cp_prop_info.insn_node_no = (short *)xmalloc((max_uid+1) * sizeof(short));
  cp_prop_info.in = (set *)xmalloc(N_BLOCKS * sizeof(set) * 4);

  while (change)
  {
    change = 0;

    bzero(cp_prop_info.copy_uses, n_regs * sizeof(list_p));

    /* find all the copies */
    find_copies(insns);
    
    if (cp_prop_info.n_copies != 0)
    {
      /* run the dataflow */
      copyprop_dfa();

      /* do the propagation */
      change = copyprop_rewrite();

      free(cp_prop_info.copyprop_pool);
    }

    /* free the space used */
    free_lists(&copy_list_blks);
  }

  free(cp_prop_info.in);
  free(cp_prop_info.insn_node_no);
  free(cp_prop_info.cpy_nodes);
  free(cp_prop_info.copy_uses);
#ifdef VERY_CONSERVATIVE_PROPAGATION
  free(cp_prop_info.rg_local);
#endif
  free(s_preds);
  free_lists(&pred_list_blks);

  return 0;
}
#endif
