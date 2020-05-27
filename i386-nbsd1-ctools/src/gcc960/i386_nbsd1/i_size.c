
#include "config.h"

#ifdef IMSTG
#if 0
#include "tree.h"	/* only necessary for dataflow.h */
#endif
#include "rtl.h"
#include "real.h"
#if 0
#include "regs.h"
#include "flags.h"
#endif

#include "assert.h"
#if 0
#include "basic-block.h"
#include "i_dataflow.h"
#include "i_df_set.h"
#include "insn-flags.h"
#endif

static int
i960_estimate_const_int (mode, val)
enum machine_mode mode;
unsigned int val;
{ /* Estimate number of words to load 'val' */

  unsigned hi = (val & 0x80000000) ? -1 : 0;
  unsigned dum_lo, dum_hi;
  int vlo;

  assert (mode==QImode || mode||HImode || mode==SImode);

  zero_extend_double (mode, val, hi, &dum_lo, &dum_hi);
  vlo = dum_lo;

  if (vlo >= 0)
  { /* ldconst 0..31,X  ->  mov 0..31,X  */
    /* ldconst 32..63,X -> add 31,nn,X  */
    if (vlo < 63)
      return 1;
  }
  else if (vlo < 0)
  { /* ldconst -1..-31  -> sub 0,0..31,X  */
    if (vlo >= -31)
      /* return 'sub -(%1),0,%0' */
      return 1;
  
    /* ldconst -32  -> not 31,X  */
    if (vlo == -32)
      return 1;
  }

  /* If const is a single bit.  */
  if (bitpos (vlo) >= 0)
    return 1;

  /* If const is a bit string of less than 6 bits (1..31 shifted).  */
  if (is_mask (vlo))
  { int s, e;
    if (bitstr (vlo, &s, &e) < 6)
      return 1;
  }
  return 2;
}

#define SPLIT_BIG_MOVES (((TARGET_J_SERIES) && !flag_space_opt))

static int
i960_estimate_ldconst (mode, src)
enum machine_mode mode;
rtx src;
{
  /* Anything that isn't a compile time constant, such as a SYMBOL_REF,
     must be a ldconst insn.  */

  if (GET_CODE (src) != CONST_INT && GET_CODE (src) != CONST_DOUBLE)
    return 2;

  switch (mode)
  { long f[4];
    REAL_VALUE_TYPE d;
    unsigned int tg_lo;
    unsigned int tg_hi;

    case TFmode:
    { assert (GET_CODE(src)==CONST_DOUBLE);
      REAL_VALUE_FROM_CONST_DOUBLE (d, src);
      REAL_VALUE_TO_TARGET_LONG_DOUBLE(d,f);

      if (f[0]==0 && f[1]==0 && f[2]==0)
        return SPLIT_BIG_MOVES ? 3 : 1;

      if (fp_literal(src,VOIDmode))
        return 1;

      return i960_estimate_const_int (SImode, f[0]) +
             i960_estimate_const_int (SImode, f[1]) +
             i960_estimate_const_int (SImode, f[2]);
    }

    case DFmode:
    { assert (GET_CODE(src)==CONST_DOUBLE);
      REAL_VALUE_FROM_CONST_DOUBLE (d, src);
      REAL_VALUE_TO_TARGET_DOUBLE(d,f);

      if (f[0]==0 && f[1]==0)
        return SPLIT_BIG_MOVES ? 2 : 1;

      if (fp_literal(src,VOIDmode))
        return 1;

      return i960_estimate_const_int (SImode, f[0]) +
             i960_estimate_const_int (SImode, f[1]);
    }

    case SFmode:
    { assert (GET_CODE(src)==CONST_DOUBLE);
      REAL_VALUE_FROM_CONST_DOUBLE (d, src);
      REAL_VALUE_TO_TARGET_SINGLE(d, f[0]);

      return i960_estimate_const_int (SImode, f[0]);
    }

    case DImode:
    { get_cnst_val(src, VOIDmode, &tg_lo, &tg_hi, 0);

      if (tg_hi == 0 && tg_lo <= 31)
        return SPLIT_BIG_MOVES ? 2 : 1;

      return i960_estimate_const_int (SImode, tg_hi) +
             i960_estimate_const_int (SImode, tg_lo);
    }

    case QImode:
    case HImode:
    case SImode:
    { get_cnst_val(src, VOIDmode, &tg_lo, &tg_hi, 0);
      return i960_estimate_const_int (mode, tg_lo);
    }

    case TImode:
    default:
      abort();
  }
}

