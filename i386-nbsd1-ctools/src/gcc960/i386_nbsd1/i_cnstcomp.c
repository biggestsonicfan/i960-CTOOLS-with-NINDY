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
#include "basic-block.h"
#include "hard-reg-set.h"
#else
#include "list.h"
#include "set.h"
#include "basicblk.h"
#include "hreg_set.h"
#endif

#ifndef HARD_REGNO_REF
#define HARD_REGNO_REF(X, RGNO_P, RGSIZE_P) \
  generic_hard_regno_ref(X, RGNO_P, RGSIZE_P)
#endif

/*
 * Given a SUBREG or REG rtx for a hard register return the first of the
 * hard registers referred to, and the number referred to by the rtx.
 * RP is a pointer to an int for the register number.  RSP is a pointer
 * to an int for the number of registers referred to.
 * For any other rtx *RP must go back as -1.
 */
void
generic_hard_regno_ref(x, rgno_p, rgsize_p)
rtx x;
int *rgno_p;
int *rgsize_p;
{
  int rgno = -1;
  int rgsize = -1;
  int word_size = GET_MODE_SIZE(word_mode);

  switch (GET_CODE(x))
  {
    case SUBREG:
      if (GET_CODE(XEXP(x,0)) != REG)
        break;
      rgno = REGNO(XEXP(x,0));
      rgno += XINT(x,1);
      rgsize = (RTX_BYTES(x) + word_size - 1) / word_size;
      break;

    case REG:
      rgno = REGNO(x);
      rgsize = (RTX_BYTES(x) + word_size - 1) / word_size;
      break;

    default:
      break;
  }
  *rgno_p = rgno;
  *rgsize_p = rgsize;
}

#define GET_LIST_INSN(P) GET_LIST_ELT(P, rtx)
#define GET_LIST_CAND(P) GET_LIST_ELT(P, cand_ptr)
#define GET_LIST_PRED(P) GET_LIST_ELT(P, int)
#define GET_LIST_SUCC(P) GET_LIST_ELT(P, int)

typedef struct cand_rec {
  int  cand_no;
  list_p cand_insns;
  int  dreg;
  enum machine_mode cnst_mode;
  rtx  cnst_rtx;
} * cand_ptr;

static cand_ptr s_cands;
static list_p * insn_gens;
static list_p * cand_uses;
static list_p * s_preds;
static list_block_p list_blks;

/* These two are simply used for temporary calculations. */
static set_array s_regs_insn_sets[SET_SIZE(FIRST_PSEUDO_REGISTER)];
static set_array s_regs_insn_uses[SET_SIZE(FIRST_PSEUDO_REGISTER)];

#if 0
void
zero_extend_double(mode, l1, h1, lv, hv)
enum machine_mode mode;
unsigned l1, h1, *lv, *hv;
{
  unsigned lo;
  unsigned hi;
  unsigned m_size;

  if (mode == VOIDmode)
    mode = DImode;

  m_size = GET_MODE_BITSIZE(mode);
  
  lo = GET_MODE_MASK(mode);
  if (m_size > HOST_BITS_PER_INT)
    hi = -1;
  else
    hi = 0;

  *lv = lo & l1;
  *hv = hi & h1;
}

void
sign_extend_double(mode, l1, h1, lv, hv)
enum machine_mode mode;
unsigned l1, h1, *lv, *hv;
{
  unsigned lo;
  unsigned hi;
  unsigned m_size;

  if (mode == VOIDmode)
    mode = DImode;

  m_size = GET_MODE_BITSIZE(mode);

  if (m_size > HOST_BITS_PER_INT)
  {
    *lv = l1;
    *hv = h1;
    return;
  }

  zero_extend_double(mode, l1, h1, lv, hv);
  hi = -1;
  lo = ~GET_MODE_MASK(mode);
  if ((l1 & (1 << (m_size-1))) == 0)
    return;
  *lv |= lo;
  *hv |= hi;
}

