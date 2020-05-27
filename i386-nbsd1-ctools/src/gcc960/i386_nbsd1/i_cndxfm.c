#include "config.h"

#ifdef IMSTG
/*
 * Transformation of some basic blocks into conditional move instructions.
 */
#include "rtl.h"
#include "regs.h"
#include "flags.h"
#include "basic-block.h"
#include "i_list.h"

#ifndef MAX_CND_XFRM_INSNS
#define MAX_CND_XFRM_INSNS 4
#endif

#define REG_OR_CONST_P(X) \
  (GET_CODE(X) == REG || GET_CODE(X) == SUBREG || CONSTANT_P(X))

static list_block_p pred_list_blks;
static list_p *s_preds;
static list_p *s_succs;

static short *reg_only_in_bb;
static int *reg_dead_info;

static int
reg_dead_in_bb(reg, bb)
rtx reg;
int bb;
{
  int regno = REGNO(reg);
  unsigned int val;
  rtx t_insn;

  val = reg_dead_info[regno];
  if ((val >> 1) == bb)
    return (val & 1);

  val = bb << 1;
  t_insn = BLOCK_HEAD(bb);
  for (t_insn = BLOCK_HEAD(bb); t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    if (GET_RTX_CLASS (GET_CODE (t_insn)) == 'i')
    {
      if (reg_referenced_p(reg, PATTERN(t_insn)))
        break;

      if (reg_set_p(reg, t_insn))
      {
        val |= 1;
        break;
      }
    }

    if (t_insn == BLOCK_END(bb))
      break;
  }

  reg_dead_info[regno] = val;
  return (val & 1);
}

/*
 * This routine transforms a conditional basic-block into a series of
 * conditional instructions.  It returns 1 iff all instructions in the basic
 * block are transformed, 0 otherwise.
 */
static int
transform_block(cjmp, bb_num, alt_bb, bb_pred)
rtx cjmp;
int bb_num;
int alt_bb;
int bb_pred;
{
  rtx after = PREV_INSN(cjmp);
  rtx t_insn = BLOCK_HEAD(bb_num);
  rtx t_next;
  int dummy;
  rtx dst;
  rtx src;
  rtx rdst;
  rtx cond;
  rtx cjmp_pat;
  rtx split_insn;
  rtx new_insn;
  enum machine_mode mode;
  int need_selcc;
  int loop = 1;
  
  while (loop)
  {
    t_next = NEXT_INSN(t_insn);
    if (t_insn == BLOCK_END(bb_num))
      loop = 0;

    switch (GET_CODE(t_insn))
    {
      case INSN:
        if (GET_CODE(PATTERN(t_insn)) != SET)
          abort();

        dst = SET_DEST(PATTERN(t_insn));
        src = SET_SRC(PATTERN(t_insn));
        cjmp_pat = SET_SRC(PATTERN(cjmp));
        cond = XEXP(cjmp_pat,0);
        mode = GET_MODE(dst);

        rdst = dst;
        if (GET_CODE(dst) == SUBREG)
          rdst = XEXP(dst,0);

        need_selcc = !reg_only_in_bb[REGNO(rdst)];

        if (need_selcc)
          need_selcc = !reg_dead_in_bb(rdst, alt_bb);

        /*
         * If the destination register is only set and used within this
         * basic-block, or is dead in the alternate control flow, then it
         * doesn't need to be conditionally assigned.
         * This will save some instructions, as well as improve likelyhood
         * of gaining other improvements from other optimizations.
         */
        if (!need_selcc)
        {
          rtx note;
          /*
           * Just move this insn from where it is to after 'after'.
           * However, if this is the beginning or end of a basic-block,
           * then create a new note deleted to take t_insn's place in the
           * insn stream, and update BLOCK_HEAD, or BLOCK_END.
           */
          if (t_insn == BLOCK_HEAD(bb_num))
          {
            note = emit_note_before(NOTE_INSN_DELETED, t_insn);
            BLOCK_HEAD(bb_num) = note;
          }

          if (t_insn == BLOCK_END(bb_num))
          {
            note = emit_note_before(NOTE_INSN_DELETED, t_insn);
            BLOCK_END(bb_num) = note;
          }

          reorder_insns(t_insn, t_insn, after);
          after = t_insn;
          break;
        }

        split_insn = 0;
        if (!REG_OR_CONST_P(src))
        {
          rtx treg = gen_reg_rtx(mode);
          split_insn = make_insn_raw(gen_rtx(SET, VOIDmode, treg, src));
          src = treg;

          if (recog(PATTERN(split_insn), split_insn, &dummy) < 0)
            return 0;  /* failed, must quit this block now. */
        }

        /*
         * need to know if this is the fall through block, or if this block
         * is the one jumped to to know which side to put dst and src on.
         *
         * Fall thru blocks basic-block number is always predecessor block
         * number + 1.
         *
         * Also need to know if fall-thru is on then or else side of
         * if then else.
         */

        if ((bb_num == bb_pred + 1 && XEXP(cjmp_pat, 1) == pc_rtx) ||
            (bb_num != bb_pred + 1 && XEXP(cjmp_pat, 1) != pc_rtx))
          src = gen_rtx (IF_THEN_ELSE, GET_MODE(dst),
                         copy_rtx(cond), src, copy_rtx(dst));
        else
          src = gen_rtx (IF_THEN_ELSE, GET_MODE(dst),
                         copy_rtx(cond), copy_rtx(dst), src);

        new_insn = make_insn_raw(gen_rtx(SET, VOIDmode, dst, src));
      
        if (recog(PATTERN(new_insn), new_insn, &dummy) < 0)
          return 0;  /* failed, must quit this block now */

        if (split_insn != 0)
        {
          add_insn_after(split_insn, after);
          after = split_insn;
        }

        add_insn_after(new_insn, after);
        after = new_insn;

        /* delete the old instruction */
        PUT_CODE(t_insn, NOTE);
        NOTE_LINE_NUMBER (t_insn) = NOTE_INSN_DELETED;
        NOTE_SOURCE_FILE (t_insn) = 0;
        NOTE_CALL_COUNT (t_insn) = 0;
        break;

      case NOTE:
        if (NOTE_LINE_NUMBER(t_insn) > 0)
        {
          /* just move it after "after" */
          reorder_insns(t_insn, t_insn, after);
          after = t_insn;
        }
        break;
    }
    t_insn = t_next;
  }

  return 1;
}