typedef struct {
  unsigned short cons;
  unsigned char  reg1;
  unsigned char  reg2;
  unsigned char  mode;
  unsigned int   cost;
} op_model;

static op_model zero_op;
#define PCRG (( 1 + FIRST_PSEUDO_REGISTER ))
#define CCRG (( 1 + PCRG ))
#define PREG (( 1 + CCRG ))
#define MEMA (( 1 + PREG ))
#define MEMB (( 1 + MEMA ))
#define CONS (( 2048     ))

#define is_mema_addr(op) (( (op).cons  < CONS && !((op).reg2) ))
#define is_memb_addr(op) (( (op).cons >= CONS ||  ((op).reg2) ))
#define is_mem_val(op)   (( (op).reg1 >= MEMA  ))
#define is_mema_val(op)  (( (op).reg1 == MEMA  ))
#define is_memb_val(op)  (( (op).reg1 == MEMB  ))
#define is_reg_val(op)   (( (op).reg1 <= PREG && !(op).reg2 && !(op).cons ))
#define is_cons_val(op)  (( (op).reg1 == 0 && (op).reg2 == 0 ))
#define is_reg_lit(op)   (( is_reg_val(op) || (is_cons_val(op) && (op).cons < 32 )))
#define is_zero(op) (( (op).cons == 0 && is_cons_val(op) ))

static void
i960_load_op (p, reg)
op_model *p;
int reg;
{ /* Model a load of *p into reg, estimating the cost of the code. */

  int cost = p->cost;
  int wds  = GET_MODE_SIZE(p->mode) / SI_BYTES;

  if (wds == 0)
    wds = 1;

  assert (wds <= 4);

  if (is_reg_val (*p))
  { if (p->reg1 != reg)
      cost += SPLIT_BIG_MOVES ? wds : 1;
  }
  else if (is_mema_val (*p))
    cost += SPLIT_BIG_MOVES ? wds : 1;

  else if (is_memb_val (*p))
    cost += 2;

  else if (is_cons_val (*p))
    if (p->cons < CONS && wds == 1)
      cost += i960_estimate_const_int (p->mode, p->cons);
    else
      cost += 2 * ((wds==4) ? 3 : wds);

  else if (p->cons == 0)
    cost = 1;

  else
    cost += 1 + is_memb_addr (*p);

  *p = zero_op;
  p->cost = cost;
  p->reg1 = reg;
}

#ifndef NDX_RTX
#define NDX_RTX(R,I) (( *((rtx *) (((char *)(R)) + (I))) ))
#endif

#ifndef XEXP_OFFSET
#define XEXP_OFFSET(X,I) (( ((char*)&XEXP((X),(I))) - ((char*)(X)) ))
#endif

#ifndef REAL_INSN
#define REAL_INSN(I) \
  (GET_CODE(I)==INSN || GET_CODE(I)==JUMP_INSN || GET_CODE(I)==CALL_INSN)
#endif