void
get_cnst_val(x, mode, l_p, h_p)
rtx x;
enum machine_mode mode;
unsigned int *l_p;
unsigned int *h_p;
{
  unsigned int lo;
  unsigned int hi;

  switch (GET_CODE(x))
  {
    case CONST_INT:
      lo = INTVAL(x);
      if ((lo & 0x80000000) == 0)
        hi = 0;
      else
        hi = -1;
      break;

    case CONST_DOUBLE:
      lo = CONST_DOUBLE_LOW(x);
      hi = CONST_DOUBLE_HIGH(x);
      break;

    default:
      abort();
  }

  zero_extend_double(mode, lo, hi, l_p, h_p);
}
#endif

int
same_constant (c1, c2, mode)
rtx c1;
rtx c2;
enum machine_mode mode;
{
  unsigned int cl1;
  unsigned int ch1;
  unsigned int cl2;
  unsigned int ch2;

  get_cnst_val(c1, mode, &cl1, &ch1);
  get_cnst_val(c2, mode, &cl2, &ch2);

  if (ch1 == ch2 && cl1 == cl2)
    return 1;
  return 0;
}

static int
find_bounds(insns, max_uid_p, n_cands_p)
rtx insns;
int *max_uid_p;
int *n_cands_p;
{
  register int n_insns = 0;
  register int max_uid = 0;
  register rtx t_insn;
  register enum rtx_code c;
  int dreg;
  int dreg_size;

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
        (GET_CODE(SET_SRC(PATTERN(t_insn))) == CONST_INT ||
         GET_CODE(SET_SRC(PATTERN(t_insn))) == CONST_DOUBLE))
    {
      HARD_REGNO_REF(SET_DEST(PATTERN(t_insn)), &dreg, &dreg_size);
      if (dreg >= 0)
        n_insns += 4;  /* Can get defs for DI, SI, HI, QI */
    }
  }

  *max_uid_p = max_uid;
  *n_cands_p = n_insns;
}

static int
find_candidates(insns, cands)
rtx insns;
cand_ptr cands;
{
  register rtx t_insn;
  register int cur_cand_no = 0;
  int dreg;
  int dreg_size;

  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    insn_gens[INSN_UID(t_insn)] = 0;

    if (GET_CODE(t_insn) == INSN &&
        GET_CODE(PATTERN(t_insn)) == SET &&
        (GET_CODE(SET_SRC(PATTERN(t_insn))) == CONST_INT  ||
         GET_CODE(SET_SRC(PATTERN(t_insn))) == CONST_DOUBLE))
    {
      enum machine_mode mode;

      mode = GET_MODE(SET_DEST(PATTERN(t_insn)));
      HARD_REGNO_REF(SET_DEST(PATTERN(t_insn)), &dreg, &dreg_size);

      if (dreg >= 0 && GET_MODE_CLASS(mode) == MODE_INT)
      {
        list_p p;
        enum machine_mode nxt_mode;
        list_p new_node;
        rtx cnst_val;
        int cand_no = -1;

        cnst_val = SET_SRC(PATTERN(t_insn));

        /*
         * We have candidate.  See if there is already a candidate with
         * the same destination register, mode, and constant value.  If
         * There is then these should be numbered the same for the dataflow
         * to work well.
         */
  
        for (p = cand_uses[dreg]; p != 0; p = p->next)
        {
          if (GET_LIST_CAND(p)->dreg == dreg &&
              GET_LIST_CAND(p)->cnst_mode == mode &&
              same_constant(GET_LIST_CAND(p)->cnst_rtx, cnst_val, mode))
            cand_no = GET_LIST_CAND(p)->cand_no;
        }
 
        if (cand_no == -1)
        {
          cand_no = cur_cand_no++;
          cands[cand_no].cand_no = cand_no;
          cands[cand_no].dreg = dreg;
          cands[cand_no].cnst_rtx = cnst_val;
          cands[cand_no].cnst_mode = mode;
          cands[cand_no].cand_insns = 0;
 
          /* add the cand to the uses list for the registers. */
          new_node = alloc_list_node(&list_blks);
          new_node->next = cand_uses[dreg];
          SET_LIST_ELT(new_node, &cands[cand_no]);
          cand_uses[dreg] = new_node;
        }

        new_node = alloc_list_node(&list_blks);
        new_node->next = insn_gens[INSN_UID(t_insn)];
        SET_LIST_ELT(new_node, &cands[cand_no]);
        insn_gens[INSN_UID(t_insn)] = new_node;

        new_node = alloc_list_node(&list_blks);
        new_node->next = cands[cand_no].cand_insns;
        SET_LIST_ELT(new_node, t_insn);
        cands[cand_no].cand_insns = new_node;
      }
    }
  }
  return cur_cand_no;
}