/*
 * return -1 if the insn is not OK for the transformation,
 * 0 if the insn is OK, but won't need to be copied, and
 * 1 if the insn is OK and will need to be copied, and
 * 2 if the insn is OK but it will need to be both split and copied.
 */
static int
insn_ok_for_transform_p(t_insn, bb, alt_bb)
rtx t_insn;
int bb;
int alt_bb;
{
  rtx dst;
  rtx src;
  rtx rdst;
  int need_selcc;
  rtx end_bb_insn = BLOCK_END(bb);

  switch (GET_CODE(t_insn))
  {
    case INSN:
      if (GET_CODE(PATTERN(t_insn)) != SET)
        return -1;

      dst = SET_DEST(PATTERN(t_insn));
      src = SET_SRC(PATTERN(t_insn));

      if (GET_CODE(dst) != REG && GET_CODE(dst) != SUBREG)
        return -1;

      if (GET_MODE_CLASS(GET_MODE(dst)) == MODE_CC)
        return -1;

      rdst = dst;
      if (GET_CODE(dst) == SUBREG)
        rdst = XEXP(dst,0);

      if (reg_only_in_bb[REGNO(rdst)] >= 0)
        need_selcc = !reg_only_in_bb[REGNO(rdst)];
      else
      {
        int dreg_is_tmp;
        dreg_is_tmp = reg_in_basic_block_p(t_insn, rdst);
        reg_only_in_bb[REGNO(rdst)] = dreg_is_tmp;
        need_selcc = !dreg_is_tmp;
      }

      if (need_selcc)
        need_selcc = !reg_dead_in_bb(rdst, alt_bb);

      if (REG_OR_CONST_P(src))
        return 1;

      switch (GET_CODE(src))
      {
        case PLUS:
        case MINUS:
          if (REG_OR_CONST_P(XEXP(src,0)) &&
              REG_OR_CONST_P(XEXP(src,1)))
            return 1;  /* this requires a split & copy but its cheap op so
                          return like its just a copy. */
          break;

        case NEG:
          if (REG_OR_CONST_P(XEXP(src,0)))
            return 1;
          break;

        case MEM:
          if (!volatile_refs_p(src) &&
              (CAN_MOVE_MEM_P(src) || !may_trap_p(src)))
            return 2 - !need_selcc;
          break;

        case ZERO_EXTEND:
        case SIGN_EXTEND:
          /*
           * these can both occur right on top of mems, so do the special
           * check for MEM's here also.
           */
          if (GET_CODE(XEXP(src,0)) == MEM)
          {
            if (!volatile_refs_p(src) &&
                (CAN_MOVE_MEM_P(XEXP(src,0)) || !may_trap_p(src)))
              return 2 - !need_selcc;
            break;
          }
          /* fall thru */
          
        default:
          if (!volatile_refs_p(src) && !may_trap_p(src))
            return 2 - !need_selcc;
          break;
      }
      return -1;

    case JUMP_INSN:
      if (simplejump_p(t_insn) && t_insn == end_bb_insn)
        return 0;

      return -1;

    case CALL_INSN:
      return -1;

    default:
      return 0;
  }
}