static void
i960_size_est (p, off, res, reglit)
rtx p;
int off;
op_model *res;
{ /* Try to estimate the cost in words of a subtree.  Note that
     we have to deal with various flavors of insulting illegal
     rtl's.  The general approach is, we walk the tree and model
     what we would have to generate to fix it up.  When we generate
     code in the model, we add the cost into the operand and then
     pretend we loaded the operand into a register.  We bubble the costs
     up to the top to account for the fixup. */

  rtx r = NDX_RTX (p, off);

  enum rtx_code     c = GET_CODE(r);
  enum machine_mode m = GET_MODE(r);

  unsigned long u;
  op_model op[3];

  memset (op, 0, sizeof (op));
  memset (res, 0, sizeof (*res));

  switch (GET_RTX_CLASS(c))
  {
    case 'm':
    case 'i':
    default:
      assert (0);
      break;

    case 'x':
      switch (c)
      { case UNKNOWN:
        case NIL:
        case EXPR_LIST:
        case INSN_LIST:
        case DEFINE_INSN:
        case DEFINE_PEEPHOLE:
        case DEFINE_SPLIT:
        case DEFINE_COMBINE:
        case DEFINE_EXPAND:
        case DEFINE_DELAY:
        case DEFINE_FUNCTION_UNIT:
        case DEFINE_ASM_ATTRIBUTES:
        case SEQUENCE:
        case DEFINE_ATTR:
        case ATTR:
        case SET_ATTR:
        case SET_ATTR_ALTERNATIVE:
        case EQ_ATTR:
        case ATTR_FLAG:
        case BARRIER:
        case CODE_LABEL:
        case NOTE:
        case INLINE_HEADER:
        case GLOBAL_INLINE_HEADER:
        case UNSPEC:
        case UNSPEC_VOLATILE:
        case TRAP_IF:
        case STRICT_LOW_PART:
        case QUEUED:
        case COND:
        case PRE_DEC:
        case PRE_INC:
        case POST_DEC:
        case POST_INC:
          assert(0);
          break;

        case USE:
        case CLOBBER:
        case RETURN:
        case ASM_INPUT:
        case ASM_OPERANDS:
          break;
    
        case PARALLEL:
        { char* fmt = GET_RTX_FORMAT (c);
          int i;
    
          assert (!reglit);

          for (i=0; i < GET_RTX_LENGTH(c); i++)
          {
            assert (fmt[i] != 'e');
    
            if (fmt[i] == 'E')
            { int j = XVECLEN(r, i);
              while (--j >= 0)
              { rtx *rp = &XVECEXP(r,i,j);

                i960_size_est (rp, 0, &op[0], 0);
                assert (GET_CODE(*rp)==CALL|| is_zero(op[0]));
                res->cost += op[0].cost;
              }
            }
          }
          break;
        }
    
        case ADDR_VEC:
        case ADDR_DIFF_VEC:
          res->cost = XVECLEN(r,0);
          break;
    
        case CALL:
          i960_size_est (r, XEXP_OFFSET(r,0), &op[0], 0);
          res->reg1 = 1;
          res->cost = op[0].cost + 1 + 
		(is_memb_val(op[0]) && (TARGET_USE_CALLX || TARGET_CAVE));
          break;
    
        case SUBREG:
          c=GET_CODE(r=SUBREG_REG(r));
          goto mem_or_reg;

        case SET:
          if (GET_CODE(SET_DEST(r)) == PC)
            res->cost = 1;
          else
          { int wds  = GET_MODE_SIZE(GET_MODE(SET_DEST(r))) / SI_BYTES;

            if (wds == 0)
              wds = 1;

            i960_size_est (r, XEXP_OFFSET(r,0), &op[0], 0);
            i960_size_est (r, XEXP_OFFSET(r,1), &op[1], 0);
      
            if (is_mem_val(op[0]))
            { 
              if (op[1].cons == 0 && is_cons_val(op[1]))
                op[1].reg1 = PREG;
      
              if (!is_reg_val(op[1]))
                i960_load_op (&op[1], PREG);
      
              if (is_mema_val(op[0]))
                res->cost = SPLIT_BIG_MOVES ? wds : 1;
              else
                res->cost = 2;
            }
            else
            { 
              assert (is_reg_val(op[0]));
      
              if (is_reg_val(op[1]))
              { if (op[1].cost == 0)
                  op[1].cost = SPLIT_BIG_MOVES ? wds : 1;
              }
              else
                i960_load_op (&op[1], op[0].reg1);
            }
            res->cost += op[0].cost + op[1].cost;
          }
          m = VOIDmode;
          break;
      }
      break;

    case 'b':
      /* All we should be able to see here is chkbit.  Branch
         contexts are disallowed up at the SET. */
      assert (c == ZERO_EXTRACT);
      assert (GET_CODE(p) == COMPARE && off == XEXP_OFFSET(p,0));
      assert (GET_CODE(XEXP(r,1))==CONST_INT && INTVAL(XEXP(r,1))==1);
      i960_size_est (r, XEXP_OFFSET (r,0), &op[0], 1);
      i960_size_est (r, XEXP_OFFSET (r,2), &op[2], 1);
      res->cost = 1 + op[0].cost + op[2].cost;
      res->reg1 = PREG;
      break;

    case 'c':
      if (m!=SImode || (c!=PLUS && c!=MULT))
        goto binary;

      /* See if we can avoid generating code by filling address */

      i960_size_est (r, XEXP_OFFSET(r,0), &op[0], 0);
      i960_size_est (r, XEXP_OFFSET(r,1), &op[1], 0);
   
      if (is_mem_val(op[0]) || (op[0].reg2 && !is_cons_val(op[1])))
        i960_load_op (&op[0], PREG);
  
      if (is_mem_val(op[1]) || (op[1].reg2 && !is_cons_val(op[0])))
        i960_load_op (&op[1], PREG);
  
      if (op[1].reg2 || !op[0].reg1)
      { op[2] = op[1]; op[1] = op[0]; op[0] = op[2]; }
  
      assert (op[1].reg2 == 0);
      u = op[1].cons;
  
      if (c==PLUS)
      { /* Add the register part of op[1] into op[0] */
        if (op[1].reg1)
        { assert (op[0].reg1 != 0 && op[0].reg2 == 0);
          op[0].reg2 = op[1].reg1;
        }

        /* Add the constant part of op[1] into op[0]. */
        u += op[0].cons;
      }
      else
      { /* If we multiply no more than 1 register by a small scale,
           we represent the result by adjusting the constant (we
           just forget about the scal on the register - we don't
           have to be that exact).  Else, we just punt. */

        if (op[0].reg2 || !is_cons_val(op[1]) || (u & (u-1)) || u > 16)
          goto binary;

        u *= op[1].cons;
      }

      if (u > CONS)
        u = CONS;

      op[0].cons = u;
      op[0].cost += op[1].cost;
      *res = op[0];
      break;

    case '3':
      assert (c==IF_THEN_ELSE);

      /* We should see IF_THEN_ELSE only for JX sel insns, because we handle
         all branches by not descending into SETs whose dest is PC. */
    
      assert (GET_CODE(p) == SET && off == XEXP_OFFSET(p,1));
      assert (GET_CODE(SET_DEST(p)) != PC);
      i960_size_est (r, XEXP_OFFSET (r, 2), &op[2], 1);

    binary:
    case '2':
    case '<':
      i960_size_est (r, XEXP_OFFSET (r, 1), &op[1], 1);

    case  '1':
      i960_size_est (r, XEXP_OFFSET (r, 0), &op[0], 1);
      assert (off != 0);

      res->cost = op[0].cost + op[1].cost + op[2].cost;

      if (c==COMPARE || GET_RTX_CLASS(c) == '<')
      { res->reg1 = CCRG;

        /* No cost if comparing CC to constant */
        if (op[0].reg1 == CCRG || op[1].reg1 == CCRG)
          assert (res->cost == 0);
        else
          res->cost++;
      }
      else
      { assert (m != VOIDmode);

        res->reg1 = PREG;

        if (c!=SIGN_EXTEND && c!=ZERO_EXTEND)
          res->cost++;
      }
      break;

    case 'o':
      switch (c)
      {
        default:
        case HIGH:
        case CONST_STRING:
        case LO_SUM:
        case SCRATCH:
        case CONCAT:
        case CC0:
          assert (0);
          break;

        case CONST_INT:
          if (GET_CODE(p)==COMPARE)
            m = GET_MODE(XEXP(p,0));
          else
            m = GET_MODE(p);

          if (INTVAL(r) < CONS)
            res->cons = INTVAL(r);
          else
            res->cons = CONS;
          break;
    
        case CONST_DOUBLE:
          /* assert (m != VOIDmode); */
          /* Can't represent these.  Pretend we loaded them, remember cost */
          if (m == VOIDmode)
            m = DImode;
          res->reg1 = PREG;
          res->cost = i960_estimate_ldconst (m, r);
          break;
    
        case CONST:
        case LABEL_REF:
        case SYMBOL_REF:
          res->cons = CONS;
          m = SImode;
          break;
    
        case PC:
          res->reg1 = PCRG;
          break;
    
        case MEM:
        case REG:
        mem_or_reg:
          if (c==REG)
          {
            if (REGNO(r) >= FIRST_PSEUDO_REGISTER)
              res->reg1 = PREG;
            else
              if (REGNO(r) == CC_REGNUM)
                res->reg1 = CCRG;
              else
                res->reg1 = REGNO(r) + 1;
          }
          else
          { i960_size_est (r, XEXP_OFFSET(r,0), &op[0], 0);
            res->reg1 = MEMA + !is_mema_addr(op[0]);
            res->cost = op[0].cost;
          }
          break;
      }
  }
  res->mode = m;
  if (reglit && !is_reg_lit(*res))
    i960_load_op (res, PREG);
}

/* External entries for rtx code size estimation */
int
insn_size (insn)
rtx insn;
{
  enum rtx_code c = GET_CODE(insn);

  op_model op;
  op = zero_op;

  if (REAL_INSN(insn))
    i960_size_est (&PATTERN(insn), 0, &op, 0);

  return op.cost;
}

int
init_insn_size_est ()
{ /* Initialize ptr so print_rtl can call size calculator */
  extern int (*insn_size_est)();
  insn_size_est = insn_size;
}
#endif