static void
cand_insn_kills(insn, kill_set, set_size)
rtx insn;
set kill_set;
int set_size;
{
  register set regs_killed;
  register int rgno;
  register list_p p;

  CLR_SET(kill_set, set_size);

  /* get the set of registers killed by the instruction. */
  regs_killed = s_regs_insn_sets;
  insn_reg_use (insn, 0, regs_killed, s_regs_insn_uses);

  FORALL_SET_BEGIN(regs_killed, SET_SIZE(FIRST_PSEUDO_REGISTER), rgno)
    if (rgno < FIRST_PSEUDO_REGISTER)
    {
      for (p = cand_uses[rgno]; p != 0; p = p->next)
        SET_SET_ELT(kill_set, GET_LIST_CAND(p)->cand_no);
    }
  FORALL_SET_END;
}

static void
cnst_comp_dfa(n_cands, in, pool)
int n_cands;
set *in;
set pool;
{
  int change;
  set *out;
  set *gen;
  set *kill;
  register set tmp_set;
  list_p *preds;
  int bb_num;
  register int set_size;
  register set in_bb;
  register set out_bb;
  register set gen_bb;
  register set kill_bb;

  /*
   * allocate gen, kill, in, out.
   */
  out  = in + N_BLOCKS;
  gen  = out + N_BLOCKS;
  kill = gen + N_BLOCKS;

  set_size = SET_SIZE(n_cands);

  /*
   * initialize gen, kill. in, out
   * kill[bb_num] is the set of copies that have their source
   * operand def'ed in bb_num.
   * gen[bb_num] is the set of copies that happen in bb_num and
   * whose source isn't defed later in bb_num.
   */
  tmp_set = pool;
  pool += set_size;

  for (bb_num=0; bb_num < N_BLOCKS; bb_num++)
  {
    rtx t_insn;

    t_insn = BLOCK_HEAD(bb_num);

    in[bb_num] = in_bb = pool;     pool += set_size;
    out[bb_num] = out_bb = pool;   pool += set_size;
    gen[bb_num] = gen_bb = pool;   pool += set_size;
    kill[bb_num] = kill_bb = pool; pool += set_size;

    CLR_SET(gen_bb, set_size);
    CLR_SET(kill_bb, set_size);

    while (1)
    {
      /*
       * get the set of copies that the insn kills. Add them into the set
       * of kills for the block and delete them from the set of copies this
       * block gens.
       */
      cand_insn_kills (t_insn, tmp_set, set_size);
      OR_SETS(kill_bb, kill_bb, tmp_set, set_size);
      AND_COMPL_SETS(gen_bb, gen_bb, tmp_set, set_size);

      /*
       * Add the copies this insn gens into the basic block gens.
       */
      if (GET_CODE(t_insn) == INSN)
      {
        list_p t_p;
        for (t_p = insn_gens[INSN_UID(t_insn)]; t_p != 0; t_p = t_p->next)
          SET_SET_ELT(gen_bb, GET_LIST_CAND(t_p)->cand_no);
      }

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
}

static void
try_const_compress(insn, insn_in, n_cands)
rtx insn;
register set insn_in;
register int n_cands;
{
  int dreg;
  int dreg_size;
  register int set_size = SET_SIZE(n_cands);
  register int cand_no;
  cand_ptr cand;
  rtx cnst_rtx;

  if (GET_CODE(PATTERN(insn)) == SET &&
      (GET_CODE(SET_SRC(PATTERN(insn))) == CONST_INT ||
       GET_CODE(SET_SRC(PATTERN(insn))) == CONST_DOUBLE))
  {
    int is_expensive;
    enum machine_mode mode;
    rtx cnst_val;
    rtx dst;

    dst = SET_DEST(PATTERN(insn));
    mode = GET_MODE(dst);
    HARD_REGNO_REF(dst, &dreg, &dreg_size);

    if (dreg >= 0 &&
        (mode == QImode || mode == HImode ||
         mode == SImode || mode == DImode))
    {
      cnst_val = SET_SRC(PATTERN(insn));
      is_expensive = EXPENSIVE_CONST(cnst_val, mode);

      FORALL_SET_BEGIN(insn_in, set_size, cand_no)
        if (cand_no < n_cands)
        {
          cand = &s_cands[cand_no];
          if ((cnst_rtx = MAKE_CHEAP_CONST(cand->dreg, cand->cnst_mode,
                                           cand->cnst_rtx,
                                           mode, cnst_val, is_expensive)) != 0)
          {
            list_p p;
  
            if (GET_CODE(cnst_rtx) == REG && REGNO(cnst_rtx) == dreg)
            {
#if defined(IMSTG) && defined(DWARF_DEBUGGING_INFO) && DWARF_VERSION >= 2
	      if (flag_linenum_mods)
	        dwarfout_disable_notes(insn);
#endif
              /* delete useless register to register move. */
              PUT_CODE(insn, NOTE);
              NOTE_LINE_NUMBER(insn) = NOTE_INSN_DELETED;
              NOTE_SOURCE_FILE(insn) = 0;
              return;
            }

            /*
             * Only do a rewrite if its an expensive constant.  Otherwise
             * we are just looking to try to get rid of assignments.
             */
            if (is_expensive)
            {
              SET_SRC(PATTERN(insn)) = cnst_rtx;

              if (GET_MODE(cnst_rtx) != mode)
              {
                /* Need to modify the destination to match the new
                 * source mode.
                 */
                SET_DEST(PATTERN(insn)) = pun_rtx(dst, GET_MODE(cnst_rtx)); 
              }

              INSN_CODE(insn) = -1;

              /*
               * delete any REG_DEAD notes on the insns that generated the
               * register we used.
               */
              for (p = cand->cand_insns; p != 0; p = p->next)
              {
                rtx t_insn = GET_LIST_INSN(p);
                rtx rg_note;
                rtx prev_note;
  
                for (prev_note = 0, rg_note = REG_NOTES(t_insn); rg_note != 0;
                     prev_note = rg_note, rg_note = XEXP(rg_note, 1))
                {
                  if (REG_NOTE_KIND(rg_note) == REG_DEAD)
                  {
                    rtx rg;
                    if (GET_CODE((rg = XEXP(rg_note,0))) == REG &&
                        REGNO(rg) == cand->dreg)
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
              return;
            }
          }
        }
      FORALL_SET_END;
    }
  }
}

static void
cnst_comp_rewrite(n_cands, in, pool)
int n_cands;
set *in;
set pool;
{
  register int bb_num;
  register set tmp_set = pool;
  register int set_size = SET_SIZE(n_cands);
  
  for (bb_num = 0; bb_num < N_BLOCKS; bb_num++)
  {
    register set bb_in;
    register rtx t_insn;
    register rtx end_insn;

    t_insn = BLOCK_HEAD(bb_num);
    end_insn = BLOCK_END(bb_num);
    bb_in = in[bb_num];

    while (1)
    {
      /*
       * replace any copies that can be replaced here.
       */
      switch (GET_CODE(t_insn))
      {
        default:
          break;

        case INSN:
          try_const_compress(t_insn, bb_in, n_cands);
          /* Fall Thru */

        case JUMP_INSN:
        case CALL_INSN:
          /*
           * update bb_in to kill all copies that this insn causes
           * to die.
           */
          cand_insn_kills(t_insn, tmp_set, set_size);
          AND_COMPL_SETS(bb_in, bb_in, tmp_set, set_size);

          /*
           * update bb_in to include any copies that this instruction
           * gens.
           */
          if (GET_CODE(t_insn) == INSN)
          {
            list_p t_p;
            for (t_p = insn_gens[INSN_UID(t_insn)]; t_p != 0; t_p = t_p->next)
              SET_SET_ELT(bb_in, GET_LIST_CAND(t_p)->cand_no);
          }
          break;
      }

      if (t_insn == end_insn)
        break;

      t_insn = NEXT_INSN(t_insn);
    }
  }
}

/*
 * Run through all the insns making all constants that are direct sources
 * of sets to registers be at least as large as the natural machine registers.
 * This essentially unifies the constant values in registers so that they
 * are easily comparable in the constant optimizations.
 */
int
prepare_consts(insns)
rtx insns;
{
  register rtx t_insn;
  int dreg;
  int dreg_size;

  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    if (GET_CODE(t_insn) == INSN &&
        GET_CODE(PATTERN(t_insn)) == SET &&
        (GET_CODE(SET_SRC(PATTERN(t_insn))) == CONST_INT ||
         GET_CODE(SET_SRC(PATTERN(t_insn))) == CONST_DOUBLE))
    {
      HARD_REGNO_REF(SET_DEST(PATTERN(t_insn)), &dreg, &dreg_size);
      if (dreg > 0)
      {
        register enum machine_mode mode;
        register rtx src = SET_SRC(PATTERN(t_insn));

        mode = GET_MODE(SET_DEST(PATTERN(t_insn)));

        if (GET_MODE_CLASS(mode) == MODE_INT)
        {
          if (GET_MODE_SIZE(mode) < GET_MODE_SIZE(word_mode))
            SET_DEST(PATTERN(t_insn)) = gen_rtx(REG, word_mode, dreg);
        }
      }
    }
  }
}

int
const_compression(insns)
rtx insns;
{
  int n_cands;
  int max_uid;
  cand_ptr cands;
  set *in;
  set pool;

  /* get basic block info */
  prep_for_flow_analysis(insns);

  /*
   * First get a rough number of candidates. Then
   * we allocate space for them.  Next get all candidates, fill
   * in the candidate array.
   */
  find_bounds(insns, &max_uid, &n_cands);
  if (n_cands == 0)
    return;

  /* set up predecessor list */
  s_preds = (list_p *)xmalloc(N_BLOCKS * sizeof(list_p));
  bzero(s_preds, N_BLOCKS * sizeof(list_p));
  compute_preds_succs(s_preds, (list_p *)0, &list_blks);

  /* allocate space for data structures */
  cands = (cand_ptr)xmalloc(n_cands * sizeof(struct cand_rec));
  insn_gens = (list_p *)xmalloc((max_uid+1) * sizeof(list_p));
  cand_uses = (list_p *)xmalloc(FIRST_PSEUDO_REGISTER * sizeof(list_p));
  bzero(cand_uses, FIRST_PSEUDO_REGISTER * sizeof(list_p));
  
  n_cands = find_candidates(insns, cands);
  s_cands = cands;

  if (n_cands != 0)
  {
    in = (set *)xmalloc(N_BLOCKS * sizeof(set) * 4);
    pool = ALLOC_SETS((N_BLOCKS * 4) + 1, SET_SIZE(n_cands));
    cnst_comp_dfa(n_cands, in, pool);

    cnst_comp_rewrite(n_cands, in, pool);

    free (pool);
    free (in);
  }

  free (cand_uses);
  free (insn_gens);
  free (cands);
  free (s_preds);
  free_lists(&list_blks);
  free_all_flow_space();
}
#endif