int
do_cond_xform(insns)
rtx insns;
{
  int bb_num;
  rtx t_insn;
  rtx cond_jump;
  rtx cond;
  int any_transformed = 0;
  int max_new_insns = MAX_CND_XFRM_INSNS;
  int n_regs;
  int i;

  if (max_new_insns == 0)
    return 0;

  /* First break the function up into basic blocks */
  prep_for_flow_analysis (insns);

  /* figure out how many pred nodes are needed. */
  s_preds = (list_p *)xmalloc(N_BLOCKS * sizeof(list_p));
  s_succs = (list_p *)xmalloc(N_BLOCKS * sizeof(list_p));

  compute_preds_succs(s_preds, s_succs, &pred_list_blks);

  n_regs = max_reg_num ();
  reg_scan (insns, n_regs, 1);
  reg_only_in_bb = (short *)xmalloc(n_regs * sizeof(short));
  reg_dead_info = (int *)xmalloc(n_regs * sizeof(int));
  for (i = 0; i < n_regs; i++)
  {
    reg_only_in_bb[i] = -1;
    reg_dead_info[i] = -1;
  }

  /*
   * Now look for basic-blocks that are candidates for transformation.
   *
   * In order for a block to be eligible for transformation it must
   * have the following properties:
   *
   *  1.  The basic-block must have a single predecessor and a single
   *      successor in the control flow graph.
   *
   *  2.  The basic-blocks predecessor must end in a conditional branch.
   *
   *  3.  The basic-block must contain only instructions that are
   *      register to register assignments, or constant to register
   *      assignments.  An unconditional jump ending the basic-block
   *      is exempted from this restriction.
   *
   * Let DST be a destination of the first instruction in a block that
   * is eligible for transformation, and SRC be the source.
   * Then the instruction is transformed into
   *
   *  set DST if_then_else COND SRC DST
   *
   * and this new instruction is moved into the predecessor block immediately
   * preceding the conditional jump.  This process continues forward through
   * the basic-block until all the assignments are processed.  Any jumps are
   * left in place to be removed by jump optimization.
   */
  for (bb_num = 0; bb_num < N_BLOCKS; bb_num += 1)
  {
    int num_new_insns = 0;
    int pred_bb;
    int alt_bb;   /*
                   * This is the basic-block that is the alternate flow
                   * path of the basic-block's predecessor.
                   */

    /*
     * 1.  The basic-block must have a single predecessor and a single
     *     successor in the control flow graph.
     */
    if (!(s_preds[bb_num] != 0 && s_preds[bb_num]->next == 0 &&
          s_succs[bb_num] != 0 && s_succs[bb_num]->next == 0))
      /* Failed first requirement */
      goto failed_req;

    /* 
     * 2.  The basic-blocks predecessor must end in a conditional branch.
     */
    pred_bb = GET_LIST_ELT(s_preds[bb_num], int);
    cond_jump = BLOCK_END(pred_bb);
    if (!(GET_CODE(cond_jump) == JUMP_INSN &&
          condjump_p(cond_jump) && !simplejump_p(cond_jump)))
      /* Failed second requirement */
      goto failed_req;

    /*
     * 2.5. The first real insn preceding this basic-block must be a jump.
     *      Then if the target of the jump is either:
     *        a.  A label that immediately follows this basic-block.
     *                         or
     *        b.  The target of an unconditional jump at the end of this
     *            basic-block.
     *
     *      Then we will be able to delete a jump by transforming this
     *      basic-block.
     */
    {
      rtx targ;

      t_insn = prev_real_insn(BLOCK_HEAD(bb_num));
      if (t_insn == 0 || (!simplejump_p(t_insn) && !condjump_p(t_insn)))
        goto failed_req;
    
      targ = JUMP_LABEL(t_insn);
      if (JUMP_LABEL(t_insn) != next_label(BLOCK_END(bb_num)) &&
          (!simplejump_p(BLOCK_END(bb_num)) ||
           JUMP_LABEL(BLOCK_END(bb_num)) != targ))
        goto failed_req;
    }

#if 0
    /*
     * require that if this block ends in a unconditional jump, that the
     * unconditional jump not go to a block that ends in a return.  If it
     * does, then the return basic-block could just be copied up to here,
     * so we won't be saving a branch.
     */
    t_insn = BLOCK_END(bb_num);
    if (GET_CODE(t_insn) == JUMP_INSN && simplejump_p(t_insn))
    {
      t_insn = BLOCK_END(GET_LIST_ELT(s_succs[bb_num], int));
      if (GET_CODE(t_insn) == JUMP_INSN && GET_CODE(PATTERN(t_insn)) == RETURN)
        goto failed_req;
    }
#endif

    alt_bb = GET_LIST_ELT(s_succs[pred_bb], int);
    if (alt_bb == bb_num)
      alt_bb = GET_LIST_ELT(s_succs[pred_bb]->next, int);

    /*
     * 3. The basic-block must contain only instructions that are
     *    register to register assignments, or constant to register
     *    assignments.  An unconditional jump ending the basic-block
     *    is exempted from this restriction.
     */
    t_insn = BLOCK_HEAD(bb_num);
    while (1)
    {
      int t = insn_ok_for_transform_p(t_insn, bb_num, alt_bb);

      if (t == -1)
        goto failed_req;

      num_new_insns += t;

      if (num_new_insns > max_new_insns)
        goto failed_req;

      if (t_insn == BLOCK_END(bb_num))
        break;

      t_insn = NEXT_INSN(t_insn);
    }

    any_transformed += 
      transform_block(cond_jump, bb_num, alt_bb, pred_bb);

    failed_req: ;
  }
   

  free (s_preds);
  free (s_succs);
  free_lists(&pred_list_blks);
  free (reg_only_in_bb);
  free (reg_dead_info);

  return (any_transformed);
}
#endif
