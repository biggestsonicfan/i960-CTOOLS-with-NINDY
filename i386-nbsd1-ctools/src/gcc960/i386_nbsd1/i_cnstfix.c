#include "config.h"

#ifdef IMSTG
#include "rtl.h"
#include "regs.h"
#include "flags.h"

/*
 * This file contains the code that fixes up constant expression rtl to
 * be legal before doing flow analysis.  It only fixes up expressions
 * into legal rtls, it doesn't try to fixup simple constants that may also
 * end up needing to go into registers.  Reload will fix those constants
 * up.  Then after reload another special pass will allocate constants
 * to more global registers if possible.
 */

int const_fixup_happened;
static int any_fixup_done;

static void do_fixups();

/*
 * Fixup a reference of a constant expression which although compile time
 * constant, is not compile time computable due to limitations in the
 * assembly language, or even in the constant folding in the compiler.
 *
 * On entry:
 *   insn is the rtx for the insn the constant expression that needs to be
 *        fixed up is in.
 *   x is the rtx for the constant expression that needs to be fixed up.
 *   x_p is a pointer to the place in the rtx tree that references rtx x.
 *
 * Algorithm:
 *   Create an insn which is set pseudo-reg const-expression, and put this
 *   into the rtl immediately ahead of the insn being replaced in. Replace
 *   x in the original rtx tree by a reference to the pseudo-reg.
 *   Then look at the operands of the constant expression and see if they
 *   need fixing up. If they do then call this routine recursively passing
 *   the new insn generated, and the place in the rtx that references the
 *   stuff needing fixup.
 */
static void
fixup_const(insn, x, x_p, mode, already_legal_const)
rtx insn;
rtx x;
rtx *x_p;
enum machine_mode mode;
int already_legal_const;
{
  enum rtx_code c0;
  enum rtx_code c1;
  rtx new_insn;
  rtx new_reg_rtx;

  if (mode == VOIDmode)
    abort();

  if (!already_legal_const)
  {
    /*
     * special case of constant plus symbol_ref, symbol_ref plus constant,
     * symbol_ref - constant only need to have const node added on top of
     * them.
     */
    switch (GET_CODE(x))
    {
      case PLUS:
        c0 = GET_CODE(XEXP(x,0));
        c1 = GET_CODE(XEXP(x,1));
        if (((c0 == SYMBOL_REF || c0 == LABEL_REF) && c1 == CONST_INT) ||
            ((c1 == SYMBOL_REF || c1 == LABEL_REF) && c0 == CONST_INT))
        {
          /* just add a const node on top of this node */
          *x_p = gen_rtx(CONST, mode, x);
          INSN_CODE(insn) = -1;
          return;
        }
        break;
  
      case MINUS:
        c0 = GET_CODE(XEXP(x,0));
        c1 = GET_CODE(XEXP(x,1));
        if ((c0 == SYMBOL_REF || c0 == LABEL_REF) && c1 == CONST_INT)
        {
          /* just add a const node on top of this node */
          *x_p = gen_rtx(CONST, mode, x);
          INSN_CODE(insn) = -1;
          return;
        }
        break;
  
      case CONST:
        x = XEXP(x,0);
        if (GET_CODE(x) == PLUS)
        {
          c0 = GET_CODE(XEXP(x,0));
          c1 = GET_CODE(XEXP(x,1));
          if (((c0 == SYMBOL_REF || c0 == LABEL_REF) && c1 == CONST_INT) ||
              ((c1 == SYMBOL_REF || c1 == LABEL_REF) && c0 == CONST_INT))
            return;
        }
        else if (GET_CODE(x) == MINUS)
        {
          c0 = GET_CODE(XEXP(x,0));
          c1 = GET_CODE(XEXP(x,1));
          if ((c0 == SYMBOL_REF || c0 == LABEL_REF) && c1 == CONST_INT)
            return;
        }
        break;
  
      case MEM:
        x_p = &XEXP(x, 0);
        x = *x_p;
        if (GET_CODE(x) == CONST)
          x = XEXP(x,0);
        mode = GET_MODE(x);
        break;

      default:
        break;
    }
  }

  any_fixup_done = 1;

  /* generate the new register rtx that is necessary */
  new_reg_rtx = gen_reg_rtx(mode);

  /* generate and emit the new insn */
  /* If this is a call insn, then we need to back up past the use insns and
   * emit any fixup code before them.  If this is a jump_insn that uses cc0,
   * we need to back up to before the set of cc0 to fixup.
   */
  if (GET_CODE(insn) == CALL_INSN)
  {
    rtx use_insn = insn;
    while (PREV_INSN(use_insn) != 0 &&
           GET_CODE(PREV_INSN(use_insn)) == INSN &&
           GET_CODE(PATTERN(PREV_INSN(use_insn))) == USE)
      use_insn = PREV_INSN(use_insn);
    new_insn = emit_insn_before (gen_rtx (SET, VOIDmode, new_reg_rtx, x),
                                 use_insn);
  }
  else if (GET_CODE(insn) == JUMP_INSN)
  {
    /* Two interesting cases here.  If the machine has CC0, then the
     * immediately previous insn is the one that set it, otherwise it doesn't
     * matter where we put it.
     */
    rtx prev_insn = PREV_INSN(insn);
    if (prev_insn == 0 || GET_CODE(prev_insn) == CODE_LABEL)
      prev_insn = insn;
    new_insn = emit_insn_before (gen_rtx (SET, VOIDmode, new_reg_rtx, x),
                                 prev_insn);
  }
  else
    new_insn = emit_insn_before (gen_rtx (SET, VOIDmode, new_reg_rtx, x),
                                 insn);
  /*
   * Add REG_EQUIV note instruction that was just generated.
   */
  REG_NOTES(new_insn) = gen_rtx (EXPR_LIST, REG_EQUIV,
                                 copy_rtx(x), NULL_RTX);

  /* replace x with the new register rtx created. */
  *x_p = new_reg_rtx;
  INSN_CODE(insn) = -1;

  /*
   * If the old insn had any REG_EQUAL, or REG_EQUIV notes, they ought to be
   * deleted, since they probably refer to the constant expression we are
   * currently breaking up.
   */
  {
    rtx prev = 0;
    rtx link = REG_NOTES(insn);
    while (link != 0)
    {
      if (REG_NOTE_KIND(link) == REG_EQUAL || REG_NOTE_KIND(link) == REG_EQUIV)
      {
        if (prev == 0)
          REG_NOTES(insn) = XEXP(link,1);
        else
          XEXP(prev,1) = XEXP(link,1);
        link = XEXP(link,1);
      }
      else {
        prev = link;
        link = XEXP(link,1);
      }
    }
  }

  /*
   * now see if there is further rtx within x that needs to be
   * fixed up.
   */
  if (GET_CODE(x) == CONST)
    x = PATTERN(new_insn);
  do_fixups(new_insn, x);
}

