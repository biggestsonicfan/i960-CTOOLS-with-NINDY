#include "config.h"

#ifdef IMSTG
/*
 * Copy propagation.
 */
#include <setjmp.h>
#include "rtl.h"
#include "regs.h"
#include "flags.h"
#include "real.h"
#include "basic-block.h"
#include "hard-reg-set.h"
#include "i_list.h"
#include "i_set.h"
#include "assert.h"

#ifndef HARD_REG_VAR_ADJ
#define HARD_REG_VAR_ADJ(R,MASK) ((R) = -1, (MASK) = 0)
#endif

int any_control_folded;

static rtx try_cc0_fold();
static rtx cc0_compare_p();
static rtx get_cc_user();

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

static list_block_p cnst_list_blks;
static list_block_p pred_list_blks;

static list_p *s_preds;

static int word_size;

static void
build_cnst_prop_bb_info(insns)
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

struct cnst_prop_node
{
  int dreg;            /* destination register number */
  int dreg_mask;       /* destination register mask. */
  rtx cnst_rtx;        /* rtx for original source operand. */
  rtx insn;            /* insn that is the copy */
  int cnst_no;         /* a number that uniquely identifies a copy */
  int cnst_size;       /* size in SImodes of the constant */
};

#define GET_LIST_CNST(P) GET_LIST_ELT(P,struct cnst_prop_node *)

struct cnst_info
{
  int n_cnsts;		/* number of constants in dataflow sets */
  int n_cnst_nodes;	/* exact number of constant nodes that are valid */
  struct cnst_prop_node *cnst_nodes;  /* the constant nodes */
  short *insn_node_no;  /* a mapping for each insn number of the node number
                           of the constant it generates.  Assumes an insn can
                           generate at most a single constant node. */
  set *in;		/* the dataflow set IN */
  set cnstprop_pool;    /* the allocation for all the dataflow sets */
  int set_size;		/* the size of the dataflow sets */
  set tmp_set;		/* a temporary set needed for various operations */
  list_p * cnst_uses;   /* an array of lists.  For each register number the
                         * list associated with that register numbers is the
                         * node numbers of all constant nodes that use that
                         * register. */
  int * reg_uses;       /* For each register a usage count of the uses of that
                         * register.  We use this to delete constant sets to
                         * a particular register when the number of uses of
                         * the register becomes zero. */
};

static struct cnst_info cnst_prop_info;

/*
 * return true if the rtx represents a constant value, false otherwise.
 */
int
rtx_is_constant_p (x)
rtx x;       /* the rtx we want to know if constant. */
{
  enum rtx_code code;

  switch (code = GET_CODE(x))
  {
    case CONST_DOUBLE:
      /* Only propagate integer calculations. */
      if (GET_MODE(x) != VOIDmode  &&
          GET_MODE_CLASS(GET_MODE(x)) != MODE_INT)
        return 0;
      /* Fall through */
    case CONST_INT:
    case SYMBOL_REF:
    case LABEL_REF:
    case CONST:
      return 1;

    case COMPARE:
    case PLUS:
    case MINUS:
    case NEG:
    case MULT:
    case DIV:
    case MOD:
    case UDIV:
    case UMOD:
    case AND:
    case IOR:
    case XOR:
    case NOT:
    case LSHIFT:
    case ASHIFT:
    case ROTATE:
    case LSHIFTRT:
    case ASHIFTRT:
    case ROTATERT:
    case NE:
    case EQ:
    case GE:
    case GT:
    case LE:
    case LT:
    case GEU:
    case GTU:
    case LEU:
    case LTU:
    case SIGN_EXTEND:
    case ZERO_EXTEND:
    case SIGN_EXTRACT:
    case ZERO_EXTRACT:
    case TRUNCATE:
    case FLOAT:
    case FIX:
    case UNSIGNED_FLOAT:
    case UNSIGNED_FIX:
    {
      char *fmt;
      int i;

      fmt = GET_RTX_FORMAT(code);
      for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
      {
        if (fmt[i] == 'e')
        {
          if (!rtx_is_constant_p(XEXP(x, i)))
            return 0;
        }
      }
      return 1;
    }
    break;

    default:
      return 0;
  }
  return 0;
}

static int
find_bounds(insns, max_uid_p, n_cnsts_p)
rtx insns;
int *max_uid_p;
int *n_cnsts_p;
{
  register int n_insns = 0;
  register int max_uid = 0;
  register rtx t_insn;
  register enum rtx_code c;

  /*
   * get upper bound on number of constants to propagate. Also get
   * maximum uid seen.
   */
  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    int dreg;
    int dreg_mask;
    rtx dst;
    if (GET_CODE(t_insn) == INSN)
      INSN_CODE(t_insn) = -1;

    if (INSN_UID(t_insn) > max_uid)
      max_uid = INSN_UID(t_insn);

    if (GET_CODE(t_insn) == INSN &&
        GET_CODE(PATTERN(t_insn)) == SET &&
        !INSN_PROFILE_INSTR_P(t_insn))  /* don't propagate profiling code */
    {
      dst = SET_DEST(PATTERN(t_insn));

      /* Don't propagate anything but integer constants. */
      if (GET_MODE(dst) == VOIDmode ||
          GET_MODE_CLASS(GET_MODE(dst)) == MODE_INT)
      {
        if ((GET_CODE(dst) == REG ||
             GET_CODE(dst) == SUBREG) &&
            calc_reg_var(dst, &dreg, &dreg_mask, 1) != 0)
        {
          if (rtx_is_constant_p(SET_SRC(PATTERN(t_insn))))
            n_insns ++;
        }
      }
    }
  }

  *max_uid_p = max_uid;
  *n_cnsts_p = n_insns;
}

static int try_cfold();

static void
find_cnsts(insns, fold_first)
rtx insns;
int fold_first;
{
  register rtx t_insn;
  register struct cnst_prop_node * cnst_nodes;
  int tot_cnsts = 0;
  short *insn_node_no;
  int cur_cnst_no = 0;

  cnst_nodes = cnst_prop_info.cnst_nodes;
  insn_node_no = cnst_prop_info.insn_node_no;

  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    int dreg;
    int dreg_mask;
    register rtx dst;
    register list_p p;

    insn_node_no[INSN_UID(t_insn)] = -1;

    if (GET_CODE(t_insn) == INSN &&
        GET_CODE(PATTERN(t_insn)) == SET &&
        !INSN_PROFILE_INSTR_P(t_insn))  /* don't propagate profiling code */
    {
      dst = SET_DEST(PATTERN(t_insn));

      /* Don't propagate anything but integer constants. */
      if (GET_MODE(dst) == VOIDmode ||
          GET_MODE_CLASS(GET_MODE(dst)) == MODE_INT)
      {
        if ((GET_CODE(dst) == REG ||
             GET_CODE(dst) == SUBREG) &&
            calc_reg_var(dst, &dreg, &dreg_mask, 1) != 0)
        {
          if (rtx_is_constant_p(SET_SRC(PATTERN(t_insn))))
          {
            /*
             * we have a candidate.  Add it into the list of cnsts.
             */
            if (fold_first)
              if (try_cfold(PATTERN(t_insn),
                            &SET_SRC(PATTERN(t_insn))-&XEXP(PATTERN(t_insn),0),
                            SET_SRC(PATTERN(t_insn))))
                INSN_CODE(t_insn) = -1;
            cnst_nodes[tot_cnsts].dreg = dreg;
            cnst_nodes[tot_cnsts].dreg_mask = dreg_mask;
            cnst_nodes[tot_cnsts].cnst_rtx = SET_SRC(PATTERN(t_insn));
            cnst_nodes[tot_cnsts].insn = t_insn;
            cnst_nodes[tot_cnsts].cnst_no = -1;
            cnst_nodes[tot_cnsts].cnst_size =
             (RTX_BYTES(SET_DEST(PATTERN(t_insn))) + word_size - 1) / word_size;

            /*
             * try to find another copy the is the same.  We can do this easiest
             * by looking down the list of cnsts of the dest registers.
             */
            for (p = cnst_prop_info.cnst_uses[dreg]; p != 0; p = p->next)
            {
              if (GET_LIST_CNST(p)->dreg == dreg &&
                  rtx_equal_p(PATTERN(GET_LIST_CNST(p)->insn), PATTERN(t_insn)))
                cnst_nodes[tot_cnsts].cnst_no = GET_LIST_CNST(p)->cnst_no;
            }
        
            if (cnst_nodes[tot_cnsts].cnst_no == -1)
            {
              list_p new_node;

              cnst_nodes[tot_cnsts].cnst_no = cur_cnst_no;
              cur_cnst_no += cnst_nodes[tot_cnsts].cnst_size;

              /* add the copy to the kill list for the registers. */
              new_node = alloc_list_node(&cnst_list_blks);
              new_node->next = cnst_prop_info.cnst_uses[dreg];
              SET_LIST_ELT(new_node, &cnst_nodes[tot_cnsts]);
              cnst_prop_info.cnst_uses[dreg] = new_node;
            }

            insn_node_no[INSN_UID(t_insn)] = tot_cnsts;
            tot_cnsts ++;
          }
        }
      }
    }
  }

  cnst_prop_info.n_cnsts = cur_cnst_no;
  cnst_prop_info.n_cnst_nodes = tot_cnsts;
}

static void
calc_cnst_kill(kill_set, cnst_no, cnst_mask, use_mask)
set kill_set;
int cnst_no;
int cnst_mask;
int use_mask;
{
  /* may only kill part of a const. */
  int word_mask;

  /*
   * if the use_mask (that which is killed) wholly contains cnst_mask, then
   * it means that the whole constant is killed.  However since that which is
   * killed could be larger than the constant gen'd we need to make sure
   * we only kill parts of the constant, and not overflow into some other
   * constants bits.  This is done by setting use_mask to cnst_mask if
   * use_mask wholly contains cnst_mask.
   */
  if ((cnst_mask & use_mask) == cnst_mask)
    use_mask = cnst_mask;

  /*
   * first we normalize the cnst_mask by shifting all the zero bits on
   * the right out.  This makes the copy mask only represent that part
   * of the register actually involved in the cnst.  use_mask must be
   * kept in sync.
   */
  word_mask = (1 << word_size) - 1;
  while ((cnst_mask & word_mask) == 0)
  {
    cnst_mask >>= word_size;
    use_mask >>= word_size;
  }

  /*
   * now we figure how many words of the cnst were killed.
   * Each word in the cnst is represented by word_size bits set in the
   * mask.  We just loop through seeing which words are set.
   */
  while (use_mask != 0)
  {
    if ((use_mask & word_mask) != 0)
      SET_SET_ELT(kill_set, cnst_no);
    use_mask >>= word_size;
    cnst_no ++;
  }
}