/*
 * Return 2 if the rtx x is an expression that needs to be fixed up.
 * Return 1 if the rtx x is a constant expression that needs no fixup.
 * Return -1 if the rtx should not be fixed up and must not be visited further.
 * Return 0 otherwise.
 * For a constant expression to need fixing up means
 * that this can't be represented in the rtl as a constant due to limitations
 * of the assembler or compiler's constant folding.
 */
static int
needs_fixup_p(parent, x)
rtx parent;
rtx x;
{
  /*
   * We treat this specially here and in fixup.  This is done because
   * GCC 2.0 cannot handle having a modeless constant as the first
   * operand of a compare, so this must be fixed up at this point.
   */
  if (GET_CODE(parent) == COMPARE &&
      XEXP(parent, 0) == x &&
      rtx_is_constant_p(x, (rtx *)0))
    return 3;

  switch (GET_CODE(x))
  {
    case COMPARE:
      /* don't fix up a constant compare, but may need to fix its operands */
      return 0;

    case CONST_INT:
    case CONST_DOUBLE:
    case SYMBOL_REF:
    case LABEL_REF:
      /* these are constants that need no fixing. */
      return 1;

    case CONST:
      {
        enum rtx_code c0, c1;

        if (GET_CODE(XEXP(x,0)) == PLUS)
        {
          c0 = GET_CODE(XEXP(XEXP(x,0),0));
          c1 = GET_CODE(XEXP(XEXP(x,0),1));

          if (((c0 == SYMBOL_REF || c0 == LABEL_REF) && c1 == CONST_INT) ||
              ((c1 == SYMBOL_REF || c1 == LABEL_REF) && c0 == CONST_INT))
            return 1;
        }
        else if (GET_CODE(XEXP(x,0)) == MINUS)
        {
          c0 = GET_CODE(XEXP(XEXP(x,0),0));
          c1 = GET_CODE(XEXP(XEXP(x,0),1));

          if ((c0 == SYMBOL_REF || c0 == LABEL_REF) && c1 == CONST_INT)
            return 1;
        }
      }
      return 2;

    case MEM:
      GO_IF_LEGITIMATE_ADDRESS(GET_MODE(x), XEXP(x,0), win);
      /* It should no longer be possible to get here; nobody should
       * be generating addresses which are illegal.
       */
      abort();
      return 2;
      win: ;
      /* Do not visit the address */
      return -1;

    case REG:
    case CC0:
    case PC:
    case SUBREG:
      /* these are easy non-constants that need no fixing */
      return -1;

    default:
      /*
       * if the expression is constant, then it needs fixing up,
       * otherwise it does not, but its children may.
       */
      if (rtx_is_constant_p(x, (rtx *) 0))
        return 2;
      return 0;
  }
}

/* XEXP(x,i) is a constant.  Return VOIDmode if we should not load it now;
 * return the right mode to load it into if we should load it up now.
 *
 * Constant propagation has already run, and loop and second cse have yet
 * to run.  The idea is, if it seems that the constant is eventually
 * going to be loaded anyway, and it is a fairly expensive load,
 * we want to load it up now, in the hope that loop and cse will do the
 * right things.
 */
static enum machine_mode
fixup_const_op_mode(x,i)
rtx x;
int i;
{
  enum machine_mode m   = GET_MODE(x);
  enum rtx_code     c   = GET_CODE(x);
  enum machine_mode ret = VOIDmode;

  if (c == COMPARE)
  {
    m = GET_MODE(XEXP(x,0));
    if (m == VOIDmode)
      m = GET_MODE(XEXP(x,1));
    if (m == VOIDmode)
      m = COMPARE_MODE(x);
  }

  switch (GET_RTX_CLASS(c))
  {
    /* Shouldn't see anything we can usefully break out except as
     * operands of binary operators.  If we do have a binary operator,
     * we want to break XEXP(x,i) out if it is not cheap.
     */

    case '<':
    case 'c':
    case '2':
      if (GET_MODE_CLASS(m)==MODE_INT || GET_MODE_CLASS(m)==MODE_FLOAT)
        if (rtx_cost (XEXP(x,i), c) > 1)
          ret = m;
      break;

    case 'x':
      /* split out a store to memory of a constant if it is non-zero. */
      if (GET_CODE(x) == SET && i == 1 &&
          memory_operand(SET_DEST(x), GET_MODE(SET_DEST(x))))
      {
        if (GET_CODE(XEXP(x,i)) == CONST_INT && INTVAL(XEXP(x,i)) == 0)
          return VOIDmode;
        ret = GET_MODE(SET_DEST(x));
      }
      break;
  }
  return ret;
}

/*
 * Find and fixup any subtrees of the rtx x which are constant expressions
 * in need of fixups.  insn is the rtl instruction in which the rtx x appears
 * someplace.
 */
static void
do_fixups(insn, x)
rtx insn;
rtx x;
{
  register char *fmt;
  register int i;
  register int i_end;
  register rtx t_rtx;
  register enum machine_mode m;

  fmt = GET_RTX_FORMAT(GET_CODE(x));
  i_end = GET_RTX_LENGTH(GET_CODE(x));

  for (i = 0; i < i_end; i++)
  {
    if (fmt[i] == 'e')
    {
      t_rtx = XEXP(x,i);
      if (t_rtx != 0)
      {
        switch (needs_fixup_p(x, t_rtx))
        {
          case 3:
            fixup_const (insn, t_rtx, &XEXP(x,i), COMPARE_MODE(x), 1);
            break;
          case 2:
            fixup_const (insn, t_rtx, &XEXP(x,i), GET_MODE(t_rtx), 0);
            break;
          case 1:
            /* Further examine constants, and fixup expensive ones to
             * expose the fixups for loop and 2nd cse.  This step is not
             * required for correctness.
             */
            if ((m=fixup_const_op_mode(x,i)) != VOIDmode)
              fixup_const (insn, t_rtx, &XEXP(x,i), m, 1);
            break;
          case 0:
            do_fixups(insn, t_rtx);
            break;
          case -1:
            /* We explicitly do not want to visit the children.  This is the
             * normal path for MEMs with valid addresses.  We also come here
             * if we happen to know there are no children.
             */
            break;
          default:
            abort();
        }
      }
    }
    else if (fmt[i] == 'E')
    {
      register int j;
      register int j_end;

      j_end = XVECLEN(x,i);
      for (j = 0; j < j_end; j++)
      {
        t_rtx = XVECEXP(x,i,j);

        if (t_rtx != 0)
        {
          switch (needs_fixup_p(x, t_rtx))
          {
            case 3:
              fixup_const (insn, t_rtx, &XVECEXP(x,i,j), COMPARE_MODE(x), 1);
              break;
            case 2:
              fixup_const (insn, t_rtx, &XVECEXP(x,i,j), GET_MODE(t_rtx), 0);
              break;
            case  1:
              break;
            case 0:
              do_fixups(insn, t_rtx);
              break;
            case -1:
              break;
            default:
              abort();
          }
        }
      }
    }
  }
}

/*
 * Run through all the rtl for the function and if the instruction needs
 * to be fixed up do so.
 */
int
cleanup_consts(insns)
rtx insns;
{
  register rtx t_insn;

  any_fixup_done = 0;

  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    /*
     * We only fixup real INSNs, Nothing else should ever
     * need fixing up.
     */
    if (GET_RTX_CLASS(GET_CODE(t_insn)) == 'i')
      do_fixups(t_insn, PATTERN(t_insn));
  }

  const_fixup_happened = 1;
  return any_fixup_done;
}
#endif