static void
calc_reg_kill(kill_set, rgno, rgmask)
set kill_set;
register int rgno;
register int rgmask;
{
  register list_p p;

  for (p = cnst_prop_info.cnst_uses[rgno]; p != 0; p = p->next)
  {
    register struct cnst_prop_node * cnst_node;
    cnst_node = GET_LIST_CNST(p);

    if (cnst_node->dreg == rgno)
    {
      if ((cnst_node->dreg_mask & rgmask) != 0)
        calc_cnst_kill(kill_set, cnst_node->cnst_no,
                       cnst_node->dreg_mask, rgmask);
    }
  }
}

static void
cnsts_insn_kills(insn, kill_set)
rtx insn;
set kill_set;
{
  /*
   * build a set with all the constants that this insn kills.
   */
  enum rtx_code c;
  int i;
  int rgno;
  int rgmask;

  switch (GET_CODE(insn))
  {
    default:
      break;

    case CALL_INSN:
      /*
       * First kill all the registers in the call_used_regs.
       * Then look at the pattern of the call to see if there
       * are any kills represented in the pattern.
       */
      /* make sure all call used registers are killed. */
      for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
        if (call_used_regs[i] != 0)
        {
          rgno = i;
          rgmask = (1 << word_size) - 1;
          HARD_REG_VAR_ADJ(rgno, rgmask);
          if (rgmask != 0 && rgno >= 0)
            calc_reg_kill(kill_set, rgno, rgmask);
        }
      /* Fall through to analyze the CALL_INSN's pattern */

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
cnsts_insn_gens(insn, gen_set)
rtx insn;
set gen_set;
{
  int node_no;

  if (GET_CODE(insn) == INSN &&
      (node_no = cnst_prop_info.insn_node_no[INSN_UID(insn)]) >= 0)
  {
    int cnst_size;
    int cnst_no;
    int i;

    cnst_no = cnst_prop_info.cnst_nodes[node_no].cnst_no;
    cnst_size = cnst_prop_info.cnst_nodes[node_no].cnst_size;
    for (i = 0; i < cnst_size; i++)
    {
      int t = cnst_no + i;
      SET_SET_ELT(gen_set, t);
    }
  }
}

static void
cnstprop_dfa ()
{
  int n_cnsts;
  int change;
  set *in;
  set *out;
  set *gen;
  set *kill;
  register set tmp_set;
  list_p *preds;
  int bb_num;
  set cnstprop_pool;
  register int set_size;
  register set in_bb;
  register set out_bb;
  register set gen_bb;
  register set kill_bb;

  n_cnsts = cnst_prop_info.n_cnsts;

  /*
   * allocate gen, kill, in, out.
   */
  in   = cnst_prop_info.in;
  out  = in + N_BLOCKS;
  gen  = out + N_BLOCKS;
  kill = gen + N_BLOCKS;

  set_size = SET_SIZE(n_cnsts);
  cnstprop_pool = ALLOC_SETS((N_BLOCKS * 4) + 1, set_size);
  cnst_prop_info.cnstprop_pool = cnstprop_pool;
  cnst_prop_info.set_size = set_size;

  /*
   * initialize gen, kill. in, out
   * kill[bb_num] is the set of cnsts that have their dest register defined
   * in bb_num.
   * gen[bb_num] is the set of cnsts that happen in bb_num and
   * whose dest registers isn't later defed in bb_num.
   */
  tmp_set = cnstprop_pool;
  cnstprop_pool += set_size;

  for (bb_num=0; bb_num < N_BLOCKS; bb_num++)
  {
    rtx t_insn;

    t_insn = BLOCK_HEAD(bb_num);

    in[bb_num] = in_bb = cnstprop_pool;     cnstprop_pool += set_size;
    out[bb_num] = out_bb = cnstprop_pool;   cnstprop_pool += set_size;
    gen[bb_num] = gen_bb = cnstprop_pool;   cnstprop_pool += set_size;
    kill[bb_num] = kill_bb = cnstprop_pool; cnstprop_pool += set_size;

    CLR_SET(gen_bb, set_size);
    CLR_SET(kill_bb, set_size);

    while (1)
    {
      /*
       * get the set of cnsts that the insn kills. Add them into the set
       * of kills for the block and delete them from the set of cnsts this
       * block gens.
       */
      CLR_SET(tmp_set, set_size);
      cnsts_insn_kills (t_insn, tmp_set);
      OR_SETS(kill_bb, kill_bb, tmp_set, set_size);
      AND_COMPL_SETS(gen_bb, gen_bb, tmp_set, set_size);

      /*
       * Add the cnsts this insn gens into the basic block gens.
       */
      CLR_SET(tmp_set, set_size);
      cnsts_insn_gens(t_insn, tmp_set);
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

  cnst_prop_info.in = in;
  cnst_prop_info.tmp_set = tmp_set;
}

static int
cnst_modes_ok (repl_rtx, const_rtx)
rtx repl_rtx;
rtx const_rtx;
{
  enum machine_mode repl_mode;
  enum machine_mode const_mode;

  repl_mode = GET_MODE(repl_rtx);

  const_mode = GET_MODE(const_rtx);
  if (const_mode == VOIDmode)
  {
    switch (GET_CODE(const_rtx))
    {
      case CONST_INT:
      case CONST_DOUBLE:
        const_mode = word_mode;
        break;

      case CONST:
        const_mode = GET_MODE(XEXP(const_rtx, 0));
        break;

      default:
        break;
    }
  }

  if (repl_mode == VOIDmode || const_mode == VOIDmode)
    return 0;

  /* Only propagate integer calculations. */
  if (GET_MODE_CLASS(repl_mode) != MODE_INT ||
      GET_MODE_CLASS(const_mode) != MODE_INT)
    return 0;

  return 1;
}

static int
cnst_replaceable(cnst_node, use_mask, repl_rtx, cnst_in)
struct cnst_prop_node * cnst_node;
int use_mask;
rtx repl_rtx;
set cnst_in;
{
  int cnst_no;
  int dreg_mask;
  int word_mask;

  dreg_mask = cnst_node->dreg_mask;
  if ((dreg_mask & use_mask) != use_mask)
    return 0;

  if (!cnst_modes_ok(repl_rtx, cnst_node->cnst_rtx))
    return 0;

  word_mask = (1 << word_size) - 1;

  /*
   * Shift out the low order words of the dreg that are unused.
   */
  while ((dreg_mask & word_mask) == 0)
  {
    dreg_mask >>= word_size;
    use_mask >>= word_size;
  }

  /*
   * if only an upper part of the register is set then some bit extraction
   * is necessary on this constant, so only do this if the replacement constant
   * is something we can be guaranteed to be able to fold.
   */
  if (((dreg_mask & 1) == 0) ||
      ((use_mask & 1) == 0) ||
      (dreg_mask != use_mask))
  {
    if (GET_CODE(cnst_node->cnst_rtx) != CONST_INT &&
        GET_CODE(cnst_node->cnst_rtx) != CONST_DOUBLE)
      return 0;
  }

  /* now make sure all the replaceable use is actually available. */
  cnst_no = cnst_node->cnst_no;
  while (use_mask != 0)
  {
    if ((use_mask & word_mask) != 0)
      if (!IN_SET(cnst_in, cnst_no))
        return 0;

    cnst_no++;
    use_mask >>= word_size;
  }

  return 1;
}

static
int do_sub(repl_p, repl_rtx)
rtx *repl_p;
rtx repl_rtx;
{
  *repl_p = repl_rtx;
  return 1;
}

static int 
replace_in_rtx(is_use, repl_ok, parent_p, x_p, cnst_in)
int is_use;
int repl_ok;
rtx *parent_p;
rtx *x_p;
set cnst_in;
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
      n_repl += replace_in_rtx(1, 1, x_p, &SET_SRC(x), cnst_in);
      n_repl += replace_in_rtx(0, 0, x_p, &SET_DEST(x), cnst_in);
      break;

    case CLOBBER:
    case USE:
      n_repl += replace_in_rtx(1, 0, x_p, &XEXP(x,0), cnst_in);
      break;

    case MEM:
      /*
       * We don't bother to replace constants in the address part of
       * a memory.  Even if they do become constant they won't generally
       * allow more constant folding to go on, and better code is generally
       * gotten by having CSE'd the constant addresses out into registers
       * because of the expense in code space of encoding of the addresses.
       *
       * Another reason for not doing this is that we may make a legal memory
       * address and make it illegal, which could cause an abort later
       * on during the compilation.
       */
      n_repl += replace_in_rtx(1, 0, x_p, &XEXP(x,0), cnst_in);
      break;

    case REG:
      if (repl_ok || is_use)
      {
        register list_p p;
        int rg_no;
        int use_mask;

        /* search the cnsts to see if there is one that applies. */
        if (GET_CODE(*parent_p) == SUBREG)
        {
          if (RTX_BYTES(x) < RTX_BYTES(*parent_p) &&
              REGNO(x) >= FIRST_PSEUDO_REGISTER)
          {
            /*
             * This is a paradoxical subreg, with the upper junk being
             * essentially don't cares, so just say the use mask is only
             * as big as the pseudo register.
             */
            if (calc_reg_var(x, &rg_no, &use_mask, 1) == 0)
              break;
          }
          else
            if (calc_reg_var(*parent_p, &rg_no, &use_mask, 1) == 0)
              break;
          x_p = parent_p;
          x = *x_p;
        }
        else
          if (calc_reg_var(x, &rg_no, &use_mask, 1) == 0)
            break;

        /* record the use of this register */
        if (is_use)
          cnst_prop_info.reg_uses[rg_no] += 1;

        /* Only do replacement if its OK */
        if (!repl_ok)
          break;

        for (p = cnst_prop_info.cnst_uses[rg_no]; p != 0; p = p->next)
        {
          if (GET_LIST_CNST(p)->dreg == rg_no &&
              cnst_replaceable(GET_LIST_CNST(p), use_mask, x, cnst_in))
          {
            int dreg_mask;
            rtx cnst_rtx;
            rtx cnst_mask;
            static int byte_mask_lo[8] =
                   { 0xFF, 0xFF00, 0xFF0000, 0xFF000000,
                     0x0,  0x0,    0x0,      0x0} ;
            static int byte_mask_hi[8] =
                   { 0x0,  0x0,    0x0,      0x0,
                     0xFF, 0xFF00, 0xFF0000, 0xFF000000};
            unsigned int cnst_mask_lo;
            unsigned int cnst_mask_hi;
            int i;

            /* it's replaceable */
            cnst_rtx = GET_LIST_CNST(p)->cnst_rtx;
            dreg_mask = GET_LIST_CNST(p)->dreg_mask;

            if (dreg_mask == use_mask)
            {
              cnst_prop_info.reg_uses[rg_no] -= 1;
              return do_sub(x_p, copy_rtx(cnst_rtx));
            }

            /* We'll have to bit extract the constant, and let
               that fold out. */
            /*
             * Shift out the low order words of the dreg that are unused.
             */
            while ((dreg_mask & 1) == 0)
            {
              dreg_mask >>= 1;
              use_mask >>= 1;
            }

            assert(GET_CODE(cnst_rtx) == CONST_INT ||
                   GET_CODE(cnst_rtx) == CONST_DOUBLE);

            cnst_mask_lo = 0;
            cnst_mask_hi = 0;
            i = 0;

            while (use_mask != 0)
            {
              if ((use_mask & 1))
              {
                cnst_mask_lo |= byte_mask_lo[i];
                cnst_mask_hi |= byte_mask_hi[i];
              }
              use_mask >>= 1;
              i += 1;
            }
            
            cnst_mask = immed_double_const(cnst_mask_lo, cnst_mask_hi,
                                           VOIDmode);
            cnst_prop_info.reg_uses[rg_no] -= 1;
            return (do_sub(x_p,
                           gen_rtx(AND, GET_MODE(x), cnst_rtx, cnst_mask)));
          }
        }
      }
      break;

    case COMPARE:
      /*
       * Just need to store first operand's mode as mode of compare
       * for later folding attempts.  Then just fall through to normal
       * processing.
       */
      if (GET_MODE(XEXP(x,0)) != VOIDmode)
        COMPARE_MODE(x) = GET_MODE(XEXP(x,0));
      /* Fall-through */

    default:
      {
        register char* fmt = GET_RTX_FORMAT (code);
        register int i     = GET_RTX_LENGTH (code);

        while (--i >= 0)
        {
          if (fmt[i] == 'e')
            n_repl += replace_in_rtx(is_use, repl_ok,
                                     x_p, &XEXP(x,i), cnst_in);
          else if (fmt[i] == 'E')
          {
            register int j = XVECLEN(x,i);
            while (--j >= 0)
              n_repl += replace_in_rtx(is_use, repl_ok,
                                       x_p, &XVECEXP(x,i,j), cnst_in);

          }
        }
      }
      break;
  }

  return n_repl;
}

static int fold_after_replacement();
static int
replace_in_insn(insn, cnst_in)
rtx insn;
set cnst_in;
{
  int any_replaced = 0;

  /*
   * look through instruction and replace any uses of registers that
   * are the left side of cnsts that reach to to this insn.
   *
   * cnst_in is the set of cnsts that reach this instruction.
   */

  switch (GET_CODE(insn))
  {
    case CALL_INSN:
      replace_in_rtx(1, 1, NULL_RTX, &CALL_INSN_FUNCTION_USAGE(insn), cnst_in);
      /* Fall-Thru */

    case INSN:
    case JUMP_INSN:
      /*
       * First try to replace in the register notes, so that we keep
       * these up-to-date.
       */
      replace_in_rtx(1, 1, NULL_RTX, &REG_NOTES(insn), cnst_in);

      if (replace_in_rtx(1, 1, NULL_RTX, &PATTERN(insn), cnst_in))
      {
        any_replaced = 1;
        INSN_CODE(insn) = -1;

        /* now do any folding that is possible on this insn */
        fold_after_replacement(0, 0, PATTERN(insn));
      }
      break;

    default:
      break;
  }
  return any_replaced;
}


/*
 * Mark this instruction as deleted.  If it is an instruction with
 * a REG_RETVAL note, then delete the whole libcall.  If it is an
 * instruction with a REG_LIBCALL note, don't really delete it, otherwise
 * simply mark the instruction as deleted.
 */
void
imstg_mark_delete(t_insn)
rtx t_insn;
{
  rtx retval_note;
  if ((retval_note = find_reg_note (t_insn, REG_RETVAL, 0)) != 0)
  {
    rtx libcall_insn = XEXP(retval_note, 0);;
    assert(libcall_insn != 0);

    for (; t_insn != 0; t_insn = PREV_INSN(t_insn))
    {
#if defined(IMSTG) && defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
      if (flag_linenum_mods)
        dwarfout_disable_notes(t_insn);
#endif
      PUT_CODE(t_insn, NOTE);
      NOTE_LINE_NUMBER (t_insn) = NOTE_INSN_DELETED;
      NOTE_SOURCE_FILE (t_insn) = 0;
      NOTE_CALL_COUNT (t_insn) = 0;
    
      if (t_insn == libcall_insn)
        break;
    }
  }
  else if (find_reg_note (t_insn, REG_LIBCALL, 0) == 0)
  {
#if defined(IMSTG) && defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
    if (flag_linenum_mods)
      dwarfout_disable_notes(t_insn);
#endif
    PUT_CODE(t_insn, NOTE);
    NOTE_LINE_NUMBER (t_insn) = NOTE_INSN_DELETED;
    NOTE_SOURCE_FILE (t_insn) = 0;
    NOTE_CALL_COUNT (t_insn) = 0;
  }
}

/*
 * Return 1 if this is a comparison instruction, 0 otherwise.
 */
int
compare_p(insn)
rtx insn;
{
  rtx dst;
  rtx src;

  if (insn == 0 || GET_CODE(insn) != INSN || GET_CODE(PATTERN(insn)) != SET)
    return 0;

  dst = SET_DEST(PATTERN(insn));
  src = SET_SRC(PATTERN(insn));
  if ((dst == cc0_rtx ||
       (GET_CODE(dst) == REG && GET_MODE_CLASS(GET_MODE(dst)) == MODE_CC)) &&
      ((GET_CODE(src) == COMPARE &&
        GET_MODE_CLASS(GET_MODE(XEXP(src,0))) == MODE_INT) ||
       GET_MODE_CLASS(GET_MODE(src)) == MODE_INT))
    return 1;
  return 0;
}

/*
 * Return 1 is this basic-block ends in a conditional jump.  The basic-block
 * must have <= instr_limit instructions excluding the comparison and jump
 * instructions.  If this routine returns 1, it also returns the rtx insn
 * for the comparison, the jump_insn, and returns the set of constants that
 * are killed by all instructions between insn and the comparison insn.
 * If all these conditions do not hold true return 0.
 */
static int
condjump_safe_bb(insn, instr_limit, cmp_ptr, jmp_ptr, kill_set, set_size)
rtx insn;
int instr_limit;
rtx *cmp_ptr;
rtx *jmp_ptr;
set kill_set;
int set_size;
{
  rtx last_bb_insn;
  rtx t_insn;
  int n_others_in_block;

  if (insn == 0)
    return 0;

  /* find the last instruction in the basic-block */
  last_bb_insn = BLOCK_END(BLOCK_NUM(insn));

  /* if its not a conditional jump then don't bother */
  if (GET_CODE(last_bb_insn) != JUMP_INSN ||
      simplejump_p(last_bb_insn) || !condjump_p(last_bb_insn))
    return 0;

  /*
   * next we need to look back for the comparison.  On the way back
   * count any instructions we may encounter that aren't the comparison.
   */
  n_others_in_block = 0;
  t_insn = PREV_INSN(last_bb_insn);
  while (1)
  {
    if (compare_p(t_insn))
      break;

    if (GET_RTX_CLASS(GET_CODE(t_insn)) == 'i')
      n_others_in_block ++;

    if (t_insn == insn ||
        n_others_in_block > instr_limit)
      return 0;

    t_insn = PREV_INSN(t_insn);
  }

  /*
   * Make sure there are no instructions between the comparison and
   * the jump that use the condition code.  If there are then
   * this optimization will not be OK.
   */
  if (get_cc_user(t_insn, cc0_compare_p(PATTERN(t_insn))) != last_bb_insn)
    return 0;

  /*
   * If we got here, t_insn is the comparison, so now we just need to build
   * up the kill set and count the instructions.
   */
  *cmp_ptr = t_insn;
  *jmp_ptr = last_bb_insn;

  CLR_SET(kill_set, set_size);

  if (t_insn == insn)
    return 1;

  t_insn = PREV_INSN(t_insn);

  while (1)
  {
    if (GET_RTX_CLASS(GET_CODE(t_insn)) == 'i')
    {
      cnsts_insn_kills(t_insn, kill_set);
      n_others_in_block += 1;
      if (n_others_in_block > instr_limit)
        return 0;
    }

    if (t_insn == insn)
      break;

    t_insn = PREV_INSN(t_insn);
  }

  return 1;
}

/*
 * On entry cond_insn is the comparison insn of a basic block.
 * cond_user is the jump instruction that uses the result of the comparison.
 * We want to try to fold the the basic block assuming the dataflow
 * state represented by cnst_in.
 * 
 * However this code is not allowed to modify any of the instructions
 * that exist here, since the dataflow state represented by cnst_in may
 * not be true for all flow paths that reach this code.
 *
 * Therefore we must make copies or somehow do the folding without
 * modifying these instructions.
 *
 * The return value from this routine is a label representing where
 * the folded flow would end up if the folding is doable.
 * 0 is returned if the folding cannot be done.
 */
static rtx
fold_cond_bb(cond_insn, cond_user, cnst_in)
rtx cond_insn;
rtx cond_user;
set cnst_in;
{
  rtx cond_copy = copy_rtx(PATTERN(cond_insn));

  /*
   * now try doing constant replacement in the copied pattern.
   */
  if (replace_in_rtx(1, 1, NULL_RTX, &cond_copy, cnst_in))
  {
    rtx new_rtx;
    rtx ret_lab;
    fold_after_replacement(0, 0, cond_copy);

    if ((new_rtx = try_cc0_fold(cond_copy, cond_user)) != 0)
    {
      /*
       * its foldable, figure out to rewrite.
       */
      if (GET_CODE(new_rtx) == PC)
        /* fall thru to next insn after cond_user */
        ret_lab = get_label_after(cond_user);
      else if (GET_CODE(new_rtx) == LABEL_REF)
        ret_lab = XEXP(new_rtx, 0);
      else
        abort();

      return ret_lab;
    }
  }
  return 0;
}

/*
 * this is a routine to handle all the grossness and hacks
 * having to do with duplicating rtl operations
 * Also this routine is funny in that it doesn't duplicate
 * the insns indicated by cmp_insn and last_insn.
 *
 * cnst_in is the set of constants that are ok when we start the copy,
 * we update this set as we are going through and try to do some constant
 * folding during the copy.  This is necessary to keep the register use
 * information up-to-date.
 *
 * tmp_set is just a set passed in for our use.
 *
 * Return the last instruction that was emitted.
 */
static rtx
copy_block_after (first_insn, last_insn, after, cmp_insn,
                  cnst_in, tmp_set, set_size)
rtx first_insn;
rtx last_insn;
rtx after;
rtx cmp_insn;
set cnst_in;
set tmp_set;
int set_size;
{
  rtx retval_note;
  rtx libcall_note;
  rtx libcall_insn = 0;

  rtx source = first_insn;
  rtx copy = after;

  /*
   * We are trying to replace constants in the rtx while we
   * are copying the rtx. This means that we must keep 
   */
  for (source = first_insn; source != last_insn; source = NEXT_INSN(source))
  {
    if (source != cmp_insn)
    {
      if (GET_CODE(source) == NOTE && NOTE_LINE_NUMBER(source) > 0)
      {
        copy = emit_note_after (NOTE_LINE_NUMBER (source), after);
        NOTE_SOURCE_FILE (copy) = NOTE_SOURCE_FILE (source);
        NOTE_TRIP_COUNT (copy) = NOTE_TRIP_COUNT (source);
        after = copy;
      }
      else if (GET_RTX_CLASS(GET_CODE(source)) == 'i')
      {
        rtvec orig_asm_ops;
        rtvec copy_asm_ops;
        rtvec copy_asm_cons;

        /*
         * generate a copy and paste it into the source, getting an insn
         */
        orig_asm_ops = 0;
        copy = imstg_copy_rtx (PATTERN (source),
                               &orig_asm_ops,
                               &copy_asm_ops,
                               &copy_asm_cons);

        switch (GET_CODE (source))
        {
          case CALL_INSN:
            copy = emit_call_insn_after (copy, after);
            CALL_INSN_FUNCTION_USAGE (copy) =
                  imstg_copy_rtx(CALL_INSN_FUNCTION_USAGE(source),
                                 &orig_asm_ops,
                                 &copy_asm_ops,
                                 &copy_asm_cons);
            break;

          case INSN:
            copy = emit_insn_after (copy, after);
            break;

          default:
            abort();
        }

        /*
         * update the many magic fields
         */
        INSN_CODE (copy) = -1;
        LOG_LINKS (copy) = NULL;

        /*
         * hack for REG_LIBCALL not getting created properly.
         */
        if (REG_NOTES(source))
        {
          orig_asm_ops = 0;
          REG_NOTES (copy) = imstg_copy_rtx(REG_NOTES(source),
                                            &orig_asm_ops,
                                            &copy_asm_ops,
                                            &copy_asm_cons);
          if (!libcall_insn)
          {
            if (find_reg_note(copy, REG_LIBCALL, 0))
              libcall_insn = copy;
          }
          else
          {
            retval_note = find_reg_note (copy, REG_RETVAL, 0);
            if ( retval_note ) {
              libcall_note = find_reg_note (libcall_insn, REG_LIBCALL, 0);
              assert ( libcall_note );
              XEXP (libcall_note, 0) = copy;
              XEXP (retval_note, 0) = libcall_insn;
              libcall_insn = 0;
            }
          }
          replace_in_rtx(1, 1, NULL_RTX, &REG_NOTES(copy), cnst_in);
        }

        after = copy;

        /* Now try to fold the copied instruction. */
        if (replace_in_rtx(1, 1, NULL_RTX, &PATTERN(copy), cnst_in))
        {
          /* now do any folding that is possible on this insn */
          fold_after_replacement(0, 0, PATTERN(copy));
        }

        /*
         * now update the dataflow set.  This must be done using the
         * original instruction not the copy since no dataflow info exists
         * for the copy.
         */
        /*
         * update cnst_in to kill all cnsts that this insn causes
         * to die.
         */
        CLR_SET(tmp_set, set_size);
        cnsts_insn_kills(source, tmp_set);
        AND_COMPL_SETS(cnst_in, cnst_in, tmp_set, set_size);

        /*
         * update cnst_in to include any cnsts that this instruction
         * gens.
         */
        CLR_SET(tmp_set, set_size);
        cnsts_insn_gens(source, tmp_set);
        OR_SETS(cnst_in, cnst_in, tmp_set, set_size);
      }
    }
  }
  return copy;
}

/*
 * return true if from_bb is the only predecessor block of follow_bb.
 */
static int
only_pred_bb_p(follow_bb, from_bb)
rtx follow_bb;
rtx from_bb;
{
  list_p pred;

  if (follow_bb == 0 || from_bb == 0)
    return 0;

  pred = s_preds[BLOCK_NUM(follow_bb)];

  if (pred != 0 &&
      GET_LIST_ELT(pred, int) == BLOCK_NUM(from_bb) && pred->next == 0)
    return 1;
  return 0;
}

/*
 * Have my own here, because I cannot delete insns if they become unused.
 * This can cause basic-blocks to change out from under the rest of
 * the algorithms.  Let jump_optimize take care of this, anytime we call
 * this routine, we will be rerunning jump_optimize after.
 */
static void
cnstprop_redirect_jump (jump, nlabel)
     rtx jump, nlabel;
{
  register rtx olabel = JUMP_LABEL (jump);

  if (nlabel == olabel)
    return;

  if (! redirect_exp (&PATTERN (jump), olabel, nlabel, jump))
    return;

  JUMP_LABEL (jump) = nlabel;
  ++LABEL_NUSES (nlabel);
  --LABEL_NUSES (olabel);

  return;
}

/*
 * insn is the last instruction in a basic block.  We see if the following
 * block contains a conditional jump that would be constant if only the
 * flow out of this basic block is considered.  If this is the case then
 * we can have fewer comparisons, and thus increase code speed.
 *
 * Since insn is the last instruction in a basic block, then the block
 * must end with an unconditional jump, and conditional jump, a
 * fall-through, or a table-jump.  We only care about the first three
 * cases.
 *
 * We don't try to do this optimization if the target basic-block is the
 * only successor to insn's basic-block, because the folding will
 * happen without doing this extra work.
 *
 * If any control flow is changed then set the flag any_control_changed
 * to 1 so that the flow information can be updated.
 */
static int
try_special_control_folds(insn, cnst_in, tmp_set, set_size)
rtx insn;
set cnst_in;
set tmp_set;
int set_size;
{
  int any_fold = 0;
  rtx next_bb;
  rtx new_targ;
  rtx cmp_insn;
  rtx jmp_insn;

  if (GET_CODE(insn) == JUMP_INSN && !simplejump_p(insn) && condjump_p(insn))
  {
    /*
     * This is the case where the basic-block ends with a conditional jump.
     * Look down to the target of the jump, and the fall-through of the jump,
     * for a basic-blocks that has only a conditional jump.
     * Then see if that conditional jump would be foldable at this point
     * if the program.  If the conditional jump is foldable and its the target
     * of the jump, then just change the final destination of the conditional
     * jump we are currently at.  If its the fall-through that has the
     * conditional jump that is foldable then emit an unconditional jump to
     * the proper label after the conditional jump we are currently sitting
     * on.
     */

    /* first we try the target of the conditional jump */
    next_bb = next_real_insn(JUMP_LABEL(insn));

    if (!only_pred_bb_p(next_bb, insn) &&
        condjump_safe_bb(next_bb, 0, &cmp_insn, &jmp_insn, tmp_set, set_size))
    {
      /*
       * since I am not allowing any extra insns here, the set of kills
       * from extra instructions must be zero, so I don't need to do anything
       * with it.
       */
      if ((new_targ = fold_cond_bb(cmp_insn, jmp_insn, cnst_in)) != 0)
      {
        /* change the conditional jump to jump to the new target */
        cnstprop_redirect_jump(insn, new_targ);
        any_control_folded = 1;
        any_fold = 1;
      }
    }

    /* now we'll try the fall-through block */
    next_bb = next_real_insn(insn);

    if (!only_pred_bb_p(next_bb, insn) &&
        condjump_safe_bb(next_bb, 0, &cmp_insn, &jmp_insn, tmp_set, set_size))
    {
      /*
       * since I am not allowing any extra insns here, the set of kills
       * from extra instructions must be zero, so I don't need to do anything
       * with it.
       */
      if ((new_targ = fold_cond_bb(cmp_insn, jmp_insn, cnst_in)) != 0)
      {
        /*
         * we need to generate an unconditional jump immediately after the
         * conditional jump.
         */
        insn = emit_jump_insn_after(gen_jump(new_targ), insn);
        JUMP_LABEL(insn) = new_targ;
        ++LABEL_NUSES(new_targ);
        emit_barrier_after(insn);

        any_control_folded = 1;
        any_fold = 1;
      }
    }
  }
  else if (GET_CODE(insn) == JUMP_INSN && simplejump_p(insn))
  {
    /*
     * unconditional jump case.  Again look at the following
     * block to see if it contains a foldable conditional jump.  In this
     * case we can allow a few instructions in front of the conditional
     * providing they don't change any registers involved in the conditional
     * we want to make constant.  These instructions will be copied into
     * this block, and the end of the block will need to be redirected to
     * jump to wherever the folded conditional would have gone.
     */
    next_bb = next_real_insn(JUMP_LABEL(insn));

    if (!only_pred_bb_p(next_bb, insn) &&
        condjump_safe_bb(next_bb, 5, &cmp_insn, &jmp_insn, tmp_set, set_size))
    {
      /*
       * delete the constants that will be killed by instructions preceding
       * the comparison from cnst_in.
       */
      AND_COMPL_SETS(tmp_set, cnst_in, tmp_set, set_size); 
      if ((new_targ = fold_cond_bb(cmp_insn, jmp_insn, tmp_set)) != 0)
      {
        /*
         * We need to copy any instructions except the compare and branch
         * into the current block, before the jump.
         */
        copy_block_after(next_bb, jmp_insn, PREV_INSN(insn), cmp_insn,
                         cnst_in, tmp_set, set_size);

        /* simply redirect its target to the new target */
        cnstprop_redirect_jump(insn, new_targ);

        any_control_folded = 1;
        any_fold = 1;
      }
    }
  }
  else if (GET_CODE(insn) != JUMP_INSN)
  {
    /*
     * fall-through case.  Again look at the following
     * block to see if it contains a foldable conditional jump.  In this
     * case we can allow a few instructions in front of the conditional
     * providing they don't change any registers involved in the conditional
     * we want to make constant.  These instructions will be copied into
     * this block, and the end of the block will need to be redirected to
     * jump to wherever the folded conditional would have gone.
     */
    next_bb = next_real_insn(insn);

    if (!only_pred_bb_p(next_bb, insn) &&
        condjump_safe_bb(next_bb, 5, &cmp_insn, &jmp_insn, tmp_set, set_size))
    {
      /*
       * delete the constants that will be killed by instructions preceding
       * the comparison from cnst_in.
       */
      AND_COMPL_SETS(tmp_set, cnst_in, tmp_set, set_size); 
      if ((new_targ = fold_cond_bb(cmp_insn, jmp_insn, tmp_set)) != 0)
      {
        /*
         * We need to copy any instructions except the compare and branch
         * into the current block, before the jump.
         */
        insn = copy_block_after(next_bb, jmp_insn, insn, cmp_insn,
                                cnst_in, tmp_set, set_size);

        /*
         * We need to emit an unconditional jump to the new target label
         * since this used to fall-through.
         */
        insn = emit_jump_insn_after(gen_jump(new_targ), insn);
        JUMP_LABEL(insn) = new_targ;
        ++LABEL_NUSES(new_targ);
        emit_barrier_after(insn);

        any_control_folded = 1;
        any_fold = 1;
      }
    }
  }

  return any_fold;
}

static int
cnstprop_rewrite()
{
  int bb_num;
  int any_rewrites = 0;
  set tmp_set = cnst_prop_info.tmp_set;
  int set_size = cnst_prop_info.set_size;
  
  for (bb_num = 0; bb_num < N_BLOCKS; bb_num++)
  {
    set bb_in;
    rtx t_insn;
    rtx end_insn;

    t_insn = BLOCK_HEAD(bb_num);
    end_insn = BLOCK_END(bb_num);
    bb_in = cnst_prop_info.in[bb_num];

    while (1)
    {
      /*
       * replace any cnsts that can be replaced here.
       */
      if (replace_in_insn(t_insn, bb_in))
        any_rewrites = 1;

      /*
       * update bb_in to kill all cnsts that this insn causes
       * to die.
       */
      CLR_SET(tmp_set, set_size);
      cnsts_insn_kills(t_insn, tmp_set);
      AND_COMPL_SETS(bb_in, bb_in, tmp_set, set_size);

      /*
       * update bb_in to include any cnsts that this instruction
       * gens.
       */
      CLR_SET(tmp_set, set_size);
      cnsts_insn_gens(t_insn, tmp_set);
      OR_SETS(bb_in, bb_in, tmp_set, set_size);

      if (t_insn == end_insn)
      {
        if (try_special_control_folds(t_insn, bb_in, tmp_set, set_size))
          any_rewrites = 1;
        break;
      }

      t_insn = NEXT_INSN(t_insn);
    }
  }

  /*
   * If any rewrites were done, see if any of the destination registers
   * of the consts no longer have any uses.  If so then the original
   * instruction generating the const is deletable.
   */
  if (any_rewrites)
  {
    int n_cnst_nodes = cnst_prop_info.n_cnst_nodes;
    struct cnst_prop_node *cnst_nodes = cnst_prop_info.cnst_nodes;
    int *reg_uses = cnst_prop_info.reg_uses;
    int i;

    for (i = 0; i < n_cnst_nodes; i++)
    {
      if (reg_uses[cnst_nodes[i].dreg] == 0)
      {
        /* The original const instruction is deletable. */
        imstg_mark_delete(cnst_nodes[i].insn);
      }
    }
  }

  return any_rewrites;
}

static void
cnstprop_cc0_fold(insns)
rtx insns;
{
  rtx t_insn;

  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    if (GET_CODE(t_insn) == INSN)
    {
      rtx cc0_tmp;
      rtx cc0_user;

      if ((cc0_tmp = cc0_compare_p(PATTERN(t_insn))) != 0)
      {
        while ((cc0_user = get_cc_user(t_insn, cc0_tmp)) != 0)
        {
          rtx new_rtx = try_cc0_fold(PATTERN(t_insn), cc0_user);

          if (new_rtx == 0)
            break;  /* get out of the loop */

          /* replace the SET_SRC with the folded op0. */
          if (GET_CODE(new_rtx) == PC)
          {
            /*
             * this has become set pc pc, which is a nop.
             * Change it to be a deleted instruction.
             */
#if defined(IMSTG) && defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
            if (flag_linenum_mods)
              dwarfout_disable_notes(cc0_user);
#endif
            PUT_CODE(cc0_user, NOTE);
            NOTE_LINE_NUMBER(cc0_user) = NOTE_INSN_DELETED;
            NOTE_SOURCE_FILE(cc0_user) = 0;
      
            /* delete a reference to this label. */
            --LABEL_NUSES (JUMP_LABEL(cc0_user));
            any_control_folded = 1;
          }
          else
          {
            SET_SRC(PATTERN(cc0_user)) = new_rtx;
            INSN_CODE(cc0_user) = -1;

            if (GET_CODE(cc0_user) == JUMP_INSN)
            {
              /*
               * emit a barrier after an unconditional jump, so that
               * jump_optimize will be good about deleting the code.
               */
              emit_barrier_after (cc0_user);
              any_control_folded = 1;
            }
          }
        }
      }
    }
  }
}

/* 
 * If X is a nontrivial arithmetic operation on an argument
 * for which a constant value can be determined, modify the rtx to use
 * the folded constant value.  Return 1 if any tree modification
 * happens, 0 otherwise.
 * 
 * This routine assumes that all operands of the current rtx have
 * already had constant folding applied to them, if they are constant.
 * This routine also assumes that x is non-null.
 * 
 * This routine must be careful to keep the UD_RTX information up to
 * date for register operands.  This means that if it somehow rearanges
 * a REGISTER, SUBREG, ZERO_EXTRACT, SIGN_EXTRACT, ZERO_EXTEND, or
 * SIGN_EXTEND underneath some other operator, it must make sure that
 * it updates the dataflow for the reg ud associated with it.
 *
 * parent is the parent rtx in which the folding is taking place.
 * index is such that XEXP(parent,index) is the place where x or
 *   its resultant folded tree is to be stored.
 * x is the rtx to be folded.
 */
static int
try_cfold(parent, index, x)
rtx parent;
int index;
rtx x;
{
  register enum rtx_code code;     /* rtx x's code */

  register enum machine_mode val_mode;
  unsigned val_lo;
  unsigned val_hi;

  unsigned arg0_lo;
  unsigned arg0_hi;
  unsigned arg1_lo;
  unsigned arg1_hi;

  unsigned arg0s_lo;
  unsigned arg0s_hi;
  unsigned arg1s_lo;
  unsigned arg1s_hi;

  /* Constant equivalents of first three operands of X;
     0 when no such equivalent is known.  */
  rtx const_arg0 = 0;
  rtx const_arg1 = 0;

  val_mode = GET_MODE (x);
  code = GET_CODE (x);

  /* Now decode the kind of rtx X is
     and then return X (if nothing can be done)
     or return a folded rtx
     or store a value in VAL and drop through
     (to return a CONST_INT for the integer VAL).  */

  switch (GET_RTX_CLASS(code))
  {
    default:
      if (code != SUBREG)
        return 0;
      /* fall through */

    case '1':
      /* get the operand */
      const_arg0 = XEXP(x,0);
      if (GET_CODE (const_arg0) != CONST_INT &&
          GET_CODE (const_arg0) != CONST_DOUBLE)
        return 0;

      /* Get zero-filled constant value in arg0_lo, arg0_hi */
      get_cnst_val(const_arg0, GET_MODE(const_arg0), &arg0_lo, &arg0_hi, 1);

      switch (code)
      {
        case NOT:
          val_lo = ~ arg0_lo;
          val_hi = ~ arg0_hi;
          break;

        case NEG:
          neg_double(arg0_lo, arg0_hi, &val_lo, &val_hi);
          break;

        case TRUNCATE:
          zero_extend_double (val_mode, arg0_lo, arg0_hi, &val_lo, &val_hi);
          break;

        case ZERO_EXTEND:
        {
          enum machine_mode mode = GET_MODE (XEXP (x, 0));
          zero_extend_double (mode, arg0_lo, arg0_hi, &val_lo, &val_hi);
          break;
        }

        case SIGN_EXTEND:
        {
          enum machine_mode mode = GET_MODE (XEXP (x, 0));
          sign_extend_double (mode, arg0_lo, arg0_hi, &val_lo, &val_hi);
          break;
        }

        case SUBREG:
        {
          if (XEXP(x,1) != 0)
          {
            /* need to shift the value right by the natural register size. */
            int i = XINT(x,1) * BITS_PER_WORD; 
            while (i > 0)
            {
              arg0_lo = (arg0_lo >> 1) | (arg0_hi << (HOST_BITS_PER_INT-1));
              arg0_hi >>= 1;
              i -= 1;
            }
          }

          zero_extend_double (val_mode,  arg0_lo, arg0_hi, &val_lo, &val_hi);
          break;
        }

        default:
          return 0;
      }
      break;

    case 'c':
    case '2':
    case '<':
    {
      enum machine_mode arith_mode = val_mode;

      /* get the arguments */
      switch (GET_CODE(XEXP(x,0)))
      {
        case LABEL_REF:
        case SYMBOL_REF:
        case CONST_INT:
        case CONST:
        case CONST_DOUBLE:
          const_arg0 = XEXP(x,0);
          break;
      }

      switch (GET_CODE(XEXP(x,1)))
      {
        case LABEL_REF:
        case SYMBOL_REF:
        case CONST_INT:
        case CONST:
        case CONST_DOUBLE:
          const_arg1 = XEXP(x,1);
          break;
      }

      /*
       * If a commutative operation, place a constant integer
       * as the second operand unless the first operand is also
       * a constant integer.  Otherwise, place any constant second
       * unless the first operand is also a constant.
       */

      switch (code)
      {
        case PLUS:
        case MULT:
        case AND:
        case IOR:
        case XOR:
        case NE:
        case EQ:
          if (const_arg0 && const_arg0 == XEXP (x, 0)
              && (! (const_arg1 && const_arg1 == XEXP (x, 1))
              || (GET_CODE (const_arg0) == CONST_INT
              && GET_CODE (const_arg1) != CONST_INT)))
          {
            register rtx tem;

            tem = XEXP (x, 0); XEXP (x, 0) = XEXP (x, 1); XEXP (x, 1) = tem;
            tem = const_arg0; const_arg0 = const_arg1; const_arg1 = tem;
          }
          break;
      }

      if (const_arg0 == 0 ||
          const_arg1 == 0 ||
          (GET_CODE (const_arg0) != CONST_INT &&
           GET_CODE (const_arg0) != CONST_DOUBLE) ||
          (GET_CODE (const_arg1) != CONST_INT &&
           GET_CODE (const_arg1) != CONST_DOUBLE))
      {
        /* Even if we can't compute a constant result,
           there are some cases worth simplifying.  */
        /* Note that we cannot rely on constant args to come last,
           even for commutative operators,
           because that happens only when the constant is explicit.  */
        switch (code)
        {
          case PLUS:
            if (IS_CONST_ZERO_RTX(const_arg0))
            {
              XEXP(parent,index) = XEXP(x,1);
              return 0;
            }

            if (IS_CONST_ZERO_RTX(const_arg1))
            {
              XEXP(parent,index) = XEXP(x,0);
              return 0;
            }

            /* if either operand is a const_int and the other operand is
             * a plus then try to rearrange the operands of these two
             * things to allow more folding.
             */
            if (const_arg0 != 0 &&
                (GET_CODE(const_arg0) == CONST_INT ||
                 GET_CODE(const_arg0) == CONST_DOUBLE) &&
                GET_CODE(XEXP(x,1)) == PLUS)
            {
              rtx plus_op = XEXP(x,1);
              int swap_made = 0;

              if (GET_CODE(XEXP(plus_op,0)) != CONST_INT &&
                  GET_CODE(XEXP(plus_op,0)) != CONST_DOUBLE &&
                  GET_CODE(XEXP(plus_op,0)) != PLUS)
              {
                XEXP(x,0) = XEXP(plus_op,0);
                XEXP(plus_op,0) = const_arg0;
                swap_made = 1;
              }
              else if (GET_CODE(XEXP(plus_op,1)) != CONST_INT &&
                       GET_CODE(XEXP(plus_op,1)) != CONST_DOUBLE &&
                       GET_CODE(XEXP(plus_op,1)) != PLUS)
              {
                XEXP(x,0) = XEXP(plus_op,1);
                XEXP(plus_op,1) = const_arg0;
                swap_made = 1;
              }

              if (swap_made)
                if (try_cfold(x, 1, plus_op))
                  return (try_cfold(parent, index, x));
              return 0;
            }

            if (const_arg1 != 0 &&
                (GET_CODE(const_arg1) == CONST_INT ||
                 GET_CODE(const_arg1) == CONST_DOUBLE) &&
                GET_CODE(XEXP(x,0)) == PLUS)
            {
              rtx plus_op = XEXP(x,0);
              int swap_made = 0;

              if (GET_CODE(XEXP(plus_op,0)) != CONST_INT &&
                  GET_CODE(XEXP(plus_op,0)) != CONST_DOUBLE &&
                  GET_CODE(XEXP(plus_op,0)) != PLUS)
              {
                XEXP(x,1) = XEXP(plus_op,0);
                XEXP(plus_op,0) = const_arg1;
                swap_made = 1;
              }
              else if (GET_CODE(XEXP(plus_op,1)) != CONST_INT &&
                       GET_CODE(XEXP(plus_op,1)) != CONST_DOUBLE &&
                       GET_CODE(XEXP(plus_op,1)) != PLUS)
              {
                XEXP(x,1) = XEXP(plus_op,1);
                XEXP(plus_op,1) = const_arg1;
                swap_made = 1;
              }

              if (swap_made)
                if (try_cfold(x, 0, plus_op))
                  return (try_cfold(parent, index, x));
              return 0;
            }

            /* if either operand is a symbol_ref and the other operand is
             * a plus then try to rearrange the operands of these two
             * things to allow more folding.
             */
            if (const_arg0 != 0 &&
                (GET_CODE(const_arg0) == SYMBOL_REF ||
                 GET_CODE(const_arg0) == LABEL_REF) &&
                GET_CODE(XEXP(x,1)) == PLUS)
            {
              rtx plus_op = XEXP(x,1);
              int swap_made = 0;

              if (GET_CODE(XEXP(plus_op,0)) != CONST_INT &&
                  GET_CODE(XEXP(plus_op,0)) != CONST_DOUBLE &&
                  GET_CODE(XEXP(plus_op,0)) != SYMBOL_REF &&
                  GET_CODE(XEXP(plus_op,0)) != LABEL_REF &&
                  GET_CODE(XEXP(plus_op,0)) != PLUS)
              {
                XEXP(x,0) = XEXP(plus_op,0);
                XEXP(plus_op,0) = const_arg0;
                swap_made = 1;
              }
              else if (GET_CODE(XEXP(plus_op,1)) != CONST_INT &&
                       GET_CODE(XEXP(plus_op,1)) != CONST_DOUBLE &&
                       GET_CODE(XEXP(plus_op,1)) != SYMBOL_REF &&
                       GET_CODE(XEXP(plus_op,1)) != LABEL_REF &&
                       GET_CODE(XEXP(plus_op,1)) != PLUS)
              {
                XEXP(x,0) = XEXP(plus_op,1);
                XEXP(plus_op,1) = const_arg0;
                swap_made = 1;
              }

              if (swap_made)
                if (try_cfold(x, 1, plus_op))
                  return (try_cfold(parent, index, x));
              return 0;
            }

            if (const_arg1 != 0 &&
                (GET_CODE(const_arg1) == SYMBOL_REF ||
                 GET_CODE(const_arg1) == LABEL_REF) &&
                GET_CODE(XEXP(x,0)) == PLUS)
            {
              rtx plus_op = XEXP(x,0);
              int swap_made = 0;

              if (GET_CODE(XEXP(plus_op,0)) != CONST_INT &&
                  GET_CODE(XEXP(plus_op,0)) != CONST_DOUBLE &&
                  GET_CODE(XEXP(plus_op,0)) != SYMBOL_REF &&
                  GET_CODE(XEXP(plus_op,0)) != LABEL_REF &&
                  GET_CODE(XEXP(plus_op,0)) != PLUS)
              {
                XEXP(x,1) = XEXP(plus_op,0);
                XEXP(plus_op,0) = const_arg1;
                swap_made = 1;
              }
              else if (GET_CODE(XEXP(plus_op,1)) != CONST_INT &&
                       GET_CODE(XEXP(plus_op,1)) != CONST_DOUBLE &&
                       GET_CODE(XEXP(plus_op,1)) != SYMBOL_REF &&
                       GET_CODE(XEXP(plus_op,1)) != LABEL_REF &&
                       GET_CODE(XEXP(plus_op,1)) != PLUS)
              {
                XEXP(x,1) = XEXP(plus_op,1);
                XEXP(plus_op,1) = const_arg1;
                swap_made = 1;
              }

              if (swap_made)
                if (try_cfold(x, 0, plus_op))
                  return (try_cfold(parent, index, x));
              return 0;
            }
            break;

          case MINUS:
            if (IS_CONST_ZERO_RTX(const_arg1))
            {
              XEXP(parent,index) = XEXP(x,0);
              return 0;
            }
#if 0
            /* Can't do this if machine has no NEG pattern. */
            /* Change subtraction from zero into negation.  */
            if (const_arg0 == const0_rtx)
            {
              XEXP(parent,index) = gen_rtx (NEG, GET_MODE (x), XEXP (x,1));
              return 0;
            }
#endif

            /* Don't let a relocatable value get a negative coeff.  */
            if (const_arg0 != 0 && const_arg1 != 0 &&
                GET_CODE (const_arg1) == CONST_INT)
            {
              /*
               * change the tree into a PLUS of the negated constant,
               * and try to fold the resultant tree.
               */
              PUT_CODE(x, PLUS);

              XEXP(x,1) = gen_rtx (CONST_INT, VOIDmode, -INTVAL(const_arg1));

              try_cfold(parent, index, x);
              return 0;
            }
            break;

          case MULT:
#if 0
            /* Can't do this if machine has no NEG pattern. */
            if (const_arg1 && GET_CODE (const_arg1) == CONST_INT
                && INTVAL (const_arg1) == -1
                /* Don't do this in the case of widening multiplication.  */
                && GET_MODE (XEXP (x, 0)) == GET_MODE (x))
            {
              XEXP(parent,index) = gen_rtx (NEG, GET_MODE (x), XEXP (x,0));
              return 0;
            }

            if (const_arg0 && GET_CODE (const_arg0) == CONST_INT
                && INTVAL (const_arg0) == -1
                && GET_MODE (XEXP (x, 1)) == GET_MODE (x))
            {
              XEXP(parent,index) = gen_rtx (NEG, GET_MODE (x), XEXP (x,1));
              return 0;
            }
#endif

            if (IS_CONST_ZERO_RTX(const_arg1))
            {
              XEXP(parent,index) = const_arg1;
              return 1;
            }

            if (IS_CONST_ZERO_RTX(const_arg0))
            {
              XEXP(parent,index) = const_arg0;
              return 1;
            }

            if (const_arg1 == const1_rtx)
            {
              XEXP(parent,index) = XEXP(x,0);
              return 0;
            }

            if (const_arg0 == const1_rtx)
            {
              XEXP(parent,index) = XEXP(x,1);
              return 0;
            }
            break;

          case IOR:
            if (const_arg1 == const0_rtx)
            {
              XEXP(parent,index) = XEXP(x,0);
              return 0;
            }

            if (const_arg0 == const0_rtx)
            {
              XEXP(parent,index) = XEXP(x,1);
              return 0;
            }

            if (const_arg1 && GET_CODE (const_arg1) == CONST_INT
                && (INTVAL (const_arg1) & GET_MODE_MASK (GET_MODE (x)))
                == GET_MODE_MASK (GET_MODE (x)))
            {
              XEXP(parent,index) = const_arg1;
              return 1;
            }

            if (const_arg0 && GET_CODE (const_arg0) == CONST_INT
                && (INTVAL (const_arg0) & GET_MODE_MASK (GET_MODE (x)))
                == GET_MODE_MASK (GET_MODE (x)))
            {
              XEXP(parent,index) = const_arg0;
              return 1;
            }
            break;

          case XOR:
            if (const_arg1 == const0_rtx)
            {
              XEXP(parent,index) = XEXP(x,0);
              return 0;
            }

            if (const_arg0 == const0_rtx)
            {
              XEXP(parent,index) = XEXP(x,1);
              return 0;
            }

            if (const_arg1 && GET_CODE (const_arg1) == CONST_INT
                && (INTVAL (const_arg1) & GET_MODE_MASK (GET_MODE (x)))
                == GET_MODE_MASK (GET_MODE (x)))
            {
              XEXP(parent,index) = gen_rtx (NOT, GET_MODE (x), XEXP (x,0));
              return 0;
            }

            if (const_arg0 && GET_CODE (const_arg0) == CONST_INT
                && (INTVAL (const_arg0) & GET_MODE_MASK (GET_MODE (x)))
                == GET_MODE_MASK (GET_MODE (x)))
            {
              XEXP(parent,index) = gen_rtx (NOT, GET_MODE (x), XEXP (x,1));
              return 0;
            }
            break;

          case AND:
            if (const_arg1 == const0_rtx || const_arg0 == const0_rtx)
            {
              XEXP(parent,index) = const0_rtx;
              return 1;
            }

            if (const_arg1 && GET_CODE (const_arg1) == CONST_INT
                && (INTVAL (const_arg1) & GET_MODE_MASK (GET_MODE (x)))
                == GET_MODE_MASK (GET_MODE (x)))
            {
              XEXP(parent,index) = XEXP(x,0);
              return 0;
            }

            if (const_arg0 && GET_CODE (const_arg0) == CONST_INT
                && (INTVAL (const_arg0) & GET_MODE_MASK (GET_MODE (x)))
                == GET_MODE_MASK (GET_MODE (x)))
            {
              XEXP(parent,index) = XEXP(x,1);
              return 0;
            }
            break;

          case DIV:
          case UDIV:
            if (const_arg1 == const1_rtx)
            {
              XEXP(parent,index) = XEXP(x,0);
              return 0;
            }

            if (const_arg0 == const0_rtx)
            {
              XEXP(parent,index) = const0_rtx;
              return 1;
            }
            break;

          case UMOD:
          case MOD:
            if (const_arg0 == const0_rtx || const_arg1 == const1_rtx)
            {
              XEXP(parent,index) = const0_rtx;
              return 1;
            }
            break;

          case LSHIFT:
          case ASHIFT:
          case ROTATE:
          case ASHIFTRT:
          case LSHIFTRT:
          case ROTATERT:
            if (const_arg1 == const0_rtx)
            {
              XEXP(parent,index) = XEXP(x,0);
              return 0;
            }

            if (const_arg0 == const0_rtx)
            {
              XEXP(parent,index) = const_arg0;
              return 1;
            }
            break;
        }

        return 0;
      }

      if (arith_mode == VOIDmode)
      {
        if (GET_MODE(XEXP(x,0)) != VOIDmode)
          arith_mode = GET_MODE(XEXP(x,0));
        else if (GET_MODE(XEXP(x,1)) != VOIDmode)
          arith_mode = GET_MODE(XEXP(x,1));
      }

      /*
       * Get the integer argument values in two forms:
       * zero-extended in ARG0, ARG1 and sign-extended in ARG0S, ARG1S.
       */
      get_cnst_val(const_arg0, arith_mode, &arg0_lo, &arg0_hi, 1);
      get_cnst_val(const_arg1, arith_mode, &arg1_lo, &arg1_hi, 1);
      get_cnst_val(const_arg0, arith_mode, &arg0s_lo, &arg0s_hi, -1);
      get_cnst_val(const_arg1, arith_mode, &arg1s_lo, &arg1s_hi, -1);

      /* Compute the value of the arithmetic.  */

      switch (code)
      {
        case PLUS:
          add_double(arg0_lo, arg0_hi, arg1_lo, arg1_hi, &val_lo, &val_hi);
          break;

        case MINUS:
          neg_double(arg1_lo, arg1_hi, &val_lo, &val_hi);
          add_double(arg0_lo, arg0_hi, val_lo, val_hi, &val_lo, &val_hi);
          break;

        case MULT:
          mul_double(arg0_lo, arg0_hi, arg1_lo, arg1_hi, &val_lo, &val_hi);
          break;

        case DIV:
          if (arg1s_lo == 0 && arg1s_hi == 0)
            return 0;
          divs (arg0s_lo, arg0s_hi, arg1s_lo, arg1s_hi, &val_lo, &val_hi);
          break;

        case MOD:
          if (arg1s_lo == 0 && arg1s_hi == 0)
            return 0;
          rems (arg0s_lo, arg0s_hi, arg1s_lo, arg1s_hi, &val_lo, &val_hi);
          break;

        case UDIV:
          if (arg1_lo == 0 && arg1_hi == 0)
            return 0;
          divu (arg0_lo, arg0_hi, arg1_lo, arg1_hi, &val_lo, &val_hi);
          break;

        case UMOD:
          if (arg1_lo == 0 && arg1_hi == 0)
            return 0;
          remu (arg0_lo, arg0_hi, arg1_lo, arg1_hi, &val_lo, &val_hi);
          break;

        case AND:
          val_lo = arg0_lo & arg1_lo;
          val_hi = arg0_hi & arg1_hi;
          break;

        case IOR:
          val_lo = arg0_lo | arg1_lo;
          val_hi = arg0_hi | arg1_hi;
          break;

        case XOR:
          val_lo = arg0_lo ^ arg1_lo;
          val_hi = arg0_hi ^ arg1_hi;
          break;

        case NE:
          val_lo = (arg0_lo != arg1_lo || arg0_hi != arg1_hi);
          val_hi = 0;
          break;

        case EQ:
          val_lo = (arg0_lo == arg1_lo && arg0_hi == arg1_hi);;
          val_hi = 0;
          break;

        case LE:
          val_lo = cmple_double(arg0s_lo, arg0s_hi, arg1s_lo, arg1s_hi);
          val_hi = 0;
          break;

        case LT:
          val_lo = cmpl_double(arg0s_lo, arg0s_hi, arg1s_lo, arg1s_hi);
          val_hi = 0;
          break;

        case GE:
          val_lo = !cmpl_double(arg0s_lo, arg0s_hi, arg1s_lo, arg1s_hi);
          val_hi = 0;
          break;

        case GT:
          val_lo = !cmple_double(arg0s_lo, arg0s_hi, arg1s_lo, arg1s_hi);
          val_hi = 0;
          break;

        case LEU:
          val_lo = cmpleu_double(arg0_lo, arg0_hi, arg1_lo, arg1_hi);
          val_hi = 0;
          break;

        case LTU:
          val_lo = cmplu_double(arg0_lo, arg0_hi, arg1_lo, arg1_hi);
          val_hi = 0;
          break;

        case GEU:
          val_lo = !cmplu_double(arg0_lo, arg0_hi, arg1_lo, arg1_hi);
          val_hi = 0;
          break;

        case GTU:
          val_lo = !cmpleu_double(arg0_lo, arg0_hi, arg1_lo, arg1_hi);
          val_hi = 0;
          break;

        case LSHIFT:
          /* If target machine uses negative shift counts
             but host machine does not, simulate them.  */
          if (arg1s_hi == -1)
          {
            neg_double(arg1s_lo, arg1s_hi, &arg1s_lo, &arg1s_hi);
            goto lshiftrt;
          }
          goto ashift;

        case ASHIFT:
          /* If target machine uses negative shift counts
             but host machine does not, simulate them.  */
          if (arg1s_hi == -1)
          {
            neg_double(arg1s_lo, arg1s_hi, &arg1s_lo, &arg1s_hi);
            goto ashiftrt;
          }
        ashift : ;
          {
            int i = arg1s_lo & (HOST_BITS_PER_INT * 2 -1);
            val_lo = arg0_lo;
            val_hi = arg0_hi;
            while (i > 0)
            {
              val_hi = val_hi << 1 | (val_lo >> (HOST_BITS_PER_INT-1));
              val_lo <<= 1;
              i -= 1;
            }
          }
          break;

        case LSHIFTRT:
          /* If target machine uses negative shift counts
              but host machine does not, simulate them.  */
          if (arg1s_hi == -1)
          {
            neg_double(arg1s_lo, arg1s_hi, &arg1s_lo, &arg1s_hi);
            goto ashift;
          }
        lshiftrt: ;
          {
            int i = arg1s_lo & (HOST_BITS_PER_INT * 2 -1);
            val_lo = arg0_lo;
            val_hi = arg0_hi;
            while (i > 0)
            {
              val_lo = (val_lo >> 1) | (val_hi << (HOST_BITS_PER_INT-1));
              val_hi >>= 1;
              i -= 1;
            }
          }
          break;

        case ASHIFTRT:
          /* If target machine uses negative shift counts
              but host machine does not, simulate them.  */
          if (arg1s_hi == -1)
          {
            neg_double(arg1s_lo, arg1s_hi, &arg1s_lo, &arg1s_hi);
            goto ashift;
          }
        ashiftrt: ;
          {
            int i = arg1s_lo & (HOST_BITS_PER_INT * 2 -1);
            val_lo = arg0s_lo;
            val_hi = arg0s_hi;
            while (i > 0)
            {
              val_lo = (val_lo >> 1) | (val_hi << (HOST_BITS_PER_INT-1));
              val_hi = (val_hi >> 1) | (val_hi & (((unsigned long)1) << HOST_BITS_PER_INT-1));
              i -= 1;
            }
          }
          break;

        default:
          return 0;
      }
    }
    break;
  }

  /*
   * Clear the bits that don't belong in our mode,
   */
  sign_extend_double(val_mode, val_lo, val_hi, &val_lo, &val_hi);

  /* Now make the new constant.  */
  /*
   * If the high word is the sign extension of the upper bit of the low
   * word we can just use CONST_INT, otherwise use CONST_DOUBLE.
   */
  if (val_hi == 0 && val_lo == 0)
    XEXP(parent,index) = const0_rtx;
  else if (val_hi == 0 && val_lo == 1)
    XEXP(parent,index) = const1_rtx;
  else if (val_hi ==
           (((val_lo & (((unsigned long)1) << (HOST_BITS_PER_INT-1))) != 0) ? -1 : 0))
    XEXP(parent,index) = gen_rtx (CONST_INT, VOIDmode, val_lo);
  else
    XEXP(parent,index) = immed_double_const(val_lo, val_hi, val_mode);
  return 1;
}

static rtx
get_cc_user(x, srch)
register rtx x;
register rtx srch;
{
  /*
   * insn x sets a condition code.  Return the insn which uses this
   * cc value.  If we call or jump or run off the block without finding
   * the user, return 0.
   */

  register enum rtx_code c;

  x = NEXT_INSN (x);
  while (x!=0 && !
    (((c=GET_CODE(x)) ==INSN || c==CALL_INSN || c==JUMP_INSN) &&
     reg_mentioned_p(srch, PATTERN(x))))

    if (c==INSN || c==NOTE)
      x=NEXT_INSN(x);
    else
      x = 0;

  return x;
}

/*
 * return the rtx representing the condition codes if the rtx passed
 * in is a set of the condition codes, otherwise return 0.
 */
static rtx
cc0_compare_p(x)
rtx x;
{
  rtx srch_rtx = 0;
  if (GET_CODE(x) == INSN)
    x = PATTERN(x);

  if (GET_CODE(x) == SET)
  {
    enum rtx_code code;

    if (SET_DEST(x) == cc0_rtx)
        srch_rtx = cc0_rtx;

    if (GET_CODE(SET_DEST(x)) == REG &&
        GET_MODE_CLASS(GET_MODE(SET_DEST(x))) == MODE_CC)
      srch_rtx = SET_DEST(x);

    if ((code = GET_CODE(SET_SRC(x))) != COMPARE &&
        code != CONST_INT && code != CONST_DOUBLE)
      srch_rtx = 0;
  }

  return srch_rtx;
}


/*
 * Try to fold a comparison operator and its user.
 *
 * On entry compare_pat is the pattern for a compare that maybe constant,
 * but is at least guaranteed to be of the formr:
 *
 *  set (cc0) (compare op1 op2)
 * or
 *  set (cc0) op1
 *
 * cc_use_insn is the instruction that uses the comparison result.  It may
 * be either a jump, or setting a boolean.
 *
 * If the conditional is not foldable return 0.
 * If the conditional is foldable then return the rtx that
 * the source operand of the cc_use_insn should be changed to.
 * This could be a label, pc_rtx, or a constant 1 or zero if a boolean
 * was just being set.
 *
 * The caller of this routine must take care of rewriting the rtl.
 */
static rtx
try_cc0_fold(compare_pat, cc_use_insn)
rtx compare_pat;
rtx cc_use_insn;
{
  rtx src;
  rtx op0;
  rtx op1;
  rtx cc_use;
  enum rtx_code c;
  unsigned int op0_lo;
  unsigned int op0_hi;
  unsigned int op1_lo;
  unsigned int op1_hi;
  enum machine_mode cmp_mode;

  src = SET_SRC(compare_pat);
  if (GET_CODE(src) != COMPARE)
    return 0; /* can't fold it */

  op0 = XEXP(src,0);
  op1 = XEXP(src,1);
  cmp_mode = COMPARE_MODE(src);

  if ((c=GET_CODE(cc_use_insn))!=INSN && c!=CALL_INSN && c!=JUMP_INSN)
    return 0;

  if (GET_CODE (cc_use=PATTERN(cc_use_insn)) != SET)
    return 0;

  cc_use = SET_SRC (cc_use);

  if ((c=GET_CODE(cc_use)) == IF_THEN_ELSE)
      c = GET_CODE (XEXP(cc_use,0));

  /*
   * c now has the code for the relational operator on op0, op1.
   * based on that code fold the constants into op0.
   */
  if ((GET_CODE(op0) == CONST_INT || GET_CODE(op0) == CONST_DOUBLE) &&
      (GET_CODE(op1) == CONST_INT || GET_CODE(op1) == CONST_DOUBLE))
  {
    /* Get sign-extended constants in op0, op1 */
    get_cnst_val(op0, cmp_mode, &op0_lo, &op0_hi, -1);
    get_cnst_val(op1, cmp_mode, &op1_lo, &op1_hi, -1);
    switch (c)
    {
      case EQ:
        op0 = (op0_lo == op1_lo && op0_hi == op1_hi) ?
              const1_rtx : const0_rtx;
        break;

      case NE:
        op0 = (op0_lo == op1_lo && op0_hi == op1_hi) ?
              const0_rtx : const1_rtx;
        break;

      case LT:
        op0 = cmpl_double (op0_lo, op0_hi, op1_lo, op1_hi) ?
              const1_rtx : const0_rtx;
        break;

      case LTU:
        zero_extend_double(cmp_mode, op0_lo, op0_hi, &op0_lo, &op0_hi);
        zero_extend_double(cmp_mode, op1_lo, op1_hi, &op1_lo, &op1_hi);
        op0 = cmplu_double (op0_lo, op0_hi, op1_lo, op1_hi) ?
              const1_rtx : const0_rtx;
        break;

      case LE:
        op0 = cmple_double (op0_lo, op0_hi, op1_lo, op1_hi) ?
              const1_rtx : const0_rtx;
        break;

      case LEU:
        zero_extend_double(cmp_mode, op0_lo, op0_hi, &op0_lo, &op0_hi);
        zero_extend_double(cmp_mode, op1_lo, op1_hi, &op1_lo, &op1_hi);
        op0 = cmpleu_double (op0_lo, op0_hi, op1_lo, op1_hi) ?
              const1_rtx : const0_rtx;
        break;

      case GT:
        op0 = cmple_double (op0_lo, op0_hi, op1_lo, op1_hi) ?
              const0_rtx : const1_rtx;
        break;

      case GTU:
        zero_extend_double(cmp_mode, op0_lo, op0_hi, &op0_lo, &op0_hi);
        zero_extend_double(cmp_mode, op1_lo, op1_hi, &op1_lo, &op1_hi);
        op0 = cmpleu_double (op0_lo, op0_hi, op1_lo, op1_hi) ?
              const0_rtx : const1_rtx;
        break;

      case GE:
        op0 = cmpl_double (op0_lo, op0_hi, op1_lo, op1_hi) ?
              const0_rtx : const1_rtx;
        break;

      case GEU:
        zero_extend_double(cmp_mode, op0_lo, op0_hi, &op0_lo, &op0_hi);
        zero_extend_double(cmp_mode, op1_lo, op1_hi, &op1_lo, &op1_hi);
        op0 = cmplu_double (op0_lo, op0_hi, op1_lo, op1_hi) ?
              const0_rtx : const1_rtx;
        break;

      default:
        return 0;
    }
  }
  else
  {
    switch (c)
    {
      case EQ:
        if (rtx_equal_p(op0, op1))
        {
          op0 = const1_rtx;
          break;
        }
        return 0;
      case NE:
        if (rtx_equal_p(op0, op1))
        {
          op0 = const0_rtx;
          break;
        }
        return 0;

      default:
        return 0;
    }
  }

  if (GET_CODE(cc_use) == IF_THEN_ELSE)
  {
    if (op0 == const0_rtx)
      op0 = XEXP(cc_use, 2);
    else
      op0 = XEXP(cc_use, 1);
  }

  return op0;
}

/*
 * Do all folding of rtx that became constant due to the replacement
 * of some non-constant rtx in the rtx tree denoted by top.
 * The algorithm for doing this is to recursively go down the tree trying
 * to do constant folding, and as we come back up the tree try constant
 * folding again if anybody directly below us had luck doing some folding.
 * This routine returns 0 is no_folding was performed, and 1 if some folding
 * was done here.
 */
static int
fold_after_replacement(parent, index, top)
rtx parent;
int index;
rtx top;
{
  register char *fmt;
  register char i;
  register char end;
  rtx folded_rtx;
  int found = 0;

  end = GET_RTX_LENGTH(GET_CODE(top));
  fmt = GET_RTX_FORMAT(GET_CODE(top));

  for (i = 0; i < end; i++)
  {
    if (fmt[i] == 'e')
    {
      /* try to fold it */
      if (try_cfold(parent, index, top) != 0)
        return 1;

      if (XEXP(top,i) != 0)
        found |= fold_after_replacement(top, i, XEXP(top,i));
    }
  }

  if (found == 1 && GET_CODE(top) != PLUS)
    return found;

  if (found > 0)
  {
    /* see if there are further folding possibilities. */
    if (try_cfold(parent, index, top) != 0)
      return 1;
    return 0;
  }

  return 0;
}

static void
delete_const_nodes(x)
rtx x;
{
  /* run through the rtl deleting any CONST nodes that are found. */
  register char *fmt;
  register int i;
  register int i_end;

  fmt = GET_RTX_FORMAT(GET_CODE(x));
  i_end = GET_RTX_LENGTH(GET_CODE(x));

  for (i= 0; i < i_end; i++)
  {
    if (fmt[i] == 'e')
    {
      if (XEXP(x,i) != 0)
      {
        if (GET_CODE(XEXP(x,i)) == CONST)
          XEXP(x,i) = XEXP(XEXP(x,i),0);
        else
          delete_const_nodes(XEXP(x,i));
      }
    }
  }
}

/*
 * This routine returns 1 if all the rtx in the tree x is a constant
 * expression. It returns false otherwise.  This routine puts CONST
 * nodes on top of any constant subexpressions that are constant,
 * but are expressed as rtl expressions.
 */
static int
replace_const_nodes(x)
rtx x;
{
  register char * fmt;
  register int i;
  register int i_end;
  int all_operands_constant;
  char may_need_const[50];

  switch (GET_CODE(x))
  {
    case CONST_DOUBLE:
      /* Only propagate integer calculations. */
      if (GET_MODE(x) != VOIDmode  &&
          GET_MODE_CLASS(GET_MODE(x)) != MODE_INT)
        return 0;
      /* Fall through */
    case CONST_INT:
    case SYMBOL_REF:
    case LABEL_REF:
      return 1;


    case SUBREG:
    case REG:
    case CC0:
    case PC:
      return 0;

    case ASM_INPUT:
    case ASM_OPERANDS:
    /* don't bother with asm stuff. */
      return 0;

    default:
      break;
  }

  all_operands_constant = 1;
  fmt = GET_RTX_FORMAT(GET_CODE(x));
  i_end = GET_RTX_LENGTH(GET_CODE(x));

  for (i = 0; i < i_end; i++)
    may_need_const[i] = 0;

  for (i = 0; i < i_end; i++)
  {
    if (fmt[i] == 'e')
    {
      if (XEXP(x,i) != 0)
        if (replace_const_nodes(XEXP(x,i)) != 0)
          may_need_const[i] = 1;
        else
          all_operands_constant = 0;
    }
  }

  if (all_operands_constant &&
      GET_CODE(x) != MEM && GET_CODE(x) != COMPARE)
    return 1;

  for (i = 0; i < i_end; i++)
  {
    if (may_need_const[i])
    {
      switch (GET_CODE(XEXP(x,i)))
      {
        case CONST_INT:
        case CONST_DOUBLE:
        case SYMBOL_REF:
        case LABEL_REF:
          /* these don't need CONST added on top of them. */
          break;

        default:
          /* anything else needs a const added to it. */
          XEXP(x,i) = gen_rtx(CONST, GET_MODE(XEXP(x,i)), XEXP(x,i));
          break;
      }
    }
  }

  return 0;
}

int
do_cnstprop(insns)
rtx insns;
{
  rtx t_insn;
  int change = 1;
  int control_change = 1;
  int n_regs = max_reg_num();
  int max_cnsts;
  int max_uid;

  if (n_regs == 0)
    return 0;

  word_size = GET_MODE_SIZE(word_mode);

  build_cnst_prop_bb_info(insns);
  find_bounds(insns, &max_uid, &max_cnsts);

  if (max_cnsts == 0)
    return 0;

  cnst_prop_info.reg_uses = (int *)
      xmalloc(n_regs * sizeof(int));
  cnst_prop_info.cnst_uses = (list_p *)
      xmalloc(n_regs * sizeof(list_p));
  cnst_prop_info.cnst_nodes = (struct cnst_prop_node *)
      xmalloc(max_cnsts * sizeof(struct cnst_prop_node));
  cnst_prop_info.insn_node_no = (short *)
      xmalloc((max_uid+1) * sizeof(short));
  cnst_prop_info.in = (set *)
      xmalloc(N_BLOCKS * sizeof(set) * 4);

  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
    if (GET_RTX_CLASS(GET_CODE(t_insn)) == 'i')
      delete_const_nodes(PATTERN(t_insn));

  while (change)
  {
    change = 0;

    bzero(cnst_prop_info.cnst_uses, n_regs * sizeof(list_p));
    bzero(cnst_prop_info.reg_uses, n_regs * sizeof(int));

    /* find all the cnsts */
    find_cnsts(insns, 1);
    
    if (cnst_prop_info.n_cnsts != 0)
    {
      any_control_folded = 0;

      /* run the dataflow */
      cnstprop_dfa();

      /* do the propagation */
      change = cnstprop_rewrite();

      free(cnst_prop_info.cnstprop_pool);

      if (change)
      {
        cnstprop_cc0_fold(insns);

        if (any_control_folded)
        {
          free(s_preds);
          free_lists(&pred_list_blks);
          jump_optimize (insns, 0, 0, 0);
          build_cnst_prop_bb_info(insns);
        }

        find_bounds(insns, &max_uid, &max_cnsts);

        if (max_cnsts == 0)
          goto quit_cnstprop;

        n_regs = max_reg_num();
        cnst_prop_info.reg_uses = (int *)
            xrealloc(cnst_prop_info.reg_uses, n_regs * sizeof(int));
        cnst_prop_info.cnst_uses = (list_p *)
            xrealloc(cnst_prop_info.cnst_uses, n_regs * sizeof(list_p));
        cnst_prop_info.cnst_nodes = (struct cnst_prop_node *)
            xrealloc(cnst_prop_info.cnst_nodes,
                     max_cnsts * sizeof(struct cnst_prop_node));
        cnst_prop_info.insn_node_no = (short *)
            xrealloc(cnst_prop_info.insn_node_no, (max_uid+1) * sizeof(short));
        cnst_prop_info.in = (set *)
            xrealloc(cnst_prop_info.in, N_BLOCKS * sizeof(set) * 4);
      }
    }

    /* free the space used */
    free_lists(&cnst_list_blks);
  }

quit_cnstprop: ;
  free(cnst_prop_info.in);
  free(cnst_prop_info.insn_node_no);
  free(cnst_prop_info.cnst_nodes);
  free(cnst_prop_info.cnst_uses);
  free(cnst_prop_info.reg_uses);
  free(s_preds);
  free_lists(&pred_list_blks);

   /* put any CONST nodes that are necessary back into the rtl. */
  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    if (GET_RTX_CLASS(GET_CODE(t_insn)) == 'i')
      replace_const_nodes(PATTERN(t_insn));
  }

  return 0;
}
#endif
